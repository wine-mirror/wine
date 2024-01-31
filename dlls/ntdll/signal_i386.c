/*
 * i386 signal handling routines
 *
 * Copyright 1999 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef __i386__

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "ntdll_misc.h"
#include "wine/exception.h"
#include "wine/debug.h"
#include "ntsyscalls.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(relay);
WINE_DECLARE_DEBUG_CHANNEL(threadname);

struct x86_thread_data
{
    DWORD              fs;            /* 1d4 TEB selector */
    DWORD              gs;            /* 1d8 libc selector; update winebuild if you move this! */
    DWORD              dr0;           /* 1dc debug registers */
    DWORD              dr1;           /* 1e0 */
    DWORD              dr2;           /* 1e4 */
    DWORD              dr3;           /* 1e8 */
    DWORD              dr6;           /* 1ec */
    DWORD              dr7;           /* 1f0 */
    void              *exit_frame;    /* 1f4 exit frame pointer */
};

C_ASSERT( sizeof(struct x86_thread_data) <= 16 * sizeof(void *) );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct x86_thread_data, gs ) == 0x1d8 );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct x86_thread_data, exit_frame ) == 0x1f4 );

static inline struct x86_thread_data *x86_thread_data(void)
{
    return (struct x86_thread_data *)&NtCurrentTeb()->GdiTebBatch;
}

/* Exception record for handling exceptions happening inside exception handlers */
typedef struct
{
    EXCEPTION_REGISTRATION_RECORD frame;
    EXCEPTION_REGISTRATION_RECORD *prevFrame;
} EXC_NESTED_FRAME;

extern DWORD EXC_CallHandler( EXCEPTION_RECORD *record, EXCEPTION_REGISTRATION_RECORD *frame,
                              CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher,
                              PEXCEPTION_HANDLER handler, PEXCEPTION_HANDLER nested_handler );


#ifdef __WINE_PE_BUILD

enum syscall_ids
{
#define SYSCALL_ENTRY(id,name,args) __id_##name = id,
ALL_SYSCALLS32
#undef SYSCALL_ENTRY
};

/*******************************************************************
 *         NtQueryInformationProcess
 */
void NtQueryInformationProcess_wrapper(void)
{
    asm( ".globl " __ASM_STDCALL("NtQueryInformationProcess", 20) "\n"
         __ASM_STDCALL("NtQueryInformationProcess", 20) ":\n\t"
         "movl %0,%%eax\n\t"
         "call *%%fs:0xc0\n\t"
         "ret $20"  :: "i" (__id_NtQueryInformationProcess) );
}
#define NtQueryInformationProcess syscall_NtQueryInformationProcess

#endif /* __WINE_PE_BUILD */

/*******************************************************************
 *         syscalls
 */
#define SYSCALL_ENTRY(id,name,args) __ASM_SYSCALL_FUNC( id, name, args )
ALL_SYSCALLS32
DEFINE_SYSCALL_HELPER32()
#undef SYSCALL_ENTRY


/*******************************************************************
 *         raise_handler
 *
 * Handler for exceptions happening inside a handler.
 */
static DWORD raise_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                            CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
        return ExceptionContinueSearch;
    /* We shouldn't get here so we store faulty frame in dispatcher */
    *dispatcher = ((EXC_NESTED_FRAME*)frame)->prevFrame;
    return ExceptionNestedException;
}


/*******************************************************************
 *         unwind_handler
 *
 * Handler for exceptions happening inside an unwind handler.
 */
static DWORD unwind_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                             CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    if (!(rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND)))
        return ExceptionContinueSearch;
    /* We shouldn't get here so we store faulty frame in dispatcher */
    *dispatcher = ((EXC_NESTED_FRAME*)frame)->prevFrame;
    return ExceptionCollidedUnwind;
}


/**********************************************************************
 *           call_stack_handlers
 *
 * Call the stack handlers chain.
 */
