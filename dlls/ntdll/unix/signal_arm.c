/*
 * ARM signal handling routines
 *
 * Copyright 2002 Marcus Meissner, SuSE Linux AG
 * Copyright 2010-2013, 2015 André Hentschel
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

#ifdef __arm__

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/asm.h"
#include "unix_private.h"
#include "wine/debug.h"

static pthread_key_t teb_key;


/***********************************************************************
 *           context_to_server
 *
 * Convert a register context to the server format.
 */
NTSTATUS context_to_server( context_t *to, const CONTEXT *from )
{
    DWORD i, flags = from->ContextFlags & ~CONTEXT_ARM;  /* get rid of CPU id */

    memset( to, 0, sizeof(*to) );
    to->cpu = CPU_ARM;

    if (flags & CONTEXT_CONTROL)
    {
        to->flags |= SERVER_CTX_CONTROL;
        to->ctl.arm_regs.sp   = from->Sp;
        to->ctl.arm_regs.lr   = from->Lr;
        to->ctl.arm_regs.pc   = from->Pc;
        to->ctl.arm_regs.cpsr = from->Cpsr;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->flags |= SERVER_CTX_INTEGER;
        to->integer.arm_regs.r[0]  = from->R0;
        to->integer.arm_regs.r[1]  = from->R1;
        to->integer.arm_regs.r[2]  = from->R2;
        to->integer.arm_regs.r[3]  = from->R3;
        to->integer.arm_regs.r[4]  = from->R4;
        to->integer.arm_regs.r[5]  = from->R5;
        to->integer.arm_regs.r[6]  = from->R6;
        to->integer.arm_regs.r[7]  = from->R7;
        to->integer.arm_regs.r[8]  = from->R8;
        to->integer.arm_regs.r[9]  = from->R9;
        to->integer.arm_regs.r[10] = from->R10;
        to->integer.arm_regs.r[11] = from->R11;
        to->integer.arm_regs.r[12] = from->R12;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->flags |= SERVER_CTX_FLOATING_POINT;
        for (i = 0; i < 32; i++) to->fp.arm_regs.d[i] = from->u.D[i];
        to->fp.arm_regs.fpscr = from->Fpscr;
    }
    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        to->flags |= SERVER_CTX_DEBUG_REGISTERS;
        for (i = 0; i < ARM_MAX_BREAKPOINTS; i++) to->debug.arm_regs.bvr[i] = from->Bvr[i];
        for (i = 0; i < ARM_MAX_BREAKPOINTS; i++) to->debug.arm_regs.bcr[i] = from->Bcr[i];
        for (i = 0; i < ARM_MAX_WATCHPOINTS; i++) to->debug.arm_regs.wvr[i] = from->Wvr[i];
        for (i = 0; i < ARM_MAX_WATCHPOINTS; i++) to->debug.arm_regs.wcr[i] = from->Wcr[i];
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
    DWORD i;

    if (from->cpu != CPU_ARM) return STATUS_INVALID_PARAMETER;

    to->ContextFlags = CONTEXT_ARM;
    if (from->flags & SERVER_CTX_CONTROL)
    {
        to->ContextFlags |= CONTEXT_CONTROL;
        to->Sp   = from->ctl.arm_regs.sp;
        to->Lr   = from->ctl.arm_regs.lr;
        to->Pc   = from->ctl.arm_regs.pc;
        to->Cpsr = from->ctl.arm_regs.cpsr;
    }
    if (from->flags & SERVER_CTX_INTEGER)
    {
        to->ContextFlags |= CONTEXT_INTEGER;
        to->R0  = from->integer.arm_regs.r[0];
        to->R1  = from->integer.arm_regs.r[1];
        to->R2  = from->integer.arm_regs.r[2];
        to->R3  = from->integer.arm_regs.r[3];
        to->R4  = from->integer.arm_regs.r[4];
        to->R5  = from->integer.arm_regs.r[5];
        to->R6  = from->integer.arm_regs.r[6];
        to->R7  = from->integer.arm_regs.r[7];
        to->R8  = from->integer.arm_regs.r[8];
        to->R9  = from->integer.arm_regs.r[9];
        to->R10 = from->integer.arm_regs.r[10];
        to->R11 = from->integer.arm_regs.r[11];
        to->R12 = from->integer.arm_regs.r[12];
    }
    if (from->flags & SERVER_CTX_FLOATING_POINT)
    {
        to->ContextFlags |= CONTEXT_FLOATING_POINT;
        for (i = 0; i < 32; i++) to->u.D[i] = from->fp.arm_regs.d[i];
        to->Fpscr = from->fp.arm_regs.fpscr;
    }
    if (from->flags & SERVER_CTX_DEBUG_REGISTERS)
    {
        to->ContextFlags |= CONTEXT_DEBUG_REGISTERS;
        for (i = 0; i < ARM_MAX_BREAKPOINTS; i++) to->Bvr[i] = from->debug.arm_regs.bvr[i];
        for (i = 0; i < ARM_MAX_BREAKPOINTS; i++) to->Bcr[i] = from->debug.arm_regs.bcr[i];
        for (i = 0; i < ARM_MAX_WATCHPOINTS; i++) to->Wvr[i] = from->debug.arm_regs.wvr[i];
        for (i = 0; i < ARM_MAX_WATCHPOINTS; i++) to->Wcr[i] = from->debug.arm_regs.wcr[i];
    }
    return STATUS_SUCCESS;
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
 *		signal_init_thread
 */
void signal_init_thread( TEB *teb )
{
#if defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_8A__)
    /* Win32/ARM applications expect the TEB pointer to be in the TPIDRURW register. */
    __asm__ __volatile__( "mcr p15, 0, %0, c13, c0, 2" : : "r" (teb) );
#endif
    pthread_setspecific( teb_key, teb );
}


extern void DECLSPEC_NORETURN call_thread_exit_func( int status, void (*func)(int), TEB *teb );
__ASM_GLOBAL_FUNC( call_thread_exit_func,
                   ".arm\n\t"
                   "ldr r3, [r2, #0x1d4]\n\t"  /* teb->SystemReserved2 */
                   "mov ip, #0\n\t"
                   "str ip, [r2, #0x1d4]\n\t"
                   "cmp r3, ip\n\t"
                   "movne sp, r3\n\t"
                   "blx r1" )

/***********************************************************************
 *           signal_exit_thread
 */
void signal_exit_thread( int status, void (*func)(int) )
{
    call_thread_exit_func( status, func, NtCurrentTeb() );
}


/**********************************************************************
 *           NtCurrentTeb   (NTDLL.@)
 */
TEB * WINAPI NtCurrentTeb(void)
{
    return pthread_getspecific( teb_key );
}

#endif  /* __arm__ */
