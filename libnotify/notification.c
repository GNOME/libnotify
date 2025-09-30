/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Christian Hammond
 * Copyright (C) 2006 John Palmieri
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

#include <gio/gio.h>

#ifdef HAVE_GIO_DESKTOP_APP_INFO
#include <gio/gdesktopappinfo.h>
#endif

#include "notify.h"
#include "internal.h"
#include "launch-context.h"

#if !defined(G_PARAM_STATIC_NAME) && !defined(G_PARAM_STATIC_NICK) && \
    !defined(G_PARAM_STATIC_BLURB)
# define G_PARAM_STATIC_NAME 0
# define G_PARAM_STATIC_NICK 0
# define G_PARAM_STATIC_BLURB 0
#endif

static void     notify_notification_class_init (NotifyNotificationClass *klass);
static void     notify_notification_init       (NotifyNotification *sp);
static void     notify_notification_finalize   (GObject            *object);
static void     notify_notification_dispose    (GObject            *object);

typedef struct
{
        NotifyActionCallback cb;
        GFreeFunc            free_func;
        gpointer             user_data;
        char                 *label;
} ActionInfo;

typedef struct _NotifyNotificationPrivate
{
        guint32         id;
        char           *app_name;
        char           *app_icon;
        char           *summary;
        char           *body;
        char           *activation_token;

        /* NULL to use icon data. Anything else to have server lookup icon */
        char           *icon_name;
        GdkPixbuf      *icon_pixbuf;

        /*
         * -1   = use server default
         *  0   = never timeout
         *  > 0 = Number of milliseconds before we timeout
         */
        gint            timeout;
        guint           portal_timeout_id;

        GHashTable     *action_map;
        GHashTable     *hints;

        gboolean        has_nondefault_actions;
        gboolean        activating;

        gulong          proxy_signal_handler;

        gint            closed_reason;
} NotifyNotificationPrivate;

enum
{
        SIGNAL_CLOSED,
        LAST_SIGNAL
};

enum
{
        PROP_0,
        PROP_ID,
        PROP_APP_NAME,
        PROP_APP_ICON,
        PROP_SUMMARY,
        PROP_BODY,
        PROP_ICON_NAME,
        PROP_CLOSED_REASON,
        NUM_PROPERTIES,
};

