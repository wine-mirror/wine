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

#ifndef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN 16384
#endif

static int *nb_threads;

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
 *           init_threading
 */
TEB * CDECL init_threading( int *nb_threads_ptr, struct ldt_copy **ldt_copy, SIZE_T *size, BOOL *suspend,
                            unsigned int *cpus, BOOL *wow64, timeout_t *start_time )
{
    TEB *teb;
    SIZE_T info_size;
    struct ntdll_thread_data *thread_data;
#ifdef __i386__
    extern struct ldt_copy __wine_ldt_copy;
    *ldt_copy = &__wine_ldt_copy;
#endif
    nb_threads = nb_threads_ptr;

    teb = virtual_alloc_first_teb();
    thread_data = (struct ntdll_thread_data *)&teb->GdiTebBatch;
    thread_data->request_fd = -1;
    thread_data->reply_fd   = -1;
    thread_data->wait_fd[0] = -1;
    thread_data->wait_fd[1] = -1;

    signal_init_threading();
    signal_alloc_thread( teb );
    signal_init_thread( teb );
    dbg_init();
    server_init_process();
    info_size = server_init_thread( teb->Peb, suspend );
    virtual_map_user_shared_data();
    NtCreateKeyedEvent( &keyed_event, GENERIC_READ | GENERIC_WRITE, NULL, 0 );

    if (size) *size = info_size;
    if (cpus) *cpus = server_cpus;
    if (wow64) *wow64 = is_wow64;
    if (start_time) *start_time = server_start_time;
    return teb;
}


/* info passed to a starting thread */
struct startup_info
{
    PRTL_THREAD_START_ROUTINE entry;
    void                     *arg;
    HANDLE                    actctx;
};

/***********************************************************************
 *           start_thread
 *
 * Startup routine for a newly created thread.
 */
static void start_thread( TEB *teb )
{
    struct startup_info *info = (struct startup_info *)(teb + 1);
    struct ntdll_thread_data *thread_data = (struct ntdll_thread_data *)&teb->GdiTebBatch;
    struct debug_info debug_info;
    BOOL suspend;
    ULONG_PTR cookie;

    debug_info.str_pos = debug_info.out_pos = 0;
    thread_data->debug_info = &debug_info;
    thread_data->pthread_id = pthread_self();
    signal_init_thread( teb );
    server_init_thread( info->entry, &suspend );
    if (info->actctx)
    {
        RtlActivateActivationContext( 0, info->actctx, &cookie );
        RtlReleaseActivationContext( info->actctx );
    }
    signal_start_thread( info->entry, info->arg, suspend, RtlUserThreadStart, teb );
}


/***********************************************************************
 *           update_attr_list
 *
 * Update the output attributes.
 */
static void update_attr_list( PS_ATTRIBUTE_LIST *attr, const CLIENT_ID *id )
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
    struct startup_info *info;
    DWORD tid = 0;
    int request_pipe[2];
    SIZE_T extra_stack = PTHREAD_STACK_MIN;
    CLIENT_ID client_id;
    HANDLE actctx;
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
            *handle = wine_server_ptr_handle( result.create_thread.handle );
            client_id.UniqueProcess = ULongToHandle( result.create_thread.pid );
            client_id.UniqueThread  = ULongToHandle( result.create_thread.tid );
            if (attr_list) update_attr_list( attr_list, &client_id );
        }
        return result.create_thread.status;
    }

    if ((status = alloc_object_attributes( attr, &objattr, &len ))) return status;

    if (server_pipe( request_pipe ) == -1)
    {
        RtlFreeHeap( GetProcessHeap(), 0, objattr );
        return STATUS_TOO_MANY_OPENED_FILES;
    }
    server_send_fd( request_pipe[0] );

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

    RtlFreeHeap( GetProcessHeap(), 0, objattr );
    if (status)
    {
        close( request_pipe[1] );
        return status;
    }

    RtlGetActiveActivationContext( &actctx );

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

    info = (struct startup_info *)(teb + 1);
    info->entry  = start;
    info->arg    = param;
    info->actctx = actctx;

    teb->Tib.StackBase = stack.StackBase;
    teb->Tib.StackLimit = stack.StackLimit;
    teb->DeallocationStack = stack.DeallocationStack;

    thread_data = (struct ntdll_thread_data *)&teb->GdiTebBatch;
    thread_data->request_fd  = request_pipe[1];
    thread_data->reply_fd    = -1;
    thread_data->wait_fd[0]  = -1;
    thread_data->wait_fd[1]  = -1;
    thread_data->start_stack = (char *)teb->Tib.StackBase;

    pthread_attr_init( &pthread_attr );
    pthread_attr_setstack( &pthread_attr, teb->DeallocationStack,
                           (char *)teb->Tib.StackBase + extra_stack - (char *)teb->DeallocationStack );
    pthread_attr_setguardsize( &pthread_attr, 0 );
    pthread_attr_setscope( &pthread_attr, PTHREAD_SCOPE_SYSTEM ); /* force creating a kernel thread */
    InterlockedIncrement( nb_threads );
    if (pthread_create( &pthread_id, &pthread_attr, (void * (*)(void *))start_thread, teb ))
    {
        InterlockedDecrement( nb_threads );
        virtual_free_teb( teb );
        status = STATUS_NO_MEMORY;
    }
    pthread_attr_destroy( &pthread_attr );

done:
    pthread_sigmask( SIG_SETMASK, &sigset, NULL );
    if (status)
    {
        NtClose( *handle );
        RtlReleaseActivationContext( actctx );
        close( request_pipe[1] );
        return status;
    }
    if (attr_list) update_attr_list( attr_list, &client_id );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           start_process
 */
void CDECL start_process( PRTL_THREAD_START_ROUTINE entry, BOOL suspend, void *relay )
{
    signal_start_thread( entry, NtCurrentTeb()->Peb, suspend, relay, NtCurrentTeb() );
}


/***********************************************************************
 *           abort_thread
 */
void CDECL abort_thread( int status )
{
    pthread_sigmask( SIG_BLOCK, &server_block_set, NULL );
    if (InterlockedDecrement( nb_threads ) <= 0) _exit( get_unix_exit_code( status ));
    signal_exit_thread( status, pthread_exit_wrapper );
}


/***********************************************************************
 *           exit_thread
 */
void CDECL exit_thread( int status )
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
void CDECL exit_process( int status )
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
