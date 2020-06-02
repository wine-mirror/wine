/*
 * PowerPC signal handling routines
 *
 * Copyright 2002 Marcus Meissner, SuSE Linux AG
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

#ifdef __powerpc__

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYSCALL_H
# include <syscall.h>
#else
# ifdef HAVE_SYS_SYSCALL_H
#  include <sys/syscall.h>
# endif
#endif
#ifdef HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif
#ifdef HAVE_SYS_UCONTEXT_H
# include <sys/ucontext.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/asm.h"
#include "unix_private.h"
#include "wine/debug.h"

static pthread_key_t teb_key;


/***********************************************************************
 *           set_cpu_context
 *
 * Set the new CPU context.
 */
static void set_cpu_context( const CONTEXT *context )
{
    FIXME("not implemented\n");
}


/***********************************************************************
 *           context_to_server
 *
 * Convert a register context to the server format.
 */
NTSTATUS context_to_server( context_t *to, const CONTEXT *from )
{
    DWORD flags = from->ContextFlags;  /* no CPU id? */

    memset( to, 0, sizeof(*to) );
    to->cpu = CPU_POWERPC;

    if (flags & CONTEXT_CONTROL)
    {
        to->flags |= SERVER_CTX_CONTROL;
        to->ctl.powerpc_regs.iar   = from->Iar;
        to->ctl.powerpc_regs.msr   = from->Msr;
        to->ctl.powerpc_regs.ctr   = from->Ctr;
        to->ctl.powerpc_regs.lr    = from->Lr;
        to->ctl.powerpc_regs.dar   = from->Dar;
        to->ctl.powerpc_regs.dsisr = from->Dsisr;
        to->ctl.powerpc_regs.trap  = from->Trap;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->flags |= SERVER_CTX_INTEGER;
        to->integer.powerpc_regs.gpr[0]  = from->Gpr0;
        to->integer.powerpc_regs.gpr[1]  = from->Gpr1;
        to->integer.powerpc_regs.gpr[2]  = from->Gpr2;
        to->integer.powerpc_regs.gpr[3]  = from->Gpr3;
        to->integer.powerpc_regs.gpr[4]  = from->Gpr4;
        to->integer.powerpc_regs.gpr[5]  = from->Gpr5;
        to->integer.powerpc_regs.gpr[6]  = from->Gpr6;
        to->integer.powerpc_regs.gpr[7]  = from->Gpr7;
        to->integer.powerpc_regs.gpr[8]  = from->Gpr8;
        to->integer.powerpc_regs.gpr[9]  = from->Gpr9;
        to->integer.powerpc_regs.gpr[10] = from->Gpr10;
        to->integer.powerpc_regs.gpr[11] = from->Gpr11;
        to->integer.powerpc_regs.gpr[12] = from->Gpr12;
        to->integer.powerpc_regs.gpr[13] = from->Gpr13;
        to->integer.powerpc_regs.gpr[14] = from->Gpr14;
        to->integer.powerpc_regs.gpr[15] = from->Gpr15;
        to->integer.powerpc_regs.gpr[16] = from->Gpr16;
        to->integer.powerpc_regs.gpr[17] = from->Gpr17;
        to->integer.powerpc_regs.gpr[18] = from->Gpr18;
        to->integer.powerpc_regs.gpr[19] = from->Gpr19;
        to->integer.powerpc_regs.gpr[20] = from->Gpr20;
        to->integer.powerpc_regs.gpr[21] = from->Gpr21;
        to->integer.powerpc_regs.gpr[22] = from->Gpr22;
        to->integer.powerpc_regs.gpr[23] = from->Gpr23;
        to->integer.powerpc_regs.gpr[24] = from->Gpr24;
        to->integer.powerpc_regs.gpr[25] = from->Gpr25;
        to->integer.powerpc_regs.gpr[26] = from->Gpr26;
        to->integer.powerpc_regs.gpr[27] = from->Gpr27;
        to->integer.powerpc_regs.gpr[28] = from->Gpr28;
        to->integer.powerpc_regs.gpr[29] = from->Gpr29;
        to->integer.powerpc_regs.gpr[30] = from->Gpr30;
        to->integer.powerpc_regs.gpr[31] = from->Gpr31;
        to->integer.powerpc_regs.xer     = from->Xer;
        to->integer.powerpc_regs.cr      = from->Cr;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->flags |= SERVER_CTX_FLOATING_POINT;
        to->fp.powerpc_regs.fpr[0]  = from->Fpr0;
        to->fp.powerpc_regs.fpr[1]  = from->Fpr1;
        to->fp.powerpc_regs.fpr[2]  = from->Fpr2;
        to->fp.powerpc_regs.fpr[3]  = from->Fpr3;
        to->fp.powerpc_regs.fpr[4]  = from->Fpr4;
        to->fp.powerpc_regs.fpr[5]  = from->Fpr5;
        to->fp.powerpc_regs.fpr[6]  = from->Fpr6;
        to->fp.powerpc_regs.fpr[7]  = from->Fpr7;
        to->fp.powerpc_regs.fpr[8]  = from->Fpr8;
        to->fp.powerpc_regs.fpr[9]  = from->Fpr9;
        to->fp.powerpc_regs.fpr[10] = from->Fpr10;
        to->fp.powerpc_regs.fpr[11] = from->Fpr11;
        to->fp.powerpc_regs.fpr[12] = from->Fpr12;
        to->fp.powerpc_regs.fpr[13] = from->Fpr13;
        to->fp.powerpc_regs.fpr[14] = from->Fpr14;
        to->fp.powerpc_regs.fpr[15] = from->Fpr15;
        to->fp.powerpc_regs.fpr[16] = from->Fpr16;
        to->fp.powerpc_regs.fpr[17] = from->Fpr17;
        to->fp.powerpc_regs.fpr[18] = from->Fpr18;
        to->fp.powerpc_regs.fpr[19] = from->Fpr19;
        to->fp.powerpc_regs.fpr[20] = from->Fpr20;
        to->fp.powerpc_regs.fpr[21] = from->Fpr21;
        to->fp.powerpc_regs.fpr[22] = from->Fpr22;
        to->fp.powerpc_regs.fpr[23] = from->Fpr23;
        to->fp.powerpc_regs.fpr[24] = from->Fpr24;
        to->fp.powerpc_regs.fpr[25] = from->Fpr25;
        to->fp.powerpc_regs.fpr[26] = from->Fpr26;
        to->fp.powerpc_regs.fpr[27] = from->Fpr27;
        to->fp.powerpc_regs.fpr[28] = from->Fpr28;
        to->fp.powerpc_regs.fpr[29] = from->Fpr29;
        to->fp.powerpc_regs.fpr[30] = from->Fpr30;
        to->fp.powerpc_regs.fpr[31] = from->Fpr31;
        to->fp.powerpc_regs.fpscr   = from->Fpscr;
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           context_from_server
 *
 * Convert a register context from the server format.
 */
NTSTATUS context_from_server( CONTEXT *to, const context_t *from )
{
    if (from->cpu != CPU_POWERPC) return STATUS_INVALID_PARAMETER;

    to->ContextFlags = 0;  /* no CPU id? */
    if (from->flags & SERVER_CTX_CONTROL)
    {
        to->ContextFlags |= CONTEXT_CONTROL;
        to->Msr   = from->ctl.powerpc_regs.msr;
        to->Ctr   = from->ctl.powerpc_regs.ctr;
        to->Iar   = from->ctl.powerpc_regs.iar;
        to->Lr    = from->ctl.powerpc_regs.lr;
        to->Dar   = from->ctl.powerpc_regs.dar;
        to->Dsisr = from->ctl.powerpc_regs.dsisr;
        to->Trap  = from->ctl.powerpc_regs.trap;
    }
    if (from->flags & SERVER_CTX_INTEGER)
    {
        to->ContextFlags |= CONTEXT_INTEGER;
        to->Gpr0  = from->integer.powerpc_regs.gpr[0];
        to->Gpr1  = from->integer.powerpc_regs.gpr[1];
        to->Gpr2  = from->integer.powerpc_regs.gpr[2];
        to->Gpr3  = from->integer.powerpc_regs.gpr[3];
        to->Gpr4  = from->integer.powerpc_regs.gpr[4];
        to->Gpr5  = from->integer.powerpc_regs.gpr[5];
        to->Gpr6  = from->integer.powerpc_regs.gpr[6];
        to->Gpr7  = from->integer.powerpc_regs.gpr[7];
        to->Gpr8  = from->integer.powerpc_regs.gpr[8];
        to->Gpr9  = from->integer.powerpc_regs.gpr[9];
        to->Gpr10 = from->integer.powerpc_regs.gpr[10];
        to->Gpr11 = from->integer.powerpc_regs.gpr[11];
        to->Gpr12 = from->integer.powerpc_regs.gpr[12];
        to->Gpr13 = from->integer.powerpc_regs.gpr[13];
        to->Gpr14 = from->integer.powerpc_regs.gpr[14];
        to->Gpr15 = from->integer.powerpc_regs.gpr[15];
        to->Gpr16 = from->integer.powerpc_regs.gpr[16];
        to->Gpr17 = from->integer.powerpc_regs.gpr[17];
        to->Gpr18 = from->integer.powerpc_regs.gpr[18];
        to->Gpr19 = from->integer.powerpc_regs.gpr[19];
        to->Gpr20 = from->integer.powerpc_regs.gpr[20];
        to->Gpr21 = from->integer.powerpc_regs.gpr[21];
        to->Gpr22 = from->integer.powerpc_regs.gpr[22];
        to->Gpr23 = from->integer.powerpc_regs.gpr[23];
        to->Gpr24 = from->integer.powerpc_regs.gpr[24];
        to->Gpr25 = from->integer.powerpc_regs.gpr[25];
        to->Gpr26 = from->integer.powerpc_regs.gpr[26];
        to->Gpr27 = from->integer.powerpc_regs.gpr[27];
        to->Gpr28 = from->integer.powerpc_regs.gpr[28];
        to->Gpr29 = from->integer.powerpc_regs.gpr[29];
        to->Gpr30 = from->integer.powerpc_regs.gpr[30];
        to->Gpr31 = from->integer.powerpc_regs.gpr[31];
        to->Xer   = from->integer.powerpc_regs.xer;
        to->Cr    = from->integer.powerpc_regs.cr;
    }
    if (from->flags & SERVER_CTX_FLOATING_POINT)
    {
        to->ContextFlags |= CONTEXT_FLOATING_POINT;
        to->Fpr0  = from->fp.powerpc_regs.fpr[0];
        to->Fpr1  = from->fp.powerpc_regs.fpr[1];
        to->Fpr2  = from->fp.powerpc_regs.fpr[2];
        to->Fpr3  = from->fp.powerpc_regs.fpr[3];
        to->Fpr4  = from->fp.powerpc_regs.fpr[4];
        to->Fpr5  = from->fp.powerpc_regs.fpr[5];
        to->Fpr6  = from->fp.powerpc_regs.fpr[6];
        to->Fpr7  = from->fp.powerpc_regs.fpr[7];
        to->Fpr8  = from->fp.powerpc_regs.fpr[8];
        to->Fpr9  = from->fp.powerpc_regs.fpr[9];
        to->Fpr10 = from->fp.powerpc_regs.fpr[10];
        to->Fpr11 = from->fp.powerpc_regs.fpr[11];
        to->Fpr12 = from->fp.powerpc_regs.fpr[12];
        to->Fpr13 = from->fp.powerpc_regs.fpr[13];
        to->Fpr14 = from->fp.powerpc_regs.fpr[14];
        to->Fpr15 = from->fp.powerpc_regs.fpr[15];
        to->Fpr16 = from->fp.powerpc_regs.fpr[16];
        to->Fpr17 = from->fp.powerpc_regs.fpr[17];
        to->Fpr18 = from->fp.powerpc_regs.fpr[18];
        to->Fpr19 = from->fp.powerpc_regs.fpr[19];
        to->Fpr20 = from->fp.powerpc_regs.fpr[20];
        to->Fpr21 = from->fp.powerpc_regs.fpr[21];
        to->Fpr22 = from->fp.powerpc_regs.fpr[22];
        to->Fpr23 = from->fp.powerpc_regs.fpr[23];
        to->Fpr24 = from->fp.powerpc_regs.fpr[24];
        to->Fpr25 = from->fp.powerpc_regs.fpr[25];
        to->Fpr26 = from->fp.powerpc_regs.fpr[26];
        to->Fpr27 = from->fp.powerpc_regs.fpr[27];
        to->Fpr28 = from->fp.powerpc_regs.fpr[28];
        to->Fpr29 = from->fp.powerpc_regs.fpr[29];
        to->Fpr30 = from->fp.powerpc_regs.fpr[30];
        to->Fpr31 = from->fp.powerpc_regs.fpr[31];
        to->Fpscr = from->fp.powerpc_regs.fpscr;
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *              NtSetContextThread  (NTDLL.@)
 *              ZwSetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    NTSTATUS ret;
    BOOL self;
    context_t server_context;

    context_to_server( &server_context, context );
    ret = set_thread_context( handle, &server_context, &self );
    if (self && ret == STATUS_SUCCESS) set_cpu_context( context );
    return ret;
}


/**********************************************************************
 *           get_thread_ldt_entry
 */
NTSTATUS CDECL get_thread_ldt_entry( HANDLE handle, void *data, ULONG len, ULONG *ret_len )
{
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *           NtSetLdtEntries   (NTDLL.@)
 *           ZwSetLdtEntries   (NTDLL.@)
 */
NTSTATUS WINAPI NtSetLdtEntries( ULONG sel1, LDT_ENTRY entry1, ULONG sel2, LDT_ENTRY entry2 )
{
    return STATUS_NOT_IMPLEMENTED;
}


/**********************************************************************
 *             signal_init_threading
 */
void signal_init_threading(void)
{
    pthread_key_create( &teb_key, NULL );
}


/**********************************************************************
 *		signal_alloc_thread
 */
NTSTATUS signal_alloc_thread( TEB *teb )
{
    return STATUS_SUCCESS;
}


/**********************************************************************
 *		signal_free_thread
 */
void signal_free_thread( TEB *teb )
{
}


/**********************************************************************
 *		signal_init_thread
 */
void signal_init_thread( TEB *teb )
{
    pthread_setspecific( teb_key, teb );
}


/***********************************************************************
 *           signal_exit_thread
 */
void signal_exit_thread( int status, void (*func)(int) )
{
    func( status );
}


/**********************************************************************
 *           NtCurrentTeb   (NTDLL.@)
 */
TEB * WINAPI NtCurrentTeb(void)
{
    return pthread_getspecific( teb_key );
}

#endif  /* __powerpc__ */
