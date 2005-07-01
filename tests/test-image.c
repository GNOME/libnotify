/*
 * @file tests/test-image.c Unit test: images
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
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define DBUS_API_SUBJECT_TO_CHANGE 1

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

GMainLoop *loop;
NotifyHandle *n;

static void send(char *i, size_t rawlen, char *s, char *b)
{
	NotifyIcon *icon;

	if (rawlen > 0)
		icon = notify_icon_new_from_data(rawlen, i);
	else
		icon = notify_icon_new_from_uri(i);

	n = notify_send_notification(NULL, // replaces nothing
								 NULL,
								 NOTIFY_URGENCY_NORMAL,
								 s, b,
								 icon,
								 TRUE, time(NULL) + 5,
								 NULL, // no hints
								 NULL, // no user data
								 0);

	if (!n) {
		fprintf(stderr, "failed to send notification\n");
		exit(1);
	}

	notify_icon_destroy(icon);	
}

int main() {
	if (!notify_init("Images Test")) exit(1);

	DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, NULL);
	loop = g_main_loop_new(NULL, FALSE);

	dbus_connection_setup_with_g_main(conn, NULL);

	// these images exist on fedora core 2 workstation profile. might not on yours

	send("gnome-starthere",
		 0,
		 "Welcome to Linux!",
		 "This is your first login. To begin exploring the system, click on 'Start Here', 'Computer' or 'Applications'");

	char file[1024];
	readlink("/proc/self/exe", file, sizeof(file));
	*strrchr(file, '/') = '\0';
	strcat(file, "/../applet-critical.png");

	printf("sending %s\n", file);

	send(file,
		 0,
		 "Alert!",
		 "Warning!");


	struct stat buf;
	if (stat(file, &buf) == -1)
	{
		fprintf(stderr, "could not stat %s: %s", file, strerror(errno));
		exit(1);
	}

	int fd = open(file, O_RDONLY);

	void *pngbase = mmap(NULL, buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

	close(fd);

	send(pngbase,
		 buf.st_size,
		 "Raw image test",
		 "This is an image marshalling test");

	return 0;
}
