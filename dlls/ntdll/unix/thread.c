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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#ifdef HAVE_SCHED_H
#include <sched.h>
#endif
#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif
#ifdef HAVE_SYS_USER_H
#include <sys/user.h>
#endif
#ifdef HAVE_LIBPROCSTAT_H
#include <libprocstat.h>
#endif

#ifdef __APPLE__
#include <mach/mach.h>
#endif
#ifdef __FreeBSD__
#include <sys/thr.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "ddk/wdm.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(thread);
WINE_DECLARE_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(threadname);

pthread_key_t teb_key = 0;

static LONG nb_threads = 1;

static inline int get_unix_exit_code( NTSTATUS status )
{
    /* prevent a nonzero exit code to end up truncated to zero in unix */
    if (status && !(status & 0xff)) return 1;
    return status;
}


/***********************************************************************
 *           fpux_to_fpu
 *
 * Build a standard i386 FPU context from an extended one.
 */
void fpux_to_fpu( I386_FLOATING_SAVE_AREA *fpu, const XMM_SAVE_AREA32 *fpux )
{
    unsigned int i, tag, stack_top;

    fpu->ControlWord   = fpux->ControlWord;
    fpu->StatusWord    = fpux->StatusWord;
    fpu->ErrorOffset   = fpux->ErrorOffset;
    fpu->ErrorSelector = fpux->ErrorSelector | (fpux->ErrorOpcode << 16);
    fpu->DataOffset    = fpux->DataOffset;
    fpu->DataSelector  = fpux->DataSelector;
    fpu->Cr0NpxState   = fpux->StatusWord | 0xffff0000;

    stack_top = (fpux->StatusWord >> 11) & 7;
    fpu->TagWord = 0xffff0000;
    for (i = 0; i < 8; i++)
    {
        memcpy( &fpu->RegisterArea[10 * i], &fpux->FloatRegisters[i], 10 );
        if (!(fpux->TagWord & (1 << i))) tag = 3;  /* empty */
        else
        {
            const M128A *reg = &fpux->FloatRegisters[(i - stack_top) & 7];
            if ((reg->High & 0x7fff) == 0x7fff)  /* exponent all ones */
            {
                tag = 2;  /* special */
            }
            else if (!(reg->High & 0x7fff))  /* exponent all zeroes */
            {
                if (reg->Low) tag = 2;  /* special */
                else tag = 1;  /* zero */
            }
            else
            {
                if (reg->Low >> 63) tag = 0;  /* valid */
                else tag = 2;  /* special */
            }
        }
        fpu->TagWord |= tag << (2 * i);
    }
}


/***********************************************************************
 *           fpu_to_fpux
 *
 * Fill extended i386 FPU context from standard one.
 */
void fpu_to_fpux( XMM_SAVE_AREA32 *fpux, const I386_FLOATING_SAVE_AREA *fpu )
{
    unsigned int i;

    fpux->ControlWord   = fpu->ControlWord;
    fpux->StatusWord    = fpu->StatusWord;
    fpux->ErrorOffset   = fpu->ErrorOffset;
    fpux->ErrorSelector = fpu->ErrorSelector;
    fpux->ErrorOpcode   = fpu->ErrorSelector >> 16;
    fpux->DataOffset    = fpu->DataOffset;
    fpux->DataSelector  = fpu->DataSelector;
    fpux->TagWord       = 0;
    for (i = 0; i < 8; i++)
    {
        if (((fpu->TagWord >> (i * 2)) & 3) != 3) fpux->TagWord |= 1 << i;
        memcpy( &fpux->FloatRegisters[i], &fpu->RegisterArea[10 * i], 10 );
    }
}


/***********************************************************************
 *           get_server_context_flags
 */
static unsigned int get_server_context_flags( const void *context, USHORT machine )
{
    unsigned int flags = 0, ret = 0;

    switch (machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        flags = ((const I386_CONTEXT *)context)->ContextFlags & ~CONTEXT_i386;
        if (flags & CONTEXT_I386_CONTROL) ret |= SERVER_CTX_CONTROL;
        if (flags & CONTEXT_I386_INTEGER) ret |= SERVER_CTX_INTEGER;
        if (flags & CONTEXT_I386_SEGMENTS) ret |= SERVER_CTX_SEGMENTS;
        if (flags & CONTEXT_I386_FLOATING_POINT) ret |= SERVER_CTX_FLOATING_POINT;
        if (flags & CONTEXT_I386_DEBUG_REGISTERS) ret |= SERVER_CTX_DEBUG_REGISTERS;
        if (flags & CONTEXT_I386_EXTENDED_REGISTERS) ret |= SERVER_CTX_EXTENDED_REGISTERS | SERVER_CTX_FLOATING_POINT;
        if (flags & CONTEXT_I386_XSTATE) ret |= SERVER_CTX_YMM_REGISTERS;
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        flags = ((const AMD64_CONTEXT *)context)->ContextFlags & ~CONTEXT_AMD64;
        if (flags & CONTEXT_AMD64_CONTROL) ret |= SERVER_CTX_CONTROL;
        if (flags & CONTEXT_AMD64_INTEGER) ret |= SERVER_CTX_INTEGER;
        if (flags & CONTEXT_AMD64_SEGMENTS) ret |= SERVER_CTX_SEGMENTS;
        if (flags & CONTEXT_AMD64_FLOATING_POINT) ret |= SERVER_CTX_FLOATING_POINT;
        if (flags & CONTEXT_AMD64_DEBUG_REGISTERS) ret |= SERVER_CTX_DEBUG_REGISTERS;
        if (flags & CONTEXT_AMD64_XSTATE) ret |= SERVER_CTX_YMM_REGISTERS;
        break;
    case IMAGE_FILE_MACHINE_ARMNT:
        flags = ((const ARM_CONTEXT *)context)->ContextFlags & ~CONTEXT_ARM;
        if (flags & CONTEXT_ARM_CONTROL) ret |= SERVER_CTX_CONTROL;
        if (flags & CONTEXT_ARM_INTEGER) ret |= SERVER_CTX_INTEGER;
        if (flags & CONTEXT_ARM_FLOATING_POINT) ret |= SERVER_CTX_FLOATING_POINT;
        if (flags & CONTEXT_ARM_DEBUG_REGISTERS) ret |= SERVER_CTX_DEBUG_REGISTERS;
        break;
    case IMAGE_FILE_MACHINE_ARM64:
        flags = ((const ARM64_NT_CONTEXT *)context)->ContextFlags & ~CONTEXT_ARM64;
        if (flags & CONTEXT_ARM64_CONTROL) ret |= SERVER_CTX_CONTROL;
        if (flags & CONTEXT_ARM64_INTEGER) ret |= SERVER_CTX_INTEGER;
        if (flags & CONTEXT_ARM64_FLOATING_POINT) ret |= SERVER_CTX_FLOATING_POINT;
        if (flags & CONTEXT_ARM64_DEBUG_REGISTERS) ret |= SERVER_CTX_DEBUG_REGISTERS;
        break;
    }
    if (flags & CONTEXT_EXCEPTION_REQUEST) ret |= SERVER_CTX_EXEC_SPACE;
    return ret;
}


/***********************************************************************
 *           get_native_context_flags
 *
 * Get flags for registers that are set from the native context in WoW mode.
 */
static unsigned int get_native_context_flags( USHORT native_machine, USHORT wow_machine )
{
    switch (MAKELONG( native_machine, wow_machine ))
    {
    case MAKELONG( IMAGE_FILE_MACHINE_AMD64, IMAGE_FILE_MACHINE_I386 ):
        return SERVER_CTX_DEBUG_REGISTERS | SERVER_CTX_FLOATING_POINT | SERVER_CTX_YMM_REGISTERS | SERVER_CTX_EXEC_SPACE;
    case MAKELONG( IMAGE_FILE_MACHINE_ARM64, IMAGE_FILE_MACHINE_ARMNT ):
        return SERVER_CTX_DEBUG_REGISTERS | SERVER_CTX_FLOATING_POINT | SERVER_CTX_EXEC_SPACE;
    default:
        return SERVER_CTX_EXEC_SPACE;
    }
}


/***********************************************************************
 *           xstate_to_server
 *
 * Copy xstate to the server format.
 */
static void xstate_to_server( struct context_data *to, const CONTEXT_EX *xctx )
{
    const XSTATE *xs = (const XSTATE *)((const char *)xctx + xctx->XState.Offset);

    if (xs->CompactionMask && !(xs->CompactionMask & 4)) return;
    to->flags |= SERVER_CTX_YMM_REGISTERS;
    if (xs->Mask & 4) memcpy( &to->ymm.regs.ymm_high, &xs->YmmContext, sizeof(xs->YmmContext) );
}


/***********************************************************************
 *           exception_request_flags_to_server
 *
 * Copy exception reporting flags to the server format.
 */
static void exception_request_flags_to_server( struct context_data *to, DWORD context_flags )
{
    if (!(context_flags & CONTEXT_EXCEPTION_REPORTING)) return;
    to->flags |= SERVER_CTX_EXEC_SPACE;
    if (context_flags & CONTEXT_SERVICE_ACTIVE)        to->exec_space.space.space = EXEC_SPACE_SYSCALL;
    else if (context_flags & CONTEXT_EXCEPTION_ACTIVE) to->exec_space.space.space = EXEC_SPACE_EXCEPTION;
    else                                               to->exec_space.space.space = EXEC_SPACE_USERMODE;
}


/***********************************************************************
 *           context_to_server
 *
 * Convert a register context to the server format.
 */
