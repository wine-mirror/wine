/*
 * i386 signal handling routines
 * 
 * Copyright 1999 Alexandre Julliard
 */

#ifdef __i386__

#include "config.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYSCALL_H
# include <syscall.h>
#else
# ifdef HAVE_SYS_SYSCALL_H
#  include <sys/syscall.h>
# endif
#endif

/***********************************************************************
 * signal context platform-specific definitions
 */

#ifdef linux
typedef struct
{
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
} SIGCONTEXT;

#define HANDLER_DEF(name) void name( int __signal, SIGCONTEXT __context )
#define HANDLER_CONTEXT (&__context)

/* this is the sigaction structure from the Linux 2.1.20 kernel.  */
#undef sa_handler
struct kernel_sigaction
{
    void (*sa_handler)();
    unsigned long sa_mask;
    unsigned long sa_flags;
    void *sa_restorer;
};

/* Similar to the sigaction function in libc, except it leaves alone the
   restorer field, which is used to specify the signal stack address */
static inline int wine_sigaction( int sig, struct kernel_sigaction *new,
                                  struct kernel_sigaction *old )
{
    __asm__ __volatile__( "pushl %%ebx\n\t"
                          "movl %2,%%ebx\n\t"
                          "int $0x80\n\t"
                          "popl %%ebx"
                          : "=a" (sig)
                          : "0" (SYS_sigaction), "r" (sig), "c" (new), "d" (old) );
    if (sig>=0) return 0;
    errno = -sig;
    return -1;
}

#endif  /* linux */

#ifdef BSDI

#define EAX_sig(context)     ((context)->tf_eax)
#define EBX_sig(context)     ((context)->tf_ebx)
#define ECX_sig(context)     ((context)->tf_ecx)
#define EDX_sig(context)     ((context)->tf_edx)
#define ESI_sig(context)     ((context)->tf_esi)
#define EDI_sig(context)     ((context)->tf_edi)
#define EBP_sig(context)     ((context)->tf_ebp)
                            
#define CS_sig(context)      ((context)->tf_cs)
#define DS_sig(context)      ((context)->tf_ds)
#define ES_sig(context)      ((context)->tf_es)
#define SS_sig(context)      ((context)->tf_ss)

#include <machine/frame.h>
typedef struct trapframe SIGCONTEXT;

#define HANDLER_DEF(name) void name( int __signal, int code, SIGCONTEXT *__context )
#define HANDLER_CONTEXT __context

#define EFL_sig(context)     ((context)->tf_eflags)

#define EIP_sig(context)     (*((unsigned long*)&(context)->tf_eip))
#define ESP_sig(context)     (*((unsigned long*)&(context)->tf_esp))

#endif /* bsdi */

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)

#include <signal.h>
typedef struct sigcontext SIGCONTEXT;

#define HANDLER_DEF(name) void name( int __signal, int code, SIGCONTEXT *__context )
#define HANDLER_CONTEXT __context

#endif  /* FreeBSD */

#if defined(__svr4__) || defined(_SCO_DS) || defined(__sun)

#include <signal.h>
#ifdef _SCO_DS
#include <sys/regset.h>
#endif
/* Solaris kludge */
#undef ERR
#include <sys/ucontext.h>
#undef ERR
typedef struct ucontext SIGCONTEXT;

#define HANDLER_DEF(name) void name( int __signal, void *__siginfo, SIGCONTEXT *__context )
#define HANDLER_CONTEXT __context

#endif  /* svr4 || SCO_DS */

#ifdef __EMX__

typedef struct
{
    unsigned long ContextFlags;
    FLOATING_SAVE_AREA sc_float;
    unsigned long sc_gs;
    unsigned long sc_fs;
    unsigned long sc_es;
    unsigned long sc_ds;
    unsigned long sc_edi;
    unsigned long sc_esi;
    unsigned long sc_eax;
    unsigned long sc_ebx;
    unsigned long sc_ecx;
    unsigned long sc_edx;
    unsigned long sc_ebp;
    unsigned long sc_eip;
    unsigned long sc_cs;
    unsigned long sc_eflags;
    unsigned long sc_esp;
    unsigned long sc_ss;
} SIGCONTEXT;

