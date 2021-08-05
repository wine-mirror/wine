/*
 * Unix library functions
 *
 * Copyright (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka.
 * Copyright (C) 2001 Stefan Leichter
 * Copyright (C) 2004 Hans Leidekker
 * Copyright (C) 2005 Marcus Meissner
 * Copyright (C) 2006-2008 Kai Blin
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
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#ifdef HAVE_NETIPX_IPX_H
# include <netipx/ipx.h>
#elif defined(HAVE_LINUX_IPX_H)
# ifdef HAVE_ASM_TYPES_H
#  include <asm/types.h>
# endif
# ifdef HAVE_LINUX_TYPES_H
#  include <linux/types.h>
# endif
# include <linux/ipx.h>
#endif
#if defined(SOL_IPX) || defined(SO_DEFAULT_HEADERS)
# define HAS_IPX
#endif

#ifdef HAVE_LINUX_IRDA_H
# ifdef HAVE_LINUX_TYPES_H
#  include <linux/types.h>
# endif
# include <linux/irda.h>
# define HAS_IRDA
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winerror.h"
#include "winternl.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#include "wsipx.h"
#include "af_irda.h"
#include "wine/debug.h"

#include "ws2_32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winsock);

#ifndef HAVE_LINUX_GETHOSTBYNAME_R_6
static pthread_mutex_t host_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#define MAP(x) {WS_ ## x, x}

static const int addrinfo_flag_map[][2] =
{
    MAP( AI_PASSIVE ),
    MAP( AI_CANONNAME ),
    MAP( AI_NUMERICHOST ),
#ifdef AI_NUMERICSERV
    MAP( AI_NUMERICSERV ),
#endif
#ifdef AI_V4MAPPED
    MAP( AI_V4MAPPED ),
#endif
    MAP( AI_ALL ),
    MAP( AI_ADDRCONFIG ),
};

static const int family_map[][2] =
{
    MAP( AF_UNSPEC ),
    MAP( AF_INET ),
    MAP( AF_INET6 ),
#ifdef AF_IPX
    MAP( AF_IPX ),
#endif
#ifdef AF_IRDA
    MAP( AF_IRDA ),
#endif
};

static const int socktype_map[][2] =
{
    MAP( SOCK_STREAM ),
    MAP( SOCK_DGRAM ),
    MAP( SOCK_RAW ),
};

static const int ip_protocol_map[][2] =
{
    MAP( IPPROTO_IP ),
    MAP( IPPROTO_TCP ),
    MAP( IPPROTO_UDP ),
    MAP( IPPROTO_IPV6 ),
    MAP( IPPROTO_ICMP ),
    MAP( IPPROTO_IGMP ),
    MAP( IPPROTO_RAW ),
    {WS_IPPROTO_IPV4, IPPROTO_IPIP},
};

#undef MAP

static int addrinfo_flags_from_unix( int flags )
{
    int ws_flags = 0;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(addrinfo_flag_map); ++i)
    {
        if (flags & addrinfo_flag_map[i][1])
        {
            ws_flags |= addrinfo_flag_map[i][0];
            flags &= ~addrinfo_flag_map[i][1];
        }
    }

    if (flags)
        FIXME( "unhandled flags %#x\n", flags );
    return ws_flags;
}

static int addrinfo_flags_to_unix( int flags )
{
    int unix_flags = 0;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(addrinfo_flag_map); ++i)
    {
        if (flags & addrinfo_flag_map[i][0])
        {
            unix_flags |= addrinfo_flag_map[i][1];
            flags &= ~addrinfo_flag_map[i][0];
        }
    }

    if (flags)
        FIXME( "unhandled flags %#x\n", flags );
    return unix_flags;
}

static int family_from_unix( int family )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(family_map); ++i)
    {
        if (family == family_map[i][1])
            return family_map[i][0];
    }

    FIXME( "unhandled family %u\n", family );
    return -1;
}

static int family_to_unix( int family )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(family_map); ++i)
    {
        if (family == family_map[i][0])
            return family_map[i][1];
    }

    FIXME( "unhandled family %u\n", family );
    return -1;
}

static int socktype_from_unix( int type )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(socktype_map); ++i)
    {
        if (type == socktype_map[i][1])
            return socktype_map[i][0];
    }

    FIXME( "unhandled type %u\n", type );
    return -1;
}

static int socktype_to_unix( int type )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(socktype_map); ++i)
    {
        if (type == socktype_map[i][0])
            return socktype_map[i][1];
    }

    FIXME( "unhandled type %u\n", type );
    return -1;
}

static int protocol_from_unix( int protocol )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(ip_protocol_map); ++i)
    {
        if (protocol == ip_protocol_map[i][1])
            return ip_protocol_map[i][0];
    }

    if (protocol >= WS_NSPROTO_IPX && protocol <= WS_NSPROTO_IPX + 255)
        return protocol;

    FIXME( "unhandled protocol %u\n", protocol );
    return -1;
}

static int protocol_to_unix( int protocol )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(ip_protocol_map); ++i)
    {
        if (protocol == ip_protocol_map[i][0])
            return ip_protocol_map[i][1];
    }

    if (protocol >= WS_NSPROTO_IPX && protocol <= WS_NSPROTO_IPX + 255)
        return protocol;

    FIXME( "unhandled protocol %u\n", protocol );
    return -1;
}

static unsigned int errno_from_unix( int err )
{
    switch (err)
    {
        case EINTR:             return WSAEINTR;
        case EBADF:             return WSAEBADF;
        case EPERM:
        case EACCES:            return WSAEACCES;
        case EFAULT:            return WSAEFAULT;
        case EINVAL:            return WSAEINVAL;
        case EMFILE:            return WSAEMFILE;
        case EINPROGRESS:
        case EWOULDBLOCK:       return WSAEWOULDBLOCK;
        case EALREADY:          return WSAEALREADY;
        case ENOTSOCK:          return WSAENOTSOCK;
        case EDESTADDRREQ:      return WSAEDESTADDRREQ;
        case EMSGSIZE:          return WSAEMSGSIZE;
        case EPROTOTYPE:        return WSAEPROTOTYPE;
        case ENOPROTOOPT:       return WSAENOPROTOOPT;
        case EPROTONOSUPPORT:   return WSAEPROTONOSUPPORT;
        case ESOCKTNOSUPPORT:   return WSAESOCKTNOSUPPORT;
        case EOPNOTSUPP:        return WSAEOPNOTSUPP;
        case EPFNOSUPPORT:      return WSAEPFNOSUPPORT;
        case EAFNOSUPPORT:      return WSAEAFNOSUPPORT;
        case EADDRINUSE:        return WSAEADDRINUSE;
        case EADDRNOTAVAIL:     return WSAEADDRNOTAVAIL;
        case ENETDOWN:          return WSAENETDOWN;
        case ENETUNREACH:       return WSAENETUNREACH;
        case ENETRESET:         return WSAENETRESET;
        case ECONNABORTED:      return WSAECONNABORTED;
        case EPIPE:
        case ECONNRESET:        return WSAECONNRESET;
        case ENOBUFS:           return WSAENOBUFS;
        case EISCONN:           return WSAEISCONN;
        case ENOTCONN:          return WSAENOTCONN;
        case ESHUTDOWN:         return WSAESHUTDOWN;
        case ETOOMANYREFS:      return WSAETOOMANYREFS;
        case ETIMEDOUT:         return WSAETIMEDOUT;
        case ECONNREFUSED:      return WSAECONNREFUSED;
        case ELOOP:             return WSAELOOP;
        case ENAMETOOLONG:      return WSAENAMETOOLONG;
        case EHOSTDOWN:         return WSAEHOSTDOWN;
        case EHOSTUNREACH:      return WSAEHOSTUNREACH;
        case ENOTEMPTY:         return WSAENOTEMPTY;
#ifdef EPROCLIM
        case EPROCLIM:          return WSAEPROCLIM;
#endif
#ifdef EUSERS
        case EUSERS:            return WSAEUSERS;
#endif
#ifdef EDQUOT
        case EDQUOT:            return WSAEDQUOT;
#endif
#ifdef ESTALE
        case ESTALE:            return WSAESTALE;
#endif
#ifdef EREMOTE
        case EREMOTE:           return WSAEREMOTE;
#endif
        default:
            FIXME( "unknown error: %s", strerror( err ) );
            return WSAEFAULT;
    }
}

static UINT host_errno_from_unix( int err )
{
    WARN( "%d\n", err );

    switch (err)
    {
        case HOST_NOT_FOUND:    return WSAHOST_NOT_FOUND;
        case TRY_AGAIN:         return WSATRY_AGAIN;
        case NO_RECOVERY:       return WSANO_RECOVERY;
        case NO_DATA:           return WSANO_DATA;
        case ENOBUFS:           return WSAENOBUFS;
        case 0:                 return 0;
        default:
            WARN( "Unknown h_errno %d!\n", err );
            return WSAEOPNOTSUPP;
    }
}

static int addrinfo_err_from_unix( int err )
{
    switch (err)
    {
        case EAI_AGAIN:     return WS_EAI_AGAIN;
        case EAI_BADFLAGS:  return WS_EAI_BADFLAGS;
        case EAI_FAIL:      return WS_EAI_FAIL;
        case EAI_FAMILY:    return WS_EAI_FAMILY;
        case EAI_MEMORY:    return WS_EAI_MEMORY;
        /* EAI_NODATA is deprecated, but still used by Windows and Linux. We map
         * the newer EAI_NONAME to EAI_NODATA for now until Windows changes too. */
