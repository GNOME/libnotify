/**
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
 * Notification and request urgency levels.
 */
typedef enum
{
	NOTIFY_URGENCY_LOW,       /**< Low urgency.      */
	NOTIFY_URGENCY_NORMAL,    /**< Normal urgency.   */
	NOTIFY_URGENCY_HIGH,      /**< High urgency.     */
	NOTIFY_URGENCY_CRITICAL,  /**< Critical urgency. */

} NotifyUrgency;

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
 * @param id The ID of the notification.
 */
void notify_close_notification(guint32 id);

/**
 * Automatically closes a request.
 *
 * The default button's callback, if any, will be called.
 *
 * @param id The ID of the request.
 */
void notify_close_request(guint32 id);

/*@}*/

/**************************************************************************/
/** @name Notifications API                                               */
/**************************************************************************/
/*@{*/

/**
 * Sends a standard notification.
 *
 * @param urgency  The urgency level.
 * @param summary  The summary of the notification.
 * @param detailed The optional detailed information.
 * @param icon_uri The optional icon URI.
 * @param timeout  The optional time to automatically close the notification,
 *                 or 0.
 *
 * @return A unique ID for the notification.
 */
guint32 notify_send_notification(NotifyUrgency urgency, const char *summary,
								 const char *detailed, const char *icon_uri,
								 time_t timeout);

/**
 * Sends a standard notification with raw icon data.
 *
 * @param urgency   The urgency level.
 * @param summary   The summary of the notification.
 * @param detailed  The optional detailed information.
 * @param icon_len  The icon data length.
 * @param icon_data The icon data.
 * @param timeout   The optional time to automatically close the notification,
 *                  or 0.
 *
 * @return A unique ID for the notification.
 */
guint32 notify_send_notification_with_icon_data(NotifyUrgency urgency,
												const char *summary,
												const char *detailed,
												size_t icon_len,
												void *icon_data,
												time_t timeout);

/*@}*/

/**************************************************************************/
/** @name Requests API                                                    */
/**************************************************************************/
/*@{*/

/**
 * Sends a standard request.
 *
 * A callback has the following prototype:
 *
 * @code
 * void callback(guint32 id, guint32 button, void *user_data);
 * @endcode
 *
 * @param urgency        The urgency level.
 * @param summary        The summary of the request.
 * @param detailed       The optional detailed information.
 * @param icon_uri       The optional icon URI.
 * @param timeout        The optional time to automatically close the request,
 *                       or 0.
 * @param user_data      User-specified data to send to a callback.
 * @param default_button The default button, or -1.
 * @param button_count   The number of buttons.
 * @param ...            The buttons in string/callback pairs.
 *
 * @return A unique ID for the request.
 */
guint32 notify_send_request(NotifyUrgency urgency, const char *summary,
							const char *detailed, const char *icon_uri,
							time_t timeout, gpointer user_data,
							size_t default_button,
							size_t button_count, ...);

/**
 * Sends a standard request with raw icon data.
 *
 * A callback has the following prototype:
 *
 * @code
 * void callback(guint32 id, guint32 button, void *user_data);
 * @endcode
 *
 * @param urgency        The urgency level.
 * @param summary        The summary of the request.
 * @param detailed       The optional detailed information.
 * @param icon_len       The icon data length.
 * @param icon_data      The icon data.
 * @param timeout        The optional time to automatically close the request,
 *                       or 0.
 * @param user_data      User-specified data to send to a callback.
 * @param default_button The default button, or -1.
 * @param button_count   The number of buttons.
 * @param ...            The buttons in string/callback pairs.
 *
 * @return A unique ID for the request.
 */
guint32 notify_send_request_with_icon_data(NotifyUrgency urgency,
										   const char *summary,
										   const char *detailed,
										   size_t icon_len,
										   guchar *icon_data,
										   time_t timeout,
										   gpointer user_data,
										   gint32 default_button,
										   size_t button_count, ...);

/*@}*/

#endif /* _LIBNOTIFY_NOTIFY_H_ */
