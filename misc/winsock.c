/*
 * based on Windows Sockets 1.1 specs
 * (ftp.microsoft.com:/Advsys/winsock/spec11/WINSOCK.TXT)
 * 
 * (C) 1993,1994,1996 John Brezak, Erik Bos, Alex Korobka.
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
#endif
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>

#include "windows.h"
#include "winnt.h"
#include "heap.h"
#include "ldt.h"
#include "winsock.h"
#include "stddebug.h"
#include "debug.h"

#define dump_sockaddr(a) \
        fprintf(stderr, "sockaddr_in: family %d, address %s, port %d\n", \
                        ((struct sockaddr_in *)a)->sin_family, \
                        inet_ntoa(((struct sockaddr_in *)a)->sin_addr), \
                        ntohs(((struct sockaddr_in *)a)->sin_port))

extern void SIGNAL_MaskAsyncEvents( BOOL32 );

#pragma pack(4)

/* ----------------------------------- internal data */

extern int h_errno;
extern void __sigio(int);

ws_async_ctl            async_ctl;
int                     async_qid = -1;

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
        ((unsigned)((int)_ws_stub + (int)handle))

#define WSI_CHECK_RANGE(pwsi, pws) \
	( ((unsigned)(pws) > (unsigned)(pwsi)) && \
	  ((unsigned)(pws) < ((unsigned)(pwsi) + sizeof(WSINFO))) )

static INT16         _ws_sock_ops[] =
       { WS_SO_DEBUG, WS_SO_REUSEADDR, WS_SO_KEEPALIVE, WS_SO_DONTROUTE,
         WS_SO_BROADCAST, WS_SO_LINGER, WS_SO_OOBINLINE, WS_SO_SNDBUF,
         WS_SO_RCVBUF, WS_SO_ERROR, WS_SO_TYPE, WS_SO_DONTLINGER, 0 };
static int           _px_sock_ops[] =
       { SO_DEBUG, SO_REUSEADDR, SO_KEEPALIVE, SO_DONTROUTE, SO_BROADCAST,
         SO_LINGER, SO_OOBINLINE, SO_SNDBUF, SO_RCVBUF, SO_ERROR, SO_TYPE,
	 SO_LINGER };

static INT16 init_async_select(ws_socket* pws, HWND16 hWnd, UINT16 uMsg, UINT32 lEvent);
static int notify_client(ws_socket* pws, unsigned flag);

static int _check_ws(LPWSINFO pwsi, ws_socket* pws);
static int _check_buffer(LPWSINFO pwsi, int size);

static void fixup_wshe(struct ws_hostent* p_wshe, SEGPTR base);
static void fixup_wspe(struct ws_protoent* p_wspe, SEGPTR base);
static void fixup_wsse(struct ws_servent* p_wsse, SEGPTR base);

static int delete_async_op(ws_socket*);

static void convert_sockopt(INT16 *level, INT16 *optname)
{
  int           i;
  switch (*level)
  {
     case WS_SOL_SOCKET:
        *level = SOL_SOCKET;
        for(i=0; _ws_sock_ops[i]; i++)
            if( _ws_sock_ops[i] == *optname ) break;
        if( _ws_sock_ops[i] ) *optname = (INT16)_px_sock_ops[i];
        else fprintf(stderr, "convert_sockopt() unknown optname %d\n", *optname);
        break;
     case WS_IPPROTO_TCP:
        *optname = IPPROTO_TCP;
  }
}

static void _ws_global_init()
{
   if( !_ws_stub )
   {
     _WSHeap = HeapCreate(HEAP_ZERO_MEMORY, 8120, 32768);
     if( !(_ws_stub = WS_ALLOC(0x10)) )
       fprintf(stderr,"Fatal: failed to create WinSock heap\n");
   }
   if( async_qid == -1 ) 
     if( (async_qid = msgget(IPC_PRIVATE, IPC_CREAT | 0x1FF)) == -1 )
       fprintf(stderr,"Fatal: failed to create WinSock resource\n");
}

/* ----------------------------------- Per-thread info */

static void wsi_link(LPWSINFO pwsi)
{ if( _wsi_list ) _wsi_list->prev = pwsi;
  pwsi->next = _wsi_list; _wsi_list = pwsi; 
}

static void wsi_unlink(LPWSINFO pwsi)
{
  if( pwsi == _wsi_list ) _wsi_list = pwsi->next;
  else 
  { pwsi->prev->next = pwsi->next;
    if( pwsi->next ) pwsi->next->prev = pwsi->prev; } 
}

static LPWSINFO wsi_find(HTASK16 hTask)
{ LPWSINFO pwsi = _wsi_list;
  while( pwsi && pwsi->tid != hTask ) pwsi = pwsi->next;
  return pwsi; 
}

static ws_socket* wsi_alloc_socket(LPWSINFO pwsi, int fd)
{
  if( pwsi->last_free >= 0 )
  {
    int i = pwsi->last_free;

    pwsi->last_free = pwsi->sock[i].flags;
    pwsi->sock[i].fd = fd;
    pwsi->sock[i].flags = 0;
    return &pwsi->sock[i];
  }
  return NULL;
}

static void fd_set_normalize(fd_set* fds, LPWSINFO pwsi, ws_fd_set* ws, int* highfd)
{
  FD_ZERO(fds);
  if(ws) 
  { 
    int 	i;
    ws_socket*  pws;
    for(i=0;i<(ws->fd_count);i++) 
    {
      pws = (ws_socket*)WS_HANDLE2PTR(ws->fd_array[i]);
      if( _check_ws(pwsi, pws) ) 
      { 
        if( pws->fd > *highfd ) *highfd = pws->fd; 
        FD_SET(pws->fd, fds); 
      }
    }
  }
}

/*
 * Note weirdness here: sockets with errors belong in exceptfds, but
 * are given to us in readfds or writefds, so move them to exceptfds if
 * there is an error. Note that this means that exceptfds may have mysterious
 * sockets set in it that the program never asked for.
 */

static int inline sock_error_p(int s)
{
    unsigned int optval, optlen;

    optlen = sizeof(optval);
    getsockopt(s, SOL_SOCKET, SO_ERROR, &optval, &optlen);
    if (optval) dprintf_winsock(stddeb, "error: %d\n", optval);
    return optval != 0;
}

static void fd_set_update(LPWSINFO pwsi, fd_set* fds, ws_fd_set* ws,
			  fd_set *errorfds)
{
    if( ws )
    {
	int i, j, count = ws->fd_count;

	for( i = 0, j = 0; i < count; i++ )
	{
	    ws_socket *pws = (ws_socket*)WS_HANDLE2PTR(ws->fd_array[i]);
	    int fd = pws->fd;

	    if( _check_ws(pwsi, pws) && FD_ISSET(fd, fds) ) 
	    { 
		/* if error, move to errorfds */
		if (errorfds && (FD_ISSET(fd, errorfds) || sock_error_p(fd)))
		    FD_SET(fd, errorfds);
		else
		    ws->fd_array[j++] = ws->fd_array[i];
	    }
	}
	ws->fd_count = j;
	dprintf_winsock(stddeb, "\n");
    }
    return;
}

