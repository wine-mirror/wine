/*
 * NT exception handling routines
 *
 * Copyright 1999 Turchanov Sergey
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

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "wine/exception.h"
#include "wine/list.h"
#include "wine/debug.h"
#include "excpt.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(threadname);

typedef struct
{
    struct list                 entry;
    PVECTORED_EXCEPTION_HANDLER func;
    ULONG                       count;
} VECTORED_HANDLER;

static struct list vectored_exception_handlers = LIST_INIT(vectored_exception_handlers);
static struct list vectored_continue_handlers  = LIST_INIT(vectored_continue_handlers);

static RTL_CRITICAL_SECTION vectored_handlers_section;
static RTL_CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &vectored_handlers_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": vectored_handlers_section") }
};
static RTL_CRITICAL_SECTION vectored_handlers_section = { &critsect_debug, -1, 0, 0, 0, 0 };

static PRTL_EXCEPTION_FILTER unhandled_exception_filter;

static const char *debugstr_exception_code( DWORD code )
{
    switch (code)
    {
    case CONTROL_C_EXIT: return "CONTROL_C_EXIT";
    case DBG_CONTROL_C: return "DBG_CONTROL_C";
    case DBG_PRINTEXCEPTION_C: return "DBG_PRINTEXCEPTION_C";
    case DBG_PRINTEXCEPTION_WIDE_C: return "DBG_PRINTEXCEPTION_WIDE_C";
    case EXCEPTION_ACCESS_VIOLATION: return "EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT: return "EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_FLT_DENORMAL_OPERAND: return "EXCEPTION_FLT_DENORMAL_OPERAND";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_INEXACT_RESULT: return "EXCEPTION_FLT_INEXACT_RESULT";
    case EXCEPTION_FLT_INVALID_OPERATION: return "EXCEPTION_FLT_INVALID_OPERATION";
    case EXCEPTION_FLT_OVERFLOW: return "EXCEPTION_FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK: return "EXCEPTION_FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW: return "EXCEPTION_FLT_UNDERFLOW";
    case EXCEPTION_GUARD_PAGE: return "EXCEPTION_GUARD_PAGE";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "EXCEPTION_ILLEGAL_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR: return "EXCEPTION_IN_PAGE_ERROR";
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW: return "EXCEPTION_INT_OVERFLOW";
    case EXCEPTION_INVALID_DISPOSITION: return "EXCEPTION_INVALID_DISPOSITION";
    case EXCEPTION_INVALID_HANDLE: return "EXCEPTION_INVALID_HANDLE";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
    case EXCEPTION_PRIV_INSTRUCTION: return "EXCEPTION_PRIV_INSTRUCTION";
    case EXCEPTION_SINGLE_STEP: return "EXCEPTION_SINGLE_STEP";
    case EXCEPTION_STACK_OVERFLOW: return "EXCEPTION_STACK_OVERFLOW";
    case EXCEPTION_WINE_ASSERTION: return "EXCEPTION_WINE_ASSERTION";
    case EXCEPTION_WINE_CXX_EXCEPTION: return "EXCEPTION_WINE_CXX_EXCEPTION";
    case EXCEPTION_WINE_NAME_THREAD: return "EXCEPTION_WINE_NAME_THREAD";
    case EXCEPTION_WINE_STUB: return "EXCEPTION_WINE_STUB";
    case RPC_S_SERVER_UNAVAILABLE: return "RPC_S_SERVER_UNAVAILABLE";
    }
    return "unknown";
}


static VECTORED_HANDLER *add_vectored_handler( struct list *handler_list, ULONG first,
                                               PVECTORED_EXCEPTION_HANDLER func )
{
    VECTORED_HANDLER *handler = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*handler) );
    if (handler)
    {
        handler->func = RtlEncodePointer( func );
        handler->count = 1;
        RtlEnterCriticalSection( &vectored_handlers_section );
        if (first) list_add_head( handler_list, &handler->entry );
        else list_add_tail( handler_list, &handler->entry );
        RtlLeaveCriticalSection( &vectored_handlers_section );
    }
    return handler;
}


static ULONG remove_vectored_handler( struct list *handler_list, VECTORED_HANDLER *handler )
{
    struct list *ptr;
    ULONG ret = FALSE;

    RtlEnterCriticalSection( &vectored_handlers_section );
    LIST_FOR_EACH( ptr, handler_list )
    {
        VECTORED_HANDLER *curr_handler = LIST_ENTRY( ptr, VECTORED_HANDLER, entry );
        if (curr_handler == handler)
        {
            if (!--curr_handler->count) list_remove( ptr );
            else handler = NULL;  /* don't free it yet */
            ret = TRUE;
            break;
        }
    }
    RtlLeaveCriticalSection( &vectored_handlers_section );
    if (ret) RtlFreeHeap( GetProcessHeap(), 0, handler );
    return ret;
}


