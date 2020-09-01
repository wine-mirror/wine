/*
 * NT threads support
 *
 * Copyright 1996, 2003 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif

#define NONAMELESSUNION
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "ddk/wdm.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

#ifndef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN 16384
#endif

static int nb_threads = 1;

static inline int get_unix_exit_code( NTSTATUS status )
{
    /* prevent a nonzero exit code to end up truncated to zero in unix */
    if (status && !(status & 0xff)) return 1;
    return status;
}


/***********************************************************************
 *           pthread_exit_wrapper
 */
static void pthread_exit_wrapper( int status )
{
    close( ntdll_get_thread_data()->wait_fd[0] );
    close( ntdll_get_thread_data()->wait_fd[1] );
    close( ntdll_get_thread_data()->reply_fd );
    close( ntdll_get_thread_data()->request_fd );
    pthread_exit( UIntToPtr(status) );
}


/***********************************************************************
 *           start_thread
 *
 * Startup routine for a newly created thread.
 */
static void start_thread( TEB *teb )
{
    struct ntdll_thread_data *thread_data = (struct ntdll_thread_data *)&teb->GdiTebBatch;
    struct debug_info debug_info;
    BOOL suspend;

    debug_info.str_pos = debug_info.out_pos = 0;
    thread_data->debug_info = &debug_info;
    thread_data->pthread_id = pthread_self();
    signal_init_thread( teb );
    server_init_thread( thread_data->start, &suspend );
    signal_start_thread( thread_data->start, thread_data->param, suspend, pLdrInitializeThunk, teb );
}


/***********************************************************************
 *           update_attr_list
 *
 * Update the output attributes.
 */
static void update_attr_list( PS_ATTRIBUTE_LIST *attr, const CLIENT_ID *id, TEB *teb )
{
    SIZE_T i, count = (attr->TotalLength - sizeof(attr->TotalLength)) / sizeof(PS_ATTRIBUTE);

    for (i = 0; i < count; i++)
    {
        if (attr->Attributes[i].Attribute == PS_ATTRIBUTE_CLIENT_ID)
        {
            SIZE_T size = min( attr->Attributes[i].Size, sizeof(*id) );
            memcpy( attr->Attributes[i].ValuePtr, id, size );
            if (attr->Attributes[i].ReturnLength) *attr->Attributes[i].ReturnLength = size;
        }
        else if (attr->Attributes[i].Attribute == PS_ATTRIBUTE_TEB_ADDRESS)
        {
            SIZE_T size = min( attr->Attributes[i].Size, sizeof(teb) );
            memcpy( attr->Attributes[i].ValuePtr, &teb, size );
            if (attr->Attributes[i].ReturnLength) *attr->Attributes[i].ReturnLength = size;
        }
    }
}


/***********************************************************************
 *              NtCreateThreadEx   (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateThreadEx( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                  HANDLE process, PRTL_THREAD_START_ROUTINE start, void *param,
                                  ULONG flags, SIZE_T zero_bits, SIZE_T stack_commit,
                                  SIZE_T stack_reserve, PS_ATTRIBUTE_LIST *attr_list )
{
    sigset_t sigset;
    pthread_t pthread_id;
    pthread_attr_t pthread_attr;
    data_size_t len;
    struct object_attributes *objattr;
    struct ntdll_thread_data *thread_data;
    DWORD tid = 0;
    int request_pipe[2];
    SIZE_T extra_stack = PTHREAD_STACK_MIN;
    CLIENT_ID client_id;
    TEB *teb;
    INITIAL_TEB stack;
    NTSTATUS status;

    if (process != NtCurrentProcess())
    {
        apc_call_t call;
        apc_result_t result;

        memset( &call, 0, sizeof(call) );

        call.create_thread.type    = APC_CREATE_THREAD;
        call.create_thread.flags   = flags;
        call.create_thread.func    = wine_server_client_ptr( start );
        call.create_thread.arg     = wine_server_client_ptr( param );
        call.create_thread.reserve = stack_reserve;
        call.create_thread.commit  = stack_commit;
        status = server_queue_process_apc( process, &call, &result );
        if (status != STATUS_SUCCESS) return status;

        if (result.create_thread.status == STATUS_SUCCESS)
        {
            TEB *teb = wine_server_get_ptr( result.create_thread.teb );
            *handle = wine_server_ptr_handle( result.create_thread.handle );
            client_id.UniqueProcess = ULongToHandle( result.create_thread.pid );
            client_id.UniqueThread  = ULongToHandle( result.create_thread.tid );
            if (attr_list) update_attr_list( attr_list, &client_id, teb );
        }
        return result.create_thread.status;
    }

    if ((status = alloc_object_attributes( attr, &objattr, &len ))) return status;

    if (server_pipe( request_pipe ) == -1)
    {
        free( objattr );
        return STATUS_TOO_MANY_OPENED_FILES;
    }
    wine_server_send_fd( request_pipe[0] );

    if (!access) access = THREAD_ALL_ACCESS;

    SERVER_START_REQ( new_thread )
    {
        req->process    = wine_server_obj_handle( process );
        req->access     = access;
        req->suspend    = flags & THREAD_CREATE_FLAGS_CREATE_SUSPENDED;
        req->request_fd = request_pipe[0];
        wine_server_add_data( req, objattr, len );
        if (!(status = wine_server_call( req )))
        {
            *handle = wine_server_ptr_handle( reply->handle );
            tid = reply->tid;
        }
        close( request_pipe[0] );
    }
    SERVER_END_REQ;

    free( objattr );
    if (status)
    {
        close( request_pipe[1] );
        return status;
    }

    pthread_sigmask( SIG_BLOCK, &server_block_set, &sigset );

    if ((status = virtual_alloc_teb( &teb ))) goto done;

    if ((status = virtual_alloc_thread_stack( &stack, stack_reserve, stack_commit, &extra_stack )))
    {
        virtual_free_teb( teb );
        goto done;
    }

    client_id.UniqueProcess = ULongToHandle( GetCurrentProcessId() );
    client_id.UniqueThread  = ULongToHandle( tid );
    teb->ClientId = client_id;

    teb->Tib.StackBase = stack.StackBase;
    teb->Tib.StackLimit = stack.StackLimit;
    teb->DeallocationStack = stack.DeallocationStack;

    thread_data = (struct ntdll_thread_data *)&teb->GdiTebBatch;
    thread_data->request_fd  = request_pipe[1];
    thread_data->start_stack = (char *)teb->Tib.StackBase;
    thread_data->start = start;
    thread_data->param = param;

    pthread_attr_init( &pthread_attr );
    pthread_attr_setstack( &pthread_attr, teb->DeallocationStack,
                           (char *)teb->Tib.StackBase + extra_stack - (char *)teb->DeallocationStack );
    pthread_attr_setguardsize( &pthread_attr, 0 );
    pthread_attr_setscope( &pthread_attr, PTHREAD_SCOPE_SYSTEM ); /* force creating a kernel thread */
    InterlockedIncrement( &nb_threads );
    if (pthread_create( &pthread_id, &pthread_attr, (void * (*)(void *))start_thread, teb ))
    {
        InterlockedDecrement( &nb_threads );
        virtual_free_teb( teb );
        status = STATUS_NO_MEMORY;
    }
    pthread_attr_destroy( &pthread_attr );