static NTSTATUS call_stack_handlers( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    EXCEPTION_REGISTRATION_RECORD *frame, *dispatch, *nested_frame;
    DWORD res;

    frame = NtCurrentTeb()->Tib.ExceptionList;
    nested_frame = NULL;
    while (frame != (EXCEPTION_REGISTRATION_RECORD*)~0UL)
    {
        /* Check frame address */
        if (!is_valid_frame( (ULONG_PTR)frame ))
        {
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        /* Call handler */
        TRACE( "calling handler at %p code=%lx flags=%lx\n",
               frame->Handler, rec->ExceptionCode, rec->ExceptionFlags );
        res = EXC_CallHandler( rec, frame, context, &dispatch, frame->Handler, raise_handler );
        TRACE( "handler at %p returned %lx\n", frame->Handler, res );

        if (frame == nested_frame)
        {
            /* no longer nested */
            nested_frame = NULL;
            rec->ExceptionFlags &= ~EH_NESTED_CALL;
        }

        switch(res)
        {
        case ExceptionContinueExecution:
            if (!(rec->ExceptionFlags & EH_NONCONTINUABLE)) return STATUS_SUCCESS;
            return STATUS_NONCONTINUABLE_EXCEPTION;
        case ExceptionContinueSearch:
            break;
        case ExceptionNestedException:
            if (nested_frame < dispatch) nested_frame = dispatch;
            rec->ExceptionFlags |= EH_NESTED_CALL;
            break;
        default:
            return STATUS_INVALID_DISPOSITION;
        }
        frame = frame->Prev;
    }
    return STATUS_UNHANDLED_EXCEPTION;
}


/*******************************************************************
 *		KiUserExceptionDispatcher (NTDLL.@)
 */
NTSTATUS WINAPI dispatch_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status;
    DWORD c;

    TRACE( "code=%lx flags=%lx addr=%p ip=%08lx\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, context->Eip );
    for (c = 0; c < rec->NumberParameters; c++)
        TRACE( " info[%ld]=%08Ix\n", c, rec->ExceptionInformation[c] );

    if (rec->ExceptionCode == EXCEPTION_WINE_STUB)
    {
        if (rec->ExceptionInformation[1] >> 16)
            MESSAGE( "wine: Call from %p to unimplemented function %s.%s, aborting\n",
                     rec->ExceptionAddress,
                     (char*)rec->ExceptionInformation[0], (char*)rec->ExceptionInformation[1] );
        else
            MESSAGE( "wine: Call from %p to unimplemented function %s.%Id, aborting\n",
                     rec->ExceptionAddress,
                     (char*)rec->ExceptionInformation[0], rec->ExceptionInformation[1] );
    }
    else if (rec->ExceptionCode == EXCEPTION_WINE_NAME_THREAD && rec->ExceptionInformation[0] == 0x1000)
    {
        if ((DWORD)rec->ExceptionInformation[2] == -1 || (DWORD)rec->ExceptionInformation[2] == GetCurrentThreadId())
            WARN_(threadname)( "Thread renamed to %s\n", debugstr_a((char *)rec->ExceptionInformation[1]) );
        else
            WARN_(threadname)( "Thread ID %04lx renamed to %s\n", (DWORD)rec->ExceptionInformation[2],
                               debugstr_a((char *)rec->ExceptionInformation[1]) );

        set_native_thread_name((DWORD)rec->ExceptionInformation[2], (char *)rec->ExceptionInformation[1]);
    }
    else if (rec->ExceptionCode == DBG_PRINTEXCEPTION_C)
    {
        WARN( "%s\n", debugstr_an((char *)rec->ExceptionInformation[1], rec->ExceptionInformation[0] - 1) );
    }
    else if (rec->ExceptionCode == DBG_PRINTEXCEPTION_WIDE_C)
    {
        WARN( "%s\n", debugstr_wn((WCHAR *)rec->ExceptionInformation[1], rec->ExceptionInformation[0] - 1) );
    }
    else
    {
        if (rec->ExceptionCode == STATUS_ASSERTION_FAILURE)
            ERR( "%s exception (code=%lx) raised\n", debugstr_exception_code(rec->ExceptionCode), rec->ExceptionCode );
        else
            WARN( "%s exception (code=%lx) raised\n", debugstr_exception_code(rec->ExceptionCode), rec->ExceptionCode );

        TRACE(" eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
              context->Eax, context->Ebx, context->Ecx,
              context->Edx, context->Esi, context->Edi );
        TRACE(" ebp=%08lx esp=%08lx cs=%04lx ss=%04lx ds=%04lx es=%04lx fs=%04lx gs=%04lx flags=%08lx\n",
              context->Ebp, context->Esp, context->SegCs, context->SegSs, context->SegDs,
              context->SegEs, context->SegFs, context->SegGs, context->EFlags );
    }

    if (call_vectored_handlers( rec, context ) == EXCEPTION_CONTINUE_EXECUTION)
        NtContinue( context, FALSE );

    if ((status = call_stack_handlers( rec, context )) == STATUS_SUCCESS)
        NtContinue( context, FALSE );

    if (status != STATUS_UNHANDLED_EXCEPTION) RtlRaiseStatus( status );
    return NtRaiseException( rec, context, FALSE );
}

__ASM_STDCALL_FUNC( KiUserExceptionDispatcher, 8,
                    "pushl 4(%esp)\n\t"
                    "pushl 4(%esp)\n\t"
                    "call " __ASM_STDCALL("dispatch_exception", 8) "\n\t"
                    "int3" )


/*******************************************************************
 *		KiUserApcDispatcher (NTDLL.@)
 */
__ASM_STDCALL_FUNC( KiUserApcDispatcher, 20,
                    "leal 0x14(%esp),%ebx\n\t"  /* context */
                    "pop %eax\n\t"              /* func */
                    "call *%eax\n\t"
                    "pushl -4(%ebx)\n\t"        /* alertable */
                    "pushl %ebx\n\t"            /* context */
                    "call " __ASM_STDCALL("NtContinue", 8) "\n\t"
                    "int3" )


/*******************************************************************
 *		KiUserCallbackDispatcher (NTDLL.@)
 */
void WINAPI KiUserCallbackDispatcher( ULONG id, void *args, ULONG len )
{
    NTSTATUS status = dispatch_user_callback( args, len, id );
    status = NtCallbackReturn( NULL, 0, status );
    for (;;) RtlRaiseStatus( status );
}


/***********************************************************************
 *           save_fpu
 *
 * Save the thread FPU context.
 */
static inline void save_fpu( CONTEXT *context )
{
#ifdef __GNUC__
    struct
    {
        DWORD ControlWord;
        DWORD StatusWord;
        DWORD TagWord;
        DWORD ErrorOffset;
        DWORD ErrorSelector;
        DWORD DataOffset;
        DWORD DataSelector;
    }
    float_status;

    context->ContextFlags |= CONTEXT_FLOATING_POINT;
    __asm__ __volatile__( "fnsave %0; fwait" : "=m" (context->FloatSave) );

    /* Reset unmasked exceptions status to avoid firing an exception. */
    memcpy(&float_status, &context->FloatSave, sizeof(float_status));
    float_status.StatusWord &= float_status.ControlWord | 0xffffff80;

    __asm__ __volatile__( "fldenv %0" : : "m" (float_status) );
#endif
}


/***********************************************************************
 *           save_fpux
 *
 * Save the thread FPU extended context.
 */
static inline void save_fpux( CONTEXT *context )
{
#ifdef __GNUC__
    /* we have to enforce alignment by hand */
    char buffer[sizeof(XSAVE_FORMAT) + 16];
    XSAVE_FORMAT *state = (XSAVE_FORMAT *)(((ULONG_PTR)buffer + 15) & ~15);

    context->ContextFlags |= CONTEXT_EXTENDED_REGISTERS;
    __asm__ __volatile__( "fxsave %0" : "=m" (*state) );
    memcpy( context->ExtendedRegisters, state, sizeof(*state) );
#endif
}


/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
__ASM_STDCALL_FUNC( RtlCaptureContext, 4,
                    "pushl %eax\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    "movl 8(%esp),%eax\n\t"    /* context */
                    "movl $0x10007,(%eax)\n\t" /* context->ContextFlags */
                    "movw %gs,0x8c(%eax)\n\t"  /* context->SegGs */
                    "movw %fs,0x90(%eax)\n\t"  /* context->SegFs */
                    "movw %es,0x94(%eax)\n\t"  /* context->SegEs */
                    "movw %ds,0x98(%eax)\n\t"  /* context->SegDs */
                    "movl %edi,0x9c(%eax)\n\t" /* context->Edi */
                    "movl %esi,0xa0(%eax)\n\t" /* context->Esi */
                    "movl %ebx,0xa4(%eax)\n\t" /* context->Ebx */
                    "movl %edx,0xa8(%eax)\n\t" /* context->Edx */
                    "movl %ecx,0xac(%eax)\n\t" /* context->Ecx */
                    "movl 0(%ebp),%edx\n\t"
                    "movl %edx,0xb4(%eax)\n\t" /* context->Ebp */
                    "movl 4(%ebp),%edx\n\t"
                    "movl %edx,0xb8(%eax)\n\t" /* context->Eip */
                    "movw %cs,0xbc(%eax)\n\t"  /* context->SegCs */
                    "pushfl\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    "popl 0xc0(%eax)\n\t"      /* context->EFlags */
                    __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                    "leal 8(%ebp),%edx\n\t"
                    "movl %edx,0xc4(%eax)\n\t" /* context->Esp */
                    "movw %ss,0xc8(%eax)\n\t"  /* context->SegSs */
                    "popl 0xb0(%eax)\n\t"      /* context->Eax */
                    __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                    "ret $4" )

/*******************************************************************
 *              RtlRestoreContext (NTDLL.@)
 */
void CDECL RtlRestoreContext( CONTEXT *context, EXCEPTION_RECORD *rec )
{
    TRACE( "returning to %p stack %p\n", (void *)context->Eip, (void *)context->Esp );
    NtContinue( context, FALSE );
}

/*******************************************************************
 *		RtlUnwind (NTDLL.@)
 */
void WINAPI __regs_RtlUnwind( EXCEPTION_REGISTRATION_RECORD* pEndFrame, PVOID targetIp,
                              PEXCEPTION_RECORD pRecord, PVOID retval, CONTEXT *context )
{
    EXCEPTION_RECORD record;
    EXCEPTION_REGISTRATION_RECORD *frame, *dispatch;
    DWORD res;

    context->Eax = (DWORD)retval;

    /* build an exception record, if we do not have one */
    if (!pRecord)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = (void *)context->Eip;
        record.NumberParameters = 0;
        pRecord = &record;
    }

    pRecord->ExceptionFlags |= EH_UNWINDING | (pEndFrame ? 0 : EH_EXIT_UNWIND);

    TRACE( "code=%lx flags=%lx\n", pRecord->ExceptionCode, pRecord->ExceptionFlags );
    TRACE( "eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
           context->Eax, context->Ebx, context->Ecx, context->Edx, context->Esi, context->Edi );
    TRACE( "ebp=%08lx esp=%08lx eip=%08lx cs=%04x ds=%04x fs=%04x gs=%04x flags=%08lx\n",
           context->Ebp, context->Esp, context->Eip, LOWORD(context->SegCs), LOWORD(context->SegDs),
           LOWORD(context->SegFs), LOWORD(context->SegGs), context->EFlags );

    /* get chain of exception frames */
    frame = NtCurrentTeb()->Tib.ExceptionList;
    while ((frame != (EXCEPTION_REGISTRATION_RECORD*)~0UL) && (frame != pEndFrame))
    {
        /* Check frame address */
        if (pEndFrame && (frame > pEndFrame))
            raise_status( STATUS_INVALID_UNWIND_TARGET, pRecord );

        if (!is_valid_frame( (ULONG_PTR)frame )) raise_status( STATUS_BAD_STACK, pRecord );

        /* Call handler */
        TRACE( "calling handler at %p code=%lx flags=%lx\n",
               frame->Handler, pRecord->ExceptionCode, pRecord->ExceptionFlags );
        res = EXC_CallHandler( pRecord, frame, context, &dispatch, frame->Handler, unwind_handler );
        TRACE( "handler at %p returned %lx\n", frame->Handler, res );

        switch(res)
        {
        case ExceptionContinueSearch:
            break;
        case ExceptionCollidedUnwind:
            frame = dispatch;
            break;
        default:
            raise_status( STATUS_INVALID_DISPOSITION, pRecord );
            break;
        }
        frame = __wine_pop_frame( frame );
    }
    NtContinue( context, FALSE );
}
__ASM_STDCALL_FUNC( RtlUnwind, 16,
                    "pushl %ebp\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                    "movl %esp,%ebp\n\t"
                    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                    "leal -(0x2cc+8)(%esp),%esp\n\t" /* sizeof(CONTEXT) + alignment */
                    "pushl %eax\n\t"
                    "leal 4(%esp),%eax\n\t"          /* context */
                    "xchgl %eax,(%esp)\n\t"
                    "call " __ASM_STDCALL("RtlCaptureContext",4) "\n\t"
                    "leal 24(%ebp),%eax\n\t"
                    "movl %eax,0xc4(%esp)\n\t"       /* context->Esp */
                    "pushl %esp\n\t"
                    "pushl 20(%ebp)\n\t"
                    "pushl 16(%ebp)\n\t"
                    "pushl 12(%ebp)\n\t"
                    "pushl 8(%ebp)\n\t"
                    "call " __ASM_STDCALL("__regs_RtlUnwind",20) "\n\t"
                    "leave\n\t"
                    __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                    __ASM_CFI(".cfi_same_value %ebp\n\t")
                    "ret $16" )  /* actually never returns */


