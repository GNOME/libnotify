/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2004-2006 Christian Hammond <chipx86@chipx86.com>
 * Copyright (C) 2004-2006 Mike Hearn <mike@navi.cx>
 * Copyright (C) 2010 Red Hat, Inc.
 * Copyright © 2010 Christian Persch
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include <glib.h>
#include <gio/gio.h>

#include "notify.h"
#include "internal.h"
#include "notify-marshal.h"

/**
 * SECTION:notify
 * @Short_description: Notification API
 * @Title: notify
 */

static gboolean         _initted = FALSE;
static char            *_app_name = NULL;
static GDBusProxy      *_proxy = NULL;
static GList           *_active_notifications = NULL;
static int              _spec_version_major = 0;
static int              _spec_version_minor = 0;

gboolean
_notify_check_spec_version (int major,
                            int minor)
{
       if (_spec_version_major > major)
               return TRUE;
       if (_spec_version_major < major)
               return FALSE;
       return _spec_version_minor >= minor;
}

static gboolean
_notify_get_server_info (char **ret_name,
                         char **ret_vendor,
                         char **ret_version,
                         char **ret_spec_version,
                         GError **error)
{
        GDBusProxy *proxy;
        GVariant   *result;

        proxy = _notify_get_proxy (error);
        if (proxy == NULL) {
                return FALSE;
        }

        result = g_dbus_proxy_call_sync (proxy,
                                         "GetServerInformation",
                                         g_variant_new ("()"),
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1 /* FIXME shorter timeout? */,
                                         NULL,
                                         error);
        if (result == NULL) {
                return FALSE;
        }
        if (!g_variant_is_of_type (result, G_VARIANT_TYPE ("(ssss)"))) {
                g_variant_unref (result);
                g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                             "Unexpected reply type");
                return FALSE;
        }

        g_variant_get (result, "(ssss)",
                       ret_name,
                       ret_vendor,
                       ret_version,
                       ret_spec_version);
        g_variant_unref (result);
        return TRUE;
}

static gboolean
_notify_update_spec_version (GError **error)
{
       char *spec_version;

       if (!_notify_get_server_info (NULL, NULL, NULL, &spec_version, error)) {
               return FALSE;
       }

       sscanf (spec_version,
               "%d.%d",
               &_spec_version_major,
               &_spec_version_minor);

       g_free (spec_version);

       return TRUE;
}


/**
 * notify_set_app_name:
 * @app_name: The name of the application
 *
 * Sets the application name.
 *
 */
void
notify_set_app_name (const char *app_name)
{
        g_free (_app_name);
        _app_name = g_strdup (app_name);
}

/**
 * notify_init:
 * @app_name: The name of the application initializing libnotify.
 *
 * Initialized libnotify. This must be called before any other functions.
 *
 * Returns: %TRUE if successful, or %FALSE on error.
 */
gboolean
notify_init (const char *app_name)
{
        g_return_val_if_fail (app_name != NULL, FALSE);
        g_return_val_if_fail (*app_name != '\0', FALSE);

        if (_initted)
                return TRUE;

        notify_set_app_name (app_name);

        g_type_init ();

        _initted = TRUE;

        return TRUE;
}

/**
 * notify_get_app_name:
 *
 * Gets the application name registered.
 *
 * Returns: The registered application name, passed to notify_init().
 */
const char *
notify_get_app_name (void)
{
        return _app_name;
}

/**
 * notify_uninit:
 *
 * Uninitialized libnotify.
 *
 * This should be called when the program no longer needs libnotify for
 * the rest of its lifecycle, typically just before exitting.
 */
void
notify_uninit (void)
{
        GList *l;

        if (!_initted) {
                return;
        }

        if (_app_name != NULL) {
                g_free (_app_name);
                _app_name = NULL;
        }

        for (l = _active_notifications; l != NULL; l = l->next) {
                NotifyNotification *n = NOTIFY_NOTIFICATION (l->data);

                if (_notify_notification_get_timeout (n) == 0 ||
                    _notify_notification_has_nondefault_actions (n)) {
                        notify_notification_close (n, NULL);
                }
        }

        if (_proxy != NULL) {
            g_object_unref (_proxy);
            _proxy = NULL;
        }

        _initted = FALSE;
}

