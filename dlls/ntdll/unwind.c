/*
 * Exception unwinding
 *
 * Copyright 1999, 2005, 2019, 2024 Alexandre Julliard
 * Copyright 2021 Martin Storsjö
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

#ifndef __i386__

#include <stdlib.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/list.h"
#include "ntdll_misc.h"
#include "unwind.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(unwind);


/***********************************************************************
 * C specific handler
 */
extern LONG __C_ExecuteExceptionFilter( EXCEPTION_POINTERS *ptrs, void *frame,
                                        PEXCEPTION_FILTER filter, BYTE *nonvolatile );

#define DUMP_SCOPE_TABLE(base,table) do { \
        for (unsigned int i = 0; i < table->Count; i++) \
            TRACE( "  %u: %p-%p handler %p target %p\n", i, \
                   (char *)base + table->ScopeRecord[i].BeginAddress,   \
                   (char *)base + table->ScopeRecord[i].EndAddress,     \
                   (char *)base + table->ScopeRecord[i].HandlerAddress, \
                   (char *)base + table->ScopeRecord[i].JumpTarget );   \
    } while(0)


/***********************************************************************
 * Dynamic unwind tables
 */

struct dynamic_unwind_entry
{
    struct list       entry;
    ULONG_PTR         base;
    ULONG_PTR         end;
    RUNTIME_FUNCTION *table;
    DWORD             count;
    DWORD             max_count;
    PGET_RUNTIME_FUNCTION_CALLBACK callback;
    PVOID             context;
};

static struct list dynamic_unwind_list = LIST_INIT(dynamic_unwind_list);

static RTL_CRITICAL_SECTION dynamic_unwind_section;
static RTL_CRITICAL_SECTION_DEBUG dynamic_unwind_debug =
{
    0, 0, &dynamic_unwind_section,
    { &dynamic_unwind_debug.ProcessLocksList, &dynamic_unwind_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dynamic_unwind_section") }
};
static RTL_CRITICAL_SECTION dynamic_unwind_section = { &dynamic_unwind_debug, -1, 0, 0, 0, 0 };


static RUNTIME_FUNCTION *lookup_dynamic_function_table( ULONG_PTR pc, ULONG_PTR *base, ULONG *count )
{
    struct dynamic_unwind_entry *entry;
    RUNTIME_FUNCTION *ret = NULL;

    RtlEnterCriticalSection( &dynamic_unwind_section );
    LIST_FOR_EACH_ENTRY( entry, &dynamic_unwind_list, struct dynamic_unwind_entry, entry )
    {
        if (pc >= entry->base && pc < entry->end)
        {
            *base = entry->base;
            if (entry->callback)
            {
                ret = entry->callback( pc, entry->context );
                *count = 1;
            }
            else
            {
                ret = entry->table;
                *count = entry->count;
            }
            break;
        }
    }
    RtlLeaveCriticalSection( &dynamic_unwind_section );
    return ret;
}


/**********************************************************************
 *              RtlInstallFunctionTableCallback   (NTDLL.@)
 */
BOOLEAN CDECL RtlInstallFunctionTableCallback( ULONG_PTR table, ULONG_PTR base, DWORD length,
                                               PGET_RUNTIME_FUNCTION_CALLBACK callback, PVOID context,
                                               PCWSTR dll )
{
    struct dynamic_unwind_entry *entry;

    TRACE( "%Ix %Ix %ld %p %p %s\n", table, base, length, callback, context, wine_dbgstr_w(dll) );

    /* NOTE: Windows doesn't check if the provided callback is a NULL pointer */

    /* both low-order bits must be set */
    if ((table & 0x3) != 0x3)
        return FALSE;

    entry = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*entry) );
    if (!entry)
        return FALSE;

    entry->base      = base;
    entry->end       = base + length;
    entry->table     = (RUNTIME_FUNCTION *)table;
    entry->count     = 0;
    entry->max_count = 0;
    entry->callback  = callback;
    entry->context   = context;

    RtlEnterCriticalSection( &dynamic_unwind_section );
    list_add_tail( &dynamic_unwind_list, &entry->entry );
    RtlLeaveCriticalSection( &dynamic_unwind_section );

    return TRUE;
}


/*************************************************************************
 *              RtlAddGrowableFunctionTable   (NTDLL.@)
 */
NTSTATUS WINAPI RtlAddGrowableFunctionTable( void **table, RUNTIME_FUNCTION *functions, DWORD count,
                                             DWORD max_count, ULONG_PTR base, ULONG_PTR end )
{
    struct dynamic_unwind_entry *entry;

    TRACE( "%p, %p, %lu, %lu, %Ix, %Ix\n", table, functions, count, max_count, base, end );

    entry = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*entry) );
    if (!entry)
        return STATUS_NO_MEMORY;

    entry->base      = base;
    entry->end       = end;
    entry->table     = functions;
    entry->count     = count;
    entry->max_count = max_count;
    entry->callback  = NULL;
    entry->context   = NULL;

    RtlEnterCriticalSection( &dynamic_unwind_section );
    list_add_tail( &dynamic_unwind_list, &entry->entry );
    RtlLeaveCriticalSection( &dynamic_unwind_section );

    *table = entry;

    return STATUS_SUCCESS;
}


/*************************************************************************
 *              RtlGrowFunctionTable   (NTDLL.@)
 */
void WINAPI RtlGrowFunctionTable( void *table, DWORD count )
{
    struct dynamic_unwind_entry *entry;

    TRACE( "%p, %lu\n", table, count );

    RtlEnterCriticalSection( &dynamic_unwind_section );
    LIST_FOR_EACH_ENTRY( entry, &dynamic_unwind_list, struct dynamic_unwind_entry, entry )
    {
        if (entry == table)
        {
            if (count > entry->count && count <= entry->max_count)
                entry->count = count;
            break;
        }
    }
    RtlLeaveCriticalSection( &dynamic_unwind_section );
}


/*************************************************************************
 *              RtlDeleteGrowableFunctionTable   (NTDLL.@)
 */
void WINAPI RtlDeleteGrowableFunctionTable( void *table )
{
    struct dynamic_unwind_entry *entry, *to_free = NULL;

    TRACE( "%p\n", table );

    RtlEnterCriticalSection( &dynamic_unwind_section );
    LIST_FOR_EACH_ENTRY( entry, &dynamic_unwind_list, struct dynamic_unwind_entry, entry )
    {
        if (entry == table)
        {
            to_free = entry;
            list_remove( &entry->entry );
            break;
        }
    }
    RtlLeaveCriticalSection( &dynamic_unwind_section );

    RtlFreeHeap( GetProcessHeap(), 0, to_free );
}


/**********************************************************************
 *              RtlDeleteFunctionTable   (NTDLL.@)
 */
BOOLEAN CDECL RtlDeleteFunctionTable( RUNTIME_FUNCTION *table )
{
    struct dynamic_unwind_entry *entry, *to_free = NULL;

    TRACE( "%p\n", table );

    RtlEnterCriticalSection( &dynamic_unwind_section );
    LIST_FOR_EACH_ENTRY( entry, &dynamic_unwind_list, struct dynamic_unwind_entry, entry )
    {
        if (entry->table == table)
        {
            to_free = entry;
            list_remove( &entry->entry );
            break;
        }
    }
    RtlLeaveCriticalSection( &dynamic_unwind_section );

    if (!to_free) return FALSE;

    RtlFreeHeap( GetProcessHeap(), 0, to_free );
    return TRUE;
}


/**********************************************************************
 *              RtlLookupFunctionTable   (NTDLL.@)
 */
PRUNTIME_FUNCTION WINAPI RtlLookupFunctionTable( ULONG_PTR pc, ULONG_PTR *base, ULONG *len )
{
    LDR_DATA_TABLE_ENTRY *module;

    if (LdrFindEntryForAddress( (void *)pc, &module )) return NULL;
    *base = (ULONG_PTR)module->DllBase;
#ifdef __arm64ec__
    if (RtlIsEcCode( pc ))
    {
        IMAGE_ARM64EC_METADATA *metadata = arm64ec_get_module_metadata( module->DllBase );
        if (!metadata) return NULL;
        *len = metadata->ExtraRFETableSize;
        return (RUNTIME_FUNCTION *)(*base + metadata->ExtraRFETable);
    }
#endif
    return RtlImageDirectoryEntryToData( module->DllBase, TRUE, IMAGE_DIRECTORY_ENTRY_EXCEPTION, len );
}


/***********************************************************************
 * ARM64 support
 */
#if defined(__aarch64__) || defined(__arm64ec__)

struct unwind_info_ext
{
    WORD epilog;
    BYTE codes;
    BYTE reserved;
};

struct unwind_info_epilog
{
    DWORD offset : 18;
    DWORD res : 4;
    DWORD index : 10;
};

