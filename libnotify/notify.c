/** -*- mode: c-mode; tab-width: 4; indent-tabs-mode: t; -*-
 *
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

/* FIXME: do we want so many arguments in the API? */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef DBUS_API_SUBJECT_TO_CHANGE
# define DBUS_API_SUBJECT_TO_CHANGE 1
#endif

#include "notify.h"
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
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
	char *uri;

	size_t raw_len;
	guchar *raw_data;
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
		dbus_message_iter_init(message, iter);

	return message;
}

static void
_notify_dbus_message_iter_append_string_or_nil(DBusMessageIter *iter,
											   const char *str)
{
	g_return_if_fail(iter != NULL);

	if (str == NULL)
		dbus_message_iter_append_nil(iter);
	else
		dbus_message_iter_append_string(iter, str);
}

#if 0
static char *
_notify_dbus_message_iter_get_string_or_nil(DBusMessageIter *iter)
{
	int arg_type;

	g_return_val_if_fail(iter != NULL, NULL);

	arg_type = dbus_message_iter_get_arg_type(iter);

	if (arg_type == DBUS_TYPE_STRING)
		return dbus_message_iter_get_string(iter);

	g_return_val_if_fail(arg_type == DBUS_TYPE_NIL, NULL);

	return NULL;
}

static void
_notify_dbus_message_iter_append_app_info(DBusMessageIter *iter)
{
	g_return_if_fail(iter != NULL);

	dbus_message_iter_append_string(iter, _app_name);
	dbus_message_iter_append_nil(iter); /* App Icon */
}
#endif

