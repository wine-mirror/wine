/*
 * x86-64 signal handling routines
 *
 * Copyright 1999, 2005 Alexandre Julliard
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

#if defined(__x86_64__) && !defined(__arm64ec__)

#include <stdlib.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/list.h"
#include "ntdll_misc.h"
#include "wine/debug.h"
#include "ntsyscalls.h"

WINE_DEFAULT_DEBUG_CHANNEL(unwind);
WINE_DECLARE_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(relay);
WINE_DECLARE_DEBUG_CHANNEL(threadname);

typedef struct _SCOPE_TABLE
{
    ULONG Count;
    struct
    {
        ULONG BeginAddress;
        ULONG EndAddress;
        ULONG HandlerAddress;
        ULONG JumpTarget;
    } ScopeRecord[1];
} SCOPE_TABLE, *PSCOPE_TABLE;


/* layering violation: the setjmp buffer is defined in msvcrt, but used by RtlUnwindEx */
struct MSVCRT_JUMP_BUFFER
{
    ULONG64 Frame;
    ULONG64 Rbx;
    ULONG64 Rsp;
    ULONG64 Rbp;
    ULONG64 Rsi;
    ULONG64 Rdi;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;
    ULONG64 Rip;
    ULONG  MxCsr;
    USHORT FpCsr;
    USHORT Spare;
    M128A   Xmm6;
    M128A   Xmm7;
    M128A   Xmm8;
    M128A   Xmm9;
    M128A   Xmm10;
    M128A   Xmm11;
    M128A   Xmm12;
    M128A   Xmm13;
    M128A   Xmm14;
    M128A   Xmm15;
};


/*******************************************************************
 *         syscalls
 */
#define SYSCALL_ENTRY(id,name,args) __ASM_SYSCALL_FUNC( id, name )
ALL_SYSCALLS64
#undef SYSCALL_ENTRY


/***********************************************************************
 * Definitions for Win32 unwind tables
 */

union handler_data
{
    RUNTIME_FUNCTION chain;
    ULONG handler;
};

struct opcode
{
    BYTE offset;
    BYTE code : 4;
    BYTE info : 4;
};

struct UNWIND_INFO
{
    BYTE version : 3;
    BYTE flags : 5;
    BYTE prolog;
    BYTE count;
    BYTE frame_reg : 4;
    BYTE frame_offset : 4;
    struct opcode opcodes[1];  /* info->count entries */
    /* followed by handler_data */
};

#define UWOP_PUSH_NONVOL     0
#define UWOP_ALLOC_LARGE     1
#define UWOP_ALLOC_SMALL     2
#define UWOP_SET_FPREG       3
#define UWOP_SAVE_NONVOL     4
#define UWOP_SAVE_NONVOL_FAR 5
#define UWOP_EPILOG          6
#define UWOP_SAVE_XMM128     8
#define UWOP_SAVE_XMM128_FAR 9
#define UWOP_PUSH_MACHFRAME  10

static void dump_unwind_info( ULONG64 base, RUNTIME_FUNCTION *function )
{
    static const char * const reg_names[16] =
        { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
          "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15" };

    union handler_data *handler_data;
    struct UNWIND_INFO *info;
    unsigned int i, count;

    TRACE( "**** func %lx-%lx\n", function->BeginAddress, function->EndAddress );
    for (;;)
    {
        if (function->UnwindData & 1)
        {
            RUNTIME_FUNCTION *next = (RUNTIME_FUNCTION *)((char *)base + (function->UnwindData & ~1));
            TRACE( "unwind info for function %p-%p chained to function %p-%p\n",
                   (char *)base + function->BeginAddress, (char *)base + function->EndAddress,
                   (char *)base + next->BeginAddress, (char *)base + next->EndAddress );
            function = next;
            continue;
        }
        info = (struct UNWIND_INFO *)((char *)base + function->UnwindData);

        TRACE( "unwind info at %p flags %x prolog 0x%x bytes function %p-%p\n",
               info, info->flags, info->prolog,
               (char *)base + function->BeginAddress, (char *)base + function->EndAddress );

        if (info->frame_reg)
            TRACE( "    frame register %s offset 0x%x(%%rsp)\n",
                   reg_names[info->frame_reg], info->frame_offset * 16 );

        for (i = 0; i < info->count; i++)
        {
            TRACE( "    0x%x: ", info->opcodes[i].offset );
            switch (info->opcodes[i].code)
            {
            case UWOP_PUSH_NONVOL:
                TRACE( "pushq %%%s\n", reg_names[info->opcodes[i].info] );
                break;
            case UWOP_ALLOC_LARGE:
                if (info->opcodes[i].info)
                {
                    count = *(DWORD *)&info->opcodes[i+1];
                    i += 2;
                }
                else
                {
                    count = *(USHORT *)&info->opcodes[i+1] * 8;
                    i++;
                }
                TRACE( "subq $0x%x,%%rsp\n", count );
                break;
            case UWOP_ALLOC_SMALL:
                count = (info->opcodes[i].info + 1) * 8;
                TRACE( "subq $0x%x,%%rsp\n", count );
                break;
            case UWOP_SET_FPREG:
                TRACE( "leaq 0x%x(%%rsp),%s\n",
                     info->frame_offset * 16, reg_names[info->frame_reg] );
                break;
            case UWOP_SAVE_NONVOL:
                count = *(USHORT *)&info->opcodes[i+1] * 8;
                TRACE( "movq %%%s,0x%x(%%rsp)\n", reg_names[info->opcodes[i].info], count );
                i++;
                break;
            case UWOP_SAVE_NONVOL_FAR:
                count = *(DWORD *)&info->opcodes[i+1];
                TRACE( "movq %%%s,0x%x(%%rsp)\n", reg_names[info->opcodes[i].info], count );
                i += 2;
                break;
            case UWOP_SAVE_XMM128:
                count = *(USHORT *)&info->opcodes[i+1] * 16;
                TRACE( "movaps %%xmm%u,0x%x(%%rsp)\n", info->opcodes[i].info, count );
                i++;
                break;
            case UWOP_SAVE_XMM128_FAR:
                count = *(DWORD *)&info->opcodes[i+1];
                TRACE( "movaps %%xmm%u,0x%x(%%rsp)\n", info->opcodes[i].info, count );
                i += 2;
                break;
            case UWOP_PUSH_MACHFRAME:
                TRACE( "PUSH_MACHFRAME %u\n", info->opcodes[i].info );
                break;
            case UWOP_EPILOG:
                if (info->version == 2)
                {
                    unsigned int offset;
                    if (info->opcodes[i].info)
                        offset = info->opcodes[i].offset;
                    else
                        offset = (info->opcodes[i+1].info << 8) + info->opcodes[i+1].offset;
                    TRACE("epilog %p-%p\n", (char *)base + function->EndAddress - offset,
                            (char *)base + function->EndAddress - offset + info->opcodes[i].offset );
                    i += 1;
                    break;
                }
            default:
                FIXME( "unknown code %u\n", info->opcodes[i].code );
                break;
            }
        }

        handler_data = (union handler_data *)&info->opcodes[(info->count + 1) & ~1];
        if (info->flags & UNW_FLAG_CHAININFO)
        {
            TRACE( "    chained to function %p-%p\n",
                   (char *)base + handler_data->chain.BeginAddress,
                   (char *)base + handler_data->chain.EndAddress );
            function = &handler_data->chain;
            continue;
        }
        if (info->flags & (UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER))
            TRACE( "    handler %p data at %p\n",
                   (char *)base + handler_data->handler, &handler_data->handler + 1 );
        break;
    }
}

static void dump_scope_table( ULONG64 base, const SCOPE_TABLE *table )
{
    unsigned int i;

    TRACE( "scope table at %p\n", table );
    for (i = 0; i < table->Count; i++)
        TRACE( "  %u: %p-%p handler %p target %p\n", i,
               (char *)base + table->ScopeRecord[i].BeginAddress,
               (char *)base + table->ScopeRecord[i].EndAddress,
               (char *)base + table->ScopeRecord[i].HandlerAddress,
               (char *)base + table->ScopeRecord[i].JumpTarget );
}


/***********************************************************************
 *           virtual_unwind
 */
static NTSTATUS virtual_unwind( ULONG type, DISPATCHER_CONTEXT *dispatch, CONTEXT *context )
{
    LDR_DATA_TABLE_ENTRY *module;
    NTSTATUS status;

    dispatch->ImageBase = 0;
    dispatch->ScopeIndex = 0;
    dispatch->ControlPc = context->Rip;

    /* first look for PE exception information */

    if ((dispatch->FunctionEntry = lookup_function_info( context->Rip, &dispatch->ImageBase, &module )))
    {
        dispatch->LanguageHandler = RtlVirtualUnwind( type, dispatch->ImageBase, context->Rip,
                                                      dispatch->FunctionEntry, context,
                                                      &dispatch->HandlerData, &dispatch->EstablisherFrame,
                                                      NULL );
        return STATUS_SUCCESS;
    }

    /* then look for host system exception information */

    if (!module || (module->Flags & LDR_WINE_INTERNAL))
    {
        struct unwind_builtin_dll_params params = { type, dispatch, context };

        status = WINE_UNIX_CALL( unix_unwind_builtin_dll, &params );
        if (!status && dispatch->LanguageHandler && !module)
        {
            FIXME( "calling personality routine in system library not supported yet\n" );
            dispatch->LanguageHandler = NULL;
        }
        if (status != STATUS_UNSUCCESSFUL) return status;
    }
    else WARN( "exception data not found in %s\n", debugstr_w(module->BaseDllName.Buffer) );

    /* no exception information, treat as a leaf function */

    dispatch->EstablisherFrame = context->Rsp;
    dispatch->LanguageHandler = NULL;
    context->Rip = *(ULONG64 *)context->Rsp;
    context->Rsp = context->Rsp + sizeof(ULONG64);
    return STATUS_SUCCESS;
}


