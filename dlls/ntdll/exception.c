/*
 * NT exception handling routines
 * 
 * Copyright 1999 Turchanov Sergey
 * Copyright 1999 Alexandre Julliard
 */

#include <signal.h>
#include "winnt.h"
#include "ntddk.h"
#include "process.h"
#include "global.h"
#include "wine/exception.h"
#include "stackframe.h"
#include "sig_context.h"
#include "miscemu.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(seh)

/* Exception record for handling exceptions happening inside exception handlers */
typedef struct
{
    EXCEPTION_FRAME frame;
    EXCEPTION_FRAME *prevFrame;
} EXC_NESTED_FRAME;

#ifdef __i386__
# define GET_IP(context) ((LPVOID)EIP_reg(context))
#else
# define GET_IP(context) ((LPVOID)0)
#endif  /* __i386__ */

/* Default hook for built-in debugger */
static DWORD default_hook( EXCEPTION_RECORD *rec, CONTEXT *ctx, BOOL first )
{
    if (!first)
    {
    	DPRINTF( "stopping process due to unhandled exception %08lx.\n",
                 rec->ExceptionCode );
	raise( SIGSTOP );
    }
    return 0;  /* not handled */
}
static DEBUGHOOK debug_hook = default_hook;

/*******************************************************************
 *         EXC_SetDebugEventHook
 *         EXC_GetDebugEventHook
 *
 * Set/Get the hook for the built-in debugger.
 *
 * FIXME: the built-in debugger should use the normal debug events.
 */
void EXC_SetDebugEventHook( DEBUGHOOK hook )
{
    debug_hook = hook;
}
DEBUGHOOK EXC_GetDebugEventHook(void)
{
    return debug_hook;
}

/*******************************************************************
 *         EXC_RaiseHandler
 *
 * Handler for exceptions happening inside a handler.
 */
static DWORD CALLBACK EXC_RaiseHandler( EXCEPTION_RECORD *rec, EXCEPTION_FRAME *frame,
                                        CONTEXT *context, EXCEPTION_FRAME **dispatcher )
{
    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
        return ExceptionContinueSearch;
    /* We shouldn't get here so we store faulty frame in dispatcher */
    *dispatcher = ((EXC_NESTED_FRAME*)frame)->prevFrame;
    return ExceptionNestedException;
}


/*******************************************************************
 *         EXC_UnwindHandler
 *
 * Handler for exceptions happening inside an unwind handler.
 */
static DWORD CALLBACK EXC_UnwindHandler( EXCEPTION_RECORD *rec, EXCEPTION_FRAME *frame,
                                         CONTEXT *context, EXCEPTION_FRAME **dispatcher )
{
    if (!(rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND)))
        return ExceptionContinueSearch;
    /* We shouldn't get here so we store faulty frame in dispatcher */
    *dispatcher = ((EXC_NESTED_FRAME*)frame)->prevFrame;
    return ExceptionCollidedUnwind;
}


/*******************************************************************
 *         EXC_CallHandler
 *
 * Call an exception handler, setting up an exception frame to catch exceptions
 * happening during the handler execution.
 */
static DWORD EXC_CallHandler( PEXCEPTION_HANDLER handler, PEXCEPTION_HANDLER nested_handler,
                              EXCEPTION_RECORD *record, EXCEPTION_FRAME *frame,
                              CONTEXT *context, EXCEPTION_FRAME **dispatcher )
{
    EXC_NESTED_FRAME newframe;
    DWORD ret;

    newframe.frame.Handler = nested_handler;
    newframe.prevFrame     = frame;
    EXC_push_frame( &newframe.frame );
    TRACE( "calling handler at %p code=%lx flags=%lx\n",
           handler, record->ExceptionCode, record->ExceptionFlags );
    ret = handler( record, frame, context, dispatcher );
    TRACE( "handler returned %lx\n", ret );
    EXC_pop_frame( &newframe.frame );
    return ret;
}


/*******************************************************************
 *         EXC_DefaultHandling
 *
 * Default handling for exceptions. Called when we didn't find a suitable handler.
 */
static void EXC_DefaultHandling( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    if ((PROCESS_Current()->flags & PDB32_DEBUGGED) &&
        (DEBUG_SendExceptionEvent( rec, FALSE ) == DBG_CONTINUE))
        return;  /* continue execution */

    if (debug_hook( rec, context, FALSE ) == DBG_CONTINUE)
        return;  /* continue execution */

    if (rec->ExceptionFlags & EH_STACK_INVALID)
        ERR("Exception frame is not in stack limits => unable to dispatch exception.\n");
    else if (rec->ExceptionCode == EXCEPTION_NONCONTINUABLE_EXCEPTION)
        ERR("Process attempted to continue execution after noncontinuable exception.\n");
    else
        ERR("Unhandled exception code %lx flags %lx addr %p\n",
            rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );
    TerminateProcess( GetCurrentProcess(), 1 );
}