static const BYTE unwind_code_len[256] =
{
/* 00 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 20 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 40 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 60 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 80 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* a0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* c0 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* e0 */ 4,1,2,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

static unsigned int get_sequence_len( BYTE *ptr, BYTE *end )
{
    unsigned int ret = 0;

    while (ptr < end)
    {
        if (*ptr == 0xe4 || *ptr == 0xe5) break;
        if ((*ptr & 0xf8) != 0xe8) ret++;  /* custom stack frames don't count */
        ptr += unwind_code_len[*ptr];
    }
    return ret;
}

static void restore_regs( int reg, int count, int pos, ARM64_NT_CONTEXT *context,
                          KNONVOLATILE_CONTEXT_POINTERS_ARM64 *ptrs )
{
    int i, offset = max( 0, pos );
    for (i = 0; i < count; i++)
    {
        if (ptrs && reg + i >= 19) (&ptrs->X19)[reg + i - 19] = (DWORD64 *)context->Sp + i + offset;
        context->X[reg + i] = ((DWORD64 *)context->Sp)[i + offset];
    }
    if (pos < 0) context->Sp += -8 * pos;
}

static void restore_fpregs( int reg, int count, int pos, ARM64_NT_CONTEXT *context,
                            KNONVOLATILE_CONTEXT_POINTERS_ARM64 *ptrs )
{
    int i, offset = max( 0, pos );
    for (i = 0; i < count; i++)
    {
        if (ptrs && reg + i >= 8) (&ptrs->D8)[reg + i - 8] = (DWORD64 *)context->Sp + i + offset;
        context->V[reg + i].D[0] = ((double *)context->Sp)[i + offset];
    }
    if (pos < 0) context->Sp += -8 * pos;
}

static void restore_qregs( int reg, int count, int pos, ARM64_NT_CONTEXT *context,
                           KNONVOLATILE_CONTEXT_POINTERS_ARM64 *ptrs )
{
    int i, offset = max( 0, pos );
    for (i = 0; i < count; i++)
    {
        if (ptrs && reg + i >= 8) (&ptrs->D8)[reg + i - 8] = (DWORD64 *)context->Sp + 2 * (i + offset);
        context->V[reg + i].Low  = ((DWORD64 *)context->Sp)[2 * (i + offset)];
        context->V[reg + i].High = ((DWORD64 *)context->Sp)[2 * (i + offset) + 1];
    }
    if (pos < 0) context->Sp += -16 * pos;
}

static void restore_any_reg( int reg, int count, int type, int pos, ARM64_NT_CONTEXT *context,
                             KNONVOLATILE_CONTEXT_POINTERS_ARM64 *ptrs )
{
    if (reg & 0x20) pos = -pos - 1;

    switch (type)
    {
    case 0:
        if (count > 1 || pos < 0) pos *= 2;
        restore_regs( reg & 0x1f, count, pos, context, ptrs );
        break;
    case 1:
        if (count > 1 || pos < 0) pos *= 2;
        restore_fpregs( reg & 0x1f, count, pos, context, ptrs );
        break;
    case 2:
        restore_qregs( reg & 0x1f, count, pos, context, ptrs );
        break;
    }
}

static void do_pac_auth( ARM64_NT_CONTEXT *context )
{
    register DWORD64 x17 __asm__( "x17" ) = context->Lr;
    register DWORD64 x16 __asm__( "x16" ) = context->Sp;

    /* This is the autib1716 instruction. The hint instruction is used here
     * as gcc does not assemble autib1716 for pre armv8.3a targets. For
     * pre-armv8.3a targets, this is just treated as a hint instruction, which
     * is ignored. */
    __asm__( "hint 0xe" : "+r"(x17) : "r"(x16) );

    context->Lr = x17;
}

static void process_unwind_codes( BYTE *ptr, BYTE *end, ARM64_NT_CONTEXT *context,
                                  KNONVOLATILE_CONTEXT_POINTERS_ARM64 *ptrs, int skip,
                                  BOOLEAN *final_pc_from_lr )
{
    unsigned int i, val, len, save_next = 2;

    /* skip codes */
    while (ptr < end && skip)
    {
        if (*ptr == 0xe4) break;
        ptr += unwind_code_len[*ptr];
        skip--;
    }

    while (ptr < end)
    {
        if ((len = unwind_code_len[*ptr]) > 1)
        {
            if (ptr + len > end) break;
            val = ptr[0] * 0x100 + ptr[1];
        }
        else val = *ptr;

        if (*ptr < 0x20)  /* alloc_s */
            context->Sp += 16 * (val & 0x1f);
        else if (*ptr < 0x40)  /* save_r19r20_x */
            restore_regs( 19, save_next, -(val & 0x1f), context, ptrs );
        else if (*ptr < 0x80) /* save_fplr */
            restore_regs( 29, 2, val & 0x3f, context, ptrs );
        else if (*ptr < 0xc0)  /* save_fplr_x */
            restore_regs( 29, 2, -(val & 0x3f) - 1, context, ptrs );
        else if (*ptr < 0xc8)  /* alloc_m */
            context->Sp += 16 * (val & 0x7ff);
        else if (*ptr < 0xcc)  /* save_regp */
            restore_regs( 19 + ((val >> 6) & 0xf), save_next, val & 0x3f, context, ptrs );
        else if (*ptr < 0xd0)  /* save_regp_x */
            restore_regs( 19 + ((val >> 6) & 0xf), save_next, -(val & 0x3f) - 1, context, ptrs );
        else if (*ptr < 0xd4)  /* save_reg */
            restore_regs( 19 + ((val >> 6) & 0xf), 1, val & 0x3f, context, ptrs );
        else if (*ptr < 0xd6)  /* save_reg_x */
            restore_regs( 19 + ((val >> 5) & 0xf), 1, -(val & 0x1f) - 1, context, ptrs );
        else if (*ptr < 0xd8)  /* save_lrpair */
        {
            restore_regs( 19 + 2 * ((val >> 6) & 0x7), 1, val & 0x3f, context, ptrs );
            restore_regs( 30, 1, (val & 0x3f) + 1, context, ptrs );
        }
        else if (*ptr < 0xda)  /* save_fregp */
            restore_fpregs( 8 + ((val >> 6) & 0x7), save_next, val & 0x3f, context, ptrs );
        else if (*ptr < 0xdc)  /* save_fregp_x */
            restore_fpregs( 8 + ((val >> 6) & 0x7), save_next, -(val & 0x3f) - 1, context, ptrs );
        else if (*ptr < 0xde)  /* save_freg */
            restore_fpregs( 8 + ((val >> 6) & 0x7), 1, val & 0x3f, context, ptrs );
        else if (*ptr == 0xde)  /* save_freg_x */
            restore_fpregs( 8 + ((val >> 5) & 0x7), 1, -(val & 0x3f) - 1, context, ptrs );
        else if (*ptr == 0xe0)  /* alloc_l */
            context->Sp += 16 * ((ptr[1] << 16) + (ptr[2] << 8) + ptr[3]);
        else if (*ptr == 0xe1)  /* set_fp */
            context->Sp = context->Fp;
        else if (*ptr == 0xe2)  /* add_fp */
            context->Sp = context->Fp - 8 * (val & 0xff);
        else if (*ptr == 0xe3)  /* nop */
            /* nop */ ;
        else if (*ptr == 0xe4)  /* end */
            break;
        else if (*ptr == 0xe5)  /* end_c */
            /* ignore */;
        else if (*ptr == 0xe6)  /* save_next */
        {
            save_next += 2;
            ptr += len;
            continue;
        }
        else if (*ptr == 0xe7)  /* save_any_reg */
        {
            restore_any_reg( ptr[1], (ptr[1] & 0x40) ? save_next : 1,
                             ptr[2] >> 6, ptr[2] & 0x3f, context, ptrs );
        }
        else if (*ptr == 0xe9)  /* MSFT_OP_MACHINE_FRAME */
        {
            context->Pc = ((DWORD64 *)context->Sp)[1];
            context->Sp = ((DWORD64 *)context->Sp)[0];
            context->ContextFlags &= ~CONTEXT_UNWOUND_TO_CALL;
            *final_pc_from_lr = FALSE;
        }
        else if (*ptr == 0xea)  /* MSFT_OP_CONTEXT */
        {
            DWORD flags = context->ContextFlags & ~CONTEXT_UNWOUND_TO_CALL;
            ARM64_NT_CONTEXT *src_ctx = (ARM64_NT_CONTEXT *)context->Sp;
            *context = *src_ctx;
            context->ContextFlags = flags | (src_ctx->ContextFlags & CONTEXT_UNWOUND_TO_CALL);
            if (ptrs)
            {
                for (i = 19; i < 29; i++) (&ptrs->X19)[i - 19] = &src_ctx->X[i];
                for (i = 8; i < 16; i++) (&ptrs->D8)[i - 8] = &src_ctx->V[i].Low;
            }
            *final_pc_from_lr = FALSE;
        }
        else if (*ptr == 0xeb)  /* MSFT_OP_EC_CONTEXT */
        {
            DWORD flags = context->ContextFlags & ~CONTEXT_UNWOUND_TO_CALL;
            ARM64EC_NT_CONTEXT *src_ctx = (ARM64EC_NT_CONTEXT *)context->Sp;
            context_x64_to_arm( context, src_ctx );
            context->ContextFlags = flags | (src_ctx->ContextFlags & CONTEXT_UNWOUND_TO_CALL);
            if (ptrs) for (i = 8; i < 16; i++) (&ptrs->D8)[i - 8] = &src_ctx->V[i].Low;
            *final_pc_from_lr = FALSE;
        }
        else if (*ptr == 0xec)  /* MSFT_OP_CLEAR_UNWOUND_TO_CALL */
        {
            context->Pc = context->Lr;
            context->ContextFlags &= ~CONTEXT_UNWOUND_TO_CALL;
            *final_pc_from_lr = FALSE;
        }
        else if (*ptr == 0xfc)  /* pac_sign_lr */
        {
            do_pac_auth( context );
        }
        else
        {
            WARN( "unsupported code %02x\n", *ptr );
            return;
        }
        save_next = 2;
        ptr += len;
    }
}

static void *unwind_packed_data( ULONG_PTR base, ULONG_PTR pc, ARM64_RUNTIME_FUNCTION *func,
                                 ARM64_NT_CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS_ARM64 *ptrs )
{
    int i;
    unsigned int len, offset, skip = 0;
    unsigned int int_size = func->RegI * 8, fp_size = func->RegF * 8, h_size = func->H * 4, regsave, local_size;
    unsigned int int_regs, fp_regs, saved_regs, local_size_regs;

    TRACE( "function %I64x-%I64x: len=%#x flag=%x regF=%u regI=%u H=%u CR=%u frame=%x\n",
           base + func->BeginAddress, base + func->BeginAddress + func->FunctionLength * 4,
           func->FunctionLength, func->Flag, func->RegF, func->RegI, func->H, func->CR, func->FrameSize );

    if (func->CR == 1) int_size += 8;
    if (func->RegF) fp_size += 8;

    regsave = ((int_size + fp_size + 8 * 8 * func->H) + 0xf) & ~0xf;
    local_size = func->FrameSize * 16 - regsave;

    int_regs = int_size / 8;
    fp_regs = fp_size / 8;
    saved_regs = regsave / 8;
    local_size_regs = local_size / 8;

    /* check for prolog/epilog */
    if (func->Flag == 1)
    {
        offset = ((pc - base) - func->BeginAddress) / 4;
        if (offset < 17 || offset >= func->FunctionLength - 15)
        {
            len = (int_size + 8) / 16 + (fp_size + 8) / 16;
            switch (func->CR)
            {
            case 2:
                len++; /* pacibsp */
                /* fall through */
            case 3:
                len++; /* mov x29,sp */
                len++; /* stp x29,lr,[sp,0] */
                if (local_size <= 512) break;
                /* fall through */
            case 0:
            case 1:
                if (local_size) len++;  /* sub sp,sp,#local_size */
                if (local_size > 4088) len++;  /* sub sp,sp,#4088 */
                break;
            }
            if (offset < len + h_size)  /* prolog */
            {
                skip = len + h_size - offset;
            }
            else if (offset >= func->FunctionLength - (len + 1))  /* epilog */
            {
                skip = offset - (func->FunctionLength - (len + 1));
                h_size = 0;
            }
        }
    }

    if (!skip)
    {
        if (func->CR == 3 || func->CR == 2)
        {
            /* mov x29,sp */
            context->Sp = context->Fp;
            restore_regs( 29, 2, 0, context, ptrs );
        }
        context->Sp += local_size;
        if (fp_size) restore_fpregs( 8, fp_regs, int_regs, context, ptrs );
        if (func->CR == 1) restore_regs( 30, 1, int_regs - 1, context, ptrs );
        restore_regs( 19, func->RegI, -saved_regs, context, ptrs );
    }
    else
    {
        unsigned int pos = 0;

        switch (func->CR)
        {
        case 3:
        case 2:
            /* mov x29,sp */
            if (pos++ >= skip) context->Sp = context->Fp;
            if (local_size <= 512)
            {
                /* stp x29,lr,[sp,-#local_size]! */
                if (pos++ >= skip) restore_regs( 29, 2, -local_size_regs, context, ptrs );
                break;
            }
            /* stp x29,lr,[sp,0] */
            if (pos++ >= skip) restore_regs( 29, 2, 0, context, ptrs );
            /* fall through */
        case 0:
        case 1:
            if (!local_size) break;
            /* sub sp,sp,#local_size */
            if (pos++ >= skip) context->Sp += (local_size - 1) % 4088 + 1;
            if (local_size > 4088 && pos++ >= skip) context->Sp += 4088;
            break;
        }

        pos += h_size;

        if (fp_size)
        {
            if (func->RegF % 2 == 0 && pos++ >= skip)
                /* str d%u,[sp,#fp_size] */
                restore_fpregs( 8 + func->RegF, 1, int_regs + fp_regs - 1, context, ptrs );
            for (i = (func->RegF + 1) / 2 - 1; i >= 0; i--)
            {
                if (pos++ < skip) continue;
                if (!i && !int_size)
                     /* stp d8,d9,[sp,-#regsave]! */
                    restore_fpregs( 8, 2, -saved_regs, context, ptrs );
                else
                     /* stp dn,dn+1,[sp,#offset] */
                    restore_fpregs( 8 + 2 * i, 2, int_regs + 2 * i, context, ptrs );
            }
        }

        if (func->RegI % 2)
        {
            if (pos++ >= skip)
            {
                /* stp xn,lr,[sp,#offset] */
                if (func->CR == 1) restore_regs( 30, 1, int_regs - 1, context, ptrs );
                /* str xn,[sp,#offset] */
                restore_regs( 18 + func->RegI, 1,
                              (func->RegI > 1) ? func->RegI - 1 : -saved_regs,
                              context, ptrs );
            }
        }
        else if (func->CR == 1)
        {
            /* str lr,[sp,#offset] */
            if (pos++ >= skip) restore_regs( 30, 1, func->RegI ? int_regs - 1 : -saved_regs, context, ptrs );
        }

        for (i = func->RegI / 2 - 1; i >= 0; i--)
        {
            if (pos++ < skip) continue;
            if (i)
                /* stp xn,xn+1,[sp,#offset] */
                restore_regs( 19 + 2 * i, 2, 2 * i, context, ptrs );
            else
                /* stp x19,x20,[sp,-#regsave]! */
                restore_regs( 19, 2, -saved_regs, context, ptrs );
        }
    }
    if (func->CR == 2) do_pac_auth( context );
    return NULL;
}

static void *unwind_full_data( ULONG_PTR base, ULONG_PTR pc, ARM64_RUNTIME_FUNCTION *func,
                               ARM64_NT_CONTEXT *context, PVOID *handler_data,
                               KNONVOLATILE_CONTEXT_POINTERS_ARM64 *ptrs,
                               BOOLEAN *final_pc_from_lr )
{
    IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY_XDATA *info;
    struct unwind_info_epilog *info_epilog;
    unsigned int i, codes, epilogs, len, offset;
    void *data;
    BYTE *end;

    info = (IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY_XDATA *)((char *)base + func->UnwindData);
    data = info + 1;
    epilogs = info->EpilogCount;
    codes = info->CodeWords;
    if (!codes && !epilogs)
    {
        struct unwind_info_ext *infoex = data;
        codes = infoex->codes;
        epilogs = infoex->epilog;
        data = infoex + 1;
    }
    info_epilog = data;
    if (!info->EpilogInHeader) data = info_epilog + epilogs;

    offset = ((pc - base) - func->BeginAddress) / 4;
    end = (BYTE *)data + codes * 4;

    TRACE( "function %I64x-%I64x: len=%#x ver=%u X=%u E=%u epilogs=%u codes=%u\n",
           base + func->BeginAddress, base + func->BeginAddress + info->FunctionLength * 4,
           info->FunctionLength, info->Version, info->ExceptionDataPresent, info->EpilogInHeader,
           epilogs, codes * 4 );

    /* check for prolog */
    if (offset < codes * 4)
    {
        len = get_sequence_len( data, end );
        if (offset < len)
        {
            process_unwind_codes( data, end, context, ptrs, len - offset, final_pc_from_lr );
            return NULL;
        }
    }

    /* check for epilog */
    if (!info->EpilogInHeader)
    {
        for (i = 0; i < epilogs; i++)
        {
            if (offset < info_epilog[i].offset) break;
            if (offset - info_epilog[i].offset < codes * 4 - info_epilog[i].index)
            {
                BYTE *ptr = (BYTE *)data + info_epilog[i].index;
                len = get_sequence_len( ptr, end );
                if (offset <= info_epilog[i].offset + len)
                {
                    process_unwind_codes( ptr, end, context, ptrs, offset - info_epilog[i].offset, final_pc_from_lr );
                    return NULL;
                }
            }
        }
    }
    else if (info->FunctionLength - offset <= codes * 4 - epilogs)
    {
        BYTE *ptr = (BYTE *)data + epilogs;
        len = get_sequence_len( ptr, end ) + 1;
        if (offset >= info->FunctionLength - len)
        {
            process_unwind_codes( ptr, end, context, ptrs, offset - (info->FunctionLength - len), final_pc_from_lr );
            return NULL;
        }
    }

    process_unwind_codes( data, end, context, ptrs, 0, final_pc_from_lr );

    /* get handler since we are inside the main code */
    if (info->ExceptionDataPresent)
    {
        DWORD *handler_rva = (DWORD *)data + codes;
        *handler_data = handler_rva + 1;
        return (char *)base + *handler_rva;
    }
    return NULL;
}

static ARM64_RUNTIME_FUNCTION *find_function_info_arm64( ULONG_PTR pc, ULONG_PTR base,
                                                         ARM64_RUNTIME_FUNCTION *func, ULONG count )
{
    int min = 0;
    int max = count - 1;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        ULONG_PTR start = base + func[pos].BeginAddress;

        if (pc >= start)
        {
            ULONG len = func[pos].Flag ? func[pos].FunctionLength :
                ((IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY_XDATA *)(base + func[pos].UnwindData))->FunctionLength;
            if (pc < start + 4 * len) return func + pos;
            min = pos + 1;
        }
        else max = pos - 1;
    }
    return NULL;
}