/**************************************************************************
 *		__chkstk (NTDLL.@)
 *
 * Supposed to touch all the stack pages, but we shouldn't need that.
 */
__ASM_GLOBAL_FUNC( __chkstk, "ret" );


/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( RtlCaptureContext,
                   "pushfq\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   "movl $0x10000f,0x30(%rcx)\n\t"  /* context->ContextFlags */
                   "stmxcsr 0x34(%rcx)\n\t"         /* context->MxCsr */
                   "movw %cs,0x38(%rcx)\n\t"        /* context->SegCs */
                   "movw %ds,0x3a(%rcx)\n\t"        /* context->SegDs */
                   "movw %es,0x3c(%rcx)\n\t"        /* context->SegEs */
                   "movw %fs,0x3e(%rcx)\n\t"        /* context->SegFs */
                   "movw %gs,0x40(%rcx)\n\t"        /* context->SegGs */
                   "movw %ss,0x42(%rcx)\n\t"        /* context->SegSs */
                   "popq 0x44(%rcx)\n\t"            /* context->Eflags */
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   "movq %rax,0x78(%rcx)\n\t"       /* context->Rax */
                   "movq %rcx,0x80(%rcx)\n\t"       /* context->Rcx */
                   "movq %rdx,0x88(%rcx)\n\t"       /* context->Rdx */
                   "movq %rbx,0x90(%rcx)\n\t"       /* context->Rbx */
                   "leaq 8(%rsp),%rax\n\t"
                   "movq %rax,0x98(%rcx)\n\t"       /* context->Rsp */
                   "movq %rbp,0xa0(%rcx)\n\t"       /* context->Rbp */
                   "movq %rsi,0xa8(%rcx)\n\t"       /* context->Rsi */
                   "movq %rdi,0xb0(%rcx)\n\t"       /* context->Rdi */
                   "movq %r8,0xb8(%rcx)\n\t"        /* context->R8 */
                   "movq %r9,0xc0(%rcx)\n\t"        /* context->R9 */
                   "movq %r10,0xc8(%rcx)\n\t"       /* context->R10 */
                   "movq %r11,0xd0(%rcx)\n\t"       /* context->R11 */
                   "movq %r12,0xd8(%rcx)\n\t"       /* context->R12 */
                   "movq %r13,0xe0(%rcx)\n\t"       /* context->R13 */
                   "movq %r14,0xe8(%rcx)\n\t"       /* context->R14 */
                   "movq %r15,0xf0(%rcx)\n\t"       /* context->R15 */
                   "movq (%rsp),%rax\n\t"
                   "movq %rax,0xf8(%rcx)\n\t"       /* context->Rip */
                   "fxsave 0x100(%rcx)\n\t"         /* context->FltSave */
                   "ret" );

/***********************************************************************
 *		exception_handler_call_wrapper
 */
#ifdef __WINE_PE_BUILD
DWORD WINAPI exception_handler_call_wrapper( EXCEPTION_RECORD *rec, void *frame,
                                      CONTEXT *context, DISPATCHER_CONTEXT *dispatch );

C_ASSERT( offsetof(DISPATCHER_CONTEXT, LanguageHandler) == 0x30 );

__ASM_GLOBAL_FUNC( exception_handler_call_wrapper,
                   "subq $0x28, %rsp\n\t"
                   ".seh_stackalloc 0x28\n\t"
                   ".seh_endprologue\n\t"
                   "callq *0x30(%r9)\n\t"       /* dispatch->LanguageHandler */
                   "nop\n\t"                    /* avoid epilogue so handler is called */
                   "addq $0x28, %rsp\n\t"
                   "ret\n\t"
                   ".seh_handler " __ASM_NAME("nested_exception_handler") ", @except\n\t" )
#else
static DWORD exception_handler_call_wrapper( EXCEPTION_RECORD *rec, void *frame,
                                             CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    EXCEPTION_REGISTRATION_RECORD wrapper_frame;
    DWORD res;

    wrapper_frame.Handler = (PEXCEPTION_HANDLER)nested_exception_handler;
    __wine_push_frame( &wrapper_frame );
    res = dispatch->LanguageHandler( rec, (void *)dispatch->EstablisherFrame, context, dispatch );
    __wine_pop_frame( &wrapper_frame );
    return res;
}
#endif

/**********************************************************************
 *           call_handler
 *
 * Call a single exception handler.
 */
static DWORD call_handler( EXCEPTION_RECORD *rec, CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    DWORD res;

    TRACE_(seh)( "calling handler %p (rec=%p, frame=%p context=%p, dispatch=%p)\n",
                 dispatch->LanguageHandler, rec, (void *)dispatch->EstablisherFrame, dispatch->ContextRecord, dispatch );
    res = exception_handler_call_wrapper( rec, (void *)dispatch->EstablisherFrame, context, dispatch );
    TRACE_(seh)( "handler at %p returned %lu\n", dispatch->LanguageHandler, res );

    rec->ExceptionFlags &= EH_NONCONTINUABLE;
    return res;
}


/**********************************************************************
 *           call_teb_handler
 *
 * Call a single exception handler from the TEB chain.
 * FIXME: Handle nested exceptions.
 */
static DWORD call_teb_handler( EXCEPTION_RECORD *rec, CONTEXT *context, DISPATCHER_CONTEXT *dispatch,
                                  EXCEPTION_REGISTRATION_RECORD *teb_frame )
{
    DWORD res;

    TRACE_(seh)( "calling TEB handler %p (rec=%p, frame=%p context=%p, dispatch=%p)\n",
                 teb_frame->Handler, rec, teb_frame, dispatch->ContextRecord, dispatch );
    res = teb_frame->Handler( rec, teb_frame, context, (EXCEPTION_REGISTRATION_RECORD**)dispatch );
    TRACE_(seh)( "handler at %p returned %lu\n", teb_frame->Handler, res );
    return res;
}


/**********************************************************************
 *           call_stack_handlers
 *
 * Call the stack handlers chain.
 */
static NTSTATUS call_stack_handlers( EXCEPTION_RECORD *rec, CONTEXT *orig_context )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    UNWIND_HISTORY_TABLE table;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT context;
    NTSTATUS status;

    context = *orig_context;
    context.ContextFlags &= ~0x40; /* Clear xstate flag. */

    dispatch.TargetIp      = 0;
    dispatch.ContextRecord = &context;
    dispatch.HistoryTable  = &table;
    for (;;)
    {
        status = virtual_unwind( UNW_FLAG_EHANDLER, &dispatch, &context );
        if (status != STATUS_SUCCESS) return status;

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (!is_valid_frame( dispatch.EstablisherFrame ))
        {
            ERR_(seh)( "invalid frame %p (%p-%p)\n", (void *)dispatch.EstablisherFrame,
                       NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            switch (call_handler( rec, orig_context, &dispatch ))
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EH_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                rec->ExceptionFlags |= EH_NESTED_CALL;
                TRACE_(seh)( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind: {
                ULONG64 frame;

                context = *dispatch.ContextRecord;
                dispatch.ContextRecord = &context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                        dispatch.ControlPc, dispatch.FunctionEntry,
                        &context, NULL, &frame, NULL );
                goto unwind_done;
            }
            default:
                return STATUS_INVALID_DISPOSITION;
            }
        }
        /* hack: call wine handlers registered in the tib list */
        else while (is_valid_frame( (ULONG_PTR)teb_frame ) && (ULONG64)teb_frame < context.Rsp)
        {
            TRACE_(seh)( "found wine frame %p rsp %p handler %p\n",
                         teb_frame, (void *)context.Rsp, teb_frame->Handler );
            dispatch.EstablisherFrame = (ULONG64)teb_frame;
            switch (call_teb_handler( rec, orig_context, &dispatch, teb_frame ))
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EH_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                rec->ExceptionFlags |= EH_NESTED_CALL;
                TRACE_(seh)( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind: {
                ULONG64 frame;

                context = *dispatch.ContextRecord;
                dispatch.ContextRecord = &context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                        dispatch.ControlPc, dispatch.FunctionEntry,
                        &context, NULL, &frame, NULL );
                teb_frame = teb_frame->Prev;
                goto unwind_done;
            }
            default:
                return STATUS_INVALID_DISPOSITION;
            }
            teb_frame = teb_frame->Prev;
        }

        if (context.Rsp == (ULONG64)NtCurrentTeb()->Tib.StackBase) break;
    }
    return STATUS_UNHANDLED_EXCEPTION;
}


