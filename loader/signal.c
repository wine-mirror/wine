#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <syscall.h>
#include <signal.h>
#include <errno.h>
#include <linux/sched.h>
#include <asm/system.h>
 
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

char * cstack[4096];
struct sigaction segv_act;

struct sigcontext_struct {
	unsigned short gs, __gsh;
	unsigned short fs, __fsh;
	unsigned short es, __esh;
	unsigned short ds, __dsh;
	unsigned long edi;
	unsigned long esi;
	unsigned long ebp;
	unsigned long esp;
	unsigned long ebx;
	unsigned long edx;
	unsigned long ecx;
	unsigned long eax;
	unsigned long trapno;
	unsigned long err;
	unsigned long eip;
	unsigned short cs, __csh;
	unsigned long eflags;
	unsigned long esp_at_signal;
	unsigned short ss, __ssh;
	unsigned long i387;
	unsigned long oldmask;
	unsigned long cr2;
};

static void
GetTimeDate(int time_flag, struct sigcontext_struct * context)
{
    struct tm *now;
    time_t ltime;
    
    ltime = time(NULL);
    now = localtime(&ltime);
    if (time_flag)
    {
	 context->ecx = (now->tm_hour << 8) | now->tm_min;
	 context->edx = now->tm_sec << 8;
    }
    else
    {
	context->ecx = now->tm_year + 1900;
	context->edx = ((now->tm_mon + 1) << 8) | now->tm_mday;
	context->eax &= 0xff00;
	context->eax |= now->tm_wday;
    }
}

/* We handle all int21 calls here.  There is some duplicate code from
   misc/dos.c that I am unsure how to deal with, since the locations
   that we store the registers are all different */

static int
do_int21(struct sigcontext_struct * context){
	fprintf(stderr,"Doing int21 %x   ", (context->eax >> 8) & 0xff);
	switch((context->eax >> 8) & 0xff){
	case 0x30:
		context->eax = 0x0303;  /* Hey folks, this is DOS V3.3! */
		context->ebx = 0;
		context->ecx = 0;
		break;
	
		/* Ignore any attempt to set a segment vector */
	case 0x25:
		return 1;

	case 0x35:  /* Return a NULL segment selector - this will bomb
		       if anyone ever tries to use it */
		context->es = 0;
		context->ebx = 0;
		break;
		
	case 0x2a:
		GetTimeDate(0, context);
	/* Function does not return */
		
	case 0x2c:
		GetTimeDate(1, context);
		/* Function does not return */

	case 0x4c:
		exit(context->eax & 0xff);
		

	default:
		fprintf(stderr,"Unable to handle int 0x21 %x\n", context->eax);
		return 1;
	};
	return 1;
}

static int
do_int1A(struct sigcontext_struct * context){
	time_t ltime;
	int ticks;

	switch((context->eax >> 8) & 0xff){
	case 0:
		ltime = time(NULL);
		ticks = (int) (ltime * HZ);
		context->ecx = ticks >> 16;
		context->edx = ticks & 0x0000FFFF;
		context->eax = 0;  /* No midnight rollover */
		break;
	
	default:
		fprintf(stderr,"Unable to handle int 0x1A %x\n", context->eax);
		return 1;
	};
	return 1;
}

static void win_segfault(int signal, struct sigcontext_struct context){
	unsigned char * instr;
	unsigned char intno;
	unsigned int * dump;
	int i;

	/* First take care of a few preliminaries */
	if(signal != SIGSEGV) exit(1);
	if((context.cs & 7) != 7){
		fprintf(stderr,
			"Segmentation fault in Wine program (%x:%x)."
			"  Please debug\n",
			context.cs, context.eip);
		goto oops;
	};

	/*  Now take a look at the actual instruction where the program
	    bombed */
	instr = (char *) ((context.cs << 16) | (context.eip & 0xffff));

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
		if(!do_int21(&context)) goto oops;
		break;
	case 0x1A:
		if(!do_int1A(&context)) goto oops;
		break;
	default:
		fprintf(stderr,"Unexpected Windows interrupt %x\n", intno);
		goto oops;
	};

	/* OK, done handling the interrupt */

	context.eip += 2;  /* Bypass the int instruction */
	return;

 oops:
	fprintf(stderr,"In win_segfault %x:%x\n", context.cs, context.eip);
	fprintf(stderr,"Stack: %x:%x\n", context.ss, context.esp_at_signal);
	dump = (int*) &context;
	for(i=0; i<22; i++) 
	{
	    fprintf(stderr," %8.8x", *dump++);
	    if ((i % 8) == 7)
		fprintf(stderr,"\n");
	}
	fprintf(stderr,"\n");
	exit(1);
}

int
init_wine_signals(){
	segv_act.sa_handler = (__sighandler_t) win_segfault;
	/* Point to the top of the stack, minus 4 just in case, and make
	   it aligned  */
	segv_act.sa_restorer = 
		(void (*)()) (((unsigned int)(cstack + sizeof(cstack) - 4)) & ~3);
	wine_sigaction(SIGSEGV, &segv_act, NULL);
}