#endif  /* __EMX__ */


#if defined(linux) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__EMX__)

#define EAX_sig(context)     ((context)->sc_eax)
#define EBX_sig(context)     ((context)->sc_ebx)
#define ECX_sig(context)     ((context)->sc_ecx)
#define EDX_sig(context)     ((context)->sc_edx)
#define ESI_sig(context)     ((context)->sc_esi)
#define EDI_sig(context)     ((context)->sc_edi)
#define EBP_sig(context)     ((context)->sc_ebp)
                            
#define CS_sig(context)      ((context)->sc_cs)
#define DS_sig(context)      ((context)->sc_ds)
#define ES_sig(context)      ((context)->sc_es)
#define SS_sig(context)      ((context)->sc_ss)
                            
#ifdef linux
/* FS and GS are now in the sigcontext struct of FreeBSD, but not 
 * saved by the exception handling. duh.
 */
#define FS_sig(context)      ((context)->sc_fs)
#define GS_sig(context)      ((context)->sc_gs)
#define CR2_sig(context)     ((context)->cr2)
#define TRAP_sig(context)    ((context)->sc_trapno)
#define ERROR_sig(context)   ((context)->sc_err)
#define FPU_sig(context)     ((FLOATING_SAVE_AREA*)((context)->i387))
#endif

#ifndef __FreeBSD__
#define EFL_sig(context)     ((context)->sc_eflags)
#else                       
#define EFL_sig(context)     ((context)->sc_efl)
/* FreeBSD, see i386/i386/traps.c::trap_pfault va->err kludge  */
#define CR2_sig(context)     ((context)->sc_err)
#define TRAP_sig(context)    ((context)->sc_trapno)
#endif                      

#define EIP_sig(context)     (*((unsigned long*)&(context)->sc_eip))
#define ESP_sig(context)     (*((unsigned long*)&(context)->sc_esp))

#endif  /* linux || __NetBSD__ || __FreeBSD__ || __OpenBSD__ */

#if defined(__svr4__) || defined(_SCO_DS) || defined(__sun)

#ifdef _SCO_DS
#define gregs regs
#endif

#define EAX_sig(context)     ((context)->uc_mcontext.gregs[EAX])
#define EBX_sig(context)     ((context)->uc_mcontext.gregs[EBX])
#define ECX_sig(context)     ((context)->uc_mcontext.gregs[ECX])
#define EDX_sig(context)     ((context)->uc_mcontext.gregs[EDX])
#define ESI_sig(context)     ((context)->uc_mcontext.gregs[ESI])
#define EDI_sig(context)     ((context)->uc_mcontext.gregs[EDI])
#define EBP_sig(context)     ((context)->uc_mcontext.gregs[EBP])
                            
#define CS_sig(context)      ((context)->uc_mcontext.gregs[CS])
#define DS_sig(context)      ((context)->uc_mcontext.gregs[DS])
#define ES_sig(context)      ((context)->uc_mcontext.gregs[ES])
#define SS_sig(context)      ((context)->uc_mcontext.gregs[SS])
                            
#define FS_sig(context)      ((context)->uc_mcontext.gregs[FS])
#define GS_sig(context)      ((context)->uc_mcontext.gregs[GS])

#define EFL_sig(context)     ((context)->uc_mcontext.gregs[EFL])
                            
#define EIP_sig(context)     ((context)->uc_mcontext.gregs[EIP])
#ifdef R_ESP
#define ESP_sig(context)     ((context)->uc_mcontext.gregs[R_ESP])
#else
#define ESP_sig(context)     ((context)->uc_mcontext.gregs[ESP])
#endif
#ifdef TRAPNO
#define TRAP_sig(context)     ((context)->uc_mcontext.gregs[TRAPNO])
#endif