NTSTATUS WINAPI dispatch_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status;
    DWORD c;

    TRACE_(seh)( "code=%lx flags=%lx addr=%p ip=%Ix\n",
                 rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, context->Rip );
    for (c = 0; c < min( EXCEPTION_MAXIMUM_PARAMETERS, rec->NumberParameters ); c++)
        TRACE_(seh)( " info[%ld]=%016I64x\n", c, rec->ExceptionInformation[c] );

    if (rec->ExceptionCode == EXCEPTION_WINE_STUB)
    {
        if (rec->ExceptionInformation[1] >> 16)
            MESSAGE( "wine: Call from %p to unimplemented function %s.%s, aborting\n",
                     rec->ExceptionAddress,
                     (char*)rec->ExceptionInformation[0], (char*)rec->ExceptionInformation[1] );
        else
            MESSAGE( "wine: Call from %p to unimplemented function %s.%I64d, aborting\n",
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
        WARN_(seh)( "%s\n", debugstr_an((char *)rec->ExceptionInformation[1], rec->ExceptionInformation[0] - 1) );
    }
    else if (rec->ExceptionCode == DBG_PRINTEXCEPTION_WIDE_C)
    {
        WARN_(seh)( "%s\n", debugstr_wn((WCHAR *)rec->ExceptionInformation[1], rec->ExceptionInformation[0] - 1) );
    }
    else
    {
        if (rec->ExceptionCode == STATUS_ASSERTION_FAILURE)
            ERR_(seh)( "%s exception (code=%lx) raised\n", debugstr_exception_code(rec->ExceptionCode), rec->ExceptionCode );
        else
            WARN_(seh)( "%s exception (code=%lx) raised\n", debugstr_exception_code(rec->ExceptionCode), rec->ExceptionCode );

        TRACE_(seh)( " rax=%016I64x rbx=%016I64x rcx=%016I64x rdx=%016I64x\n",
                     context->Rax, context->Rbx, context->Rcx, context->Rdx );
        TRACE_(seh)( " rsi=%016I64x rdi=%016I64x rbp=%016I64x rsp=%016I64x\n",
                     context->Rsi, context->Rdi, context->Rbp, context->Rsp );
        TRACE_(seh)( "  r8=%016I64x  r9=%016I64x r10=%016I64x r11=%016I64x\n",
                     context->R8, context->R9, context->R10, context->R11 );
        TRACE_(seh)( " r12=%016I64x r13=%016I64x r14=%016I64x r15=%016I64x\n",
                     context->R12, context->R13, context->R14, context->R15 );
    }

    if (call_vectored_handlers( rec, context ) == EXCEPTION_CONTINUE_EXECUTION)
        NtContinue( context, FALSE );

    if ((status = call_stack_handlers( rec, context )) == STATUS_SUCCESS)
        NtContinue( context, FALSE );

    if (status != STATUS_UNHANDLED_EXCEPTION) RtlRaiseStatus( status );
    return NtRaiseException( rec, context, FALSE );
}


/*******************************************************************
 *		KiUserExceptionDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( KiUserExceptionDispatcher,
                   __ASM_SEH(".seh_pushframe\n\t")
                   __ASM_SEH(".seh_stackalloc 0x590\n\t")
                   __ASM_SEH(".seh_savereg %rbx,0x90\n\t")
                   __ASM_SEH(".seh_savereg %rbp,0xa0\n\t")
                   __ASM_SEH(".seh_savereg %rsi,0xa8\n\t")
                   __ASM_SEH(".seh_savereg %rdi,0xb0\n\t")
                   __ASM_SEH(".seh_savereg %r12,0xd8\n\t")
                   __ASM_SEH(".seh_savereg %r13,0xe0\n\t")
                   __ASM_SEH(".seh_savereg %r14,0xe8\n\t")
                   __ASM_SEH(".seh_savereg %r15,0xf0\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_signal_frame\n\t")
                   __ASM_CFI(".cfi_def_cfa_offset 0\n\t")
                   __ASM_CFI(".cfi_offset %rbx,0x90\n\t")
                   __ASM_CFI(".cfi_offset %rbp,0xa0\n\t")
                   __ASM_CFI(".cfi_offset %rsi,0xa8\n\t")
                   __ASM_CFI(".cfi_offset %rdi,0xb0\n\t")
                   __ASM_CFI(".cfi_offset %r12,0xd8\n\t")
                   __ASM_CFI(".cfi_offset %r13,0xe0\n\t")
                   __ASM_CFI(".cfi_offset %r14,0xe8\n\t")
                   __ASM_CFI(".cfi_offset %r15,0xf0\n\t")
                   __ASM_CFI(".cfi_offset %rip,0x590\n\t")
                   __ASM_CFI(".cfi_offset %rsp,0x5a8\n\t")
                   "cld\n\t"
                   /* some anticheats need this exact instruction here */
                   "mov " __ASM_NAME("pWow64PrepareForException") "(%rip),%rax\n\t"
                   "test %rax,%rax\n\t"
                   "jz 1f\n\t"
                   "mov %rsp,%rdx\n\t"           /* context */
                   "lea 0x4f0(%rsp),%rcx\n\t"    /* rec */
                   "call *%rax\n"
                   "1:\tmov %rsp,%rdx\n\t" /* context */
                   "lea 0x4f0(%rsp),%rcx\n\t" /* rec */
                   "call " __ASM_NAME("dispatch_exception") "\n\t"
                   "int3" )


/*******************************************************************
 *		KiUserApcDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( KiUserApcDispatcher,
                   __ASM_SEH(".seh_pushframe\n\t")
                   __ASM_SEH(".seh_stackalloc 0x4d0\n\t")  /* sizeof(CONTEXT) */
                   __ASM_SEH(".seh_savereg %rbx,0x90\n\t")
                   __ASM_SEH(".seh_savereg %rbp,0xa0\n\t")
                   __ASM_SEH(".seh_savereg %rsi,0xa8\n\t")
                   __ASM_SEH(".seh_savereg %rdi,0xb0\n\t")
                   __ASM_SEH(".seh_savereg %r12,0xd8\n\t")
                   __ASM_SEH(".seh_savereg %r13,0xe0\n\t")
                   __ASM_SEH(".seh_savereg %r14,0xe8\n\t")
                   __ASM_SEH(".seh_savereg %r15,0xf0\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_signal_frame\n\t")
                   __ASM_CFI(".cfi_def_cfa_offset 0\n\t")
                   __ASM_CFI(".cfi_offset %rbx,0x90\n\t")
                   __ASM_CFI(".cfi_offset %rbp,0xa0\n\t")
                   __ASM_CFI(".cfi_offset %rsi,0xa8\n\t")
                   __ASM_CFI(".cfi_offset %rdi,0xb0\n\t")
                   __ASM_CFI(".cfi_offset %r12,0xd8\n\t")
                   __ASM_CFI(".cfi_offset %r13,0xe0\n\t")
                   __ASM_CFI(".cfi_offset %r14,0xe8\n\t")
                   __ASM_CFI(".cfi_offset %r15,0xf0\n\t")
                   __ASM_CFI(".cfi_offset %rip,0x4d0\n\t")
                   __ASM_CFI(".cfi_offset %rsp,0x4e8\n\t")
                   "movq 0x00(%rsp),%rcx\n\t"  /* context->P1Home = arg1 */
                   "movq 0x08(%rsp),%rdx\n\t"  /* context->P2Home = arg2 */
                   "movq 0x10(%rsp),%r8\n\t"   /* context->P3Home = arg3 */
                   "movq 0x18(%rsp),%rax\n\t"  /* context->P4Home = func */
                   "movq %rsp,%r9\n\t"         /* context */
                   "callq *%rax\n\t"
                   "movq %rsp,%rcx\n\t"        /* context */
                   "movl $1,%edx\n\t"          /* alertable */
                   "call " __ASM_NAME("NtContinue") "\n\t"
                   "int3" )


/*******************************************************************
 *		KiUserCallbackDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( KiUserCallbackDispatcher,
                   __ASM_SEH(".seh_pushframe\n\t")
                   __ASM_SEH(".seh_stackalloc 0x30\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_signal_frame\n\t")
                   __ASM_CFI(".cfi_def_cfa_offset 0\n\t")
                   __ASM_CFI(".cfi_offset %rip,0x30\n\t")
                   __ASM_CFI(".cfi_offset %rsp,0x48\n\t")
                   "movq 0x20(%rsp),%rcx\n\t"  /* args */
                   "movl 0x28(%rsp),%edx\n\t"  /* len */
                   "movl 0x2c(%rsp),%r8d\n\t"  /* id */
#ifdef __WINE_PE_BUILD
                   "movq %gs:0x30,%rax\n\t"     /* NtCurrentTeb() */
                   "movq 0x60(%rax),%rax\n\t"   /* peb */
                   "movq 0x58(%rax),%rax\n\t"   /* peb->KernelCallbackTable */
                   "call *(%rax,%r8,8)\n\t"     /* KernelCallbackTable[id] */
                   ".seh_handler " __ASM_NAME("user_callback_handler") ", @except\n\t"
                   ".globl " __ASM_NAME("KiUserCallbackDispatcherReturn") "\n"
                   __ASM_NAME("KiUserCallbackDispatcherReturn") ":\n\t"
#else
                   "call " __ASM_NAME("dispatch_user_callback") "\n\t"
