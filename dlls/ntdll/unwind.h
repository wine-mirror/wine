/*
 * Exception unwinding definitions
 *
 * Copyright 2023 Alexandre Julliard
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

#ifndef __WINE_NTDLL_UNWIND_H
#define __WINE_NTDLL_UNWIND_H

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winnt.h"

#if defined(__aarch64__) || defined(__arm64ec__)

static inline ULONG ctx_flags_x64_to_arm( ULONG flags )
{
    ULONG ret = CONTEXT_ARM64;

    flags &= ~CONTEXT_AMD64;
    if (flags & CONTEXT_AMD64_CONTROL) ret |= CONTEXT_ARM64_CONTROL;
    if (flags & CONTEXT_AMD64_INTEGER) ret |= CONTEXT_ARM64_INTEGER;
    if (flags & CONTEXT_AMD64_FLOATING_POINT) ret |= CONTEXT_ARM64_FLOATING_POINT;
    return ret;
}

static inline ULONG ctx_flags_arm_to_x64( ULONG flags )
{
    ULONG ret = CONTEXT_AMD64;

    flags &= ~CONTEXT_ARM64;
    if (flags & CONTEXT_ARM64_CONTROL) ret |= CONTEXT_AMD64_CONTROL;
    if (flags & CONTEXT_ARM64_INTEGER) ret |= CONTEXT_AMD64_INTEGER;
    if (flags & CONTEXT_ARM64_FLOATING_POINT) ret |= CONTEXT_AMD64_FLOATING_POINT;
    return ret;
}

static inline UINT eflags_to_cpsr( UINT eflags )
{
    UINT ret = 0;

    if (eflags & 0x0001) ret |= 0x20000000;  /* carry */
    if (eflags & 0x0040) ret |= 0x40000000;  /* zero */
    if (eflags & 0x0080) ret |= 0x80000000;  /* negative */
    if (eflags & 0x0100) ret |= 0x00200000;  /* trap */
    if (eflags & 0x0800) ret |= 0x10000000;  /* overflow */
    return ret;
}

static inline UINT cpsr_to_eflags( UINT cpsr )
{
    UINT ret = 0x202;

    if (cpsr & 0x00200000) ret |= 0x0100;  /* trap */
    if (cpsr & 0x10000000) ret |= 0x0800;  /* overflow */
    if (cpsr & 0x20000000) ret |= 0x0001;  /* carry */
    if (cpsr & 0x40000000) ret |= 0x0040;  /* zero */
    if (cpsr & 0x80000000) ret |= 0x0080;  /* negative */
    return ret;
}

static inline UINT64 mxcsr_to_fpcsr( UINT mxcsr )
{
    UINT fpcr = 0, fpsr = 0;

    if (mxcsr & 0x0001) fpsr |= 0x0001;    /* invalid operation */
    if (mxcsr & 0x0002) fpsr |= 0x0080;    /* denormal */
    if (mxcsr & 0x0004) fpsr |= 0x0002;    /* zero-divide */
    if (mxcsr & 0x0008) fpsr |= 0x0004;    /* overflow */
    if (mxcsr & 0x0010) fpsr |= 0x0008;    /* underflow */
    if (mxcsr & 0x0020) fpsr |= 0x0010;    /* precision */

    if (mxcsr & 0x0040)    fpcr |= 0x80000;   /* denormals are zero */
    if (!(mxcsr & 0x0080)) fpcr |= 0x0100;    /* invalid operation mask */
    if (!(mxcsr & 0x0100)) fpcr |= 0x8000;    /* denormal mask */
    if (!(mxcsr & 0x0200)) fpcr |= 0x0200;    /* zero-divide mask */
    if (!(mxcsr & 0x0400)) fpcr |= 0x0400;    /* overflow mask */
    if (!(mxcsr & 0x0800)) fpcr |= 0x0800;    /* underflow mask */
    if (!(mxcsr & 0x1000)) fpcr |= 0x1000;    /* precision mask */
    if (mxcsr & 0x2000)    fpcr |= 0x800000;  /* round down */
    if (mxcsr & 0x4000)    fpcr |= 0x400000;  /* round up */
    if (mxcsr & 0x8000)    fpcr |= 0x1000000; /* flush to zero */
    return fpcr | ((UINT64)fpsr << 32);
}