#ifdef __arm64ec__
#define RtlVirtualUnwind RtlVirtualUnwind_arm64
#define RtlVirtualUnwind2 RtlVirtualUnwind2_arm64
#define RtlLookupFunctionEntry RtlLookupFunctionEntry_arm64
#define __C_specific_handler __C_specific_handler_arm64
#endif

/**********************************************************************
 *              RtlVirtualUnwind2   (NTDLL.@)
 */
NTSTATUS WINAPI RtlVirtualUnwind2( ULONG type, ULONG_PTR base, ULONG_PTR pc,
                                   ARM64_RUNTIME_FUNCTION *func, ARM64_NT_CONTEXT *context,
                                   BOOLEAN *mach_frame_unwound, void **handler_data,
                                   ULONG_PTR *frame_ret, KNONVOLATILE_CONTEXT_POINTERS_ARM64 *ctx_ptr,
                                   ULONG_PTR *limit_low, ULONG_PTR *limit_high,
                                   PEXCEPTION_ROUTINE *handler_ret, ULONG flags )
{
    BOOLEAN final_pc_from_lr = TRUE;
    TRACE( "type %lx base %I64x pc %I64x rva %I64x sp %I64x\n", type, base, pc, pc - base, context->Sp );
    if (limit_low || limit_high) FIXME( "limits not supported\n" );

    if (!func && pc == context->Lr) return STATUS_BAD_FUNCTION_TABLE;  /* invalid leaf function */

    *handler_data = NULL;
    context->ContextFlags |= CONTEXT_UNWOUND_TO_CALL;

    if (!func)  /* leaf function */
        *handler_ret = NULL;
    else if (func->Flag)
        *handler_ret = unwind_packed_data( base, pc, func, context, ctx_ptr );
    else
        *handler_ret = unwind_full_data( base, pc, func, context, handler_data, ctx_ptr, &final_pc_from_lr );

    if (final_pc_from_lr) context->Pc = context->Lr;

    TRACE( "ret: pc=%I64x lr=%I64x sp=%I64x handler=%p\n", context->Pc, context->Lr, context->Sp, *handler_ret );
    *frame_ret = context->Sp;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *              RtlVirtualUnwind   (NTDLL.@)
 */
PEXCEPTION_ROUTINE WINAPI RtlVirtualUnwind( ULONG type, ULONG_PTR base, ULONG_PTR pc,
                                            ARM64_RUNTIME_FUNCTION *func, ARM64_NT_CONTEXT *context,
                                            PVOID *handler_data, ULONG_PTR *frame_ret,
                                            KNONVOLATILE_CONTEXT_POINTERS_ARM64 *ctx_ptr )
{
    PEXCEPTION_ROUTINE handler;

    if (!RtlVirtualUnwind2( type, base, pc, func, context, NULL, handler_data,
                           frame_ret, ctx_ptr, NULL, NULL, &handler, 0 ))
        return handler;

    context->Pc = 0;
    return NULL;
}


/*******************************************************************
 *		__C_specific_handler (NTDLL.@)
 */
EXCEPTION_DISPOSITION WINAPI __C_specific_handler( EXCEPTION_RECORD *rec, void *frame,
                                                   ARM64_NT_CONTEXT *context,
                                                   DISPATCHER_CONTEXT_ARM64 *dispatch )
{
    const SCOPE_TABLE *table = dispatch->HandlerData;
    ULONG_PTR base = dispatch->ImageBase;
    ULONG_PTR pc = dispatch->ControlPc;
    unsigned int i;
    void *handler;

    TRACE( "%p %p %p %p pc %Ix\n", rec, frame, context, dispatch, pc );
    if (TRACE_ON(unwind)) DUMP_SCOPE_TABLE( base, table );

    if (dispatch->ControlPcIsUnwound) pc -= 4;

    if (rec->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND))
    {
        for (i = dispatch->ScopeIndex; i < table->Count; i++)
        {
            if (pc < base + table->ScopeRecord[i].BeginAddress) continue;
            if (pc >= base + table->ScopeRecord[i].EndAddress) continue;
            if (table->ScopeRecord[i].JumpTarget) continue;

            if (rec->ExceptionFlags & EXCEPTION_TARGET_UNWIND &&
                dispatch->TargetPc >= base + table->ScopeRecord[i].BeginAddress &&
                dispatch->TargetPc < base + table->ScopeRecord[i].EndAddress)
            {
                break;
            }
            handler = (void *)(base + table->ScopeRecord[i].HandlerAddress);
            dispatch->ScopeIndex = i + 1;
            TRACE( "scope %u calling __finally %p frame %p\n", i, handler, frame );
            __C_ExecuteExceptionFilter( ULongToPtr(TRUE), frame, handler, dispatch->NonVolatileRegisters );
        }
    }
    else
    {
        for (i = dispatch->ScopeIndex; i < table->Count; i++)
        {
            if (pc < base + table->ScopeRecord[i].BeginAddress) continue;
            if (pc >= base + table->ScopeRecord[i].EndAddress) continue;
            if (!table->ScopeRecord[i].JumpTarget) continue;

            if (table->ScopeRecord[i].HandlerAddress != EXCEPTION_EXECUTE_HANDLER)
            {
                EXCEPTION_POINTERS ptrs = { rec, (CONTEXT *)context };

                handler = (void *)(base + table->ScopeRecord[i].HandlerAddress);
                TRACE( "scope %u calling filter %p ptrs %p frame %p\n", i, handler, &ptrs, frame );
                switch (__C_ExecuteExceptionFilter( &ptrs, frame, handler, dispatch->NonVolatileRegisters ))
                {
                case EXCEPTION_EXECUTE_HANDLER:
                    break;
                case EXCEPTION_CONTINUE_SEARCH:
                    continue;
                case EXCEPTION_CONTINUE_EXECUTION:
                    return ExceptionContinueExecution;
                }
            }
            TRACE( "unwinding to target %Ix\n", base + table->ScopeRecord[i].JumpTarget );
            RtlUnwindEx( frame, (char *)base + table->ScopeRecord[i].JumpTarget,
                         rec, ULongToPtr(rec->ExceptionCode), (CONTEXT *)dispatch->ContextRecord,
                         dispatch->HistoryTable );
        }
    }
    return ExceptionContinueSearch;
}


