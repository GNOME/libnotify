/*
 * @file tests/test-multi-actions.c Unit test: multiple actions
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
#include <stdlib.h>
#include <assert.h>

#define DBUS_API_SUBJECT_TO_CHANGE 1

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

GMainLoop *loop;
NotifyHandle *n;

static void callback(NotifyHandle *handle, guint32 uid, void *user_data)
{
	char *s = NULL;

	assert( uid >= 0 && uid <= 2 );

	switch (uid)
	{
		case 0: s = "the notification"; break;
		case 1: s = "Empty Trash"; break;
		case 2: s = "Help Me"; break;
	}

	printf("You clicked %s\n", s);

	notify_close(n);

	g_main_loop_quit(loop);
}

int main() {
	if (!notify_init("Multi Action Test")) exit(1);

	DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, NULL);
	loop = g_main_loop_new(NULL, FALSE);

	dbus_connection_setup_with_g_main(conn, NULL);

	n = notify_send_notification(NULL, // replaces nothing
								 "device",
								 NOTIFY_URGENCY_NORMAL,
								 "Low disk space",
								 "You can free up some disk space by "
								 "emptying the trash can.",
								 NULL, // no icon
								 FALSE, 0, // does not expire
								 NULL, // no user data
								 3,	   // 3 actions
								 0, "default", callback,
								 1, "Empty Trash", callback,
								 2, "Help Me", callback );

	if (!n) {
		fprintf(stderr, "failed to send notification\n");
		return 1;
	}

	g_main_loop_run(loop);

	return 0;
}
