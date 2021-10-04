/*
 * nsiproxy.sys icmp_echo implementation
 *
 * Copyright 2021 Huw Davies
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

#define _NTSYSTEM_

#include "config.h"
#include <stdarg.h>

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#define USE_WS_PREFIX
#include "ddk/wdm.h"
#include "ifdef.h"
#include "netiodef.h"
#include "ipexport.h"
#include "ipmib.h"
#include "wine/nsi.h"
#include "wine/debug.h"

#include "nsiproxy_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(nsi);

static LONG icmp_sequence;

struct ip_hdr
{
    uint8_t v_hl; /* version << 4 | hdr_len */
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t saddr;
    uint32_t daddr;
};

struct icmp_hdr
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    union
    {
        struct
        {
            uint16_t id;
            uint16_t sequence;
        } echo;
    } un;
};

struct family_ops;
struct icmp_data
{
    LARGE_INTEGER send_time;
    int socket;
    unsigned short id;
    unsigned short seq;
    const struct family_ops *ops;
};

#define MAX_HANDLES 256 /* Max number of simultaneous pings - could become dynamic if need be */
static struct icmp_data *handle_table[MAX_HANDLES];
static pthread_mutex_t handle_lock = PTHREAD_MUTEX_INITIALIZER;
static struct icmp_data **next_free, **next_unused = handle_table;

static HANDLE handle_alloc( struct icmp_data *data )
{
    struct icmp_data **entry;
    HANDLE h;

    pthread_mutex_lock( &handle_lock );
    entry = next_free;
    if (entry) next_free = *(struct icmp_data ***)entry;
    else if (next_unused < handle_table + MAX_HANDLES) entry = next_unused++;
    else
    {
        pthread_mutex_unlock( &handle_lock );
        FIXME( "Exhausted icmp handle count\n" );
        return 0;
    }
    *entry = data;
    h = LongToHandle( entry - handle_table + 1 );
    pthread_mutex_unlock( &handle_lock );
    TRACE( "returning handle %p\n", h );
    return h;
}

static struct icmp_data **handle_entry( HANDLE h )
{
    unsigned int idx = HandleToLong( h );

    if (!idx || idx > MAX_HANDLES)
    {
        ERR( "Invalid icmp handle\n" );
        return NULL;
    }
    return handle_table + idx - 1;
}

static struct icmp_data *handle_data( HANDLE h )
{
    struct icmp_data **entry = handle_entry( h );

    if (!entry) return NULL;
    return *entry;
}

static void handle_free( HANDLE h )
{
    struct icmp_data **entry;

    TRACE( "%p\n", h );
    pthread_mutex_lock( &handle_lock );
    entry = handle_entry( h );
    if (entry)
    {
        *(struct icmp_data ***)entry = next_free;
        next_free = entry;
    }
    pthread_mutex_unlock( &handle_lock );
}

static void ipv4_init_icmp_hdr( struct icmp_data *data, struct icmp_hdr *icmp_hdr )
{
    icmp_hdr->type = ICMP4_ECHO_REQUEST;
    icmp_hdr->code = 0;
    icmp_hdr->checksum = 0;
    icmp_hdr->un.echo.id = data->id = getpid() & 0xffff; /* will be overwritten for linux ping socks */
    icmp_hdr->un.echo.sequence = data->seq = InterlockedIncrement( &icmp_sequence ) & 0xffff;
}

/* rfc 1071 checksum */
static unsigned short chksum( BYTE *data, unsigned int count )
{
    unsigned int sum = 0, carry = 0;
    unsigned short check, s;

    while (count > 1)
    {
        s = *(unsigned short *)data;
        data += 2;
        sum += carry;
        sum += s;
        carry = s > sum;
        count -= 2;
    }
    sum += carry; /* This won't produce another carry */
    sum = (sum & 0xffff) + (sum >> 16);

    if (count) sum += *data; /* LE-only */

    sum = (sum & 0xffff) + (sum >> 16);
    /* fold in any carry */
    sum = (sum & 0xffff) + (sum >> 16);

    check = ~sum;
    return check;
}

#ifdef __linux__
static unsigned short null_chksum( BYTE *data, unsigned int count )
{
    return 0;
}
#endif

static void ipv4_set_socket_opts( struct icmp_data *data, struct icmp_send_echo_params *params )
{
    int val;

    val = params->ttl;
    if (val) setsockopt( data->socket, IPPROTO_IP, IP_TTL, &val, sizeof(val) );
    val = params->tos;
    if (val) setsockopt( data->socket, IPPROTO_IP, IP_TOS, &val, sizeof(val) );
}

#ifdef __linux__
static void ipv4_linux_ping_set_socket_opts( struct icmp_data *data, struct icmp_send_echo_params *params )
{
    static const int val = 1;

    ipv4_set_socket_opts( data, params );

    setsockopt( data->socket, IPPROTO_IP, IP_RECVTTL, &val, sizeof(val) );
    setsockopt( data->socket, IPPROTO_IP, IP_RECVTOS, &val, sizeof(val) );
}
#endif

struct family_ops
{
    int family;
    int icmp_protocol;
    void (*init_icmp_hdr)( struct icmp_data *data, struct icmp_hdr *icmp_hdr );
    unsigned short (*chksum)( BYTE *data, unsigned int count );
    void (*set_socket_opts)( struct icmp_data *data, struct icmp_send_echo_params *params );
};

