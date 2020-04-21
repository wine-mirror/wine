/*
 * C++ exception handling (ver. 4)
 *
 * Copyright 2020 Piotr Caban
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

#ifdef __x86_64__

#include "wine/debug.h"
#include "cppexcept.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

typedef struct
{
    BYTE header;
    UINT bbt_flags;
    UINT unwind_count;
    UINT unwind_map;
    UINT tryblock_count;
    UINT tryblock_map;
    UINT ip_count;
    UINT ip_map;
    UINT frame;
} cxx_function_descr;
#define FUNC_DESCR_IS_CATCH     0x01
#define FUNC_DESCR_IS_SEPARATED 0x02
#define FUNC_DESCR_BBT          0x04
#define FUNC_DESCR_UNWIND_MAP   0x08
#define FUNC_DESCR_TRYBLOCK_MAP 0x10
#define FUNC_DESCR_EHS          0x20
#define FUNC_DESCR_NO_EXCEPT    0x40
#define FUNC_DESCR_RESERVED     0x80

typedef struct
{
    UINT flags;
    BYTE *prev;
    UINT handler;
    UINT object;
} unwind_info;

typedef struct
{
    BYTE header;
    UINT flags;
    UINT type_info;
    int offset;
    UINT handler;
    UINT ret_addr;
} catchblock_info;
#define CATCHBLOCK_FLAGS     0x01
#define CATCHBLOCK_TYPE_INFO 0x02
#define CATCHBLOCK_OFFSET    0x04
#define CATCHBLOCK_RET_ADDR  0x10

#define TYPE_FLAG_CONST      1
#define TYPE_FLAG_VOLATILE   2
#define TYPE_FLAG_REFERENCE  8

typedef struct
{
    UINT start_level;
    UINT end_level;
    UINT catch_level;
    UINT catchblock_count;
    UINT catchblock;
} tryblock_info;

typedef struct
{
    UINT ip_off; /* relative to start of function or earlier ipmap_info */
    INT state;
} ipmap_info;

static UINT decode_uint(BYTE **b)
{
    UINT ret;

    if ((**b & 1) == 0)
    {
        ret = *b[0] >> 1;
        *b += 1;
    }
    else if ((**b & 3) == 1)
    {
        ret = (*b[0] >> 2) + (*b[1] << 6);
        *b += 2;
    }
    else if ((**b & 7) == 3)
    {
        ret = (*b[0] >> 3) + (*b[1] << 5) + (*b[2] << 13);
        *b += 3;
    }
    else if ((**b & 15) == 7)
    {
        ret = (*b[0] >> 4) + (*b[1] << 4) + (*b[2] << 12) + (*b[3] << 20);
        *b += 4;
    }
    else
    {
        FIXME("not implemented - expect crash\n");
        ret = 0;
        *b += 5;
    }

    return ret;
}

static UINT read_rva(BYTE **b)
{
    UINT ret = *(UINT*)(*b);
    *b += sizeof(UINT);
    return ret;
}

static inline void* rva_to_ptr(UINT rva, ULONG64 base)
{
    return rva ? (void*)(base+rva) : NULL;
}

static BOOL read_unwind_info(BYTE **b, unwind_info *ui)
{
    BYTE *p = *b;

    memset(ui, 0, sizeof(*ui));
    ui->flags = decode_uint(b);
    ui->prev = p - (ui->flags >> 2);
    ui->flags &= 0x3;

    if (ui->flags & 0x1)
    {
        ui->handler = read_rva(b);
        ui->object = decode_uint(b); /* frame offset */
    }

    if (ui->flags & 0x2)
    {
        FIXME("unknown flag: %x\n", ui->flags);
        return FALSE;
    }
    return TRUE;
}

static void read_tryblock_info(BYTE **b, tryblock_info *ti, ULONG64 image_base)
{
    BYTE *count, *count_end;

    ti->start_level = decode_uint(b);
    ti->end_level = decode_uint(b);
    ti->catch_level = decode_uint(b);
    ti->catchblock = read_rva(b);

    count = count_end = rva_to_ptr(ti->catchblock, image_base);
    if (count)
    {
        ti->catchblock_count = decode_uint(&count_end);
        ti->catchblock += count_end - count;
    }
    else
    {
        ti->catchblock_count = 0;
    }
}

static BOOL read_catchblock_info(BYTE **b, catchblock_info *ci)
{
    memset(ci, 0, sizeof(*ci));
    ci->header = **b;
    (*b)++;
    if (ci->header & ~(CATCHBLOCK_FLAGS | CATCHBLOCK_TYPE_INFO | CATCHBLOCK_OFFSET | CATCHBLOCK_RET_ADDR))
    {
        FIXME("unknown header: %x\n", ci->header);
        return FALSE;
    }
    if (ci->header & CATCHBLOCK_FLAGS) ci->flags = decode_uint(b);
    if (ci->header & CATCHBLOCK_TYPE_INFO) ci->type_info = read_rva(b);
    if (ci->header & CATCHBLOCK_OFFSET) ci->offset = decode_uint(b);
    ci->handler = read_rva(b);
    if (ci->header & CATCHBLOCK_RET_ADDR) ci->ret_addr = decode_uint(b);
    return TRUE;
}

static void read_ipmap_info(BYTE **b, ipmap_info *ii)
{
    ii->ip_off = decode_uint(b);
    ii->state = (INT)decode_uint(b) - 1;
}

