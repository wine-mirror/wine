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

#include "config.h"

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
# include <sys/so_ioctl.h>
#endif
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#ifdef __svr4__
# include <sys/file.h>
#endif

extern int h_errno;

#include "winsock.h"
#include "windows.h"
#include "heap.h"
#include "ldt.h"
#include "message.h"
#include "selectors.h"
#include "miscemu.h"
#include "sig_context.h"
#include "debug.h"

#ifndef FASYNC
#define FASYNC FIOASYNC
#endif

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

static void fixup_wshe(struct ws_hostent* p_wshe, void* base);
static void fixup_wspe(struct ws_protoent* p_wspe, void* base);
static void fixup_wsse(struct ws_servent* p_wsse, void* base);

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

    int num = 0, num_dead = 0;
    ws_async_op*   p, *next;

    TRACE(winsock," cancelling async DNS requests... \n");

    SIGNAL_MaskAsyncEvents( TRUE );
    next = __async_op_list;
    while( (p = next) ) 
    {
	HTASK16 hWndTask = GetWindowTask16(p->hWnd);

	next = p->next;
	if(!hTask || !hWndTask || (hTask == hWndTask))
	{
	    num++;
	    if( p->flags & WSMSG_DEAD_AOP )
		num_dead++;

	    WINSOCK_cancel_async_op(p);
	    if( __opfree ) __opfree(p);
	}
    }
    SIGNAL_MaskAsyncEvents( FALSE );
    TRACE(winsock," -> %i total (%i active)\n", num, num - num_dead );
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

	      TRACE(winsock,"\treaping dead aop [%08x]\n", (unsigned)p );

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

HANDLER_DEF(WINSOCK_sigio)
{
 struct timeval         timeout;
 fd_set                 check_set;
 ws_async_op*		p_aop;

 HANDLER_INIT();

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
            char* buffer = (p_aop->flags & WSMSG_WIN32_AOP)
		  ? p_aop->b.lin_base : (char*)PTR_SEG_TO_LIN(p_aop->b.seg_base);

            read(p_aop->fd[0], buffer, LOWORD(lLength));
            switch( p_aop->flags & WSMSG_ASYNC_RQMASK )
            {
		case WSMSG_ASYNC_HOSTBYNAME:
		case WSMSG_ASYNC_HOSTBYADDR:
		     fixup_wshe((struct ws_hostent*)buffer, p_aop->b.ptr_base); break;
		case WSMSG_ASYNC_PROTOBYNAME:
		case WSMSG_ASYNC_PROTOBYNUM:
		     fixup_wspe((struct ws_protoent*)buffer, p_aop->b.ptr_base); break;
		case WSMSG_ASYNC_SERVBYNAME:
		case WSMSG_ASYNC_SERVBYPORT:
		     fixup_wsse((struct ws_servent*)buffer, p_aop->b.ptr_base); break;
		default:
                     if( p_aop->flags ) WARN(winsock,"Received unknown async request!\n");
                     return AOP_CONTROL_REMOVE;
            }
	}
        else lLength =  ((UINT32)LOWORD(lLength)) | ((unsigned)WSAENOBUFS << 16);
    } /* failure */

     /* was a __WS_ASYNC_DEBUG statement */
     TRACE(winsock, "DNS aop completed: hWnd [%04x], uMsg [%04x], "
 	  "aop [%04x], event [%08lx]\n",
 	  p_aop->hWnd, p_aop->uMsg, __ws_gethandle(p_aop), (LPARAM)lLength);

    /* FIXME: update num_async_rq */
    EVENT_DeleteIO( p_aop->fd[0], EVENT_IO_READ );
    PostMessage32A( p_aop->hWnd, p_aop->uMsg, __ws_gethandle(p_aop), (LPARAM)lLength );
    
    return AOP_CONTROL_REMOVE;  /* one-shot request */
}


HANDLE16 __WSAsyncDBQuery(LPWSINFO pwsi, HWND32 hWnd, UINT32 uMsg, INT32 type, LPSTR init,
		  	  INT32 len, LPSTR proto, void* sbuf, INT32 buflen, UINT32 flag)
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
	    async_ctl.ws_aop->b.ptr_base = sbuf; 
	    async_ctl.ws_aop->buflen = buflen;
	    async_ctl.ws_aop->flags = flag;
	    async_ctl.ws_aop->aop_control = &aop_control;

	    WINSOCK_link_async_op( async_ctl.ws_aop );

	    EVENT_AddIO( async_ctl.ws_aop->fd[0], EVENT_IO_READ );
	    pwsi->num_async_rq++;

	    async_ctl.ws_aop->pid = fork();
	    if( async_ctl.ws_aop->pid )
	    {
		TRACE(winsock, "\tasync_op = %04x (child %i)\n", 
				handle, async_ctl.ws_aop->pid);

		close(async_ctl.ws_aop->fd[1]);  /* write endpoint */
		if( async_ctl.ws_aop->pid > 0 )
		    return __ws_gethandle(async_ctl.ws_aop);

		/* fork() failed */

	        pwsi->num_async_rq--;
		EVENT_DeleteIO( async_ctl.ws_aop->fd[0], EVENT_IO_READ );
		close(async_ctl.ws_aop->fd[0]);
		pwsi->err = WSAEWOULDBLOCK;
	    }
	    else
	    {
	    	extern BOOL32 THREAD_InitDone;

	        THREAD_InitDone = FALSE;
		/* child process */

		close(async_ctl.ws_aop->fd[0]);  /* read endpoint */
		switch( flag & WSMSG_ASYNC_RQMASK )
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

    /* was a __WS_ASYNC_DEBUG statement */
    TRACE(winsock, "handler - notify aop [%d, buf %d]\n", 
	  async_ctl.ilength, async_ctl.ws_aop->buflen);
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

    /* was a __WS_ASYNC_DEBUG statement */
    TRACE(winsock, "handler - failed aop [%d, buf %d]\n", 
	  async_ctl.ilength, async_ctl.ws_aop->buflen);
}

