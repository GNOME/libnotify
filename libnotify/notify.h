/* -*- mode: c-mode; tab-width: 4; indent-tabs-mode: t; -*- */
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
 *
 * @todo We talk about URIs, but they are actually file paths not URIs
 * @todo Un-glibify?
 */

#ifndef _LIBNOTIFY_NOTIFY_H_
#define _LIBNOTIFY_NOTIFY_H_

#include <glib.h>
#include <time.h>

#include "notifycommon.h"
#include "notifynotification.h"

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

const gchar *_notify_get_app_name(void);
/*@}*/


#endif /* _LIBNOTIFY_NOTIFY_H_ */
