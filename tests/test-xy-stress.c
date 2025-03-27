/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * @file tests/test-xy.c Unit test: X, Y hints
 *
 * @Copyright(C) 2005 Christian Hammond <chipx86@chipx86.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or(at your option) any later version.
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

#include <libnotify/notify.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>

static void
_handle_closed (GObject * obj)
{
        g_message ("closing");
        g_object_unref (obj);
}

static void
emit_notification (int x, int y)
{
        char               *buffer;
        NotifyNotification *n;

        buffer = g_strdup_printf ("This notification should point to %d, %d.",
                                  x,
                                  y);

        n = notify_notification_new ("X, Y Test", buffer, NULL);
        g_free (buffer);

        notify_notification_set_hint_int32 (n, "x", x);
        notify_notification_set_hint_int32 (n, "y", y);

        g_signal_connect (G_OBJECT (n),
                          "closed",
                          G_CALLBACK (_handle_closed),
                          NULL);

        if (!notify_notification_show (n, NULL))
                fprintf (stderr, "failed to send notification\n");
}

static gboolean
_popup_random_bubble (gpointer unused)
{
        GdkDisplay     *display;
        GListModel     *monitors;
        GdkMonitor     *monitor;
        GdkRectangle    rect;
        int             x, y;

        display = gdk_display_get_default ();
        monitors = gdk_display_get_monitors (display);
        monitor = g_list_model_get_item (monitors, 0);
        gdk_monitor_get_geometry (monitor, &rect);

        x = g_random_int_range (rect.x, rect.x + rect.width - 1);
        y = g_random_int_range (rect.y, rect.y + rect.height - 1);
        emit_notification (x, y);

        return TRUE;
}

int
main (int argc, char **argv)
{
        GMainLoop *loop;

        gtk_init ();

        notify_init ("XY");

        g_timeout_add (1000, _popup_random_bubble, NULL);

        loop = g_main_loop_new (NULL, FALSE);
        g_main_loop_run (loop);

        return 0;
}
