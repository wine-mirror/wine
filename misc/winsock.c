/*
 * based on Windows Sockets 1.1 specs
 * (ftp.microsoft.com:/Advsys/winsock/spec11/WINSOCK.TXT)
 * 
 * (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka.
 *
 * NOTE: If you make any changes to fix a particular app, make sure 
 * they don't break something else like Netscape or telnet and ftp 
 * clients and servers (www.winsite.com got a lot of those).
 *
 */
 
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/ioctl.h>
#if defined(__svr4__)
#include <sys/filio.h>
#include <sys/ioccom.h>
#include <sys/sockio.h>
#endif
#if defined(__EMX__)
#include <sys/so_ioctl.h>
#include <sys/param.h>
#endif
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>

#include "winsock.h"
#include "windows.h"
#include "winnt.h"
#include "heap.h"
#include "ldt.h"
#include "task.h"
#include "message.h"
#include "miscemu.h"
#include "stddebug.h"
#include "debug.h"

#define DEBUG_SOCKADDR 0
#define dump_sockaddr(a) \
        fprintf(stderr, "sockaddr_in: family %d, address %s, port %d\n", \
                        ((struct sockaddr_in *)a)->sin_family, \
                        inet_ntoa(((struct sockaddr_in *)a)->sin_addr), \
                        ntohs(((struct sockaddr_in *)a)->sin_port))

#pragma pack(4)

/* ----------------------------------- internal data */

extern int h_errno;

static HANDLE32 	_WSHeap = 0;
static unsigned char*	_ws_stub = NULL;
static LPWSINFO         _wsi_list = NULL;

#define WS_ALLOC(size) \
	HeapAlloc(_WSHeap, HEAP_ZERO_MEMORY, (size) )
#define WS_FREE(ptr) \
	HeapFree(_WSHeap, 0, (ptr) )

#define WS_PTR2HANDLE(ptr) \
        ((short)((int)(ptr) - (int)_ws_stub))
#define WS_HANDLE2PTR(handle) \
        ((unsigned)((int)_ws_stub + (int)(handle)))

#define WSI_CHECK_RANGE(pwsi, pws) \
	( ((unsigned)(pws) > (unsigned)(pwsi)) && \
	  ((unsigned)(pws) < ((unsigned)(pwsi) + sizeof(WSINFO))) )

static INT32         _ws_sock_ops[] =
       { WS_SO_DEBUG, WS_SO_REUSEADDR, WS_SO_KEEPALIVE, WS_SO_DONTROUTE,
         WS_SO_BROADCAST, WS_SO_LINGER, WS_SO_OOBINLINE, WS_SO_SNDBUF,
         WS_SO_RCVBUF, WS_SO_ERROR, WS_SO_TYPE, WS_SO_DONTLINGER, 0 };
static int           _px_sock_ops[] =
       { SO_DEBUG, SO_REUSEADDR, SO_KEEPALIVE, SO_DONTROUTE, SO_BROADCAST,
         SO_LINGER, SO_OOBINLINE, SO_SNDBUF, SO_RCVBUF, SO_ERROR, SO_TYPE,
	 SO_LINGER };

static int   _check_ws(LPWSINFO pwsi, ws_socket* pws);
static char* _check_buffer(LPWSINFO pwsi, int size);

extern void EVENT_AddIO( int fd, unsigned flag );
extern void EVENT_DeleteIO( int fd, unsigned flag );

/***********************************************************************
 *          convert_sockopt()
 *
 * Converts socket flags from Windows format.
 */
static void convert_sockopt(INT32 *level, INT32 *optname)
{
  int i;
  switch (*level)
  {
     case WS_SOL_SOCKET:
        *level = SOL_SOCKET;
        for(i=0; _ws_sock_ops[i]; i++)
            if( _ws_sock_ops[i] == *optname ) break;
        if( _ws_sock_ops[i] ) *optname = _px_sock_ops[i];
        else fprintf(stderr, "convert_sockopt() unknown optname %d\n", *optname);
        break;
     case WS_IPPROTO_TCP:
        *optname = IPPROTO_TCP;
  }
}

/* ----------------------------------- Per-thread info (or per-process?) */

static LPWSINFO wsi_find(HTASK16 hTask)
{ 
    TDB*	   pTask = (TDB*)GlobalLock16(hTask);
    if( pTask )
    {
	if( pTask->pwsi ) return pTask->pwsi;
	else
	{
	    LPWSINFO pwsi = _wsi_list;
	    while( pwsi && pwsi->tid != hTask ) pwsi = pwsi->next;
	    if( pwsi )
		fprintf(stderr,"loose wsi struct! pwsi=0x%08x, task=0x%04x\n", 
					(unsigned)pwsi, hTask );
	    return pwsi; 
	}
    }
    return NULL;
}

static ws_socket* wsi_alloc_socket(LPWSINFO pwsi, int fd)
{
    /* Initialize a new entry in the socket table */

    if( pwsi->last_free >= 0 )
    {
	int i = pwsi->last_free;

	pwsi->last_free = pwsi->sock[i].flags;	/* free list */
	pwsi->sock[i].fd = fd;
	pwsi->sock[i].flags = 0;
	return &pwsi->sock[i];
    }
    return NULL;
}

static int wsi_strtolo(LPWSINFO pwsi, const char* name, const char* opt)
{
    /* Stuff a lowercase copy of the string into the local buffer */

    int i = strlen(name) + 2;
    char* p = _check_buffer(pwsi, i + ((opt)?strlen(opt):0));

    if( p )
    {
	do *p++ = tolower(*name); while(*name++);
	i = (p - (char*)(pwsi->buffer));
	if( opt ) do *p++ = tolower(*opt); while(*opt++);
	return i;
    }
    return 0;
}

static fd_set* fd_set_import( fd_set* fds, LPWSINFO pwsi, void* wsfds, int* highfd, BOOL32 b32 )
{
    /* translate Winsock fd set into local fd set */

    if( wsfds ) 
    { 
#define wsfds16	((ws_fd_set16*)wsfds)
#define wsfds32 ((ws_fd_set32*)wsfds)
	ws_socket* pws;
	int i, count;

	FD_ZERO(fds);
	count = b32 ? wsfds32->fd_count : wsfds16->fd_count;

	for( i = 0; i < count; i++ )
	{
	     pws = (b32) ? (ws_socket*)WS_HANDLE2PTR(wsfds32->fd_array[i])
			 : (ws_socket*)WS_HANDLE2PTR(wsfds16->fd_array[i]);
	     if( _check_ws(pwsi, pws) )
             {
		    if( pws->fd > *highfd ) *highfd = pws->fd;
		    FD_SET(pws->fd, fds);
	     }
	}
#undef wsfds32
#undef wsfds16
	return fds;
    }
    return NULL;
}

__inline__ static int sock_error_p(int s)
{
    unsigned int optval, optlen;

    optlen = sizeof(optval);
    getsockopt(s, SOL_SOCKET, SO_ERROR, &optval, &optlen);
    if (optval) dprintf_winsock(stddeb, "\t[%i] error: %d\n", s, optval);
    return optval != 0;
}

static int fd_set_export( LPWSINFO pwsi, fd_set* fds, fd_set* exceptfds, void* wsfds, BOOL32 b32 )
{
    int num_err = 0;

    /* translate local fd set into Winsock fd set, adding
     * errors to exceptfds (only if app requested it) */

    if( wsfds )
    {
#define wsfds16 ((ws_fd_set16*)wsfds)
#define wsfds32 ((ws_fd_set32*)wsfds)
	int i, j, count = (b32) ? wsfds32->fd_count : wsfds16->fd_count;

	for( i = 0, j = 0; i < count; i++ )
	{
	    ws_socket *pws = (b32) ? (ws_socket*)WS_HANDLE2PTR(wsfds32->fd_array[i])
				   : (ws_socket*)WS_HANDLE2PTR(wsfds16->fd_array[i]);
	    if( _check_ws(pwsi, pws) )
	    {
		int fd = pws->fd;

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
	    }
	}

	if( b32 ) wsfds32->fd_count = j;
	else wsfds16->fd_count = j;

	dprintf_winsock(stddeb, "\n");
#undef wsfds32
#undef wsfds16
    }
    return num_err;
}

