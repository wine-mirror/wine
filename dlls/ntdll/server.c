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
#ifdef HAVE_SYS_UCONTEXT_H
# include <sys/ucontext.h>
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
#include "wine/library.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "ntdll_misc.h"
#include "ddk/wdm.h"

/* Some versions of glibc don't define this */
#ifndef SCM_RIGHTS
#define SCM_RIGHTS 1
#endif

#ifndef MSG_CMSG_CLOEXEC
#define MSG_CMSG_CLOEXEC 0
#endif

#define SOCKETNAME "socket"        /* name of the socket file */
#define LOCKNAME   "lock"          /* name of the lock file */

const char *build_dir = NULL;
const char *data_dir = NULL;
const char *config_dir = NULL;

unsigned int server_cpus = 0;
BOOL is_wow64 = FALSE;

timeout_t server_start_time = 0;  /* time of server startup */

sigset_t server_block_set;  /* signals to block during server calls */

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
    for (;;) unix_funcs->abort_thread(1);
}


/***********************************************************************
 *           server_protocol_perror
 */
static DECLSPEC_NORETURN void server_protocol_perror( const char *err )
{
    fprintf( stderr, "wine client error:%x: ", GetCurrentThreadId() );
    perror( err );
    for (;;) unix_funcs->abort_thread(1);
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
unsigned int CDECL wine_server_call( void *req_ptr )
{
    return unix_funcs->server_call( req_ptr );
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
            if (!reply.cookie) unix_funcs->abort_thread( reply.signaled );  /* thread got killed */
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


static void invoke_apc( const user_apc_t *apc )
{
    switch( apc->type )
    {
    case APC_USER:
    {
        void (WINAPI *func)(ULONG_PTR,ULONG_PTR,ULONG_PTR) = wine_server_get_ptr( apc->user.func );
        func( apc->user.args[0], apc->user.args[1], apc->user.args[2] );
        break;
    }
    case APC_TIMER:
    {
        void (WINAPI *func)(void*, unsigned int, unsigned int) = wine_server_get_ptr( apc->user.func );
        func( wine_server_get_ptr( apc->user.args[1] ),
              (DWORD)apc->timer.time, (DWORD)(apc->timer.time >> 32) );
        break;
    }
    default:
        server_protocol_error( "get_apc_request: bad type %d\n", apc->type );
        break;
    }
}

/***********************************************************************
 *              invoke_apc
 *
 * Invoke a single APC.
 *
 */
static void invoke_system_apc( const apc_call_t *call, apc_result_t *result )
{
    SIZE_T size;
    void *addr;
    pe_image_info_t image_info;

    memset( result, 0, sizeof(*result) );

    switch (call->type)
    {
    case APC_NONE:
        break;
    case APC_ASYNC_IO:
    {
        IO_STATUS_BLOCK *iosb = wine_server_get_ptr( call->async_io.sb );
        NTSTATUS (**user)(void *, IO_STATUS_BLOCK *, NTSTATUS) = wine_server_get_ptr( call->async_io.user );
        result->type = call->type;
        result->async_io.status = (*user)( user, iosb, call->async_io.status );
        if (result->async_io.status != STATUS_PENDING)
            result->async_io.total = iosb->Information;
        break;
    }
    case APC_VIRTUAL_ALLOC:
        result->type = call->type;
        addr = wine_server_get_ptr( call->virtual_alloc.addr );
        size = call->virtual_alloc.size;
        if ((ULONG_PTR)addr == call->virtual_alloc.addr && size == call->virtual_alloc.size)
        {
            result->virtual_alloc.status = virtual_alloc( &addr, call->virtual_alloc.zero_bits_64, &size,
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
            result->map_view.status = virtual_map_section( wine_server_ptr_handle(call->map_view.handle),
                                                           &addr,
                                                           call->map_view.zero_bits_64, 0,
                                                           &offset, &size,
                                                           call->map_view.alloc_type, call->map_view.prot,
                                                           &image_info );
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
    case APC_BREAK_PROCESS:
        result->type = APC_BREAK_PROCESS;
        result->break_process.status = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE, NULL, 0, 0,
                                                            DbgUiRemoteBreakin, NULL, NULL, NULL );
        break;
    default:
        server_protocol_error( "get_apc_request: bad type %d\n", call->type );
        break;
    }
}


/***********************************************************************
 *              server_select
 */
unsigned int server_select( const select_op_t *select_op, data_size_t size, UINT flags,
                            timeout_t abs_timeout, CONTEXT *context, RTL_CRITICAL_SECTION *cs, user_apc_t *user_apc )
{
    unsigned int ret;
    int cookie;
    obj_handle_t apc_handle = 0;
    context_t server_context;
    BOOL suspend_context = FALSE;
    apc_call_t call;
    apc_result_t result;
    sigset_t old_set;

    memset( &result, 0, sizeof(result) );
    if (context)
    {
        suspend_context = TRUE;
        context_to_server( &server_context, context );
    }

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
                    wine_server_add_data( req, &server_context, sizeof(server_context) );
                    suspend_context = FALSE; /* server owns the context now */
                }
                if (context) wine_server_set_reply( req, &server_context, sizeof(server_context) );
                ret = unix_funcs->server_call_unlocked( req );
                apc_handle  = reply->apc_handle;
                call        = reply->call;
                if (wine_server_reply_size( reply ))
                {
                    DWORD context_flags = context->ContextFlags; /* unchanged registers are still available */
                    context_from_server( context, &server_context );
                    context->ContextFlags |= context_flags;
                }
            }
            SERVER_END_REQ;

            if (ret != STATUS_KERNEL_APC) break;
            invoke_system_apc( &call, &result );

            /* don't signal multiple times */
            if (size >= sizeof(select_op->signal_and_wait) && select_op->op == SELECT_SIGNAL_AND_WAIT)
                size = offsetof( select_op_t, signal_and_wait.signal );
        }
        pthread_sigmask( SIG_SETMASK, &old_set, NULL );
        if (cs)
        {
            RtlLeaveCriticalSection( cs );
            cs = NULL;
        }
        if (ret != STATUS_PENDING) break;

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
    BOOL user_apc = FALSE;
    unsigned int ret;
    user_apc_t apc;

    if (abs_timeout < 0)
    {
        LARGE_INTEGER now;

        RtlQueryPerformanceCounter(&now);
        abs_timeout -= now.QuadPart;
    }

    for (;;)
    {
        ret = server_select( select_op, size, flags, abs_timeout, NULL, NULL, &apc );
        if (ret != STATUS_USER_APC) break;
        invoke_apc( &apc );

        /* if we ran a user apc we have to check once more if additional apcs are queued,
         * but we don't want to wait */
        abs_timeout = 0;
        user_apc = TRUE;
        size = 0;
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
            invoke_system_apc( call, result );
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
    unix_funcs->server_send_fd( fd );
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
    return unix_funcs->server_fd_to_handle( fd, access, attributes, handle );
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
    return unix_funcs->server_handle_to_fd( handle, access, unix_fd, options );
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
    unix_funcs->server_release_fd( handle, unix_fd );
}


/***********************************************************************
 *           server_init_process
 *
 * Start the server and create the initial socket pair.
 */
void server_init_process(void)
{
    /* setup the signal mask */
    sigemptyset( &server_block_set );
    sigaddset( &server_block_set, SIGALRM );
    sigaddset( &server_block_set, SIGIO );
    sigaddset( &server_block_set, SIGINT );
    sigaddset( &server_block_set, SIGHUP );
    sigaddset( &server_block_set, SIGUSR1 );
    sigaddset( &server_block_set, SIGUSR2 );
    sigaddset( &server_block_set, SIGCHLD );
    unix_funcs->server_init_process();
}


/***********************************************************************
 *           server_init_process_done
 */
void server_init_process_done(void)
{
#ifdef __i386__
    extern struct ldt_copy *__wine_ldt_copy;
#endif
    PEB *peb = NtCurrentTeb()->Peb;
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( peb->ImageBaseAddress );
    void *entry = (char *)peb->ImageBaseAddress + nt->OptionalHeader.AddressOfEntryPoint;
    NTSTATUS status;
    int suspend;

    unix_funcs->server_init_process_done();

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
        req->ldt_copy = wine_server_client_ptr( __wine_ldt_copy );
#endif
        req->entry    = wine_server_client_ptr( entry );
        req->gui      = (nt->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_WINDOWS_CUI);
        status = wine_server_call( req );
        suspend = reply->suspend;
    }
    SERVER_END_REQ;

    assert( !status );
    signal_start_process( entry, suspend );
}
