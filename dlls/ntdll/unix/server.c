/*
 * Wine server communication
 *
 * Copyright (C) 1998 Alexandre Julliard
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
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_LWP_H
#include <lwp.h>
#endif
#ifdef HAVE_PTHREAD_NP_H
# include <pthread_np.h>
#endif
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_SYS_PRCTL_H
# include <sys/prctl.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
# include <sys/syscall.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_SYS_THR_H
#include <sys/thr.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef __APPLE__
#include <crt_externs.h>
#include <spawn.h>
#ifndef _POSIX_SPAWN_DISABLE_ASLR
#define _POSIX_SPAWN_DISABLE_ASLR 0x0100
#endif
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "unix_private.h"
#include "ddk/wdm.h"

WINE_DEFAULT_DEBUG_CHANNEL(server);

#ifndef MSG_CMSG_CLOEXEC
#define MSG_CMSG_CLOEXEC 0
#endif

#define SOCKETNAME "socket"        /* name of the socket file */
#define LOCKNAME   "lock"          /* name of the lock file */

static const char *server_dir;

unsigned int supported_machines_count = 0;
USHORT supported_machines[8] = { 0 };
USHORT native_machine = 0;
BOOL process_exiting = FALSE;
#ifndef _WIN64
BOOL is_wow64 = FALSE;
#endif

timeout_t server_start_time = 0;  /* time of server startup */

sigset_t server_block_set;  /* signals to block during server calls */
static int fd_socket = -1;  /* socket to exchange file descriptors with the server */
static pid_t server_pid;
static pthread_mutex_t fd_cache_mutex = PTHREAD_MUTEX_INITIALIZER;

/* atomically exchange a 64-bit value */
static inline LONG64 interlocked_xchg64( LONG64 *dest, LONG64 val )
{
#ifdef _WIN64
    return (LONG64)InterlockedExchangePointer( (void **)dest, (void *)val );
#else
    LONG64 tmp = *dest;
    while (InterlockedCompareExchange64( dest, val, tmp ) != tmp) tmp = *dest;
    return tmp;
#endif
}

#ifdef __GNUC__
static void fatal_error( const char *err, ... ) __attribute__((noreturn, format(printf,1,2)));
static void fatal_perror( const char *err, ... ) __attribute__((noreturn, format(printf,1,2)));
static void server_connect_error( const char *serverdir ) __attribute__((noreturn));
#endif

/* die on a fatal error; use only during initialization */
static void fatal_error( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wine: " );
    vfprintf( stderr, err, args );
    va_end( args );
    exit(1);
}

/* die on a fatal error; use only during initialization */
static void fatal_perror( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wine: " );
    vfprintf( stderr, err, args );
    perror( " " );
    va_end( args );
    exit(1);
}

/***********************************************************************
 *           server_protocol_error
 */
static DECLSPEC_NORETURN void server_protocol_error( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wine client error:%x: ", GetCurrentThreadId() );
    vfprintf( stderr, err, args );
    va_end( args );
    abort_thread(1);
}


/***********************************************************************
 *           server_protocol_perror
 */
static DECLSPEC_NORETURN void server_protocol_perror( const char *err )
{
    fprintf( stderr, "wine client error:%x: ", GetCurrentThreadId() );
    perror( err );
    abort_thread(1);
}


/***********************************************************************
 *           send_request
 *
 * Send a request to the server.
 */
static unsigned int send_request( const struct __server_request_info *req )
{
    unsigned int i;
    int ret;

    if (!req->u.req.request_header.request_size)
    {
        if ((ret = write( ntdll_get_thread_data()->request_fd, &req->u.req,
                          sizeof(req->u.req) )) == sizeof(req->u.req)) return STATUS_SUCCESS;

    }
    else
    {
        struct iovec vec[__SERVER_MAX_DATA+1];

        vec[0].iov_base = (void *)&req->u.req;
        vec[0].iov_len = sizeof(req->u.req);
        for (i = 0; i < req->data_count; i++)
        {
            vec[i+1].iov_base = (void *)req->data[i].ptr;
            vec[i+1].iov_len = req->data[i].size;
        }
        if ((ret = writev( ntdll_get_thread_data()->request_fd, vec, i+1 )) ==
            req->u.req.request_header.request_size + sizeof(req->u.req)) return STATUS_SUCCESS;
    }

    if (ret >= 0) server_protocol_error( "partial write %d\n", ret );
    if (errno == EPIPE) abort_thread(0);
    if (errno == EFAULT) return STATUS_ACCESS_VIOLATION;
    server_protocol_perror( "write" );
}


/***********************************************************************
 *           read_reply_data
 *
 * Read data from the reply buffer; helper for wait_reply.
 */
static void read_reply_data( void *buffer, size_t size )
{
    int ret;

    for (;;)
    {
        if ((ret = read( ntdll_get_thread_data()->reply_fd, buffer, size )) > 0)
        {
            if (!(size -= ret)) return;
            buffer = (char *)buffer + ret;
            continue;
        }
        if (!ret) break;
        if (errno == EINTR) continue;
        if (errno == EPIPE) break;
        server_protocol_perror("read");
    }
    /* the server closed the connection; time to die... */
    abort_thread(0);
}


/***********************************************************************
 *           wait_reply
 *
 * Wait for a reply from the server.
 */
static inline unsigned int wait_reply( struct __server_request_info *req )
{
    read_reply_data( &req->u.reply, sizeof(req->u.reply) );
    if (req->u.reply.reply_header.reply_size)
        read_reply_data( req->reply_data, req->u.reply.reply_header.reply_size );
    return req->u.reply.reply_header.error;
}


/***********************************************************************
 *           server_call_unlocked
 */
unsigned int server_call_unlocked( void *req_ptr )
{
    struct __server_request_info * const req = req_ptr;
    unsigned int ret;

    if ((ret = send_request( req ))) return ret;
    return wait_reply( req );
}


/***********************************************************************
 *           wine_server_call
 *
 * Perform a server call.
 */
unsigned int CDECL wine_server_call( void *req_ptr )
{
    sigset_t old_set;
    unsigned int ret;

    pthread_sigmask( SIG_BLOCK, &server_block_set, &old_set );
    ret = server_call_unlocked( req_ptr );
    pthread_sigmask( SIG_SETMASK, &old_set, NULL );
    return ret;
}


/***********************************************************************
 *           server_enter_uninterrupted_section
 */
void server_enter_uninterrupted_section( pthread_mutex_t *mutex, sigset_t *sigset )
{
    pthread_sigmask( SIG_BLOCK, &server_block_set, sigset );
    mutex_lock( mutex );
}


/***********************************************************************
 *           server_leave_uninterrupted_section
 */
void server_leave_uninterrupted_section( pthread_mutex_t *mutex, sigset_t *sigset )
{
    mutex_unlock( mutex );
    pthread_sigmask( SIG_SETMASK, sigset, NULL );
}


/***********************************************************************
 *              wait_select_reply
 *
 * Wait for a reply on the waiting pipe of the current thread.
 */
