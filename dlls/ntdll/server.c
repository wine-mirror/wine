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
    unix_funcs->start_process( entry, suspend, kernel32_start_process );
}
