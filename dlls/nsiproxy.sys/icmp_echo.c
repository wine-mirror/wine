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

#include "config.h"
#include <stdarg.h>

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <poll.h>
#include <sys/socket.h>

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

struct icmp_reply_ctx
{
    SOCKADDR_INET addr;
    ULONG status;
    ULONG round_trip_time;
    LONG data_size;
    BYTE ttl;
    BYTE tos;
    BYTE flags;
    BYTE options_size;
    void *options_data;
    void *data;
};

struct family_ops;
struct icmp_data
{
    LARGE_INTEGER send_time;
    int socket;
    int cancel_pipe[2];
    unsigned short id;
    unsigned short seq;
    const struct family_ops *ops;
    struct sockaddr_storage src_storage;
    struct sockaddr_storage dst_storage;
    int src_len;
    int dst_len;
    BOOL ping_socket;
};

#define MAX_HANDLES 256 /* Max number of simultaneous pings - could become dynamic if need be */
static struct icmp_data *handle_table[MAX_HANDLES];
static pthread_mutex_t handle_lock = PTHREAD_MUTEX_INITIALIZER;
static struct icmp_data **next_free, **next_unused = handle_table;

static icmp_handle handle_alloc( struct icmp_data *data )
{
    struct icmp_data **entry;
    icmp_handle h;

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
    h = entry - handle_table + 1;
    pthread_mutex_unlock( &handle_lock );
    TRACE( "returning handle %x\n", h );
    return h;
}

static struct icmp_data **handle_entry( icmp_handle h )
{
    if (!h || h > MAX_HANDLES)
    {
        ERR( "Invalid icmp handle\n" );
        return NULL;
    }
    return handle_table + h - 1;
}

static struct icmp_data *handle_data( icmp_handle h )
{
    struct icmp_data **entry = handle_entry( h );

    if (!entry) return NULL;
    return *entry;
}