HANDLE16 __ws_gethandle( void* ptr )
{
    return (HANDLE16)WS_PTR2HANDLE(ptr);
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
 *      WSAStartup16()			(WINSOCK.115)
 *
 * Create socket control struct, attach it to the global list and
 * update a pointer in the task struct.
 */
INT16 WINAPI WSAStartup16(UINT16 wVersionRequested, LPWSADATA lpWSAData)
{
    WSADATA WINSOCK_data = { 0x0101, 0x0101,
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
			   WS_MAX_UDP_DATAGRAM, (SEGPTR)NULL };
    HTASK16             tid = GetCurrentTask();
    LPWSINFO            pwsi;

    dprintf_winsock(stddeb, "WSAStartup: verReq=%x\n", wVersionRequested);

    if (LOBYTE(wVersionRequested) < 1 || (LOBYTE(wVersionRequested) == 1 &&
        HIBYTE(wVersionRequested) < 1)) return WSAVERNOTSUPPORTED;

    if (!lpWSAData) return WSAEINVAL;

    /* initialize socket heap */

    if( !_ws_stub )
    {
	_WSHeap = HeapCreate(HEAP_ZERO_MEMORY, 8120, 32768);
	if( !(_ws_stub = WS_ALLOC(0x10)) )
	{
	    fprintf(stderr,"Fatal: failed to create WinSock heap\n");
	    return 0;
	}
    }
    if( _WSHeap == 0 ) return WSASYSNOTREADY;

    /* create socket array for this task */
  
    pwsi = wsi_find(GetCurrentTask());
    if( pwsi == NULL )
    {
	TDB* pTask = (TDB*)GlobalLock16( tid );

	if( (pwsi = (LPWSINFO)WS_ALLOC( sizeof(WSINFO))) )
	{
	    int i = 0;
	    pwsi->tid = tid;
	    for( i = 0; i < WS_MAX_SOCKETS_PER_PROCESS; i++ )
	    {
		pwsi->sock[i].fd = -1; 
		pwsi->sock[i].flags = i + 1; 
	    }
	    pwsi->sock[WS_MAX_SOCKETS_PER_PROCESS - 1].flags = -1;
	}
	else return WSASYSNOTREADY;

        /* add this control struct to the global list */

	pwsi->prev = NULL;
	if( _wsi_list ) 
	    _wsi_list->prev = pwsi;
	pwsi->next = _wsi_list; 
	_wsi_list = pwsi;
	pTask->pwsi = pwsi;
    }
    else pwsi->num_startup++;

    /* return winsock information */

    memcpy(lpWSAData, &WINSOCK_data, sizeof(WINSOCK_data));

    dprintf_winsock(stddeb, "WSAStartup: succeeded\n");
    return 0;
}

/***********************************************************************
 *      WSAStartup32()			(WSOCK32.115)
 */
INT32 WINAPI WSAStartup32(UINT32 wVersionRequested, LPWSADATA lpWSAData)
{
    return WSAStartup16( wVersionRequested, lpWSAData );
}

/***********************************************************************
 *      WSACleanup()			(WINSOCK.116)
 *
 * Cleanup functions of varying impact.
 */
void WINSOCK_Shutdown()
{
    /* Called on exit(), has to remove all outstanding async DNS processes.  */

    WINSOCK_cancel_task_aops( 0, __ws_memfree );
}

INT32 WINSOCK_DeleteTaskWSI( TDB* pTask, LPWSINFO pwsi )
{
    /* WSACleanup() backend, called on task termination as well.
     * Real DLL would have registered its own signal handler with
     * TaskSetSignalHandler() and waited until USIG_TERMINATION/USIG_GPF
     * but this scheme is much more straightforward.
     */

    int	i, j, n;

    if( --pwsi->num_startup > 0 ) return 0;

    SIGNAL_MaskAsyncEvents( TRUE );
    WINSOCK_cancel_task_aops( pTask->hSelf, __ws_memfree );
    SIGNAL_MaskAsyncEvents( FALSE );

    /* unlink socket control struct */

    if( pwsi == _wsi_list ) 
	_wsi_list = pwsi->next;
    else
	pwsi->prev->next = pwsi->next;
    if( pwsi->next ) pwsi->next->prev = pwsi->prev; 

    if( _wsi_list == NULL ) 
	WINSOCK_Shutdown();	/* just in case */

    if( pwsi->flags & WSI_BLOCKINGCALL )
	dprintf_winsock(stddeb,"\tinside blocking call!\n");

/* FIXME: aop_control() doesn't decrement pwsi->num_async_rq
 *
 *    if( pwsi->num_async_rq )
 *	  dprintf_winsock(stddeb,"\thave %i outstanding async ops!\n", pwsi->num_async_rq );
 */

    for(i = 0, j = 0, n = 0; i < WS_MAX_SOCKETS_PER_PROCESS; i++)
	if( pwsi->sock[i].fd != -1 )
	{
	    if( pwsi->sock[i].psop )
	    {
		n++;
		WSAAsyncSelect32( (SOCKET16)WS_PTR2HANDLE(pwsi->sock + i), 0, 0, 0 );
	    }
            close(pwsi->sock[i].fd); j++; 
        }
    if( j ) 
	  dprintf_winsock(stddeb,"\tclosed %i sockets, killed %i async selects!\n", j, n);

    /* delete scratch buffers */

    if( pwsi->buffer ) SEGPTR_FREE(pwsi->buffer);
    if( pwsi->dbuffer ) SEGPTR_FREE(pwsi->dbuffer);
	
    if( pTask )
        pTask->pwsi = NULL;
    memset( pwsi, 0, sizeof(WSINFO) );
    WS_FREE(pwsi);
    return 0;
}

INT32 WINAPI WSACleanup(void)
{
    HTASK16	hTask = GetCurrentTask();

    dprintf_winsock(stddeb, "WSACleanup(%04x)\n", hTask );
    if( hTask )
    {
	LPWSINFO pwsi = wsi_find(hTask);
	if( pwsi )
	    return WINSOCK_DeleteTaskWSI( (TDB*)GlobalLock16(hTask), pwsi );
	return SOCKET_ERROR;
    }
    else
	WINSOCK_Shutdown(); /* remove all outstanding DNS requests */
    return 0;
}


/***********************************************************************
 *      WSAGetLastError()		(WSOCK32.111)(WINSOCK.111)
 */
INT32 WINAPI WSAGetLastError(void)
{
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());
    INT16		ret;

    dprintf_winsock(stddeb, "WSAGetLastError(%08x)", (unsigned)pwsi);

    ret = (pwsi) ? pwsi->err : WSANOTINITIALISED;

    dprintf_winsock(stddeb, " = %i\n", (int)ret);
    return ret;
}

/***********************************************************************
 *      WSASetLastError32()		(WSOCK32.112)
 */
void WINAPI WSASetLastError32(INT32 iError)
{
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WSASetLastError(%08x): %d\n", (unsigned)pwsi, (int)iError);
    if( pwsi ) pwsi->err = iError;
}

/***********************************************************************
 *      WSASetLastError16()		(WINSOCK.112)
 */
void WINAPI WSASetLastError16(INT16 iError)
{
    WSASetLastError32(iError);
}

int _check_ws(LPWSINFO pwsi, ws_socket* pws)
{
    if( pwsi )
	if( pwsi->flags & WSI_BLOCKINGCALL ) pwsi->err = WSAEINPROGRESS;
	else if( WSI_CHECK_RANGE(pwsi, pws) ) return 1;
		 else pwsi->err = WSAENOTSOCK;
    return 0;
}

char* _check_buffer(LPWSINFO pwsi, int size)
{
    if( pwsi->buffer && pwsi->buflen >= size ) return pwsi->buffer;
    else SEGPTR_FREE(pwsi->buffer);

    pwsi->buffer = (char*)SEGPTR_ALLOC((pwsi->buflen = size)); 
    return pwsi->buffer;
}

/* ----------------------------------- i/o APIs */

/***********************************************************************
 *		accept()		(WSOCK32.1)
 */
SOCKET32 WINAPI WINSOCK_accept32(SOCKET32 s, struct sockaddr *addr,
                                 INT32 *addrlen32)
{
    ws_socket*	pws  = (ws_socket*)WS_HANDLE2PTR((SOCKET16)s);
    LPWSINFO	pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_ACCEPT(%08x): socket %04x\n", 
				  (unsigned)pwsi, (UINT16)s ); 
    if( _check_ws(pwsi, pws) )
    {
	int 	sock, fd_flags;

	fd_flags = fcntl(pws->fd, F_GETFL, 0);

	if( (sock = accept(pws->fd, addr, addrlen32)) >= 0 )
	{
	    ws_socket*	pnew = wsi_alloc_socket(pwsi, sock); 
	    if( pnew )
	    {
		s = (SOCKET32)WS_PTR2HANDLE(pnew);
		if( pws->psop && pws->flags & WS_FD_ACCEPT )
		{
		    EVENT_AddIO( pws->fd, EVENT_IO_READ );	/* reenabler */

		    /* async select the accept()'ed socket */
		    WSAAsyncSelect32( s, pws->psop->hWnd, pws->psop->uMsg,
				      pws->flags & ~WS_FD_ACCEPT );
		}
		return s;
            } 
	    else pwsi->err = WSAENOBUFS;
	} 
	else pwsi->err = wsaErrno();
    }
    return INVALID_SOCKET32;
}

/***********************************************************************
 *              accept()		(WINSOCK.1)
 */
SOCKET16 WINAPI WINSOCK_accept16(SOCKET16 s, struct sockaddr* addr,
                                 INT16* addrlen16 )
{
    INT32 addrlen32 = addrlen16 ? *addrlen16 : 0;
    SOCKET32 retSocket = WINSOCK_accept32( s, addr, &addrlen32 );
    if( addrlen16 ) *addrlen16 = (INT16)addrlen32;
    return (SOCKET16)retSocket;
}

/***********************************************************************
 *		bind()			(WSOCK32.2)
 */
INT32 WINAPI WINSOCK_bind32(SOCKET32 s, struct sockaddr *name, INT32 namelen)
{
    ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_BIND(%08x): socket %04x, ptr %8x, length %d\n", 
			   (unsigned)pwsi, s, (int) name, namelen);
#if DEBUG_SOCKADDR
    dump_sockaddr(name);
#endif

    if ( _check_ws(pwsi, pws) )
      if ( namelen >= sizeof(*name) ) 
	if ( ((struct ws_sockaddr_in *)name)->sin_family == AF_INET )
	  if ( bind(pws->fd, name, namelen) < 0 ) 
	  {
	     int	loc_errno = errno;
	     dprintf_winsock(stddeb,"\tfailure - errno = %i\n", errno);
	     errno = loc_errno;
	     switch(errno)
	     {
		case EBADF: pwsi->err = WSAENOTSOCK; break;
		case EADDRNOTAVAIL: pwsi->err = WSAEINVAL; break;
		default: pwsi->err = wsaErrno();
	     }
	  }
	  else return 0; /* success */
	else pwsi->err = WSAEAFNOSUPPORT;
      else pwsi->err = WSAEFAULT;
    return SOCKET_ERROR;
}

/***********************************************************************
 *              bind()			(WINSOCK.2)
 */
INT16 WINAPI WINSOCK_bind16(SOCKET16 s, struct sockaddr *name, INT16 namelen)
{
  return (INT16)WINSOCK_bind32( s, name, namelen );
}

/***********************************************************************
 *		closesocket()		(WSOCK32.3)
 */
INT32 WINAPI WINSOCK_closesocket32(SOCKET32 s)
{
    ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_CLOSE(%08x): socket %08x\n", (unsigned)pwsi, s);

    if( _check_ws(pwsi, pws) )
    { 
	int	fd = pws->fd;

	if( pws->psop ) WSAAsyncSelect32( s, 0, 0, 0 );

	pws->fd = -1;
	pws->flags = (unsigned)pwsi->last_free;
	pwsi->last_free = pws - &pwsi->sock[0]; /* add to free list */

	if( close(fd) == 0 ) 
	    return 0;
	pwsi->err = (errno == EBADF) ? WSAENOTSOCK : wsaErrno();
    }
    return SOCKET_ERROR;
}

/***********************************************************************
 *              closesocket()           (WINSOCK.3)
 */
INT16 WINAPI WINSOCK_closesocket16(SOCKET16 s)
{
    return (INT16)WINSOCK_closesocket32(s);
}

/***********************************************************************
 *		connect()		(WSOCK32.4)
 */
INT32 WINAPI WINSOCK_connect32(SOCKET32 s, struct sockaddr *name, INT32 namelen)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_CONNECT(%08x): socket %04x, ptr %8x, length %d\n", 
			   (unsigned)pwsi, s, (int) name, namelen);
#if DEBUG_SOCKADDR
  dump_sockaddr(name);
#endif

  if( _check_ws(pwsi, pws) )
  {
    if (connect(pws->fd, name, namelen) == 0) 
    { 
	if( pws->psop && (pws->flags & WS_FD_CONNECT) )
	{
	    /* application did AsyncSelect() but then went
	     * ahead and called connect() without waiting for 
	     * notification.
	     *
	     * FIXME: Do we have to post a notification message 
	     *        in this case?
	     */

	    if( !(pws->flags & WS_FD_CONNECTED) )
	    {
		if( pws->flags & (WS_FD_READ | WS_FD_CLOSE) )
		    EVENT_AddIO( pws->fd, EVENT_IO_READ );
		else
		    EVENT_DeleteIO( pws->fd, EVENT_IO_READ );
		if( pws->flags & WS_FD_WRITE )
		    EVENT_AddIO( pws->fd, EVENT_IO_WRITE );
		else
		    EVENT_DeleteIO( pws->fd, EVENT_IO_WRITE );
	    }
	}
	pws->flags |= WS_FD_CONNECTED;
	pws->flags &= ~(WS_FD_INACTIVE | WS_FD_CONNECT | WS_FD_LISTENING);
        return 0; 
    }
    pwsi->err = (errno == EINPROGRESS) ? WSAEWOULDBLOCK : wsaErrno();
  }
  return SOCKET_ERROR;
}

/***********************************************************************
 *              connect()               (WINSOCK.4)
 */
INT16 WINAPI WINSOCK_connect16(SOCKET16 s, struct sockaddr *name, INT16 namelen)
{
  return (INT16)WINSOCK_connect32( s, name, namelen );
}

/***********************************************************************
 *		getpeername()		(WSOCK32.5)
 */