#endif
                   "xorq %rcx,%rcx\n\t"         /* ret_ptr */
                   "xorl %edx,%edx\n\t"         /* ret_len */
                   "movl %eax,%r8d\n\t"         /* status */
                   "call " __ASM_NAME("NtCallbackReturn") "\n\t"
                   "movl %eax,%ecx\n\t"         /* status */
                   "call " __ASM_NAME("RtlRaiseStatus") "\n\t"
                   "int3" )


/**************************************************************************
 *              RtlIsEcCode (NTDLL.@)
 */
BOOLEAN WINAPI RtlIsEcCode( const void *ptr )
{
    return FALSE;
}


static ULONG64 get_int_reg( CONTEXT *context, int reg )
{
    return *(&context->Rax + reg);
}

static void set_int_reg( CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr, int reg, ULONG64 *val )
{
    *(&context->Rax + reg) = *val;
    if (ctx_ptr) ctx_ptr->IntegerContext[reg] = val;
}

static void set_float_reg( CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr, int reg, M128A *val )
{
    /* Use a memcpy() to avoid issues if val is misaligned. */
    memcpy(&context->Xmm0 + reg, val, sizeof(*val));
    if (ctx_ptr) ctx_ptr->FloatingContext[reg] = val;
}

static int get_opcode_size( struct opcode op )
{
    switch (op.code)
    {
    case UWOP_ALLOC_LARGE:
        return 2 + (op.info != 0);
    case UWOP_SAVE_NONVOL:
    case UWOP_SAVE_XMM128:
    case UWOP_EPILOG:
        return 2;
    case UWOP_SAVE_NONVOL_FAR:
    case UWOP_SAVE_XMM128_FAR:
        return 3;
    default:
        return 1;
    }
}

static BOOL is_inside_epilog( BYTE *pc, ULONG64 base, const RUNTIME_FUNCTION *function )
{
    /* add or lea must be the first instruction, and it must have a rex.W prefix */
    if ((pc[0] & 0xf8) == 0x48)
    {
        switch (pc[1])
        {
        case 0x81: /* add $nnnn,%rsp */
            if (pc[0] == 0x48 && pc[2] == 0xc4)
            {
                pc += 7;
                break;
            }
            return FALSE;
        case 0x83: /* add $n,%rsp */
            if (pc[0] == 0x48 && pc[2] == 0xc4)
            {
                pc += 4;
                break;
            }
            return FALSE;
        case 0x8d: /* lea n(reg),%rsp */
            if (pc[0] & 0x06) return FALSE;  /* rex.RX must be cleared */
            if (((pc[2] >> 3) & 7) != 4) return FALSE;  /* dest reg mus be %rsp */
            if ((pc[2] & 7) == 4) return FALSE;  /* no SIB byte allowed */
            if ((pc[2] >> 6) == 1)  /* 8-bit offset */
            {
                pc += 4;
                break;
            }
            if ((pc[2] >> 6) == 2)  /* 32-bit offset */
            {
                pc += 7;
                break;
            }
            return FALSE;
        }
    }

    /* now check for various pop instructions */

    for (;;)
    {
        if ((*pc & 0xf0) == 0x40) pc++;  /* rex prefix */

        switch (*pc)
        {
        case 0x58: /* pop %rax/%r8 */
        case 0x59: /* pop %rcx/%r9 */
        case 0x5a: /* pop %rdx/%r10 */
        case 0x5b: /* pop %rbx/%r11 */
        case 0x5c: /* pop %rsp/%r12 */
        case 0x5d: /* pop %rbp/%r13 */
        case 0x5e: /* pop %rsi/%r14 */
        case 0x5f: /* pop %rdi/%r15 */
            pc++;
            continue;
        case 0xc2: /* ret $nn */
        case 0xc3: /* ret */
            return TRUE;
        case 0xe9: /* jmp nnnn */
            pc += 5 + *(LONG *)(pc + 1);
            if (pc - (BYTE *)base >= function->BeginAddress && pc - (BYTE *)base < function->EndAddress)
                continue;
            break;
        case 0xeb: /* jmp n */
            pc += 2 + (signed char)pc[1];
            if (pc - (BYTE *)base >= function->BeginAddress && pc - (BYTE *)base < function->EndAddress)
                continue;
            break;
        case 0xf3: /* rep; ret (for amd64 prediction bug) */
            return pc[1] == 0xc3;
        }
        return FALSE;
    }
}

/* execute a function epilog, which must have been validated with is_inside_epilog() */
static void interpret_epilog( BYTE *pc, CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr )
{
    for (;;)
    {
        BYTE rex = 0;

        if ((*pc & 0xf0) == 0x40) rex = *pc++ & 0x0f;  /* rex prefix */

        switch (*pc)
        {
        case 0x58: /* pop %rax/r8 */
        case 0x59: /* pop %rcx/r9 */
        case 0x5a: /* pop %rdx/r10 */
        case 0x5b: /* pop %rbx/r11 */
        case 0x5c: /* pop %rsp/r12 */
        case 0x5d: /* pop %rbp/r13 */
        case 0x5e: /* pop %rsi/r14 */
        case 0x5f: /* pop %rdi/r15 */
            set_int_reg( context, ctx_ptr, *pc - 0x58 + (rex & 1) * 8, (ULONG64 *)context->Rsp );
            context->Rsp += sizeof(ULONG64);
            pc++;
            continue;
        case 0x81: /* add $nnnn,%rsp */
            context->Rsp += *(LONG *)(pc + 2);
            pc += 2 + sizeof(LONG);
            continue;
        case 0x83: /* add $n,%rsp */
            context->Rsp += (signed char)pc[2];
            pc += 3;
            continue;
        case 0x8d:
            if ((pc[1] >> 6) == 1)  /* lea n(reg),%rsp */
            {
                context->Rsp = get_int_reg( context, (pc[1] & 7) + (rex & 1) * 8 ) + (signed char)pc[2];
                pc += 3;
            }
            else  /* lea nnnn(reg),%rsp */
            {
                context->Rsp = get_int_reg( context, (pc[1] & 7) + (rex & 1) * 8 ) + *(LONG *)(pc + 2);
                pc += 2 + sizeof(LONG);
            }
            continue;
        case 0xc2: /* ret $nn */
            context->Rip = *(ULONG64 *)context->Rsp;
            context->Rsp += sizeof(ULONG64) + *(WORD *)(pc + 1);
            return;
        case 0xc3: /* ret */
        case 0xf3: /* rep; ret */
            context->Rip = *(ULONG64 *)context->Rsp;
            context->Rsp += sizeof(ULONG64);
            return;
        case 0xe9: /* jmp nnnn */
            pc += 5 + *(LONG *)(pc + 1);
            continue;
        case 0xeb: /* jmp n */
            pc += 2 + (signed char)pc[1];
            continue;
        }
        return;
    }
}

/**********************************************************************
 *              RtlVirtualUnwind   (NTDLL.@)
 */