/**********************************************************************
 *              RtlLookupFunctionEntry   (NTDLL.@)
 */
PARM64_RUNTIME_FUNCTION WINAPI RtlLookupFunctionEntry( ULONG_PTR pc, ULONG_PTR *base,
                                                       UNWIND_HISTORY_TABLE *table )
{
    ARM64_RUNTIME_FUNCTION *func;
    ULONG_PTR dynbase;
    ULONG size;

    if ((func = (ARM64_RUNTIME_FUNCTION *)RtlLookupFunctionTable( pc, base, &size )))
        return find_function_info_arm64( pc, *base, func, size / sizeof(*func));

    if ((func = (ARM64_RUNTIME_FUNCTION *)lookup_dynamic_function_table( pc, &dynbase, &size )))
    {
        ARM64_RUNTIME_FUNCTION *ret = find_function_info_arm64( pc, dynbase, func, size );
        if (ret) *base = dynbase;
        return ret;
    }

    *base = 0;
    return NULL;
}


/**********************************************************************
 *              RtlAddFunctionTable   (NTDLL.@)
 */
#ifndef __arm64ec__
BOOLEAN CDECL RtlAddFunctionTable( RUNTIME_FUNCTION *table, DWORD count, ULONG_PTR base )
{
    ULONG_PTR end = base;
    void *ret;

    if (count)
    {
        RUNTIME_FUNCTION *func = table + count - 1;
        ULONG len = func->Flag ? func->FunctionLength :
            ((IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY_XDATA *)(base + func->UnwindData))->FunctionLength;
        end += func->BeginAddress + 4 * len;
    }
    return !RtlAddGrowableFunctionTable( &ret, table, count, 0, base, end );
}
#else
#undef RtlVirtualUnwind
#undef RtlVirtualUnwind2
#undef RtlLookupFunctionEntry
#undef __C_specific_handler
#endif

#endif  /* __aarch64__ */


/***********************************************************************
 * ARM support
 */
#ifdef __arm__

struct unwind_info
{
    DWORD function_length : 18;
    DWORD version : 2;
    DWORD x : 1;
    DWORD e : 1;
    DWORD f : 1;
    DWORD epilog : 5;
    DWORD codes : 4;
};

struct unwind_info_ext
{
    WORD epilog;
    BYTE codes;
    BYTE reserved;
};

struct unwind_info_epilog
{
    DWORD offset : 18;
    DWORD res : 2;
    DWORD cond : 4;
    DWORD index : 8;
};

static const BYTE unwind_code_len[256] =
{
/* 00 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 20 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 40 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 60 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 80 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* a0 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* c0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* e0 */ 1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,4,3,4,1,1,1,1,1
};

static const BYTE unwind_instr_len[256] =
{
/* 00 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* 20 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* 40 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* 60 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* 80 */ 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
/* a0 */ 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
/* c0 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,4,4,4,4,4,4,4,4,
/* e0 */ 4,4,4,4,4,4,4,4,4,4,4,4,2,2,0,4,0,0,0,0,0,4,4,2,2,4,4,2,4,2,4,0
};

static unsigned int get_sequence_len( BYTE *ptr, BYTE *end, int include_end )
{
    unsigned int ret = 0;

    while (ptr < end)
    {
        if (*ptr >= 0xfd)
        {
            if (*ptr <= 0xfe && include_end)
                ret += unwind_instr_len[*ptr];
            break;
        }
        ret += unwind_instr_len[*ptr];
        ptr += unwind_code_len[*ptr];
    }
    return ret;
}

static void pop_regs_mask( int mask, CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    int i;
    for (i = 0; i <= 12; i++)
    {
        if (!(mask & (1 << i))) continue;
        if (ptrs && i >= 4 && i <= 11) (&ptrs->R4)[i - 4] = (DWORD *)context->Sp;
        if (i >= 4) (&context->R0)[i] = *(DWORD *)context->Sp;
        context->Sp += 4;
    }
}

static void pop_regs_range( int last, CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    int i;
    for (i = 4; i <= last; i++)
    {
        if (ptrs) (&ptrs->R4)[i - 4] = (DWORD *)context->Sp;
        (&context->R0)[i] = *(DWORD *)context->Sp;
        context->Sp += 4;
    }
}

static void pop_lr( int increment, CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    if (ptrs) ptrs->Lr = (DWORD *)context->Sp;
    context->Lr = *(DWORD *)context->Sp;
    context->Sp += increment;
}

static void pop_fpregs_range( int first, int last, CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    int i;
    for (i = first; i <= last; i++)
    {
        if (ptrs && i >= 8 && i <= 15) (&ptrs->D8)[i - 8] = (ULONGLONG *)context->Sp;
        context->D[i] = *(ULONGLONG *)context->Sp;
        context->Sp += 8;
    }
}

static void ms_opcode( BYTE opcode, CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    switch (opcode)
    {
    case 1:  /* MSFT_OP_MACHINE_FRAME */
        context->Pc = ((DWORD *)context->Sp)[1];
        context->Sp = ((DWORD *)context->Sp)[0];
        context->ContextFlags &= ~CONTEXT_UNWOUND_TO_CALL;
        break;
    case 2:  /* MSFT_OP_CONTEXT */
    {
        int i;
        CONTEXT *src = (CONTEXT *)context->Sp;

        *context = *src;
        if (!ptrs) break;
        for (i = 0; i < 8; i++) (&ptrs->R4)[i] = &src->R4 + i;
        ptrs->Lr = &src->Lr;
        for (i = 0; i < 8; i++) (&ptrs->D8)[i] = &src->D[i + 8];
        break;
    }
    default:
        WARN( "unsupported code %02x\n", opcode );
        break;
    }
}

static void process_unwind_codes( BYTE *ptr, BYTE *end, CONTEXT *context,
                                  KNONVOLATILE_CONTEXT_POINTERS *ptrs, int skip )
{
    unsigned int val, len;
    unsigned int i;

    /* skip codes */
    while (ptr < end && skip)
    {
        if (*ptr >= 0xfd) break;
        skip -= unwind_instr_len[*ptr];
        ptr += unwind_code_len[*ptr];
    }

    while (ptr < end)
    {
        len = unwind_code_len[*ptr];
        if (ptr + len > end) break;
        val = 0;
        for (i = 0; i < len; i++)
            val = (val << 8) | ptr[i];

        if (*ptr <= 0x7f)      /* add sp, sp, #x */
            context->Sp += 4 * (val & 0x7f);
        else if (*ptr <= 0xbf) /* pop {r0-r12,lr} */
        {
            pop_regs_mask( val & 0x1fff, context, ptrs );
            if (val & 0x2000)
                pop_lr( 4, context, ptrs );
        }
        else if (*ptr <= 0xcf) /* mov sp, rX */
            context->Sp = (&context->R0)[val & 0x0f];
        else if (*ptr <= 0xd7) /* pop {r4-rX,lr} */
        {
            pop_regs_range( (val & 0x03) + 4, context, ptrs );
            if (val & 0x04)
                pop_lr( 4, context, ptrs );
        }
        else if (*ptr <= 0xdf) /* pop {r4-rX,lr} */
        {
            pop_regs_range( (val & 0x03) + 8, context, ptrs );
            if (val & 0x04)
                pop_lr( 4, context, ptrs );
        }
        else if (*ptr <= 0xe7) /* vpop {d8-dX} */
            pop_fpregs_range( 8, (val & 0x07) + 8, context, ptrs );
        else if (*ptr <= 0xeb) /* add sp, sp, #x */
            context->Sp += 4 * (val & 0x3ff);
        else if (*ptr <= 0xed) /* pop {r0-r12,lr} */
        {
            pop_regs_mask( val & 0xff, context, ptrs );
            if (val & 0x100)
                pop_lr( 4, context, ptrs );
        }
        else if (*ptr <= 0xee) /* Microsoft-specific 0x00-0x0f, Available 0x10-0xff */
            ms_opcode( val & 0xff, context, ptrs );
        else if (*ptr <= 0xef && ((val & 0xff) <= 0x0f)) /* ldr lr, [sp], #x */
            pop_lr( 4 * (val & 0x0f), context, ptrs );
        else if (*ptr <= 0xf4) /* Available */
            WARN( "unsupported code %02x\n", *ptr );
        else if (*ptr <= 0xf5) /* vpop {dS-dE} */
            pop_fpregs_range( (val & 0xf0) >> 4, (val & 0x0f), context, ptrs );
        else if (*ptr <= 0xf6) /* vpop {dS-dE} */
            pop_fpregs_range( ((val & 0xf0) >> 4) + 16, (val & 0x0f) + 16, context, ptrs );
        else if (*ptr == 0xf7 || *ptr == 0xf9) /* add sp, sp, #x */
            context->Sp += 4 * (val & 0xffff);
        else if (*ptr == 0xf8 || *ptr == 0xfa) /* add sp, sp, #x */
            context->Sp += 4 * (val & 0xffffff);
        else if (*ptr <= 0xfc)  /* nop */
            /* nop */ ;
        else                    /* end */
            break;

        ptr += len;
    }
}

