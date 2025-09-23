/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright 2025 Canonical Ltd */
/* Author: Alessandro Astone, Marco Trevisan */

#include "launch-context.h"

struct _NotificationAppLaunchContext
{
        GAppLaunchContext parent_instance;

        char *activation_token;
};

G_DEFINE_TYPE (NotificationAppLaunchContext,
               notification_app_launch_context,
               G_TYPE_APP_LAUNCH_CONTEXT);

GAppLaunchContext *
notification_app_launch_context_new (NotifyNotification *notification)
{
        NotificationAppLaunchContext *self;
        const char *activation_token;

        activation_token = notify_notification_get_activation_token (notification);
        if (!activation_token) {
                return NULL;
        }

        self = g_object_new (notification_app_launch_context_get_type (), NULL);
        self->activation_token = g_strdup (activation_token);

        return G_APP_LAUNCH_CONTEXT (self);
}

static void
notification_app_launch_context_init (NotificationAppLaunchContext *self)
{}

static void
notification_app_launch_context_finalize (GObject *object)
{
        NotificationAppLaunchContext *self = NOTIFICATION_APP_LAUNCH_CONTEXT (object);

        g_clear_pointer (&self->activation_token, g_free);

        G_OBJECT_CLASS (notification_app_launch_context_parent_class)->finalize (object);
}

static char *
notification_app_launch_context_get_startup_notify_id (GAppLaunchContext *context,
                                                       GAppInfo *,
                                                       GList *)
{
        NotificationAppLaunchContext *self = NOTIFICATION_APP_LAUNCH_CONTEXT (context);

        return g_strdup (self->activation_token);
}

static void
notification_app_launch_context_class_init (NotificationAppLaunchContextClass *klass)
{
        GAppLaunchContextClass *context_class = G_APP_LAUNCH_CONTEXT_CLASS (klass);
        GObjectClass* object_class = G_OBJECT_CLASS (klass);

        context_class->get_startup_notify_id = notification_app_launch_context_get_startup_notify_id;
        object_class->finalize = notification_app_launch_context_finalize;
}
