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
#include <gtk/gtk.h>

static int      count = 0;

static void
on_mapped (GtkWidget      *widget,
           void           *user_data)
{
        NotifyNotification *n = NOTIFY_NOTIFICATION (user_data);

        g_signal_handlers_disconnect_by_func (widget, on_mapped, user_data);

        notify_notification_show (n, NULL);
}

static void
on_clicked (GtkButton *button,
            void      *user_data)
{
        gchar              *buf;
        NotifyNotification *n = NOTIFY_NOTIFICATION (user_data);

        count++;
        buf = g_strdup_printf ("You clicked the button %i times", count);
        notify_notification_update (n, "Widget Attachment Test", buf, NULL);
        g_free (buf);

        notify_notification_show (n, NULL);
}

int
main (int argc, char *argv[])
{
        NotifyNotification *n;
        GtkWidget      *window;
        GtkWidget      *button;

        gtk_init ();
        notify_init ("Replace Test");

        window = gtk_window_new ();
        g_signal_connect (G_OBJECT (window),
                          "delete_event",
                          G_CALLBACK (gtk_window_destroy),
                          NULL);

        button = gtk_button_new_with_label ("click here to change notification");

        gtk_window_set_child (GTK_WINDOW (window), button);

        n = notify_notification_new ("Widget Attachment Test",
                                     "Button has not been clicked yet",
                                     NULL); //no icon

        notify_notification_set_timeout (n, 0); //don't timeout

        g_signal_connect (G_OBJECT (button),
                          "clicked",
                          G_CALLBACK (on_clicked),
                          n);
        g_signal_connect_after (G_OBJECT (button),
                                "map",
                                G_CALLBACK (on_mapped),
                                n);

        while (g_list_model_get_n_items (gtk_window_get_toplevels ()) > 0)
                g_main_context_iteration (NULL, TRUE);

        return 0;
}
