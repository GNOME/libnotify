/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or(at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#include <libnotify/notify.h>
#include <stdio.h>
#include <unistd.h>
#include <glib.h>

int
main ()
{
        NotifyNotification *n;
        GError             *error;
        error = NULL;

#if !GLIB_CHECK_VERSION (2, 36, 0)
        g_type_init ();
#endif

        notify_init ("Replace Test");

        n = notify_notification_new ("Summary",
                                     "First message",
                                     NULL);  //no icon

        notify_notification_set_timeout (n, 0); //don't timeout

        if (!notify_notification_show (n, &error)) {
                fprintf (stderr, "failed to send notification: %s\n",
                         error->message);
                g_error_free (error);
                return 1;
        }

        sleep (3);

        notify_notification_update (n,
                                    "Second Summary",
                                    "First mesage was replaced",
                                    NULL);
        notify_notification_set_timeout (n, NOTIFY_EXPIRES_DEFAULT);

        if (!notify_notification_show (n, &error)) {
                fprintf (stderr,
                         "failed to send notification: %s\n",
                         error->message);
                g_error_free (error);
                return 1;
        }

        return 0;
}
