#ifndef WINELIB
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <setjmp.h>

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__svr4__)
#include <sys/syscall.h>
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
struct sigaction segv_act;
struct sigaction usr2_act;

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


#if defined(linux) 
static void win_fault(int signal, struct sigcontext_struct context_struct)
{
    struct sigcontext_struct *context = &context_struct;
#elif defined(__svr4__)
static void win_fault(int signal, void *siginfo, ucontext_t *context)
{
#else
static void win_fault(int signal, int code, struct sigcontext *context)
{
#endif
    int i;
    
    if (signal != SIGTRAP)
    {
        if (CS_reg(context) == WINE_CODE_SELECTOR)
        {
            fprintf(stderr, "Segmentation fault in Wine program (%x:%lx)."
                            "  Please debug\n",
                            CS_reg(context), EIP_reg(context) );
        }
        else if (INSTR_EmulateInstruction( context )) return;
        fprintf( stderr,"In win_fault %x:%lx\n",
                 CS_reg(context), EIP_reg(context) );
    }
    XUngrabPointer(display, CurrentTime);
    XUngrabServer(display);
    XFlush(display);
    wine_debug( signal, context );  /* Enter our debugger */
}

void init_wine_signals(void)
{
        extern void stop_wait(int a);
#ifdef linux
	segv_act.sa_handler = (__sighandler_t) win_fault;
	/* Point to the top of the stack, minus 4 just in case, and make
	   it aligned  */
	segv_act.sa_restorer = 
	    (void (*)()) (((unsigned int)(cstack) + sizeof(cstack) - 4) & ~3);
	/* Point to the top of the stack, minus 4 just in case, and make
	   it aligned  */
	wine_sigaction(SIGSEGV, &segv_act, NULL);
	wine_sigaction(SIGILL, &segv_act, NULL);
	wine_sigaction(SIGFPE, &segv_act, NULL);
#ifdef SIGBUS
	wine_sigaction(SIGBUS, &segv_act, NULL);
#endif
	wine_sigaction(SIGTRAP, &segv_act, NULL); /* For breakpoints */
#ifdef CONFIG_IPC
	usr2_act.sa_restorer= segv_act.sa_restorer;
	usr2_act.sa_handler = (__sighandler_t) stop_wait;
	wine_sigaction(SIGUSR2, &usr2_act, NULL);
#endif  /* CONFIG_IPC */
#endif  /* linux */
#if defined(__NetBSD__) || defined(__FreeBSD__)
        sigset_t sig_mask;
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
        sigemptyset(&sig_mask);
        segv_act.sa_handler = (void (*)) win_fault;
	segv_act.sa_flags = SA_ONSTACK;
        segv_act.sa_mask = sig_mask;
	if (sigaction(SIGBUS, &segv_act, NULL) < 0) {
                perror("sigaction: SIGBUS");
                exit(1);
        }
        segv_act.sa_handler = (void (*)) win_fault;
	segv_act.sa_flags = SA_ONSTACK;
        segv_act.sa_mask = sig_mask;
	if (sigaction(SIGSEGV, &segv_act, NULL) < 0) {
                perror("sigaction: SIGSEGV");
                exit(1);
        }
        segv_act.sa_handler = (void (*)) win_fault; /* For breakpoints */
	segv_act.sa_flags = SA_ONSTACK;
        segv_act.sa_mask = sig_mask;
	if (sigaction(SIGTRAP, &segv_act, NULL) < 0) {
                perror("sigaction: SIGTRAP");
                exit(1);
        }
#ifdef CONFIG_IPC
        usr2_act.sa_handler = (void (*)) stop_wait; /* For breakpoints */
	usr2_act.sa_flags = SA_ONSTACK;
        usr2_act.sa_mask = sig_mask;
	if (sigaction(SIGUSR2, &usr2_act, NULL) < 0) {
                perror("sigaction: SIGUSR2");
                exit(1);
        }
#endif  /* CONFIG_IPC */
#endif  /* __FreeBSD__ || __NetBSD__ */
#if defined (__svr4__)
        sigset_t sig_mask;
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
        sigemptyset(&sig_mask);
        segv_act.sa_handler = (void (*)) win_fault;
	segv_act.sa_flags = SA_ONSTACK | SA_SIGINFO;
        segv_act.sa_mask = sig_mask;
	if (sigaction(SIGBUS, &segv_act, NULL) < 0) {
            perror("sigaction: SIGBUS");
            exit(1);
        }
        segv_act.sa_handler = (void (*)) win_fault;
	segv_act.sa_flags = SA_ONSTACK | SA_SIGINFO;
        segv_act.sa_mask = sig_mask;
	if (sigaction(SIGSEGV, &segv_act, NULL) < 0) {
            perror("sigaction: SIGSEGV");
            exit(1);
        }
        

        segv_act.sa_handler = (void (*)) win_fault; /* For breakpoints */
	segv_act.sa_flags = SA_ONSTACK | SA_SIGINFO;
        segv_act.sa_mask = sig_mask;
	if (sigaction(SIGTRAP, &segv_act, NULL) < 0) {
            perror("sigaction: SIGTRAP");
            exit(1);
        }
#ifdef CONFIG_IPC
        usr2_act.sa_handler = (void (*)) stop_wait; /* For breakpoints */
	usr2_act.sa_flags = SA_ONSTACK | SA_SIGINFO;
        usr2_act.sa_mask = sig_mask;
	if (sigaction(SIGUSR2, &usr2_act, NULL) < 0) {
            perror("sigaction: SIGUSR2");
            exit(1);
        }
#endif  /* CONFIG_IPC */
        
#endif  /* __svr4__ */
}

#endif /* ifndef WINELIB */