static int wait_select_reply( void *cookie )
{
    int signaled;
    struct wake_up_reply reply;
    for (;;)
    {
        int ret;
        ret = read( ntdll_get_thread_data()->wait_fd[0], &reply, sizeof(reply) );
        if (ret == sizeof(reply))
        {
            if (!reply.cookie) abort_thread( reply.signaled );  /* thread got killed */
            if (wine_server_get_ptr(reply.cookie) == cookie) return reply.signaled;
            /* we stole another reply, wait for the real one */
            signaled = wait_select_reply( cookie );
            /* and now put the wrong one back in the pipe */
            for (;;)
            {
                ret = write( ntdll_get_thread_data()->wait_fd[1], &reply, sizeof(reply) );
                if (ret == sizeof(reply)) break;
                if (ret >= 0) server_protocol_error( "partial wakeup write %d\n", ret );
                if (errno == EINTR) continue;
                server_protocol_perror("wakeup write");
            }
            return signaled;
        }
        if (ret >= 0) server_protocol_error( "partial wakeup read %d\n", ret );
        if (errno == EINTR) continue;
        server_protocol_perror("wakeup read");
    }
}


/***********************************************************************
 *              invoke_user_apc
 */
static NTSTATUS invoke_user_apc( CONTEXT *context, const user_apc_t *apc, NTSTATUS status )
{
    return call_user_apc_dispatcher( context, apc->args[0], apc->args[1], apc->args[2],
                                     wine_server_get_ptr( apc->func ), status );
}


/***********************************************************************
 *              invoke_system_apc
 */
static void invoke_system_apc( const apc_call_t *call, apc_result_t *result, BOOL self )
{
    SIZE_T size, bits;
    void *addr;

    memset( result, 0, sizeof(*result) );

    switch (call->type)
    {
    case APC_NONE:
        break;
    case APC_ASYNC_IO:
    {
        IO_STATUS_BLOCK *iosb = wine_server_get_ptr( call->async_io.sb );
        struct async_fileio *user = wine_server_get_ptr( call->async_io.user );
        ULONG_PTR info = 0;

        result->type = call->type;
        result->async_io.status = user->callback( user, &info, call->async_io.status );
        if (result->async_io.status != STATUS_PENDING)
        {
            result->async_io.total = info;
            iosb->Status = result->async_io.status;
            iosb->Information = info;
        }
        break;
    }
    case APC_VIRTUAL_ALLOC:
        result->type = call->type;
        addr = wine_server_get_ptr( call->virtual_alloc.addr );
        size = call->virtual_alloc.size;
        bits = call->virtual_alloc.zero_bits;
        if ((ULONG_PTR)addr == call->virtual_alloc.addr && size == call->virtual_alloc.size &&
            bits == call->virtual_alloc.zero_bits)
        {
            result->virtual_alloc.status = NtAllocateVirtualMemory( NtCurrentProcess(), &addr, bits, &size,
                                                                    call->virtual_alloc.op_type,
                                                                    call->virtual_alloc.prot );
            result->virtual_alloc.addr = wine_server_client_ptr( addr );
            result->virtual_alloc.size = size;
        }
        else result->virtual_alloc.status = STATUS_WORKING_SET_LIMIT_RANGE;
        break;
    case APC_VIRTUAL_FREE:
        result->type = call->type;
        addr = wine_server_get_ptr( call->virtual_free.addr );
        size = call->virtual_free.size;
        if ((ULONG_PTR)addr == call->virtual_free.addr && size == call->virtual_free.size)
        {
            result->virtual_free.status = NtFreeVirtualMemory( NtCurrentProcess(), &addr, &size,
                                                               call->virtual_free.op_type );
            result->virtual_free.addr = wine_server_client_ptr( addr );
            result->virtual_free.size = size;
        }
        else result->virtual_free.status = STATUS_INVALID_PARAMETER;
        break;
    case APC_VIRTUAL_QUERY:
    {
        MEMORY_BASIC_INFORMATION info;
        result->type = call->type;
        addr = wine_server_get_ptr( call->virtual_query.addr );
        if ((ULONG_PTR)addr == call->virtual_query.addr)
            result->virtual_query.status = NtQueryVirtualMemory( NtCurrentProcess(),
                                                                 addr, MemoryBasicInformation, &info,
                                                                 sizeof(info), NULL );
        else
            result->virtual_query.status = STATUS_WORKING_SET_LIMIT_RANGE;

        if (result->virtual_query.status == STATUS_SUCCESS)
        {
            result->virtual_query.base       = wine_server_client_ptr( info.BaseAddress );
            result->virtual_query.alloc_base = wine_server_client_ptr( info.AllocationBase );
            result->virtual_query.size       = info.RegionSize;
            result->virtual_query.prot       = info.Protect;
            result->virtual_query.alloc_prot = info.AllocationProtect;
            result->virtual_query.state      = info.State >> 12;
            result->virtual_query.alloc_type = info.Type >> 16;
        }
        break;
    }
    case APC_VIRTUAL_PROTECT:
        result->type = call->type;
        addr = wine_server_get_ptr( call->virtual_protect.addr );
        size = call->virtual_protect.size;
        if ((ULONG_PTR)addr == call->virtual_protect.addr && size == call->virtual_protect.size)
        {
            result->virtual_protect.status = NtProtectVirtualMemory( NtCurrentProcess(), &addr, &size,
                                                                     call->virtual_protect.prot,
                                                                     &result->virtual_protect.prot );
            result->virtual_protect.addr = wine_server_client_ptr( addr );
            result->virtual_protect.size = size;
        }
        else result->virtual_protect.status = STATUS_INVALID_PARAMETER;
        break;
    case APC_VIRTUAL_FLUSH:
        result->type = call->type;
        addr = wine_server_get_ptr( call->virtual_flush.addr );
        size = call->virtual_flush.size;
        if ((ULONG_PTR)addr == call->virtual_flush.addr && size == call->virtual_flush.size)
        {
            result->virtual_flush.status = NtFlushVirtualMemory( NtCurrentProcess(),
                                                                 (const void **)&addr, &size, 0 );
            result->virtual_flush.addr = wine_server_client_ptr( addr );
            result->virtual_flush.size = size;
        }
        else result->virtual_flush.status = STATUS_INVALID_PARAMETER;
        break;
    case APC_VIRTUAL_LOCK:
        result->type = call->type;
        addr = wine_server_get_ptr( call->virtual_lock.addr );
        size = call->virtual_lock.size;
        if ((ULONG_PTR)addr == call->virtual_lock.addr && size == call->virtual_lock.size)
        {
            result->virtual_lock.status = NtLockVirtualMemory( NtCurrentProcess(), &addr, &size, 0 );
            result->virtual_lock.addr = wine_server_client_ptr( addr );
            result->virtual_lock.size = size;
        }
        else result->virtual_lock.status = STATUS_INVALID_PARAMETER;
        break;
    case APC_VIRTUAL_UNLOCK:
        result->type = call->type;
        addr = wine_server_get_ptr( call->virtual_unlock.addr );
        size = call->virtual_unlock.size;
        if ((ULONG_PTR)addr == call->virtual_unlock.addr && size == call->virtual_unlock.size)
        {
            result->virtual_unlock.status = NtUnlockVirtualMemory( NtCurrentProcess(), &addr, &size, 0 );
            result->virtual_unlock.addr = wine_server_client_ptr( addr );
            result->virtual_unlock.size = size;
        }
        else result->virtual_unlock.status = STATUS_INVALID_PARAMETER;
        break;
    case APC_MAP_VIEW:
        result->type = call->type;
        addr = wine_server_get_ptr( call->map_view.addr );
        size = call->map_view.size;
        bits = call->map_view.zero_bits;
        if ((ULONG_PTR)addr == call->map_view.addr && size == call->map_view.size &&
            bits == call->map_view.zero_bits)
        {
            LARGE_INTEGER offset;
            offset.QuadPart = call->map_view.offset;
            result->map_view.status = NtMapViewOfSection( wine_server_ptr_handle(call->map_view.handle),
                                                          NtCurrentProcess(),
                                                          &addr, bits, 0, &offset, &size, 0,
                                                          call->map_view.alloc_type, call->map_view.prot );
            result->map_view.addr = wine_server_client_ptr( addr );
            result->map_view.size = size;
        }
        else result->map_view.status = STATUS_INVALID_PARAMETER;
        if (!self) NtClose( wine_server_ptr_handle(call->map_view.handle) );
        break;
    case APC_UNMAP_VIEW:
        result->type = call->type;
        addr = wine_server_get_ptr( call->unmap_view.addr );
        if ((ULONG_PTR)addr == call->unmap_view.addr)
            result->unmap_view.status = NtUnmapViewOfSection( NtCurrentProcess(), addr );
        else
            result->unmap_view.status = STATUS_INVALID_PARAMETER;
        break;
    case APC_CREATE_THREAD:
    {
        ULONG_PTR buffer[offsetof( PS_ATTRIBUTE_LIST, Attributes[2] ) / sizeof(ULONG_PTR)];
        PS_ATTRIBUTE_LIST *attr = (PS_ATTRIBUTE_LIST *)buffer;
        CLIENT_ID id;
        HANDLE handle;
        TEB *teb;
        ULONG_PTR zero_bits = call->create_thread.zero_bits;
        SIZE_T reserve = call->create_thread.reserve;
        SIZE_T commit = call->create_thread.commit;
        void *func = wine_server_get_ptr( call->create_thread.func );
        void *arg  = wine_server_get_ptr( call->create_thread.arg );

        result->type = call->type;
        if (reserve == call->create_thread.reserve && commit == call->create_thread.commit &&
            (ULONG_PTR)func == call->create_thread.func && (ULONG_PTR)arg == call->create_thread.arg)
        {
            attr->TotalLength = sizeof(buffer);
            attr->Attributes[0].Attribute    = PS_ATTRIBUTE_CLIENT_ID;
            attr->Attributes[0].Size         = sizeof(id);
            attr->Attributes[0].ValuePtr     = &id;
            attr->Attributes[0].ReturnLength = NULL;
            attr->Attributes[1].Attribute    = PS_ATTRIBUTE_TEB_ADDRESS;
            attr->Attributes[1].Size         = sizeof(teb);
            attr->Attributes[1].ValuePtr     = &teb;
            attr->Attributes[1].ReturnLength = NULL;
            result->create_thread.status = NtCreateThreadEx( &handle, THREAD_ALL_ACCESS, NULL,
                                                             NtCurrentProcess(), func, arg,
                                                             call->create_thread.flags, zero_bits,
                                                             commit, reserve, attr );
            result->create_thread.handle = wine_server_obj_handle( handle );
            result->create_thread.pid = HandleToULong(id.UniqueProcess);
            result->create_thread.tid = HandleToULong(id.UniqueThread);
            result->create_thread.teb = wine_server_client_ptr( teb );
        }
        else result->create_thread.status = STATUS_INVALID_PARAMETER;
        break;
    }
    case APC_DUP_HANDLE:
    {
        HANDLE dst_handle = NULL;

        result->type = call->type;

        result->dup_handle.status = NtDuplicateObject( NtCurrentProcess(),
                                                       wine_server_ptr_handle(call->dup_handle.src_handle),
                                                       wine_server_ptr_handle(call->dup_handle.dst_process),
                                                       &dst_handle, call->dup_handle.access,
                                                       call->dup_handle.attributes, call->dup_handle.options );
        result->dup_handle.handle = wine_server_obj_handle( dst_handle );
        if (!self) NtClose( wine_server_ptr_handle(call->dup_handle.dst_process) );
        break;
    }
    case APC_BREAK_PROCESS:
    {
        HANDLE handle;

        result->type = APC_BREAK_PROCESS;
        result->break_process.status = NtCreateThreadEx( &handle, THREAD_ALL_ACCESS, NULL,
                                                         NtCurrentProcess(), pDbgUiRemoteBreakin, NULL,
                                                         0, 0, 0, 0, NULL );
        if (!result->break_process.status) NtClose( handle );
        break;
    }
    default:
        server_protocol_error( "get_apc_request: bad type %d\n", call->type );
        break;
    }
}


