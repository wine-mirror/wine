/*
 * asynchronous winsock services
 * 
 * (C) 1996 Alex Korobka.
 *
 * FIXME: telftp16 (ftp part) stalls on AsyncSelect with FD_ACCEPT.
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
#ifdef __svr4__
#include <sys/file.h>
#include <sys/filio.h>
#endif

extern int h_errno;

#include "windows.h"
#include "winsock.h"
#include "debug.h"

#define __WS_ASYNC_DEBUG	0

static int		__async_io_max_fd = 0;
static fd_set		__async_io_fdset;
static ws_async_op*	__async_op_list = NULL;

extern ws_async_ctl	async_ctl;
extern int		async_qid;

fd_set		 fd_read, fd_write, fd_excp;

/* ----------------------------------- async/non-blocking I/O */

int WINSOCK_async_io(int fd, int async)
{
    int fd_flags;

    fcntl(fd, F_SETOWN, getpid());

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

void WINSOCK_cancel_async_op(HTASK16 hTask)
{
  ws_async_op*   p = __async_op_list;
  while( p ) 
     if(hTask == GetWindowTask16(p->hWnd)) 
        p->flags = 0;
}

void WINSOCK_link_async_op(ws_async_op* p_aop)
{
  if( __async_op_list ) __async_op_list->prev = p_aop;
  else FD_ZERO(&__async_io_fdset);

  p_aop->next = __async_op_list; 
  p_aop->prev = NULL;
  __async_op_list = p_aop;

  FD_SET(p_aop->fd[0], &__async_io_fdset);
  if( p_aop->fd[0] > __async_io_max_fd ) 
		     __async_io_max_fd = p_aop->fd[0];
}

void WINSOCK_unlink_async_op(ws_async_op* p_aop)
{
  if( p_aop == __async_op_list ) __async_op_list = p_aop->next;
  else
  { p_aop->prev->next = p_aop->next;
    if( p_aop->next ) p_aop->next->prev = p_aop->prev; }
  FD_CLR(p_aop->fd[0], &__async_io_fdset); 
  if( p_aop->fd[0] == __async_io_max_fd )
		      __async_io_max_fd--;
}

/* ----------------------------------- SIGIO handler -
 *
 * link_async_op/unlink_async_op allow to install generic
 * async IO handlers (provided that aop_control function is defined).
 *
 * Note: AsyncGetXbyY expilicitly raise it.
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
	      if( p_aop->pid ) 
	      { 
		kill(p_aop->pid, SIGKILL);
		waitpid(p_aop->pid, NULL, WNOHANG);
	      }
	      WINSOCK_unlink_async_op( p_aop );
	  }
   check_set = __async_io_fdset;
  }
}

/* ----------------------------------- child process IPC */

static void _sigusr1_handler_child(int sig)
{
   /* read message queue to decide which
    * async_ctl parameters to update 
    *
    * Note: we don't want to have SA_RESTART on this signal
    * handler, otherwise select() won't notice changed fd sets.
    */

   signal( SIGUSR1, _sigusr1_handler_child );
   while( msgrcv(async_qid, (struct msgbuf*)&async_ctl.ip,
          MTYPE_PARENT_SIZE, MTYPE_PARENT, IPC_NOWAIT) != -1 )
   {
       /* only ip.lParam is updated */
#if __WS_ASYNC_DEBUG
       printf("handler - event %08x\n", async_ctl.ip.lParam );
#endif

       switch( async_ctl.ip.lParam )
       {
	  /* These are events we are notified of.
	   */

          case   WS_FD_CONNECTED: async_ctl.lEvent &= ~WS_FD_CONNECT;
				  FD_SET(async_ctl.ws_sock->fd, &fd_read);
				  FD_SET(async_ctl.ws_sock->fd, &fd_write);
			  	  break;

          case   WS_FD_ACCEPT:  async_ctl.ws_sock->flags |= WS_FD_ACCEPT;
				FD_SET(async_ctl.ws_sock->fd, &fd_read);
                                FD_SET(async_ctl.ws_sock->fd, &fd_write);
                                break;
          case   WS_FD_OOB:     async_ctl.lEvent |= WS_FD_OOB;
				FD_SET(async_ctl.ws_sock->fd, &fd_excp);
                                break;
          case   WS_FD_READ:    async_ctl.lEvent |= WS_FD_READ;
				FD_SET(async_ctl.ws_sock->fd, &fd_read);
                                break;
          case   WS_FD_WRITE:   async_ctl.lEvent |= WS_FD_WRITE;
				FD_SET(async_ctl.ws_sock->fd, &fd_write);
                                break;
          default:
       }
   }
}

static int notify_parent( unsigned flag )
{
  if( flag & WSMSG_ASYNC_SELECT )
  {
     async_ctl.ip.mtype = MTYPE_CLIENT;
     while( msgsnd(async_qid, (struct msgbuf*)&(async_ctl.ip),
                               MTYPE_CLIENT_SIZE, 0) == -1 )
     {
       if( errno == EINTR ) continue;
#ifdef EIDRM
       else if( errno == EIDRM ) _exit(0);
#endif
       else 
       { 
	 perror("AsyncSelect(child)"); 
	 return 0; 
       }
     }
     kill(getppid(), SIGUSR1); 

#if __WS_ASYNC_DEBUG
  printf("handler - notify [%08x]\n", async_ctl.ip.lParam);
#endif
  }
  else /* use half-duplex pipe to handle variable length packets */
  {
     write(async_ctl.ws_aop->fd[1], &async_ctl.lLength, sizeof(unsigned) );
     write(async_ctl.ws_aop->fd[1], async_ctl.buffer, async_ctl.lLength );
     kill(getppid(), SIGIO);    /* simulate async I/O */
#if __WS_ASYNC_DEBUG
  printf("handler - notify aop [%d, buf %d]\n", async_ctl.lLength, async_ctl.ws_aop->buflen);
#endif
     pause();
  }
  return 1;
}

/* ----------------------------------- async select */

static void setup_fd_sets()
{
   FD_ZERO(&fd_read); FD_ZERO(&fd_write); FD_ZERO(&fd_excp);

   if( async_ctl.lEvent & WS_FD_OOB) 
       FD_SET(async_ctl.ws_sock->fd, &fd_excp);
   if( async_ctl.lEvent & (WS_FD_ACCEPT | WS_FD_READ |
			   WS_FD_CONNECT | WS_FD_CLOSE) ) 
       FD_SET(async_ctl.ws_sock->fd, &fd_read);
   if( async_ctl.lEvent & (WS_FD_WRITE | WS_FD_CONNECT) ) 
       FD_SET(async_ctl.ws_sock->fd, &fd_write);
}

static void setup_sig_sets(sigset_t* sig_block)
{
   sigemptyset(sig_block);
   sigaddset(sig_block, SIGUSR1);
   sigprocmask( SIG_BLOCK, sig_block, NULL);
   signal( SIGUSR1, _sigusr1_handler_child );
}

void WINSOCK_do_async_select()
{
  sigset_t    sig_block;
  int	      sock_type, bytes;

  getsockopt(async_ctl.ws_sock->fd, SOL_SOCKET, SO_TYPE, &sock_type, &bytes);
  setup_sig_sets(&sig_block);
  setup_fd_sets();

  while(1)
  {
    int		val;

    if( sock_type != SOCK_STREAM )
        async_ctl.lEvent &= ~(WS_FD_ACCEPT | WS_FD_CONNECT);
    sigprocmask( SIG_UNBLOCK, &sig_block, NULL); 

#if __WS_ASYNC_DEBUG
    printf("select(2)[%i,%i,%i]... ", 
	     FD_ISSET(async_ctl.ws_sock->fd, &fd_read),
	     FD_ISSET(async_ctl.ws_sock->fd, &fd_write),
	     FD_ISSET(async_ctl.ws_sock->fd, &fd_excp));
#endif
    if( (val = select(async_ctl.ws_sock->fd + 1, 
		     &fd_read, &fd_write, &fd_excp, NULL)) == -1 )
      if( errno == EINTR ) continue;
#if __WS_ASYNC_DEBUG
    printf("got %i events\n", val);
#endif

#if __WS_ASYNC_DEBUG
    if( FD_ISSET(async_ctl.ws_sock->fd, &fd_read) )
	printf("handler - read is READY! [%08x]\n", async_ctl.lEvent & (WS_FD_READ | WS_FD_CLOSE));
#endif

    sigprocmask( SIG_BLOCK, &sig_block, NULL);
    async_ctl.ip.lParam = 0;
    if( async_ctl.ws_sock->flags & WS_FD_ACCEPT )
    {
	/* listening socket */
	
	FD_CLR(async_ctl.ws_sock->fd, &fd_read);
	FD_CLR(async_ctl.ws_sock->fd, &fd_write);

	async_ctl.ip.lParam = WSAMAKESELECTREPLY( WS_FD_ACCEPT, 0 );
        notify_parent( WSMSG_ASYNC_SELECT );
	continue;
    }
    else /* I/O socket */
    {
	if( async_ctl.lEvent & WS_FD_CONNECT )
	{
	  if( FD_ISSET(async_ctl.ws_sock->fd, &fd_write) ) 
	  {
	      /* success - reinit fd sets to start I/O */

	      if( async_ctl.lEvent & (WS_FD_READ | WS_FD_CLOSE))
		   FD_SET(async_ctl.ws_sock->fd, &fd_read);
	      else FD_CLR(async_ctl.ws_sock->fd, &fd_read);
	      if( async_ctl.lEvent & WS_FD_WRITE )
		   FD_SET(async_ctl.ws_sock->fd, &fd_write);
	      else FD_CLR(async_ctl.ws_sock->fd, &fd_write);

	      async_ctl.ip.lParam = WSAMAKESELECTREPLY( WS_FD_CONNECT, 0 );
	      async_ctl.lEvent &= ~WS_FD_CONNECT; /* one-shot */
	  }
	  else if( FD_ISSET(async_ctl.ws_sock->fd, &fd_read) )
	  {
              /* failure - do read() to get correct errno */

              if( read(async_ctl.ws_sock->fd, &bytes, 4) == -1 )
                  async_ctl.ip.lParam = WSAMAKESELECTREPLY( WS_FD_CONNECT, wsaErrno() );
              else continue;
	  } else continue; /* OOB?? */

          notify_parent( WSMSG_ASYNC_SELECT );
	}
	else /* connected socket */
	{

	  if( async_ctl.lEvent & WS_FD_OOB )
	    if( FD_ISSET(async_ctl.ws_sock->fd, &fd_excp) )
	    {
	      async_ctl.ip.lParam = WSAMAKESELECTREPLY( WS_FD_OOB, 0 );
	      async_ctl.lEvent &= ~WS_FD_OOB;
	      FD_CLR(async_ctl.ws_sock->fd, &fd_excp);
	      notify_parent( WSMSG_ASYNC_SELECT );
	    }
	    else FD_SET(async_ctl.ws_sock->fd, &fd_excp);

          if( async_ctl.lEvent & WS_FD_WRITE )
            if( FD_ISSET( async_ctl.ws_sock->fd, &fd_write ) )
            {
              async_ctl.ip.lParam = WSAMAKESELECTREPLY( WS_FD_WRITE, 0 );
              async_ctl.lEvent &= ~WS_FD_WRITE;
              FD_CLR(async_ctl.ws_sock->fd, &fd_write);
              notify_parent( WSMSG_ASYNC_SELECT );
            }
            else FD_SET(async_ctl.ws_sock->fd, &fd_write);

	  if( async_ctl.lEvent & (WS_FD_READ | WS_FD_CLOSE) )
	    if( FD_ISSET(async_ctl.ws_sock->fd, &fd_read) )
	    {
	      int 	ok = 0;

	      if( sock_type == SOCK_RAW ) ok = 1;
	      else if( ioctl( async_ctl.ws_sock->fd, FIONREAD, (char*)&bytes) == -1 )
	      {
		  async_ctl.ip.lParam = WSAMAKESELECTREPLY( WS_FD_READ, wsaErrno() );
		  FD_CLR( async_ctl.ws_sock->fd, &fd_read );
		  bytes = 0;
	      }

	      if( bytes || ok )	/* got data */
	      {
#if __WS_ASYNC_DEBUG
		  if( ok ) printf("\traw/datagram read pending\n");
		  else printf("\t%i bytes pending\n", bytes );
#endif
		  if( async_ctl.lEvent & WS_FD_READ )
		  {
		     async_ctl.ip.lParam = WSAMAKESELECTREPLY( WS_FD_READ, 0 );
		     async_ctl.lEvent &= ~WS_FD_READ;
		     if( !(async_ctl.lEvent & WS_FD_CLOSE) ) 
			   FD_CLR( async_ctl.ws_sock->fd, &fd_read );
		  }
		  else if( !(async_ctl.lEvent & (WS_FD_WRITE | WS_FD_OOB)) ) 
		       {
			  sigprocmask( SIG_UNBLOCK, &sig_block, NULL);
			  pause();
			  sigprocmask( SIG_BLOCK, &sig_block, NULL);
		       }
		       else continue;
	      }
	      else		/* 0 bytes to read */
	      {
		  val = read( async_ctl.ws_sock->fd, (char*)&bytes, 4);
	          if( errno == EWOULDBLOCK || errno == EINTR ) 
		  { 
#if __WS_ASYNC_DEBUG
		    printf("\twould block..\n");
#endif
		    continue;
		  }
		  switch( val )
		  {
		    case  0: errno = ENETDOWN;	/* ENETDOWN */
		    case -1: 			/* ECONNRESET */
			     async_ctl.ip.lParam = WSAMAKESELECTREPLY( WS_FD_CLOSE, wsaErrno() );
			     break;
		    default: continue;
		  }
		  async_ctl.lEvent &= ~(WS_FD_CLOSE | WS_FD_READ); /* one-shot */
		  FD_ZERO(&fd_read); FD_ZERO(&fd_write); 
	      }

	      notify_parent( WSMSG_ASYNC_SELECT );
	  }
	  else FD_SET(async_ctl.ws_sock->fd, &fd_read);

	} /* connected socket */
    } /* I/O socket */
  } /* while */
}


/* ----------------------------------- getXbyY requests */

static void _async_fail()
{
     async_ctl.lLength =
        (h_errno < 0) ? (unsigned)WSAMAKEASYNCREPLY( 0, wsaErrno() )
                      : (unsigned)WSAMAKEASYNCREPLY( 0, wsaHerrno() );
     write(async_ctl.ws_aop->fd[1], &async_ctl.lLength, sizeof(unsigned) );
     kill(getppid(), SIGIO);    /* simulate async I/O */
     pause();
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
  p_he = (flag & WSMSG_ASYNC_HOSTBYNAME)
	 ? gethostbyname(async_ctl.init)
	 : gethostbyaddr(async_ctl.init,
		 	 async_ctl.lLength, async_ctl.lEvent);
  if( p_he ) size = WS_dup_he(pwsi, p_he, WS_DUP_SEGPTR | WS_DUP_OFFSET );
  if( size )
  {
     async_ctl.buffer = pwsi->buffer;
     async_ctl.lLength = (unsigned)WSAMAKEASYNCREPLY( (UINT16)size, 0 );
     notify_parent( flag );
  }
  else _async_fail();
}

void WS_do_async_getproto(LPWSINFO pwsi, unsigned flag )
{
  int			size = 0;
  struct protoent*	p_pe;

  close(async_ctl.ws_aop->fd[0]);
  p_pe = (flag & WSMSG_ASYNC_PROTOBYNAME)
	 ? getprotobyname(async_ctl.init)
	 : getprotobynumber(async_ctl.lEvent);
  if( p_pe ) size = WS_dup_pe(pwsi, p_pe, WS_DUP_SEGPTR | WS_DUP_OFFSET );
  if( size )
  {
     async_ctl.buffer = pwsi->buffer;
     async_ctl.lLength = (unsigned)WSAMAKEASYNCREPLY( (UINT16)size, 0 );
     notify_parent( flag );
  } 
  else _async_fail();
}

void WS_do_async_getserv(LPWSINFO pwsi, unsigned flag )
{
  int			size = 0;
  struct servent* 	p_se;

  close(async_ctl.ws_aop->fd[0]);
  p_se = (flag & WSMSG_ASYNC_SERVBYNAME)
	 ? getservbyname(async_ctl.init, async_ctl.buffer)
	 : getservbyport(async_ctl.lEvent, async_ctl.init);
  if( p_se ) size = WS_dup_se(pwsi, p_se, WS_DUP_SEGPTR | WS_DUP_OFFSET );
  if( size )
  {
     async_ctl.buffer = pwsi->buffer;
     async_ctl.lLength = (unsigned)WSAMAKEASYNCREPLY( (UINT16)size, 0 );
     notify_parent( flag );
  }
  else _async_fail();
}

