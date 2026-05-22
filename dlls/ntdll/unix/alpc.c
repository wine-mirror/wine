/*
 * Advanced Local Procedure Call
 *
 * Copyright 2026 Zhiyi Zhang for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "ntstatus.h"
#include "wine/debug.h"
#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(alpc);

NTSTATUS WINAPI NtAlpcAcceptConnectPort( HANDLE *communication_port, HANDLE connection_port,
                                         DWORD flags, OBJECT_ATTRIBUTES *obj_attr,
                                         ALPC_PORT_ATTRIBUTES *port_attr, void *port_context,
                                         ALPC_PORT_MESSAGE *send_msg,
                                         ALPC_MESSAGE_ATTRIBUTES *send_msg_attr, BOOLEAN accept )
{
    FIXME( "%p, %p, %#x, %p, %p, %p, %p, %p, %d stub!\n", communication_port, connection_port,
           (unsigned int)flags, obj_attr, port_attr, port_context, send_msg, send_msg_attr, accept );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI NtAlpcConnectPort( HANDLE *port_handle, UNICODE_STRING *port_name,
                                   OBJECT_ATTRIBUTES *obj_attr, ALPC_PORT_ATTRIBUTES *port_attr,
                                   DWORD flags, PSID required_server_sid,
                                   ALPC_PORT_MESSAGE *connect_msg, SIZE_T *connect_msg_size,
                                   ALPC_MESSAGE_ATTRIBUTES *send_msg_attr,
                                   ALPC_MESSAGE_ATTRIBUTES *recv_msg_attr, LARGE_INTEGER *timeout )
{
    FIXME( "%p, %s, %p, %p, %#x, %p, %p, %p, %p, %p, %p stub!\n", port_handle,
           debugstr_us( port_name ), obj_attr, port_attr, (unsigned int)flags, required_server_sid,
           connect_msg, connect_msg_size, send_msg_attr, recv_msg_attr, timeout );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI NtAlpcCreatePort( HANDLE *port_handle, OBJECT_ATTRIBUTES *obj_attr, ALPC_PORT_ATTRIBUTES *port_attr )
{
    FIXME( "%p, %p, %p stub!\n", port_handle, obj_attr, port_attr );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI NtAlpcDisconnectPort( HANDLE port_handle, ULONG flags )
{
    FIXME( "%p, %#x stub!\n", port_handle, (unsigned int)flags );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI NtAlpcSendWaitReceivePort( HANDLE port_handle, ULONG flags,
                                           ALPC_PORT_MESSAGE *send_msg,
                                           ALPC_MESSAGE_ATTRIBUTES *send_msg_attr,
                                           ALPC_PORT_MESSAGE *recv_msg, SIZE_T *recv_buffer_size,
                                           ALPC_MESSAGE_ATTRIBUTES *recv_msg_attr,
                                           LARGE_INTEGER *timeout )
{
    FIXME( "%p, %#x, %p, %p, %p, %p, %p, %p stub!\n", port_handle, (unsigned int)flags, send_msg,
           send_msg_attr, recv_msg, recv_buffer_size, recv_msg_attr, timeout );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI NtAlpcImpersonateClientOfPort( HANDLE port_handle, ALPC_PORT_MESSAGE *msg, void *reserved )
{
    FIXME( "%p, %p, %p stub!\n", port_handle, msg, reserved );
    return STATUS_NOT_IMPLEMENTED;
}