static NTSTATUS context_to_server( struct context_data *to, USHORT to_machine, const void *src, USHORT from_machine )
{
    DWORD i, flags;

    memset( to, 0, sizeof(*to) );
    to->machine = to_machine;

    switch (MAKELONG( from_machine, to_machine ))
    {
    case MAKELONG( IMAGE_FILE_MACHINE_I386, IMAGE_FILE_MACHINE_I386 ):
    {
        const I386_CONTEXT *from = src;

        flags = from->ContextFlags & ~CONTEXT_i386;
        if (flags & CONTEXT_I386_CONTROL)
        {
            to->flags |= SERVER_CTX_CONTROL;
            to->ctl.i386_regs.ebp    = from->Ebp;
            to->ctl.i386_regs.esp    = from->Esp;
            to->ctl.i386_regs.eip    = from->Eip;
            to->ctl.i386_regs.cs     = from->SegCs;
            to->ctl.i386_regs.ss     = from->SegSs;
            to->ctl.i386_regs.eflags = from->EFlags;
        }
        if (flags & CONTEXT_I386_INTEGER)
        {
            to->flags |= SERVER_CTX_INTEGER;
            to->integer.i386_regs.eax = from->Eax;
            to->integer.i386_regs.ebx = from->Ebx;
            to->integer.i386_regs.ecx = from->Ecx;
            to->integer.i386_regs.edx = from->Edx;
            to->integer.i386_regs.esi = from->Esi;
            to->integer.i386_regs.edi = from->Edi;
        }
        if (flags & CONTEXT_I386_SEGMENTS)
        {
            to->flags |= SERVER_CTX_SEGMENTS;
            to->seg.i386_regs.ds = from->SegDs;
            to->seg.i386_regs.es = from->SegEs;
            to->seg.i386_regs.fs = from->SegFs;
            to->seg.i386_regs.gs = from->SegGs;
        }
        if (flags & CONTEXT_I386_FLOATING_POINT)
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
        if (flags & CONTEXT_I386_DEBUG_REGISTERS)
        {
            to->flags |= SERVER_CTX_DEBUG_REGISTERS;
            to->debug.i386_regs.dr0 = from->Dr0;
            to->debug.i386_regs.dr1 = from->Dr1;
            to->debug.i386_regs.dr2 = from->Dr2;
            to->debug.i386_regs.dr3 = from->Dr3;
            to->debug.i386_regs.dr6 = from->Dr6;
            to->debug.i386_regs.dr7 = from->Dr7;
        }
        if (flags & CONTEXT_I386_EXTENDED_REGISTERS)
        {
            to->flags |= SERVER_CTX_EXTENDED_REGISTERS;
            memcpy( to->ext.i386_regs, from->ExtendedRegisters, sizeof(to->ext.i386_regs) );
        }
        if (flags & CONTEXT_I386_XSTATE)
            xstate_to_server( to, (const CONTEXT_EX *)(from + 1) );
        exception_request_flags_to_server( to, flags );
        return STATUS_SUCCESS;
    }

    case MAKELONG( IMAGE_FILE_MACHINE_I386, IMAGE_FILE_MACHINE_AMD64 ):
    {
        const I386_CONTEXT *from = src;

        flags = from->ContextFlags & ~CONTEXT_i386;
        if (flags & CONTEXT_I386_CONTROL)
        {
            to->flags |= SERVER_CTX_CONTROL;
            to->ctl.x86_64_regs.rbp    = from->Ebp;
            to->ctl.x86_64_regs.rsp    = from->Esp;
            to->ctl.x86_64_regs.rip    = from->Eip;
            to->ctl.x86_64_regs.cs     = from->SegCs;
            to->ctl.x86_64_regs.ss     = from->SegSs;
            to->ctl.x86_64_regs.flags  = from->EFlags;
        }
        if (flags & CONTEXT_I386_INTEGER)
        {
            to->flags |= SERVER_CTX_INTEGER;
            to->integer.x86_64_regs.rax = from->Eax;
            to->integer.x86_64_regs.rbx = from->Ebx;
            to->integer.x86_64_regs.rcx = from->Ecx;
            to->integer.x86_64_regs.rdx = from->Edx;
            to->integer.x86_64_regs.rsi = from->Esi;
            to->integer.x86_64_regs.rdi = from->Edi;
        }
        if (flags & CONTEXT_I386_SEGMENTS)
        {
            to->flags |= SERVER_CTX_SEGMENTS;
            to->seg.x86_64_regs.ds = from->SegDs;
            to->seg.x86_64_regs.es = from->SegEs;
            to->seg.x86_64_regs.fs = from->SegFs;
            to->seg.x86_64_regs.gs = from->SegGs;
        }
        if (flags & CONTEXT_I386_DEBUG_REGISTERS)
        {
            to->flags |= SERVER_CTX_DEBUG_REGISTERS;
            to->debug.x86_64_regs.dr0 = from->Dr0;
            to->debug.x86_64_regs.dr1 = from->Dr1;
            to->debug.x86_64_regs.dr2 = from->Dr2;
            to->debug.x86_64_regs.dr3 = from->Dr3;
            to->debug.x86_64_regs.dr6 = from->Dr6;
            to->debug.x86_64_regs.dr7 = from->Dr7;
        }
        if (flags & CONTEXT_I386_EXTENDED_REGISTERS)
        {
            to->flags |= SERVER_CTX_FLOATING_POINT;
            memcpy( to->fp.x86_64_regs.fpregs, from->ExtendedRegisters, sizeof(to->fp.x86_64_regs.fpregs) );
        }
        else if (flags & CONTEXT_I386_FLOATING_POINT)
        {
            to->flags |= SERVER_CTX_FLOATING_POINT;
            fpu_to_fpux( (XMM_SAVE_AREA32 *)to->fp.x86_64_regs.fpregs, &from->FloatSave );
        }
        if (flags & CONTEXT_I386_XSTATE)
            xstate_to_server( to, (const CONTEXT_EX *)(from + 1) );
        exception_request_flags_to_server( to, flags );
        return STATUS_SUCCESS;
    }

    case MAKELONG( IMAGE_FILE_MACHINE_AMD64, IMAGE_FILE_MACHINE_AMD64 ):
    {
        const AMD64_CONTEXT *from = src;

        flags = from->ContextFlags & ~CONTEXT_AMD64;
        if (flags & CONTEXT_AMD64_CONTROL)
        {
            to->flags |= SERVER_CTX_CONTROL;
            to->ctl.x86_64_regs.rbp   = from->Rbp;
            to->ctl.x86_64_regs.rip   = from->Rip;
            to->ctl.x86_64_regs.rsp   = from->Rsp;
            to->ctl.x86_64_regs.cs    = from->SegCs;
            to->ctl.x86_64_regs.ss    = from->SegSs;
            to->ctl.x86_64_regs.flags = from->EFlags;
        }
        if (flags & CONTEXT_AMD64_INTEGER)
        {
            to->flags |= SERVER_CTX_INTEGER;
            to->integer.x86_64_regs.rax = from->Rax;
            to->integer.x86_64_regs.rcx = from->Rcx;
            to->integer.x86_64_regs.rdx = from->Rdx;
            to->integer.x86_64_regs.rbx = from->Rbx;
            to->integer.x86_64_regs.rsi = from->Rsi;
            to->integer.x86_64_regs.rdi = from->Rdi;
            to->integer.x86_64_regs.r8  = from->R8;
            to->integer.x86_64_regs.r9  = from->R9;
            to->integer.x86_64_regs.r10 = from->R10;
            to->integer.x86_64_regs.r11 = from->R11;
            to->integer.x86_64_regs.r12 = from->R12;
            to->integer.x86_64_regs.r13 = from->R13;
            to->integer.x86_64_regs.r14 = from->R14;
            to->integer.x86_64_regs.r15 = from->R15;
        }
        if (flags & CONTEXT_AMD64_SEGMENTS)
        {
            to->flags |= SERVER_CTX_SEGMENTS;
            to->seg.x86_64_regs.ds = from->SegDs;
            to->seg.x86_64_regs.es = from->SegEs;
            to->seg.x86_64_regs.fs = from->SegFs;
            to->seg.x86_64_regs.gs = from->SegGs;
        }
        if (flags & CONTEXT_AMD64_FLOATING_POINT)
        {
            to->flags |= SERVER_CTX_FLOATING_POINT;
            memcpy( to->fp.x86_64_regs.fpregs, &from->FltSave, sizeof(to->fp.x86_64_regs.fpregs) );
        }
        if (flags & CONTEXT_AMD64_DEBUG_REGISTERS)
        {
            to->flags |= SERVER_CTX_DEBUG_REGISTERS;
            to->debug.x86_64_regs.dr0 = from->Dr0;
            to->debug.x86_64_regs.dr1 = from->Dr1;
            to->debug.x86_64_regs.dr2 = from->Dr2;
            to->debug.x86_64_regs.dr3 = from->Dr3;
            to->debug.x86_64_regs.dr6 = from->Dr6;
            to->debug.x86_64_regs.dr7 = from->Dr7;
        }
        if (flags & CONTEXT_AMD64_XSTATE)
            xstate_to_server( to, (const CONTEXT_EX *)(from + 1) );
        exception_request_flags_to_server( to, flags );
        return STATUS_SUCCESS;
    }

    case MAKELONG( IMAGE_FILE_MACHINE_AMD64, IMAGE_FILE_MACHINE_I386 ):
    {
        const AMD64_CONTEXT *from = src;

        flags = from->ContextFlags & ~CONTEXT_AMD64;
        if (flags & CONTEXT_AMD64_CONTROL)
        {
            to->flags |= SERVER_CTX_CONTROL;
            to->ctl.i386_regs.ebp    = from->Rbp;
            to->ctl.i386_regs.eip    = from->Rip;
            to->ctl.i386_regs.esp    = from->Rsp;
            to->ctl.i386_regs.cs     = from->SegCs;
            to->ctl.i386_regs.ss     = from->SegSs;
            to->ctl.i386_regs.eflags = from->EFlags;
        }
        if (flags & CONTEXT_AMD64_INTEGER)
        {
            to->flags |= SERVER_CTX_INTEGER;
            to->integer.i386_regs.eax = from->Rax;
            to->integer.i386_regs.ecx = from->Rcx;
            to->integer.i386_regs.edx = from->Rdx;
            to->integer.i386_regs.ebx = from->Rbx;
            to->integer.i386_regs.esi = from->Rsi;
            to->integer.i386_regs.edi = from->Rdi;
        }
        if (flags & CONTEXT_AMD64_SEGMENTS)
        {
            to->flags |= SERVER_CTX_SEGMENTS;
            to->seg.i386_regs.ds = from->SegDs;
            to->seg.i386_regs.es = from->SegEs;
            to->seg.i386_regs.fs = from->SegFs;
            to->seg.i386_regs.gs = from->SegGs;
        }
        if (flags & CONTEXT_AMD64_FLOATING_POINT)
        {
            I386_FLOATING_SAVE_AREA fpu;

            to->flags |= SERVER_CTX_EXTENDED_REGISTERS | SERVER_CTX_FLOATING_POINT;
            memcpy( to->ext.i386_regs, &from->FltSave, sizeof(to->ext.i386_regs) );
            fpux_to_fpu( &fpu, &from->FltSave );
            to->fp.i386_regs.ctrl     = fpu.ControlWord;
            to->fp.i386_regs.status   = fpu.StatusWord;
            to->fp.i386_regs.tag      = fpu.TagWord;
            to->fp.i386_regs.err_off  = fpu.ErrorOffset;
            to->fp.i386_regs.err_sel  = fpu.ErrorSelector;
            to->fp.i386_regs.data_off = fpu.DataOffset;
            to->fp.i386_regs.data_sel = fpu.DataSelector;
            to->fp.i386_regs.cr0npx   = fpu.Cr0NpxState;
            memcpy( to->fp.i386_regs.regs, fpu.RegisterArea, sizeof(to->fp.i386_regs.regs) );
        }
        if (flags & CONTEXT_AMD64_DEBUG_REGISTERS)
        {
            to->flags |= SERVER_CTX_DEBUG_REGISTERS;
            to->debug.i386_regs.dr0 = from->Dr0;
            to->debug.i386_regs.dr1 = from->Dr1;
            to->debug.i386_regs.dr2 = from->Dr2;
            to->debug.i386_regs.dr3 = from->Dr3;
            to->debug.i386_regs.dr6 = from->Dr6;
            to->debug.i386_regs.dr7 = from->Dr7;
        }
        if (flags & CONTEXT_AMD64_XSTATE)
            xstate_to_server( to, (const CONTEXT_EX *)(from + 1) );
        exception_request_flags_to_server( to, flags );
        return STATUS_SUCCESS;
    }

    case MAKELONG( IMAGE_FILE_MACHINE_ARMNT, IMAGE_FILE_MACHINE_ARMNT ):
    {
        const ARM_CONTEXT *from = src;

        flags = from->ContextFlags & ~CONTEXT_ARM;
        if (flags & CONTEXT_ARM_CONTROL)
        {
            to->flags |= SERVER_CTX_CONTROL;
            to->ctl.arm_regs.sp   = from->Sp;
            to->ctl.arm_regs.lr   = from->Lr;
            to->ctl.arm_regs.pc   = from->Pc;
            to->ctl.arm_regs.cpsr = from->Cpsr;
        }
        if (flags & CONTEXT_ARM_INTEGER)
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
        if (flags & CONTEXT_ARM_FLOATING_POINT)
        {
            to->flags |= SERVER_CTX_FLOATING_POINT;
            for (i = 0; i < 32; i++) to->fp.arm_regs.d[i] = from->D[i];
            to->fp.arm_regs.fpscr = from->Fpscr;
        }
        if (flags & CONTEXT_ARM_DEBUG_REGISTERS)
        {
            to->flags |= SERVER_CTX_DEBUG_REGISTERS;
            for (i = 0; i < ARM_MAX_BREAKPOINTS; i++) to->debug.arm_regs.bvr[i] = from->Bvr[i];
            for (i = 0; i < ARM_MAX_BREAKPOINTS; i++) to->debug.arm_regs.bcr[i] = from->Bcr[i];
            for (i = 0; i < ARM_MAX_WATCHPOINTS; i++) to->debug.arm_regs.wvr[i] = from->Wvr[i];
            for (i = 0; i < ARM_MAX_WATCHPOINTS; i++) to->debug.arm_regs.wcr[i] = from->Wcr[i];
        }
        exception_request_flags_to_server( to, flags );
        return STATUS_SUCCESS;
    }

    case MAKELONG( IMAGE_FILE_MACHINE_ARM64, IMAGE_FILE_MACHINE_ARM64 ):
    {
        const ARM64_NT_CONTEXT *from = src;

        flags = from->ContextFlags & ~CONTEXT_ARM64;
        if (flags & CONTEXT_ARM64_CONTROL)
        {
            to->flags |= SERVER_CTX_CONTROL;
            to->integer.arm64_regs.x[29] = from->Fp;
            to->integer.arm64_regs.x[30] = from->Lr;
            to->ctl.arm64_regs.sp     = from->Sp;
            to->ctl.arm64_regs.pc     = from->Pc;
            to->ctl.arm64_regs.pstate = from->Cpsr;
        }
        if (flags & CONTEXT_ARM64_INTEGER)
        {
            to->flags |= SERVER_CTX_INTEGER;
            for (i = 0; i <= 28; i++) to->integer.arm64_regs.x[i] = from->X[i];
        }
        if (flags & CONTEXT_ARM64_FLOATING_POINT)
        {
            to->flags |= SERVER_CTX_FLOATING_POINT;
            for (i = 0; i < 32; i++)
            {
                to->fp.arm64_regs.q[i].low = from->V[i].Low;
                to->fp.arm64_regs.q[i].high = from->V[i].High;
            }
            to->fp.arm64_regs.fpcr = from->Fpcr;
            to->fp.arm64_regs.fpsr = from->Fpsr;
        }
        if (flags & CONTEXT_ARM64_DEBUG_REGISTERS)
        {
            to->flags |= SERVER_CTX_DEBUG_REGISTERS;
            for (i = 0; i < ARM64_MAX_BREAKPOINTS; i++) to->debug.arm64_regs.bcr[i] = from->Bcr[i];
            for (i = 0; i < ARM64_MAX_BREAKPOINTS; i++) to->debug.arm64_regs.bvr[i] = from->Bvr[i];
            for (i = 0; i < ARM64_MAX_WATCHPOINTS; i++) to->debug.arm64_regs.wcr[i] = from->Wcr[i];
            for (i = 0; i < ARM64_MAX_WATCHPOINTS; i++) to->debug.arm64_regs.wvr[i] = from->Wvr[i];
        }
        exception_request_flags_to_server( to, flags );
        return STATUS_SUCCESS;
    }

    case MAKELONG( IMAGE_FILE_MACHINE_ARM64, IMAGE_FILE_MACHINE_I386 ):
    case MAKELONG( IMAGE_FILE_MACHINE_I386, IMAGE_FILE_MACHINE_ARM64 ):
        return STATUS_SUCCESS;

    default:
        return STATUS_INVALID_PARAMETER;
    }
}


