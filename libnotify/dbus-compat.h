/**
 * @file libnotify/dbus-compat.h Private D-BUS Compatibility API
 *
 * @Copyright (C) 2005 Christian Hammond.
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
#ifndef _NOTIFY_DBUS_COMPAT_H_
#define _NOTIFY_DBUS_COMPAT_H_

#define NOTIFY_CHECK_DBUS_VERSION(major, minor) \
	(DBUS_MAJOR_VER > (major) || \
	 (DBUS_MAJOR_VER == (major) && DBUS_MINOR_VER >= (minor)))

#if NOTIFY_CHECK_DBUS_VERSION(0, 30)
# define _notify_dbus_message_iter_append_byte(iter, val) \
	dbus_message_iter_append_basic((iter), DBUS_TYPE_BYTE, &(val))
# define _notify_dbus_message_iter_append_boolean(iter, val) \
	dbus_message_iter_append_basic((iter), DBUS_TYPE_BOOLEAN, &(val))
# define _notify_dbus_message_iter_append_string(iter, val) \
	dbus_message_iter_append_basic((iter), DBUS_TYPE_STRING, &(val))
# define _notify_dbus_message_iter_append_int32(iter, val) \
	dbus_message_iter_append_basic((iter), DBUS_TYPE_INT32, &(val))
# define _notify_dbus_message_iter_append_uint32(iter, val) \
	dbus_message_iter_append_basic((iter), DBUS_TYPE_UINT32, &(val))
# define _notify_dbus_message_iter_append_double(iter, val) \
	dbus_message_iter_append_basic((iter), DBUS_TYPE_DOUBLE, &(val))

# define _notify_dbus_message_iter_append_byte_array(iter, data, len) \
	{ \
		DBusMessageIter array_iter; \
		dbus_message_iter_open_container((iter), DBUS_TYPE_ARRAY, \
										 DBUS_TYPE_BYTE_AS_STRING, \
										 &array_iter); \
		dbus_message_iter_append_fixed_array(&array_iter, DBUS_TYPE_BYTE, \
											 &(data), (len)); \
		dbus_message_iter_close_container((iter), &array_iter); \
	}
# define _notify_dbus_message_iter_append_boolean_array(iter, data, len) \
	{ \
		DBusMessageIter array_iter; \
		dbus_message_iter_open_container((iter), DBUS_TYPE_ARRAY, \
										 DBUS_TYPE_BOOLEAN_AS_STRING, \
										 &array_iter); \
		dbus_message_iter_append_fixed_array(&array_iter, DBUS_TYPE_BOOLEAN, \
											 &(data), (len)); \
		dbus_message_iter_close_container((iter), &array_iter); \
	}
# define _notify_dbus_message_iter_append_int32_array(iter, data, len) \
	{ \
		DBusMessageIter array_iter; \
		dbus_message_iter_open_container((iter), DBUS_TYPE_ARRAY, \
										 DBUS_TYPE_INT32_AS_STRING, \
										 &array_iter); \
		dbus_message_iter_append_fixed_array(&array_iter, DBUS_TYPE_INT32, \
											 &(data), (len)); \
		dbus_message_iter_close_container((iter), &array_iter); \
	}
# define _notify_dbus_message_iter_append_uint32_array(iter, data, len) \
	{ \
		DBusMessageIter array_iter; \
		dbus_message_iter_open_container((iter), DBUS_TYPE_ARRAY, \
										 DBUS_TYPE_UINT32_AS_STRING, \
										 &array_iter); \
		dbus_message_iter_append_fixed_array(&array_iter, DBUS_TYPE_UINT32, \
											 &(data), (len)); \
		dbus_message_iter_close_container((iter), &array_iter); \
	}
# define _notify_dbus_message_iter_append_string_array(iter, data, len) \
	{ \
		DBusMessageIter array_iter; \
		int i; \
		dbus_message_iter_open_container((iter), DBUS_TYPE_ARRAY, \
										 DBUS_TYPE_STRING_AS_STRING, \
										 &array_iter); \
		for (i = 0; i < len; i++) \
		{ \
			dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, \
										   &((char **)data)[i]); \
		} \
		dbus_message_iter_close_container((iter), &array_iter); \
	}

# define _notify_dbus_message_iter_get_byte(iter, retvar) \
	dbus_message_iter_get_basic((iter), &(retvar))
# define _notify_dbus_message_iter_get_boolean(iter, retvar) \
	dbus_message_iter_get_basic((iter), &(retvar))
# define _notify_dbus_message_iter_get_string(iter, retvar) \
	dbus_message_iter_get_basic((iter), &(retvar))
# define _notify_dbus_message_iter_get_int32(iter, retvar) \
	dbus_message_iter_get_basic((iter), &(retvar))
# define _notify_dbus_message_iter_get_uint32(iter, retvar) \
	dbus_message_iter_get_basic((iter), &(retvar))
# define _notify_dbus_message_iter_get_double(iter, retvar) \
	dbus_message_iter_get_basic((iter), &(retvar))

# define _notify_dbus_message_iter_get_fixed_array(iter, data, len) \
	{ \
		DBusMessageIter array_iter; \
		dbus_message_iter_recurse((iter), &array_iter); \
		dbus_message_iter_get_fixed_array(&array_iter, (data), (len)); \
	}

# define _notify_dbus_message_iter_get_byte_array(iter, data, len) \
	_notify_dbus_message_iter_get_fixed_array((iter), (data), (len))
# define _notify_dbus_message_iter_get_boolean_array(iter, data, len) \
	_notify_dbus_message_iter_get_fixed_array((iter), (data), (len))
# define _notify_dbus_message_iter_get_int32_array(iter, data, len) \
	_notify_dbus_message_iter_get_fixed_array((iter), (data), (len))
# define _notify_dbus_message_iter_get_uint32_array(iter, data, len) \
	_notify_dbus_message_iter_get_fixed_array((iter), (data), (len))

#else /* D-BUS < 0.30 */
# define DBUS_INTERFACE_DBUS        DBUS_INTERFACE_ORG_FREEDESKTOP_DBUS
# define DBUS_SERVICE_DBUS          DBUS_SERVICE_ORG_FREEDESKTOP_DBUS
# define DBUS_PATH_DBUS             DBUS_PATH_ORG_FREEDESKTOP_DBUS
# define DBUS_ERROR_SERVICE_UNKNOWN DBUS_ERROR_SERVICE_DOES_NOT_EXIST