/**********************************************************************
 *           call_vectored_handlers
 *
 * Call the vectored handlers chain.
 */
static LONG call_vectored_handlers( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    struct list *ptr;
    LONG ret = EXCEPTION_CONTINUE_SEARCH;
    EXCEPTION_POINTERS except_ptrs;
    PVECTORED_EXCEPTION_HANDLER func;
    VECTORED_HANDLER *handler, *to_free = NULL;

    except_ptrs.ExceptionRecord = rec;
    except_ptrs.ContextRecord = context;

    RtlEnterCriticalSection( &vectored_handlers_section );
    ptr = list_head( &vectored_exception_handlers );
    while (ptr)
    {
        handler = LIST_ENTRY( ptr, VECTORED_HANDLER, entry );
        handler->count++;
        func = RtlDecodePointer( handler->func );
        RtlLeaveCriticalSection( &vectored_handlers_section );
        RtlFreeHeap( GetProcessHeap(), 0, to_free );
        to_free = NULL;

        TRACE( "calling handler at %p code=%lx flags=%lx\n",
               func, rec->ExceptionCode, rec->ExceptionFlags );
        ret = func( &except_ptrs );
        TRACE( "handler at %p returned %lx\n", func, ret );

        RtlEnterCriticalSection( &vectored_handlers_section );
        ptr = list_next( &vectored_exception_handlers, ptr );
        if (!--handler->count)  /* removed during execution */
        {
            list_remove( &handler->entry );
            to_free = handler;
        }
        if (ret == EXCEPTION_CONTINUE_EXECUTION) break;
    }
    RtlLeaveCriticalSection( &vectored_handlers_section );
    RtlFreeHeap( GetProcessHeap(), 0, to_free );
    return ret;
}


/*******************************************************************
 *		dispatch_exception
 */
NTSTATUS WINAPI dispatch_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status;
    DWORD i;

    switch (rec->ExceptionCode)
    {
    case EXCEPTION_WINE_STUB:
        if (rec->ExceptionInformation[1] >> 16)
            MESSAGE( "wine: Call from %p to unimplemented function %s.%s, aborting\n",
                     rec->ExceptionAddress,
                     (char *)rec->ExceptionInformation[0], (char *)rec->ExceptionInformation[1] );
        else
            MESSAGE( "wine: Call from %p to unimplemented function %s.%u, aborting\n",
                     rec->ExceptionAddress,
                     (char *)rec->ExceptionInformation[0], (USHORT)rec->ExceptionInformation[1] );
        break;

    case EXCEPTION_WINE_NAME_THREAD:
        if (rec->ExceptionInformation[0] == 0x1000)
        {
            const char *name = (char *)rec->ExceptionInformation[1];
            DWORD tid = (DWORD)rec->ExceptionInformation[2];

            if (tid == -1 || tid == GetCurrentThreadId())
                WARN_(threadname)( "Thread renamed to %s\n", debugstr_a(name) );
            else
                WARN_(threadname)( "Thread ID %04lx renamed to %s\n", tid, debugstr_a(name) );
            set_native_thread_name( tid, name );
        }
        break;

    case DBG_PRINTEXCEPTION_C:
        WARN( "%s\n", debugstr_an((char *)rec->ExceptionInformation[1], rec->ExceptionInformation[0] - 1) );
        break;

    case DBG_PRINTEXCEPTION_WIDE_C:
        WARN( "%s\n", debugstr_wn((WCHAR *)rec->ExceptionInformation[1], rec->ExceptionInformation[0] - 1) );
        break;

    case STATUS_ASSERTION_FAILURE:
        ERR( "assertion failure exception\n" );
        break;

    default:
        if (!TRACE_ON(seh)) WARN( "%s exception (code=%lx) raised\n",
                                  debugstr_exception_code(rec->ExceptionCode), rec->ExceptionCode );
        break;
    }

    TRACE( "code=%lx (%s) flags=%lx addr=%p\n",
           rec->ExceptionCode, debugstr_exception_code(rec->ExceptionCode),
           rec->ExceptionFlags, rec->ExceptionAddress );
    for (i = 0; i < min( EXCEPTION_MAXIMUM_PARAMETERS, rec->NumberParameters ); i++)
        TRACE( " info[%ld]=%p\n", i, (void *)rec->ExceptionInformation[i] );
    TRACE_CONTEXT( context );

    if (call_vectored_handlers( rec, context ) == EXCEPTION_CONTINUE_EXECUTION)
        NtContinue( context, FALSE );

    if ((status = call_seh_handlers( rec, context )) == STATUS_SUCCESS)
        NtContinue( context, FALSE );

    if (status != STATUS_UNHANDLED_EXCEPTION) RtlRaiseStatus( status );
    return NtRaiseException( rec, context, FALSE );
}


