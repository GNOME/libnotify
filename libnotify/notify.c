/**
 * @file libnotify/notify.c Notifications library
 *
 * @Copyright (C) 2004 Christian Hammond
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
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <stdio.h>

#define NOTIFY_DBUS_SERVICE        "org.freedesktop.Notifications"
#define NOTIFY_DBUS_CORE_INTERFACE "org.freedesktop.Notifications"
#define NOTIFY_DBUS_CORE_OBJECT    "/org/freedesktop/Notifications"

static DBusConnection *_dbus_conn = NULL;
static gboolean _initted = FALSE;
static gboolean _filters_added = FALSE;
static guint32 _init_ref_count = 0;
static char *_app_name = NULL;

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
#endif

static void
_notify_dbus_message_iter_append_app_info(DBusMessageIter *iter)
{
	g_return_if_fail(iter != NULL);

	dbus_message_iter_append_string(iter, _app_name);
	dbus_message_iter_append_nil(iter); /* App Icon */
}

static DBusHandlerResult
_filter_func(DBusConnection *dbus_conn, DBusMessage *message, void *user_data)
{
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gboolean
_notify_connect(void)
{
	DBusError error;

	dbus_error_init(&error);

	_dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &error);

	if (_dbus_conn == NULL)
	{
		fprintf(stderr, "Error connecting to session bus: %s\n",
				error.message);
		dbus_error_free(&error);

		return FALSE;
	}

	dbus_connection_set_exit_on_disconnect(_dbus_conn, FALSE);

	if (!dbus_bus_activate_service(_dbus_conn, NOTIFY_DBUS_SERVICE, 0,
								   NULL, &error))
	{
		fprintf(stderr, "Error activating %s service: %s\n",
				NOTIFY_DBUS_SERVICE, error.message);

		dbus_error_free(&error);

		return FALSE;
	}

	if (!dbus_connection_add_filter(_dbus_conn, _filter_func, NULL, NULL))
	{
		fprintf(stderr, "Error creating D-BUS message filter.\n");

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
		fprintf(stderr, "Error subscribing to signals: %s\n", error.message);

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

	_notify_disconnect();
}

gboolean
notify_is_initted(void)
{
	return _initted;
}

void
notify_close_notification(guint32 id)
{
	DBusMessage *message;
	DBusMessageIter iter;

	g_return_if_fail(id > 0);

	message = _notify_dbus_message_new("CloseNotification", &iter);

	g_return_if_fail(message != NULL);

	dbus_message_iter_append_uint32(&iter, id);

	dbus_connection_send(_dbus_conn, message, NULL);
	dbus_message_unref(message);
}

void
notify_close_request(guint32 id)
{
	DBusMessage *message;
	DBusMessageIter iter;

	g_return_if_fail(id > 0);

	message = _notify_dbus_message_new("CloseRequest", &iter);

	g_return_if_fail(message != NULL);

	dbus_message_iter_append_uint32(&iter, id);

	dbus_connection_send(_dbus_conn, message, NULL);
	dbus_message_unref(message);
}

static guint32
_notify_send_notification(NotifyUrgency urgency, const char *summary,
						  const char *detailed, const char *icon_uri,
						  size_t icon_len, guchar *icon_data, time_t timeout)
{
	DBusMessage *message, *reply;
	DBusMessageIter iter;
	DBusError error;
	guint32 id;

	message = _notify_dbus_message_new("SendNotification", &iter);

	g_return_val_if_fail(message != NULL, 0);

	_notify_dbus_message_iter_append_app_info(&iter);
	dbus_message_iter_append_uint32(&iter, urgency);
	dbus_message_iter_append_string(&iter, summary);
	_notify_dbus_message_iter_append_string_or_nil(&iter, detailed);

	if (icon_len > 0 && icon_data != NULL)
		dbus_message_iter_append_byte_array(&iter, icon_data, icon_len);
	else
		_notify_dbus_message_iter_append_string_or_nil(&iter, icon_uri);

	dbus_message_iter_append_uint32(&iter, timeout);

	dbus_error_init(&error);

	reply = dbus_connection_send_with_reply_and_block(_dbus_conn, message,
													  -1, &error);

	dbus_message_unref(message);

	if (dbus_error_is_set(&error))
	{
		fprintf(stderr, "Error sending SendNotification: %s\n",
				error.message);
		dbus_error_free(&error);

		return 0;
	}

	dbus_message_iter_init(reply, &iter);
	id = dbus_message_iter_get_uint32(&iter);

	dbus_message_unref(reply);
	dbus_error_free(&error);

	return id;
}

guint32
notify_send_notification(NotifyUrgency urgency, const char *summary,
						 const char *detailed, const char *icon_uri,
						 time_t timeout)
{
	g_return_val_if_fail(summary != NULL, 0);

	return _notify_send_notification(urgency, summary, detailed, icon_uri,
									 0, NULL, timeout);
}

guint32
notify_send_notification_with_icon_data(NotifyUrgency urgency,
										const char *summary,
										const char *detailed,
										size_t icon_len, guchar *icon_data,
										time_t timeout)
{
	g_return_val_if_fail(summary != NULL, 0);

	return _notify_send_notification(urgency, summary, detailed, NULL,
									 icon_len, icon_data, timeout);
}

static guint32
_notify_send_request(NotifyUrgency urgency, const char *summary,
					 const char *detailed, const char *icon_uri,
					 size_t icon_len, guchar *icon_data, time_t timeout,
					 gpointer user_data, guint32 default_button,
					 size_t button_count, va_list buttons)
{
	DBusMessage *message, *reply;
	DBusMessageIter iter;
	DBusError error;
	guint32 id;
	guint32 i;
	char *text;
	NotifyCallback cb;

	message = _notify_dbus_message_new("SendRequest", &iter);

	g_return_val_if_fail(message != NULL, 0);

	_notify_dbus_message_iter_append_app_info(&iter);
	dbus_message_iter_append_uint32(&iter, urgency);
	dbus_message_iter_append_string(&iter, summary);
	_notify_dbus_message_iter_append_string_or_nil(&iter, detailed);

	if (icon_len > 0 && icon_data != NULL)
		dbus_message_iter_append_byte_array(&iter, icon_data, icon_len);
	else
		_notify_dbus_message_iter_append_string_or_nil(&iter, icon_uri);

	dbus_message_iter_append_uint32(&iter, timeout);
	dbus_message_iter_append_uint32(&iter, default_button);
	dbus_message_iter_append_uint32(&iter, button_count);

	for (i = 0; i < button_count; i++)
	{
		text = va_arg(buttons, char *);
		cb   = va_arg(buttons, NotifyCallback);

		dbus_message_iter_append_string(&iter, text);
	}

	dbus_error_init(&error);

	reply = dbus_connection_send_with_reply_and_block(_dbus_conn, message,
													  -1, &error);

	dbus_message_unref(message);

	if (dbus_error_is_set(&error))
	{
		fprintf(stderr, "Error sending SendNotification: %s\n",
				error.message);
		dbus_error_free(&error);

		return 0;
	}

	dbus_message_iter_init(reply, &iter);
	id = dbus_message_iter_get_uint32(&iter);

	dbus_message_unref(reply);
	dbus_error_free(&error);

	return id;
}

guint32
notify_send_request(NotifyUrgency urgency, const char *summary,
					const char *detailed, const char *icon_uri,
					time_t timeout, gpointer user_data,
					size_t default_button, size_t button_count, ...)
{
	va_list buttons;
	guint32 id;

	g_return_val_if_fail(summary != NULL,  0);
	g_return_val_if_fail(button_count > 1, 0);

	va_start(buttons, button_count);
	id = notify_send_request_varg(urgency, summary, detailed, icon_uri,
								  timeout, user_data, default_button,
								  button_count, buttons);
	va_end(buttons);

	return id;
}

guint32
notify_send_request_varg(NotifyUrgency urgency, const char *summary,
						 const char *detailed, const char *icon_uri,
						 time_t timeout, gpointer user_data,
						 size_t default_button, size_t button_count,
						 va_list buttons)
{
	g_return_val_if_fail(summary != NULL,  0);
	g_return_val_if_fail(button_count > 1, 0);

	return _notify_send_request(urgency, summary, detailed, icon_uri,
								0, NULL, timeout, user_data, default_button,
								button_count, buttons);
}

guint32
notify_send_request_with_icon_data(NotifyUrgency urgency,
								   const char *summary, const char *detailed,
								   size_t icon_len, guchar *icon_data,
								   time_t timeout, gpointer user_data,
								   gint32 default_button, size_t button_count,
								   ...)
{
	va_list buttons;
	guint32 id;

	g_return_val_if_fail(summary != NULL,  0);
	g_return_val_if_fail(button_count > 1, 0);

	va_start(buttons, button_count);
	id = notify_send_request_with_icon_data_varg(urgency, summary, detailed,
												 icon_len, icon_data, timeout,
												 user_data, default_button,
												 button_count, buttons);
	va_end(buttons);

	return id;
}

guint32
notify_send_request_with_icon_data_varg(NotifyUrgency urgency,
										const char *summary,
										const char *detailed,
										size_t icon_len, guchar *icon_data,
										time_t timeout, gpointer user_data,
										gint32 default_button,
										size_t button_count, va_list buttons)
{
	g_return_val_if_fail(summary != NULL,  0);
	g_return_val_if_fail(button_count > 1, 0);

	return _notify_send_request(urgency, summary, detailed, NULL, icon_len,
								icon_data, timeout, user_data, default_button,
								button_count, buttons);
}