static inline void dump_type(UINT type_rva, ULONG64 base)
{
    const cxx_type_info *type = rva_to_ptr(type_rva, base);

    TRACE("flags %x type %x %s offsets %d,%d,%d size %d copy ctor %x(%p)\n",
            type->flags, type->type_info, dbgstr_type_info(rva_to_ptr(type->type_info, base)),
            type->offsets.this_offset, type->offsets.vbase_descr, type->offsets.vbase_offset,
            type->size, type->copy_ctor, rva_to_ptr(type->copy_ctor, base));
}

static BOOL validate_cxx_function_descr4(const cxx_function_descr *descr, DISPATCHER_CONTEXT *dispatch)
{
    ULONG64 image_base = dispatch->ImageBase;
    BYTE *unwind_map = rva_to_ptr(descr->unwind_map, image_base);
    BYTE *tryblock_map = rva_to_ptr(descr->tryblock_map, image_base);
    BYTE *ip_map = rva_to_ptr(descr->ip_map, image_base);
    UINT i, j;
    char *ip;

    TRACE("header 0x%x\n", descr->header);
    TRACE("basic block transformations flags: 0x%x\n", descr->bbt_flags);

    TRACE("unwind table: 0x%x(%p) %d\n", descr->unwind_map, unwind_map, descr->unwind_count);
    for (i = 0; i < descr->unwind_count; i++)
    {
        BYTE *entry = unwind_map;
        unwind_info ui;

        if (!read_unwind_info(&unwind_map, &ui)) return FALSE;
        if (ui.prev < (BYTE*)rva_to_ptr(descr->unwind_map, image_base)) ui.prev = NULL;
        TRACE("    %d (%p): flags 0x%x prev %p func 0x%x(%p) object 0x%x\n",
                i, entry, ui.flags, ui.prev, ui.handler,
                rva_to_ptr(ui.handler, image_base), ui.object);
    }

    TRACE("try table: 0x%x(%p) %d\n", descr->tryblock_map, tryblock_map, descr->tryblock_count);
    for (i = 0; i < descr->tryblock_count; i++)
    {
        tryblock_info ti;
        BYTE *catchblock;

        read_tryblock_info(&tryblock_map, &ti, image_base);
        catchblock = rva_to_ptr(ti.catchblock, image_base);
        TRACE("    %d: start %d end %d catchlevel %d catch 0x%x(%p) %d\n",
                i, ti.start_level, ti.end_level, ti.catch_level,
                ti.catchblock, catchblock, ti.catchblock_count);
        for (j = 0; j < ti.catchblock_count; j++)
        {
            catchblock_info ci;
            if (!read_catchblock_info(&catchblock, &ci)) return FALSE;
            TRACE("        %d: header 0x%x offset %d handler 0x%x(%p) "
                    "ret addr %x type %x %s\n", j, ci.header, ci.offset,
                    ci.handler, rva_to_ptr(ci.handler, image_base),
                    ci.ret_addr, ci.type_info,
                    dbgstr_type_info(rva_to_ptr(ci.type_info, image_base)));
        }
    }

    TRACE("ipmap: 0x%x(%p) %d\n", descr->ip_map, ip_map, descr->ip_count);
    ip = rva_to_ptr(dispatch->FunctionEntry->BeginAddress, image_base);
    for (i = 0; i < descr->ip_count; i++)
    {
        ipmap_info ii;

        read_ipmap_info(&ip_map, &ii);
        ip += ii.ip_off;
        TRACE("    %d: ip offset 0x%x (%p) state %d\n", i, ii.ip_off, ip, ii.state);
    }

    TRACE("establisher frame: %x\n", descr->frame);
    return TRUE;
}

EXCEPTION_DISPOSITION __cdecl __CxxFrameHandler4(EXCEPTION_RECORD *rec,
        ULONG64 frame, CONTEXT *context, DISPATCHER_CONTEXT *dispatch)
{
    cxx_function_descr descr;
    BYTE *p, *count, *count_end;

    FIXME("%p %lx %p %p\n", rec, frame, context, dispatch);

    memset(&descr, 0, sizeof(descr));
    p = rva_to_ptr(*(UINT*)dispatch->HandlerData, dispatch->ImageBase);
    descr.header = *p++;

    if (descr.header & ~(FUNC_DESCR_IS_CATCH | FUNC_DESCR_UNWIND_MAP |
                FUNC_DESCR_TRYBLOCK_MAP | FUNC_DESCR_EHS))
    {
        FIXME("unsupported flags: %x\n", descr.header);
        return ExceptionContinueSearch;
    }

    if (descr.header & FUNC_DESCR_BBT) descr.bbt_flags = decode_uint(&p);
    if (descr.header & FUNC_DESCR_UNWIND_MAP)
    {
        descr.unwind_map = read_rva(&p);
        count_end = count = rva_to_ptr(descr.unwind_map, dispatch->ImageBase);
        descr.unwind_count = decode_uint(&count_end);
        descr.unwind_map += count_end - count;
    }
    if (descr.header & FUNC_DESCR_TRYBLOCK_MAP)
    {
        descr.tryblock_map = read_rva(&p);
        count_end = count = rva_to_ptr(descr.tryblock_map, dispatch->ImageBase);
        descr.tryblock_count = decode_uint(&count_end);
        descr.tryblock_map += count_end - count;
    }
    descr.ip_map = read_rva(&p);
    count_end = count = rva_to_ptr(descr.ip_map, dispatch->ImageBase);
    descr.ip_count = decode_uint(&count_end);
    descr.ip_map += count_end - count;
    if (descr.header & FUNC_DESCR_IS_CATCH) descr.frame = decode_uint(&p);

    if (!validate_cxx_function_descr4(&descr, dispatch))
        return ExceptionContinueSearch;

    return ExceptionContinueSearch;
}

#endif