/*******************************************************************
 *		raise_exception_full_context
 *
 * Raise an exception with the full CPU context.
 */
void raise_exception_full_context( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    save_fpu( context );
    save_fpux( context );
    /* FIXME: xstate */
    context->Dr0 = x86_thread_data()->dr0;
    context->Dr1 = x86_thread_data()->dr1;
    context->Dr2 = x86_thread_data()->dr2;
    context->Dr3 = x86_thread_data()->dr3;
    context->Dr6 = x86_thread_data()->dr6;
    context->Dr7 = x86_thread_data()->dr7;
    context->ContextFlags |= CONTEXT_DEBUG_REGISTERS;

    RtlRaiseStatus( NtRaiseException( rec, context, TRUE ));
}


/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
__ASM_STDCALL_FUNC( RtlRaiseException, 4,
                    "pushl %ebp\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                    "movl %esp,%ebp\n\t"
                    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                    "leal -0x2cc(%esp),%esp\n\t"  /* sizeof(CONTEXT) */
                    "pushl %esp\n\t"              /* context */
                    "call " __ASM_STDCALL("RtlCaptureContext",4) "\n\t"
                    "movl 4(%ebp),%eax\n\t"       /* return address */
                    "movl 8(%ebp),%ecx\n\t"       /* rec */
                    "movl %eax,12(%ecx)\n\t"      /* rec->ExceptionAddress */
                    "leal 12(%ebp),%eax\n\t"
                    "movl %eax,0xc4(%esp)\n\t"    /* context->Esp */
                    "movl %esp,%eax\n\t"
                    "pushl %eax\n\t"
                    "pushl %ecx\n\t"
                    "call " __ASM_NAME("raise_exception_full_context") "\n\t"
                    "leave\n\t"
                    __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                    __ASM_CFI(".cfi_same_value %ebp\n\t")
                    "ret $4" )  /* actually never returns */


