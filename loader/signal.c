#ifndef WINELIB
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <setjmp.h>

#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/syscall.h>
#include <sys/param.h>
#else
#include <syscall.h>
#endif

#include "wine.h"
#include "prototypes.h"
#include "miscemu.h"
#include "registers.h"
#include "win.h"

#if !defined(BSD4_4) || defined(linux) || defined(__FreeBSD__)
char * cstack[4096];
#endif
struct sigaction segv_act;

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


#ifdef linux
static void win_fault(int signal, struct sigcontext_struct context_struct)
{
    struct sigcontext_struct *context = &context_struct;
#else
static void win_fault(int signal, int code, struct sigcontext *context)
{
#endif

#if !(defined (linux) || defined (__NetBSD__))
    int i, *dump;
#endif

    if(signal == SIGTRAP) EIP--;   /* Back up over the int3 instruction. */
    else if (CS == WINE_CODE_SELECTOR)
    {
	fprintf(stderr, "Segmentation fault in Wine program (%x:%lx)."
		        "  Please debug\n", CS, EIP );
    }
    else if (INSTR_EmulateInstruction( context )) return;

    XUngrabPointer(display, CurrentTime);
    XUngrabServer(display);
    XFlush(display);
    fprintf(stderr,"In win_fault %x:%lx\n", CS, EIP );
#if defined(linux) || defined(__NetBSD__) || defined(__FreeBSD__)
    wine_debug(signal, (int *)context);  /* Enter our debugger */
#else
    fprintf(stderr,"Stack: %x:%x\n", SS, ESP );
    dump = (int*) context;
    for(i=0; i<22; i++) 
    {
	fprintf(stderr," %8.8x", *dump++);
	if ((i % 8) == 7)
	    fprintf(stderr,"\n");
    }
    fprintf(stderr,"\n");
    exit(1);
#endif
}

void init_wine_signals(void)
{
#ifdef linux
	segv_act.sa_handler = (__sighandler_t) win_fault;
	/* Point to the top of the stack, minus 4 just in case, and make
	   it aligned  */
	segv_act.sa_restorer = 
		(void (*)()) (((unsigned int)(cstack) + sizeof(cstack) - 4) & ~3);
	wine_sigaction(SIGSEGV, &segv_act, NULL);
	wine_sigaction(SIGILL, &segv_act, NULL);
	wine_sigaction(SIGFPE, &segv_act, NULL);
#ifdef SIGBUS
	wine_sigaction(SIGBUS, &segv_act, NULL);
#endif
	wine_sigaction(SIGTRAP, &segv_act, NULL); /* For breakpoints */
#endif
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
#endif
}

#endif /* ifndef WINELIB */
