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
#ifdef linux
#define inline __inline__  /* So we can compile with -ansi */
#include <linux/sched.h>
#include <asm/system.h>
#undef inline
#endif

#include "wine.h"
#include "dos_fs.h"
#include "segmem.h"
#include "prototypes.h"
#include "miscemu.h"
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

              case 0x13: return do_int13(scp);
	      case 0x15: return do_int15(scp);
	      case 0x16: return do_int16(scp);
	      case 0x1a: return do_int1a(scp);
	      case 0x21: return do_int21(scp);

	      case 0x22:
		scp->sc_eax = 0x1234;
		scp->sc_ebx = 0x5678;
		scp->sc_ecx = 0x9abc;
		scp->sc_edx = 0xdef0;
		return 1;

	      case 0x25: return do_int25(scp);
	      case 0x26: return do_int26(scp);
              case 0x2a: return do_int2a(scp);
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
#if !(defined (linux) || defined (__NetBSD__))
	int i, *dump;
#endif

	/* First take care of a few preliminaries */
#ifdef linux
    if(signal != SIGSEGV 
       && signal != SIGILL 
#ifdef SIGBUS
       && signal != SIGBUS 
#endif
       && signal != SIGTRAP) 
    {
	exit(1);
    }

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
    if(signal != SIGBUS && signal != SIGSEGV && signal != SIGTRAP) 
	exit(1);
    if(scp->sc_cs == 0x1f)
    {
#endif
	fprintf(stderr,
		"Segmentation fault in Wine program (%x:%lx)."
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
            
      case 0xe4: /* inb al,XX */
            inportb_abs(scp);
	    scp->sc_eip += 2;
            break;

      case 0xe5: /* in ax,XX */
            inport_abs(scp);
	    scp->sc_eip += 2;
            break;

      case 0xe6: /* outb XX,al */
            outportb_abs(scp);
	    scp->sc_eip += 2;
            break;

      case 0xe7: /* out XX,ax */
            outport_abs(scp);
	    scp->sc_eip += 2;
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

      case 0xfa: /* cli, ignored */
	    scp->sc_eip++;
            break;

      case 0xfb: /* sti, ignored */
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
    fprintf(stderr,"In win_fault %x:%lx\n", scp->sc_cs, scp->sc_eip);
#if defined(linux) || defined(__NetBSD__) || defined(__FreeBSD__)
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
	wine_sigaction(SIGILL, &segv_act, NULL);
#ifdef SIGBUS
	wine_sigaction(SIGBUS, &segv_act, NULL);
#endif
	wine_sigaction(SIGTRAP, &segv_act, NULL); /* For breakpoints */
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
        sigset_t sig_mask;
#if defined(BSD4_4) && !defined (__FreeBSD__)
        struct sigaltstack ss;
        
        if ((ss.ss_base = malloc(MINSIGSTKSZ)) == NULL) {
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
#else
        struct sigstack ss;
        
        ss.ss_sp = (char *) (((unsigned int)(cstack) + sizeof(cstack) - 4) & ~3);
        ss.ss_onstack = 0;
        if (sigstack(&ss, NULL) < 0) {
                perror("sigstack");
                exit(1);
        }
#endif
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

static sigjmp_buf segv_jmpbuf;

static void
segv_handler()
{
    siglongjmp(segv_jmpbuf, 1);
}

int
test_memory( char *p, int write )
{
    int ret = FALSE;
    struct sigaction new_act;
    struct sigaction old_act;

    memset(&new_act, 0, sizeof new_act);
    new_act.sa_handler = segv_handler;
    if (sigsetjmp( segv_jmpbuf, 1 ) == 0) {
	char c = 100;
	if (sigaction(SIGSEGV, &new_act, &old_act) < 0)
	    perror("sigaction");
	c = *p;
	if (write)
	    *p = c;
	ret = TRUE;
    }
#ifdef linux
    wine_sigaction(SIGSEGV, &old_act, NULL);
#else
    sigaction(SIGSEGV, &old_act, NULL);
#endif
    return ret;
}

#endif /* ifndef WINELIB */