#ifdef EAI_NODATA
        case EAI_NODATA:    return WS_EAI_NODATA;
#endif
#ifdef EAI_NONAME
        case EAI_NONAME:    return WS_EAI_NODATA;
#endif
        case EAI_SERVICE:   return WS_EAI_SERVICE;
        case EAI_SOCKTYPE:  return WS_EAI_SOCKTYPE;
        case EAI_SYSTEM:
            /* some broken versions of glibc return EAI_SYSTEM and set errno to
             * 0 instead of returning EAI_NONAME */
            return errno ? errno_from_unix( errno ) : WS_EAI_NONAME;

        default:
            FIXME( "unhandled error %d\n", err );
            return err;
    }
}

union unix_sockaddr
{
    struct sockaddr addr;
    struct sockaddr_in in;
    struct sockaddr_in6 in6;
#ifdef HAS_IPX
    struct sockaddr_ipx ipx;
#endif
#ifdef HAS_IRDA
    struct sockaddr_irda irda;
#endif
};

/* different from the version in ntdll and server; it does not return failure if
 * given a short buffer */
static int sockaddr_from_unix( const union unix_sockaddr *uaddr, struct WS_sockaddr *wsaddr, socklen_t wsaddrlen )
{
    memset( wsaddr, 0, wsaddrlen );

    switch (uaddr->addr.sa_family)
    {
    case AF_INET:
    {
        struct WS_sockaddr_in win = {0};

        if (wsaddrlen >= sizeof(win))
        {
            win.sin_family = WS_AF_INET;
            win.sin_port = uaddr->in.sin_port;
            memcpy( &win.sin_addr, &uaddr->in.sin_addr, sizeof(win.sin_addr) );
            memcpy( wsaddr, &win, sizeof(win) );
        }
        return sizeof(win);
    }

    case AF_INET6:
    {
        struct WS_sockaddr_in6 win = {0};

        if (wsaddrlen >= sizeof(win))
        {
            win.sin6_family = WS_AF_INET6;
            win.sin6_port = uaddr->in6.sin6_port;
            win.sin6_flowinfo = uaddr->in6.sin6_flowinfo;
            memcpy( &win.sin6_addr, &uaddr->in6.sin6_addr, sizeof(win.sin6_addr) );
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID
            win.sin6_scope_id = uaddr->in6.sin6_scope_id;
#endif
            memcpy( wsaddr, &win, sizeof(win) );
        }
        return sizeof(win);
    }

#ifdef HAS_IPX
    case AF_IPX:
    {
        struct WS_sockaddr_ipx win = {0};

        if (wsaddrlen >= sizeof(win))
        {
            win.sa_family = WS_AF_IPX;
            memcpy( win.sa_netnum, &uaddr->ipx.sipx_network, sizeof(win.sa_netnum) );
            memcpy( win.sa_nodenum, &uaddr->ipx.sipx_node, sizeof(win.sa_nodenum) );
            win.sa_socket = uaddr->ipx.sipx_port;
            memcpy( wsaddr, &win, sizeof(win) );
        }
        return sizeof(win);
    }
#endif

#ifdef HAS_IRDA
    case AF_IRDA:
    {
        SOCKADDR_IRDA win;

        if (wsaddrlen >= sizeof(win))
        {
            win.irdaAddressFamily = WS_AF_IRDA;
            memcpy( win.irdaDeviceID, &uaddr->irda.sir_addr, sizeof(win.irdaDeviceID) );
            if (uaddr->irda.sir_lsap_sel != LSAP_ANY)
                snprintf( win.irdaServiceName, sizeof(win.irdaServiceName), "LSAP-SEL%u", uaddr->irda.sir_lsap_sel );
            else
                memcpy( win.irdaServiceName, uaddr->irda.sir_name, sizeof(win.irdaServiceName) );
            memcpy( wsaddr, &win, sizeof(win) );
        }
        return sizeof(win);
    }
#endif

    case AF_UNSPEC:
        return 0;

    default:
        FIXME( "unknown address family %d\n", uaddr->addr.sa_family );
        return 0;
    }
}

