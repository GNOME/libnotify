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
	NOTIFY_URGENCY_HIGH,      /**< High urgency.     */
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
 * Creates an icon with the specified icon URI.
 *
 * @param icon_uri The icon URI.
 *
 * @return The icon.
 */
NotifyIcon *notify_icon_new(const char *icon_uri);

/**
 * Creates an icon with the specified icon data.
 *
 * @param icon_len  The icon data length.
 * @param icon_data The icon data.
 *
 * @return The icon.
 */
NotifyIcon *notify_icon_new_with_data(size_t icon_len,
									  const guchar *icon_data);

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
 * @param urgency        The urgency level.
 * @param summary        The summary of the notification.
 * @param detailed       The optional detailed information.
 * @param icon           The optional icon.
 * @param expires        TRUE if the notification should automatically expire,,
 *                       or FALSE to keep it open until manually closed.
 * @param expire_time    The optional time to automatically close the
 *                       notification, or 0 for the daemon's default.
 * @param user_data      User-specified data to send to a callback.
 * @param action_count   The number of actions.
 * @param ...            The actions in uint32/string/callback sets.
 *
 * @return A unique ID for the notification.
 */
NotifyHandle *notify_send_notification(NotifyHandle *replaces,
									   NotifyUrgency urgency,
									   const char *summary,
									   const char *detailed,
									   const NotifyIcon *icon,
									   gboolean expires, time_t expire_time,
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
 * @param urgency        The urgency level.
 * @param summary        The summary of the notification.
 * @param detailed       The optional detailed information.
 * @param icon           The optional icon.
 * @param expires        TRUE if the notification should automatically expire,
 *                       or FALSE to keep it open until manually closed.
 * @param expire_time    The optional time to automatically close the
 *                       notification, or 0 for the daemon's default.
 * @param user_data      User-specified data to send to a callback.
 * @param action_count   The number of actions.
 * @param actions        The actions in string/callback pairs.
 *
 * @return A unique ID for the notification.
 */
NotifyHandle *notify_send_notification_varg(NotifyHandle *replaces,
											NotifyUrgency urgency,
											const char *summary,
											const char *detailed,
											const NotifyIcon *icon,
											gboolean expires,
											time_t expire_time,
											gpointer user_data,
											size_t action_count,
											va_list actions);

/*@}*/

#endif /* _LIBNOTIFY_NOTIFY_H_ */