static inline UINT fpcsr_to_mxcsr( UINT fpcr, UINT fpsr )
{
    UINT ret = 0;

    if (fpsr & 0x0001) ret |= 0x0001;      /* invalid operation */
    if (fpsr & 0x0002) ret |= 0x0004;      /* zero-divide */
    if (fpsr & 0x0004) ret |= 0x0008;      /* overflow */
    if (fpsr & 0x0008) ret |= 0x0010;      /* underflow */
    if (fpsr & 0x0010) ret |= 0x0020;      /* precision */
    if (fpsr & 0x0080) ret |= 0x0002;      /* denormal */

    if (fpcr & 0x0080000)    ret |= 0x0040;   /* denormals are zero */
    if (!(fpcr & 0x0000100)) ret |= 0x0080;   /* invalid operation mask */
    if (!(fpcr & 0x0000200)) ret |= 0x0200;   /* zero-divide mask */
    if (!(fpcr & 0x0000400)) ret |= 0x0400;   /* overflow mask */
    if (!(fpcr & 0x0000800)) ret |= 0x0800;   /* underflow mask */
    if (!(fpcr & 0x0001000)) ret |= 0x1000;   /* precision mask */
    if (!(fpcr & 0x0008000)) ret |= 0x0100;   /* denormal mask */
    if (fpcr & 0x0400000)    ret |= 0x4000;   /* round up */
    if (fpcr & 0x0800000)    ret |= 0x2000;   /* round down */
    if (fpcr & 0x1000000)    ret |= 0x8000;   /* flush to zero */
    return ret;
}

static inline void context_x64_to_arm( ARM64_NT_CONTEXT *arm_ctx, const ARM64EC_NT_CONTEXT *ec_ctx )
{
    UINT64 fpcsr;

    arm_ctx->ContextFlags = ctx_flags_x64_to_arm( ec_ctx->ContextFlags );
    arm_ctx->Cpsr = eflags_to_cpsr( ec_ctx->AMD64_EFlags );
    arm_ctx->X0   = ec_ctx->X0;
    arm_ctx->X1   = ec_ctx->X1;
    arm_ctx->X2   = ec_ctx->X2;
    arm_ctx->X3   = ec_ctx->X3;
    arm_ctx->X4   = ec_ctx->X4;
    arm_ctx->X5   = ec_ctx->X5;
    arm_ctx->X6   = ec_ctx->X6;
    arm_ctx->X7   = ec_ctx->X7;
    arm_ctx->X8   = ec_ctx->X8;
    arm_ctx->X9   = ec_ctx->X9;
    arm_ctx->X10  = ec_ctx->X10;
    arm_ctx->X11  = ec_ctx->X11;
    arm_ctx->X12  = ec_ctx->X12;
    arm_ctx->X13  = 0;
    arm_ctx->X14  = 0;
    arm_ctx->X15  = ec_ctx->X15;
    arm_ctx->X16  = ec_ctx->X16_0 | ((DWORD64)ec_ctx->X16_1 << 16) | ((DWORD64)ec_ctx->X16_2 << 32) | ((DWORD64)ec_ctx->X16_3 << 48);
    arm_ctx->X17  = ec_ctx->X17_0 | ((DWORD64)ec_ctx->X17_1 << 16) | ((DWORD64)ec_ctx->X17_2 << 32) | ((DWORD64)ec_ctx->X17_3 << 48);
    arm_ctx->X18  = 0;
    arm_ctx->X19  = ec_ctx->X19;
    arm_ctx->X20  = ec_ctx->X20;
    arm_ctx->X21  = ec_ctx->X21;
    arm_ctx->X22  = ec_ctx->X22;
    arm_ctx->X23  = 0;
    arm_ctx->X24  = 0;
    arm_ctx->X25  = ec_ctx->X25;
    arm_ctx->X26  = ec_ctx->X26;
    arm_ctx->X27  = ec_ctx->X27;
    arm_ctx->X28  = 0;
    arm_ctx->Fp   = ec_ctx->Fp;
    arm_ctx->Lr   = ec_ctx->Lr;
    arm_ctx->Sp   = ec_ctx->Sp;
    arm_ctx->Pc   = ec_ctx->Pc;
    memcpy( arm_ctx->V, ec_ctx->V, 16 * sizeof(arm_ctx->V[0]) );
    memset( arm_ctx->V + 16, 0, sizeof(*arm_ctx) - offsetof( ARM64_NT_CONTEXT, V[16] ));
    fpcsr = mxcsr_to_fpcsr( ec_ctx->AMD64_MxCsr );
    arm_ctx->Fpcr = fpcsr;
    arm_ctx->Fpsr = fpcsr >> 32;
}

