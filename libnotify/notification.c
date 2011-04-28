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

#include "notify.h"
#include "internal.h"


/**
 * SECTION:notification
 * @Short_description: A passive pop-up notification.
 * @Title: NotifyNotification
 *
 * #NotifyNotification represents a passive pop-up notification. It can
 * contain summary text, body text, and an icon, as well as hints specifying
 * how the notification should be presented. The notification is rendered
 * by a notification daemon, and may present the notification in any number
 * of ways. As such, there is a clear separation of content and presentation,
 * and this API enforces that.
 */


#if !defined(G_PARAM_STATIC_NAME) && !defined(G_PARAM_STATIC_NICK) && \
    !defined(G_PARAM_STATIC_BLURB)
# define G_PARAM_STATIC_NAME 0
# define G_PARAM_STATIC_NICK 0
# define G_PARAM_STATIC_BLURB 0
#endif

static void     notify_notification_class_init (NotifyNotificationClass *klass);
static void     notify_notification_init       (NotifyNotification *sp);
static void     notify_notification_finalize   (GObject            *object);

typedef struct
{
        NotifyActionCallback cb;
        GFreeFunc            free_func;
        gpointer             user_data;

} CallbackPair;

struct _NotifyNotificationPrivate
{
        guint32         id;
        char           *app_name;
        char           *summary;
        char           *body;

        /* NULL to use icon data. Anything else to have server lookup icon */
        char           *icon_name;

        /*
         * -1   = use server default
         *  0   = never timeout
         *  > 0 = Number of milliseconds before we timeout
         */
        gint            timeout;

        GSList         *actions;
        GHashTable     *action_map;
        GHashTable     *hints;

        gboolean        has_nondefault_actions;
        gboolean        updates_pending;

        gulong          proxy_signal_handler;

        gint            closed_reason;
};

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
        PROP_SUMMARY,
        PROP_BODY,
        PROP_ICON_NAME,
        PROP_CLOSED_REASON
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

static GObjectClass *parent_class = NULL;

G_DEFINE_TYPE (NotifyNotification, notify_notification, G_TYPE_OBJECT)

static GObject *
notify_notification_constructor (GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_params)
{
        GObject *object;

        object = parent_class->constructor (type,
                                            n_construct_properties,
                                            construct_params);

        _notify_cache_add_notification (NOTIFY_NOTIFICATION (object));

        return object;
}

