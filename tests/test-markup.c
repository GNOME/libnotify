/*
 * @file tests/test-markup.c Unit test: markup
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

int main() {
	notify_init("Markup");

	NotifyHandle *n = notify_send_notification(
		NULL, // replaces nothing
		NULL,
		NOTIFY_URGENCY_NORMAL,
		"Summary",
		"Some <b>bold</b>, <u>underlined</u>, <i>italic</i>, "
		"<a href='http://www.google.com'>linked</a> text",
		NULL, // no icon
		TRUE, time(NULL) + 5,
		NULL, // no hints
		NULL, // no user data
		0); // no actions

	if (!n) {
		fprintf(stderr, "failed to send notification\n");
		return 1;
	}

	return 0;
}