/***********************************************************************
 *           xstate_from_server
 *
 * Copy xstate from the server format.
 */
static void xstate_from_server( CONTEXT_EX *xctx, const struct context_data *from )
{
    XSTATE *xs = (XSTATE *)((char *)xctx + xctx->XState.Offset);
    unsigned int i;

    xs->Mask &= ~(ULONG64)4;

    if (xs->CompactionMask)
    {
        xs->CompactionMask &= ~(UINT64)3;
        if (!(xs->CompactionMask & 4)) return;
    }

    for (i = 0; i < ARRAY_SIZE( from->ymm.regs.ymm_high); i++)
    {
        if (!from->ymm.regs.ymm_high[i].low && !from->ymm.regs.ymm_high[i].high) continue;
        memcpy( &xs->YmmContext, &from->ymm.regs, sizeof(xs->YmmContext) );
        xs->Mask |= 4;
        break;
    }
}


/***********************************************************************
 *           exception_request_flags_from_server
 *
 * Copy exception reporting flags from the server format.
 */
static void exception_request_flags_from_server( DWORD *context_flags, const struct context_data *from )
{
    if (!(*context_flags & CONTEXT_EXCEPTION_REQUEST) || !(from->flags & SERVER_CTX_EXEC_SPACE)) return;
    *context_flags = (*context_flags & ~(CONTEXT_SERVICE_ACTIVE | CONTEXT_EXCEPTION_ACTIVE)) | CONTEXT_EXCEPTION_REPORTING;
    if (from->exec_space.space.space == EXEC_SPACE_SYSCALL)        *context_flags |= CONTEXT_SERVICE_ACTIVE;
    else if (from->exec_space.space.space == EXEC_SPACE_EXCEPTION) *context_flags |= CONTEXT_EXCEPTION_ACTIVE;
}


/***********************************************************************
 *           context_from_server
 *
 * Convert a register context from the server format.
 */