static void fd_set_update_except(LPWSINFO pwsi, fd_set *fds, ws_fd_set *ws,
				 fd_set *errorfds)
{
    if (ws)
    {
	int i, j, count = ws->fd_count;

	for (i=j=0; i < count; i++)
	{
	    ws_socket *pws = (ws_socket *)WS_HANDLE2PTR(ws->fd_array[i]);

	    if (_check_ws(pwsi, pws) && (FD_ISSET(pws->fd, fds)
					 || FD_ISSET(pws->fd, errorfds)))
	      ws->fd_array[j++] = ws->fd_array[i];
	}
	ws->fd_count = j;
    }
    return;
}

/* ----------------------------------- API ----- 
 *
 * Init / cleanup / error checking.
 */

INT16 WSAStartup(UINT16 wVersionRequested, LPWSADATA lpWSAData)
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
                        #else
                                "Unknown",
                        #endif
			   WS_MAX_SOCKETS_PER_THREAD,
			   WS_MAX_UDP_DATAGRAM, NULL };
  HTASK16               tid = GetCurrentTask();
  LPWSINFO              pwsi;

  dprintf_winsock(stddeb, "WSAStartup: verReq=%x\n", wVersionRequested);

  if (LOBYTE(wVersionRequested) < 1 || (LOBYTE(wVersionRequested) == 1 &&
      HIBYTE(wVersionRequested) < 1)) return WSAVERNOTSUPPORTED;

  if (!lpWSAData) return WSAEINVAL;

  _ws_global_init();
  if( _WSHeap == 0 ) return WSASYSNOTREADY;
  
  pwsi = wsi_find(GetCurrentTask());
  if( pwsi == NULL )
  {
    if( (pwsi = (LPWSINFO)WS_ALLOC( sizeof(WSINFO))) )
    {
      int i = 0;
      pwsi->tid = tid;
      for( i = 0; i < WS_MAX_SOCKETS_PER_THREAD; i++ )
      { 
	pwsi->sock[i].fd = -1; 
	pwsi->sock[i].flags = i + 1; 
      }
      pwsi->sock[WS_MAX_SOCKETS_PER_THREAD - 1].flags = -1;
    } 
    else return WSASYSNOTREADY;
    wsi_link(pwsi);
  } else pwsi->num_startup++;

  /* return winsock information */
  memcpy(lpWSAData, &WINSOCK_data, sizeof(WINSOCK_data));

  dprintf_winsock(stddeb, "WSAStartup: succeeded\n");
  return(0);
}

INT16 WSACleanup(void)
{
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  /* FIXME: do global cleanup if no current task */

  dprintf_winsock(stddeb, "WSACleanup(%08x)\n", (unsigned)pwsi);
  if( pwsi )
  {
      int	i, j, n;

      if( pwsi->num_startup-- ) return 0;

      SIGNAL_MaskAsyncEvents( TRUE );
      WINSOCK_cancel_async_op(GetCurrentTask());
      SIGNAL_MaskAsyncEvents( FALSE );

      wsi_unlink(pwsi);
      if( _wsi_list == NULL && async_qid != -1 )
        if( msgctl(async_qid, IPC_RMID, NULL) == -1 )
        { 
          fprintf(stderr,"failed to delete WS message queue.\n");
        } else async_qid = -1;

      if( pwsi->flags & WSI_BLOCKINGCALL )
	  dprintf_winsock(stddeb,"\tinside blocking call!\n");
      if( pwsi->num_async_rq )
	  dprintf_winsock(stddeb,"\thave %i outstanding async ops!\n", pwsi->num_async_rq );

      for(i = 0, j = 0, n = 0; i < WS_MAX_SOCKETS_PER_THREAD; i++)
	if( pwsi->sock[i].fd != -1 )
	{ 
	  n += delete_async_op(&pwsi->sock[i]);
          close(pwsi->sock[i].fd); j++; 
        }
      if( j ) 
	  dprintf_winsock(stddeb,"\tclosed %i sockets, killed %i async selects!\n", j, n);

      if( pwsi->buffer ) SEGPTR_FREE(pwsi->buffer);
      WS_FREE(pwsi);
      return 0;
  }
  return SOCKET_ERROR;
}

INT16 WSAGetLastError(void)
{
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());
  INT16		ret;

  dprintf_winsock(stddeb, "WSAGetLastError(%08x)", (unsigned)pwsi);

  ret = (pwsi) ? pwsi->errno : WSANOTINITIALISED;

  dprintf_winsock(stddeb, " = %i\n", (int)ret);
  return ret;
}

void WSASetLastError(INT16 iError)
{
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WSASetLastError(%08x): %d\n", (unsigned)pwsi, (int)iError);

  if( pwsi ) pwsi->errno = iError;
}

int _check_ws(LPWSINFO pwsi, ws_socket* pws)
{
  if( pwsi )
    if( pwsi->flags & WSI_BLOCKINGCALL ) pwsi->errno = WSAEINPROGRESS;
    else if( WSI_CHECK_RANGE(pwsi, pws) ) return 1;
    else pwsi->errno = WSAENOTSOCK;
  return 0;
}

int _check_buffer(LPWSINFO pwsi, int size)
{
  if( pwsi->buffer && pwsi->buflen >= size ) return 1;
  else SEGPTR_FREE(pwsi->buffer);
  pwsi->buffer = (char*)SEGPTR_ALLOC((pwsi->buflen = size)); 
  return (pwsi->buffer != NULL);
}

/* ----- socket operations */

SOCKET16 WINSOCK_accept(SOCKET16 s, struct sockaddr *addr, INT16 *addrlen16)
{
  ws_socket*	pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO	pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_ACCEPT(%08x): socket %04x\n", 
				   (unsigned)pwsi, (UINT16)s); 
  if( _check_ws(pwsi, pws) )
  {
     int 	sock, fd_flags, addrlen32 = *addrlen16;

     /* this is how block info is supposed to be used -
      * WSAIsBlocking() would then check WSI_BLOCKINGCALL bit.
      */

     fd_flags = fcntl(pws->fd, F_GETFL, 0);
     if( !(fd_flags & O_NONBLOCK) ) pwsi->flags |= WSI_BLOCKINGCALL; 

     if( (sock = accept(pws->fd, addr, &addrlen32)) >= 0 )
     {
        ws_socket*	pnew = wsi_alloc_socket(pwsi, sock); 
	notify_client(pws, WS_FD_ACCEPT);
        if( pnew )
        {
	  if( pws->p_aop )
	      init_async_select(pnew, pws->p_aop->hWnd,
				      pws->p_aop->uMsg,
				      pws->p_aop->flags & ~WS_FD_ACCEPT );

	  pwsi->flags &= ~WSI_BLOCKINGCALL; 
	  return (SOCKET16)WS_PTR2HANDLE(pnew);
        } 
	else pwsi->errno = WSAENOBUFS;
     } 
     else pwsi->errno = wsaErrno();

     pwsi->flags &= ~WSI_BLOCKINGCALL;
  }
  return INVALID_SOCKET;
}

INT16 WINSOCK_bind(SOCKET16 s, struct sockaddr *name, INT16 namelen)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_BIND(%08x): socket %04x, ptr %8x, length %d\n", 
			   (unsigned)pwsi, s, (int) name, namelen);
#if 0
  dump_sockaddr(name);
