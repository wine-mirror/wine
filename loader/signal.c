#ifndef WINELIB
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/syscall.h>
#else
#include <syscall.h>
#endif
#ifdef linux
#include <linux/sched.h>
#include <asm/system.h>
#endif

#include "wine.h"
#include "segmem.h"
#include "prototypes.h"
#include "win.h"
 
char * cstack[4096];
struct sigaction segv_act;

#ifdef linux
extern void ___sig_restore();
extern void ___masksig_restore();
#endif

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

int do_int(int intnum, struct sigcontext_struct *scp)
{
	switch(intnum)
	{
	      case 0x10: return do_int10(scp);

	      case 0x11:  
		scp->sc_eax = (scp->sc_eax & 0xffff0000L) | DOS_GetEquipment();
		return 1;

	      case 0x12:               
		scp->sc_eax = (scp->sc_eax & 0xffff0000L) | 640L; 
		return 1;	/* get base mem size */                

	      case 0x15: return do_int15(scp);
	      case 0x16: return do_int16(scp);
	      case 0x1A: return do_int1A(scp);
	      case 0x21: return do_int21(scp);

	      case 0x22:
		scp->sc_eax = 0x1234;
		scp->sc_ebx = 0x5678;
		scp->sc_ecx = 0x9abc;
		scp->sc_edx = 0xdef0;
		return 1;

	      case 0x25: return do_int25(scp);
	      case 0x26: return do_int26(scp);
	      case 0x2f: return do_int2f(scp);
	      case 0x31: return do_int31(scp);
	}
	return 0;
}

#ifdef linux
static void win_fault(int signal, struct sigcontext_struct context)
{
    struct sigcontext_struct *scp = &context;
#else
static void win_fault(int signal, int code, struct sigcontext *scp)
{
#endif
    unsigned char * instr;
    unsigned int * dump;
    int i;

	/* First take care of a few preliminaries */
#ifdef linux
    if(signal != SIGSEGV && signal != SIGTRAP) 
	exit(1);

    /* And back up over the int3 instruction. */
    if(signal == SIGTRAP) {
      scp->sc_eip--;
      goto oops;
    };

    if((scp->sc_cs & 7) != 7)
    {
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
/*         set_es(0x27); set_ds(0x27); */
    if(signal != SIGBUS) 
	exit(1);
    if(scp->sc_cs == 0x1f)
    {
#endif
	fprintf(stderr,
		"Segmentation fault in Wine program (%x:%x)."
		"  Please debug\n",
		scp->sc_cs, scp->sc_eip);
	goto oops;
    };

    /*  Now take a look at the actual instruction where the program
	bombed */
    instr = (unsigned char *) SAFEMAKEPTR(scp->sc_cs, scp->sc_eip);

    switch(*instr)
    {
      case 0xcd: /* int <XX> */
            instr++;
	    if (!do_int(*instr, scp)) {
		fprintf(stderr,"Unexpected Windows interrupt %x\n", *instr);
		goto oops;
	    }
	    scp->sc_eip += 2;  /* Bypass the int instruction */
            break;
            
      case 0xec: /* inb al,dx */
            inportb(scp);
	    scp->sc_eip++;
            break;

      case 0xed: /* in ax,dx */
            inport(scp);
	    scp->sc_eip++;  
            break;

      case 0xee: /* outb dx,al */
            outportb(scp);
	    scp->sc_eip++;
            break;
      
      case 0xef: /* out dx,ax */
            outport(scp);
	    scp->sc_eip++;
            break;

      default:
		fprintf(stderr, "Unexpected Windows program segfault"
			" - opcode = %x\n", *instr);
		goto oops;
    }
    
    /* OK, done handling the interrupt */

    return;

  oops:
    XUngrabPointer(display, CurrentTime);
	XUngrabServer(display);
	XFlush(display);
    fprintf(stderr,"In win_fault %x:%x\n", scp->sc_cs, scp->sc_eip);
#ifdef linux
    wine_debug(signal, scp);  /* Enter our debugger */
#else
    fprintf(stderr,"Stack: %x:%x\n", scp->sc_ss, scp->sc_esp);
    dump = (int*) scp;
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

int init_wine_signals(void)
{
#ifdef linux
	segv_act.sa_handler = (__sighandler_t) win_fault;
	/* Point to the top of the stack, minus 4 just in case, and make
	   it aligned  */
	segv_act.sa_restorer = 
		(void (*)()) (((unsigned int)(cstack) + sizeof(cstack) - 4) & ~3);
	wine_sigaction(SIGSEGV, &segv_act, NULL);
	wine_sigaction(SIGTRAP, &segv_act, NULL); /* For breakpoints */
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
        struct sigstack ss;
        sigset_t sig_mask;
        
        ss.ss_sp = (char *) (((unsigned int)(cstack) + sizeof(cstack) - 4) & ~3);
        ss.ss_onstack = 0;
        if (sigstack(&ss, NULL) < 0) {
                perror("sigstack");
                exit(1);
        }
        sigemptyset(&sig_mask);
        segv_act.sa_handler = (__sighandler_t) win_fault;
	segv_act.sa_flags = SA_ONSTACK;
        segv_act.sa_mask = sig_mask;
	if (sigaction(SIGBUS, &segv_act, NULL) < 0) {
                perror("sigaction");
                exit(1);
        }
#endif
}

#endif /* ifndef WINELIB */