#if defined(__WINE_PE_BUILD) && !defined(__i386__)

/*******************************************************************
 *		user_callback_handler
 *
 * Exception handler for KiUserCallbackDispatcher.
 */
EXCEPTION_DISPOSITION WINAPI user_callback_handler( EXCEPTION_RECORD *record, void *frame,
                                                    CONTEXT *context, void *dispatch )
{
    if (!(record->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND)))
    {
        ERR( "ignoring exception %lx\n", record->ExceptionCode );
        RtlUnwind( frame, KiUserCallbackDispatcherReturn, record, ULongToPtr(record->ExceptionCode) );
    }
    return ExceptionContinueSearch;
}

#else

/*******************************************************************
 *		dispatch_user_callback
 *
 * Implementation of KiUserCallbackDispatcher.
 */
NTSTATUS WINAPI dispatch_user_callback( void *args, ULONG len, ULONG id )
{
    NTSTATUS status;

    __TRY
    {
        KERNEL_CALLBACK_PROC func = NtCurrentTeb()->Peb->KernelCallbackTable[id];
        status = func( args, len );
    }
    __EXCEPT_ALL
    {
        status = GetExceptionCode();
        ERR( "ignoring exception %lx\n", status );
    }
    __ENDTRY
    return status;
}

#endif


/*******************************************************************
 *		raise_status
 *
 * Implementation of RtlRaiseStatus with a specific exception record.
 */
void DECLSPEC_NORETURN raise_status( NTSTATUS status, EXCEPTION_RECORD *rec )
{
    EXCEPTION_RECORD ExceptionRec;

    ExceptionRec.ExceptionCode    = status;
    ExceptionRec.ExceptionFlags   = EXCEPTION_NONCONTINUABLE;
    ExceptionRec.ExceptionRecord  = rec;
    ExceptionRec.NumberParameters = 0;
    for (;;) RtlRaiseException( &ExceptionRec );  /* never returns */
}


/***********************************************************************
 *            RtlRaiseStatus  (NTDLL.@)
 *
 * Raise an exception with ExceptionCode = status
 */
void DECLSPEC_NORETURN WINAPI RtlRaiseStatus( NTSTATUS status )
{
    raise_status( status, NULL );
}


/*******************************************************************
 *		KiRaiseUserExceptionDispatcher  (NTDLL.@)
 */
NTSTATUS WINAPI KiRaiseUserExceptionDispatcher(void)
{
    DWORD code = NtCurrentTeb()->ExceptionCode;
    EXCEPTION_RECORD rec = { code };
    RtlRaiseException( &rec );
    return code;
}


/*******************************************************************
 *         RtlAddVectoredContinueHandler   (NTDLL.@)
 */
PVOID WINAPI RtlAddVectoredContinueHandler( ULONG first, PVECTORED_EXCEPTION_HANDLER func )
{
    return add_vectored_handler( &vectored_continue_handlers, first, func );
}


/*******************************************************************
 *         RtlRemoveVectoredContinueHandler   (NTDLL.@)
 */
ULONG WINAPI RtlRemoveVectoredContinueHandler( PVOID handler )
{
    return remove_vectored_handler( &vectored_continue_handlers, handler );
}


/*******************************************************************
 *         RtlAddVectoredExceptionHandler   (NTDLL.@)
 */
PVOID WINAPI DECLSPEC_HOTPATCH RtlAddVectoredExceptionHandler( ULONG first, PVECTORED_EXCEPTION_HANDLER func )
{
    return add_vectored_handler( &vectored_exception_handlers, first, func );
}


/*******************************************************************
 *         RtlRemoveVectoredExceptionHandler   (NTDLL.@)
 */
ULONG WINAPI RtlRemoveVectoredExceptionHandler( PVOID handler )
{
    return remove_vectored_handler( &vectored_exception_handlers, handler );
}


/*******************************************************************
 *         RtlSetUnhandledExceptionFilter   (NTDLL.@)
 */
