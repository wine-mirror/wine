/*
 * DBus declarations.
 *
 * Copyright 2024 Vibhav Pant
 * Copyright 2025 Vibhav Pant
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_BLUETOOTHAPIS_UNIXLIB_DBUS_H
#define __WINE_BLUETOOTHAPIS_UNIXLIB_DBUS_H

#include "config.h"

#ifdef SONAME_LIBDBUS_1
#include <dbus/dbus.h>
#endif

#ifdef SONAME_LIBDBUS_1

#define DBUS_FUNCS               \
    DO_FUNC(dbus_bus_add_match); \
    DO_FUNC(dbus_bus_get); \
    DO_FUNC(dbus_bus_get_id); \
    DO_FUNC(dbus_bus_get_private); \
    DO_FUNC(dbus_bus_remove_match); \
    DO_FUNC(dbus_connection_add_filter); \
    DO_FUNC(dbus_connection_close); \
    DO_FUNC(dbus_connection_flush); \
    DO_FUNC(dbus_connection_free_preallocated_send); \
    DO_FUNC(dbus_connection_get_is_anonymous); \
    DO_FUNC(dbus_connection_get_is_authenticated); \
    DO_FUNC(dbus_connection_get_is_connected); \
    DO_FUNC(dbus_connection_get_server_id); \
    DO_FUNC(dbus_connection_get_unix_process_id); \
    DO_FUNC(dbus_connection_get_unix_fd); \
    DO_FUNC(dbus_connection_get_unix_user); \
    DO_FUNC(dbus_connection_preallocate_send); \
    DO_FUNC(dbus_connection_read_write_dispatch); \
    DO_FUNC(dbus_connection_remove_filter); \
    DO_FUNC(dbus_connection_ref); \
    DO_FUNC(dbus_connection_send); \
    DO_FUNC(dbus_connection_send_preallocated); \
    DO_FUNC(dbus_connection_send_with_reply); \
    DO_FUNC(dbus_connection_send_with_reply_and_block); \
    DO_FUNC(dbus_connection_try_register_object_path); \
    DO_FUNC(dbus_connection_unref); \
    DO_FUNC(dbus_connection_unregister_object_path); \
    DO_FUNC(dbus_error_free); \
    DO_FUNC(dbus_error_has_name) ; \
    DO_FUNC(dbus_error_init); \
    DO_FUNC(dbus_error_is_set); \
    DO_FUNC(dbus_free); \
    DO_FUNC(dbus_free_string_array); \
    DO_FUNC(dbus_message_append_args); \
    DO_FUNC(dbus_message_get_args); \
    DO_FUNC(dbus_message_iter_get_element_count); \
    DO_FUNC(dbus_message_get_interface); \
    DO_FUNC(dbus_message_get_member); \
    DO_FUNC(dbus_message_get_path); \
    DO_FUNC(dbus_message_get_sender); \
    DO_FUNC(dbus_message_get_serial); \
    DO_FUNC(dbus_message_get_signature); \
    DO_FUNC(dbus_message_get_type); \
    DO_FUNC(dbus_message_has_signature); \
    DO_FUNC(dbus_message_iter_has_next); \
    DO_FUNC(dbus_message_is_error); \
    DO_FUNC(dbus_message_is_method_call); \
    DO_FUNC(dbus_message_is_signal); \
    DO_FUNC(dbus_message_iter_append_basic); \
    DO_FUNC(dbus_message_iter_abandon_container); \
    DO_FUNC(dbus_message_iter_close_container); \
    DO_FUNC(dbus_message_iter_get_arg_type); \
    DO_FUNC(dbus_message_iter_get_element_type); \
    DO_FUNC(dbus_message_iter_get_basic); \
    DO_FUNC(dbus_message_iter_get_fixed_array); \
    DO_FUNC(dbus_message_iter_get_signature); \
    DO_FUNC(dbus_message_iter_init); \
    DO_FUNC(dbus_message_iter_init_append); \
    DO_FUNC(dbus_message_iter_next); \
    DO_FUNC(dbus_message_iter_open_container); \
    DO_FUNC(dbus_message_iter_recurse); \
    DO_FUNC(dbus_message_new_error); \
    DO_FUNC(dbus_message_new_error_printf); \
    DO_FUNC(dbus_message_new_method_return); \
    DO_FUNC(dbus_message_new_method_call); \
    DO_FUNC(dbus_message_ref); \
    DO_FUNC(dbus_message_unref); \
    DO_FUNC(dbus_pending_call_block); \
    DO_FUNC(dbus_pending_call_cancel); \
    DO_FUNC(dbus_pending_call_get_completed); \
    DO_FUNC(dbus_pending_call_set_notify); \
    DO_FUNC(dbus_pending_call_steal_reply); \
    DO_FUNC(dbus_pending_call_unref); \
    DO_FUNC(dbus_set_error_from_message); \
    DO_FUNC(dbus_threads_init_default);

#define DO_FUNC( f ) extern typeof( f ) *p_##f
DBUS_FUNCS;
#undef DO_FUNC

#endif /* SONAME_LIBDBUS_1 */
#endif /* __WINE_BLUETOOTHAPIS_UNIXLIB_DBUS_H */
