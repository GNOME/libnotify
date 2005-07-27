/* -*- mode: c-mode; tab-width: 4; indent-tabs-mode: t; -*- */
/**
 * @file libnotify/notify.c Notifications library
 *
 * @Copyright (C) 2004 Christian Hammond <chipx86@chipx86.com>
 * @Copyright (C) 2004 Mike Hearn <mike@navi.cx>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef DBUS_API_SUBJECT_TO_CHANGE
# define DBUS_API_SUBJECT_TO_CHANGE 1
#endif

#include "notify.h"
#include "dbus-compat.h"
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#define NOTIFY_DBUS_SERVICE        "org.freedesktop.Notifications"
#define NOTIFY_DBUS_CORE_INTERFACE "org.freedesktop.Notifications"
#define NOTIFY_DBUS_CORE_OBJECT    "/org/freedesktop/Notifications"

struct _NotifyHandle
{
	guint32 id;

	guint32 replaces;

	gpointer user_data;

	guint32 action_count;
	GHashTable *actions_table;
};

struct _NotifyIcon
{
	int frames;

	char **uri;

	size_t *raw_len;
	guchar **raw_data;
};

typedef struct
{
	guint32 id;
	char *text;
	NotifyCallback cb;

} NotifyAction;

static DBusConnection *_dbus_conn = NULL;
static gboolean _initted = FALSE;
static gboolean _filters_added = FALSE;
static guint32 _init_ref_count = 0;
static char *_app_name = NULL;
static GHashTable *_handles = NULL;

#ifdef __GNUC__
#  define format_func __attribute__((format(printf, 1, 2)))
#else /* no format string checking with this compiler */
#  define format_func
#endif

static void format_func
print_error(char *message, ...)
{
	char buf[1024];
	va_list args;

	va_start(args, message);
	vsnprintf(buf, sizeof(buf), message, args);
	va_end(args);

	fprintf(stderr, "%s(%d): libnotify: %s",
			(getenv("_") ? getenv("_") : ""), getpid(), buf);
}

static NotifyHandle *
_notify_handle_new(guint32 id)
{
	NotifyHandle *handle;

	handle = g_new0(NotifyHandle, 1);

	handle->id = id;

	handle->replaces = 0;

	g_hash_table_insert(_handles, &handle->id, handle);

	return handle;
}

static void
_notify_handle_destroy(NotifyHandle *handle)
{
	g_return_if_fail(handle != NULL);

	if (handle->actions_table != NULL)
		g_hash_table_destroy(handle->actions_table);

	g_free(handle);
}

static void
_notify_action_destroy(NotifyAction *action)
{
	g_return_if_fail(action != NULL);

	if (action->text != NULL)
		g_free(action->text);

	g_free(action);
}

static DBusMessage *
_notify_dbus_message_new(const char *name, DBusMessageIter *iter)
{
	DBusMessage *message;

	g_return_val_if_fail(name  != NULL, NULL);
	g_return_val_if_fail(*name != '\0', NULL);

	message = dbus_message_new_method_call(NOTIFY_DBUS_SERVICE,
										   NOTIFY_DBUS_CORE_OBJECT,
										   NOTIFY_DBUS_CORE_INTERFACE,
										   name);

	g_return_val_if_fail(message != NULL, NULL);

	if (iter != NULL)
		dbus_message_iter_init_append(message, iter);

	return message;
}

static void
_notify_dbus_message_iter_append_string_or_empty(DBusMessageIter *iter,
												 const char *str)
{
	g_return_if_fail(iter != NULL);

	if (str == NULL)
		str = "";

	_notify_dbus_message_iter_append_string(iter, str);
}

