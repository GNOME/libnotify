/**
 * @file libnotify/notification.c Notification object
 *
 * @Copyright (C) 2006 Christian Hammond
 * @Copyright (C) 2006 John Palmieri
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
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include <libnotify/notify.h>
#include <libnotify/internal.h>

#define CHECK_DBUS_VERSION(major, minor) \
	(DBUS_MAJOR_VER > (major) || \
	 (DBUS_MAJOR_VER == (major) && DBUS_MINOR_VER >= (minor)))

static void notify_notification_class_init(NotifyNotificationClass *klass);
static void notify_notification_init(NotifyNotification *sp);
static void notify_notification_finalize(GObject *object);
static void _close_signal_handler(DBusGProxy *proxy, guint32 id,
								  NotifyNotification *notification);

static void _action_signal_handler(DBusGProxy *proxy, guint32 id,
								   gchar *action,
								   NotifyNotification *notification);

typedef struct
{
	NotifyActionCallback cb;
	GFreeFunc free_func;
	gpointer user_data;

} CallbackPair;

struct _NotifyNotificationPrivate
{
	guint32 id;
	gchar *summary;
	gchar *body;

	/* NULL to use icon data. Anything else to have server lookup icon */
	gchar *icon_name;

	/*
	 * -1   = use server default
	 *  0   = never timeout
	 *  > 0 = Number of milliseconds before we timeout
    */
	gint timeout;

	GSList *actions;
	GHashTable *action_map;
	GHashTable *hints;

	GtkWidget *attached_widget;
	gint widget_old_x;
	gint widget_old_y;

	gboolean has_nondefault_actions;
	gboolean updates_pending;
	gboolean signals_registered;
};

enum
{
	SIGNAL_CLOSED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };
static GObjectClass *parent_class = NULL;

G_DEFINE_TYPE(NotifyNotification, notify_notification, G_TYPE_OBJECT);

