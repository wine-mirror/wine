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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_LWP_H
#include <lwp.h>
#endif
#ifdef HAVE_PTHREAD_NP_H
# include <pthread_np.h>
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
#ifdef HAVE_SYS_UCONTEXT_H
# include <sys/ucontext.h>
#endif
#ifdef HAVE_SYS_THR_H
#include <sys/thr.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/library.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(server);

/* Some versions of glibc don't define this */
#ifndef SCM_RIGHTS
#define SCM_RIGHTS 1
#endif

#ifndef MSG_CMSG_CLOEXEC
#define MSG_CMSG_CLOEXEC 0
#endif

#define SOCKETNAME "socket"        /* name of the socket file */
#define LOCKNAME   "lock"          /* name of the lock file */

#ifdef __i386__
static const enum cpu_type client_cpu = CPU_x86;
#elif defined(__x86_64__)
static const enum cpu_type client_cpu = CPU_x86_64;
#elif defined(__powerpc__)
static const enum cpu_type client_cpu = CPU_POWERPC;
#elif defined(__arm__)
static const enum cpu_type client_cpu = CPU_ARM;
#elif defined(__aarch64__)
static const enum cpu_type client_cpu = CPU_ARM64;
#else
#error Unsupported CPU
#endif

unsigned int server_cpus = 0;
BOOL is_wow64 = FALSE;

timeout_t server_start_time = 0;  /* time of server startup */

sigset_t server_block_set;  /* signals to block during server calls */
static int fd_socket = -1;  /* socket to exchange file descriptors with the server */
static pid_t server_pid;

static RTL_CRITICAL_SECTION fd_cache_section;
static RTL_CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &fd_cache_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": fd_cache_section") }
};
static RTL_CRITICAL_SECTION fd_cache_section = { &critsect_debug, -1, 0, 0, 0, 0 };


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
 *           wine_server_call (NTDLL.@)
 *
 * Perform a server call.
 *
 * PARAMS
 *     req_ptr [I/O] Function dependent data
 *
 * RETURNS
 *     Depends on server function being called, but usually an NTSTATUS code.
 *
 * NOTES
 *     Use the SERVER_START_REQ and SERVER_END_REQ to help you fill out the
 *     server request structure for the particular call. E.g:
 *|     SERVER_START_REQ( event_op )
 *|     {
 *|         req->handle = handle;
 *|         req->op     = SET_EVENT;
 *|         ret = wine_server_call( req );
 *|     }
 *|     SERVER_END_REQ;
 */
unsigned int wine_server_call( void *req_ptr )
{
    struct __server_request_info * const req = req_ptr;
    sigset_t old_set;
    unsigned int ret;

    pthread_sigmask( SIG_BLOCK, &server_block_set, &old_set );
    ret = send_request( req );
    if (!ret) ret = wait_reply( req );
    pthread_sigmask( SIG_SETMASK, &old_set, NULL );
    return ret;
}


/***********************************************************************
 *           server_enter_uninterrupted_section
 */
void server_enter_uninterrupted_section( RTL_CRITICAL_SECTION *cs, sigset_t *sigset )
{
    pthread_sigmask( SIG_BLOCK, &server_block_set, sigset );
    RtlEnterCriticalSection( cs );
}


/***********************************************************************
 *           server_leave_uninterrupted_section
 */