static void     notify_notification_set_property (GObject      *object,
                                                  guint         prop_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void     notify_notification_get_property (GObject      *object,
                                                  guint         prop_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
static guint    signals[LAST_SIGNAL] = { 0 };
static GParamSpec *properties[NUM_PROPERTIES] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (NotifyNotification, notify_notification, G_TYPE_OBJECT)

static GObject *
notify_notification_constructor (GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_params)
{
        GObject *object;
        GObjectClass *object_class = G_OBJECT_CLASS (notify_notification_parent_class);

        object = object_class->constructor (type,
                                            n_construct_properties,
                                            construct_params);

        _notify_cache_add_notification (NOTIFY_NOTIFICATION (object));

        return object;
}

static void
notify_notification_class_init (NotifyNotificationClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = notify_notification_constructor;
        object_class->get_property = notify_notification_get_property;
        object_class->set_property = notify_notification_set_property;
        object_class->dispose = notify_notification_dispose;
        object_class->finalize = notify_notification_finalize;

        /**
         * NotifyNotification::closed:
         * @notification: The object which received the signal.
         *
         * Emitted when the notification is closed.
         *
         * Note that when a [class@Notify.Notification] is used in a sandboxed
         * environment where XDG Desktop Notification Portal is implicitly used,
         * the signal [signal@Notify.Notification::closed] is only emitted when
         * the notification is closed in response to an user action response.
         *
         * NO signal will be emitted if the user or the daemon dismissed the
         * notification for any other reason.
         */
        signals[SIGNAL_CLOSED] =
                g_signal_new ("closed",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (NotifyNotificationClass, closed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        /**
         * NotifyNotification:id:
         *
         * The Id of the notification.
         */
        properties[PROP_ID] = g_param_spec_int ("id", "ID",
                                                "The notification ID",
                                                0,
                                                G_MAXINT32,
                                                0,
                                                G_PARAM_READWRITE
                                                | G_PARAM_CONSTRUCT
                                                | G_PARAM_STATIC_NAME
                                                | G_PARAM_STATIC_NICK
                                                | G_PARAM_STATIC_BLURB);

        /**
         * NotifyNotification:app-name:
         *
         * The name of the application for the notification.
         *
         * Since: 0.7.3
         */
        properties[PROP_APP_NAME] = g_param_spec_string ("app-name",
                                                         "Application name",
                                                         "The application name to use for this notification",
                                                         NULL,
                                                         G_PARAM_READWRITE
                                                         | G_PARAM_STATIC_NAME
                                                         | G_PARAM_STATIC_NICK
                                                         | G_PARAM_STATIC_BLURB);

        /**
         * NotifyNotification:app-icon:
         *
         * The icon of the application for the notification.
         *
         * Since: 0.8.4
         */
        properties[PROP_APP_ICON] = g_param_spec_string ("app-icon",
                                                         "Application icon",
                                                         "The application icon to use for this notification as filename or icon theme-compliant name",
                                                         NULL,
                                                         G_PARAM_READWRITE
                                                         | G_PARAM_STATIC_NAME
                                                         | G_PARAM_STATIC_NICK
                                                         | G_PARAM_STATIC_BLURB);


        /**
         * NotifyNotification:summary:
         *
         * The summary of the notification.
         */
        properties[PROP_SUMMARY] = g_param_spec_string ("summary",
                                                        "Summary",
                                                        "The summary text",
                                                        NULL,
                                                        G_PARAM_READWRITE
                                                        | G_PARAM_CONSTRUCT
                                                        | G_PARAM_STATIC_NAME
                                                        | G_PARAM_STATIC_NICK
                                                        | G_PARAM_STATIC_BLURB);

        /**
         * NotifyNotification:body:
         *
         * The body of the notification.
         */
        properties[PROP_BODY] = g_param_spec_string ("body",
                                                     "Message Body",
                                                     "The message body text",
                                                     NULL,
                                                     G_PARAM_READWRITE
                                                     | G_PARAM_CONSTRUCT
                                                     | G_PARAM_STATIC_NAME
                                                     | G_PARAM_STATIC_NICK
                                                                      | G_PARAM_STATIC_BLURB);

        /**
         * NotifyNotification:icon-name:
         *
         * The icon-name of the icon to be displayed on the notification.
         */
        properties[PROP_ICON_NAME] = g_param_spec_string ("icon-name",
                                                          "Icon Name",
                                                          "The icon filename or icon theme-compliant name",
                                                          NULL,
                                                          G_PARAM_READWRITE
                                                          | G_PARAM_CONSTRUCT
                                                          | G_PARAM_STATIC_NAME
                                                          | G_PARAM_STATIC_NICK
                                                          | G_PARAM_STATIC_BLURB);

        /**
         * NotifyNotification:closed-reason:
         *
         * The closed reason of the notification.
         *
         * See [signal@Notification::closed].
         */
        properties[PROP_CLOSED_REASON] = g_param_spec_int ("closed-reason",
                                                           "Closed Reason",
                                                           "The reason code for why the notification was closed",
                                                           NOTIFY_CLOSED_REASON_UNSET,
                                                           G_MAXINT32,
                                                           NOTIFY_CLOSED_REASON_UNSET,
                                                           G_PARAM_READABLE
                                                           | G_PARAM_STATIC_NAME
                                                           | G_PARAM_STATIC_NICK
                                                           | G_PARAM_STATIC_BLURB);

        g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

static void
notify_notification_update_internal (NotifyNotification *notification,
                                     const char         *summary,
                                     const char         *body,
                                     const char         *icon);

static void
notify_notification_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
        NotifyNotification        *notification = NOTIFY_NOTIFICATION (object);
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);

        switch (prop_id) {
        case PROP_ID:
                priv->id = g_value_get_int (value);
                break;

        case PROP_APP_NAME:
                notify_notification_set_app_name (notification,
                                                  g_value_get_string (value));
                break;

        case PROP_APP_ICON:
                notify_notification_set_app_icon (notification,
                                                  g_value_get_string (value));
                break;

        case PROP_SUMMARY:
                notify_notification_update_internal (notification,
                                                     g_value_get_string (value),
                                                     priv->body,
                                                     priv->icon_name);
                break;

        case PROP_BODY:
                notify_notification_update_internal (notification,
                                                     priv->summary,
                                                     g_value_get_string (value),
                                                     priv->icon_name);
                break;

        case PROP_ICON_NAME:
                notify_notification_update_internal (notification,
                                                     priv->summary,
                                                     priv->body,
                                                     g_value_get_string (value));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
notify_notification_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
        NotifyNotification        *notification = NOTIFY_NOTIFICATION (object);
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);

        switch (prop_id) {
        case PROP_ID:
                g_value_set_int (value, priv->id);
                break;

        case PROP_SUMMARY:
                g_value_set_string (value, priv->summary);
                break;

        case PROP_APP_NAME:
                g_value_set_string (value, priv->app_name);
                break;

        case PROP_APP_ICON:
                g_value_set_string (value, priv->app_icon);
                break;

        case PROP_BODY:
                g_value_set_string (value, priv->body);
                break;

        case PROP_ICON_NAME:
                g_value_set_string (value, priv->icon_name);
                break;

        case PROP_CLOSED_REASON:
                g_value_set_int (value, priv->closed_reason);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
destroy_action_info (ActionInfo *action_info)
{
        if (action_info->user_data != NULL && action_info->free_func != NULL) {
                action_info->free_func (action_info->user_data);
        }

        g_free (action_info->label);
        g_free (action_info);
}

static void
notify_notification_init (NotifyNotification *notification)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);

        priv->timeout = NOTIFY_EXPIRES_DEFAULT;
        priv->closed_reason = NOTIFY_CLOSED_REASON_UNSET;
        priv->hints = g_hash_table_new_full (g_str_hash,
                                             g_str_equal,
                                             g_free,
                                             (GDestroyNotify) g_variant_unref);

        priv->action_map = g_hash_table_new_full (g_str_hash,
                                                  g_str_equal,
                                                  g_free,
                                                  (GDestroyNotify) destroy_action_info);
}

static void
notify_notification_dispose (GObject *object)
{
        NotifyNotification        *notification = NOTIFY_NOTIFICATION (object);
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);
        GDBusProxy                *proxy;

        g_clear_handle_id (&priv->portal_timeout_id, g_source_remove);

        proxy = _notify_get_proxy (NULL);
        if (proxy != NULL) {
                g_clear_signal_handler (&priv->proxy_signal_handler, proxy);
        }

        G_OBJECT_CLASS (notify_notification_parent_class)->dispose (object);
}

static void
notify_notification_finalize (GObject *object)
{
        NotifyNotification        *notification = NOTIFY_NOTIFICATION (object);
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);

        _notify_cache_remove_notification (notification);

        g_free (priv->app_name);
        g_free (priv->app_icon);
        g_free (priv->summary);
        g_free (priv->body);
        g_free (priv->icon_name);
        g_free (priv->activation_token);
        g_clear_object (&priv->icon_pixbuf);

        if (priv->action_map != NULL)
                g_hash_table_destroy (priv->action_map);

        if (priv->hints != NULL)
                g_hash_table_destroy (priv->hints);

        G_OBJECT_CLASS (notify_notification_parent_class)->finalize (object);
}

static gboolean
maybe_warn_portal_unsupported_feature (const char *feature_name)
{
        if (!_notify_uses_portal_notifications ()) {
                return FALSE;
        }

        g_message ("%s is not available when using Portal Notifications",
                   feature_name);
        return TRUE;
}

/**
 * notify_notification_new:
 * @summary: (not nullable): The required summary text.
 * @body: (nullable): The optional body text.
 * @icon: (nullable): The optional icon theme icon name or filename.
 *
 * Creates a new #NotifyNotification.
 *
 * The summary text is required, but all other parameters are optional.
 *
 * Returns: The new #NotifyNotification.
 */
NotifyNotification *
notify_notification_new (const char *summary,
                         const char *body,
                         const char *icon)
{
        return g_object_new (NOTIFY_TYPE_NOTIFICATION,
                             "summary", summary,
                             "body", body,
                             "icon-name", icon,
                             NULL);
}

static gchar *
try_prepend_path (const char *base_path,
                  const char *path)
{
        gchar *path_filename;
        gchar *path_ret;
        gboolean was_uri;

        if (!path || *path == '\0')
                return NULL;

        was_uri = TRUE;
        path_ret = NULL;
        path_filename = g_filename_from_uri (base_path, NULL, NULL);

        if (path_filename == NULL) {
                was_uri = FALSE;

                if (base_path && base_path[0] == G_DIR_SEPARATOR) {
                        path_filename = g_strdup (base_path);
                } else {
                        path_filename = realpath (base_path, NULL);

                        if (path_filename == NULL) {
                                /* File path is not existing, but let's check
                                 * if it's under the base path before giving up
                                 */
                                path_filename = g_strdup (base_path);
                        }
                }
        }

        if (g_str_has_prefix (path_filename, path)) {
                path_ret = g_strdup (path_filename);
        } else {
                g_debug ("Trying to look at file '%s' in the '%s' prefix.",
                         base_path,
                         path);
                path_ret = g_build_filename (path, path_filename, NULL);
        }

        if (!g_file_test (path_ret, G_FILE_TEST_EXISTS)) {
                g_debug ("Nothing found at %s", path_ret);
                g_free (path_ret);
                path_ret = NULL;
        } else if (was_uri) {
                gchar *uri = g_filename_to_uri (path_ret, NULL, NULL);

                if (uri != NULL) {
                        g_free (path_ret);
                        path_ret = uri;
                }
        }

        g_free (path_filename);

        return path_ret;
}

static gchar *
try_prepend_snap_desktop (NotifyNotification *notification,
                          const gchar        *desktop)
{
        gchar *ret = NULL;

        /*
         * if it's an absolute path, try prepending $SNAP, otherwise try
         * ${SNAP_NAME}_; snap .desktop files are in the format
         * ${SNAP_NAME}_desktop_file_name
         */
        ret = try_prepend_path (desktop, _notify_get_snap_path ());

        if (ret == NULL && _notify_get_snap_name () != NULL &&
            strchr (desktop, G_DIR_SEPARATOR) == NULL) {
#ifdef HAVE_GIO_DESKTOP_APP_INFO
                g_autoptr(GDesktopAppInfo) app_info = NULL;
                g_autofree char *desktop_id = NULL;

                desktop_id = g_strconcat (desktop, ".desktop", NULL);
                if ((app_info = g_desktop_app_info_new (desktop_id))) {
                        /* Just return the desktop file if it's usable. */
                        g_clear_object (&app_info);
                        return NULL;
                }
#endif /* HAVE_GIO_DESKTOP_APP_INFO */

                ret = g_strdup_printf ("%s_%s", _notify_get_snap_name (), desktop);
        }

        return ret;
}

static gchar *
try_prepend_snap (NotifyNotification *notification,
                  const gchar        *value)
{
        /* hardcoded paths to icons might be relocated under $SNAP */
        return try_prepend_path (value, _notify_get_snap_path ());
}


static void
notify_notification_update_internal (NotifyNotification *notification,
                                     const char         *summary,
                                     const char         *body,
                                     const char         *icon)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);

        if (priv->summary != summary) {
                g_free (priv->summary);
                priv->summary = g_strdup (summary);
                g_object_notify_by_pspec (G_OBJECT (notification), properties[PROP_SUMMARY]);
        }

        if (priv->body != body) {
                g_free (priv->body);
                priv->body = (body != NULL && *body != '\0' ? g_strdup (body) : NULL);
                g_object_notify_by_pspec (G_OBJECT (notification), properties[PROP_BODY]);
        }

        if (priv->icon_name != icon) {
                gchar *snapped_icon;

                g_free (priv->icon_name);
                priv->icon_name = (icon != NULL
                                                 && *icon != '\0' ? g_strdup (icon) : NULL);
                snapped_icon = try_prepend_snap (notification, priv->icon_name);
                if (snapped_icon != NULL) {
                        g_debug ("Icon updated in snap environment: '%s' -> '%s'\n",
                                 priv->icon_name, snapped_icon);
                        g_free (priv->icon_name);
                        priv->icon_name = snapped_icon;
                }

                notify_notification_set_hint (notification,
                                              NOTIFY_NOTIFICATION_HINT_IMAGE_PATH,
                                              priv->icon_name ?
                                              g_variant_new_string (priv->icon_name) : NULL);

                g_object_notify_by_pspec (G_OBJECT (notification), properties[PROP_ICON_NAME]);
        }
}