#endif

  if ( _check_ws(pwsi, pws) )
    if (namelen >= sizeof(*name)) 
       if ( ((struct sockaddr_in *)name)->sin_family == AF_INET )
	  if ( bind(pws->fd, name, namelen) < 0 ) 
	  {
	     int	loc_errno = errno;
	     dprintf_winsock(stddeb,"\tfailure - errno = %i\n", errno);
	     errno = loc_errno;
	     switch(errno)
	     {
		case EBADF: pwsi->errno = WSAENOTSOCK; break;
		case EADDRNOTAVAIL: pwsi->errno = WSAEINVAL; break;
		default: pwsi->errno = wsaErrno();
	     }
	  }
	  else return 0;
       else pwsi->errno = WSAEAFNOSUPPORT;
    else pwsi->errno = WSAEFAULT;
  return SOCKET_ERROR;
}

INT16 WINSOCK_closesocket(SOCKET16 s)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_CLOSE(%08x): socket %08x\n", (unsigned)pwsi, s);

  if( _check_ws(pwsi, pws) )
  { 
    int		fd = pws->fd;

    delete_async_op(pws);
    pws->p_aop = NULL; pws->fd = -1;
    pws->flags = (unsigned)pwsi->last_free;
    pwsi->last_free = pws - &pwsi->sock[0];
    if (close(fd) < 0) pwsi->errno = (errno == EBADF) ? WSAENOTSOCK : wsaErrno();
    else return 0;
  }
  return SOCKET_ERROR;
}

INT16 WINSOCK_connect(SOCKET16 s, struct sockaddr *name, INT16 namelen)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_CONNECT(%08x): socket %04x, ptr %8x, length %d\n", 
			   (unsigned)pwsi, s, (int) name, namelen);
#if 0
  dump_sockaddr(name);
#endif

  if( _check_ws(pwsi, pws) )
  {
    if (connect(pws->fd, name, namelen) == 0) 
    { 
	if( pws->p_aop )
	    /* we need to notify handler process if
	     * connect() succeeded NOT in response to winsock message
	     */
	    notify_client(pws, WS_FD_CONNECTED);

        pws->flags &= ~(WS_FD_INACTIVE | WS_FD_CONNECT); 
        return 0; 
    }
    pwsi->errno = (errno == EINPROGRESS) ? WSAEWOULDBLOCK : wsaErrno();
  }
  return SOCKET_ERROR;
}

INT16 WINSOCK_getpeername(SOCKET16 s, struct sockaddr *name, INT16 *namelen)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_GETPEERNAME(%08x): socket: %04x, ptr %8x, ptr %8x\n", 
			   (unsigned)pwsi, s, (int) name, *namelen);
  if( _check_ws(pwsi, pws) )
  {
    int	namelen32 = *namelen;
    if (getpeername(pws->fd, name, &namelen32) == 0) 
    { 
#if 0
	dump_sockaddr(name);
#endif
       *namelen = (INT16)namelen32; 
        return 0; 
    }
    pwsi->errno = (h_errno < 0) ? wsaErrno() : wsaHerrno();
  }
  return SOCKET_ERROR;
}

INT16 WINSOCK_getsockname(SOCKET16 s, struct sockaddr *name, INT16 *namelen)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_GETSOCKNAME(%08x): socket: %04x, ptr %8x, ptr %8x\n", 
			  (unsigned)pwsi, s, (int) name, (int) *namelen);
  if( _check_ws(pwsi, pws) )
  {
    int namelen32 = *namelen;
    if (getsockname(pws->fd, name, &namelen32) == 0)
    { 
	*namelen = (INT16)namelen32; 
	 return 0; 
    }
    pwsi->errno = (h_errno < 0) ? wsaErrno() : wsaHerrno();
  }
  return SOCKET_ERROR;
}

INT16 WINSOCK_getsockopt(SOCKET16 s, INT16 level, 
			 INT16 optname, char *optval, INT16 *optlen)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_GETSOCKOPT(%08x): socket: %04x, opt %d, ptr %8x, ptr %8x\n", 
			   (unsigned)pwsi, s, level, (int) optval, (int) *optlen);

  if( _check_ws(pwsi, pws) )
  {
     int	optlen32 = *optlen;

     convert_sockopt(&level, &optname);
     if (getsockopt(pws->fd, (int) level, optname, optval, &optlen32) == 0 )
     { *optlen = (INT16)optlen32; return 0; }
     pwsi->errno = (errno == EBADF) ? WSAENOTSOCK : wsaErrno();
  }
  return SOCKET_ERROR;
}

u_long  WINSOCK_htonl(u_long hostlong)   { return( htonl(hostlong) ); }         
u_short WINSOCK_htons(u_short hostshort) { return( htons(hostshort) ); }
u_long  WINSOCK_inet_addr(char *cp)      { return( inet_addr(cp) ); }
u_long  WINSOCK_ntohl(u_long netlong)    { return( ntohl(netlong) ); }
u_short WINSOCK_ntohs(u_short netshort)  { return( ntohs(netshort) ); }

SEGPTR WINSOCK_inet_ntoa(struct in_addr in)
{
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());
  char*		s = inet_ntoa(in);

  if( pwsi )
  {
    if( s == NULL ) { pwsi->errno = wsaErrno(); return NULL; }
    if( _check_buffer( pwsi, 32 ) )
    { 
      strncpy(pwsi->buffer, s, 32 );
      return SEGPTR_GET(pwsi->buffer); 
    }
    pwsi->errno = WSAENOBUFS;
  }
  return (SEGPTR)NULL;
}

INT16 WINSOCK_ioctlsocket(SOCKET16 s, UINT32 cmd, UINT32 *argp)
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
	case WS_FIONREAD:   newcmd=FIONREAD; break;
	case WS_FIONBIO:    newcmd=FIONBIO;  
			    if( pws->p_aop && *argp == 0 ) 
			    { 
				pwsi->errno = WSAEINVAL; 
				return SOCKET_ERROR; 
			    }
			    break;
	case WS_SIOCATMARK: newcmd=SIOCATMARK; break;
	case WS_IOW('f',125,u_long): 
			  fprintf(stderr,"Warning: WS1.1 shouldn't be using async I/O\n");
			  pwsi->errno = WSAEINVAL; return SOCKET_ERROR;
	default:	  fprintf(stderr,"Warning: Unknown WS_IOCTL cmd (%08x)\n", cmd);
    }
    if( ioctl(pws->fd, newcmd, (char*)argp ) == 0 ) return 0;
    pwsi->errno = (errno == EBADF) ? WSAENOTSOCK : wsaErrno(); 
  }
  return SOCKET_ERROR;
}

INT16 WINSOCK_listen(SOCKET16 s, INT16 backlog)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_LISTEN(%08x): socket %04x, backlog %d\n", 
			  (unsigned)pwsi, s, backlog);
  if( _check_ws(pwsi, pws) )
  {
    if( !pws->p_aop )
    {
      int  fd_flags = fcntl(pws->fd, F_GETFL, 0);
      if( !(fd_flags & O_NONBLOCK) ) pws->flags |= WS_FD_ACCEPT;
    }
    else notify_client(pws, WS_FD_ACCEPT);

    if (listen(pws->fd, backlog) == 0) return 0;
    pwsi->errno = wsaErrno();
  }
  return SOCKET_ERROR;
}