static void *unwind_packed_data( ULONG_PTR base, ULONG_PTR pc, RUNTIME_FUNCTION *func,
                                 CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    int i, pos = 0;
    int pf = 0, ef = 0, fpoffset = 0, stack = func->StackAdjust;
    int prologue_regmask = 0;
    int epilogue_regmask = 0;
    unsigned int offset, len;
    BYTE prologue[10], *prologue_end, epilogue[20], *epilogue_end;

    TRACE( "function %lx-%lx: len=%#x flag=%x ret=%u H=%u reg=%u R=%u L=%u C=%u stackadjust=%x\n",
           base + func->BeginAddress, base + func->BeginAddress + func->FunctionLength * 2,
           func->FunctionLength, func->Flag, func->Ret,
           func->H, func->Reg, func->R, func->L, func->C, func->StackAdjust );

    offset = (pc - base) - func->BeginAddress;
    if (func->StackAdjust >= 0x03f4)
    {
        pf = func->StackAdjust & 0x04;
        ef = func->StackAdjust & 0x08;
        stack = (func->StackAdjust & 3) + 1;
    }

    if (!func->R || pf)
    {
        int first = 4, last = func->Reg + 4;
        if (pf)
        {
            first = (~func->StackAdjust) & 3;
            if (func->R)
                last = 3;
        }
        for (i = first; i <= last; i++)
            prologue_regmask |= 1 << i;
        fpoffset = last + 1 - first;
    }

    if (!func->R || ef)
    {
        int first = 4, last = func->Reg + 4;
        if (ef)
        {
            first = (~func->StackAdjust) & 3;
            if (func->R)
                last = 3;
        }
        for (i = first; i <= last; i++)
            epilogue_regmask |= 1 << i;
    }

    if (func->C)
    {
        prologue_regmask |= 1 << 11;
        epilogue_regmask |= 1 << 11;
    }

    if (func->L)
    {
        prologue_regmask |= 1 << 14; /* lr */
        if (func->Ret != 0)
            epilogue_regmask |= 1 << 14; /* lr */
        else if (!func->H)
            epilogue_regmask |= 1 << 15; /* pc */
    }

    /* Synthesize prologue opcodes */
    if (stack && !pf)
    {
        if (stack <= 0x7f)
        {
            prologue[pos++] = stack; /* sub sp, sp, #x */
        }
        else
        {
            prologue[pos++] = 0xe8 | (stack >> 8); /* sub.w sp, sp, #x */
            prologue[pos++] = stack & 0xff;
        }
    }

    if (func->R && func->Reg != 7)
        prologue[pos++] = 0xe0 | func->Reg; /* vpush {d8-dX} */

    if (func->C && fpoffset == 0)
        prologue[pos++] = 0xfb; /* mov r11, sp - handled as nop16 */
    else if (func->C)
        prologue[pos++] = 0xfc; /* add r11, sp, #x - handled as nop32 */

    if (prologue_regmask & 0xf00) /* r8-r11 set */
    {
        int bitmask = prologue_regmask & 0x1fff;
        if (prologue_regmask & (1 << 14)) /* lr */
            bitmask |= 0x2000;
        prologue[pos++] = 0x80 | (bitmask >> 8); /* push.w {r0-r12,lr} */
        prologue[pos++] = bitmask & 0xff;
    }
    else if (prologue_regmask) /* r0-r7, lr set */
    {
        int bitmask = prologue_regmask & 0xff;
        if (prologue_regmask & (1 << 14)) /* lr */
            bitmask |= 0x100;
        prologue[pos++] = 0xec | (bitmask >> 8); /* push {r0-r7,lr} */
        prologue[pos++] = bitmask & 0xff;
    }

    if (func->H)
        prologue[pos++] = 0x04; /* push {r0-r3} - handled as sub sp, sp, #16 */

    prologue[pos++] = 0xff; /* end */
    prologue_end = &prologue[pos];

    /* Synthesize epilogue opcodes */
    pos = 0;
    if (stack && !ef)
    {
        if (stack <= 0x7f)
        {
            epilogue[pos++] = stack; /* sub sp, sp, #x */
        }
        else
        {
            epilogue[pos++] = 0xe8 | (stack >> 8); /* sub.w sp, sp, #x */
            epilogue[pos++] = stack & 0xff;
        }
    }

    if (func->R && func->Reg != 7)
        epilogue[pos++] = 0xe0 | func->Reg; /* vpush {d8-dX} */

    if (epilogue_regmask & 0x7f00) /* r8-r11, lr set */
    {
        int bitmask = epilogue_regmask & 0x1fff;
        if (epilogue_regmask & (3 << 14)) /* lr or pc */
            bitmask |= 0x2000;
        epilogue[pos++] = 0x80 | (bitmask >> 8); /* push.w {r0-r12,lr} */
        epilogue[pos++] = bitmask & 0xff;
    }
    else if (epilogue_regmask) /* r0-r7, pc set */
    {
        int bitmask = epilogue_regmask & 0xff;
        if (epilogue_regmask & (1 << 15)) /* pc */
            bitmask |= 0x100; /* lr */
        epilogue[pos++] = 0xec | (bitmask >> 8); /* push {r0-r7,lr} */
        epilogue[pos++] = bitmask & 0xff;
    }

    if (func->H && !(func->L && func->Ret == 0))
        epilogue[pos++] = 0x04; /* add sp, sp, #16 */
    else if (func->H && (func->L && func->Ret == 0))
    {
        epilogue[pos++] = 0xef; /* ldr lr, [sp], #20 */
        epilogue[pos++] = 5;
    }

    if (func->Ret == 1)
        epilogue[pos++] = 0xfd; /* bx lr */
    else if (func->Ret == 2)
        epilogue[pos++] = 0xfe; /* b address */
    else
        epilogue[pos++] = 0xff; /* end */
    epilogue_end = &epilogue[pos];

    if (func->Flag == 1 && offset < 4 * (prologue_end - prologue)) {
        /* Check prologue */
        len = get_sequence_len( prologue, prologue_end, 0 );
        if (offset < len)
        {
            process_unwind_codes( prologue, prologue_end, context, ptrs, len - offset );
            return NULL;
        }
    }

    if (func->Ret != 3 && 2 * func->FunctionLength - offset <= 4 * (epilogue_end - epilogue)) {
        /* Check epilogue */
        len = get_sequence_len( epilogue, epilogue_end, 1 );
        if (offset >= 2 * func->FunctionLength - len)
        {
            process_unwind_codes( epilogue, epilogue_end, context, ptrs, offset - (2 * func->FunctionLength - len) );
            return NULL;
        }
    }

    /* Execute full prologue */
    process_unwind_codes( prologue, prologue_end, context, ptrs, 0 );

    return NULL;
}

static void *unwind_full_data( ULONG_PTR base, ULONG_PTR pc, RUNTIME_FUNCTION *func,
                               CONTEXT *context, PVOID *handler_data, KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    struct unwind_info *info;
    struct unwind_info_epilog *info_epilog;
    unsigned int i, codes, epilogs, len, offset;
    void *data;
    BYTE *end;

    info = (struct unwind_info *)((char *)base + func->UnwindData);
    data = info + 1;
    epilogs = info->epilog;
    codes = info->codes;
    if (!codes && !epilogs)
    {
        struct unwind_info_ext *infoex = data;
        codes = infoex->codes;
        epilogs = infoex->epilog;
        data = infoex + 1;
    }
    info_epilog = data;
    if (!info->e) data = info_epilog + epilogs;

    offset = (pc - base) - func->BeginAddress;
    end = (BYTE *)data + codes * 4;

    TRACE( "function %lx-%lx: len=%#x ver=%u X=%u E=%u F=%u epilogs=%u codes=%u\n",
           base + func->BeginAddress, base + func->BeginAddress + info->function_length * 2,
           info->function_length, info->version, info->x, info->e, info->f, epilogs, codes * 4 );

    /* check for prolog */
    if (offset < codes * 4 * 4 && !info->f)
    {
        len = get_sequence_len( data, end, 0 );
        if (offset < len)
        {
            process_unwind_codes( data, end, context, ptrs, len - offset );
            return NULL;
        }
    }

    /* check for epilog */
    if (!info->e)
    {
        for (i = 0; i < epilogs; i++)
        {
            /* TODO: Currently not checking epilogue conditions. */
            if (offset < 2 * info_epilog[i].offset) break;
            if (offset - 2 * info_epilog[i].offset < (codes * 4 - info_epilog[i].index) * 4)
            {
                BYTE *ptr = (BYTE *)data + info_epilog[i].index;
                len = get_sequence_len( ptr, end, 1 );
                if (offset <= 2 * info_epilog[i].offset + len)
                {
                    process_unwind_codes( ptr, end, context, ptrs, offset - 2 * info_epilog[i].offset );
                    return NULL;
                }
            }
        }
    }
    else if (2 * info->function_length - offset <= (codes * 4 - epilogs) * 4)
    {
        BYTE *ptr = (BYTE *)data + epilogs;
        len = get_sequence_len( ptr, end, 1 );
        if (offset >= 2 * info->function_length - len)
        {
            process_unwind_codes( ptr, end, context, ptrs, offset - (2 * info->function_length - len) );
            return NULL;
        }
    }

    process_unwind_codes( data, end, context, ptrs, 0 );

    /* get handler since we are inside the main code */
    if (info->x)
    {
        DWORD *handler_rva = (DWORD *)data + codes;
        *handler_data = handler_rva + 1;
        return (char *)base + *handler_rva;
    }
    return NULL;
}

static RUNTIME_FUNCTION *find_function_info( ULONG_PTR pc, ULONG_PTR base,
                                             RUNTIME_FUNCTION *func, ULONG count )
{
    int min = 0;
    int max = count - 1;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        ULONG_PTR start = base + (func[pos].BeginAddress & ~1);

        if (pc >= start)
        {
            struct unwind_info *info = (struct unwind_info *)(base + func[pos].UnwindData);
            if (pc < start + 2 * (func[pos].Flag ? func[pos].FunctionLength : info->function_length))
                return func + pos;
            min = pos + 1;
        }
        else max = pos - 1;
    }
    return NULL;
}


/**********************************************************************
 *              RtlVirtualUnwind2   (NTDLL.@)
 */