static void
notify_notification_class_init (NotifyNotificationClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->constructor = notify_notification_constructor;
        object_class->get_property = notify_notification_get_property;
        object_class->set_property = notify_notification_set_property;
        object_class->finalize = notify_notification_finalize;

        /**
	 * NotifyNotification::closed:
	 * @notification: The object which received the signal.
	 *
	 * Emitted when the notification is closed.
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

        g_object_class_install_property (object_class,
                                         PROP_ID,
                                         g_param_spec_int ("id", "ID",
                                                           "The notification ID",
                                                           0,
                                                           G_MAXINT32,
                                                           0,
                                                           G_PARAM_READWRITE
                                                           | G_PARAM_CONSTRUCT
                                                           | G_PARAM_STATIC_NAME
                                                           | G_PARAM_STATIC_NICK
                                                           | G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_APP_NAME,
                                         g_param_spec_string ("app-name",
                                                              "Application name",
                                                              "The application name to use for this notification",
                                                              NULL,
                                                              G_PARAM_READWRITE
                                                              | G_PARAM_STATIC_NAME
                                                              | G_PARAM_STATIC_NICK
                                                              | G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_SUMMARY,
                                         g_param_spec_string ("summary",
                                                              "Summary",
                                                              "The summary text",
                                                              NULL,
                                                              G_PARAM_READWRITE
                                                              | G_PARAM_CONSTRUCT
                                                              | G_PARAM_STATIC_NAME
                                                              | G_PARAM_STATIC_NICK
                                                              | G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_BODY,
                                         g_param_spec_string ("body",
                                                              "Message Body",
                                                              "The message body text",
                                                              NULL,
                                                              G_PARAM_READWRITE
                                                              | G_PARAM_CONSTRUCT
                                                              | G_PARAM_STATIC_NAME
                                                              | G_PARAM_STATIC_NICK
                                                              | G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_ICON_NAME,
                                         g_param_spec_string ("icon-name",
                                                              "Icon Name",
                                                              "The icon filename or icon theme-compliant name",
                                                              NULL,
                                                              G_PARAM_READWRITE
                                                              | G_PARAM_CONSTRUCT
                                                              | G_PARAM_STATIC_NAME
                                                              | G_PARAM_STATIC_NICK
                                                              | G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_CLOSED_REASON,
                                         g_param_spec_int ("closed-reason",
                                                           "Closed Reason",
                                                           "The reason code for why the notification was closed",
                                                           -1,
                                                           G_MAXINT32,
                                                           -1,
                                                           G_PARAM_READABLE
                                                           | G_PARAM_STATIC_NAME
                                                           | G_PARAM_STATIC_NICK
                                                           | G_PARAM_STATIC_BLURB));
}

static void
notify_notification_update_internal (NotifyNotification *notification,
                                     const char         *app_name,
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
        NotifyNotificationPrivate *priv = notification->priv;

        switch (prop_id) {
        case PROP_ID:
                priv->id = g_value_get_int (value);
                break;

        case PROP_APP_NAME:
                notify_notification_update_internal (notification,
                                                     g_value_get_string (value),
                                                     priv->summary,
                                                     priv->body,
                                                     priv->icon_name);
                break;

        case PROP_SUMMARY:
                notify_notification_update_internal (notification,
                                                     priv->app_name,
                                                     g_value_get_string (value),
                                                     priv->body,
                                                     priv->icon_name);
                break;

        case PROP_BODY:
                notify_notification_update_internal (notification,
                                                     priv->app_name,
                                                     priv->summary,
                                                     g_value_get_string (value),
                                                     priv->icon_name);
                break;

        case PROP_ICON_NAME:
                notify_notification_update_internal (notification,
                                                     priv->app_name,
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
        NotifyNotificationPrivate *priv = notification->priv;

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
destroy_pair (CallbackPair *pair)
{
        if (pair->user_data != NULL && pair->free_func != NULL) {
                pair->free_func (pair->user_data);
        }

        g_free (pair);
}

static void
notify_notification_init (NotifyNotification *obj)
{
        obj->priv = g_new0 (NotifyNotificationPrivate, 1);
        obj->priv->timeout = NOTIFY_EXPIRES_DEFAULT;
        obj->priv->closed_reason = -1;
        obj->priv->hints = g_hash_table_new_full (g_str_hash,
                                                  g_str_equal,
                                                  g_free,
                                                  (GDestroyNotify) g_variant_unref);

        obj->priv->action_map = g_hash_table_new_full (g_str_hash,
                                                       g_str_equal,
                                                       g_free,
                                                       (GDestroyNotify) destroy_pair);
}

static void
notify_notification_finalize (GObject *object)
{
        NotifyNotification        *obj = NOTIFY_NOTIFICATION (object);
        NotifyNotificationPrivate *priv = obj->priv;
        GDBusProxy                *proxy;

        _notify_cache_remove_notification (obj);

        g_free (priv->app_name);
        g_free (priv->summary);
        g_free (priv->body);
        g_free (priv->icon_name);

        if (priv->actions != NULL) {
                g_slist_foreach (priv->actions, (GFunc) g_free, NULL);
                g_slist_free (priv->actions);
        }

        if (priv->action_map != NULL)
                g_hash_table_destroy (priv->action_map);

        if (priv->hints != NULL)
                g_hash_table_destroy (priv->hints);

        proxy = _notify_get_proxy (NULL);
        if (proxy != NULL && priv->proxy_signal_handler != 0) {
                g_signal_handler_disconnect (proxy, priv->proxy_signal_handler);
        }

        g_free (obj->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * notify_notification_new:
 * @summary: The required summary text.
 * @body: (allow-none): The optional body text.
 * @icon: (allow-none): The optional icon theme icon name or filename.
 *
 * Creates a new #NotifyNotification. The summary text is required, but
 * all other parameters are optional.
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

static void
notify_notification_update_internal (NotifyNotification *notification,
                                     const char         *app_name,
                                     const char         *summary,
                                     const char         *body,
                                     const char         *icon)
{
        if (notification->priv->app_name != app_name) {
                g_free (notification->priv->app_name);
                notification->priv->app_name = g_strdup (app_name);
                g_object_notify (G_OBJECT (notification), "app-name");
        }

        if (notification->priv->summary != summary) {
                g_free (notification->priv->summary);
                notification->priv->summary = g_strdup (summary);
                g_object_notify (G_OBJECT (notification), "summary");
        }

        if (notification->priv->body != body) {
                g_free (notification->priv->body);
                notification->priv->body = (body != NULL
                                            && *body != '\0' ? g_strdup (body) : NULL);
                g_object_notify (G_OBJECT (notification), "body");
        }

        if (notification->priv->icon_name != icon) {
                g_free (notification->priv->icon_name);
                notification->priv->icon_name = (icon != NULL
                                                 && *icon != '\0' ? g_strdup (icon) : NULL);
                g_object_notify (G_OBJECT (notification), "icon-name");
        }

        notification->priv->updates_pending = TRUE;
}

/**
 * notify_notification_update:
 * @notification: The notification to update.
 * @summary: The new required summary text.
 * @body: (allow-none): The optional body text.
 * @icon: (allow-none): The optional icon theme icon name or filename.
 *
 * Updates the notification text and icon. This won't send the update out
 * and display it on the screen. For that, you will need to call
 * notify_notification_show().
 *
 * Returns: %TRUE, unless an invalid parameter was passed.
 */
gboolean
notify_notification_update (NotifyNotification *notification,
                            const char         *summary,
                            const char         *body,
                            const char         *icon)
{
        g_return_val_if_fail (notification != NULL, FALSE);
        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (notification), FALSE);
        g_return_val_if_fail (summary != NULL && *summary != '\0', FALSE);

        notify_notification_update_internal (notification,
                                             notification->priv->app_name,
                                             summary, body, icon);

        return TRUE;
}