#endif  /* svr4 || SCO_DS */


/* exception code definitions (already defined by FreeBSD) */
#ifndef __FreeBSD__  /* FIXME: other BSDs? */
#define T_DIVIDE        0   /* Division by zero exception */
#define T_TRCTRAP       1   /* Single-step exception */
#define T_NMI           2   /* NMI interrupt */
#define T_BPTFLT        3   /* Breakpoint exception */
#define T_OFLOW         4   /* Overflow exception */
#define T_BOUND         5   /* Bound range exception */
#define T_PRIVINFLT     6   /* Invalid opcode exception */
#define T_DNA           7   /* Device not available exception */
#define T_DOUBLEFLT     8   /* Double fault exception */
#define T_FPOPFLT       9   /* Coprocessor segment overrun */
#define T_TSSFLT        10  /* Invalid TSS exception */
#define T_SEGNPFLT      11  /* Segment not present exception */
#define T_STKFLT        12  /* Stack fault */
#define T_PROTFLT       13  /* General protection fault */
#define T_PAGEFLT       14  /* Page fault */
#define T_RESERVED      15  /* Unknown exception */
#define T_ARITHTRAP     16  /* Floating point exception */
#define T_ALIGNFLT      17  /* Alignment check exception */
#define T_MCHK          18  /* Machine check exception */
#define T_CACHEFLT      19  /* Cache flush exception */
#endif

#define T_UNKNOWN     (-1)  /* Unknown fault (TRAP_sig not defined) */

#include "wine/exception.h"
#include "winnt.h"
#include "stackframe.h"
#include "global.h"
#include "miscemu.h"
#include "ntddk.h"
#include "syslevel.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(seh)



/***********************************************************************
 *           handler_init
 *
 * Initialization code for a signal handler.
 * Restores the proper %fs value for the current thread.
 */
static inline void handler_init( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
    WORD fs;
    /* get %fs at time of the fault */
#ifdef FS_sig
    fs = FS_sig(sigcontext);
#else
    GET_FS(fs);
#endif
    context->SegFs = fs;
    /* now restore a proper %fs for the fault handler */
    if (!IS_SELECTOR_SYSTEM(CS_sig(sigcontext))) fs = SYSLEVEL_Win16CurrentTeb;
    if (!fs) fs = SYSLEVEL_EmergencyTeb;
    SET_FS(fs);
}

/***********************************************************************
 *           get_trap_code
 *
 * Get the trap code for a signal.
 */
static inline int get_trap_code( const SIGCONTEXT *sigcontext )
{
#ifdef TRAP_sig
    return TRAP_sig(sigcontext);
#else
    return T_UNKNOWN;  /* unknown trap code */
#endif
}

/***********************************************************************
 *           save_context
 *
 * Set the register values from a sigcontext. 
 */
static void save_context( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
    context->Eax    = EAX_sig(sigcontext);
    context->Ebx    = EBX_sig(sigcontext);
    context->Ecx    = ECX_sig(sigcontext);
    context->Edx    = EDX_sig(sigcontext);
    context->Esi    = ESI_sig(sigcontext);
    context->Edi    = EDI_sig(sigcontext);
    context->Ebp    = EBP_sig(sigcontext);
    context->EFlags = EFL_sig(sigcontext);
    context->Eip    = EIP_sig(sigcontext);
    context->Esp    = ESP_sig(sigcontext);
    context->SegCs  = LOWORD(CS_sig(sigcontext));
    context->SegDs  = LOWORD(DS_sig(sigcontext));
    context->SegEs  = LOWORD(ES_sig(sigcontext));
    context->SegSs  = LOWORD(SS_sig(sigcontext));
    /* %fs already handled in handler_init */
#ifdef GS_sig
    context->SegGs  = LOWORD(GS_sig(sigcontext));
#else
    GET_GS( context->SegGs );
    context->SegGs &= 0xffff;
#endif
    if (ISV86(context)) V86BASE(context) = (DWORD)DOSMEM_MemoryBase(0);
}