INT32 WINAPI WINSOCK_getpeername32(SOCKET32 s, struct sockaddr *name,
                                   INT32 *namelen)
{
    ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_GETPEERNAME(%08x): socket: %04x, ptr %8x, ptr %8x\n", 
			   (unsigned)pwsi, s, (int) name, *namelen);
    if( _check_ws(pwsi, pws) )
    {
	if (getpeername(pws->fd, name, namelen) == 0) 
	    return 0; 
	pwsi->err = (h_errno < 0) ? wsaErrno() : wsaHerrno();
    }
    return SOCKET_ERROR;
}

/***********************************************************************
 *              getpeername()		(WINSOCK.5)
 */
INT16 WINAPI WINSOCK_getpeername16(SOCKET16 s, struct sockaddr *name,
                                   INT16 *namelen16)
{
    INT32 namelen32 = *namelen16;
    INT32 retVal = WINSOCK_getpeername32( s, name, &namelen32 );

#if DEBUG_SOCKADDR
    dump_sockaddr(name);
#endif

   *namelen16 = namelen32;
    return (INT16)retVal;
}

/***********************************************************************
 *		getsockname()		(WSOCK32.6)
 */
INT32 WINAPI WINSOCK_getsockname32(SOCKET32 s, struct sockaddr *name,
                                   INT32 *namelen)
{
    ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_GETSOCKNAME(%08x): socket: %04x, ptr %8x, ptr %8x\n", 
			  (unsigned)pwsi, s, (int) name, (int) *namelen);
    if( _check_ws(pwsi, pws) )
    {
	if (getsockname(pws->fd, name, namelen) == 0)
	    return 0; 
	pwsi->err = (h_errno < 0) ? wsaErrno() : wsaHerrno();
    }
    return SOCKET_ERROR;
}

/***********************************************************************
 *              getsockname()		(WINSOCK.6)
 */
INT16 WINAPI WINSOCK_getsockname16(SOCKET16 s, struct sockaddr *name,
                                   INT16 *namelen16)
{
    INT32 retVal;

    if( namelen16 )
    {
        INT32 namelen32 = *namelen16;
        retVal = WINSOCK_getsockname32( s, name, &namelen32 );
       *namelen16 = namelen32;

#if DEBUG_SOCKADDR
    dump_sockaddr(name);
#endif

    }
    else retVal = SOCKET_ERROR;
    return (INT16)retVal;
}


/***********************************************************************
 *		getsockopt()		(WSOCK32.7)
 */
INT32 WINAPI WINSOCK_getsockopt32(SOCKET32 s, INT32 level, 
                                  INT32 optname, char *optval, INT32 *optlen)
{
    ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_GETSOCKOPT(%08x): socket: %04x, opt %d, ptr %8x, ptr %8x\n", 
			   (unsigned)pwsi, s, level, (int) optval, (int) *optlen);
    if( _check_ws(pwsi, pws) )
    {
	convert_sockopt(&level, &optname);
	if (getsockopt(pws->fd, (int) level, optname, optval, optlen) == 0 )
	    return 0;
	pwsi->err = (errno == EBADF) ? WSAENOTSOCK : wsaErrno();
    }
    return SOCKET_ERROR;
}

/***********************************************************************
 *              getsockopt()		(WINSOCK.7)
 */
INT16 WINAPI WINSOCK_getsockopt16(SOCKET16 s, INT16 level,
                                  INT16 optname, char *optval, INT16 *optlen)
{
    INT32 optlen32;
    INT32 *p = &optlen32;
    INT32 retVal;
    if( optlen ) optlen32 = *optlen; else p = NULL;
    retVal = WINSOCK_getsockopt32( s, (UINT16)level, optname, optval, p );
    if( optlen ) *optlen = optlen32;
    return (INT16)retVal;
}

/***********************************************************************
 *		htonl()			(WINSOCK.8)(WSOCK32.8)
 */
u_long WINAPI WINSOCK_htonl(u_long hostlong)   { return( htonl(hostlong) ); }
/***********************************************************************
 *		htons()			(WINSOCK.9)(WSOCK32.9)
 */
u_short WINAPI WINSOCK_htons(u_short hostshort) { return( htons(hostshort) ); }
/***********************************************************************
 *		inet_addr()		(WINSOCK.10)(WSOCK32.10)
 */
u_long WINAPI WINSOCK_inet_addr(char *cp)      { return( inet_addr(cp) ); }
/***********************************************************************
 *		htohl()			(WINSOCK.14)(WSOCK32.14)
 */
u_long WINAPI WINSOCK_ntohl(u_long netlong)    { return( ntohl(netlong) ); }
/***********************************************************************
 *		ntohs()			(WINSOCK.15)(WSOCK32.15)
 */
u_short WINAPI WINSOCK_ntohs(u_short netshort)  { return( ntohs(netshort) ); }

/***********************************************************************
 *		inet_ntoa()		(WINSOCK.11)(WSOCK32.11)
 */
char* WINAPI WINSOCK_inet_ntoa32(struct in_addr in)
{
  /* use "buffer for dummies" here because some applications have 
   * propensity to decode addresses in ws_hostent structure without 
   * saving them first...
   */

    LPWSINFO      pwsi = wsi_find(GetCurrentTask());

    if( pwsi )
    {
	char*	s = inet_ntoa(in);
	if( s ) 
	{
	    if( pwsi->dbuffer == NULL )
		if((pwsi->dbuffer = (char*) SEGPTR_ALLOC(32)) == NULL )
		{
		    pwsi->err = WSAENOBUFS;
		    return NULL;
		}
	    strncpy(pwsi->dbuffer, s, 32 );
	    return pwsi->dbuffer; 
	}
	pwsi->err = wsaErrno();
    }
    return NULL;
}

SEGPTR WINAPI WINSOCK_inet_ntoa16(struct in_addr in)
{
  char* retVal = WINSOCK_inet_ntoa32(in);
  return retVal ? SEGPTR_GET(retVal) : (SEGPTR)NULL;
}

/***********************************************************************
 *		ioctlsocket()		(WSOCK32.12)
 */
INT32 WINAPI WINSOCK_ioctlsocket32(SOCKET32 s, UINT32 cmd, UINT32 *argp)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_IOCTL(%08x): socket %04x, cmd %08x, ptr %8x\n", 
			  (unsigned)pwsi, s, cmd, (unsigned) argp);
  if( _check_ws(pwsi, pws) )
  {
    long 	newcmd  = cmd;

    switch( cmd )
    {
	case WS_FIONREAD:   
		newcmd=FIONREAD; 
		break;

	case WS_FIONBIO:    
		newcmd=FIONBIO;  
		if( pws->psop && *argp == 0 ) 
		{
		    /* AsyncSelect()'ed sockets are always nonblocking */
		    pwsi->err = WSAEINVAL; 
		    return SOCKET_ERROR; 
		}
		break;

	case WS_SIOCATMARK: 
		newcmd=SIOCATMARK; 
		break;

	case WS_IOW('f',125,u_long): 
		fprintf(stderr,"Warning: WS1.1 shouldn't be using async I/O\n");
		pwsi->err = WSAEINVAL; 
		return SOCKET_ERROR;

	default:	  
		/* Netscape tries hard to use bogus ioctl 0x667e */
		dprintf_winsock(stddeb,"\tunknown WS_IOCTL cmd (%08x)\n", cmd);
    }
    if( ioctl(pws->fd, newcmd, (char*)argp ) == 0 ) return 0;
    pwsi->err = (errno == EBADF) ? WSAENOTSOCK : wsaErrno(); 
  }
  return SOCKET_ERROR;
}

/***********************************************************************
 *              ioctlsocket()           (WINSOCK.12)
 */
INT16 WINAPI WINSOCK_ioctlsocket16(SOCKET16 s, UINT32 cmd, UINT32 *argp)
{
    return (INT16)WINSOCK_ioctlsocket32( s, cmd, argp );
}


/***********************************************************************
 *		listen()		(WSOCK32.13)
 */
INT32 WINAPI WINSOCK_listen32(SOCKET32 s, INT32 backlog)
{
    ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_LISTEN(%08x): socket %04x, backlog %d\n", 
			    (unsigned)pwsi, s, backlog);
    if( _check_ws(pwsi, pws) )
    {
	if (listen(pws->fd, backlog) == 0)
	{
	    if( !pws->psop )
	    {
		int  fd_flags = fcntl(pws->fd, F_GETFL, 0);
		if( !(fd_flags & O_NONBLOCK) ) pws->flags |= WS_FD_ACCEPT;
	    }
	    pws->flags |= WS_FD_LISTENING;
	    pws->flags &= ~(WS_FD_INACTIVE | WS_FD_CONNECT | WS_FD_CONNECTED); /* just in case */
	    return 0;
	}
	pwsi->err = wsaErrno();
    }
    else if( pwsi ) pwsi->err = WSAENOTSOCK;
    return SOCKET_ERROR;
}

/***********************************************************************
 *              listen()		(WINSOCK.13)
 */
INT16 WINAPI WINSOCK_listen16(SOCKET16 s, INT16 backlog)
{
    return (INT16)WINSOCK_listen32( s, backlog );
}


/***********************************************************************
 *		recv()			(WSOCK32.16)
 */
INT32 WINAPI WINSOCK_recv32(SOCKET32 s, char *buf, INT32 len, INT32 flags)
{
    ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_RECV(%08x): socket %04x, buf %8x, len %d, flags %d",
                          (unsigned)pwsi, s, (unsigned)buf, len, flags);
    if( _check_ws(pwsi, pws) )
    {
	INT32 length;
	if ((length = recv(pws->fd, buf, len, flags)) >= 0) 
	{ 
	    dprintf_winsock(stddeb, " -> %i bytes\n", length);

	    if( pws->psop && (pws->flags & (WS_FD_READ | WS_FD_CLOSE)) )
		EVENT_AddIO( pws->fd, EVENT_IO_READ );	/* reenabler */

	    return length;
	}
	pwsi->err = wsaErrno();
    }
    else if( pwsi ) pwsi->err = WSAENOTSOCK;
    dprintf_winsock(stddeb, " -> ERROR\n");
    return SOCKET_ERROR;
}

/***********************************************************************
 *              recv()			(WINSOCK.16)
 */
INT16 WINAPI WINSOCK_recv16(SOCKET16 s, char *buf, INT16 len, INT16 flags)
{
    return (INT16)WINSOCK_recv32( s, buf, len, flags );
}


/***********************************************************************
 *		recvfrom()		(WSOCK32.17)
 */
INT32 WINAPI WINSOCK_recvfrom32(SOCKET32 s, char *buf, INT32 len, INT32 flags, 
                                struct sockaddr *from, INT32 *fromlen32)
{
    ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_RECVFROM(%08x): socket %04x, ptr %08x, len %d, flags %d",
                          (unsigned)pwsi, s, (unsigned)buf, len, flags);
#if DEBUG_SOCKADDR
    if( from ) dump_sockaddr(from);
    else fprintf(stderr, "\tfrom = NULL\n");
#endif

    if( _check_ws(pwsi, pws) )
    {
	int length;

	if ((length = recvfrom(pws->fd, buf, len, flags, from, fromlen32)) >= 0)
	{
	    dprintf_winsock(stddeb, " -> %i bytes\n", length);

	    if( pws->psop && (pws->flags & (WS_FD_READ | WS_FD_CLOSE)) )
		EVENT_AddIO( pws->fd, EVENT_IO_READ );  /* reenabler */

	    return (INT16)length;
	}
	pwsi->err = wsaErrno();
    }
    else if( pwsi ) pwsi->err = WSAENOTSOCK;
    dprintf_winsock(stddeb, " -> ERROR\n");
    return SOCKET_ERROR;
}