static void
proxy_g_signal_cb (GDBusProxy *proxy,
                   const char *sender_name,
                   const char *signal_name,
                   GVariant   *parameters,
                   NotifyNotification *notification)
{
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        if (g_strcmp0 (signal_name, "NotificationClosed") == 0 &&
            g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(uu)"))) {
                guint32 id, reason;

                g_variant_get (parameters, "(uu)", &id, &reason);
                if (id != notification->priv->id)
                        return;

                g_object_ref (G_OBJECT (notification));
                notification->priv->closed_reason = reason;
                g_signal_emit (notification, signals[SIGNAL_CLOSED], 0);
                notification->priv->id = 0;
                g_object_unref (G_OBJECT (notification));
        } else if (g_strcmp0 (signal_name, "ActionInvoked") == 0 &&
                   g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(us)"))) {
                guint32 id;
                const char *action;
                CallbackPair *pair;

                g_variant_get (parameters, "(u&s)", &id, &action);

                if (id != notification->priv->id)
                        return;

                pair = (CallbackPair *) g_hash_table_lookup (notification->priv->action_map,
                                                            action);

                if (pair == NULL) {
                        if (g_ascii_strcasecmp (action, "default")) {
                                g_warning ("Received unknown action %s", action);
                        }
                } else {
                        pair->cb (notification, (char *) action, pair->user_data);
                }
        }
}

/**
 * notify_notification_show:
 * @notification: The notification.
 * @error: The returned error information.
 *
 * Tells the notification server to display the notification on the screen.
 *
 * Returns: %TRUE if successful. On error, this will return %FALSE and set
 *          @error.
 */
