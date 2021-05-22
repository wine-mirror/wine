/*
 * Socket driver ioctls
 *
 * Copyright 2020 Zebediah Figura for CodeWeavers
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

#ifndef __WINE_WINE_AFD_H
#define __WINE_WINE_AFD_H

#include <winternl.h>
#include <winioctl.h>
#include "wine/server_protocol.h"

#define IOCTL_AFD_LISTEN                    CTL_CODE(FILE_DEVICE_BEEP, 0x802, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define IOCTL_AFD_RECV                      CTL_CODE(FILE_DEVICE_BEEP, 0x805, METHOD_NEITHER,  FILE_ANY_ACCESS)

struct afd_listen_params
{
    int unknown1;
    int backlog;
    int unknown2;
};

#define AFD_RECV_FORCE_ASYNC    0x2

#define AFD_MSG_NOT_OOB         0x0020
#define AFD_MSG_OOB             0x0040
#define AFD_MSG_PEEK            0x0080
#define AFD_MSG_WAITALL         0x4000

struct afd_recv_params
{
    const WSABUF *buffers;
    unsigned int count;
    int recv_flags;
    int msg_flags;
};

#define IOCTL_AFD_WINE_CREATE               CTL_CODE(FILE_DEVICE_NETWORK, 200, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_ACCEPT               CTL_CODE(FILE_DEVICE_NETWORK, 201, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_ACCEPT_INTO          CTL_CODE(FILE_DEVICE_NETWORK, 202, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_CONNECT              CTL_CODE(FILE_DEVICE_NETWORK, 203, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_SHUTDOWN             CTL_CODE(FILE_DEVICE_NETWORK, 204, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_AFD_WINE_ADDRESS_LIST_CHANGE  CTL_CODE(FILE_DEVICE_NETWORK, 323, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct afd_create_params
{
    int family, type, protocol;
    unsigned int flags;
};

struct afd_accept_into_params
{
    obj_handle_t accept_handle;
    unsigned int recv_len, local_len;
};

struct afd_connect_params
{
    int addr_len;
    int synchronous;
    /* VARARG(addr, struct sockaddr, addr_len); */
    /* VARARG(data, bytes); */
};

#endif
