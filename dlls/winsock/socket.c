/*
 * based on Windows Sockets 1.1 specs
 * (ftp.microsoft.com:/Advsys/winsock/spec11/WINSOCK.TXT)
 *
 * Copyright (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTE: If you make any changes to fix a particular app, make sure
 * they don't break something else like Netscape or telnet and ftp
 * clients and servers (www.winsite.com got a lot of those).
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_IPC_H
# include <sys/ipc.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif

#if defined(__EMX__)
# include <sys/so_ioctl.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifdef HAVE_SYS_MSG_H
# include <sys/msg.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_SYS_UIO_H
# include <sys/uio.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#ifdef HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif
#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif
#ifdef HAVE_IPX_GNU
# include <netipx/ipx.h>
# define HAVE_IPX
#endif
#ifdef HAVE_IPX_LINUX
# include <asm/types.h>
# include <linux/ipx.h>
# define HAVE_IPX
#endif

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#include "wine/winbase16.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#include "wsipx.h"
#include "wine/winsock16.h"
#include "winnt.h"
#include "wownt32.h"
#include "wine/server.h"
#include "wine/debug.h"

#ifdef __FreeBSD__
# define sipx_network    sipx_addr.x_net
# define sipx_node       sipx_addr.x_host.c_host
#endif  /* __FreeBSD__ */

WINE_DEFAULT_DEBUG_CHANNEL(winsock);

/* critical section to protect some non-rentrant net function */
extern CRITICAL_SECTION csWSgetXXXbyYYY;

#define DEBUG_SOCKADDR 0
#define dump_sockaddr(a) \
        DPRINTF("sockaddr_in: family %d, address %s, port %d\n", \
                        ((struct sockaddr_in *)a)->sin_family, \
                        inet_ntoa(((struct sockaddr_in *)a)->sin_addr), \
                        ntohs(((struct sockaddr_in *)a)->sin_port))

/* HANDLE<->SOCKET conversion (SOCKET is UINT_PTR). */
#define SOCKET2HANDLE(s) ((HANDLE)(s))
#define HANDLE2SOCKET(h) ((SOCKET)(h))

/****************************************************************
 * Async IO declarations
 ****************************************************************/
#include "async.h"

static DWORD ws2_async_get_status (const struct async_private *ovp);
static DWORD ws2_async_get_count  (const struct async_private *ovp);
static void  ws2_async_set_status (struct async_private *ovp, const DWORD status);
static void CALLBACK ws2_async_call_completion (ULONG_PTR data);
static void ws2_async_cleanup ( struct async_private *ovp );

static struct async_ops ws2_async_ops =
{
    ws2_async_get_status,
    ws2_async_set_status,
    ws2_async_get_count,
    ws2_async_call_completion,
    ws2_async_cleanup
};

static struct async_ops ws2_nocomp_async_ops =
{
    ws2_async_get_status,
    ws2_async_set_status,
    ws2_async_get_count,
    NULL,                     /* call_completion */
    ws2_async_cleanup
};

typedef struct ws2_async
{
    async_private                       async;
    LPWSAOVERLAPPED                     overlapped;
    LPWSAOVERLAPPED                     user_overlapped;
    LPWSAOVERLAPPED_COMPLETION_ROUTINE  completion_func;
    struct iovec                        *iovec;
    int                                 n_iovecs;
    struct WS_sockaddr            *addr;
    union {
        int val;     /* for send operations */
        int *ptr;    /* for recv operations */
    }                                   addrlen;
    DWORD                               flags;
} ws2_async;

/****************************************************************/

/* ----------------------------------- internal data */

/* ws_... struct conversion flags */

#define WS_DUP_LINEAR           0x0001
#define WS_DUP_SEGPTR           0x0002          /* internal pointers are SEGPTRs */
                                                /* by default, internal pointers are linear */
typedef struct          /* WSAAsyncSelect() control struct */
{
  HANDLE      service, event, sock;
  HWND        hWnd;
  UINT        uMsg;
  LONG        lEvent;
} ws_select_info;

#define WS_MAX_SOCKETS_PER_PROCESS      128     /* reasonable guess */
#define WS_MAX_UDP_DATAGRAM             1024

#define PROCFS_NETDEV_FILE   "/proc/net/dev" /* Points to the file in the /proc fs
                                                that lists the network devices.
                                                Do we need an #ifdef LINUX for this? */

static void *he_buffer;          /* typecast for Win16/32 ws_hostent */
static SEGPTR he_buffer_seg;
static void *se_buffer;          /* typecast for Win16/32 ws_servent */
static SEGPTR se_buffer_seg;
static void *pe_buffer;          /* typecast for Win16/32 ws_protoent */
static SEGPTR pe_buffer_seg;
static char* local_buffer;
static SEGPTR dbuffer_seg;
static INT num_startup;          /* reference counter */
static FARPROC blocking_hook;

/* function prototypes */
static int WS_dup_he(struct hostent* p_he, int flag);
static int WS_dup_pe(struct protoent* p_pe, int flag);
static int WS_dup_se(struct servent* p_se, int flag);

typedef void	WIN_hostent;
typedef void	WIN_protoent;
typedef void	WIN_servent;

int WSAIOCTL_GetInterfaceCount(void);
int WSAIOCTL_GetInterfaceName(int intNumber, char *intName);

UINT16 wsaErrno(void);
UINT16 wsaHerrno(int errnr);

static HANDLE 	_WSHeap = 0;

#define WS_ALLOC(size) \
	HeapAlloc(_WSHeap, HEAP_ZERO_MEMORY, (size) )
#define WS_FREE(ptr) \
	HeapFree(_WSHeap, 0, (ptr) )

static INT         _ws_sock_ops[] =
       { WS_SO_DEBUG, WS_SO_REUSEADDR, WS_SO_KEEPALIVE, WS_SO_DONTROUTE,
         WS_SO_BROADCAST, WS_SO_LINGER, WS_SO_OOBINLINE, WS_SO_SNDBUF,
         WS_SO_RCVBUF, WS_SO_ERROR, WS_SO_TYPE,
#ifdef SO_RCVTIMEO
	 WS_SO_RCVTIMEO,
#endif
#ifdef SO_SNDTIMEO
	 WS_SO_SNDTIMEO,
#endif
	 0 };
static int           _px_sock_ops[] =
       { SO_DEBUG, SO_REUSEADDR, SO_KEEPALIVE, SO_DONTROUTE, SO_BROADCAST,
         SO_LINGER, SO_OOBINLINE, SO_SNDBUF, SO_RCVBUF, SO_ERROR, SO_TYPE,
#ifdef SO_RCVTIMEO
	 SO_RCVTIMEO,
#endif
#ifdef SO_SNDTIMEO
	 SO_SNDTIMEO,
#endif
	};

static INT _ws_tcp_ops[] = {
#ifdef TCP_NODELAY
	WS_TCP_NODELAY,
#endif
	0
};
static int _px_tcp_ops[] = {
#ifdef TCP_NODELAY
	TCP_NODELAY,
#endif
	0
};

static const int _ws_ip_ops[] =
{
    WS_IP_MULTICAST_IF,
    WS_IP_MULTICAST_TTL,
    WS_IP_MULTICAST_LOOP,
    WS_IP_ADD_MEMBERSHIP,
    WS_IP_DROP_MEMBERSHIP,
    WS_IP_OPTIONS,
    WS_IP_HDRINCL,
    WS_IP_TOS,
    WS_IP_TTL,
    0
};

static const int _px_ip_ops[] =
{
    IP_MULTICAST_IF,
    IP_MULTICAST_TTL,
    IP_MULTICAST_LOOP,
    IP_ADD_MEMBERSHIP,
    IP_DROP_MEMBERSHIP,
    IP_OPTIONS,
    IP_HDRINCL,
    IP_TOS,
    IP_TTL,
    0
};

static DWORD opentype_tls_index = -1;  /* TLS index for SO_OPENTYPE flag */

inline static DWORD NtStatusToWSAError ( const DWORD status )
{
    /* We only need to cover the status codes set by server async request handling */
    DWORD wserr;
    switch ( status )
    {
    case STATUS_SUCCESS:              wserr = 0;                     break;
    case STATUS_PENDING:              wserr = WSA_IO_PENDING;        break;
    case STATUS_INVALID_HANDLE:       wserr = WSAENOTSOCK;           break;  /* WSAEBADF ? */
    case STATUS_INVALID_PARAMETER:    wserr = WSAEINVAL;             break;
    case STATUS_PIPE_DISCONNECTED:    wserr = WSAESHUTDOWN;          break;
    case STATUS_CANCELLED:            wserr = WSA_OPERATION_ABORTED; break;
    case STATUS_TIMEOUT:              wserr = WSAETIMEDOUT;          break;
    case STATUS_NO_MEMORY:            wserr = WSAEFAULT;             break;
    default:
        if ( status >= WSABASEERR && status <= WSABASEERR+1004 )
            /* It is not a NT status code but a winsock error */
            wserr = status;
        else
        {
            wserr = RtlNtStatusToDosError( status );
            FIXME ( "Status code %08lx converted to DOS error code %lx\n", status, wserr );
        }
    }
    return wserr;
}

/* set last error code from NT status without mapping WSA errors */
inline static unsigned int set_error( unsigned int err )
{
    if (err)
    {
        err = NtStatusToWSAError ( err );
        SetLastError( err );
    }
    return err;
}

static char* check_buffer(int size);

inline static int _get_sock_fd(SOCKET s)
{
    int fd;

    if (set_error( wine_server_handle_to_fd( SOCKET2HANDLE(s), GENERIC_READ, &fd, NULL, NULL ) ))
        return -1;
    return fd;
}

inline static int _get_sock_fd_type( SOCKET s, DWORD access, enum fd_type *type, int *flags )
{
    int fd;
    if (set_error( wine_server_handle_to_fd( SOCKET2HANDLE(s), access, &fd, type, flags ) )) return -1;
    if ( ( (access & GENERIC_READ)  && (*flags & FD_FLAG_RECV_SHUTDOWN ) ) ||
         ( (access & GENERIC_WRITE) && (*flags & FD_FLAG_SEND_SHUTDOWN ) ) )
    {
        close (fd);
        WSASetLastError ( WSAESHUTDOWN );
        return -1;
    }
    return fd;
}

static void _enable_event( HANDLE s, unsigned int event,
                           unsigned int sstate, unsigned int cstate )
{
    SERVER_START_REQ( enable_socket_event )
    {
        req->handle = s;
        req->mask   = event;
        req->sstate = sstate;
        req->cstate = cstate;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}

static int _is_blocking(SOCKET s)
{
    int ret;
    SERVER_START_REQ( get_socket_event )
    {
        req->handle  = SOCKET2HANDLE(s);
        req->service = FALSE;
        req->c_event = 0;
        wine_server_call( req );
        ret = (reply->state & FD_WINE_NONBLOCKING) == 0;
    }
    SERVER_END_REQ;
    return ret;
}

static unsigned int _get_sock_mask(SOCKET s)
{
    unsigned int ret;
    SERVER_START_REQ( get_socket_event )
    {
        req->handle  = SOCKET2HANDLE(s);
        req->service = FALSE;
        req->c_event = 0;
        wine_server_call( req );
        ret = reply->mask;
    }
    SERVER_END_REQ;
    return ret;
}

static void _sync_sock_state(SOCKET s)
{
    /* do a dummy wineserver request in order to let
       the wineserver run through its select loop once */
    (void)_is_blocking(s);
}

static int _get_sock_error(SOCKET s, unsigned int bit)
{
    int events[FD_MAX_EVENTS];

    SERVER_START_REQ( get_socket_event )
    {
        req->handle  = SOCKET2HANDLE(s);
        req->service = FALSE;
        req->c_event = 0;
        wine_server_set_reply( req, events, sizeof(events) );
        wine_server_call( req );
    }
    SERVER_END_REQ;
    return events[bit];
}

static void WINSOCK_DeleteIData(void)
{
    /* delete scratch buffers */

    UnMapLS( he_buffer_seg );
    UnMapLS( se_buffer_seg );
    UnMapLS( pe_buffer_seg );
    UnMapLS( dbuffer_seg );
    if (he_buffer) HeapFree( GetProcessHeap(), 0, he_buffer );
    if (se_buffer) HeapFree( GetProcessHeap(), 0, se_buffer );
    if (pe_buffer) HeapFree( GetProcessHeap(), 0, pe_buffer );
    if (local_buffer) HeapFree( GetProcessHeap(), 0, local_buffer );
    he_buffer = NULL;
    se_buffer = NULL;
    pe_buffer = NULL;
    local_buffer = NULL;
    he_buffer_seg = 0;
    se_buffer_seg = 0;
    pe_buffer_seg = 0;
    dbuffer_seg = 0;
    num_startup = 0;
}

/***********************************************************************
 *		DllMain (WS2_32.init)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%lx %p\n", hInstDLL, fdwReason, fImpLoad);
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        opentype_tls_index = TlsAlloc();
        break;
    case DLL_PROCESS_DETACH:
        TlsFree( opentype_tls_index );
	WINSOCK_DeleteIData();
	break;
    }
    return TRUE;
}

/***********************************************************************
 *          convert_sockopt()
 *
 * Converts socket flags from Windows format.
 * Return 1 if converted, 0 if not (error).
 */
static int convert_sockopt(INT *level, INT *optname)
{
  int i;
  switch (*level)
  {
     case WS_SOL_SOCKET:
        *level = SOL_SOCKET;
        for(i=0; _ws_sock_ops[i]; i++)
            if( _ws_sock_ops[i] == *optname ) break;
        if( _ws_sock_ops[i] ) {
	    *optname = _px_sock_ops[i];
	    return 1;
	}
        FIXME("Unknown SOL_SOCKET optname 0x%x\n", *optname);
        break;
     case WS_IPPROTO_TCP:
        *level = IPPROTO_TCP;
        for(i=0; _ws_tcp_ops[i]; i++)
		if ( _ws_tcp_ops[i] == *optname ) break;
        if( _ws_tcp_ops[i] ) {
	    *optname = _px_tcp_ops[i];
	    return 1;
	}
        FIXME("Unknown IPPROTO_TCP optname 0x%x\n", *optname);
	break;
     case WS_IPPROTO_IP:
        *level = IPPROTO_IP;
	for(i=0; _ws_ip_ops[i]; i++) {
	    if (_ws_ip_ops[i] == *optname ) break;
	}
	if( _ws_ip_ops[i] ) {
	    *optname = _px_ip_ops[i];
	    return 1;
	}
	FIXME("Unknown IPPROTO_IP optname 0x%x\n", *optname);
	break;
     default: FIXME("Unimplemented or unknown socket level\n");
  }
  return 0;
}

/* ----------------------------------- Per-thread info (or per-process?) */

static int wsi_strtolo(const char* name, const char* opt)
{
    /* Stuff a lowercase copy of the string into the local buffer */

    int i = strlen(name) + 2;
    char* p = check_buffer(i + ((opt)?strlen(opt):0));

    if( p )
    {
	do *p++ = tolower(*name); while(*name++);
	i = (p - local_buffer);
	if( opt ) do *p++ = tolower(*opt); while(*opt++);
	return i;
    }
    return 0;
}

static fd_set* fd_set_import( fd_set* fds, void* wsfds, int* highfd, int lfd[], BOOL b32 )
{
    /* translate Winsock fd set into local fd set */

    if( wsfds )
    {
#define wsfds16	((ws_fd_set16*)wsfds)
#define wsfds32 ((WS_fd_set*)wsfds)
	int i, count;

	FD_ZERO(fds);
	count = b32 ? wsfds32->fd_count : wsfds16->fd_count;

	for( i = 0; i < count; i++ )
	{
	     int s = (b32) ? wsfds32->fd_array[i]
			   : wsfds16->fd_array[i];
             int fd = _get_sock_fd(s);
	     if (fd != -1)
             {
                    lfd[ i ] = fd;
		    if( fd > *highfd ) *highfd = fd;
		    FD_SET(fd, fds);
	     }
	     else lfd[ i ] = -1;
	}
#undef wsfds32
#undef wsfds16
	return fds;
    }
    return NULL;
}

inline static int sock_error_p(int s)
{
    unsigned int optval, optlen;

    optlen = sizeof(optval);
    getsockopt(s, SOL_SOCKET, SO_ERROR, (void *) &optval, &optlen);
    if (optval) WARN("\t[%i] error: %d\n", s, optval);
    return optval != 0;
}

static int fd_set_export( fd_set* fds, fd_set* exceptfds, void* wsfds, int lfd[], BOOL b32 )
{
    int num_err = 0;

    /* translate local fd set into Winsock fd set, adding
     * errors to exceptfds (only if app requested it) */

    if( wsfds )
    {
#define wsfds16 ((ws_fd_set16*)wsfds)
#define wsfds32 ((WS_fd_set*)wsfds)
	int i, j, count = (b32) ? wsfds32->fd_count : wsfds16->fd_count;

	for( i = 0, j = 0; i < count; i++ )
	{
	    if( lfd[i] >= 0 )
	    {
		int fd = lfd[i];
		if( FD_ISSET(fd, fds) )
		{
		    if ( exceptfds && sock_error_p(fd) )
		    {
			FD_SET(fd, exceptfds);
			num_err++;
		    }
		    else if( b32 )
			     wsfds32->fd_array[j++] = wsfds32->fd_array[i];
			 else
			     wsfds16->fd_array[j++] = wsfds16->fd_array[i];
		}
		close(fd);
		lfd[i] = -1;
	    }
	}

	if( b32 ) wsfds32->fd_count = j;
	else wsfds16->fd_count = j;

	TRACE("\n");
#undef wsfds32
#undef wsfds16
    }
    return num_err;
}

static void fd_set_unimport( void* wsfds, int lfd[], BOOL b32 )
{
    if ( wsfds )
    {
#define wsfds16 ((ws_fd_set16*)wsfds)
#define wsfds32 ((WS_fd_set*)wsfds)
	int i, count = (b32) ? wsfds32->fd_count : wsfds16->fd_count;

	for( i = 0; i < count; i++ )
	    if ( lfd[i] >= 0 )
		close(lfd[i]);

	TRACE("\n");
#undef wsfds32
#undef wsfds16
    }
}

static int do_block( int fd, int mask )
{
    fd_set fds[3];
    int i, r;

    FD_ZERO(&fds[0]);
    FD_ZERO(&fds[1]);
    FD_ZERO(&fds[2]);
    for (i=0; i<3; i++)
	if (mask & (1<<i))
	    FD_SET(fd, &fds[i]);
    i = select( fd+1, &fds[0], &fds[1], &fds[2], NULL );
    if (i <= 0) return -1;
    r = 0;
    for (i=0; i<3; i++)
	if (FD_ISSET(fd, &fds[i]))
	    r |= 1<<i;
    return r;
}

void* __ws_memalloc( int size )
{
    return WS_ALLOC(size);
}

void __ws_memfree(void* ptr)
{
    WS_FREE(ptr);
}


/* ----------------------------------- API -----
 *
 * Init / cleanup / error checking.
 */

/***********************************************************************
 *      WSAStartup			(WINSOCK.115)
 *
 * Create socket control struct, attach it to the global list and
 * update a pointer in the task struct.
 */
INT16 WINAPI WSAStartup16(UINT16 wVersionRequested, LPWSADATA16 lpWSAData)
{
    static const WSADATA16 data =
    {
        0x0101, 0x0101,
        "WINE Sockets 1.1",
#ifdef linux
        "Linux/i386",
#elif defined(__NetBSD__)
        "NetBSD/i386",
#elif defined(sunos)
        "SunOS",
#elif defined(__FreeBSD__)
        "FreeBSD",
#elif defined(__OpenBSD__)
        "OpenBSD/i386",
#else
        "Unknown",
#endif
        WS_MAX_SOCKETS_PER_PROCESS,
        WS_MAX_UDP_DATAGRAM,
        0
    };

    TRACE("verReq=%x\n", wVersionRequested);

    if (LOBYTE(wVersionRequested) < 1 || (LOBYTE(wVersionRequested) == 1 &&
        HIBYTE(wVersionRequested) < 1)) return WSAVERNOTSUPPORTED;

    if (!lpWSAData) return WSAEINVAL;

    /* initialize socket heap */

    if( !_WSHeap )
    {
	_WSHeap = HeapCreate(HEAP_ZERO_MEMORY, 8120, 32768);
	if( !_WSHeap )
	{
	    ERR("Fatal: failed to create WinSock heap\n");
	    return 0;
	}
    }
    if( _WSHeap == 0 ) return WSASYSNOTREADY;

    num_startup++;

    /* return winsock information */

    memcpy(lpWSAData, &data, sizeof(data));

    TRACE("succeeded\n");
    return 0;
}

/***********************************************************************
 *      WSAStartup		(WS2_32.115)
 */
int WINAPI WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData)
{
    TRACE("verReq=%x\n", wVersionRequested);

    if (LOBYTE(wVersionRequested) < 1)
        return WSAVERNOTSUPPORTED;

    if (!lpWSAData) return WSAEINVAL;

    /* initialize socket heap */

    if( !_WSHeap )
    {
	_WSHeap = HeapCreate(HEAP_ZERO_MEMORY, 8120, 32768);
	if( !_WSHeap )
	{
	    ERR("Fatal: failed to create WinSock heap\n");
	    return 0;
	}
    }
    if( _WSHeap == 0 ) return WSASYSNOTREADY;

    num_startup++;

    /* that's the whole of the negotiation for now */
    lpWSAData->wVersion = wVersionRequested;
    /* return winsock information */
    lpWSAData->wHighVersion = 0x0202;
    strcpy(lpWSAData->szDescription, "WinSock 2.0" );
    strcpy(lpWSAData->szSystemStatus, "Running" );
    lpWSAData->iMaxSockets = WS_MAX_SOCKETS_PER_PROCESS;
    lpWSAData->iMaxUdpDg = WS_MAX_UDP_DATAGRAM;
    /* don't do anything with lpWSAData->lpVendorInfo */
    /* (some apps don't allocate the space for this field) */

    TRACE("succeeded\n");
    return 0;
}


