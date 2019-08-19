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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/server.h"
#include "wine/list.h"
#include "wine/debug.h"
#include "excpt.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

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
 *           wait_suspend
 *
 * Wait until the thread is no longer suspended.
 */
void wait_suspend( CONTEXT *context )
{
    LARGE_INTEGER timeout;
    int saved_errno = errno;
    context_t server_context;
    DWORD flags = context->ContextFlags;

    context_to_server( &server_context, context );

    /* store the context we got at suspend time */
    SERVER_START_REQ( set_suspend_context )
    {
        wine_server_add_data( req, &server_context, sizeof(server_context) );
        wine_server_call( req );
    }
    SERVER_END_REQ;

    /* wait with 0 timeout, will only return once the thread is no longer suspended */
    timeout.QuadPart = 0;
    server_select( NULL, 0, SELECT_INTERRUPTIBLE, &timeout );

    /* retrieve the new context */
    SERVER_START_REQ( get_suspend_context )
    {
        wine_server_set_reply( req, &server_context, sizeof(server_context) );
        wine_server_call( req );
        if (wine_server_reply_size( reply ))
        {
            context_from_server( context, &server_context );
            context->ContextFlags |= flags;  /* unchanged registers are still available */
        }
    }
    SERVER_END_REQ;

    errno = saved_errno;
}


/**********************************************************************
 *           send_debug_event
 *
 * Send an EXCEPTION_DEBUG_EVENT event to the debugger.
 */