PVOID WINAPI RtlVirtualUnwind( ULONG type, ULONG64 base, ULONG64 pc,
                               RUNTIME_FUNCTION *function, CONTEXT *context,
                               PVOID *data, ULONG64 *frame_ret,
                               KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr )
{
    union handler_data *handler_data;
    ULONG64 frame, off;
    struct UNWIND_INFO *info;
    unsigned int i, prolog_offset;
    BOOL mach_frame = FALSE;

    TRACE( "type %lx rip %I64x rsp %I64x\n", type, pc, context->Rsp );
    if (TRACE_ON(seh)) dump_unwind_info( base, function );

    frame = *frame_ret = context->Rsp;
    for (;;)
    {
        info = (struct UNWIND_INFO *)((char *)base + function->UnwindData);
        handler_data = (union handler_data *)&info->opcodes[(info->count + 1) & ~1];

        if (info->version != 1 && info->version != 2)
        {
            FIXME( "unknown unwind info version %u at %p\n", info->version, info );
            return NULL;
        }

        if (info->frame_reg)
            frame = get_int_reg( context, info->frame_reg ) - info->frame_offset * 16;

        /* check if in prolog */
        if (pc >= base + function->BeginAddress && pc < base + function->BeginAddress + info->prolog)
        {
            TRACE("inside prolog.\n");
            prolog_offset = pc - base - function->BeginAddress;
        }
        else
        {
            prolog_offset = ~0;
            /* Since Win10 1809 epilogue does not have a special treatment in case of zero opcode count. */
            if (info->count && is_inside_epilog( (BYTE *)pc, base, function ))
            {
                TRACE("inside epilog.\n");
                interpret_epilog( (BYTE *)pc, context, ctx_ptr );
                *frame_ret = frame;
                return NULL;
            }
        }

        for (i = 0; i < info->count; i += get_opcode_size(info->opcodes[i]))
        {
            if (prolog_offset < info->opcodes[i].offset) continue; /* skip it */

            switch (info->opcodes[i].code)
            {
            case UWOP_PUSH_NONVOL:  /* pushq %reg */
                set_int_reg( context, ctx_ptr, info->opcodes[i].info, (ULONG64 *)context->Rsp );
                context->Rsp += sizeof(ULONG64);
                break;
            case UWOP_ALLOC_LARGE:  /* subq $nn,%rsp */
                if (info->opcodes[i].info) context->Rsp += *(DWORD *)&info->opcodes[i+1];
                else context->Rsp += *(USHORT *)&info->opcodes[i+1] * 8;
                break;
            case UWOP_ALLOC_SMALL:  /* subq $n,%rsp */
                context->Rsp += (info->opcodes[i].info + 1) * 8;
                break;
            case UWOP_SET_FPREG:  /* leaq nn(%rsp),%framereg */
                context->Rsp = *frame_ret = frame;
                break;
            case UWOP_SAVE_NONVOL:  /* movq %reg,n(%rsp) */
                off = frame + *(USHORT *)&info->opcodes[i+1] * 8;
                set_int_reg( context, ctx_ptr, info->opcodes[i].info, (ULONG64 *)off );
                break;
            case UWOP_SAVE_NONVOL_FAR:  /* movq %reg,nn(%rsp) */
                off = frame + *(DWORD *)&info->opcodes[i+1];
                set_int_reg( context, ctx_ptr, info->opcodes[i].info, (ULONG64 *)off );
                break;
            case UWOP_SAVE_XMM128:  /* movaps %xmmreg,n(%rsp) */
                off = frame + *(USHORT *)&info->opcodes[i+1] * 16;
                set_float_reg( context, ctx_ptr, info->opcodes[i].info, (M128A *)off );
                break;
            case UWOP_SAVE_XMM128_FAR:  /* movaps %xmmreg,nn(%rsp) */
                off = frame + *(DWORD *)&info->opcodes[i+1];
                set_float_reg( context, ctx_ptr, info->opcodes[i].info, (M128A *)off );
                break;
            case UWOP_PUSH_MACHFRAME:
                if (info->flags & UNW_FLAG_CHAININFO)
                {
                    FIXME("PUSH_MACHFRAME with chained unwind info.\n");
                    break;
                }
                if (i + get_opcode_size(info->opcodes[i]) < info->count )
                {
                    FIXME("PUSH_MACHFRAME is not the last opcode.\n");
                    break;
                }

                if (info->opcodes[i].info)
                    context->Rsp += 0x8;

                context->Rip = *(ULONG64 *)context->Rsp;
                context->Rsp = *(ULONG64 *)(context->Rsp + 24);
                mach_frame = TRUE;
                break;
            case UWOP_EPILOG:
                if (info->version == 2)
                    break; /* nothing to do */
            default:
                FIXME( "unknown code %u\n", info->opcodes[i].code );
                break;
            }
        }

        if (!(info->flags & UNW_FLAG_CHAININFO)) break;
        function = &handler_data->chain;  /* restart with the chained info */
    }

    if (!mach_frame)
    {
        /* now pop return address */
        context->Rip = *(ULONG64 *)context->Rsp;
        context->Rsp += sizeof(ULONG64);
    }

    if (!(info->flags & type)) return NULL;  /* no matching handler */
    if (prolog_offset != ~0) return NULL;  /* inside prolog */

    *data = &handler_data->handler + 1;
    return (char *)base + handler_data->handler;
}

struct unwind_exception_frame
{
    EXCEPTION_REGISTRATION_RECORD frame;
    char dummy[0x10]; /* Layout 'dispatch' accessed from unwind_exception_handler() so it is above register
                       * save space when .seh handler is used. */
    DISPATCHER_CONTEXT *dispatch;
};

/**********************************************************************
 *           unwind_exception_handler
 *
 * Handler for exceptions happening while calling an unwind handler.
 */
DWORD __cdecl unwind_exception_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    struct unwind_exception_frame *unwind_frame = (struct unwind_exception_frame *)frame;
    DISPATCHER_CONTEXT *dispatch = (DISPATCHER_CONTEXT *)dispatcher;

    /* copy the original dispatcher into the current one, except for the TargetIp */
    dispatch->ControlPc        = unwind_frame->dispatch->ControlPc;
    dispatch->ImageBase        = unwind_frame->dispatch->ImageBase;
    dispatch->FunctionEntry    = unwind_frame->dispatch->FunctionEntry;
    dispatch->EstablisherFrame = unwind_frame->dispatch->EstablisherFrame;
    dispatch->ContextRecord    = unwind_frame->dispatch->ContextRecord;
    dispatch->LanguageHandler  = unwind_frame->dispatch->LanguageHandler;
    dispatch->HandlerData      = unwind_frame->dispatch->HandlerData;
    dispatch->HistoryTable     = unwind_frame->dispatch->HistoryTable;
    dispatch->ScopeIndex       = unwind_frame->dispatch->ScopeIndex;
    TRACE( "detected collided unwind\n" );
    return ExceptionCollidedUnwind;
}

/***********************************************************************
 *		unwind_handler_call_wrapper
 */
#ifdef __WINE_PE_BUILD
DWORD WINAPI unwind_handler_call_wrapper( EXCEPTION_RECORD *rec, void *frame,
                                   CONTEXT *context, DISPATCHER_CONTEXT *dispatch );

C_ASSERT( sizeof(struct unwind_exception_frame) == 0x28 );
C_ASSERT( offsetof(struct unwind_exception_frame, dispatch) == 0x20 );
C_ASSERT( offsetof(DISPATCHER_CONTEXT, LanguageHandler) == 0x30 );

__ASM_GLOBAL_FUNC( unwind_handler_call_wrapper,
                   "subq $0x28,%rsp\n\t"
                   ".seh_stackalloc 0x28\n\t"
                   ".seh_endprologue\n\t"
                   "movq %r9,0x20(%rsp)\n\t"   /* unwind_exception_frame->dispatch */
                   "callq *0x30(%r9)\n\t"      /* dispatch->LanguageHandler */
                   "nop\n\t"                   /* avoid epilogue so handler is called */
                   "addq $0x28, %rsp\n\t"
                   "ret\n\t"
                   ".seh_handler " __ASM_NAME("unwind_exception_handler") ", @except, @unwind\n\t" )
#else
static DWORD unwind_handler_call_wrapper( EXCEPTION_RECORD *rec, void *frame,
                                          CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    struct unwind_exception_frame wrapper_frame;
    DWORD res;

    wrapper_frame.frame.Handler = unwind_exception_handler;
    wrapper_frame.dispatch = dispatch;
    __wine_push_frame( &wrapper_frame.frame );
    res = dispatch->LanguageHandler( rec, (void *)dispatch->EstablisherFrame, dispatch->ContextRecord, dispatch );
    __wine_pop_frame( &wrapper_frame.frame );
    return res;
}
#endif

/**********************************************************************
 *           call_unwind_handler
 *
 * Call a single unwind handler.
 */
DWORD call_unwind_handler( EXCEPTION_RECORD *rec, DISPATCHER_CONTEXT *dispatch )
{
    DWORD res;

    TRACE( "calling handler %p (rec=%p, frame=%p context=%p, dispatch=%p)\n",
           dispatch->LanguageHandler, rec, (void *)dispatch->EstablisherFrame, dispatch->ContextRecord, dispatch );
    res = unwind_handler_call_wrapper( rec, (void *)dispatch->EstablisherFrame, dispatch->ContextRecord, dispatch );
    TRACE( "handler %p returned %lx\n", dispatch->LanguageHandler, res );

    switch (res)
    {
    case ExceptionContinueSearch:
    case ExceptionCollidedUnwind:
        break;
    default:
        raise_status( STATUS_INVALID_DISPOSITION, rec );
        break;
    }

    return res;
}


/**********************************************************************
 *           call_teb_unwind_handler
 *
 * Call a single unwind handler from the TEB chain.
 */
static DWORD call_teb_unwind_handler( EXCEPTION_RECORD *rec, DISPATCHER_CONTEXT *dispatch,
                                     EXCEPTION_REGISTRATION_RECORD *teb_frame )
{
    DWORD res;

    TRACE( "calling TEB handler %p (rec=%p, frame=%p context=%p, dispatch=%p)\n",
           teb_frame->Handler, rec, teb_frame, dispatch->ContextRecord, dispatch );
    res = teb_frame->Handler( rec, teb_frame, dispatch->ContextRecord, (EXCEPTION_REGISTRATION_RECORD**)dispatch );
    TRACE( "handler at %p returned %lu\n", teb_frame->Handler, res );

    switch (res)
    {
    case ExceptionContinueSearch:
    case ExceptionCollidedUnwind:
        break;
    default:
        raise_status( STATUS_INVALID_DISPOSITION, rec );
        break;
    }

    return res;
}


/**********************************************************************
 *           call_consolidate_callback
 *
 * Wrapper function to call a consolidate callback from a fake frame.
 * If the callback executes RtlUnwindEx (like for example done in C++ handlers),
 * we have to skip all frames which were already processed. To do that we
 * trick the unwinding functions into thinking the call came from the specified
 * context. All CFI instructions are either DW_CFA_def_cfa_expression or
 * DW_CFA_expression, and the expressions have the following format:
 *
 * DW_OP_breg6; sleb128 0x10            | Load %rbp + 0x10
 * DW_OP_deref                          | Get *(%rbp + 0x10) == context
 * DW_OP_plus_uconst; uleb128 <OFFSET>  | Add offset to get struct member
 * [DW_OP_deref]                        | Dereference, only for CFA
 */
extern void * WINAPI call_consolidate_callback( CONTEXT *context,
                                                void *(CALLBACK *callback)(EXCEPTION_RECORD *),
                                                EXCEPTION_RECORD *rec );