/***********************************************************************
 *      WSACleanup			(WINSOCK.116)
 *      WSACleanup			(WS2_32.116)
 */
INT WINAPI WSACleanup(void)
{
    if (num_startup)
    {
        if (--num_startup > 0) return 0;
        WINSOCK_DeleteIData();
        return 0;
    }
    SetLastError(WSANOTINITIALISED);
    return SOCKET_ERROR;
}


/***********************************************************************
 *      WSAGetLastError		(WINSOCK.111)
 *      WSAGetLastError		(WS2_32.111)
 */
INT WINAPI WSAGetLastError(void)
{
	return GetLastError();
}

/***********************************************************************
 *      WSASetLastError		(WS2_32.112)
 */
void WINAPI WSASetLastError(INT iError) {
    SetLastError(iError);
}

/***********************************************************************
 *      WSASetLastError		(WINSOCK.112)
 */
void WINAPI WSASetLastError16(INT16 iError)
{
    WSASetLastError(iError);
}

static char* check_buffer(int size)
{
    static int local_buflen;

    if (local_buffer)
    {
        if (local_buflen >= size ) return local_buffer;
        HeapFree( GetProcessHeap(), 0, local_buffer );
    }
    local_buffer = HeapAlloc( GetProcessHeap(), 0, (local_buflen = size) );
    return local_buffer;
}

static struct ws_hostent* check_buffer_he(int size)
{
    static int he_len;
    if (he_buffer)
    {
        if (he_len >= size ) return he_buffer;
        UnMapLS( he_buffer_seg );
        HeapFree( GetProcessHeap(), 0, he_buffer );
    }
    he_buffer = HeapAlloc( GetProcessHeap(), 0, (he_len = size) );
    he_buffer_seg = MapLS( he_buffer );
    return he_buffer;
}

static void* check_buffer_se(int size)
{
    static int se_len;
    if (se_buffer)
    {
        if (se_len >= size ) return se_buffer;
        UnMapLS( se_buffer_seg );
        HeapFree( GetProcessHeap(), 0, se_buffer );
    }
    se_buffer = HeapAlloc( GetProcessHeap(), 0, (se_len = size) );
    se_buffer_seg = MapLS( se_buffer );
    return se_buffer;
}

static struct ws_protoent* check_buffer_pe(int size)
{
    static int pe_len;
    if (pe_buffer)
    {
        if (pe_len >= size ) return pe_buffer;
        UnMapLS( pe_buffer_seg );
        HeapFree( GetProcessHeap(), 0, pe_buffer );
    }
    pe_buffer = HeapAlloc( GetProcessHeap(), 0, (pe_len = size) );
    pe_buffer_seg = MapLS( he_buffer );
    return pe_buffer;
}

/* ----------------------------------- i/o APIs */

#ifdef HAVE_IPX
#define SUPPORTED_PF(pf) ((pf)==WS_AF_INET || (pf)== WS_AF_IPX)
#else
#define SUPPORTED_PF(pf) ((pf)==WS_AF_INET)
#endif


/**********************************************************************/

/* Returns the converted address if successful, NULL if it was too small to
 * start with. Note that the returned pointer may be the original pointer
 * if no conversion is necessary.
 */
static const struct sockaddr* ws_sockaddr_ws2u(const struct WS_sockaddr* wsaddr, int wsaddrlen, int *uaddrlen)
{
    switch (wsaddr->sa_family)
    {
#ifdef HAVE_IPX
    case WS_AF_IPX:
        {
            struct WS_sockaddr_ipx* wsipx=(struct WS_sockaddr_ipx*)wsaddr;
            struct sockaddr_ipx* uipx;

            if (wsaddrlen<sizeof(struct WS_sockaddr_ipx))
                return NULL;

            *uaddrlen=sizeof(struct sockaddr_ipx);
            uipx=malloc(*uaddrlen);
            uipx->sipx_family=AF_IPX;
            uipx->sipx_port=wsipx->sa_socket;
            /* copy sa_netnum and sa_nodenum to sipx_network and sipx_node
             * in one go
             */
            memcpy(&uipx->sipx_network,wsipx->sa_netnum,sizeof(uipx->sipx_network)+sizeof(uipx->sipx_node));
#ifdef IPX_FRAME_NONE
            uipx->sipx_type=IPX_FRAME_NONE;
#endif
            memset(&uipx->sipx_zero,0,sizeof uipx->sipx_zero);
            return (const struct sockaddr*)uipx;
        }
#endif

    default:
        if (wsaddrlen<sizeof(struct WS_sockaddr))
            return NULL;

        /* No conversion needed, just return the original address */
        *uaddrlen=wsaddrlen;
        return (const struct sockaddr*)wsaddr;
    }
    return NULL;
}

/* Allocates a Unix sockaddr structure to receive the data */
inline struct sockaddr* ws_sockaddr_alloc(const struct WS_sockaddr* wsaddr, int* wsaddrlen, int* uaddrlen)
{
    if (wsaddr==NULL)
    {
      ERR( "WINE shouldn't pass a NULL wsaddr! Attempting to continue\n" );

      /* This is not strictly the right thing to do. Hope it works however */
      *uaddrlen=0;

      return NULL;
    }

    if (*wsaddrlen==0)
        *uaddrlen=0;
    else
        *uaddrlen=max(sizeof(struct sockaddr),*wsaddrlen);

    return malloc(*uaddrlen);
}

/* Returns 0 if successful, -1 if the buffer is too small */
static int ws_sockaddr_u2ws(const struct sockaddr* uaddr, int uaddrlen, struct WS_sockaddr* wsaddr, int* wsaddrlen)
{
    int res;

    switch(uaddr->sa_family)
    {
#ifdef HAVE_IPX
    case AF_IPX:
        {
            struct sockaddr_ipx* uipx=(struct sockaddr_ipx*)uaddr;
            struct WS_sockaddr_ipx* wsipx=(struct WS_sockaddr_ipx*)wsaddr;

            res=-1;
            switch (*wsaddrlen) /* how much can we copy? */
            {
            default:
                res=0; /* enough */
                *wsaddrlen=uaddrlen;
                wsipx->sa_socket=uipx->sipx_port;
                /* fall through */
            case 13:
            case 12:
                memcpy(wsipx->sa_nodenum,uipx->sipx_node,sizeof(wsipx->sa_nodenum));
                /* fall through */
            case 11:
            case 10:
            case 9:
            case 8:
            case 7:
            case 6:
                memcpy(wsipx->sa_netnum,&uipx->sipx_network,sizeof(wsipx->sa_netnum));
                /* fall through */
            case 5:
            case 4:
            case 3:
            case 2:
                wsipx->sa_family=WS_AF_IPX;
                /* fall through */
            case 1:
            case 0:
                /* way too small */
                break;
            }
        }
        break;
#endif

    default:
        /* No conversion needed */
        memcpy(wsaddr,uaddr,*wsaddrlen);
        if (*wsaddrlen<uaddrlen) {
            res=-1;
        } else {
            *wsaddrlen=uaddrlen;
            res=0;
        }
    }
    return res;
}

/* to be called to free the memory allocated by ws_sockaddr_ws2u or
 * ws_sockaddr_alloc
 */
inline void ws_sockaddr_free(const struct sockaddr* uaddr, const struct WS_sockaddr* wsaddr)
{
    if (uaddr!=NULL && uaddr!=(const struct sockaddr*)wsaddr)
        free((void*)uaddr);
}

/**************************************************************************
 * Functions for handling overlapped I/O
 **************************************************************************/

static DWORD ws2_async_get_status (const struct async_private *ovp)
{
    return ((ws2_async*) ovp)->overlapped->Internal;
}

static VOID ws2_async_set_status (struct async_private *ovp, const DWORD status)
{
    ((ws2_async*) ovp)->overlapped->Internal = status;
}

static DWORD ws2_async_get_count (const struct async_private *ovp)
{
    return ((ws2_async*) ovp)->overlapped->InternalHigh;
}

static void ws2_async_cleanup ( struct async_private *ap )
{
    struct ws2_async *as = (struct ws2_async*) ap;

    TRACE ( "as: %p uovl %p ovl %p\n", as, as->user_overlapped, as->overlapped );
    if ( !as->user_overlapped )
    {
        if ( as->overlapped->hEvent != INVALID_HANDLE_VALUE )
            WSACloseEvent ( as->overlapped->hEvent  );
        HeapFree ( GetProcessHeap(), 0, as->overlapped );
    }

    if ( as->iovec )
        HeapFree ( GetProcessHeap(), 0, as->iovec );

    HeapFree ( GetProcessHeap(), 0, as );
}

static void CALLBACK ws2_async_call_completion (ULONG_PTR data)
{
    ws2_async* as = (ws2_async*) data;

    TRACE ("data: %p\n", as);

    as->completion_func ( NtStatusToWSAError (as->overlapped->Internal),
                          as->overlapped->InternalHigh,
                          as->user_overlapped,
                          as->flags );
    ws2_async_cleanup ( &as->async );
}

/***********************************************************************
 *              WS2_make_async          (INTERNAL)
 */

static void WS2_async_recv (async_private *as);
static void WS2_async_send (async_private *as);

inline static struct ws2_async*
WS2_make_async (SOCKET s, int fd, int type, struct iovec *iovec, DWORD dwBufferCount,
                LPDWORD lpFlags, struct WS_sockaddr *addr,
                LPINT addrlen, LPWSAOVERLAPPED lpOverlapped,
                LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    struct ws2_async *wsa = HeapAlloc ( GetProcessHeap(), 0, sizeof ( ws2_async ) );

    TRACE ( "wsa %p\n", wsa );

    if (!wsa)
        return NULL;

    wsa->async.ops = ( lpCompletionRoutine ? &ws2_async_ops : &ws2_nocomp_async_ops );
    wsa->async.handle = (HANDLE) s;
    wsa->async.fd = fd;
    wsa->async.type = type;
    switch (type)
    {
    case ASYNC_TYPE_READ:
        wsa->flags = *lpFlags;
        wsa->async.func = WS2_async_recv;
        wsa->addrlen.ptr = addrlen;
        break;
    case ASYNC_TYPE_WRITE:
        wsa->flags = 0;
        wsa->async.func = WS2_async_send;
        wsa->addrlen.val = *addrlen;
        break;
    default:
        ERR ("Invalid async type: %d\n", type);
    }
    wsa->user_overlapped = lpOverlapped;
    wsa->completion_func = lpCompletionRoutine;
    wsa->iovec = iovec;
    wsa->n_iovecs = dwBufferCount;
    wsa->addr = addr;

    if ( lpOverlapped )
    {
        wsa->overlapped = lpOverlapped;
        wsa->async.event = ( lpCompletionRoutine ? INVALID_HANDLE_VALUE : lpOverlapped->hEvent );
    }
    else
    {
        wsa->overlapped = HeapAlloc ( GetProcessHeap(), 0,
                                      sizeof (WSAOVERLAPPED) );
        if ( !wsa->overlapped )
            goto error;
        wsa->async.event = wsa->overlapped->hEvent = INVALID_HANDLE_VALUE;
    }

    wsa->overlapped->InternalHigh = 0;
    TRACE ( "wsa %p, ops %p, h %p, ev %p, fd %d, func %p, ov %p, uov %p, cfunc %p\n",
            wsa, wsa->async.ops, wsa->async.handle, wsa->async.event, wsa->async.fd, wsa->async.func,
            wsa->overlapped, wsa->user_overlapped, wsa->completion_func );

    return wsa;

error:
    TRACE ("Error\n");
    HeapFree ( GetProcessHeap(), 0, wsa );
    return NULL;
}