static DBusHandlerResult
_filter_func(DBusConnection *dbus_conn, DBusMessage *message, void *user_data)
{
	DBusMessageIter iter;

	if (dbus_message_is_signal(message, NOTIFY_DBUS_CORE_INTERFACE,
							   "NotificationClosed"))
	{
		guint32 id, reason;

		dbus_message_iter_init(message, &iter);

		_notify_dbus_message_iter_get_uint32(&iter, id);
		dbus_message_iter_next(&iter);

		_notify_dbus_message_iter_get_uint32(&iter, reason);

		g_hash_table_remove(_handles, &id);
	}
	else if (dbus_message_is_signal(message, NOTIFY_DBUS_CORE_INTERFACE,
									"ActionInvoked"))
	{
		guint32 id, action_id;
		NotifyHandle *handle;

		dbus_message_iter_init(message, &iter);

		_notify_dbus_message_iter_get_uint32(&iter, id);
		dbus_message_iter_next(&iter);

		_notify_dbus_message_iter_get_uint32(&iter, action_id);

		handle = g_hash_table_lookup(_handles, &id);

		if (handle == NULL)
			goto exit;

		if (handle->actions_table == NULL)
		{
			print_error("An action (%d) was invoked for a notification (%d) "
						"with no actions listed!\n",
						action_id, id);
		}
		else
		{
			NotifyAction *action;

			action = g_hash_table_lookup(handle->actions_table, &action_id);

			if (action == NULL)
			{
				print_error("An invalid action (%d) was invoked for "
							"notification %d\n", action_id, id);
			}
			else if (action->cb != NULL)
			{
				action->cb(handle, action_id, handle->user_data);
			}
		}
	}
	else
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

exit:
	return DBUS_HANDLER_RESULT_HANDLED;
}

static gboolean
_notify_connect(void)
{
	DBusError error;

	dbus_error_init(&error);

	_dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &error);

	if (_dbus_conn == NULL)
	{
		print_error("Error connecting to session bus: %s\n", error.message);

		dbus_error_free(&error);

		return FALSE;
	}

	dbus_connection_set_exit_on_disconnect(_dbus_conn, FALSE);

	if (!dbus_bus_start_service_by_name(_dbus_conn, NOTIFY_DBUS_SERVICE,
										0, NULL, &error))
	{
		print_error("Error activating %s service: %s\n",
					NOTIFY_DBUS_SERVICE, error.message);

		dbus_error_free(&error);

		return FALSE;
	}

	if (!dbus_connection_add_filter(_dbus_conn, _filter_func, NULL, NULL))
	{
		print_error("Error creating D-BUS message filter.\n");

		dbus_error_free(&error);

		return FALSE;
	}

	dbus_bus_add_match(_dbus_conn,
					   "type=signal,"
					   "interface=" DBUS_INTERFACE_DBUS ","
					   "sender=" DBUS_SERVICE_DBUS ,
					   &error);

	dbus_bus_add_match(_dbus_conn,
					   "type=signal,"
					   "interface=" NOTIFY_DBUS_CORE_INTERFACE ","
					   "path=" NOTIFY_DBUS_CORE_OBJECT ,
					   &error);

	if (dbus_error_is_set(&error))
	{
		print_error("Error subscribing to signals: %s\n", error.message);

		dbus_error_free(&error);

		return FALSE;
	}

	_filters_added = TRUE;

	dbus_error_free(&error);

	return TRUE;
}

static void
_notify_disconnect(void)
{
	if (_dbus_conn == NULL)
		return;

	if (_filters_added)
	{
		dbus_connection_remove_filter(_dbus_conn, _filter_func, NULL);

		_filters_added = FALSE;
	}

	dbus_connection_disconnect(_dbus_conn);
	dbus_connection_unref(_dbus_conn);

	_dbus_conn = NULL;
}

gboolean
notify_init(const char *app_name)
{
	g_return_val_if_fail(app_name  != NULL, FALSE);
	g_return_val_if_fail(*app_name != '\0', FALSE);

	_init_ref_count++;

	if (_initted)
		return TRUE;

	if (!_notify_connect())
	{
		_notify_disconnect();

		return FALSE;
	}

	_app_name = g_strdup(app_name);

	_handles = g_hash_table_new_full(g_int_hash, g_int_equal,
									 NULL, (GFreeFunc)_notify_handle_destroy);

#ifdef HAVE_ATEXIT
	atexit(notify_uninit);
#endif /* HAVE_ATEXIT */

	_initted = TRUE;

	return TRUE;
}