NTSTATUS WINAPI RtlVirtualUnwind2( ULONG type, ULONG_PTR base, ULONG_PTR pc,
                                   RUNTIME_FUNCTION *func, CONTEXT *context,
                                   BOOLEAN *mach_frame_unwound, void **handler_data,
                                   ULONG_PTR *frame_ret, KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr,
                                   ULONG_PTR *limit_low, ULONG_PTR *limit_high,
                                   PEXCEPTION_ROUTINE *handler_ret, ULONG flags )
{
    TRACE( "type %lx base %Ix pc %Ix rva %Ix sp %lx\n", type, base, pc, pc - base, context->Sp );
    if (limit_low || limit_high) FIXME( "limits not supported\n" );

    context->Pc = 0;

    if (!func && pc == context->Lr) return STATUS_BAD_FUNCTION_TABLE;  /* invalid leaf function */

    *handler_data = NULL;

    if (!func)  /* leaf function */
        *handler_ret = NULL;
    else if (func->Flag)
        *handler_ret = unwind_packed_data( base, pc, func, context, ctx_ptr );
    else
        *handler_ret = unwind_full_data( base, pc, func, context, handler_data, ctx_ptr );

    TRACE( "ret: pc=%lx lr=%lx sp=%lx handler=%p\n", context->Pc, context->Lr, context->Sp, *handler_ret );
    if (!context->Pc)
    {
        context->Pc = context->Lr;
        context->ContextFlags |= CONTEXT_UNWOUND_TO_CALL;
    }
    *frame_ret = context->Sp;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *            RtlVirtualUnwind  (NTDLL.@)
 */
PEXCEPTION_ROUTINE WINAPI RtlVirtualUnwind( ULONG type, ULONG_PTR base, ULONG_PTR pc,
                                            RUNTIME_FUNCTION *func, CONTEXT *context,
                                            PVOID *handler_data, ULONG_PTR *frame_ret,
                                            KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr )
{
    PEXCEPTION_ROUTINE handler;

    if (!RtlVirtualUnwind2( type, base, pc, func, context, NULL, handler_data,
                           frame_ret, ctx_ptr, NULL, NULL, &handler, 0 ))
        return handler;

    context->Pc = 0;
    return NULL;
}


/*******************************************************************
 *              __C_specific_handler  (NTDLL.@)
 */
EXCEPTION_DISPOSITION WINAPI __C_specific_handler( EXCEPTION_RECORD *rec, void *frame,
                                                   CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    const SCOPE_TABLE *table = dispatch->HandlerData;
    ULONG_PTR base = dispatch->ImageBase;
    ULONG_PTR pc = dispatch->ControlPc;
    unsigned int i;
    void *handler;

    TRACE( "%p %p %p %p pc %Ix\n", rec, frame, context, dispatch, pc );
    if (TRACE_ON(unwind)) DUMP_SCOPE_TABLE( base, table );

    if (dispatch->ControlPcIsUnwound) pc -= 2;

    if (rec->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND))
    {
        for (i = dispatch->ScopeIndex; i < table->Count; i++)
        {
            if (pc < base + table->ScopeRecord[i].BeginAddress) continue;
            if (pc >= base + table->ScopeRecord[i].EndAddress) continue;
            if (table->ScopeRecord[i].JumpTarget) continue;

            if (rec->ExceptionFlags & EXCEPTION_TARGET_UNWIND &&
                dispatch->TargetPc >= base + table->ScopeRecord[i].BeginAddress &&
                dispatch->TargetPc < base + table->ScopeRecord[i].EndAddress)
            {
                break;
            }
            handler = (void *)(base + table->ScopeRecord[i].HandlerAddress);
            dispatch->ScopeIndex = i + 1;
            TRACE( "scope %u calling __finally %p frame %p\n", i, handler, frame );
            __C_ExecuteExceptionFilter( ULongToPtr(TRUE), frame, handler, dispatch->NonVolatileRegisters );
        }
    }
    else
    {
        for (i = dispatch->ScopeIndex; i < table->Count; i++)
        {
            if (pc < base + table->ScopeRecord[i].BeginAddress) continue;
            if (pc >= base + table->ScopeRecord[i].EndAddress) continue;
            if (!table->ScopeRecord[i].JumpTarget) continue;

            if (table->ScopeRecord[i].HandlerAddress != EXCEPTION_EXECUTE_HANDLER)
            {
                EXCEPTION_POINTERS ptrs = { rec, context };

                handler = (void *)(base + table->ScopeRecord[i].HandlerAddress);
                TRACE( "scope %u calling filter %p ptrs %p frame %p\n", i, handler, &ptrs, frame );
                switch (__C_ExecuteExceptionFilter( &ptrs, frame, handler, dispatch->NonVolatileRegisters ))
                {
                case EXCEPTION_EXECUTE_HANDLER:
                    break;
                case EXCEPTION_CONTINUE_SEARCH:
                    continue;
                case EXCEPTION_CONTINUE_EXECUTION:
                    return ExceptionContinueExecution;
                }
            }
            TRACE( "unwinding to target %lx\n", base + table->ScopeRecord[i].JumpTarget );
            RtlUnwindEx( frame, (char *)base + table->ScopeRecord[i].JumpTarget,
                         rec, ULongToPtr(rec->ExceptionCode), dispatch->ContextRecord,
                         dispatch->HistoryTable );
        }
    }
    return ExceptionContinueSearch;
}


/**********************************************************************
 *              RtlLookupFunctionEntry   (NTDLL.@)
 */
PRUNTIME_FUNCTION WINAPI RtlLookupFunctionEntry( ULONG_PTR pc, ULONG_PTR *base,
                                                 UNWIND_HISTORY_TABLE *table )
{
    RUNTIME_FUNCTION *func;
    ULONG_PTR dynbase;
    ULONG size;

    if ((func = RtlLookupFunctionTable( pc, base, &size )))
        return find_function_info( pc, *base, func, size / sizeof(*func));

    if ((func = lookup_dynamic_function_table( pc, &dynbase, &size )))
    {
        RUNTIME_FUNCTION *ret = find_function_info( pc, dynbase, func, size );
        if (ret) *base = dynbase;
        return ret;
    }

    *base = 0;
    return NULL;
}


/**********************************************************************
 *              RtlAddFunctionTable   (NTDLL.@)
 */
BOOLEAN CDECL RtlAddFunctionTable( RUNTIME_FUNCTION *table, DWORD count, ULONG_PTR base )
{
    ULONG_PTR end = base;
    void *ret;

    if (count)
    {
        RUNTIME_FUNCTION *func = table + count - 1;
        struct unwind_info *info = (struct unwind_info *)(base + func->UnwindData);
        end += func->BeginAddress + 2 * (func->Flag ? func->FunctionLength : info->function_length);
    }
    return !RtlAddGrowableFunctionTable( &ret, table, count, 0, base, end );
}

#endif  /* __arm__ */


/***********************************************************************
 * x86-64 support
 */
#ifdef __x86_64__

union handler_data
{
    RUNTIME_FUNCTION chain;
    ULONG handler;
};

struct opcode
{
    BYTE offset;
    BYTE code : 4;
    BYTE info : 4;
};

struct UNWIND_INFO
{
    BYTE version : 3;
    BYTE flags : 5;
    BYTE prolog;
    BYTE count;
    BYTE frame_reg : 4;
    BYTE frame_offset : 4;
    struct opcode opcodes[1];  /* info->count entries */
    /* followed by handler_data */
};

#define UWOP_PUSH_NONVOL     0
#define UWOP_ALLOC_LARGE     1
#define UWOP_ALLOC_SMALL     2
#define UWOP_SET_FPREG       3
#define UWOP_SAVE_NONVOL     4
#define UWOP_SAVE_NONVOL_FAR 5
#define UWOP_EPILOG          6
#define UWOP_SAVE_XMM128     8
#define UWOP_SAVE_XMM128_FAR 9
#define UWOP_PUSH_MACHFRAME  10

static void dump_unwind_info( ULONG64 base, RUNTIME_FUNCTION *function )
{
    static const char * const reg_names[16] =
        { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
          "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15" };

    union handler_data *handler_data;
    struct UNWIND_INFO *info;
    unsigned int i, count;

    TRACE( "**** func %lx-%lx\n", function->BeginAddress, function->EndAddress );
    for (;;)
    {
        if (function->UnwindData & 1)
        {
            RUNTIME_FUNCTION *next = (RUNTIME_FUNCTION *)((char *)base + (function->UnwindData & ~1));
            TRACE( "unwind info for function %p-%p chained to function %p-%p\n",
                   (char *)base + function->BeginAddress, (char *)base + function->EndAddress,
                   (char *)base + next->BeginAddress, (char *)base + next->EndAddress );
            function = next;
            continue;
        }
        info = (struct UNWIND_INFO *)((char *)base + function->UnwindData);

        TRACE( "unwind info at %p flags %x prolog 0x%x bytes function %p-%p\n",
               info, info->flags, info->prolog,
               (char *)base + function->BeginAddress, (char *)base + function->EndAddress );

        if (info->frame_reg)
            TRACE( "    frame register %s offset 0x%x(%%rsp)\n",
                   reg_names[info->frame_reg], info->frame_offset * 16 );

        for (i = 0; i < info->count; i++)
        {
            TRACE( "    0x%x: ", info->opcodes[i].offset );
            switch (info->opcodes[i].code)
            {
            case UWOP_PUSH_NONVOL:
                TRACE( "pushq %%%s\n", reg_names[info->opcodes[i].info] );
                break;
            case UWOP_ALLOC_LARGE:
                if (info->opcodes[i].info)
                {
                    count = *(DWORD *)&info->opcodes[i+1];
                    i += 2;
                }
                else
                {
                    count = *(USHORT *)&info->opcodes[i+1] * 8;
                    i++;
                }
                TRACE( "subq $0x%x,%%rsp\n", count );
                break;
            case UWOP_ALLOC_SMALL:
                count = (info->opcodes[i].info + 1) * 8;
                TRACE( "subq $0x%x,%%rsp\n", count );
                break;
            case UWOP_SET_FPREG:
                TRACE( "leaq 0x%x(%%rsp),%s\n",
                     info->frame_offset * 16, reg_names[info->frame_reg] );
                break;
            case UWOP_SAVE_NONVOL:
                count = *(USHORT *)&info->opcodes[i+1] * 8;
                TRACE( "movq %%%s,0x%x(%%rsp)\n", reg_names[info->opcodes[i].info], count );
                i++;
                break;
            case UWOP_SAVE_NONVOL_FAR:
                count = *(DWORD *)&info->opcodes[i+1];
                TRACE( "movq %%%s,0x%x(%%rsp)\n", reg_names[info->opcodes[i].info], count );
                i += 2;
                break;
            case UWOP_SAVE_XMM128:
                count = *(USHORT *)&info->opcodes[i+1] * 16;
                TRACE( "movaps %%xmm%u,0x%x(%%rsp)\n", info->opcodes[i].info, count );
                i++;
                break;
            case UWOP_SAVE_XMM128_FAR:
                count = *(DWORD *)&info->opcodes[i+1];
                TRACE( "movaps %%xmm%u,0x%x(%%rsp)\n", info->opcodes[i].info, count );
                i += 2;
                break;
            case UWOP_PUSH_MACHFRAME:
                TRACE( "PUSH_MACHFRAME %u\n", info->opcodes[i].info );
                break;
            case UWOP_EPILOG:
                if (info->version == 2)
                {
                    unsigned int offset;
                    if (info->opcodes[i].info)
                        offset = info->opcodes[i].offset;
                    else
                        offset = (info->opcodes[i+1].info << 8) + info->opcodes[i+1].offset;
                    TRACE("epilog %p-%p\n", (char *)base + function->EndAddress - offset,
                            (char *)base + function->EndAddress - offset + info->opcodes[i].offset );
                    i += 1;
                    break;
                }
            default:
                FIXME( "unknown code %u\n", info->opcodes[i].code );
                break;
            }
        }

        handler_data = (union handler_data *)&info->opcodes[(info->count + 1) & ~1];
        if (info->flags & UNW_FLAG_CHAININFO)
        {
            TRACE( "    chained to function %p-%p\n",
                   (char *)base + handler_data->chain.BeginAddress,
                   (char *)base + handler_data->chain.EndAddress );
            function = &handler_data->chain;
            continue;
        }
        if (info->flags & (UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER))
            TRACE( "    handler %p data at %p\n",
                   (char *)base + handler_data->handler, &handler_data->handler + 1 );
        break;
    }
}

