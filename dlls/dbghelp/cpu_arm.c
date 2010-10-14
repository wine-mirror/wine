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
    FIXME("not done for ARM\n");
    return 0;
}

static void* arm_fetch_context_reg(CONTEXT* ctx, unsigned regno, unsigned* size)
{
    FIXME("not done for ARM\n");
    return NULL;
}

static const char* arm_fetch_regname(unsigned regno)
{
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

struct cpu cpu_arm = {
    IMAGE_FILE_MACHINE_ARM,
    4,
    arm_get_addr,
    arm_stack_walk,
    NULL,
    arm_map_dwarf_register,
    arm_fetch_context_reg,
    arm_fetch_regname,
};