static BOOL addrinfo_in_list( const struct WS_addrinfo *list, const struct WS_addrinfo *ai )
{
    const struct WS_addrinfo *cursor = list;
    while (cursor)
    {
        if (ai->ai_flags == cursor->ai_flags &&
            ai->ai_family == cursor->ai_family &&
            ai->ai_socktype == cursor->ai_socktype &&
            ai->ai_protocol == cursor->ai_protocol &&
            ai->ai_addrlen == cursor->ai_addrlen &&
            !memcmp( ai->ai_addr, cursor->ai_addr, ai->ai_addrlen ) &&
            ((ai->ai_canonname && cursor->ai_canonname && !strcmp( ai->ai_canonname, cursor->ai_canonname ))
            || (!ai->ai_canonname && !cursor->ai_canonname)))
        {
            return TRUE;
        }
        cursor = cursor->ai_next;
    }
    return FALSE;
}

static int CDECL unix_getaddrinfo( const char *node, const char *service, const struct WS_addrinfo *hints,
                                   struct WS_addrinfo *info, unsigned int *size )
{
#ifdef HAVE_GETADDRINFO
    struct addrinfo unix_hints = {0};
    struct addrinfo *unix_info, *src;
    struct WS_addrinfo *dst, *prev = NULL;
    unsigned int needed_size = 0;
    int ret;