/***********************************************************************
 *              recvfrom()		(WINSOCK.17)
 */
INT16 WINAPI WINSOCK_recvfrom16(SOCKET16 s, char *buf, INT16 len, INT16 flags,
                                struct sockaddr *from, INT16 *fromlen16)
{
    INT32 fromlen32;
    INT32 *p = &fromlen32;
    INT32 retVal;

    if( fromlen16 ) fromlen32 = *fromlen16; else p = NULL;
    retVal = WINSOCK_recvfrom32( s, buf, len, flags, from, p );
    if( fromlen16 ) *fromlen16 = fromlen32;
    return (INT16)retVal;
}

/***********************************************************************
 *		select()		(WINSOCK.18)(WSOCK32.18)
 */
static INT32 __ws_select( BOOL32 b32, void *ws_readfds, void *ws_writefds, void *ws_exceptfds,
			  struct timeval *timeout )
{
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());
	
    dprintf_winsock(stddeb, "WS_SELECT(%08x): read %8x, write %8x, excp %8x\n", 
    (unsigned) pwsi, (unsigned) ws_readfds, (unsigned) ws_writefds, (unsigned) ws_exceptfds);

    if( pwsi )
    {
	int         highfd = 0;
	fd_set      readfds, writefds, exceptfds;
	fd_set     *p_read, *p_write, *p_except;

	p_read = fd_set_import(&readfds, pwsi, ws_readfds, &highfd, b32);
	p_write = fd_set_import(&writefds, pwsi, ws_writefds, &highfd, b32);
	p_except = fd_set_import(&exceptfds, pwsi, ws_exceptfds, &highfd, b32);

	if( (highfd = select(highfd + 1, p_read, p_write, p_except, timeout)) > 0 )
	{
	    fd_set_export(pwsi, &readfds, p_except, ws_readfds, b32);
	    fd_set_export(pwsi, &writefds, p_except, ws_writefds, b32);

	    if (p_except && ws_exceptfds)
	    {
#define wsfds16 ((ws_fd_set16*)ws_exceptfds)
#define wsfds32 ((ws_fd_set32*)ws_exceptfds)
		int i, j, count = (b32) ? wsfds32->fd_count : wsfds16->fd_count;

		for (i = j = 0; i < count; i++)
		{
		    ws_socket *pws = (b32) ? (ws_socket *)WS_HANDLE2PTR(wsfds32->fd_array[i])
					   : (ws_socket *)WS_HANDLE2PTR(wsfds16->fd_array[i]);
		    if( _check_ws(pwsi, pws) && FD_ISSET(pws->fd, &exceptfds) )
		    {
			if( b32 )
				wsfds32->fd_array[j++] = wsfds32->fd_array[i];
			else
				wsfds16->fd_array[j++] = wsfds16->fd_array[i];
		    }
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
	if( ws_readfds ) ((ws_fd_set32*)ws_readfds)->fd_count = 0;
	if( ws_writefds ) ((ws_fd_set32*)ws_writefds)->fd_count = 0;
	if( ws_exceptfds ) ((ws_fd_set32*)ws_exceptfds)->fd_count = 0;

        if( highfd == 0 ) return 0;
	pwsi->err = wsaErrno();
    } 
    return SOCKET_ERROR;
}

INT16 WINAPI WINSOCK_select16(INT16 nfds, ws_fd_set16 *ws_readfds,
                              ws_fd_set16 *ws_writefds, ws_fd_set16 *ws_exceptfds,
                              struct timeval *timeout)
{
    return (INT16)__ws_select( FALSE, ws_readfds, ws_writefds, ws_exceptfds, timeout );
}

INT32 WINAPI WINSOCK_select32(INT32 nfds, ws_fd_set32 *ws_readfds,
                              ws_fd_set32 *ws_writefds, ws_fd_set32 *ws_exceptfds,
                              struct timeval *timeout)
{
    /* struct timeval is the same for both 32- and 16-bit code */
    return (INT32)__ws_select( TRUE, ws_readfds, ws_writefds, ws_exceptfds, timeout );
}


/***********************************************************************
 *		send()			(WSOCK32.19)
 */
INT32 WINAPI WINSOCK_send32(SOCKET32 s, char *buf, INT32 len, INT32 flags)
{
    ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_SEND(%08x): socket %04x, ptr %08x, length %d, flags %d\n", 
			   (unsigned)pwsi, s, (unsigned) buf, len, flags);
    if( _check_ws(pwsi, pws) )
    {
	int	length;

	if ((length = send(pws->fd, buf, len, flags)) < 0 ) 
	{
	    pwsi->err = wsaErrno();
	    if( pwsi->err == WSAEWOULDBLOCK && 
		pws->psop && pws->flags & WS_FD_WRITE )
		EVENT_AddIO( pws->fd, EVENT_IO_WRITE );	/* reenabler */
	}
	else return (INT16)length;
    }
    else if( pwsi ) pwsi->err = WSAENOTSOCK;
    return SOCKET_ERROR;
}

/***********************************************************************
 *              send()			(WINSOCK.19)
 */
INT16 WINAPI WINSOCK_send16(SOCKET16 s, char *buf, INT16 len, INT16 flags)
{
    return WINSOCK_send32( s, buf, len, flags );
}

/***********************************************************************
 *		sendto()		(WSOCK32.20)
 */
INT32 WINAPI WINSOCK_sendto32(SOCKET32 s, char *buf, INT32 len, INT32 flags,
                              struct sockaddr *to, INT32 tolen)
{
    ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_SENDTO(%08x): socket %04x, ptr %08x, length %d, flags %d\n",
                          (unsigned)pwsi, s, (unsigned) buf, len, flags);
    if( _check_ws(pwsi, pws) )
    {
	INT32	length;

	if ((length = sendto(pws->fd, buf, len, flags, to, tolen)) < 0 )
	{
	    pwsi->err = wsaErrno();
	    if( pwsi->err == WSAEWOULDBLOCK &&
		pws->psop && pws->flags & WS_FD_WRITE )
		EVENT_AddIO( pws->fd, EVENT_IO_WRITE ); /* reenabler */
	} 
	else return length;
    }
    else if( pwsi ) pwsi->err = WSAENOTSOCK;
    return SOCKET_ERROR;
}

/***********************************************************************
 *              sendto()		(WINSOCK.20)
 */
INT16 WINAPI WINSOCK_sendto16(SOCKET16 s, char *buf, INT16 len, INT16 flags,
                              struct sockaddr *to, INT16 tolen)
{
    return (INT16)WINSOCK_sendto32( s, buf, len, flags, to, tolen );
}

/***********************************************************************
 *		setsockopt()		(WSOCK32.21)
 */
INT32 WINAPI WINSOCK_setsockopt32(SOCKET16 s, INT32 level, INT32 optname, 
                                  char *optval, INT32 optlen)
{
    ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_SETSOCKOPT(%08x): socket %04x, lev %d, opt %d, ptr %08x, len %d\n",
			  (unsigned)pwsi, s, level, optname, (int) optval, optlen);
    if( _check_ws(pwsi, pws) )
    {
	convert_sockopt(&level, &optname);
	if (setsockopt(pws->fd, level, optname, optval, optlen) == 0) return 0;
	pwsi->err = wsaErrno();
    }
    else if( pwsi ) pwsi->err = WSAENOTSOCK;
    return SOCKET_ERROR;
}

/***********************************************************************
 *              setsockopt()		(WINSOCK.21)
 */
INT16 WINAPI WINSOCK_setsockopt16(SOCKET16 s, INT16 level, INT16 optname,
                                  char *optval, INT16 optlen)
{
    INT32 linger32[2];
    if( !optval ) return SOCKET_ERROR;
    if( optname == SO_LINGER )
    {
	INT16* ptr = (INT16*)optval;
	linger32[0] = ptr[0];
	linger32[1] = ptr[1];
	optval = (char*)&linger32;
	optlen = sizeof(linger32);
    }
    return (INT16)WINSOCK_setsockopt32( s, (UINT16)level, optname, optval, optlen );
}


/***********************************************************************
 *		shutdown()		(WSOCK32.22)
 */
INT32 WINAPI WINSOCK_shutdown32(SOCKET32 s, INT32 how)
{
    ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_SHUTDOWN(%08x): socket %04x, how %i\n",
			    (unsigned)pwsi, s, how );
    if( _check_ws(pwsi, pws) )
    {
	if( pws->psop )
	    switch( how )
	    {
		case 0: /* drop receives */
			if( pws->flags & (WS_FD_READ | WS_FD_CLOSE) )
			    EVENT_DeleteIO( pws->fd, EVENT_IO_READ );
			pws->flags &= ~(WS_FD_READ | WS_FD_CLOSE);
#ifdef SHUT_RD
			how = SHUT_RD;
#endif
			break;

		case 1: /* drop sends */
			if( pws->flags & WS_FD_WRITE )
			    EVENT_DeleteIO( pws->fd, EVENT_IO_WRITE );
			pws->flags &= ~WS_FD_WRITE;
#ifdef SHUT_WR
			how = SHUT_WR;
#endif
			break;

		case 2: /* drop all */
#ifdef SHUT_RDWR
			how = SHUT_RDWR;
#endif
		default:
			WSAAsyncSelect32( s, 0, 0, 0 );
			break;
	    }

	if (shutdown(pws->fd, how) == 0) 
	{
	    if( how > 1 ) 
	    {
		pws->flags &= ~(WS_FD_CONNECTED | WS_FD_LISTENING);
		pws->flags |= WS_FD_INACTIVE;
	    }
	    return 0;
	}
	pwsi->err = wsaErrno();
    } 
    else if( pwsi ) pwsi->err = WSAENOTSOCK;
    return SOCKET_ERROR;
}

/***********************************************************************
 *              shutdown()		(WINSOCK.22)
 */
INT16 WINAPI WINSOCK_shutdown16(SOCKET16 s, INT16 how)
{
    return (INT16)WINSOCK_shutdown32( s, how );
}


/***********************************************************************
 *		socket()		(WSOCK32.23)
 */
SOCKET32 WINAPI WINSOCK_socket32(INT32 af, INT32 type, INT32 protocol)
{
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_SOCKET(%08x): af=%d type=%d protocol=%d\n", 
			  (unsigned)pwsi, af, type, protocol);

  if( pwsi )
  {
    int		sock;

    /* check the socket family */
    switch(af) 
    {
	case AF_INET:
	case AF_UNSPEC: break;
	default:        pwsi->err = WSAEAFNOSUPPORT; 
			return INVALID_SOCKET32;
    }

    /* check the socket type */
    switch(type) 
    {
	case SOCK_STREAM:
	case SOCK_DGRAM:
	case SOCK_RAW:  break;
	default:        pwsi->err = WSAESOCKTNOSUPPORT; 
			return INVALID_SOCKET32;
    }

    /* check the protocol type */
    if ( protocol < 0 )  /* don't support negative values */
    { pwsi->err = WSAEPROTONOSUPPORT; return INVALID_SOCKET32; }

    if ( af == AF_UNSPEC)  /* did they not specify the address family? */
        switch(protocol) 
	{
          case IPPROTO_TCP:
             if (type == SOCK_STREAM) { af = AF_INET; break; }
          case IPPROTO_UDP:
             if (type == SOCK_DGRAM)  { af = AF_INET; break; }
          default: pwsi->err = WSAEPROTOTYPE; return INVALID_SOCKET32;
        }

    if ((sock = socket(af, type, protocol)) >= 0) 
    {
        ws_socket*      pnew = wsi_alloc_socket(pwsi, sock);

	dprintf_winsock(stddeb,"\tcreated %i (handle %04x)\n", sock, (UINT16)WS_PTR2HANDLE(pnew));

        if( pnew ) 
	{
	    pnew->flags |= WS_FD_INACTIVE;
	    return (SOCKET16)WS_PTR2HANDLE(pnew);
	}
	
        close(sock);
        pwsi->err = WSAENOBUFS;
        return INVALID_SOCKET32;
    }

    if (errno == EPERM) /* raw socket denied */
    {
        fprintf(stderr, "WS_SOCKET: not enough privileges\n");
        pwsi->err = WSAESOCKTNOSUPPORT;
    } else pwsi->err = wsaErrno();
  }
 
  dprintf_winsock(stddeb, "\t\tfailed!\n");
  return INVALID_SOCKET32;
}