/***********************************************************************
 *            RtlRaiseException  (NTDLL.464)
 */
void WINAPI REGS_FUNC(RtlRaiseException)( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    PEXCEPTION_FRAME frame, dispatch, nested_frame;
    EXCEPTION_RECORD newrec;
    DWORD res;

    TRACE( "code=%lx flags=%lx\n", rec->ExceptionCode, rec->ExceptionFlags );

    if ((PROCESS_Current()->flags & PDB32_DEBUGGED) &&
        (DEBUG_SendExceptionEvent( rec, TRUE ) == DBG_CONTINUE))
        return;  /* continue execution */

    if (debug_hook( rec, context, TRUE ) == DBG_CONTINUE)
        return;  /* continue execution */

    frame = NtCurrentTeb()->except;
    nested_frame = NULL;
    while (frame != (PEXCEPTION_FRAME)0xFFFFFFFF)
    {
        /* Check frame address */
        if (((void*)frame < NtCurrentTeb()->stack_low) ||
            ((void*)(frame+1) > NtCurrentTeb()->stack_top) ||
            (int)frame & 3)
        {
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        /* Call handler */
        res = EXC_CallHandler( frame->Handler, EXC_RaiseHandler, rec, frame, context, &dispatch );
        if (frame == nested_frame)
        {
            /* no longer nested */
            nested_frame = NULL;
            rec->ExceptionFlags &= ~EH_NESTED_CALL;
        }

        switch(res)
        {
        case ExceptionContinueExecution:
            if (!(rec->ExceptionFlags & EH_NONCONTINUABLE)) return;
            newrec.ExceptionCode    = STATUS_NONCONTINUABLE_EXCEPTION;
            newrec.ExceptionFlags   = EH_NONCONTINUABLE;
            newrec.ExceptionRecord  = rec;
            newrec.NumberParameters = 0;
            RtlRaiseException( &newrec );  /* never returns */
            break;
        case ExceptionContinueSearch:
            break;
        case ExceptionNestedException:
            if (nested_frame < dispatch) nested_frame = dispatch;
            rec->ExceptionFlags |= EH_NESTED_CALL;
            break;
        default:
            newrec.ExceptionCode    = STATUS_INVALID_DISPOSITION;
            newrec.ExceptionFlags   = EH_NONCONTINUABLE;
            newrec.ExceptionRecord  = rec;
            newrec.NumberParameters = 0;
            RtlRaiseException( &newrec );  /* never returns */
            break;
        }
        frame = frame->Prev;
    }
    EXC_DefaultHandling( rec, context );
}


/*******************************************************************
 *         RtlUnwind  (KERNEL32.590) (NTDLL.518)
 */
void WINAPI REGS_FUNC(RtlUnwind)( PEXCEPTION_FRAME pEndFrame, LPVOID unusedEip, 
                                  PEXCEPTION_RECORD pRecord, DWORD returnEax,
                                  CONTEXT *context )
{
    EXCEPTION_RECORD record, newrec;
    PEXCEPTION_FRAME frame, dispatch;

#ifdef __i386__
    EAX_reg(context) = returnEax;
#endif

    /* build an exception record, if we do not have one */
    if (!pRecord)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = GET_IP(context); 
        record.NumberParameters = 0;
        pRecord = &record;
    }

    pRecord->ExceptionFlags |= EH_UNWINDING | (pEndFrame ? 0 : EH_EXIT_UNWIND);

    TRACE( "code=%lx flags=%lx\n", pRecord->ExceptionCode, pRecord->ExceptionFlags );

    /* get chain of exception frames */
    frame = NtCurrentTeb()->except;
    while ((frame != (PEXCEPTION_FRAME)0xffffffff) && (frame != pEndFrame))
    {
        /* Check frame address */
        if (pEndFrame && (frame > pEndFrame))
        {
            newrec.ExceptionCode    = STATUS_INVALID_UNWIND_TARGET;
            newrec.ExceptionFlags   = EH_NONCONTINUABLE;
            newrec.ExceptionRecord  = pRecord;
            newrec.NumberParameters = 0;
            RtlRaiseException( &newrec );  /* never returns */
        }
        if (((void*)frame < NtCurrentTeb()->stack_low) ||
            ((void*)(frame+1) > NtCurrentTeb()->stack_top) ||
            (int)frame & 3)
        {
            newrec.ExceptionCode    = STATUS_BAD_STACK;
            newrec.ExceptionFlags   = EH_NONCONTINUABLE;
            newrec.ExceptionRecord  = pRecord;
            newrec.NumberParameters = 0;
            RtlRaiseException( &newrec );  /* never returns */
        }

        /* Call handler */
        switch(EXC_CallHandler( frame->Handler, EXC_UnwindHandler, pRecord,
                                frame, context, &dispatch ))
        {
        case ExceptionContinueSearch:
            break;
        case ExceptionCollidedUnwind:
            frame = dispatch;
            break;
        default:
            newrec.ExceptionCode    = STATUS_INVALID_DISPOSITION;
            newrec.ExceptionFlags   = EH_NONCONTINUABLE;
            newrec.ExceptionRecord  = pRecord;
            newrec.NumberParameters = 0;
            RtlRaiseException( &newrec );  /* never returns */
            break;
        }
        frame = EXC_pop_frame( frame );
    }
}