/*************************************************************************
 *		RtlCaptureStackBackTrace (NTDLL.@)
 */
USHORT WINAPI RtlCaptureStackBackTrace( ULONG skip, ULONG count, PVOID *buffer, ULONG *hash )
{
    CONTEXT context;
    ULONG i;
    ULONG *frame;

    RtlCaptureContext( &context );
    if (hash) *hash = 0;
    frame = (ULONG *)context.Ebp;

    while (skip--)
    {
        if (!is_valid_frame( (ULONG_PTR)frame )) return 0;
        frame = (ULONG *)*frame;
    }

    for (i = 0; i < count; i++)
    {
        if (!is_valid_frame( (ULONG_PTR)frame )) break;
        buffer[i] = (void *)frame[1];
        if (hash) *hash += frame[1];
        frame = (ULONG *)*frame;
    }
    return i;
}


/***********************************************************************
 *           RtlUserThreadStart (NTDLL.@)
 */
__ASM_STDCALL_FUNC( RtlUserThreadStart, 8,
                   "movl %ebx,8(%esp)\n\t"  /* arg */
                   "movl %eax,4(%esp)\n\t"  /* entry */
                   "jmp " __ASM_NAME("call_thread_func") )

/* wrapper to call BaseThreadInitThunk */
extern void DECLSPEC_NORETURN call_thread_func_wrapper( void *thunk, PRTL_THREAD_START_ROUTINE entry, void *arg );
__ASM_GLOBAL_FUNC( call_thread_func_wrapper,
                  "pushl %ebp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "subl $4,%esp\n\t"
                   "andl $~0xf,%esp\n\t"
                   "xorl %ecx,%ecx\n\t"
                   "movl 12(%ebp),%edx\n\t"
                   "movl 16(%ebp),%eax\n\t"
                   "movl %eax,(%esp)\n\t"
                   "call *8(%ebp)" )

