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
#define DBUS_API_SUBJECT_TO_CHANGE 1

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
}

void
notify_close_request(guint32 id)
{
}

guint32
notify_send_notification(NotifyUrgency urgency, const char *summary,
						 const char *detailed, const char *icon_uri,
						 time_t timeout)
{
	return 0;
}

guint32
notify_send_notification_with_icon_data(NotifyUrgency urgency,
										const char *summary,
										const char *detailed,
										size_t icon_len, void *icon_data,
										time_t timeout)
{
	return 0;
}

guint32
notify_send_request(NotifyUrgency urgency, const char *summary,
					const char *detailed, const char *icon_uri,
					time_t timeout, gpointer user_data,
					size_t default_button, size_t button_count, ...)
{
	return 0;
}

guint32
notify_send_request_with_icon_data(NotifyUrgency urgency,
								   const char *summary, const char *detailed,
								   size_t icon_len, guchar *icon_data,
								   time_t timeout, gpointer user_data,
								   gint32 default_button, size_t button_count,
								   ...)
{
	return 0;
}