    /* servname tweak required by OSX and BSD kernels */
    if (service && !service[0]) service = "0";

    if (hints)
    {
        unix_hints.ai_flags = addrinfo_flags_to_unix( hints->ai_flags );

        if (hints->ai_family)
            unix_hints.ai_family = family_to_unix( hints->ai_family );

        if (hints->ai_socktype)
        {
            if ((unix_hints.ai_socktype = socktype_to_unix( hints->ai_socktype )) < 0)
                return WSAESOCKTNOSUPPORT;
        }

        if (hints->ai_protocol)
            unix_hints.ai_protocol = max( protocol_to_unix( hints->ai_protocol ), 0 );

        /* Windows allows some invalid combinations */
        if (unix_hints.ai_protocol == IPPROTO_TCP
                && unix_hints.ai_socktype != SOCK_STREAM
                && unix_hints.ai_socktype != SOCK_SEQPACKET)
        {
            WARN( "ignoring invalid type %u for TCP\n", unix_hints.ai_socktype );
            unix_hints.ai_socktype = 0;
        }
        else if (unix_hints.ai_protocol == IPPROTO_UDP && unix_hints.ai_socktype != SOCK_DGRAM)
        {
            WARN( "ignoring invalid type %u for UDP\n", unix_hints.ai_socktype );
            unix_hints.ai_socktype = 0;
        }
        else if (unix_hints.ai_protocol >= WS_NSPROTO_IPX && unix_hints.ai_protocol <= WS_NSPROTO_IPX + 255
                && unix_hints.ai_socktype != SOCK_DGRAM)
        {
            WARN( "ignoring invalid type %u for IPX\n", unix_hints.ai_socktype );
            unix_hints.ai_socktype = 0;
        }
        else if (unix_hints.ai_protocol == IPPROTO_IPV6)
        {
            WARN( "ignoring protocol IPv6\n" );
            unix_hints.ai_protocol = 0;
        }
    }