static const struct family_ops ipv4 =
{
    AF_INET,
    IPPROTO_ICMP,
    ipv4_init_icmp_hdr,
    chksum,
    ipv4_set_socket_opts,
};

#ifdef __linux__
/* linux ipv4 ping sockets behave more like ipv6 raw sockets */
static const struct family_ops ipv4_linux_ping =
{
    AF_INET,
    IPPROTO_ICMP,
    ipv4_init_icmp_hdr,
    null_chksum,
    ipv4_linux_ping_set_socket_opts,
};
#endif

static IP_STATUS errno_to_ip_status( int err )
{
    switch( err )
    {
    case EHOSTUNREACH: return IP_DEST_HOST_UNREACHABLE;
    default: return IP_GENERAL_FAILURE;
    }
}

static int SOCKADDR_INET_to_sockaddr( const SOCKADDR_INET *in, struct sockaddr *out, int len )
{
    switch (in->si_family)
    {
    case WS_AF_INET:
    {
        struct sockaddr_in *sa = (struct sockaddr_in *)out;

        if (len < sizeof(*sa)) return 0;
        sa->sin_family = AF_INET;
        sa->sin_port = in->Ipv4.sin_port;
        sa->sin_addr.s_addr = in->Ipv4.sin_addr.WS_s_addr;
        return sizeof(*sa);
    }
    case WS_AF_INET6:
    {
        struct sockaddr_in6 *sa = (struct sockaddr_in6 *)out;

        if (len < sizeof(*sa)) return 0;
        sa->sin6_family = AF_INET6;
        sa->sin6_port = in->Ipv6.sin6_port;
        sa->sin6_flowinfo = in->Ipv6.sin6_flowinfo;
        memcpy( sa->sin6_addr.s6_addr, in->Ipv6.sin6_addr.WS_s6_addr, sizeof(sa->sin6_addr.s6_addr) );
        sa->sin6_scope_id = in->Ipv6.sin6_scope_id;
        return sizeof(*sa);
    }
    }
    return 0;
}

static NTSTATUS icmp_data_create( ADDRESS_FAMILY win_family, struct icmp_data **icmp_data )
{
    struct icmp_data *data;
    const struct family_ops *ops;

    if (win_family == WS_AF_INET) ops = &ipv4;
    else return STATUS_INVALID_PARAMETER;

    data = malloc( sizeof(*data) );
    if (!data) return STATUS_NO_MEMORY;

    data->socket = socket( ops->family, SOCK_RAW, ops->icmp_protocol );
    if (data->socket < 0) /* Try a ping-socket */
    {
        TRACE( "failed to open raw sock, trying a dgram sock\n" );
        data->socket = socket( ops->family, SOCK_DGRAM, ops->icmp_protocol );
        if (data->socket < 0)
        {
            WARN( "Unable to create socket\n" );
            free( data );
            return STATUS_ACCESS_DENIED;
        }
#ifdef __linux__
        if (ops->family == AF_INET) ops = &ipv4_linux_ping;
#endif
    }
    data->ops = ops;

    *icmp_data = data;
    return STATUS_SUCCESS;
}

static void icmp_data_free( struct icmp_data *data )
{
    close( data->socket );
    free( data );
}

NTSTATUS icmp_send_echo( void *args )
{
    struct icmp_send_echo_params *params = args;
    struct icmp_hdr *icmp_hdr; /* this is the same for both ipv4 and ipv6 */
    struct sockaddr_storage dst_storage;
    struct sockaddr *dst = (struct sockaddr *)&dst_storage;
    struct icmp_data *data;
    int dst_len, ret;
    NTSTATUS status;

    status = icmp_data_create( params->dst->si_family, &data );
    if (status) return status;
    data->ops->set_socket_opts( data, params );

    icmp_hdr = malloc( sizeof(*icmp_hdr) + params->request_size );
    if (!icmp_hdr)
    {
        icmp_data_free( data );
        return STATUS_NO_MEMORY;
    }
    data->ops->init_icmp_hdr( data, icmp_hdr );
    memcpy( icmp_hdr + 1, params->request, params->request_size );
    icmp_hdr->checksum = data->ops->chksum( (BYTE *)icmp_hdr, sizeof(*icmp_hdr) + params->request_size );

    dst_len = SOCKADDR_INET_to_sockaddr( params->dst, dst, sizeof(dst_storage) );

    NtQueryPerformanceCounter( &data->send_time, NULL );
    ret = sendto( data->socket, icmp_hdr, sizeof(*icmp_hdr) + params->request_size, 0, dst, dst_len );
    free( icmp_hdr );

    if (ret < 0)
    {
        TRACE( "sendto() rets %d errno %d\n", ret, errno );
        icmp_data_free( data );
        params->ip_status = errno_to_ip_status( errno );
        return STATUS_SUCCESS;
    }

    params->handle = handle_alloc( data );
    if (!params->handle) icmp_data_free( data );
    return params->handle ? STATUS_PENDING : STATUS_NO_MEMORY;
}

NTSTATUS icmp_close( void *args )
{
    HANDLE handle = args;
    struct icmp_data *data = handle_data( handle );

    if (!data) return STATUS_INVALID_PARAMETER;
    icmp_data_free( data );
    handle_free( handle );
    return STATUS_SUCCESS;
}