void call_thread_func( PRTL_THREAD_START_ROUTINE entry, void *arg )
{
    __TRY
    {
        call_thread_func_wrapper( pBaseThreadInitThunk, entry, arg );
    }
    __EXCEPT(call_unhandled_exception_filter)
    {
        NtTerminateProcess( GetCurrentProcess(), GetExceptionCode() );
    }
    __ENDTRY
}

/***********************************************************************
 *           signal_start_thread
 */
extern void CDECL DECLSPEC_NORETURN signal_start_thread( CONTEXT *ctx );
__ASM_GLOBAL_FUNC( signal_start_thread,
                   "movl 4(%esp),%esi\n\t"   /* context */
                   "leal -12(%esi),%edi\n\t"
                   /* clear the thread stack */
                   "andl $~0xfff,%edi\n\t"   /* round down to page size */
                   "movl $0xf0000,%ecx\n\t"
                   "subl %ecx,%edi\n\t"
                   "movl %edi,%esp\n\t"
                   "xorl %eax,%eax\n\t"
                   "shrl $2,%ecx\n\t"
                   "rep; stosl\n\t"
                   /* switch to the initial context */
                   "leal -12(%esi),%esp\n\t"
                   "movl $1,4(%esp)\n\t"
                   "movl %esi,(%esp)\n\t"
                   "call " __ASM_STDCALL("NtContinue", 8) )

