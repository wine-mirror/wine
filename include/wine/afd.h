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
#include <mswsock.h>
#include "wine/server_protocol.h"

#ifdef USE_WS_PREFIX
# define WS(x)    WS_##x
#else
# define WS(x)    x
#endif

#define IOCTL_AFD_BIND                      CTL_CODE(FILE_DEVICE_BEEP, 0x800, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define IOCTL_AFD_LISTEN                    CTL_CODE(FILE_DEVICE_BEEP, 0x802, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define IOCTL_AFD_RECV                      CTL_CODE(FILE_DEVICE_BEEP, 0x805, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define IOCTL_AFD_POLL                      CTL_CODE(FILE_DEVICE_BEEP, 0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_GETSOCKNAME               CTL_CODE(FILE_DEVICE_BEEP, 0x80b, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define IOCTL_AFD_EVENT_SELECT              CTL_CODE(FILE_DEVICE_BEEP, 0x821, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define IOCTL_AFD_GET_EVENTS                CTL_CODE(FILE_DEVICE_BEEP, 0x822, METHOD_NEITHER,  FILE_ANY_ACCESS)

enum afd_poll_bit
{
    AFD_POLL_BIT_READ           = 0,
    AFD_POLL_BIT_OOB            = 1,
    AFD_POLL_BIT_WRITE          = 2,
    AFD_POLL_BIT_HUP            = 3,
    AFD_POLL_BIT_RESET          = 4,
    AFD_POLL_BIT_CLOSE          = 5,
    AFD_POLL_BIT_CONNECT        = 6,
    AFD_POLL_BIT_ACCEPT         = 7,
    AFD_POLL_BIT_CONNECT_ERR    = 8,
    /* IOCTL_AFD_GET_EVENTS has space for 13 events. */
    AFD_POLL_BIT_UNK1           = 9,
    AFD_POLL_BIT_UNK2           = 10,
    AFD_POLL_BIT_UNK3           = 11,
    AFD_POLL_BIT_UNK4           = 12,
    AFD_POLL_BIT_COUNT          = 13,
};

#define AFD_POLL_READ           0x0001
#define AFD_POLL_OOB            0x0002
#define AFD_POLL_WRITE          0x0004
#define AFD_POLL_HUP            0x0008
#define AFD_POLL_RESET          0x0010
#define AFD_POLL_CLOSE          0x0020
#define AFD_POLL_CONNECT        0x0040
#define AFD_POLL_ACCEPT         0x0080
#define AFD_POLL_CONNECT_ERR    0x0100
/* I have never seen these reported, but StarCraft Remastered polls for them. */
#define AFD_POLL_UNK1           0x0200
#define AFD_POLL_UNK2           0x0400

struct afd_bind_params
{
    int unknown;
    struct WS(sockaddr) addr; /* variable size */
};

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

#include <pshpack4.h>
struct afd_poll_params
{
    LONGLONG timeout;
    unsigned int count;
    BOOLEAN unknown;
    BOOLEAN padding[3];
    struct
    {
        SOCKET socket;
        int flags;
        NTSTATUS status;
    } sockets[1];
};
#include <poppack.h>

struct afd_event_select_params
{
    HANDLE event;
    int mask;
};

struct afd_event_select_params_64
{
    ULONGLONG event;
    int mask;
};

struct afd_event_select_params_32
{
    ULONG event;
    int mask;
};

struct afd_get_events_params
{
    int flags;
    NTSTATUS status[13];
};

#define IOCTL_AFD_WINE_CREATE               CTL_CODE(FILE_DEVICE_NETWORK, 200, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_ACCEPT               CTL_CODE(FILE_DEVICE_NETWORK, 201, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_ACCEPT_INTO          CTL_CODE(FILE_DEVICE_NETWORK, 202, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_CONNECT              CTL_CODE(FILE_DEVICE_NETWORK, 203, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_SHUTDOWN             CTL_CODE(FILE_DEVICE_NETWORK, 204, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_RECVMSG              CTL_CODE(FILE_DEVICE_NETWORK, 205, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_SENDMSG              CTL_CODE(FILE_DEVICE_NETWORK, 206, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_TRANSMIT             CTL_CODE(FILE_DEVICE_NETWORK, 207, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_ADDRESS_LIST_CHANGE  CTL_CODE(FILE_DEVICE_NETWORK, 208, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_FIONBIO              CTL_CODE(FILE_DEVICE_NETWORK, 209, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_COMPLETE_ASYNC       CTL_CODE(FILE_DEVICE_NETWORK, 210, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_FIONREAD             CTL_CODE(FILE_DEVICE_NETWORK, 211, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_SIOCATMARK           CTL_CODE(FILE_DEVICE_NETWORK, 212, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_GET_INTERFACE_LIST   CTL_CODE(FILE_DEVICE_NETWORK, 213, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_KEEPALIVE_VALS       CTL_CODE(FILE_DEVICE_NETWORK, 214, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_MESSAGE_SELECT       CTL_CODE(FILE_DEVICE_NETWORK, 215, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_GETPEERNAME          CTL_CODE(FILE_DEVICE_NETWORK, 216, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_DEFER                CTL_CODE(FILE_DEVICE_NETWORK, 217, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_GET_INFO             CTL_CODE(FILE_DEVICE_NETWORK, 218, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_GET_SO_ACCEPTCONN    CTL_CODE(FILE_DEVICE_NETWORK, 219, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_GET_SO_BROADCAST     CTL_CODE(FILE_DEVICE_NETWORK, 220, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_SET_SO_BROADCAST     CTL_CODE(FILE_DEVICE_NETWORK, 221, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_GET_SO_ERROR         CTL_CODE(FILE_DEVICE_NETWORK, 222, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_GET_SO_KEEPALIVE     CTL_CODE(FILE_DEVICE_NETWORK, 223, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_SET_SO_KEEPALIVE     CTL_CODE(FILE_DEVICE_NETWORK, 224, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_GET_SO_LINGER        CTL_CODE(FILE_DEVICE_NETWORK, 225, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_SET_SO_LINGER        CTL_CODE(FILE_DEVICE_NETWORK, 226, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_GET_SO_OOBINLINE     CTL_CODE(FILE_DEVICE_NETWORK, 227, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_WINE_SET_SO_OOBINLINE     CTL_CODE(FILE_DEVICE_NETWORK, 228, METHOD_BUFFERED, FILE_ANY_ACCESS)

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
    /* VARARG(addr, struct WS(sockaddr), addr_len); */
    /* VARARG(data, bytes); */
};

struct afd_recvmsg_params
{
    WSABUF *control;
    struct WS(sockaddr) *addr;
    int *addr_len;
    unsigned int *ws_flags;
    int force_async;
    unsigned int count;
    WSABUF *buffers;
};

struct afd_sendmsg_params
{
    const struct WS(sockaddr) *addr;
    unsigned int addr_len;
    unsigned int ws_flags;
    int force_async;
    unsigned int count;
    const WSABUF *buffers;
};

struct afd_transmit_params
{
    HANDLE file;
    DWORD file_len;
    DWORD buffer_size;
    LARGE_INTEGER offset;
    TRANSMIT_FILE_BUFFERS buffers;
    DWORD flags;
};

struct afd_message_select_params
{
    obj_handle_t handle;
    user_handle_t window;
    unsigned int message;
    int mask;
};

struct afd_get_info_params
{
    int family, type, protocol;
};

#endif
