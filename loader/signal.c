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
#include "dos_fs.h"
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

int do_int(int intnum, struct sigcontext_struct *context)
{
	switch(intnum)
	{
	      case 0x10: return do_int10(context);

	      case 0x11:  
		AX = DOS_GetEquipment();
		return 1;

	      case 0x12:               
		AX = 640;
		return 1;	/* get base mem size */                

              case 0x13: return do_int13(context);
	      case 0x15: return do_int15(context);
	      case 0x16: return do_int16(context);
	      case 0x1a: return do_int1a(context);
	      case 0x21: return do_int21(context);

	      case 0x22:
		AX = 0x1234;
		BX = 0x5678;
		CX = 0x9abc;
		DX = 0xdef0;
		return 1;

              case 0x25: return do_int25(context);
              case 0x26: return do_int26(context);
              case 0x2a: return do_int2a(context);
	      case 0x2f: return do_int2f(context);
	      case 0x31: return do_int31(context);
	      case 0x5c: return do_int5c(context);

              default:
                fprintf(stderr,"int%02x: Unimplemented!\n", intnum);
                break;
	}
	return 0;
}

#ifdef linux
static void win_fault(int signal, struct sigcontext_struct context_struct)
{
    struct sigcontext_struct *context = &context_struct;
#else
static void win_fault(int signal, int code, struct sigcontext *context)
{
#endif
    unsigned char * instr;
    WORD *stack;
#if !(defined (linux) || defined (__NetBSD__))
    int i, *dump;
#endif

	/* First take care of a few preliminaries */
#ifdef linux
    if(signal != SIGSEGV 
       && signal != SIGILL 
       && signal != SIGFPE 
#ifdef SIGBUS
       && signal != SIGBUS 
#endif
       && signal != SIGTRAP) 
    {
	exit(1);
    }

    /* And back up over the int3 instruction. */
    if(signal == SIGTRAP) {
      EIP--;
      goto oops;
    }
#endif
#ifdef __NetBSD__
/*         set_es(0x1f); set_ds(0x1f); */
    if(signal != SIGBUS && signal != SIGSEGV && signal != SIGTRAP) 
	exit(1);
#endif
#ifdef __FreeBSD__
/*         set_es(0x27); set_ds(0x27); */
    if(signal != SIGBUS && signal != SIGSEGV && signal != SIGTRAP) 
	exit(1);
#endif
    if (CS == WINE_CODE_SELECTOR)
    {
	fprintf(stderr,
		"Segmentation fault in Wine program (%x:%lx)."
		"  Please debug\n", CS, EIP );
	goto oops;
    }

    /*  Now take a look at the actual instruction where the program
	bombed */
    instr = (unsigned char *) PTR_SEG_OFF_TO_LIN( CS, EIP );

    switch(*instr)
    {
      case 0xcd: /* int <XX> */
            instr++;
	    if (!do_int(*instr, context)) {
		fprintf(stderr,"Unexpected Windows interrupt %x\n", *instr);
		goto oops;
	    }
	    EIP += 2;  /* Bypass the int instruction */
            break;

      case 0xcf: /* iret */
            stack = (WORD *)PTR_SEG_OFF_TO_LIN( SS, SP );
            EIP = *stack++;
            CS  = *stack++;
            EFL = *stack;
            SP += 6;  /* Pop the return address and flags */
            break;

      case 0xe4: /* inb al,XX */
            inportb_abs(context);
	    EIP += 2;
            break;

      case 0xe5: /* in ax,XX */
            inport_abs(context);
	    EIP += 2;
            break;

      case 0xe6: /* outb XX,al */
            outportb_abs(context);
	    EIP += 2;
            break;

      case 0xe7: /* out XX,ax */
            outport_abs(context);
	    EIP += 2;
            break;

      case 0xec: /* inb al,dx */
            inportb(context);
	    EIP++;
            break;

      case 0xed: /* in ax,dx */
            inport(context);
	    EIP++;  
            break;

      case 0xee: /* outb dx,al */
            outportb(context);
	    EIP++;
            break;
      
      case 0xef: /* out dx,ax */
            outport(context);
	    EIP++;
            break;

      case 0xfa: /* cli, ignored */
	    EIP++;
            break;

      case 0xfb: /* sti, ignored */
	    EIP++;
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
