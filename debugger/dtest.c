#include <signal.h>
#include <stdio.h>

extern void wine_debug(unsigned int*);


#ifdef linux
#include <linux/sched.h>
#include <asm/system.h>
#endif

struct sigaction segv_act;

#ifdef linux

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

#ifdef linux
static void win_fault(int signal, struct sigcontext_struct context){
        struct sigcontext_struct *scp = &context;
#else
static void win_fault(int signal, int code, struct sigcontext *scp){
#endif

	wine_debug((unsigned int *) scp);  /* Enter our debugger */
}

char realtext[] = "This is what should really be printed\n";

int 
main(){
	char * pnt;
#ifdef linux
	segv_act.sa_handler = (__sighandler_t) win_fault;
	sigaction(SIGSEGV, &segv_act, NULL);
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
        sigset_t sig_mask;
        
        sigemptyset(&sig_mask);
        segv_act.sa_handler = (__sighandler_t) win_fault;
	segv_act.sa_flags = 0;
        segv_act.sa_mask = sig_mask;
	if (sigaction(SIGSEGV, &segv_act, NULL) < 0) {
                perror("sigaction");
                exit(1);
        }
#endif

	fprintf(stderr,"%x\n", realtext);

	/* Now force a segmentation fault */
	pnt = (char *) 0xc0000000;

	fprintf(stderr,"%s", pnt);
	return 0;

}


unsigned int * wine_files = NULL;

GetEntryPointFromOrdinal(int wpnt, int ordinal) {}