/**
 * notify_is_initted:
 *
 * Gets whether or not libnotify is initialized.
 *
 * Returns: %TRUE if libnotify is initialized, or %FALSE otherwise.
 */
gboolean
notify_is_initted (void)
{
        return _initted;
}

/*
 * _notify_get_proxy:
 * @error: (allow-none): a location to store a #GError, or %NULL
 *
 * Synchronously creates the #GDBusProxy for the notification service,
 * and caches the result.
 *
 * Returns: the #GDBusProxy for the notification service, or %NULL on error
 */
GDBusProxy *
_notify_get_proxy (GError **error)
{
        if (_proxy != NULL)
                return _proxy;

        _proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                                                NULL,
                                                NOTIFY_DBUS_NAME,
                                                NOTIFY_DBUS_CORE_OBJECT,
                                                NOTIFY_DBUS_CORE_INTERFACE,
                                                NULL,
                                                error);
        if (_proxy == NULL) {
                return NULL;
        }

        if (!_notify_update_spec_version (error)) {
               g_object_unref (_proxy);
               _proxy = NULL;
               return NULL;
        }

        g_object_add_weak_pointer (G_OBJECT (_proxy), (gpointer *) &_proxy);

        return _proxy;
}

/**
 * notify_get_server_caps:
 *
 * Synchronously queries the server for its capabilities and returns them in a #GList.
 *
 * Returns: (transfer full) (element-type utf8): a #GList of server capability strings. Free
 *   the list elements with g_free() and the list itself with g_list_free().
 */
GList *
notify_get_server_caps (void)
{
        GDBusProxy *proxy;
        GVariant   *result;
        char      **cap, **caps;
        GList      *list = NULL;

        proxy = _notify_get_proxy (NULL);
        if (proxy == NULL) {
                g_warning ("Failed to connect to proxy");
                return NULL;
        }

        result = g_dbus_proxy_call_sync (proxy,
                                         "GetCapabilities",
                                         g_variant_new ("()"),
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1 /* FIXME shorter timeout? */,
                                         NULL,
                                         NULL);
        if (result == NULL) {
                return NULL;
        }
        if (!g_variant_is_of_type (result, G_VARIANT_TYPE ("(as)"))) {
                g_variant_unref (result);
                return NULL;
        }

        g_variant_get (result, "(^as)", &caps);

        for (cap = caps; *cap != NULL; cap++) {
                list = g_list_prepend (list, *cap);
        }

        g_free (caps);
        g_variant_unref (result);

        return g_list_reverse (list);
}

/**
 * notify_get_server_info:
 * @ret_name: (out) (allow-none) (transfer full): a location to store the server name, or %NULL
 * @ret_vendor: (out) (allow-none) (transfer full): a location to store the server vendor, or %NULL
 * @ret_version: (out) (allow-none) (transfer full): a location to store the server version, or %NULL
 * @ret_spec_version: (out) (allow-none) (transfer full): a location to store the version the service is compliant with, or %NULL
 *
 * Synchronously queries the server for its information, specifically, the name, vendor,
 * server version, and the version of the notifications specification that it
 * is compliant with.
 *
 * Returns: %TRUE if successful, and the variables passed will be set, %FALSE
 *          on error. The returned strings must be freed with g_free
 */
gboolean
notify_get_server_info (char **ret_name,
                        char **ret_vendor,
                        char **ret_version,
                        char **ret_spec_version)
{
        return _notify_get_server_info (ret_name, ret_vendor, ret_version, ret_spec_version, NULL);
}

void
_notify_cache_add_notification (NotifyNotification *n)
{
        _active_notifications = g_list_prepend (_active_notifications, n);
}

void
_notify_cache_remove_notification (NotifyNotification *n)
{
        _active_notifications = g_list_remove (_active_notifications, n);
}