/***********************************************************************
 *              server_select
 */
unsigned int server_select( const select_op_t *select_op, data_size_t size, UINT flags,
                            timeout_t abs_timeout, context_t *context, pthread_mutex_t *mutex,
                            user_apc_t *user_apc )
{
    unsigned int ret;
    int cookie;
    obj_handle_t apc_handle = 0;
    BOOL suspend_context = !!context;
    apc_call_t call;
    apc_result_t result;
    sigset_t old_set;
    int signaled;

    memset( &result, 0, sizeof(result) );

    do
    {
        pthread_sigmask( SIG_BLOCK, &server_block_set, &old_set );
        for (;;)
        {
            SERVER_START_REQ( select )
            {
                req->flags    = flags;
                req->cookie   = wine_server_client_ptr( &cookie );
                req->prev_apc = apc_handle;
                req->timeout  = abs_timeout;
                req->size     = size;
                wine_server_add_data( req, &result, sizeof(result) );
                wine_server_add_data( req, select_op, size );
                if (suspend_context)
                {
                    data_size_t ctx_size = (context[1].machine ? 2 : 1) * sizeof(*context);
                    wine_server_add_data( req, context, ctx_size );
                    suspend_context = FALSE; /* server owns the context now */
                }
                if (context) wine_server_set_reply( req, context, 2 * sizeof(*context) );
                ret = server_call_unlocked( req );
                signaled    = reply->signaled;
                apc_handle  = reply->apc_handle;
                call        = reply->call;
            }
            SERVER_END_REQ;

            if (ret != STATUS_KERNEL_APC) break;
            invoke_system_apc( &call, &result, FALSE );

            /* don't signal multiple times */
            if (size >= sizeof(select_op->signal_and_wait) && select_op->op == SELECT_SIGNAL_AND_WAIT)
                size = offsetof( select_op_t, signal_and_wait.signal );
        }
        pthread_sigmask( SIG_SETMASK, &old_set, NULL );
        if (mutex)
        {
            mutex_unlock( mutex );
            mutex = NULL;
        }
        if (signaled) break;

        ret = wait_select_reply( &cookie );
    }
    while (ret == STATUS_USER_APC || ret == STATUS_KERNEL_APC);

    if (ret == STATUS_USER_APC) *user_apc = call.user;
    return ret;
}


/***********************************************************************
 *              server_wait
 */