done:
    pthread_sigmask( SIG_SETMASK, &sigset, NULL );
    if (status)
    {
        NtClose( *handle );
        close( request_pipe[1] );
        return status;
    }
    if (attr_list) update_attr_list( attr_list, &client_id, teb );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           abort_thread
 */
void abort_thread( int status )
{
    pthread_sigmask( SIG_BLOCK, &server_block_set, NULL );
    if (InterlockedDecrement( &nb_threads ) <= 0) abort_process( status );
    signal_exit_thread( status, pthread_exit_wrapper );
}


/***********************************************************************
 *           abort_process
 */
void abort_process( int status )
{
    _exit( get_unix_exit_code( status ));
}


/***********************************************************************
 *           exit_thread
 */
static void exit_thread( int status )
{
    static void *prev_teb;
    TEB *teb;

    pthread_sigmask( SIG_BLOCK, &server_block_set, NULL );

    if ((teb = InterlockedExchangePointer( &prev_teb, NtCurrentTeb() )))
    {
        struct ntdll_thread_data *thread_data = (struct ntdll_thread_data *)&teb->GdiTebBatch;

        if (thread_data->pthread_id)
        {
            pthread_join( thread_data->pthread_id, NULL );
            virtual_free_teb( teb );
        }
    }
    signal_exit_thread( status, pthread_exit_wrapper );
}


/***********************************************************************
 *           exit_process
 */
void exit_process( int status )
{
    pthread_sigmask( SIG_BLOCK, &server_block_set, NULL );
    signal_exit_thread( get_unix_exit_code( status ), exit );
}


/**********************************************************************
 *           wait_suspend
 *
 * Wait until the thread is no longer suspended.
 */
void wait_suspend( CONTEXT *context )
{
    int saved_errno = errno;

    /* wait with 0 timeout, will only return once the thread is no longer suspended */
    server_select( NULL, 0, SELECT_INTERRUPTIBLE, 0, context, NULL, NULL );
    errno = saved_errno;
}


/**********************************************************************
 *           send_debug_event
 *
 * Send an EXCEPTION_DEBUG_EVENT event to the debugger.
 */
NTSTATUS send_debug_event( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    NTSTATUS ret;
    DWORD i;
    obj_handle_t handle = 0;
    client_ptr_t params[EXCEPTION_MAXIMUM_PARAMETERS];
    CONTEXT exception_context = *context;
    select_op_t select_op;
    sigset_t old_set;

    if (!NtCurrentTeb()->Peb->BeingDebugged) return 0;  /* no debugger present */

    pthread_sigmask( SIG_BLOCK, &server_block_set, &old_set );

    for (i = 0; i < min( rec->NumberParameters, EXCEPTION_MAXIMUM_PARAMETERS ); i++)
        params[i] = rec->ExceptionInformation[i];

    SERVER_START_REQ( queue_exception_event )
    {
        req->first   = first_chance;
        req->code    = rec->ExceptionCode;
        req->flags   = rec->ExceptionFlags;
        req->record  = wine_server_client_ptr( rec->ExceptionRecord );
        req->address = wine_server_client_ptr( rec->ExceptionAddress );
        req->len     = i * sizeof(params[0]);
        wine_server_add_data( req, params, req->len );
        if (!(ret = wine_server_call( req ))) handle = reply->handle;
    }
    SERVER_END_REQ;

    if (handle)
    {
        select_op.wait.op = SELECT_WAIT;
        select_op.wait.handles[0] = handle;
        server_select( &select_op, offsetof( select_op_t, wait.handles[1] ), SELECT_INTERRUPTIBLE,
                       TIMEOUT_INFINITE, &exception_context, NULL, NULL );

        SERVER_START_REQ( get_exception_status )
        {
            req->handle = handle;
            ret = wine_server_call( req );
        }
        SERVER_END_REQ;
        if (ret >= 0) *context = exception_context;
    }

    pthread_sigmask( SIG_SETMASK, &old_set, NULL );
    return ret;
}


