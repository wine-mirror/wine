#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef __NetBSD__
#include <sys/syscall.h>
#else
#include <syscall.h>
#endif
#include <signal.h>
#include <errno.h>
#ifdef linux
#include <linux/sched.h>
#include <asm/system.h>
#endif
#include "wine.h"
#include "segmem.h"
 
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

#ifdef linux
static void win_fault(int signal, struct sigcontext_struct context){
        struct sigcontext_struct *scp = &context;
#else
static void win_fault(int signal, int code, struct sigcontext *scp){
#endif
	unsigned char * instr;
	unsigned char intno;
	unsigned int * dump;
	int i;

	/* First take care of a few preliminaries */
#ifdef linux
	if(signal != SIGSEGV) exit(1);
	if((scp->sc_cs & 7) != 7){
#endif
#ifdef __NetBSD__
/*         set_es(0x27); set_ds(0x27); */
	if(signal != SIGBUS) exit(1);
	if(scp->sc_cs == 0x1f){
#endif
		fprintf(stderr,
			"Segmentation fault in Wine program (%x:%x)."
			"  Please debug\n",
			scp->sc_cs, scp->sc_eip);
		goto oops;
	};

	/*  Now take a look at the actual instruction where the program
	    bombed */
	instr = (char *) SAFEMAKEPTR(scp->sc_cs, scp->sc_eip);

	if(*instr != 0xcd) {
		fprintf(stderr,
			"Unexpected Windows program segfault"
			" - opcode = %x\n", *instr);
#if 0
		return;
#else
		goto oops;
#endif
	};

	instr++;
	intno = *instr;
	switch(intno){
	case 0x21:
		if(!do_int21(scp)) goto oops;
		break;
        case 0x11:  
  		scp->sc_eax = (scp->sc_eax & 0xffff0000L) | DOS_GetEquipment();
		break;
        case 0x12:               
          	scp->sc_eax = (scp->sc_eax & 0xffff0000L) | 640L; 
          	break;				/* get base mem size */                
	case 0x1A:
		if(!do_int1A(scp)) goto oops;
		break;
	default:
		fprintf(stderr,"Unexpected Windows interrupt %x\n", intno);
		goto oops;
	};

	/* OK, done handling the interrupt */

	scp->sc_eip += 2;  /* Bypass the int instruction */
	return;
 oops:
	fprintf(stderr,"In win_fault %x:%x\n", scp->sc_cs, scp->sc_eip);
#ifdef linux
	wine_debug(scp);  /* Enter our debugger */
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

int
init_wine_signals(){
#ifdef linux
	segv_act.sa_handler = (__sighandler_t) win_fault;
	/* Point to the top of the stack, minus 4 just in case, and make
	   it aligned  */
	segv_act.sa_restorer = 
		(void (*)()) (((unsigned int)(cstack) + sizeof(cstack) - 4) & ~3);
	wine_sigaction(SIGSEGV, &segv_act, NULL);
#endif
#ifdef __NetBSD__
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