static ULONG64 get_int_reg( CONTEXT *context, int reg )
{
    return *(&context->Rax + reg);
}

static void set_int_reg( CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr, int reg, ULONG64 *val )
{
    *(&context->Rax + reg) = *val;
    if (ctx_ptr) ctx_ptr->IntegerContext[reg] = val;
}

static void set_float_reg( CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr, int reg, M128A *val )
{
    /* Use a memcpy() to avoid issues if val is misaligned. */
    memcpy(&context->Xmm0 + reg, val, sizeof(*val));
    if (ctx_ptr) ctx_ptr->FloatingContext[reg] = val;
}

static int get_opcode_size( struct opcode op )
{
    switch (op.code)
    {
    case UWOP_ALLOC_LARGE:
        return 2 + (op.info != 0);
    case UWOP_SAVE_NONVOL:
    case UWOP_SAVE_XMM128:
    case UWOP_EPILOG:
        return 2;
    case UWOP_SAVE_NONVOL_FAR:
    case UWOP_SAVE_XMM128_FAR:
        return 3;
    default:
        return 1;
    }
}

static BOOL is_inside_epilog( BYTE *pc, ULONG64 base, const RUNTIME_FUNCTION *function )
{
    /* add or lea must be the first instruction, and it must have a rex.W prefix */
    if ((pc[0] & 0xf8) == 0x48)
    {
        switch (pc[1])
        {
        case 0x81: /* add $nnnn,%rsp */
            if (pc[0] == 0x48 && pc[2] == 0xc4)
            {
                pc += 7;
                break;
            }
            return FALSE;
        case 0x83: /* add $n,%rsp */
            if (pc[0] == 0x48 && pc[2] == 0xc4)
            {
                pc += 4;
                break;
            }
            return FALSE;
        case 0x8d: /* lea n(reg),%rsp */
            if (pc[0] & 0x06) return FALSE;  /* rex.RX must be cleared */
            if (((pc[2] >> 3) & 7) != 4) return FALSE;  /* dest reg mus be %rsp */
            if ((pc[2] & 7) == 4) return FALSE;  /* no SIB byte allowed */
            if ((pc[2] >> 6) == 1)  /* 8-bit offset */
            {
                pc += 4;
                break;
            }
            if ((pc[2] >> 6) == 2)  /* 32-bit offset */
            {
                pc += 7;
                break;
            }
            return FALSE;
        }
    }

    /* now check for various pop instructions */

    for (;;)
    {
        BYTE rex = 0;

        if ((*pc & 0xf0) == 0x40) rex = *pc++ & 0x0f;  /* rex prefix */

        switch (*pc)
        {
        case 0x58: /* pop %rax/%r8 */
        case 0x59: /* pop %rcx/%r9 */
        case 0x5a: /* pop %rdx/%r10 */
        case 0x5b: /* pop %rbx/%r11 */
        case 0x5c: /* pop %rsp/%r12 */
        case 0x5d: /* pop %rbp/%r13 */
        case 0x5e: /* pop %rsi/%r14 */
        case 0x5f: /* pop %rdi/%r15 */
            pc++;
            continue;
        case 0xc2: /* ret $nn */
        case 0xc3: /* ret */
            return TRUE;
        case 0xe9: /* jmp nnnn */
            pc += 5 + *(LONG *)(pc + 1);
            return !(pc - (BYTE *)base >= function->BeginAddress && pc - (BYTE *)base < function->EndAddress);
        case 0xeb: /* jmp n */
            pc += 2 + (signed char)pc[1];
            return !(pc - (BYTE *)base >= function->BeginAddress && pc - (BYTE *)base < function->EndAddress);
        case 0xf3: /* rep; ret (for amd64 prediction bug) */
            return pc[1] == 0xc3;
        case 0xff: /* jmp */
            if (rex && rex != 8) return FALSE;
            if (pc[1] == 0x25) return TRUE;
            return rex && ((pc[1] >> 3) & 7) == 4;
        }
        return FALSE;
    }
}

/* execute a function epilog, which must have been validated with is_inside_epilog() */
static void interpret_epilog( BYTE *pc, CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr )
{
    for (;;)
    {
        BYTE rex = 0;

        if ((*pc & 0xf0) == 0x40) rex = *pc++ & 0x0f;  /* rex prefix */

        switch (*pc)
        {
        case 0x58: /* pop %rax/r8 */
        case 0x59: /* pop %rcx/r9 */
        case 0x5a: /* pop %rdx/r10 */
        case 0x5b: /* pop %rbx/r11 */
        case 0x5c: /* pop %rsp/r12 */
        case 0x5d: /* pop %rbp/r13 */
        case 0x5e: /* pop %rsi/r14 */
        case 0x5f: /* pop %rdi/r15 */
            set_int_reg( context, ctx_ptr, *pc - 0x58 + (rex & 1) * 8, (ULONG64 *)context->Rsp );
            context->Rsp += sizeof(ULONG64);
            pc++;
            continue;
        case 0x81: /* add $nnnn,%rsp */
            context->Rsp += *(LONG *)(pc + 2);
            pc += 2 + sizeof(LONG);
            continue;
        case 0x83: /* add $n,%rsp */
            context->Rsp += (signed char)pc[2];
            pc += 3;
            continue;
        case 0x8d:
            if ((pc[1] >> 6) == 1)  /* lea n(reg),%rsp */
            {
                context->Rsp = get_int_reg( context, (pc[1] & 7) + (rex & 1) * 8 ) + (signed char)pc[2];
                pc += 3;
            }
            else  /* lea nnnn(reg),%rsp */
            {
                context->Rsp = get_int_reg( context, (pc[1] & 7) + (rex & 1) * 8 ) + *(LONG *)(pc + 2);
                pc += 2 + sizeof(LONG);
            }
            continue;
        case 0xc2: /* ret $nn */
            context->Rip = *(ULONG64 *)context->Rsp;
            context->Rsp += sizeof(ULONG64) + *(WORD *)(pc + 1);
            return;
        case 0xe9: /* jmp nnnn */
        case 0xeb: /* jmp n */
        case 0xc3: /* ret */
        case 0xf3: /* rep; ret */
        case 0xff: /* jmp */
            context->Rip = *(ULONG64 *)context->Rsp;
            context->Rsp += sizeof(ULONG64);
            return;
        }
        return;
    }
}

static RUNTIME_FUNCTION *find_function_info( ULONG_PTR pc, ULONG_PTR base,
                                             RUNTIME_FUNCTION *func, ULONG count )
{
    int min = 0;
    int max = count - 1;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        if (pc < base + func[pos].BeginAddress) max = pos - 1;
        else if (pc >= base + func[pos].EndAddress) min = pos + 1;
        else
        {
            func += pos;
            while (func->UnwindData & 1)  /* follow chained entry */
                func = (RUNTIME_FUNCTION *)(base + (func->UnwindData & ~1));
            return func;
        }
    }
    return NULL;
}


/**********************************************************************
 *              RtlVirtualUnwind2   (NTDLL.@)
 */