INT16 WINSOCK_recv(SOCKET16 s, char *buf, INT16 len, INT16 flags)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_RECV(%08x): socket %04x, buf %8x, len %d, flags %d",
                          (unsigned)pwsi, s, (unsigned)buf, len, flags);
  if( _check_ws(pwsi, pws) )
  {
    int length;
    if ((length = recv(pws->fd, buf, len, flags)) >= 0) 
    { 
	dprintf_winsock(stddeb, " -> %i bytes\n", length);
	notify_client(pws, WS_FD_READ);
	return (INT16)length;
    }
    pwsi->errno = wsaErrno();
  }
  dprintf_winsock(stddeb, " -> ERROR\n");
  return SOCKET_ERROR;
}

INT16 WINSOCK_recvfrom(SOCKET16 s, char *buf, INT16 len, INT16 flags, 
		struct sockaddr *from, INT16 *fromlen16)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_RECVFROM(%08x): socket %04x, ptr %08x, len %d, flags %d\n",
                          (unsigned)pwsi, s, (unsigned)buf, len, flags);
  if( _check_ws(pwsi, pws) )
  {
    int length, fromlen32 = *fromlen16;
    if ((length = recvfrom(pws->fd, buf, len, flags, from, &fromlen32)) >= 0) 
    {   
      *fromlen16 = fromlen32; 
       notify_client(pws, WS_FD_READ);
       return (INT16)length;
    }
    pwsi->errno = wsaErrno();
  }
  return SOCKET_ERROR;
}

INT16 WINSOCK_select(INT16 nfds, ws_fd_set *ws_readfds,
				 ws_fd_set *ws_writefds,
				 ws_fd_set *ws_exceptfds, struct timeval *timeout)
{
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());
	
  dprintf_winsock(stddeb, "WS_SELECT(%08x): nfds %d (ignored), read %8x, write %8x, excp %8x\n", 
	  (unsigned) pwsi, nfds, (unsigned) ws_readfds, (unsigned) ws_writefds, (unsigned) ws_exceptfds);

  if( pwsi )
  {
     int         highfd = 0;
     fd_set      readfds, writefds, exceptfds, errorfds;

     fd_set_normalize(&readfds, pwsi, ws_readfds, &highfd);
     fd_set_normalize(&writefds, pwsi, ws_writefds, &highfd);
     fd_set_normalize(&exceptfds, pwsi, ws_exceptfds, &highfd);
     FD_ZERO(&errorfds);

     if( (highfd = select(highfd + 1, &readfds, &writefds, &exceptfds, timeout)) >= 0 )
     {
	  if( highfd )
	  {
	    fd_set_update(pwsi, &readfds, ws_readfds, &errorfds);
	    fd_set_update(pwsi, &writefds, ws_writefds, &errorfds);
 	    fd_set_update_except(pwsi, &exceptfds, ws_exceptfds, &errorfds);
	  }
	  return highfd; 
     }
     pwsi->errno = wsaErrno();
  } 
  return SOCKET_ERROR;
}

INT16 WINSOCK_send(SOCKET16 s, char *buf, INT16 len, INT16 flags)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_SEND(%08x): socket %04x, ptr %08x, length %d, flags %d\n", 
			  (unsigned)pwsi, s, (unsigned) buf, len, flags);
  if( _check_ws(pwsi, pws) )
  {
    int		length;
    if ((length = send(pws->fd, buf, len, flags)) < 0 ) 
    {  
	length = SOCKET_ERROR;
	pwsi->errno = wsaErrno();
    }
    notify_client(pws, WS_FD_WRITE);
    return (INT16)length;
  }
  return SOCKET_ERROR;
}

INT16 WINSOCK_sendto(SOCKET16 s, char *buf, INT16 len, INT16 flags,
		     struct sockaddr *to, INT16 tolen)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_SENDTO(%08x): socket %04x, ptr %08x, length %d, flags %d\n",
                          (unsigned)pwsi, s, (unsigned) buf, len, flags);
  if( _check_ws(pwsi, pws) )
  {
    int		length;

    if ((length = sendto(pws->fd, buf, len, flags, to, tolen)) < 0 )
    {
	length = SOCKET_ERROR;
        pwsi->errno = wsaErrno();
    }
    notify_client(pws, WS_FD_WRITE);
    return (INT16)length;
  }
  return SOCKET_ERROR;
}

INT16 WINSOCK_setsockopt(SOCKET16 s, INT16 level, INT16 optname, 
			 char *optval, INT16 optlen)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_SETSOCKOPT(%08x): socket %04x, level %d, opt %d, ptr %08x, len %d\n",
			  (unsigned)pwsi, s, level, optname, (int) optval, optlen);
  if( _check_ws(pwsi, pws) )
  {
    int		linger32[2];
    convert_sockopt(&level, &optname);
    if( optname == SO_LINGER )
    {
	INT16*	   ptr = (INT16*)optval;
        linger32[0] = ptr[0];
	linger32[1] = ptr[1]; 
	optval = (char*)&linger32;
	optlen = sizeof(linger32);
    }
    if (setsockopt(pws->fd, level, optname, optval, optlen) == 0) return 0;
    pwsi->errno = wsaErrno();
  }
  return SOCKET_ERROR;
}

INT16 WINSOCK_shutdown(SOCKET16 s, INT16 how)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_SHUTDOWN(%08x): socket %04x, how %i\n",
			   (unsigned)pwsi, s, how );
  if( _check_ws(pwsi, pws) )
  {
    pws->flags  = WS_FD_INACTIVE;
    delete_async_op(pws);

    if (shutdown(pws->fd, how) == 0) return 0;
    pwsi->errno = wsaErrno();
  }
  return SOCKET_ERROR;
}

SOCKET16 WINSOCK_socket(INT16 af, INT16 type, INT16 protocol)
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
	default:        pwsi->errno = WSAEAFNOSUPPORT; return INVALID_SOCKET;
    }

    /* check the socket type */
    switch(type) 
    {
	case SOCK_STREAM:
	case SOCK_DGRAM:
	case SOCK_RAW: break;
	default:       pwsi->errno = WSAESOCKTNOSUPPORT; return INVALID_SOCKET;
    }

    /* check the protocol type */
    if ( protocol < 0 )  /* don't support negative values */
    { pwsi->errno = WSAEPROTONOSUPPORT; return INVALID_SOCKET; }

    if ( af == AF_UNSPEC)  /* did they not specify the address family? */
        switch(protocol) 
	{
          case IPPROTO_TCP:
             if (type == SOCK_STREAM) { af = AF_INET; break; }
          case IPPROTO_UDP:
             if (type == SOCK_DGRAM)  { af = AF_INET; break; }
          default: pwsi->errno = WSAEPROTOTYPE; return INVALID_SOCKET;
        }

    if ((sock = socket(af, type, protocol)) >= 0) 
    {
        ws_socket*      pnew = wsi_alloc_socket(pwsi, sock);

/*	printf("created %04x (%i)\n", sock, (UINT16)WS_PTR2HANDLE(pnew));
 */
        if( pnew ) return (SOCKET16)WS_PTR2HANDLE(pnew);
        else
	{
	    close(sock);
	    pwsi->errno = WSAENOBUFS;
	    return INVALID_SOCKET;
	}
    }

    if (errno == EPERM) /* raw socket denied */
    {
        fprintf(stderr, "WS_SOCKET: not enough privileges\n");
        pwsi->errno = WSAESOCKTNOSUPPORT;
    } else pwsi->errno = wsaErrno();
  }
 
  dprintf_winsock(stddeb, "\t\tfailed!\n");
  return INVALID_SOCKET;
}
    

