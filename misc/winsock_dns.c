/*
 * asynchronous DNS services
 * 
 * (C) 1996,1997 Alex Korobka.
 *
 * TODO: Fork dns lookup helper during the startup (with a pipe
 *       for communication) and make it fork for a database request
 *       instead of forking the main process (i.e. something like
 *       Netscape 4.0).
 */

#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <errno.h>
#ifdef __EMX__
#include <sys/so_ioctl.h>
#include <sys/param.h>
#endif
#ifdef __svr4__
#include <sys/file.h>
#include <sys/filio.h>
#endif

extern int h_errno;

#include "windows.h"
#include "heap.h"
#include "ldt.h"
#include "message.h"
#include "miscemu.h"
#include "winsock.h"
#include "debug.h"

#ifndef FASYNC
#define FASYNC FIOASYNC
#endif

#define __WS_ASYNC_DEBUG	0

typedef struct          /* async DNS op control struct */
{
  ws_async_op*  ws_aop;
  char*         buffer;
  int           type;
  union
  {
    char*       init;
    char*       name;
    char*       addr;
  } rq;
  unsigned      ilength;
} ws_async_ctl;

extern HANDLE16	__ws_gethandle( void* ptr );
extern void*	__ws_memalloc( int size );
extern void	__ws_memfree( void* ptr );

/* NOTE: ws_async_op list is traversed inside the SIGIO handler! */

static int		__async_io_max_fd = 0;
static fd_set		__async_io_fdset;
static ws_async_op*	__async_op_list = NULL;

static void fixup_wshe(struct ws_hostent* p_wshe, SEGPTR base);
static void fixup_wspe(struct ws_protoent* p_wspe, SEGPTR base);
static void fixup_wsse(struct ws_servent* p_wsse, SEGPTR base);

extern void EVENT_AddIO( int fd, unsigned flag );
extern void EVENT_DeleteIO( int fd, unsigned flag );

/* ----------------------------------- async/non-blocking I/O */

int WINSOCK_async_io(int fd, int async)
{
    int fd_flags;

#ifndef __EMX__
    fcntl(fd, F_SETOWN, getpid());
#endif

    fd_flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, (async)? fd_flags | FASYNC
                                  : fd_flags & ~FASYNC ) != -1) return 0;
    return -1;
}

int WINSOCK_unblock_io(int fd, int noblock)
{
    int fd_flags;

    fd_flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, (noblock)? fd_flags |  O_NONBLOCK
                                    : fd_flags & ~O_NONBLOCK ) != -1) return 0;
    return -1;
}

int WINSOCK_check_async_op(ws_async_op* p_aop)
{
  ws_async_op*   p = __async_op_list;
  while( p ) if( p == p_aop ) return 1;
	     else p = p->next;
  return 0;
}

int WINSOCK_cancel_async_op(ws_async_op* p_aop)
{
    /* SIGIO unsafe! */

    if( WINSOCK_check_async_op(p_aop) )
    {
	if( !(p_aop->flags & WSMSG_DEAD_AOP) )
	{
            kill(p_aop->pid, SIGKILL);
            waitpid(p_aop->pid, NULL, 0); /* just in case */
            close(p_aop->fd[0]);
	}
        WINSOCK_unlink_async_op(p_aop);
	EVENT_DeleteIO( p_aop->fd[0], EVENT_IO_READ );
        p_aop->flags = 0;
	p_aop->hWnd = p_aop->uMsg = 0;
        return 1;
    }
    return 0;
}

void WINSOCK_cancel_task_aops(HTASK16 hTask, void (*__opfree)(void*))
{
    /* SIGIO safe, hTask == 0 cancels all outstanding async ops */

    int num = 0;
    ws_async_op*   p, *next;

    dprintf_winsock(stddeb,"\tcancelling async DNS requests... ");

    SIGNAL_MaskAsyncEvents( TRUE );
    next = __async_op_list;
    while( (p = next) ) 
    {
	HTASK16 hWndTask = GetWindowTask16(p->hWnd);

	next = p->next;
	if(!hTask || !hWndTask || (hTask == hWndTask))
	{
	    WINSOCK_cancel_async_op(p);
	    if( __opfree ) __opfree(p);
	    num++;
	}
    }
    SIGNAL_MaskAsyncEvents( FALSE );
    dprintf_winsock(stddeb,"%i total\n", num );
}

