#ifndef WINELIB
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

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__svr4__) || defined(_SCO_DS) || defined(__EMX__)
#if !defined(_SCO_DS) && !defined(__EMX__)
#include <sys/syscall.h>
#endif
#include <sys/param.h>
#else
#include <syscall.h>
#endif

#include "debugger.h"
#include "options.h"
#include "sigcontext.h"
#include "win.h"
#include "winsock.h"

#if !defined(BSD4_4) || defined(linux) || defined(__FreeBSD__)
char * cstack[4096];
#endif

#ifdef linux
extern void ___sig_restore();
extern void ___masksig_restore();

/* Similar to the sigaction function in libc, except it leaves alone the
   restorer field */

static int
wine_sigaction(int sig,struct sigaction * new, struct sigaction * old)
{
	__asm__("int $0x80":"=a" (sig)
		:"0" (SYS_sigaction),"b" (sig),"c" (new),"d" (old));
	if (sig>=0)
		return 0;
	errno = -sig;
	return -1;
}
#endif /* linux */


#ifdef linux
#define HANDLER_DEF(name) void name (int signal, SIGCONTEXT context_struct)
#define HANDLER_PROLOG SIGCONTEXT *context = &context_struct; (void)context; {
#define HANDLER_EPILOG }
#elif defined(__svr4__) || defined(_SCO_DS)
#define HANDLER_DEF(name) void name (int signal, void *siginfo, SIGCONTEXT *context)
#define HANDLER_PROLOG  /* nothing */
#define HANDLER_EPILOG  /* nothing */
#else
#define HANDLER_DEF(name) void name (int signal, int code, SIGCONTEXT *context)
#define HANDLER_PROLOG  /* nothing */
#define HANDLER_EPILOG  /* nothing */
#endif

extern BOOL32 INSTR_EmulateInstruction( SIGCONTEXT *context );

/**********************************************************************
 *		wine_timer
 *
 * SIGALRM handler.
 */
static
HANDLER_DEF(wine_timer)
{
  HANDLER_PROLOG;
  /* Should do real-time timers here */
  DOSMEM_Tick();
  HANDLER_EPILOG;
}

/**********************************************************************
 *              SIGNAL_break
 * 
 * Handle Ctrl-C and such
 */
static
HANDLER_DEF(SIGNAL_break)
{
  HANDLER_PROLOG;
  if (Options.debug) wine_debug( signal, context );  /* Enter our debugger */
  exit(0);
  HANDLER_EPILOG;
}

/**********************************************************************
 *              SIGNAL_child
 * 
 * wait4 terminated child processes
 */
static
HANDLER_DEF(SIGNAL_child)
{
  HANDLER_PROLOG;
#ifdef HAVE_WAIT4
  wait4( 0, NULL, WNOHANG, NULL);
#elif defined (HAVE_WAITPID)
  /* I am sort-of guessing that this is the same as the wait4 call.  */
  waitpid (0, NULL, WNOHANG);
#else
  wait(NULL);
#endif
  HANDLER_EPILOG;
}


/**********************************************************************
 *		SIGNAL_trap
 *
 * SIGTRAP handler.
 */
static
HANDLER_DEF(SIGNAL_trap)
{
  HANDLER_PROLOG;
  wine_debug( signal, context );  /* Enter our debugger */
  HANDLER_EPILOG;
}


/**********************************************************************
 *		SIGNAL_fault
 *
 * Segfault handler.
 */
static
HANDLER_DEF(SIGNAL_fault)
{
  HANDLER_PROLOG;
  if (CS_sig(context) == WINE_CODE_SELECTOR)
    {
        fprintf( stderr, "Segmentation fault in Wine program (%x:%lx)."
                         "  Please debug.\n",
                 CS_sig(context), EIP_sig(context) );
    }
  else
    {
        if (INSTR_EmulateInstruction( context )) return;
        fprintf( stderr, "Segmentation fault in Windows program %x:%lx.\n",
                 CS_sig(context), EIP_sig(context) );
    }
  wine_debug( signal, context );
  HANDLER_EPILOG;
}


/**********************************************************************
 *		SIGNAL_SetHandler
 */