/* ----- database functions 
 *
 * Note that ws_...ent structures we return have SEGPTR pointers inside them.
 */

static char*	NULL_STRING = "NULL";

/*
struct WIN_hostent *
*/
SEGPTR WINSOCK_gethostbyaddr(const char *addr, INT16 len, INT16 type)
{
  LPWSINFO      	pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_GetHostByAddr(%08x): ptr %8x, len %d, type %d\n", 
			  (unsigned)pwsi, (unsigned) addr, len, type);
  if( pwsi )
  {
    struct hostent* 	host;
    if( (host = gethostbyaddr(addr, len, type)) != NULL )
      if( WS_dup_he(pwsi, host, WS_DUP_SEGPTR) )
          return SEGPTR_GET(pwsi->buffer);
      else pwsi->errno = WSAENOBUFS;
    else pwsi->errno = (h_errno < 0) ? wsaErrno() : wsaHerrno();
  } 
  return NULL;
}

/*
struct WIN_hostent *
*/
SEGPTR WINSOCK_gethostbyname(const char *name)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_GetHostByName(%08x): %s\n",
                          (unsigned)pwsi, (name)?name:"NULL");
  if( pwsi )
  {
    struct hostent*     host;
    if( (host = gethostbyname(name)) != NULL )
      if( WS_dup_he(pwsi, host, WS_DUP_SEGPTR) )
          return SEGPTR_GET(pwsi->buffer);
      else pwsi->errno = WSAENOBUFS;
    else pwsi->errno = (h_errno < 0) ? wsaErrno() : wsaHerrno();
  }
  return NULL;
}

INT16 WINSOCK_gethostname(char *name, INT16 namelen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_GetHostName(%08x): name %s, len %d\n", 
			  (unsigned)pwsi, (name)?name:NULL_STRING, namelen);
  if( pwsi )
  {
    if (gethostname(name, namelen) == 0) return 0;
    pwsi->errno = (errno == EINVAL) ? WSAEFAULT : wsaErrno();
  }
  return SOCKET_ERROR;
}

/*
struct WIN_protoent *
*/
SEGPTR WINSOCK_getprotobyname(char *name)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_GetProtoByName(%08x): %s\n",
                          (unsigned)pwsi, (name)?name:NULL_STRING);
  if( pwsi )
  {
    struct protoent*     proto;
    if( (proto = getprotobyname(name)) != NULL )
      if( WS_dup_pe(pwsi, proto, WS_DUP_SEGPTR) )
          return SEGPTR_GET(pwsi->buffer);
      else pwsi->errno = WSAENOBUFS;
    else pwsi->errno = (h_errno < 0) ? wsaErrno() : wsaHerrno();
  }
  return NULL;
}

/*
struct WIN_protoent *
*/
SEGPTR WINSOCK_getprotobynumber(INT16 number)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_GetProtoByNumber(%08x): %i\n", (unsigned)pwsi, number);

  if( pwsi )
  {
    struct protoent*     proto;
    if( (proto = getprotobynumber(number)) != NULL )
      if( WS_dup_pe(pwsi, proto, WS_DUP_SEGPTR) )
          return SEGPTR_GET(pwsi->buffer);
      else pwsi->errno = WSAENOBUFS;
    else pwsi->errno = WSANO_DATA;
  }
  return NULL;
}

/*
struct WIN_servent *
*/
SEGPTR WINSOCK_getservbyname(const char *name, const char *proto)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_GetServByName(%08x): '%s', '%s'\n", 
			  (unsigned)pwsi, (name)?name:NULL_STRING, (proto)?proto:NULL_STRING);

  if( pwsi )
  {
    struct servent*     serv;
    if( (serv = getservbyname(name, proto)) != NULL )
      if( WS_dup_se(pwsi, serv, WS_DUP_SEGPTR) )
          return SEGPTR_GET(pwsi->buffer);
      else pwsi->errno = WSAENOBUFS;
    else pwsi->errno = (h_errno < 0) ? wsaErrno() : wsaHerrno();
  }
  return NULL;
}

/*
struct WIN_servent *
*/
SEGPTR WINSOCK_getservbyport(INT16 port, const char *proto)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_GetServByPort(%08x): %i, '%s'\n",
                          (unsigned)pwsi, (int)port, (proto)?proto:NULL_STRING);
  if( pwsi )
  {
    struct servent*     serv;
    if( (serv = getservbyport(port, proto)) != NULL )
      if( WS_dup_se(pwsi, serv, WS_DUP_SEGPTR) )
          return SEGPTR_GET(pwsi->buffer);
      else pwsi->errno = WSAENOBUFS;
    else pwsi->errno = (h_errno < 0) ? wsaErrno() : wsaHerrno();
  }
  return NULL;
}


/* ----------------------------------- Windows sockets extensions -- *
 *								     *
 * ----------------------------------------------------------------- */

static int aop_control(ws_async_op* p_aop, int flag )
{
  unsigned	lLength;

  read(p_aop->fd[0], &lLength, sizeof(unsigned));
  if( LOWORD(lLength) )
    if( LOWORD(lLength) <= p_aop->buflen )
    {
      char* buffer = (char*)PTR_SEG_TO_LIN(p_aop->buffer_base);
      read(p_aop->fd[0], buffer, LOWORD(lLength));
      switch( p_aop->flags )
      {
	case WSMSG_ASYNC_HOSTBYNAME:
	case WSMSG_ASYNC_HOSTBYADDR: 
	     fixup_wshe((struct ws_hostent*)buffer, p_aop->buffer_base); break;
	case WSMSG_ASYNC_PROTOBYNAME:
	case WSMSG_ASYNC_PROTOBYNUM:
	     fixup_wspe((struct ws_protoent*)buffer, p_aop->buffer_base); break;
	case WSMSG_ASYNC_SERVBYNAME:
	case WSMSG_ASYNC_SERVBYPORT:
	     fixup_wsse((struct ws_servent*)buffer, p_aop->buffer_base); break;
	default:
	     if( p_aop->flags ) fprintf(stderr,"Received unknown async request!\n"); 
	     return AOP_CONTROL_REMOVE;
      }
    }
    else lLength =  ((UINT32)LOWORD(lLength)) | ((unsigned)WSAENOBUFS << 16);

#if 0
  printf("async op completed: hWnd [%04x], uMsg [%04x], aop [%04x], event [%08x]\n",
	 p_aop->hWnd, p_aop->uMsg, (HANDLE16)WS_PTR2HANDLE(p_aop), (LPARAM)lLength);
#endif

  PostMessage(p_aop->hWnd, p_aop->uMsg, (HANDLE16)WS_PTR2HANDLE(p_aop), (LPARAM)lLength);
  return AOP_CONTROL_REMOVE;
}


