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
 
#include "config.h"

#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_IPC_H
# include <sys/ipc.h>
#endif
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#if defined(__svr4__) || defined(__sun)
#include <sys/ioccom.h>
#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif
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
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif

#include "wine/winbase16.h"
#include "winsock2.h"
#include "winnt.h"
#include "heap.h"
#include "task.h"
#include "message.h"
#include "miscemu.h"
#include "wine/port.h"
#include "services.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(winsock)

#define DEBUG_SOCKADDR 0
#define dump_sockaddr(a) \
        DPRINTF("sockaddr_in: family %d, address %s, port %d\n", \
                        ((struct sockaddr_in *)a)->sin_family, \
                        inet_ntoa(((struct sockaddr_in *)a)->sin_addr), \
                        ntohs(((struct sockaddr_in *)a)->sin_port))

/* ----------------------------------- internal data */

/* ws_... struct conversion flags */

#define WS_DUP_LINEAR           0x0001
#define WS_DUP_NATIVE           0x0000          /* not used anymore */
#define WS_DUP_OFFSET           0x0002          /* internal pointers are offsets */
#define WS_DUP_SEGPTR           0x0004          /* internal pointers are SEGPTRs */
                                                /* by default, internal pointers are linear */
typedef struct          /* WSAAsyncSelect() control struct */
{
  HANDLE      service, event, sock;
  HWND        hWnd;
  UINT        uMsg;
} ws_select_info;  

#define WS_MAX_SOCKETS_PER_PROCESS      128     /* reasonable guess */
#define WS_MAX_UDP_DATAGRAM             1024

#define WSI_BLOCKINGCALL        0x00000001      /* per-thread info flags */
#define WSI_BLOCKINGHOOK        0x00000002      /* 32-bit callback */

typedef struct _WSINFO
{
  DWORD			dwThisProcess;
  struct _WSINFO       *lpNextIData;

  unsigned		flags;
  INT16			num_startup;		/* reference counter */
  INT16			num_async_rq;
  INT16			last_free;		/* entry in the socket table */
  UINT16		buflen;
  char*			buffer;			/* allocated from SEGPTR heap */
  struct ws_hostent	*he;
  int			helen;
  struct ws_servent	*se;
  int			selen;
  struct ws_protoent	*pe;
  int			pelen;
  char*			dbuffer;		/* buffer for dummies (32 bytes) */

  DWORD			blocking_hook;
} WSINFO, *LPWSINFO;

/* function prototypes */
int WS_dup_he(LPWSINFO pwsi, struct hostent* p_he, int flag);
int WS_dup_pe(LPWSINFO pwsi, struct protoent* p_pe, int flag);
int WS_dup_se(LPWSINFO pwsi, struct servent* p_se, int flag);

UINT16 wsaErrno(void);
UINT16 wsaHerrno(void);
                                                      
static HANDLE 	_WSHeap = 0;

#define WS_ALLOC(size) \
	HeapAlloc(_WSHeap, HEAP_ZERO_MEMORY, (size) )
#define WS_FREE(ptr) \
	HeapFree(_WSHeap, 0, (ptr) )

static INT         _ws_sock_ops[] =
       { WS_SO_DEBUG, WS_SO_REUSEADDR, WS_SO_KEEPALIVE, WS_SO_DONTROUTE,
         WS_SO_BROADCAST, WS_SO_LINGER, WS_SO_OOBINLINE, WS_SO_SNDBUF,
         WS_SO_RCVBUF, WS_SO_ERROR, WS_SO_TYPE, WS_SO_DONTLINGER,
#ifdef SO_RCVTIMEO
	 WS_SO_RCVTIMEO,
#endif
	 0 };
static int           _px_sock_ops[] =
       { SO_DEBUG, SO_REUSEADDR, SO_KEEPALIVE, SO_DONTROUTE, SO_BROADCAST,
         SO_LINGER, SO_OOBINLINE, SO_SNDBUF, SO_RCVBUF, SO_ERROR, SO_TYPE,
	 SO_LINGER,
#ifdef SO_RCVTIMEO
	 SO_RCVTIMEO,
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

/* we need a special routine to handle WSA* errors */
static inline int sock_server_call( enum request req )
{
    unsigned int res = server_call_noerr( req );
    if (res)
    {
        /* do not map WSA errors */
        if ((res < WSABASEERR) || (res >= 0x10000000)) res = RtlNtStatusToDosError(res);
        SetLastError( res );
    }
    return res;
}

static int   _check_ws(LPWSINFO pwsi, SOCKET s);
static char* _check_buffer(LPWSINFO pwsi, int size);

static int _get_sock_fd(SOCKET s)
{
    struct get_read_fd_request *req = get_req_buffer();
    int fd;
    
    req->handle = s;
    server_call_fd( REQ_GET_READ_FD, -1, &fd );
    if (fd == -1)
        FIXME("handle %d is not a socket (GLE %ld)\n",s,GetLastError());
    return fd;    
}

static void _enable_event(SOCKET s, unsigned int event,
			  unsigned int sstate, unsigned int cstate)
{
    struct enable_socket_event_request *req = get_req_buffer();
    
    req->handle = s;
    req->mask   = event;
    req->sstate = sstate;
    req->cstate = cstate;
    sock_server_call( REQ_ENABLE_SOCKET_EVENT );
}

static int _is_blocking(SOCKET s)
{
    struct get_socket_event_request *req = get_req_buffer();
    
    req->handle  = s;
    req->service = FALSE;
    req->s_event = 0;
    sock_server_call( REQ_GET_SOCKET_EVENT );
    return (req->state & WS_FD_NONBLOCKING) == 0;
}

static unsigned int _get_sock_mask(SOCKET s)
{
    struct get_socket_event_request *req = get_req_buffer();
    
    req->handle  = s;
    req->service = FALSE;
    req->s_event = 0;
    sock_server_call( REQ_GET_SOCKET_EVENT );
    return req->mask;
}

static void _sync_sock_state(SOCKET s)
{
    /* do a dummy wineserver request in order to let
       the wineserver run through its select loop once */
    (void)_is_blocking(s);
}

static int _get_sock_error(SOCKET s, unsigned int bit)
{
    struct get_socket_event_request *req = get_req_buffer();
    
    req->handle  = s;
    req->service = FALSE;
    req->s_event = 0;
    sock_server_call( REQ_GET_SOCKET_EVENT );
    return req->errors[bit];
}

static LPWSINFO lpFirstIData = NULL;

static LPWSINFO WINSOCK_GetIData(void)
{
    DWORD pid = GetCurrentProcessId();
    LPWSINFO iData;

    for (iData = lpFirstIData; iData; iData = iData->lpNextIData) {
	if (iData->dwThisProcess == pid)
	    break;
    }
    return iData;
}

static BOOL WINSOCK_CreateIData(void)
{
    LPWSINFO iData;
    
    iData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WSINFO));
    if (!iData)
	return FALSE;
    iData->dwThisProcess = GetCurrentProcessId();
    iData->lpNextIData = lpFirstIData;
    lpFirstIData = iData;
    return TRUE;
}

static void WINSOCK_DeleteIData(void)
{
    LPWSINFO iData = WINSOCK_GetIData();
    LPWSINFO* ppid;
    if (iData) {
	for (ppid = &lpFirstIData; *ppid; ppid = &(*ppid)->lpNextIData) {
	    if (*ppid == iData) {
	        *ppid = iData->lpNextIData;
	        break;
	    }
	}

	if( iData->flags & WSI_BLOCKINGCALL )
	    TRACE("\tinside blocking call!\n");

	/* delete scratch buffers */

	if( iData->buffer ) SEGPTR_FREE(iData->buffer);
	if( iData->dbuffer ) SEGPTR_FREE(iData->dbuffer);

	HeapFree(GetProcessHeap(), 0, iData);
    }
}

BOOL WINAPI WSOCK32_LibMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("0x%x 0x%lx %p\n", hInstDLL, fdwReason, fImpLoad);
    switch (fdwReason) {
    case DLL_PROCESS_DETACH:
	WINSOCK_DeleteIData();
	break;
    }
    return TRUE;
}

BOOL WINAPI WINSOCK_LibMain(DWORD fdwReason, HINSTANCE hInstDLL, WORD ds,
                            WORD wHeapSize, DWORD dwReserved1, WORD wReserved2)
{
    TRACE("0x%x 0x%lx\n", hInstDLL, fdwReason);
    switch (fdwReason) {
    case DLL_PROCESS_DETACH:
	WINSOCK_DeleteIData();
	break;
    }
    return TRUE;
}
                                                                          
/***********************************************************************
 *          convert_sockopt()
 *
 * Converts socket flags from Windows format.
 */
static void convert_sockopt(INT *level, INT *optname)
{
  int i;
  switch (*level)
  {
     case WS_SOL_SOCKET:
        *level = SOL_SOCKET;
        for(i=0; _ws_sock_ops[i]; i++)
            if( _ws_sock_ops[i] == *optname ) break;
        if( _ws_sock_ops[i] ) *optname = _px_sock_ops[i];
        else FIXME("Unknown SOL_SOCKET optname %d\n", *optname);
        break;
     case WS_IPPROTO_TCP:
        *level = IPPROTO_TCP;
        for(i=0; _ws_tcp_ops[i]; i++)
		if ( _ws_tcp_ops[i] == *optname ) break;
        if( _ws_tcp_ops[i] ) *optname = _px_tcp_ops[i];
        else FIXME("Unknown IPPROTO_TCP optname %d\n", *optname);
	break;
  }
}

