#ifndef _LIBNOTIFY_NOTIFY_COMMON_H_
#define _LIBNOTIFY_NOTIFY_COMMON_H_
#include <glib.h>

#define NOTIFY_TIMEOUT_DEFAULT -1
#define NOTIFY_TIMEOUT_NEVER 0

#define NOTIFY_DBUS_NAME           "org.freedesktop.Notifications"
#define NOTIFY_DBUS_CORE_INTERFACE "org.freedesktop.Notifications"
#define NOTIFY_DBUS_CORE_OBJECT    "/org/freedesktop/Notifications"

/**
 * Notification urgency levels.
 */
typedef enum
{
	NOTIFY_URGENCY_LOW,       /**< Low urgency.      */
	NOTIFY_URGENCY_NORMAL,    /**< Normal urgency.   */
	NOTIFY_URGENCY_CRITICAL,  /**< Critical urgency. */

} NotifyUrgency;

#endif /* _LIBNOTIFY_NOTIFY_H_ */
