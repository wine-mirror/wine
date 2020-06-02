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
void CDECL init_threading( int *nb_threads_ptr, struct ldt_copy **ldt_copy )
{
#ifdef __i386__
    extern struct ldt_copy __wine_ldt_copy;
    *ldt_copy = &__wine_ldt_copy;
#endif
    nb_threads = nb_threads_ptr;
    signal_init_threading();
}


/***********************************************************************
 *           alloc_thread
 */
NTSTATUS CDECL alloc_thread( TEB *teb )
{
    return signal_alloc_thread( teb );
}


/***********************************************************************
 *           free_thread
 */
void CDECL free_thread( TEB *teb )
{
    signal_free_thread( teb );
}


/***********************************************************************
 *           init_thread
 */
void CDECL init_thread( TEB *teb )
{
    signal_init_thread( teb );
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
    pthread_sigmask( SIG_BLOCK, &server_block_set, NULL );
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
