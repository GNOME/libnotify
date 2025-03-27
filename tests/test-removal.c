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

#include <stdlib.h>
#include <libnotify/notify.h>
#include <gtk/gtk.h>

static GMainLoop *loop;

static void
next_callback (NotifyNotification *n,
               const char         *action)
{
        g_assert (action != NULL);

        printf ("You clicked Next\n");

        notify_notification_close (n, NULL);

        g_main_loop_quit (loop);
}

int
main (int argc, char *argv[])
{
        NotifyNotification *n;

        loop = g_main_loop_new (NULL, FALSE);

        notify_init ("Urgency");

        n = notify_notification_new ("Low Urgency",
                                     "Joe signed online.",
                                     NULL);
        notify_notification_set_urgency (n, NOTIFY_URGENCY_LOW);
        if (!notify_notification_show (n, NULL)) {
                fprintf (stderr, "failed to send notification\n");
                exit (1);
        }

        sleep (3);

        if (!notify_notification_close (n, NULL)) {
                fprintf (stderr, "failed to remove notification\n");
                exit (1);
        }

        g_object_unref (G_OBJECT (n));


        n = notify_notification_new ("Normal Urgency",
                                     "You have a meeting in 10 minutes.",
                                     NULL);
        notify_notification_set_urgency (n, NOTIFY_URGENCY_NORMAL);
        if (!notify_notification_show (n, NULL)) {
                fprintf (stderr, "failed to send notification\n");
                exit (1);
        }

        sleep (3);

        if (!notify_notification_close (n, NULL)) {
                fprintf (stderr, "failed to remove notification\n");
                exit (1);
        }

        g_object_unref (G_OBJECT (n));


        n = notify_notification_new ("Critical Urgency",
                                     "This message will self-destruct in 10 seconds.",
                                     NULL);
        notify_notification_set_urgency (n, NOTIFY_URGENCY_CRITICAL);
        notify_notification_set_timeout (n, NOTIFY_EXPIRES_NEVER);
        notify_notification_add_action (n,
                                        "media-skip-forward",
                                        "Next",
                                        (NotifyActionCallback) next_callback,
                                        NULL,
                                        NULL);

        if (!notify_notification_show (n, NULL)) {
                fprintf (stderr, "failed to send notification\n");
                exit (1);
        }

        sleep (3);

        if (!notify_notification_close (n, NULL)) {
                fprintf (stderr, "failed to remove notification\n");
                exit (1);
        }

        g_main_loop_run (loop);

        g_object_unref (G_OBJECT (n));

        return 0;
}
