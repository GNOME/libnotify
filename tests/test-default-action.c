/*
 * @file tests/test-default-action.c Unit test: default action
 *
 * @Copyright (C) 2004 Mike Hearn <mike@navi.cx>
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

#include <libnotify/notify.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#define DBUS_API_SUBJECT_TO_CHANGE 1

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

GMainLoop *loop;
NotifyHandle *n;

static void callback(NotifyHandle *handle, guint32 uid, void *user_data)
{
    assert( uid == 0 );

    notify_close(n);

    g_main_loop_quit(loop);
}

int main() {
    if (!notify_init("Default Action Test")) exit(1);

    DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, NULL);
    loop = g_main_loop_new(NULL, FALSE);

    dbus_connection_setup_with_g_main(conn, NULL);
    
    n = notify_send_notification(NULL, // replaces nothing
                                 NOTIFY_URGENCY_NORMAL,
                                 "Matt is online", NULL,
                                 NULL, // no icon
                                 FALSE, 0, // does not expire
                                 NULL, // no user data
                                 1,
                                 0, "default", callback); // 1 action

    if (!n) {
        fprintf(stderr, "failed to send notification\n");
        return 1;
    }

    g_main_loop_run(loop);

    return 0;
}