__ASM_GLOBAL_FUNC( call_consolidate_callback,
                   "pushq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")

                   /* Setup SEH machine frame. */
                   "subq $0x28,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 0x28\n\t")
                   "movq 0xf8(%rcx),%rax\n\t" /* Context->Rip */
                   "movq %rax,(%rsp)\n\t"
                   "movq 0x98(%rcx),%rax\n\t" /* context->Rsp */
                   "movq %rax,0x18(%rsp)\n\t"
                   __ASM_SEH(".seh_pushframe\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")

                   "subq $0x108,%rsp\n\t" /* 10*16 (float regs) + 8*8 (int regs) + 32 (shadow store) + 8 (align). */
                   __ASM_SEH(".seh_stackalloc 0x108\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 0x108\n\t")

                   /* Setup CFI unwind to context. */
                   "movq %rcx,0x10(%rbp)\n\t"
                   __ASM_CFI(".cfi_remember_state\n\t")
                   __ASM_CFI(".cfi_escape 0x0f,0x07,0x76,0x10,0x06,0x23,0x98,0x01,0x06\n\t") /* CFA    */
                   __ASM_CFI(".cfi_escape 0x10,0x03,0x06,0x76,0x10,0x06,0x23,0x90,0x01\n\t") /* %rbx   */
                   __ASM_CFI(".cfi_escape 0x10,0x04,0x06,0x76,0x10,0x06,0x23,0xa8,0x01\n\t") /* %rsi   */
                   __ASM_CFI(".cfi_escape 0x10,0x05,0x06,0x76,0x10,0x06,0x23,0xb0,0x01\n\t") /* %rdi   */
                   __ASM_CFI(".cfi_escape 0x10,0x06,0x06,0x76,0x10,0x06,0x23,0xa0,0x01\n\t") /* %rbp   */
                   __ASM_CFI(".cfi_escape 0x10,0x0c,0x06,0x76,0x10,0x06,0x23,0xd8,0x01\n\t") /* %r12   */
                   __ASM_CFI(".cfi_escape 0x10,0x0d,0x06,0x76,0x10,0x06,0x23,0xe0,0x01\n\t") /* %r13   */
                   __ASM_CFI(".cfi_escape 0x10,0x0e,0x06,0x76,0x10,0x06,0x23,0xe8,0x01\n\t") /* %r14   */
                   __ASM_CFI(".cfi_escape 0x10,0x0f,0x06,0x76,0x10,0x06,0x23,0xf0,0x01\n\t") /* %r15   */
                   __ASM_CFI(".cfi_escape 0x10,0x10,0x06,0x76,0x10,0x06,0x23,0xf8,0x01\n\t") /* %rip   */
                   __ASM_CFI(".cfi_escape 0x10,0x17,0x06,0x76,0x10,0x06,0x23,0x80,0x04\n\t") /* %xmm6  */
                   __ASM_CFI(".cfi_escape 0x10,0x18,0x06,0x76,0x10,0x06,0x23,0x90,0x04\n\t") /* %xmm7  */
                   __ASM_CFI(".cfi_escape 0x10,0x19,0x06,0x76,0x10,0x06,0x23,0xa0,0x04\n\t") /* %xmm8  */
                   __ASM_CFI(".cfi_escape 0x10,0x1a,0x06,0x76,0x10,0x06,0x23,0xb0,0x04\n\t") /* %xmm9  */
                   __ASM_CFI(".cfi_escape 0x10,0x1b,0x06,0x76,0x10,0x06,0x23,0xc0,0x04\n\t") /* %xmm10 */
                   __ASM_CFI(".cfi_escape 0x10,0x1c,0x06,0x76,0x10,0x06,0x23,0xd0,0x04\n\t") /* %xmm11 */
                   __ASM_CFI(".cfi_escape 0x10,0x1d,0x06,0x76,0x10,0x06,0x23,0xe0,0x04\n\t") /* %xmm12 */
                   __ASM_CFI(".cfi_escape 0x10,0x1e,0x06,0x76,0x10,0x06,0x23,0xf0,0x04\n\t") /* %xmm13 */
                   __ASM_CFI(".cfi_escape 0x10,0x1f,0x06,0x76,0x10,0x06,0x23,0x80,0x05\n\t") /* %xmm14 */
                   __ASM_CFI(".cfi_escape 0x10,0x20,0x06,0x76,0x10,0x06,0x23,0x90,0x05\n\t") /* %xmm15 */

                   /* Setup SEH unwind registers restore. */
                   "movq 0xa0(%rcx),%rax\n\t" /* context->Rbp */
                   "movq %rax,0x100(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %rbp, 0x100\n\t")
                   "movq 0x90(%rcx),%rax\n\t" /* context->Rbx */
                   "movq %rax,0x20(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %rbx, 0x20\n\t")
                   "movq 0xa8(%rcx),%rax\n\t" /* context->Rsi */
                   "movq %rax,0x28(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %rsi, 0x28\n\t")
                   "movq 0xb0(%rcx),%rax\n\t" /* context->Rdi */
                   "movq %rax,0x30(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %rdi, 0x30\n\t")

                   "movq 0xd8(%rcx),%rax\n\t" /* context->R12 */
                   "movq %rax,0x38(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %r12, 0x38\n\t")
                   "movq 0xe0(%rcx),%rax\n\t" /* context->R13 */
                   "movq %rax,0x40(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %r13, 0x40\n\t")
                   "movq 0xe8(%rcx),%rax\n\t" /* context->R14 */
                   "movq %rax,0x48(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %r14, 0x48\n\t")
                   "movq 0xf0(%rcx),%rax\n\t" /* context->R15 */
                   "movq %rax,0x50(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %r15, 0x50\n\t")
                   "pushq %rsi\n\t"
                   "pushq %rdi\n\t"
                   "leaq 0x200(%rcx),%rsi\n\t"
                   "leaq 0x70(%rsp),%rdi\n\t"
                   "movq $0x14,%rcx\n\t"
                   "cld\n\t"
                   "rep; movsq\n\t"
                   "popq %rdi\n\t"
                   "popq %rsi\n\t"
                   __ASM_SEH(".seh_savexmm %xmm6, 0x60\n\t")
                   __ASM_SEH(".seh_savexmm %xmm7, 0x70\n\t")
                   __ASM_SEH(".seh_savexmm %xmm8, 0x80\n\t")
                   __ASM_SEH(".seh_savexmm %xmm9, 0x90\n\t")
                   __ASM_SEH(".seh_savexmm %xmm10, 0xa0\n\t")
                   __ASM_SEH(".seh_savexmm %xmm11, 0xb0\n\t")
                   __ASM_SEH(".seh_savexmm %xmm12, 0xc0\n\t")
                   __ASM_SEH(".seh_savexmm %xmm13, 0xd0\n\t")
                   __ASM_SEH(".seh_savexmm %xmm14, 0xe0\n\t")
                   __ASM_SEH(".seh_savexmm %xmm15, 0xf0\n\t")

                   /* call the callback. */
                   "movq %r8,%rcx\n\t"
                   "callq *%rdx\n\t"
                   __ASM_CFI(".cfi_restore_state\n\t")
                   "nop\n\t" /* Otherwise RtlVirtualUnwind() will think we are inside epilogue and
                              * interpret / execute the rest of opcodes here instead of unwind through
                              * machine frame. */
                   "leaq 0(%rbp),%rsp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rsp\n\t")
                   "popq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "ret")

/*******************************************************************
 *              RtlRestoreContext (NTDLL.@)
 */
void CDECL RtlRestoreContext( CONTEXT *context, EXCEPTION_RECORD *rec )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;

    if (rec && rec->ExceptionCode == STATUS_LONGJUMP && rec->NumberParameters >= 1)
    {
        struct MSVCRT_JUMP_BUFFER *jmp = (struct MSVCRT_JUMP_BUFFER *)rec->ExceptionInformation[0];
        context->Rbx   = jmp->Rbx;
        context->Rsp   = jmp->Rsp;
        context->Rbp   = jmp->Rbp;
        context->Rsi   = jmp->Rsi;
        context->Rdi   = jmp->Rdi;
        context->R12   = jmp->R12;
        context->R13   = jmp->R13;
        context->R14   = jmp->R14;
        context->R15   = jmp->R15;
        context->Rip   = jmp->Rip;
        context->Xmm6  = jmp->Xmm6;
        context->Xmm7  = jmp->Xmm7;
        context->Xmm8  = jmp->Xmm8;
        context->Xmm9  = jmp->Xmm9;
        context->Xmm10 = jmp->Xmm10;
        context->Xmm11 = jmp->Xmm11;
        context->Xmm12 = jmp->Xmm12;
        context->Xmm13 = jmp->Xmm13;
        context->Xmm14 = jmp->Xmm14;
        context->Xmm15 = jmp->Xmm15;
        context->MxCsr = jmp->MxCsr;
        context->FltSave.MxCsr = jmp->MxCsr;
        context->FltSave.ControlWord = jmp->FpCsr;
    }
    else if (rec && rec->ExceptionCode == STATUS_UNWIND_CONSOLIDATE && rec->NumberParameters >= 1)
    {
        PVOID (CALLBACK *consolidate)(EXCEPTION_RECORD *) = (void *)rec->ExceptionInformation[0];
        TRACE_(seh)( "calling consolidate callback %p (rec=%p)\n", consolidate, rec );
        context->Rip = (ULONG64)call_consolidate_callback( context, consolidate, rec );
    }

    /* hack: remove no longer accessible TEB frames */
    while (is_valid_frame( (ULONG_PTR)teb_frame ) && (ULONG64)teb_frame < context->Rsp)
    {
        TRACE_(seh)( "removing TEB frame: %p\n", teb_frame );
        teb_frame = __wine_pop_frame( teb_frame );
    }

    TRACE_(seh)( "returning to %p stack %p\n", (void *)context->Rip, (void *)context->Rsp );
    NtContinue( context, FALSE );
}


