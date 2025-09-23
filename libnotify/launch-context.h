/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright 2025 Canonical Ltd */
/* Authors: Alessandro Astone, Marco Trevisan */

#pragma once

#include <gio/gio.h>
#include "notification.h"

G_DECLARE_FINAL_TYPE (NotificationAppLaunchContext,
                      notification_app_launch_context,
                      NOTIFICATION, APP_LAUNCH_CONTEXT, GAppLaunchContext);

GAppLaunchContext *notification_app_launch_context_new (NotifyNotification *notification);
