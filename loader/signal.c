/*
 * Wine signal handling
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <setjmp.h>

#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/wait.h>

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__svr4__) || defined(_SCO_DS) || defined(__EMX__)
#if !defined(_SCO_DS) && !defined(__EMX__)
#include <sys/syscall.h>
#endif
#include <sys/param.h>
#else
#include <syscall.h>
#endif

#include "miscemu.h"
#include "winsock.h"


/* Linux sigaction function */

#if defined(linux) && defined(__i386__)
/* This is the sigaction structure from the Linux 2.1.20 kernel.  */
struct kernel_sigaction
{
    void (*sa_handler)();
    unsigned long sa_mask;
    unsigned long sa_flags;
    void (*sa_restorer)();
};

/* Similar to the sigaction function in libc, except it leaves alone the
   restorer field, which is used to specify the signal stack address */
static __inline__ int wine_sigaction( int sig, struct kernel_sigaction *new,
                                      struct kernel_sigaction *old )
{
    __asm__ __volatile__( "int $0x80"
                          : "=a" (sig)
                          : "0" (SYS_sigaction),
                            "b" (sig),
                            "c" (new),
                            "d" (old) );
    if (sig>=0)
        return 0;
    errno = -sig;
    return -1;
}
#endif /* linux && __i386__ */


/* Signal stack */

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
# define SIGNAL_STACK_SIZE  MINSIGSTKSZ
#elif defined (__svr4__) || defined(_SCO_DS)
# define SIGNAL_STACK_SIZE  SIGSTKSZ
#else
# define SIGNAL_STACK_SIZE  4096
#endif

static char SIGNAL_Stack[SIGNAL_STACK_SIZE];


/**********************************************************************
 *              SIGNAL_child
 * 
 * wait4 terminated child processes
 */
static void SIGNAL_child(void)
{
#ifdef HAVE_WAIT4
    wait4( 0, NULL, WNOHANG, NULL);
#elif defined (HAVE_WAITPID)
    /* I am sort-of guessing that this is the same as the wait4 call.  */
    waitpid (0, NULL, WNOHANG);
#else
    wait(NULL);
#endif
}


/**********************************************************************
 *		SIGNAL_SetHandler
 */
void SIGNAL_SetHandler( int sig, void (*func)(), int flags )
{
    int ret;

#if defined(linux) && defined(__i386__)

    struct kernel_sigaction sig_act;
    sig_act.sa_handler = func;
    sig_act.sa_flags = SA_RESTART | (flags) ? SA_NOMASK : 0;
    /* Point to the top of the stack, minus 4 just in case, and make
       it aligned  */
    sig_act.sa_restorer = 
        (void (*)())((int)(SIGNAL_Stack + sizeof(SIGNAL_Stack) - 4) & ~3);
    ret = wine_sigaction( sig, &sig_act, NULL );

#else  /* linux && __i386__ */

    struct sigaction sig_act;
    sigset_t sig_mask;
    sigemptyset(&sig_mask);
    sig_act.sa_handler = func;
    sig_act.sa_mask = sig_mask;

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

extern void stop_wait(int a);
extern void WINSOCK_sigio(int a);


/**********************************************************************
 *		SIGNAL_Init
 */
BOOL32 SIGNAL_Init(void)
{
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined (__svr4__) || defined(_SCO_DS)
    struct sigaltstack ss;
    ss.ss_sp    = SIGNAL_Stack;
    ss.ss_size  = sizeof(SIGNAL_Stack);
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) < 0)
    {
        perror("sigstack");
        return FALSE;
    }
#endif  /* __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __svr4__ || _SCO_DS */
    
    SIGNAL_SetHandler( SIGCHLD, (void (*)())SIGNAL_child, 1);
#ifdef CONFIG_IPC
    SIGNAL_SetHandler( SIGUSR2, (void (*)())stop_wait, 1); 	/* For IPC */
#endif
#ifdef SIGIO
    SIGNAL_SetHandler( SIGIO,   (void (*)())WINSOCK_sigio, 0); 
#endif
    return TRUE;
}


/**********************************************************************
 *              SIGNAL_MaskAsyncEvents
 */
void SIGNAL_MaskAsyncEvents( BOOL32 flag )
{
  sigset_t 	set;
  sigemptyset(&set);
#ifdef SIGIO
  sigaddset(&set, SIGIO);
#endif
  sigaddset(&set, SIGUSR1);
#ifdef CONFIG_IPC
  sigaddset(&set, SIGUSR2);
#endif
  sigprocmask( (flag) ? SIG_BLOCK : SIG_UNBLOCK , &set, NULL);
}
