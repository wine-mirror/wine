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

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__svr4__) || defined(_SCO_DS)
#ifndef _SCO_DS
#include <sys/syscall.h>
#endif
#include <sys/param.h>
#else
#include <syscall.h>
#endif

#include "debugger.h"
#include "miscemu.h"
#include "options.h"
#include "registers.h"
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
#endif


/**********************************************************************
 *		wine_timer
 *
 * SIGALRM handler.
 */
#ifdef linux
static void wine_timer(int signal, SIGCONTEXT context_struct)
{
#elif defined(__svr4__)
static void wine_timer(int signal, void *siginfo, SIGCONTEXT *context)
{
#else
static void wine_timer(int signal, int code, SIGCONTEXT *context)
{
#endif
    /* Should do real-time timers here */

    DOSMEM_Tick();
}

/**********************************************************************
 *              SIGNAL_break
 * 
 * Handle Ctrl-C and such
 */
#ifdef linux
static void SIGNAL_break(int signal, SIGCONTEXT context_struct)
{
    SIGCONTEXT *context = &context_struct;
#elif defined(__svr4__) || defined(_SCO_DS)
static void SIGNAL_break(int signal, void *siginfo, SIGCONTEXT *context)
{
#else
static void SIGNAL_break(int signal, int code, SIGCONTEXT *context)
{
#endif
    if (Options.debug) wine_debug( signal, context );  /* Enter our debugger */
    exit(0);
}

/**********************************************************************
 *              SIGNAL_child
 * 
 * wait4 terminated child processes
 */
#ifdef linux
static void SIGNAL_child(int signal, SIGCONTEXT context_struct)
{
    SIGCONTEXT *context = &context_struct;
#elif defined(__svr4__) || defined(_SCO_DS)
static void SIGNAL_child(int signal, void *siginfo, SIGCONTEXT *context)
{
#else
static void SIGNAL_child(int signal, int code, SIGCONTEXT *context)
{
#endif
   wait4( 0, NULL, WNOHANG, NULL);
}


/**********************************************************************
 *		SIGNAL_trap
 *
 * SIGTRAP handler.
 */
#ifdef linux
static void SIGNAL_trap(int signal, SIGCONTEXT context_struct)
{
    SIGCONTEXT *context = &context_struct;
#elif defined(__svr4__) || defined(_SCO_DS)
static void SIGNAL_trap(int signal, void *siginfo, SIGCONTEXT *context)
{
#else
static void SIGNAL_trap(int signal, int code, SIGCONTEXT *context)
{
#endif
    wine_debug( signal, context );  /* Enter our debugger */
}


/**********************************************************************
 *		SIGNAL_fault
 *
 * Segfault handler.
 */
#ifdef linux
static void SIGNAL_fault(int signal, SIGCONTEXT context_struct)
{
    SIGCONTEXT *context = &context_struct;
#elif defined(__svr4__) || defined(_SCO_DS)
static void SIGNAL_fault(int signal, void *siginfo, SIGCONTEXT *context)
{
#else
static void SIGNAL_fault(int signal, int code, SIGCONTEXT *context)
{
#endif
    if (CS_reg(context) == WINE_CODE_SELECTOR)
    {
        fprintf( stderr, "Segmentation fault in Wine program (%x:%lx)."
                         "  Please debug.\n",
                 CS_reg(context), EIP_reg(context) );
    }
    else
    {
        if (INSTR_EmulateInstruction( context )) return;
        fprintf( stderr, "Segmentation fault in Windows program %x:%lx.\n",
                 CS_reg(context), EIP_reg(context) );
    }
    wine_debug( signal, context );
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
    SIGNAL_SetHandler( SIGIO,   (void (*)())WINSOCK_sigio, 0); 
    return TRUE;
}


/**********************************************************************
 *		SIGNAL_StartBIOSTimer
 *
 * Start the BIOS tick timer.
 */
void SIGNAL_StartBIOSTimer(void)
{
    struct itimerval vt_timer;
    static int timer_started = 0;

    if (timer_started) return;
    timer_started = 1;
    vt_timer.it_interval.tv_sec = 0;
    vt_timer.it_interval.tv_usec = 54929;
    vt_timer.it_value = vt_timer.it_interval;

    setitimer(ITIMER_REAL, &vt_timer, NULL);
}

/**********************************************************************
 *              SIGNAL_MaskAsyncEvents
 */
void SIGNAL_MaskAsyncEvents( BOOL32 flag )
{
  sigset_t 	set;
  sigemptyset(&set);
  sigaddset(&set, SIGIO);
  sigaddset(&set, SIGUSR1);
#ifdef CONFIG_IPC
  sigaddset(&set, SIGUSR2);
#endif
  sigprocmask( (flag) ? SIG_BLOCK : SIG_UNBLOCK , &set, NULL);
}

#endif /* ifndef WINELIB */