gboolean
notify_glib_init(const char *app_name, GMainContext *context)
{
	if (!notify_init(app_name))
		return FALSE;

	notify_setup_with_g_main(context);

	return TRUE;
}

void
notify_uninit(void)
{

	_init_ref_count--;

	if (_init_ref_count != 0)
		return;

	if (_app_name != NULL)
	{
		g_free(_app_name);
		_app_name = NULL;
	}

	if (_handles != NULL)
	{
		g_hash_table_destroy(_handles);
		_handles = NULL;
	}

	_notify_disconnect();
}

gboolean
notify_is_initted(void)
{
	return _initted;
}

void
notify_setup_with_g_main(GMainContext *context)
{
	dbus_connection_setup_with_g_main(_dbus_conn, context);
}

void
notify_close(NotifyHandle *handle)
{
	DBusMessage *message;
	DBusMessageIter iter;

	g_return_if_fail(handle != NULL);

	/* Don't close other applications' notifications! */
	g_return_if_fail(g_hash_table_lookup(_handles, &handle->id) != NULL);

	message = _notify_dbus_message_new("CloseNotification", &iter);

	g_return_if_fail(message != NULL);

	_notify_dbus_message_iter_append_uint32(&iter, handle->id);

	dbus_connection_send_with_reply_and_block(_dbus_conn, message, -1, NULL);
	dbus_message_unref(message);
}

gboolean
notify_get_server_info(char **ret_name, char **ret_vendor, char **ret_version)
{
	DBusMessage *message, *reply;
	DBusMessageIter iter;
	DBusError error;
	char *name, *vendor, *version;

	message = _notify_dbus_message_new("GetServerInformation", NULL);

	g_return_val_if_fail(message != NULL, FALSE);

	dbus_error_init(&error);

	reply = dbus_connection_send_with_reply_and_block(_dbus_conn, message,
													  -1, &error);

	dbus_message_unref(message);

	if (dbus_error_is_set(&error))
	{
		print_error("Error sending GetServerInformation: %s\n", error.message);

		dbus_error_free(&error);

		return FALSE;
	}

	dbus_error_free(&error);

	dbus_message_iter_init(reply, &iter);

	_notify_dbus_message_iter_get_string(&iter, name);
	dbus_message_iter_next(&iter);

	_notify_dbus_message_iter_get_string(&iter, vendor);
	dbus_message_iter_next(&iter);

	_notify_dbus_message_iter_get_string(&iter, version);
	dbus_message_iter_next(&iter);

	dbus_message_unref(reply);

	if (ret_name != NULL)
		*ret_name = g_strdup(name);

	if (ret_vendor != NULL)
		*ret_vendor = g_strdup(vendor);

	if (ret_version != NULL)
		*ret_version = g_strdup(version);

#if !NOTIFY_CHECK_DBUS_VERSION(0, 30)
	dbus_free(name);
	dbus_free(vendor);
	dbus_free(version);
#endif

	return TRUE;
}

GList *
notify_get_server_caps(void)
{
	DBusMessage *message, *reply;
	DBusMessageIter iter;
	DBusError error;
	GList *caps = NULL;
#if NOTIFY_CHECK_DBUS_VERSION(0, 30)
	DBusMessageIter array_iter;
#else
	char **temp_array;
	int num_items, i;
#endif

	message = _notify_dbus_message_new("GetCapabilities", NULL);

	g_return_val_if_fail(message != NULL, FALSE);

	dbus_error_init(&error);

	reply = dbus_connection_send_with_reply_and_block(_dbus_conn, message,
													  -1, &error);

	dbus_message_unref(message);

	if (dbus_error_is_set(&error))
	{
		print_error("Error sending GetCapabilities: %s\n", error.message);

		dbus_error_free(&error);

		return FALSE;
	}

	dbus_error_free(&error);

	dbus_message_iter_init(reply, &iter);

#if NOTIFY_CHECK_DBUS_VERSION(0, 30)
	dbus_message_iter_recurse(&iter, &array_iter);

	while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRING)
	{
		const char *value;

		dbus_message_iter_get_basic(&array_iter, &value);

		caps = g_list_append(caps, g_strdup(value));

		dbus_message_iter_next(&array_iter);
	}