void WINSOCK_link_async_op(ws_async_op* p_aop)
{
  /* SIGIO safe */

  p_aop->prev = NULL;
  SIGNAL_MaskAsyncEvents( TRUE );
  if( __async_op_list )
  {
      ws_async_op*	p = __async_op_list;
      __async_op_list->prev = p_aop;

      /* traverse the list and retire dead ops created
       * by the signal handler (see below). */

      while( p )
      {
	  if( p->flags & WSMSG_DEAD_AOP )
	  {
	      ws_async_op* dead = p;

	      dprintf_winsock(stddeb,"\treaping dead aop [%08x]\n", (unsigned)p );

	      p = p->next;
	      WINSOCK_unlink_async_op( dead );
	      __ws_memfree( dead );
	      continue;
	  }
	  p = p->next;
      }
  }
  else FD_ZERO(&__async_io_fdset);
  p_aop->next = __async_op_list;
  __async_op_list = p_aop;
  SIGNAL_MaskAsyncEvents( FALSE );

  FD_SET(p_aop->fd[0], &__async_io_fdset);
  if( p_aop->fd[0] > __async_io_max_fd ) 
		     __async_io_max_fd = p_aop->fd[0];
}

void WINSOCK_unlink_async_op(ws_async_op* p_aop)
{
  /* SIGIO unsafe! */

  if( p_aop == __async_op_list ) __async_op_list = p_aop->next;
  else
      p_aop->prev->next = p_aop->next;
  if( p_aop->next ) p_aop->next->prev = p_aop->prev;

  FD_CLR(p_aop->fd[0], &__async_io_fdset); 
  if( p_aop->fd[0] == __async_io_max_fd )
		      __async_io_max_fd--;
}

/* ----------------------------------- SIGIO handler -
 *
 * link_async_op/unlink_async_op allow to install generic
 * async IO handlers (provided that aop_control function is defined).
 *
 * Note: pipe-based handlers must raise explicit SIGIO with kill(2).
 */

void WINSOCK_sigio(int signal)
{
 struct timeval         timeout;
 fd_set                 check_set;
 ws_async_op*		p_aop;

 check_set = __async_io_fdset;
 memset(&timeout, 0, sizeof(timeout));

 while( select(__async_io_max_fd + 1,
              &check_set, NULL, NULL, &timeout) > 0)
 {
   for( p_aop = __async_op_list;
	p_aop ; p_aop = p_aop->next )
      if( FD_ISSET(p_aop->fd[0], &check_set) )
          if( p_aop->aop_control(p_aop, AOP_IO) == AOP_CONTROL_REMOVE )
	  {
	     /* NOTE: memory management is signal-unsafe, therefore
	      * we can only set a flag to remove this p_aop later on.
	      */

	      p_aop->flags = WSMSG_DEAD_AOP;
	      close(p_aop->fd[0]);
	      FD_CLR(p_aop->fd[0],&__async_io_fdset);
	      if( p_aop->fd[0] == __async_io_max_fd )
				 __async_io_max_fd = p_aop->fd[0];
	      if( p_aop->pid ) 
	      { 
		  kill(p_aop->pid, SIGKILL); 
		  waitpid(p_aop->pid, NULL, WNOHANG);
		  p_aop->pid = 0;
	      }
	  }
   check_set = __async_io_fdset;
  }
}

/* ----------------------------------- getXbyY requests */

static	 ws_async_ctl	async_ctl; /* child process control struct */

static int aop_control(ws_async_op* p_aop, int flag )
{
    unsigned    lLength;

  /* success:   LOWORD(lLength) has the length of the struct
   *            to read.
   * failure:   LOWORD(lLength) is zero, HIWORD(lLength) contains
   *            the error code.
   */

    read(p_aop->fd[0], &lLength, sizeof(unsigned));
    if( LOWORD(lLength) )
    {
	if( (int)LOWORD(lLength) <= p_aop->buflen )
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
    } /* failure */

#if __WS_ASYNC_DEBUG
    printf("DNS aop completed: hWnd [%04x], uMsg [%04x], aop [%04x], event [%08x]\n",
         p_aop->hWnd, p_aop->uMsg, __ws_gethandle(p_aop), (LPARAM)lLength);
#endif

    /* FIXME: update num_async_rq */
    EVENT_DeleteIO( p_aop->fd[0], EVENT_IO_READ );
    PostMessage16( p_aop->hWnd, p_aop->uMsg, __ws_gethandle(p_aop), (LPARAM)lLength );
    
    return AOP_CONTROL_REMOVE;  /* one-shot request */
}