/***********************************************************************
 *              socket()		(WINSOCK.23)
 */
SOCKET16 WINAPI WINSOCK_socket16(INT16 af, INT16 type, INT16 protocol)
{
    return (SOCKET16)WINSOCK_socket32( af, type, protocol );
}
    

/* ----------------------------------- DNS services
 *
 * IMPORTANT: 16-bit API structures have SEGPTR pointers inside them.
 * Also, we have to use wsock32 stubs to convert structures and
 * error codes from Unix to WSA, hence there is no direct mapping in 
 * the relay32/wsock32.spec.
 */

static char*	NULL_STRING = "NULL";

/***********************************************************************
 *		gethostbyaddr()		(WINSOCK.51)(WSOCK32.51)
 */
static struct WIN_hostent* __ws_gethostbyaddr(const char *addr, int len, int type, int dup_flag)
{
    LPWSINFO      	pwsi = wsi_find(GetCurrentTask());

    if( pwsi )
    {
	struct hostent*	host;
	if( (host = gethostbyaddr(addr, len, type)) != NULL )
	    if( WS_dup_he(pwsi, host, dup_flag) )
		return (struct WIN_hostent*)(pwsi->buffer);
	    else 
		pwsi->err = WSAENOBUFS;
	else 
	    pwsi->err = (h_errno < 0) ? wsaErrno() : wsaHerrno();
    }
    return NULL;
}

SEGPTR WINAPI WINSOCK_gethostbyaddr16(const char *addr, INT16 len, INT16 type)
{
    struct WIN_hostent* retval;
    dprintf_winsock(stddeb, "WS_GetHostByAddr16: ptr %08x, len %d, type %d\n",
                            (unsigned) addr, len, type);
    retval = __ws_gethostbyaddr( addr, len, type, WS_DUP_SEGPTR );
    return retval ? SEGPTR_GET(retval) : ((SEGPTR)NULL);
}

struct WIN_hostent* WINAPI WINSOCK_gethostbyaddr32(const char *addr, INT32 len,
                                                INT32 type)
{
    dprintf_winsock(stddeb, "WS_GetHostByAddr32: ptr %08x, len %d, type %d\n",
                             (unsigned) addr, len, type);
    return __ws_gethostbyaddr(addr, len, type, WS_DUP_LINEAR);
}

/***********************************************************************
 *		gethostbyname()		(WINSOCK.52)(WSOCK32.52)
 */
static struct WIN_hostent * __ws_gethostbyname(const char *name, int dup_flag)
{
    LPWSINFO              pwsi = wsi_find(GetCurrentTask());

    if( pwsi )
    {
	struct hostent*     host;
	if( (host = gethostbyname(name)) != NULL )
	     if( WS_dup_he(pwsi, host, dup_flag) )
		 return (struct WIN_hostent*)(pwsi->buffer);
	     else pwsi->err = WSAENOBUFS;
	else pwsi->err = (h_errno < 0) ? wsaErrno() : wsaHerrno();
    }
    return NULL;
}

SEGPTR WINAPI WINSOCK_gethostbyname16(const char *name)
{
    struct WIN_hostent* retval;
    dprintf_winsock(stddeb, "WS_GetHostByName16: %s\n", (name)?name:NULL_STRING);
    retval = __ws_gethostbyname( name, WS_DUP_SEGPTR );
    return (retval)? SEGPTR_GET(retval) : ((SEGPTR)NULL) ;
}

struct WIN_hostent* WINAPI WINSOCK_gethostbyname32(const char* name)
{
    dprintf_winsock(stddeb, "WS_GetHostByName32: %s\n", (name)?name:NULL_STRING);
    return __ws_gethostbyname( name, WS_DUP_LINEAR );
}


/***********************************************************************
 *		getprotobyname()	(WINSOCK.53)(WSOCK32.53)
 */
static struct WIN_protoent* __ws_getprotobyname(const char *name, int dup_flag)
{
    LPWSINFO              pwsi = wsi_find(GetCurrentTask());

    if( pwsi )
    {
	struct protoent*     proto;
	if( (proto = getprotobyname(name)) != NULL )
	    if( WS_dup_pe(pwsi, proto, dup_flag) )
		return (struct WIN_protoent*)(pwsi->buffer);
	    else pwsi->err = WSAENOBUFS;
	else pwsi->err = (h_errno < 0) ? wsaErrno() : wsaHerrno();
    }
    return NULL;
}

SEGPTR WINAPI WINSOCK_getprotobyname16(const char *name)
{
    struct WIN_protoent* retval;
    dprintf_winsock(stddeb, "WS_GetProtoByName16: %s\n", (name)?name:NULL_STRING);
    retval = __ws_getprotobyname(name, WS_DUP_SEGPTR);
    return retval ? SEGPTR_GET(retval) : ((SEGPTR)NULL);
}

struct WIN_protoent* WINAPI WINSOCK_getprotobyname32(const char* name)
{
    dprintf_winsock(stddeb, "WS_GetProtoByName32: %s\n", (name)?name:NULL_STRING);
    return __ws_getprotobyname(name, WS_DUP_LINEAR);
}


/***********************************************************************
 *		getprotobynumber()	(WINSOCK.54)(WSOCK32.54)
 */
static struct WIN_protoent* __ws_getprotobynumber(int number, int dup_flag)
{
    LPWSINFO              pwsi = wsi_find(GetCurrentTask());

    if( pwsi )
    {
	struct protoent*     proto;
	if( (proto = getprotobynumber(number)) != NULL )
	    if( WS_dup_pe(pwsi, proto, dup_flag) )
		return (struct WIN_protoent*)(pwsi->buffer);
	    else pwsi->err = WSAENOBUFS;
	else pwsi->err = WSANO_DATA;
    }
    return NULL;
}

SEGPTR WINAPI WINSOCK_getprotobynumber16(INT16 number)
{
    struct WIN_protoent* retval;
    dprintf_winsock(stddeb, "WS_GetProtoByNumber16: %i\n", number);
    retval = __ws_getprotobynumber(number, WS_DUP_SEGPTR);
    return retval ? SEGPTR_GET(retval) : ((SEGPTR)NULL);
}

struct WIN_protoent* WINAPI WINSOCK_getprotobynumber32(INT32 number)
{
    dprintf_winsock(stddeb, "WS_GetProtoByNumber32: %i\n", number);
    return __ws_getprotobynumber(number, WS_DUP_LINEAR);
}


/***********************************************************************
 *		getservbyname()		(WINSOCK.55)(WSOCK32.55)
 */
struct WIN_servent* __ws_getservbyname(const char *name, const char *proto, int dup_flag)
{
    LPWSINFO              pwsi = wsi_find(GetCurrentTask());

    if( pwsi )
    {
	struct servent*     serv;
	int i = wsi_strtolo( pwsi, name, proto );

	if( i )
	    if( (serv = getservbyname(pwsi->buffer, pwsi->buffer + i)) != NULL )
		if( WS_dup_se(pwsi, serv, dup_flag) )
		    return (struct WIN_servent*)(pwsi->buffer);
		else pwsi->err = WSAENOBUFS;
	    else pwsi->err = (h_errno < 0) ? wsaErrno() : wsaHerrno();
	else pwsi->err = WSAENOBUFS;
    }
    return NULL;
}

SEGPTR WINAPI WINSOCK_getservbyname16(const char *name, const char *proto)
{
    struct WIN_servent* retval;
    dprintf_winsock(stddeb, "WS_GetServByName16: '%s', '%s'\n",
                            (name)?name:NULL_STRING, (proto)?proto:NULL_STRING);
    retval = __ws_getservbyname(name, proto, WS_DUP_SEGPTR);
    return retval ? SEGPTR_GET(retval) : ((SEGPTR)NULL);
}

struct WIN_servent* WINAPI WINSOCK_getservbyname32(const char *name, const char *proto)
{
    dprintf_winsock(stddeb, "WS_GetServByName32: '%s', '%s'\n",
                            (name)?name:NULL_STRING, (proto)?proto:NULL_STRING);
    return __ws_getservbyname(name, proto, WS_DUP_LINEAR);
}


/***********************************************************************
 *		getservbyport()		(WINSOCK.56)(WSOCK32.56)
 */
static struct WIN_servent* __ws_getservbyport(int port, const char* proto, int dup_flag)
{
    LPWSINFO              pwsi = wsi_find(GetCurrentTask());

    if( pwsi )
    {
	struct servent*     serv;
	int i = wsi_strtolo( pwsi, proto, NULL );

	if( i )
	    if( (serv = getservbyport(port, pwsi->buffer)) != NULL )
		if( WS_dup_se(pwsi, serv, dup_flag) )
		    return (struct WIN_servent*)(pwsi->buffer);
		else pwsi->err = WSAENOBUFS;
	    else pwsi->err = (h_errno < 0) ? wsaErrno() : wsaHerrno();
	else pwsi->err = WSAENOBUFS;
    }
    return NULL;
}

SEGPTR WINAPI WINSOCK_getservbyport16(INT16 port, const char *proto)
{
    struct WIN_servent* retval;
    dprintf_winsock(stddeb, "WS_GetServByPort16: %i, '%s'\n",
                            (int)port, (proto)?proto:NULL_STRING);
    retval = __ws_getservbyport(port, proto, WS_DUP_SEGPTR);
    return retval ? SEGPTR_GET(retval) : ((SEGPTR)NULL);
}

struct WIN_servent* WINAPI WINSOCK_getservbyport32(INT32 port, const char *proto)
{
    dprintf_winsock(stddeb, "WS_GetServByPort32: %i, '%s'\n",
                            (int)port, (proto)?proto:NULL_STRING);
    return __ws_getservbyport(port, proto, WS_DUP_LINEAR);
}


/***********************************************************************
 *              gethostname()           (WSOCK32.57)
 */
INT32 WINAPI WINSOCK_gethostname32(char *name, INT32 namelen)
{
    LPWSINFO              pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_GetHostName(%08x): name %s, len %d\n",
                          (unsigned)pwsi, (name)?name:NULL_STRING, namelen);
    if( pwsi )
    {
	if (gethostname(name, namelen) == 0) return 0;
	pwsi->err = (errno == EINVAL) ? WSAEFAULT : wsaErrno();
    }
    return SOCKET_ERROR;
}

/***********************************************************************
 *              gethostname()           (WINSOCK.57)
 */
INT16 WINAPI WINSOCK_gethostname16(char *name, INT16 namelen)
{
    return (INT16)WINSOCK_gethostname32(name, namelen);
}