/**
 * notify_notification_update:
 * @notification: The notification to update.
 * @summary: The new required summary text.
 * @body: (nullable): The optional body text.
 * @icon: (nullable): The optional icon theme icon name or filename.
 *
 * Updates the notification text and icon.
 *
 * This won't send the update out and display it on the screen. For that, you
 * will need to call [method@Notification.show].
 *
 * Returns: %TRUE, unless an invalid parameter was passed.
 */
gboolean
notify_notification_update (NotifyNotification *notification,
                            const char         *summary,
                            const char         *body,
                            const char         *icon)
{
        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (notification), FALSE);
        g_return_val_if_fail (summary != NULL && *summary != '\0', FALSE);

        notify_notification_update_internal (notification,
                                             summary, body, icon);

        return TRUE;
}

static char *
get_portal_notification_id (NotifyNotification *notification)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);
        char *app_id;
        char *notification_id;

        g_assert (_notify_uses_portal_notifications ());

        if (_notify_get_snap_name ()) {
                app_id = g_strdup_printf ("snap.%s_%s",
                                          _notify_get_snap_name (),
                                          _notify_get_snap_app ());
        } else {
                app_id = g_strdup_printf ("flatpak.%s",
                                          _notify_get_flatpak_app ());
        }

        notification_id = g_strdup_printf ("libnotify-%s-%s-%u",
                                           app_id,
                                           notify_get_app_name (),
                                           priv->id);

        g_free (app_id);

        return notification_id;
}

static gboolean
activate_action (NotifyNotification *notification,
                 const gchar        *action)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);
        ActionInfo *action_info;

        action_info = g_hash_table_lookup (priv->action_map, action);

        if (!action_info) {
                return FALSE;
        }

        priv->activating = TRUE;
        action_info->cb (notification, (char *) action, action_info->user_data);
        priv->activating = FALSE;
        g_clear_pointer (&priv->activation_token, g_free);

        return TRUE;
}

static gboolean
close_notification (NotifyNotification *notification,
                    NotifyClosedReason  reason)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);

        if (priv->closed_reason != NOTIFY_CLOSED_REASON_UNSET ||
            reason == NOTIFY_CLOSED_REASON_UNSET) {
                return FALSE;
        }

        g_object_ref (G_OBJECT (notification));
        priv->closed_reason = reason;
        g_signal_emit (notification, signals[SIGNAL_CLOSED], 0);
        priv->id = 0;
        g_object_unref (G_OBJECT (notification));

        return TRUE;
}