/***********************************************************************
 *           restore_context
 *
 * Build a sigcontext from the register values.
 */
static void restore_context( const CONTEXT *context, SIGCONTEXT *sigcontext )
{
    EAX_sig(sigcontext) = context->Eax;
    EBX_sig(sigcontext) = context->Ebx;
    ECX_sig(sigcontext) = context->Ecx;
    EDX_sig(sigcontext) = context->Edx;
    ESI_sig(sigcontext) = context->Esi;
    EDI_sig(sigcontext) = context->Edi;
    EBP_sig(sigcontext) = context->Ebp;
    EFL_sig(sigcontext) = context->EFlags;
    EIP_sig(sigcontext) = context->Eip;
    ESP_sig(sigcontext) = context->Esp;
    CS_sig(sigcontext)  = context->SegCs;
    DS_sig(sigcontext)  = context->SegDs;
    ES_sig(sigcontext)  = context->SegEs;
    SS_sig(sigcontext)  = context->SegSs;
#ifdef FS_sig
    FS_sig(sigcontext)  = context->SegFs;
#else
    SET_FS( context->SegFs );
#endif
#ifdef GS_sig
    GS_sig(sigcontext)  = context->SegGs;
#else
    SET_GS( context->SegGs );
#endif
}


/***********************************************************************
 *           save_fpu
 *
 * Set the FPU context from a sigcontext. 
 */
static void inline save_fpu( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
#ifdef FPU_sig
    if (FPU_sig(sigcontext))
    {
        context->FloatSave = *FPU_sig(sigcontext);
        return;
    }
#endif  /* FPU_sig */
#ifdef __GNUC__
    __asm__ __volatile__( "fnsave %0; fwait" : "=m" (context->FloatSave) );
#endif  /* __GNUC__ */
}


/***********************************************************************
 *           restore_fpu
 *
 * Restore the FPU context to a sigcontext. 
 */
static void inline restore_fpu( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
#ifdef FPU_sig
    if (FPU_sig(sigcontext))
    {
        *FPU_sig(sigcontext) = context->FloatSave;
        return;
    }
#endif  /* FPU_sig */
#ifdef __GNUC__
    /* avoid nested exceptions */
    context->FloatSave.StatusWord &= context->FloatSave.ControlWord | 0xffffff80;
    __asm__ __volatile__( "frstor %0; fwait" : : "m" (context->FloatSave) );
#endif  /* __GNUC__ */
}


/**********************************************************************
 *		get_fpu_code
 *
 * Get the FPU exception code from the FPU status.
 */
static inline DWORD get_fpu_code( const CONTEXT *context )
{
    DWORD status = context->FloatSave.StatusWord;

    if (status & 0x01)  /* IE */
    {
        if (status & 0x40)  /* SF */
            return EXCEPTION_FLT_STACK_CHECK;
        else
            return EXCEPTION_FLT_INVALID_OPERATION;
    }
    if (status & 0x02) return EXCEPTION_FLT_DENORMAL_OPERAND;  /* DE flag */
    if (status & 0x04) return EXCEPTION_FLT_DIVIDE_BY_ZERO;    /* ZE flag */
    if (status & 0x08) return EXCEPTION_FLT_OVERFLOW;          /* OE flag */
    if (status & 0x10) return EXCEPTION_FLT_UNDERFLOW;         /* UE flag */
    if (status & 0x20) return EXCEPTION_FLT_INEXACT_RESULT;    /* PE flag */
    return EXCEPTION_FLT_INVALID_OPERATION;  /* generic error */
}


/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV and related errors.
 */
static HANDLER_DEF(segv_handler)
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    handler_init( &context, HANDLER_CONTEXT );