/*******************************************************************
 *         NtRaiseException    (NTDLL.175)
 *
 * Real prototype:
 *    DWORD WINAPI NtRaiseException( EXCEPTION_RECORD *rec, CONTEXT *ctx, BOOL first );
 */
void WINAPI REGS_FUNC(NtRaiseException)( EXCEPTION_RECORD *rec, CONTEXT *ctx,
                                         BOOL first, CONTEXT *context )
{
    REGS_FUNC(RtlRaiseException)( rec, ctx );
    *context = *ctx;
}


/***********************************************************************
 *            RtlRaiseStatus  (NTDLL.465)
 *
 * Raise an exception with ExceptionCode = status
 */
void WINAPI RtlRaiseStatus( NTSTATUS status )
{
    EXCEPTION_RECORD ExceptionRec;

    ExceptionRec.ExceptionCode    = status;
    ExceptionRec.ExceptionFlags   = EH_NONCONTINUABLE;
    ExceptionRec.ExceptionRecord  = NULL;
    ExceptionRec.NumberParameters = 0;
    RtlRaiseException( &ExceptionRec );
}


/***********************************************************************
 *           DebugBreak   (KERNEL32.181)
 */
void WINAPI REGS_FUNC(DebugBreak)( CONTEXT *context )
{
    EXCEPTION_RECORD rec;

    rec.ExceptionCode    = EXCEPTION_BREAKPOINT;
    rec.ExceptionFlags   = EH_NONCONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.NumberParameters = 0;
    REGS_FUNC(RtlRaiseException)( &rec, context );
}


/***********************************************************************
 *           DebugBreak16   (KERNEL.203)
 */
void WINAPI DebugBreak16( CONTEXT86 *context )
{
#ifdef __i386__
    REGS_FUNC(DebugBreak)( context );
#endif  /* defined(__i386__) */
}


/***********************************************************************
 *           EXC_SaveContext
 *
 * Set the register values from a sigcontext. 
 */
static void EXC_SaveContext( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
#ifdef __i386__
    EAX_reg(context) = EAX_sig(sigcontext);
    EBX_reg(context) = EBX_sig(sigcontext);
    ECX_reg(context) = ECX_sig(sigcontext);
    EDX_reg(context) = EDX_sig(sigcontext);
    ESI_reg(context) = ESI_sig(sigcontext);
    EDI_reg(context) = EDI_sig(sigcontext);
    EBP_reg(context) = EBP_sig(sigcontext);
    EFL_reg(context) = EFL_sig(sigcontext);
    EIP_reg(context) = EIP_sig(sigcontext);
    ESP_reg(context) = ESP_sig(sigcontext);
    CS_reg(context)  = LOWORD(CS_sig(sigcontext));
    DS_reg(context)  = LOWORD(DS_sig(sigcontext));
    ES_reg(context)  = LOWORD(ES_sig(sigcontext));
    SS_reg(context)  = LOWORD(SS_sig(sigcontext));
#ifdef FS_sig
    FS_reg(context)  = LOWORD(FS_sig(sigcontext));
#else
    GET_FS( FS_reg(context) );
    FS_reg(context) &= 0xffff;
#endif
#ifdef GS_sig
    GS_reg(context)  = LOWORD(GS_sig(sigcontext));
#else
    GET_GS( GS_reg(context) );
    GS_reg(context) &= 0xffff;
#endif
    if (ISV86(context)) V86BASE(context) = (DWORD)DOSMEM_MemoryBase(0);
#endif  /* __i386__ */
}


/***********************************************************************
 *           EXC_RestoreContext
 *
 * Build a sigcontext from the register values.
 */