static void
proxy_g_signal_cb (GDBusProxy *proxy,
                   const char *sender_name,
                   const char *signal_name,
                   GVariant   *parameters,
                   NotifyNotification *notification)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);

        const char *interface;

        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        interface = g_dbus_proxy_get_interface_name (proxy);

        if (g_strcmp0 (signal_name, "NotificationClosed") == 0 &&
            g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(uu)"))) {
                guint32 id, reason;

                g_variant_get (parameters, "(uu)", &id, &reason);
                if (id != priv->id)
                        return;

                close_notification (notification, reason);
        } else if (g_strcmp0 (signal_name, "ActionInvoked") == 0 &&
                   g_str_equal (interface, NOTIFY_DBUS_CORE_INTERFACE) &&
                   g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(us)"))) {
                guint32 id;
                const char *action;

                g_variant_get (parameters, "(u&s)", &id, &action);

                if (id != priv->id)
                        return;

                if (!activate_action (notification, action) &&
                    g_ascii_strcasecmp (action, "default")) {
                        g_warning ("Received unknown action %s", action);
                }
        } else if (g_strcmp0 (signal_name, "ActivationToken") == 0 &&
                   g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(us)"))) {
                guint32 id;
                const char *activation_token;

                g_variant_get (parameters, "(u&s)", &id, &activation_token);

                if (id != priv->id)
                        return;

                g_free (priv->activation_token);
                priv->activation_token = g_strdup (activation_token);
        } else if (g_str_equal (signal_name, "ActionInvoked") &&
                   g_str_equal (interface, NOTIFY_PORTAL_DBUS_CORE_INTERFACE) &&
                   g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(ssav)"))) {
                char *notification_id;
                const char *id;
                const char *action;
                GVariant *parameter;

                g_variant_get (parameters, "(&s&s@av)", &id, &action, &parameter);
                g_variant_unref (parameter);

                notification_id = get_portal_notification_id (notification);

                if (!g_str_equal (notification_id, id)) {
                        g_free (notification_id);
                        return;
                }

                if (!activate_action (notification, action) &&
                    g_str_equal (action, "default-action") &&
                    !_notify_get_snap_app ()) {
                        g_warning ("Received unknown action %s", action);
                }

                close_notification (notification, NOTIFY_CLOSED_REASON_DISMISSED);

                g_free (notification_id);
        } else {
                g_debug ("Unhandled signal '%s.%s'", interface, signal_name);
        }
}

static gboolean
remove_portal_notification (GDBusProxy         *proxy,
                            NotifyNotification *notification,
                            NotifyClosedReason  reason,
                            GError            **error)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);
        GVariant *ret;
        gchar *notification_id;

        if (priv->portal_timeout_id) {
                g_source_remove (priv->portal_timeout_id);
                priv->portal_timeout_id = 0;
        }

        notification_id = get_portal_notification_id (notification);

        ret = g_dbus_proxy_call_sync (proxy,
                                      "RemoveNotification",
                                      g_variant_new ("(s)", notification_id),
                                      G_DBUS_CALL_FLAGS_NONE,
                                      -1,
                                      NULL,
                                      error);

        g_free (notification_id);

        if (!ret) {
                return FALSE;
        }

        close_notification (notification, reason);

        g_variant_unref (ret);

        return TRUE;
}

static gboolean
on_portal_timeout (gpointer data)
{
        NotifyNotification *notification = data;
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);
        GDBusProxy *proxy;

        priv->portal_timeout_id = 0;

        proxy = _notify_get_proxy (NULL);
        if (proxy == NULL) {
                return FALSE;
        }

        remove_portal_notification (proxy, notification,
                                    NOTIFY_CLOSED_REASON_EXPIRED, NULL);
        return FALSE;
}

static GIcon *
get_notification_gicon (NotifyNotification  *notification,
                        GError             **error)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);
        GFileInputStream *input;
        GFile *file = NULL;
        GIcon *gicon = NULL;

        if (priv->icon_pixbuf) {
                return G_ICON (g_object_ref (priv->icon_pixbuf));
        }

        if (!priv->icon_name) {
                return NULL;
        }

        if (g_uri_is_valid (priv->icon_name, G_URI_FLAGS_PARSE_RELAXED, NULL)) {
                file = g_file_new_for_uri (priv->icon_name);
        } else if (g_file_test (priv->icon_name, G_FILE_TEST_EXISTS)) {
                file = g_file_new_for_path (priv->icon_name);
        } else {
                return g_themed_icon_new (priv->icon_name);
        }

        input = g_file_read (file, NULL, error);

        if (input) {
                GByteArray *bytes_array = g_byte_array_new ();
                guint8 buf[1024];

                while (TRUE) {
                        gssize read;

                        read = g_input_stream_read (G_INPUT_STREAM (input),
                                                    buf,
                                                    G_N_ELEMENTS (buf),
                                                    NULL, NULL);

                        if (read > 0) {
                                g_byte_array_append (bytes_array, buf, read);
                        } else {
                                if (read < 0) {
                                        g_byte_array_unref (bytes_array);
                                        bytes_array = NULL;
                                }

                                break;
                        }
                }

                if (bytes_array && bytes_array->len) {
                        GBytes *bytes;

                        bytes = g_byte_array_free_to_bytes (bytes_array);
                        bytes_array = NULL;

                        gicon = g_bytes_icon_new (bytes);
                } else if (bytes_array) {
                        g_byte_array_unref (bytes_array);
                }
        }

        g_clear_object (&input);
        g_clear_object (&file);

        return gicon;
}

