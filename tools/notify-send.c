/**
 * @file notify-send.c A tool for sending notifications.
 *
 * Copyright (C) 2004 Christian Hammond.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 */
#include <libnotify/notify.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <popt.h>

#define N_(x) (x)

int
main(int argc, const char **argv)
{
	const gchar *summary = NULL;
	const gchar *description = NULL;
	char *urgency_str = NULL;
	gchar *sound = NULL;
	gchar *icons = NULL;
	gchar *icon_str = NULL;
	NotifyIcon *icon = NULL;
	NotifyUrgency urgency = NOTIFY_URGENCY_NORMAL;
	time_t expire_time;
	char ch;
	poptContext opt_ctx;
	const char **args;
	struct poptOption options[] =
	{
		{ "urgency", 'u', POPT_ARG_STRING | POPT_ARGFLAG_STRIP, &urgency_str,
		  0, N_("Specifies the urgency level (low, normal, high, critical)"),
		  NULL },
		{ "expire-time", 't', POPT_ARG_INT | POPT_ARGFLAG_STRIP, &expire_time,
		  0, N_("Specifies the timestamp at which to expire the notification, or if < current time, specifies timeout in seconds from current time"),
		  NULL },
		{ "sound", 's', POPT_ARG_STRING | POPT_ARGFLAG_STRIP, &sound, 0,
		  N_("Specifies a sound file to play on notification."), NULL },
		{ "icon",  'i', POPT_ARG_STRING | POPT_ARGFLAG_STRIP, &icons, 0,
		  N_("Specifies an icon filename or stock icon to display."),
		  N_("ICON1,ICON2,...") },
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	opt_ctx = poptGetContext("notify-send", argc, argv, options, 0);
	poptSetOtherOptionHelp(opt_ctx, "[OPTIONS]* <summary> [description]");

	while ((ch = poptGetNextOpt(opt_ctx)) >= 0)
		;

	if (ch < -1 || (args = poptGetArgs(opt_ctx)) == NULL)
	{
		poptPrintUsage(opt_ctx, stderr, 0);
		exit(1);
	}

	if (args[0] != NULL)
		summary = args[0];

	if (summary == NULL)
	{
		poptPrintUsage(opt_ctx, stderr, 0);
		exit(1);
	}

	if (args[1] != NULL)
	{
		description = args[1];

		if (args[2] != NULL)
		{
			poptPrintUsage(opt_ctx, stderr, 0);
			exit(1);
		}
	}

	if (icons != NULL)
	{
		char *c;

		/* XXX */
		if ((c = strchr(icons, ',')) != NULL)
			*c = '\0';

		icon_str = icons;

		icon = notify_icon_new(icon_str);
	}

	if (urgency_str != NULL)
	{
		if (!strcasecmp(urgency_str, "low"))
			urgency = NOTIFY_URGENCY_LOW;
		else if (!strcasecmp(urgency_str, "normal"))
			urgency = NOTIFY_URGENCY_NORMAL;
		else if (!strcasecmp(urgency_str, "high"))
			urgency = NOTIFY_URGENCY_HIGH;
		else if (!strcasecmp(urgency_str, "critical"))
			urgency = NOTIFY_URGENCY_CRITICAL;
		else
		{
			poptPrintHelp(opt_ctx, stderr, 0);
			exit(1);
		}
	}

	if (!notify_init("notify-send"))
	{
		fprintf(stderr, "Unable to initialize libnotify\n");
		exit(1);
	}

    /* if the given time is < current time, treat it as a timeout in seconds (ie 5 seconds) */
    if (expire_time < time(NULL)) expire_time += time(NULL);
        
	notify_send_notification(0, urgency, summary, description, icon,
							 TRUE, expire_time, NULL, 0);

	if (icon != NULL)
		notify_icon_destroy(icon);

	poptFreeContext(opt_ctx);
	notify_uninit();

	return 0;
}