/* ----------------------------------- Per-thread info (or per-process?) */

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

static fd_set* fd_set_import( fd_set* fds, LPWSINFO pwsi, void* wsfds, int* highfd, int lfd[], BOOL b32 )
{
    /* translate Winsock fd set into local fd set */

    if( wsfds ) 
    { 
#define wsfds16	((ws_fd_set16*)wsfds)
#define wsfds32 ((ws_fd_set32*)wsfds)
	int i, count;

	FD_ZERO(fds);
	count = b32 ? wsfds32->fd_count : wsfds16->fd_count;

	for( i = 0; i < count; i++ )
	{
	     int s = (b32) ? wsfds32->fd_array[i]
			   : wsfds16->fd_array[i];
	     if( _check_ws(pwsi, s) )
             {
                    int fd = _get_sock_fd(s);
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

static int fd_set_export( LPWSINFO pwsi, fd_set* fds, fd_set* exceptfds, void* wsfds, int lfd[], BOOL b32 )
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
#define wsfds32 ((ws_fd_set32*)wsfds)
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
    LPWSINFO            pwsi;

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

    pwsi = WINSOCK_GetIData();
    if( pwsi == NULL )
    {
        WINSOCK_CreateIData();
        pwsi = WINSOCK_GetIData();
	if (!pwsi) return WSASYSNOTREADY;
    }
    pwsi->num_startup++;

    /* return winsock information */

    memcpy(lpWSAData, &WINSOCK_data, sizeof(WINSOCK_data));

    TRACE("succeeded\n");
    return 0;
}

/***********************************************************************
 *      WSAStartup32()			(WSOCK32.115)
 */
INT WINAPI WSAStartup(UINT wVersionRequested, LPWSADATA lpWSAData)
{
    return WSAStartup16( wVersionRequested, lpWSAData );
}

/***********************************************************************
 *      WSACleanup()			(WINSOCK.116)
 */
INT WINAPI WSACleanup(void)
{
    LPWSINFO pwsi = WINSOCK_GetIData();
    if( pwsi ) {
	if( --pwsi->num_startup > 0 ) return 0;

	WINSOCK_DeleteIData();
	return 0;
    }
    return SOCKET_ERROR;
}


/***********************************************************************
 *      WSAGetLastError()		(WSOCK32.111)(WINSOCK.111)
 */
INT WINAPI WSAGetLastError(void)
{
	return GetLastError();
}

/***********************************************************************
 *      WSASetLastError32()		(WSOCK32.112)
 */
void WINAPI WSASetLastError(INT iError) {
    SetLastError(iError);
}

/***********************************************************************
 *      WSASetLastError16()		(WINSOCK.112)
 */
void WINAPI WSASetLastError16(INT16 iError)
{
    WSASetLastError(iError);
}

int _check_ws(LPWSINFO pwsi, SOCKET s)
{
    if( pwsi )
    {
	int fd;
	if( pwsi->flags & WSI_BLOCKINGCALL ) SetLastError(WSAEINPROGRESS);
	if ( (fd = _get_sock_fd(s)) < 0 ) {
	    SetLastError(WSAENOTSOCK);
	    return 0;
	}
	/* FIXME: maybe check whether fd is really a socket? */
	close( fd );
	return 1;
    }
    return 0;
}

char* _check_buffer(LPWSINFO pwsi, int size)
{
    if( pwsi->buffer && pwsi->buflen >= size ) return pwsi->buffer;
    else SEGPTR_FREE(pwsi->buffer);

    pwsi->buffer = (char*)SEGPTR_ALLOC((pwsi->buflen = size)); 
    return pwsi->buffer;
}

struct ws_hostent* _check_buffer_he(LPWSINFO pwsi, int size)
{
    if( pwsi->he && pwsi->helen >= size ) return pwsi->he;
    else SEGPTR_FREE(pwsi->he);

    pwsi->he = (struct ws_hostent*)SEGPTR_ALLOC((pwsi->helen = size)); 
    return pwsi->he;
}

struct ws_servent* _check_buffer_se(LPWSINFO pwsi, int size)
{
    if( pwsi->se && pwsi->selen >= size ) return pwsi->se;
    else SEGPTR_FREE(pwsi->se);

    pwsi->se = (struct ws_servent*)SEGPTR_ALLOC((pwsi->selen = size)); 
    return pwsi->se;
}

struct ws_protoent* _check_buffer_pe(LPWSINFO pwsi, int size)
{
    if( pwsi->pe && pwsi->pelen >= size ) return pwsi->pe;
    else SEGPTR_FREE(pwsi->pe);

    pwsi->pe = (struct ws_protoent*)SEGPTR_ALLOC((pwsi->pelen = size)); 
    return pwsi->pe;
}

/* ----------------------------------- i/o APIs */

/***********************************************************************
 *		accept()		(WSOCK32.1)
 */
SOCKET WINAPI WINSOCK_accept(SOCKET s, struct sockaddr *addr,
                                 INT *addrlen32)
{
    LPWSINFO                 pwsi = WINSOCK_GetIData();
#ifdef HAVE_IPX
    struct ws_sockaddr_ipx*  addr2 = (struct ws_sockaddr_ipx *)addr;
#endif
    struct accept_socket_request *req = get_req_buffer();

    TRACE("(%08x): socket %04x\n", 
				  (unsigned)pwsi, (UINT16)s ); 
    if( _check_ws(pwsi, s) )
    {
	if (_is_blocking(s))
	{
	    /* block here */
	    int fd = _get_sock_fd(s);
	    do_block(fd, 5);
	    close(fd);
	    _sync_sock_state(s); /* let wineserver notice connection */
	    /* retrieve any error codes from it */
	    SetLastError(_get_sock_error(s, FD_ACCEPT_BIT));
	    /* FIXME: care about the error? */
	}
	req->lhandle = s;
	req->access  = GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE;
	req->inherit = TRUE;
	sock_server_call( REQ_ACCEPT_SOCKET );
	if( req->handle >= 0 )
	{
	    int fd = _get_sock_fd( s = req->handle );
	    if( getpeername(fd, addr, addrlen32) != -1 )
	    {
#ifdef HAVE_IPX
		if (addr && ((struct sockaddr_ipx *)addr)->sipx_family == AF_IPX) {
		    addr = (struct sockaddr *)
				malloc(addrlen32 ? *addrlen32 : sizeof(*addr2));
		    memcpy(addr, addr2,
				addrlen32 ? *addrlen32 : sizeof(*addr2));
		    addr2->sipx_family = WS_AF_IPX;
		    addr2->sipx_network = ((struct sockaddr_ipx *)addr)->sipx_network;
		    addr2->sipx_port = ((struct sockaddr_ipx *)addr)->sipx_port;
		    memcpy(addr2->sipx_node,
			((struct sockaddr_ipx *)addr)->sipx_node, IPX_NODE_LEN);
		    free(addr);
		}
#endif
	    } else SetLastError(wsaErrno());
	    close(fd);
	    return s;
	}
    }
    return INVALID_SOCKET;
}

/***********************************************************************
 *              accept()		(WINSOCK.1)
 */
SOCKET16 WINAPI WINSOCK_accept16(SOCKET16 s, struct sockaddr* addr,
                                 INT16* addrlen16 )
{
    INT addrlen32 = addrlen16 ? *addrlen16 : 0;
    SOCKET retSocket = WINSOCK_accept( s, addr, &addrlen32 );
    if( addrlen16 ) *addrlen16 = (INT16)addrlen32;
    return (SOCKET16)retSocket;
}

/***********************************************************************
 *		bind()			(WSOCK32.2)
 */
INT WINAPI WINSOCK_bind(SOCKET s, struct sockaddr *name, INT namelen)
{
    LPWSINFO                 pwsi = WINSOCK_GetIData();
#ifdef HAVE_IPX
    struct ws_sockaddr_ipx*  name2 = (struct ws_sockaddr_ipx *)name;
#endif

    TRACE("(%08x): socket %04x, ptr %8x, length %d\n", 
			   (unsigned)pwsi, s, (int) name, namelen);
#if DEBUG_SOCKADDR
    dump_sockaddr(name);
#endif

    if ( _check_ws(pwsi, s) )
    {
      int fd = _get_sock_fd(s);
      /* FIXME: what family does this really map to on the Unix side? */
      if (name && ((struct ws_sockaddr_ipx *)name)->sipx_family == WS_AF_PUP)
	((struct ws_sockaddr_ipx *)name)->sipx_family = AF_UNSPEC;
#ifdef HAVE_IPX
      else if (name &&
		((struct ws_sockaddr_ipx *)name)->sipx_family == WS_AF_IPX)
      {
	name = (struct sockaddr *) malloc(sizeof(struct sockaddr_ipx));
	memset(name, '\0', sizeof(struct sockaddr_ipx));
	((struct sockaddr_ipx *)name)->sipx_family = AF_IPX;
	((struct sockaddr_ipx *)name)->sipx_port = name2->sipx_port;
	((struct sockaddr_ipx *)name)->sipx_network = name2->sipx_network;
	memcpy(((struct sockaddr_ipx *)name)->sipx_node,
		name2->sipx_node, IPX_NODE_LEN);
	namelen = sizeof(struct sockaddr_ipx);
      }
#endif
      if ( namelen >= sizeof(*name) ) 
      {
	if ( name && (((struct ws_sockaddr_in *)name)->sin_family == AF_INET
#ifdef HAVE_IPX
             || ((struct sockaddr_ipx *)name)->sipx_family == AF_IPX
#endif
           ))
        {
	  if ( bind(fd, name, namelen) < 0 ) 
	  {
	     int	loc_errno = errno;
	     WARN("\tfailure - errno = %i\n", errno);
	     errno = loc_errno;
	     switch(errno)
	     {
		case EBADF: SetLastError(WSAENOTSOCK); break;
		case EADDRNOTAVAIL: SetLastError(WSAEINVAL); break;
		default: SetLastError(wsaErrno());break;
	     }
	  }
	  else {
#ifdef HAVE_IPX
	    if (((struct sockaddr_ipx *)name)->sipx_family == AF_IPX)
		free(name);
#endif
	    close(fd);
	    return 0; /* success */
	  }
        } else SetLastError(WSAEAFNOSUPPORT);
      } else SetLastError(WSAEFAULT);
#ifdef HAVE_IPX
      if (name && ((struct sockaddr_ipx *)name)->sipx_family == AF_IPX)
	free(name);
#endif
      close(fd);
    }
    return SOCKET_ERROR;
}

/***********************************************************************
 *              bind()			(WINSOCK.2)
 */
INT16 WINAPI WINSOCK_bind16(SOCKET16 s, struct sockaddr *name, INT16 namelen)
{
  return (INT16)WINSOCK_bind( s, name, namelen );
}

/***********************************************************************
 *		closesocket()		(WSOCK32.3)
 */
INT WINAPI WINSOCK_closesocket(SOCKET s)
{
    LPWSINFO      pwsi = WINSOCK_GetIData();

    TRACE("(%08x): socket %08x\n", (unsigned)pwsi, s);

    if( _check_ws(pwsi, s) )
    { 
	if( CloseHandle(s) )
	    return 0;
    }
    return SOCKET_ERROR;
}

/***********************************************************************
 *              closesocket()           (WINSOCK.3)
 */
INT16 WINAPI WINSOCK_closesocket16(SOCKET16 s)
{
    return (INT16)WINSOCK_closesocket(s);
}

/***********************************************************************
 *		connect()		(WSOCK32.4)
 */
INT WINAPI WINSOCK_connect(SOCKET s, struct sockaddr *name, INT namelen)
{
  LPWSINFO                 pwsi = WINSOCK_GetIData();
#ifdef HAVE_IPX
  struct ws_sockaddr_ipx*  name2 = (struct ws_sockaddr_ipx *)name;
#endif

  TRACE("(%08x): socket %04x, ptr %8x, length %d\n", 
			   (unsigned)pwsi, s, (int) name, namelen);
#if DEBUG_SOCKADDR
  dump_sockaddr(name);
#endif

  if( _check_ws(pwsi, s) )
  {
    int fd = _get_sock_fd(s);
    if (name && ((struct ws_sockaddr_ipx *)name)->sipx_family == WS_AF_PUP)
	((struct ws_sockaddr_ipx *)name)->sipx_family = AF_UNSPEC;
#ifdef HAVE_IPX
    else if (name && ((struct ws_sockaddr_ipx *)name)->sipx_family == WS_AF_IPX)
    {
	name = (struct sockaddr *) malloc(sizeof(struct sockaddr_ipx));
	memset(name, '\0', sizeof(struct sockaddr_ipx));
	((struct sockaddr_ipx *)name)->sipx_family = AF_IPX;
	((struct sockaddr_ipx *)name)->sipx_port = name2->sipx_port;
	((struct sockaddr_ipx *)name)->sipx_network = name2->sipx_network;
	memcpy(((struct sockaddr_ipx *)name)->sipx_node,
		name2->sipx_node, IPX_NODE_LEN);
	namelen = sizeof(struct sockaddr_ipx);
    }
#endif
    if (connect(fd, name, namelen) == 0) {
	close(fd);
	goto connect_success;
    }
    if (errno == EINPROGRESS)
    {
	/* tell wineserver that a connection is in progress */
	_enable_event(s, FD_CONNECT|FD_READ|FD_WRITE,
		      WS_FD_CONNECT|WS_FD_READ|WS_FD_WRITE,
		      WS_FD_CONNECTED|WS_FD_LISTENING);
	if (_is_blocking(s))
	{
	    int result;
	    /* block here */
	    do_block(fd, 6);
	    _sync_sock_state(s); /* let wineserver notice connection */
	    /* retrieve any error codes from it */
	    result = _get_sock_error(s, FD_CONNECT_BIT);
	    if (result)
		SetLastError(result);
	    else {
		close(fd);
		goto connect_success;
	    }
	}
	else SetLastError(WSAEWOULDBLOCK);
	close(fd);
    }
    else
    {
	SetLastError(wsaErrno());
	close(fd);
    }
  }
#ifdef HAVE_IPX
  if (name && ((struct sockaddr_ipx *)name)->sipx_family == AF_IPX)
    free(name);
#endif
  return SOCKET_ERROR;
connect_success:
#ifdef HAVE_IPX
    if (((struct sockaddr_ipx *)name)->sipx_family == AF_IPX)
	free(name);
#endif
    _enable_event(s, FD_CONNECT|FD_READ|FD_WRITE,
		  WS_FD_CONNECTED|WS_FD_READ|WS_FD_WRITE,
		  WS_FD_CONNECT|WS_FD_LISTENING);
    return 0; 
}

/***********************************************************************
 *              connect()               (WINSOCK.4)
 */
INT16 WINAPI WINSOCK_connect16(SOCKET16 s, struct sockaddr *name, INT16 namelen)
{
  return (INT16)WINSOCK_connect( s, name, namelen );
}

/***********************************************************************
 *		getpeername()		(WSOCK32.5)
 */
INT WINAPI WINSOCK_getpeername(SOCKET s, struct sockaddr *name,
                                   INT *namelen)
{
    LPWSINFO                 pwsi = WINSOCK_GetIData();
#ifdef HAVE_IPX
    struct ws_sockaddr_ipx*  name2 = (struct ws_sockaddr_ipx *)name;
#endif

    TRACE("(%08x): socket: %04x, ptr %8x, ptr %8x\n", 
			   (unsigned)pwsi, s, (int) name, *namelen);
    if( _check_ws(pwsi, s) )
    {
	int fd = _get_sock_fd(s);
	if (getpeername(fd, name, namelen) == 0) {
#ifdef HAVE_IPX
	    if (((struct ws_sockaddr_ipx *)name)->sipx_family == AF_IPX) {
		name = (struct sockaddr *)
				malloc(namelen ? *namelen : sizeof(*name2));
		memcpy(name, name2, namelen ? *namelen : sizeof(*name2));
		name2->sipx_family = WS_AF_IPX;
		name2->sipx_network = ((struct sockaddr_ipx *)name)->sipx_network;
		name2->sipx_port = ((struct sockaddr_ipx *)name)->sipx_port;
		memcpy(name2->sipx_node,
			((struct sockaddr_ipx *)name)->sipx_node, IPX_NODE_LEN);
		free(name);
	    }
#endif
	    close(fd);
	    return 0; 
	}
	SetLastError(wsaErrno());
	close(fd);
    }
    return SOCKET_ERROR;
}

/***********************************************************************
 *              getpeername()		(WINSOCK.5)
 */
INT16 WINAPI WINSOCK_getpeername16(SOCKET16 s, struct sockaddr *name,
                                   INT16 *namelen16)
{
    INT namelen32 = *namelen16;
    INT retVal = WINSOCK_getpeername( s, name, &namelen32 );

#if DEBUG_SOCKADDR
    dump_sockaddr(name);
#endif

   *namelen16 = namelen32;
    return (INT16)retVal;
}

/***********************************************************************
 *		getsockname()		(WSOCK32.6)
 */
INT WINAPI WINSOCK_getsockname(SOCKET s, struct sockaddr *name,
                                   INT *namelen)
{
    LPWSINFO                 pwsi = WINSOCK_GetIData();
#ifdef HAVE_IPX
    struct ws_sockaddr_ipx*  name2 = (struct ws_sockaddr_ipx *)name;
#endif

    TRACE("(%08x): socket: %04x, ptr %8x, ptr %8x\n", 
			  (unsigned)pwsi, s, (int) name, (int) *namelen);
    if( _check_ws(pwsi, s) )
    {
	int fd = _get_sock_fd(s);
	if (getsockname(fd, name, namelen) == 0) {
#ifdef HAVE_IPX
	    if (((struct sockaddr_ipx *)name)->sipx_family == AF_IPX) {
		name = (struct sockaddr *)
				malloc(namelen ? *namelen : sizeof(*name2));
		memcpy(name, name2, namelen ? *namelen : sizeof(*name2));
		name2->sipx_family = WS_AF_IPX;
		name2->sipx_network = ((struct sockaddr_ipx *)name)->sipx_network;
		name2->sipx_port = ((struct sockaddr_ipx *)name)->sipx_port;
		memcpy(name2->sipx_node,
			((struct sockaddr_ipx *)name)->sipx_node, IPX_NODE_LEN);
		free(name);
	    }
#endif
	    close(fd);
	    return 0; 
	}
	SetLastError(wsaErrno());
	close(fd);
    }
    return SOCKET_ERROR;
}

/***********************************************************************
 *              getsockname()		(WINSOCK.6)
 */
INT16 WINAPI WINSOCK_getsockname16(SOCKET16 s, struct sockaddr *name,
                                   INT16 *namelen16)
{
    INT retVal;

    if( namelen16 )
    {
        INT namelen32 = *namelen16;
        retVal = WINSOCK_getsockname( s, name, &namelen32 );
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
INT WINAPI WINSOCK_getsockopt(SOCKET s, INT level, 
                                  INT optname, char *optval, INT *optlen)
{
    LPWSINFO      pwsi = WINSOCK_GetIData();

    TRACE("(%08x): socket: %04x, opt %d, ptr %8x, ptr %8x\n", 
			   (unsigned)pwsi, s, level, (int) optval, (int) *optlen);
    if( _check_ws(pwsi, s) )
    {
	int fd = _get_sock_fd(s);
	convert_sockopt(&level, &optname);
	if (getsockopt(fd, (int) level, optname, optval, optlen) == 0 )
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
 *              getsockopt()		(WINSOCK.7)
 */
INT16 WINAPI WINSOCK_getsockopt16(SOCKET16 s, INT16 level,
                                  INT16 optname, char *optval, INT16 *optlen)
{
    INT optlen32;
    INT *p = &optlen32;
    INT retVal;
    if( optlen ) optlen32 = *optlen; else p = NULL;
    retVal = WINSOCK_getsockopt( s, (UINT16)level, optname, optval, p );
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
char* WINAPI WINSOCK_inet_ntoa(struct in_addr in)
{
  /* use "buffer for dummies" here because some applications have 
   * propensity to decode addresses in ws_hostent structure without 
   * saving them first...
   */

    LPWSINFO      pwsi = WINSOCK_GetIData();

    if( pwsi )
    {
	char*	s = inet_ntoa(in);
	if( s ) 
	{
            if( pwsi->dbuffer == NULL ) {
                /* Yes, 16: 4*3 digits + 3 '.' + 1 '\0' */
		if((pwsi->dbuffer = (char*) SEGPTR_ALLOC(16)) == NULL )
		{
		    SetLastError(WSAENOBUFS);
		    return NULL;
		}
            }
	    strcpy(pwsi->dbuffer, s);
	    return pwsi->dbuffer; 
	}
	SetLastError(wsaErrno());
    }
    return NULL;
}

SEGPTR WINAPI WINSOCK_inet_ntoa16(struct in_addr in)
{
  char* retVal = WINSOCK_inet_ntoa(in);
  return retVal ? SEGPTR_GET(retVal) : (SEGPTR)NULL;
}

/***********************************************************************
 *		ioctlsocket()		(WSOCK32.12)
 */
INT WINAPI WINSOCK_ioctlsocket(SOCKET s, LONG cmd, ULONG *argp)
{
  LPWSINFO      pwsi = WINSOCK_GetIData();

  TRACE("(%08x): socket %04x, cmd %08lx, ptr %8x\n", 
			  (unsigned)pwsi, s, cmd, (unsigned) argp);
  if( _check_ws(pwsi, s) )
  {
    int		fd = _get_sock_fd(s);
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
		    SetLastError(WSAEINVAL); 
		    close(fd);
		    return SOCKET_ERROR; 
		}
		close(fd);
		if (*argp)
		    _enable_event(s, 0, WS_FD_NONBLOCKING, 0);
		else
		    _enable_event(s, 0, 0, WS_FD_NONBLOCKING);
		return 0;

	case WS_SIOCATMARK: 
		newcmd=SIOCATMARK; 
		break;

	case WS_IOW('f',125,u_long): 
		WARN("Warning: WS1.1 shouldn't be using async I/O\n");
		SetLastError(WSAEINVAL); 
		return SOCKET_ERROR;

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
 *              ioctlsocket()           (WINSOCK.12)
 */
INT16 WINAPI WINSOCK_ioctlsocket16(SOCKET16 s, LONG cmd, ULONG *argp)
{
    return (INT16)WINSOCK_ioctlsocket( s, cmd, argp );
}


/***********************************************************************
 *		listen()		(WSOCK32.13)
 */
INT WINAPI WINSOCK_listen(SOCKET s, INT backlog)
{
    LPWSINFO      pwsi = WINSOCK_GetIData();

    TRACE("(%08x): socket %04x, backlog %d\n", 
			    (unsigned)pwsi, s, backlog);
    if( _check_ws(pwsi, s) )
    {
	int fd = _get_sock_fd(s);
	if (listen(fd, backlog) == 0)
	{
	    close(fd);
	    _enable_event(s, FD_ACCEPT,
			  WS_FD_LISTENING,
			  WS_FD_CONNECT|WS_FD_CONNECTED);
	    return 0;
	}
	SetLastError(wsaErrno());
    }
    else SetLastError(WSAENOTSOCK);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              listen()		(WINSOCK.13)
 */
INT16 WINAPI WINSOCK_listen16(SOCKET16 s, INT16 backlog)
{
    return (INT16)WINSOCK_listen( s, backlog );
}


/***********************************************************************
 *		recv()			(WSOCK32.16)
 */
INT WINAPI WINSOCK_recv(SOCKET s, char *buf, INT len, INT flags)
{
    LPWSINFO      pwsi = WINSOCK_GetIData();

    TRACE("(%08x): socket %04x, buf %8x, len %d, "
		    "flags %d\n", (unsigned)pwsi, s, (unsigned)buf, 
		    len, flags);
    if( _check_ws(pwsi, s) )
    {
	int fd = _get_sock_fd(s);
	INT length;

	if (_is_blocking(s))
	{
	    /* block here */
	    /* FIXME: OOB and exceptfds? */
	    do_block(fd, 1);
	}
	if ((length = recv(fd, buf, len, flags)) >= 0) 
	{ 
	    TRACE(" -> %i bytes\n", length);

	    close(fd);
	    _enable_event(s, FD_READ, 0, 0);
	    return length;
	}
	SetLastError(wsaErrno());
	close(fd);
    }
    else SetLastError(WSAENOTSOCK);
    WARN(" -> ERROR\n");
    return SOCKET_ERROR;
}

/***********************************************************************
 *              recv()			(WINSOCK.16)
 */
INT16 WINAPI WINSOCK_recv16(SOCKET16 s, char *buf, INT16 len, INT16 flags)
{
    return (INT16)WINSOCK_recv( s, buf, len, flags );
}


/***********************************************************************
 *		recvfrom()		(WSOCK32.17)
 */
INT WINAPI WINSOCK_recvfrom(SOCKET s, char *buf, INT len, INT flags, 
                                struct sockaddr *from, INT *fromlen32)
{
    LPWSINFO                 pwsi = WINSOCK_GetIData();
#ifdef HAVE_IPX
    struct ws_sockaddr_ipx*  from2 = (struct ws_sockaddr_ipx *)from;
#endif

    TRACE("(%08x): socket %04x, ptr %08x, "
		    "len %d, flags %d\n", (unsigned)pwsi, s, (unsigned)buf,
		    len, flags);
#if DEBUG_SOCKADDR
    if( from ) dump_sockaddr(from);
    else DPRINTF("from = NULL\n");
#endif

    if( _check_ws(pwsi, s) )
    {
	int fd = _get_sock_fd(s);
	int length;

	if (_is_blocking(s))
	{
	    /* block here */
	    /* FIXME: OOB and exceptfds */
	    do_block(fd, 1);
	}
	if ((length = recvfrom(fd, buf, len, flags, from, fromlen32)) >= 0)
	{
	    TRACE(" -> %i bytes\n", length);

#ifdef HAVE_IPX
	if (from && ((struct sockaddr_ipx *)from)->sipx_family == AF_IPX) {
	    from = (struct sockaddr *)
				malloc(fromlen32 ? *fromlen32 : sizeof(*from2));
	    memcpy(from, from2, fromlen32 ? *fromlen32 : sizeof(*from2));
	    from2->sipx_family = WS_AF_IPX;
	    from2->sipx_network = ((struct sockaddr_ipx *)from)->sipx_network;
	    from2->sipx_port = ((struct sockaddr_ipx *)from)->sipx_port;
	    memcpy(from2->sipx_node,
			((struct sockaddr_ipx *)from)->sipx_node, IPX_NODE_LEN);
	    free(from);
	}
#endif
	    close(fd);
	    _enable_event(s, FD_READ, 0, 0);
	    return (INT16)length;
	}
	SetLastError(wsaErrno());
	close(fd);
    }
    else SetLastError(WSAENOTSOCK);
    WARN(" -> ERROR\n");
#ifdef HAVE_IPX
    if (from && ((struct sockaddr_ipx *)from)->sipx_family == AF_IPX) {
	from = (struct sockaddr *)
				malloc(fromlen32 ? *fromlen32 : sizeof(*from2));
	memcpy(from, from2, fromlen32 ? *fromlen32 : sizeof(*from2));
	from2->sipx_family = WS_AF_IPX;
	from2->sipx_network = ((struct sockaddr_ipx *)from)->sipx_network;
	from2->sipx_port = ((struct sockaddr_ipx *)from)->sipx_port;
	memcpy(from2->sipx_node,
		((struct sockaddr_ipx *)from)->sipx_node, IPX_NODE_LEN);
	free(from);
    }
#endif
    return SOCKET_ERROR;
}

/***********************************************************************
 *              recvfrom()		(WINSOCK.17)
 */
INT16 WINAPI WINSOCK_recvfrom16(SOCKET16 s, char *buf, INT16 len, INT16 flags,
                                struct sockaddr *from, INT16 *fromlen16)
{
    INT fromlen32;
    INT *p = &fromlen32;
    INT retVal;

    if( fromlen16 ) fromlen32 = *fromlen16; else p = NULL;
    retVal = WINSOCK_recvfrom( s, buf, len, flags, from, p );
    if( fromlen16 ) *fromlen16 = fromlen32;
    return (INT16)retVal;
}

/***********************************************************************
 *		select()		(WINSOCK.18)(WSOCK32.18)
 */
static INT __ws_select( BOOL b32, void *ws_readfds, void *ws_writefds, void *ws_exceptfds,
			  struct timeval *timeout )
{
    LPWSINFO      pwsi = WINSOCK_GetIData();
	
    TRACE("(%08x): read %8x, write %8x, excp %8x\n", 
    (unsigned) pwsi, (unsigned) ws_readfds, (unsigned) ws_writefds, (unsigned) ws_exceptfds);

    if( pwsi )
    {
	int         highfd = 0;
	fd_set      readfds, writefds, exceptfds;
	fd_set     *p_read, *p_write, *p_except;
	int         readfd[FD_SETSIZE], writefd[FD_SETSIZE], exceptfd[FD_SETSIZE];

	p_read = fd_set_import(&readfds, pwsi, ws_readfds, &highfd, readfd, b32);
	p_write = fd_set_import(&writefds, pwsi, ws_writefds, &highfd, writefd, b32);
	p_except = fd_set_import(&exceptfds, pwsi, ws_exceptfds, &highfd, exceptfd, b32);

	if( (highfd = select(highfd + 1, p_read, p_write, p_except, timeout)) > 0 )
	{
	    fd_set_export(pwsi, &readfds, p_except, ws_readfds, readfd, b32);
	    fd_set_export(pwsi, &writefds, p_except, ws_writefds, writefd, b32);

	    if (p_except && ws_exceptfds)
	    {
#define wsfds16 ((ws_fd_set16*)ws_exceptfds)
#define wsfds32 ((ws_fd_set32*)ws_exceptfds)
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
	if( ws_readfds ) ((ws_fd_set32*)ws_readfds)->fd_count = 0;
	if( ws_writefds ) ((ws_fd_set32*)ws_writefds)->fd_count = 0;
	if( ws_exceptfds ) ((ws_fd_set32*)ws_exceptfds)->fd_count = 0;

        if( highfd == 0 ) return 0;
	SetLastError(wsaErrno());
    } 
    return SOCKET_ERROR;
}

INT16 WINAPI WINSOCK_select16(INT16 nfds, ws_fd_set16 *ws_readfds,
                              ws_fd_set16 *ws_writefds, ws_fd_set16 *ws_exceptfds,
                              struct timeval *timeout)
{
    return (INT16)__ws_select( FALSE, ws_readfds, ws_writefds, ws_exceptfds, timeout );
}

INT WINAPI WINSOCK_select(INT nfds, ws_fd_set32 *ws_readfds,
                              ws_fd_set32 *ws_writefds, ws_fd_set32 *ws_exceptfds,
                              struct timeval *timeout)
{
    /* struct timeval is the same for both 32- and 16-bit code */
    return (INT)__ws_select( TRUE, ws_readfds, ws_writefds, ws_exceptfds, timeout );
}


/***********************************************************************
 *		send()			(WSOCK32.19)
 */
INT WINAPI WINSOCK_send(SOCKET s, char *buf, INT len, INT flags)
{
    LPWSINFO      pwsi = WINSOCK_GetIData();

    TRACE("(%08x): socket %04x, ptr %08x, length %d, flags %d\n", 
			   (unsigned)pwsi, s, (unsigned) buf, len, flags);
    if( _check_ws(pwsi, s) )
    {
	int	fd = _get_sock_fd(s);
	int	length;

	if (_is_blocking(s))
	{
	    /* block here */
	    /* FIXME: exceptfds */
	    do_block(fd, 2);
	}
	if ((length = send(fd, buf, len, flags)) < 0 ) 
	{
	    SetLastError(wsaErrno());
	    if( GetLastError() == WSAEWOULDBLOCK )
		_enable_event(s, FD_WRITE, 0, 0);
	}
	else
	{
	    close(fd);
	    return (INT16)length;
	}
	close(fd);
    }
    else SetLastError(WSAENOTSOCK);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              send()			(WINSOCK.19)
 */
INT16 WINAPI WINSOCK_send16(SOCKET16 s, char *buf, INT16 len, INT16 flags)
{
    return WINSOCK_send( s, buf, len, flags );
}

/***********************************************************************
 *		sendto()		(WSOCK32.20)
 */
INT WINAPI WINSOCK_sendto(SOCKET s, char *buf, INT len, INT flags,
                              struct sockaddr *to, INT tolen)
{
    LPWSINFO                 pwsi = WINSOCK_GetIData();
#ifdef HAVE_IPX
    struct ws_sockaddr_ipx*  to2 = (struct ws_sockaddr_ipx *)to;
#endif

    TRACE("(%08x): socket %04x, ptr %08x, length %d, flags %d\n",
                          (unsigned)pwsi, s, (unsigned) buf, len, flags);
    if( _check_ws(pwsi, s) )
    {
	int	fd = _get_sock_fd(s);
	INT	length;

	if (to && ((struct ws_sockaddr_ipx *)to)->sipx_family == WS_AF_PUP)
	    ((struct ws_sockaddr_ipx *)to)->sipx_family = AF_UNSPEC;
#ifdef HAVE_IPX
	else if (to &&
		((struct ws_sockaddr_ipx *)to)->sipx_family == WS_AF_IPX)
	{
	    to = (struct sockaddr *) malloc(sizeof(struct sockaddr_ipx));
	    memset(to, '\0', sizeof(struct sockaddr_ipx));
	    ((struct sockaddr_ipx *)to)->sipx_family = AF_IPX;
	    ((struct sockaddr_ipx *)to)->sipx_port = to2->sipx_port;
	    ((struct sockaddr_ipx *)to)->sipx_network = to2->sipx_network;
	    memcpy(((struct sockaddr_ipx *)to)->sipx_node,
			to2->sipx_node, IPX_NODE_LEN);
	    tolen = sizeof(struct sockaddr_ipx);
	}
#endif
	if (_is_blocking(s))
	{
	    /* block here */
	    /* FIXME: exceptfds */
	    do_block(fd, 2);
	}
	if ((length = sendto(fd, buf, len, flags, to, tolen)) < 0 )
	{
	    SetLastError(wsaErrno());
	    if( GetLastError() == WSAEWOULDBLOCK )
		_enable_event(s, FD_WRITE, 0, 0);
	} 
	else {
#ifdef HAVE_IPX
	    if (to && ((struct sockaddr_ipx *)to)->sipx_family == AF_IPX) {
		free(to);
	    }
#endif
	    close(fd);
	    return length;
	}
	close(fd);
    }
    else SetLastError(WSAENOTSOCK);
#ifdef HAVE_IPX
    if (to && ((struct sockaddr_ipx *)to)->sipx_family == AF_IPX) {
	free(to);
    }
#endif
    return SOCKET_ERROR;
}

/***********************************************************************
 *              sendto()		(WINSOCK.20)
 */
INT16 WINAPI WINSOCK_sendto16(SOCKET16 s, char *buf, INT16 len, INT16 flags,
                              struct sockaddr *to, INT16 tolen)
{
    return (INT16)WINSOCK_sendto( s, buf, len, flags, to, tolen );
}

/***********************************************************************
 *		setsockopt()		(WSOCK32.21)
 */
INT WINAPI WINSOCK_setsockopt(SOCKET16 s, INT level, INT optname, 
                                  char *optval, INT optlen)
{
    LPWSINFO      pwsi = WINSOCK_GetIData();

    TRACE("(%08x): socket %04x, lev %d, opt %d, ptr %08x, len %d\n",
			  (unsigned)pwsi, s, level, optname, (int) optval, optlen);
    if( _check_ws(pwsi, s) )
    {
	struct	linger linger;
	int fd = _get_sock_fd(s);

	convert_sockopt(&level, &optname);
	if (optname == SO_LINGER && optval) {
		/* yes, uses unsigned short in both win16/win32 */
		linger.l_onoff	= ((UINT16*)optval)[0];
		linger.l_linger	= ((UINT16*)optval)[1];
		/* FIXME: what is documented behavior if SO_LINGER optval
		   is null?? */
		optval = (char*)&linger;
		optlen = sizeof(struct linger);
	}
	if (setsockopt(fd, level, optname, optval, optlen) == 0)
	{
	    close(fd);
	    return 0;
	}
	SetLastError(wsaErrno());
	close(fd);
    }
    else SetLastError(WSAENOTSOCK);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              setsockopt()		(WINSOCK.21)
 */
INT16 WINAPI WINSOCK_setsockopt16(SOCKET16 s, INT16 level, INT16 optname,
                                  char *optval, INT16 optlen)
{
    if( !optval ) return SOCKET_ERROR;
    return (INT16)WINSOCK_setsockopt( s, (UINT16)level, optname, optval, optlen );
}


/***********************************************************************
 *		shutdown()		(WSOCK32.22)
 */
INT WINAPI WINSOCK_shutdown(SOCKET s, INT how)
{
    LPWSINFO      pwsi = WINSOCK_GetIData();

    TRACE("(%08x): socket %04x, how %i\n",
			    (unsigned)pwsi, s, how );
    if( _check_ws(pwsi, s) )
    {
	int fd = _get_sock_fd(s);
	    switch( how )
	    {
		case 0: /* drop receives */
			_enable_event(s, 0, 0, WS_FD_READ);
#ifdef SHUT_RD
			how = SHUT_RD;
#endif
			break;

		case 1: /* drop sends */
			_enable_event(s, 0, 0, WS_FD_WRITE);
#ifdef SHUT_WR
			how = SHUT_WR;
#endif
			break;

		case 2: /* drop all */
#ifdef SHUT_RDWR
			how = SHUT_RDWR;
#endif
		default:
			WSAAsyncSelect( s, 0, 0, 0 );
			break;
	    }

	if (shutdown(fd, how) == 0) 
	{
	    if( how > 1 ) 
	    {
		_enable_event(s, 0, 0, WS_FD_CONNECTED|WS_FD_LISTENING);
	    }
	    close(fd);
	    return 0;
	}
	SetLastError(wsaErrno());
	close(fd);
    } 
    else SetLastError(WSAENOTSOCK);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              shutdown()		(WINSOCK.22)
 */
INT16 WINAPI WINSOCK_shutdown16(SOCKET16 s, INT16 how)
{
    return (INT16)WINSOCK_shutdown( s, how );
}


/***********************************************************************
 *		socket()		(WSOCK32.23)
 */
SOCKET WINAPI WINSOCK_socket(INT af, INT type, INT protocol)
{
  LPWSINFO      pwsi = WINSOCK_GetIData();
  struct create_socket_request *req = get_req_buffer();

  TRACE("(%08x): af=%d type=%d protocol=%d\n", 
			  (unsigned)pwsi, af, type, protocol);

  if( pwsi )
  {
    /* check the socket family */
    switch(af) 
    {
#ifdef HAVE_IPX
	case WS_AF_IPX:	af = AF_IPX;
#endif
	case AF_INET:
	case AF_UNSPEC: break;
	default:        SetLastError(WSAEAFNOSUPPORT); 
			return INVALID_SOCKET;
    }

    /* check the socket type */
    switch(type) 
    {
	case SOCK_STREAM:
	case SOCK_DGRAM:
	case SOCK_RAW:  break;
	default:        SetLastError(WSAESOCKTNOSUPPORT); 
			return INVALID_SOCKET;
    }

    /* check the protocol type */
    if ( protocol < 0 )  /* don't support negative values */
    { SetLastError(WSAEPROTONOSUPPORT); return INVALID_SOCKET; }

    if ( af == AF_UNSPEC)  /* did they not specify the address family? */
        switch(protocol) 
	{
          case IPPROTO_TCP:
             if (type == SOCK_STREAM) { af = AF_INET; break; }
          case IPPROTO_UDP:
             if (type == SOCK_DGRAM)  { af = AF_INET; break; }
          default: SetLastError(WSAEPROTOTYPE); return INVALID_SOCKET;
        }

    req->family   = af;
    req->type     = type;
    req->protocol = protocol;
    req->access   = GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE;
    req->inherit  = TRUE;
    sock_server_call( REQ_CREATE_SOCKET );
    if ( req->handle >= 0)
    {
	TRACE("\tcreated %04x\n", req->handle);

	return req->handle;
    }

    if (GetLastError() == WSAEACCES) /* raw socket denied */
    {
	if (type == SOCK_RAW)
	    MESSAGE("WARNING: Trying to create a socket of type SOCK_RAW, will fail unless running as root\n");
        else
            MESSAGE("WS_SOCKET: not enough privileges to create socket, try running as root\n");
        SetLastError(WSAESOCKTNOSUPPORT);
    }
  }
 
  WARN("\t\tfailed!\n");
  return INVALID_SOCKET;
}

/***********************************************************************
 *              socket()		(WINSOCK.23)
 */
SOCKET16 WINAPI WINSOCK_socket16(INT16 af, INT16 type, INT16 protocol)
{
    return (SOCKET16)WINSOCK_socket( af, type, protocol );
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
    LPWSINFO      	pwsi = WINSOCK_GetIData();

    if( pwsi )
    {
	struct hostent*	host;
	if( (host = gethostbyaddr(addr, len, type)) != NULL )
	    if( WS_dup_he(pwsi, host, dup_flag) )
		return (struct WIN_hostent*)(pwsi->he);
	    else 
		SetLastError(WSAENOBUFS);
	else 
	    SetLastError((h_errno < 0) ? wsaErrno() : wsaHerrno());
    }
    return NULL;
}

SEGPTR WINAPI WINSOCK_gethostbyaddr16(const char *addr, INT16 len, INT16 type)
{
    struct WIN_hostent* retval;
    TRACE("ptr %08x, len %d, type %d\n",
                            (unsigned) addr, len, type);
    retval = __ws_gethostbyaddr( addr, len, type, WS_DUP_SEGPTR );
    return retval ? SEGPTR_GET(retval) : ((SEGPTR)NULL);
}

struct WIN_hostent* WINAPI WINSOCK_gethostbyaddr(const char *addr, INT len,
                                                INT type)
{
    TRACE("ptr %08x, len %d, type %d\n",
                             (unsigned) addr, len, type);
    return __ws_gethostbyaddr(addr, len, type, WS_DUP_LINEAR);
}

/***********************************************************************
 *		gethostbyname()		(WINSOCK.52)(WSOCK32.52)
 */
static struct WIN_hostent * __ws_gethostbyname(const char *name, int dup_flag)
{
    LPWSINFO              pwsi = WINSOCK_GetIData();

    if( pwsi )
    {
	struct hostent*     host;
	if( (host = gethostbyname(name)) != NULL )
	     if( WS_dup_he(pwsi, host, dup_flag) )
		 return (struct WIN_hostent*)(pwsi->he);
	     else SetLastError(WSAENOBUFS);
	else SetLastError((h_errno < 0) ? wsaErrno() : wsaHerrno());
    }
    return NULL;
}

SEGPTR WINAPI WINSOCK_gethostbyname16(const char *name)
{
    struct WIN_hostent* retval;
    TRACE("%s\n", (name)?name:NULL_STRING);
    retval = __ws_gethostbyname( name, WS_DUP_SEGPTR );
    return (retval)? SEGPTR_GET(retval) : ((SEGPTR)NULL) ;
}

struct WIN_hostent* WINAPI WINSOCK_gethostbyname(const char* name)
{
    TRACE("%s\n", (name)?name:NULL_STRING);
    return __ws_gethostbyname( name, WS_DUP_LINEAR );
}


/***********************************************************************
 *		getprotobyname()	(WINSOCK.53)(WSOCK32.53)
 */
static struct WIN_protoent* __ws_getprotobyname(const char *name, int dup_flag)
{
    LPWSINFO              pwsi = WINSOCK_GetIData();

    if( pwsi )
    {
	struct protoent*     proto;
	if( (proto = getprotobyname(name)) != NULL )
	    if( WS_dup_pe(pwsi, proto, dup_flag) )
		return (struct WIN_protoent*)(pwsi->pe);
	    else SetLastError(WSAENOBUFS);
	else SetLastError((h_errno < 0) ? wsaErrno() : wsaHerrno());
    }
    return NULL;
}

SEGPTR WINAPI WINSOCK_getprotobyname16(const char *name)
{
    struct WIN_protoent* retval;
    TRACE("%s\n", (name)?name:NULL_STRING);
    retval = __ws_getprotobyname(name, WS_DUP_SEGPTR);
    return retval ? SEGPTR_GET(retval) : ((SEGPTR)NULL);
}

struct WIN_protoent* WINAPI WINSOCK_getprotobyname(const char* name)
{
    TRACE("%s\n", (name)?name:NULL_STRING);
    return __ws_getprotobyname(name, WS_DUP_LINEAR);
}


/***********************************************************************
 *		getprotobynumber()	(WINSOCK.54)(WSOCK32.54)
 */
static struct WIN_protoent* __ws_getprotobynumber(int number, int dup_flag)
{
    LPWSINFO              pwsi = WINSOCK_GetIData();

    if( pwsi )
    {
	struct protoent*     proto;
	if( (proto = getprotobynumber(number)) != NULL )
	    if( WS_dup_pe(pwsi, proto, dup_flag) )
		return (struct WIN_protoent*)(pwsi->pe);
	    else SetLastError(WSAENOBUFS);
	else SetLastError(WSANO_DATA);
    }
    return NULL;
}

SEGPTR WINAPI WINSOCK_getprotobynumber16(INT16 number)
{
    struct WIN_protoent* retval;
    TRACE("%i\n", number);
    retval = __ws_getprotobynumber(number, WS_DUP_SEGPTR);
    return retval ? SEGPTR_GET(retval) : ((SEGPTR)NULL);
}

struct WIN_protoent* WINAPI WINSOCK_getprotobynumber(INT number)
{
    TRACE("%i\n", number);
    return __ws_getprotobynumber(number, WS_DUP_LINEAR);
}


/***********************************************************************
 *		getservbyname()		(WINSOCK.55)(WSOCK32.55)
 */
struct WIN_servent* __ws_getservbyname(const char *name, const char *proto, int dup_flag)
{
    LPWSINFO              pwsi = WINSOCK_GetIData();

    if( pwsi )
    {
	struct servent*     serv;
	int i = wsi_strtolo( pwsi, name, proto );

	if( i )
	    if( (serv = getservbyname(pwsi->buffer, pwsi->buffer + i)) != NULL )
		if( WS_dup_se(pwsi, serv, dup_flag) )
		    return (struct WIN_servent*)(pwsi->se);
		else SetLastError(WSAENOBUFS);
	    else SetLastError((h_errno < 0) ? wsaErrno() : wsaHerrno());
	else SetLastError(WSAENOBUFS);
    }
    return NULL;
}

SEGPTR WINAPI WINSOCK_getservbyname16(const char *name, const char *proto)
{
    struct WIN_servent* retval;
    TRACE("'%s', '%s'\n",
                            (name)?name:NULL_STRING, (proto)?proto:NULL_STRING);
    retval = __ws_getservbyname(name, proto, WS_DUP_SEGPTR);
    return retval ? SEGPTR_GET(retval) : ((SEGPTR)NULL);
}

struct WIN_servent* WINAPI WINSOCK_getservbyname(const char *name, const char *proto)
{
    TRACE("'%s', '%s'\n",
                            (name)?name:NULL_STRING, (proto)?proto:NULL_STRING);
    return __ws_getservbyname(name, proto, WS_DUP_LINEAR);
}


/***********************************************************************
 *		getservbyport()		(WINSOCK.56)(WSOCK32.56)
 */
static struct WIN_servent* __ws_getservbyport(int port, const char* proto, int dup_flag)
{
    LPWSINFO              pwsi = WINSOCK_GetIData();

    if( pwsi )
    {
	struct servent*     serv;
	int i = wsi_strtolo( pwsi, proto, NULL );

	if( i )
	    if( (serv = getservbyport(port, pwsi->buffer)) != NULL )
		if( WS_dup_se(pwsi, serv, dup_flag) )
		    return (struct WIN_servent*)(pwsi->se);
		else SetLastError(WSAENOBUFS);
	    else SetLastError((h_errno < 0) ? wsaErrno() : wsaHerrno());
	else SetLastError(WSAENOBUFS);
    }
    return NULL;
}

SEGPTR WINAPI WINSOCK_getservbyport16(INT16 port, const char *proto)
{
    struct WIN_servent* retval;
    TRACE("%i, '%s'\n",
                            (int)port, (proto)?proto:NULL_STRING);
    retval = __ws_getservbyport(port, proto, WS_DUP_SEGPTR);
    return retval ? SEGPTR_GET(retval) : ((SEGPTR)NULL);
}

struct WIN_servent* WINAPI WINSOCK_getservbyport(INT port, const char *proto)
{
    TRACE("%i, '%s'\n",
                            (int)port, (proto)?proto:NULL_STRING);
    return __ws_getservbyport(port, proto, WS_DUP_LINEAR);
}


/***********************************************************************
 *              gethostname()           (WSOCK32.57)
 */
INT WINAPI WINSOCK_gethostname(char *name, INT namelen)
{
    LPWSINFO              pwsi = WINSOCK_GetIData();

    TRACE("(%08x): name %s, len %d\n",
                          (unsigned)pwsi, (name)?name:NULL_STRING, namelen);
    if( pwsi )
    {
	if (gethostname(name, namelen) == 0) return 0;
	SetLastError((errno == EINVAL) ? WSAEFAULT : wsaErrno());
    }
    return SOCKET_ERROR;
}

/***********************************************************************
 *              gethostname()           (WINSOCK.57)
 */
INT16 WINAPI WINSOCK_gethostname16(char *name, INT16 namelen)
{
    return (INT16)WINSOCK_gethostname(name, namelen);
}


/* ------------------------------------- Windows sockets extensions -- *
 *								       *
 * ------------------------------------------------------------------- */

int WINAPI WSAEnumNetworkEvents(SOCKET s, WSAEVENT hEvent, LPWSANETWORKEVENTS lpEvent)
{
    LPWSINFO      pwsi = WINSOCK_GetIData();
    struct get_socket_event_request *req = get_req_buffer();

    TRACE("(%08x): %08x, hEvent %08x, lpEvent %08x\n",
			  (unsigned)pwsi, s, hEvent, (unsigned)lpEvent );
    if( _check_ws(pwsi, s) )
    {
	req->handle  = s;
	req->service = TRUE;
	req->s_event = 0;
	sock_server_call( REQ_GET_SOCKET_EVENT );
	lpEvent->lNetworkEvents = req->pmask;
	memcpy(lpEvent->iErrorCode, req->errors, sizeof(lpEvent->iErrorCode));
	if (hEvent)
	    ResetEvent(hEvent);
	return 0;
    }
    else SetLastError(WSAEINVAL);
    return SOCKET_ERROR; 
}

int WINAPI WSAEventSelect(SOCKET s, WSAEVENT hEvent, LONG lEvent)
{
    LPWSINFO      pwsi = WINSOCK_GetIData();
    struct set_socket_event_request *req = get_req_buffer();

    TRACE("(%08x): %08x, hEvent %08x, event %08x\n",
			  (unsigned)pwsi, s, hEvent, (unsigned)lEvent );
    if( _check_ws(pwsi, s) )
    {
	req->handle = s;
	req->mask   = lEvent;
	req->event  = hEvent;
	sock_server_call( REQ_SET_SOCKET_EVENT );
	return 0;
    }
    else SetLastError(WSAEINVAL);
    return SOCKET_ERROR; 
}

/***********************************************************************
 *      WSAAsyncSelect()		(WINSOCK.101)(WSOCK32.101)
 */

VOID CALLBACK WINSOCK_DoAsyncEvent( ULONG_PTR ptr )
{
    /* FIXME: accepted socket uses same event object as listening socket by default
     * (at least before a new WSAAsyncSelect is issued), must handle it somehow */
    ws_select_info *info = (ws_select_info*)ptr;
    struct get_socket_event_request *req = get_req_buffer();
    unsigned int i, pmask;

    TRACE("socket %08x, event %08x\n", info->sock, info->event);
    SetLastError(0);
    req->handle  = info->sock;
    req->service = TRUE;
    req->s_event = info->event; /* <== avoid race conditions */
    sock_server_call( REQ_GET_SOCKET_EVENT );
    if ( GetLastError() == WSAEINVAL )
    {
	/* orphaned event (socket closed or something) */
	TRACE("orphaned event, self-destructing\n");
	SERVICE_Delete( info->service );
	WS_FREE(info);
	return;
    }
    /* dispatch network events */
    pmask = req->pmask;
    for (i=0; i<FD_MAX_EVENTS; i++)
	if (pmask & (1<<i)) {
	    TRACE("post: event bit %d, error %d\n", i, req->errors[i]);
	    PostMessageA(info->hWnd, info->uMsg, info->sock,
			 WSAMAKESELECTREPLY(1<<i, req->errors[i]));
	}
}

INT WINAPI WSAAsyncSelect(SOCKET s, HWND hWnd, UINT uMsg, LONG lEvent)
{
    LPWSINFO      pwsi = WINSOCK_GetIData();

    TRACE("(%08x): %04x, hWnd %04x, uMsg %08x, event %08x\n",
			  (unsigned)pwsi, (SOCKET16)s, (HWND16)hWnd, uMsg, (unsigned)lEvent );
    if( _check_ws(pwsi, s) )
    {
	if( lEvent )
	{
	    ws_select_info *info = (ws_select_info*)WS_ALLOC(sizeof(ws_select_info));
	    if( info )
	    {
		HANDLE hObj = CreateEventA( NULL, FALSE, FALSE, NULL );
		INT err;
		
		info->sock  = s;
		info->event = hObj;
		info->hWnd  = hWnd;
		info->uMsg  = uMsg;
		info->service = SERVICE_AddObject( hObj, WINSOCK_DoAsyncEvent, (ULONG_PTR)info );

		err = WSAEventSelect( s, hObj, lEvent | WS_FD_SERVEVENT );
		if (err) {
		    SERVICE_Delete( info->service );
		    WS_FREE(info);
		    return err;
		}

		return 0; /* success */
	    }
	    else SetLastError(WSAENOBUFS);
	} 
	else
	{
	    WSAEventSelect(s, 0, 0);
	    return 0;
	}
    } 
    else SetLastError(WSAEINVAL);
    return SOCKET_ERROR; 
}

INT16 WINAPI WSAAsyncSelect16(SOCKET16 s, HWND16 hWnd, UINT16 wMsg, LONG lEvent)
{
    return (INT16)WSAAsyncSelect( s, hWnd, wMsg, lEvent );
}

/***********************************************************************
 *		WSARecvEx()			(WSOCK32.1107)
 *
 * WSARecvEx is a Microsoft specific extension to winsock that is identical to recv
 * except that has an in/out argument call flags that has the value MSG_PARTIAL ored
 * into the flags parameter when a partial packet is read. This only applies to
 * sockets using the datagram protocol. This method does not seem to be implemented
 * correctly by microsoft as the winsock implementation does not set the MSG_PARTIAL
 * flag when a fragmented packet arrives.
 */
INT     WINAPI   WSARecvEx(SOCKET s, char *buf, INT len, INT *flags) {
  FIXME("(WSARecvEx) partial packet return value not set \n");

  return WINSOCK_recv(s, buf, len, *flags);
}


/***********************************************************************
 *              WSARecvEx16()			(WINSOCK.1107)
 *
 * See description for WSARecvEx()
 */
INT16     WINAPI WSARecvEx16(SOCKET16 s, char *buf, INT16 len, INT16 *flags) {
  FIXME("(WSARecvEx16) partial packet return value not set \n");

  return WINSOCK_recv16(s, buf, len, *flags);
}


/***********************************************************************
 *	__WSAFDIsSet()			(WINSOCK.151)
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
 *      __WSAFDIsSet()			(WSOCK32.151)
 */
INT WINAPI __WSAFDIsSet(SOCKET s, ws_fd_set32 *set)
{
  int i = set->fd_count;

  TRACE("(%d,%8lx(%i))\n", s,(unsigned long)set, i);

  while (i--)
      if (set->fd_array[i] == s) return 1;
  return 0;
}

/***********************************************************************
 *      WSAIsBlocking()			(WINSOCK.114)(WSOCK32.114)
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
 *      WSACancelBlockingCall()		(WINSOCK.113)(WSOCK32.113)
 */
INT WINAPI WSACancelBlockingCall(void)
{
  LPWSINFO              pwsi = WINSOCK_GetIData();

  TRACE("(%08x)\n", (unsigned)pwsi);

  if( pwsi ) return 0;
  return SOCKET_ERROR;
}


/***********************************************************************
 *      WSASetBlockingHook16()		(WINSOCK.109)
 */
FARPROC16 WINAPI WSASetBlockingHook16(FARPROC16 lpBlockFunc)
{
  FARPROC16		prev;
  LPWSINFO              pwsi = WINSOCK_GetIData();

  TRACE("(%08x): hook %08x\n", 
	       (unsigned)pwsi, (unsigned) lpBlockFunc);
  if( pwsi ) 
  { 
      prev = (FARPROC16)pwsi->blocking_hook; 
      pwsi->blocking_hook = (DWORD)lpBlockFunc; 
      pwsi->flags &= ~WSI_BLOCKINGHOOK;
      return prev; 
  }
  return 0;
}


/***********************************************************************
 *      WSASetBlockingHook32()
 */
FARPROC WINAPI WSASetBlockingHook(FARPROC lpBlockFunc)
{
  FARPROC             prev;
  LPWSINFO              pwsi = WINSOCK_GetIData();

  TRACE("(%08x): hook %08x\n",
	       (unsigned)pwsi, (unsigned) lpBlockFunc);
  if( pwsi ) {
      prev = (FARPROC)pwsi->blocking_hook;
      pwsi->blocking_hook = (DWORD)lpBlockFunc;
      pwsi->flags |= WSI_BLOCKINGHOOK;
      return prev;
  }
  return NULL;
}


/***********************************************************************
 *      WSAUnhookBlockingHook16()	(WINSOCK.110)
 */
INT16 WINAPI WSAUnhookBlockingHook16(void)
{
    LPWSINFO              pwsi = WINSOCK_GetIData();

    TRACE("(%08x)\n", (unsigned)pwsi);
    if( pwsi ) return (INT16)(pwsi->blocking_hook = 0);
    return SOCKET_ERROR;
}


/***********************************************************************
 *      WSAUnhookBlockingHook32()
 */
INT WINAPI WSAUnhookBlockingHook(void)
{
    LPWSINFO              pwsi = WINSOCK_GetIData();

    TRACE("(%08x)\n", (unsigned)pwsi);
    if( pwsi )
    {
	pwsi->blocking_hook = 0;
	pwsi->flags &= ~WSI_BLOCKINGHOOK;
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
      
      FIXME("(ICMP_ECHO) to 0x%08x stub \n", addr);
      break;
    }
  default:
    FIXME("(%lx,%lx,%p,%p,%p,%p) stub\n",
	  protocoll,action,inbuf,inbuflen,outbuf,outbuflen);
  }
  return FALSE;
}
/*********************************************************
 *       WS_s_perror         WSOCK32.1108 
 */
void WINAPI WS_s_perror(LPCSTR message)
{
    FIXME("(%s): stub\n",message);
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

     _check_buffer_he(pwsi, size);
     p_to = (struct ws_hostent*)pwsi->he;
     p = (char*)pwsi->he;
     p_base = (flag & WS_DUP_OFFSET) ? NULL
				     : ((flag & WS_DUP_SEGPTR) ? (char*)SEGPTR_GET(p) : p);
     p += sizeof(struct ws_hostent);
     p_name = p;
     strcpy(p, p_he->h_name); p += strlen(p) + 1;
     p_aliases = p;
     p += list_dup(p_he->h_aliases, p, p_base + (p - (char*)pwsi->he), 0);
     p_addr = p;
     list_dup(p_he->h_addr_list, p, p_base + (p - (char*)pwsi->he), p_he->h_length);

     p_to->h_addrtype = (INT16)p_he->h_addrtype; 
     p_to->h_length = (INT16)p_he->h_length;
     p_to->h_name = (SEGPTR)(p_base + (p_name - (char*)pwsi->he));
     p_to->h_aliases = (SEGPTR)(p_base + (p_aliases - (char*)pwsi->he));
     p_to->h_addr_list = (SEGPTR)(p_base + (p_addr - (char*)pwsi->he));

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

     _check_buffer_pe(pwsi, size);
     p_to = (struct ws_protoent*)pwsi->pe;
     p = (char*)pwsi->pe; 
     p_base = (flag & WS_DUP_OFFSET) ? NULL
				     : ((flag & WS_DUP_SEGPTR) ? (char*)SEGPTR_GET(p) : p);
     p += sizeof(struct ws_protoent);
     p_name = p;
     strcpy(p, p_pe->p_name); p += strlen(p) + 1;
     p_aliases = p;
     list_dup(p_pe->p_aliases, p, p_base + (p - (char*)pwsi->pe), 0);

     p_to->p_proto = (INT16)p_pe->p_proto;
     p_to->p_name = (SEGPTR)(p_base) + (p_name - (char*)pwsi->pe);
     p_to->p_aliases = (SEGPTR)((p_base) + (p_aliases - (char*)pwsi->pe)); 

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

     _check_buffer_se(pwsi, size);
     p_to = (struct ws_servent*)pwsi->se;
     p = (char*)pwsi->se;
     p_base = (flag & WS_DUP_OFFSET) ? NULL 
				     : ((flag & WS_DUP_SEGPTR) ? (char*)SEGPTR_GET(p) : p);
     p += sizeof(struct ws_servent);
     p_name = p;
     strcpy(p, p_se->s_name); p += strlen(p) + 1;
     p_proto = p;
     strcpy(p, p_se->s_proto); p += strlen(p) + 1;
     p_aliases = p;
     list_dup(p_se->s_aliases, p, p_base + (p - (char*)pwsi->se), 0);

     p_to->s_port = (INT16)p_se->s_port;
     p_to->s_name = (SEGPTR)(p_base + (p_name - (char*)pwsi->se));
     p_to->s_proto = (SEGPTR)(p_base + (p_proto - (char*)pwsi->se));
     p_to->s_aliases = (SEGPTR)(p_base + (p_aliases - (char*)pwsi->se)); 

     size += (sizeof(struct ws_servent) - sizeof(struct servent));
   }
   return size;
}

/* ----------------------------------- error handling */

UINT16 wsaErrno(void)
{
    int	loc_errno = errno; 
#ifdef HAVE_STRERROR
    WARN("errno %d, (%s).\n", loc_errno, strerror(loc_errno));
#else
    WARN("errno %d\n", loc_errno);
#endif

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

UINT16 wsaHerrno(void)
{
    int		loc_errno = h_errno;

    WARN("h_errno %d.\n", loc_errno);

    switch(loc_errno)
    {
	case HOST_NOT_FOUND:	return WSAHOST_NOT_FOUND;
	case TRY_AGAIN:		return WSATRY_AGAIN;
	case NO_RECOVERY:	return WSANO_RECOVERY;
	case NO_DATA:		return WSANO_DATA; 

	case 0:			return 0;
        default:
		WARN("Unknown h_errno %d!\n", loc_errno);
		return WSAEOPNOTSUPP;
    }
}