static gboolean
add_portal_notification (GDBusProxy         *proxy,
                         NotifyNotification *notification,
                         GError            **error)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);
        GIcon *icon;
        GVariant *urgency;
        GVariant *ret;
        GVariantBuilder builder;
        GError *local_error = NULL;
        static guint32 portal_notification_count = 0;
        char *notification_id;

        g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);

        g_variant_builder_add (&builder, "{sv}", "title",
                               g_variant_new_string (priv->summary ? priv->summary : ""));
        g_variant_builder_add (&builder, "{sv}", "body",
                               g_variant_new_string (priv->body ? priv->body : ""));

        if (g_hash_table_lookup (priv->action_map, "default")) {
                g_variant_builder_add (&builder, "{sv}", "default-action",
                                       g_variant_new_string ("default"));
        } else if (g_hash_table_lookup (priv->action_map, "DEFAULT")) {
                g_variant_builder_add (&builder, "{sv}", "default-action",
                                       g_variant_new_string ("DEFAULT"));
        }

        if (priv->has_nondefault_actions) {
                GHashTableIter iter;
                gpointer key, value;
                GVariantBuilder buttons;

                g_variant_builder_init (&buttons, G_VARIANT_TYPE ("aa{sv}"));

                g_hash_table_iter_init (&iter, priv->action_map);
                while (g_hash_table_iter_next (&iter, &key, &value)) {
                        GVariantBuilder button;
                        const char *action = key;
                        ActionInfo *action_info = value;
                        const char *label = action_info->label;

                        g_variant_builder_init (&button, G_VARIANT_TYPE_VARDICT);

                        g_variant_builder_add (&button, "{sv}", "action",
                                               g_variant_new_string (action));
                        g_variant_builder_add (&button, "{sv}", "label",
                                               g_variant_new_string (label));

                        g_variant_builder_add (&buttons, "@a{sv}",
                                               g_variant_builder_end (&button));
                }

                g_variant_builder_add (&builder, "{sv}", "buttons",
                                       g_variant_builder_end (&buttons));
        }

        urgency = g_hash_table_lookup (priv->hints, NOTIFY_NOTIFICATION_HINT_URGENCY);
        if (urgency) {
                switch (g_variant_get_byte (urgency)) {
                case NOTIFY_URGENCY_LOW:
                        g_variant_builder_add (&builder, "{sv}", "priority",
                                               g_variant_new_string ("low"));
                        break;
                case NOTIFY_URGENCY_NORMAL:
                        g_variant_builder_add (&builder, "{sv}", "priority",
                                               g_variant_new_string ("normal"));
                        break;
                case NOTIFY_URGENCY_CRITICAL:
                        g_variant_builder_add (&builder, "{sv}", "priority",
                                               g_variant_new_string ("urgent"));
                        break;
                default:
                        g_warn_if_reached ();
                }
        }

        icon = get_notification_gicon (notification, &local_error);
        if (icon) {
                GVariant *serialized_icon = g_icon_serialize (icon);

                g_variant_builder_add (&builder, "{sv}", "icon",
                                       serialized_icon);
                g_variant_unref (serialized_icon);
                g_clear_object (&icon);
        } else if (local_error) {
                g_propagate_error (error, local_error);
                return FALSE;
        }

        if (!priv->id) {
                priv->id = ++portal_notification_count;
        } else if (priv->closed_reason == NOTIFY_CLOSED_REASON_UNSET) {
                remove_portal_notification (proxy, notification,
                                            NOTIFY_CLOSED_REASON_UNSET, NULL);
        }

        notification_id = get_portal_notification_id (notification);

        ret = g_dbus_proxy_call_sync (proxy,
                                      "AddNotification",
                                      g_variant_new ("(s@a{sv})",
                                                     notification_id,
                                                     g_variant_builder_end (&builder)),
                                      G_DBUS_CALL_FLAGS_NONE,
                                      -1,
                                      NULL,
                                      error);

        if (priv->portal_timeout_id) {
                g_source_remove (priv->portal_timeout_id);
                priv->portal_timeout_id = 0;
        }

        g_free (notification_id);

        if (!ret) {
                return FALSE;
        }

        if (priv->timeout > 0) {
                priv->portal_timeout_id = g_timeout_add (priv->timeout,
                                                         on_portal_timeout,
                                                         notification);
        }

        g_variant_unref (ret);

        return TRUE;
}

const char *
get_hint_name (NotifyNotification *notification,
               const char         *hint)
{
        if (g_str_equal (hint, NOTIFY_NOTIFICATION_HINT_IMAGE_DATA)) {
                if (_notify_check_spec_version (1, 2)) {
                        return hint;
                }
                if (_notify_check_spec_version (1, 1)) {
                        return NOTIFY_NOTIFICATION_HINT_IMAGE_DATA_LEGACY;
                }
                return "icon_data";
        }

        if (g_str_equal (hint, NOTIFY_NOTIFICATION_HINT_IMAGE_PATH)) {
                if (_notify_check_spec_version (1, 2)) {
                        return hint;
                }
                if (_notify_check_spec_version (1, 1)) {
                        return NOTIFY_NOTIFICATION_HINT_IMAGE_PATH_LEGACY;
                }

                /* Before 1.1 only one image/icon could be specified and the
                 * icon_data hint didn't allow for a path or icon name,
                 * therefore the icon is set as the app icon of the Notify call
                 */
                return NULL;
        }

        return hint;
}

/**
 * notify_notification_show:
 * @notification: The notification.
 * @error: The returned error information.
 *
 * Tells the notification server to display the notification on the screen.
 *
 * Returns: %TRUE if successful. On error, this will return %FALSE and set
 *   @error.
 */
gboolean
notify_notification_show (NotifyNotification *notification,
                          GError            **error)
{
        NotifyNotificationPrivate *priv;
        GDBusProxy                *proxy;
        GVariantBuilder            actions_builder, hints_builder;
        GHashTableIter             iter;
        gpointer                   key, data;
        GVariant                  *result;
        GApplication              *application = NULL;
        const char                *app_icon = NULL;

        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (notification), FALSE);
        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

        if (!notify_is_initted ()) {
                g_warning ("you must call notify_init() before showing");
                g_assert_not_reached ();
        }

        priv = notify_notification_get_instance_private (notification);
        proxy = _notify_get_proxy (error);
        if (proxy == NULL) {
                return FALSE;
        }

        if (priv->proxy_signal_handler == 0) {
                priv->proxy_signal_handler = g_signal_connect_object (proxy,
                                                                      "g-signal",
                                                                      G_CALLBACK (proxy_g_signal_cb),
                                                                      notification,
                                                                      0);
        }

        if (_notify_uses_portal_notifications ()) {
                return add_portal_notification (proxy, notification, error);
        }

        g_variant_builder_init (&actions_builder, G_VARIANT_TYPE ("as"));
        g_hash_table_iter_init (&iter, priv->action_map);
        while (g_hash_table_iter_next (&iter, &key, &data)) {
                ActionInfo *action_info = data;

                g_variant_builder_add (&actions_builder, "s", key);
                g_variant_builder_add (&actions_builder, "s", action_info->label);
        }

        g_variant_builder_init (&hints_builder, G_VARIANT_TYPE ("a{sv}"));
        g_hash_table_iter_init (&iter, priv->hints);
        while (g_hash_table_iter_next (&iter, &key, &data)) {
                const char *hint = get_hint_name (notification, key);
                if (!hint) {
                        continue;
                }

                g_variant_builder_add (&hints_builder, "{sv}", hint, data);
        }

        if (g_hash_table_lookup (priv->hints, "sender-pid") == NULL) {
                g_variant_builder_add (&hints_builder, "{sv}", "sender-pid",
                                       g_variant_new_int64 (getpid ()));
        }

        if (_notify_get_snap_app () &&
            g_hash_table_lookup (priv->hints,
                                 NOTIFY_NOTIFICATION_HINT_DESKTOP_ENTRY) == NULL) {
                gchar *snap_desktop;

                snap_desktop = g_strdup_printf ("%s_%s",
                                                _notify_get_snap_name (),
                                                _notify_get_snap_app ());

                g_debug ("Using desktop entry: %s", snap_desktop);
                g_variant_builder_add (&hints_builder, "{sv}",
                                       NOTIFY_NOTIFICATION_HINT_DESKTOP_ENTRY,
                                       g_variant_new_take_string (snap_desktop));
        }

        if (!_notify_get_snap_app ()) {
                application = g_application_get_default ();
        }

        if (application != NULL) {
            GVariant *desktop_entry = g_hash_table_lookup (priv->hints,
                                                           NOTIFY_NOTIFICATION_HINT_DESKTOP_ENTRY);

            if (desktop_entry == NULL) {
                const char *application_id = g_application_get_application_id (application);

                g_debug ("Using desktop entry: %s", application_id);
                g_variant_builder_add (&hints_builder, "{sv}",
                                       NOTIFY_NOTIFICATION_HINT_DESKTOP_ENTRY,
                                       g_variant_new_string (application_id));
            }
        }

        app_icon = priv->app_icon ? priv->app_icon : notify_get_app_icon ();

        /* Use the icon_name as app icon only before there was a hint for it */
        if (!app_icon && !_notify_check_spec_version (1, 1)) {
            app_icon = priv->icon_name;
        }

        priv->closed_reason = NOTIFY_CLOSED_REASON_UNSET;

        /* TODO: make this nonblocking */
        result = g_dbus_proxy_call_sync (proxy,
                                         "Notify",
                                         g_variant_new ("(susssasa{sv}i)",
                                                        priv->app_name ? priv->app_name : notify_get_app_name (),
                                                        priv->id,
                                                        app_icon ? app_icon : "",
                                                        priv->summary ? priv->summary : "",
                                                        priv->body ? priv->body : "",
                                                        &actions_builder,
                                                        &hints_builder,
                                                        priv->timeout),
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1 /* FIXME ? */,
                                         NULL,
                                         error);
        if (result == NULL) {
                return FALSE;
        }
        if (!g_variant_is_of_type (result, G_VARIANT_TYPE ("(u)"))) {
                g_variant_unref (result);
                g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                             "Unexpected reply type");
                return FALSE;
        }

        g_variant_get (result, "(u)", &priv->id);
        g_variant_unref (result);

        return TRUE;
}