NTSTATUS send_debug_event( EXCEPTION_RECORD *rec, int first_chance, CONTEXT *context )
{
    NTSTATUS ret;
    DWORD i;
    obj_handle_t handle = 0;
    client_ptr_t params[EXCEPTION_MAXIMUM_PARAMETERS];
    context_t server_context;
    select_op_t select_op;

    if (!NtCurrentTeb()->Peb->BeingDebugged) return 0;  /* no debugger present */

    for (i = 0; i < min( rec->NumberParameters, EXCEPTION_MAXIMUM_PARAMETERS ); i++)
        params[i] = rec->ExceptionInformation[i];

    context_to_server( &server_context, context );

    SERVER_START_REQ( queue_exception_event )
    {
        req->first   = first_chance;
        req->code    = rec->ExceptionCode;
        req->flags   = rec->ExceptionFlags;
        req->record  = wine_server_client_ptr( rec->ExceptionRecord );
        req->address = wine_server_client_ptr( rec->ExceptionAddress );
        req->len     = i * sizeof(params[0]);
        wine_server_add_data( req, params, req->len );
        wine_server_add_data( req, &server_context, sizeof(server_context) );
        if (!wine_server_call( req )) handle = reply->handle;
    }
    SERVER_END_REQ;
    if (!handle) return 0;

    select_op.wait.op = SELECT_WAIT;
    select_op.wait.handles[0] = handle;
    server_select( &select_op, offsetof( select_op_t, wait.handles[1] ), SELECT_INTERRUPTIBLE, NULL );

    SERVER_START_REQ( get_exception_status )
    {
        req->handle = handle;
        wine_server_set_reply( req, &server_context, sizeof(server_context) );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (ret >= 0) context_from_server( context, &server_context );
    return ret;
}


/**********************************************************************
 *           call_vectored_handlers
 *
 * Call the vectored handlers chain.
 */
LONG call_vectored_handlers( EXCEPTION_RECORD *rec, CONTEXT *context )
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

        TRACE( "calling handler at %p code=%x flags=%x\n",
               func, rec->ExceptionCode, rec->ExceptionFlags );
        ret = func( &except_ptrs );
        TRACE( "handler at %p returned %x\n", func, ret );

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
 *		raise_status
 *
 * Implementation of RtlRaiseStatus with a specific exception record.
 */
void raise_status( NTSTATUS status, EXCEPTION_RECORD *rec )
{
    EXCEPTION_RECORD ExceptionRec;

    ExceptionRec.ExceptionCode    = status;
    ExceptionRec.ExceptionFlags   = EH_NONCONTINUABLE;
    ExceptionRec.ExceptionRecord  = rec;
    ExceptionRec.NumberParameters = 0;
    for (;;) RtlRaiseException( &ExceptionRec );  /* never returns */
}


/***********************************************************************
 *            RtlRaiseStatus  (NTDLL.@)
 *
 * Raise an exception with ExceptionCode = status
 */
void WINAPI RtlRaiseStatus( NTSTATUS status )
{
    raise_status( status, NULL );
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


#if defined(__x86_64__) || defined(__arm__) || defined(__aarch64__)

struct dynamic_unwind_entry
{
    struct list       entry;
    ULONG_PTR         base;
    ULONG_PTR         end;
    RUNTIME_FUNCTION *table;
    DWORD             count;
    DWORD             max_count;
    PGET_RUNTIME_FUNCTION_CALLBACK callback;
    PVOID             context;
};

static struct list dynamic_unwind_list = LIST_INIT(dynamic_unwind_list);

static RTL_CRITICAL_SECTION dynamic_unwind_section;
static RTL_CRITICAL_SECTION_DEBUG dynamic_unwind_debug =
{
    0, 0, &dynamic_unwind_section,
    { &dynamic_unwind_debug.ProcessLocksList, &dynamic_unwind_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dynamic_unwind_section") }
};
static RTL_CRITICAL_SECTION dynamic_unwind_section = { &dynamic_unwind_debug, -1, 0, 0, 0, 0 };

static ULONG_PTR get_runtime_function_end( RUNTIME_FUNCTION *func, ULONG_PTR addr )
{
#ifdef __x86_64__
    return func->EndAddress;
#elif defined(__arm__)
    if (func->u.s.Flag) return func->BeginAddress + func->u.s.FunctionLength * 2;
    else
    {
        struct unwind_info
        {
            DWORD function_length : 18;
            DWORD version : 2;
            DWORD x : 1;
            DWORD e : 1;
            DWORD f : 1;
            DWORD count : 5;
            DWORD words : 4;
        } *info = (struct unwind_info *)(addr + func->u.UnwindData);
        return func->BeginAddress + info->function_length * 2;
    }
#else  /* __aarch64__ */
    if (func->u.s.Flag) return func->BeginAddress + func->u.s.FunctionLength * 4;
    else
    {
        struct unwind_info
        {
            DWORD function_length : 18;
            DWORD version : 2;
            DWORD x : 1;
            DWORD e : 1;
            DWORD epilog : 5;
            DWORD codes : 5;
        } *info = (struct unwind_info *)(addr + func->u.UnwindData);
        return func->BeginAddress + info->function_length * 4;
    }
#endif
}

/**********************************************************************
 *              RtlAddFunctionTable   (NTDLL.@)
 */
BOOLEAN CDECL RtlAddFunctionTable( RUNTIME_FUNCTION *table, DWORD count, ULONG_PTR addr )
{
    struct dynamic_unwind_entry *entry;

    TRACE( "%p %u %lx\n", table, count, addr );

    /* NOTE: Windows doesn't check if table is aligned or a NULL pointer */

    entry = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*entry) );
    if (!entry)
        return FALSE;

    entry->base      = addr;
    entry->end       = addr + (count ? get_runtime_function_end( &table[count - 1], addr ) : 0);
    entry->table     = table;
    entry->count     = count;
    entry->max_count = 0;
    entry->callback  = NULL;
    entry->context   = NULL;

    RtlEnterCriticalSection( &dynamic_unwind_section );
    list_add_tail( &dynamic_unwind_list, &entry->entry );
    RtlLeaveCriticalSection( &dynamic_unwind_section );
    return TRUE;
}


/**********************************************************************
 *              RtlInstallFunctionTableCallback   (NTDLL.@)
 */
BOOLEAN CDECL RtlInstallFunctionTableCallback( ULONG_PTR table, ULONG_PTR base, DWORD length,
                                               PGET_RUNTIME_FUNCTION_CALLBACK callback, PVOID context,
                                               PCWSTR dll )
{
    struct dynamic_unwind_entry *entry;

    TRACE( "%lx %lx %d %p %p %s\n", table, base, length, callback, context, wine_dbgstr_w(dll) );

    /* NOTE: Windows doesn't check if the provided callback is a NULL pointer */

    /* both low-order bits must be set */
    if ((table & 0x3) != 0x3)
        return FALSE;

    entry = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*entry) );
    if (!entry)
        return FALSE;

    entry->base      = base;
    entry->end       = base + length;
    entry->table     = (RUNTIME_FUNCTION *)table;
    entry->count     = 0;
    entry->max_count = 0;
    entry->callback  = callback;
    entry->context   = context;

    RtlEnterCriticalSection( &dynamic_unwind_section );
    list_add_tail( &dynamic_unwind_list, &entry->entry );
    RtlLeaveCriticalSection( &dynamic_unwind_section );

    return TRUE;
}


