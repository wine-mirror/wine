#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <syscall.h>
#include <signal.h>
#include <errno.h>
#ifdef linux
#include <linux/sched.h>
#include <asm/system.h>
#endif
 
char * cstack[4096];
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

struct sigcontext_struct {
	unsigned short sc_gs, __gsh;
	unsigned short sc_fs, __fsh;
	unsigned short sc_es, __esh;
	unsigned short sc_ds, __dsh;
	unsigned long sc_edi;
	unsigned long sc_esi;
	unsigned long sc_ebp;
	unsigned long sc_esp;
	unsigned long sc_ebx;
	unsigned long sc_edx;
	unsigned long sc_ecx;
	unsigned long sc_eax;
	unsigned long sc_trapno;
	unsigned long sc_err;
	unsigned long sc_eip;
	unsigned short sc_cs, __csh;
	unsigned long sc_eflags;
	unsigned long esp_at_signal;
	unsigned short sc_ss, __ssh;
	unsigned long i387;
	unsigned long oldmask;
	unsigned long cr2;
};
#endif

#ifdef __NetBSD__
#define sigcontext_struct sigcontext
#define HZ 100
#endif

static void
GetTimeDate(int time_flag, struct sigcontext_struct * context)
{
    struct tm *now;
    time_t ltime;
    
    ltime = time(NULL);
    now = localtime(&ltime);
    if (time_flag)
    {
	 context->sc_ecx = (now->tm_hour << 8) | now->tm_min;
	 context->sc_edx = now->tm_sec << 8;
    }
    else
    {
	context->sc_ecx = now->tm_year + 1900;
	context->sc_edx = ((now->tm_mon + 1) << 8) | now->tm_mday;
	context->sc_eax &= 0xff00;
	context->sc_eax |= now->tm_wday;
    }
}

/* We handle all int21 calls here.  There is some duplicate code from
   misc/dos.c that I am unsure how to deal with, since the locations
   that we store the registers are all different */

static int
do_int21(struct sigcontext_struct * context){
	fprintf(stderr,"Doing int21 %x   ", (context->sc_eax >> 8) & 0xff);
	switch((context->sc_eax >> 8) & 0xff){
	case 0x30:
		context->sc_eax = 0x0303;  /* Hey folks, this is DOS V3.3! */
		context->sc_ebx = 0;
		context->sc_ecx = 0;
		break;
	
		/* Ignore any attempt to set a segment vector */
	case 0x25:
		return 1;

	case 0x35:  /* Return a NULL segment selector - this will bomb
		       if anyone ever tries to use it */
		context->sc_es = 0;
		context->sc_ebx = 0;
		break;
		
	case 0x2a:
		GetTimeDate(0, context);
	/* Function does not return */
		
	case 0x2c:
		GetTimeDate(1, context);
		/* Function does not return */

	case 0x4c:
		exit(context->sc_eax & 0xff);
		

	default:
		fprintf(stderr,"Unable to handle int 0x21 %x\n", context->sc_eax);
		return 1;
	};
	return 1;
}

static int
do_int1A(struct sigcontext_struct * context){
	time_t ltime;
	int ticks;

	switch((context->sc_eax >> 8) & 0xff){
	case 0:
		ltime = time(NULL);
		ticks = (int) (ltime * HZ);
		context->sc_ecx = ticks >> 16;
		context->sc_edx = ticks & 0x0000FFFF;
		context->sc_eax = 0;  /* No midnight rollover */
		break;
	
	default:
		fprintf(stderr,"Unable to handle int 0x1A %x\n", context->sc_eax);
		return 1;
	};
	return 1;
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
	instr = (char *) ((scp->sc_cs << 16) | (scp->sc_eip & 0xffff));

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
		(void (*)()) (((unsigned int)(cstack + sizeof(cstack) - 4)) & ~3);
	wine_sigaction(SIGSEGV, &segv_act, NULL);
#endif
#ifdef __NetBSD__
        struct sigstack ss;
        sigset_t sig_mask;
        
        ss.ss_sp = (char *) (((unsigned int)(cstack + sizeof(cstack) - 4)) & ~3);
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

