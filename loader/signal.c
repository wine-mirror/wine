/*
 * Wine signal handling
 *
 * Copyright 1995 Alexandre Julliard
 */

#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <setjmp.h>

#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYSCALL_H
# include <syscall.h>
#else
# ifdef HAVE_SYS_SYSCALL_H
#  include <sys/syscall.h>
# endif
#endif

#include "winsock.h"
#include "global.h"
#include "options.h"
#include "miscemu.h"
#include "dosexe.h"
#include "thread.h"
#include "wine/exception.h"
#include "debugtools.h"


/* Linux sigaction function */

#if defined(linux) && defined(__i386__)
/* This is the sigaction structure from the Linux 2.1.20 kernel.  */

#undef sa_handler
struct kernel_sigaction
{
    void (*sa_handler)();
    unsigned long sa_mask;
    unsigned long sa_flags;
    void (*sa_restorer)();
};

/* Similar to the sigaction function in libc, except it leaves alone the
   restorer field, which is used to specify the signal stack address */
static inline int wine_sigaction( int sig, struct kernel_sigaction *new,
                                      struct kernel_sigaction *old )
{
#ifdef __PIC__
    __asm__ __volatile__( "pushl %%ebx\n\t"
                          "movl %2,%%ebx\n\t"
                          "int $0x80\n\t"
                          "popl %%ebx"
                          : "=a" (sig)
                          : "0" (SYS_sigaction),
                            "r" (sig),
                            "c" (new),
                            "d" (old) );
#else
    __asm__ __volatile__( "int $0x80"
                          : "=a" (sig)
                          : "0" (SYS_sigaction),
                            "b" (sig),
                            "c" (new),
                            "d" (old) );
#endif  /* __PIC__ */
    if (sig>=0)
        return 0;
    errno = -sig;
    return -1;
}
#endif /* linux && __i386__ */

/* Signal stack */

static char SIGNAL_Stack[16384];


/**********************************************************************
 *		SIGNAL_SetHandler
 */
void SIGNAL_SetHandler( int sig, void (*func)() )
{
    int ret;

#if defined(linux) && defined(__i386__)

    struct kernel_sigaction sig_act;
    sig_act.sa_handler = func;
    sig_act.sa_flags   = SA_RESTART | SA_NOMASK;
    sig_act.sa_mask    = 0;
    /* Point to the top of the stack, minus 4 just in case, and make
       it aligned  */
    sig_act.sa_restorer = 
        (void (*)())((int)(SIGNAL_Stack + sizeof(SIGNAL_Stack) - 4) & ~3);
    ret = wine_sigaction( sig, &sig_act, NULL );

#else  /* linux && __i386__ */

    struct sigaction sig_act;
    sig_act.sa_handler = func;
    sigemptyset( &sig_act.sa_mask );

# if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    sig_act.sa_flags = SA_ONSTACK;
# elif defined (__svr4__) || defined(_SCO_DS)
    sig_act.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART;
# elif defined(__EMX__)
    sig_act.sa_flags = 0; /* FIXME: EMX has only SA_ACK and SA_SYSV */
# else
    sig_act.sa_flags = 0;
# endif
    ret = sigaction( sig, &sig_act, NULL );

#endif  /* linux && __i386__ */

    if (ret < 0)
    {
        perror( "sigaction" );
        exit(1);
    }
}


/**********************************************************************
 *		SIGNAL_Init
 */
BOOL SIGNAL_Init(void)
{
#ifdef HAVE_WORKING_SIGALTSTACK
    struct sigaltstack ss;
    ss.ss_sp    = SIGNAL_Stack;
    ss.ss_size  = sizeof(SIGNAL_Stack);
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) < 0)
    {
        perror("sigaltstack");
	/* fall through on error and try it differently */
    }
#endif  /* HAVE_SIGALTSTACK */
    
    /* ignore SIGPIPE so that WINSOCK can get a EPIPE error instead  */
    signal (SIGPIPE, SIG_IGN);
    /* automatic child reaping to avoid zombies */
    signal (SIGCHLD, SIG_IGN);
    EXC_InitHandlers();
    return TRUE;
}