HANDLE16 __WSAsyncDBQuery(LPWSINFO pwsi, HWND16 hWnd, UINT16 uMsg, INT16 type, LPSTR init,
		  	  INT16 len, LPSTR proto, SEGPTR sbuf, INT16 buflen, UINT32 flag)
{
    /* queue 'flag' request and fork off its handler */

    async_ctl.ws_aop = (ws_async_op*)__ws_memalloc(sizeof(ws_async_op));

    if( async_ctl.ws_aop )
    {
	HANDLE16        handle = __ws_gethandle(async_ctl.ws_aop);

	if( pipe(async_ctl.ws_aop->fd) == 0 )
	{
	    async_ctl.rq.init = (char*)init;
	    async_ctl.ilength = len;
	    async_ctl.buffer = proto;
	    async_ctl.type = type;

	    async_ctl.ws_aop->hWnd = hWnd;
	    async_ctl.ws_aop->uMsg = uMsg;
	    async_ctl.ws_aop->buffer_base = sbuf; async_ctl.ws_aop->buflen = buflen;
	    async_ctl.ws_aop->flags = flag;
	    async_ctl.ws_aop->aop_control = &aop_control;

	    WINSOCK_link_async_op( async_ctl.ws_aop );

	    async_ctl.ws_aop->pid = fork();
	    if( async_ctl.ws_aop->pid )
	    {
		close(async_ctl.ws_aop->fd[1]);  /* write endpoint */
		dprintf_winsock(stddeb, "\tasync_op = %04x (child %i)\n",
					handle, async_ctl.ws_aop->pid);
		if( async_ctl.ws_aop->pid > 0 )
		{
		    EVENT_AddIO( async_ctl.ws_aop->fd[0], EVENT_IO_READ );
		    pwsi->num_async_rq++;
		    return __ws_gethandle(async_ctl.ws_aop);
		}

		/* fork() failed */
		close(async_ctl.ws_aop->fd[0]);
		pwsi->err = WSAEWOULDBLOCK;
	    }
	    else
	    {
		/* child process */

		close(async_ctl.ws_aop->fd[0]);  /* read endpoint */
		switch( flag )
		{
		    case WSMSG_ASYNC_HOSTBYADDR:
		    case WSMSG_ASYNC_HOSTBYNAME:
			WS_do_async_gethost(pwsi, flag);
			break;
		    case WSMSG_ASYNC_PROTOBYNUM:
		    case WSMSG_ASYNC_PROTOBYNAME:
			WS_do_async_getproto(pwsi, flag);
			break;
		    case WSMSG_ASYNC_SERVBYPORT:
		    case WSMSG_ASYNC_SERVBYNAME:
			WS_do_async_getserv(pwsi, flag);
			break;
		}
		_exit(0); 	/* skip  atexit()'ed cleanup */
	    }
	}
	else pwsi->err = wsaErrno(); /* failed to create pipe */

	__ws_memfree((void*)async_ctl.ws_aop);
    } else pwsi->err = WSAEWOULDBLOCK;
    return 0;
}

static int _async_notify()
{
    /* use half-duplex pipe to send variable length packets 
     * to the parent process */
     
    write(async_ctl.ws_aop->fd[1], &async_ctl.ilength, sizeof(unsigned));
    write(async_ctl.ws_aop->fd[1], async_ctl.buffer, async_ctl.ilength );

#ifndef __EMX__
    kill(getppid(), SIGIO);    /* simulate async I/O */
#endif

#if __WS_ASYNC_DEBUG
    printf("handler - notify aop [%d, buf %d]\n", async_ctl.ilength, async_ctl.ws_aop->buflen);
#endif
    return 1;
}

static void _async_fail()
{
     /* write a DWORD with error code (low word is zero) */

     async_ctl.ilength =
        (h_errno < 0) ? (unsigned)WSAMAKEASYNCREPLY( 0, wsaErrno() )
                      : (unsigned)WSAMAKEASYNCREPLY( 0, wsaHerrno() );
     write(async_ctl.ws_aop->fd[1], &async_ctl.ilength, sizeof(unsigned) );
#ifndef __EMX__
     kill(getppid(), SIGIO);    /* simulate async I/O */
#endif

#if __WS_ASYNC_DEBUG
    printf("handler - failed aop [%d, buf %d]\n", async_ctl.ilength, async_ctl.ws_aop->buflen);
#endif
}