/*******************************************************************
 *		NtRaiseException (NTDLL.@)
 */
NTSTATUS WINAPI NtRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    NTSTATUS status = send_debug_event( rec, context, first_chance );

    if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
        NtSetContextThread( GetCurrentThread(), context );

    if (first_chance) call_user_exception_dispatcher( rec, context, pKiUserExceptionDispatcher );

    if (rec->ExceptionFlags & EH_STACK_INVALID)
        ERR("Exception frame is not in stack limits => unable to dispatch exception.\n");
    else if (rec->ExceptionCode == STATUS_NONCONTINUABLE_EXCEPTION)
        ERR("Process attempted to continue execution after noncontinuable exception.\n");
    else
        ERR("Unhandled exception code %x flags %x addr %p\n",
            rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );

    NtTerminateProcess( NtCurrentProcess(), rec->ExceptionCode );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *              NtOpenThread   (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenThread( HANDLE *handle, ACCESS_MASK access,
                              const OBJECT_ATTRIBUTES *attr, const CLIENT_ID *id )
{
    NTSTATUS ret;

    SERVER_START_REQ( open_thread )
    {
        req->tid        = HandleToULong(id->UniqueThread);
        req->access     = access;
        req->attributes = attr ? attr->Attributes : 0;
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtSuspendThread   (NTDLL.@)
 */
NTSTATUS WINAPI NtSuspendThread( HANDLE handle, ULONG *count )
{
    NTSTATUS ret;

    SERVER_START_REQ( suspend_thread )
    {
        req->handle = wine_server_obj_handle( handle );
        if (!(ret = wine_server_call( req )))
        {
            if (count) *count = reply->count;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtResumeThread   (NTDLL.@)
 */
NTSTATUS WINAPI NtResumeThread( HANDLE handle, ULONG *count )
{
    NTSTATUS ret;

    SERVER_START_REQ( resume_thread )
    {
        req->handle = wine_server_obj_handle( handle );
        if (!(ret = wine_server_call( req )))
        {
            if (count) *count = reply->count;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtAlertResumeThread   (NTDLL.@)
 */
NTSTATUS WINAPI NtAlertResumeThread( HANDLE handle, ULONG *count )
{
    FIXME( "stub: should alert thread %p\n", handle );
    return NtResumeThread( handle, count );
}


/******************************************************************************
 *              NtAlertThread   (NTDLL.@)
 */
NTSTATUS WINAPI NtAlertThread( HANDLE handle )
{
    FIXME( "stub: %p\n", handle );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *              NtTerminateThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtTerminateThread( HANDLE handle, LONG exit_code )
{
    NTSTATUS ret;
    BOOL self = (handle == GetCurrentThread());

    if (!self || exit_code)
    {
        SERVER_START_REQ( terminate_thread )
        {
            req->handle    = wine_server_obj_handle( handle );
            req->exit_code = exit_code;
            ret = wine_server_call( req );
            self = !ret && reply->self;
        }
        SERVER_END_REQ;
    }
    if (self) exit_thread( exit_code );
    return ret;
}


/******************************************************************************
 *              NtQueueApcThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueueApcThread( HANDLE handle, PNTAPCFUNC func, ULONG_PTR arg1,
                                  ULONG_PTR arg2, ULONG_PTR arg3 )
{
    NTSTATUS ret;

    SERVER_START_REQ( queue_apc )
    {
        req->handle = wine_server_obj_handle( handle );
        if (func)
        {
            req->call.type              = APC_USER;
            req->call.user.user.func    = wine_server_client_ptr( func );
            req->call.user.user.args[0] = arg1;
            req->call.user.user.args[1] = arg2;
            req->call.user.user.args[2] = arg3;
        }
        else req->call.type = APC_NONE;  /* wake up only */
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *              set_thread_context
 */
NTSTATUS set_thread_context( HANDLE handle, const context_t *context, BOOL *self )
{
    NTSTATUS ret;

    SERVER_START_REQ( set_thread_context )
    {
        req->handle  = wine_server_obj_handle( handle );
        wine_server_add_data( req, context, sizeof(*context) );
        ret = wine_server_call( req );
        *self = reply->self;
    }
    SERVER_END_REQ;

    return ret;
}


/***********************************************************************
 *              get_thread_context
 */
NTSTATUS get_thread_context( HANDLE handle, context_t *context, unsigned int flags, BOOL *self )
{
    NTSTATUS ret;

    SERVER_START_REQ( get_thread_context )
    {
        req->handle  = wine_server_obj_handle( handle );
        req->flags   = flags;
        wine_server_set_reply( req, context, sizeof(*context) );
        ret = wine_server_call( req );
        *self = reply->self;
        handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    if (ret == STATUS_PENDING)
    {
        LARGE_INTEGER timeout;
        timeout.QuadPart = -1000000;
        if (NtWaitForSingleObject( handle, FALSE, &timeout ))
        {
            NtClose( handle );
            return STATUS_ACCESS_DENIED;
        }
        SERVER_START_REQ( get_thread_context )
        {
            req->handle  = wine_server_obj_handle( handle );
            req->flags   = flags;
            wine_server_set_reply( req, context, sizeof(*context) );
            ret = wine_server_call( req );
        }
        SERVER_END_REQ;
    }
    return ret;
}


#ifdef __x86_64__

/***********************************************************************
 *           wow64_get_server_context_flags
 */
static unsigned int wow64_get_server_context_flags( DWORD flags )
{
    unsigned int ret = 0;

    flags &= ~WOW64_CONTEXT_i386;  /* get rid of CPU id */
    if (flags & WOW64_CONTEXT_CONTROL) ret |= SERVER_CTX_CONTROL;
    if (flags & WOW64_CONTEXT_INTEGER) ret |= SERVER_CTX_INTEGER;
    if (flags & WOW64_CONTEXT_SEGMENTS) ret |= SERVER_CTX_SEGMENTS;
    if (flags & WOW64_CONTEXT_FLOATING_POINT) ret |= SERVER_CTX_FLOATING_POINT;
    if (flags & WOW64_CONTEXT_DEBUG_REGISTERS) ret |= SERVER_CTX_DEBUG_REGISTERS;
    if (flags & WOW64_CONTEXT_EXTENDED_REGISTERS) ret |= SERVER_CTX_EXTENDED_REGISTERS;
    return ret;
}

/***********************************************************************
 *           wow64_context_from_server
 */
static NTSTATUS wow64_context_from_server( WOW64_CONTEXT *to, const context_t *from )
{
    if (from->cpu != CPU_x86) return STATUS_INVALID_PARAMETER;

    to->ContextFlags = WOW64_CONTEXT_i386;
    if (from->flags & SERVER_CTX_CONTROL)
    {
        to->ContextFlags |= WOW64_CONTEXT_CONTROL;
        to->Ebp    = from->ctl.i386_regs.ebp;
        to->Esp    = from->ctl.i386_regs.esp;
        to->Eip    = from->ctl.i386_regs.eip;
        to->SegCs  = from->ctl.i386_regs.cs;
        to->SegSs  = from->ctl.i386_regs.ss;
        to->EFlags = from->ctl.i386_regs.eflags;
    }
    if (from->flags & SERVER_CTX_INTEGER)
    {
        to->ContextFlags |= WOW64_CONTEXT_INTEGER;
        to->Eax = from->integer.i386_regs.eax;
        to->Ebx = from->integer.i386_regs.ebx;
        to->Ecx = from->integer.i386_regs.ecx;
        to->Edx = from->integer.i386_regs.edx;
        to->Esi = from->integer.i386_regs.esi;
        to->Edi = from->integer.i386_regs.edi;
    }
    if (from->flags & SERVER_CTX_SEGMENTS)
    {
        to->ContextFlags |= WOW64_CONTEXT_SEGMENTS;
        to->SegDs = from->seg.i386_regs.ds;
        to->SegEs = from->seg.i386_regs.es;
        to->SegFs = from->seg.i386_regs.fs;
        to->SegGs = from->seg.i386_regs.gs;
    }
    if (from->flags & SERVER_CTX_FLOATING_POINT)
    {
        to->ContextFlags |= WOW64_CONTEXT_FLOATING_POINT;
        to->FloatSave.ControlWord   = from->fp.i386_regs.ctrl;
        to->FloatSave.StatusWord    = from->fp.i386_regs.status;
        to->FloatSave.TagWord       = from->fp.i386_regs.tag;
        to->FloatSave.ErrorOffset   = from->fp.i386_regs.err_off;
        to->FloatSave.ErrorSelector = from->fp.i386_regs.err_sel;
        to->FloatSave.DataOffset    = from->fp.i386_regs.data_off;
        to->FloatSave.DataSelector  = from->fp.i386_regs.data_sel;
        to->FloatSave.Cr0NpxState   = from->fp.i386_regs.cr0npx;
        memcpy( to->FloatSave.RegisterArea, from->fp.i386_regs.regs, sizeof(to->FloatSave.RegisterArea) );
    }
    if (from->flags & SERVER_CTX_DEBUG_REGISTERS)
    {
        to->ContextFlags |= WOW64_CONTEXT_DEBUG_REGISTERS;
        to->Dr0 = from->debug.i386_regs.dr0;
        to->Dr1 = from->debug.i386_regs.dr1;
        to->Dr2 = from->debug.i386_regs.dr2;
        to->Dr3 = from->debug.i386_regs.dr3;
        to->Dr6 = from->debug.i386_regs.dr6;
        to->Dr7 = from->debug.i386_regs.dr7;
    }
    if (from->flags & SERVER_CTX_EXTENDED_REGISTERS)
    {
        to->ContextFlags |= WOW64_CONTEXT_EXTENDED_REGISTERS;
        memcpy( to->ExtendedRegisters, from->ext.i386_regs, sizeof(to->ExtendedRegisters) );
    }
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           wow64_context_to_server
 */
static void wow64_context_to_server( context_t *to, const WOW64_CONTEXT *from )
{
    DWORD flags = from->ContextFlags & ~WOW64_CONTEXT_i386;  /* get rid of CPU id */

    memset( to, 0, sizeof(*to) );
    to->cpu = CPU_x86;

    if (flags & WOW64_CONTEXT_CONTROL)
    {
        to->flags |= SERVER_CTX_CONTROL;
        to->ctl.i386_regs.ebp    = from->Ebp;
        to->ctl.i386_regs.esp    = from->Esp;
        to->ctl.i386_regs.eip    = from->Eip;
        to->ctl.i386_regs.cs     = from->SegCs;
        to->ctl.i386_regs.ss     = from->SegSs;
        to->ctl.i386_regs.eflags = from->EFlags;
    }
    if (flags & WOW64_CONTEXT_INTEGER)
    {
        to->flags |= SERVER_CTX_INTEGER;
        to->integer.i386_regs.eax = from->Eax;
        to->integer.i386_regs.ebx = from->Ebx;
        to->integer.i386_regs.ecx = from->Ecx;
        to->integer.i386_regs.edx = from->Edx;
        to->integer.i386_regs.esi = from->Esi;
        to->integer.i386_regs.edi = from->Edi;
    }
    if (flags & WOW64_CONTEXT_SEGMENTS)
    {
        to->flags |= SERVER_CTX_SEGMENTS;
        to->seg.i386_regs.ds = from->SegDs;
        to->seg.i386_regs.es = from->SegEs;
        to->seg.i386_regs.fs = from->SegFs;
        to->seg.i386_regs.gs = from->SegGs;
    }
    if (flags & WOW64_CONTEXT_FLOATING_POINT)
    {
        to->flags |= SERVER_CTX_FLOATING_POINT;
        to->fp.i386_regs.ctrl     = from->FloatSave.ControlWord;
        to->fp.i386_regs.status   = from->FloatSave.StatusWord;
        to->fp.i386_regs.tag      = from->FloatSave.TagWord;
        to->fp.i386_regs.err_off  = from->FloatSave.ErrorOffset;
        to->fp.i386_regs.err_sel  = from->FloatSave.ErrorSelector;
        to->fp.i386_regs.data_off = from->FloatSave.DataOffset;
        to->fp.i386_regs.data_sel = from->FloatSave.DataSelector;
        to->fp.i386_regs.cr0npx   = from->FloatSave.Cr0NpxState;
        memcpy( to->fp.i386_regs.regs, from->FloatSave.RegisterArea, sizeof(to->fp.i386_regs.regs) );
    }
    if (flags & WOW64_CONTEXT_DEBUG_REGISTERS)
    {
        to->flags |= SERVER_CTX_DEBUG_REGISTERS;
        to->debug.i386_regs.dr0 = from->Dr0;
        to->debug.i386_regs.dr1 = from->Dr1;
        to->debug.i386_regs.dr2 = from->Dr2;
        to->debug.i386_regs.dr3 = from->Dr3;
        to->debug.i386_regs.dr6 = from->Dr6;
        to->debug.i386_regs.dr7 = from->Dr7;
    }
    if (flags & WOW64_CONTEXT_EXTENDED_REGISTERS)
    {
        to->flags |= SERVER_CTX_EXTENDED_REGISTERS;
        memcpy( to->ext.i386_regs, from->ExtendedRegisters, sizeof(to->ext.i386_regs) );
    }
}

#endif /* __x86_64__ */

#ifdef linux
BOOL get_thread_times(int unix_pid, int unix_tid, LARGE_INTEGER *kernel_time, LARGE_INTEGER *user_time)
{
    unsigned long clocks_per_sec = sysconf( _SC_CLK_TCK );
    unsigned long usr, sys;
    const char *pos;
    char buf[512];
    FILE *f;
    int i;

    if (unix_tid == -1)
        sprintf( buf, "/proc/%u/stat", unix_pid );
    else
        sprintf( buf, "/proc/%u/task/%u/stat", unix_pid, unix_tid );
    if (!(f = fopen( buf, "r" )))
    {
        WARN("Failed to open %s: %s\n", buf, strerror(errno));
        return FALSE;
    }

    pos = fgets( buf, sizeof(buf), f );
    fclose( f );

    /* the process name is printed unescaped, so we have to skip to the last ')'
     * to avoid misinterpreting the string */
    if (pos) pos = strrchr( pos, ')' );
    if (pos) pos = strchr( pos + 1, ' ' );
    if (pos) pos++;

    /* skip over the following fields: state, ppid, pgid, sid, tty_nr, tty_pgrp,
     * task->flags, min_flt, cmin_flt, maj_flt, cmaj_flt */
    for (i = 0; i < 11 && pos; i++)
    {
        pos = strchr( pos + 1, ' ' );
        if (pos) pos++;
    }

    /* the next two values are user and system time */
    if (pos && (sscanf( pos, "%lu %lu", &usr, &sys ) == 2))
    {
        kernel_time->QuadPart = (ULONGLONG)sys * 10000000 / clocks_per_sec;
        user_time->QuadPart = (ULONGLONG)usr * 10000000 / clocks_per_sec;
        return TRUE;
    }

    ERR("Failed to parse %s\n", debugstr_a(buf));
    return FALSE;
}
#else
BOOL get_thread_times(int unix_pid, int unix_tid, LARGE_INTEGER *kernel_time, LARGE_INTEGER *user_time)
{
    static int once;
    if (!once++) FIXME("not implemented on this platform\n");
    return FALSE;
}
#endif

/******************************************************************************
 *              NtQueryInformationThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryInformationThread( HANDLE handle, THREADINFOCLASS class,
                                          void *data, ULONG length, ULONG *ret_len )
{
    NTSTATUS status;

    TRACE("(%p,%d,%p,%x,%p)\n", handle, class, data, length, ret_len);

    switch (class)
    {
    case ThreadBasicInformation:
    {
        THREAD_BASIC_INFORMATION info;
        const ULONG_PTR affinity_mask = get_system_affinity_mask();

        SERVER_START_REQ( get_thread_info )
        {
            req->handle = wine_server_obj_handle( handle );
            if (!(status = wine_server_call( req )))
            {
                info.ExitStatus             = reply->exit_code;
                info.TebBaseAddress         = wine_server_get_ptr( reply->teb );
                info.ClientId.UniqueProcess = ULongToHandle(reply->pid);
                info.ClientId.UniqueThread  = ULongToHandle(reply->tid);
                info.AffinityMask           = reply->affinity & affinity_mask;
                info.Priority               = reply->priority;
                info.BasePriority           = reply->priority;  /* FIXME */
            }
        }
        SERVER_END_REQ;
        if (status == STATUS_SUCCESS)
        {
            if (data) memcpy( data, &info, min( length, sizeof(info) ));
            if (ret_len) *ret_len = min( length, sizeof(info) );
        }
        return status;
    }

    case ThreadAffinityMask:
    {
        const ULONG_PTR affinity_mask = get_system_affinity_mask();
        ULONG_PTR affinity = 0;

        SERVER_START_REQ( get_thread_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->access = THREAD_QUERY_INFORMATION;
            if (!(status = wine_server_call( req ))) affinity = reply->affinity & affinity_mask;
        }
        SERVER_END_REQ;
        if (status == STATUS_SUCCESS)
        {
            if (data) memcpy( data, &affinity, min( length, sizeof(affinity) ));
            if (ret_len) *ret_len = min( length, sizeof(affinity) );
        }
        return status;
    }

    case ThreadTimes:
    {
        KERNEL_USER_TIMES kusrt;
        int unix_pid, unix_tid;

        SERVER_START_REQ( get_thread_times )
        {
            req->handle = wine_server_obj_handle( handle );
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
            {
                kusrt.CreateTime.QuadPart = reply->creation_time;
                kusrt.ExitTime.QuadPart = reply->exit_time;
                unix_pid = reply->unix_pid;
                unix_tid = reply->unix_tid;
            }
        }
        SERVER_END_REQ;
        if (status == STATUS_SUCCESS)
        {
            BOOL ret = FALSE;

            kusrt.KernelTime.QuadPart = kusrt.UserTime.QuadPart = 0;
            if (unix_pid != -1 && unix_tid != -1)
                ret = get_thread_times( unix_pid, unix_tid, &kusrt.KernelTime, &kusrt.UserTime );
            if (!ret && handle == GetCurrentThread())
            {
                /* fall back to process times */
                struct tms time_buf;
                long clocks_per_sec = sysconf(_SC_CLK_TCK);

                times(&time_buf);
                kusrt.KernelTime.QuadPart = (ULONGLONG)time_buf.tms_stime * 10000000 / clocks_per_sec;
                kusrt.UserTime.QuadPart = (ULONGLONG)time_buf.tms_utime * 10000000 / clocks_per_sec;
            }
            if (data) memcpy( data, &kusrt, min( length, sizeof(kusrt) ));
            if (ret_len) *ret_len = min( length, sizeof(kusrt) );
        }
        return status;
    }

    case ThreadDescriptorTableEntry:
        return get_thread_ldt_entry( handle, data, length, ret_len );

    case ThreadAmILastThread:
    {
        if (length != sizeof(ULONG)) return STATUS_INFO_LENGTH_MISMATCH;
        SERVER_START_REQ( get_thread_info )
        {
            req->handle = wine_server_obj_handle( handle );
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
            {
                ULONG last = reply->last;
                if (data) memcpy( data, &last, sizeof(last) );
                if (ret_len) *ret_len = sizeof(last);
            }
        }
        SERVER_END_REQ;
        return status;
    }

    case ThreadQuerySetWin32StartAddress:
    {
        SERVER_START_REQ( get_thread_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->access = THREAD_QUERY_INFORMATION;
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
            {
                PRTL_THREAD_START_ROUTINE entry = wine_server_get_ptr( reply->entry_point );
                if (data) memcpy( data, &entry, min( length, sizeof(entry) ) );
                if (ret_len) *ret_len = min( length, sizeof(entry) );
            }
        }
        SERVER_END_REQ;
        return status;
    }

    case ThreadGroupInformation:
    {
        const ULONG_PTR affinity_mask = get_system_affinity_mask();
        GROUP_AFFINITY affinity;

        memset( &affinity, 0, sizeof(affinity) );
        affinity.Group = 0; /* Wine only supports max 64 processors */

        SERVER_START_REQ( get_thread_info )
        {
            req->handle = wine_server_obj_handle( handle );
            if (!(status = wine_server_call( req ))) affinity.Mask = reply->affinity & affinity_mask;
        }
        SERVER_END_REQ;
        if (status == STATUS_SUCCESS)
        {
            if (data) memcpy( data, &affinity, min( length, sizeof(affinity) ));
            if (ret_len) *ret_len = min( length, sizeof(affinity) );
        }
        return status;
    }

    case ThreadIsIoPending:
        FIXME( "ThreadIsIoPending info class not supported yet\n" );
        if (length != sizeof(BOOL)) return STATUS_INFO_LENGTH_MISMATCH;
        if (!data) return STATUS_ACCESS_DENIED;
        *(BOOL*)data = FALSE;
        if (ret_len) *ret_len = sizeof(BOOL);
        return STATUS_SUCCESS;

    case ThreadSuspendCount:
        if (length != sizeof(ULONG)) return STATUS_INFO_LENGTH_MISMATCH;
        if (!data) return STATUS_ACCESS_VIOLATION;

        SERVER_START_REQ( get_thread_info )
        {
            req->handle = wine_server_obj_handle( handle );
            if (!(status = wine_server_call( req ))) *(ULONG *)data = reply->suspend_count;
        }
        SERVER_END_REQ;
        return status;

    case ThreadDescription:
    {
        THREAD_DESCRIPTION_INFORMATION *info = data;
        data_size_t len, desc_len = 0;
        WCHAR *ptr;

        len = length >= sizeof(*info) ? length - sizeof(*info) : 0;
        ptr = info ? (WCHAR *)(info + 1) : NULL;

        SERVER_START_REQ( get_thread_info )
        {
            req->handle = wine_server_obj_handle( handle );
            if (ptr) wine_server_set_reply( req, ptr, len );
            status = wine_server_call( req );
            desc_len = reply->desc_len;
        }
        SERVER_END_REQ;

        if (!info) status = STATUS_BUFFER_TOO_SMALL;
        else if (status == STATUS_SUCCESS)
        {
            info->Description.Length = info->Description.MaximumLength = desc_len;
            info->Description.Buffer = ptr;
        }

        if (ret_len && (status == STATUS_SUCCESS || status == STATUS_BUFFER_TOO_SMALL))
            *ret_len = sizeof(*info) + desc_len;
        return status;
    }

    case ThreadWow64Context:
    {
#ifdef __x86_64__
        BOOL self;
        WOW64_CONTEXT *context = data;
        context_t server_context;
        unsigned int server_flags;

        if (length != sizeof(*context)) return STATUS_INFO_LENGTH_MISMATCH;
        server_flags = wow64_get_server_context_flags( context->ContextFlags );
        if ((status = get_thread_context( handle, &server_context, server_flags, &self ))) return status;
        if (self) return STATUS_INVALID_PARAMETER;
        status = wow64_context_from_server( context, &server_context );
        if (ret_len && !status) *ret_len = sizeof(*context);
        return status;
#else
        return STATUS_INVALID_INFO_CLASS;
#endif
    }

    case ThreadHideFromDebugger:
        if (length != sizeof(BOOLEAN)) return STATUS_INFO_LENGTH_MISMATCH;
        if (!data) return STATUS_ACCESS_VIOLATION;
        SERVER_START_REQ( get_thread_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->access = THREAD_QUERY_INFORMATION;
            if ((status = wine_server_call( req ))) return status;
            *(BOOLEAN*)data = reply->dbg_hidden;
        }
        SERVER_END_REQ;
        if (ret_len) *ret_len = sizeof(BOOLEAN);
        return STATUS_SUCCESS;

    case ThreadPriority:
    case ThreadBasePriority:
    case ThreadImpersonationToken:
    case ThreadEnableAlignmentFaultFixup:
    case ThreadEventPair_Reusable:
    case ThreadZeroTlsCell:
    case ThreadPerformanceCount:
    case ThreadIdealProcessor:
    case ThreadPriorityBoost:
    case ThreadSetTlsArrayAddress:
    default:
        FIXME( "info class %d not supported yet\n", class );
        return STATUS_NOT_IMPLEMENTED;
    }
}


/******************************************************************************
 *              NtSetInformationThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetInformationThread( HANDLE handle, THREADINFOCLASS class,
                                        const void *data, ULONG length )
{
    NTSTATUS status;

    TRACE("(%p,%d,%p,%x)\n", handle, class, data, length);

    switch (class)
    {
    case ThreadZeroTlsCell:
        if (handle == GetCurrentThread())
        {
            if (length != sizeof(DWORD)) return STATUS_INVALID_PARAMETER;
            return virtual_clear_tls_index( *(const ULONG *)data );
        }
        FIXME( "ZeroTlsCell not supported on other threads\n" );
        return STATUS_NOT_IMPLEMENTED;

    case ThreadImpersonationToken:
    {
        const HANDLE *token = data;

        if (length != sizeof(HANDLE)) return STATUS_INVALID_PARAMETER;
        TRACE("Setting ThreadImpersonationToken handle to %p\n", *token );
        SERVER_START_REQ( set_thread_info )
        {
            req->handle   = wine_server_obj_handle( handle );
            req->token    = wine_server_obj_handle( *token );
            req->mask     = SET_THREAD_INFO_TOKEN;
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
        return status;
    }

    case ThreadBasePriority:
    {
        const DWORD *pprio = data;
        if (length != sizeof(DWORD)) return STATUS_INVALID_PARAMETER;
        SERVER_START_REQ( set_thread_info )
        {
            req->handle   = wine_server_obj_handle( handle );
            req->priority = *pprio;
            req->mask     = SET_THREAD_INFO_PRIORITY;
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
        return status;
    }

    case ThreadAffinityMask:
    {
        const ULONG_PTR affinity_mask = get_system_affinity_mask();
        ULONG_PTR req_aff;

        if (length != sizeof(ULONG_PTR)) return STATUS_INVALID_PARAMETER;
        req_aff = *(const ULONG_PTR *)data & affinity_mask;
        if (!req_aff) return STATUS_INVALID_PARAMETER;

        SERVER_START_REQ( set_thread_info )
        {
            req->handle   = wine_server_obj_handle( handle );
            req->affinity = req_aff;
            req->mask     = SET_THREAD_INFO_AFFINITY;
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
        return status;
    }

    case ThreadHideFromDebugger:
        if (length) return STATUS_INFO_LENGTH_MISMATCH;
        SERVER_START_REQ( set_thread_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->mask   = SET_THREAD_INFO_DBG_HIDDEN;
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
        return status;

    case ThreadQuerySetWin32StartAddress:
    {
        const PRTL_THREAD_START_ROUTINE *entry = data;
        if (length != sizeof(PRTL_THREAD_START_ROUTINE)) return STATUS_INVALID_PARAMETER;
        SERVER_START_REQ( set_thread_info )
        {
            req->handle   = wine_server_obj_handle( handle );
            req->mask     = SET_THREAD_INFO_ENTRYPOINT;
            req->entry_point = wine_server_client_ptr( *entry );
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
        return status;
    }

    case ThreadGroupInformation:
    {
        const ULONG_PTR affinity_mask = get_system_affinity_mask();
        const GROUP_AFFINITY *req_aff;

        if (length != sizeof(*req_aff)) return STATUS_INVALID_PARAMETER;
        if (!data) return STATUS_ACCESS_VIOLATION;
        req_aff = data;

        /* On Windows the request fails if the reserved fields are set */
        if (req_aff->Reserved[0] || req_aff->Reserved[1] || req_aff->Reserved[2])
            return STATUS_INVALID_PARAMETER;

        /* Wine only supports max 64 processors */
        if (req_aff->Group) return STATUS_INVALID_PARAMETER;
        if (req_aff->Mask & ~affinity_mask) return STATUS_INVALID_PARAMETER;
        if (!req_aff->Mask) return STATUS_INVALID_PARAMETER;
        SERVER_START_REQ( set_thread_info )
        {
            req->handle   = wine_server_obj_handle( handle );
            req->affinity = req_aff->Mask;
            req->mask     = SET_THREAD_INFO_AFFINITY;
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
        return status;
    }

    case ThreadDescription:
    {
        const THREAD_DESCRIPTION_INFORMATION *info = data;

        if (length != sizeof(*info)) return STATUS_INFO_LENGTH_MISMATCH;
        if (!info) return STATUS_ACCESS_VIOLATION;
        if (info->Description.Length != info->Description.MaximumLength) return STATUS_INVALID_PARAMETER;
        if (info->Description.Length && !info->Description.Buffer) return STATUS_ACCESS_VIOLATION;

        SERVER_START_REQ( set_thread_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->mask   = SET_THREAD_INFO_DESCRIPTION;
            wine_server_add_data( req, info->Description.Buffer, info->Description.Length );
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
        return status;
    }

    case ThreadWow64Context:
    {
#ifdef __x86_64__
        BOOL self;
        const WOW64_CONTEXT *context = data;
        context_t server_context;

        if (length != sizeof(*context)) return STATUS_INFO_LENGTH_MISMATCH;
        wow64_context_to_server( &server_context, context );
        return set_thread_context( handle, &server_context, &self );
#else
        return STATUS_INVALID_INFO_CLASS;
#endif
    }

    case ThreadBasicInformation:
    case ThreadTimes:
    case ThreadPriority:
    case ThreadDescriptorTableEntry:
    case ThreadEnableAlignmentFaultFixup:
    case ThreadEventPair_Reusable:
    case ThreadPerformanceCount:
    case ThreadAmILastThread:
    case ThreadIdealProcessor:
    case ThreadPriorityBoost:
    case ThreadSetTlsArrayAddress:
    case ThreadIsIoPending:
    default:
        FIXME( "info class %d not supported yet\n", class );
        return STATUS_NOT_IMPLEMENTED;
    }
}


/******************************************************************************
 *              NtGetCurrentProcessorNumber  (NTDLL.@)
 */
ULONG WINAPI NtGetCurrentProcessorNumber(void)
{
    ULONG processor;

#if defined(__linux__) && defined(__NR_getcpu)
    int res = syscall(__NR_getcpu, &processor, NULL, NULL);
    if (res != -1) return processor;
#endif

    if (NtCurrentTeb()->Peb->NumberOfProcessors > 1)
    {
        ULONG_PTR thread_mask, processor_mask;

        if (!NtQueryInformationThread( GetCurrentThread(), ThreadAffinityMask,
                                       &thread_mask, sizeof(thread_mask), NULL ))
        {
            for (processor = 0; processor < NtCurrentTeb()->Peb->NumberOfProcessors; processor++)
            {
                processor_mask = (1 << processor);
                if (thread_mask & processor_mask)
                {
                    if (thread_mask != processor_mask)
                        FIXME( "need multicore support (%d processors)\n",
                               NtCurrentTeb()->Peb->NumberOfProcessors );
                    return processor;
                }
            }
        }
    }
    /* fallback to the first processor */
    return 0;
}
