/*
 * @file tests/test-animation.c Unit test: animated images
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
#include <math.h>

#include <gdk-pixbuf/gdk-pixbuf.h>


/// WRITE ME! ///

int frames = 10;

// returns array of pixbufs for a pulsing animation
GdkPixbuf **generate_animation()
{
	int i;
	GdkPixbuf *file = gdk_pixbuf_new_from_file("applet-critical.png", NULL);
	double alpha = 1.0;

	GdkPixbuf **array = g_malloc(sizeof(GdkPixbuf *) * frames);

	for (i = 0; i < frames; i++)
	{
		GdkPixbuf *buf = gdk_pixbuf_copy(file);

		alpha = sin(M_PI + ((M_PI / frames) * i)) + 1.0;

		gdk_pixbuf_composite(file, buf, 0, 0,
							 gdk_pixbuf_get_width(buf),
							 gdk_pixbuf_get_height(buf),
							 0, 0, 1.0, 1.0, GDK_INTERP_NEAREST,
							 alpha);

		array[i] = buf;
	}

	return array;
}


int main()
{
	int i;
	notify_init("Animations");

	for (i = 0; i < frames; i++)
	{

	}

	NotifyHandle *n = notify_send_notification(NULL, // replaces nothing
											   NULL,
											   NOTIFY_URGENCY_NORMAL,
											   "Summary", "Content",
											   NULL, // no icon
											   TRUE, time(NULL) + 5,
											   NULL, // no user data
											   0); // no actions

	if (!n) {
		fprintf(stderr, "failed to send notification\n");
		return 1;
	}

	return 0;
}