/**
 * notify_notification_set_timeout:
 * @notification: The notification.
 * @timeout: The timeout in milliseconds.
 *
 * Sets the timeout of the notification.
 *
 * To set the default time, pass %NOTIFY_EXPIRES_DEFAULT as @timeout. To set the
 * notification to never expire, pass %NOTIFY_EXPIRES_NEVER.
 *
 * Note that the timeout may be ignored by the server.
 */
void
notify_notification_set_timeout (NotifyNotification *notification,
                                 gint                timeout)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);

        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        priv->timeout = timeout;
}

gint
_notify_notification_get_timeout (const NotifyNotification *notification)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private ((NotifyNotification *) notification);

        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION ((NotifyNotification *) notification), -1);

        return priv->timeout;
}

/**
 * notify_notification_set_category:
 * @notification: The notification.
 * @category: The category.
 *
 * Sets the category of this notification.
 *
 * This can be used by the notification server to filter or display the data in
 * a certain way.
 */
void
notify_notification_set_category (NotifyNotification *notification,
                                  const char         *category)
{
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        if (maybe_warn_portal_unsupported_feature ("Category")) {
                return;
        }

        if (category != NULL && category[0] != '\0') {
                notify_notification_set_hint_string (notification,
                                                     NOTIFY_NOTIFICATION_HINT_CATEGORY,
                                                     category);
        }
}

/**
 * notify_notification_set_urgency:
 * @notification: The notification.
 * @urgency: The urgency level.
 *
 * Sets the urgency level of this notification.
 */
void
notify_notification_set_urgency (NotifyNotification *notification,
                                 NotifyUrgency       urgency)
{
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        notify_notification_set_hint_byte (notification,
                                           NOTIFY_NOTIFICATION_HINT_URGENCY,
                                           (guchar) urgency);
}

/**
 * notify_notification_set_icon_from_pixbuf:
 * @notification: The notification.
 * @icon: The icon.
 *
 * Sets the icon in the notification from a #GdkPixbuf.
 *
 * Deprecated: 0.5. Use [method@Notification.set_image_from_pixbuf] instead.
 */
void
notify_notification_set_icon_from_pixbuf (NotifyNotification *notification,
                                          GdkPixbuf          *icon)
{
        notify_notification_set_image_from_pixbuf (notification, icon);
}

/**
 * notify_notification_set_image_from_pixbuf:
 * @notification: The notification.
 * @pixbuf: The image.
 *
 * Sets the image in the notification from a [class@GdkPixbuf.Pixbuf].
 *
 * Since: 0.5
 */
void
notify_notification_set_image_from_pixbuf (NotifyNotification *notification,
                                           GdkPixbuf          *pixbuf)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);
        gint            width;
        gint            height;
        gint            rowstride;
        gint            bits_per_sample;
        gint            n_channels;
        guchar         *image;
        gboolean        has_alpha;
        gsize           image_len;
        GVariant       *value;

        g_return_if_fail (pixbuf == NULL || GDK_IS_PIXBUF (pixbuf));

        g_clear_object (&priv->icon_pixbuf);

        if (pixbuf == NULL) {
                notify_notification_set_hint (notification,
                                              NOTIFY_NOTIFICATION_HINT_IMAGE_DATA,
                                              NULL);
                return;
        }

        if (_notify_uses_portal_notifications ()) {
                priv->icon_pixbuf = g_object_ref (pixbuf);
                return;
        }

        g_object_get (pixbuf,
                      "width", &width,
                      "height", &height,
                      "rowstride", &rowstride,
                      "n-channels", &n_channels,
                      "bits-per-sample", &bits_per_sample,
                      "pixels", &image,
                      "has-alpha", &has_alpha,
                      NULL);
        image_len = (height - 1) * rowstride + width *
                ((n_channels * bits_per_sample + 7) / 8);

        value = g_variant_new ("(iiibii@ay)",
                               width,
                               height,
                               rowstride,
                               has_alpha,
                               bits_per_sample,
                               n_channels,
                               g_variant_new_from_data (G_VARIANT_TYPE ("ay"),
                                                        image,
                                                        image_len,
                                                        TRUE,
                                                        (GDestroyNotify) g_object_unref,
                                                        g_object_ref (pixbuf)));
        notify_notification_set_hint (notification,
                                      NOTIFY_NOTIFICATION_HINT_IMAGE_DATA,
                                      value);
}