/*************************************************************************
 *              RtlAddGrowableFunctionTable   (NTDLL.@)
 */
DWORD WINAPI RtlAddGrowableFunctionTable( void **table, RUNTIME_FUNCTION *functions, DWORD count,
                                          DWORD max_count, ULONG_PTR base, ULONG_PTR end )
{
    struct dynamic_unwind_entry *entry;

    TRACE( "%p, %p, %u, %u, %lx, %lx\n", table, functions, count, max_count, base, end );

    entry = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*entry) );
    if (!entry)
        return STATUS_NO_MEMORY;

    entry->base      = base;
    entry->end       = end;
    entry->table     = functions;
    entry->count     = count;
    entry->max_count = max_count;
    entry->callback  = NULL;
    entry->context   = NULL;

    RtlEnterCriticalSection( &dynamic_unwind_section );
    list_add_tail( &dynamic_unwind_list, &entry->entry );
    RtlLeaveCriticalSection( &dynamic_unwind_section );

    *table = entry;

    return STATUS_SUCCESS;
}


/*************************************************************************
 *              RtlGrowFunctionTable   (NTDLL.@)
 */
void WINAPI RtlGrowFunctionTable( void *table, DWORD count )
{
    struct dynamic_unwind_entry *entry;

    TRACE( "%p, %u\n", table, count );

    RtlEnterCriticalSection( &dynamic_unwind_section );
    LIST_FOR_EACH_ENTRY( entry, &dynamic_unwind_list, struct dynamic_unwind_entry, entry )
    {
        if (entry == table)
        {
            if (count > entry->count && count <= entry->max_count)
                entry->count = count;
            break;
        }
    }
    RtlLeaveCriticalSection( &dynamic_unwind_section );
}


/*************************************************************************
 *              RtlDeleteGrowableFunctionTable   (NTDLL.@)
 */
void WINAPI RtlDeleteGrowableFunctionTable( void *table )
{
    struct dynamic_unwind_entry *entry, *to_free = NULL;

    TRACE( "%p\n", table );

    RtlEnterCriticalSection( &dynamic_unwind_section );
    LIST_FOR_EACH_ENTRY( entry, &dynamic_unwind_list, struct dynamic_unwind_entry, entry )
    {
        if (entry == table)
        {
            to_free = entry;
            list_remove( &entry->entry );
            break;
        }
    }
    RtlLeaveCriticalSection( &dynamic_unwind_section );

    RtlFreeHeap( GetProcessHeap(), 0, to_free );
}


/**********************************************************************
 *              RtlDeleteFunctionTable   (NTDLL.@)
 */
BOOLEAN CDECL RtlDeleteFunctionTable( RUNTIME_FUNCTION *table )
{
    struct dynamic_unwind_entry *entry, *to_free = NULL;

    TRACE( "%p\n", table );

    RtlEnterCriticalSection( &dynamic_unwind_section );
    LIST_FOR_EACH_ENTRY( entry, &dynamic_unwind_list, struct dynamic_unwind_entry, entry )
    {
        if (entry->table == table)
        {
            to_free = entry;
            list_remove( &entry->entry );
            break;
        }
    }
    RtlLeaveCriticalSection( &dynamic_unwind_section );

    if (!to_free) return FALSE;

    RtlFreeHeap( GetProcessHeap(), 0, to_free );
    return TRUE;
}


