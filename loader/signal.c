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
#include "registers.h"
#include "win.h"

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
static void wine_timer(int signal, struct sigcontext_struct context_struct)
{
#elif defined(__svr4__)
static void wine_timer(int signal, void *siginfo, ucontext_t *context)
{
#else
static void wine_timer(int signal, int code, struct sigcontext *context)
{
#endif
    /* Should do real-time timers here */

    DOSMEM_Tick();
}


/**********************************************************************
 *		win_fault
 *
 * Segfault handler.
 */
#ifdef linux
static void win_fault(int signal, struct sigcontext_struct context_struct)
{
    struct sigcontext_struct *context = &context_struct;
#elif defined(__svr4__) || defined(_SCO_DS)
static void win_fault(int signal, void *siginfo, ucontext_t *context)
{
#else
static void win_fault(int signal, int code, struct sigcontext *context)
{
#endif
    if (signal == SIGTRAP)
    {
        /* If SIGTRAP not caused by breakpoint or single step 
           don't jump into the debugger */
        if (!(EFL_reg(context) & STEP_FLAG))
        {
            DBG_ADDR addr;
            addr.seg = CS_reg(context);
            addr.off = EIP_reg(context) - 1;
            if (DEBUG_FindBreakpoint(&addr) == -1) return;
        }
    }
    else if (signal != SIGHUP)
    {
        if (CS_reg(context) == WINE_CODE_SELECTOR)
        {
            fprintf(stderr, "Segmentation fault in Wine program (%x:%lx)."
                            "  Please debug.\n",
                            CS_reg(context), EIP_reg(context) );
        }
        else
        {
            if (INSTR_EmulateInstruction( context )) return;
            fprintf( stderr, "Segmentation fault in Windows program %x:%lx.\n",
                     CS_reg(context), EIP_reg(context) );
        }
    }

    XUngrabPointer(display, CurrentTime);
    XUngrabServer(display);
    XFlush(display);
    wine_debug( signal, context );  /* Enter our debugger */
}


/**********************************************************************
 *		SIGNAL_SetHandler
 */
static void SIGNAL_SetHandler( int sig, void (*func)() )
{
    int ret;
    struct sigaction sig_act;

#ifdef linux
    sig_act.sa_handler = func;
    sig_act.sa_flags = SA_RESTART | SA_NOMASK;
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
    sig_act.sa_flags = SA_ONSTACK | SA_SIGINFO;
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


/**********************************************************************
 *		init_wine_signals
 */
void init_wine_signals(void)
{
#if defined(__NetBSD__) || defined(__FreeBSD__)
    struct sigaltstack ss;
        
#if !defined (__FreeBSD__) 
    if ((ss.ss_base = malloc(MINSIGSTKSZ)) == NULL) {
#else
    if ((ss.ss_sp = malloc(MINSIGSTKSZ)) == NULL) {
#endif
        fprintf(stderr, "Unable to allocate signal stack (%d bytes)\n",
                MINSIGSTKSZ);
        exit(1);
    }
    ss.ss_size = MINSIGSTKSZ;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) < 0) {
        perror("sigstack");
        exit(1);
    }
#endif  /* __FreeBSD__ || __NetBSD__ */

#if defined (__svr4__) || defined(_SCO_DS)
    struct sigaltstack ss;
        
    if ((ss.ss_sp = malloc(SIGSTKSZ) ) == NULL) {
        fprintf(stderr, "Unable to allocate signal stack (%d bytes)\n",
                SIGSTKSZ);
        exit(1);
    }
    ss.ss_size = SIGSTKSZ;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) < 0) {
        perror("sigstack");
        exit(1);
    }
#endif  /* __svr4__ || _SCO_DS */
    
    SIGNAL_SetHandler( SIGALRM, (void (*)())wine_timer );
    SIGNAL_SetHandler( SIGSEGV, (void (*)())win_fault );
    SIGNAL_SetHandler( SIGILL,  (void (*)())win_fault );
    SIGNAL_SetHandler( SIGFPE,  (void (*)())win_fault );
    SIGNAL_SetHandler( SIGTRAP, (void (*)())win_fault ); /* For debugger */
    SIGNAL_SetHandler( SIGHUP,  (void (*)())win_fault ); /* For forced break */
#ifdef SIGBUS
    SIGNAL_SetHandler( SIGBUS,  (void (*)())win_fault );
#endif
#ifdef CONFIG_IPC
    SIGNAL_SetHandler( SIGUSR2, (void (*)())stop_wait ); /* For IPC */
#endif
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

#endif /* ifndef WINELIB */