typedef gchar * (*StringParserFunc) (NotifyNotification *, const gchar *);

static GVariant *
get_parsed_variant (NotifyNotification *notification,
                    const char         *key,
                    GVariant           *variant,
                    StringParserFunc    str_parser)
{
        const char *str = g_variant_get_string (variant, NULL);
        gchar *parsed = str_parser (notification, str);

        if (parsed != NULL && g_strcmp0 (str, parsed) != 0) {
                g_debug ("Hint %s updated in snap environment: '%s' -> '%s'\n",
                         key, str, parsed);
                g_variant_unref (variant);
                variant = g_variant_new_take_string (parsed);
        }

        return variant;
}

static GVariant *
maybe_parse_snap_hint_value (NotifyNotification *notification,
                             const gchar *key,
                             GVariant    *value)
{
        StringParserFunc parse_func = NULL;

        if (!_notify_get_snap_path ())
                return value;

        if (g_strcmp0 (key, NOTIFY_NOTIFICATION_HINT_DESKTOP_ENTRY) == 0) {
                parse_func = try_prepend_snap_desktop;
        } else if (g_strcmp0 (key, NOTIFY_NOTIFICATION_HINT_IMAGE_PATH) == 0 ||
                   g_strcmp0 (key, NOTIFY_NOTIFICATION_HINT_IMAGE_PATH_LEGACY) == 0 ||
                   g_strcmp0 (key, NOTIFY_NOTIFICATION_HINT_SOUND_FILE) == 0) {
                parse_func = try_prepend_snap;
        }

        if (parse_func == NULL) {
                return value;
        }

        return get_parsed_variant (notification, key, value, parse_func);
}

/**
 * notify_notification_set_hint:
 * @notification: a #NotifyNotification
 * @key: the hint key
 * @value: (nullable): the hint value
 *
 * Sets a hint for @key with value @value.
 *
 * If @value is %NULL, a previously set hint for @key is unset.
 *
 * If @value is floating, it is consumed.
 *
 * Since: 0.6
 */
void
notify_notification_set_hint (NotifyNotification *notification,
                              const char         *key,
                              GVariant           *value)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);

        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));
        g_return_if_fail (key != NULL && *key != '\0');

        if (value != NULL) {
                value = maybe_parse_snap_hint_value (notification, key, value);
                g_hash_table_insert (priv->hints,
                                     g_strdup (key),
                                     g_variant_ref_sink (value));
        } else {
                g_hash_table_remove (priv->hints, key);
        }
}

/**
 * notify_notification_set_app_name:
 * @notification: a #NotifyNotification
 * @app_name: (nullable): the localised application name
 *
 * Sets the application name for the notification.
 *
 * If this function is not called or if @app_name is %NULL, the application name
 * will be set from the value used in [func@init] or overridden with
 * [func@set_app_name].
 *
 * Since: 0.7.3
 */
void
notify_notification_set_app_name (NotifyNotification *notification,
                                  const char         *app_name)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);

        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        if (_notify_uses_portal_notifications ()) {
                return;
        }

        g_free (priv->app_name);
        priv->app_name = g_strdup (app_name);

        g_object_notify_by_pspec (G_OBJECT (notification), properties[PROP_APP_NAME]);
}

/**
 * notify_notification_set_app_icon:
 * @notification: a #NotifyNotification
 * @app_icon: (nullable): The optional icon theme icon name or filename.
 *
 * Sets the application icon for the notification.
 *
 * If this function is not called or if @app_icon is %NULL, the application icon
 * will be set from the value set via [func@set_app_icon].
 *
 * Since: 0.8.4
 */
void
notify_notification_set_app_icon (NotifyNotification *notification,
                                  const char         *app_icon)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        if (maybe_warn_portal_unsupported_feature ("App Icon")) {
                return;
        }

        g_free (priv->app_icon);
        priv->app_icon = g_strdup (app_icon);

        g_object_notify_by_pspec (G_OBJECT (notification), properties[PROP_APP_ICON]);
}


/**
 * notify_notification_set_hint_int32:
 * @notification: The notification.
 * @key: The hint.
 * @value: The hint's value.
 *
 * Sets a hint with a 32-bit integer value.
 *
 * Deprecated: 0.6. Use [method@Notification.set_hint] instead
 */
void
notify_notification_set_hint_int32 (NotifyNotification *notification,
                                    const char         *key,
                                    gint                value)
{
        notify_notification_set_hint (notification, key,
                                      g_variant_new_int32 (value));
}


/**
 * notify_notification_set_hint_uint32:
 * @notification: The notification.
 * @key: The hint.
 * @value: The hint's value.
 *
 * Sets a hint with an unsigned 32-bit integer value.
 *
 * Deprecated: 0.6. Use [method@Notification.set_hint] instead
 */
void
notify_notification_set_hint_uint32 (NotifyNotification *notification,
                                     const char         *key,
                                     guint               value)
{
        notify_notification_set_hint (notification, key,
                                      g_variant_new_uint32 (value));
}

/**
 * notify_notification_set_hint_double:
 * @notification: The notification.
 * @key: The hint.
 * @value: The hint's value.
 *
 * Sets a hint with a double value.
 *
 * Deprecated: 0.6. Use [method@Notification.set_hint] instead
 */
void
notify_notification_set_hint_double (NotifyNotification *notification,
                                     const char         *key,
                                     gdouble             value)
{
        notify_notification_set_hint (notification, key,
                                      g_variant_new_double (value));
}

/**
 * notify_notification_set_hint_byte:
 * @notification: The notification.
 * @key: The hint.
 * @value: The hint's value.
 *
 * Sets a hint with a byte value.
 *
 * Deprecated: 0.6. Use [method@Notification.set_hint] instead
 */
void
notify_notification_set_hint_byte (NotifyNotification *notification,
                                   const char         *key,
                                   guchar              value)
{
        notify_notification_set_hint (notification, key,
                                      g_variant_new_byte (value));
}

/**
 * notify_notification_set_hint_byte_array:
 * @notification: The notification.
 * @key: The hint.
 * @value: (array length=len): The hint's value.
 * @len: The length of the byte array.
 *
 * Sets a hint with a byte array value.
 *
 * The length of @value must be passed as @len.
 *
 * Deprecated: 0.6. Use [method@Notification.set_hint] instead
 */