static DBusHandlerResult
_filter_func(DBusConnection *dbus_conn, DBusMessage *message, void *user_data)
{
	DBusMessageIter iter;

	if (dbus_message_is_signal(message, NOTIFY_DBUS_CORE_INTERFACE,
							   "NotificationClosed"))
	{
		guint32 id, reason;

		dbus_message_iter_init(message, &iter);

		id = dbus_message_iter_get_uint32(&iter);
		dbus_message_iter_next(&iter);

		reason = dbus_message_iter_get_uint32(&iter);

		g_hash_table_remove(_handles, &id);
	}
	else if (dbus_message_is_signal(message, NOTIFY_DBUS_CORE_INTERFACE,
									"ActionInvoked"))
	{
		guint32 id, action_id;
		NotifyHandle *handle;

		dbus_message_iter_init(message, &iter);

		id = dbus_message_iter_get_uint32(&iter);
		dbus_message_iter_next(&iter);

		action_id = dbus_message_iter_get_uint32(&iter);

		handle = g_hash_table_lookup(_handles, &id);

		if (handle->actions_table == NULL)
		{
			print_error("An action (%d) was invoked for a notification (%d) "
						"with no actions listed!\n",
						action_id, id);
		}
		else
		{
			NotifyAction *action;

			action = g_hash_table_lookup(handle->actions_table,
										 &action_id);

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

	if (!dbus_bus_activate_service(_dbus_conn, NOTIFY_DBUS_SERVICE, 0, NULL,
								   &error))
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
					   "type='signal',"
					   "interface='" DBUS_INTERFACE_ORG_FREEDESKTOP_DBUS "',"
					   "sender='" DBUS_SERVICE_ORG_FREEDESKTOP_DBUS "'",
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

	dbus_connection_unref(_dbus_conn);
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

	_handles = g_hash_table_new_full(g_direct_hash, g_int_equal,
									 NULL, (GFreeFunc)_notify_handle_destroy);

#ifdef HAVE_ATEXIT
	atexit(notify_uninit);
#endif /* HAVE_ATEXIT */

	_initted = TRUE;

	return TRUE;
}

void
notify_uninit(void)
{
	g_return_if_fail(notify_is_initted());

	_init_ref_count--;

	if (_init_ref_count > 0)
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
notify_close(NotifyHandle *handle)
{
	DBusMessage *message;
	DBusMessageIter iter;

	g_return_if_fail(handle != NULL);

	message = _notify_dbus_message_new("CloseNotification", &iter);

	g_return_if_fail(message != NULL);

	dbus_message_iter_append_uint32(&iter, handle->id);

	dbus_connection_send(_dbus_conn, message, NULL);
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

	name = dbus_message_iter_get_string(&iter);
	dbus_message_iter_next(&iter);

	vendor = dbus_message_iter_get_string(&iter);
	dbus_message_iter_next(&iter);

	version = dbus_message_iter_get_string(&iter);
	dbus_message_iter_next(&iter);

	dbus_message_unref(reply);

	if (ret_name != NULL)
		*ret_name = g_strdup(name);

	if (ret_vendor != NULL)
		*ret_vendor = g_strdup(vendor);

	if (ret_version != NULL)
		*ret_version = g_strdup(version);

	dbus_free(name);
	dbus_free(vendor);
	dbus_free(version);

	return TRUE;
}

GList *
notify_get_server_caps(void)
{
	DBusMessage *message, *reply;
	DBusMessageIter iter;
	DBusError error;
	GList *caps = NULL;
	char **temp_array;
	int num_items, i;

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

	if (!dbus_message_iter_get_string_array(&iter, &temp_array, &num_items))
		return NULL;

	dbus_message_unref(reply);

	for (i = 0; i < num_items; i++)
		caps = g_list_append(caps, g_strdup(temp_array[i]));

	dbus_free_string_array(temp_array);

	return caps;
}

/**************************************************************************
 * Icon API
 **************************************************************************/
NotifyIcon *
notify_icon_new(const char *icon_uri)
{
	NotifyIcon *icon;

	g_return_val_if_fail(icon_uri  != NULL, NULL);
	g_return_val_if_fail(*icon_uri != '\0', NULL);

	icon = g_new0(NotifyIcon, 1);

	icon->uri = g_strdup(icon_uri);

	return icon;
}

NotifyIcon *
notify_icon_new_with_data(size_t icon_len, const guchar *icon_data)
{
	NotifyIcon *icon;

	g_return_val_if_fail(icon_len  >  0,    NULL);
	g_return_val_if_fail(icon_data != NULL, NULL);

	icon = g_new0(NotifyIcon, 1);

	icon->raw_len  = icon_len;
	icon->raw_data = g_memdup(icon_data, icon_len);

	return icon;
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
notify_send_notification(NotifyHandle *replaces, NotifyUrgency urgency, const char *summary,
						 const char *detailed, const NotifyIcon *icon,
						 gboolean expires, time_t expire_time,
						 gpointer user_data, size_t action_count, ...)
{
	va_list actions;
	NotifyHandle *handle;

	g_return_val_if_fail(summary != NULL, 0);

	va_start(actions, action_count);
	handle = notify_send_notification_varg(replaces, urgency, summary, detailed, icon,
										   expires, expire_time, user_data,
										   action_count, actions);
	va_end(actions);

	return handle;
}

NotifyHandle *
notify_send_notification_varg(NotifyHandle *replaces, NotifyUrgency urgency, const char *summary,
							  const char *detailed, const NotifyIcon *icon,
							  gboolean expires, time_t expire_time,
							  gpointer user_data, size_t action_count,
							  va_list actions)
{
	DBusMessage *message, *reply;
	DBusMessageIter iter, array_iter, dict_iter;
	DBusError error;
	GHashTable *table;
	guint32 id;
	guint32 i;
	NotifyHandle *handle;

	message = _notify_dbus_message_new("Notify", &iter);

	g_return_val_if_fail(message != NULL, 0);

#if 0
	_notify_dbus_message_iter_append_app_info(&iter);
#endif

	dbus_message_iter_append_uint32(&iter, replaces ? replaces->id : 0);
	dbus_message_iter_append_byte(&iter, urgency);
	dbus_message_iter_append_string(&iter, summary);
	_notify_dbus_message_iter_append_string_or_nil(&iter, detailed);

	/*
	 * NOTE: D-BUS 0.22cvs is the first to allow empty arrays, *I think*.
	 *       For now, allow a NIL.
	 */
	if (icon == NULL)
		dbus_message_iter_append_nil(&iter);
	else if (icon->raw_len > 0 && icon->raw_data != NULL)
	{
		dbus_message_iter_append_array(&iter, &array_iter, DBUS_TYPE_ARRAY);
		dbus_message_iter_append_byte_array(&array_iter, icon->raw_data,
											icon->raw_len);
	}
	else
	{
		dbus_message_iter_append_array(&iter, &array_iter, DBUS_TYPE_STRING);
		dbus_message_iter_append_string(&array_iter, icon->uri);
	}

	dbus_message_iter_append_nil(&iter); /* Sound */

	/* Actions */
	dbus_message_iter_append_dict(&iter, &dict_iter);

	table = g_hash_table_new_full(g_int_hash, g_int_equal, NULL,
								  (GFreeFunc)_notify_action_destroy);

	for (i = 0; i < action_count; i++)
	{
		NotifyAction *action;

		action = g_new0(NotifyAction, 1);

		action->id   = va_arg(actions, guint32);
		action->text = g_strdup((va_arg(actions, char *)));
		action->cb   = va_arg(actions, NotifyCallback);

		dbus_message_iter_append_dict_key(&dict_iter, action->text);
		dbus_message_iter_append_uint32(&dict_iter, action->id);

		g_hash_table_insert(table, GINT_TO_POINTER(action->id), action);
	}

	if (expires)
		dbus_message_iter_append_uint32(&iter, expire_time);
	else
		dbus_message_iter_append_nil(&iter);

	dbus_error_init(&error);

	reply = dbus_connection_send_with_reply_and_block(_dbus_conn, message,
													  -1, &error);

	dbus_message_unref(message);

	if (dbus_error_is_set(&error))
	{
		print_error("Error sending Notify: %s\n", error.message);

		dbus_error_free(&error);

		g_hash_table_destroy(table);

		return 0;
	}

	dbus_message_iter_init(reply, &iter);
	id = dbus_message_iter_get_uint32(&iter);

	dbus_message_unref(reply);
	dbus_error_free(&error);

	handle = _notify_handle_new(id);
	handle->actions_table = table;
	handle->action_count  = action_count;

	return handle;
}