void WINAPI RtlSetUnhandledExceptionFilter( PRTL_EXCEPTION_FILTER filter )
{
    unhandled_exception_filter = filter;
}


/*******************************************************************
 *         call_unhandled_exception_filter
 */
LONG WINAPI call_unhandled_exception_filter( PEXCEPTION_POINTERS eptr )
{
    if (!unhandled_exception_filter) return EXCEPTION_CONTINUE_SEARCH;
    return unhandled_exception_filter( eptr );
}

/*******************************************************************
 *         call_unhandled_exception_handler
 */
EXCEPTION_DISPOSITION WINAPI call_unhandled_exception_handler( EXCEPTION_RECORD *rec, void *frame,
                                                               CONTEXT *context, void *dispatch )
{
    EXCEPTION_POINTERS ep = { rec, context };

    switch (call_unhandled_exception_filter( &ep ))
    {
    case EXCEPTION_CONTINUE_SEARCH:
        return ExceptionContinueSearch;
    case EXCEPTION_CONTINUE_EXECUTION:
        return ExceptionContinueExecution;
    case EXCEPTION_EXECUTE_HANDLER:
        break;
    }
    NtTerminateProcess( GetCurrentProcess(), rec->ExceptionCode );
    return ExceptionContinueExecution;
}


/*************************************************************
 *            _assert
 */
void DECLSPEC_NORETURN __cdecl _assert( const char *str, const char *file, unsigned int line )
{
    ERR( "%s:%u: Assertion failed %s\n", file, line, debugstr_a(str) );
    RtlRaiseStatus( EXCEPTION_WINE_ASSERTION );
}


/*************************************************************
 *            __wine_spec_unimplemented_stub
 *
 * ntdll-specific implementation to avoid depending on kernel functions.
 * Can be removed once ntdll.spec no longer contains stubs.
 */
void __cdecl __wine_spec_unimplemented_stub( const char *module, const char *function )
{
    EXCEPTION_RECORD record;

    record.ExceptionCode    = EXCEPTION_WINE_STUB;
    record.ExceptionFlags   = EXCEPTION_NONCONTINUABLE;
    record.ExceptionRecord  = NULL;
    record.ExceptionAddress = __wine_spec_unimplemented_stub;
    record.NumberParameters = 2;
    record.ExceptionInformation[0] = (ULONG_PTR)module;
    record.ExceptionInformation[1] = (ULONG_PTR)function;
    for (;;) RtlRaiseException( &record );
}


/*************************************************************
 *            IsBadStringPtrA
 *
 * IsBadStringPtrA replacement for ntdll, to catch exception in debug traces.
 */
BOOL DECLSPEC_NOINLINE WINAPI IsBadStringPtrA( LPCSTR str, UINT_PTR max )
{
    if (!str) return TRUE;
    __TRY
    {
        volatile const char *p = str;
        while (p != str + max) if (!*p++) break;
    }
    __EXCEPT_PAGE_FAULT
    {
        return TRUE;
    }
    __ENDTRY
    return FALSE;
}

/*************************************************************
 *            IsBadStringPtrW
 *
 * IsBadStringPtrW replacement for ntdll, to catch exception in debug traces.
 */
BOOL DECLSPEC_NOINLINE WINAPI IsBadStringPtrW( LPCWSTR str, UINT_PTR max )
{
    if (!str) return TRUE;
    __TRY
    {
        volatile const WCHAR *p = str;
        while (p != str + max) if (!*p++) break;
    }
    __EXCEPT_PAGE_FAULT
    {
        return TRUE;
    }
    __ENDTRY
    return FALSE;
}

#ifdef __i386__
__ASM_STDCALL_IMPORT(IsBadStringPtrA,8)
__ASM_STDCALL_IMPORT(IsBadStringPtrW,8)
#else
__ASM_GLOBAL_IMPORT(IsBadStringPtrA)
__ASM_GLOBAL_IMPORT(IsBadStringPtrW)
#endif


/*************************************************************************
 *		RtlCaptureStackBackTrace (NTDLL.@)
 */
USHORT WINAPI RtlCaptureStackBackTrace( ULONG skip, ULONG count, void **buffer, ULONG *hash_ret )
{
    ULONG i, ret, hash;

    TRACE( "(%lu, %lu, %p, %p)\n", skip, count, buffer, hash_ret );

    skip++;  /* skip our own frame */
    ret = RtlWalkFrameChain( buffer, count + skip, skip << 8 );
    if (hash_ret)
    {
        for (i = hash = 0; i < ret; i++) hash += (ULONG_PTR)buffer[i];
        *hash_ret = hash;
    }
    return ret;
}