/***********************************************************************
 *              WS2_recv                (INTERNAL)
 *
 * Work horse for both synchronous and asynchronous recv() operations.
 */
static int WS2_recv ( int fd, struct iovec* iov, int count,
                      struct WS_sockaddr *lpFrom, LPINT lpFromlen,
                      LPDWORD lpFlags )
{
    struct msghdr hdr;
    int n;
    TRACE ( "fd %d, iovec %p, count %d addr %p, len %p, flags %lx\n",
            fd, iov, count, lpFrom, lpFromlen, *lpFlags);

    hdr.msg_name = NULL;

    if ( lpFrom )
    {
#if DEBUG_SOCKADDR
        dump_sockaddr (lpFrom);
#endif

        hdr.msg_namelen = *lpFromlen;
        hdr.msg_name = ws_sockaddr_alloc ( lpFrom, lpFromlen, &hdr.msg_namelen );
        if ( !hdr.msg_name )
        {
            WSASetLastError ( WSAEFAULT );
            n = -1;
            goto out;
        }
    }
    else
        hdr.msg_namelen = 0;

    hdr.msg_iov = iov;
    hdr.msg_iovlen = count;
#ifdef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    hdr.msg_accrights = NULL;
    hdr.msg_accrightslen = 0;
#else
    hdr.msg_control = NULL;
    hdr.msg_controllen = 0;
    hdr.msg_flags = 0;
#endif

    if ( (n = recvmsg (fd, &hdr, *lpFlags)) == -1 )
    {
        TRACE ( "recvmsg error %d\n", errno);
        goto out;
    }

    if ( lpFrom &&
         ws_sockaddr_u2ws ( hdr.msg_name, hdr.msg_namelen,
                            lpFrom, lpFromlen ) != 0 )
    {
        /* The from buffer was too small, but we read the data
         * anyway. Is that really bad?
         */
        WSASetLastError ( WSAEFAULT );
        WARN ( "Address buffer too small\n" );
    }

out:

    ws_sockaddr_free ( hdr.msg_name, lpFrom );
    TRACE ("-> %d\n", n);
    return n;
}

/***********************************************************************
 *              WS2_async_recv          (INTERNAL)
 *
 * Handler for overlapped recv() operations.
 */
static void WS2_async_recv ( async_private *as )
{
    ws2_async* wsa = (ws2_async*) as;
    int result, err;

    TRACE ( "async %p\n", wsa );

    if ( wsa->overlapped->Internal != STATUS_PENDING )
    {
        TRACE ( "status: %ld\n", wsa->overlapped->Internal );
        return;
    }

    result = WS2_recv ( wsa->async.fd, wsa->iovec, wsa->n_iovecs,
                        wsa->addr, wsa->addrlen.ptr, &wsa->flags );

    if (result >= 0)
    {
        wsa->overlapped->Internal = STATUS_SUCCESS;
        wsa->overlapped->InternalHigh = result;
        TRACE ( "received %d bytes\n", result );
        _enable_event ( wsa->async.handle, FD_READ, 0, 0 );
        return;
    }

    err = wsaErrno ();
    if ( err == WSAEINTR || err == WSAEWOULDBLOCK )  /* errno: EINTR / EAGAIN */
    {
        wsa->overlapped->Internal = STATUS_PENDING;
        _enable_event ( wsa->async.handle, FD_READ, 0, 0 );
        TRACE ( "still pending\n" );
    }
    else
    {
        wsa->overlapped->Internal = err;
        TRACE ( "Error: %x\n", err );
    }
}

/***********************************************************************
 *              WS2_send                (INTERNAL)
 *
 * Work horse for both synchronous and asynchronous send() operations.
 */
static int WS2_send ( int fd, struct iovec* iov, int count,
                      const struct WS_sockaddr *to, INT tolen, DWORD dwFlags )
{
    struct msghdr hdr;
    int n = -1;
    TRACE ( "fd %d, iovec %p, count %d addr %p, len %d, flags %lx\n",
            fd, iov, count, to, tolen, dwFlags);

    hdr.msg_name = NULL;

    if ( to )
    {
#if DEBUG_SOCKADDR
        dump_sockaddr (to);
#endif
        hdr.msg_name = (struct sockaddr*) ws_sockaddr_ws2u ( to, tolen, &hdr.msg_namelen );
        if ( !hdr.msg_name )
        {
            WSASetLastError ( WSAEFAULT );
            goto out;
        }
    }
    else
        hdr.msg_namelen = 0;

    hdr.msg_iov = iov;
    hdr.msg_iovlen = count;
#ifdef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    hdr.msg_accrights = NULL;
    hdr.msg_accrightslen = 0;
#else
    hdr.msg_control = NULL;
    hdr.msg_controllen = 0;
    hdr.msg_flags = 0;
#endif

    n = sendmsg (fd, &hdr, dwFlags);

out:
    ws_sockaddr_free ( hdr.msg_name, to );
    return n;
}

/***********************************************************************
 *              WS2_async_send          (INTERNAL)
 *
 * Handler for overlapped send() operations.
 */
static void WS2_async_send ( async_private *as )
{
    ws2_async* wsa = (ws2_async*) as;
    int result, err;

    TRACE ( "async %p\n", wsa );

    if ( wsa->overlapped->Internal != STATUS_PENDING )
    {
        TRACE ( "status: %ld\n", wsa->overlapped->Internal );
        return;
    }

    result = WS2_send ( wsa->async.fd, wsa->iovec, wsa->n_iovecs,
                        wsa->addr, wsa->addrlen.val, wsa->flags );

    if (result >= 0)
    {
        wsa->overlapped->Internal = STATUS_SUCCESS;
        wsa->overlapped->InternalHigh = result;
        TRACE ( "sent %d bytes\n", result );
        _enable_event ( wsa->async.handle, FD_WRITE, 0, 0 );
        return;
    }

    err = wsaErrno ();
    if ( err == WSAEINTR )
    {
        wsa->overlapped->Internal = STATUS_PENDING;
        _enable_event ( wsa->async.handle, FD_WRITE, 0, 0 );
        TRACE ( "still pending\n" );
    }
    else
    {
        /* We set the status to a winsock error code and check for that
           later in NtStatusToWSAError () */
        wsa->overlapped->Internal = err;
        TRACE ( "Error: %x\n", err );
    }
}

/***********************************************************************
 *              WS2_async_shutdown      (INTERNAL)
 *
 * Handler for shutdown() operations on overlapped sockets.
 */
static void WS2_async_shutdown ( async_private *as )
{
    ws2_async* wsa = (ws2_async*) as;
    int err = 1;

    TRACE ( "async %p %d\n", wsa, wsa->async.type );
    switch ( wsa->async.type )
    {
    case ASYNC_TYPE_READ:
        err = shutdown ( wsa->async.fd, 0 );
        break;
    case ASYNC_TYPE_WRITE:
        err = shutdown ( wsa->async.fd, 1 );
        break;
    default:
        ERR ("invalid type: %d\n", wsa->async.type );
    }

    if ( err )
        wsa->overlapped->Internal = wsaErrno ();
    else
        wsa->overlapped->Internal = STATUS_SUCCESS;
}

/***********************************************************************
 *  WS2_register_async_shutdown         (INTERNAL)
 *
 * Helper function for WS_shutdown() on overlapped sockets.
 */
static int WS2_register_async_shutdown ( SOCKET s, int fd, int type )
{
    struct ws2_async *wsa;
    int ret, err = WSAEFAULT;
    DWORD dwflags = 0;
    int len = 0;
    LPWSAOVERLAPPED ovl = HeapAlloc (GetProcessHeap(), 0, sizeof ( WSAOVERLAPPED ));

    TRACE ("s %d fd %d type %d\n", s, fd, type);
    if (!ovl)
        goto out;

    ovl->hEvent = WSACreateEvent ();
    if ( ovl->hEvent == WSA_INVALID_EVENT  )
        goto out_free;

    wsa = WS2_make_async ( s, fd, type, NULL, 0,
                           &dwflags, NULL, &len, ovl, NULL );
    if ( !wsa )
        goto out_close;

    /* Hack: this will cause ws2_async_cleanup() to free the overlapped structure */
    wsa->user_overlapped = NULL;
    wsa->async.func = WS2_async_shutdown;
    if ( (ret = register_new_async ( &wsa->async )) )
    {
        err = NtStatusToWSAError ( ret );
        goto out;
    }
    /* Try immediate completion */
    while ( WaitForSingleObjectEx ( ovl->hEvent, 0, TRUE ) == STATUS_USER_APC );
    return 0;

out_close:
    WSACloseEvent ( ovl->hEvent );
out_free:
    HeapFree ( GetProcessHeap(), 0, ovl );
out:
    return err;
}

/***********************************************************************
 *		accept		(WS2_32.1)
 */
SOCKET WINAPI WS_accept(SOCKET s, struct WS_sockaddr *addr,
                                 int *addrlen32)
{
    int fd = _get_sock_fd(s);

    TRACE("socket %04x\n", (UINT16)s );
    if (fd != -1)
    {
        SOCKET as;
	if (_is_blocking(s))
	{
	    /* block here */
	    do_block(fd, 5);
	    _sync_sock_state(s); /* let wineserver notice connection */
	    /* retrieve any error codes from it */
	    SetLastError(_get_sock_error(s, FD_ACCEPT_BIT));
	    /* FIXME: care about the error? */
	}
        close(fd);
        SERVER_START_REQ( accept_socket )
        {
            req->lhandle = SOCKET2HANDLE(s);
            req->access  = GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE;
            req->inherit = TRUE;
            set_error( wine_server_call( req ) );
            as = HANDLE2SOCKET( reply->handle );
        }
        SERVER_END_REQ;
	if (as)
	{
	    if (addr)
                WS_getpeername(as, addr, addrlen32);
	    return as;
	}
    }
    else
    {
        SetLastError(WSAENOTSOCK);
    }
    return INVALID_SOCKET;
}

/***********************************************************************
 *              accept		(WINSOCK.1)
 */
SOCKET16 WINAPI WINSOCK_accept16(SOCKET16 s, struct WS_sockaddr* addr,
                                 INT16* addrlen16 )
{
    INT addrlen32 = addrlen16 ? *addrlen16 : 0;
    SOCKET retSocket = WS_accept( s, addr, &addrlen32 );
    if( addrlen16 ) *addrlen16 = (INT16)addrlen32;
    return (SOCKET16)retSocket;
}

/***********************************************************************
 *		bind			(WS2_32.2)
 */
int WINAPI WS_bind(SOCKET s, const struct WS_sockaddr* name, int namelen)
{
    int fd = _get_sock_fd(s);
    int res;

    TRACE("socket %04x, ptr %p, length %d\n", s, name, namelen);
#if DEBUG_SOCKADDR
    dump_sockaddr(name);
#endif

    res=SOCKET_ERROR;
    if (fd != -1)
    {
        if (!name || !SUPPORTED_PF(name->sa_family))
        {
            SetLastError(WSAEAFNOSUPPORT);
        }
        else
        {
            const struct sockaddr* uaddr;
            int uaddrlen;

            uaddr=ws_sockaddr_ws2u(name,namelen,&uaddrlen);
            if (uaddr == NULL)
            {
                SetLastError(WSAEFAULT);
            }
            else
            {
                int on = 1;
                /* The game GrandPrixLegends binds more than one time, but does
                 * not do a SO_REUSEADDR - Stevens says this is ok */
                TRACE( "Setting WS_SO_REUSEADDR on socket before we bind it\n");
                WS_setsockopt( s, WS_SOL_SOCKET, WS_SO_REUSEADDR, (char*)&on, sizeof(on) );

                if (bind(fd, uaddr, uaddrlen) < 0)
                {
                    int loc_errno = errno;
                    WARN("\tfailure - errno = %i\n", errno);
                    errno = loc_errno;
                    switch (errno)
                    {
                    case EBADF:
                        SetLastError(WSAENOTSOCK);
                        break;
                    case EADDRNOTAVAIL:
                        SetLastError(WSAEINVAL);
                        break;
                    default:
                        SetLastError(wsaErrno());
                        break;
                    }
                }
                else
                {
                    res=0; /* success */
                }
                ws_sockaddr_free(uaddr,name);
            }
        }
        close(fd);
    }
    else
    {
        SetLastError(WSAENOTSOCK);
    }
    return res;
}

/***********************************************************************
 *              bind			(WINSOCK.2)
 */
INT16 WINAPI WINSOCK_bind16(SOCKET16 s, struct WS_sockaddr *name, INT16 namelen)
{
  return (INT16)WS_bind( s, name, namelen );
}

/***********************************************************************
 *		closesocket		(WS2_32.3)
 */
int WINAPI WS_closesocket(SOCKET s)
{
    TRACE("socket %08x\n", s);
    if (CloseHandle(SOCKET2HANDLE(s))) return 0;
    return SOCKET_ERROR;
}

/***********************************************************************
 *              closesocket           (WINSOCK.3)
 */
INT16 WINAPI WINSOCK_closesocket16(SOCKET16 s)
{
    return (INT16)WS_closesocket(s);
}

/***********************************************************************
 *		connect		(WS2_32.4)
 */
int WINAPI WS_connect(SOCKET s, const struct WS_sockaddr* name, int namelen)
{
    int fd = _get_sock_fd(s);

    TRACE("socket %04x, ptr %p, length %d\n", s, name, namelen);
#if DEBUG_SOCKADDR
    dump_sockaddr(name);
#endif

    if (fd != -1)
    {
        const struct sockaddr* uaddr;
        int uaddrlen;

        uaddr=ws_sockaddr_ws2u(name,namelen,&uaddrlen);
        if (uaddr == NULL)
        {
            SetLastError(WSAEFAULT);
        }
        else
        {
            int rc;

            rc=connect(fd, uaddr, uaddrlen);
            ws_sockaddr_free(uaddr,name);
            if (rc == 0)
                goto connect_success;
        }

        if (errno == EINPROGRESS)
        {
            /* tell wineserver that a connection is in progress */
            _enable_event(SOCKET2HANDLE(s), FD_CONNECT|FD_READ|FD_WRITE,
                          FD_CONNECT|FD_READ|FD_WRITE,
                          FD_WINE_CONNECTED|FD_WINE_LISTENING);
            if (_is_blocking(s))
            {
                int result;
                /* block here */
                do_block(fd, 7);
                _sync_sock_state(s); /* let wineserver notice connection */
                /* retrieve any error codes from it */
                result = _get_sock_error(s, FD_CONNECT_BIT);
                if (result)
                    SetLastError(result);
                else
                {
                    goto connect_success;
                }
            }
            else
            {
                SetLastError(WSAEWOULDBLOCK);
            }
        }
        else
        {
            SetLastError(wsaErrno());
        }
        close(fd);
    }
    else
    {
        SetLastError(WSAENOTSOCK);
    }
    return SOCKET_ERROR;

connect_success:
    close(fd);
    _enable_event(SOCKET2HANDLE(s), FD_CONNECT|FD_READ|FD_WRITE,
                  FD_WINE_CONNECTED|FD_READ|FD_WRITE,
                  FD_CONNECT|FD_WINE_LISTENING);
    return 0;
}

/***********************************************************************
 *              connect               (WINSOCK.4)
 */
INT16 WINAPI WINSOCK_connect16(SOCKET16 s, struct WS_sockaddr *name, INT16 namelen)
{
  return (INT16)WS_connect( s, name, namelen );
}

/***********************************************************************
 *              WSAConnect             (WS2_32.30)
 */
int WINAPI WSAConnect ( SOCKET s, const struct WS_sockaddr* name, int namelen,
                        LPWSABUF lpCallerData, LPWSABUF lpCalleeData,
                        LPQOS lpSQOS, LPQOS lpGQOS )
{
    if ( lpCallerData || lpCalleeData || lpSQOS || lpGQOS )
        FIXME ("unsupported parameters!\n");
    return WS_connect ( s, name, namelen );
}


/***********************************************************************
 *		getpeername		(WS2_32.5)
 */
int WINAPI WS_getpeername(SOCKET s, struct WS_sockaddr *name, int *namelen)
{
    int fd;
    int res;

    TRACE("socket: %04x, ptr %p, len %8x\n", s, name, *namelen);

    /* Check if what we've received is valid. Should we use IsBadReadPtr? */
    if( (name == NULL) || (namelen == NULL) )
    {
        SetLastError( WSAEFAULT );
        return SOCKET_ERROR;
    }

    fd = _get_sock_fd(s);
    res = SOCKET_ERROR;

    if (fd != -1)
    {
        struct sockaddr* uaddr;
        int uaddrlen;

        uaddr=ws_sockaddr_alloc(name,namelen,&uaddrlen);
        if (getpeername(fd, uaddr, &uaddrlen) != 0)
        {
            SetLastError(wsaErrno());
        }
        else if (ws_sockaddr_u2ws(uaddr,uaddrlen,name,namelen) != 0)
        {
            /* The buffer was too small */
            SetLastError(WSAEFAULT);
        }
        else
        {
            res=0;
        }
        ws_sockaddr_free(uaddr,name);
        close(fd);
    }
    else
    {
        SetLastError(WSAENOTSOCK);
    }
    return res;
}