static void EXC_RestoreContext( const CONTEXT *context, SIGCONTEXT *sigcontext )
{
#ifdef __i386__
    EAX_sig(sigcontext) = EAX_reg(context);
    EBX_sig(sigcontext) = EBX_reg(context);
    ECX_sig(sigcontext) = ECX_reg(context);
    EDX_sig(sigcontext) = EDX_reg(context);
    ESI_sig(sigcontext) = ESI_reg(context);
    EDI_sig(sigcontext) = EDI_reg(context);
    EBP_sig(sigcontext) = EBP_reg(context);
    EFL_sig(sigcontext) = EFL_reg(context);
    EIP_sig(sigcontext) = EIP_reg(context);
    ESP_sig(sigcontext) = ESP_reg(context);
    CS_sig(sigcontext)  = CS_reg(context);
    DS_sig(sigcontext)  = DS_reg(context);
    ES_sig(sigcontext)  = ES_reg(context);
    SS_sig(sigcontext)  = SS_reg(context);
#ifdef FS_sig
    FS_sig(sigcontext)  = FS_reg(context);
#else
    SET_FS( FS_reg(context) );
#endif
#ifdef GS_sig
    GS_sig(sigcontext)  = GS_reg(context);
#else
    SET_GS( GS_reg(context) );
#endif
#endif  /* __i386__ */
}


/**********************************************************************
 *		EXC_segv
 *
 * Handler for SIGSEGV and related errors.
 */
static HANDLER_DEF(EXC_segv)
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    HANDLER_INIT();

#if defined(TRAP_sig) && defined(CR2_sig)
    /* we want the page-fault case to be fast */
    if (TRAP_sig(HANDLER_CONTEXT) == 14)
        if (VIRTUAL_HandleFault( (LPVOID)CR2_sig(HANDLER_CONTEXT) )) return;
#endif

    EXC_SaveContext( &context, HANDLER_CONTEXT );
    rec.ExceptionRecord  = NULL;
    rec.ExceptionFlags   = EH_NONCONTINUABLE;
    rec.ExceptionAddress = GET_IP(&context);
    rec.NumberParameters = 0;

#ifdef TRAP_sig
    switch(TRAP_sig(HANDLER_CONTEXT))
    {
    case 4:   /* Overflow exception */
        rec.ExceptionCode = EXCEPTION_INT_OVERFLOW;
        break;
    case 5:   /* Bound range exception */
        rec.ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
        break;
    case 6:   /* Invalid opcode exception */
        rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    case 12:  /* Stack fault */
        rec.ExceptionCode = EXCEPTION_STACK_OVERFLOW;
        break;
    case 11:  /* Segment not present exception */
    case 13:  /* General protection fault */
        if (INSTR_EmulateInstruction( &context )) goto restore;
        rec.ExceptionCode = EXCEPTION_PRIV_INSTRUCTION;
        break;
    case 14:  /* Page fault */
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
    case 17:  /* Alignment check exception */
        /* FIXME: pass through exception handler first? */
    	if (EFL_reg(&context) & 0x00040000)
        {
            /* Disable AC flag, return */
            EFL_reg(&context) &= ~0x00040000;
            goto restore;
 	}
        rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    default:
        ERR( "Got unexpected trap %ld\n", TRAP_sig(HANDLER_CONTEXT) );
        /* fall through */
    case 2:   /* NMI interrupt */
    case 7:   /* Device not available exception */
    case 8:   /* Double fault exception */
    case 10:  /* Invalid TSS exception */
    case 15:  /* Unknown exception */
    case 18:  /* Machine check exception */
    case 19:  /* Cache flush exception */
        rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    }
#else  /* TRAP_sig */
# ifdef __i386
    if (INSTR_EmulateInstruction( &context )) return;
# endif
    rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;  /* generic error */
#endif  /* TRAP_sig */

    REGS_FUNC(RtlRaiseException)( &rec, &context );
 restore:
    EXC_RestoreContext( &context, HANDLER_CONTEXT );
}


/**********************************************************************
 *		EXC_trap
 *
 * Handler for SIGTRAP.
 */
static HANDLER_DEF(EXC_trap)
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    HANDLER_INIT();

#ifdef TRAP_sig
    rec.ExceptionCode = (TRAP_sig(HANDLER_CONTEXT) == 1) ?
                            EXCEPTION_SINGLE_STEP : EXCEPTION_BREAKPOINT;
#else
    rec.ExceptionCode = EXCEPTION_BREAKPOINT;