/*************************************************************************
 *		RtlGetCallersAddress (NTDLL.@)
 */
void WINAPI RtlGetCallersAddress( void **caller, void **parent )
{
    void *buffer[2];
    ULONG count = ARRAY_SIZE(buffer), skip = 2;  /* skip our frame and the parent */

    count = RtlWalkFrameChain( buffer, count + skip, skip << 8 );
    *caller = count > 0 ? buffer[0] : NULL;
    *parent = count > 1 ? buffer[1] : NULL;
}


/**********************************************************************
 *              RtlGetEnabledExtendedFeatures   (NTDLL.@)
 */
ULONG64 WINAPI RtlGetEnabledExtendedFeatures(ULONG64 feature_mask)
{
    return user_shared_data->XState.EnabledFeatures & feature_mask;
}

struct context_copy_range
{
    ULONG start;
    ULONG flag;
};

static const struct context_copy_range copy_ranges_amd64[] =
{
    {0x38, 0x1}, {0x3a, 0x4}, { 0x42, 0x1}, { 0x48, 0x10}, { 0x78,  0x2}, { 0x98, 0x1},
    {0xa0, 0x2}, {0xf8, 0x1}, {0x100, 0x8}, {0x2a0,    0}, {0x4b0, 0x10}, {0x4d0,   0}
};

static const struct context_copy_range copy_ranges_x86[] =
{
    {  0x4, 0x10}, {0x1c, 0x8}, {0x8c, 0x4}, {0x9c, 0x2}, {0xb4, 0x1}, {0xcc, 0x20}, {0x1ec, 0},
    {0x2cc,    0},
};

static const struct context_parameters
{
    ULONG arch_flag;
    ULONG supported_flags;
    ULONG context_size;    /* sizeof(CONTEXT) */
    ULONG legacy_size;     /* Legacy context size */
    ULONG context_ex_size; /* sizeof(CONTEXT_EX) */
    ULONG alignment;       /* Used when computing size of context. */
    ULONG true_alignment;  /* Used for actual alignment. */
    ULONG flags_offset;
    const struct context_copy_range *copy_ranges;
}
arch_context_parameters[] =
{
    {
        CONTEXT_AMD64,
        0xd8000000 | CONTEXT_AMD64_ALL | CONTEXT_AMD64_XSTATE,
        sizeof(AMD64_CONTEXT),
        sizeof(AMD64_CONTEXT),
        0x20,
        7,
        TYPE_ALIGNMENT(AMD64_CONTEXT) - 1,
        offsetof(AMD64_CONTEXT,ContextFlags),
        copy_ranges_amd64
    },
    {
        CONTEXT_i386,
        0xd8000000 | CONTEXT_I386_ALL | CONTEXT_I386_XSTATE,
        sizeof(I386_CONTEXT),
        offsetof(I386_CONTEXT,ExtendedRegisters),
        0x18,
        3,
        TYPE_ALIGNMENT(I386_CONTEXT) - 1,
        offsetof(I386_CONTEXT,ContextFlags),
        copy_ranges_x86
    },
};

static const struct context_parameters *context_get_parameters( ULONG context_flags )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(arch_context_parameters); ++i)
    {
        if (context_flags & arch_context_parameters[i].arch_flag)
            return context_flags & ~arch_context_parameters[i].supported_flags ? NULL : &arch_context_parameters[i];
    }
    return NULL;
}

/* offset is from the start of XSAVE_AREA_HEADER. */
static int next_compacted_xstate_offset( int off, UINT64 compaction_mask, int feature_idx )
{
    const UINT64 feature_mask = (UINT64)1 << feature_idx;

    if (compaction_mask & feature_mask) off += user_shared_data->XState.Features[feature_idx].Size;
    if (user_shared_data->XState.AlignedFeatures & (feature_mask << 1))
        off = (off + 63) & ~63;
    return off;
}

/* size includes XSAVE_AREA_HEADER but not XSAVE_FORMAT (legacy save area). */
static int xstate_get_compacted_size( UINT64 mask )
{
    UINT64 compaction_mask;
    unsigned int i;
    int off;

    compaction_mask = ((UINT64)1 << 63) | mask;
    mask >>= 2;
    off = sizeof(XSAVE_AREA_HEADER);
    i = 2;
    while (mask)
    {
        if (mask == 1) return off + user_shared_data->XState.Features[i].Size;
        off = next_compacted_xstate_offset( off, compaction_mask, i );
        mask >>= 1;
        ++i;
    }
    return off;
}