static inline void context_arm_to_x64( ARM64EC_NT_CONTEXT *ec_ctx, const ARM64_NT_CONTEXT *arm_ctx )
{
    memset( ec_ctx, 0, sizeof(*ec_ctx) );
    ec_ctx->ContextFlags = ctx_flags_arm_to_x64( arm_ctx->ContextFlags );
    ec_ctx->AMD64_SegCs  = 0x33;
    ec_ctx->AMD64_SegDs  = 0x2b;
    ec_ctx->AMD64_SegEs  = 0x2b;
    ec_ctx->AMD64_SegFs  = 0x53;
    ec_ctx->AMD64_SegGs  = 0x2b;
    ec_ctx->AMD64_SegSs  = 0x2b;
    ec_ctx->AMD64_EFlags = cpsr_to_eflags( arm_ctx->Cpsr );
    ec_ctx->AMD64_MxCsr  = ec_ctx->AMD64_MxCsr_copy = fpcsr_to_mxcsr( arm_ctx->Fpcr, arm_ctx->Fpsr );

    ec_ctx->X8    = arm_ctx->X8;
    ec_ctx->X0    = arm_ctx->X0;
    ec_ctx->X1    = arm_ctx->X1;
    ec_ctx->X27   = arm_ctx->X27;
    ec_ctx->Sp    = arm_ctx->Sp;
    ec_ctx->Fp    = arm_ctx->Fp;
    ec_ctx->X25   = arm_ctx->X25;
    ec_ctx->X26   = arm_ctx->X26;
    ec_ctx->X2    = arm_ctx->X2;
    ec_ctx->X3    = arm_ctx->X3;
    ec_ctx->X4    = arm_ctx->X4;
    ec_ctx->X5    = arm_ctx->X5;
    ec_ctx->X19   = arm_ctx->X19;
    ec_ctx->X20   = arm_ctx->X20;
    ec_ctx->X21   = arm_ctx->X21;
    ec_ctx->X22   = arm_ctx->X22;
    ec_ctx->Pc    = arm_ctx->Pc;
    ec_ctx->Lr    = arm_ctx->Lr;
    ec_ctx->X6    = arm_ctx->X6;
    ec_ctx->X7    = arm_ctx->X7;
    ec_ctx->X9    = arm_ctx->X9;
    ec_ctx->X10   = arm_ctx->X10;
    ec_ctx->X11   = arm_ctx->X11;
    ec_ctx->X12   = arm_ctx->X12;
    ec_ctx->X15   = arm_ctx->X15;
    ec_ctx->X16_0 = arm_ctx->X16;
    ec_ctx->X16_1 = arm_ctx->X16 >> 16;
    ec_ctx->X16_2 = arm_ctx->X16 >> 32;
    ec_ctx->X16_3 = arm_ctx->X16 >> 48;
    ec_ctx->X17_0 = arm_ctx->X17;
    ec_ctx->X17_1 = arm_ctx->X17 >> 16;
    ec_ctx->X17_2 = arm_ctx->X17 >> 32;
    ec_ctx->X17_3 = arm_ctx->X17 >> 48;

    memcpy( ec_ctx->V, arm_ctx->V, sizeof(ec_ctx->V) );
}

#endif /* __aarch64__ || __arm64ec__ */

#endif /* __WINE_NTDLL_UNWIND_H */