/* ------------------------------------- Windows sockets extensions -- *
 *								       *
 * ------------------------------------------------------------------- */


/***********************************************************************
 *          Asynchronous DNS services
 */

/* winsock_dns.c */
extern HANDLE16 __WSAsyncDBQuery(LPWSINFO pwsi, HWND32 hWnd, UINT32 uMsg, INT32 type, LPCSTR init,
				 INT32 len, LPCSTR proto, void* sbuf, INT32 buflen, UINT32 flag);

/***********************************************************************
 *       WSAAsyncGetHostByAddr()	(WINSOCK.102)
 */
HANDLE16 WINAPI WSAAsyncGetHostByAddr16(HWND16 hWnd, UINT16 uMsg, LPCSTR addr,
                               INT16 len, INT16 type, SEGPTR sbuf, INT16 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetHostByAddr16(%08x): hwnd %04x, msg %04x, addr %08x[%i]\n",
                          (unsigned)pwsi, hWnd, uMsg, (unsigned)addr , len );

  if( pwsi ) 
    return __WSAsyncDBQuery(pwsi, hWnd, uMsg, type, addr, len,
			    NULL, (void*)sbuf, buflen, WSMSG_ASYNC_HOSTBYADDR );
  return 0;
}

/***********************************************************************
 *       WSAAsyncGetHostByAddr()        (WSOCK32.102)
 */
HANDLE32 WINAPI WSAAsyncGetHostByAddr32(HWND32 hWnd, UINT32 uMsg, LPCSTR addr,
                               INT32 len, INT32 type, LPSTR sbuf, INT32 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetHostByAddr32(%08x): hwnd %04x, msg %08x, addr %08x[%i]\n",
                          (unsigned)pwsi, (HWND16)hWnd, uMsg, (unsigned)addr , len );

  if( pwsi )
    return __WSAsyncDBQuery(pwsi, hWnd, uMsg, type, addr, len,
                            NULL, (void*)sbuf, buflen, WSMSG_ASYNC_HOSTBYADDR | WSMSG_ASYNC_WIN32);
  return 0;
}


/***********************************************************************
 *       WSAAsyncGetHostByName()	(WINSOCK.103)
 */
HANDLE16 WINAPI WSAAsyncGetHostByName16(HWND16 hWnd, UINT16 uMsg, LPCSTR name, 
                                      SEGPTR sbuf, INT16 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetHostByName16(%08x): hwnd %04x, msg %04x, host %s, 
buffer %i\n", (unsigned)pwsi, hWnd, uMsg, (name)?name:NULL_STRING, (int)buflen );

  if( pwsi )
    return __WSAsyncDBQuery(pwsi, hWnd, uMsg, 0, name, 0,
                            NULL, (void*)sbuf, buflen, WSMSG_ASYNC_HOSTBYNAME );
  return 0;
}                     

/***********************************************************************
 *       WSAAsyncGetHostByName32()	(WSOCK32.103)
 */
HANDLE32 WINAPI WSAAsyncGetHostByName32(HWND32 hWnd, UINT32 uMsg, LPCSTR name, 
					LPSTR sbuf, INT32 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());
  dprintf_winsock(stddeb, "WS_AsyncGetHostByName32(%08x): hwnd %04x, msg %08x, host %s, 
buffer %i\n", (unsigned)pwsi, (HWND16)hWnd, uMsg, (name)?name:NULL_STRING, (int)buflen );
  if( pwsi )
    return __WSAsyncDBQuery(pwsi, hWnd, uMsg, 0, name, 0,
 			    NULL, (void*)sbuf, buflen, WSMSG_ASYNC_HOSTBYNAME | WSMSG_ASYNC_WIN32);
  return 0;
}                     


/***********************************************************************
 *       WSAAsyncGetProtoByName()	(WINSOCK.105)
 */
HANDLE16 WINAPI WSAAsyncGetProtoByName16(HWND16 hWnd, UINT16 uMsg, LPCSTR name, 
                                         SEGPTR sbuf, INT16 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetProtoByName16(%08x): hwnd %04x, msg %08x, protocol %s\n",
                          (unsigned)pwsi, (HWND16)hWnd, uMsg, (name)?name:NULL_STRING );

  if( pwsi )
    return __WSAsyncDBQuery(pwsi, hWnd, uMsg, 0, name, 0,
                            NULL, (void*)sbuf, buflen, WSMSG_ASYNC_PROTOBYNAME );
  return 0;
}

/***********************************************************************
 *       WSAAsyncGetProtoByName()       (WSOCK32.105)
 */
HANDLE32 WINAPI WSAAsyncGetProtoByName32(HWND32 hWnd, UINT32 uMsg, LPCSTR name,
                                         LPSTR sbuf, INT32 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetProtoByName32(%08x): hwnd %04x, msg %08x, protocol %s\n",
                          (unsigned)pwsi, (HWND16)hWnd, uMsg, (name)?name:NULL_STRING );

  if( pwsi )
    return __WSAsyncDBQuery(pwsi, hWnd, uMsg, 0, name, 0,
                            NULL, (void*)sbuf, buflen, WSMSG_ASYNC_PROTOBYNAME | WSMSG_ASYNC_WIN32);
  return 0;
}


/***********************************************************************
 *       WSAAsyncGetProtoByNumber()	(WINSOCK.104)
 */
HANDLE16 WINAPI WSAAsyncGetProtoByNumber16(HWND16 hWnd, UINT16 uMsg, INT16 number, 
                                           SEGPTR sbuf, INT16 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetProtoByNumber16(%08x): hwnd %04x, msg %04x, num %i\n",
                          (unsigned)pwsi, hWnd, uMsg, number );

  if( pwsi )
    return __WSAsyncDBQuery(pwsi, hWnd, uMsg, number, NULL, 0,
                            NULL, (void*)sbuf, buflen, WSMSG_ASYNC_PROTOBYNUM );
  return 0;
}

/***********************************************************************
 *       WSAAsyncGetProtoByNumber()     (WSOCK32.104)
 */
HANDLE32 WINAPI WSAAsyncGetProtoByNumber32(HWND32 hWnd, UINT32 uMsg, INT32 number,
                                           LPSTR sbuf, INT32 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetProtoByNumber32(%08x): hwnd %04x, msg %08x, num %i\n",
                          (unsigned)pwsi, (HWND16)hWnd, uMsg, number );

  if( pwsi )
    return __WSAsyncDBQuery(pwsi, hWnd, uMsg, number, NULL, 0,
                            NULL, (void*)sbuf, buflen, WSMSG_ASYNC_PROTOBYNUM | WSMSG_ASYNC_WIN32);
  return 0;
}


/***********************************************************************
 *       WSAAsyncGetServByName()	(WINSOCK.107)
 */
HANDLE16 WINAPI WSAAsyncGetServByName16(HWND16 hWnd, UINT16 uMsg, LPCSTR name, 
                                        LPCSTR proto, SEGPTR sbuf, INT16 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetServByName16(%08x): hwnd %04x, msg %04x, name %s, proto %s\n",
                   (unsigned)pwsi, hWnd, uMsg, (name)?name:NULL_STRING, (proto)?proto:NULL_STRING );

  if( pwsi )
  {
      int i = wsi_strtolo( pwsi, name, proto );

      if( i )
          return __WSAsyncDBQuery(pwsi, hWnd, uMsg, 0, pwsi->buffer, 0,
                    pwsi->buffer + i, (void*)sbuf, buflen, WSMSG_ASYNC_SERVBYNAME );
  }
  return 0;
}

/***********************************************************************
 *       WSAAsyncGetServByName()        (WSOCK32.107)
 */
HANDLE32 WINAPI WSAAsyncGetServByName32(HWND32 hWnd, UINT32 uMsg, LPCSTR name,
                                        LPCSTR proto, LPSTR sbuf, INT32 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetServByName32(%08x): hwnd %04x, msg %08x, name %s, proto %s\n",
           (unsigned)pwsi, (HWND16)hWnd, uMsg, (name)?name:NULL_STRING, (proto)?proto:NULL_STRING );
  if( pwsi )
  {
      int i = wsi_strtolo( pwsi, name, proto );

      if( i )
          return __WSAsyncDBQuery(pwsi, hWnd, uMsg, 0, pwsi->buffer, 0,
                 pwsi->buffer + i, (void*)sbuf, buflen, WSMSG_ASYNC_SERVBYNAME | WSMSG_ASYNC_WIN32);
  }
  return 0;
}


/***********************************************************************
 *       WSAAsyncGetServByPort()	(WINSOCK.106)
 */
HANDLE16 WINAPI WSAAsyncGetServByPort16(HWND16 hWnd, UINT16 uMsg, INT16 port, 
                                        LPCSTR proto, SEGPTR sbuf, INT16 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetServByPort16(%08x): hwnd %04x, msg %04x, port %i, proto %s\n",
                           (unsigned)pwsi, hWnd, uMsg, port, (proto)?proto:NULL_STRING );

  if( pwsi )
  {
      int i = wsi_strtolo( pwsi, proto, NULL );

      if( i )
	  return __WSAsyncDBQuery(pwsi, hWnd, uMsg, port, pwsi->buffer, 0,
		NULL, (void*)sbuf, buflen, WSMSG_ASYNC_SERVBYPORT );
  }
  return 0;
}

/***********************************************************************
 *       WSAAsyncGetServByPort()        (WSOCK32.106)
 */
HANDLE32 WINAPI WSAAsyncGetServByPort32(HWND32 hWnd, UINT32 uMsg, INT32 port,
                                        LPCSTR proto, LPSTR sbuf, INT32 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetServByPort32(%08x): hwnd %04x, msg %08x, port %i, proto %s\n",
                           (unsigned)pwsi, (HWND16)hWnd, uMsg, port, (proto)?proto:NULL_STRING );

  if( pwsi )
  {
      int i = wsi_strtolo( pwsi, proto, NULL );

      if( i )
	  return __WSAsyncDBQuery(pwsi, hWnd, uMsg, port, pwsi->buffer, 0,
		 NULL, (void*)sbuf, buflen, WSMSG_ASYNC_SERVBYPORT | WSMSG_ASYNC_WIN32);
  }
  return 0;
}


/***********************************************************************
 *       WSACancelAsyncRequest()	(WINSOCK.108)(WSOCK32.109)
 */
INT32 WINAPI WSACancelAsyncRequest32(HANDLE32 hAsyncTaskHandle)
{
    INT32		retVal = SOCKET_ERROR;
    LPWSINFO		pwsi = wsi_find(GetCurrentTask());
    ws_async_op*	p_aop = (ws_async_op*)WS_HANDLE2PTR(hAsyncTaskHandle);

    dprintf_winsock(stddeb, "WS_CancelAsyncRequest(%08x): handle %08x\n", 
			   (unsigned)pwsi, hAsyncTaskHandle);
    if( pwsi )
    {
	SIGNAL_MaskAsyncEvents( TRUE );	/* block SIGIO */
	if( WINSOCK_cancel_async_op(p_aop) )
	{
	    WS_FREE(p_aop);
	    pwsi->num_async_rq--;
	    retVal = 0;
	}
	else pwsi->err = WSAEINVAL;
	SIGNAL_MaskAsyncEvents( FALSE );
    }
    return retVal;
}

INT16 WINAPI WSACancelAsyncRequest16(HANDLE16 hAsyncTaskHandle)
{
    return (HANDLE16)WSACancelAsyncRequest16((HANDLE32)hAsyncTaskHandle);
}