#else /* D-BUS < 0.30 */
	dbus_message_iter_get_string_array(&iter, &temp_array, &num_items);

	for (i = 0; i < num_items; i++)
		caps = g_list_append(caps, g_strdup(temp_array[i]));

	dbus_free_string_array(temp_array);
#endif /* D-BUS < 0.30 */

	dbus_message_unref(reply);

	return caps;
}


/**************************************************************************
 * Notify Hints API
 **************************************************************************/

NotifyHints *
notify_hints_new(void)
{
	return g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

void
notify_hints_set_string(NotifyHints *hints, const char *key,
						const char *value)
{
	g_return_if_fail(hints != NULL);
	g_return_if_fail(key != NULL && *key != '\0');
	g_return_if_fail(value != NULL && *value != '\0');

	g_hash_table_replace(hints, g_strdup(key), g_strdup(value));
}

void
notify_hints_set_int(NotifyHints *hints, const char *key, int value)
{
	g_return_if_fail(hints != NULL);
	g_return_if_fail(key != NULL && *key != '\0');

	g_hash_table_replace(hints, g_strdup(key), g_strdup_printf("%d", value));
}


/**************************************************************************
 * Icon API
 **************************************************************************/

NotifyIcon *
notify_icon_new()
{
	NotifyIcon *icon;

	icon = g_new0(NotifyIcon, 1);

	return icon;
}

NotifyIcon *
notify_icon_new_from_uri(const char *icon_uri)
{
	NotifyIcon *icon;

	g_return_val_if_fail(icon_uri  != NULL, NULL);
	g_return_val_if_fail(*icon_uri != '\0', NULL);

	icon = g_new0(NotifyIcon, 1);

	icon->frames = 1;
	icon->uri    = g_malloc(sizeof(char *));
	icon->uri[0] = g_strdup(icon_uri);

	return icon;
}

NotifyIcon *
notify_icon_new_from_data(size_t icon_len, const guchar *icon_data)
{
	NotifyIcon *icon;

	g_return_val_if_fail(icon_len  >  0,    NULL);
	g_return_val_if_fail(icon_data != NULL, NULL);

	icon = g_new0(NotifyIcon, 1);

	icon->frames      = 1;
	icon->raw_len     = g_malloc(sizeof(icon->raw_len));
	icon->raw_len[0]  = icon_len;
	icon->raw_data    = g_malloc(sizeof(guchar *));
	icon->raw_data[0] = g_memdup(icon_data, icon_len);

	return icon;
}

gboolean
notify_icon_add_frame_from_data(NotifyIcon *icon, size_t icon_len, const guchar *icon_data)
{
	g_return_val_if_fail(icon != NULL, FALSE);
	g_return_val_if_fail(icon_data != NULL, FALSE);
	g_return_val_if_fail(icon_len != 0, FALSE);

	if (icon->frames) g_return_val_if_fail(icon->raw_len != NULL, FALSE);

	icon->frames++;
	icon->raw_len = g_realloc(icon->raw_len, sizeof(size_t) * icon->frames);
	icon->raw_len[icon->frames - 1] = icon_len;
	icon->raw_data = g_realloc(icon->raw_data, sizeof(guchar *) * icon->frames);
	icon->raw_data[icon->frames - 1] = g_memdup(icon_data, icon_len);

	return TRUE;
}

gboolean
notify_icon_add_frame_from_uri(NotifyIcon *icon, const char *uri)
{
	g_return_val_if_fail(icon != NULL, FALSE);
	g_return_val_if_fail(uri  != NULL, FALSE);

	if (icon->frames)
	{
		g_return_val_if_fail(icon->uri != NULL, FALSE);
	}

	icon->frames++;
	icon->uri = g_realloc(icon->uri, sizeof(char *) * icon->frames);
	icon->uri[icon->frames - 1] = g_strdup(uri);

	return TRUE;
}

void
notify_icon_destroy(NotifyIcon *icon)
{
	g_return_if_fail(icon != NULL);

	if (icon->uri != NULL)
		g_free(icon->uri);

	if (icon->raw_data != NULL)
		g_free(icon->raw_data);

	g_free(icon);
}

/**************************************************************************
 * Notifications API
 **************************************************************************/
NotifyHandle *
notify_send_notification(NotifyHandle *replaces, const char *type,
						 NotifyUrgency urgency, const char *summary,
						 const char *body, const NotifyIcon *icon,
						 gboolean expires, time_t timeout,
						 GHashTable *hints, gpointer user_data,
						 size_t action_count, ...)
{
	va_list actions;
	NotifyHandle *handle;

	g_return_val_if_fail(summary != NULL, 0);

	va_start(actions, action_count);
	handle = notify_send_notification_varg(replaces, type, urgency, summary,
										   body, icon, expires,
										   timeout, hints, user_data,
										   action_count, actions);
	va_end(actions);

	return handle;
}

static void
hint_foreach_func(const gchar *key, const gchar *value, DBusMessageIter *iter)
{
#if NOTIFY_CHECK_DBUS_VERSION(0, 30)
	DBusMessageIter entry_iter;

	dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY, NULL,
									 &entry_iter);
	dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key);
	dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &value);
	dbus_message_iter_close_container(iter, &entry_iter);
