/*
 * @file tests/test-basic.c Unit test: basics
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
	NotifyNotification *n;

	notify_init("Basics");

	n = notify_notification_new ("Summary", 
                                     "Content that is very long 8374983278r32j4 rhjjfh dw8f 43jhf 8ds7 ur2389f jdbjkt h8924yf jkdbjkt 892hjfiHER98HEJIF BDSJHF hjdhF JKLH 890YRHEJHFU 89HRJKSHFJ YE8UI HR3UIH89EFHIUEUF9DHFUIBuiew f89hsajiJ FHJKDSKJFH SDJKFH KJASDFJK HKJADSHFK JSAHF89WE HUIIUG JG kjG JKGJGHJg JHG H J HJGJHDG HJKJG hgd hgjhf df h3eui fusidyaiu rh f98ehkrnm e8rv9y 43heh vijdhjkewdkjsjfjk sdhkjf hdkj fadskj hfkjdsh",
                                     NULL, NULL);
        notify_notification_set_timeout (n, 3000); //3 seconds

	if (!notify_notification_show (n, NULL)) {
		fprintf(stderr, "failed to send notification\n");
		return 1;
	}

	return 0;
}
