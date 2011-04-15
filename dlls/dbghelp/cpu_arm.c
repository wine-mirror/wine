/*
 * File cpu_arm.c
 *
 * Copyright (C) 2009-2009, Eric Pouech
 * Copyright (C) 2010, André Hentschel
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

#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "dbghelp_private.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

static unsigned arm_get_addr(HANDLE hThread, const CONTEXT* ctx,
                             enum cpu_addr ca, ADDRESS64* addr)
{
    addr->Mode    = AddrModeFlat;
    addr->Segment = 0; /* don't need segment */
    switch (ca)
    {
#ifdef __arm__
    case cpu_addr_pc:    addr->Offset = ctx->Pc; return TRUE;
    case cpu_addr_stack: addr->Offset = ctx->Sp; return TRUE;
    case cpu_addr_frame: addr->Offset = ctx->Fp; return TRUE;
#endif
    default: addr->Mode = -1;
        return FALSE;
    }
}

static BOOL arm_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context)
{
    FIXME("not done for ARM\n");
    return FALSE;
}

static unsigned arm_map_dwarf_register(unsigned regno)
{
    if (regno <= 15) return CV_ARM_R0 + regno;
    if (regno == 128) return CV_ARM_CPSR;

    FIXME("Don't know how to map register %d\n", regno);
    return CV_ARM_NOREG;
}

static void* arm_fetch_context_reg(CONTEXT* ctx, unsigned regno, unsigned* size)
{
    FIXME("not done for ARM\n");
    return NULL;
}

static const char* arm_fetch_regname(unsigned regno)
{
    switch (regno)
    {
    case CV_ARM_R0 +  0: return "r0";
    case CV_ARM_R0 +  1: return "r1";
    case CV_ARM_R0 +  2: return "r2";
    case CV_ARM_R0 +  3: return "r3";
    case CV_ARM_R0 +  4: return "r4";
    case CV_ARM_R0 +  5: return "r5";
    case CV_ARM_R0 +  6: return "r6";
    case CV_ARM_R0 +  7: return "r7";
    case CV_ARM_R0 +  8: return "r8";
    case CV_ARM_R0 +  9: return "r9";
    case CV_ARM_R0 + 10: return "r10";
    case CV_ARM_R0 + 11: return "r11";
    case CV_ARM_R0 + 12: return "r12";

    case CV_ARM_SP: return "sp";
    case CV_ARM_LR: return "lr";
    case CV_ARM_PC: return "pc";
    case CV_ARM_CPSR: return "cpsr";
    }
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

struct cpu cpu_arm = {
    IMAGE_FILE_MACHINE_ARM,
    4,
    CV_ARM_NOREG, /* FIXME */
    arm_get_addr,
    arm_stack_walk,
    NULL,
    arm_map_dwarf_register,
    arm_fetch_context_reg,
    arm_fetch_regname,
};