    ret = getaddrinfo( node, service, hints ? &unix_hints : NULL, &unix_info );
    if (ret)
        return addrinfo_err_from_unix( ret );

    for (src = unix_info; src != NULL; src = src->ai_next)
    {
        needed_size += sizeof(struct WS_addrinfo);
        if (src->ai_canonname)
            needed_size += strlen( src->ai_canonname ) + 1;
        needed_size += sockaddr_from_unix( (const union unix_sockaddr *)src->ai_addr, NULL, 0 );
    }

    if (*size < needed_size)
    {
        *size = needed_size;
        freeaddrinfo( unix_info );
        return ERROR_INSUFFICIENT_BUFFER;
    }

    dst = info;

    memset( info, 0, needed_size );

    for (src = unix_info; src != NULL; src = src->ai_next)
    {
        void *next = dst + 1;

        dst->ai_flags = addrinfo_flags_from_unix( src->ai_flags );
        dst->ai_family = family_from_unix( src->ai_family );
        if (hints)
        {
            dst->ai_socktype = hints->ai_socktype;
            dst->ai_protocol = hints->ai_protocol;
        }
        else
        {
            dst->ai_socktype = socktype_from_unix( src->ai_socktype );
            dst->ai_protocol = protocol_from_unix( src->ai_protocol );
        }
        if (src->ai_canonname)
        {
            size_t len = strlen( src->ai_canonname ) + 1;

            dst->ai_canonname = next;
            memcpy( dst->ai_canonname, src->ai_canonname, len );
            next = dst->ai_canonname + len;
        }

        dst->ai_addrlen = sockaddr_from_unix( (const union unix_sockaddr *)src->ai_addr, NULL, 0 );
        dst->ai_addr = next;
        sockaddr_from_unix( (const union unix_sockaddr *)src->ai_addr, dst->ai_addr, dst->ai_addrlen );
        next = (char *)dst->ai_addr + dst->ai_addrlen;

        if (dst == info || !addrinfo_in_list( info, dst ))
        {
            if (prev)
                prev->ai_next = dst;
            prev = dst;
            dst = next;
        }
    }

    dst->ai_next = NULL;

    freeaddrinfo( unix_info );
    return 0;
#else
    FIXME( "getaddrinfo() not found during build time\n" );
    return WS_EAI_FAIL;
#endif
}