/***********************************************************************
 *      WSAAsyncSelect()		(WINSOCK.101)(WSOCK32.101)
 */

static ws_select_op* __ws_select_list = NULL;

BOOL32 WINSOCK_HandleIO( int* max_fd, int num_pending, 
			 fd_set pending_set[3], fd_set event_set[3] )
{
    /* This function is called by the event dispatcher
     * with the pending_set[] containing the result of select() and
     * the event_set[] containing all fd that are being watched */

    ws_select_op*	psop = __ws_select_list;
    BOOL32		bPost = FALSE;
    DWORD		dwEvent, dwErrBytes;
    int			num_posted;

    dprintf_winsock(stddeb,"WINSOCK_HandleIO: %i pending descriptors\n", num_pending );

    for( num_posted = dwEvent = 0 ; psop; psop = psop->next )
    {
	unsigned	flags = psop->pws->flags;
	int		fd = psop->pws->fd;
	int		r, w, e;

	w = 0;
	if( (r = FD_ISSET( fd, &pending_set[EVENT_IO_READ] )) ||
	    (w = FD_ISSET( fd, &pending_set[EVENT_IO_WRITE] )) ||
	    (e = FD_ISSET( fd, &pending_set[EVENT_IO_EXCEPT] )) )
	{
	    /* This code removes WS_FD flags on one-shot events (WS_FD_CLOSE, 
	     * WS_FD_CONNECT), otherwise it clears descriptors in the io_set.
	     * Reenabling calls turn them back on.
	     */

	    dprintf_winsock(stddeb,"\tchecking psop = 0x%08x\n", (unsigned) psop );

	    num_pending--;

	    /* Now figure out what kind of event we've got. The worst problem
	     * we have to contend with is that some out of control applications 
	     * really want to use mutually exclusive AsyncSelect() flags all at
	     * the same time.
	     */

	    if((flags & WS_FD_ACCEPT) && (flags & WS_FD_LISTENING))
	    {
		/* WS_FD_ACCEPT is valid only if the socket is in the
		 * listening state */

		FD_CLR( fd, &event_set[EVENT_IO_WRITE] );
		if( r )
		{
		    FD_CLR( fd, &event_set[EVENT_IO_READ] ); /* reenabled by the next accept() */
		    dwEvent = WSAMAKESELECTREPLY( WS_FD_ACCEPT, 0 );
		    bPost = TRUE;
		} 
		else continue;
	    }
	    else if( flags & WS_FD_CONNECT )
	    {
		/* connecting socket */

		if( w || (w = FD_ISSET( fd, &pending_set[EVENT_IO_WRITE] )) )
		{
		    /* ready to write means that socket is connected 
		     *
		     * FIXME: Netscape calls AsyncSelect( s, ... WS_FD_CONNECT .. )
		     * right after s = socket() and somehow "s" becomes writeable 
		     * before it goes through connect()!?!?
		     */

		    psop->pws->flags |= WS_FD_CONNECTED;
		    psop->pws->flags &= ~(WS_FD_CONNECT | WS_FD_INACTIVE);
		    dwEvent = WSAMAKESELECTREPLY( WS_FD_CONNECT, 0 );

		    if( flags & (WS_FD_READ | WS_FD_CLOSE))
			FD_SET( fd, &event_set[EVENT_IO_READ] );
		    else 
			FD_CLR( fd, &event_set[EVENT_IO_READ] );
		    if( flags & WS_FD_WRITE ) 
			FD_SET( fd, &event_set[EVENT_IO_WRITE] );
		    else 
			FD_CLR( fd, &event_set[EVENT_IO_WRITE] );
		    bPost = TRUE;
		}
		else if( r )
		{
		    /* failure - do read() to get correct errno */

		    if( read( fd, &dwErrBytes, sizeof(dwErrBytes) ) == -1 )
		    {
			dwEvent = WSAMAKESELECTREPLY( WS_FD_CONNECT, wsaErrno() );
			bPost = TRUE;
		    }
		}
		/* otherwise bPost stays FALSE, should probably clear event_set  */
	    }
	    else 
	    {
		/* connected socket, no WS_FD_OOB code for now. */

		if( flags & WS_FD_WRITE &&
		   (w || (w = FD_ISSET( fd, &pending_set[EVENT_IO_WRITE] ))) )
		{
		    /* this will be reenabled when send() or sendto() fail with
		     * WSAEWOULDBLOCK */

		    if( PostMessage32A( psop->hWnd, psop->uMsg, (WPARAM32)WS_PTR2HANDLE(psop->pws), 
			              (LPARAM)WSAMAKESELECTREPLY( WS_FD_WRITE, 0 ) ) )
		    {
			dprintf_winsock(stddeb, "\t    hwnd %04x - %04x, %08x\n",
                                psop->hWnd, psop->uMsg, (unsigned)MAKELONG(WS_FD_WRITE, 0) );
			FD_CLR( fd, &event_set[EVENT_IO_WRITE] );
			num_posted++;
		    }
		}

		if( r && (flags & (WS_FD_READ | WS_FD_CLOSE)) )
		{
		    int val = (flags & WS_FD_RAW);

		     /* WS_FD_RAW is set by the WSAAsyncSelect() init */

		    bPost = TRUE;
		    if( !val && ioctl( fd, FIONREAD, (char*)&dwErrBytes) == -1 )
		    {
			/* weirdness */

			dwEvent = WSAMAKESELECTREPLY( WS_FD_READ, wsaErrno() );
		    }
		    else if( val || dwErrBytes )
		    {
			/* got pending data, will be reenabled by recv() or recvfrom() */

			FD_CLR( fd, &event_set[EVENT_IO_READ] );
			dwEvent = WSAMAKESELECTREPLY( WS_FD_READ, 0 );
		    }
		    else
		    {
			/* 0 bytes to read - connection reset by peer? */

			do
			    val = read( fd, (char*)&dwErrBytes, sizeof(dwErrBytes));
			while( errno == EINTR );
			if( errno != EWOULDBLOCK )
			{
			    switch( val )
			    {
				case  0: errno = ENETDOWN;	/* soft reset, fall through */
				case -1: 			/* hard reset */
					 dwEvent = WSAMAKESELECTREPLY( WS_FD_CLOSE, wsaErrno() );
					 break;

				default: bPost = FALSE;
					 continue;		/* FIXME: this is real bad */
			    }
			}
			else { bPost = FALSE; continue; }	/* more weirdness */

			/* this is it, this socket is closed */

			psop->pws->flags &= ~(WS_FD_READ | WS_FD_CLOSE | WS_FD_WRITE);
			FD_CLR( fd, &event_set[EVENT_IO_READ] );
			FD_CLR( fd, &event_set[EVENT_IO_WRITE] );

			if( *max_fd == (fd + 1) ) (*max_fd)--;
		    }
		}
	    }

	    if( bPost )
	    {
		dprintf_winsock(stddeb, "\t    hwnd %04x - %04x, %08x\n", 
				psop->hWnd, psop->uMsg, (unsigned)dwEvent );
		PostMessage32A( psop->hWnd, psop->uMsg, 
			      (WPARAM32)WS_PTR2HANDLE(psop->pws), (LPARAM)dwEvent );
		bPost = FALSE;
		num_posted++;
	    }
	}
	if( num_pending <= 0 ) break;
    }

    dprintf_winsock(stddeb, "\tdone, %i posted events\n", num_posted );
    return ( num_posted ) ? TRUE : FALSE;
}

INT32 WINAPI WSAAsyncSelect32(SOCKET32 s, HWND32 hWnd, UINT32 uMsg, UINT32 lEvent)
{
    ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
    LPWSINFO      pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_AsyncSelect(%08x): %04x, hWnd %04x, uMsg %08x, event %08x\n",
			  (unsigned)pwsi, (SOCKET16)s, (HWND16)hWnd, uMsg, (unsigned)lEvent );
    if( _check_ws(pwsi, pws) )
    {
	ws_select_op* psop;

	if( (psop = pws->psop) )
	{
	    /* delete previous control struct */

	    if( psop == __ws_select_list )
		__ws_select_list = psop->next;
	    else
		psop->prev->next = psop->next; 
	    if( psop->next ) psop->next->prev = psop->prev;

	    if( pws->flags & (WS_FD_ACCEPT | WS_FD_CONNECT | WS_FD_READ | WS_FD_CLOSE) )
		EVENT_DeleteIO( pws->fd, EVENT_IO_READ );
	    if( pws->flags & (WS_FD_CONNECT | WS_FD_WRITE) )
		EVENT_DeleteIO( pws->fd, EVENT_IO_WRITE );

	    dprintf_winsock(stddeb,"\tremoving psop = 0x%08x\n", (unsigned) psop );

	    WS_FREE( pws->psop );
	    pws->flags &= ~(WS_FD_RAW | WS_FD_ACCEPT | WS_FD_CONNECT | 
			    WS_FD_READ | WS_FD_WRITE | WS_FD_CLOSE);
	    pws->psop = NULL;
	}

	if( lEvent )
	{
	    psop = (ws_select_op*)WS_ALLOC(sizeof(ws_select_op));
	    if( psop )
	    {
		int sock_type, bytes = sizeof(int);

		WINSOCK_unblock_io( pws->fd, TRUE );

		psop->prev = NULL;
		psop->next = __ws_select_list;
		if( __ws_select_list )
		    __ws_select_list->prev = psop;
		__ws_select_list = psop;
		
		psop->pws = pws;
		psop->hWnd = hWnd;
		psop->uMsg = uMsg;

		pws->psop = psop;
		pws->flags |= (0x0000FFFF & lEvent);
		getsockopt(pws->fd, SOL_SOCKET, SO_TYPE, &sock_type, &bytes);
		if( sock_type == SOCK_RAW ) pws->flags |= WS_FD_RAW;

		if( lEvent & (WS_FD_ACCEPT | WS_FD_CONNECT | WS_FD_READ | WS_FD_CLOSE) )
		    EVENT_AddIO( pws->fd, EVENT_IO_READ );
		if( lEvent & (WS_FD_CONNECT | WS_FD_WRITE) )
		    EVENT_AddIO( pws->fd, EVENT_IO_WRITE );

		/* TODO: handle WS_FD_ACCEPT right away if the socket is readable */

		dprintf_winsock(stddeb,"\tcreating psop = 0x%08x\n", (unsigned)psop );

		return 0; /* success */
	    }
	    else pwsi->err = WSAENOBUFS;
	} 
	else return 0;
    } 
    else if( pwsi ) pwsi->err = WSAEINVAL;
    return SOCKET_ERROR; 
}

INT16 WINAPI WSAAsyncSelect16(SOCKET16 s, HWND16 hWnd, UINT16 wMsg, UINT32 lEvent)
{
    return (INT16)WSAAsyncSelect32( s, hWnd, wMsg, lEvent );
}


/***********************************************************************
 *	__WSAFDIsSet()			(WINSOCK.151)
 */
INT16 WINAPI __WSAFDIsSet16(SOCKET16 s, ws_fd_set16 *set)
{
  int i = set->fd_count;
  
  dprintf_winsock(stddeb, "__WSAFDIsSet16(%d,%8lx(%i))\n", s,(unsigned long)set, i);
    
  while (i--)
      if (set->fd_array[i] == s) return 1;
  return 0;
}                                                            

/***********************************************************************
 *      __WSAFDIsSet()			(WSOCK32.151)
 */