/*******************************************************************
 *		RtlUnwindEx (NTDLL.@)
 */
void WINAPI RtlUnwindEx( PVOID end_frame, PVOID target_ip, EXCEPTION_RECORD *rec,
                         PVOID retval, CONTEXT *context, UNWIND_HISTORY_TABLE *table )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    EXCEPTION_RECORD record;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT new_context;
    NTSTATUS status;
    DWORD i;

    RtlCaptureContext( context );
    new_context = *context;

    /* build an exception record, if we do not have one */
    if (!rec)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = (void *)context->Rip;
        record.NumberParameters = 0;
        rec = &record;
    }

    rec->ExceptionFlags |= EH_UNWINDING | (end_frame ? 0 : EH_EXIT_UNWIND);

    TRACE( "code=%lx flags=%lx end_frame=%p target_ip=%p rip=%016I64x\n",
           rec->ExceptionCode, rec->ExceptionFlags, end_frame, target_ip, context->Rip );
    for (i = 0; i < min( EXCEPTION_MAXIMUM_PARAMETERS, rec->NumberParameters ); i++)
        TRACE( " info[%ld]=%016I64x\n", i, rec->ExceptionInformation[i] );
    TRACE(" rax=%016I64x rbx=%016I64x rcx=%016I64x rdx=%016I64x\n",
          context->Rax, context->Rbx, context->Rcx, context->Rdx );
    TRACE(" rsi=%016I64x rdi=%016I64x rbp=%016I64x rsp=%016I64x\n",
          context->Rsi, context->Rdi, context->Rbp, context->Rsp );
    TRACE("  r8=%016I64x  r9=%016I64x r10=%016I64x r11=%016I64x\n",
          context->R8, context->R9, context->R10, context->R11 );
    TRACE(" r12=%016I64x r13=%016I64x r14=%016I64x r15=%016I64x\n",
          context->R12, context->R13, context->R14, context->R15 );

    dispatch.EstablisherFrame = context->Rsp;
    dispatch.TargetIp         = (ULONG64)target_ip;
    dispatch.ContextRecord    = context;
    dispatch.HistoryTable     = table;

    for (;;)
    {
        status = virtual_unwind( UNW_FLAG_UHANDLER, &dispatch, &new_context );
        if (status != STATUS_SUCCESS) raise_status( status, rec );

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (!is_valid_frame( dispatch.EstablisherFrame ))
        {
            ERR( "invalid frame %p (%p-%p)\n", (void *)dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            if (end_frame && (dispatch.EstablisherFrame > (ULONG64)end_frame))
            {
                ERR( "invalid end frame %p/%p\n", (void *)dispatch.EstablisherFrame, end_frame );
                raise_status( STATUS_INVALID_UNWIND_TARGET, rec );
            }
            if (dispatch.EstablisherFrame == (ULONG64)end_frame) rec->ExceptionFlags |= EH_TARGET_UNWIND;
            if (call_unwind_handler( rec, &dispatch ) == ExceptionCollidedUnwind)
            {
                ULONG64 frame;

                new_context = *dispatch.ContextRecord;
                new_context.ContextFlags &= ~0x40;
                *context = new_context;
                dispatch.ContextRecord = context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                        dispatch.ControlPc, dispatch.FunctionEntry,
                        &new_context, NULL, &frame, NULL );
                rec->ExceptionFlags |= EH_COLLIDED_UNWIND;
                goto unwind_done;
            }
            rec->ExceptionFlags &= ~EH_COLLIDED_UNWIND;
        }
        else  /* hack: call builtin handlers registered in the tib list */
        {
            DWORD64 backup_frame = dispatch.EstablisherFrame;
            while (is_valid_frame( (ULONG_PTR)teb_frame ) &&
                   (ULONG64)teb_frame < new_context.Rsp &&
                   (ULONG64)teb_frame < (ULONG64)end_frame)
            {
                TRACE( "found builtin frame %p handler %p\n", teb_frame, teb_frame->Handler );
                dispatch.EstablisherFrame = (ULONG64)teb_frame;
                if (call_teb_unwind_handler( rec, &dispatch, teb_frame ) == ExceptionCollidedUnwind)
                {
                    ULONG64 frame;

                    teb_frame = __wine_pop_frame( teb_frame );

                    new_context = *dispatch.ContextRecord;
                    new_context.ContextFlags &= ~0x40;
                    *context = new_context;
                    dispatch.ContextRecord = context;
                    RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                            dispatch.ControlPc, dispatch.FunctionEntry,
                            &new_context, NULL, &frame, NULL );
                    rec->ExceptionFlags |= EH_COLLIDED_UNWIND;
                    goto unwind_done;
                }
                teb_frame = __wine_pop_frame( teb_frame );
            }
            if ((ULONG64)teb_frame == (ULONG64)end_frame && (ULONG64)end_frame < new_context.Rsp) break;
            dispatch.EstablisherFrame = backup_frame;
        }

        if (dispatch.EstablisherFrame == (ULONG64)end_frame) break;
        *context = new_context;
    }

    context->Rax = (ULONG64)retval;
    context->Rip = (ULONG64)target_ip;
    RtlRestoreContext(context, rec);
}


/*******************************************************************
 *		RtlUnwind (NTDLL.@)
 */
void WINAPI RtlUnwind( void *frame, void *target_ip, EXCEPTION_RECORD *rec, void *retval )
{
    CONTEXT context;
    RtlUnwindEx( frame, target_ip, rec, retval, &context, NULL );
}


/*******************************************************************
 *		_local_unwind (NTDLL.@)
 */
void WINAPI _local_unwind( void *frame, void *target_ip )
{
    CONTEXT context;
    RtlUnwindEx( frame, target_ip, NULL, NULL, &context, NULL );
}

/*******************************************************************
 *		__C_specific_handler (NTDLL.@)
 */
EXCEPTION_DISPOSITION WINAPI __C_specific_handler( EXCEPTION_RECORD *rec,
                                                   void *frame,
                                                   CONTEXT *context,
                                                   struct _DISPATCHER_CONTEXT *dispatch )
{
    SCOPE_TABLE *table = dispatch->HandlerData;
    ULONG i;

    TRACE_(seh)( "%p %p %p %p\n", rec, frame, context, dispatch );
    if (TRACE_ON(seh)) dump_scope_table( dispatch->ImageBase, table );

    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
    {
        for (i = dispatch->ScopeIndex; i < table->Count; i++)
        {
            if (dispatch->ControlPc >= dispatch->ImageBase + table->ScopeRecord[i].BeginAddress &&
                dispatch->ControlPc < dispatch->ImageBase + table->ScopeRecord[i].EndAddress)
            {
                PTERMINATION_HANDLER handler;

                if (table->ScopeRecord[i].JumpTarget) continue;

                if (rec->ExceptionFlags & EH_TARGET_UNWIND &&
                    dispatch->TargetIp >= dispatch->ImageBase + table->ScopeRecord[i].BeginAddress &&
                    dispatch->TargetIp < dispatch->ImageBase + table->ScopeRecord[i].EndAddress)
                {
                    break;
                }

                handler = (PTERMINATION_HANDLER)(dispatch->ImageBase + table->ScopeRecord[i].HandlerAddress);
                dispatch->ScopeIndex = i+1;

                TRACE_(seh)( "calling __finally %p frame %p\n", handler, frame );
                handler( TRUE, frame );
            }
        }
        return ExceptionContinueSearch;
    }

    for (i = dispatch->ScopeIndex; i < table->Count; i++)
    {
        if (dispatch->ControlPc >= dispatch->ImageBase + table->ScopeRecord[i].BeginAddress &&
            dispatch->ControlPc < dispatch->ImageBase + table->ScopeRecord[i].EndAddress)
        {
            if (!table->ScopeRecord[i].JumpTarget) continue;
            if (table->ScopeRecord[i].HandlerAddress != EXCEPTION_EXECUTE_HANDLER)
            {
                EXCEPTION_POINTERS ptrs;
                PEXCEPTION_FILTER filter;

                filter = (PEXCEPTION_FILTER)(dispatch->ImageBase + table->ScopeRecord[i].HandlerAddress);
                ptrs.ExceptionRecord = rec;
                ptrs.ContextRecord = context;
                TRACE_(seh)( "calling filter %p ptrs %p frame %p\n", filter, &ptrs, frame );
                switch (filter( &ptrs, frame ))
                {
                case EXCEPTION_EXECUTE_HANDLER:
                    break;
                case EXCEPTION_CONTINUE_SEARCH:
                    continue;
                case EXCEPTION_CONTINUE_EXECUTION:
                    return ExceptionContinueExecution;
                }
            }
            TRACE( "unwinding to target %p\n", (char *)dispatch->ImageBase + table->ScopeRecord[i].JumpTarget );
            RtlUnwindEx( frame, (char *)dispatch->ImageBase + table->ScopeRecord[i].JumpTarget,
                         rec, 0, dispatch->ContextRecord, dispatch->HistoryTable );
        }
    }
    return ExceptionContinueSearch;
}


