/*
 * PowerPC64 signal handling routines
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
 *
 *
 * ############################################################################
 * ###  THIS FILE IS A LINK STUB, NOT A WORKING SIGNAL/UNWIND IMPLEMENTATION ###
 * ############################################################################
 *
 * It exists so that ntdll.so links and the remainder of the Wine tree can be
 * built and its own porting problems discovered.  Every entry point that needs
 * real machine-level work (syscall dispatcher, user-mode callback frames,
 * exception dispatch, signal handlers, context capture/restore) is a FIXME
 * that fails loudly at run time.  Nothing here will actually run Wine.
 *
 * The real implementation has to be ported from wine-fork commit 6a4af726c3
 * (ntdll: Add PPC64 signal handling), which predates the syscall-frame /
 * syscall-dispatcher architecture used by modern Wine and therefore cannot be
 * transplanted directly.
 */

#if 0
#pragma makedep unix
#endif

#ifdef __powerpc64__

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/asm.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);


/***********************************************************************
 *           set_process_instrumentation_callback
 */
void set_process_instrumentation_callback( void *callback )
{
    if (callback) FIXME( "Not supported.\n" );
}


/***********************************************************************
 *              get_native_context
 */
void *get_native_context( CONTEXT *context )
{
    return context;
}


/***********************************************************************
 *              get_wow_context
 */
void *get_wow_context( CONTEXT *context )
{
    return NULL;  /* no WoW64 machines are supported on PowerPC64 */
}


/***********************************************************************
 *              NtSetContextThread  (NTDLL.@)
 *              ZwSetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    BOOL self = (handle == GetCurrentThread());
    NTSTATUS ret;

    if (!self)
    {
        ret = set_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_POWERPC64 );
        if (ret || !self) return ret;
    }
    FIXME( "not implemented for the current thread\n" );
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *              NtGetContextThread  (NTDLL.@)
 *              ZwGetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    BOOL self = (handle == GetCurrentThread());
    NTSTATUS ret;

    if (!self)
    {
        ret = get_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_POWERPC64 );
        if (ret || !self) return ret;
    }
    FIXME( "not implemented for the current thread\n" );
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           signal_set_full_context
 */
NTSTATUS signal_set_full_context( CONTEXT *context )
{
    return NtSetContextThread( GetCurrentThread(), context );
}


/***********************************************************************
 *              get_thread_wow64_context
 */
NTSTATUS get_thread_wow64_context( HANDLE handle, void *ctx, ULONG size )
{
    return STATUS_INVALID_INFO_CLASS;
}


/***********************************************************************
 *              set_thread_wow64_context
 */
NTSTATUS set_thread_wow64_context( HANDLE handle, const void *ctx, ULONG size )
{
    return STATUS_INVALID_INFO_CLASS;
}


/**********************************************************************
 *           get_thread_ldt_entry
 */
NTSTATUS get_thread_ldt_entry( HANDLE handle, THREAD_DESCRIPTOR_INFORMATION *info, ULONG len )
{
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           NtCallbackReturn  (NTDLL.@)
 */
NTSTATUS WINAPI NtCallbackReturn( void *ret_ptr, ULONG ret_len, NTSTATUS status )
{
    FIXME( "not implemented\n" );
    return STATUS_NO_CALLBACK_ACTIVE;
}


/***********************************************************************
 *           call_user_apc_dispatcher
 */
NTSTATUS call_user_apc_dispatcher( CONTEXT *context, unsigned int flags, ULONG_PTR arg1, ULONG_PTR arg2,
                                   ULONG_PTR arg3, PNTAPCFUNC func, NTSTATUS status )
{
    ERR( "not implemented on PowerPC64\n" );
    abort();
}


/***********************************************************************
 *           call_raise_user_exception_dispatcher
 */
void call_raise_user_exception_dispatcher( struct thread_data *data )
{
    ERR( "not implemented on PowerPC64\n" );
    abort();
}


/***********************************************************************
 *           call_user_exception_dispatcher
 */
NTSTATUS call_user_exception_dispatcher( struct thread_data *data, EXCEPTION_RECORD *rec, CONTEXT *context )
{
    ERR( "not implemented on PowerPC64\n" );
    abort();
}


/***********************************************************************
 *           __wine_syscall_dispatcher
 *
 * The real one has to be hand-written assembly that saves a syscall frame and
 * jumps into the syscall table; see signal_arm64.c for the shape of it.
 */
void __wine_syscall_dispatcher(void)
{
    ERR( "not implemented on PowerPC64\n" );
    abort();
}

void __wine_syscall_dispatcher_return(void)
{
    ERR( "not implemented on PowerPC64\n" );
    abort();
}

void __wine_unix_call_dispatcher(void)
{
    ERR( "not implemented on PowerPC64\n" );
    abort();
}


/**********************************************************************
 *             signal_alloc_thread
 */
NTSTATUS signal_alloc_thread( TEB *teb )
{
    return STATUS_SUCCESS;
}


/**********************************************************************
 *             signal_free_thread
 */
void signal_free_thread( TEB *teb )
{
}


/**********************************************************************
 *		signal_init_process
 */
void signal_init_process( TEB *teb )
{
    ERR( "signal handling is not implemented on PowerPC64\n" );
    abort();
}


/***********************************************************************
 *           signal_start_thread
 */
void signal_start_thread( PRTL_THREAD_START_ROUTINE entry, void *arg, TEB *teb )
{
    ERR( "not implemented on PowerPC64\n" );
    abort();
}

#endif  /* __powerpc64__ */