void dump_ws_hostent_offset(struct ws_hostent* wshe)
{
  int		i;
  char*		base = (char*)wshe;
  unsigned*	ptr;

  DUMP("h_name = %08x\t[%s]\n", 
       (unsigned)wshe->h_name, base + (unsigned)wshe->h_name);
  DUMP("h_aliases = %08x\t[%08x]\n", 
       (unsigned)wshe->h_aliases, (unsigned)(base+(unsigned)wshe->h_aliases));
  ptr = (unsigned*)(base + (unsigned)wshe->h_aliases);
  for(i = 0; ptr[i]; i++ )
  {
	DUMP("%i - %08x [%s]\n", i + 1, ptr[i], ((char*)base) + ptr[i]);
  }
  DUMP("h_length = %i\n", wshe->h_length);
}

void WS_do_async_gethost(LPWSINFO pwsi, unsigned flag )
{  
  int			size = 0;
  struct hostent* 	p_he;

  close(async_ctl.ws_aop->fd[0]);

  p_he = (flag & WSMSG_ASYNC_HOSTBYNAME)
	 ? gethostbyname(async_ctl.rq.name)
	 : gethostbyaddr(async_ctl.rq.name,
		 	 async_ctl.ilength, async_ctl.type);

  TRACE(winsock,"DNS: got hostent for [%s]\n", async_ctl.rq.name );

  if( p_he ) /* convert to the Winsock format with internal pointers as offsets */
      size = WS_dup_he(pwsi, p_he, WS_DUP_OFFSET | 
		     ((flag & WSMSG_WIN32_AOP) ? WS_DUP_LINEAR : WS_DUP_SEGPTR) );
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

  TRACE(winsock,"DNS: got protoent for [%s]\n", async_ctl.rq.name );

  if( p_pe ) /* convert to the Winsock format with internal pointers as offsets */
      size = WS_dup_pe(pwsi, p_pe, WS_DUP_OFFSET |
		     ((flag & WSMSG_WIN32_AOP) ? WS_DUP_LINEAR : WS_DUP_SEGPTR) );
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

  if( p_se ) /* convert to the Winsock format with internal pointers as offsets */
      size = WS_dup_se(pwsi, p_se, WS_DUP_OFFSET |
		      ((flag & WSMSG_WIN32_AOP) ? WS_DUP_LINEAR : WS_DUP_SEGPTR) );
  if( size )
  {
      async_ctl.buffer = pwsi->buffer;
      async_ctl.ilength = (unsigned)WSAMAKEASYNCREPLY( (UINT16)size, 0 );
     _async_notify( flag );
  }
  else _async_fail();
}

/* ----------------------------------- helper functions -
 *
 * Raw results from the pipe contain internal pointers stored as
 * offsets relative to the beginning of the buffer and we need
 * to apply a fixup before passing them to applications.
 *
 * NOTE: It is possible to exploit the fact that fork() doesn't 
 * change the buffer address by storing fixed up pointers right 
 * in the handler. However, this will get in the way if we ever 
 * get around to implementing DNS helper daemon a-la Netscape 4.x.
 */

void fixup_wshe(struct ws_hostent* p_wshe, void* base)
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

void fixup_wspe(struct ws_protoent* p_wspe, void* base)
{
   int i;
   unsigned*       p_aliases = (unsigned*)((char*)p_wspe + (unsigned)p_wspe->p_aliases);
   ((unsigned)(p_wspe->p_name)) += (unsigned)base;
   ((unsigned)(p_wspe->p_aliases)) += (unsigned)base;
   for(i=0;p_aliases[i];i++) p_aliases[i] += (unsigned)base;
}

void fixup_wsse(struct ws_servent* p_wsse, void* base)
{
   int i;
   unsigned*       p_aliases = (unsigned*)((char*)p_wsse + (unsigned)p_wsse->s_aliases);
   ((unsigned)(p_wsse->s_name)) += (unsigned)base;
   ((p_wsse->s_proto)) += (unsigned)base;
   ((p_wsse->s_aliases)) += (unsigned)base;
   for(i=0;p_aliases[i];i++) p_aliases[i] += (unsigned)base;
}