static NTSTATUS context_from_server( void *dst, const struct context_data *from, USHORT machine )
{
    DWORD i, to_flags;

    switch (MAKELONG( from->machine, machine ))
    {
    case MAKELONG( IMAGE_FILE_MACHINE_I386, IMAGE_FILE_MACHINE_I386 ):
    {
        I386_CONTEXT *to = dst;

        to_flags = to->ContextFlags & ~CONTEXT_i386;
        if ((from->flags & SERVER_CTX_CONTROL) && (to_flags & CONTEXT_I386_CONTROL))
        {
            to->ContextFlags |= CONTEXT_I386_CONTROL;
            to->Ebp    = from->ctl.i386_regs.ebp;
            to->Esp    = from->ctl.i386_regs.esp;
            to->Eip    = from->ctl.i386_regs.eip;
            to->SegCs  = from->ctl.i386_regs.cs;
            to->SegSs  = from->ctl.i386_regs.ss;
            to->EFlags = from->ctl.i386_regs.eflags;
        }
        if ((from->flags & SERVER_CTX_INTEGER) && (to_flags & CONTEXT_I386_INTEGER))
        {
            to->ContextFlags |= CONTEXT_I386_INTEGER;
            to->Eax = from->integer.i386_regs.eax;
            to->Ebx = from->integer.i386_regs.ebx;
            to->Ecx = from->integer.i386_regs.ecx;
            to->Edx = from->integer.i386_regs.edx;
            to->Esi = from->integer.i386_regs.esi;
            to->Edi = from->integer.i386_regs.edi;
        }
        if ((from->flags & SERVER_CTX_SEGMENTS) && (to_flags & CONTEXT_I386_SEGMENTS))
        {
            to->ContextFlags |= CONTEXT_I386_SEGMENTS;
            to->SegDs = from->seg.i386_regs.ds;
            to->SegEs = from->seg.i386_regs.es;
            to->SegFs = from->seg.i386_regs.fs;
            to->SegGs = from->seg.i386_regs.gs;
        }
        if ((from->flags & SERVER_CTX_FLOATING_POINT) && (to_flags & CONTEXT_I386_FLOATING_POINT))
        {
            to->ContextFlags |= CONTEXT_I386_FLOATING_POINT;
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
        if ((from->flags & SERVER_CTX_DEBUG_REGISTERS) && (to_flags & CONTEXT_I386_DEBUG_REGISTERS))
        {
            to->ContextFlags |= CONTEXT_I386_DEBUG_REGISTERS;
            to->Dr0 = from->debug.i386_regs.dr0;
            to->Dr1 = from->debug.i386_regs.dr1;
            to->Dr2 = from->debug.i386_regs.dr2;
            to->Dr3 = from->debug.i386_regs.dr3;
            to->Dr6 = from->debug.i386_regs.dr6;
            to->Dr7 = from->debug.i386_regs.dr7;
        }
        if ((from->flags & SERVER_CTX_EXTENDED_REGISTERS) && (to_flags & CONTEXT_I386_EXTENDED_REGISTERS))
        {
            to->ContextFlags |= CONTEXT_I386_EXTENDED_REGISTERS;
            memcpy( to->ExtendedRegisters, from->ext.i386_regs, sizeof(to->ExtendedRegisters) );
        }
        if ((from->flags & SERVER_CTX_YMM_REGISTERS) && (to_flags & CONTEXT_I386_XSTATE))
            xstate_from_server( (CONTEXT_EX *)(to + 1), from );
        exception_request_flags_from_server( &to->ContextFlags, from );
        return STATUS_SUCCESS;
    }

    case MAKELONG( IMAGE_FILE_MACHINE_AMD64, IMAGE_FILE_MACHINE_I386 ):
    {
        I386_CONTEXT *to = dst;

        to_flags = to->ContextFlags & ~CONTEXT_i386;
        if ((from->flags & SERVER_CTX_CONTROL) && (to_flags & CONTEXT_I386_CONTROL))
        {
            to->ContextFlags |= CONTEXT_I386_CONTROL;
            to->Ebp    = from->ctl.x86_64_regs.rbp;
            to->Esp    = from->ctl.x86_64_regs.rsp;
            to->Eip    = from->ctl.x86_64_regs.rip;
            to->SegCs  = from->ctl.x86_64_regs.cs;
            to->SegSs  = from->ctl.x86_64_regs.ss;
            to->EFlags = from->ctl.x86_64_regs.flags;
        }
        if ((from->flags & SERVER_CTX_INTEGER) && (to_flags & CONTEXT_I386_INTEGER))
        {
            to->ContextFlags |= CONTEXT_I386_INTEGER;
            to->Eax = from->integer.x86_64_regs.rax;
            to->Ebx = from->integer.x86_64_regs.rbx;
            to->Ecx = from->integer.x86_64_regs.rcx;
            to->Edx = from->integer.x86_64_regs.rdx;
            to->Esi = from->integer.x86_64_regs.rsi;
            to->Edi = from->integer.x86_64_regs.rdi;
        }
        if ((from->flags & SERVER_CTX_SEGMENTS) && (to_flags & CONTEXT_I386_SEGMENTS))
        {
            to->ContextFlags |= CONTEXT_I386_SEGMENTS;
            to->SegDs = from->seg.x86_64_regs.ds;
            to->SegEs = from->seg.x86_64_regs.es;
            to->SegFs = from->seg.x86_64_regs.fs;
            to->SegGs = from->seg.x86_64_regs.gs;
        }
        if (from->flags & SERVER_CTX_FLOATING_POINT)
        {
            if (to_flags & CONTEXT_I386_EXTENDED_REGISTERS)
            {
                to->ContextFlags |= CONTEXT_I386_EXTENDED_REGISTERS;
                memcpy( to->ExtendedRegisters, from->fp.x86_64_regs.fpregs, sizeof(to->ExtendedRegisters) );
            }
            if (to_flags & CONTEXT_I386_FLOATING_POINT)
            {
                to->ContextFlags |= CONTEXT_I386_FLOATING_POINT;
                fpux_to_fpu( &to->FloatSave, (XMM_SAVE_AREA32 *)from->fp.x86_64_regs.fpregs );
            }
        }
        if ((from->flags & SERVER_CTX_DEBUG_REGISTERS) && (to_flags & CONTEXT_I386_DEBUG_REGISTERS))
        {
            to->ContextFlags |= CONTEXT_I386_DEBUG_REGISTERS;
            to->Dr0 = from->debug.x86_64_regs.dr0;
            to->Dr1 = from->debug.x86_64_regs.dr1;
            to->Dr2 = from->debug.x86_64_regs.dr2;
            to->Dr3 = from->debug.x86_64_regs.dr3;
            to->Dr6 = from->debug.x86_64_regs.dr6;
            to->Dr7 = from->debug.x86_64_regs.dr7;
        }
        if ((from->flags & SERVER_CTX_YMM_REGISTERS) && (to_flags & CONTEXT_I386_XSTATE))
            xstate_from_server( (CONTEXT_EX *)(to + 1), from );
        exception_request_flags_from_server( &to->ContextFlags, from );
        return STATUS_SUCCESS;
    }

    case MAKELONG( IMAGE_FILE_MACHINE_AMD64, IMAGE_FILE_MACHINE_AMD64 ):
    {
        AMD64_CONTEXT *to = dst;

        to_flags = to->ContextFlags & ~CONTEXT_AMD64;
        if ((from->flags & SERVER_CTX_CONTROL) && (to_flags & CONTEXT_AMD64_CONTROL))
        {
            to->ContextFlags |= CONTEXT_AMD64_CONTROL;
            to->Rbp    = from->ctl.x86_64_regs.rbp;
            to->Rip    = from->ctl.x86_64_regs.rip;
            to->Rsp    = from->ctl.x86_64_regs.rsp;
            to->SegCs  = from->ctl.x86_64_regs.cs;
            to->SegSs  = from->ctl.x86_64_regs.ss;
            to->EFlags = from->ctl.x86_64_regs.flags;
        }
        if ((from->flags & SERVER_CTX_INTEGER) && (to_flags & CONTEXT_AMD64_INTEGER))
        {
            to->ContextFlags |= CONTEXT_AMD64_INTEGER;
            to->Rax = from->integer.x86_64_regs.rax;
            to->Rcx = from->integer.x86_64_regs.rcx;
            to->Rdx = from->integer.x86_64_regs.rdx;
            to->Rbx = from->integer.x86_64_regs.rbx;
            to->Rsi = from->integer.x86_64_regs.rsi;
            to->Rdi = from->integer.x86_64_regs.rdi;
            to->R8  = from->integer.x86_64_regs.r8;
            to->R9  = from->integer.x86_64_regs.r9;
            to->R10 = from->integer.x86_64_regs.r10;
            to->R11 = from->integer.x86_64_regs.r11;
            to->R12 = from->integer.x86_64_regs.r12;
            to->R13 = from->integer.x86_64_regs.r13;
            to->R14 = from->integer.x86_64_regs.r14;
            to->R15 = from->integer.x86_64_regs.r15;
        }
        if ((from->flags & SERVER_CTX_SEGMENTS) && (to_flags & CONTEXT_AMD64_SEGMENTS))
        {
            to->ContextFlags |= CONTEXT_AMD64_SEGMENTS;
            to->SegDs = from->seg.x86_64_regs.ds;
            to->SegEs = from->seg.x86_64_regs.es;
            to->SegFs = from->seg.x86_64_regs.fs;
            to->SegGs = from->seg.x86_64_regs.gs;
        }
        if ((from->flags & SERVER_CTX_FLOATING_POINT) && (to_flags & CONTEXT_AMD64_FLOATING_POINT))
        {
            to->ContextFlags |= CONTEXT_AMD64_FLOATING_POINT;
            memcpy( &to->FltSave, from->fp.x86_64_regs.fpregs, sizeof(from->fp.x86_64_regs.fpregs) );
            to->MxCsr = to->FltSave.MxCsr;
        }
        if ((from->flags & SERVER_CTX_DEBUG_REGISTERS) && (to_flags & CONTEXT_AMD64_DEBUG_REGISTERS))
        {
            to->ContextFlags |= CONTEXT_AMD64_DEBUG_REGISTERS;
            to->Dr0 = from->debug.x86_64_regs.dr0;
            to->Dr1 = from->debug.x86_64_regs.dr1;
            to->Dr2 = from->debug.x86_64_regs.dr2;
            to->Dr3 = from->debug.x86_64_regs.dr3;
            to->Dr6 = from->debug.x86_64_regs.dr6;
            to->Dr7 = from->debug.x86_64_regs.dr7;
        }
        if ((from->flags & SERVER_CTX_YMM_REGISTERS) && (to_flags & CONTEXT_AMD64_XSTATE))
            xstate_from_server( (CONTEXT_EX *)(to + 1), from );
        exception_request_flags_from_server( &to->ContextFlags, from );
        return STATUS_SUCCESS;
    }

    case MAKELONG( IMAGE_FILE_MACHINE_I386, IMAGE_FILE_MACHINE_AMD64 ):
    {
        AMD64_CONTEXT *to = dst;

        to_flags = to->ContextFlags & ~CONTEXT_AMD64;
        if ((from->flags & SERVER_CTX_CONTROL) && (to_flags & CONTEXT_AMD64_CONTROL))
        {
            to->ContextFlags |= CONTEXT_AMD64_CONTROL;
            to->Rbp    = from->ctl.i386_regs.ebp;
            to->Rip    = from->ctl.i386_regs.eip;
            to->Rsp    = from->ctl.i386_regs.esp;
            to->SegCs  = from->ctl.i386_regs.cs;
            to->SegSs  = from->ctl.i386_regs.ss;
            to->EFlags = from->ctl.i386_regs.eflags;
        }
        if ((from->flags & SERVER_CTX_INTEGER) && (to_flags & CONTEXT_AMD64_INTEGER))
        {
            to->ContextFlags |= CONTEXT_AMD64_INTEGER;
            to->Rax = from->integer.i386_regs.eax;
            to->Rcx = from->integer.i386_regs.ecx;
            to->Rdx = from->integer.i386_regs.edx;
            to->Rbx = from->integer.i386_regs.ebx;
            to->Rsi = from->integer.i386_regs.esi;
            to->Rdi = from->integer.i386_regs.edi;
        }
        if ((from->flags & SERVER_CTX_SEGMENTS) && (to_flags & CONTEXT_AMD64_SEGMENTS))
        {
            to->ContextFlags |= CONTEXT_AMD64_SEGMENTS;
            to->SegDs = from->seg.i386_regs.ds;
            to->SegEs = from->seg.i386_regs.es;
            to->SegFs = from->seg.i386_regs.fs;
            to->SegGs = from->seg.i386_regs.gs;
        }
        if ((from->flags & SERVER_CTX_EXTENDED_REGISTERS) && (to_flags & CONTEXT_AMD64_FLOATING_POINT))
        {
            to->ContextFlags |= CONTEXT_AMD64_FLOATING_POINT;
            memcpy( &to->FltSave, from->ext.i386_regs, sizeof(to->FltSave) );
        }
        else if ((from->flags & SERVER_CTX_FLOATING_POINT) && (to_flags & CONTEXT_AMD64_FLOATING_POINT))
        {
            I386_FLOATING_SAVE_AREA fpu;

            to->ContextFlags |= CONTEXT_AMD64_FLOATING_POINT;
            fpu.ControlWord   = from->fp.i386_regs.ctrl;
            fpu.StatusWord    = from->fp.i386_regs.status;
            fpu.TagWord       = from->fp.i386_regs.tag;
            fpu.ErrorOffset   = from->fp.i386_regs.err_off;
            fpu.ErrorSelector = from->fp.i386_regs.err_sel;
            fpu.DataOffset    = from->fp.i386_regs.data_off;
            fpu.DataSelector  = from->fp.i386_regs.data_sel;
            fpu.Cr0NpxState   = from->fp.i386_regs.cr0npx;
            memcpy( fpu.RegisterArea, from->fp.i386_regs.regs, sizeof(fpu.RegisterArea) );
            fpu_to_fpux( &to->FltSave, &fpu );
        }
        if ((from->flags & SERVER_CTX_DEBUG_REGISTERS) && (to_flags & CONTEXT_AMD64_DEBUG_REGISTERS))
        {
            to->ContextFlags |= CONTEXT_AMD64_DEBUG_REGISTERS;
            to->Dr0 = from->debug.i386_regs.dr0;
            to->Dr1 = from->debug.i386_regs.dr1;
            to->Dr2 = from->debug.i386_regs.dr2;
            to->Dr3 = from->debug.i386_regs.dr3;
            to->Dr6 = from->debug.i386_regs.dr6;
            to->Dr7 = from->debug.i386_regs.dr7;
        }
        if ((from->flags & SERVER_CTX_YMM_REGISTERS) && (to_flags & CONTEXT_AMD64_XSTATE))
            xstate_from_server( (CONTEXT_EX *)(to + 1), from );
        exception_request_flags_from_server( &to->ContextFlags, from );
        return STATUS_SUCCESS;
    }

    case MAKELONG( IMAGE_FILE_MACHINE_ARMNT, IMAGE_FILE_MACHINE_ARMNT ):
    {
        ARM_CONTEXT *to = dst;

        to_flags = to->ContextFlags & ~CONTEXT_ARM;
        if ((from->flags & SERVER_CTX_CONTROL) && (to_flags & CONTEXT_ARM_CONTROL))
        {
            to->ContextFlags |= CONTEXT_ARM_CONTROL;
            to->Sp   = from->ctl.arm_regs.sp;
            to->Lr   = from->ctl.arm_regs.lr;
            to->Pc   = from->ctl.arm_regs.pc;
            to->Cpsr = from->ctl.arm_regs.cpsr;
        }
        if ((from->flags & SERVER_CTX_INTEGER) && (to_flags & CONTEXT_ARM_INTEGER))
        {
            to->ContextFlags |= CONTEXT_ARM_INTEGER;
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
        if ((from->flags & SERVER_CTX_FLOATING_POINT) && (to_flags & CONTEXT_ARM_FLOATING_POINT))
        {
            to->ContextFlags |= CONTEXT_ARM_FLOATING_POINT;
            for (i = 0; i < 32; i++) to->D[i] = from->fp.arm_regs.d[i];
            to->Fpscr = from->fp.arm_regs.fpscr;
        }
        if ((from->flags & SERVER_CTX_DEBUG_REGISTERS) && (to_flags & CONTEXT_ARM_DEBUG_REGISTERS))
        {
            to->ContextFlags |= CONTEXT_ARM_DEBUG_REGISTERS;
            for (i = 0; i < ARM_MAX_BREAKPOINTS; i++) to->Bvr[i] = from->debug.arm_regs.bvr[i];
            for (i = 0; i < ARM_MAX_BREAKPOINTS; i++) to->Bcr[i] = from->debug.arm_regs.bcr[i];
            for (i = 0; i < ARM_MAX_WATCHPOINTS; i++) to->Wvr[i] = from->debug.arm_regs.wvr[i];
            for (i = 0; i < ARM_MAX_WATCHPOINTS; i++) to->Wcr[i] = from->debug.arm_regs.wcr[i];
        }
        exception_request_flags_from_server( &to->ContextFlags, from );
        return STATUS_SUCCESS;
    }

    case MAKELONG( IMAGE_FILE_MACHINE_ARM64, IMAGE_FILE_MACHINE_ARM64 ):
    {
        ARM64_NT_CONTEXT *to = dst;

        to_flags = to->ContextFlags & ~CONTEXT_ARM64;
        if ((from->flags & SERVER_CTX_CONTROL) && (to_flags & CONTEXT_ARM64_CONTROL))
        {
            to->ContextFlags |= CONTEXT_ARM64_CONTROL;
            to->Fp   = from->integer.arm64_regs.x[29];
            to->Lr   = from->integer.arm64_regs.x[30];
            to->Sp   = from->ctl.arm64_regs.sp;
            to->Pc   = from->ctl.arm64_regs.pc;
            to->Cpsr = from->ctl.arm64_regs.pstate;
        }
        if ((from->flags & SERVER_CTX_INTEGER) && (to_flags & CONTEXT_ARM64_INTEGER))
        {
            to->ContextFlags |= CONTEXT_ARM64_INTEGER;
            for (i = 0; i <= 28; i++) to->X[i] = from->integer.arm64_regs.x[i];
        }
        if ((from->flags & SERVER_CTX_FLOATING_POINT) && (to_flags & CONTEXT_ARM64_FLOATING_POINT))
        {
            to->ContextFlags |= CONTEXT_ARM64_FLOATING_POINT;
            for (i = 0; i < 32; i++)
            {
                to->V[i].Low = from->fp.arm64_regs.q[i].low;
                to->V[i].High = from->fp.arm64_regs.q[i].high;
            }
            to->Fpcr = from->fp.arm64_regs.fpcr;
            to->Fpsr = from->fp.arm64_regs.fpsr;
        }
        if ((from->flags & SERVER_CTX_DEBUG_REGISTERS) && (to_flags & CONTEXT_ARM64_DEBUG_REGISTERS))
        {
            to->ContextFlags |= CONTEXT_ARM64_DEBUG_REGISTERS;
            for (i = 0; i < ARM64_MAX_BREAKPOINTS; i++) to->Bcr[i] = from->debug.arm64_regs.bcr[i];
            for (i = 0; i < ARM64_MAX_BREAKPOINTS; i++) to->Bvr[i] = from->debug.arm64_regs.bvr[i];
            for (i = 0; i < ARM64_MAX_WATCHPOINTS; i++) to->Wcr[i] = from->debug.arm64_regs.wcr[i];
            for (i = 0; i < ARM64_MAX_WATCHPOINTS; i++) to->Wvr[i] = from->debug.arm64_regs.wvr[i];
        }
        exception_request_flags_from_server( &to->ContextFlags, from );
        return STATUS_SUCCESS;
    }

    case MAKELONG( IMAGE_FILE_MACHINE_ARM64, IMAGE_FILE_MACHINE_I386 ):
    case MAKELONG( IMAGE_FILE_MACHINE_I386, IMAGE_FILE_MACHINE_ARM64 ):
        return STATUS_SUCCESS;

    default:
        return STATUS_INVALID_PARAMETER;
    }
}


/***********************************************************************
 *           contexts_to_server
 */
static void contexts_to_server( struct context_data server_contexts[2], CONTEXT *context )
{
    unsigned int count = 0;
    void *native_context = get_native_context( context );
    void *wow_context = get_wow_context( context );

    if (native_context)
    {
        context_to_server( &server_contexts[count++], native_machine, native_context, native_machine );
        if (wow_context) context_to_server( &server_contexts[count++], main_image_info.Machine,
                                            wow_context, main_image_info.Machine );
        else if (native_machine != main_image_info.Machine)
            context_to_server( &server_contexts[count++], main_image_info.Machine,
                               native_context, native_machine );
    }
    else
        context_to_server( &server_contexts[count++], native_machine,
                           wow_context, main_image_info.Machine );

    if (count < 2) memset( &server_contexts[1], 0, sizeof(server_contexts[1]) );
}


/***********************************************************************
 *           contexts_from_server
 */
static void contexts_from_server( CONTEXT *context, struct context_data server_contexts[2] )
{
    void *native_context = get_native_context( context );
    void *wow_context = get_wow_context( context );

    if (native_context)
    {
        context_from_server( native_context, &server_contexts[0], native_machine );
        if (wow_context)
            context_from_server( wow_context, &server_contexts[1], main_image_info.Machine );
        else
            context_from_server( native_context, &server_contexts[1], native_machine );
    }
    else context_from_server( wow_context, &server_contexts[0], main_image_info.Machine );
}


/***********************************************************************
 *           pthread_exit_wrapper
 */
static DECLSPEC_NORETURN void pthread_exit_wrapper( int status )
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
    BOOL suspend;

    thread_data->pthread_id = pthread_self();
    pthread_setspecific( teb_key, teb );
    server_init_thread( thread_data->start, &suspend );
    signal_start_thread( thread_data->start, thread_data->param, suspend, teb );
}


/***********************************************************************
 *           get_machine_context_size
 */
static SIZE_T get_machine_context_size( USHORT machine )
{
    switch (machine)
    {
    case IMAGE_FILE_MACHINE_I386:  return sizeof(I386_CONTEXT);
    case IMAGE_FILE_MACHINE_ARMNT: return sizeof(ARM_CONTEXT);
    case IMAGE_FILE_MACHINE_AMD64: return sizeof(AMD64_CONTEXT);
    case IMAGE_FILE_MACHINE_ARM64: return sizeof(ARM64_NT_CONTEXT);
    default: return 0;
    }
}


/***********************************************************************
 *           get_cpu_area
 *
 * cf. RtlWow64GetCurrentCpuArea
 */
void *get_cpu_area( USHORT machine )
{
    WOW64_CPURESERVED *cpu;
    ULONG align;

    if (!is_wow64()) return NULL;
#ifdef _WIN64
    cpu = NtCurrentTeb()->TlsSlots[WOW64_TLS_CPURESERVED];
#else
    cpu = ULongToPtr( NtCurrentTeb64()->TlsSlots[WOW64_TLS_CPURESERVED] );
#endif
    if (cpu->Machine != machine) return NULL;
    switch (cpu->Machine)
    {
    case IMAGE_FILE_MACHINE_I386: align = TYPE_ALIGNMENT(I386_CONTEXT); break;
    case IMAGE_FILE_MACHINE_AMD64: align = TYPE_ALIGNMENT(AMD64_CONTEXT); break;
    case IMAGE_FILE_MACHINE_ARMNT: align = TYPE_ALIGNMENT(ARM_CONTEXT); break;
    case IMAGE_FILE_MACHINE_ARM64: align = TYPE_ALIGNMENT(ARM64_NT_CONTEXT); break;
    default: return NULL;
    }
    return (void *)(((ULONG_PTR)(cpu + 1) + align - 1) & ~((ULONG_PTR)align - 1));
}


/***********************************************************************
 *           set_thread_id
 */
void set_thread_id( TEB *teb, DWORD pid, DWORD tid )
{
    WOW_TEB *wow_teb = get_wow_teb( teb );

    teb->ClientId.UniqueProcess = ULongToHandle( pid );
    teb->ClientId.UniqueThread  = ULongToHandle( tid );
    teb->RealClientId = teb->ClientId;
    if (wow_teb)
    {
        wow_teb->ClientId.UniqueProcess = pid;
        wow_teb->ClientId.UniqueThread  = tid;
        wow_teb->RealClientId = wow_teb->ClientId;
    }
}


/***********************************************************************
 *           init_thread_stack
 */
NTSTATUS init_thread_stack( TEB *teb, ULONG_PTR limit, SIZE_T reserve_size, SIZE_T commit_size )
{
    struct ntdll_thread_data *thread_data = (struct ntdll_thread_data *)&teb->GdiTebBatch;
    WOW_TEB *wow_teb = get_wow_teb( teb );
    INITIAL_TEB stack;
    NTSTATUS status;

    /* kernel stack */
    if ((status = virtual_alloc_thread_stack( &stack, limit_4g, 0, kernel_stack_size, kernel_stack_size, FALSE )))
        return status;
    thread_data->kernel_stack = stack.DeallocationStack;

    if (wow_teb)
    {
        WOW64_CPURESERVED *cpu;
        SIZE_T cpusize = sizeof(WOW64_CPURESERVED) +
            ((get_machine_context_size( main_image_info.Machine ) + 7) & ~7) + sizeof(ULONG64);

        /* 64-bit stack */
        if ((status = virtual_alloc_thread_stack( &stack, limit_4g, 0, 0x40000, 0x40000, TRUE ))) return status;
        cpu = (WOW64_CPURESERVED *)(((ULONG_PTR)stack.StackBase - cpusize) & ~15);
        cpu->Machine = main_image_info.Machine;

#ifdef _WIN64
        teb->Tib.StackBase = teb->TlsSlots[WOW64_TLS_CPURESERVED] = cpu;
        teb->Tib.StackLimit = stack.StackLimit;
        teb->DeallocationStack = stack.DeallocationStack;

        /* 32-bit stack */
        if (!limit || limit > user_space_wow_limit) limit = user_space_wow_limit;
        if ((status = virtual_alloc_thread_stack( &stack, 0, limit, reserve_size, commit_size, TRUE )))
            return status;
        wow_teb->Tib.StackBase = PtrToUlong( stack.StackBase );
        wow_teb->Tib.StackLimit = PtrToUlong( stack.StackLimit );
        wow_teb->DeallocationStack = PtrToUlong( stack.DeallocationStack );
        return STATUS_SUCCESS;
#else
        wow_teb->Tib.StackBase = wow_teb->TlsSlots[WOW64_TLS_CPURESERVED] = PtrToUlong( cpu );
        wow_teb->Tib.StackLimit = PtrToUlong( stack.StackLimit );
        wow_teb->DeallocationStack = PtrToUlong( stack.DeallocationStack );
#endif
    }

#ifdef __aarch64__
    if (is_arm64ec())
    {
        CHPE_V2_CPU_AREA_INFO *cpu_area;
        const SIZE_T chpev2_stack_size = 0x40000;

        /* emulator stack */
        if ((status = virtual_alloc_thread_stack( &stack, limit_4g, 0, chpev2_stack_size, chpev2_stack_size, FALSE )))
            return status;

        cpu_area = stack.DeallocationStack;
        cpu_area->ContextAmd64 = (ARM64EC_NT_CONTEXT *)&cpu_area->EmulatorDataInline;
        cpu_area->EmulatorStackBase  = (ULONG_PTR)stack.StackBase;
        cpu_area->EmulatorStackLimit = (ULONG_PTR)stack.StackLimit + page_size;
        teb->ChpeV2CpuAreaInfo = cpu_area;
    }
#endif

    /* native stack */
    if ((status = virtual_alloc_thread_stack( &stack, 0, limit, reserve_size, commit_size, TRUE )))
        return status;
    teb->Tib.StackBase = stack.StackBase;
    teb->Tib.StackLimit = stack.StackLimit;
    teb->DeallocationStack = stack.DeallocationStack;
    return STATUS_SUCCESS;
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
 *              NtCreateThread   (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateThread( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                HANDLE process, CLIENT_ID *id, CONTEXT *ctx, INITIAL_TEB *teb,
                                BOOLEAN suspended )
{
    FIXME( "%p %d %p %p %p %p %p %d, stub!\n",
           handle, (int)access, attr, process, id, ctx, teb, suspended );
    return STATUS_NOT_IMPLEMENTED;
}

/***********************************************************************
 *              NtCreateThreadEx   (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateThreadEx( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                  HANDLE process, PRTL_THREAD_START_ROUTINE start, void *param,
                                  ULONG flags, ULONG_PTR zero_bits, SIZE_T stack_commit,
                                  SIZE_T stack_reserve, PS_ATTRIBUTE_LIST *attr_list )
{
    static const ULONG supported_flags = THREAD_CREATE_FLAGS_CREATE_SUSPENDED | THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER;
    sigset_t sigset;
    pthread_t pthread_id;
    pthread_attr_t pthread_attr;
    data_size_t len;
    struct object_attributes *objattr;
    struct ntdll_thread_data *thread_data;
    DWORD tid = 0;
    int request_pipe[2];
    TEB *teb;
    unsigned int status;

    if (flags & ~supported_flags)
        FIXME( "Unsupported flags %#x.\n", (int)flags );

    if (zero_bits > 21 && zero_bits < 32) return STATUS_INVALID_PARAMETER_3;
#ifndef _WIN64
    if (!is_old_wow64() && zero_bits >= 32) return STATUS_INVALID_PARAMETER_3;
#endif

    if (process != NtCurrentProcess())
    {
        union apc_call call;
        union apc_result result;

        memset( &call, 0, sizeof(call) );

        call.create_thread.type      = APC_CREATE_THREAD;
        call.create_thread.flags     = flags;
        call.create_thread.func      = wine_server_client_ptr( start );
        call.create_thread.arg       = wine_server_client_ptr( param );
        call.create_thread.zero_bits = zero_bits;
        call.create_thread.reserve   = stack_reserve;
        call.create_thread.commit    = stack_commit;
        status = server_queue_process_apc( process, &call, &result );
        if (status != STATUS_SUCCESS) return status;

        if (result.create_thread.status == STATUS_SUCCESS)
        {
            CLIENT_ID client_id;
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
        req->flags      = flags;
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

    if ((status = init_thread_stack( teb, get_zero_bits_limit( zero_bits ), stack_reserve, stack_commit )))
    {
        virtual_free_teb( teb );
        goto done;
    }

    set_thread_id( teb, GetCurrentProcessId(), tid );

    thread_data = (struct ntdll_thread_data *)&teb->GdiTebBatch;
    thread_data->request_fd  = request_pipe[1];
    thread_data->start = start;
    thread_data->param = param;

    pthread_attr_init( &pthread_attr );
    pthread_attr_setstack( &pthread_attr, thread_data->kernel_stack, kernel_stack_size );
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
    if (attr_list) update_attr_list( attr_list, &teb->ClientId, teb );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           abort_thread
 */
void abort_thread( int status )
{
    pthread_sigmask( SIG_BLOCK, &server_block_set, NULL );
    if (InterlockedDecrement( &nb_threads ) <= 0) abort_process( status );
    pthread_exit_wrapper( status );
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
static DECLSPEC_NORETURN void exit_thread( int status )
{
    static void *prev_teb;
    TEB *teb;

    pthread_sigmask( SIG_BLOCK, &server_block_set, NULL );

    if (InterlockedDecrement( &nb_threads ) <= 0) exit_process( status );

    if ((teb = InterlockedExchangePointer( &prev_teb, NtCurrentTeb() )))
    {
        struct ntdll_thread_data *thread_data = (struct ntdll_thread_data *)&teb->GdiTebBatch;

        if (thread_data->pthread_id)
        {
            pthread_join( thread_data->pthread_id, NULL );
            virtual_free_teb( teb );
        }
    }
    pthread_exit_wrapper( status );
}


/***********************************************************************
 *           exit_process
 */
void exit_process( int status )
{
    pthread_sigmask( SIG_BLOCK, &server_block_set, NULL );
    process_exit_wrapper( get_unix_exit_code( status ));
}


/**********************************************************************
 *           wait_suspend
 *
 * Wait until the thread is no longer suspended.
 */
void wait_suspend( CONTEXT *context )
{
    int saved_errno = errno;
    struct context_data server_contexts[2];

    contexts_to_server( server_contexts, context );
    /* wait with 0 timeout, will only return once the thread is no longer suspended */
    server_select( NULL, 0, SELECT_INTERRUPTIBLE, 0, server_contexts, NULL );
    contexts_from_server( context, server_contexts );
    errno = saved_errno;
}


/**********************************************************************
 *           send_debug_event
 *
 * Send an EXCEPTION_DEBUG_EVENT event to the debugger.
 */
NTSTATUS send_debug_event( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance, BOOL exception )
{
    unsigned int ret;
    DWORD i;
    obj_handle_t handle = 0;
    client_ptr_t params[EXCEPTION_MAXIMUM_PARAMETERS];
    union select_op select_op;
    sigset_t old_set;

    if (!peb->BeingDebugged) return 0;  /* no debugger present */

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
        struct context_data server_contexts[2];

        select_op.wait.op = SELECT_WAIT;
        select_op.wait.handles[0] = handle;

        contexts_to_server( server_contexts, context );
        server_contexts[0].flags |= SERVER_CTX_EXEC_SPACE;
        server_contexts[0].exec_space.space.space = exception ? EXEC_SPACE_EXCEPTION : EXEC_SPACE_SYSCALL;
        server_select( &select_op, offsetof( union select_op, wait.handles[1] ), SELECT_INTERRUPTIBLE,
                       TIMEOUT_INFINITE, server_contexts, NULL );

        SERVER_START_REQ( get_exception_status )
        {
            req->handle = handle;
            ret = wine_server_call( req );
        }
        SERVER_END_REQ;
        if (NT_SUCCESS(ret)) contexts_from_server( context, server_contexts );
    }

    pthread_sigmask( SIG_SETMASK, &old_set, NULL );
    return ret;
}


/*******************************************************************
 *		NtRaiseException (NTDLL.@)
 */
NTSTATUS WINAPI NtRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    NTSTATUS status = send_debug_event( rec, context, first_chance, !(is_win64 || is_wow64() || is_old_wow64()) );

    if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
        return NtContinue( context, FALSE );

    if (first_chance) return call_user_exception_dispatcher( rec, context );

    if (rec->ExceptionFlags & EXCEPTION_STACK_INVALID)
        ERR_(seh)("Exception frame is not in stack limits => unable to dispatch exception.\n");
    else if (rec->ExceptionCode == STATUS_NONCONTINUABLE_EXCEPTION)
        ERR_(seh)("Process attempted to continue execution after noncontinuable exception.\n");
    else
        ERR_(seh)("Unhandled exception code %x flags %x addr %p\n",
                  (int)rec->ExceptionCode, (int)rec->ExceptionFlags, rec->ExceptionAddress );

    NtTerminateProcess( NtCurrentProcess(), rec->ExceptionCode );
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           NtCurrentTeb   (NTDLL.@)
 */
TEB * WINAPI NtCurrentTeb(void)
{
    return pthread_getspecific( teb_key );
}


/***********************************************************************
 *              NtOpenThread   (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenThread( HANDLE *handle, ACCESS_MASK access,
                              const OBJECT_ATTRIBUTES *attr, const CLIENT_ID *id )
{
    unsigned int ret;

    *handle = 0;

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
    unsigned int ret;

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
    unsigned int ret;

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
    unsigned int ret;
    BOOL self;

    SERVER_START_REQ( terminate_thread )
    {
        req->handle    = wine_server_obj_handle( handle );
        req->exit_code = exit_code;
        ret = wine_server_call( req );
        self = !ret && reply->self;
    }
    SERVER_END_REQ;

    if (self)
    {
        server_select( NULL, 0, SELECT_INTERRUPTIBLE, 0, NULL, NULL );
        exit_thread( exit_code );
    }
    return ret;
}


/******************************************************************************
 *              NtQueueApcThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueueApcThread( HANDLE handle, PNTAPCFUNC func, ULONG_PTR arg1,
                                  ULONG_PTR arg2, ULONG_PTR arg3 )
{
    unsigned int ret;
    union apc_call call;

    SERVER_START_REQ( queue_apc )
    {
        req->handle = wine_server_obj_handle( handle );
        if (func)
        {
            call.type         = APC_USER;
            call.user.func    = wine_server_client_ptr( func );
            call.user.args[0] = arg1;
            call.user.args[1] = arg2;
            call.user.args[2] = arg3;
            wine_server_add_data( req, &call, sizeof(call) );
        }
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtQueueApcThreadEx  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueueApcThreadEx( HANDLE handle, HANDLE reserve_handle, PNTAPCFUNC func,
                                    ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3 )
{
    FIXME( "reserve handle should be used: %p\n", reserve_handle );
    return NtQueueApcThread( handle, func, arg1, arg2, arg3 );
}


/***********************************************************************
 *              set_thread_context
 */
NTSTATUS set_thread_context( HANDLE handle, const void *context, BOOL *self, USHORT machine )
{
    struct context_data server_contexts[2];
    unsigned int count = 0;
    unsigned int ret;

    context_to_server( &server_contexts[count++], native_machine, context, machine );
    if (machine != native_machine)
        context_to_server( &server_contexts[count++], machine, context, machine );

    SERVER_START_REQ( set_thread_context )
    {
        req->handle  = wine_server_obj_handle( handle );
        req->native_flags = server_contexts[0].flags & get_native_context_flags( native_machine, machine );
        wine_server_add_data( req, server_contexts, count * sizeof(server_contexts[0]) );
        ret = wine_server_call( req );
        *self = reply->self;
    }
    SERVER_END_REQ;

    return ret;
}


/***********************************************************************
 *              get_thread_context
 */
NTSTATUS get_thread_context( HANDLE handle, void *context, BOOL *self, USHORT machine )
{
    unsigned int ret;
    HANDLE context_handle;
    struct context_data server_contexts[2];
    unsigned int count;
    unsigned int flags = get_server_context_flags( context, machine );

    SERVER_START_REQ( get_thread_context )
    {
        req->handle  = wine_server_obj_handle( handle );
        req->flags   = flags;
        req->machine = machine;
        req->native_flags = flags & get_native_context_flags( native_machine, machine );
        wine_server_set_reply( req, server_contexts, sizeof(server_contexts) );
        ret = wine_server_call( req );
        *self = reply->self;
        context_handle = wine_server_ptr_handle( reply->handle );
        count = wine_server_reply_size( reply ) / sizeof(server_contexts[0]);
    }
    SERVER_END_REQ;

    if (ret == STATUS_PENDING)
    {
        NtWaitForSingleObject( context_handle, FALSE, NULL );

        SERVER_START_REQ( get_thread_context )
        {
            req->context = wine_server_obj_handle( context_handle );
            req->flags   = flags;
            req->machine = machine;
            req->native_flags = flags & get_native_context_flags( native_machine, machine );
            wine_server_set_reply( req, server_contexts, sizeof(server_contexts) );
            ret = wine_server_call( req );
            count = wine_server_reply_size( reply ) / sizeof(server_contexts[0]);
        }
        SERVER_END_REQ;
    }
    if (!ret && count)
    {
        ret = context_from_server( context, &server_contexts[0], machine );
        if (!ret && count > 1) ret = context_from_server( context, &server_contexts[1], machine );
    }
    return ret;
}


/***********************************************************************
 *              ntdll_set_exception_jmp_buf
 */
void ntdll_set_exception_jmp_buf( jmp_buf jmp )
{
    assert( !jmp || !ntdll_get_thread_data()->jmp_buf );
    ntdll_get_thread_data()->jmp_buf = jmp;
}


BOOL get_thread_times(int unix_pid, int unix_tid, LARGE_INTEGER *kernel_time, LARGE_INTEGER *user_time)
{
#ifdef linux
    unsigned long clocks_per_sec = sysconf( _SC_CLK_TCK );
    unsigned long usr, sys;
    const char *pos;
    char buf[512];
    FILE *f;
    int i;

    if (unix_tid == -1)
        snprintf( buf, sizeof(buf), "/proc/%u/stat", unix_pid );
    else
        snprintf( buf, sizeof(buf), "/proc/%u/task/%u/stat", unix_pid, unix_tid );
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
#elif defined(HAVE_LIBPROCSTAT)
    struct procstat *pstat;
    struct kinfo_proc *kip;
    unsigned int proc_count;
    BOOL ret = FALSE;

    pstat = procstat_open_sysctl();
    if (!pstat)
        return FALSE;
    if (unix_tid == -1)
        kip = procstat_getprocs(pstat, KERN_PROC_PID, unix_pid, &proc_count);
    else
        kip = procstat_getprocs(pstat, KERN_PROC_PID | KERN_PROC_INC_THREAD, unix_pid, &proc_count);
    if (kip)
    {
        unsigned int i;
        for (i = 0; i < proc_count; i++)
        {
            if (unix_tid == -1 || kip[i].ki_tid == unix_tid)
            {
                kernel_time->QuadPart = 10000000 * (ULONGLONG)kip[i].ki_rusage.ru_stime.tv_sec +
                    10 * (ULONGLONG)kip[i].ki_rusage.ru_stime.tv_usec;
                user_time->QuadPart = 10000000 * (ULONGLONG)kip[i].ki_rusage.ru_utime.tv_sec +
                    10 * (ULONGLONG)kip[i].ki_rusage.ru_utime.tv_usec;
                ret = TRUE;
                break;
            }
        }
        procstat_freeprocs(pstat, kip);
    }
    procstat_close(pstat);
    return ret;
#else
    static int once;
    if (!once++) FIXME("not implemented on this platform\n");
    return FALSE;
#endif
}

static void set_native_thread_name( HANDLE handle, const UNICODE_STRING *name )
{
#ifdef linux
    unsigned int status;
    char path[64], nameA[64];
    int unix_pid, unix_tid, len, fd;

    SERVER_START_REQ( get_thread_times )
    {
        req->handle = wine_server_obj_handle( handle );
        status = wine_server_call( req );
        if (status == STATUS_SUCCESS)
        {
            unix_pid = reply->unix_pid;
            unix_tid = reply->unix_tid;
        }
    }
    SERVER_END_REQ;

    if (status != STATUS_SUCCESS || unix_pid == -1 || unix_tid == -1)
        return;

    if (unix_pid != getpid())
    {
        static int once;
        if (!once++) FIXME("cross-process native thread naming not supported\n");
        return;
    }

    len = ntdll_wcstoumbs( name->Buffer, name->Length / sizeof(WCHAR), nameA, sizeof(nameA), FALSE );
    snprintf(path, sizeof(path), "/proc/%u/task/%u/comm", unix_pid, unix_tid);
    if ((fd = open( path, O_WRONLY )) != -1)
    {
        write( fd, nameA, len );
        close( fd );
    }
#elif defined(__APPLE__)
    char nameA[MAXTHREADNAMESIZE];
    int len;
    THREAD_BASIC_INFORMATION info;

    if (NtQueryInformationThread( handle, ThreadBasicInformation, &info, sizeof(info), NULL ))
        return;

    if (HandleToULong( info.ClientId.UniqueProcess ) != GetCurrentProcessId())
    {
        static int once;
        if (!once++) FIXME("cross-process native thread naming not supported\n");
        return;
    }

    if (HandleToULong( info.ClientId.UniqueThread ) != GetCurrentThreadId())
    {
        static int once;
        if (!once++) FIXME("setting other thread name not supported\n");
        return;
    }

    len = ntdll_wcstoumbs( name->Buffer, name->Length / sizeof(WCHAR), nameA, sizeof(nameA) - 1, FALSE );
    nameA[len] = '\0';
    pthread_setname_np( nameA );
#elif defined(__FreeBSD__)
    unsigned int status;
    char nameA[64];
    int unix_pid, unix_tid, len;

    SERVER_START_REQ( get_thread_times )
    {
        req->handle = wine_server_obj_handle( handle );
        status = wine_server_call( req );
        if (status == STATUS_SUCCESS)
        {
            unix_pid = reply->unix_pid;
            unix_tid = reply->unix_tid;
        }
    }
    SERVER_END_REQ;

    if (status != STATUS_SUCCESS || unix_pid == -1 || unix_tid == -1)
        return;

    if (unix_pid != getpid())
    {
        static int once;
        if (!once++) FIXME("cross-process native thread naming not supported\n");
        return;
    }

    len = ntdll_wcstoumbs( name->Buffer, name->Length / sizeof(WCHAR), nameA, sizeof(nameA), FALSE );
    nameA[len] = '\0';
    thr_set_name( unix_tid, nameA );
#else
    static int once;
    if (!once++) FIXME("not implemented on this platform\n");
#endif
}

static BOOL is_process_wow64( const CLIENT_ID *id )
{
    HANDLE handle;
    ULONG_PTR info;
    BOOL ret = FALSE;

    if (id->UniqueProcess == ULongToHandle(GetCurrentProcessId())) return is_old_wow64();
    if (!NtOpenProcess( &handle, PROCESS_QUERY_LIMITED_INFORMATION, NULL, id ))
    {
        if (!NtQueryInformationProcess( handle, ProcessWow64Information, &info, sizeof(info), NULL ))
            ret = !!info;
        NtClose( handle );
    }
    return ret;
}

/******************************************************************************
 *              NtQueryInformationThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryInformationThread( HANDLE handle, THREADINFOCLASS class,
                                          void *data, ULONG length, ULONG *ret_len )
{
    unsigned int status;

    TRACE("(%p,%d,%p,%x,%p)\n", handle, class, data, (int)length, ret_len);

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
            if (is_old_wow64())
            {
                if (info.TebBaseAddress && is_process_wow64( &info.ClientId ))
                    info.TebBaseAddress = (char *)info.TebBaseAddress + teb_offset;
                else
                    info.TebBaseAddress = NULL;
            }
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

    case ThreadIsTerminated:
    {
        ULONG terminated;

        if (length != sizeof(ULONG)) return STATUS_INFO_LENGTH_MISMATCH;
        SERVER_START_REQ( get_thread_info )
        {
            req->handle = wine_server_obj_handle( handle );
            if (!(status = wine_server_call( req ))) terminated = !!(reply->flags & GET_THREAD_INFO_FLAG_TERMINATED);
        }
        SERVER_END_REQ;
        if (!status)
        {
            *(ULONG *)data = terminated;
            if (ret_len) *ret_len = sizeof(ULONG);
        }
        return status;
    }

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

    case ThreadNameInformation:
    {
        THREAD_NAME_INFORMATION *info = data;
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
            info->ThreadName.Length = info->ThreadName.MaximumLength = desc_len;
            info->ThreadName.Buffer = ptr;
        }

        if (ret_len && (status == STATUS_SUCCESS || status == STATUS_BUFFER_TOO_SMALL))
            *ret_len = sizeof(*info) + desc_len;
        return status;
    }

    case ThreadWow64Context:
        return get_thread_wow64_context( handle, data, length );

    case ThreadHideFromDebugger:
        /* TP Shell Service depends on ThreadHideFromDebugger returning
         * STATUS_ACCESS_VIOLATION if *ret_len is not writable, before
         * any other checks. Despite the status, the variable does not
         * actually seem to be written at that time. */
        if (ret_len) *(volatile ULONG *)ret_len |= 0;

        if (length != sizeof(BOOLEAN)) return STATUS_INFO_LENGTH_MISMATCH;
        if (!data) return STATUS_ACCESS_VIOLATION;
        SERVER_START_REQ( get_thread_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->access = THREAD_QUERY_INFORMATION;
            if ((status = wine_server_call( req ))) return status;
            *(BOOLEAN*)data = !!(reply->flags & GET_THREAD_INFO_FLAG_DBG_HIDDEN);
        }
        SERVER_END_REQ;
        if (ret_len) *ret_len = sizeof(BOOLEAN);
        return STATUS_SUCCESS;

    case ThreadPriorityBoost:
    {
        DWORD *value = data;

        if (length != sizeof(ULONG)) return STATUS_INFO_LENGTH_MISMATCH;
        if (ret_len) *ret_len = sizeof(ULONG);
        *value = 0;
        return STATUS_SUCCESS;
    }

    case ThreadIdealProcessorEx:
    {
        PROCESSOR_NUMBER *number = data;

        FIXME( "ThreadIdealProcessorEx info class - stub\n" );
        if (length != sizeof(*number)) return STATUS_INFO_LENGTH_MISMATCH;
        memset( number, 0, sizeof(*number) );
        if (ret_len) *ret_len = sizeof(*number);
        return STATUS_SUCCESS;
    }

    case ThreadIdealProcessor:
    case ThreadEnableAlignmentFaultFixup:
        return STATUS_INVALID_INFO_CLASS;

    case ThreadPriority:
    case ThreadBasePriority:
    case ThreadImpersonationToken:
    case ThreadEventPair_Reusable:
    case ThreadZeroTlsCell:
    case ThreadPerformanceCount:
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
    unsigned int status;

    TRACE("(%p,%d,%p,%x)\n", handle, class, data, (int)length);

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

    case ThreadNameInformation:
    {
        const THREAD_NAME_INFORMATION *info = data;
        THREAD_BASIC_INFORMATION tbi;

        if (length != sizeof(*info)) return STATUS_INFO_LENGTH_MISMATCH;
        if (!info) return STATUS_ACCESS_VIOLATION;
        if (info->ThreadName.Length && !info->ThreadName.Buffer) return STATUS_ACCESS_VIOLATION;

        status = NtQueryInformationThread( handle, ThreadBasicInformation, &tbi, sizeof(tbi), NULL );
        if (handle == GetCurrentThread() || (!status && (HandleToULong(tbi.ClientId.UniqueThread) == GetCurrentThreadId())))
            WARN_(threadname)( "Thread renamed to %s\n", debugstr_us(&info->ThreadName) );
        else if (!status)
            WARN_(threadname)( "Thread ID %04x renamed to %s\n", (int)HandleToULong( tbi.ClientId.UniqueThread ), debugstr_us(&info->ThreadName) );
        else
            WARN_(threadname)( "Thread handle %p renamed to %s\n", handle, debugstr_us(&info->ThreadName) );

        SERVER_START_REQ( set_thread_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->mask   = SET_THREAD_INFO_DESCRIPTION;
            wine_server_add_data( req, info->ThreadName.Buffer, info->ThreadName.Length );
            status = wine_server_call( req );
        }
        SERVER_END_REQ;

        set_native_thread_name( handle, &info->ThreadName );

        return status;
    }

    case ThreadWineNativeThreadName:
    {
        const THREAD_NAME_INFORMATION *info = data;

        if (length != sizeof(*info)) return STATUS_INFO_LENGTH_MISMATCH;

        set_native_thread_name( handle, &info->ThreadName );
        return STATUS_SUCCESS;
    }

    case ThreadWow64Context:
        return set_thread_wow64_context( handle, data, length );

    case ThreadEnableAlignmentFaultFixup:
        if (length != sizeof(BOOLEAN)) return STATUS_INFO_LENGTH_MISMATCH;
        if (!data) return STATUS_ACCESS_VIOLATION;
        FIXME( "ThreadEnableAlignmentFaultFixup stub!\n" );
        return STATUS_SUCCESS;

    case ThreadPowerThrottlingState:
        if (length != sizeof(THREAD_POWER_THROTTLING_STATE)) return STATUS_INFO_LENGTH_MISMATCH;
        if (!data) return STATUS_ACCESS_VIOLATION;
        FIXME( "ThreadPowerThrottling stub!\n" );
        return STATUS_SUCCESS;

    case ThreadIdealProcessor:
    {
        const ULONG *number = data;

        if (length != sizeof(*number)) return STATUS_INFO_LENGTH_MISMATCH;
        if (*number > MAXIMUM_PROCESSORS) return STATUS_INVALID_PARAMETER;
        FIXME( "ThreadIdealProcessor stub!\n" );
        return STATUS_SUCCESS;
    }

    case ThreadPriorityBoost:
        WARN("Unimplemented class ThreadPriorityBoost.\n");
        return STATUS_SUCCESS;

    case ThreadManageWritesToExecutableMemory:
    {
#ifdef __aarch64__
        const MANAGE_WRITES_TO_EXECUTABLE_MEMORY *mem = data;

        if (length != sizeof(*mem)) return STATUS_INFO_LENGTH_MISMATCH;
        if (handle != GetCurrentThread()) return STATUS_NOT_SUPPORTED;
        if (mem->Version != 2) return STATUS_REVISION_MISMATCH;
        if (mem->ProcessEnableWriteExceptions) return STATUS_INVALID_PARAMETER;
        ntdll_get_thread_data()->allow_writes = mem->ThreadAllowWrites;
        return STATUS_SUCCESS;
#else
        return STATUS_NOT_SUPPORTED;
#endif
    }

    case ThreadBasicInformation:
    case ThreadTimes:
    case ThreadPriority:
    case ThreadDescriptorTableEntry:
    case ThreadEventPair_Reusable:
    case ThreadPerformanceCount:
    case ThreadAmILastThread:
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

#if defined(HAVE_SCHED_GETCPU)
    int res = sched_getcpu();
    if (res >= 0) return res;
#elif defined(__APPLE__) && (defined(__x86_64__) || defined(__i386__))
    struct {
        unsigned long p1, p2;
    } p;
    __asm__ __volatile__("sidt %[p]" : [p] "=&m"(p));
    processor = (ULONG)(p.p1 & 0xfff);
    return processor;
#endif

    if (peb->NumberOfProcessors > 1)
    {
        ULONG_PTR thread_mask, processor_mask;

        if (!NtQueryInformationThread( GetCurrentThread(), ThreadAffinityMask,
                                       &thread_mask, sizeof(thread_mask), NULL ))
        {
            for (processor = 0; processor < peb->NumberOfProcessors; processor++)
            {
                processor_mask = (1 << processor);
                if (thread_mask & processor_mask)
                {
                    if (thread_mask != processor_mask)
                        FIXME( "need multicore support (%d processors)\n",
                               (int)peb->NumberOfProcessors );
                    return processor;
                }
            }
        }
    }
    /* fallback to the first processor */
    return 0;
}


/******************************************************************************
 *              NtGetNextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtGetNextThread( HANDLE process, HANDLE thread, ACCESS_MASK access, ULONG attributes,
                                 ULONG flags, HANDLE *handle )
{
    HANDLE ret_handle = 0;
    unsigned int ret;

    TRACE( "process %p, thread %p, access %#x, attributes %#x, flags %#x, handle %p.\n",
            process, thread, (int)access, (int)attributes, (int)flags, handle );

    SERVER_START_REQ( get_next_thread )
    {
        req->process = wine_server_obj_handle( process );
        req->last = wine_server_obj_handle( thread );
        req->access = access;
        req->attributes = attributes;
        req->flags = flags;
        if (!(ret = wine_server_call( req ))) ret_handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    *handle = ret_handle;
    return ret;
}
