/* -*- mode: c-mode; tab-width: 4; indent-tabs-mode: t; -*- */
/**
 * @file libnotify/notify.c Notifications library
 *
 * @Copyright (C) 2004 Christian Hammond <chipx86@chipx86.com>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef DBUS_API_SUBJECT_TO_CHANGE
# define DBUS_API_SUBJECT_TO_CHANGE 1
#endif

#include "notify.h"
#include "dbus-compat.h"
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

static gboolean _initted = FALSE;
static gchar *_app_name = NULL;
#ifdef __GNUC__
#  define format_func __attribute__((format(printf, 1, 2)))
#else /* no format string checking with this compiler */
#  define format_func
#endif

#if 0
static void format_func
print_error (char *message, ...)
{
  char buf[1024];
  va_list args;

  va_start (args, message);
  vsnprintf (buf, sizeof (buf), message, args);
  va_end (args);

  fprintf (stderr, "%s(%d): libnotify: %s",
	   (getenv ("_") ? getenv ("_") : ""), getpid (), buf);
}
#endif

gboolean
notify_init (const char *app_name)
{
  g_return_val_if_fail (app_name != NULL, FALSE);
  g_return_val_if_fail (*app_name != '\0', FALSE);

  if (_initted)
    return TRUE;

  _app_name = g_strdup (app_name);

  g_type_init ();

#ifdef HAVE_ATEXIT
  atexit (notify_uninit);
#endif /* HAVE_ATEXIT */

  _initted = TRUE;

  return TRUE;
}

const gchar *
_notify_get_app_name (void)
{
  return _app_name;
}

void
notify_uninit (void)
{

  if (_app_name != NULL)
    {
      g_free (_app_name);
      _app_name = NULL;
    }

  /* TODO: keep track of all notifications and destroy them here? */
}

gboolean
notify_is_initted (void)
{
  return _initted;
}