static void SIGNAL_SetHandler( int sig, void (*func)(), int flags )
{
    int ret;
    struct sigaction sig_act;

#ifdef linux
    sig_act.sa_handler = func;
    sig_act.sa_flags = SA_RESTART | (flags) ? SA_NOMASK : 0;
    /* Point to the top of the stack, minus 4 just in case, and make
       it aligned  */
    sig_act.sa_restorer = 
        (void (*)()) (((unsigned int)(cstack) + sizeof(cstack) - 4) & ~3);
    ret = wine_sigaction( sig, &sig_act, NULL );
#endif  /* linux */

#if defined(__NetBSD__) || defined(__FreeBSD__)
    sigset_t sig_mask;
    sigemptyset(&sig_mask);
    sig_act.sa_handler = func;
    sig_act.sa_flags = SA_ONSTACK;
    sig_act.sa_mask = sig_mask;
    ret = sigaction( sig, &sig_act, NULL );
#endif  /* __FreeBSD__ || __NetBSD__ */

#if defined (__svr4__) || defined(_SCO_DS)
    sigset_t sig_mask;
    sigemptyset(&sig_mask);
    sig_act.sa_handler = func;
    sig_act.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART;
    sig_act.sa_mask = sig_mask;
    ret = sigaction( sig, &sig_act, NULL );
#endif  /* __svr4__ || _SCO_DS */

#if defined(__EMX__)
    sigset_t sig_mask;
    sigemptyset(&sig_mask);
    sig_act.sa_handler = func;
    sig_act.sa_flags = 0; /* FIXME: EMX has only SA_ACK and SA_SYSV */
    sig_act.sa_mask = sig_mask;
    ret = sigaction( sig, &sig_act, NULL );
#endif  /* __EMX__ */

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
#if defined(__NetBSD__) || defined(__FreeBSD__)
    struct sigaltstack ss;
        
    if ((ss.ss_sp = malloc(MINSIGSTKSZ)) == NULL) {
        fprintf(stderr, "Unable to allocate signal stack (%d bytes)\n",
                MINSIGSTKSZ);
        return FALSE;
    }
    ss.ss_size = MINSIGSTKSZ;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) < 0) {
        perror("sigstack");
        return FALSE;
    }
#endif  /* __FreeBSD__ || __NetBSD__ */

#if defined (__svr4__) || defined(_SCO_DS)
    struct sigaltstack ss;
        
    if ((ss.ss_sp = malloc(SIGSTKSZ) ) == NULL) {
        fprintf(stderr, "Unable to allocate signal stack (%d bytes)\n",
                SIGSTKSZ);
        return FALSE;
    }
    ss.ss_size = SIGSTKSZ;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) < 0) {
        perror("sigstack");
        return FALSE;
    }
#endif  /* __svr4__ || _SCO_DS */
    
    SIGNAL_SetHandler( SIGALRM, (void (*)())wine_timer, 1);
    SIGNAL_SetHandler( SIGINT,  (void (*)())SIGNAL_break, 1);
    SIGNAL_SetHandler( SIGCHLD, (void (*)())SIGNAL_child, 1);
    SIGNAL_SetHandler( SIGSEGV, (void (*)())SIGNAL_fault, 1);
    SIGNAL_SetHandler( SIGILL,  (void (*)())SIGNAL_fault, 1);
    SIGNAL_SetHandler( SIGFPE,  (void (*)())SIGNAL_fault, 1);
    SIGNAL_SetHandler( SIGTRAP, (void (*)())SIGNAL_trap, 1); 	/* debugger */
    SIGNAL_SetHandler( SIGHUP,  (void (*)())SIGNAL_trap, 1); 	/* forced break */
#ifdef SIGBUS
    SIGNAL_SetHandler( SIGBUS,  (void (*)())SIGNAL_fault, 1);
#endif
#ifdef CONFIG_IPC
    SIGNAL_SetHandler( SIGUSR2, (void (*)())stop_wait, 1); 	/* For IPC */
#endif
#ifndef __EMX__ /* FIXME */
    SIGNAL_SetHandler( SIGIO,   (void (*)())WINSOCK_sigio, 0); 
#endif
    return TRUE;
}


/**********************************************************************
 *		SIGNAL_StartBIOSTimer
 *
 * Start the BIOS tick timer.
 */
void SIGNAL_StartBIOSTimer(void)
{
#ifndef __EMX__ /* FIXME: Time don't work... Use BIOS directly instead */
    struct itimerval vt_timer;
    static int timer_started = 0;

    if (timer_started) return;
    timer_started = 1;
    vt_timer.it_interval.tv_sec = 0;
    vt_timer.it_interval.tv_usec = 54929;
    vt_timer.it_value = vt_timer.it_interval;

    setitimer(ITIMER_REAL, &vt_timer, NULL);
#endif
}

/**********************************************************************
 *              SIGNAL_MaskAsyncEvents
 */
void SIGNAL_MaskAsyncEvents( BOOL32 flag )
{
  sigset_t 	set;
  sigemptyset(&set);
#ifndef __EMX__ /* FIXME */
  sigaddset(&set, SIGIO);
#endif
  sigaddset(&set, SIGUSR1);
#ifdef CONFIG_IPC
  sigaddset(&set, SIGUSR2);
#endif
  sigprocmask( (flag) ? SIG_BLOCK : SIG_UNBLOCK , &set, NULL);
}

#endif /* ifndef WINELIB */