static int xstate_get_size( UINT64 mask )
{
    unsigned int i;

    mask >>= 2;
    if (!mask) return sizeof(XSAVE_AREA_HEADER);
    i = 2;
    while (mask != 1)
    {
        mask >>= 1;
        ++i;
    }
    return user_shared_data->XState.Features[i].Offset + user_shared_data->XState.Features[i].Size - sizeof(XSAVE_FORMAT);
}

/**********************************************************************
 *              RtlGetExtendedContextLength2    (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetExtendedContextLength2( ULONG context_flags, ULONG *length, ULONG64 compaction_mask )
{
    const struct context_parameters *p;
    ULONG64 supported_mask;
    ULONG64 size;

    TRACE( "context_flags %#lx, length %p, compaction_mask %s.\n", context_flags, length,
            wine_dbgstr_longlong(compaction_mask) );

    if (!(p = context_get_parameters( context_flags )))
        return STATUS_INVALID_PARAMETER;

    if (!(context_flags & 0x40))
    {
        *length = p->context_size + p->context_ex_size + p->alignment;
        return STATUS_SUCCESS;
    }

    if (!(supported_mask = RtlGetEnabledExtendedFeatures( ~(ULONG64)0) ))
        return STATUS_NOT_SUPPORTED;

    size = p->context_size + p->context_ex_size + 63;

    compaction_mask &= supported_mask & ~(ULONG64)3;
    if (user_shared_data->XState.CompactionEnabled) size += xstate_get_compacted_size( compaction_mask );
    else if (compaction_mask)                       size += xstate_get_size( compaction_mask );
    else                                            size += sizeof(XSAVE_AREA_HEADER);

    *length = size;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *              RtlGetExtendedContextLength    (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetExtendedContextLength( ULONG context_flags, ULONG *length )
{
    return RtlGetExtendedContextLength2( context_flags, length, ~(ULONG64)0 );
}


/**********************************************************************
 *              RtlInitializeExtendedContext2    (NTDLL.@)
 */
NTSTATUS WINAPI RtlInitializeExtendedContext2( void *context, ULONG context_flags, CONTEXT_EX **context_ex,
        ULONG64 compaction_mask )
{
    const struct context_parameters *p;
    ULONG64 supported_mask = 0;
    CONTEXT_EX *c_ex;

    TRACE( "context %p, context_flags %#lx, context_ex %p, compaction_mask %s.\n",
            context, context_flags, context_ex, wine_dbgstr_longlong(compaction_mask));

    if (!(p = context_get_parameters( context_flags )))
        return STATUS_INVALID_PARAMETER;

    if ((context_flags & 0x40) && !(supported_mask = RtlGetEnabledExtendedFeatures( ~(ULONG64)0 )))
        return STATUS_NOT_SUPPORTED;

    context = (void *)(((ULONG_PTR)context + p->true_alignment) & ~(ULONG_PTR)p->true_alignment);
    *(ULONG *)((BYTE *)context + p->flags_offset) = context_flags;

    *context_ex = c_ex = (CONTEXT_EX *)((BYTE *)context + p->context_size);
    c_ex->Legacy.Offset = c_ex->All.Offset = -(LONG)p->context_size;
    c_ex->Legacy.Length = context_flags & 0x20 ? p->context_size : p->legacy_size;

    if (context_flags & 0x40)
    {
        XSTATE *xs;

        compaction_mask &= supported_mask;

        xs = (XSTATE *)(((ULONG_PTR)c_ex + p->context_ex_size + 63) & ~(ULONG_PTR)63);

        c_ex->XState.Offset = (ULONG_PTR)xs - (ULONG_PTR)c_ex;
        compaction_mask &= supported_mask;

        if (user_shared_data->XState.CompactionEnabled) c_ex->XState.Length = xstate_get_compacted_size( compaction_mask );
        else if (compaction_mask & ~(ULONG64)3)         c_ex->XState.Length = xstate_get_size( compaction_mask );
        else                                            c_ex->XState.Length = sizeof(XSAVE_AREA_HEADER);

        memset( xs, 0, c_ex->XState.Length );
        if (user_shared_data->XState.CompactionEnabled)
            xs->CompactionMask = ((ULONG64)1 << 63) | compaction_mask;

        c_ex->All.Length = p->context_size + c_ex->XState.Offset + c_ex->XState.Length;
    }
    else
    {
        c_ex->XState.Offset = 25; /* According to the tests, it is just 25 if CONTEXT_XSTATE is not specified. */
        c_ex->XState.Length = 0;
        c_ex->All.Length = p->context_size + 24; /* sizeof(CONTEXT_EX) minus 8 alignment bytes on x64. */
    }

    return STATUS_SUCCESS;
}