void dump_ws_hostent_offset(struct ws_hostent* wshe)
{
  int		i;
  char*		base = (char*)wshe;
  unsigned*	ptr;

  printf("h_name = %08x\t[%s]\n", (unsigned)wshe->h_name, base + (unsigned)wshe->h_name);
  printf("h_aliases = %08x\t[%08x]\n", (unsigned)wshe->h_aliases, 
				       (unsigned)(base + (unsigned)wshe->h_aliases));
  ptr = (unsigned*)(base + (unsigned)wshe->h_aliases);
  for(i = 0; ptr[i]; i++ )
  {
	printf("%i - %08x ", i + 1, ptr[i]);
	printf(" [%s]\n", ((char*)base) + ptr[i]);
  }
  printf("h_length = %i\n", wshe->h_length);
}

void WS_do_async_gethost(LPWSINFO pwsi, unsigned flag )
{  
  int			size = 0;
  struct hostent* 	p_he;

  close(async_ctl.ws_aop->fd[0]);

  dprintf_winsock(stddeb,"DNS: getting hostent for [%s]\n", async_ctl.rq.name );

  p_he = (flag & WSMSG_ASYNC_HOSTBYNAME)
	 ? gethostbyname(async_ctl.rq.name)
	 : gethostbyaddr(async_ctl.rq.name,
		 	 async_ctl.ilength, async_ctl.type);
  dprintf_winsock(stddeb,"DNS: done!\n");

  if( p_he ) size = WS_dup_he(pwsi, p_he, WS_DUP_SEGPTR | WS_DUP_OFFSET );
  if( size )
  {
     async_ctl.buffer = pwsi->buffer;
     async_ctl.ilength = (unsigned)WSAMAKEASYNCREPLY( (UINT16)size, 0 );
     _async_notify( flag );
  }
  else _async_fail();
}

void WS_do_async_getproto(LPWSINFO pwsi, unsigned flag )
{
  int			size = 0;
  struct protoent*	p_pe;

  close(async_ctl.ws_aop->fd[0]);
  p_pe = (flag & WSMSG_ASYNC_PROTOBYNAME)
	 ? getprotobyname(async_ctl.rq.name)
	 : getprotobynumber(async_ctl.type);
  if( p_pe ) size = WS_dup_pe(pwsi, p_pe, WS_DUP_SEGPTR | WS_DUP_OFFSET );
  if( size )
  {
     async_ctl.buffer = pwsi->buffer;
     async_ctl.ilength = (unsigned)WSAMAKEASYNCREPLY( (UINT16)size, 0 );
     _async_notify( flag );
  } 
  else _async_fail();
}

void WS_do_async_getserv(LPWSINFO pwsi, unsigned flag )
{
  int			size = 0;
  struct servent* 	p_se;

  close(async_ctl.ws_aop->fd[0]);
  p_se = (flag & WSMSG_ASYNC_SERVBYNAME)
	 ? getservbyname(async_ctl.rq.name, async_ctl.buffer)
	 : getservbyport(async_ctl.type, async_ctl.buffer);
  if( p_se ) size = WS_dup_se(pwsi, p_se, WS_DUP_SEGPTR | WS_DUP_OFFSET );
  if( size )
  {
     async_ctl.buffer = pwsi->buffer;
     async_ctl.ilength = (unsigned)WSAMAKEASYNCREPLY( (UINT16)size, 0 );
     _async_notify( flag );
  }
  else _async_fail();
}

/* ----------------------------------- helper functions */

void fixup_wshe(struct ws_hostent* p_wshe, SEGPTR base)
{
   /* add 'base' to ws_hostent pointers to convert them from offsets */

   int i;
   unsigned*    p_aliases,*p_addr;

   p_aliases = (unsigned*)((char*)p_wshe + (unsigned)p_wshe->h_aliases);
   p_addr = (unsigned*)((char*)p_wshe + (unsigned)p_wshe->h_addr_list);
   ((unsigned)(p_wshe->h_name)) += (unsigned)base;
   ((unsigned)(p_wshe->h_aliases)) += (unsigned)base;
   ((unsigned)(p_wshe->h_addr_list)) += (unsigned)base;
   for(i=0;p_aliases[i];i++) p_aliases[i] += (unsigned)base;
   for(i=0;p_addr[i];i++) p_addr[i] += (unsigned)base;
}

void fixup_wspe(struct ws_protoent* p_wspe, SEGPTR base)
{
   int i;
   unsigned*       p_aliases = (unsigned*)((char*)p_wspe + (unsigned)p_wspe->p_aliases);
   ((unsigned)(p_wspe->p_name)) += (unsigned)base;
   ((unsigned)(p_wspe->p_aliases)) += (unsigned)base;
   for(i=0;p_aliases[i];i++) p_aliases[i] += (unsigned)base;
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