/* helper for lookup_function_info() */
static RUNTIME_FUNCTION *find_function_info( ULONG_PTR pc, ULONG_PTR base,
                                             RUNTIME_FUNCTION *func, ULONG size )
{
    int min = 0;
    int max = size - 1;

    while (min <= max)
    {
#ifdef __x86_64__
        int pos = (min + max) / 2;
        if (pc < base + func[pos].BeginAddress) max = pos - 1;
        else if (pc >= base + func[pos].EndAddress) min = pos + 1;
        else
        {
            func += pos;
            while (func->UnwindData & 1)  /* follow chained entry */
                func = (RUNTIME_FUNCTION *)(base + (func->UnwindData & ~1));
            return func;
        }
#elif defined(__arm__)
        int pos = (min + max) / 2;
        if (pc < base + (func[pos].BeginAddress & ~1)) max = pos - 1;
        else if (pc >= base + get_runtime_function_end( &func[pos], base )) min = pos + 1;
        else return func + pos;
#else  /* __aarch64__ */
        int pos = (min + max) / 2;
        if (pc < base + func[pos].BeginAddress) max = pos - 1;
        else if (pc >= base + get_runtime_function_end( &func[pos], base )) min = pos + 1;
        else return func + pos;
#endif
    }
    return NULL;
}

/**********************************************************************
 *           lookup_function_info
 */
RUNTIME_FUNCTION *lookup_function_info( ULONG_PTR pc, ULONG_PTR *base, LDR_MODULE **module )
{
    RUNTIME_FUNCTION *func = NULL;
    struct dynamic_unwind_entry *entry;
    ULONG size;

    /* PE module or wine module */
    if (!LdrFindEntryForAddress( (void *)pc, module ))
    {
        *base = (ULONG_PTR)(*module)->BaseAddress;
        if ((func = RtlImageDirectoryEntryToData( (*module)->BaseAddress, TRUE,
                                                  IMAGE_DIRECTORY_ENTRY_EXCEPTION, &size )))
        {
            /* lookup in function table */
            func = find_function_info( pc, (ULONG_PTR)(*module)->BaseAddress, func, size/sizeof(*func) );
        }
    }
    else
    {
        *module = NULL;

        RtlEnterCriticalSection( &dynamic_unwind_section );
        LIST_FOR_EACH_ENTRY( entry, &dynamic_unwind_list, struct dynamic_unwind_entry, entry )
        {
            if (pc >= entry->base && pc < entry->end)
            {
                *base = entry->base;
                /* use callback or lookup in function table */
                if (entry->callback)
                    func = entry->callback( pc, entry->context );
                else
                    func = find_function_info( pc, entry->base, entry->table, entry->count );
                break;
            }
        }
        RtlLeaveCriticalSection( &dynamic_unwind_section );
    }

    return func;
}

/**********************************************************************
 *              RtlLookupFunctionEntry   (NTDLL.@)
 */
PRUNTIME_FUNCTION WINAPI RtlLookupFunctionEntry( ULONG_PTR pc, ULONG_PTR *base,
                                                 UNWIND_HISTORY_TABLE *table )
{
    LDR_MODULE *module;
    RUNTIME_FUNCTION *func;

    /* FIXME: should use the history table to make things faster */

    if (!(func = lookup_function_info( pc, base, &module )))
    {
        *base = 0;
        WARN( "no exception table found for %lx\n", pc );
    }
    return func;
}

#endif  /* __x86_64__ || __arm__ || __aarch64__ */

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
    record.ExceptionFlags   = EH_NONCONTINUABLE;
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
BOOL WINAPI IsBadStringPtrA( LPCSTR str, UINT_PTR max )
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
BOOL WINAPI IsBadStringPtrW( LPCWSTR str, UINT_PTR max )
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
