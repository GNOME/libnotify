/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2025 Marco Trevisan.
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

#pragma once

G_BEGIN_DECLS

/**
 * NOTIFY_NOTIFICATION_HINT_ACTION_ICONS:
 *
 * When set, a server that has the "action-icons" capability will attempt to
 * interpret any action identifier as a named icon. The localized display name
 * will be used to annotate the icon for accessibility purposes. The icon name
 * should be compliant with the Freedesktop.org Icon Naming Specification.
 *
 * Requires server supporting specification version >= 1.2.
 *
 * Hint [type@GLib.VariantType]: [const@GLib.VARIANT_TYPE_BOOLEAN] (`b`).
 *
 * Since: 0.8.8
 */
#define NOTIFY_NOTIFICATION_HINT_ACTION_ICONS "action-icons"

/**
 * NOTIFY_NOTIFICATION_HINT_CATEGORY:
 *
 * The type of notification this is.
 *
 * Hint [type@GLib.VariantType]: [const@GLib.VARIANT_TYPE_STRING] (`s`).
 *
 * Since: 0.8.8
 */
#define NOTIFY_NOTIFICATION_HINT_CATEGORY "category"

/**
 * NOTIFY_NOTIFICATION_HINT_DESKTOP_ENTRY:
 *
 * This specifies the name of the desktop filename representing the calling
 * program. This should be the same as the prefix used for the application's
 * .desktop file. An example would be "rhythmbox" from "rhythmbox.desktop". This
 * can be used by the daemon to retrieve the correct icon for the application,
 * for logging purposes, etc.
 *
 * Hint [type@GLib.VariantType]: [const@GLib.VARIANT_TYPE_STRING] (`s`).
 *
 * Since: 0.8.8
 */
#define NOTIFY_NOTIFICATION_HINT_DESKTOP_ENTRY "desktop-entry"

/**
 * NOTIFY_NOTIFICATION_HINT_IMAGE_DATA:
 *
 * This is a raw data image format which describes the width,
 * height, rowstride, has alpha, bits per sample, channels and image data
 * respectively.
 *
 * Requires server supporting specification version >= 1.2
 *
 * [type@GLib.VariantType]: `(iiibiiay)`
 *
 * Since: 0.8.8
 */
#define NOTIFY_NOTIFICATION_HINT_IMAGE_DATA "image-data"

/**
 * NOTIFY_NOTIFICATION_HINT_IMAGE_DATA_LEGACY:
 *
 * This is a raw data image format which describes the width,
 * height, rowstride, has alpha, bits per sample, channels and image data
 * respectively.
 *
 * Requires server supporting specification version >= 1.1
 *
 * [type@GLib.VariantType]: `(iiibiiay)`
 *
 * Since: 0.8.8
 */
#define NOTIFY_NOTIFICATION_HINT_IMAGE_DATA_LEGACY "image_data"

/**
 * NOTIFY_NOTIFICATION_HINT_IMAGE_PATH:
 *
 * Alternative way to define the notification image.
 *
 * Requires server supporting specification version >= 1.2.
 *
 * Hint [type@GLib.VariantType]: [const@GLib.VARIANT_TYPE_STRING] (`s`).
 *
 * Since: 0.8.8
 */
#define NOTIFY_NOTIFICATION_HINT_IMAGE_PATH "image-path"

/**
 * NOTIFY_NOTIFICATION_HINT_IMAGE_PATH_LEGACY:
 *
 * Alternative way to define the notification image.
 *
 * Requires server supporting specification version >= 1.1.
 *
 * Hint [type@GLib.VariantType]: [const@GLib.VARIANT_TYPE_STRING] (`s`).
 *
 * Since: 0.8.8
 */
#define NOTIFY_NOTIFICATION_HINT_IMAGE_PATH_LEGACY "image_path"

/**
 * NOTIFY_NOTIFICATION_HINT_RESIDENT:
 *
 * When set the server will not automatically remove the notification when an
 * action has been invoked. The notification will remain resident in the server
 * until it is explicitly removed by the user or by the sender. This hint is
 * likely only useful when the server has the "persistence" capability.
 *
 * Requires server supporting specification version >= 1.2.
 *
 * Hint [type@GLib.VariantType]: [const@GLib.VARIANT_TYPE_BOOLEAN] (`b`).
 *
 * Since: 0.8.8
 */
#define NOTIFY_NOTIFICATION_HINT_RESIDENT "resident"

/**
 * NOTIFY_NOTIFICATION_HINT_SOUND_FILE:
 *
 * The path to a sound file to play when the notification pops up.
 *
 * Hint [type@GLib.VariantType]: [const@GLib.VARIANT_TYPE_STRING] (`s`).
 *
 * Since: 0.8.8
 */
#define NOTIFY_NOTIFICATION_HINT_SOUND_FILE "sound-file"

/**
 * NOTIFY_NOTIFICATION_HINT_SOUND_NAME:
 *
 * A themeable named sound from the freedesktop.org sound naming specification
 * to play when the notification pops up. Similar to icon-name, only for sounds.
 * An example would be "message-new-instant".
 *
 * Hint [type@GLib.VariantType]: [const@GLib.VARIANT_TYPE_STRING] (`s`).
 *
 * Since: 0.8.8
 */
#define NOTIFY_NOTIFICATION_HINT_SOUND_NAME "sound-name"

/**
 * NOTIFY_NOTIFICATION_HINT_SUPPRESS_SOUND:
 *
 * Causes the server to suppress playing any sounds, if it has that ability.
 * This is usually set when the client itself is going to play its own sound.
 *
 * Hint [type@GLib.VariantType]: [const@GLib.VARIANT_TYPE_BOOLEAN] (`b`).
 *
 * Since: 0.8.8
 */
#define NOTIFY_NOTIFICATION_HINT_SUPPRESS_SOUND "suppress-sound"

/**
 * NOTIFY_NOTIFICATION_HINT_TRANSIENT:
 *
 * When set the server will treat the notification as transient and by-pass the
 * server's persistence capability, if it should exist.
 *
 * Requires server supporting specification version >= 1.2.
 *
 * Hint [type@GLib.VariantType]: [const@GLib.VARIANT_TYPE_BOOLEAN] (`b`).
 *
 * Since: 0.8.8
 */
#define NOTIFY_NOTIFICATION_HINT_TRANSIENT "transient"

/**
 * NOTIFY_NOTIFICATION_HINT_X:
 *
 * Specifies the X location on the screen that the notification should point to.
 * The "y" hint must also be specified.
 *
 * Hint [type@GLib.VariantType]: [const@GLib.VARIANT_TYPE_INT32] (`i`).
 *
 * Since: 0.8.8
 */
#define NOTIFY_NOTIFICATION_HINT_X "x"

/**
 * NOTIFY_NOTIFICATION_HINT_Y:
 *
 * Specifies the Y location on the screen that the notification should point to.
 * The "x" hint must also be specified.
 *
 * Hint [type@GLib.VariantType]: [const@GLib.VARIANT_TYPE_INT32] (`i`).
 *
 * Since: 0.8.8
 */
#define NOTIFY_NOTIFICATION_HINT_Y "y"

/**
 * NOTIFY_NOTIFICATION_HINT_URGENCY:
 *
 * The urgency level.
 *
 * Hint [type@GLib.VariantType]: [const@GLib.VARIANT_TYPE_BYTE] (`y`).
 *
 * Since: 0.8.8
 */
#define NOTIFY_NOTIFICATION_HINT_URGENCY "urgency"

G_END_DECLS

