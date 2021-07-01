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

#define WINE_AFD_IOC(x) CTL_CODE(FILE_DEVICE_NETWORK, x, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_AFD_WINE_CREATE                           WINE_AFD_IOC(200)
#define IOCTL_AFD_WINE_ACCEPT                           WINE_AFD_IOC(201)
#define IOCTL_AFD_WINE_ACCEPT_INTO                      WINE_AFD_IOC(202)
#define IOCTL_AFD_WINE_CONNECT                          WINE_AFD_IOC(203)
#define IOCTL_AFD_WINE_SHUTDOWN                         WINE_AFD_IOC(204)
#define IOCTL_AFD_WINE_RECVMSG                          WINE_AFD_IOC(205)
#define IOCTL_AFD_WINE_SENDMSG                          WINE_AFD_IOC(206)
#define IOCTL_AFD_WINE_TRANSMIT                         WINE_AFD_IOC(207)
#define IOCTL_AFD_WINE_ADDRESS_LIST_CHANGE              WINE_AFD_IOC(208)
#define IOCTL_AFD_WINE_FIONBIO                          WINE_AFD_IOC(209)
#define IOCTL_AFD_WINE_COMPLETE_ASYNC                   WINE_AFD_IOC(210)
#define IOCTL_AFD_WINE_FIONREAD                         WINE_AFD_IOC(211)
#define IOCTL_AFD_WINE_SIOCATMARK                       WINE_AFD_IOC(212)
#define IOCTL_AFD_WINE_GET_INTERFACE_LIST               WINE_AFD_IOC(213)
#define IOCTL_AFD_WINE_KEEPALIVE_VALS                   WINE_AFD_IOC(214)
#define IOCTL_AFD_WINE_MESSAGE_SELECT                   WINE_AFD_IOC(215)
#define IOCTL_AFD_WINE_GETPEERNAME                      WINE_AFD_IOC(216)
#define IOCTL_AFD_WINE_DEFER                            WINE_AFD_IOC(217)
#define IOCTL_AFD_WINE_GET_INFO                         WINE_AFD_IOC(218)
#define IOCTL_AFD_WINE_GET_SO_ACCEPTCONN                WINE_AFD_IOC(219)
#define IOCTL_AFD_WINE_GET_SO_BROADCAST                 WINE_AFD_IOC(220)
#define IOCTL_AFD_WINE_SET_SO_BROADCAST                 WINE_AFD_IOC(221)
#define IOCTL_AFD_WINE_GET_SO_ERROR                     WINE_AFD_IOC(222)
#define IOCTL_AFD_WINE_GET_SO_KEEPALIVE                 WINE_AFD_IOC(223)
#define IOCTL_AFD_WINE_SET_SO_KEEPALIVE                 WINE_AFD_IOC(224)
#define IOCTL_AFD_WINE_GET_SO_LINGER                    WINE_AFD_IOC(225)
#define IOCTL_AFD_WINE_SET_SO_LINGER                    WINE_AFD_IOC(226)
#define IOCTL_AFD_WINE_GET_SO_OOBINLINE                 WINE_AFD_IOC(227)
#define IOCTL_AFD_WINE_SET_SO_OOBINLINE                 WINE_AFD_IOC(228)
#define IOCTL_AFD_WINE_SET_SO_RCVBUF                    WINE_AFD_IOC(229)
#define IOCTL_AFD_WINE_GET_SO_RCVBUF                    WINE_AFD_IOC(230)
#define IOCTL_AFD_WINE_SET_SO_RCVTIMEO                  WINE_AFD_IOC(231)
#define IOCTL_AFD_WINE_GET_SO_RCVTIMEO                  WINE_AFD_IOC(232)
#define IOCTL_AFD_WINE_GET_SO_REUSEADDR                 WINE_AFD_IOC(233)
#define IOCTL_AFD_WINE_SET_SO_REUSEADDR                 WINE_AFD_IOC(234)
#define IOCTL_AFD_WINE_SET_SO_SNDBUF                    WINE_AFD_IOC(235)
#define IOCTL_AFD_WINE_GET_SO_SNDBUF                    WINE_AFD_IOC(236)
#define IOCTL_AFD_WINE_GET_SO_SNDTIMEO                  WINE_AFD_IOC(237)
#define IOCTL_AFD_WINE_SET_SO_SNDTIMEO                  WINE_AFD_IOC(238)
#define IOCTL_AFD_WINE_SET_IP_ADD_MEMBERSHIP            WINE_AFD_IOC(239)
#define IOCTL_AFD_WINE_SET_IP_ADD_SOURCE_MEMBERSHIP     WINE_AFD_IOC(240)
#define IOCTL_AFD_WINE_SET_IP_BLOCK_SOURCE              WINE_AFD_IOC(241)
#define IOCTL_AFD_WINE_GET_IP_DONTFRAGMENT              WINE_AFD_IOC(242)
#define IOCTL_AFD_WINE_SET_IP_DONTFRAGMENT              WINE_AFD_IOC(243)

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