#ifdef CR2_sig
    /* we want the page-fault case to be fast */
    if (get_trap_code(HANDLER_CONTEXT) == T_PAGEFLT)
        if (VIRTUAL_HandleFault( (LPVOID)CR2_sig(HANDLER_CONTEXT) )) return;
#endif

    save_context( &context, HANDLER_CONTEXT );
    rec.ExceptionRecord  = NULL;
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionAddress = (LPVOID)context.Eip;
    rec.NumberParameters = 0;

    switch(get_trap_code(HANDLER_CONTEXT))
    {
    case T_OFLOW:   /* Overflow exception */
        rec.ExceptionCode = EXCEPTION_INT_OVERFLOW;
        break;
    case T_BOUND:   /* Bound range exception */
        rec.ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
        break;
    case T_PRIVINFLT:   /* Invalid opcode exception */
        rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    case T_STKFLT:  /* Stack fault */
        rec.ExceptionCode = EXCEPTION_STACK_OVERFLOW;
        break;
    case T_SEGNPFLT:  /* Segment not present exception */
    case T_PROTFLT:   /* General protection fault */
    case T_UNKNOWN:   /* Unknown fault code */
        if (INSTR_EmulateInstruction( &context )) goto restore;
        rec.ExceptionCode = EXCEPTION_PRIV_INSTRUCTION;
        break;
    case T_PAGEFLT:  /* Page fault */
#ifdef CR2_sig
        rec.NumberParameters = 2;
#ifdef ERROR_sig
        rec.ExceptionInformation[0] = (ERROR_sig(HANDLER_CONTEXT) & 2) != 0;
#else
        rec.ExceptionInformation[0] = 0;
#endif /* ERROR_sig */
        rec.ExceptionInformation[1] = CR2_sig(HANDLER_CONTEXT);
#endif /* CR2_sig */
        rec.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
        break;
    case T_ALIGNFLT:  /* Alignment check exception */
        /* FIXME: pass through exception handler first? */
    	if (context.EFlags & 0x00040000)
        {
            /* Disable AC flag, return */
            context.EFlags &= ~0x00040000;
            goto restore;
 	}
        rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    default:
        ERR( "Got unexpected trap %d\n", get_trap_code(HANDLER_CONTEXT) );
        /* fall through */
    case T_NMI:       /* NMI interrupt */
    case T_DNA:       /* Device not available exception */
    case T_DOUBLEFLT: /* Double fault exception */
    case T_TSSFLT:    /* Invalid TSS exception */
    case T_RESERVED:  /* Unknown exception */
    case T_MCHK:      /* Machine check exception */
#ifdef T_CACHEFLT
    case T_CACHEFLT:  /* Cache flush exception */
#endif
        rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    }

    EXC_RtlRaiseException( &rec, &context );
 restore:
    restore_context( &context, HANDLER_CONTEXT );
}


/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static HANDLER_DEF(trap_handler)
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    handler_init( &context, HANDLER_CONTEXT );

    switch(get_trap_code(HANDLER_CONTEXT))
    {
    case T_TRCTRAP:  /* Single-step exception */
        rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
        break;
    case T_BPTFLT:   /* Breakpoint exception */
    default:
        rec.ExceptionCode = EXCEPTION_BREAKPOINT;
        break;
    }
    save_context( &context, HANDLER_CONTEXT );
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context.Eip;
    rec.NumberParameters = 0;
    EXC_RtlRaiseException( &rec, &context );
    restore_context( &context, HANDLER_CONTEXT );
}


