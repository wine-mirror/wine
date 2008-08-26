/*
 * Copyright 2008 Hans Leidekker for CodeWeavers
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

#include "config.h"
#include "wine/port.h"
#include "wine/debug.h"

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#ifdef HAVE_POLL_H
# include <poll.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winhttp.h"

/* to avoid conflicts with the Unix socket headers */
#define USE_WS_PREFIX
#include "winsock2.h"

#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

#define DEFAULT_SEND_TIMEOUT        30
#define DEFAULT_RECEIVE_TIMEOUT     30

#ifndef HAVE_GETADDRINFO

/* critical section to protect non-reentrant gethostbyname() */
static CRITICAL_SECTION cs_gethostbyname;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &cs_gethostbyname,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": cs_gethostbyname") }
};
static CRITICAL_SECTION cs_gethostbyname = { &critsect_debug, -1, 0, 0, 0, 0 };

#endif

/* translate a unix error code into a winsock error code */
static int sock_get_error( int err )
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
        case EWOULDBLOCK:       return WSAEWOULDBLOCK;
        case EINPROGRESS:       return WSAEINPROGRESS;
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
    default: errno = err; perror( "sock_set_error" ); return WSAEFAULT;
    }
    return err;
}

BOOL netconn_init( netconn_t *conn )
{
    conn->socket = -1;
    return TRUE;
}

BOOL netconn_connected( netconn_t *conn )
{
    return (conn->socket != -1);
}

BOOL netconn_create( netconn_t *conn, int domain, int type, int protocol )
{
    if ((conn->socket = socket( domain, type, protocol )) == -1)
    {
        WARN("unable to create socket (%s)\n", strerror(errno));
        set_last_error( sock_get_error( errno ) );
        return FALSE;
    }
    return TRUE;
}

BOOL netconn_close( netconn_t *conn )
{
    int res;

    res = close( conn->socket );
    conn->socket = -1;
    if (res == -1)
    {
        set_last_error( sock_get_error( errno ) );
        return FALSE;
    }
    return TRUE;
}

BOOL netconn_connect( netconn_t *conn, const struct sockaddr *sockaddr, unsigned int addr_len )
{
    if (connect( conn->socket, sockaddr, addr_len ) == -1)
    {
        WARN("unable to connect to host (%s)\n", strerror(errno));
        set_last_error( sock_get_error( errno ) );
        return FALSE;
    }
    return TRUE;
}

BOOL netconn_send( netconn_t *conn, const void *msg, size_t len, int flags, int *sent )
{
    if (!netconn_connected( conn )) return FALSE;
    if ((*sent = send( conn->socket, msg, len, flags )) == -1)
    {
        set_last_error( sock_get_error( errno ) );
        return FALSE;
    }
    return TRUE;
}

BOOL netconn_recv( netconn_t *conn, void *buf, size_t len, int flags, int *recvd )
{
    *recvd = 0;
    if (!netconn_connected( conn )) return FALSE;
    if (!len) return TRUE;
    if ((*recvd = recv( conn->socket, buf, len, flags )) == -1)
    {
        set_last_error( sock_get_error( errno ) );
        return FALSE;
    }
    return TRUE;
}

BOOL netconn_query_data_available( netconn_t *conn, DWORD *available )
{
#ifdef FIONREAD
    int ret, unread;
#endif
    *available = 0;
    if (!netconn_connected( conn )) return FALSE;
#ifdef FIONREAD
    if (!(ret = ioctl( conn->socket, FIONREAD, &unread )))
    {
        TRACE("%d bytes of queued, but unread data\n", unread);
        *available += unread;
    }
#endif
    return TRUE;
}

BOOL netconn_get_next_line( netconn_t *conn, char *buffer, DWORD *buflen )
{
    struct pollfd pfd;
    BOOL ret = FALSE;
    DWORD recvd = 0;

    if (!netconn_connected( conn )) return FALSE;

    pfd.fd = conn->socket;
    pfd.events = POLLIN;
    while (recvd < *buflen)
    {
        if (poll( &pfd, 1, DEFAULT_RECEIVE_TIMEOUT * 1000 ) > 0)
        {
            if (recv( conn->socket, &buffer[recvd], 1, 0 ) <= 0)
            {
                set_last_error( sock_get_error( errno ) );
                break;
            }
            if (buffer[recvd] == '\n')
            {
                ret = TRUE;
                break;
            }
            if (buffer[recvd] != '\r') recvd++;
        }
        else
        {
            set_last_error( ERROR_WINHTTP_TIMEOUT );
            break;
        }
    }
    if (ret)
    {
        buffer[recvd++] = 0;
        *buflen = recvd;
        TRACE("received line %s\n", debugstr_a(buffer));
        return TRUE;
    }
    return FALSE;
}

DWORD netconn_set_timeout( netconn_t *netconn, BOOL send, int value )
{
    int res;
    struct timeval tv;

    /* value is in milliseconds, convert to struct timeval */
    tv.tv_sec = value / 1000;
    tv.tv_usec = (value % 1000) * 1000;

    if ((res = setsockopt( netconn->socket, SOL_SOCKET, send ? SO_SNDTIMEO : SO_RCVTIMEO, &tv, sizeof(tv) ) == -1))
    {
        WARN("setsockopt failed (%s)\n", strerror( errno ));
        return sock_get_error( errno );
    }
    return ERROR_SUCCESS;
}

BOOL netconn_resolve( WCHAR *hostnameW, INTERNET_PORT port, struct sockaddr_in *sa )
{
    char *hostname;
#ifdef HAVE_GETADDRINFO
    struct addrinfo *res, hints;
    int ret;
#else
    struct hostent *he;
#endif

    if (!(hostname = strdupWA( hostnameW ))) return FALSE;

#ifdef HAVE_GETADDRINFO
    memset( &hints, 0, sizeof(struct addrinfo) );
    hints.ai_family = AF_INET;

    ret = getaddrinfo( hostname, NULL, &hints, &res );
    heap_free( hostname );
    if (ret != 0)
    {
        TRACE("failed to get address of %s (%s)\n", debugstr_a(hostname), gai_strerror(ret));
        return FALSE;
    }
    memset( sa, 0, sizeof(struct sockaddr_in) );
    memcpy( &sa->sin_addr, &((struct sockaddr_in *)res->ai_addr)->sin_addr, sizeof(struct in_addr) );
    sa->sin_family = res->ai_family;
    sa->sin_port = htons( port );

    freeaddrinfo( res );
#else
    EnterCriticalSection( &cs_gethostbyname );

    he = gethostbyname( hostname );
    heap_free( hostname );
    if (!he)
    {
        TRACE("failed to get address of %s (%d)\n", debugstr_a(hostname), h_errno);
        LeaveCriticalSection( &cs_gethostbyname );
        return FALSE;
    }
    memset( sa, 0, sizeof(struct sockaddr_in) );
    memcpy( (char *)&sa->sin_addr, he->h_addr, he->h_length );
    sa->sin_family = he->h_addrtype;
    sa->sin_port = htons( port );

    LeaveCriticalSection( &cs_gethostbyname );
#endif
    return TRUE;
}