#endif  /* TRAP_sig */

    EXC_SaveContext( &context, HANDLER_CONTEXT );
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = GET_IP(&context);
    rec.NumberParameters = 0;
    REGS_FUNC(RtlRaiseException)( &rec, &context );
    EXC_RestoreContext( &context, HANDLER_CONTEXT );
}


/***********************************************************************
 *           EXC_SaveFPU
 *
 * Set the FPU context from a sigcontext. 
 */
static void inline EXC_SaveFPU( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
#ifdef __i386__
# ifdef FPU_sig
    if (FPU_sig(sigcontext))
    {
        context->FloatSave = *FPU_sig(sigcontext);
        return;
    }
# endif  /* FPU_sig */
# ifdef __GNUC__
    __asm__ __volatile__( "fnsave %0; fwait" : "=m" (context->FloatSave) );
# endif  /* __GNUC__ */
#endif  /* __i386__ */
}


/***********************************************************************
 *           EXC_RestoreFPU
 *
 * Restore the FPU context to a sigcontext. 
 */
static void inline EXC_RestoreFPU( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
#ifdef __i386__
# ifdef FPU_sig
    if (FPU_sig(sigcontext))
    {
        *FPU_sig(sigcontext) = context->FloatSave;
        return;
    }
# endif  /* FPU_sig */
# ifdef __GNUC__
    /* avoid nested exceptions */
    context->FloatSave.StatusWord &= context->FloatSave.ControlWord | 0xffffff80;
    __asm__ __volatile__( "frstor %0; fwait" : : "m" (context->FloatSave) );
# endif  /* __GNUC__ */
#endif  /* __i386__ */
}


/**********************************************************************
 *		EXC_GetFPUCode
 *
 * Get the FPU exception code from the FPU status.
 */
static inline DWORD EXC_GetFPUCode( const CONTEXT *context )
{
#ifdef __i386__
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
#endif  /* __i386__ */
    return EXCEPTION_FLT_INVALID_OPERATION;  /* generic error */
}


/**********************************************************************
 *		EXC_fpe
 *
 * Handler for SIGFPE.
 */
static HANDLER_DEF(EXC_fpe)
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    HANDLER_INIT();

    EXC_SaveFPU( &context, HANDLER_CONTEXT );

#ifdef TRAP_sig
    switch(TRAP_sig(HANDLER_CONTEXT))
    {
    case 0:   /* Division by zero exception */
        rec.ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
        break;
    case 9:   /* Coprocessor segment overrun */
        rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    case 16:  /* Floating point exception */
        rec.ExceptionCode = EXC_GetFPUCode( &context );
        break;
    default:
        ERR( "Got unexpected trap %ld\n", TRAP_sig(HANDLER_CONTEXT) );
        rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    }
#else  /* TRAP_sig */
    rec.ExceptionCode = EXC_GetFPUCode( &context );
#endif  /* TRAP_sig */

    EXC_SaveContext( &context, HANDLER_CONTEXT );
    rec.ExceptionFlags   = EH_NONCONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = GET_IP(&context);
    rec.NumberParameters = 0;
    REGS_FUNC(RtlRaiseException)( &rec, &context );
    EXC_RestoreContext( &context, HANDLER_CONTEXT );
    EXC_RestoreFPU( &context, HANDLER_CONTEXT );
}


/**********************************************************************
 *		EXC_int
 *
 * Handler for SIGINT.
 */
static HANDLER_DEF(EXC_int)
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    HANDLER_INIT();

    EXC_SaveContext( &context, HANDLER_CONTEXT );
    rec.ExceptionCode    = CONTROL_C_EXIT;
    rec.ExceptionFlags   = EH_NONCONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = GET_IP(&context);
    rec.NumberParameters = 0;
    REGS_FUNC(RtlRaiseException)( &rec, &context );
    EXC_RestoreContext( &context, HANDLER_CONTEXT );
}


/**********************************************************************
 *          EXC_InitHandlers
 *
 * Initialize the signal handlers for exception handling.
 */
void EXC_InitHandlers(void)
{
    SIGNAL_SetHandler( SIGINT,  (void (*)())EXC_int );
    SIGNAL_SetHandler( SIGFPE,  (void (*)())EXC_fpe );
    SIGNAL_SetHandler( SIGSEGV, (void (*)())EXC_segv );
    SIGNAL_SetHandler( SIGILL,  (void (*)())EXC_segv );
#ifdef SIGBUS
    SIGNAL_SetHandler( SIGBUS,  (void (*)())EXC_segv );
#endif
#ifdef SIGTRAP
    SIGNAL_SetHandler( SIGTRAP, (void (*)())EXC_trap );
#endif
    return;
}