/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( RtlRaiseException,
                   "sub $0x4f8,%rsp\n\t"
                   __ASM_SEH(".seh_stackalloc 0x4f8\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 0x4f8\n\t")
                   "movq %rcx,0x500(%rsp)\n\t"
                   "leaq 0x20(%rsp),%rcx\n\t"
                   "call " __ASM_NAME("RtlCaptureContext") "\n\t"
                   "leaq 0x20(%rsp),%rdx\n\t"   /* context pointer */
                   "leaq 0x500(%rsp),%rax\n\t"  /* orig stack pointer */
                   "movq %rax,0x98(%rdx)\n\t"   /* context->Rsp */
                   "movq (%rax),%rcx\n\t"       /* original first parameter */
                   "movq %rcx,0x80(%rdx)\n\t"   /* context->Rcx */
                   "movq 0x4f8(%rsp),%rax\n\t"  /* return address */
                   "movq %rax,0xf8(%rdx)\n\t"   /* context->Rip */
                   "movq %rax,0x10(%rcx)\n\t"   /* rec->ExceptionAddress */
                   "movl $1,%r8d\n\t"
                   "movq %gs:(0x30),%rax\n\t"   /* Teb */
                   "movq 0x60(%rax),%rax\n\t"   /* Peb */
                   "cmpb $0,0x02(%rax)\n\t"     /* BeingDebugged */
                   "jne 1f\n\t"
                   "call " __ASM_NAME("dispatch_exception") "\n"
                   "1:\tcall " __ASM_NAME("NtRaiseException") "\n\t"
                   "movq %rax,%rcx\n\t"
                   "call " __ASM_NAME("RtlRaiseStatus") /* does not return */ );


static inline ULONG hash_pointers( void **ptrs, ULONG count )
{
    /* Based on MurmurHash2, which is in the public domain */
    static const ULONG m = 0x5bd1e995;
    static const ULONG r = 24;
    ULONG hash = count * sizeof(void*);
    for (; count > 0; ptrs++, count--)
    {
        ULONG_PTR data = (ULONG_PTR)*ptrs;
        ULONG k1 = (ULONG)(data & 0xffffffff), k2 = (ULONG)(data >> 32);
        k1 *= m;
        k1 = (k1 ^ (k1 >> r)) * m;
        k2 *= m;
        k2 = (k2 ^ (k2 >> r)) * m;
        hash = (((hash * m) ^ k1) * m) ^ k2;
    }
    hash = (hash ^ (hash >> 13)) * m;
    return hash ^ (hash >> 15);
}


/*************************************************************************
 *		RtlCaptureStackBackTrace (NTDLL.@)
 */
USHORT WINAPI RtlCaptureStackBackTrace( ULONG skip, ULONG count, PVOID *buffer, ULONG *hash )
{
    UNWIND_HISTORY_TABLE table;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT context;
    NTSTATUS status;
    ULONG i;
    USHORT num_entries = 0;

    TRACE( "(%lu, %lu, %p, %p)\n", skip, count, buffer, hash );

    RtlCaptureContext( &context );
    dispatch.TargetIp      = 0;
    dispatch.ContextRecord = &context;
    dispatch.HistoryTable  = &table;
    if (hash) *hash = 0;
    for (i = 0; i < skip + count; i++)
    {
        status = virtual_unwind( UNW_FLAG_NHANDLER, &dispatch, &context );
        if (status != STATUS_SUCCESS) return i;

        if (!dispatch.EstablisherFrame) break;

        if (!is_valid_frame( dispatch.EstablisherFrame ))
        {
            ERR( "invalid frame %p (%p-%p)\n", (void *)dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            break;
        }

        if (context.Rsp == (ULONG64)NtCurrentTeb()->Tib.StackBase) break;

        if (i >= skip) buffer[num_entries++] = (void *)context.Rip;
    }
    if (hash && num_entries > 0) *hash = hash_pointers( buffer, num_entries );
    TRACE( "captured %hu frames\n", num_entries );
    return num_entries;
}


/***********************************************************************
 *           RtlUserThreadStart (NTDLL.@)
 */
#ifdef __WINE_PE_BUILD
__ASM_GLOBAL_FUNC( RtlUserThreadStart,
                   "subq $0x28,%rsp\n\t"
                   ".seh_stackalloc 0x28\n\t"
                   ".seh_endprologue\n\t"
                   "movq %rdx,%r8\n\t"
                   "movq %rcx,%rdx\n\t"
                   "xorq %rcx,%rcx\n\t"
                   "movq " __ASM_NAME( "pBaseThreadInitThunk" ) "(%rip),%r9\n\t"
                   "call *%r9\n\t"
                   "int3\n\t"
                   ".seh_handler " __ASM_NAME("call_unhandled_exception_handler") ", @except" )
#else
void WINAPI RtlUserThreadStart( PRTL_THREAD_START_ROUTINE entry, void *arg )
{
    __TRY
    {
        pBaseThreadInitThunk( 0, (LPTHREAD_START_ROUTINE)entry, arg );
    }
    __EXCEPT(call_unhandled_exception_filter)
    {
        NtTerminateProcess( GetCurrentProcess(), GetExceptionCode() );
    }
    __ENDTRY
}
#endif


/***********************************************************************
 *           signal_start_thread
 */
extern void CDECL DECLSPEC_NORETURN signal_start_thread( CONTEXT *ctx );
__ASM_GLOBAL_FUNC( signal_start_thread,
                   "movq %rcx,%rbx\n\t"        /* context */
                   /* clear the thread stack */
                   "andq $~0xfff,%rcx\n\t"     /* round down to page size */
                   "leaq -0xf0000(%rcx),%rdi\n\t"
                   "movq %rdi,%rsp\n\t"
                   "subq %rdi,%rcx\n\t"
                   "xorl %eax,%eax\n\t"
                   "shrq $3,%rcx\n\t"
                   "rep; stosq\n\t"
                   /* switch to the initial context */
                   "leaq -32(%rbx),%rsp\n\t"
                   "movq %rbx,%rcx\n\t"
                   "movl $1,%edx\n\t"
                   "call " __ASM_NAME("NtContinue") )


/******************************************************************
 *		LdrInitializeThunk (NTDLL.@)
 */
void WINAPI LdrInitializeThunk( CONTEXT *context, ULONG_PTR unk2, ULONG_PTR unk3, ULONG_PTR unk4 )
{
    loader_init( context, (void **)&context->Rcx );
    TRACE_(relay)( "\1Starting thread proc %p (arg=%p)\n", (void *)context->Rcx, (void *)context->Rdx );
    signal_start_thread( context );
}


/***********************************************************************
 *           process_breakpoint
 */
#ifdef __WINE_PE_BUILD
__ASM_GLOBAL_FUNC( process_breakpoint,
                   ".seh_endprologue\n\t"
                   ".seh_handler process_breakpoint_handler, @except\n\t"
                   "int $3\n\t"
                   "ret\n"
                   "process_breakpoint_handler:\n\t"
                   "incq 0xf8(%r8)\n\t"  /* context->Rip */
                   "xorl %eax,%eax\n\t"  /* ExceptionContinueExecution */
                   "ret" )
#else
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
#endif


/***********************************************************************
 *		DbgUiRemoteBreakin   (NTDLL.@)
 */
#ifdef __WINE_PE_BUILD
__ASM_GLOBAL_FUNC( DbgUiRemoteBreakin,
                   "subq $0x28,%rsp\n\t"
                   ".seh_stackalloc 0x28\n\t"
                   ".seh_endprologue\n\t"
                   ".seh_handler DbgUiRemoteBreakin_handler, @except\n\t"
                   "mov %gs:0x30,%rax\n\t"
                   "mov 0x60(%rax),%rax\n\t"
                   "cmpb $0,2(%rax)\n\t"
                   "je 1f\n\t"
                   "call " __ASM_NAME("DbgBreakPoint") "\n"
                   "1:\txorl %ecx,%ecx\n\t"
                   "call " __ASM_NAME("RtlExitUserThread") "\n"
                   "DbgUiRemoteBreakin_handler:\n\t"
                   "movq %rdx,%rsp\n\t"  /* frame */
                   "jmp 1b" )
#else
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
#endif

/**********************************************************************
 *		DbgBreakPoint   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgBreakPoint, "int $3; ret"
                    "\n\tnop; nop; nop; nop; nop; nop; nop; nop"
                    "\n\tnop; nop; nop; nop; nop; nop" );

/**********************************************************************
 *		DbgUserBreakPoint   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgUserBreakPoint, "int $3; ret"
                    "\n\tnop; nop; nop; nop; nop; nop; nop; nop"
                    "\n\tnop; nop; nop; nop; nop; nop" );

#endif  /* __x86_64__ */