# define dbus_message_iter_init_append(msg, iter) \
	dbus_message_iter_init(msg, iter)

# define dbus_bus_start_service_by_name(conn, service, flags, result, error) \
	dbus_bus_activate_service((conn), (service), (flags), (result), (error))

# define dbus_message_iter_close_container(iter, container_iter)

# define _notify_dbus_message_iter_append_byte(iter, val) \
	dbus_message_iter_append_byte((iter), (val))
# define _notify_dbus_message_iter_append_boolean(iter, val) \
	dbus_message_iter_append_boolean((iter), (val))
# define _notify_dbus_message_iter_append_string(iter, val) \
	dbus_message_iter_append_string((iter), (val))
# define _notify_dbus_message_iter_append_int32(iter, val) \
	dbus_message_iter_append_int32((iter), (val))
# define _notify_dbus_message_iter_append_uint32(iter, val) \
	dbus_message_iter_append_uint32((iter), (val))
# define _notify_dbus_message_iter_append_double(iter, val) \
	dbus_message_iter_append_double((iter), (val))

# define _notify_dbus_message_iter_append_byte_array(iter, data, len) \
	dbus_message_iter_append_byte_array((iter), (data), (len))
# define _notify_dbus_message_iter_append_boolean_array(iter, data, len) \
	dbus_message_iter_append_boolean_array((iter), (data), (len))
# define _notify_dbus_message_iter_append_int32_array(iter, data, len) \
	dbus_message_iter_append_int32_array((iter), (data), (len))
# define _notify_dbus_message_iter_append_uint32_array(iter, data, len) \
	dbus_message_iter_append_uint32_array((iter), (data), (len))
# define _notify_dbus_message_iter_append_string_array(iter, data, len) \
	dbus_message_iter_append_string_array((iter), (const char **)(data), (len))

# define _notify_dbus_message_iter_get_byte(iter, retvar) \
	retvar = dbus_message_iter_get_byte((iter))
# define _notify_dbus_message_iter_get_boolean(iter, retvar) \
	retvar = dbus_message_iter_get_boolean((iter))
# define _notify_dbus_message_iter_get_string(iter, retvar) \
	retvar = dbus_message_iter_get_string((iter))
# define _notify_dbus_message_iter_get_int32(iter, retvar) \
	retvar = dbus_message_iter_get_int32((iter))
# define _notify_dbus_message_iter_get_uint32(iter, retvar) \
	retvar = dbus_message_iter_get_uint32((iter))
# define _notify_dbus_message_iter_get_double(iter, retvar) \
	retvar = dbus_message_iter_get_double((iter))

# define _notify_dbus_message_iter_get_byte_array(iter, data, len) \
	dbus_message_iter_get_byte_array((iter), (data), (len))
# define _notify_dbus_message_iter_get_boolean_array(iter, data, len) \
	dbus_message_iter_get_boolean_array((iter), (data), (len))
# define _notify_dbus_message_iter_get_int32_array(iter, data, len) \
	dbus_message_iter_get_int32_array((iter), (data), (len))
# define _notify_dbus_message_iter_get_uint32_array(iter, data, len) \
	dbus_message_iter_get_uint32_array((iter), (data), (len))
#endif

#endif /* NOTIFY_DBUS_COMPAT_H_ */
