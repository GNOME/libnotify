/** -*- mode: c-mode; tab-width: 4; indent-tabs-mode: t; -*-
 * @file libnotify/notify.h Notifications library
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
 *
 * TODO:
 *  - We talk about URIs, but they are actually file paths not URIs
 *  - Un-glibify?
 */

#ifndef _LIBNOTIFY_NOTIFY_H_
#define _LIBNOTIFY_NOTIFY_H_

#include <glib.h>
#include <time.h>

/**
 * Notification urgency levels.
 */
typedef enum
{
	NOTIFY_URGENCY_LOW,       /**< Low urgency.      */
	NOTIFY_URGENCY_NORMAL,    /**< Normal urgency.   */
	NOTIFY_URGENCY_CRITICAL,  /**< Critical urgency. */

} NotifyUrgency;

typedef struct _NotifyHandle NotifyHandle;
typedef struct _NotifyIcon   NotifyIcon;

typedef void (*NotifyCallback)(NotifyHandle *, guint32, gpointer);

/**************************************************************************/
/** @name libnotify Base API                                              */
/**************************************************************************/
/*@{*/

/**
 * Initializes the notifications library.
 *
 * @param app_name The application name.
 *
 * @return TRUE if the library initialized properly and a connection to a
 *         notification server was made.
 */
gboolean notify_init(const char *app_name);

/**
 * Uninitializes the notifications library.
 *
 * This will be automatically called on exit unless previously called.
 */
void notify_uninit(void);

/**
 * Returns whether or not the notification library is initialized.
 *
 * @return TRUE if the library is initialized, or FALSE.
 */
gboolean notify_is_initted(void);

/**
 * Manually closes a notification.
 *
 * @param handle The notification handle.
 */
void notify_close(NotifyHandle *handle);


/**
 * Returns the server information.
 *
 * The strings returned must be freed.
 *
 * @param ret_name    The returned product name of the server.
 * @param ret_vendor  The returned vendor.
 * @param ret_version The returned specification version supported.
 *
 * @return TRUE if the call succeeded, or FALSE if there were errors.
 */
gboolean notify_get_server_info(char **ret_name, char **ret_vendor,
								char **ret_version);

/**
 * Returns the server's capabilities.
 *
 * The returned list and the strings inside must all be freed.
 *
 * @return The list of capabilities, or NULL on error.
 */
GList *notify_get_server_caps(void);

/*@}*/

/**************************************************************************/
/** @name NotifyIcon API                                                  */
/**************************************************************************/
/*@{*/

/**
 * Creates an empty (invalid) icon. You must add at least one frame,
 * otherwise the icon will be rejected. The first add_frame function
 * you call determines if this is a raw data or URI based icon.
 *
 * This function is useful when adding data from a loop.
 *
 * @return A new invalid icon.
 */
NotifyIcon *notify_icon_new();

/**
 * Creates an icon with the specified icon URI as the first frame.
 * You can then add more frames by calling notify_icon_add_frame_from_uri.
 * Note that you can't mix raw data and file URIs in the same icon.
 *
 * @param icon_uri The icon URI.
 *
 * @return The icon.
 */
NotifyIcon *notify_icon_new_from_uri(const char *icon_uri);

/**
 * Creates an icon with the specified icon data as the first frame.
 * You can then add more frames by calling notify_icon_add_frame_from_data.
 * Note that you can't mix raw data and file URIs in the same icon.
 *
 * @param icon_len  The icon data length.
 * @param icon_data The icon data.
 *
 * @return The icon.
 */
NotifyIcon *notify_icon_new_from_data(size_t icon_len,
									  const guchar *icon_data);

/**
 * Adds a frame from raw data (a png file) to the icons animation.
 *
 * @param icon      The icon to add the frame to
 * @param icon_len  The frame data length
 * @param icon_data The frame data
 *
 * @return TRUE if was successful, FALSE if this icon isn't a raw data
 * icon or there was a bad parameter.
 */
gboolean notify_icon_add_frame_from_data(NotifyIcon *icon,
										 size_t icon_len,
										 const guchar *icon_data);

/**
 * Adds a frame from a URI to the icons animation.
 *
 * @param icon      The icon to add the frame to
 * @param icon_uri  The URI of the icon file for the frame
 *
 * @return TRUE if was successful, FALSE if this icon isn't a file URI
 * icon or there was a bad parameter.
 */
gboolean notify_icon_add_frame_from_uri(NotifyIcon *icon,
										 const char *icon_uri);

/**
 * Destroys an icon.
 *
 * @param icon The icon to destroy.
 */
void notify_icon_destroy(NotifyIcon *icon);

/*@}*/

/**************************************************************************/
/** @name Notifications API                                               */
/**************************************************************************/
/*@{*/

/**
 * Sends a notification.
 *
 * A callback has the following prototype:
 *
 * @code
 * void callback(NotifyHandle *handle, guint32 action_id, void *user_data);
 * @endcode
 *
 * @param replaces       The ID of the notification to atomically replace
 * @param type           The optional notification type.
 * @param urgency        The urgency level.
 * @param summary        The summary of the notification.
 * @param body           The optional body.
 * @param icon           The optional icon.
 * @param expires        TRUE if the notification should automatically expire,,
 *                       or FALSE to keep it open until manually closed.
 * @param timeout        The optional timeout to automatically close the
 *                       notification, or 0 for the daemon's default.
 * @param user_data      User-specified data to send to a callback.
 * @param action_count   The number of actions.
 * @param ...            The actions in uint32/string/callback sets.
 *
 * @return A unique ID for the notification.
 */
NotifyHandle *notify_send_notification(NotifyHandle *replaces,
									   const char *type,
									   NotifyUrgency urgency,
									   const char *summary,
									   const char *body,
									   const NotifyIcon *icon,
									   gboolean expires, time_t timeout,
									   gpointer user_data,
									   size_t action_count, ...);

/**
 * Sends a notification, taking a va_list for the actions.
 *
 * A callback has the following prototype:
 *
 * @code
 * void callback(NotifyHandle *handle, guint32 action, void *user_data);
 * @endcode
 *
 * @param replaces       The handle of the notification to atomically replace
 * @param type           The optional notification type.
 * @param urgency        The urgency level.
 * @param summary        The summary of the notification.
 * @param detailed       The optional body.
 * @param icon           The optional icon.
 * @param expires        TRUE if the notification should automatically expire,
 *                       or FALSE to keep it open until manually closed.
 * @param timeout        The optional timeout to automatically close the
 *                       notification, or 0 for the daemon's default.
 * @param user_data      User-specified data to send to a callback.
 * @param action_count   The number of actions.
 * @param actions        The actions in string/callback pairs.
 *
 * @return A unique ID for the notification.
 */
NotifyHandle *notify_send_notification_varg(NotifyHandle *replaces,
											const char *type,
											NotifyUrgency urgency,
											const char *summary,
											const char *detailed,
											const NotifyIcon *icon,
											gboolean expires,
											time_t timeout,
											gpointer user_data,
											size_t action_count,
											va_list actions);

/*@}*/

#endif /* _LIBNOTIFY_NOTIFY_H_ */