/***********************************************************************
 *              getpeername		(WINSOCK.5)
 */
INT16 WINAPI WINSOCK_getpeername16(SOCKET16 s, struct WS_sockaddr *name,
                                   INT16 *namelen16)
{
    INT namelen32 = *namelen16;
    INT retVal = WS_getpeername( s, name, &namelen32 );

#if DEBUG_SOCKADDR
    dump_sockaddr(name);
#endif

   *namelen16 = namelen32;
    return (INT16)retVal;
}

/***********************************************************************
 *		getsockname		(WS2_32.6)
 */
int WINAPI WS_getsockname(SOCKET s, struct WS_sockaddr *name, int *namelen)
{
    int fd;
    int res;

    TRACE("socket: %04x, ptr %p, len %8x\n", s, name, *namelen);

    /* Check if what we've received is valid. Should we use IsBadReadPtr? */
    if( (name == NULL) || (namelen == NULL) )
    {
        SetLastError( WSAEFAULT );
        return SOCKET_ERROR;
    }

    fd = _get_sock_fd(s);
    res = SOCKET_ERROR;

    if (fd != -1)
    {
        struct sockaddr* uaddr;
        int uaddrlen;

        uaddr=ws_sockaddr_alloc(name,namelen,&uaddrlen);
        if (getsockname(fd, uaddr, &uaddrlen) != 0)
        {
            SetLastError(wsaErrno());
        }
        else if (ws_sockaddr_u2ws(uaddr,uaddrlen,name,namelen) != 0)
        {
            /* The buffer was too small */
            SetLastError(WSAEFAULT);
        }
        else
        {
            res=0;
        }
        close(fd);
    }
    else
    {
        SetLastError(WSAENOTSOCK);
    }
    return res;
}

/***********************************************************************
 *              getsockname		(WINSOCK.6)
 */
INT16 WINAPI WINSOCK_getsockname16(SOCKET16 s, struct WS_sockaddr *name,
                                   INT16 *namelen16)
{
    INT retVal;

    if( namelen16 )
    {
        INT namelen32 = *namelen16;
        retVal = WS_getsockname( s, name, &namelen32 );
       *namelen16 = namelen32;

#if DEBUG_SOCKADDR
    dump_sockaddr(name);
#endif

    }
    else retVal = SOCKET_ERROR;
    return (INT16)retVal;
}


/***********************************************************************
 *		getsockopt		(WS2_32.7)
 */
INT WINAPI WS_getsockopt(SOCKET s, INT level,
                                  INT optname, char *optval, INT *optlen)
{
    int fd;

    TRACE("socket: %04x, level 0x%x, name 0x%x, ptr %8x, len %d\n", s, level,
          (int) optname, (int) optval, (int) *optlen);
    /* SO_OPENTYPE does not require a valid socket handle. */
    if (level == WS_SOL_SOCKET && optname == WS_SO_OPENTYPE)
    {
        if (!optlen || *optlen < sizeof(int) || !optval)
        {
            SetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }
        *(int *)optval = (int)TlsGetValue( opentype_tls_index );
        *optlen = sizeof(int);
        TRACE("getting global SO_OPENTYPE = 0x%x\n", *((int*)optval) );
        return 0;
    }

    fd = _get_sock_fd(s);
    if (fd != -1)
    {
	if (!convert_sockopt(&level, &optname)) {
	    SetLastError(WSAENOPROTOOPT);	/* Unknown option */
        } else {
	    if (getsockopt(fd, (int) level, optname, optval, optlen) == 0 )
	    {
		close(fd);
		return 0;
	    }
	    SetLastError((errno == EBADF) ? WSAENOTSOCK : wsaErrno());
	}
	close(fd);
    }
    return SOCKET_ERROR;
}


/***********************************************************************
 *              getsockopt		(WINSOCK.7)
 */
INT16 WINAPI WINSOCK_getsockopt16(SOCKET16 s, INT16 level,
                                  INT16 optname, char *optval, INT16 *optlen)
{
    INT optlen32;
    INT *p = &optlen32;
    INT retVal;
    if( optlen ) optlen32 = *optlen; else p = NULL;
    retVal = WS_getsockopt( s, (UINT16)level, optname, optval, p );
    if( optlen ) *optlen = optlen32;
    return (INT16)retVal;
}


/***********************************************************************
 *		htonl			(WINSOCK.8)
 *		htonl			(WS2_32.8)
 */
u_long WINAPI WS_htonl(u_long hostlong)
{
    return htonl(hostlong);
}


/***********************************************************************
 *		htons			(WINSOCK.9)
 *		htons			(WS2_32.9)
 */
u_short WINAPI WS_htons(u_short hostshort)
{
    return htons(hostshort);
}


/***********************************************************************
 *		inet_addr		(WINSOCK.10)
 *		inet_addr		(WS2_32.11)
 */
u_long WINAPI WS_inet_addr(const char *cp)
{
    return inet_addr(cp);
}


/***********************************************************************
 *		ntohl			(WINSOCK.14)
 *		ntohl			(WS2_32.14)
 */
u_long WINAPI WS_ntohl(u_long netlong)
{
    return ntohl(netlong);
}


/***********************************************************************
 *		ntohs			(WINSOCK.15)
 *		ntohs			(WS2_32.15)
 */
u_short WINAPI WS_ntohs(u_short netshort)
{
    return ntohs(netshort);
}


/***********************************************************************
 *		inet_ntoa		(WS2_32.12)
 */
char* WINAPI WS_inet_ntoa(struct WS_in_addr in)
{
  /* use "buffer for dummies" here because some applications have
   * propensity to decode addresses in ws_hostent structure without
   * saving them first...
   */
    static char dbuffer[16]; /* Yes, 16: 4*3 digits + 3 '.' + 1 '\0' */

    char* s = inet_ntoa(*((struct in_addr*)&in));
    if( s )
    {
        strcpy(dbuffer, s);
        return dbuffer;
    }
    SetLastError(wsaErrno());
    return NULL;
}

/***********************************************************************
 *		inet_ntoa		(WINSOCK.11)
 */
SEGPTR WINAPI WINSOCK_inet_ntoa16(struct in_addr in)
{
    char* retVal;
    if (!(retVal = WS_inet_ntoa(*((struct WS_in_addr*)&in)))) return 0;
    if (!dbuffer_seg) dbuffer_seg = MapLS( retVal );
    return dbuffer_seg;
}


/**********************************************************************
 *              WSAIoctl                (WS2_32.50)
 *
 *
 *   FIXME:  Only SIO_GET_INTERFACE_LIST option implemented.
 */
INT WINAPI WSAIoctl (SOCKET s,
                     DWORD   dwIoControlCode,
                     LPVOID  lpvInBuffer,
                     DWORD   cbInBuffer,
                     LPVOID  lpbOutBuffer,
                     DWORD   cbOutBuffer,
                     LPDWORD lpcbBytesReturned,
                     LPWSAOVERLAPPED lpOverlapped,
                     LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
   int fd = _get_sock_fd(s);

   if (fd != -1)
   {
      switch( dwIoControlCode )
      {
         case SIO_GET_INTERFACE_LIST:
         {
            INTERFACE_INFO* intArray = (INTERFACE_INFO*)lpbOutBuffer;
            int i, numInt;
            struct ifreq ifInfo;
            char ifName[512];


            TRACE ("-> SIO_GET_INTERFACE_LIST request\n");

            numInt = WSAIOCTL_GetInterfaceCount();
            if (numInt < 0)
            {
               ERR ("Unable to open /proc filesystem to determine number of network interfaces!\n");
               close(fd);
               WSASetLastError(WSAEINVAL);
               return (SOCKET_ERROR);
            }

            for (i=0; i<numInt; i++)
            {
               if (!WSAIOCTL_GetInterfaceName(i, ifName))
               {
                  ERR ("Error parsing /proc filesystem!\n");
                  close(fd);
                  WSASetLastError(WSAEINVAL);
                  return (SOCKET_ERROR);
               }

               ifInfo.ifr_addr.sa_family = AF_INET;

               /* IP Address */
               strcpy (ifInfo.ifr_name, ifName);
               if (ioctl(fd, SIOCGIFADDR, &ifInfo) < 0)
               {
                  ERR ("Error obtaining IP address\n");
                  close(fd);
                  WSASetLastError(WSAEINVAL);
                  return (SOCKET_ERROR);
               }
               else
               {
                  struct WS_sockaddr_in *ipTemp = (struct WS_sockaddr_in *)&ifInfo.ifr_addr;

                  intArray->iiAddress.AddressIn.sin_family = AF_INET;
                  intArray->iiAddress.AddressIn.sin_port = ipTemp->sin_port;
                  intArray->iiAddress.AddressIn.sin_addr.WS_s_addr = ipTemp->sin_addr.S_un.S_addr;
               }

               /* Broadcast Address */
               strcpy (ifInfo.ifr_name, ifName);
               if (ioctl(fd, SIOCGIFBRDADDR, &ifInfo) < 0)
               {
                  ERR ("Error obtaining Broadcast IP address\n");
                  close(fd);
                  WSASetLastError(WSAEINVAL);
                  return (SOCKET_ERROR);
               }
               else
               {
                  struct WS_sockaddr_in *ipTemp = (struct WS_sockaddr_in *)&ifInfo.ifr_broadaddr;

                  intArray->iiBroadcastAddress.AddressIn.sin_family = AF_INET;
                  intArray->iiBroadcastAddress.AddressIn.sin_port = ipTemp->sin_port;
                  intArray->iiBroadcastAddress.AddressIn.sin_addr.WS_s_addr = ipTemp->sin_addr.S_un.S_addr;
               }

               /* Subnet Mask */
               strcpy (ifInfo.ifr_name, ifName);
               if (ioctl(fd, SIOCGIFNETMASK, &ifInfo) < 0)
               {
                  ERR ("Error obtaining Subnet IP address\n");
                  close(fd);
                  WSASetLastError(WSAEINVAL);
                  return (SOCKET_ERROR);
               }
               else
               {
                  /* Trying to avoid some compile problems across platforms.
                     (Linux, FreeBSD, Solaris...) */
                  #ifndef ifr_netmask
                    #ifndef ifr_addr
                       intArray->iiNetmask.AddressIn.sin_family = AF_INET;
                       intArray->iiNetmask.AddressIn.sin_port = 0;
                       intArray->iiNetmask.AddressIn.sin_addr.WS_s_addr = 0;
                       ERR ("Unable to determine Netmask on your platform!\n");
                    #else
                       struct WS_sockaddr_in *ipTemp = (struct WS_sockaddr_in *)&ifInfo.ifr_addr;

                       intArray->iiNetmask.AddressIn.sin_family = AF_INET;
                       intArray->iiNetmask.AddressIn.sin_port = ipTemp->sin_port;
                       intArray->iiNetmask.AddressIn.sin_addr.WS_s_addr = ipTemp->sin_addr.S_un.S_addr;
                    #endif
                  #else
                     struct WS_sockaddr_in *ipTemp = (struct WS_sockaddr_in *)&ifInfo.ifr_netmask;

                     intArray->iiNetmask.AddressIn.sin_family = AF_INET;
                     intArray->iiNetmask.AddressIn.sin_port = ipTemp->sin_port;
                     intArray->iiNetmask.AddressIn.sin_addr.WS_s_addr = ipTemp->sin_addr.S_un.S_addr;
                  #endif
               }

               /* Socket Status Flags */
               strcpy(ifInfo.ifr_name, ifName);
               if (ioctl(fd, SIOCGIFFLAGS, &ifInfo) < 0)
               {
                  ERR ("Error obtaining status flags for socket!\n");
                  close(fd);
                  WSASetLastError(WSAEINVAL);
                  return (SOCKET_ERROR);
               }
               else
               {
                  /* FIXME - Is this the right flag to use? */
                  intArray->iiFlags = ifInfo.ifr_flags;
               }
               intArray++; /* Prepare for another interface */
            }

            /* Calculate the size of the array being returned */
            *lpcbBytesReturned = sizeof(INTERFACE_INFO) * numInt;
            break;
         }

         default:
         {
            WARN("\tunsupported WS_IOCTL cmd (%08lx)\n", dwIoControlCode);
            close(fd);
            WSASetLastError(WSAEOPNOTSUPP);
            return (SOCKET_ERROR);
         }
      }

      /* Function executed with no errors */
      close(fd);
      return (0);
   }
   else
   {
      WSASetLastError(WSAENOTSOCK);
      return (SOCKET_ERROR);
   }
}


/*
  Helper function for WSAIoctl - Get count of the number of interfaces
  by parsing /proc filesystem.
*/
int WSAIOCTL_GetInterfaceCount(void)
{
   FILE *procfs;
   char buf[512];  /* Size doesn't matter, something big */
   int  intcnt=0;


   /* Open /proc filesystem file for network devices */
   procfs = fopen(PROCFS_NETDEV_FILE, "r");
   if (!procfs)
   {
      /* If we can't open the file, return an error */
      return (-1);
   }

   /* Omit first two lines, they are only headers */
   fgets(buf, sizeof buf, procfs);
   fgets(buf, sizeof buf, procfs);

   while (fgets(buf, sizeof buf, procfs))
   {
      /* Each line in the file represents a network interface */
      intcnt++;
   }

   fclose(procfs);
   return(intcnt);
}


/*
   Helper function for WSAIoctl - Get name of device from interface number
   by parsing /proc filesystem.
*/
int WSAIOCTL_GetInterfaceName(int intNumber, char *intName)
{
   FILE *procfs;
   char buf[512]; /* Size doesn't matter, something big */
   int  i;

   /* Open /proc filesystem file for network devices */
   procfs = fopen(PROCFS_NETDEV_FILE, "r");
   if (!procfs)
   {
      /* If we can't open the file, return an error */
      return (-1);
   }

   /* Omit first two lines, they are only headers */
   fgets(buf, sizeof(buf), procfs);
   fgets(buf, sizeof(buf), procfs);

   for (i=0; i<intNumber; i++)
   {
      /* Skip the lines that don't interest us. */
      fgets(buf, sizeof(buf), procfs);
   }
   fgets(buf, sizeof(buf), procfs); /* This is the line we want */


   /* Parse out the line, grabbing only the name of the device
      to the intName variable

      The Line comes in like this: (we only care about the device name)
      lo:   21970 377 0 0 0 0 0 0 21970 377 0 0 0 0 0 0
   */
   i=0;
   while (isspace(buf[i])) /* Skip initial space(s) */
   {
      i++;
   }

   while (buf[i])
   {
      if (isspace(buf[i]))
      {
         break;
      }

      if (buf[i] == ':')  /* FIXME: Not sure if this block (alias detection) works properly */
      {
         /* This interface could be an alias... */
         int hold = i;
         char *dotname = intName;
         *intName++ = buf[i++];

         while (isdigit(buf[i]))
         {
            *intName++ = buf[i++];
         }

         if (buf[i] != ':')
         {
            /* ... It wasn't, so back up */
            i = hold;
            intName = dotname;
         }

         if (buf[i] == '\0')
         {
            fclose(procfs);
            return(FALSE);
         }

         i++;
         break;
      }

      *intName++ = buf[i++];
   }
   *intName++ = '\0';

   fclose(procfs);
   return(TRUE);
 }


/***********************************************************************
 *		ioctlsocket		(WS2_32.10)
 */
int WINAPI WS_ioctlsocket(SOCKET s, long cmd, u_long *argp)
{
  int fd = _get_sock_fd(s);

  TRACE("socket %04x, cmd %08lx, ptr %8x\n", s, cmd, (unsigned) argp);
  if (fd != -1)
  {
    long 	newcmd  = cmd;

    switch( cmd )
    {
	case WS_FIONREAD:
		newcmd=FIONREAD;
		break;

	case WS_FIONBIO:
		newcmd=FIONBIO;
		if( _get_sock_mask(s) )
		{
		    /* AsyncSelect()'ed sockets are always nonblocking */
		    if (*argp) {
			close(fd);
			return 0;
		    }
		    SetLastError(WSAEINVAL);
		    close(fd);
		    return SOCKET_ERROR;
		}
		close(fd);
		if (*argp)
		    _enable_event(SOCKET2HANDLE(s), 0, FD_WINE_NONBLOCKING, 0);
		else
		    _enable_event(SOCKET2HANDLE(s), 0, 0, FD_WINE_NONBLOCKING);
		return 0;

	case WS_SIOCATMARK:
		newcmd=SIOCATMARK;
		break;

	case WS__IOW('f',125,u_long):
		WARN("Warning: WS1.1 shouldn't be using async I/O\n");
		SetLastError(WSAEINVAL);
		return SOCKET_ERROR;

        case SIOCGIFBRDADDR:
        case SIOCGIFNETMASK:
        case SIOCGIFADDR:
                /* These don't need any special handling.  They are used by
                   WsControl, and are here to suppress an unecessary warning. */
                break;


	default:
		/* Netscape tries hard to use bogus ioctl 0x667e */
		WARN("\tunknown WS_IOCTL cmd (%08lx)\n", cmd);
    }
    if( ioctl(fd, newcmd, (char*)argp ) == 0 )
    {
	close(fd);
	return 0;
    }
    SetLastError((errno == EBADF) ? WSAENOTSOCK : wsaErrno());
    close(fd);
  }
  return SOCKET_ERROR;
}