/**********************************************************************
 *              RtlInitializeExtendedContext    (NTDLL.@)
 */
NTSTATUS WINAPI RtlInitializeExtendedContext( void *context, ULONG context_flags, CONTEXT_EX **context_ex )
{
    return RtlInitializeExtendedContext2( context, context_flags, context_ex, ~(ULONG64)0 );
}


/**********************************************************************
 *              RtlLocateExtendedFeature2    (NTDLL.@)
 */
void * WINAPI RtlLocateExtendedFeature2( CONTEXT_EX *context_ex, ULONG feature_id,
        XSTATE_CONFIGURATION *xstate_config, ULONG *length )
{
    UINT64 feature_mask = (ULONG64)1 << feature_id;
    XSAVE_AREA_HEADER *xs;
    unsigned int offset, i;

    TRACE( "context_ex %p, feature_id %lu, xstate_config %p, length %p.\n",
            context_ex, feature_id, xstate_config, length );

    if (!xstate_config)
    {
        FIXME( "NULL xstate_config.\n" );
        return NULL;
    }

    if (xstate_config != &user_shared_data->XState)
    {
        FIXME( "Custom xstate configuration is not supported.\n" );
        return NULL;
    }

    if (feature_id < 2 || feature_id >= 64)
        return NULL;

    xs = (XSAVE_AREA_HEADER *)((BYTE *)context_ex + context_ex->XState.Offset);

    if (length)
        *length = xstate_config->Features[feature_id].Size;

    if (xstate_config->CompactionEnabled)
    {
        if (!(xs->CompactionMask & feature_mask)) return NULL;
        offset = sizeof(XSAVE_AREA_HEADER);
        for (i = 2; i < feature_id; ++i)
            offset = next_compacted_xstate_offset( offset, xs->CompactionMask, i );
    }
    else
    {
        if (!(feature_mask & xstate_config->EnabledFeatures)) return NULL;
        offset = xstate_config->Features[feature_id].Offset - sizeof(XSAVE_FORMAT);
    }

    if (context_ex->XState.Length < offset + xstate_config->Features[feature_id].Size)
        return NULL;

    return (BYTE *)xs + offset;
}


/**********************************************************************
 *              RtlLocateExtendedFeature    (NTDLL.@)
 */
void * WINAPI RtlLocateExtendedFeature( CONTEXT_EX *context_ex, ULONG feature_id,
        ULONG *length )
{
    return RtlLocateExtendedFeature2( context_ex, feature_id, &user_shared_data->XState, length );
}

/**********************************************************************
 *              RtlLocateLegacyContext      (NTDLL.@)
 */
void * WINAPI RtlLocateLegacyContext( CONTEXT_EX *context_ex, ULONG *length )
{
    if (length)
        *length = context_ex->Legacy.Length;

    return (BYTE *)context_ex + context_ex->Legacy.Offset;
}

/**********************************************************************
 *              RtlSetExtendedFeaturesMask  (NTDLL.@)
 */
void WINAPI RtlSetExtendedFeaturesMask( CONTEXT_EX *context_ex, ULONG64 feature_mask )
{
    XSTATE *xs = (XSTATE *)((BYTE *)context_ex + context_ex->XState.Offset);

    xs->Mask = RtlGetEnabledExtendedFeatures( feature_mask ) & ~(ULONG64)3;
}


/**********************************************************************
 *              RtlGetExtendedFeaturesMask  (NTDLL.@)
 */
ULONG64 WINAPI RtlGetExtendedFeaturesMask( CONTEXT_EX *context_ex )
{
    XSTATE *xs = (XSTATE *)((BYTE *)context_ex + context_ex->XState.Offset);

    return xs->Mask & ~(ULONG64)3;
}


static void context_copy_ranges( BYTE *d, DWORD context_flags, BYTE *s, const struct context_parameters *p )
{
    const struct context_copy_range *range;
    unsigned int start;

    *((ULONG *)(d + p->flags_offset)) |= context_flags;

    start = 0;
    range = p->copy_ranges;
    do
    {
        if (range->flag & context_flags)
        {
            if (!start)
                start = range->start;
        }
        else if (start)
        {
            memcpy( d + start, s + start, range->start - start );
            start = 0;
        }
    }
    while (range++->start != p->context_size);
}


/***********************************************************************
 *              RtlCopyContext  (NTDLL.@)
 */