#else
	dbus_message_iter_append_dict_key(iter, key);
	dbus_message_iter_append_string(iter, value);
#endif
}

NotifyHandle *
notify_send_notification_varg(NotifyHandle *replaces, const char *type,
							  NotifyUrgency urgency, const char *summary,
							  const char *body, const NotifyIcon *icon,
							  gboolean expires, time_t timeout,
							  GHashTable *hints, gpointer user_data,
							  size_t action_count, va_list actions)
{
	DBusMessage *message, *reply;
	DBusMessageIter iter, array_iter, dict_iter;
	DBusError error;
	GHashTable *table;
	guint32 id;
	guint32 i;
	guint32 replaces_id;
	guint32 timeout_time;
	NotifyHandle *handle;

	g_return_val_if_fail(notify_is_initted(), NULL);

	message = _notify_dbus_message_new("Notify", &iter);

	g_return_val_if_fail(message != NULL, NULL);

	replaces_id = (replaces != NULL ? replaces->id : 0);

	_notify_dbus_message_iter_append_string_or_empty(&iter, _app_name);
	_notify_dbus_message_iter_append_string_or_empty(&iter, NULL);
	_notify_dbus_message_iter_append_uint32(&iter, replaces_id);
	_notify_dbus_message_iter_append_string_or_empty(&iter, type);
	_notify_dbus_message_iter_append_byte(&iter, urgency);
	_notify_dbus_message_iter_append_string(&iter, summary);
	_notify_dbus_message_iter_append_string_or_empty(&iter, body);

	if (icon == NULL)
	{
		_notify_dbus_message_iter_append_string_or_empty(&iter, NULL);
	}
	else if (icon->raw_data)
	{
		int i;

#if NOTIFY_CHECK_DBUS_VERSION(0, 30)
		dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
										 DBUS_TYPE_ARRAY_AS_STRING,
										 &array_iter);
#else
		dbus_message_iter_append_array(&iter, &array_iter, DBUS_TYPE_ARRAY);
#endif

		for (i = 0; i < icon->frames; i++)
		{
			_notify_dbus_message_iter_append_byte_array(&array_iter,
														icon->raw_data[i],
														icon->raw_len[i]);
		}

		dbus_message_iter_close_container(&iter, &array_iter);
	}
	else
	{
		int i;

		g_assert(icon->uri != NULL); /* can be either raw data OR uri */

#if NOTIFY_CHECK_DBUS_VERSION(0, 30)
		dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
										 DBUS_TYPE_STRING_AS_STRING,
										 &array_iter);