gboolean
notify_notification_show (NotifyNotification *notification,
                          GError            **error)
{
        NotifyNotificationPrivate *priv;
        GDBusProxy                *proxy;
        GVariantBuilder            actions_builder, hints_builder;
        GSList                    *l;
        GHashTableIter             iter;
        gpointer                   key, data;
        GVariant                  *result;

        g_return_val_if_fail (notification != NULL, FALSE);
        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (notification), FALSE);
        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

        if (!notify_is_initted ()) {
                g_warning ("you must call notify_init() before showing");
                g_assert_not_reached ();
        }

        priv = notification->priv;
        proxy = _notify_get_proxy (error);
        if (proxy == NULL) {
                return FALSE;
        }

        if (priv->proxy_signal_handler == 0) {
                priv->proxy_signal_handler = g_signal_connect (proxy,
                                                               "g-signal",
                                                               G_CALLBACK (proxy_g_signal_cb),
                                                               notification);
        }

        g_variant_builder_init (&actions_builder, G_VARIANT_TYPE ("as"));
        for (l = priv->actions; l != NULL; l = l->next) {
                g_variant_builder_add (&actions_builder, "s", l->data);
        }

        g_variant_builder_init (&hints_builder, G_VARIANT_TYPE ("a{sv}"));
        g_hash_table_iter_init (&iter, priv->hints);
        while (g_hash_table_iter_next (&iter, &key, &data)) {
                g_variant_builder_add (&hints_builder, "{sv}", key, data);
        }

        /* TODO: make this nonblocking */
        result = g_dbus_proxy_call_sync (proxy,
                                         "Notify",
                                         g_variant_new ("(susssasa{sv}i)",
                                                        priv->app_name ? priv->app_name : notify_get_app_name (),
                                                        priv->id,
                                                        priv->icon_name ? priv->icon_name : "",
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
 * Sets the timeout of the notification. To set the default time, pass
 * %NOTIFY_EXPIRES_DEFAULT as @timeout. To set the notification to never
 * expire, pass %NOTIFY_EXPIRES_NEVER.
 */
void
notify_notification_set_timeout (NotifyNotification *notification,
                                 gint                timeout)
{
        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        notification->priv->timeout = timeout;
}

gint
_notify_notification_get_timeout (const NotifyNotification *notification)
{
        g_return_val_if_fail (notification != NULL, -1);
        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (notification), -1);

        return notification->priv->timeout;
}

/**
 * notify_notification_set_category:
 * @notification: The notification.
 * @category: The category.
 *
 * Sets the category of this notification. This can be used by the
 * notification server to filter or display the data in a certain way.
 */
void
notify_notification_set_category (NotifyNotification *notification,
                                  const char         *category)
{
        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        if (category != NULL && category[0] != '\0') {
                notify_notification_set_hint_string (notification,
                                                     "category",
                                                     category);
        }
}

/**
 * notify_notification_set_urgency:
 * @notification: The notification.
 * @urgency: The urgency level.
 *
 * Sets the urgency level of this notification.
 *
 * See: #NotifyUrgency
 */
void
notify_notification_set_urgency (NotifyNotification *notification,
                                 NotifyUrgency       urgency)
{
        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        notify_notification_set_hint_byte (notification,
                                           "urgency",
                                           (guchar) urgency);
}

/**
 * notify_notification_set_icon_from_pixbuf:
 * @notification: The notification.
 * @icon: The icon.
 *
 * Sets the icon in the notification from a #GdkPixbuf.
 * Deprecated: use notify_notification_set_image_from_pixbuf() instead.
 *
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
 * Sets the image in the notification from a #GdkPixbuf.
 *
 */
void
notify_notification_set_image_from_pixbuf (NotifyNotification *notification,
                                           GdkPixbuf          *pixbuf)
{
        gint            width;
        gint            height;
        gint            rowstride;
        gint            bits_per_sample;
        gint            n_channels;
        guchar         *image;
        gboolean        has_alpha;
        gsize           image_len;
        GVariant       *value;
        const char     *hint_name;

        g_return_if_fail (pixbuf == NULL || GDK_IS_PIXBUF (pixbuf));

        if (_notify_check_spec_version(1, 2)) {
                hint_name = "image-data";
        } else if (_notify_check_spec_version(1, 1)) {
                hint_name = "image_data";
        } else {
                hint_name = "icon_data";
        }

        if (pixbuf == NULL) {
                notify_notification_set_hint (notification, hint_name, NULL);
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
        notify_notification_set_hint (notification, hint_name, value);
}

/**
 * notify_notification_set_hint:
 * @notification: a #NotifyNotification
 * @key: the hint key
 * @value: (allow-none): the hint value, or %NULL to unset the hint
 *
 * Sets a hint for @key with value @value. If @value is %NULL,
 * a previously set hint for @key is unset.
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
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));
        g_return_if_fail (key != NULL && *key != '\0');

        if (value != NULL) {
                g_hash_table_insert (notification->priv->hints,
                                    g_strdup (key),
                                    g_variant_ref_sink (value));
        } else {
                g_hash_table_remove (notification->priv->hints, key);
        }
}

/**
 * notify_notification_set_app_name:
 * @notification: a #NotifyNotification
 * @app_name: the localised application name
 *
 * Sets the application name for the notification. If this function is
 * not called or if @app_name is %NULL, the application name will be
 * set from the value used in notify_init() or overridden with
 * notify_set_app_name().
 *
 * Since: 0.7.3
 */
void
notify_notification_set_app_name (NotifyNotification *notification,
                                  const char         *app_name)
{
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        g_free (notification->priv->app_name);
        notification->priv->app_name = g_strdup (app_name);

        g_object_notify (G_OBJECT (notification), "app-name");
}

/**
 * notify_notification_set_hint_int32:
 * @notification: The notification.
 * @key: The hint.
 * @value: The hint's value.
 *
 * Sets a hint with a 32-bit integer value.
 *
 * Deprecated: 0.6. Use notify_notification_set_hint() instead
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
 * Deprecated: 0.6. Use notify_notification_set_hint() instead
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
 * Deprecated: 0.6. Use notify_notification_set_hint() instead
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
 * Deprecated: 0.6. Use notify_notification_set_hint() instead
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
 * @value: The hint's value.
 * @len: The length of the byte array.
 *
 * Sets a hint with a byte array value. The length of @value must be passed
 * as @len.
 *
 * Deprecated: 0.6. Use notify_notification_set_hint() instead
 */
void
notify_notification_set_hint_byte_array (NotifyNotification *notification,
                                         const char         *key,
                                         const guchar       *value,
                                         gsize               len)
{
        gpointer value_dup;

        g_return_if_fail (value != NULL || len == 0);

        value_dup = g_memdup (value, len);
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
 * Deprecated: 0.6. Use notify_notification_set_hint() instead
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

static gboolean
_remove_all (void)
{
        return TRUE;
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
        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        g_hash_table_foreach_remove (notification->priv->hints,
                                     (GHRFunc) _remove_all,
                                     NULL);
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
        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        g_hash_table_foreach_remove (notification->priv->action_map,
                                     (GHRFunc) _remove_all,
                                     NULL);

        if (notification->priv->actions != NULL) {
                g_slist_foreach (notification->priv->actions,
                                 (GFunc) g_free,
                                 NULL);
                g_slist_free (notification->priv->actions);
        }

        notification->priv->actions = NULL;
        notification->priv->has_nondefault_actions = FALSE;
}

/**
 * notify_notification_add_action:
 * @notification: The notification.
 * @action: The action ID.
 * @label: The human-readable action label.
 * @callback: (scope async): The action's callback function.
 * @user_data: (allow-none): Optional custom data to pass to @callback.
 * @free_func: (scope async) (allow-none): An optional function to free @user_data when the notification
 *             is destroyed.
 *
 * Adds an action to a notification. When the action is invoked, the
 * specified callback function will be called, along with the value passed
 * to @user_data.
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
        CallbackPair              *pair;

        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));
        g_return_if_fail (action != NULL && *action != '\0');
        g_return_if_fail (label != NULL && *label != '\0');
        g_return_if_fail (callback != NULL);

        priv = notification->priv;

        priv->actions = g_slist_append (priv->actions, g_strdup (action));
        priv->actions = g_slist_append (priv->actions, g_strdup (label));

        pair = g_new0 (CallbackPair, 1);
        pair->cb = callback;
        pair->user_data = user_data;
        pair->free_func = free_func;
        g_hash_table_insert (priv->action_map, g_strdup (action), pair);

        if (!notification->priv->has_nondefault_actions &&
            g_ascii_strcasecmp (action, "default") != 0) {
                notification->priv->has_nondefault_actions = TRUE;
        }
}

gboolean
_notify_notification_has_nondefault_actions (const NotifyNotification *n)
{
        g_return_val_if_fail (n != NULL, FALSE);
        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (n), FALSE);

        return n->priv->has_nondefault_actions;
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

        priv = notification->priv;

        proxy = _notify_get_proxy (error);
        if (proxy == NULL) {
                return FALSE;
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
 * Returns the closed reason code for the notification. This is valid only
 * after the "closed" signal is emitted.
 *
 * Returns: The closed reason code.
 */
gint
notify_notification_get_closed_reason (const NotifyNotification *notification)
{
        g_return_val_if_fail (notification != NULL, -1);
        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (notification), -1);

        return notification->priv->closed_reason;
}
