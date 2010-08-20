/*
 * File cpu_sparc.c
 *
 * Copyright (C) 2009-2009, Eric Pouech
 * Copyright (C) 2010, Austin English
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

static unsigned sparc_get_addr(HANDLE hThread, const CONTEXT* ctx,
                             enum cpu_addr ca, ADDRESS64* addr)
{
   switch (ca)
    {
    case cpu_addr_pc:
    case cpu_addr_stack:
    case cpu_addr_frame:
    default:
         FIXME("not done for Sparc\n");
    }
    return FALSE;
}

static BOOL sparc_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context)
{
    FIXME("not done for Sparc\n");
    return FALSE;
}

static unsigned sparc_map_dwarf_register(unsigned regno)
{
    FIXME("not done for Sparc\n");
    return 0;
}

static void* sparc_fetch_context_reg(CONTEXT* ctx, unsigned regno, unsigned* size)
{
    FIXME("not done for Sparc\n");
    return NULL;
}

static const char* sparc_fetch_regname(unsigned regno)
{
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

struct cpu cpu_sparc = {
    IMAGE_FILE_MACHINE_SPARC,
    4,
    sparc_get_addr,
    sparc_stack_walk,
    NULL,
    sparc_map_dwarf_register,
    sparc_fetch_context_reg,
    sparc_fetch_regname,
};
