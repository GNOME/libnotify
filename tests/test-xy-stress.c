/*
 * @file tests/test-xy.c Unit test: X, Y hints
 *
 * @Copyright (C) 2005 Christian Hammond <chipx86@chipx86.com>
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
#include <gdk/gdk.h>
#include <stdio.h>
#include <unistd.h>

static void
emit_notification(int x, int y)
{
	NotifyHints *hints;
	static char buffer[BUFSIZ];

	hints = notify_hints_new();
	notify_hints_set_int(hints, "x", x);
	notify_hints_set_int(hints, "y", y);

	g_snprintf(buffer, sizeof(buffer),
			   "This notification should point to %d, %d.", x, y);

	NotifyHandle *n = notify_send_notification(
		NULL, // replaces nothing
		NULL,
		NOTIFY_URGENCY_NORMAL,
		"X, Y Test",
		buffer,
		NULL, // no icon
		TRUE, time(NULL) + 5,
		hints,
		NULL, // no user data
		0); // no actions

	if (!n) {
		fprintf(stderr, "failed to send notification\n");
	}
}

int
main(int argc, char **argv)
{
	GdkDisplay *display;
	GdkScreen *screen;
	int screen_x2, screen_y2;

	gdk_init(&argc, &argv);

	notify_init("XY");

	display   = gdk_display_get_default();
	screen    = gdk_display_get_default_screen(display);
	screen_x2 = gdk_screen_get_width(screen)  - 1;
	screen_y2 = gdk_screen_get_height(screen) - 1;

	emit_notification(0, 0);
	emit_notification(screen_x2, 0);
	emit_notification(5, 150);
	emit_notification(screen_x2 - 5, 150);
	emit_notification(0, screen_y2 / 2);
	emit_notification(screen_x2, screen_y2 / 2);
	emit_notification(5, screen_y2 - 150);
	emit_notification(screen_x2 - 5, screen_y2 - 150);
	emit_notification(0, screen_y2);
	emit_notification(screen_x2, screen_y2);

	return 0;
}