unsigned int server_wait( const select_op_t *select_op, data_size_t size, UINT flags,
                          const LARGE_INTEGER *timeout )
{
    timeout_t abs_timeout = timeout ? timeout->QuadPart : TIMEOUT_INFINITE;
    unsigned int ret;
    user_apc_t apc;

    if (abs_timeout < 0)
    {
        LARGE_INTEGER now;

        NtQueryPerformanceCounter( &now, NULL );
        abs_timeout -= now.QuadPart;
    }

    ret = server_select( select_op, size, flags, abs_timeout, NULL, NULL, &apc );
    if (ret == STATUS_USER_APC) return invoke_user_apc( NULL, &apc, ret );

    /* A test on Windows 2000 shows that Windows always yields during
       a wait, but a wait that is hit by an event gets a priority
       boost as well.  This seems to model that behavior the closest.  */
    if (ret == STATUS_TIMEOUT) NtYieldExecution();
    return ret;
}


/***********************************************************************
 *              NtContinue  (NTDLL.@)
 */
NTSTATUS WINAPI NtContinue( CONTEXT *context, BOOLEAN alertable )
{
    user_apc_t apc;
    NTSTATUS status;

    if (alertable)
    {
        status = server_select( NULL, 0, SELECT_INTERRUPTIBLE | SELECT_ALERTABLE, 0, NULL, NULL, &apc );
        if (status == STATUS_USER_APC) return invoke_user_apc( context, &apc, status );
    }
    return signal_set_full_context( context );
}


/***********************************************************************
 *              NtTestAlert  (NTDLL.@)
 */