/******************************************************************
 *		LdrInitializeThunk (NTDLL.@)
 */
void WINAPI LdrInitializeThunk( CONTEXT *context, ULONG_PTR unk2, ULONG_PTR unk3, ULONG_PTR unk4 )
{
    loader_init( context, (void **)&context->Eax );
    TRACE_(relay)( "\1Starting thread proc %p (arg=%p)\n", (void *)context->Eax, (void *)context->Ebx );
    signal_start_thread( context );
}


/***********************************************************************
 *           process_breakpoint
 */
void WINAPI process_breakpoint(void)
{
    __TRY
    {
        __asm__ volatile("int $3");
    }
    __EXCEPT_ALL
    {
        /* do nothing */
    }
    __ENDTRY
}


/***********************************************************************
 *		DbgUiRemoteBreakin   (NTDLL.@)
 */
void WINAPI DbgUiRemoteBreakin( void *arg )
{
    if (NtCurrentTeb()->Peb->BeingDebugged)
    {
        __TRY
        {
            DbgBreakPoint();
        }
        __EXCEPT_ALL
        {
            /* do nothing */
        }
        __ENDTRY
    }
    RtlExitUserThread( STATUS_SUCCESS );
}


/**********************************************************************
 *		DbgBreakPoint   (NTDLL.@)
 */
