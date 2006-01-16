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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

static gboolean _initted = FALSE;
static gchar *_app_name = NULL;
static DBusGProxy *_proxy = NULL;

#ifdef __GNUC__
#  define format_func __attribute__((format(printf, 1, 2)))
#else /* no format string checking with this compiler */
#  define format_func
#endif

#if 0
static void format_func
print_error (char *message, ...)
{
  char buf[1024];
  va_list args;

  va_start (args, message);
  vsnprintf (buf, sizeof (buf), message, args);
  va_end (args);

  fprintf (stderr, "%s(%d): libnotify: %s",
	   (getenv ("_") ? getenv ("_") : ""), getpid (), buf);
}
#endif

gboolean
notify_init (const char *app_name)
{
  g_return_val_if_fail (app_name != NULL, FALSE);
  g_return_val_if_fail (*app_name != '\0', FALSE);

  if (_initted)
    return TRUE;

  _app_name = g_strdup (app_name);

  g_type_init ();

#ifdef HAVE_ATEXIT
  atexit (notify_uninit);
#endif /* HAVE_ATEXIT */

  _initted = TRUE;

  return TRUE;
}

const gchar *
notify_get_app_name (void)
{
  return _app_name;
}

void
notify_uninit (void)
{

  if (_app_name != NULL)
    {
      g_free (_app_name);
      _app_name = NULL;
    }

  /* TODO: keep track of all notifications and destroy them here? */
}

gboolean
notify_is_initted (void)
{
  return _initted;
}

static DBusGProxy *
get_proxy(void)
{
	DBusGConnection *bus;
	GError *error = NULL;

	if (_proxy != NULL)
		return _proxy;

	bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

	if (error != NULL)
	{
		g_message("Unable to get session bus: %s", error->message);
		g_error_free(error);
		return NULL;
	}

	_proxy = dbus_g_proxy_new_for_name(bus,
									   NOTIFY_DBUS_NAME,
									   NOTIFY_DBUS_CORE_OBJECT,
									   NOTIFY_DBUS_CORE_INTERFACE);
	dbus_g_connection_unref(bus);

	return _proxy;
}

GList *
notify_get_server_caps(void)
{
	GError *error = NULL;
	char **caps = NULL, **cap;
	GList *result = NULL;
	DBusGProxy *proxy = get_proxy();

	g_return_val_if_fail(proxy != NULL, NULL);

	if (!dbus_g_proxy_call(proxy, "GetCapabilities", &error,
						   G_TYPE_INVALID,
						   G_TYPE_STRV, &caps, G_TYPE_INVALID))
	{
		g_message("GetCapabilities call failed: %s", error->message);
		g_error_free(error);
		return NULL;
	}

	for (cap = caps; *cap != NULL; cap++)
	{
		result = g_list_append(result, g_strdup(*cap));
	}

	g_strfreev(caps);

	return result;
}

gboolean
notify_get_server_info(char **ret_name, char **ret_vendor,
					   char **ret_version, char **ret_spec_version)
{
	GError *error = NULL;
	DBusGProxy *proxy = get_proxy();

	g_return_val_if_fail(proxy != NULL, FALSE);

	if (!dbus_g_proxy_call(proxy, "GetServerInformation", &error,
						   G_TYPE_INVALID,
						   G_TYPE_STRING, ret_name,
						   G_TYPE_STRING, ret_vendor,
						   G_TYPE_STRING, ret_version,
						   G_TYPE_STRING, ret_spec_version,
						   G_TYPE_INVALID))
	{
		g_message("GetServerInformation call failed: %s", error->message);
		return FALSE;
	}

	return TRUE;
}