void
notify_notification_set_hint_byte_array (NotifyNotification *notification,
                                         const char         *key,
                                         const guchar       *value,
                                         gsize               len)
{
        gpointer value_dup;

        g_return_if_fail (value != NULL || len == 0);

#ifdef GLIB_VERSION_2_68
        value_dup = g_memdup2 (value, len);
#else
        value_dup = g_memdup (value, len);
#endif
        notify_notification_set_hint (notification, key,
                                      g_variant_new_from_data (G_VARIANT_TYPE ("ay"),
                                                               value_dup,
                                                               len,
                                                               TRUE,
                                                               g_free,
                                                               value_dup));
}

/**
 * notify_notification_set_hint_string:
 * @notification: The notification.
 * @key: The hint.
 * @value: The hint's value.
 *
 * Sets a hint with a string value.
 *
 * Deprecated: 0.6. Use [method@Notification.set_hint] instead
 */
void
notify_notification_set_hint_string (NotifyNotification *notification,
                                     const char         *key,
                                     const char         *value)
{
        if (value != NULL && value[0] != '\0') {
                notify_notification_set_hint (notification,
                                              key,
                                              g_variant_new_string (value));
        }
}

/**
 * notify_notification_clear_hints:
 * @notification: The notification.
 *
 * Clears all hints from the notification.
 */
void
notify_notification_clear_hints (NotifyNotification *notification)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);

        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        g_hash_table_remove_all (priv->hints);
}

/**
 * notify_notification_clear_actions:
 * @notification: The notification.
 *
 * Clears all actions from the notification.
 */
void
notify_notification_clear_actions (NotifyNotification *notification)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);

        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        g_hash_table_remove_all (priv->action_map);

        priv->has_nondefault_actions = FALSE;
}

/**
 * notify_notification_add_action:
 * @notification: The notification.
 * @action: The action ID.
 * @label: The human-readable action label.
 * @callback: The action's callback function.
 * @user_data: Optional custom data to pass to @callback.
 * @free_func: (type GLib.DestroyNotify): An optional function to free @user_data when the notification
 *   is destroyed.
 *
 * Adds an action to a notification.
 *
 * When the action is invoked, the specified callback function will be called,
 * along with the value passed to @user_data.
 */
void
notify_notification_add_action (NotifyNotification  *notification,
                                const char          *action,
                                const char          *label,
                                NotifyActionCallback callback,
                                gpointer             user_data,
                                GFreeFunc            free_func)
{
        NotifyNotificationPrivate *priv;
        ActionInfo              *action_info;

        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));
        g_return_if_fail (action != NULL && *action != '\0');
        g_return_if_fail (label != NULL && *label != '\0');
        g_return_if_fail (callback != NULL);

        priv = notify_notification_get_instance_private (notification);

        action_info = g_new0 (ActionInfo, 1);
        action_info->cb = callback;
        action_info->user_data = user_data;
        action_info->free_func = free_func;
        action_info->label = g_strdup (label);
        g_hash_table_insert (priv->action_map, g_strdup (action), action_info);

        if (!priv->has_nondefault_actions &&
            g_ascii_strcasecmp (action, "default") != 0) {
                priv->has_nondefault_actions = TRUE;
        }
}

/**
 * notify_notification_get_activation_token:
 * @notification: The notification.
 *
 * Gets the activation token of the notification.
 *
 * If an an action is currently being activated, return the activation token.
 * This function is intended to be used in a [callback@ActionCallback] to get
 * the activation token for the activated action, if the notification daemon
 * supports it.
 *
 * Returns: (nullable) (transfer none): The current activation token, or %NULL if none
 *
 * Since: 0.7.10
 */
const char *
notify_notification_get_activation_token (NotifyNotification *notification)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);

        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (notification), NULL);
        g_return_val_if_fail (priv->activating, NULL);

        return priv->activation_token;
}

/**
 * notify_notification_get_activation_app_launch_context:
 * @notification: The notification.
 *
 * Gets an application launch context for the notification action
 * activation.
 *
 * If an an action is currently being activated, gets a
 * a [class@Gio.AppLaunchContext] that can be used to launch applications using
 * the current activation token (see [method@Notification.get_activation_token]).
 *
 * This function is intended to be used in a [callback@ActionCallback] to get
 * the launch context for the activated action, if the notification daemon
 * supports it.
 *
 * Returns: (nullable) (transfer full): The [class@Gio.AppLaunchContext] for
 *  the current activation token, or %NULL if unset
 *
 * Since: 0.8.7
 */
GAppLaunchContext *
notify_notification_get_activation_app_launch_context (NotifyNotification *notification)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private (notification);

        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (notification), NULL);
        g_return_val_if_fail (priv->activating, NULL);

        return notification_app_launch_context_new (notification);
}

gboolean
_notify_notification_has_nondefault_actions (const NotifyNotification *n)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private ((NotifyNotification *) n);

        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION ((NotifyNotification *) n), FALSE);

        return priv->has_nondefault_actions;
}

/**
 * notify_notification_close:
 * @notification: The notification.
 * @error: The returned error information.
 *
 * Synchronously tells the notification server to hide the notification on the screen.
 *
 * Returns: %TRUE on success, or %FALSE on error with @error filled in
 */
gboolean
notify_notification_close (NotifyNotification *notification,
                           GError            **error)
{
        NotifyNotificationPrivate *priv;
        GDBusProxy  *proxy;
        GVariant   *result;

        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (notification), FALSE);
        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

        priv = notify_notification_get_instance_private (notification);

        proxy = _notify_get_proxy (error);
        if (proxy == NULL) {
                return FALSE;
        }

        if (_notify_uses_portal_notifications ()) {
                return remove_portal_notification (proxy, notification,
                                                   NOTIFY_CLOSED_REASON_API_REQUEST,
                                                   error);
        }

        /* FIXME: make this nonblocking! */
        result = g_dbus_proxy_call_sync (proxy,
                                         "CloseNotification",
                                         g_variant_new ("(u)", priv->id),
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1 /* FIXME! */,
                                         NULL,
                                         error);
        if (result == NULL) {
                return FALSE;
        }

        g_variant_unref (result);

        return TRUE;
}

/**
 * notify_notification_get_closed_reason:
 * @notification: The notification.
 *
 * Returns the closed reason code for the notification.
 *
 * This is valid only after the [signal@Notification::closed] signal is emitted.
 *
 * Since version 0.8.0 the returned value is of type [enum@ClosedReason].
 *
 * Returns: An integer representing the closed reason code
 *   (Since 0.8.0 it's also a [enum@ClosedReason]).
 */
gint
notify_notification_get_closed_reason (const NotifyNotification *notification)
{
        NotifyNotificationPrivate *priv =
                notify_notification_get_instance_private ((NotifyNotification *) notification);

        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION ((NotifyNotification *) notification),
                              NOTIFY_CLOSED_REASON_UNSET);

        return priv->closed_reason;
}