/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static HANDLER_DEF(fpe_handler)
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    handler_init( &context, HANDLER_CONTEXT );

    save_fpu( &context, HANDLER_CONTEXT );

    switch(get_trap_code(HANDLER_CONTEXT))
    {
    case T_DIVIDE:   /* Division by zero exception */
        rec.ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
        break;
    case T_FPOPFLT:   /* Coprocessor segment overrun */
        rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    case T_ARITHTRAP:  /* Floating point exception */
    case T_UNKNOWN:    /* Unknown fault code */
        rec.ExceptionCode = get_fpu_code( &context );
        break;
    default:
        ERR( "Got unexpected trap %d\n", get_trap_code(HANDLER_CONTEXT) );
        rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    }
    save_context( &context, HANDLER_CONTEXT );
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context.Eip;
    rec.NumberParameters = 0;
    EXC_RtlRaiseException( &rec, &context );
    restore_context( &context, HANDLER_CONTEXT );
    restore_fpu( &context, HANDLER_CONTEXT );
}


/**********************************************************************
 *		int_handler
 *
 * Handler for SIGINT.
 */
static HANDLER_DEF(int_handler)
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    handler_init( &context, HANDLER_CONTEXT );
    save_context( &context, HANDLER_CONTEXT );
    rec.ExceptionCode    = CONTROL_C_EXIT;
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context.Eip;
    rec.NumberParameters = 0;
    EXC_RtlRaiseException( &rec, &context );
    restore_context( &context, HANDLER_CONTEXT );
}


/***********************************************************************
 *           set_handler
 *
 * Set a signal handler
 */
static int set_handler( int sig, void (*func)() )
{
#ifdef linux
    struct kernel_sigaction sig_act;
    sig_act.sa_handler = func;
    sig_act.sa_flags   = SA_RESTART | SA_NOMASK;
    sig_act.sa_mask    = 0;
    /* point to the top of the stack */
    sig_act.sa_restorer = (char *)NtCurrentTeb()->signal_stack + SIGNAL_STACK_SIZE;
    return wine_sigaction( sig, &sig_act, NULL );
#else  /* linux */
    struct sigaction sig_act;
    sig_act.sa_handler = func;
    sigemptyset( &sig_act.sa_mask );

# if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    sig_act.sa_flags = SA_ONSTACK;
# elif defined (__svr4__) || defined(_SCO_DS)
    sig_act.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART;
# elif defined(__EMX__)
    sig_act.sa_flags = 0; /* FIXME: EMX has only SA_ACK and SA_SYSV */
# else
    sig_act.sa_flags = 0;
# endif
    return sigaction( sig, &sig_act, NULL );
#endif  /* linux */
}


/**********************************************************************
 *		SIGNAL_Init
 */
BOOL SIGNAL_Init(void)
{
#ifdef HAVE_WORKING_SIGALTSTACK
    struct sigaltstack ss;
    if ((ss.ss_sp = NtCurrentTeb()->signal_stack))
    {
        ss.ss_size  = SIGNAL_STACK_SIZE;
        ss.ss_flags = 0;
        if (sigaltstack(&ss, NULL) < 0)
        {
            perror("sigaltstack");
            /* fall through on error and try it differently */
        }
    }
#endif  /* HAVE_SIGALTSTACK */
    
    /* ignore SIGPIPE so that WINSOCK can get a EPIPE error instead  */
    signal( SIGPIPE, SIG_IGN );
    /* automatic child reaping to avoid zombies */
    signal( SIGCHLD, SIG_IGN );

    if (set_handler( SIGINT,  (void (*)())int_handler ) == -1) goto error;
    if (set_handler( SIGFPE,  (void (*)())fpe_handler ) == -1) goto error;
    if (set_handler( SIGSEGV, (void (*)())segv_handler ) == -1) goto error;
    if (set_handler( SIGILL,  (void (*)())segv_handler ) == -1) goto error;
#ifdef SIGBUS
    if (set_handler( SIGBUS,  (void (*)())segv_handler ) == -1) goto error;
#endif
#ifdef SIGTRAP
    if (set_handler( SIGTRAP, (void (*)())trap_handler ) == -1) goto error;
#endif
    return TRUE;

 error:
    perror("sigaction");
    return FALSE;
}

#endif  /* __i386__ */