static void handle_free( icmp_handle h )
{
    struct icmp_data **entry;

    TRACE( "%x\n", h );
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
static unsigned short chksum( struct icmp_data *icmp_data, BYTE *data, unsigned int count )
{
    unsigned int sum = 0, carry = 0;
    unsigned short check, s;

    if (icmp_data->ping_socket) return 0;

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

static int ipv4_set_reply_ip_status( IP_STATUS ip_status, unsigned int bits, void *out )
{
    if (bits == 32)
    {
        struct icmp_echo_reply_32 *reply = out;
        memset( reply, 0, sizeof(*reply) );
        reply->status = ip_status;
        return sizeof(*reply);
    }
    else
    {
        struct icmp_echo_reply_64 *reply = out;
        memset( reply, 0, sizeof(*reply) );
        reply->status = ip_status;
        return sizeof(*reply);
    }
}

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

static int ipv4_reply_buffer_len( struct icmp_listen_params *params )
{
    int struct_len = (params->bits == 32) ? sizeof(struct icmp_echo_reply_32) : sizeof(struct icmp_echo_reply_64);
    return sizeof(struct ip_hdr) + sizeof(struct icmp_hdr) + params->reply_len - struct_len;
}

#ifdef __linux__
static int ipv4_linux_ping_reply_buffer_len( struct icmp_listen_params *params )
{
    int struct_len = (params->bits == 32) ? sizeof(struct icmp_echo_reply_32) : sizeof(struct icmp_echo_reply_64);
    return sizeof(struct icmp_hdr) + params->reply_len - struct_len;
}
#endif

static BOOL ipv4_parse_ip_hdr( struct msghdr *msg, int recvd, int *ip_hdr_len,
                               struct icmp_reply_ctx *ctx )
{
    struct ip_hdr *ip_hdr;

    if (recvd < sizeof(*ip_hdr)) return FALSE;
    ip_hdr = msg->msg_iov[0].iov_base;
    if (ip_hdr->v_hl >> 4 != 4 || ip_hdr->protocol != IPPROTO_ICMP) return FALSE;
    *ip_hdr_len = (ip_hdr->v_hl & 0xf) << 2;
    if (*ip_hdr_len < sizeof(*ip_hdr)) return FALSE;
    ctx->options_data = ip_hdr + 1;
    ctx->ttl = ip_hdr->ttl;
    ctx->tos = ip_hdr->tos;
    ctx->flags = ip_hdr->frag_off >> 13;
    ctx->options_size = *ip_hdr_len - sizeof(*ip_hdr);

    return TRUE;
}

#ifdef __linux__
static BOOL ipv4_linux_ping_parse_ip_hdr( struct msghdr *msg, int recvd, int *ip_hdr_len,
                                          struct icmp_reply_ctx *ctx )
{
    struct cmsghdr *cmsg;

    *ip_hdr_len = 0;
    ctx->options_data = NULL;
    ctx->ttl = 0;
    ctx->tos = 0;
    ctx->flags = 0;
    ctx->options_size = 0; /* FIXME from IP_OPTIONS but will require checking for space in the reply */

    for (cmsg = CMSG_FIRSTHDR( msg ); cmsg; cmsg = CMSG_NXTHDR( msg, cmsg ))
    {
        if (cmsg->cmsg_level != IPPROTO_IP) continue;
        switch (cmsg->cmsg_type)
        {
        case IP_TTL:
            ctx->ttl = *(BYTE *)CMSG_DATA( cmsg );
            break;
        case IP_TOS:
            ctx->tos = *(BYTE *)CMSG_DATA( cmsg );
            break;
        }
    }
    return TRUE;
}
#endif

static int ipv4_parse_icmp_hdr( struct icmp_data *data, struct icmp_hdr *icmp, int icmp_size,
                                struct icmp_reply_ctx *ctx )
{
    static const IP_STATUS unreach_codes[] =
    {
        IP_DEST_NET_UNREACHABLE,  /* ICMP_UNREACH_NET */
        IP_DEST_HOST_UNREACHABLE, /* ICMP_UNREACH_HOST */
        IP_DEST_PROT_UNREACHABLE, /* ICMP_UNREACH_PROTOCOL */
        IP_DEST_PORT_UNREACHABLE, /* ICMP_UNREACH_PORT */
        IP_PACKET_TOO_BIG,        /* ICMP_UNREACH_NEEDFRAG */
        IP_BAD_ROUTE,             /* ICMP_UNREACH_SRCFAIL */
        IP_DEST_NET_UNREACHABLE,  /* ICMP_UNREACH_NET_UNKNOWN */
        IP_DEST_HOST_UNREACHABLE, /* ICMP_UNREACH_HOST_UNKNOWN */
        IP_DEST_HOST_UNREACHABLE, /* ICMP_UNREACH_ISOLATED */
        IP_DEST_NET_UNREACHABLE,  /* ICMP_UNREACH_NET_PROHIB */
        IP_DEST_HOST_UNREACHABLE, /* ICMP_UNREACH_HOST_PROHIB */
        IP_DEST_NET_UNREACHABLE,  /* ICMP_UNREACH_TOSNET */
        IP_DEST_HOST_UNREACHABLE, /* ICMP_UNREACH_TOSHOST */
        IP_DEST_HOST_UNREACHABLE, /* ICMP_UNREACH_FILTER_PROHIB */
        IP_DEST_HOST_UNREACHABLE, /* ICMP_UNREACH_HOST_PRECEDENCE */
        IP_DEST_HOST_UNREACHABLE  /* ICMP_UNREACH_PRECEDENCE_CUTOFF */
    };
    const struct ip_hdr *orig_ip_hdr;
    const struct icmp_hdr *orig_icmp_hdr;
    int orig_ip_hdr_len;
    IP_STATUS status;

    switch (icmp->type)
    {
    case ICMP4_ECHO_REPLY:
        if ((!data->ping_socket && icmp->un.echo.id != data->id) ||
            icmp->un.echo.sequence != data->seq) return -1;

        ctx->status = IP_SUCCESS;
        return icmp_size - sizeof(*icmp);

    case ICMP4_DST_UNREACH:
        if (icmp->code < ARRAY_SIZE(unreach_codes))
            status = unreach_codes[icmp->code];
        else
            status = IP_DEST_HOST_UNREACHABLE;
        break;

    case ICMP4_TIME_EXCEEDED:
        if (icmp->code == 1) /* ICMP_TIMXCEED_REASS */
            status = IP_TTL_EXPIRED_REASSEM;
        else
            status = IP_TTL_EXPIRED_TRANSIT;
        break;

    case ICMP4_PARAM_PROB:
        status = IP_PARAM_PROBLEM;
        break;

    case ICMP4_SOURCE_QUENCH:
        status = IP_SOURCE_QUENCH;
        break;

    default:
        return -1;
    }

    if (data->ping_socket) return 0;

    /* Check that the appended packet is really ours -
     * all handled icmp replies have an 8-byte header
     * followed by the original ip hdr. */
    if (icmp_size < sizeof(*icmp) + sizeof(*orig_ip_hdr)) return -1;
    orig_ip_hdr = (struct ip_hdr *)(icmp + 1);
    if (orig_ip_hdr->v_hl >> 4 != 4 || orig_ip_hdr->protocol != IPPROTO_ICMP) return -1;
    orig_ip_hdr_len = (orig_ip_hdr->v_hl & 0xf) << 2;
    if (icmp_size < sizeof(*icmp) + orig_ip_hdr_len + sizeof(*orig_icmp_hdr)) return -1;
    orig_icmp_hdr = (const struct icmp_hdr *)((const BYTE *)orig_ip_hdr + orig_ip_hdr_len);
    if (orig_icmp_hdr->type != ICMP4_ECHO_REQUEST ||
        orig_icmp_hdr->code != 0 ||
        (!data->ping_socket && orig_icmp_hdr->un.echo.id != data->id) ||
        orig_icmp_hdr->un.echo.sequence != data->seq) return -1;

    ctx->status = status;
    return 0;
}

static void ipv4_fill_reply( struct icmp_listen_params *params, struct icmp_reply_ctx *ctx)
{
    void *options_data;
    ULONG data_offset;
    if (params->bits == 32)
    {
        struct icmp_echo_reply_32 *reply = params->reply;
        data_offset = sizeof(*reply) + ((ctx->options_size + 3) & ~3);
        reply->addr = ctx->addr.Ipv4.sin_addr.WS_s_addr;
        reply->status = ctx->status;
        reply->round_trip_time = ctx->round_trip_time;
        reply->data_size = ctx->data_size;
        reply->num_of_pkts = 1;
        reply->data_ptr = params->user_reply_ptr + data_offset;
        reply->opts.ttl = ctx->ttl;
        reply->opts.tos = ctx->tos;
        reply->opts.flags = ctx->flags;
        reply->opts.options_size = ctx->options_size;
        reply->opts.options_ptr = params->user_reply_ptr + sizeof(*reply);
        options_data = reply + 1;
    }
    else
    {
        struct icmp_echo_reply_64 *reply = params->reply;
        data_offset = sizeof(*reply) + ((ctx->options_size + 3) & ~3);
        reply->addr = ctx->addr.Ipv4.sin_addr.WS_s_addr;
        reply->status = ctx->status;
        reply->round_trip_time = ctx->round_trip_time;
        reply->data_size = ctx->data_size;
        reply->num_of_pkts = 1;
        reply->data_ptr = params->user_reply_ptr + data_offset;
        reply->opts.ttl = ctx->ttl;
        reply->opts.tos = ctx->tos;
        reply->opts.flags = ctx->flags;
        reply->opts.options_size = ctx->options_size;
        reply->opts.options_ptr = params->user_reply_ptr + sizeof(*reply);
        options_data = reply + 1;
    }

    memcpy( options_data, ctx->options_data, ctx->options_size );
    if (ctx->options_size & 3)
        memset( (char *)options_data + ctx->options_size, 0, 4 - (ctx->options_size & 3) );

    memcpy( (char *)params->reply + data_offset, ctx->data, ctx->data_size );
    params->reply_len = data_offset + ctx->data_size;
}

struct family_ops
{
    int family;
    int icmp_protocol;
    void (*init_icmp_hdr)( struct icmp_data *data, struct icmp_hdr *icmp_hdr );
    unsigned short (*chksum)( struct icmp_data *icmp_data, BYTE *data, unsigned int count );
    int (*set_reply_ip_status)( IP_STATUS ip_status, unsigned int bits, void *out );
    void (*set_socket_opts)( struct icmp_data *data, struct icmp_send_echo_params *params );
    int (*reply_buffer_len)( struct icmp_listen_params *params );
    BOOL (*parse_ip_hdr)( struct msghdr *msg, int recvd, int *ip_hdr_len, struct icmp_reply_ctx *ctx );
    int (*parse_icmp_hdr)( struct icmp_data *data, struct icmp_hdr *icmp, int icmp_len, struct icmp_reply_ctx *ctx );
    void (*fill_reply)( struct icmp_listen_params *params, struct icmp_reply_ctx *ctx );
};

static const struct family_ops ipv4 =
{
    AF_INET,
    IPPROTO_ICMP,
    ipv4_init_icmp_hdr,
    chksum,
    ipv4_set_reply_ip_status,
    ipv4_set_socket_opts,
    ipv4_reply_buffer_len,
    ipv4_parse_ip_hdr,
    ipv4_parse_icmp_hdr,
    ipv4_fill_reply,
};

#ifdef __linux__
/* linux ipv4 ping sockets behave more like ipv6 raw sockets */
static const struct family_ops ipv4_linux_ping =
{
    AF_INET,
    IPPROTO_ICMP,
    ipv4_init_icmp_hdr,
    chksum,
    ipv4_set_reply_ip_status,
    ipv4_linux_ping_set_socket_opts,
    ipv4_linux_ping_reply_buffer_len,
    ipv4_linux_ping_parse_ip_hdr,
    ipv4_parse_icmp_hdr,
    ipv4_fill_reply,
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

static BOOL sockaddr_to_SOCKADDR_INET( const struct sockaddr *in, SOCKADDR_INET *out )
{
    switch (in->sa_family)
    {
    case AF_INET:
    {
        struct sockaddr_in *sa = (struct sockaddr_in *)in;

        out->Ipv4.sin_family = WS_AF_INET;
        out->Ipv4.sin_port = sa->sin_port;
        out->Ipv4.sin_addr.WS_s_addr = sa->sin_addr.s_addr;
        return TRUE;
    }
    case AF_INET6:
    {
        struct sockaddr_in6 *sa = (struct sockaddr_in6 *)in;

        out->Ipv6.sin6_family = WS_AF_INET6;
        out->Ipv6.sin6_port = sa->sin6_port;
        out->Ipv6.sin6_flowinfo = sa->sin6_flowinfo;
        memcpy( out->Ipv6.sin6_addr.WS_s6_addr, sa->sin6_addr.s6_addr, sizeof(sa->sin6_addr.s6_addr) );
        out->Ipv6.sin6_scope_id = sa->sin6_scope_id;
        return TRUE;
    }
    }
    return FALSE;
}

static NTSTATUS icmp_data_create( ADDRESS_FAMILY win_family, struct icmp_data **icmp_data )
{
    struct icmp_data *data;
    const struct family_ops *ops;

    if (win_family == WS_AF_INET) ops = &ipv4;
    else return STATUS_INVALID_PARAMETER;

    data = malloc( sizeof(*data) );
    if (!data) return STATUS_NO_MEMORY;

    data->ping_socket = FALSE;
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
        data->ping_socket = TRUE;
#endif
    }
    if (pipe( data->cancel_pipe ))
    {
        close( data->socket );
        free( data );
        return STATUS_ACCESS_DENIED;
    }

    data->ops = ops;
    *icmp_data = data;
    return STATUS_SUCCESS;
}

static void icmp_data_free( struct icmp_data *data )
{
    close( data->socket );
    close( data->cancel_pipe[0] );
    close( data->cancel_pipe[1] );
    free( data );
}

NTSTATUS icmp_send_echo( void *args )
{
    struct icmp_send_echo_params *params = args;
    struct icmp_hdr *icmp_hdr; /* this is the same for both ipv4 and ipv6 */
    struct icmp_data *data;
    int ret;
    struct sockaddr *src, *dst;

    NTSTATUS status;

    status = icmp_data_create( params->dst->si_family, &data );
    if (status) return status;

    src = (struct sockaddr *)&data->src_storage;
    dst = (struct sockaddr *)&data->dst_storage;
    data->src_len = SOCKADDR_INET_to_sockaddr( params->src, src, sizeof(data->src_storage) );
    data->dst_len = SOCKADDR_INET_to_sockaddr( params->dst, dst, sizeof(data->dst_storage) );

    if (bind( data->socket, src, data->src_len ))
    {
        icmp_data_free( data );
        return STATUS_INVALID_ADDRESS_COMPONENT;
    }

    data->ops->set_socket_opts( data, params );

    icmp_hdr = malloc( sizeof(*icmp_hdr) + params->request_size );
    if (!icmp_hdr)
    {
        icmp_data_free( data );
        return STATUS_NO_MEMORY;
    }
    data->ops->init_icmp_hdr( data, icmp_hdr );
    memcpy( icmp_hdr + 1, params->request, params->request_size );
    icmp_hdr->checksum = data->ops->chksum( data, (BYTE *)icmp_hdr, sizeof(*icmp_hdr) + params->request_size );

    NtQueryPerformanceCounter( &data->send_time, NULL );
    ret = sendto( data->socket, icmp_hdr, sizeof(*icmp_hdr) + params->request_size, 0, dst, data->dst_len );
    free( icmp_hdr );

    if (ret < 0)
    {
        TRACE( "sendto() rets %d errno %d\n", ret, errno );
        params->reply_len = data->ops->set_reply_ip_status( errno_to_ip_status( errno ), params->bits, params->reply );
        icmp_data_free( data );
        return STATUS_SUCCESS;
    }

    *params->handle = handle_alloc( data );
    if (!*params->handle) icmp_data_free( data );
    return *params->handle ? STATUS_PENDING : STATUS_NO_MEMORY;
}

static int get_timeout( LARGE_INTEGER start, UINT timeout )
{
    LARGE_INTEGER now, end;

    end.QuadPart = start.QuadPart + (ULONGLONG)timeout * 10000;
    NtQueryPerformanceCounter( &now, NULL );
    if (now.QuadPart >= end.QuadPart) return 0;

    return min( (end.QuadPart - now.QuadPart) / 10000, INT_MAX );
}

static ULONG get_rtt( LARGE_INTEGER start )
{
    LARGE_INTEGER now;

    NtQueryPerformanceCounter( &now, NULL );
    return (now.QuadPart - start.QuadPart) / 10000;
}

static NTSTATUS recv_msg( struct icmp_data *data, struct icmp_listen_params *params )
{
    struct sockaddr_storage addr;
    struct icmp_reply_ctx ctx;
    struct iovec iov[1];
    BYTE cmsg_buf[1024];
    struct msghdr msg = { .msg_name = &addr, .msg_namelen = sizeof(addr),
                          .msg_iov = iov, .msg_iovlen = ARRAY_SIZE(iov),
                          .msg_control = cmsg_buf, .msg_controllen = sizeof(cmsg_buf) };
    int ip_hdr_len, recvd, reply_buf_len;
    char *reply_buf;
    struct icmp_hdr *icmp_hdr;

    reply_buf_len = data->ops->reply_buffer_len( params );
    reply_buf = malloc( reply_buf_len );
    if (!reply_buf) return STATUS_NO_MEMORY;

    iov[0].iov_base = reply_buf;
    iov[0].iov_len = reply_buf_len;

    recvd = recvmsg( data->socket, &msg, 0 );
    TRACE( "recvmsg() rets %d errno %d addr_len %d iovlen %d msg_flags %x\n",
           recvd, errno, msg.msg_namelen, (int)iov[0].iov_len, msg.msg_flags );

    if (recvd < 0) goto skip;
    if (!data->ops->parse_ip_hdr( &msg, recvd, &ip_hdr_len, &ctx )) goto skip;
    if (recvd < ip_hdr_len + sizeof(*icmp_hdr)) goto skip;

    icmp_hdr = (struct icmp_hdr *)(reply_buf + ip_hdr_len);
    if ((ctx.data_size = data->ops->parse_icmp_hdr( data, icmp_hdr, recvd - ip_hdr_len, &ctx )) < 0) goto skip;
    if (ctx.data_size && msg.msg_flags & MSG_TRUNC)
    {
        free( reply_buf );
        params->reply_len = data->ops->set_reply_ip_status( IP_GENERAL_FAILURE, params->bits, params->reply );
        return STATUS_SUCCESS;
    }

    sockaddr_to_SOCKADDR_INET( (struct sockaddr *)&addr, &ctx.addr );
    ctx.round_trip_time = get_rtt( data->send_time );
    ctx.data = icmp_hdr + 1;

    data->ops->fill_reply( params, &ctx );

    free( reply_buf );
    return STATUS_SUCCESS;

skip:
    free( reply_buf );
    return STATUS_RETRY;
}

NTSTATUS icmp_listen( void *args )
{
    struct icmp_listen_params *params = args;
    struct icmp_data *data;
    struct pollfd fds[2];
    NTSTATUS status;
    int ret;

    data = handle_data( params->handle );
    if (!data) return STATUS_INVALID_PARAMETER;

    fds[0].fd = data->socket;
    fds[0].events = POLLIN;
    fds[1].fd = data->cancel_pipe[0];
    fds[1].events = POLLIN;

    while ((ret = poll( fds, ARRAY_SIZE(fds), get_timeout( data->send_time, params->timeout ) )) > 0)
    {
        if (fds[1].revents & POLLIN)
        {
            TRACE( "cancelled\n" );
            return STATUS_CANCELLED;
        }

        status = recv_msg( data, params );
        if (status == STATUS_RETRY) continue;
        return status;
    }

    if (!ret) /* timeout */
    {
        TRACE( "timeout\n" );
        params->reply_len = data->ops->set_reply_ip_status( IP_REQ_TIMED_OUT, params->bits, params->reply );
        return STATUS_SUCCESS;
    }
    /* ret < 0 */
    params->reply_len = data->ops->set_reply_ip_status( errno_to_ip_status( errno ), params->bits, params->reply );
    return STATUS_SUCCESS;
}

NTSTATUS icmp_cancel_listen( void *args )
{
    struct icmp_cancel_listen_params *params = args;
    struct icmp_data *data = handle_data( params->handle );

    if (!data) return STATUS_INVALID_PARAMETER;
    write( data->cancel_pipe[1], "x", 1 );
    return STATUS_SUCCESS;
}

NTSTATUS icmp_close( void *args )
{
    struct icmp_close_params *params = args;
    struct icmp_data *data = handle_data( params->handle );

    if (!data) return STATUS_INVALID_PARAMETER;
    icmp_data_free( data );
    handle_free( params->handle );
    return STATUS_SUCCESS;
}