void server_leave_uninterrupted_section( RTL_CRITICAL_SECTION *cs, sigset_t *sigset )
{
    RtlLeaveCriticalSection( cs );
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
 *              invoke_apc
 *
 * Invoke a single APC. Return TRUE if a user APC has been run.
 */
static BOOL invoke_apc( const apc_call_t *call, apc_result_t *result )
{
    BOOL user_apc = FALSE;
    SIZE_T size;
    void *addr;

    memset( result, 0, sizeof(*result) );

    switch (call->type)
    {
    case APC_NONE:
        break;
    case APC_USER:
    {
        void (WINAPI *func)(ULONG_PTR,ULONG_PTR,ULONG_PTR) = wine_server_get_ptr( call->user.func );
        func( call->user.args[0], call->user.args[1], call->user.args[2] );
        user_apc = TRUE;
        break;
    }
    case APC_TIMER:
    {
        void (WINAPI *func)(void*, unsigned int, unsigned int) = wine_server_get_ptr( call->timer.func );
        func( wine_server_get_ptr( call->timer.arg ),
              (DWORD)call->timer.time, (DWORD)(call->timer.time >> 32) );
        user_apc = TRUE;
        break;
    }
    case APC_ASYNC_IO:
    {
        void *apc = NULL;
        IO_STATUS_BLOCK *iosb = wine_server_get_ptr( call->async_io.sb );
        NTSTATUS (*func)(void *, IO_STATUS_BLOCK *, NTSTATUS, void **) = wine_server_get_ptr( call->async_io.func );
        result->type = call->type;
        result->async_io.status = func( wine_server_get_ptr( call->async_io.user ),
                                        iosb, call->async_io.status, &apc );
        if (result->async_io.status != STATUS_PENDING)
        {
            result->async_io.total = iosb->Information;
            result->async_io.apc   = wine_server_client_ptr( apc );
        }
        break;
    }
    case APC_VIRTUAL_ALLOC:
        result->type = call->type;
        addr = wine_server_get_ptr( call->virtual_alloc.addr );
        size = call->virtual_alloc.size;
        if ((ULONG_PTR)addr == call->virtual_alloc.addr && size == call->virtual_alloc.size)
        {
            result->virtual_alloc.status = NtAllocateVirtualMemory( NtCurrentProcess(), &addr,
                                                                    call->virtual_alloc.zero_bits, &size,
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
        if ((ULONG_PTR)addr == call->map_view.addr && size == call->map_view.size)
        {
            LARGE_INTEGER offset;
            offset.QuadPart = call->map_view.offset;
            result->map_view.status = NtMapViewOfSection( wine_server_ptr_handle(call->map_view.handle),
                                                          NtCurrentProcess(), &addr,
                                                          call->map_view.zero_bits, 0,
                                                          &offset, &size, ViewShare,
                                                          call->map_view.alloc_type, call->map_view.prot );
            result->map_view.addr = wine_server_client_ptr( addr );
            result->map_view.size = size;
        }
        else result->map_view.status = STATUS_INVALID_PARAMETER;
        NtClose( wine_server_ptr_handle(call->map_view.handle) );
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
        CLIENT_ID id;
        HANDLE handle;
        SIZE_T reserve = call->create_thread.reserve;
        SIZE_T commit = call->create_thread.commit;
        void *func = wine_server_get_ptr( call->create_thread.func );
        void *arg  = wine_server_get_ptr( call->create_thread.arg );

        result->type = call->type;
        if (reserve == call->create_thread.reserve && commit == call->create_thread.commit &&
            (ULONG_PTR)func == call->create_thread.func && (ULONG_PTR)arg == call->create_thread.arg)
        {
            result->create_thread.status = RtlCreateUserThread( NtCurrentProcess(), NULL,
                                                                call->create_thread.suspend, NULL,
                                                                reserve, commit, func, arg, &handle, &id );
            result->create_thread.handle = wine_server_obj_handle( handle );
            result->create_thread.tid = HandleToULong(id.UniqueThread);
        }
        else result->create_thread.status = STATUS_INVALID_PARAMETER;
        break;
    }
    default:
        server_protocol_error( "get_apc_request: bad type %d\n", call->type );
        break;
    }
    return user_apc;
}


/***********************************************************************
 *              server_select
 */
unsigned int server_select( const select_op_t *select_op, data_size_t size, UINT flags,
                            const LARGE_INTEGER *timeout )
{
    unsigned int ret;
    int cookie;
    BOOL user_apc = FALSE;
    obj_handle_t apc_handle = 0;
    apc_call_t call;
    apc_result_t result;
    timeout_t abs_timeout = timeout ? timeout->QuadPart : TIMEOUT_INFINITE;

    memset( &result, 0, sizeof(result) );

    for (;;)
    {
        SERVER_START_REQ( select )
        {
            req->flags    = flags;
            req->cookie   = wine_server_client_ptr( &cookie );
            req->prev_apc = apc_handle;
            req->timeout  = abs_timeout;
            wine_server_add_data( req, &result, sizeof(result) );
            wine_server_add_data( req, select_op, size );
            ret = wine_server_call( req );
            abs_timeout = reply->timeout;
            apc_handle  = reply->apc_handle;
            call        = reply->call;
        }
        SERVER_END_REQ;
        if (ret == STATUS_PENDING) ret = wait_select_reply( &cookie );
        if (ret != STATUS_USER_APC) break;
        if (invoke_apc( &call, &result ))
        {
            /* if we ran a user apc we have to check once more if an object got signaled,
             * but we don't want to wait */
            abs_timeout = 0;
            user_apc = TRUE;
        }

        /* don't signal multiple times */
        if (size >= sizeof(select_op->signal_and_wait) && select_op->op == SELECT_SIGNAL_AND_WAIT)
            size = offsetof( select_op_t, signal_and_wait.signal );
    }

    if (ret == STATUS_TIMEOUT && user_apc) ret = STATUS_USER_APC;

    /* A test on Windows 2000 shows that Windows always yields during
       a wait, but a wait that is hit by an event gets a priority
       boost as well.  This seems to model that behavior the closest.  */
    if (ret == STATUS_TIMEOUT) NtYieldExecution();

    return ret;
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
            invoke_apc( call, result );
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
            if (ret) NtClose( handle );
        }
        return ret;
    }
}