__ASM_STDCALL_FUNC( DbgBreakPoint, 0, "int $3; ret"
                    "\n\tnop; nop; nop; nop; nop; nop; nop; nop"
                    "\n\tnop; nop; nop; nop; nop; nop" );

/**********************************************************************
 *		DbgUserBreakPoint   (NTDLL.@)
 */
__ASM_STDCALL_FUNC( DbgUserBreakPoint, 0, "int $3; ret"
                    "\n\tnop; nop; nop; nop; nop; nop; nop; nop"
                    "\n\tnop; nop; nop; nop; nop; nop" );

/**********************************************************************
 *           NtCurrentTeb   (NTDLL.@)
 */
__ASM_STDCALL_FUNC( NtCurrentTeb, 0, ".byte 0x64\n\tmovl 0x18,%eax\n\tret" )


/**************************************************************************
 *           _chkstk   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( _chkstk,
                   "negl %eax\n\t"
                   "addl %esp,%eax\n\t"
                   "xchgl %esp,%eax\n\t"
                   "movl 0(%eax),%eax\n\t"  /* copy return address from old location */
                   "movl %eax,0(%esp)\n\t"
                   "ret" )

/**************************************************************************
 *           _alloca_probe   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( _alloca_probe,
                   "negl %eax\n\t"
                   "addl %esp,%eax\n\t"
                   "xchgl %esp,%eax\n\t"
                   "movl 0(%eax),%eax\n\t"  /* copy return address from old location */
                   "movl %eax,0(%esp)\n\t"
                   "ret" )


/**********************************************************************
 *		EXC_CallHandler   (internal)
 *
 * Some exception handlers depend on EBP to have a fixed position relative to
 * the exception frame.
 * Shrinker depends on (*1) doing what it does,
 * (*2) being the exact instruction it is and (*3) beginning with 0x64
 * (i.e. the %fs prefix to the movl instruction). It also depends on the
 * function calling the handler having only 5 parameters (*4).
 */
__ASM_GLOBAL_FUNC( EXC_CallHandler,
                  "pushl %ebp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %ebx\n\t"
                   __ASM_CFI(".cfi_rel_offset %ebx,-4\n\t")
                   "movl 28(%ebp), %edx\n\t" /* ugly hack to pass the 6th param needed because of Shrinker */
                   "pushl 24(%ebp)\n\t"
                   "pushl 20(%ebp)\n\t"
                   "pushl 16(%ebp)\n\t"
                   "pushl 12(%ebp)\n\t"
                   "pushl 8(%ebp)\n\t"
                   "call " __ASM_NAME("call_exception_handler") "\n\t"
                   "popl %ebx\n\t"
                   __ASM_CFI(".cfi_same_value %ebx\n\t")
                   "leave\n"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret" )
__ASM_GLOBAL_FUNC(call_exception_handler,
                  "pushl %ebp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                  "subl $12,%esp\n\t"
                  "pushl 12(%ebp)\n\t"      /* make any exceptions in this... */
                  "pushl %edx\n\t"          /* handler be handled by... */
                  ".byte 0x64\n\t"
                  "pushl (0)\n\t"           /* nested_handler (passed in edx). */
                  ".byte 0x64\n\t"
                  "movl %esp,(0)\n\t"       /* push the new exception frame onto the exception stack. */
                  "pushl 20(%ebp)\n\t"
                  "pushl 16(%ebp)\n\t"
                  "pushl 12(%ebp)\n\t"
                  "pushl 8(%ebp)\n\t"
                  "movl 24(%ebp), %ecx\n\t" /* (*1) */
                  "call *%ecx\n\t"          /* call handler. (*2) */
                  ".byte 0x64\n\t"
                  "movl (0), %esp\n\t"      /* restore previous... (*3) */
                  ".byte 0x64\n\t"
                  "popl (0)\n\t"            /* exception frame. */
                  "movl %ebp, %esp\n\t"     /* restore saved stack, in case it was corrupted */
                  "popl %ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                  "ret $20" )            /* (*4) */

#endif  /* __i386__ */