NTSTATUS WINAPI RtlCopyContext( CONTEXT *dst, DWORD context_flags, CONTEXT *src )
{
    DWORD context_size, arch_flag, flags_offset, dst_flags, src_flags;
    static const DWORD arch_mask = CONTEXT_i386 | CONTEXT_AMD64;
    const struct context_parameters *p;
    BYTE *d, *s;

    TRACE("dst %p, context_flags %#lx, src %p.\n", dst, context_flags, src);

    if (context_flags & 0x40 && !RtlGetEnabledExtendedFeatures( ~(ULONG64)0 )) return STATUS_NOT_SUPPORTED;

    arch_flag = context_flags & arch_mask;
    switch (arch_flag)
    {
    case CONTEXT_i386:
        context_size = sizeof( I386_CONTEXT );
        flags_offset = offsetof( I386_CONTEXT, ContextFlags );
        break;
    case CONTEXT_AMD64:
        context_size = sizeof( AMD64_CONTEXT );
        flags_offset = offsetof( AMD64_CONTEXT, ContextFlags );
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    d = (BYTE *)dst;
    s = (BYTE *)src;
    dst_flags = *(DWORD *)(d + flags_offset);
    src_flags = *(DWORD *)(s + flags_offset);

    if ((dst_flags & arch_mask) != arch_flag || (src_flags & arch_mask) != arch_flag)
        return STATUS_INVALID_PARAMETER;

    context_flags &= src_flags;
    if (context_flags & ~dst_flags & 0x40) return STATUS_BUFFER_OVERFLOW;

    if (context_flags & 0x40)
        return RtlCopyExtendedContext( (CONTEXT_EX *)(d + context_size), context_flags,
                                       (CONTEXT_EX *)(s + context_size) );

    if (!(p = context_get_parameters( context_flags )))
        return STATUS_INVALID_PARAMETER;

    context_copy_ranges( d, context_flags, s, p );
    return STATUS_SUCCESS;
}


/**********************************************************************
 *              RtlCopyExtendedContext      (NTDLL.@)
 */
NTSTATUS WINAPI RtlCopyExtendedContext( CONTEXT_EX *dst, ULONG context_flags, CONTEXT_EX *src )
{
    const struct context_parameters *p;
    XSAVE_AREA_HEADER *dst_xs, *src_xs;
    ULONG64 feature_mask;
    unsigned int i, off, size;

    TRACE( "dst %p, context_flags %#lx, src %p.\n", dst, context_flags, src );

    if (!(p = context_get_parameters( context_flags )))
        return STATUS_INVALID_PARAMETER;

    if (!(feature_mask = RtlGetEnabledExtendedFeatures( ~(ULONG64)0 )) && context_flags & 0x40)
        return STATUS_NOT_SUPPORTED;

    context_copy_ranges( RtlLocateLegacyContext( dst, NULL ), context_flags, RtlLocateLegacyContext( src, NULL ), p );

    if (!(context_flags & 0x40))
        return STATUS_SUCCESS;

    if (dst->XState.Length < sizeof(XSAVE_AREA_HEADER))
        return STATUS_BUFFER_OVERFLOW;

    dst_xs = (XSAVE_AREA_HEADER *)((BYTE *)dst + dst->XState.Offset);
    src_xs = (XSAVE_AREA_HEADER *)((BYTE *)src + src->XState.Offset);

    memset(dst_xs, 0, sizeof(XSAVE_AREA_HEADER));
    dst_xs->Mask = (src_xs->Mask & ~(ULONG64)3) & feature_mask;
    dst_xs->CompactionMask = user_shared_data->XState.CompactionEnabled
            ? ((ULONG64)1 << 63) | (src_xs->CompactionMask & feature_mask) : 0;


    if (dst_xs->CompactionMask) feature_mask &= dst_xs->CompactionMask;
    feature_mask = dst_xs->Mask >> 2;

    i = 2;
    off = sizeof(XSAVE_AREA_HEADER);
    while (1)
    {
        if (feature_mask & 1)
        {
            if (!dst_xs->CompactionMask) off = user_shared_data->XState.Features[i].Offset - sizeof(XSAVE_FORMAT);
            size = user_shared_data->XState.Features[i].Size;
            if (src->XState.Length < off + size || dst->XState.Length < off + size) break;
            memcpy( (BYTE *)dst_xs + off, (BYTE *)src_xs + off, size );
        }
        if (!(feature_mask >>= 1)) break;
        if (dst_xs->CompactionMask) off = next_compacted_xstate_offset( off, dst_xs->CompactionMask, i);
        ++i;
    }
    return STATUS_SUCCESS;
}