NTSTATUS WINAPI NtTestAlert(void)
{
    user_apc_t apc;
    NTSTATUS status;

    status = server_select( NULL, 0, SELECT_INTERRUPTIBLE | SELECT_ALERTABLE, 0, NULL, NULL, &apc );
    if (status == STATUS_USER_APC) invoke_user_apc( NULL, &apc, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           server_queue_process_apc
 */
unsigned int server_queue_process_apc( HANDLE process, const apc_call_t *call, apc_result_t *result )
{
    for (;;)
    {
        unsigned int ret;
        HANDLE handle = 0;
        BOOL self = FALSE;

        SERVER_START_REQ( queue_apc )
        {
            req->handle = wine_server_obj_handle( process );
            req->call = *call;
            if (!(ret = wine_server_call( req )))
            {
                handle = wine_server_ptr_handle( reply->handle );
                self = reply->self;
            }
        }
        SERVER_END_REQ;
        if (ret != STATUS_SUCCESS) return ret;

        if (self)
        {
            invoke_system_apc( call, result, TRUE );
        }
        else
        {
            NtWaitForSingleObject( handle, FALSE, NULL );

            SERVER_START_REQ( get_apc_result )
            {
                req->handle = wine_server_obj_handle( handle );
                if (!(ret = wine_server_call( req ))) *result = reply->result;
            }
            SERVER_END_REQ;

            if (!ret && result->type == APC_NONE) continue;  /* APC didn't run, try again */
        }
        return ret;
    }
}


/***********************************************************************
 *           wine_server_send_fd
 *
 * Send a file descriptor to the server.
 */
void wine_server_send_fd( int fd )
{
    struct send_fd data;
    struct msghdr msghdr;
    struct iovec vec;
    int ret;

#ifdef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    msghdr.msg_accrights    = (void *)&fd;
    msghdr.msg_accrightslen = sizeof(fd);
#else  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */
    char cmsg_buffer[256];
    struct cmsghdr *cmsg;
    msghdr.msg_control    = cmsg_buffer;
    msghdr.msg_controllen = sizeof(cmsg_buffer);
    msghdr.msg_flags      = 0;
    cmsg = CMSG_FIRSTHDR( &msghdr );
    cmsg->cmsg_len   = CMSG_LEN( sizeof(fd) );
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type  = SCM_RIGHTS;
    *(int *)CMSG_DATA(cmsg) = fd;
    msghdr.msg_controllen = cmsg->cmsg_len;
#endif  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */

    msghdr.msg_name    = NULL;
    msghdr.msg_namelen = 0;
    msghdr.msg_iov     = &vec;
    msghdr.msg_iovlen  = 1;

    vec.iov_base = (void *)&data;
    vec.iov_len  = sizeof(data);

    data.tid = GetCurrentThreadId();
    data.fd  = fd;

    for (;;)
    {
        if ((ret = sendmsg( fd_socket, &msghdr, 0 )) == sizeof(data)) return;
        if (ret >= 0) server_protocol_error( "partial write %d\n", ret );
        if (errno == EINTR) continue;
        if (errno == EPIPE) abort_thread(0);
        server_protocol_perror( "sendmsg" );
    }
}


/***********************************************************************
 *           receive_fd
 *
 * Receive a file descriptor passed from the server.
 */
static int receive_fd( obj_handle_t *handle )
{
    struct iovec vec;
    struct msghdr msghdr;
    int ret, fd = -1;

#ifdef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    msghdr.msg_accrights    = (void *)&fd;
    msghdr.msg_accrightslen = sizeof(fd);
#else  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */
    char cmsg_buffer[256];
    msghdr.msg_control    = cmsg_buffer;
    msghdr.msg_controllen = sizeof(cmsg_buffer);
    msghdr.msg_flags      = 0;
#endif  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */

    msghdr.msg_name    = NULL;
    msghdr.msg_namelen = 0;
    msghdr.msg_iov     = &vec;
    msghdr.msg_iovlen  = 1;
    vec.iov_base = (void *)handle;
    vec.iov_len  = sizeof(*handle);

    for (;;)
    {
        if ((ret = recvmsg( fd_socket, &msghdr, MSG_CMSG_CLOEXEC )) > 0)
        {
#ifndef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
            struct cmsghdr *cmsg;
            for (cmsg = CMSG_FIRSTHDR( &msghdr ); cmsg; cmsg = CMSG_NXTHDR( &msghdr, cmsg ))
            {
                if (cmsg->cmsg_level != SOL_SOCKET) continue;
                if (cmsg->cmsg_type == SCM_RIGHTS) fd = *(int *)CMSG_DATA(cmsg);
#ifdef SCM_CREDENTIALS
                else if (cmsg->cmsg_type == SCM_CREDENTIALS)
                {
                    struct ucred *ucred = (struct ucred *)CMSG_DATA(cmsg);
                    server_pid = ucred->pid;
                }
#endif
            }
#endif  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */
            if (fd != -1) fcntl( fd, F_SETFD, FD_CLOEXEC ); /* in case MSG_CMSG_CLOEXEC is not supported */
            return fd;
        }
        if (!ret) break;
        if (errno == EINTR) continue;
        if (errno == EPIPE) break;
        server_protocol_perror("recvmsg");
    }
    /* the server closed the connection; time to die... */
    abort_thread(0);
}


/***********************************************************************/
/* fd cache support */

union fd_cache_entry
{
    LONG64 data;
    struct
    {
        int fd;
        enum server_fd_type type : 5;
        unsigned int        access : 3;
        unsigned int        options : 24;
    } s;
};

C_ASSERT( sizeof(union fd_cache_entry) == sizeof(LONG64) );

#define FD_CACHE_BLOCK_SIZE  (65536 / sizeof(union fd_cache_entry))
#define FD_CACHE_ENTRIES     128

static union fd_cache_entry *fd_cache[FD_CACHE_ENTRIES];
static union fd_cache_entry fd_cache_initial_block[FD_CACHE_BLOCK_SIZE];

static inline unsigned int handle_to_index( HANDLE handle, unsigned int *entry )
{
    unsigned int idx = (wine_server_obj_handle(handle) >> 2) - 1;
    *entry = idx / FD_CACHE_BLOCK_SIZE;
    return idx % FD_CACHE_BLOCK_SIZE;
}


/***********************************************************************
 *           add_fd_to_cache
 *
 * Caller must hold fd_cache_mutex.
 */
static BOOL add_fd_to_cache( HANDLE handle, int fd, enum server_fd_type type,
                            unsigned int access, unsigned int options )
{
    unsigned int entry, idx = handle_to_index( handle, &entry );
    union fd_cache_entry cache;

    if (entry >= FD_CACHE_ENTRIES)
    {
        FIXME( "too many allocated handles, not caching %p\n", handle );
        return FALSE;
    }

    if (!fd_cache[entry])  /* do we need to allocate a new block of entries? */
    {
        if (!entry) fd_cache[0] = fd_cache_initial_block;
        else
        {
            void *ptr = anon_mmap_alloc( FD_CACHE_BLOCK_SIZE * sizeof(union fd_cache_entry),
                                         PROT_READ | PROT_WRITE );
            if (ptr == MAP_FAILED) return FALSE;
            fd_cache[entry] = ptr;
        }
    }

    /* store fd+1 so that 0 can be used as the unset value */
    cache.s.fd = fd + 1;
    cache.s.type = type;
    cache.s.access = access;
    cache.s.options = options;
    cache.data = interlocked_xchg64( &fd_cache[entry][idx].data, cache.data );
    assert( !cache.s.fd );
    return TRUE;
}


/***********************************************************************
 *           get_cached_fd
 */
static inline NTSTATUS get_cached_fd( HANDLE handle, int *fd, enum server_fd_type *type,
                                      unsigned int *access, unsigned int *options )
{
    unsigned int entry, idx = handle_to_index( handle, &entry );
    union fd_cache_entry cache;

    if (entry >= FD_CACHE_ENTRIES || !fd_cache[entry]) return STATUS_INVALID_HANDLE;

    cache.data = InterlockedCompareExchange64( &fd_cache[entry][idx].data, 0, 0 );
    if (!cache.data) return STATUS_INVALID_HANDLE;

    /* if fd type is invalid, fd stores an error value */
    if (cache.s.type == FD_TYPE_INVALID) return cache.s.fd - 1;

    *fd = cache.s.fd - 1;
    if (type) *type = cache.s.type;
    if (access) *access = cache.s.access;
    if (options) *options = cache.s.options;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           remove_fd_from_cache
 */
static int remove_fd_from_cache( HANDLE handle )
{
    unsigned int entry, idx = handle_to_index( handle, &entry );
    int fd = -1;

    if (entry < FD_CACHE_ENTRIES && fd_cache[entry])
    {
        union fd_cache_entry cache;
        cache.data = interlocked_xchg64( &fd_cache[entry][idx].data, 0 );
        if (cache.s.type != FD_TYPE_INVALID) fd = cache.s.fd - 1;
    }

    return fd;
}


/***********************************************************************
 *           server_get_unix_fd
 *
 * The returned unix_fd should be closed iff needs_close is non-zero.
 */
int server_get_unix_fd( HANDLE handle, unsigned int wanted_access, int *unix_fd,
                        int *needs_close, enum server_fd_type *type, unsigned int *options )
{
    sigset_t sigset;
    obj_handle_t fd_handle;
    int ret, fd = -1;
    unsigned int access = 0;

    *unix_fd = -1;
    *needs_close = 0;
    wanted_access &= FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA;

    ret = get_cached_fd( handle, &fd, type, &access, options );
    if (ret != STATUS_INVALID_HANDLE) goto done;

    server_enter_uninterrupted_section( &fd_cache_mutex, &sigset );
    ret = get_cached_fd( handle, &fd, type, &access, options );
    if (ret == STATUS_INVALID_HANDLE)
    {
        SERVER_START_REQ( get_handle_fd )
        {
            req->handle = wine_server_obj_handle( handle );
            if (!(ret = wine_server_call( req )))
            {
                if (type) *type = reply->type;
                if (options) *options = reply->options;
                access = reply->access;
                if ((fd = receive_fd( &fd_handle )) != -1)
                {
                    assert( wine_server_ptr_handle(fd_handle) == handle );
                    *needs_close = (!reply->cacheable ||
                                    !add_fd_to_cache( handle, fd, reply->type,
                                                      reply->access, reply->options ));
                }
                else ret = STATUS_TOO_MANY_OPENED_FILES;
            }
            else if (reply->cacheable)
            {
                add_fd_to_cache( handle, ret, FD_TYPE_INVALID, 0, 0 );
            }
        }
        SERVER_END_REQ;
    }
    server_leave_uninterrupted_section( &fd_cache_mutex, &sigset );

done:
    if (!ret && ((access & wanted_access) != wanted_access))
    {
        ret = STATUS_ACCESS_DENIED;
        if (*needs_close) close( fd );
    }
    if (!ret) *unix_fd = fd;
    return ret;
}


/***********************************************************************
 *           wine_server_fd_to_handle
 */
NTSTATUS CDECL wine_server_fd_to_handle( int fd, unsigned int access, unsigned int attributes, HANDLE *handle )
{
    NTSTATUS ret;

    *handle = 0;
    wine_server_send_fd( fd );

    SERVER_START_REQ( alloc_file_handle )
    {
        req->access     = access;
        req->attributes = attributes;
        req->fd         = fd;
        if (!(ret = wine_server_call( req ))) *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           wine_server_handle_to_fd
 *
 * Retrieve the file descriptor corresponding to a file handle.
 */
NTSTATUS CDECL wine_server_handle_to_fd( HANDLE handle, unsigned int access, int *unix_fd,
                                         unsigned int *options )
{
    int needs_close;
    NTSTATUS ret = server_get_unix_fd( handle, access, unix_fd, &needs_close, NULL, options );

    if (!ret && !needs_close)
    {
        if ((*unix_fd = dup(*unix_fd)) == -1) ret = STATUS_TOO_MANY_OPENED_FILES;
    }
    return ret;
}


/***********************************************************************
 *           server_pipe
 *
 * Create a pipe for communicating with the server.
 */
int server_pipe( int fd[2] )
{
    int ret;
#ifdef HAVE_PIPE2
    static BOOL have_pipe2 = TRUE;

    if (have_pipe2)
    {
        if (!(ret = pipe2( fd, O_CLOEXEC ))) return ret;
        if (errno == ENOSYS || errno == EINVAL) have_pipe2 = FALSE;  /* don't try again */
    }
#endif
    if (!(ret = pipe( fd )))
    {
        fcntl( fd[0], F_SETFD, FD_CLOEXEC );
        fcntl( fd[1], F_SETFD, FD_CLOEXEC );
    }
    return ret;
}


/***********************************************************************
 *           init_server_dir
 */
static const char *init_server_dir( dev_t dev, ino_t ino )
{
    char *p, *dir;
    size_t len = sizeof("/server-") + 2 * sizeof(dev) + 2 * sizeof(ino) + 2;

#ifdef __ANDROID__  /* there's no /tmp dir on Android */
    len += strlen( config_dir ) + sizeof("/.wineserver");
    dir = malloc( len );
    strcpy( dir, config_dir );
    strcat( dir, "/.wineserver/server-" );
#else
    len += sizeof("/tmp/.wine-") + 12;
    dir = malloc( len );
    sprintf( dir, "/tmp/.wine-%u/server-", getuid() );
#endif
    p = dir + strlen( dir );
    if (dev != (unsigned long)dev)
        p += sprintf( p, "%lx%08lx-", (unsigned long)((unsigned long long)dev >> 32), (unsigned long)dev );
    else
        p += sprintf( p, "%lx-", (unsigned long)dev );

    if (ino != (unsigned long)ino)
        sprintf( p, "%lx%08lx", (unsigned long)((unsigned long long)ino >> 32), (unsigned long)ino );
    else
        sprintf( p, "%lx", (unsigned long)ino );
    return dir;
}


/***********************************************************************
 *           setup_config_dir
 *
 * Setup the wine configuration dir.
 */
static int setup_config_dir(void)
{
    char *p;
    struct stat st;
    int fd_cwd = open( ".", O_RDONLY );

    if (chdir( config_dir ) == -1)
    {
        if (errno != ENOENT) fatal_perror( "cannot use directory %s", config_dir );
        if ((p = strrchr( config_dir, '/' )) && p != config_dir)
        {
            while (p > config_dir + 1 && p[-1] == '/') p--;
            *p = 0;
            if (!stat( config_dir, &st ) && st.st_uid != getuid())
                fatal_error( "'%s' is not owned by you, refusing to create a configuration directory there\n",
                             config_dir );
            *p = '/';
        }
        mkdir( config_dir, 0777 );
        if (chdir( config_dir ) == -1) fatal_perror( "chdir to %s", config_dir );
        MESSAGE( "wine: created the configuration directory '%s'\n", config_dir );
    }

    if (stat( ".", &st ) == -1) fatal_perror( "stat %s", config_dir );
    if (st.st_uid != getuid()) fatal_error( "'%s' is not owned by you\n", config_dir );

    server_dir = init_server_dir( st.st_dev, st.st_ino );

    if (!mkdir( "dosdevices", 0777 ))
    {
        mkdir( "drive_c", 0777 );
        symlink( "../drive_c", "dosdevices/c:" );
        symlink( "/", "dosdevices/z:" );
    }
    else if (errno != EEXIST) fatal_perror( "cannot create %s/dosdevices", config_dir );

    if (fd_cwd == -1) fd_cwd = open( "dosdevices/c:", O_RDONLY );
    fcntl( fd_cwd, F_SETFD, FD_CLOEXEC );
    return fd_cwd;
}


/***********************************************************************
 *           server_connect_error
 *
 * Try to display a meaningful explanation of why we couldn't connect
 * to the server.
 */
static void server_connect_error( const char *serverdir )
{
    int fd;
    struct flock fl;

    if ((fd = open( LOCKNAME, O_WRONLY )) == -1)
        fatal_error( "for some mysterious reason, the wine server never started.\n" );

    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 1;
    if (fcntl( fd, F_GETLK, &fl ) != -1)
    {
        if (fl.l_type == F_WRLCK)  /* the file is locked */
            fatal_error( "a wine server seems to be running, but I cannot connect to it.\n"
                         "   You probably need to kill that process (it might be pid %d).\n",
                         (int)fl.l_pid );
        fatal_error( "for some mysterious reason, the wine server failed to run.\n" );
    }
    fatal_error( "the file system of '%s' doesn't support locks,\n"
          "   and there is a 'socket' file in that directory that prevents wine from starting.\n"
          "   You should make sure no wine server is running, remove that file and try again.\n",
                 serverdir );
}


/***********************************************************************
 *           server_connect
 *
 * Attempt to connect to an existing server socket.
 * We need to be in the server directory already.
 */
static int server_connect(void)
{
    struct sockaddr_un addr;
    struct stat st;
    int s, slen, retry, fd_cwd;

    fd_cwd = setup_config_dir();

    /* chdir to the server directory */
    if (chdir( server_dir ) == -1)
    {
        if (errno != ENOENT) fatal_perror( "chdir to %s", server_dir );
        start_server( TRACE_ON(server) );
        if (chdir( server_dir ) == -1) fatal_perror( "chdir to %s", server_dir );
    }

    /* make sure we are at the right place */
    if (stat( ".", &st ) == -1) fatal_perror( "stat %s", server_dir );
    if (st.st_uid != getuid()) fatal_error( "'%s' is not owned by you\n", server_dir );
    if (st.st_mode & 077) fatal_error( "'%s' must not be accessible by other users\n", server_dir );

    for (retry = 0; retry < 6; retry++)
    {
        /* if not the first try, wait a bit to leave the previous server time to exit */
        if (retry)
        {
            usleep( 100000 * retry * retry );
            start_server( TRACE_ON(server) );
            if (lstat( SOCKETNAME, &st ) == -1) continue;  /* still no socket, wait a bit more */
        }
        else if (lstat( SOCKETNAME, &st ) == -1) /* check for an already existing socket */
        {
            if (errno != ENOENT) fatal_perror( "lstat %s/%s", server_dir, SOCKETNAME );
            start_server( TRACE_ON(server) );
            if (lstat( SOCKETNAME, &st ) == -1) continue;  /* still no socket, wait a bit more */
        }

        /* make sure the socket is sane (ISFIFO needed for Solaris) */
        if (!S_ISSOCK(st.st_mode) && !S_ISFIFO(st.st_mode))
            fatal_error( "'%s/%s' is not a socket\n", server_dir, SOCKETNAME );
        if (st.st_uid != getuid())
            fatal_error( "'%s/%s' is not owned by you\n", server_dir, SOCKETNAME );

        /* try to connect to it */
        addr.sun_family = AF_UNIX;
        strcpy( addr.sun_path, SOCKETNAME );
        slen = sizeof(addr) - sizeof(addr.sun_path) + strlen(addr.sun_path) + 1;
#ifdef HAVE_STRUCT_SOCKADDR_UN_SUN_LEN
        addr.sun_len = slen;
#endif
        if ((s = socket( AF_UNIX, SOCK_STREAM, 0 )) == -1) fatal_perror( "socket" );
#ifdef SO_PASSCRED
        else
        {
            int enable = 1;
            setsockopt( s, SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable) );
        }
#endif
        if (connect( s, (struct sockaddr *)&addr, slen ) != -1)
        {
            /* switch back to the starting directory */
            if (fd_cwd != -1)
            {
                fchdir( fd_cwd );
                close( fd_cwd );
            }
            fcntl( s, F_SETFD, FD_CLOEXEC );
            return s;
        }
        close( s );
    }
    server_connect_error( server_dir );
}


#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <servers/bootstrap.h>

/* send our task port to the server */
static void send_server_task_port(void)
{
    mach_port_t bootstrap_port, wineserver_port;
    kern_return_t kret;

    struct {
        mach_msg_header_t           header;
        mach_msg_body_t             body;
        mach_msg_port_descriptor_t  task_port;
    } msg;

    if (task_get_bootstrap_port(mach_task_self(), &bootstrap_port) != KERN_SUCCESS) return;

    if (!server_dir)
    {
        struct stat st;
        stat( config_dir, &st );
        server_dir = init_server_dir( st.st_dev, st.st_ino );
    }
    kret = bootstrap_look_up(bootstrap_port, server_dir, &wineserver_port);
    if (kret != KERN_SUCCESS)
        fatal_error( "cannot find the server port: 0x%08x\n", kret );

    mach_port_deallocate(mach_task_self(), bootstrap_port);

    msg.header.msgh_bits        = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0) | MACH_MSGH_BITS_COMPLEX;
    msg.header.msgh_size        = sizeof(msg);
    msg.header.msgh_remote_port = wineserver_port;
    msg.header.msgh_local_port  = MACH_PORT_NULL;

    msg.body.msgh_descriptor_count  = 1;
    msg.task_port.name              = mach_task_self();
    msg.task_port.disposition       = MACH_MSG_TYPE_COPY_SEND;
    msg.task_port.type              = MACH_MSG_PORT_DESCRIPTOR;

    kret = mach_msg_send(&msg.header);
    if (kret != KERN_SUCCESS)
        server_protocol_error( "mach_msg_send failed: 0x%08x\n", kret );

    mach_port_deallocate(mach_task_self(), wineserver_port);
}
#endif  /* __APPLE__ */


/***********************************************************************
 *           get_unix_tid
 *
 * Retrieve the Unix tid to use on the server side for the current thread.
 */
static int get_unix_tid(void)
{
    int ret = -1;
#ifdef HAVE_PTHREAD_GETTHREADID_NP
    ret = pthread_getthreadid_np();
#elif defined(linux)
    ret = syscall( __NR_gettid );
#elif defined(__sun)
    ret = pthread_self();
#elif defined(__APPLE__)
    ret = mach_thread_self();
    mach_port_deallocate(mach_task_self(), ret);
#elif defined(__NetBSD__)
    ret = _lwp_self();
#elif defined(__FreeBSD__)
    long lwpid;
    thr_self( &lwpid );
    ret = lwpid;
#elif defined(__DragonFly__)
    ret = lwp_gettid();
#endif
    return ret;
}


/***********************************************************************
 *           init_thread_pipe
 *
 * Create the server->client communication pipe.
 */
static int init_thread_pipe(void)
{
    int reply_pipe[2];
    stack_t ss;

    ss.ss_sp    = get_signal_stack();
    ss.ss_size  = signal_stack_size;
    ss.ss_flags = 0;
    sigaltstack( &ss, NULL );

    if (server_pipe( reply_pipe ) == -1) server_protocol_perror( "pipe" );
    if (server_pipe( ntdll_get_thread_data()->wait_fd ) == -1) server_protocol_perror( "pipe" );
    wine_server_send_fd( reply_pipe[1] );
    wine_server_send_fd( ntdll_get_thread_data()->wait_fd[1] );
    ntdll_get_thread_data()->reply_fd = reply_pipe[0];
    return reply_pipe[1];
}


/***********************************************************************
 *           process_exit_wrapper
 *
 * Close server socket and exit process normally.
 */
void process_exit_wrapper( int status )
{
    close( fd_socket );
    exit( status );
}


/***********************************************************************
 *           server_init_process
 *
 * Start the server and create the initial socket pair.
 */
size_t server_init_process(void)
{
    const char *arch = getenv( "WINEARCH" );
    const char *env_socket = getenv( "WINESERVERSOCKET" );
    obj_handle_t version;
    unsigned int i;
    int ret, reply_pipe;
    struct sigaction sig_act;
    size_t info_size;
    DWORD pid, tid;

    server_pid = -1;
    if (env_socket)
    {
        fd_socket = atoi( env_socket );
        if (fcntl( fd_socket, F_SETFD, FD_CLOEXEC ) == -1)
            fatal_perror( "Bad server socket %d", fd_socket );
        unsetenv( "WINESERVERSOCKET" );
    }
    else
    {
        const char *arch = getenv( "WINEARCH" );

        if (arch && strcmp( arch, "win32" ) && strcmp( arch, "win64" ))
            fatal_error( "WINEARCH set to invalid value '%s', it must be either win32 or win64.\n", arch );

        fd_socket = server_connect();
    }

    /* setup the signal mask */
    sigemptyset( &server_block_set );
    sigaddset( &server_block_set, SIGALRM );
    sigaddset( &server_block_set, SIGIO );
    sigaddset( &server_block_set, SIGINT );
    sigaddset( &server_block_set, SIGHUP );
    sigaddset( &server_block_set, SIGUSR1 );
    sigaddset( &server_block_set, SIGUSR2 );
    sigaddset( &server_block_set, SIGCHLD );
    pthread_sigmask( SIG_BLOCK, &server_block_set, NULL );

    /* receive the first thread request fd on the main socket */
    ntdll_get_thread_data()->request_fd = receive_fd( &version );

#ifdef SO_PASSCRED
    /* now that we hopefully received the server_pid, disable SO_PASSCRED */
    {
        int enable = 0;
        setsockopt( fd_socket, SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable) );
    }
#endif

    if (version != SERVER_PROTOCOL_VERSION)
        server_protocol_error( "version mismatch %d/%d.\n"
                               "Your %s binary was not upgraded correctly,\n"
                               "or you have an older one somewhere in your PATH.\n"
                               "Or maybe the wrong wineserver is still running?\n",
                               version, SERVER_PROTOCOL_VERSION,
                               (version > SERVER_PROTOCOL_VERSION) ? "wine" : "wineserver" );
#if defined(__linux__) && defined(HAVE_PRCTL)
    /* work around Ubuntu's ptrace breakage */
    if (server_pid != -1) prctl( 0x59616d61 /* PR_SET_PTRACER */, server_pid );
#endif

    /* ignore SIGPIPE so that we get an EPIPE error instead  */
    sig_act.sa_handler = SIG_IGN;
    sig_act.sa_flags   = 0;
    sigemptyset( &sig_act.sa_mask );
    sigaction( SIGPIPE, &sig_act, NULL );

    reply_pipe = init_thread_pipe();

    SERVER_START_REQ( init_first_thread )
    {
        req->unix_pid    = getpid();
        req->unix_tid    = get_unix_tid();
        req->reply_fd    = reply_pipe;
        req->wait_fd     = ntdll_get_thread_data()->wait_fd[1];
        req->debug_level = (TRACE_ON(server) != 0);
        wine_server_set_reply( req, supported_machines, sizeof(supported_machines) );
        ret = wine_server_call( req );
        pid               = reply->pid;
        tid               = reply->tid;
        peb->SessionId    = reply->session_id;
        info_size         = reply->info_size;
        server_start_time = reply->server_start;
        supported_machines_count = wine_server_reply_size( reply ) / sizeof(*supported_machines);
    }
    SERVER_END_REQ;
    close( reply_pipe );

    if (ret) server_protocol_error( "init_first_thread failed with status %x\n", ret );

    if (!supported_machines_count)
        fatal_error( "'%s' is a 64-bit installation, it cannot be used with a 32-bit wineserver.\n",
                     config_dir );

    native_machine = supported_machines[0];
    if (is_machine_64bit( native_machine ))
    {
        if (arch && !strcmp( arch, "win32" ))
            fatal_error( "WINEARCH set to win32 but '%s' is a 64-bit installation.\n", config_dir );
#ifndef _WIN64
        is_wow64 = TRUE;
        NtCurrentTeb()->GdiBatchCount = PtrToUlong( (char *)NtCurrentTeb() - teb_offset );
        NtCurrentTeb()->WowTebOffset  = -teb_offset;
#endif
    }
    else
    {
        if (is_win64)
            fatal_error( "'%s' is a 32-bit installation, it cannot support 64-bit applications.\n", config_dir );
        if (arch && !strcmp( arch, "win64" ))
            fatal_error( "WINEARCH set to win64 but '%s' is a 32-bit installation.\n", config_dir );
    }

    set_thread_id( NtCurrentTeb(), pid, tid );

    for (i = 0; i < supported_machines_count; i++)
        if (supported_machines[i] == current_machine) return info_size;

    fatal_error( "wineserver doesn't support the %04x architecture\n", current_machine );
}


/***********************************************************************
 *           server_init_process_done
 */
void server_init_process_done(void)
{
    void *entry, *teb;
    NTSTATUS status;
    int suspend, needs_close, unixdir;

    if (peb->ProcessParameters->CurrentDirectory.Handle &&
        !server_get_unix_fd( peb->ProcessParameters->CurrentDirectory.Handle,
                             FILE_TRAVERSE, &unixdir, &needs_close, NULL, NULL ))
    {
        fchdir( unixdir );
        if (needs_close) close( unixdir );
    }
    else chdir( "/" ); /* avoid locking removable devices */

#ifdef __APPLE__
    send_server_task_port();
#endif
    if (main_image_info.ImageCharacteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE)
        virtual_set_large_address_space();

    /* Install signal handlers; this cannot be done earlier, since we cannot
     * send exceptions to the debugger before the create process event that
     * is sent by init_process_done */
    signal_init_process();

    /* always send the native TEB */
    if (!(teb = NtCurrentTeb64())) teb = NtCurrentTeb();

    /* Signal the parent process to continue */
    SERVER_START_REQ( init_process_done )
    {
        req->teb      = wine_server_client_ptr( teb );
        req->peb      = NtCurrentTeb64() ? NtCurrentTeb64()->Peb : wine_server_client_ptr( peb );
#ifdef __i386__
        req->ldt_copy = wine_server_client_ptr( &__wine_ldt_copy );
#endif
        status = wine_server_call( req );
        suspend = reply->suspend;
        entry = wine_server_get_ptr( reply->entry );
    }
    SERVER_END_REQ;

    assert( !status );
    signal_start_thread( entry, peb, suspend, NtCurrentTeb() );
}


/***********************************************************************
 *           server_init_thread
 *
 * Send an init thread request.
 */
void server_init_thread( void *entry_point, BOOL *suspend )
{
    void *teb;
    int reply_pipe = init_thread_pipe();

    /* always send the native TEB */
    if (!(teb = NtCurrentTeb64())) teb = NtCurrentTeb();

    SERVER_START_REQ( init_thread )
    {
        req->unix_tid  = get_unix_tid();
        req->teb       = wine_server_client_ptr( teb );
        req->entry     = wine_server_client_ptr( entry_point );
        req->reply_fd  = reply_pipe;
        req->wait_fd   = ntdll_get_thread_data()->wait_fd[1];
        wine_server_call( req );
        *suspend = reply->suspend;
    }
    SERVER_END_REQ;
    close( reply_pipe );
}


/***********************************************************************
 *           DbgUiIssueRemoteBreakin
 */
NTSTATUS WINAPI DbgUiIssueRemoteBreakin( HANDLE process )
{
    apc_call_t call;
    apc_result_t result;
    NTSTATUS status;

    memset( &call, 0, sizeof(call) );
    call.type = APC_BREAK_PROCESS;
    status = server_queue_process_apc( process, &call, &result );
    if (status) return status;
    return result.break_process.status;
}


/******************************************************************************
 *           NtDuplicateObject
 */
NTSTATUS WINAPI NtDuplicateObject( HANDLE source_process, HANDLE source, HANDLE dest_process, HANDLE *dest,
                                   ACCESS_MASK access, ULONG attributes, ULONG options )
{
    sigset_t sigset;
    NTSTATUS ret;
    int fd = -1;

    if (dest) *dest = 0;

    if ((options & DUPLICATE_CLOSE_SOURCE) && source_process != NtCurrentProcess())
    {
        apc_call_t call;
        apc_result_t result;

        memset( &call, 0, sizeof(call) );

        call.dup_handle.type        = APC_DUP_HANDLE;
        call.dup_handle.src_handle  = wine_server_obj_handle( source );
        call.dup_handle.dst_process = wine_server_obj_handle( dest_process );
        call.dup_handle.access      = access;
        call.dup_handle.attributes  = attributes;
        call.dup_handle.options     = options;
        ret = server_queue_process_apc( source_process, &call, &result );
        if (ret != STATUS_SUCCESS) return ret;

        if (!result.dup_handle.status)
            *dest = wine_server_ptr_handle( result.dup_handle.handle );
        return result.dup_handle.status;
    }

    server_enter_uninterrupted_section( &fd_cache_mutex, &sigset );

    /* always remove the cached fd; if the server request fails we'll just
     * retrieve it again */
    if (options & DUPLICATE_CLOSE_SOURCE)
        fd = remove_fd_from_cache( source );

    SERVER_START_REQ( dup_handle )
    {
        req->src_process = wine_server_obj_handle( source_process );
        req->src_handle  = wine_server_obj_handle( source );
        req->dst_process = wine_server_obj_handle( dest_process );
        req->access      = access;
        req->attributes  = attributes;
        req->options     = options;
        if (!(ret = wine_server_call( req )))
        {
            if (dest) *dest = wine_server_ptr_handle( reply->handle );
        }
    }
    SERVER_END_REQ;

    server_leave_uninterrupted_section( &fd_cache_mutex, &sigset );

    if (fd != -1) close( fd );
    return ret;
}


/**************************************************************************
 *           NtClose
 */
NTSTATUS WINAPI NtClose( HANDLE handle )
{
    sigset_t sigset;
    HANDLE port;
    NTSTATUS ret;
    int fd;

    server_enter_uninterrupted_section( &fd_cache_mutex, &sigset );

    /* always remove the cached fd; if the server request fails we'll just
     * retrieve it again */
    fd = remove_fd_from_cache( handle );

    SERVER_START_REQ( close_handle )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;

    server_leave_uninterrupted_section( &fd_cache_mutex, &sigset );

    if (fd != -1) close( fd );

    if (ret != STATUS_INVALID_HANDLE || !handle) return ret;
    if (!peb->BeingDebugged) return ret;
    if (!NtQueryInformationProcess( NtCurrentProcess(), ProcessDebugPort, &port, sizeof(port), NULL) && port)
    {
        NtCurrentTeb()->ExceptionCode = ret;
        call_raise_user_exception_dispatcher();
    }
    return ret;
}