static HANDLE16 __WSAsyncDBQuery(LPWSINFO pwsi, HWND16 hWnd, UINT16 uMsg, LPCSTR init,
				 INT16 len, INT16 type, SEGPTR sbuf, INT16 buflen, UINT32 flag)
{
  /* queue 'flag' request and fork off its handler */

  async_ctl.ws_aop = (ws_async_op*)WS_ALLOC(sizeof(ws_async_op));

  if( async_ctl.ws_aop )
  {
      HANDLE16        handle = (HANDLE16)WS_PTR2HANDLE(async_ctl.ws_aop);

      if( pipe(async_ctl.ws_aop->fd) == 0 )
      {
	async_ctl.ws_aop->init = (char*)init;
	async_ctl.lLength = len;
	async_ctl.lEvent = type;

        async_ctl.ws_aop->hWnd = hWnd;
        async_ctl.ws_aop->uMsg = uMsg;

	async_ctl.ws_aop->buffer_base = sbuf; async_ctl.ws_aop->buflen = buflen;
	async_ctl.ws_aop->flags = flag;
	async_ctl.ws_aop->aop_control = &aop_control;
	WINSOCK_link_async_op( async_ctl.ws_aop );

        async_ctl.ws_aop->pid = fork();
        if( async_ctl.ws_aop->pid )
        {
            close(async_ctl.ws_aop->fd[1]);        /* write endpoint */

           /* Damn, BSD'ish SIGIO doesn't work on pipes/streams
            *
            * async_io(async_ctl.ws_aop->fd[0], 1);
            */

            dprintf_winsock(stddeb, "\tasync_op = %04x (child %i)\n",
                                    handle, async_ctl.ws_aop->pid);
            return handle;
        } else
                /* child process */
                 {
                   close(async_ctl.ws_aop->fd[0]); /* read endpoint */
		   switch(flag)
		   {
		     case WSMSG_ASYNC_HOSTBYADDR:
		     case WSMSG_ASYNC_HOSTBYNAME:
 			WS_do_async_gethost(pwsi,flag);
		     case WSMSG_ASYNC_PROTOBYNUM:
		     case WSMSG_ASYNC_PROTOBYNAME:
			WS_do_async_getproto(pwsi,flag);
		     case WSMSG_ASYNC_SERVBYPORT:
		     case WSMSG_ASYNC_SERVBYNAME:
			WS_do_async_getserv(pwsi,flag);
		   }
                 }
      }
      WS_FREE(async_ctl.ws_aop);
      pwsi->errno = wsaErrno();
  } else pwsi->errno = WSAEWOULDBLOCK;
  return 0;
}

HANDLE16 WSAAsyncGetHostByAddr(HWND16 hWnd, UINT16 uMsg, LPCSTR addr,
                               INT16 len, INT16 type, SEGPTR sbuf, INT16 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetHostByAddr(%08x): hwnd %04x, msg %04x, addr %08x[%i]\n",
                          (unsigned)pwsi, hWnd, uMsg, (unsigned)addr , len );

  if( pwsi ) 
    return __WSAsyncDBQuery(pwsi, hWnd, uMsg, addr, len,
			    type, sbuf, buflen, WSMSG_ASYNC_HOSTBYADDR );
  return 0;
}


HANDLE16 WSAAsyncGetHostByName(HWND16 hWnd, UINT16 uMsg, LPCSTR name, 
                               SEGPTR sbuf, INT16 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetHostByName(%08x): hwnd %04x, msg %04x, host %s, buffer %i\n",
                          (unsigned)pwsi, hWnd, uMsg, (name)?name:NULL_STRING, (int)buflen );

  if( pwsi )
    return __WSAsyncDBQuery(pwsi, hWnd, uMsg, name, 0,
                            0, sbuf, buflen, WSMSG_ASYNC_HOSTBYNAME );
  return 0;
}                     


HANDLE16 WSAAsyncGetProtoByName(HWND16 hWnd, UINT16 uMsg, LPCSTR name, 
                                SEGPTR sbuf, INT16 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetProtoByName(%08x): hwnd %04x, msg %04x, protocol %s\n",
                          (unsigned)pwsi, hWnd, uMsg, (name)?name:NULL_STRING );

  if( pwsi )
    return __WSAsyncDBQuery(pwsi, hWnd, uMsg, name, 0,
                            0, sbuf, buflen, WSMSG_ASYNC_PROTOBYNAME );
  return 0;
}


HANDLE16 WSAAsyncGetProtoByNumber(HWND16 hWnd, UINT16 uMsg, INT16 number, 
                                  SEGPTR sbuf, INT16 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetProtoByNumber(%08x): hwnd %04x, msg %04x, num %i\n",
                          (unsigned)pwsi, hWnd, uMsg, number );

  if( pwsi )
    return __WSAsyncDBQuery(pwsi, hWnd, uMsg, NULL, 0,
                            number, sbuf, buflen, WSMSG_ASYNC_PROTOBYNUM );
  return 0;
}


HANDLE16 WSAAsyncGetServByName(HWND16 hWnd, UINT16 uMsg, LPCSTR name, 
                               LPCSTR proto, SEGPTR sbuf, INT16 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetServByName(%08x): hwnd %04x, msg %04x, name %s, proto %s\n",
                   (unsigned)pwsi, hWnd, uMsg, (name)?name:NULL_STRING, (proto)?proto:NULL_STRING );

  if( pwsi )
  { 
    async_ctl.buffer = (char*)proto;
    return __WSAsyncDBQuery(pwsi, hWnd, uMsg, name, 0,
                            0, sbuf, buflen, WSMSG_ASYNC_SERVBYNAME );
  }
  return 0;
}


HANDLE16 WSAAsyncGetServByPort(HWND16 hWnd, UINT16 uMsg, INT16 port, 
			       LPCSTR proto, SEGPTR sbuf, INT16 buflen)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncGetServByPort(%08x): hwnd %04x, msg %04x, port %i, proto %s\n",
                           (unsigned)pwsi, hWnd, uMsg, port, (proto)?proto:NULL_STRING );

  if( pwsi )
    return __WSAsyncDBQuery(pwsi, hWnd, uMsg, proto, 0,
                            port, sbuf, buflen, WSMSG_ASYNC_SERVBYPORT );
  return 0;
}

INT16 WSACancelAsyncRequest(HANDLE16 hAsyncTaskHandle)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());
  ws_async_op*		p_aop = (ws_async_op*)WS_HANDLE2PTR(hAsyncTaskHandle);

  dprintf_winsock(stddeb, "WS_CancelAsyncRequest(%08x): handle %04x\n", 
			   (unsigned)pwsi, hAsyncTaskHandle);
  if( pwsi )
    if( WINSOCK_check_async_op(p_aop) )
    {
  	kill(p_aop->pid, SIGKILL); 
	waitpid(p_aop->pid, NULL, 0);
	close(p_aop->fd[0]);
	WINSOCK_unlink_async_op(p_aop);
	WS_FREE(p_aop);
	return 0;
    }
    else pwsi->errno = WSAEINVAL;
  return SOCKET_ERROR;
}

/* ----- asynchronous select() */

int delete_async_op(ws_socket* pws)
{
  if( pws->p_aop )
  {
    kill(pws->p_aop->pid, SIGKILL);
    waitpid(pws->p_aop->pid, NULL, 0);
    WS_FREE(pws->p_aop); return 1;
    pws->flags &= WS_FD_INTERNAL;
  }
  return 0;
}