static void
notify_notification_class_init(NotifyNotificationClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);

	object_class->finalize = notify_notification_finalize;

	/* Create signals here: */
	signals[SIGNAL_CLOSED] =
		g_signal_new("closed",
					 G_TYPE_FROM_CLASS(object_class),
					 G_SIGNAL_RUN_FIRST,
					 G_STRUCT_OFFSET(NotifyNotificationClass, closed),
					 NULL, NULL,
					 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
_g_value_free(GValue *value)
{
	g_value_unset(value);
	g_free(value);
}

static void
destroy_pair(CallbackPair *pair)
{
	if (pair->user_data != NULL && pair->free_func != NULL)
		pair->free_func(pair->user_data);

	g_free(pair);
}

static void
notify_notification_init(NotifyNotification *obj)
{
	obj->priv = g_new0(NotifyNotificationPrivate, 1);
	obj->priv->timeout = NOTIFY_EXPIRES_DEFAULT;
	obj->priv->hints = g_hash_table_new_full(g_str_hash, g_str_equal,
											 g_free,
											 (GFreeFunc)_g_value_free);

	obj->priv->action_map = g_hash_table_new_full(g_str_hash, g_str_equal,
												  g_free,
												  (GFreeFunc)destroy_pair);
}

static void
notify_notification_finalize(GObject *object)
{
	NotifyNotification *obj = NOTIFY_NOTIFICATION(object);
	NotifyNotificationPrivate *priv = obj->priv;
	DBusGProxy *proxy = _notify_get_g_proxy();

	_notify_cache_remove_notification(obj);

	g_free(priv->summary);
	g_free(priv->body);
	g_free(priv->icon_name);

	if (priv->actions != NULL)
	{
		g_slist_foreach(priv->actions, (GFunc)g_free, NULL);
		g_slist_free(priv->actions);
	}

	if (priv->action_map != NULL)
		g_hash_table_destroy(priv->action_map);

	if (priv->hints != NULL)
		g_hash_table_destroy(priv->hints);

	if (priv->attached_widget != NULL)
		g_object_unref(G_OBJECT(priv->attached_widget));

	if (priv->signals_registered)
	{
		dbus_g_proxy_disconnect_signal(proxy, "NotificationClosed",
									   G_CALLBACK(_close_signal_handler),
									   object);
		dbus_g_proxy_disconnect_signal(proxy, "ActionInvoked",
									   G_CALLBACK(_action_signal_handler),
									   object);
	}

	g_free(obj->priv);

	G_OBJECT_CLASS(parent_class)->finalize(object);
}

static gboolean
_notify_notification_update_applet_hints(NotifyNotification *n)
{
	NotifyNotificationPrivate *priv = n->priv;
	gboolean update_pending = FALSE;

	if (priv->attached_widget != NULL)
	{
		gint x, y, h, w;
		GtkWidget *widget = priv->attached_widget;
		GtkRequisition requisition;

		gtk_widget_size_request(widget, &requisition);
		w = requisition.width;
		h = requisition.height;

		gdk_window_get_origin(widget->window, &x, &y);

		if (GTK_WIDGET_NO_WINDOW(widget))
		{
			x += widget->allocation.x;
			y += widget->allocation.y;
		}

		x += widget->allocation.width / 2;
		y += widget->allocation.height / 2;

		if (x != priv->widget_old_x)
		{
			notify_notification_set_hint_int32(n, "x", x);
			priv->widget_old_x = x;

			update_pending = TRUE;
		}

		if (y != priv->widget_old_y)
		{
			notify_notification_set_hint_int32(n, "y", y);
			priv->widget_old_y = y;

			update_pending = TRUE;
		}
	}

	return update_pending;
}

#if 0

/* This is left here just incase we revisit autoupdating
   One thought would be to check for updates every time the icon
   is redrawn but we still have to deal with the race conditions
   that could occure between the server and the client so we will
   leave this alone for now.
*/
static gboolean
_idle_check_updates(void *user_data)
{
	NotifyNotification *n = NOTIFY_NOTIFICATION(user_data);
	NotifyNotificationPrivate *priv = n->priv;

	if (priv->is_visible)
	{
		priv->updates_pending = _notify_notification_update_applet_hints(n);

		if (priv->updates_pending)
		{
			/* Try again if we fail on next idle */
			priv->updates_pending = !notify_notification_show(n, NULL);
		}
	}
	else
	{
		priv->updates_pending = FALSE;
	}

	return TRUE;
}
#endif

NotifyNotification *
notify_notification_new(const gchar *summary, const gchar *body,
						const gchar *icon, GtkWidget *attach)
{
	NotifyNotification *obj;

	g_return_val_if_fail(summary != NULL && *summary != '\0',     NULL);
	g_return_val_if_fail(attach == NULL || GTK_IS_WIDGET(attach), NULL);

	obj = NOTIFY_NOTIFICATION(g_object_new(NOTIFY_TYPE_NOTIFICATION, NULL));

	obj->priv->summary = g_strdup(summary);

	if (body != NULL && *body != '\0')
		obj->priv->body = g_strdup(body);

	if (icon != NULL && *icon != '\0')
		obj->priv->icon_name = g_strdup(icon);

	if (attach != NULL)
	{
		g_object_ref(G_OBJECT(attach));
		obj->priv->attached_widget = attach;
	}

	_notify_cache_add_notification(obj);

	return obj;
}

gboolean
notify_notification_update(NotifyNotification *notification,
						   const gchar *summary, const gchar *body,
						   const gchar *icon)
{
	g_return_val_if_fail(notification != NULL,                 FALSE);
	g_return_val_if_fail(NOTIFY_IS_NOTIFICATION(notification), FALSE);
	g_return_val_if_fail(summary != NULL && *summary != '\0',  FALSE);

	g_free(notification->priv->summary);
	g_free(notification->priv->body);
	g_free(notification->priv->icon_name);

	notification->priv->summary = g_strdup(summary);

	if (body != NULL && *body != '\0')
		notification->priv->body = g_strdup(body);

	if (icon != NULL && *icon != '\0')
		notification->priv->icon_name = g_strdup(icon);

	notification->priv->updates_pending = TRUE;

	return TRUE;
}

void
notify_notification_attach_to_widget(NotifyNotification *notification,
									 GtkWidget *attach)
{
	g_return_if_fail(notification != NULL);
	g_return_if_fail(NOTIFY_IS_NOTIFICATION(notification));

	if (notification->priv->attached_widget != NULL)
		g_object_unref(notification->priv->attached_widget);

	notification->priv->attached_widget =
		(attach != NULL ? g_object_ref(attach) : NULL);
}

static void
_close_signal_handler(DBusGProxy *proxy, guint32 id,
					  NotifyNotification *notification)
{
	if (id == notification->priv->id)
		g_signal_emit(notification, signals[SIGNAL_CLOSED], 0);
}

static void
_action_signal_handler(DBusGProxy *proxy, guint32 id, gchar *action,
					   NotifyNotification *notification)
{
	CallbackPair *pair;

	g_return_if_fail(notification != NULL);
	g_return_if_fail(NOTIFY_IS_NOTIFICATION(notification));

	if (id != notification->priv->id)
		return;

	pair = (CallbackPair *)g_hash_table_lookup(
		notification->priv->action_map, action);

	if (pair == NULL)
	{
		if (g_ascii_strcasecmp(action, "default"))
			g_warning("Received unknown action %s", action);
	}
	else
	{
		pair->cb(notification, action, pair->user_data);
	}
}

static gchar **
_gslist_to_string_array(GSList *list)
{
	GSList *l;
	GArray *a = g_array_sized_new(TRUE, FALSE, sizeof(gchar *),
								  g_slist_length(list));

	for (l = list; l != NULL; l = l->next)
		g_array_append_val(a, l->data);

	return (gchar **)g_array_free(a, FALSE);
}

gboolean
notify_notification_show(NotifyNotification *notification, GError **error)
{
	NotifyNotificationPrivate *priv;
	GError *tmp_error = NULL;
	gchar **action_array;
	DBusGProxy *proxy;

	g_return_val_if_fail(notification != NULL, FALSE);
	g_return_val_if_fail(NOTIFY_IS_NOTIFICATION(notification), FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	priv = notification->priv;
	proxy = _notify_get_g_proxy();

	if (!priv->signals_registered)
	{
		dbus_g_proxy_connect_signal(proxy, "NotificationClosed",
									G_CALLBACK(_close_signal_handler),
									notification, NULL);

		dbus_g_proxy_connect_signal(proxy, "ActionInvoked",
									G_CALLBACK(_action_signal_handler),
									notification, NULL);

		priv->signals_registered = TRUE;
	}

	/* If attached to a widget, modify x and y in hints */
	_notify_notification_update_applet_hints(notification);

	action_array = _gslist_to_string_array(priv->actions);

	/* TODO: make this nonblocking */
	dbus_g_proxy_call(proxy, "Notify", &tmp_error,
					  G_TYPE_STRING, notify_get_app_name(),
					  G_TYPE_UINT, priv->id,
					  G_TYPE_STRING, priv->icon_name,
					  G_TYPE_STRING, priv->summary,
					  G_TYPE_STRING, priv->body,
					  G_TYPE_STRV, action_array,
					  dbus_g_type_get_map("GHashTable", G_TYPE_STRING,
										  G_TYPE_VALUE), priv->hints,
					  G_TYPE_INT, priv->timeout,
					  G_TYPE_INVALID,
					  G_TYPE_UINT, &priv->id,
					  G_TYPE_INVALID);

	/* Don't free the elements because they are owned by priv->actions */
	g_free(action_array);

	if (tmp_error != NULL)
	{
		g_propagate_error(error, tmp_error);
		return FALSE;
	}

	return TRUE;
}

void
notify_notification_set_timeout(NotifyNotification *notification,
								gint timeout)
{
	g_return_if_fail(notification != NULL);
	g_return_if_fail(NOTIFY_IS_NOTIFICATION(notification));

	notification->priv->timeout = timeout;
}

gint
_notify_notification_get_timeout(const NotifyNotification *notification)
{
	g_return_val_if_fail(notification != NULL, -1);
	g_return_val_if_fail(NOTIFY_IS_NOTIFICATION(notification), -1);

	return notification->priv->timeout;
}

void
notify_notification_set_category(NotifyNotification *notification,
								 const char *category)
{
	g_return_if_fail(notification != NULL);
	g_return_if_fail(NOTIFY_IS_NOTIFICATION(notification));

	notify_notification_set_hint_string(notification, "category", category);
}

void
notify_notification_set_urgency(NotifyNotification *notification,
								NotifyUrgency l)
{
	g_return_if_fail(notification != NULL);
	g_return_if_fail(NOTIFY_IS_NOTIFICATION(notification));

	notify_notification_set_hint_byte(notification, "urgency", (guchar)l);
}

#if CHECK_DBUS_VERSION(0, 60)
static gboolean
_gvalue_array_append_int(GValueArray *array, gint i)
{
	GValue *value = g_new0(GValue, 1);

	if (value == NULL)
		return FALSE;

	g_value_init(value, G_TYPE_INT);
	g_value_set_int(value, i);
	g_value_array_append(array, value);

	return TRUE;
}

static gboolean
_gvalue_array_append_bool(GValueArray *array, gboolean b)
{
	GValue *value = g_new0(GValue, 1);

	if (value == NULL)
		return FALSE;

	g_value_init(value, G_TYPE_BOOLEAN);
	g_value_set_boolean(value, b);
	g_value_array_append(array, value);

	return TRUE;
}

static gboolean
_gvalue_array_append_byte_array(GValueArray *array, guchar *bytes, gsize len)
{
	GArray *byte_array;
	GValue *value;

	byte_array = g_array_sized_new(FALSE, FALSE, sizeof(guchar), len);

	if (byte_array == NULL)
		return FALSE;

	byte_array = g_array_append_vals(byte_array, bytes, len);

	if ((value = g_new0(GValue, 1)) == NULL)
	{
		g_array_free(byte_array, TRUE);
		return FALSE;
	}

	g_value_init(value, dbus_g_type_get_collection("GArray", G_TYPE_CHAR));
	g_value_set_boxed_take_ownership(value, byte_array);
	g_value_array_append(array, value);

	return TRUE;
}
#endif /* D-BUS >= 0.60 */

void
notify_notification_set_icon_from_pixbuf(NotifyNotification *notification,
										 GdkPixbuf *icon)
{
#if CHECK_DBUS_VERSION(0, 60)
	gint width;
	gint height;
	gint rowstride;
	gint bits_per_sample;
	gint n_channels;
	guchar *image;
	gsize image_len;
	GValueArray *image_struct;
	GValue *value;
#endif

	g_return_if_fail(notification != NULL);
	g_return_if_fail(NOTIFY_IS_NOTIFICATION(notification));

#if CHECK_DBUS_VERSION(0, 60)
	width           = gdk_pixbuf_get_width(icon);
	height          = gdk_pixbuf_get_height(icon);
	rowstride       = gdk_pixbuf_get_rowstride(icon);
	n_channels      = gdk_pixbuf_get_n_channels(icon);
	bits_per_sample = gdk_pixbuf_get_bits_per_sample(icon);
	image_len       = (height - 1) * rowstride + width *
	                  ((n_channels * bits_per_sample + 7) / 8);

	image = gdk_pixbuf_get_pixels(icon);

	image_struct = g_value_array_new(1);

	_gvalue_array_append_int(image_struct, width);
	_gvalue_array_append_int(image_struct, height);
	_gvalue_array_append_int(image_struct, rowstride);
	_gvalue_array_append_bool(image_struct, gdk_pixbuf_get_has_alpha(icon));
	_gvalue_array_append_int(image_struct, bits_per_sample);
	_gvalue_array_append_int(image_struct, n_channels);
	_gvalue_array_append_byte_array(image_struct, image, image_len);

	value = g_new0(GValue, 1);
	g_value_init(value, G_TYPE_VALUE_ARRAY);
	g_value_set_boxed(value, image_struct);

	g_hash_table_insert(notification->priv->hints,
						g_strdup("icon_data"), value);
#else /* D-BUS < 0.60 */
	g_warning("Raw images and pixbufs require D-BUS >= 0.60");
#endif
}

void
notify_notification_set_hint_int32(NotifyNotification *notification,
								   const gchar *key, gint value)
{
	GValue *hint_value;

	g_return_if_fail(notification != NULL);
	g_return_if_fail(NOTIFY_IS_NOTIFICATION(notification));
	g_return_if_fail(key != NULL && *key != '\0');

	hint_value = g_new0(GValue, 1);
	g_value_init(hint_value, G_TYPE_INT);
	g_value_set_int(hint_value, value);
	g_hash_table_insert(notification->priv->hints,
						g_strdup(key), hint_value);
}

void
notify_notification_set_hint_double(NotifyNotification *notification,
									const gchar *key, gdouble value)
{
	GValue *hint_value;

	g_return_if_fail(notification != NULL);
	g_return_if_fail(NOTIFY_IS_NOTIFICATION(notification));
	g_return_if_fail(key != NULL && *key != '\0');

	hint_value = g_new0(GValue, 1);
	g_value_init(hint_value, G_TYPE_FLOAT);
	g_value_set_float(hint_value, value);
	g_hash_table_insert(notification->priv->hints,
						g_strdup(key), hint_value);
}

void
notify_notification_set_hint_byte(NotifyNotification *notification,
								  const gchar *key, guchar value)
{
	GValue *hint_value;

	g_return_if_fail(notification != NULL);
	g_return_if_fail(NOTIFY_IS_NOTIFICATION(notification));
	g_return_if_fail(key != NULL && *key != '\0');

	hint_value = g_new0(GValue, 1);
	g_value_init(hint_value, G_TYPE_UCHAR);
	g_value_set_uchar(hint_value, value);

	g_hash_table_insert(notification->priv->hints, g_strdup(key), hint_value);
}

void
notify_notification_set_hint_byte_array(NotifyNotification *notification,
										const gchar *key,
										const guchar *value, gsize len)
{
	GValue *hint_value;
	GArray *byte_array;

	g_return_if_fail(notification != NULL);
	g_return_if_fail(NOTIFY_IS_NOTIFICATION(notification));
	g_return_if_fail(key != NULL && *key != '\0');
	g_return_if_fail(value != NULL);
	g_return_if_fail(len > 0);

	byte_array = g_array_sized_new(FALSE, FALSE, sizeof(guchar), len);
	byte_array = g_array_append_vals(byte_array, value, len);

	hint_value = g_new0(GValue, 1);
	g_value_init(hint_value, dbus_g_type_get_collection("GArray",
														G_TYPE_UCHAR));
	g_value_set_boxed_take_ownership(hint_value, byte_array);

	g_hash_table_insert(notification->priv->hints,
						g_strdup(key), hint_value);
}

void
notify_notification_set_hint_string(NotifyNotification *notification,
									const gchar *key, const gchar *value)
{
	GValue *hint_value;

	g_return_if_fail(notification != NULL);
	g_return_if_fail(NOTIFY_IS_NOTIFICATION(notification));
	g_return_if_fail(key != NULL && *key != '\0');

	hint_value = g_new0(GValue, 1);
	g_value_init(hint_value, G_TYPE_STRING);
	g_value_set_string(hint_value, value);
	g_hash_table_insert(notification->priv->hints,
						g_strdup(key), hint_value);
}

static gboolean
_remove_all(void)
{
	return TRUE;
}

void
notify_notification_clear_hints(NotifyNotification *notification)
{
	g_return_if_fail(notification != NULL);
	g_return_if_fail(NOTIFY_IS_NOTIFICATION(notification));

	g_hash_table_foreach_remove(notification->priv->hints,
								(GHRFunc)_remove_all, NULL);
}

void
notify_notification_clear_actions(NotifyNotification *notification)
{
	g_return_if_fail(notification != NULL);
	g_return_if_fail(NOTIFY_IS_NOTIFICATION(notification));

	g_hash_table_foreach_remove(notification->priv->action_map,
								(GHRFunc)_remove_all, NULL);

	if (notification->priv->actions != NULL)
	{
		g_slist_foreach(notification->priv->actions, (GFunc)g_free, NULL);
		g_slist_free(notification->priv->actions);
	}

	notification->priv->actions = NULL;
	notification->priv->has_nondefault_actions = FALSE;
}

void
notify_notification_add_action(NotifyNotification *notification,
							   const char *action,
							   const char *label,
							   NotifyActionCallback callback,
							   gpointer user_data, GFreeFunc free_func)
{
	NotifyNotificationPrivate *priv;
	CallbackPair *pair;

	g_return_if_fail(notification != NULL);
	g_return_if_fail(NOTIFY_IS_NOTIFICATION(notification));
	g_return_if_fail(action != NULL && *action != '\0');
	g_return_if_fail(label != NULL && *label != '\0');
	g_return_if_fail(callback != NULL);

	priv = notification->priv;

	priv->actions = g_slist_append(priv->actions, g_strdup(action));
	priv->actions = g_slist_append(priv->actions, g_strdup(label));

	pair = g_new0(CallbackPair, 1);
	pair->cb = callback;
	pair->user_data = user_data;
	g_hash_table_insert(priv->action_map, g_strdup(action), pair);

	if (notification->priv->has_nondefault_actions &&
		g_ascii_strcasecmp(action, "default"))
	{
		notification->priv->has_nondefault_actions = TRUE;
	}
}

gboolean
_notify_notification_has_nondefault_actions(const NotifyNotification *n)
{
	g_return_val_if_fail(n != NULL, FALSE);
	g_return_val_if_fail(NOTIFY_IS_NOTIFICATION(n), FALSE);

	return n->priv->has_nondefault_actions;
}

gboolean
notify_notification_close(NotifyNotification *notification,
						  GError **error)
{
	NotifyNotificationPrivate *priv;
	GError *tmp_error = NULL;

	g_return_val_if_fail(notification != NULL, FALSE);
	g_return_val_if_fail(NOTIFY_IS_NOTIFICATION(notification), FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	priv = notification->priv;

	dbus_g_proxy_call(_notify_get_g_proxy(), "CloseNotification", &tmp_error,
					  G_TYPE_UINT, priv->id, G_TYPE_INVALID,
					  G_TYPE_INVALID);

	if (tmp_error != NULL)
	{
		g_propagate_error(error, tmp_error);
		return FALSE;
	}

	return TRUE;
}