NTSTATUS WINAPI RtlVirtualUnwind2( ULONG type, ULONG_PTR base, ULONG_PTR pc,
                                   RUNTIME_FUNCTION *function, CONTEXT *context,
                                   BOOLEAN *mach_frame_unwound, void **data,
                                   ULONG_PTR *frame_ret, KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr,
                                   ULONG_PTR *limit_low, ULONG_PTR *limit_high,
                                   PEXCEPTION_ROUTINE *handler_ret, ULONG flags )
{
    union handler_data *handler_data;
    ULONG64 frame, off;
    struct UNWIND_INFO *info;
    unsigned int i, prolog_offset;
    BOOL mach_frame = FALSE;

#ifdef __arm64ec__
    if (RtlIsEcCode( pc ))
    {
        DWORD flags = context->ContextFlags & ~CONTEXT_UNWOUND_TO_CALL;
        ARM64_NT_CONTEXT arm_context;
        NTSTATUS status;

        context_x64_to_arm( &arm_context, (ARM64EC_NT_CONTEXT *)context );
        status = RtlVirtualUnwind2_arm64( type, base, pc, (ARM64_RUNTIME_FUNCTION *)function,
                                          &arm_context, NULL, data, frame_ret, NULL,
                                          limit_low, limit_high, handler_ret, flags );
        context_arm_to_x64( (ARM64EC_NT_CONTEXT *)context, &arm_context );
        context->ContextFlags = flags | (arm_context.ContextFlags & CONTEXT_UNWOUND_TO_CALL);
        return status;
    }
#endif

    TRACE( "type %lx base %I64x rip %I64x rva %I64x rsp %I64x\n", type, base, pc, pc - base, context->Rsp );
    if (limit_low || limit_high) FIXME( "limits not supported\n" );

    frame = *frame_ret = context->Rsp;

    if (!function)  /* leaf function */
    {
        context->Rip = *(ULONG64 *)context->Rsp;
        context->Rsp += sizeof(ULONG64);
        *data = NULL;
        *handler_ret = NULL;
        return STATUS_SUCCESS;
    }

    if (TRACE_ON(unwind)) dump_unwind_info( base, function );

    for (;;)
    {
        info = (struct UNWIND_INFO *)((char *)base + function->UnwindData);
        handler_data = (union handler_data *)&info->opcodes[(info->count + 1) & ~1];

        if (info->version != 1 && info->version != 2)
        {
            FIXME( "unknown unwind info version %u at %p\n", info->version, info );
            return STATUS_BAD_FUNCTION_TABLE;
        }

        if (info->frame_reg)
            frame = get_int_reg( context, info->frame_reg ) - info->frame_offset * 16;

        /* check if in prolog */
        if (pc >= base + function->BeginAddress && pc < base + function->BeginAddress + info->prolog)
        {
            TRACE("inside prolog.\n");
            prolog_offset = pc - base - function->BeginAddress;
        }
        else
        {
            prolog_offset = ~0;
            /* Since Win10 1809 epilogue does not have a special treatment in case of zero opcode count. */
            if (info->count && is_inside_epilog( (BYTE *)pc, base, function ))
            {
                TRACE("inside epilog.\n");
                interpret_epilog( (BYTE *)pc, context, ctx_ptr );
                *frame_ret = info->frame_reg ? context->Rsp - 8 : frame;
                *handler_ret = NULL;
                return STATUS_SUCCESS;
            }
        }

        for (i = 0; i < info->count; i += get_opcode_size(info->opcodes[i]))
        {
            if (prolog_offset < info->opcodes[i].offset) continue; /* skip it */

            switch (info->opcodes[i].code)
            {
            case UWOP_PUSH_NONVOL:  /* pushq %reg */
                set_int_reg( context, ctx_ptr, info->opcodes[i].info, (ULONG64 *)context->Rsp );
                context->Rsp += sizeof(ULONG64);
                break;
            case UWOP_ALLOC_LARGE:  /* subq $nn,%rsp */
                if (info->opcodes[i].info) context->Rsp += *(DWORD *)&info->opcodes[i+1];
                else context->Rsp += *(USHORT *)&info->opcodes[i+1] * 8;
                break;
            case UWOP_ALLOC_SMALL:  /* subq $n,%rsp */
                context->Rsp += (info->opcodes[i].info + 1) * 8;
                break;
            case UWOP_SET_FPREG:  /* leaq nn(%rsp),%framereg */
                context->Rsp = *frame_ret = frame;
                break;
            case UWOP_SAVE_NONVOL:  /* movq %reg,n(%rsp) */
                off = frame + *(USHORT *)&info->opcodes[i+1] * 8;
                set_int_reg( context, ctx_ptr, info->opcodes[i].info, (ULONG64 *)off );
                break;
            case UWOP_SAVE_NONVOL_FAR:  /* movq %reg,nn(%rsp) */
                off = frame + *(DWORD *)&info->opcodes[i+1];
                set_int_reg( context, ctx_ptr, info->opcodes[i].info, (ULONG64 *)off );
                break;
            case UWOP_SAVE_XMM128:  /* movaps %xmmreg,n(%rsp) */
                off = frame + *(USHORT *)&info->opcodes[i+1] * 16;
                set_float_reg( context, ctx_ptr, info->opcodes[i].info, (M128A *)off );
                break;
            case UWOP_SAVE_XMM128_FAR:  /* movaps %xmmreg,nn(%rsp) */
                off = frame + *(DWORD *)&info->opcodes[i+1];
                set_float_reg( context, ctx_ptr, info->opcodes[i].info, (M128A *)off );
                break;
            case UWOP_PUSH_MACHFRAME:
                if (info->flags & UNW_FLAG_CHAININFO)
                {
                    FIXME("PUSH_MACHFRAME with chained unwind info.\n");
                    break;
                }
                if (i + get_opcode_size(info->opcodes[i]) < info->count )
                {
                    FIXME("PUSH_MACHFRAME is not the last opcode.\n");
                    break;
                }

                if (info->opcodes[i].info)
                    context->Rsp += 0x8;

                context->Rip = *(ULONG64 *)context->Rsp;
                context->Rsp = *(ULONG64 *)(context->Rsp + 24);
                mach_frame = TRUE;
                break;
            case UWOP_EPILOG:
                if (info->version == 2)
                    break; /* nothing to do */
            default:
                FIXME( "unknown code %u\n", info->opcodes[i].code );
                break;
            }
        }

        if (!(info->flags & UNW_FLAG_CHAININFO)) break;
        function = &handler_data->chain;  /* restart with the chained info */
    }

    if (!mach_frame)
    {
        /* now pop return address */
        context->Rip = *(ULONG64 *)context->Rsp;
        context->Rsp += sizeof(ULONG64);
    }

    *handler_ret = NULL;

    if (!(info->flags & type)) return STATUS_SUCCESS;  /* no matching handler */
    if (prolog_offset != ~0) return STATUS_SUCCESS;  /* inside prolog */

    *handler_ret = (PEXCEPTION_ROUTINE)((char *)base + handler_data->handler);
    *data = &handler_data->handler + 1;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *              RtlVirtualUnwind   (NTDLL.@)
 */
PEXCEPTION_ROUTINE WINAPI RtlVirtualUnwind( ULONG type, ULONG64 base, ULONG64 pc,
                                            RUNTIME_FUNCTION *func, CONTEXT *context,
                                            PVOID *handler_data, ULONG64 *frame_ret,
                                            KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr )
{
    PEXCEPTION_ROUTINE handler;

    if (!RtlVirtualUnwind2( type, base, pc, func, context, NULL, handler_data,
                           frame_ret, ctx_ptr, NULL, NULL, &handler, 0 ))
        return handler;

    context->Rip = 0;
    return NULL;
}


/*******************************************************************
 *		__C_specific_handler (NTDLL.@)
 */
EXCEPTION_DISPOSITION WINAPI __C_specific_handler( EXCEPTION_RECORD *rec, void *frame,
                                                   CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    const SCOPE_TABLE *table = dispatch->HandlerData;
    ULONG_PTR base = dispatch->ImageBase;
    ULONG_PTR pc = dispatch->ControlPc;
    unsigned int i;

#ifdef __arm64ec__
    if (RtlIsEcCode( pc ))
        return __C_specific_handler_arm64( rec, frame, (ARM64_NT_CONTEXT *)context,
                                           (DISPATCHER_CONTEXT_ARM64 *)dispatch );
#endif

    TRACE( "%p %p %p %p pc %Ix\n", rec, frame, context, dispatch, pc );
    if (TRACE_ON(unwind)) DUMP_SCOPE_TABLE( base, table );

    if (rec->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND))
    {
        for (i = dispatch->ScopeIndex; i < table->Count; i++)
        {
            if (pc < base + table->ScopeRecord[i].BeginAddress) continue;
            if (pc >= base + table->ScopeRecord[i].EndAddress) continue;
            if (table->ScopeRecord[i].JumpTarget) continue;

            if (rec->ExceptionFlags & EXCEPTION_TARGET_UNWIND &&
                dispatch->TargetIp >= base + table->ScopeRecord[i].BeginAddress &&
                dispatch->TargetIp < base + table->ScopeRecord[i].EndAddress)
            {
                break;
            }
            else
            {
                PTERMINATION_HANDLER handler = (void *)(base + table->ScopeRecord[i].HandlerAddress);
                dispatch->ScopeIndex = i + 1;
                TRACE( "scope %u calling __finally %p frame %p\n", i, handler, frame );
                handler( TRUE, frame );
            }
        }
    }
    else
    {
        for (i = dispatch->ScopeIndex; i < table->Count; i++)
        {
            if (pc < base + table->ScopeRecord[i].BeginAddress) continue;
            if (pc >= base + table->ScopeRecord[i].EndAddress) continue;
            if (!table->ScopeRecord[i].JumpTarget) continue;

            if (table->ScopeRecord[i].HandlerAddress != EXCEPTION_EXECUTE_HANDLER)
            {
                EXCEPTION_POINTERS ptrs = { rec, context };
                PEXCEPTION_FILTER filter = (void *)(base + table->ScopeRecord[i].HandlerAddress);

                TRACE( "scope %u calling filter %p ptrs %p frame %p\n", i, filter, &ptrs, frame );
                switch (filter( &ptrs, frame ))
                {
                case EXCEPTION_EXECUTE_HANDLER:
                    break;
                case EXCEPTION_CONTINUE_SEARCH:
                    continue;
                case EXCEPTION_CONTINUE_EXECUTION:
                    return ExceptionContinueExecution;
                }
            }
            TRACE( "unwinding to target %Ix\n", base + table->ScopeRecord[i].JumpTarget );
            RtlUnwindEx( frame, (char *)base + table->ScopeRecord[i].JumpTarget,
                         rec, ULongToPtr(rec->ExceptionCode), dispatch->ContextRecord,
                         dispatch->HistoryTable );
        }
    }
    return ExceptionContinueSearch;
}


/**********************************************************************
 *              RtlLookupFunctionEntry   (NTDLL.@)
 */
PRUNTIME_FUNCTION WINAPI RtlLookupFunctionEntry( ULONG_PTR pc, ULONG_PTR *base,
                                                 UNWIND_HISTORY_TABLE *table )
{
    RUNTIME_FUNCTION *func;
    ULONG_PTR dynbase;
    ULONG size;

#ifdef __arm64ec__
    if (RtlIsEcCode( pc ))
        return (RUNTIME_FUNCTION *)RtlLookupFunctionEntry_arm64( pc, base, table );
#endif

    if ((func = RtlLookupFunctionTable( pc, base, &size )))
        return find_function_info( pc, *base, func, size / sizeof(*func));

    if ((func = lookup_dynamic_function_table( pc, &dynbase, &size )))
    {
        RUNTIME_FUNCTION *ret = find_function_info( pc, dynbase, func, size );
        if (ret) *base = dynbase;
        return ret;
    }

    *base = 0;
    return NULL;
}


/**********************************************************************
 *              RtlAddFunctionTable   (NTDLL.@)
 */
BOOLEAN CDECL RtlAddFunctionTable( RUNTIME_FUNCTION *table, DWORD count, ULONG_PTR base )
{
    ULONG_PTR end = base;
    void *ret;

    if (count) end += table[count - 1].EndAddress;

    return !RtlAddGrowableFunctionTable( &ret, table, count, 0, base, end );
}

#endif  /* __x86_64__ */


/***********************************************************************
 * Generic unwind functions
 */


/***********************************************************************
 *            RtlUnwind  (NTDLL.@)
 */
void WINAPI RtlUnwind( void *frame, void *target_ip, EXCEPTION_RECORD *rec, void *retval )
{
    CONTEXT context;
    RtlUnwindEx( frame, target_ip, rec, retval, &context, NULL );
}

__ASM_GLOBAL_IMPORT(RtlUnwind)


/*******************************************************************
 *		_local_unwind (NTDLL.@)
 */
void WINAPI _local_unwind( void *frame, void *target_ip )
{
    RtlUnwind( frame, target_ip, NULL, NULL );
}

#endif  /* !__i386__ */