INT32 WINAPI __WSAFDIsSet32(SOCKET32 s, ws_fd_set32 *set)
{
  int i = set->fd_count;

  dprintf_winsock(stddeb, "__WSAFDIsSet32(%d,%8lx(%i))\n", s,(unsigned long)set, i);

  while (i--)
      if (set->fd_array[i] == s) return 1;
  return 0;
}

/***********************************************************************
 *      WSAIsBlocking()			(WINSOCK.114)(WSOCK32.114)
 */
BOOL32 WINAPI WSAIsBlocking(void)
{
  /* By default WinSock should set all its sockets to non-blocking mode
   * and poll in PeekMessage loop when processing "blocking" ones. This 
   * function is supposed to tell if the program is in this loop. Our 
   * blocking calls are truly blocking so we always return FALSE.
   *
   * Note: It is allowed to call this function without prior WSAStartup().
   */

  dprintf_winsock(stddeb, "WS_IsBlocking()\n");
  return FALSE;
}

/***********************************************************************
 *      WSACancelBlockingCall()		(WINSOCK.113)(WSOCK32.113)
 */
INT32 WINAPI WSACancelBlockingCall(void)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_CancelBlockingCall(%08x)\n", (unsigned)pwsi);

  if( pwsi ) return 0;
  return SOCKET_ERROR;
}


/***********************************************************************
 *      WSASetBlockingHook16()		(WINSOCK.109)
 */
FARPROC16 WINAPI WSASetBlockingHook16(FARPROC16 lpBlockFunc)
{
  FARPROC16		prev;
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_SetBlockingHook16(%08x): hook %08x\n", 
			  (unsigned)pwsi, (unsigned) lpBlockFunc);
  if( pwsi ) 
  { 
      prev = (FARPROC16)pwsi->blocking_hook; 
      pwsi->blocking_hook = (DWORD)lpBlockFunc; 
      pwsi->flags &= ~WSI_BLOCKINGHOOK32;
      return prev; 
  }
  return 0;
}


/***********************************************************************
 *      WSASetBlockingHook32()
 */
FARPROC32 WINAPI WSASetBlockingHook32(FARPROC32 lpBlockFunc)
{
  FARPROC32             prev;
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_SetBlockingHook32(%08x): hook %08x\n",
                          (unsigned)pwsi, (unsigned) lpBlockFunc);
  if( pwsi ) {
      prev = (FARPROC32)pwsi->blocking_hook;
      pwsi->blocking_hook = (DWORD)lpBlockFunc;
      pwsi->flags |= WSI_BLOCKINGHOOK32;
      return prev;
  }
  return NULL;
}


/***********************************************************************
 *      WSAUnhookBlockingHook16()	(WINSOCK.110)
 */
INT16 WINAPI WSAUnhookBlockingHook16(void)
{
    LPWSINFO              pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_UnhookBlockingHook16(%08x)\n", (unsigned)pwsi);
    if( pwsi ) return (INT16)(pwsi->blocking_hook = 0);
    return SOCKET_ERROR;
}


/***********************************************************************
 *      WSAUnhookBlockingHook32()
 */
INT32 WINAPI WSAUnhookBlockingHook32(void)
{
    LPWSINFO              pwsi = wsi_find(GetCurrentTask());

    dprintf_winsock(stddeb, "WS_UnhookBlockingHook32(%08x)\n", (unsigned)pwsi);
    if( pwsi )
    {
	pwsi->blocking_hook = 0;
	pwsi->flags &= ~WSI_BLOCKINGHOOK32;
	return 0;
    }
    return SOCKET_ERROR;
}

/*
 *      TCP/IP action codes.
 */


#define WSCNTL_TCPIP_QUERY_INFO             0x00000000
#define WSCNTL_TCPIP_SET_INFO               0x00000001
#define WSCNTL_TCPIP_ICMP_ECHO              0x00000002
#define WSCNTL_TCPIP_TEST                   0x00000003


/***********************************************************************
 *      WsControl()
 *
 * WsControl seems to be an undocumented Win95 function. A lot of 
 * discussion about WsControl can be found on the net, e.g.
 * Subject:      Re: WSOCK32.DLL WsControl Exported Function
 * From:         "Peter Rindfuss" <rindfuss-s@medea.wz-berlin.de>
 * Date:         1997/08/17
 */

DWORD WINAPI WsControl(DWORD protocoll,DWORD action,
		      LPVOID inbuf,LPDWORD inbuflen,
                      LPVOID outbuf,LPDWORD outbuflen) 
{

  switch (action) {
  case WSCNTL_TCPIP_ICMP_ECHO:
    {
      unsigned int addr = *(unsigned int*)inbuf;
#if 0
      int timeout= *(unsigned int*)(inbuf+4);
      short x1 = *(unsigned short*)(inbuf+8);
      short sendbufsize = *(unsigned short*)(inbuf+10);
      char x2 = *(unsigned char*)(inbuf+12);
      char ttl = *(unsigned char*)(inbuf+13);
      char service = *(unsigned char*)(inbuf+14);
      char type= *(unsigned char*)(inbuf+15); /* 0x2: don't fragment*/
#endif      
      
      fprintf(stdnimp,"WsControl(ICMP_ECHO) to 0x%08x stub \n",
	      addr);
      break;
    }
  default:
    fprintf(stdnimp,"WsControl(%lx,%lx,%p,%p,%p,%p) stub\n",
	    protocoll,action,inbuf,inbuflen,outbuf,outbuflen);
  }
  return FALSE;
}
/*********************************************************
 *       WS_s_perror         WSOCK32.1108 
 */
void WINAPI WS_s_perror(LPCSTR message)
{
    fprintf(stdnimp,"s_perror %s stub\n",message);
	return;
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

int WS_dup_he(LPWSINFO pwsi, struct hostent* p_he, int flag)
{
   /* Convert hostent structure into ws_hostent so that the data fits 
    * into pwsi->buffer. Internal pointers can be linear, SEGPTR, or 
    * relative to pwsi->buffer depending on "flag" value. Returns size
    * of the data copied (also in the pwsi->buflen).
    */

   int size = hostent_size(p_he);
   if( size )
   {
     struct ws_hostent* p_to;
     char* p_name,*p_aliases,*p_addr,*p_base,*p;

     _check_buffer(pwsi, size);
     p_to = (struct ws_hostent*)pwsi->buffer;
     p = pwsi->buffer;
     p_base = (flag & WS_DUP_OFFSET) ? NULL
				     : ((flag & WS_DUP_SEGPTR) ? (char*)SEGPTR_GET(p) : p);
     p += sizeof(struct ws_hostent);
     p_name = p;
     strcpy(p, p_he->h_name); p += strlen(p) + 1;
     p_aliases = p;
     p += list_dup(p_he->h_aliases, p, p_base + (p - pwsi->buffer), 0);
     p_addr = p;
     list_dup(p_he->h_addr_list, p, p_base + (p - pwsi->buffer), p_he->h_length);

     p_to->h_addrtype = (INT16)p_he->h_addrtype; 
     p_to->h_length = (INT16)p_he->h_length;
     p_to->h_name = (SEGPTR)(p_base + (p_name - pwsi->buffer));
     p_to->h_aliases = (SEGPTR)(p_base + (p_aliases - pwsi->buffer));
     p_to->h_addr_list = (SEGPTR)(p_base + (p_addr - pwsi->buffer));

     size += (sizeof(struct ws_hostent) - sizeof(struct hostent));
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

int WS_dup_pe(LPWSINFO pwsi, struct protoent* p_pe, int flag)
{
   int size = protoent_size(p_pe);
   if( size )
   {
     struct ws_protoent* p_to;
     char* p_name,*p_aliases,*p_base,*p;

     _check_buffer(pwsi, size);
     p_to = (struct ws_protoent*)pwsi->buffer;
     p = pwsi->buffer; 
     p_base = (flag & WS_DUP_OFFSET) ? NULL
				     : ((flag & WS_DUP_SEGPTR) ? (char*)SEGPTR_GET(p) : p);
     p += sizeof(struct ws_protoent);
     p_name = p;
     strcpy(p, p_pe->p_name); p += strlen(p) + 1;
     p_aliases = p;
     list_dup(p_pe->p_aliases, p, p_base + (p - pwsi->buffer), 0);

     p_to->p_proto = (INT16)p_pe->p_proto;
     p_to->p_name = (SEGPTR)(p_base) + (p_name - pwsi->buffer);
     p_to->p_aliases = (SEGPTR)((p_base) + (p_aliases - pwsi->buffer)); 

     size += (sizeof(struct ws_protoent) - sizeof(struct protoent));
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

int WS_dup_se(LPWSINFO pwsi, struct servent* p_se, int flag)
{
   int size = servent_size(p_se);
   if( size )
   {
     struct ws_servent* p_to;
     char* p_name,*p_aliases,*p_proto,*p_base,*p;

     _check_buffer(pwsi, size);
     p_to = (struct ws_servent*)pwsi->buffer;
     p = pwsi->buffer;
     p_base = (flag & WS_DUP_OFFSET) ? NULL 
				     : ((flag & WS_DUP_SEGPTR) ? (char*)SEGPTR_GET(p) : p);
     p += sizeof(struct ws_servent);
     p_name = p;
     strcpy(p, p_se->s_name); p += strlen(p) + 1;
     p_proto = p;
     strcpy(p, p_se->s_proto); p += strlen(p) + 1;
     p_aliases = p;
     list_dup(p_se->s_aliases, p, p_base + (p - pwsi->buffer), 0);

     p_to->s_port = (INT16)p_se->s_port;
     p_to->s_name = (SEGPTR)(p_base + (p_name - pwsi->buffer));
     p_to->s_proto = (SEGPTR)(p_base + (p_proto - pwsi->buffer));
     p_to->s_aliases = (SEGPTR)(p_base + (p_aliases - pwsi->buffer)); 

     size += (sizeof(struct ws_servent) - sizeof(struct servent));
   }
   return size;
}

/* ----------------------------------- error handling */

UINT16 wsaErrno(void)
{
    int	loc_errno = errno; 
#if defined(__FreeBSD__)
       dprintf_winsock(stderr, "winsock: errno %d, (%s).\n", 
                			 errno, strerror(errno));
#else
       dprintf_winsock(stderr, "winsock: errno %d\n", errno);
#endif

    switch(loc_errno)
    {
	case EINTR:		return WSAEINTR;
	case EBADF:		return WSAEBADF;
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
		fprintf(stderr, "winsock: unknown errno %d!\n", errno);
		return WSAEOPNOTSUPP;
    }
}

UINT16 wsaHerrno(void)
{
    int		loc_errno = h_errno;

#if defined(__FreeBSD__)
    dprintf_winsock(stderr, "winsock: h_errno %d, (%s).\n", 
               	    h_errno, strerror(h_errno));
#else
    dprintf_winsock(stderr, "winsock: h_errno %d.\n", h_errno);
#ifndef sun
    if( debugging_winsock )  herror("wine: winsock: wsaherrno");
#endif
#endif

    switch(loc_errno)
    {
	case HOST_NOT_FOUND:	return WSAHOST_NOT_FOUND;
	case TRY_AGAIN:		return WSATRY_AGAIN;
	case NO_RECOVERY:	return WSANO_RECOVERY;
	case NO_DATA:		return WSANO_DATA; 

	case 0:			return 0;
        default:
		fprintf(stderr, "winsock: unknown h_errno %d!\n", h_errno);
		return WSAEOPNOTSUPP;
    }
}