/***********************************************************************
 *              ioctlsocket           (WINSOCK.12)
 */
INT16 WINAPI WINSOCK_ioctlsocket16(SOCKET16 s, LONG cmd, ULONG *argp)
{
    return (INT16)WS_ioctlsocket( s, cmd, argp );
}


/***********************************************************************
 *		listen		(WS2_32.13)
 */
int WINAPI WS_listen(SOCKET s, int backlog)
{
    int fd = _get_sock_fd(s);

    TRACE("socket %04x, backlog %d\n", s, backlog);
    if (fd != -1)
    {
	if (listen(fd, backlog) == 0)
	{
	    close(fd);
	    _enable_event(SOCKET2HANDLE(s), FD_ACCEPT,
			  FD_WINE_LISTENING,
			  FD_CONNECT|FD_WINE_CONNECTED);
	    return 0;
	}
	SetLastError(wsaErrno());
    }
    else SetLastError(WSAENOTSOCK);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              listen		(WINSOCK.13)
 */
INT16 WINAPI WINSOCK_listen16(SOCKET16 s, INT16 backlog)
{
    return (INT16)WS_listen( s, backlog );
}


/***********************************************************************
 *		recv			(WS2_32.16)
 */
int WINAPI WS_recv(SOCKET s, char *buf, int len, int flags)
{
    DWORD n, dwFlags = flags;
    WSABUF wsabuf;

    wsabuf.len = len;
    wsabuf.buf = buf;

    if ( WSARecvFrom (s, &wsabuf, 1, &n, &dwFlags, NULL, NULL, NULL, NULL) == SOCKET_ERROR )
        return SOCKET_ERROR;
    else
        return n;
}

/***********************************************************************
 *              recv			(WINSOCK.16)
 */
INT16 WINAPI WINSOCK_recv16(SOCKET16 s, char *buf, INT16 len, INT16 flags)
{
    return (INT16)WS_recv( s, buf, len, flags );
}


/***********************************************************************
 *		recvfrom		(WS2_32.17)
 */
int WINAPI WS_recvfrom(SOCKET s, char *buf, INT len, int flags,
                                struct WS_sockaddr *from, int *fromlen)
{
    DWORD n, dwFlags = flags;
    WSABUF wsabuf;

    wsabuf.len = len;
    wsabuf.buf = buf;

    if ( WSARecvFrom (s, &wsabuf, 1, &n, &dwFlags, from, fromlen, NULL, NULL) == SOCKET_ERROR )
        return SOCKET_ERROR;
    else
        return n;
}

/***********************************************************************
 *              recvfrom		(WINSOCK.17)
 */
INT16 WINAPI WINSOCK_recvfrom16(SOCKET16 s, char *buf, INT16 len, INT16 flags,
                                struct WS_sockaddr *from, INT16 *fromlen16)
{
    INT fromlen32;
    INT *p = &fromlen32;
    INT retVal;

    if( fromlen16 ) fromlen32 = *fromlen16; else p = NULL;
    retVal = WS_recvfrom( s, buf, len, flags, from, p );
    if( fromlen16 ) *fromlen16 = fromlen32;
    return (INT16)retVal;
}

/***********************************************************************
 *		__ws_select
 */
static int __ws_select(BOOL b32,
                       void *ws_readfds, void *ws_writefds, void *ws_exceptfds,
                       const struct WS_timeval *ws_timeout)
{
    int         highfd = 0;
    fd_set      readfds, writefds, exceptfds;
    fd_set     *p_read, *p_write, *p_except;
    int         readfd[FD_SETSIZE], writefd[FD_SETSIZE], exceptfd[FD_SETSIZE];
    struct timeval timeout, *timeoutaddr = NULL;

    TRACE("read %p, write %p, excp %p timeout %p\n",
          ws_readfds, ws_writefds, ws_exceptfds, ws_timeout);

    p_read = fd_set_import(&readfds, ws_readfds, &highfd, readfd, b32);
    p_write = fd_set_import(&writefds, ws_writefds, &highfd, writefd, b32);
    p_except = fd_set_import(&exceptfds, ws_exceptfds, &highfd, exceptfd, b32);
    if (ws_timeout)
    {
        timeoutaddr = &timeout;
        timeout.tv_sec=ws_timeout->tv_sec;
        timeout.tv_usec=ws_timeout->tv_usec;
    }

    if( (highfd = select(highfd + 1, p_read, p_write, p_except, timeoutaddr)) > 0 )
    {
        fd_set_export(&readfds, p_except, ws_readfds, readfd, b32);
        fd_set_export(&writefds, p_except, ws_writefds, writefd, b32);

        if (p_except && ws_exceptfds)
        {
#define wsfds16 ((ws_fd_set16*)ws_exceptfds)
#define wsfds32 ((WS_fd_set*)ws_exceptfds)
            int i, j, count = (b32) ? wsfds32->fd_count : wsfds16->fd_count;

            for (i = j = 0; i < count; i++)
            {
                int fd = exceptfd[i];
                if( fd >= 0 && FD_ISSET(fd, &exceptfds) )
                {
                    if( b32 )
                        wsfds32->fd_array[j++] = wsfds32->fd_array[i];
                    else
                        wsfds16->fd_array[j++] = wsfds16->fd_array[i];
                }
                if( fd >= 0 ) close(fd);
                exceptfd[i] = -1;
            }
            if( b32 )
                wsfds32->fd_count = j;
            else
                wsfds16->fd_count = j;
#undef wsfds32
#undef wsfds16
        }
        return highfd;
    }
    fd_set_unimport(ws_readfds, readfd, b32);
    fd_set_unimport(ws_writefds, writefd, b32);
    fd_set_unimport(ws_exceptfds, exceptfd, b32);
    if( ws_readfds ) ((WS_fd_set*)ws_readfds)->fd_count = 0;
    if( ws_writefds ) ((WS_fd_set*)ws_writefds)->fd_count = 0;
    if( ws_exceptfds ) ((WS_fd_set*)ws_exceptfds)->fd_count = 0;

    if( highfd == 0 ) return 0;
    SetLastError(wsaErrno());
    return SOCKET_ERROR;
}

/***********************************************************************
 *		select			(WINSOCK.18)
 */
INT16 WINAPI WINSOCK_select16(INT16 nfds, ws_fd_set16 *ws_readfds,
                              ws_fd_set16 *ws_writefds, ws_fd_set16 *ws_exceptfds,
                              struct WS_timeval* timeout)
{
    return (INT16)__ws_select( FALSE, ws_readfds, ws_writefds, ws_exceptfds, timeout );
}

/***********************************************************************
 *		select			(WS2_32.18)
 */
int WINAPI WS_select(int nfds, WS_fd_set *ws_readfds,
                     WS_fd_set *ws_writefds, WS_fd_set *ws_exceptfds,
                     const struct WS_timeval* timeout)
{
    /* struct timeval is the same for both 32- and 16-bit code */
    return (INT)__ws_select( TRUE, ws_readfds, ws_writefds, ws_exceptfds, timeout );
}


/***********************************************************************
 *		send			(WS2_32.19)
 */
int WINAPI WS_send(SOCKET s, const char *buf, int len, int flags)
{
    DWORD n;
    WSABUF wsabuf;

    wsabuf.len = len;
    wsabuf.buf = (char*) buf;

    if ( WSASendTo ( s, &wsabuf, 1, &n, flags, NULL, 0, NULL, NULL) == SOCKET_ERROR )
        return SOCKET_ERROR;
    else
        return n;
}

/***********************************************************************
 *		WSASend			(WS2_32.72)
 */
INT WINAPI WSASend( SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                    LPDWORD lpNumberOfBytesSent, DWORD dwFlags,
                    LPWSAOVERLAPPED lpOverlapped,
                    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine )
{
    return WSASendTo ( s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags,
                       NULL, 0, lpOverlapped, lpCompletionRoutine );
}

/***********************************************************************
 *              WSASendDisconnect       (WS2_32.73)
 */
INT WINAPI WSASendDisconnect( SOCKET s, LPWSABUF lpBuffers )
{
    return WS_shutdown ( s, SD_SEND );
}


/***********************************************************************
 *		WSASendTo		(WS2_32.74)
 */
INT WINAPI WSASendTo( SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                      LPDWORD lpNumberOfBytesSent, DWORD dwFlags,
                      const struct WS_sockaddr *to, int tolen,
                      LPWSAOVERLAPPED lpOverlapped,
                      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine )
{
    int i, n, fd, err = WSAENOTSOCK, flags, ret;
    struct iovec* iovec;
    struct ws2_async *wsa;
    enum fd_type type;

    TRACE ("socket %04x, wsabuf %p, nbufs %ld, flags %ld, to %p, tolen %d, ovl %p, func %p\n",
           s, lpBuffers, dwBufferCount, dwFlags,
           to, tolen, lpOverlapped, lpCompletionRoutine);

    fd = _get_sock_fd_type( s, GENERIC_WRITE, &type, &flags );
    TRACE ( "fd=%d, type=%d, flags=%x\n", fd, type, flags );

    if ( fd == -1 )
    {
        err = WSAGetLastError ();
        goto error;
    }

    iovec = HeapAlloc (GetProcessHeap(), 0, dwBufferCount * sizeof (struct iovec) );

    if ( !iovec )
    {
        err = WSAEFAULT;
        goto err_close;
    }

    for ( i = 0; i < dwBufferCount; i++ )
    {
        iovec[i].iov_base = lpBuffers[i].buf;
        iovec[i].iov_len  = lpBuffers[i].len;
    }

    if ( (lpOverlapped || lpCompletionRoutine) && flags & FD_FLAG_OVERLAPPED )
    {
        wsa = WS2_make_async ( s, fd, ASYNC_TYPE_WRITE, iovec, dwBufferCount,
                               &dwFlags, (struct WS_sockaddr*) to, &tolen,
                               lpOverlapped, lpCompletionRoutine );
        if ( !wsa )
        {
            err = WSAEFAULT;
            goto err_free;
        }

        if ( ( ret = register_new_async ( &wsa->async )) )
        {
            err = NtStatusToWSAError ( ret );

            if ( !lpOverlapped )
                HeapFree ( GetProcessHeap(), 0, wsa->overlapped );
            HeapFree ( GetProcessHeap(), 0, wsa );
            goto err_free;
        }

        /* Try immediate completion */
        if ( lpOverlapped && !NtResetEvent( lpOverlapped->hEvent, NULL ) )
        {
            if  ( WSAGetOverlappedResult ( s, lpOverlapped,
                                           lpNumberOfBytesSent, FALSE, &dwFlags) )
                return 0;

            if ( (err = WSAGetLastError ()) != WSA_IO_INCOMPLETE )
                goto error;
        }

        WSASetLastError ( WSA_IO_PENDING );
        return SOCKET_ERROR;
    }

    if (_is_blocking(s))
    {
        /* FIXME: exceptfds? */
        do_block(fd, 2);
    }

    n = WS2_send ( fd, iovec, dwBufferCount, to, tolen, dwFlags );
    if ( n == -1 )
    {
        err = wsaErrno();
        if ( err == WSAEWOULDBLOCK )
            _enable_event (SOCKET2HANDLE(s), FD_WRITE, 0, 0);
        goto err_free;
    }

    TRACE(" -> %i bytes\n", n);
    *lpNumberOfBytesSent = n;

    HeapFree ( GetProcessHeap(), 0, iovec );
    close ( fd );
    return 0;

err_free:
    HeapFree ( GetProcessHeap(), 0, iovec );

err_close:
    close ( fd );

error:
    WARN (" -> ERROR %d\n", err);
    WSASetLastError (err);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              send			(WINSOCK.19)
 */
INT16 WINAPI WINSOCK_send16(SOCKET16 s, char *buf, INT16 len, INT16 flags)
{
    return WS_send( s, buf, len, flags );
}

/***********************************************************************
 *		sendto		(WS2_32.20)
 */
int WINAPI WS_sendto(SOCKET s, const char *buf, int len, int flags,
                              const struct WS_sockaddr *to, int tolen)
{
    DWORD n;
    WSABUF wsabuf;

    wsabuf.len = len;
    wsabuf.buf = (char*) buf;

    if ( WSASendTo (s, &wsabuf, 1, &n, flags, to, tolen, NULL, NULL) == SOCKET_ERROR )
        return SOCKET_ERROR;
    else
        return n;
}

/***********************************************************************
 *              sendto		(WINSOCK.20)
 */
INT16 WINAPI WINSOCK_sendto16(SOCKET16 s, char *buf, INT16 len, INT16 flags,
                              struct WS_sockaddr *to, INT16 tolen)
{
    return (INT16)WS_sendto( s, buf, len, flags, to, tolen );
}

/***********************************************************************
 *		setsockopt		(WS2_32.21)
 */
int WINAPI WS_setsockopt(SOCKET s, int level, int optname,
                                  const char *optval, int optlen)
{
    int fd;

    TRACE("socket: %04x, level 0x%x, name 0x%x, ptr %8x, len %d\n", s, level,
          (int) optname, (int) optval, optlen);
    /* SO_OPENTYPE does not require a valid socket handle. */
    if (level == WS_SOL_SOCKET && optname == WS_SO_OPENTYPE)
    {
        if (optlen < sizeof(int) || !optval)
        {
            SetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }
        TlsSetValue( opentype_tls_index, (LPVOID)*(int *)optval );
        TRACE("setting global SO_OPENTYPE to 0x%x\n", *(int *)optval );
        return 0;
    }

    /* For some reason the game GrandPrixLegends does set SO_DONTROUTE on its
     * socket. This will either not happen under windows or it is ignored in
     * windows (but it works in linux and therefor prevents the game to find
     * games outsite the current network) */
    if ( level==WS_SOL_SOCKET && optname==WS_SO_DONTROUTE ) 
    {
        FIXME("Does windows ignore SO_DONTROUTE?\n");
        return 0;
    }


    fd = _get_sock_fd(s);
    if (fd != -1)
    {
	struct	linger linger;
        int woptval;
        struct timeval tval;

	/* Is a privileged and useless operation, so we don't. */
	if ((optname == WS_SO_DEBUG) && (level == WS_SOL_SOCKET)) {
	    FIXME("(%d,SOL_SOCKET,SO_DEBUG,%p(%ld)) attempted (is privileged). Ignoring.\n",s,optval,*(DWORD*)optval);
	    return 0;
	}

        if(optname == WS_SO_DONTLINGER && level == WS_SOL_SOCKET) {
	    /* This is unique to WinSock and takes special conversion */
            linger.l_onoff	= *((int*)optval) ? 0: 1;
            linger.l_linger	= 0;
            optname=SO_LINGER;
            optval = (char*)&linger;
            optlen = sizeof(struct linger);
            level = SOL_SOCKET;
        }else{
            if (!convert_sockopt(&level, &optname)) {
	        ERR("Invalid level (%d) or optname (%d)\n", level, optname);
		SetLastError(WSAENOPROTOOPT);
		close(fd);
		return SOCKET_ERROR;
	    }
            if (optname == SO_LINGER && optval) {
                /* yes, uses unsigned short in both win16/win32 */
                linger.l_onoff	= ((UINT16*)optval)[0];
                linger.l_linger	= ((UINT16*)optval)[1];
                /* FIXME: what is documented behavior if SO_LINGER optval
                   is null?? */
                optval = (char*)&linger;
                optlen = sizeof(struct linger);
            } else if (optlen < sizeof(int)){
                woptval= *((INT16 *) optval);
                optval= (char*) &woptval;
                optlen=sizeof(int);
            }
#ifdef SO_RCVTIMEO
           if (level == SOL_SOCKET && optname == SO_RCVTIMEO) {
               if (optlen == sizeof(UINT32)) {
                   /* WinSock passes miliseconds instead of struct timeval */
                   tval.tv_usec = *(PUINT32)optval % 1000;
                   tval.tv_sec = *(PUINT32)optval / 1000;
                   /* min of 500 milisec */
                   if (tval.tv_sec == 0 && tval.tv_usec < 500) tval.tv_usec = 500;
	            optlen = sizeof(struct timeval);
	            optval = (char*)&tval;
               } else if (optlen == sizeof(struct timeval)) {
                   WARN("SO_RCVTIMEO for %d bytes: assuming unixism\n", optlen);
		} else {
                   WARN("SO_RCVTIMEO for %d bytes is weird: ignored\n", optlen);
		    close(fd);
		    return 0;
		}
	    }
#endif
#ifdef SO_SNDTIMEO
           if (level == SOL_SOCKET && optname == SO_SNDTIMEO) {
               if (optlen == sizeof(UINT32)) {
                   /* WinSock passes miliseconds instead of struct timeval */
                   tval.tv_usec = *(PUINT32)optval % 1000;
                   tval.tv_sec = *(PUINT32)optval / 1000;
                   /* min of 500 milisec */
                   if (tval.tv_sec == 0 && tval.tv_usec < 500) tval.tv_usec = 500;
                   optlen = sizeof(struct timeval);
                   optval = (char*)&tval;
               } else if (optlen == sizeof(struct timeval)) {
                   WARN("SO_SNDTIMEO for %d bytes: assuming unixism\n", optlen);
               } else {
                   WARN("SO_SNDTIMEO for %d bytes is weird: ignored\n", optlen);
                   close(fd);
                   return 0;
               }
           }
#endif
	}
        if(optname == SO_RCVBUF && *(int*)optval < 2048) {
            WARN("SO_RCVBF for %d bytes is too small: ignored\n", *(int*)optval );
            close( fd);
            return 0;
        }

	if (setsockopt(fd, level, optname, optval, optlen) == 0)
	{
	    close(fd);
	    return 0;
	}
	TRACE("Setting socket error, %d\n", wsaErrno());
	SetLastError(wsaErrno());
	close(fd);
    }
    else SetLastError(WSAENOTSOCK);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              setsockopt		(WINSOCK.21)
 */
INT16 WINAPI WINSOCK_setsockopt16(SOCKET16 s, INT16 level, INT16 optname,
                                  char *optval, INT16 optlen)
{
    if( !optval ) return SOCKET_ERROR;
    return (INT16)WS_setsockopt( s, (UINT16)level, optname, optval, optlen );
}


/***********************************************************************
 *		shutdown		(WS2_32.22)
 */
int WINAPI WS_shutdown(SOCKET s, int how)
{
    int fd, fd0 = -1, fd1 = -1, flags, err = WSAENOTSOCK;
    enum fd_type type;
    unsigned int clear_flags = 0;

    fd = _get_sock_fd_type ( s, 0, &type, &flags );
    TRACE("socket %04x, how %i %d %d \n", s, how, type, flags );

    if (fd == -1)
        return SOCKET_ERROR;

    switch( how )
    {
    case 0: /* drop receives */
        clear_flags |= FD_READ;
        break;
    case 1: /* drop sends */
        clear_flags |= FD_WRITE;
        break;
    case 2: /* drop all */
        clear_flags |= FD_READ|FD_WRITE;
    default:
        clear_flags |= FD_WINE_LISTENING;
    }

    if ( flags & FD_FLAG_OVERLAPPED ) {

        switch ( how )
        {
        case SD_RECEIVE:
            fd0 = fd;
            break;
        case SD_SEND:
            fd1 = fd;
            break;
        case SD_BOTH:
        default:
            fd0 = fd;
            fd1 = _get_sock_fd ( s );
        }

        if ( fd0 != -1 )
        {
            err = WS2_register_async_shutdown ( s, fd0, ASYNC_TYPE_READ );
            if ( err )
            {
                close ( fd0 );
                goto error;
            }
        }
        if ( fd1 != -1 )
        {
            err = WS2_register_async_shutdown ( s, fd1, ASYNC_TYPE_WRITE );
            if ( err )
            {
                close ( fd1 );
                goto error;
            }
        }
    }
    else /* non-overlapped mode */
    {
        if ( shutdown( fd, how ) )
        {
            err = wsaErrno ();
            close ( fd );
            goto error;
        }
        close(fd);
    }

    _enable_event( SOCKET2HANDLE(s), 0, 0, clear_flags );
    if ( how > 1) WSAAsyncSelect( s, 0, 0, 0 );
    return 0;

error:
    _enable_event( SOCKET2HANDLE(s), 0, 0, clear_flags );
    WSASetLastError ( err );
    return SOCKET_ERROR;
}

/***********************************************************************
 *              shutdown		(WINSOCK.22)
 */
INT16 WINAPI WINSOCK_shutdown16(SOCKET16 s, INT16 how)
{
    return (INT16)WS_shutdown( s, how );
}


/***********************************************************************
 *		socket		(WS2_32.23)
 */
SOCKET WINAPI WS_socket(int af, int type, int protocol)
{
    TRACE("af=%d type=%d protocol=%d\n", af, type, protocol);

    return WSASocketA ( af, type, protocol, NULL, 0,
                        (TlsGetValue(opentype_tls_index) ? 0 : WSA_FLAG_OVERLAPPED) );
}

/***********************************************************************
 *              socket		(WINSOCK.23)
 */
SOCKET16 WINAPI WINSOCK_socket16(INT16 af, INT16 type, INT16 protocol)
{
    return (SOCKET16)WS_socket( af, type, protocol );
}


/* ----------------------------------- DNS services
 *
 * IMPORTANT: 16-bit API structures have SEGPTR pointers inside them.
 * Also, we have to use wsock32 stubs to convert structures and
 * error codes from Unix to WSA, hence there is no direct mapping in
 * the relay32/wsock32.spec.
 */


/***********************************************************************
 *		__ws_gethostbyaddr
 */
static WIN_hostent* __ws_gethostbyaddr(const char *addr, int len, int type, int dup_flag)
{
    WIN_hostent *retval = NULL;

    struct hostent* host;
#if HAVE_LINUX_GETHOSTBYNAME_R_6
    char *extrabuf;
    int ebufsize=1024;
    struct hostent hostentry;
    int locerr=ENOBUFS;
    host = NULL;
    extrabuf=HeapAlloc(GetProcessHeap(),0,ebufsize) ;
    while(extrabuf) {
        int res = gethostbyaddr_r(addr, len, type,
                                  &hostentry, extrabuf, ebufsize, &host, &locerr);
        if( res != ERANGE) break;
        ebufsize *=2;
        extrabuf=HeapReAlloc(GetProcessHeap(),0,extrabuf,ebufsize) ;
    }
    if (!host) SetLastError((locerr < 0) ? wsaErrno() : wsaHerrno(locerr));
#else
    EnterCriticalSection( &csWSgetXXXbyYYY );
    host = gethostbyaddr(addr, len, type);
    if (!host) SetLastError((h_errno < 0) ? wsaErrno() : wsaHerrno(h_errno));
#endif
    if( host != NULL )
    {
        if( WS_dup_he(host, dup_flag) )
            retval = he_buffer;
        else
            SetLastError(WSAENOBUFS);
    }
#ifdef  HAVE_LINUX_GETHOSTBYNAME_R_6
    HeapFree(GetProcessHeap(),0,extrabuf);
#else
    LeaveCriticalSection( &csWSgetXXXbyYYY );
#endif
    return retval;
}

/***********************************************************************
 *		gethostbyaddr		(WINSOCK.51)
 */
SEGPTR WINAPI WINSOCK_gethostbyaddr16(const char *addr, INT16 len, INT16 type)
{
    TRACE("ptr %p, len %d, type %d\n", addr, len, type);
    if (!__ws_gethostbyaddr( addr, len, type, WS_DUP_SEGPTR )) return 0;
    return he_buffer_seg;
}

/***********************************************************************
 *		gethostbyaddr		(WS2_32.51)
 */
struct WS_hostent* WINAPI WS_gethostbyaddr(const char *addr, int len,
                                                int type)
{
    TRACE("ptr %08x, len %d, type %d\n",
                             (unsigned) addr, len, type);
    return __ws_gethostbyaddr(addr, len, type, WS_DUP_LINEAR);
}

/***********************************************************************
 *		__ws_gethostbyname
 */
static WIN_hostent * __ws_gethostbyname(const char *name, int dup_flag)
{
    WIN_hostent *retval = NULL;
    struct hostent*     host;
#ifdef  HAVE_LINUX_GETHOSTBYNAME_R_6
    char *extrabuf;
    int ebufsize=1024;
    struct hostent hostentry;
    int locerr = ENOBUFS;
    host = NULL;
    extrabuf=HeapAlloc(GetProcessHeap(),0,ebufsize) ;
    while(extrabuf) {
        int res = gethostbyname_r(name, &hostentry, extrabuf, ebufsize, &host, &locerr);
        if( res != ERANGE) break;
        ebufsize *=2;
        extrabuf=HeapReAlloc(GetProcessHeap(),0,extrabuf,ebufsize) ;
    }
    if (!host) SetLastError((locerr < 0) ? wsaErrno() : wsaHerrno(locerr));
#else
    EnterCriticalSection( &csWSgetXXXbyYYY );
    host = gethostbyname(name);
    if (!host) SetLastError((h_errno < 0) ? wsaErrno() : wsaHerrno(h_errno));
#endif
    if( host  != NULL )
    {
        if( WS_dup_he(host, dup_flag) )
            retval = he_buffer;
        else SetLastError(WSAENOBUFS);
    }
#ifdef  HAVE_LINUX_GETHOSTBYNAME_R_6
    HeapFree(GetProcessHeap(),0,extrabuf);
#else
    LeaveCriticalSection( &csWSgetXXXbyYYY );
#endif
    return retval;
}

/***********************************************************************
 *		gethostbyname		(WINSOCK.52)
 */
SEGPTR WINAPI WINSOCK_gethostbyname16(const char *name)
{
    TRACE( "%s\n", debugstr_a(name) );
    if (!__ws_gethostbyname( name, WS_DUP_SEGPTR )) return 0;
    return he_buffer_seg;
}

/***********************************************************************
 *		gethostbyname		(WS2_32.52)
 */
struct WS_hostent* WINAPI WS_gethostbyname(const char* name)
{
    TRACE( "%s\n", debugstr_a(name) );
    return __ws_gethostbyname( name, WS_DUP_LINEAR );
}


/***********************************************************************
 *		__ws_getprotobyname
 */
static WIN_protoent* __ws_getprotobyname(const char *name, int dup_flag)
{
    WIN_protoent* retval = NULL;
#ifdef HAVE_GETPROTOBYNAME
    struct protoent*     proto;
    EnterCriticalSection( &csWSgetXXXbyYYY );
    if( (proto = getprotobyname(name)) != NULL )
    {
        if( WS_dup_pe(proto, dup_flag) )
            retval = pe_buffer;
        else SetLastError(WSAENOBUFS);
    }
    else {
        MESSAGE("protocol %s not found; You might want to add "
                "this to /etc/protocols\n", debugstr_a(name) );
        SetLastError(WSANO_DATA);
    }
    LeaveCriticalSection( &csWSgetXXXbyYYY );
#endif
    return retval;
}

/***********************************************************************
 *		getprotobyname		(WINSOCK.53)
 */
SEGPTR WINAPI WINSOCK_getprotobyname16(const char *name)
{
    TRACE( "%s\n", debugstr_a(name) );
    if (!__ws_getprotobyname(name, WS_DUP_SEGPTR)) return 0;
    return pe_buffer_seg;
}

/***********************************************************************
 *		getprotobyname		(WS2_32.53)
 */
struct WS_protoent* WINAPI WS_getprotobyname(const char* name)
{
    TRACE( "%s\n", debugstr_a(name) );
    return __ws_getprotobyname(name, WS_DUP_LINEAR);
}


/***********************************************************************
 *		__ws_getprotobynumber
 */
static WIN_protoent* __ws_getprotobynumber(int number, int dup_flag)
{
    WIN_protoent* retval = NULL;
#ifdef HAVE_GETPROTOBYNUMBER
    struct protoent*     proto;
    EnterCriticalSection( &csWSgetXXXbyYYY );
    if( (proto = getprotobynumber(number)) != NULL )
    {
        if( WS_dup_pe(proto, dup_flag) )
            retval = pe_buffer;
        else SetLastError(WSAENOBUFS);
    }
    else {
        MESSAGE("protocol number %d not found; You might want to add "
                "this to /etc/protocols\n", number );
        SetLastError(WSANO_DATA);
    }
    LeaveCriticalSection( &csWSgetXXXbyYYY );
#endif
    return retval;
}

/***********************************************************************
 *		getprotobynumber	(WINSOCK.54)
 */
SEGPTR WINAPI WINSOCK_getprotobynumber16(INT16 number)
{
    TRACE("%i\n", number);
    if (!__ws_getprotobynumber(number, WS_DUP_SEGPTR)) return 0;
    return pe_buffer_seg;
}

/***********************************************************************
 *		getprotobynumber	(WS2_32.54)
 */
struct WS_protoent* WINAPI WS_getprotobynumber(int number)
{
    TRACE("%i\n", number);
    return __ws_getprotobynumber(number, WS_DUP_LINEAR);
}


/***********************************************************************
 *		__ws_getservbyname
 */
static WIN_servent* __ws_getservbyname(const char *name, const char *proto, int dup_flag)
{
    WIN_servent* retval = NULL;
    struct servent*     serv;
    int i = wsi_strtolo( name, proto );

    if( i ) {
        EnterCriticalSection( &csWSgetXXXbyYYY );
        serv = getservbyname(local_buffer,
                             proto && *proto ? (local_buffer + i) : NULL);
        if( serv != NULL )
        {
            if( WS_dup_se(serv, dup_flag) )
                retval = se_buffer;
            else SetLastError(WSAENOBUFS);
        }
        else {
            MESSAGE("service %s protocol %s not found; You might want to add "
                    "this to /etc/services\n", debugstr_a(local_buffer),
                    proto ? debugstr_a(local_buffer+i):"*");
            SetLastError(WSANO_DATA);
        }
        LeaveCriticalSection( &csWSgetXXXbyYYY );
    }
    else SetLastError(WSAENOBUFS);
    return retval;
}

/***********************************************************************
 *		getservbyname		(WINSOCK.55)
 */
SEGPTR WINAPI WINSOCK_getservbyname16(const char *name, const char *proto)
{
    TRACE( "%s, %s\n", debugstr_a(name), debugstr_a(proto) );
    if (!__ws_getservbyname(name, proto, WS_DUP_SEGPTR)) return 0;
    return se_buffer_seg;
}

/***********************************************************************
 *		getservbyname		(WS2_32.55)
 */
struct WS_servent* WINAPI WS_getservbyname(const char *name, const char *proto)
{
    TRACE( "%s, %s\n", debugstr_a(name), debugstr_a(proto) );
    return __ws_getservbyname(name, proto, WS_DUP_LINEAR);
}


/***********************************************************************
 *		__ws_getservbyport
 */
static WIN_servent* __ws_getservbyport(int port, const char* proto, int dup_flag)
{
    WIN_servent* retval = NULL;
#ifdef HAVE_GETSERVBYPORT
    struct servent*     serv;
    if (!proto || wsi_strtolo( proto, NULL )) {
        EnterCriticalSection( &csWSgetXXXbyYYY );
        if( (serv = getservbyport(port, proto && *proto ? local_buffer :
                                                          NULL)) != NULL ) {
            if( WS_dup_se(serv, dup_flag) )
                retval = se_buffer;
            else SetLastError(WSAENOBUFS);
        }
        else {
            MESSAGE("service on port %lu protocol %s not found; You might want to add "
                    "this to /etc/services\n", (unsigned long)ntohl(port),
                    proto ? debugstr_a(local_buffer) : "*");
            SetLastError(WSANO_DATA);
        }
        LeaveCriticalSection( &csWSgetXXXbyYYY );
    }
    else SetLastError(WSAENOBUFS);
#endif
    return retval;
}

/***********************************************************************
 *		getservbyport		(WINSOCK.56)
 */
SEGPTR WINAPI WINSOCK_getservbyport16(INT16 port, const char *proto)
{
    TRACE("%d (i.e. port %d), %s\n", (int)port, (int)ntohl(port), debugstr_a(proto));
    if (!__ws_getservbyport(port, proto, WS_DUP_SEGPTR)) return 0;
    return se_buffer_seg;
}

/***********************************************************************
 *		getservbyport		(WS2_32.56)
 */
struct WS_servent* WINAPI WS_getservbyport(int port, const char *proto)
{
    TRACE("%d (i.e. port %d), %s\n", (int)port, (int)ntohl(port), debugstr_a(proto));
    return __ws_getservbyport(port, proto, WS_DUP_LINEAR);
}


/***********************************************************************
 *              gethostname           (WS2_32.57)
 */
int WINAPI WS_gethostname(char *name, int namelen)
{
    TRACE("name %p, len %d\n", name, namelen);

    if (gethostname(name, namelen) == 0)
    {
        TRACE("<- '%s'\n", name);
        return 0;
    }
    SetLastError((errno == EINVAL) ? WSAEFAULT : wsaErrno());
    TRACE("<- ERROR !\n");
    return SOCKET_ERROR;
}

/***********************************************************************
 *              gethostname           (WINSOCK.57)
 */
INT16 WINAPI WINSOCK_gethostname16(char *name, INT16 namelen)
{
    return (INT16)WS_gethostname(name, namelen);
}


/* ------------------------------------- Windows sockets extensions -- *
 *								       *
 * ------------------------------------------------------------------- */

/***********************************************************************
 *		WSAEnumNetworkEvents (WS2_32.36)
 */
int WINAPI WSAEnumNetworkEvents(SOCKET s, WSAEVENT hEvent, LPWSANETWORKEVENTS lpEvent)
{
    int ret;

    TRACE("%08x, hEvent %p, lpEvent %08x\n", s, hEvent, (unsigned)lpEvent );

    SERVER_START_REQ( get_socket_event )
    {
        req->handle  = SOCKET2HANDLE(s);
        req->service = TRUE;
        req->c_event = hEvent;
        wine_server_set_reply( req, lpEvent->iErrorCode, sizeof(lpEvent->iErrorCode) );
        if (!(ret = wine_server_call(req))) lpEvent->lNetworkEvents = reply->pmask & reply->mask;
    }
    SERVER_END_REQ;
    if (!ret) return 0;
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/***********************************************************************
 *		WSAEventSelect (WS2_32.39)
 */
int WINAPI WSAEventSelect(SOCKET s, WSAEVENT hEvent, LONG lEvent)
{
    int ret;

    TRACE("%08x, hEvent %p, event %08x\n", s, hEvent, (unsigned)lEvent );

    SERVER_START_REQ( set_socket_event )
    {
        req->handle = SOCKET2HANDLE(s);
        req->mask   = lEvent;
        req->event  = hEvent;
        req->window = 0;
        req->msg    = 0;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (!ret) return 0;
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/**********************************************************************
 *      WSAGetOverlappedResult (WS2_32.40)
 */
BOOL WINAPI WSAGetOverlappedResult ( SOCKET s, LPWSAOVERLAPPED lpOverlapped,
                                     LPDWORD lpcbTransfer, BOOL fWait,
                                     LPDWORD lpdwFlags )
{
    DWORD r;

    TRACE ( "socket %d ovl %p trans %p, wait %d flags %p\n",
            s, lpOverlapped, lpcbTransfer, fWait, lpdwFlags );

    if ( !(lpOverlapped && lpOverlapped->hEvent) )
    {
        ERR ( "Invalid pointer\n" );
        WSASetLastError (WSA_INVALID_PARAMETER);
        return FALSE;
    }

    if ( fWait )
    {
        while ( WaitForSingleObjectEx (lpOverlapped->hEvent, INFINITE, TRUE) == STATUS_USER_APC );
    }
    else if ( lpOverlapped->Internal == STATUS_PENDING )
    {
        /* Wait in order to give APCs a chance to run. */
        /* This is cheating, so we must set the event again in case of success -
           it may be a non-manual reset event. */
        while ( (r = WaitForSingleObjectEx (lpOverlapped->hEvent, 0, TRUE)) == STATUS_USER_APC );
        if ( r == WAIT_OBJECT_0 )
            NtSetEvent ( lpOverlapped->hEvent, NULL );
    }

    if ( lpcbTransfer )
        *lpcbTransfer = lpOverlapped->InternalHigh;

    if ( lpdwFlags )
        *lpdwFlags = lpOverlapped->Offset;

    switch ( lpOverlapped->Internal )
    {
    case STATUS_SUCCESS:
        return TRUE;
    case STATUS_PENDING:
        WSASetLastError ( WSA_IO_INCOMPLETE );
        if (fWait) ERR ("PENDING status after waiting!\n");
        return FALSE;
    default:
        WSASetLastError ( NtStatusToWSAError ( lpOverlapped->Internal ));
        return FALSE;
    }
}


/***********************************************************************
 *      WSAAsyncSelect			(WS2_32.101)
 */
INT WINAPI WSAAsyncSelect(SOCKET s, HWND hWnd, UINT uMsg, LONG lEvent)
{
    int ret;

    TRACE("%x, hWnd %p, uMsg %08x, event %08lx\n", s, hWnd, uMsg, lEvent );

    SERVER_START_REQ( set_socket_event )
    {
        req->handle = SOCKET2HANDLE(s);
        req->mask   = lEvent;
        req->event  = 0;
        req->window = hWnd;
        req->msg    = uMsg;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (!ret) return 0;
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/***********************************************************************
 *      WSAAsyncSelect		(WINSOCK.101)
 */
INT16 WINAPI WSAAsyncSelect16(SOCKET16 s, HWND16 hWnd, UINT16 wMsg, LONG lEvent)
{
    return (INT16)WSAAsyncSelect( s, HWND_32(hWnd), wMsg, lEvent );
}

/***********************************************************************
 *              WSARecvEx			(WINSOCK.1107)
 *
 * See description for WSARecvEx()
 */
INT16     WINAPI WSARecvEx16(SOCKET16 s, char *buf, INT16 len, INT16 *flags)
{
  FIXME("(WSARecvEx16) partial packet return value not set \n");

  return WINSOCK_recv16(s, buf, len, *flags);
}


/***********************************************************************
 *      WSACreateEvent          (WS2_32.31)
 *
 */
WSAEVENT WINAPI WSACreateEvent(void)
{
    /* Create a manual-reset event, with initial state: unsignealed */
    TRACE("\n");

    return CreateEventA(NULL, TRUE, FALSE, NULL);
}

/***********************************************************************
 *      WSACloseEvent          (WS2_32.29)
 *
 */
BOOL WINAPI WSACloseEvent(WSAEVENT event)
{
    TRACE ("event=%p\n", event);

    return CloseHandle(event);
}

/***********************************************************************
 *      WSASocketA          (WS2_32.78)
 *
 */
SOCKET WINAPI WSASocketA(int af, int type, int protocol,
                         LPWSAPROTOCOL_INFOA lpProtocolInfo,
                         GROUP g, DWORD dwFlags)
{
    SOCKET ret;

   /*
      FIXME: The "advanced" parameters of WSASocketA (lpProtocolInfo,
      g, dwFlags except WSA_FLAG_OVERLAPPED) are ignored.
   */

   TRACE("af=%d type=%d protocol=%d protocol_info=%p group=%d flags=0x%lx\n",
         af, type, protocol, lpProtocolInfo, g, dwFlags );

    /* hack for WSADuplicateSocket */
    if (lpProtocolInfo && lpProtocolInfo->dwServiceFlags4 == 0xff00ff00) {
      ret = lpProtocolInfo->dwCatalogEntryId;
      TRACE("\tgot duplicate %04x\n", ret);
      return ret;
    }

    /* check the socket family */
    switch(af)
    {
#ifdef HAVE_IPX
        case WS_AF_IPX: af = AF_IPX;
#endif
        case AF_INET:
        case AF_UNSPEC:
            break;
        default:
            SetLastError(WSAEAFNOSUPPORT);
            return INVALID_SOCKET;
    }

    /* check the socket type */
    switch(type)
    {
        case WS_SOCK_STREAM:
            type=SOCK_STREAM;
            break;
        case WS_SOCK_DGRAM:
            type=SOCK_DGRAM;
            break;
        case WS_SOCK_RAW:
            type=SOCK_RAW;
            break;
        default:
            SetLastError(WSAESOCKTNOSUPPORT);
            return INVALID_SOCKET;
    }

    /* check the protocol type */
    if ( protocol < 0 )  /* don't support negative values */
    {
        SetLastError(WSAEPROTONOSUPPORT);
        return INVALID_SOCKET;
    }

    if ( af == AF_UNSPEC)  /* did they not specify the address family? */
        switch(protocol)
	{
          case IPPROTO_TCP:
             if (type == SOCK_STREAM) { af = AF_INET; break; }
          case IPPROTO_UDP:
             if (type == SOCK_DGRAM)  { af = AF_INET; break; }
          default: SetLastError(WSAEPROTOTYPE); return INVALID_SOCKET;
        }

    SERVER_START_REQ( create_socket )
    {
        req->family   = af;
        req->type     = type;
        req->protocol = protocol;
        req->access   = GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE;
        req->flags    = dwFlags;
        req->inherit  = TRUE;
        set_error( wine_server_call( req ) );
        ret = HANDLE2SOCKET( reply->handle );
    }
    SERVER_END_REQ;
    if (ret)
    {
        TRACE("\tcreated %04x\n", ret );
        return ret;
    }

    if (GetLastError() == WSAEACCES) /* raw socket denied */
    {
        if (type == SOCK_RAW)
            MESSAGE("WARNING: Trying to create a socket of type SOCK_RAW, will fail unless running as root\n");
        else
            MESSAGE("WS_SOCKET: not enough privileges to create socket, try running as root\n");
        SetLastError(WSAESOCKTNOSUPPORT);
    }

    WARN("\t\tfailed!\n");
    return INVALID_SOCKET;
}


/***********************************************************************
 *	__WSAFDIsSet			(WINSOCK.151)
 */
INT16 WINAPI __WSAFDIsSet16(SOCKET16 s, ws_fd_set16 *set)
{
  int i = set->fd_count;

  TRACE("(%d,%8lx(%i))\n", s,(unsigned long)set, i);

  while (i--)
      if (set->fd_array[i] == s) return 1;
  return 0;
}

/***********************************************************************
 *      __WSAFDIsSet			(WS2_32.151)
 */
int WINAPI __WSAFDIsSet(SOCKET s, WS_fd_set *set)
{
  int i = set->fd_count;

  TRACE("(%d,%8lx(%i))\n", s,(unsigned long)set, i);

  while (i--)
      if (set->fd_array[i] == s) return 1;
  return 0;
}

/***********************************************************************
 *      WSAIsBlocking			(WINSOCK.114)
 *      WSAIsBlocking			(WS2_32.114)
 */
BOOL WINAPI WSAIsBlocking(void)
{
  /* By default WinSock should set all its sockets to non-blocking mode
   * and poll in PeekMessage loop when processing "blocking" ones. This
   * function is supposed to tell if the program is in this loop. Our
   * blocking calls are truly blocking so we always return FALSE.
   *
   * Note: It is allowed to call this function without prior WSAStartup().
   */

  TRACE("\n");
  return FALSE;
}

/***********************************************************************
 *      WSACancelBlockingCall		(WINSOCK.113)
 *      WSACancelBlockingCall		(WS2_32.113)
 */
INT WINAPI WSACancelBlockingCall(void)
{
    TRACE("\n");
    return 0;
}


/***********************************************************************
 *      WSASetBlockingHook		(WINSOCK.109)
 */
FARPROC16 WINAPI WSASetBlockingHook16(FARPROC16 lpBlockFunc)
{
  FARPROC16 prev = (FARPROC16)blocking_hook;
  blocking_hook = (FARPROC)lpBlockFunc;
  TRACE("hook %p\n", lpBlockFunc);
  return prev;
}


/***********************************************************************
 *      WSASetBlockingHook (WS2_32.109)
 */
FARPROC WINAPI WSASetBlockingHook(FARPROC lpBlockFunc)
{
  FARPROC prev = blocking_hook;
  blocking_hook = lpBlockFunc;
  TRACE("hook %p\n", lpBlockFunc);
  return prev;
}


/***********************************************************************
 *      WSAUnhookBlockingHook	(WINSOCK.110)
 */
INT16 WINAPI WSAUnhookBlockingHook16(void)
{
    blocking_hook = NULL;
    return 0;
}


/***********************************************************************
 *      WSAUnhookBlockingHook (WS2_32.110)
 */
INT WINAPI WSAUnhookBlockingHook(void)
{
    blocking_hook = NULL;
    return 0;
}


/* ----------------------------------- end of API stuff */

/* ----------------------------------- helper functions -
 *
 * TODO: Merge WS_dup_..() stuff into one function that
 * would operate with a generic structure containing internal
 * pointers (via a template of some kind).
 */

static int list_size(char** l, int item_size)
{
  int i,j = 0;
  if(l)
  { for(i=0;l[i];i++)
	j += (item_size) ? item_size : strlen(l[i]) + 1;
    j += (i + 1) * sizeof(char*); }
  return j;
}

static int list_dup(char** l_src, char* ref, char* base, int item_size)
{
   /* base is either either equal to ref or 0 or SEGPTR */

   char*		p = ref;
   char**		l_to = (char**)ref;
   int			i,j,k;

   for(j=0;l_src[j];j++) ;
   p += (j + 1) * sizeof(char*);
   for(i=0;i<j;i++)
   { l_to[i] = base + (p - ref);
     k = ( item_size ) ? item_size : strlen(l_src[i]) + 1;
     memcpy(p, l_src[i], k); p += k; }
   l_to[i] = NULL;
   return (p - ref);
}

/* ----- hostent */

static int hostent_size(struct hostent* p_he)
{
  int size = 0;
  if( p_he )
  { size  = sizeof(struct hostent);
    size += strlen(p_he->h_name) + 1;
    size += list_size(p_he->h_aliases, 0);
    size += list_size(p_he->h_addr_list, p_he->h_length ); }
  return size;
}

/* duplicate hostent entry
 * and handle all Win16/Win32 dependent things (struct size, ...) *correctly*.
 * Dito for protoent and servent.
 */
static int WS_dup_he(struct hostent* p_he, int flag)
{
    /* Convert hostent structure into ws_hostent so that the data fits
     * into local_buffer. Internal pointers can be linear, SEGPTR, or
     * relative to local_buffer depending on "flag" value. Returns size
     * of the data copied.
     */

    int size = hostent_size(p_he);
    if( size )
    {
	char *p_name,*p_aliases,*p_addr,*p_base,*p;
	char *p_to;
	struct ws_hostent16 *p_to16;
	struct WS_hostent *p_to32;

	check_buffer_he(size);
	p_to = he_buffer;
	p_to16 = he_buffer;
	p_to32 = he_buffer;

	p = p_to;
	p_base = (flag & WS_DUP_SEGPTR) ? (char*)he_buffer_seg : he_buffer;
	p += (flag & WS_DUP_SEGPTR) ?
	    sizeof(struct ws_hostent16) : sizeof(struct WS_hostent);
	p_name = p;
	strcpy(p, p_he->h_name); p += strlen(p) + 1;
	p_aliases = p;
	p += list_dup(p_he->h_aliases, p, p_base + (p - p_to), 0);
	p_addr = p;
	list_dup(p_he->h_addr_list, p, p_base + (p - p_to), p_he->h_length);

	if (flag & WS_DUP_SEGPTR) /* Win16 */
	{
	    p_to16->h_addrtype = (INT16)p_he->h_addrtype;
	    p_to16->h_length = (INT16)p_he->h_length;
	    p_to16->h_name = (SEGPTR)(p_base + (p_name - p_to));
	    p_to16->h_aliases = (SEGPTR)(p_base + (p_aliases - p_to));
	    p_to16->h_addr_list = (SEGPTR)(p_base + (p_addr - p_to));
	    size += (sizeof(struct ws_hostent16) - sizeof(struct hostent));
	}
	else /* Win32 */
	{
	    p_to32->h_addrtype = p_he->h_addrtype;
	    p_to32->h_length = p_he->h_length;
	    p_to32->h_name = (p_base + (p_name - p_to));
	    p_to32->h_aliases = (char **)(p_base + (p_aliases - p_to));
	    p_to32->h_addr_list = (char **)(p_base + (p_addr - p_to));
	    size += (sizeof(struct WS_hostent) - sizeof(struct hostent));
	}
    }
    return size;
}

/* ----- protoent */

static int protoent_size(struct protoent* p_pe)
{
  int size = 0;
  if( p_pe )
  { size  = sizeof(struct protoent);
    size += strlen(p_pe->p_name) + 1;
    size += list_size(p_pe->p_aliases, 0); }
  return size;
}

static int WS_dup_pe(struct protoent* p_pe, int flag)
{
    int size = protoent_size(p_pe);
    if( size )
    {
	char *p_to;
	struct ws_protoent16 *p_to16;
	struct WS_protoent *p_to32;
	char *p_name,*p_aliases,*p_base,*p;

	check_buffer_pe(size);
	p_to = pe_buffer;
	p_to16 = pe_buffer;
	p_to32 = pe_buffer;
	p = p_to;
	p_base = (flag & WS_DUP_SEGPTR) ? (char*)pe_buffer_seg : pe_buffer;
	p += (flag & WS_DUP_SEGPTR) ?
	    sizeof(struct ws_protoent16) : sizeof(struct WS_protoent);
	p_name = p;
	strcpy(p, p_pe->p_name); p += strlen(p) + 1;
	p_aliases = p;
	list_dup(p_pe->p_aliases, p, p_base + (p - p_to), 0);

	if (flag & WS_DUP_SEGPTR) /* Win16 */
	{
	    p_to16->p_proto = (INT16)p_pe->p_proto;
	    p_to16->p_name = (SEGPTR)(p_base) + (p_name - p_to);
	    p_to16->p_aliases = (SEGPTR)((p_base) + (p_aliases - p_to));
	    size += (sizeof(struct ws_protoent16) - sizeof(struct protoent));
	}
	else /* Win32 */
	{
	    p_to32->p_proto = p_pe->p_proto;
	    p_to32->p_name = (p_base) + (p_name - p_to);
	    p_to32->p_aliases = (char **)((p_base) + (p_aliases - p_to));
	    size += (sizeof(struct WS_protoent) - sizeof(struct protoent));
	}
    }
    return size;
}

/* ----- servent */

static int servent_size(struct servent* p_se)
{
  int size = 0;
  if( p_se )
  { size += sizeof(struct servent);
    size += strlen(p_se->s_proto) + strlen(p_se->s_name) + 2;
    size += list_size(p_se->s_aliases, 0); }
  return size;
}

static int WS_dup_se(struct servent* p_se, int flag)
{
    int size = servent_size(p_se);
    if( size )
    {
	char *p_name,*p_aliases,*p_proto,*p_base,*p;
	char *p_to;
	struct ws_servent16 *p_to16;
	struct WS_servent *p_to32;

	check_buffer_se(size);
	p_to = se_buffer;
	p_to16 = se_buffer;
	p_to32 = se_buffer;
	p = p_to;
	p_base = (flag & WS_DUP_SEGPTR) ? (char*)se_buffer_seg : se_buffer;
	p += (flag & WS_DUP_SEGPTR) ?
	    sizeof(struct ws_servent16) : sizeof(struct WS_servent);
	p_name = p;
	strcpy(p, p_se->s_name); p += strlen(p) + 1;
	p_proto = p;
	strcpy(p, p_se->s_proto); p += strlen(p) + 1;
	p_aliases = p;
	list_dup(p_se->s_aliases, p, p_base + (p - p_to), 0);

	if (flag & WS_DUP_SEGPTR) /* Win16 */
	{
	    p_to16->s_port = (INT16)p_se->s_port;
	    p_to16->s_name = (SEGPTR)(p_base + (p_name - p_to));
	    p_to16->s_proto = (SEGPTR)(p_base + (p_proto - p_to));
	    p_to16->s_aliases = (SEGPTR)(p_base + (p_aliases - p_to));
	    size += (sizeof(struct ws_servent16) - sizeof(struct servent));
	}
	else /* Win32 */
	{
	    p_to32->s_port = p_se->s_port;
	    p_to32->s_name = (p_base + (p_name - p_to));
	    p_to32->s_proto = (p_base + (p_proto - p_to));
	    p_to32->s_aliases = (char **)(p_base + (p_aliases - p_to));
	    size += (sizeof(struct WS_servent) - sizeof(struct servent));
	}
    }
    return size;
}

/* ----------------------------------- error handling */

UINT16 wsaErrno(void)
{
    int	loc_errno = errno;
    WARN("errno %d, (%s).\n", loc_errno, strerror(loc_errno));

    switch(loc_errno)
    {
	case EINTR:		return WSAEINTR;
	case EBADF:		return WSAEBADF;
	case EPERM:
	case EACCES:		return WSAEACCES;
	case EFAULT:		return WSAEFAULT;
	case EINVAL:		return WSAEINVAL;
	case EMFILE:		return WSAEMFILE;
	case EWOULDBLOCK:	return WSAEWOULDBLOCK;
	case EINPROGRESS:	return WSAEINPROGRESS;
	case EALREADY:		return WSAEALREADY;
	case ENOTSOCK:		return WSAENOTSOCK;
	case EDESTADDRREQ:	return WSAEDESTADDRREQ;
	case EMSGSIZE:		return WSAEMSGSIZE;
	case EPROTOTYPE:	return WSAEPROTOTYPE;
	case ENOPROTOOPT:	return WSAENOPROTOOPT;
	case EPROTONOSUPPORT:	return WSAEPROTONOSUPPORT;
	case ESOCKTNOSUPPORT:	return WSAESOCKTNOSUPPORT;
	case EOPNOTSUPP:	return WSAEOPNOTSUPP;
	case EPFNOSUPPORT:	return WSAEPFNOSUPPORT;
	case EAFNOSUPPORT:	return WSAEAFNOSUPPORT;
	case EADDRINUSE:	return WSAEADDRINUSE;
	case EADDRNOTAVAIL:	return WSAEADDRNOTAVAIL;
	case ENETDOWN:		return WSAENETDOWN;
	case ENETUNREACH:	return WSAENETUNREACH;
	case ENETRESET:		return WSAENETRESET;
	case ECONNABORTED:	return WSAECONNABORTED;
	case EPIPE:
	case ECONNRESET:	return WSAECONNRESET;
	case ENOBUFS:		return WSAENOBUFS;
	case EISCONN:		return WSAEISCONN;
	case ENOTCONN:		return WSAENOTCONN;
	case ESHUTDOWN:		return WSAESHUTDOWN;
	case ETOOMANYREFS:	return WSAETOOMANYREFS;
	case ETIMEDOUT:		return WSAETIMEDOUT;
	case ECONNREFUSED:	return WSAECONNREFUSED;
	case ELOOP:		return WSAELOOP;
	case ENAMETOOLONG:	return WSAENAMETOOLONG;
	case EHOSTDOWN:		return WSAEHOSTDOWN;
	case EHOSTUNREACH:	return WSAEHOSTUNREACH;
	case ENOTEMPTY:		return WSAENOTEMPTY;
#ifdef EPROCLIM
	case EPROCLIM:		return WSAEPROCLIM;
#endif
#ifdef EUSERS
	case EUSERS:		return WSAEUSERS;
#endif
#ifdef EDQUOT
	case EDQUOT:		return WSAEDQUOT;
#endif
#ifdef ESTALE
	case ESTALE:		return WSAESTALE;
#endif
#ifdef EREMOTE
	case EREMOTE:		return WSAEREMOTE;
#endif

       /* just in case we ever get here and there are no problems */
	case 0:			return 0;
        default:
		WARN("Unknown errno %d!\n", loc_errno);
		return WSAEOPNOTSUPP;
    }
}

UINT16 wsaHerrno(int loc_errno)
{

    WARN("h_errno %d.\n", loc_errno);

    switch(loc_errno)
    {
	case HOST_NOT_FOUND:	return WSAHOST_NOT_FOUND;
	case TRY_AGAIN:		return WSATRY_AGAIN;
	case NO_RECOVERY:	return WSANO_RECOVERY;
	case NO_DATA:		return WSANO_DATA;
	case ENOBUFS:		return WSAENOBUFS;

	case 0:			return 0;
        default:
		WARN("Unknown h_errno %d!\n", loc_errno);
		return WSAEOPNOTSUPP;
    }
}


/***********************************************************************
 *		WSARecv			(WS2_32.67)
 */
int WINAPI WSARecv (SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
		    LPDWORD NumberOfBytesReceived, LPDWORD lpFlags,
		    LPWSAOVERLAPPED lpOverlapped,
		    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    return WSARecvFrom (s, lpBuffers, dwBufferCount, NumberOfBytesReceived, lpFlags,
                        NULL, NULL, lpOverlapped, lpCompletionRoutine);
}

/***********************************************************************
 *              WSARecvFrom             (WS2_32.69)
 */
INT WINAPI WSARecvFrom( SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                        LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, struct WS_sockaddr *lpFrom,
                        LPINT lpFromlen, LPWSAOVERLAPPED lpOverlapped,
                        LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine )

{
    int i, n, fd, err = WSAENOTSOCK, flags, ret;
    struct iovec* iovec;
    struct ws2_async *wsa;
    enum fd_type type;

    TRACE("socket %04x, wsabuf %p, nbufs %ld, flags %ld, from %p, fromlen %ld, ovl %p, func %p\n",
          s, lpBuffers, dwBufferCount, *lpFlags, lpFrom,
          (lpFromlen ? *lpFromlen : -1L),
          lpOverlapped, lpCompletionRoutine);

    fd = _get_sock_fd_type( s, GENERIC_READ, &type, &flags );
    TRACE ( "fd=%d, type=%d, flags=%x\n", fd, type, flags );

    if (fd == -1)
    {
        err = WSAGetLastError ();
        goto error;
    }

    iovec = HeapAlloc ( GetProcessHeap(), 0, dwBufferCount * sizeof (struct iovec) );
    if ( !iovec )
    {
        err = WSAEFAULT;
        goto err_close;
    }

    for (i = 0; i < dwBufferCount; i++)
    {
        iovec[i].iov_base = lpBuffers[i].buf;
        iovec[i].iov_len  = lpBuffers[i].len;
    }

    if ( (lpOverlapped || lpCompletionRoutine) && flags & FD_FLAG_OVERLAPPED )
    {
        wsa = WS2_make_async ( s, fd, ASYNC_TYPE_READ, iovec, dwBufferCount,
                               lpFlags, lpFrom, lpFromlen,
                               lpOverlapped, lpCompletionRoutine );

        if ( !wsa )
        {
            err = WSAEFAULT;
            goto err_free;
        }

        if ( ( ret = register_new_async ( &wsa->async )) )
        {
            err = NtStatusToWSAError ( ret );

            if ( !lpOverlapped )
                HeapFree ( GetProcessHeap(), 0, wsa->overlapped );
            HeapFree ( GetProcessHeap(), 0, wsa );
            goto err_free;
        }

        /* Try immediate completion */
        if ( lpOverlapped && !NtResetEvent( lpOverlapped->hEvent, NULL ) )
        {
            if  ( WSAGetOverlappedResult ( s, lpOverlapped,
                                           lpNumberOfBytesRecvd, FALSE, lpFlags) )
                return 0;

            if ( (err = WSAGetLastError ()) != WSA_IO_INCOMPLETE )
                goto error;
        }

        WSASetLastError ( WSA_IO_PENDING );
        return SOCKET_ERROR;
    }

    if ( _is_blocking(s) )
    {
        /* block here */
        /* FIXME: OOB and exceptfds? */
        do_block(fd, 1);
    }

    n = WS2_recv ( fd, iovec, dwBufferCount, lpFrom, lpFromlen, lpFlags );
    if ( n == -1 )
    {
        err = wsaErrno();
        goto err_free;
    }

    TRACE(" -> %i bytes\n", n);
    *lpNumberOfBytesRecvd = n;

    HeapFree (GetProcessHeap(), 0, iovec);
    close(fd);
    _enable_event(SOCKET2HANDLE(s), FD_READ, 0, 0);

    return 0;

err_free:
    HeapFree (GetProcessHeap(), 0, iovec);

err_close:
    close (fd);

error:
    WARN(" -> ERROR %d\n", err);
    WSASetLastError ( err );
    return SOCKET_ERROR;
}

/***********************************************************************
 *              WSCInstallProvider             (WS2_32.88)
 */
INT WINAPI WSCInstallProvider( const LPGUID lpProviderId,
                               LPCWSTR lpszProviderDllPath,
                               const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
                               DWORD dwNumberOfEntries,
                               LPINT lpErrno )
{
    FIXME("(%s, %s, %p, %ld, %p): stub !\n", debugstr_guid(lpProviderId),
          debugstr_w(lpszProviderDllPath), lpProtocolInfoList,
          dwNumberOfEntries, lpErrno);
    *lpErrno = 0;
    return 0;
}


/***********************************************************************
 *              WSCDeinstallProvider             (WS2_32.83)
 */
INT WINAPI WSCDeinstallProvider(LPGUID lpProviderId, LPINT lpErrno)
{
    FIXME("(%s, %p): stub !\n", debugstr_guid(lpProviderId), lpErrno);
    *lpErrno = 0;
    return 0;
}


/***********************************************************************
 *              WSAAccept                        (WS2_32.26)
 */
SOCKET WINAPI WSAAccept( SOCKET s, struct WS_sockaddr *addr, LPINT addrlen,
               LPCONDITIONPROC lpfnCondition, DWORD dwCallbackData)
{

       int ret = 0, size = 0;
       WSABUF CallerId, CallerData, CalleeId, CalleeData;
       /*        QOS SQOS, GQOS; */
       GROUP g;
       SOCKET cs;
       SOCKADDR src_addr, dst_addr;

       TRACE("Socket  %u, sockaddr %p, addrlen %p, fnCondition %p, dwCallbackData %ld\n",
               s, addr, addrlen, lpfnCondition, dwCallbackData);


       size = sizeof(src_addr);
       cs = WS_accept(s, &src_addr, &size);

       if (cs == SOCKET_ERROR) return SOCKET_ERROR;

       CallerId.buf = (char *)&src_addr;
       CallerId.len = sizeof(src_addr);

       CallerData.buf = NULL;
       CallerData.len = (ULONG)NULL;

       WS_getsockname(cs, &dst_addr, &size);

       CalleeId.buf = (char *)&dst_addr;
       CalleeId.len = sizeof(dst_addr);


       ret = (*lpfnCondition)(&CallerId, &CallerData, NULL, NULL,
                       &CalleeId, &CalleeData, &g, dwCallbackData);

       switch (ret)
       {
               case CF_ACCEPT:
                       if (addr && addrlen)
                               addr = memcpy(addr, &src_addr, (*addrlen > size) ?  size : *addrlen );
                       return cs;
               case CF_DEFER:
                       SERVER_START_REQ ( set_socket_deferred )
                       {
                           req->handle = SOCKET2HANDLE (s);
                           req->deferred = SOCKET2HANDLE (cs);
                           if ( !wine_server_call_err ( req ) )
                           {
                               SetLastError ( WSATRY_AGAIN );
                               WS_closesocket ( cs );
                           }
                       }
                       SERVER_END_REQ;
                       return SOCKET_ERROR;
               case CF_REJECT:
                       WS_closesocket(cs);
                       SetLastError(WSAECONNREFUSED);
                       return SOCKET_ERROR;
               default:
                       FIXME("Unknown return type from Condition function\n");
                       SetLastError(WSAENOTSOCK);
                       return SOCKET_ERROR;
               }
}

/***********************************************************************
 *              WSAEnumProtocolsA                        (WS2_32.37)
 */
int WINAPI WSAEnumProtocolsA(LPINT lpiProtocols, LPWSAPROTOCOL_INFOA lpProtocolBuffer, LPDWORD lpdwBufferLength)
{
    FIXME("(%p,%p,%p): stub\n", lpiProtocols,lpProtocolBuffer, lpdwBufferLength);
    return 0;
}

/***********************************************************************
 *              WSAEnumProtocolsW                        (WS2_32.38)
 */
int WINAPI WSAEnumProtocolsW(LPINT lpiProtocols, LPWSAPROTOCOL_INFOW lpProtocolBuffer, LPDWORD lpdwBufferLength)
{
    FIXME("(%p,%p,%p): stub\n", lpiProtocols,lpProtocolBuffer, lpdwBufferLength);
    return 0;
}

/***********************************************************************
 *              WSADuplicateSocketA                      (WS2_32.32)
 */
int WINAPI WSADuplicateSocketA( SOCKET s, DWORD dwProcessId, LPWSAPROTOCOL_INFOA lpProtocolInfo )
{
   HANDLE hProcess;

   TRACE("(%d,%lx,%p)\n", s, dwProcessId, lpProtocolInfo);
   memset(lpProtocolInfo, 0, sizeof(*lpProtocolInfo));
   /* FIXME: WS_getsockopt(s, WS_SOL_SOCKET, SO_PROTOCOL_INFO, lpProtocolInfo, sizeof(*lpProtocolInfo)); */
   /* I don't know what the real Windoze does next, this is a hack */
   /* ...we could duplicate and then use ConvertToGlobalHandle on the duplicate, then let
    * the target use the global duplicate, or we could copy a reference to us to the structure
    * and let the target duplicate it from us, but let's do it as simple as possible */
   hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwProcessId);
   DuplicateHandle(GetCurrentProcess(), SOCKET2HANDLE(s),
                   hProcess, (LPHANDLE)&lpProtocolInfo->dwCatalogEntryId,
                   0, FALSE, DUPLICATE_SAME_ACCESS);
   CloseHandle(hProcess);
   lpProtocolInfo->dwServiceFlags4 = 0xff00ff00; /* magic */
   return 0;
}

/***********************************************************************
 *              WSAInstallServiceClassA                  (WS2_32.48)
 */
int WINAPI WSAInstallServiceClassA(LPWSASERVICECLASSINFOA info)
{
    FIXME("Request to install service %s\n",debugstr_a(info->lpszServiceClassName));
    return WSAEACCES;
}

/***********************************************************************
 *              WSAInstallServiceClassW                  (WS2_32.49)
 */
int WINAPI WSAInstallServiceClassW(LPWSASERVICECLASSINFOW info)
{
    FIXME("Request to install service %s\n",debugstr_w(info->lpszServiceClassName));
    return WSAEACCES;
}