static int hostent_from_unix( const struct hostent *unix_host, struct WS_hostent *host, unsigned int *const size )
{
    unsigned int needed_size = sizeof( struct WS_hostent ), alias_count = 0, addr_count = 0, i;
    char *p;

    needed_size += strlen( unix_host->h_name );

    for (alias_count = 0; unix_host->h_aliases[alias_count] != NULL; ++alias_count)
        needed_size += sizeof(char *) + strlen( unix_host->h_aliases[alias_count] ) + 1;
    needed_size += sizeof(char *); /* null terminator */

    for (addr_count = 0; unix_host->h_addr_list[addr_count] != NULL; ++addr_count)
        needed_size += sizeof(char *) + unix_host->h_length;
    needed_size += sizeof(char *); /* null terminator */

    if (*size < needed_size)
    {
        *size = needed_size;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    memset( host, 0, needed_size );

    /* arrange the memory in the same order as windows >= XP */

    host->h_addrtype = family_from_unix( unix_host->h_addrtype );
    host->h_length = unix_host->h_length;

    p = (char *)(host + 1);
    host->h_aliases = (char **)p;
    p += (alias_count + 1) * sizeof(char *);
    host->h_addr_list = (char **)p;
    p += (addr_count + 1) * sizeof(char *);

    for (i = 0; i < addr_count; ++i)
    {
        host->h_addr_list[i] = p;
        memcpy( host->h_addr_list[i], unix_host->h_addr_list[i], unix_host->h_length );
        p += unix_host->h_length;
    }

    for (i = 0; i < alias_count; ++i)
    {
        size_t len = strlen( unix_host->h_aliases[i] ) + 1;

        host->h_aliases[i] = p;
        memcpy( host->h_aliases[i], unix_host->h_aliases[i], len );
        p += len;
    }

    host->h_name = p;
    strcpy( host->h_name, unix_host->h_name );

    return 0;
}


static int CDECL unix_gethostbyaddr( const void *addr, int len, int family,
                                     struct WS_hostent *const host, unsigned int *size )
{
    const struct in_addr loopback = { htonl( INADDR_LOOPBACK ) };
    int unix_family = family_to_unix( family );
    struct hostent *unix_host;
    int ret;

    if (family == WS_AF_INET && len == 4 && !memcmp( addr, magic_loopback_addr, 4 ))
        addr = &loopback;

#ifdef HAVE_LINUX_GETHOSTBYNAME_R_6
    {
        char *unix_buffer, *new_buffer;
        struct hostent stack_host;
        int unix_size = 1024;
        int locerr;

        if (!(unix_buffer = malloc( unix_size )))
            return WSAENOBUFS;

        while (gethostbyaddr_r( addr, len, unix_family, &stack_host, unix_buffer,
                                unix_size, &unix_host, &locerr ) == ERANGE)
        {
            unix_size *= 2;
            if (!(new_buffer = realloc( unix_buffer, unix_size )))
            {
                free( unix_buffer );
                return WSAENOBUFS;
            }
            unix_buffer = new_buffer;
        }

        if (!unix_host)
            return (locerr < 0 ? errno_from_unix( errno ) : host_errno_from_unix( locerr ));

        ret = hostent_from_unix( unix_host, host, size );

        free( unix_buffer );
        return ret;
    }
#else
    pthread_mutex_lock( &host_mutex );

    if (!(unix_host = gethostbyaddr( addr, len, unix_family )))
    {
        ret = (h_errno < 0 ? errno_from_unix( errno ) : host_errno_from_unix( h_errno ));
        pthread_mutex_unlock( &host_mutex );
        return ret;
    }

    ret = hostent_from_unix( unix_host, host, size );

    pthread_mutex_unlock( &host_mutex );
    return ret;
#endif
}


#ifdef HAVE_LINUX_GETHOSTBYNAME_R_6
static int CDECL unix_gethostbyname( const char *name, struct WS_hostent *const host, unsigned int *size )
{
    struct hostent stack_host, *unix_host;
    char *unix_buffer, *new_buffer;
    int unix_size = 1024;
    int locerr;
    int ret;

    if (!(unix_buffer = malloc( unix_size )))
        return WSAENOBUFS;

    while (gethostbyname_r( name, &stack_host, unix_buffer, unix_size, &unix_host, &locerr ) == ERANGE)
    {
        unix_size *= 2;
        if (!(new_buffer = realloc( unix_buffer, unix_size )))
        {
            free( unix_buffer );
            return WSAENOBUFS;
        }
        unix_buffer = new_buffer;
    }

    if (!unix_host)
        return (locerr < 0 ? errno_from_unix( errno ) : host_errno_from_unix( locerr ));

    ret = hostent_from_unix( unix_host, host, size );

    free( unix_buffer );
    return ret;
}
#else
static int CDECL unix_gethostbyname( const char *name, struct WS_hostent *const host, unsigned int *size )
{
    struct hostent *unix_host;
    int ret;

    pthread_mutex_lock( &host_mutex );

    if (!(unix_host = gethostbyname( name )))
    {
        ret = (h_errno < 0 ? errno_from_unix( errno ) : host_errno_from_unix( h_errno ));
        pthread_mutex_unlock( &host_mutex );
        return ret;
    }

    ret = hostent_from_unix( unix_host, host, size );

    pthread_mutex_unlock( &host_mutex );
    return ret;
}
#endif


static int CDECL unix_gethostname( char *name, int len )
{
    if (!gethostname( name, len ))
        return 0;
    return errno_from_unix( errno );
}


static const struct unix_funcs funcs =
{
    unix_getaddrinfo,
    unix_gethostbyaddr,
    unix_gethostbyname,
    unix_gethostname,
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;

    *(const struct unix_funcs **)ptr_out = &funcs;
    return STATUS_SUCCESS;
}
