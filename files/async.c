/*
 * Generic async UNIX file IO handling
 *
 * Copyright 1996,1997 Alex Korobka
 * Copyright 1998 Marcus Meissner
 */
/*
 * This file handles asynchronous signaling for UNIX filedescriptors. 
 * The passed handler gets called when input arrived for the filedescriptor.
 * 
 * This is done either by the kernel or (in the WINSOCK case) by the pipe
 * handler, since pipes do not support asynchronous signaling.
 * (Not all possible filedescriptors support async IO. Generic files do not
 *  for instance, sockets do, ptys don't.)
 * 
 * To make this a bit better, we would need an additional thread doing select()
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#ifdef HAVE_SYS_FILE_H
# include <sys/file.h>
#endif

#include "xmalloc.h"
#include "wintypes.h"
#include "miscemu.h"
#include "selectors.h"
#include "sig_context.h"
#include "async.h"
#include "debug.h"

typedef struct _async_fd {
	int	unixfd;
	void	(*handler)(int fd,void *private);
	void	*private;
} ASYNC_FD;

static ASYNC_FD	*asyncfds = NULL;
static int	 nrofasyncfds = 0;

/***************************************************************************
 *		ASYNC_sigio				[internal]
 * 
 * Signal handler for asynchronous IO.
 *
 * Note: This handler and the function it calls may not block. Neither they
 * are allowed to use blocking IO (write/read). No memory management.
 * No possible blocking synchronization of any kind.
 */
HANDLER_DEF(ASYNC_sigio) {
	struct timeval	timeout;
	fd_set	rset,wset;
	int	i,maxfd=0;

	HANDLER_INIT();

	if (!nrofasyncfds) 
		return;
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	for (i=nrofasyncfds;i--;) {
		if (asyncfds[i].unixfd == -1)
			continue;
		FD_SET(asyncfds[i].unixfd,&rset);
		FD_SET(asyncfds[i].unixfd,&wset);
		if (maxfd<asyncfds[i].unixfd)
			maxfd=asyncfds[i].unixfd;
	}
	/* select() with timeout values set to 0 is nonblocking. */
	memset(&timeout,0,sizeof(timeout));
	if (select(maxfd+1,&rset,&wset,NULL,&timeout)<=0)
		return; /* Can't be. hmm */
	for (i=nrofasyncfds;i--;)
		if (	(FD_ISSET(asyncfds[i].unixfd,&rset)) ||
			(FD_ISSET(asyncfds[i].unixfd,&wset))
		)
			asyncfds[i].handler(asyncfds[i].unixfd,asyncfds[i].private);
}

/***************************************************************************
 *		ASYNC_MakeFDAsync			[internal]
 *
 * Makes the passed filedescriptor async (or not) depending on flag.
 */
static BOOL32 ASYNC_MakeFDAsync(int unixfd,int async) {
    int	flags;

#if !defined(FASYNC) && defined(FIOASYNC)
#define FASYNC FIOASYNC
#endif

#ifdef F_SETOWN
    if (-1==fcntl(unixfd,F_SETOWN,getpid()))
    	perror("fcntl F_SETOWN <pid>");
#endif
#ifdef FASYNC
    if (-1==fcntl(unixfd,F_GETFL,&flags)) {
	perror("fcntl F_GETFL");
	return FALSE;
    }
    if (async)
	flags|=FASYNC;
    else
	flags&=~FASYNC;
    if (-1==fcntl(unixfd,F_SETFL,&flags)) {
	perror("fcntl F_SETFL FASYNC");
	return FALSE;
    }
    return TRUE;
#else
    return FALSE;
#endif
}

/***************************************************************************
 *		ASYNC_RegisterFD			[internal]
 *
 * Register a UNIX filedescriptor with handler and private data pointer. 
 * this function is _NOT_ safe to be called from a signal handler.
 *
 * Additional Constraint:  The handler passed to this function _MUST_ adhere
 * to the same signalsafeness as ASYNC_sigio itself. (nonblocking, no thread/
 * signal unsafe operations, no blocking synchronization)
 */
void ASYNC_RegisterFD(int unixfd,void (*handler)(int fd,void *private),void *private) {
    int	i;

    SIGNAL_MaskAsyncEvents( TRUE );
    for (i=0;i<nrofasyncfds;i++) {
	if (asyncfds[i].unixfd==unixfd) {
	    /* Might be a leftover entry. Make fd async anyway... */
	    if (asyncfds[i].handler==handler) {
	        ASYNC_MakeFDAsync(unixfd,1);
		SIGNAL_MaskAsyncEvents( FALSE );
	    	return;
	    }
	}
    }
   for (i=0;i<nrofasyncfds;i++)
	if (asyncfds[i].unixfd == -1)
		break;
   if (i==nrofasyncfds) {
       if (nrofasyncfds)
	   asyncfds=(ASYNC_FD*)xrealloc(asyncfds,sizeof(ASYNC_FD)*(nrofasyncfds+1));
       else
	   asyncfds=(ASYNC_FD*)xmalloc(sizeof(ASYNC_FD)*1);
       nrofasyncfds++;
   }
   asyncfds[i].unixfd	= unixfd;
   asyncfds[i].handler	= handler;
   asyncfds[i].private	= private;
   ASYNC_MakeFDAsync(unixfd,1);
   SIGNAL_MaskAsyncEvents( FALSE );
}

/***************************************************************************
 *		ASYNC_UnregisterFD			[internal]
 *
 * Unregister a UNIX filedescriptor with handler. This function is basically
 * signal safe, but try to not call it in the signal handler anyway.
 */
void ASYNC_UnregisterFD(int unixfd,void (*handler)(int fd,void *private)) {
    int	i;

    for (i=nrofasyncfds;i--;)
	if ((asyncfds[i].unixfd==unixfd)||(asyncfds[i].handler==handler))
	    break;
    if (i==nrofasyncfds)
    	return;
    asyncfds[i].unixfd	= -1;
    asyncfds[i].handler	= NULL;
    asyncfds[i].private	= NULL;
    return;
}