/***********************************************************************
 *           wine_server_send_fd   (NTDLL.@)
 *
 * Send a file descriptor to the server.
 *
 * PARAMS
 *     fd [I] file descriptor to send
 *
 * RETURNS
 *     nothing
 */
void CDECL wine_server_send_fd( int fd )
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

struct fd_cache_entry
{
    int fd;
    enum server_fd_type type : 5;
    unsigned int        access : 3;
    unsigned int        options : 24;
};

#define FD_CACHE_BLOCK_SIZE  (65536 / sizeof(struct fd_cache_entry))
#define FD_CACHE_ENTRIES     128

static struct fd_cache_entry *fd_cache[FD_CACHE_ENTRIES];
static struct fd_cache_entry fd_cache_initial_block[FD_CACHE_BLOCK_SIZE];

static inline unsigned int handle_to_index( HANDLE handle, unsigned int *entry )
{
    unsigned int idx = (wine_server_obj_handle(handle) >> 2) - 1;
    *entry = idx / FD_CACHE_BLOCK_SIZE;
    return idx % FD_CACHE_BLOCK_SIZE;
}


/***********************************************************************
 *           add_fd_to_cache
 *
 * Caller must hold fd_cache_section.
 */
static BOOL add_fd_to_cache( HANDLE handle, int fd, enum server_fd_type type,
                            unsigned int access, unsigned int options )
{
    unsigned int entry, idx = handle_to_index( handle, &entry );
    int prev_fd;

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
            void *ptr = wine_anon_mmap( NULL, FD_CACHE_BLOCK_SIZE * sizeof(struct fd_cache_entry),
                                        PROT_READ | PROT_WRITE, 0 );
            if (ptr == MAP_FAILED) return FALSE;
            fd_cache[entry] = ptr;
        }
    }
    /* store fd+1 so that 0 can be used as the unset value */
    prev_fd = interlocked_xchg( &fd_cache[entry][idx].fd, fd + 1 ) - 1;
    fd_cache[entry][idx].type = type;
    fd_cache[entry][idx].access = access;
    fd_cache[entry][idx].options = options;
    if (prev_fd != -1) close( prev_fd );
    return TRUE;
}


/***********************************************************************
 *           get_cached_fd
 *
 * Caller must hold fd_cache_section.
 */
static inline int get_cached_fd( HANDLE handle, enum server_fd_type *type,
                                 unsigned int *access, unsigned int *options )
{
    unsigned int entry, idx = handle_to_index( handle, &entry );
    int fd = -1;

    if (entry < FD_CACHE_ENTRIES && fd_cache[entry])
    {
        fd = fd_cache[entry][idx].fd - 1;
        if (type) *type = fd_cache[entry][idx].type;
        if (access) *access = fd_cache[entry][idx].access;
        if (options) *options = fd_cache[entry][idx].options;
    }
    return fd;
}


/***********************************************************************
 *           server_remove_fd_from_cache
 */
int server_remove_fd_from_cache( HANDLE handle )
{
    unsigned int entry, idx = handle_to_index( handle, &entry );
    int fd = -1;

    if (entry < FD_CACHE_ENTRIES && fd_cache[entry])
        fd = interlocked_xchg( &fd_cache[entry][idx].fd, 0 ) - 1;

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
    int ret = 0, fd;
    unsigned int access = 0;

    *unix_fd = -1;
    *needs_close = 0;
    wanted_access &= FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA;

    server_enter_uninterrupted_section( &fd_cache_section, &sigset );

    fd = get_cached_fd( handle, type, &access, options );
    if (fd != -1) goto done;

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
    }
    SERVER_END_REQ;