void _sigusr1_handler_parent(int sig)
{
  /* child process puts MTYPE_CLIENT data packet into the
   * 'async_qid' message queue and signals us with SIGUSR1.
   * This handler reads the queue and posts 'uMsg' notification
   * message.
   */

  ipc_packet		ipack;

  signal( SIGUSR1, _sigusr1_handler_parent);
  while( msgrcv(async_qid, (struct msgbuf*)&ipack,
          MTYPE_CLIENT_SIZE, MTYPE_CLIENT, IPC_NOWAIT) != -1 )
  {
    if( ipack.wParam && abs((short)ipack.wParam) < 32768 )
    {
      ws_socket*        pws = (ws_socket*)WS_HANDLE2PTR(ipack.wParam);
      if( pws->p_aop && abs((char*)_ws_stub - (char*)pws->p_aop) < 32768 )
      {
	  pws->flags &= ~(ipack.lParam);
#if 0
          printf("async event - hWnd %04x, uMsg %04x [%08x]\n",
                  pws->p_aop->hWnd, pws->p_aop->uMsg, ipack.lParam );
#endif
	  PostMessage(pws->p_aop->hWnd, pws->p_aop->uMsg, 
		     (WPARAM16)ipack.wParam, (LPARAM)ipack.lParam );
      }
      else fprintf(stderr,"AsyncSelect:stray async_op in socket %04x!\n", ipack.wParam);
    }
    else fprintf(stderr,"AsyncSelect:stray socket at %04x!\n", ipack.wParam);
  }
}

int notify_client( ws_socket* pws, unsigned flag )
{
  if( pws->p_aop && ((pws->p_aop->flags & flag) ||
		     (flag == WS_FD_CONNECTED && pws->flags & WS_FD_CONNECT)) )
  {
     async_ctl.ip.mtype = MTYPE_PARENT;
     async_ctl.ip.lParam = flag;
     while( msgsnd(async_qid, (struct msgbuf*)&(async_ctl.ip),
                               MTYPE_PARENT_SIZE, 0) == -1 )
     { 
	if( errno == EINTR ) continue;
	else
	{
	    perror("AsyncSelect(parent)"); 
	    delete_async_op(pws);
	    pws->flags &= WS_FD_INTERNAL;
	    return 0;
	}
     }
     kill(pws->p_aop->pid, SIGUSR1);
     return 1;
  }
  return 0;
}

INT16 init_async_select(ws_socket* pws, HWND16 hWnd, UINT16 uMsg, UINT32 lEvent)
{
    ws_async_op*        p_aop;

    if( delete_async_op(pws) )  /* delete old async handler if any */
    {
      pws->p_aop = NULL;
      pws->flags &= WS_FD_INTERNAL;
    }
    if( lEvent == 0 ) return 0;

    /* setup async handler - some data may be redundant */

    WINSOCK_unblock_io(pws->fd, 1);
    if( (p_aop = (ws_async_op*)WS_ALLOC(sizeof(ws_async_op))) )
    {
      p_aop->hWnd = hWnd;
      p_aop->uMsg = uMsg;
      pws->p_aop = p_aop;

      async_ctl.lEvent = p_aop->flags = lEvent;
      async_ctl.ws_sock = pws;
      async_ctl.ip.wParam = (UINT16)WS_PTR2HANDLE(pws);
      async_ctl.ip.lParam = 0;

      p_aop->pid = fork();
      if( p_aop->pid != -1 )
        if( p_aop->pid == 0 ) WINSOCK_do_async_select(); /* child process */
	else pws->flags |= lEvent;

      signal( SIGUSR1, _sigusr1_handler_parent );
      return 0;                                  /* Wine process */
    }
    return SOCKET_ERROR;
}

INT16 WSAAsyncSelect(SOCKET16 s, HWND16 hWnd, UINT16 uMsg, UINT32 lEvent)
{
  ws_socket*    pws  = (ws_socket*)WS_HANDLE2PTR(s);
  LPWSINFO      pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_AsyncSelect(%08x): %04x, hWnd %04x, uMsg %04x, event %08x\n",
			  (unsigned)pwsi, s, hWnd, uMsg, (unsigned)lEvent );
  if( _check_ws(pwsi, pws) )
    if( init_async_select(pws, hWnd, uMsg, lEvent) == 0 ) return 0;
    else pwsi->errno = WSAENOBUFS;
  return SOCKET_ERROR; 
}

/* ----- miscellaneous */

INT16 __WSAFDIsSet(SOCKET16 fd, ws_fd_set *set)
{
  int i = set->fd_count;
  
  dprintf_winsock(stddeb, "__WSAFDIsSet(%d,%8lx)\n",fd,(unsigned long)set);
    
  while (i--)
      if (set->fd_array[i] == fd) return 1;
  return 0;
}                                                            

BOOL16 WSAIsBlocking(void)
{
  /* By default WinSock should set all its sockets to non-blocking mode
   * and poll in PeekMessage loop when processing "blocking" ones. This 
   * function * is supposed to tell if program is in this loop. Our 
   * blocking calls are truly blocking so we always return FALSE.
   *
   * Note: It is allowed to call this function without prior WSAStartup().
   */

  dprintf_winsock(stddeb, "WS_IsBlocking()\n");
  return FALSE;
}

INT16 WSACancelBlockingCall(void)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_CancelBlockingCall(%08x)\n", (unsigned)pwsi);

  if( pwsi ) return 0;
  return SOCKET_ERROR;
}

FARPROC16 WSASetBlockingHook(FARPROC16 lpBlockFunc)
{
  FARPROC16		prev;
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_SetBlockingHook(%08x): hook %08x\n", 
			  (unsigned)pwsi, (unsigned) lpBlockFunc);

  if( pwsi ) { 
      prev = pwsi->blocking_hook; 
      pwsi->blocking_hook = lpBlockFunc; 
      return prev; 
  }
  return 0;
}

INT16 WSAUnhookBlockingHook(void)
{
  LPWSINFO              pwsi = wsi_find(GetCurrentTask());

  dprintf_winsock(stddeb, "WS_UnhookBlockingHook(%08x)\n", (unsigned)pwsi);
  if( pwsi ) return (INT16)(INT32)(pwsi->blocking_hook = (FARPROC16)NULL);
  return SOCKET_ERROR;
}

VOID
WsControl(DWORD x1,DWORD x2,LPDWORD x3,LPDWORD x4,LPDWORD x5,LPDWORD x6) 
{
	fprintf(stdnimp,"WsControl(%lx,%lx,%p,%p,%p,%p)\n",
		x1,x2,x3,x4,x5,x6
	);
	fprintf(stdnimp,"WsControl(x,x,%lx,%lx,%lx,%lx)\n",
		x3?*x3:0,x4?*x4:0,x5?*x5:0,x6?*x6:0
	);
	return;
}
/* ----------------------------------- end of API stuff */