#else
		dbus_message_iter_append_array(&iter, &array_iter, DBUS_TYPE_STRING);
#endif

		for (i = 0; i < icon->frames; i++)
		{
			_notify_dbus_message_iter_append_string(&array_iter,
													icon->uri[i]);
		}

		dbus_message_iter_close_container(&iter, &array_iter);
	}

	/* Actions */
#if NOTIFY_CHECK_DBUS_VERSION(0, 30)
	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
									 DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
									 DBUS_TYPE_STRING_AS_STRING
									 DBUS_TYPE_UINT32_AS_STRING
									 DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
									 &dict_iter);
#else
	dbus_message_iter_append_dict(&iter, &dict_iter);
#endif

	table = g_hash_table_new_full(g_int_hash, g_int_equal, NULL,
								  (GFreeFunc)_notify_action_destroy);

	for (i = 0; i < action_count; i++)
	{
		NotifyAction *action;
#if NOTIFY_CHECK_DBUS_VERSION(0, 30)
		DBusMessageIter entry_iter;
#endif

		action = g_new0(NotifyAction, 1);

		action->id   = va_arg(actions, guint32);
		action->text = g_strdup((va_arg(actions, char *)));
		action->cb   = va_arg(actions, NotifyCallback);

#if NOTIFY_CHECK_DBUS_VERSION(0, 30)
		dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY,
										 NULL, &entry_iter);
		dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING,
									   &action->text);
		dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_UINT32,
									   &action->id);
		dbus_message_iter_close_container(&dict_iter, &entry_iter);
#else
		dbus_message_iter_append_dict_key(&dict_iter, action->text);
		dbus_message_iter_append_uint32(&dict_iter, action->id);
#endif

		g_hash_table_insert(table, &action->id, action);
	}

	dbus_message_iter_close_container(&iter, &dict_iter);

	/* Hints */
#if NOTIFY_CHECK_DBUS_VERSION(0, 30)
	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
									 DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
									 DBUS_TYPE_STRING_AS_STRING
									 DBUS_TYPE_STRING_AS_STRING
									 DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
									 &dict_iter);
#else
	dbus_message_iter_append_dict(&iter, &dict_iter);
#endif

	if (hints != NULL)
	{
		g_hash_table_foreach(hints, (GHFunc)hint_foreach_func, &dict_iter);
	}

#if NOTIFY_CHECK_DBUS_VERSION(0, 30)
	dbus_message_iter_close_container(&iter, &dict_iter);
#endif

	/* Expires */
	_notify_dbus_message_iter_append_boolean(&iter, expires);

	/* Expire Timeout */
	timeout_time = (expires ? timeout : 0);
	_notify_dbus_message_iter_append_uint32(&iter, timeout_time);

	dbus_error_init(&error);

	reply = dbus_connection_send_with_reply_and_block(_dbus_conn, message,
													  -1, &error);

	dbus_message_unref(message);

	if (dbus_error_is_set(&error))
	{
		print_error("error sending notification: %s\n", error.message);

		dbus_error_free(&error);

		g_hash_table_destroy(table);

		return 0;
	}

	dbus_message_iter_init(reply, &iter);
	_notify_dbus_message_iter_get_uint32(&iter, id);

	dbus_message_unref(reply);
	dbus_error_free(&error);

	if (hints != NULL)
		g_hash_table_destroy(hints);

	handle = _notify_handle_new(id);
	handle->actions_table = table;
	handle->action_count  = action_count;
	handle->user_data = user_data;

	return handle;
}