done:
    server_leave_uninterrupted_section( &fd_cache_section, &sigset );
    if (!ret && ((access & wanted_access) != wanted_access))
    {
        ret = STATUS_ACCESS_DENIED;
        if (*needs_close) close( fd );
    }
    if (!ret) *unix_fd = fd;
    return ret;
}


/***********************************************************************
 *           wine_server_fd_to_handle   (NTDLL.@)
 *
 * Allocate a file handle for a Unix file descriptor.
 *
 * PARAMS
 *     fd      [I] Unix file descriptor.
 *     access  [I] Win32 access flags.
 *     attributes [I] Object attributes.
 *     handle  [O] Address where Wine file handle will be stored.
 *
 * RETURNS
 *     NTSTATUS code
 */
int CDECL wine_server_fd_to_handle( int fd, unsigned int access, unsigned int attributes, HANDLE *handle )
{
    int ret;

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
 *           wine_server_handle_to_fd   (NTDLL.@)
 *
 * Retrieve the file descriptor corresponding to a file handle.
 *
 * PARAMS
 *     handle  [I] Wine file handle.
 *     access  [I] Win32 file access rights requested.
 *     unix_fd [O] Address where Unix file descriptor will be stored.
 *     options [O] Address where the file open options will be stored. Optional.
 *
 * RETURNS
 *     NTSTATUS code
 */
int CDECL wine_server_handle_to_fd( HANDLE handle, unsigned int access, int *unix_fd,
                              unsigned int *options )
{
    int needs_close, ret = server_get_unix_fd( handle, access, unix_fd, &needs_close, NULL, options );

    if (!ret && !needs_close)
    {
        if ((*unix_fd = dup(*unix_fd)) == -1) ret = FILE_GetNtStatus();
    }
    return ret;
}


/***********************************************************************
 *           wine_server_release_fd   (NTDLL.@)
 *
 * Release the Unix file descriptor returned by wine_server_handle_to_fd.
 *
 * PARAMS
 *     handle  [I] Wine file handle.
 *     unix_fd [I] Unix file descriptor to release.
 *
 * RETURNS
 *     nothing
 */
void CDECL wine_server_release_fd( HANDLE handle, int unix_fd )
{
    close( unix_fd );
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
 *           start_server
 *
 * Start a new wine server.
 */
static void start_server(void)
{
    static BOOL started;  /* we only try once */
    char *argv[3];
    static char wineserver[] = "server/wineserver";
    static char debug[] = "-d";

    if (!started)
    {
        int status;
        int pid = fork();
        if (pid == -1) fatal_perror( "fork" );
        if (!pid)
        {
            argv[0] = wineserver;
            argv[1] = TRACE_ON(server) ? debug : NULL;
            argv[2] = NULL;
            wine_exec_wine_binary( argv[0], argv, getenv("WINESERVER") );
            fatal_error( "could not exec wineserver\n" );
        }
        waitpid( pid, &status, 0 );
        status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        if (status == 2) return;  /* server lock held by someone else, will retry later */
        if (status) exit(status);  /* server failed */
        started = TRUE;
    }
}


/***********************************************************************
 *           setup_config_dir
 *
 * Setup the wine configuration dir.
 */
static void setup_config_dir(void)
{
    const char *p, *config_dir = wine_get_config_dir();

    if (chdir( config_dir ) == -1)
    {
        if (errno != ENOENT) fatal_perror( "chdir to %s\n", config_dir );

        if ((p = strrchr( config_dir, '/' )) && p != config_dir)
        {
            struct stat st;
            char *tmp_dir;

            if (!(tmp_dir = malloc( p + 1 - config_dir ))) fatal_error( "out of memory\n" );
            memcpy( tmp_dir, config_dir, p - config_dir );
            tmp_dir[p - config_dir] = 0;
            if (!stat( tmp_dir, &st ) && st.st_uid != getuid())
                fatal_error( "'%s' is not owned by you, refusing to create a configuration directory there\n",
                             tmp_dir );
            free( tmp_dir );
        }

        mkdir( config_dir, 0777 );
        if (chdir( config_dir ) == -1) fatal_perror( "chdir to %s\n", config_dir );

        if ((p = getenv( "WINEARCH" )) && !strcmp( p, "win32" ))
        {
            /* force creation of a 32-bit prefix */
            int fd = open( "system.reg", O_WRONLY | O_CREAT | O_EXCL, 0666 );
            if (fd != -1)
            {
                static const char regfile[] = "WINE REGISTRY Version 2\n\n#arch=win32\n";
                write( fd, regfile, sizeof(regfile) - 1 );
                close( fd );
            }
        }
        MESSAGE( "wine: created the configuration directory '%s'\n", config_dir );
    }

    if (mkdir( "dosdevices", 0777 ) == -1)
    {
        if (errno == EEXIST) return;
        fatal_perror( "cannot create %s/dosdevices\n", config_dir );
    }

    /* create the drive symlinks */

    mkdir( "drive_c", 0777 );
    symlink( "../drive_c", "dosdevices/c:" );
    symlink( "/", "dosdevices/z:" );
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
    const char *serverdir;
    struct sockaddr_un addr;
    struct stat st;
    int s, slen, retry, fd_cwd;

    /* retrieve the current directory */
    fd_cwd = open( ".", O_RDONLY );
    if (fd_cwd != -1) fcntl( fd_cwd, F_SETFD, 1 ); /* set close on exec flag */

    setup_config_dir();
    serverdir = wine_get_server_dir();

    /* chdir to the server directory */
    if (chdir( serverdir ) == -1)
    {
        if (errno != ENOENT) fatal_perror( "chdir to %s", serverdir );
        start_server();
        if (chdir( serverdir ) == -1) fatal_perror( "chdir to %s", serverdir );
    }

    /* make sure we are at the right place */
    if (stat( ".", &st ) == -1) fatal_perror( "stat %s", serverdir );
    if (st.st_uid != getuid()) fatal_error( "'%s' is not owned by you\n", serverdir );
    if (st.st_mode & 077) fatal_error( "'%s' must not be accessible by other users\n", serverdir );

    for (retry = 0; retry < 6; retry++)
    {
        /* if not the first try, wait a bit to leave the previous server time to exit */
        if (retry)
        {
            usleep( 100000 * retry * retry );
            start_server();
            if (lstat( SOCKETNAME, &st ) == -1) continue;  /* still no socket, wait a bit more */
        }
        else if (lstat( SOCKETNAME, &st ) == -1) /* check for an already existing socket */
        {
            if (errno != ENOENT) fatal_perror( "lstat %s/%s", serverdir, SOCKETNAME );
            start_server();
            if (lstat( SOCKETNAME, &st ) == -1) continue;  /* still no socket, wait a bit more */
        }

        /* make sure the socket is sane (ISFIFO needed for Solaris) */
        if (!S_ISSOCK(st.st_mode) && !S_ISFIFO(st.st_mode))
            fatal_error( "'%s/%s' is not a socket\n", serverdir, SOCKETNAME );
        if (st.st_uid != getuid())
            fatal_error( "'%s/%s' is not owned by you\n", serverdir, SOCKETNAME );

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
            fcntl( s, F_SETFD, 1 ); /* set close on exec flag */
            return s;
        }
        close( s );
    }
    server_connect_error( serverdir );
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

    kret = bootstrap_look_up(bootstrap_port, (char*)wine_get_server_dir(), &wineserver_port);
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
 *           server_init_process
 *
 * Start the server and create the initial socket pair.
 */
void server_init_process(void)
{
    obj_handle_t version;
    const char *env_socket = getenv( "WINESERVERSOCKET" );

    server_pid = -1;
    if (env_socket)
    {
        fd_socket = atoi( env_socket );
        if (fcntl( fd_socket, F_SETFD, 1 ) == -1)
            fatal_perror( "Bad server socket %d", fd_socket );
        unsetenv( "WINESERVERSOCKET" );
    }
    else fd_socket = server_connect();

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
#ifdef __APPLE__
    send_server_task_port();
#endif
#if defined(__linux__) && defined(HAVE_PRCTL)
    /* work around Ubuntu's ptrace breakage */
    if (server_pid != -1) prctl( 0x59616d61 /* PR_SET_PTRACER */, server_pid );
#endif
}


/***********************************************************************
 *           server_init_process_done
 */
NTSTATUS server_init_process_done(void)
{
    PEB *peb = NtCurrentTeb()->Peb;
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( peb->ImageBaseAddress );
    NTSTATUS status;

    /* Install signal handlers; this cannot be done earlier, since we cannot
     * send exceptions to the debugger before the create process event that
     * is sent by REQ_INIT_PROCESS_DONE.
     * We do need the handlers in place by the time the request is over, so
     * we set them up here. If we segfault between here and the server call
     * something is very wrong... */
    signal_init_process();

    /* Signal the parent process to continue */
    SERVER_START_REQ( init_process_done )
    {
        req->module   = wine_server_client_ptr( peb->ImageBaseAddress );
#ifdef __i386__
        req->ldt_copy = wine_server_client_ptr( &wine_ldt_copy );
#endif
        req->entry    = wine_server_client_ptr( (char *)peb->ImageBaseAddress + nt->OptionalHeader.AddressOfEntryPoint );
        req->gui      = (nt->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_WINDOWS_CUI);
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    return status;
}


/***********************************************************************
 *           server_init_thread
 *
 * Send an init thread request. Return 0 if OK.
 */
size_t server_init_thread( void *entry_point )
{
    static const char *cpu_names[] = { "x86", "x86_64", "PowerPC", "ARM", "ARM64" };
    static const BOOL is_win64 = (sizeof(void *) > sizeof(int));
    const char *arch = getenv( "WINEARCH" );
    int ret;
    int reply_pipe[2];
    struct sigaction sig_act;
    size_t info_size;

    sig_act.sa_handler = SIG_IGN;
    sig_act.sa_flags   = 0;
    sigemptyset( &sig_act.sa_mask );

    /* ignore SIGPIPE so that we get an EPIPE error instead  */
    sigaction( SIGPIPE, &sig_act, NULL );

    /* create the server->client communication pipes */
    if (server_pipe( reply_pipe ) == -1) server_protocol_perror( "pipe" );
    if (server_pipe( ntdll_get_thread_data()->wait_fd ) == -1) server_protocol_perror( "pipe" );
    wine_server_send_fd( reply_pipe[1] );
    wine_server_send_fd( ntdll_get_thread_data()->wait_fd[1] );
    ntdll_get_thread_data()->reply_fd = reply_pipe[0];
    close( reply_pipe[1] );

    SERVER_START_REQ( init_thread )
    {
        req->unix_pid    = getpid();
        req->unix_tid    = get_unix_tid();
        req->teb         = wine_server_client_ptr( NtCurrentTeb() );
        req->entry       = wine_server_client_ptr( entry_point );
        req->reply_fd    = reply_pipe[1];
        req->wait_fd     = ntdll_get_thread_data()->wait_fd[1];
        req->debug_level = (TRACE_ON(server) != 0);
        req->cpu         = client_cpu;
        ret = wine_server_call( req );
        NtCurrentTeb()->ClientId.UniqueProcess = ULongToHandle(reply->pid);
        NtCurrentTeb()->ClientId.UniqueThread  = ULongToHandle(reply->tid);
        info_size         = reply->info_size;
        server_start_time = reply->server_start;
        server_cpus       = reply->all_cpus;
    }
    SERVER_END_REQ;

    is_wow64 = !is_win64 && (server_cpus & (1 << CPU_x86_64)) != 0;
    ntdll_get_thread_data()->wow64_redir = is_wow64;

    switch (ret)
    {
    case STATUS_SUCCESS:
        if (arch)
        {
            if (!strcmp( arch, "win32" ) && (is_win64 || is_wow64))
                fatal_error( "WINEARCH set to win32 but '%s' is a 64-bit installation.\n",
                             wine_get_config_dir() );
            if (!strcmp( arch, "win64" ) && !is_win64 && !is_wow64)
                fatal_error( "WINEARCH set to win64 but '%s' is a 32-bit installation.\n",
                             wine_get_config_dir() );
        }
        return info_size;
    case STATUS_INVALID_IMAGE_WIN_64:
        fatal_error( "'%s' is a 32-bit installation, it cannot support 64-bit applications.\n",
                     wine_get_config_dir() );
    case STATUS_NOT_SUPPORTED:
        fatal_error( "'%s' is a 64-bit installation, it cannot be used with a 32-bit wineserver.\n",
                     wine_get_config_dir() );
    case STATUS_INVALID_IMAGE_FORMAT:
        fatal_error( "wineserver doesn't support the %s architecture\n", cpu_names[client_cpu] );
    default:
        server_protocol_error( "init_thread failed with status %x\n", ret );
    }
}