/* ----------------------------------- helper functions */

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
   /* Duplicate hostent structure and flatten data (with its pointers)
    * into pwsi->buffer. Internal pointers can be linear, SEGPTR, or 
    * relative to 0 depending on "flag" value. Return data size (also 
    * in the pwsi->buflen).
    */

   int size = hostent_size(p_he);
   if( size )
   {
     char*           p_name,*p_aliases,*p_addr,*p_base,*p;

     _check_buffer(pwsi, size);
     p = pwsi->buffer;
     p_base = (flag & WS_DUP_OFFSET) ? NULL
				     : ((flag & WS_DUP_SEGPTR) ? (char*)SEGPTR_GET(p) : p);
     p += (flag & WS_DUP_SEGPTR) ? sizeof(struct ws_hostent) : sizeof(struct hostent);
     p_name = p;
     strcpy(p, p_he->h_name); p += strlen(p) + 1;
     p_aliases = p;
     p += list_dup(p_he->h_aliases, p, p_base + (p - pwsi->buffer), 0);
     p_addr = p;
     list_dup(p_he->h_addr_list, p, p_base + (p - pwsi->buffer), p_he->h_length);
     if( !(flag & WS_DUP_SEGPTR) )
     { struct hostent* p_to = (struct hostent*)pwsi->buffer;
       p_to->h_addrtype = p_he->h_addrtype; p_to->h_length = p_he->h_length;
       p_to->h_name = p_base + (p_name - pwsi->buffer);
       p_to->h_aliases = (char**)(p_base + (p_aliases - pwsi->buffer));
       p_to->h_addr_list = (char**)(p_base + (p_addr - pwsi->buffer)); }
     else
     { struct ws_hostent* p_to = (struct ws_hostent*)pwsi->buffer;
       p_to->h_addrtype = (INT16)p_he->h_addrtype; 
       p_to->h_length = (INT16)p_he->h_length;
       p_to->h_name = (SEGPTR)(p_base + (p_name - pwsi->buffer));
       p_to->h_aliases = (SEGPTR)(p_base + (p_aliases - pwsi->buffer));
       p_to->h_addr_list = (SEGPTR)(p_base + (p_addr - pwsi->buffer));
       return (size + sizeof(struct ws_hostent) - sizeof(struct hostent)); }
   }
   return size;
}

void fixup_wshe(struct ws_hostent* p_wshe, SEGPTR base)
{
   /* add 'base' to ws_hostent pointers to convert them from offsets */ 

   int i;
   unsigned*	p_aliases,*p_addr;

   p_aliases = (unsigned*)((char*)p_wshe + (unsigned)p_wshe->h_aliases); 
   p_addr = (unsigned*)((char*)p_wshe + (unsigned)p_wshe->h_addr_list);
   ((unsigned)(p_wshe->h_name)) += (unsigned)base;
   ((unsigned)(p_wshe->h_aliases)) += (unsigned)base;
   ((unsigned)(p_wshe->h_addr_list)) += (unsigned)base;
   for(i=0;p_aliases[i];i++) p_aliases[i] += (unsigned)base;
   for(i=0;p_addr[i];i++) p_addr[i] += (unsigned)base;
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
     char*            p_name,*p_aliases,*p_base,*p;

     _check_buffer(pwsi, size);
     p = pwsi->buffer; 
     p_base = (flag & WS_DUP_OFFSET) ? NULL
				     : ((flag & WS_DUP_SEGPTR) ? (char*)SEGPTR_GET(p) : p);
     p += (flag & WS_DUP_SEGPTR)? sizeof(struct ws_protoent) : sizeof(struct protoent);
     p_name = p;
     strcpy(p, p_pe->p_name); p += strlen(p) + 1;
     p_aliases = p;
     list_dup(p_pe->p_aliases, p, p_base + (p - pwsi->buffer), 0);
     if( !(flag & WS_DUP_NATIVE) )
     { struct protoent* p_to = (struct protoent*)pwsi->buffer;
       p_to->p_proto = p_pe->p_proto;
       p_to->p_name = p_base + (p_name - pwsi->buffer); 
       p_to->p_aliases = (char**)(p_base + (p_aliases - pwsi->buffer)); }
     else
     { struct ws_protoent* p_to = (struct ws_protoent*)pwsi->buffer;
       p_to->p_proto = (INT16)p_pe->p_proto;
       p_to->p_name = (SEGPTR)(p_base) + (p_name - pwsi->buffer);
       p_to->p_aliases = (SEGPTR)((p_base) + (p_aliases - pwsi->buffer)); 
       return (size + sizeof(struct ws_protoent) - sizeof(struct protoent)); }
   }
   return size;
}

void fixup_wspe(struct ws_protoent* p_wspe, SEGPTR base)
{
   int i;
   unsigned*       p_aliases = (unsigned*)((char*)p_wspe + (unsigned)p_wspe->p_aliases); 
   ((unsigned)(p_wspe->p_name)) += (unsigned)base;
   ((unsigned)(p_wspe->p_aliases)) += (unsigned)base;
   for(i=0;p_aliases[i];i++) p_aliases[i] += (unsigned)base;
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
     char*           p_name,*p_aliases,*p_proto,*p_base,*p;

     _check_buffer(pwsi, size);
     p = pwsi->buffer;
     p_base = (flag & WS_DUP_OFFSET) ? NULL 
				     : ((flag & WS_DUP_SEGPTR) ? (char*)SEGPTR_GET(p) : p);
     p += (flag & WS_DUP_SEGPTR)? sizeof(struct ws_servent) : sizeof(struct servent);
     p_name = p;
     strcpy(p, p_se->s_name); p += strlen(p) + 1;
     p_proto = p;
     strcpy(p, p_se->s_proto); p += strlen(p) + 1;
     p_aliases = p;
     list_dup(p_se->s_aliases, p, p_base + (p - pwsi->buffer), 0);

     if( !(flag & WS_DUP_SEGPTR) )
     { struct servent* p_to = (struct servent*)pwsi->buffer;
       p_to->s_port = p_se->s_port;
       p_to->s_name = p_base + (p_name - pwsi->buffer); 
       p_to->s_proto = p_base + (p_proto - pwsi->buffer);
       p_to->s_aliases = (char**)(p_base + (p_aliases - pwsi->buffer)); }
     else
     { struct ws_servent* p_to = (struct ws_servent*)pwsi->buffer;
       p_to->s_port = (INT16)p_se->s_port;
       p_to->s_name = (SEGPTR)(p_base + (p_name - pwsi->buffer));
       p_to->s_proto = (SEGPTR)(p_base + (p_proto - pwsi->buffer));
       p_to->s_aliases = (SEGPTR)(p_base + (p_aliases - pwsi->buffer)); 
       return (size + sizeof(struct ws_servent) - sizeof(struct servent)); }
   }
   return size;
}

void fixup_wsse(struct ws_servent* p_wsse, SEGPTR base)
{
   int i;
   unsigned*       p_aliases = (unsigned*)((char*)p_wsse + (unsigned)p_wsse->s_aliases);
   ((unsigned)(p_wsse->s_name)) += (unsigned)base;
   ((p_wsse->s_proto)) += (unsigned)base;
   ((p_wsse->s_aliases)) += (unsigned)base;
   for(i=0;p_aliases[i];i++) p_aliases[i] += (unsigned)base;
}

/* ----------------------------------- error handling */

UINT16 wsaErrno(void)
{
    int	loc_errno = errno; 
#if defined(__FreeBSD__)
       dprintf_winsock(stderr, "winsock: errno %d, (%s).\n", 
                			 errno, sys_errlist[errno]);
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
	case ESTALE:		return WSAESTALE;
	case EREMOTE:		return WSAEREMOTE;

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
               	    h_errno, sys_errlist[h_errno]);
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


