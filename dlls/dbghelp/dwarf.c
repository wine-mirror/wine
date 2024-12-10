/*
 * File dwarf.c - read dwarf2 information from the ELF modules
 *
 * Copyright (C) 2005, Raphael Junqueira
 * Copyright (C) 2006-2011, Eric Pouech
 * Copyright (C) 2010, Alexandre Julliard
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

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <zlib.h>

#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "oleauto.h"

#include "dbghelp_private.h"
#include "image_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp_dwarf);

/* FIXME:
 * - Functions:
 *      o unspecified parameters
 *      o inlined functions
 *      o Debug{Start|End}Point
 *      o CFA
 * - Udt
 *      o proper types loading (nesting)
 */

#if 0
static void dump(const void* ptr, unsigned len)
{
    int         i, j;
    BYTE        msg[128];
    static const char hexof[] = "0123456789abcdef";
    const       BYTE* x = ptr;

    for (i = 0; i < len; i += 16)
    {
        sprintf(msg, "%08x: ", i);
        memset(msg + 10, ' ', 3 * 16 + 1 + 16);
        for (j = 0; j < min(16, len - i); j++)
        {
            msg[10 + 3 * j + 0] = hexof[x[i + j] >> 4];
            msg[10 + 3 * j + 1] = hexof[x[i + j] & 15];
            msg[10 + 3 * j + 2] = ' ';
            msg[10 + 3 * 16 + 1 + j] = (x[i + j] >= 0x20 && x[i + j] < 0x7f) ?
                x[i + j] : '.';
        }
        msg[10 + 3 * 16] = ' ';
        msg[10 + 3 * 16 + 1 + 16] = '\0';
        TRACE("%s\n", msg);
    }
}
#endif

/**
 *
 * Main Specs:
 *  http://www.eagercon.com/dwarf/dwarf3std.htm
 *  http://www.eagercon.com/dwarf/dwarf-2.0.0.pdf
 *
 * dwarf2.h: http://www.hakpetzna.com/b/binutils/dwarf2_8h-source.html
 *
 * example of projects who do dwarf2 parsing:
 *  http://www.x86-64.org/cgi-bin/cvsweb.cgi/binutils.dead/binutils/readelf.c?rev=1.1.1.2
 *  http://elis.ugent.be/diota/log/ltrace_elf.c
 */
#include "dwarf.h"

/**
 * Parsers
 */

typedef struct dwarf2_abbrev_entry_attr_s
{
  ULONG_PTR attribute;
  ULONG_PTR form;
  struct dwarf2_abbrev_entry_attr_s* next;
} dwarf2_abbrev_entry_attr_t;

typedef struct dwarf2_abbrev_entry_s
{
    ULONG_PTR entry_code;
    ULONG_PTR tag;
    unsigned char have_child;
    unsigned num_attr;
    dwarf2_abbrev_entry_attr_t* attrs;
} dwarf2_abbrev_entry_t;

struct dwarf2_block
{
    unsigned                    size;
    const unsigned char*        ptr;
};

struct attribute
{
    ULONG_PTR                   form;
    enum {attr_direct, attr_abstract_origin, attr_specification} gotten_from;
    union
    {
        ULONG_PTR                       uvalue;
        ULONGLONG                       lluvalue;
        LONG_PTR                        svalue;
        LONGLONG                        llsvalue;
        const char*                     string;
        struct dwarf2_block             block;
    } u;
    const struct dwarf2_debug_info_s*   debug_info;
};

typedef struct dwarf2_debug_info_s
{
    const dwarf2_abbrev_entry_t*abbrev;
    struct symt*                symt;
    const unsigned char**       data;
    struct vector               children;
    struct dwarf2_debug_info_s* parent;
    struct dwarf2_parse_context_s* unit_ctx;
} dwarf2_debug_info_t;

typedef struct dwarf2_section_s
{
    BOOL                        compressed;
    const unsigned char*        address;
    unsigned                    size;
    DWORD_PTR                   rva;
} dwarf2_section_t;

enum dwarf2_sections {section_debug, section_string, section_abbrev, section_line, section_ranges, section_max};

typedef struct dwarf2_traverse_context_s
{
    const unsigned char*        data;
    const unsigned char*        end_data;
} dwarf2_traverse_context_t;

typedef struct dwarf2_cuhead_s
{
    unsigned char               word_size; /* size of a word on target machine */
    unsigned char               version;
    unsigned char               offset_size; /* size of offset inside DWARF */
} dwarf2_cuhead_t;

typedef struct dwarf2_parse_module_context_s
{
    ULONG_PTR                   load_offset;
    const dwarf2_section_t*     sections;
    struct module*              module;
    const struct elf_thunk_area*thunks;
    struct vector               unit_contexts;
    struct dwarf2_dwz_alternate_s* dwz;
    DWORD                       cu_versions;
} dwarf2_parse_module_context_t;

typedef struct dwarf2_dwz_alternate_s
{
    struct image_file_map*      fmap;
    dwarf2_section_t            sections[section_max];
    struct image_section_map    sectmap[section_max];
    dwarf2_parse_module_context_t module_ctx;
} dwarf2_dwz_alternate_t;

enum unit_status
{
    UNIT_ERROR,
    UNIT_NOTLOADED,
    UNIT_LOADED,
    UNIT_LOADED_FAIL,
    UNIT_BEINGLOADED,
};

/* this is the context used for parsing a compilation unit
 * inside an ELF/PE section (likely .debug_info)
 */
typedef struct dwarf2_parse_context_s
{
    dwarf2_parse_module_context_t* module_ctx;
    unsigned                    section;
    struct pool                 pool;
    struct symt_compiland*      compiland;
    struct sparse_array         abbrev_table;
    struct sparse_array         debug_info_table;
    ULONG_PTR                   ref_offset;
    char*                       cpp_name;
    dwarf2_cuhead_t             head;
    enum unit_status            status;
    dwarf2_traverse_context_t   traverse_DIE;
    unsigned                    language;
} dwarf2_parse_context_t;

/* stored in the dbghelp's module internal structure for later reuse */
struct dwarf2_module_info_s
{
    dwarf2_cuhead_t**           cuheads;
    unsigned                    num_cuheads;
    dwarf2_section_t            debug_loc;
    dwarf2_section_t            debug_frame;
    dwarf2_section_t            eh_frame;
    unsigned char               word_size;
};

#define loc_dwarf2_location_list        (loc_user + 0)
#define loc_dwarf2_block                (loc_user + 1)
#define loc_dwarf2_frame_cfa            (loc_user + 2)

/* forward declarations */
static struct symt* dwarf2_parse_enumeration_type(dwarf2_debug_info_t* entry);
static BOOL dwarf2_parse_compilation_unit(dwarf2_parse_context_t* ctx);
static dwarf2_parse_context_t* dwarf2_locate_cu(dwarf2_parse_module_context_t* module_ctx, ULONG_PTR ref);

static unsigned char dwarf2_get_byte(const unsigned char* ptr)
{
    return *ptr;
}

static unsigned char dwarf2_parse_byte(dwarf2_traverse_context_t* ctx)
{
    unsigned char uvalue = dwarf2_get_byte(ctx->data);
    ctx->data += 1;
    return uvalue;
}

static unsigned short dwarf2_get_u2(const unsigned char* ptr)
{
    return *(const UINT16*)ptr;
}

static unsigned short dwarf2_parse_u2(dwarf2_traverse_context_t* ctx)
{
    unsigned short uvalue = dwarf2_get_u2(ctx->data);
    ctx->data += 2;
    return uvalue;
}

static ULONG_PTR dwarf2_get_u4(const unsigned char* ptr)
{
    return *(const UINT32*)ptr;
}

static ULONG_PTR dwarf2_parse_u4(dwarf2_traverse_context_t* ctx)
{
    ULONG_PTR uvalue = dwarf2_get_u4(ctx->data);
    ctx->data += 4;
    return uvalue;
}

static DWORD64 dwarf2_get_u8(const unsigned char* ptr)
{
    return *(const UINT64*)ptr;
}

static DWORD64 dwarf2_parse_u8(dwarf2_traverse_context_t* ctx)
{
    DWORD64 uvalue = dwarf2_get_u8(ctx->data);
    ctx->data += 8;
    return uvalue;
}

static ULONG64 dwarf2_get_leb128_as_unsigned(const unsigned char* ptr, const unsigned char** end)
{
    ULONG64 ret = 0;
    unsigned char byte;
    unsigned shift = 0;

    do
    {
        byte = dwarf2_get_byte(ptr++);
        ret |= (ULONG64)(byte & 0x7f) << shift;
        shift += 7;
    } while (byte & 0x80);
    if ((ret >> (shift - 7)) != (byte & 0x7F)) FIXME("Overflow in LEB128 encoding\n");

    if (end) *end = ptr;
    return ret;
}

static ULONG_PTR dwarf2_leb128_as_unsigned(dwarf2_traverse_context_t* ctx)
{
    ULONG64 ret;

    assert(ctx);

    ret = dwarf2_get_leb128_as_unsigned(ctx->data, &ctx->data);
    if (ret != (ULONG_PTR)ret) WARN("Dropping bits from LEB128 value\n");
    return ret;
}

static LONG64 dwarf2_get_leb128_as_signed(const unsigned char* ptr, const unsigned char** end)
{
    ULONG64 ret = 0;
    unsigned char byte;
    unsigned shift = 0;

    do
    {
        byte = dwarf2_get_byte(ptr++);
        ret |= (ULONG64)(byte & 0x7f) << shift;
        shift += 7;
    } while (byte & 0x80);

    if (end) *end = ptr;
    if ((shift < sizeof(ULONG64) * 8) && (byte & 0x40))
        /* as spec: sign bit of byte is 2nd high order bit (80x40)
         *  -> 0x80 is used as flag.
         */
        ret |= ~(ULONG64)0 << shift;
    return ret;
}

static LONG_PTR dwarf2_leb128_as_signed(dwarf2_traverse_context_t* ctx)
{
    LONG64 ret = 0;

    assert(ctx);

    ret = dwarf2_get_leb128_as_signed(ctx->data, &ctx->data);
    if (ret != (LONG_PTR)ret) WARN("Dropping bits from LEB128 value\n");
    return ret;
}

static unsigned dwarf2_leb128_length(const dwarf2_traverse_context_t* ctx)
{
    unsigned    ret;
    for (ret = 0; ctx->data[ret] & 0x80; ret++);
    return ret + 1;
}

/******************************************************************
 *		dwarf2_get_addr
 *
 * Returns an address.
 * We assume that in all cases word size from Dwarf matches the size of
 * addresses in platform where the exec is compiled.
 */
static ULONG_PTR dwarf2_get_addr(const unsigned char* ptr, unsigned word_size)
{
    ULONG_PTR ret;

    switch (word_size)
    {
    case 4:
        ret = dwarf2_get_u4(ptr);
        break;
    case 8:
        ret = dwarf2_get_u8(ptr);
	break;
    default:
        FIXME("Unsupported Word Size %u\n", word_size);
        ret = 0;
    }
    return ret;
}

static inline ULONG_PTR dwarf2_parse_addr(dwarf2_traverse_context_t* ctx, unsigned word_size)
{
    ULONG_PTR ret = dwarf2_get_addr(ctx->data, word_size);
    ctx->data += word_size;
    return ret;
}

static inline ULONG_PTR dwarf2_parse_addr_head(dwarf2_traverse_context_t* ctx, const dwarf2_cuhead_t* head)
{
    return dwarf2_parse_addr(ctx, head->word_size);
}

static ULONG_PTR dwarf2_parse_offset(dwarf2_traverse_context_t* ctx, unsigned char offset_size)
{
    ULONG_PTR ret = dwarf2_get_addr(ctx->data, offset_size);
    ctx->data += offset_size;
    return ret;
}

static ULONG_PTR dwarf2_parse_3264(dwarf2_traverse_context_t* ctx, unsigned char* ofsz)
{
    ULONG_PTR ret = dwarf2_parse_u4(ctx);
    if (ret == 0xffffffff)
    {
        ret = dwarf2_parse_u8(ctx);
        *ofsz = 8;
    }
    else *ofsz = 4;
    return ret;
}

static const char* dwarf2_debug_traverse_ctx(const dwarf2_traverse_context_t* ctx) 
{
    return wine_dbg_sprintf("ctx(%p)", ctx->data); 
}

static const char* dwarf2_debug_unit_ctx(const dwarf2_parse_context_t* ctx)
{
    return wine_dbg_sprintf("ctx(%p,%s)",
                            ctx, debugstr_w(ctx->module_ctx->module->modulename));
}

static const char* dwarf2_debug_di(const dwarf2_debug_info_t* di)
{
    return wine_dbg_sprintf("debug_info(abbrev:%p,symt:%p) in %s",
                            di->abbrev, di->symt, dwarf2_debug_unit_ctx(di->unit_ctx));
}

static dwarf2_abbrev_entry_t*
dwarf2_abbrev_table_find_entry(const struct sparse_array* abbrev_table,
                               ULONG_PTR entry_code)
{
    assert( NULL != abbrev_table );
    return sparse_array_find(abbrev_table, entry_code);
}

static void dwarf2_parse_abbrev_set(dwarf2_traverse_context_t* abbrev_ctx, 
                                    struct sparse_array* abbrev_table,
                                    struct pool* pool)
{
    ULONG_PTR entry_code;
    dwarf2_abbrev_entry_t* abbrev_entry;
    dwarf2_abbrev_entry_attr_t* new = NULL;
    dwarf2_abbrev_entry_attr_t* last = NULL;
    ULONG_PTR attribute;
    ULONG_PTR form;

    assert( NULL != abbrev_ctx );

    TRACE("%s, end at %p\n",
          dwarf2_debug_traverse_ctx(abbrev_ctx), abbrev_ctx->end_data); 

    sparse_array_init(abbrev_table, sizeof(dwarf2_abbrev_entry_t), 32);
    while (abbrev_ctx->data < abbrev_ctx->end_data)
    {
        TRACE("now at %s\n", dwarf2_debug_traverse_ctx(abbrev_ctx)); 
        entry_code = dwarf2_leb128_as_unsigned(abbrev_ctx);
        TRACE("found entry_code %Iu\n", entry_code);
        if (!entry_code)
        {
            TRACE("NULL entry code at %s\n", dwarf2_debug_traverse_ctx(abbrev_ctx)); 
            break;
        }
        abbrev_entry = sparse_array_add(abbrev_table, entry_code, pool);
        assert( NULL != abbrev_entry );

        abbrev_entry->entry_code = entry_code;
        abbrev_entry->tag        = dwarf2_leb128_as_unsigned(abbrev_ctx);
        abbrev_entry->have_child = dwarf2_parse_byte(abbrev_ctx);
        abbrev_entry->attrs      = NULL;
        abbrev_entry->num_attr   = 0;

        TRACE("table:(%p,#%u) entry_code(%Iu) tag(0x%Ix) have_child(%u) -> %p\n",
              abbrev_table, sparse_array_length(abbrev_table),
              entry_code, abbrev_entry->tag, abbrev_entry->have_child, abbrev_entry);

        last = NULL;
        while (1)
        {
            attribute = dwarf2_leb128_as_unsigned(abbrev_ctx);
            form = dwarf2_leb128_as_unsigned(abbrev_ctx);
            if (!attribute) break;

            new = pool_alloc(pool, sizeof(dwarf2_abbrev_entry_attr_t));
            assert(new);

            new->attribute = attribute;
            new->form      = form;
            new->next      = NULL;
            if (abbrev_entry->attrs)    last->next = new;
            else                        abbrev_entry->attrs = new;
            last = new;
            abbrev_entry->num_attr++;
        }
    }
    TRACE("found %u entries\n", sparse_array_length(abbrev_table));
}

static void dwarf2_swallow_attribute(dwarf2_traverse_context_t* ctx,
                                     const dwarf2_cuhead_t* head,
                                     const dwarf2_abbrev_entry_attr_t* abbrev_attr)
{
    unsigned    step;

    TRACE("(attr:0x%Ix,form:0x%Ix)\n", abbrev_attr->attribute, abbrev_attr->form);

    switch (abbrev_attr->form)
    {
    case DW_FORM_flag_present: step = 0; break;
    case DW_FORM_ref_addr: step = (head->version >= 3) ? head->offset_size : head->word_size; break;
    case DW_FORM_addr:   step = head->word_size; break;
    case DW_FORM_flag:
    case DW_FORM_data1:
    case DW_FORM_ref1:   step = 1; break;
    case DW_FORM_data2:
    case DW_FORM_ref2:   step = 2; break;
    case DW_FORM_data4:
    case DW_FORM_ref4:   step = 4; break;
    case DW_FORM_data8:
    case DW_FORM_ref8:   step = 8; break;
    case DW_FORM_sdata:
    case DW_FORM_ref_udata:
    case DW_FORM_udata:  step = dwarf2_leb128_length(ctx); break;
    case DW_FORM_string: step = strlen((const char*)ctx->data) + 1; break;
    case DW_FORM_exprloc:
    case DW_FORM_block:  step = dwarf2_leb128_as_unsigned(ctx); break;
    case DW_FORM_block1: step = dwarf2_parse_byte(ctx); break;
    case DW_FORM_block2: step = dwarf2_parse_u2(ctx); break;
    case DW_FORM_block4: step = dwarf2_parse_u4(ctx); break;
    case DW_FORM_sec_offset:
    case DW_FORM_GNU_ref_alt:
    case DW_FORM_GNU_strp_alt:
    case DW_FORM_strp:   step = head->offset_size; break;
    default:
        FIXME("Unhandled attribute form %Ix\n", abbrev_attr->form);
        return;
    }
    ctx->data += step;
}

static BOOL dwarf2_fill_attr(const dwarf2_parse_context_t* ctx,
                             const dwarf2_abbrev_entry_attr_t* abbrev_attr,
                             const unsigned char* data,
                             struct attribute* attr)
{
    attr->form = abbrev_attr->form;
    switch (attr->form)
    {
    case DW_FORM_ref_addr:
        if (ctx->head.version >= 3)
            attr->u.uvalue = dwarf2_get_addr(data, ctx->head.offset_size);
        else
            attr->u.uvalue = dwarf2_get_addr(data, ctx->head.word_size);
        TRACE("addr<0x%Ix>\n", attr->u.uvalue);
        break;

    case DW_FORM_addr:
        attr->u.uvalue = dwarf2_get_addr(data, ctx->head.word_size);
        TRACE("addr<0x%Ix>\n", attr->u.uvalue);
        break;

    case DW_FORM_flag:
        attr->u.uvalue = dwarf2_get_byte(data);
        TRACE("flag<0x%Ix>\n", attr->u.uvalue);
        break;

    case DW_FORM_flag_present:
        attr->u.uvalue = 1;
        TRACE("flag_present\n");
        break;

    case DW_FORM_data1:
        attr->u.uvalue = dwarf2_get_byte(data);
        TRACE("data1<%Iu>\n", attr->u.uvalue);
        break;

    case DW_FORM_data2:
        attr->u.uvalue = dwarf2_get_u2(data);
        TRACE("data2<%Iu>\n", attr->u.uvalue);
        break;

    case DW_FORM_data4:
        attr->u.uvalue = dwarf2_get_u4(data);
        TRACE("data4<%Iu>\n", attr->u.uvalue);
        break;

    case DW_FORM_data8:
        attr->u.lluvalue = dwarf2_get_u8(data);
        TRACE("data8<%Ix>\n", attr->u.uvalue);
        break;

    case DW_FORM_ref1:
        attr->u.uvalue = ctx->ref_offset + dwarf2_get_byte(data);
        TRACE("ref1<0x%Ix>\n", attr->u.uvalue);
        break;

    case DW_FORM_ref2:
        attr->u.uvalue = ctx->ref_offset + dwarf2_get_u2(data);
        TRACE("ref2<0x%Ix>\n", attr->u.uvalue);
        break;

    case DW_FORM_ref4:
        attr->u.uvalue = ctx->ref_offset + dwarf2_get_u4(data);
        TRACE("ref4<0x%Ix>\n", attr->u.uvalue);
        break;
    
    case DW_FORM_ref8:
        FIXME("Unhandled 64-bit support\n");
        break;

    case DW_FORM_sdata:
        attr->u.llsvalue = dwarf2_get_leb128_as_signed(data, NULL);
        break;

    case DW_FORM_ref_udata:
        attr->u.lluvalue = ctx->ref_offset + dwarf2_get_leb128_as_unsigned(data, NULL);
        TRACE("ref_udata<0x%Ix>\n", attr->u.uvalue);
        break;

    case DW_FORM_udata:
        attr->u.lluvalue = dwarf2_get_leb128_as_unsigned(data, NULL);
        TRACE("udata<0x%Ix>\n", attr->u.uvalue);
        break;

    case DW_FORM_string:
        attr->u.string = (const char *)data;
        TRACE("string<%s>\n", debugstr_a(attr->u.string));
        break;

    case DW_FORM_strp:
        {
            ULONG_PTR ofs = dwarf2_get_addr(data, ctx->head.offset_size);
            if (ofs >= ctx->module_ctx->sections[section_string].size)
            {
                ERR("Out of bounds string offset (%08Ix)\n", ofs);
                attr->u.string = "<<outofbounds-strp>>";
            }
            else
            {
                attr->u.string = (const char*)ctx->module_ctx->sections[section_string].address + ofs;
                TRACE("strp<%s>\n", debugstr_a(attr->u.string));
            }
        }
        break;

    case DW_FORM_block:
    case DW_FORM_exprloc:
        attr->u.block.size = dwarf2_get_leb128_as_unsigned(data, &attr->u.block.ptr);
        TRACE("block<%p,%u>\n", attr->u.block.ptr, attr->u.block.size);
        break;

    case DW_FORM_block1:
        attr->u.block.size = dwarf2_get_byte(data);
        attr->u.block.ptr  = data + 1;
        TRACE("block<%p,%u>\n", attr->u.block.ptr, attr->u.block.size);
        break;

    case DW_FORM_block2:
        attr->u.block.size = dwarf2_get_u2(data);
        attr->u.block.ptr  = data + 2;
        TRACE("block<%p,%u>\n", attr->u.block.ptr, attr->u.block.size);
        break;

    case DW_FORM_block4:
        attr->u.block.size = dwarf2_get_u4(data);
        attr->u.block.ptr  = data + 4;
        TRACE("block<%p,%u>\n", attr->u.block.ptr, attr->u.block.size);
        break;

    case DW_FORM_sec_offset:
        attr->u.lluvalue = dwarf2_get_addr(data, ctx->head.offset_size);
        TRACE("sec_offset<%I64x>\n", attr->u.lluvalue);
        break;

    case DW_FORM_GNU_ref_alt:
        if (!ctx->module_ctx->dwz)
        {
            ERR("No DWZ file present for GNU_ref_alt in %s\n", debugstr_w(ctx->module_ctx->module->modulename));
            attr->u.uvalue = 0;
            return FALSE;
        }
        attr->u.uvalue = dwarf2_get_addr(data, ctx->head.offset_size);
        TRACE("ref_alt<0x%Ix>\n", attr->u.uvalue);
        break;

    case DW_FORM_GNU_strp_alt:
        if (ctx->module_ctx->dwz)
        {
            ULONG_PTR ofs = dwarf2_get_addr(data, ctx->head.offset_size);
            if (ofs < ctx->module_ctx->dwz->sections[section_string].size)
            {
                attr->u.string = (const char*)ctx->module_ctx->dwz->sections[section_string].address + ofs;
                TRACE("strp_alt<%s>\n", debugstr_a(attr->u.string));
            }
            else
            {
                ERR("out of bounds strp_alt: 0x%Ix 0x%x (%u)\n", ofs, ctx->module_ctx->dwz->sections[section_string].size, ctx->head.offset_size);
                attr->u.string = "<<outofbounds-strpalt>>";
            }
        }
        else
        {
            ERR("No DWZ file present for GNU_strp_alt in %s\n", debugstr_w(ctx->module_ctx->module->modulename));
            attr->u.string = "<<noDWZ-strpalt>>";
        }
        break;

    default:
        FIXME("Unhandled attribute form %Ix\n", abbrev_attr->form);
        break;
    }
    return TRUE;
}

static struct symt *symt_get_real_type(struct symt *symt)
{
    while (symt && symt->tag == SymTagTypedef)
        symt = ((struct symt_typedef*)symt)->type;
    return symt;
}

static BOOL dwarf2_fill_in_variant(struct module *module, VARIANT *v, const struct attribute *attr, struct symt *type)
{
    ULONG64 uinteger;
    LONG64 sinteger;
    enum BasicType bt = btInt;

    type = symt_get_real_type(type);
    if (symt_check_tag(type, SymTagBaseType))
        bt = ((struct symt_basic*)type)->bt;

    /* data1, data2, data4 can hold either signed or unsigned values...
     * so ensure proper extension of signed types...
     */
    switch (attr->form)
    {
    case DW_FORM_data1:
        uinteger = attr->u.uvalue;
        sinteger = (char)(unsigned char)attr->u.uvalue;
        break;
    case DW_FORM_data2:
        uinteger = attr->u.uvalue;
        sinteger = (short)(unsigned short)attr->u.uvalue;
        break;
    case DW_FORM_data4:
        uinteger = attr->u.uvalue;
        sinteger = (int)(unsigned int)attr->u.uvalue;
        break;

    case DW_FORM_udata:
    case DW_FORM_data8:
        sinteger = uinteger = attr->u.lluvalue;
        break;

    case DW_FORM_sdata:
        uinteger = sinteger = attr->u.llsvalue;
        break;

    case DW_FORM_strp:
    case DW_FORM_string:
        /* FIXME: native doesn't report const strings from here !!
         * however, the value of the string is in the code somewhere
         */
        V_VT(v) = VT_BYREF;
        V_BYREF(v) = pool_strdup(&module->pool, attr->u.string);
        return TRUE;
        break;

    case DW_FORM_block:
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_exprloc:
        V_VT(v) = VT_I4;
        switch (attr->u.block.size)
        {
        case 1:     V_I4(v) = *(BYTE*)attr->u.block.ptr;    break;
        case 2:     V_I4(v) = *(USHORT*)attr->u.block.ptr;  break;
        case 4:     V_I4(v) = *(DWORD*)attr->u.block.ptr;   break;
        default:
            V_VT(v) = VT_BYREF;
            V_BYREF(v) = pool_alloc(&module->pool, attr->u.block.size);
            memcpy(V_BYREF(v), attr->u.block.ptr, attr->u.block.size);
        }
        return TRUE;
        break;
    case DW_FORM_sec_offset:
    case DW_FORM_addr:
        FIXME("Unexpected form %Ix\n", attr->form);
    default:
        V_VT(v) = VT_EMPTY;
        return FALSE;
    }
    /* native always stores in the shortest format in variant */
    if (bt == btChar || bt == btInt || bt == btLong)
    {
        if (sinteger == (signed char)sinteger)
        {
            V_VT(v) = VT_I1;
            V_I1(v) = sinteger;
        }
        if (sinteger == (short int)sinteger)
        {
            V_VT(v) = VT_I2;
            V_I2(v) = sinteger;
        }
        else if (sinteger == (int)sinteger)
        {
            V_VT(v) = VT_I4;
            V_I4(v) = sinteger;
        }
        else
        {
            V_VT(v) = VT_I8;
            V_I8(v) = sinteger;
        }
    }
    else if (bt == btUInt || bt == btULong || bt == btWChar)
    {
        if (uinteger == (unsigned char)uinteger)
        {
            V_VT(v) = VT_UI1;
            V_UI1(v) = uinteger;
        }
        else if (uinteger == (unsigned short int)uinteger)
        {
            V_VT(v) = VT_UI2;
            V_UI2(v) = uinteger;
        }
        else if (uinteger == (unsigned int)uinteger)
        {
            V_VT(v) = VT_UI4;
            V_UI4(v) = uinteger;
        }
        else
        {
            V_VT(v) = VT_UI8;
            V_UI8(v) = uinteger;
        }
    }
    else
    {
        FIXME("Unexpected base type bt=%x for form=%Ix\n", bt, attr->form);
        return FALSE;
    }

    return TRUE;
}

static dwarf2_debug_info_t* dwarf2_jump_to_debug_info(struct attribute* attr);

static BOOL dwarf2_find_attribute(const dwarf2_debug_info_t* di,
                                  unsigned at, struct attribute* attr)
{
    unsigned                    i, refidx = 0;
    dwarf2_abbrev_entry_attr_t* abbrev_attr;
    dwarf2_abbrev_entry_attr_t* ref_abbrev_attr = NULL;

    attr->gotten_from = attr_direct;
    while (di)
    {
        ref_abbrev_attr = NULL;
        attr->debug_info = di;
        for (i = 0, abbrev_attr = di->abbrev->attrs; abbrev_attr; i++, abbrev_attr = abbrev_attr->next)
        {
            if (abbrev_attr->attribute == at)
            {
                return dwarf2_fill_attr(di->unit_ctx, abbrev_attr, di->data[i], attr);
            }
            if ((abbrev_attr->attribute == DW_AT_abstract_origin ||
                 abbrev_attr->attribute == DW_AT_specification) &&
                at != DW_AT_sibling)
            {
                if (ref_abbrev_attr)
                    FIXME("two references %Ix and %Ix\n", ref_abbrev_attr->attribute, abbrev_attr->attribute);
                ref_abbrev_attr = abbrev_attr;
                refidx = i;
                attr->gotten_from = (abbrev_attr->attribute == DW_AT_abstract_origin) ?
                    attr_abstract_origin : attr_specification;
            }
        }
        /* do we have either an abstract origin or a specification debug entry to look into ? */
        if (!ref_abbrev_attr || !dwarf2_fill_attr(di->unit_ctx, ref_abbrev_attr, di->data[refidx], attr))
            break;
        if (!(di = dwarf2_jump_to_debug_info(attr)))
        {
            FIXME("Should have found the debug info entry\n");
            break;
        }
    }
    return FALSE;
}

static dwarf2_debug_info_t* dwarf2_jump_to_debug_info(struct attribute* attr)
{
    dwarf2_parse_context_t* ref_ctx = NULL;
    BOOL with_other = TRUE;
    dwarf2_debug_info_t* ret;

    switch (attr->form)
    {
    case DW_FORM_ref_addr:
        ref_ctx = dwarf2_locate_cu(attr->debug_info->unit_ctx->module_ctx, attr->u.uvalue);
        break;
    case DW_FORM_GNU_ref_alt:
        if (attr->debug_info->unit_ctx->module_ctx->dwz)
            ref_ctx = dwarf2_locate_cu(&attr->debug_info->unit_ctx->module_ctx->dwz->module_ctx, attr->u.uvalue);
        break;
    default:
        with_other = FALSE;
        ref_ctx = attr->debug_info->unit_ctx;
        break;
    }
    if (!ref_ctx) return FALSE;
    /* There are cases where we end up with a circular reference between two (or more)
     * compilation units. Before this happens, try to see if we can refer to an already
     * loaded debug_info in the target compilation unit (even if all the debug_info
     * haven't been loaded yet).
     */
    if (ref_ctx->status == UNIT_BEINGLOADED &&
        (ret = sparse_array_find(&ref_ctx->debug_info_table, attr->u.uvalue)))
        return ret;
    if (with_other)
    {
        /* ensure CU is fully loaded */
        if (ref_ctx != attr->debug_info->unit_ctx && !dwarf2_parse_compilation_unit(ref_ctx))
            return NULL;
    }
    return sparse_array_find(&ref_ctx->debug_info_table, attr->u.uvalue);
}

static void dwarf2_load_one_entry(dwarf2_debug_info_t*);

#define Wine_DW_no_register     0x7FFFFFFF

static unsigned dwarf2_map_register(int regno, const struct module* module)
{
    if (regno == Wine_DW_no_register)
    {
        FIXME("What the heck map reg 0x%x\n",regno);
        return 0;
    }
    return module->cpu->map_dwarf_register(regno, module, FALSE);
}

static enum location_error
compute_location(const struct module *module, const dwarf2_cuhead_t* head,
                 dwarf2_traverse_context_t* ctx, struct location* loc,
                 HANDLE hproc, const struct location* frame)
{
    DWORD_PTR tmp, stack[64];
    unsigned stk;
    unsigned char op;
    BOOL piece_found = FALSE;

    stack[stk = 0] = 0;

    loc->kind = loc_absolute;
    loc->reg = Wine_DW_no_register;

    while (ctx->data < ctx->end_data)
    {
        op = dwarf2_parse_byte(ctx);

        if (op >= DW_OP_lit0 && op <= DW_OP_lit31)
            stack[++stk] = op - DW_OP_lit0;
        else if (op >= DW_OP_reg0 && op <= DW_OP_reg31)
        {
            /* dbghelp APIs don't know how to cope with this anyway
             * (for example 'long long' stored in two registers)
             * FIXME: We should tell winedbg how to deal with it (sigh)
             */
            if (!piece_found)
            {
                DWORD   cvreg = dwarf2_map_register(op - DW_OP_reg0, module);
                if (loc->reg != Wine_DW_no_register)
                    FIXME("Only supporting one reg (%s/%d -> %s/%ld)\n",
                          module->cpu->fetch_regname(loc->reg), loc->reg,
                          module->cpu->fetch_regname(cvreg), cvreg);
                loc->reg = cvreg;
            }
            loc->kind = loc_register;
        }
        else if (op >= DW_OP_breg0 && op <= DW_OP_breg31)
        {
            /* dbghelp APIs don't know how to cope with this anyway
             * (for example 'long long' stored in two registers)
             * FIXME: We should tell winedbg how to deal with it (sigh)
             */
            if (!piece_found)
            {
                DWORD   cvreg = dwarf2_map_register(op - DW_OP_breg0, module);
                if (loc->reg != Wine_DW_no_register)
                    FIXME("Only supporting one breg (%s/%d -> %s/%ld)\n",
                          module->cpu->fetch_regname(loc->reg), loc->reg,
                          module->cpu->fetch_regname(cvreg), cvreg);
                loc->reg = cvreg;
            }
            stack[++stk] = dwarf2_leb128_as_signed(ctx);
            loc->kind = loc_regrel;
        }
        else switch (op)
        {
        case DW_OP_nop:         break;
        case DW_OP_addr:        stack[++stk] = dwarf2_parse_addr_head(ctx, head); break;
        case DW_OP_const1u:     stack[++stk] = dwarf2_parse_byte(ctx); break;
        case DW_OP_const1s:     stack[++stk] = dwarf2_parse_byte(ctx); break;
        case DW_OP_const2u:     stack[++stk] = dwarf2_parse_u2(ctx); break;
        case DW_OP_const2s:     stack[++stk] = dwarf2_parse_u2(ctx); break;
        case DW_OP_const4u:     stack[++stk] = dwarf2_parse_u4(ctx); break;
        case DW_OP_const4s:     stack[++stk] = dwarf2_parse_u4(ctx); break;
        case DW_OP_const8u:     stack[++stk] = dwarf2_parse_u8(ctx); break;
        case DW_OP_const8s:     stack[++stk] = dwarf2_parse_u8(ctx); break;
        case DW_OP_constu:      stack[++stk] = dwarf2_leb128_as_unsigned(ctx); break;
        case DW_OP_consts:      stack[++stk] = dwarf2_leb128_as_signed(ctx); break;
        case DW_OP_dup:         stack[stk + 1] = stack[stk]; stk++; break;
        case DW_OP_drop:        stk--; break;
        case DW_OP_over:        stack[stk + 1] = stack[stk - 1]; stk++; break;
        case DW_OP_pick:        stack[stk + 1] = stack[stk - dwarf2_parse_byte(ctx)]; stk++; break;
        case DW_OP_swap:        tmp = stack[stk]; stack[stk] = stack[stk-1]; stack[stk-1] = tmp; break;
        case DW_OP_rot:         tmp = stack[stk]; stack[stk] = stack[stk-1]; stack[stk-1] = stack[stk-2]; stack[stk-2] = tmp; break;
        case DW_OP_abs:         stack[stk] = sizeof(stack[stk]) == 8 ? llabs((INT64)stack[stk]) : abs((INT32)stack[stk]); break;
        case DW_OP_neg:         stack[stk] = -stack[stk]; break;
        case DW_OP_not:         stack[stk] = ~stack[stk]; break;
        case DW_OP_and:         stack[stk-1] &= stack[stk]; stk--; break;
        case DW_OP_or:          stack[stk-1] |= stack[stk]; stk--; break;
        case DW_OP_minus:       stack[stk-1] -= stack[stk]; stk--; break;
        case DW_OP_mul:         stack[stk-1] *= stack[stk]; stk--; break;
        case DW_OP_plus:        stack[stk-1] += stack[stk]; stk--; break;
        case DW_OP_xor:         stack[stk-1] ^= stack[stk]; stk--; break;
        case DW_OP_shl:         stack[stk-1] <<= stack[stk]; stk--; break;
        case DW_OP_shr:         stack[stk-1] >>= stack[stk]; stk--; break;
        case DW_OP_plus_uconst: stack[stk] += dwarf2_leb128_as_unsigned(ctx); break;
        case DW_OP_shra:        stack[stk-1] = stack[stk-1] / (1 << stack[stk]); stk--; break;
        case DW_OP_div:         stack[stk-1] = stack[stk-1] / stack[stk]; stk--; break;
        case DW_OP_mod:         stack[stk-1] = stack[stk-1] % stack[stk]; stk--; break;
        case DW_OP_ge:          stack[stk-1] = (stack[stk-1] >= stack[stk]); stk--; break;
        case DW_OP_gt:          stack[stk-1] = (stack[stk-1] >  stack[stk]); stk--; break;
        case DW_OP_le:          stack[stk-1] = (stack[stk-1] <= stack[stk]); stk--; break;
        case DW_OP_lt:          stack[stk-1] = (stack[stk-1] <  stack[stk]); stk--; break;
        case DW_OP_eq:          stack[stk-1] = (stack[stk-1] == stack[stk]); stk--; break;
        case DW_OP_ne:          stack[stk-1] = (stack[stk-1] != stack[stk]); stk--; break;
        case DW_OP_skip:        tmp = dwarf2_parse_u2(ctx); ctx->data += tmp; break;
        case DW_OP_bra:
            tmp = dwarf2_parse_u2(ctx);
            if (!stack[stk--]) ctx->data += tmp;
            break;
        case DW_OP_regx:
            tmp = dwarf2_leb128_as_unsigned(ctx);
            if (!piece_found)
            {
                if (loc->reg != Wine_DW_no_register)
                    FIXME("Only supporting one reg\n");
                loc->reg = dwarf2_map_register(tmp, module);
            }
            loc->kind = loc_register;
            break;
        case DW_OP_bregx:
            tmp = dwarf2_leb128_as_unsigned(ctx);
            if (loc->reg != Wine_DW_no_register)
                FIXME("Only supporting one regx\n");
            loc->reg = dwarf2_map_register(tmp, module);
            stack[++stk] = dwarf2_leb128_as_signed(ctx);
            loc->kind = loc_regrel;
            break;
        case DW_OP_fbreg:
            if (loc->reg != Wine_DW_no_register)
                FIXME("Only supporting one reg (%s/%d -> -2)\n",
                      module->cpu->fetch_regname(loc->reg), loc->reg);
            if (frame && frame->kind == loc_register)
            {
                loc->kind = loc_regrel;
                loc->reg = frame->reg;
                stack[++stk] = dwarf2_leb128_as_signed(ctx);
            }
            else if (frame && frame->kind == loc_regrel)
            {
                loc->kind = loc_regrel;
                loc->reg = frame->reg;
                stack[++stk] = dwarf2_leb128_as_signed(ctx) + frame->offset;
            }
            else
            {
                /* FIXME: this could be later optimized by not recomputing
                 * this very location expression
                 */
                loc->kind = loc_dwarf2_block;
                stack[++stk] = dwarf2_leb128_as_signed(ctx);
            }
            break;
        case DW_OP_piece:
            {
                unsigned sz = dwarf2_leb128_as_unsigned(ctx);
                WARN("Not handling OP_piece (size=%d)\n", sz);
                piece_found = TRUE;
            }
            break;
        case DW_OP_deref:
            if (!stk)
            {
                FIXME("Unexpected empty stack\n");
                return loc_err_internal;
            }
            if (loc->reg != Wine_DW_no_register)
            {
                WARN("Too complex expression for deref\n");
                return loc_err_too_complex;
            }
            if (hproc)
            {
                DWORD_PTR addr = stack[stk--];
                DWORD_PTR deref = 0;

                if (!ReadProcessMemory(hproc, (void*)addr, &deref, head->word_size, NULL))
                {
                    WARN("Couldn't read memory at %Ix\n", addr);
                    return loc_err_cant_read;
                }
                stack[++stk] = deref;
            }
            else
            {
               loc->kind = loc_dwarf2_block;
            }
            break;
        case DW_OP_deref_size:
            if (!stk)
            {
                FIXME("Unexpected empty stack\n");
                return loc_err_internal;
            }
            if (loc->reg != Wine_DW_no_register)
            {
                WARN("Too complex expression for deref\n");
                return loc_err_too_complex;
            }
            if (hproc)
            {
                DWORD_PTR addr = stack[stk--];
                BYTE derefsize = dwarf2_parse_byte(ctx);
                DWORD64 deref;

                if (!ReadProcessMemory(hproc, (void*)addr, &deref, derefsize, NULL))
                {
                    WARN("Couldn't read memory at %Ix\n", addr);
                       return loc_err_cant_read;
                }

                switch (derefsize)
                {
                   case 1: stack[++stk] = *(unsigned char*)&deref; break;
                   case 2: stack[++stk] = *(unsigned short*)&deref; break;
                   case 4: stack[++stk] = *(DWORD*)&deref; break;
                   case 8: if (head->word_size >= derefsize) stack[++stk] = deref; break;
                }
            }
            else
            {
                dwarf2_parse_byte(ctx);
                loc->kind = loc_dwarf2_block;
            }
            break;
        case DW_OP_stack_value:
            /* Expected behaviour is that this is the last instruction of this
             * expression and just the "top of stack" value should be put to loc->offset. */
            break;
        default:
            if (op < DW_OP_lo_user) /* as DW_OP_hi_user is 0xFF, we don't need to test against it */
                FIXME("Unhandled attr op: %x\n", op);
            /* FIXME else unhandled extension */
            return loc_err_internal;
        }
    }
    loc->offset = stack[stk];
    return 0;
}

static BOOL dwarf2_compute_location_attr(dwarf2_parse_context_t* ctx,
                                         const dwarf2_debug_info_t* di,
                                         ULONG_PTR dw,
                                         struct location* loc,
                                         const struct location* frame)
{
    struct attribute xloc;

    if (!dwarf2_find_attribute(di, dw, &xloc)) return FALSE;

    switch (xloc.form)
    {
    case DW_FORM_data4:
        if (ctx->head.version < 4)
        {
            loc->kind = loc_dwarf2_location_list;
            loc->reg = Wine_DW_no_register;
            loc->offset = xloc.u.uvalue;
            return TRUE;
        }
        /* fall through */
    case DW_FORM_data1:
    case DW_FORM_data2:
        loc->kind = loc_absolute;
        loc->reg = 0;
        loc->offset = xloc.u.uvalue;
        return TRUE;
    case DW_FORM_udata:
        loc->kind = loc_absolute;
        loc->reg = 0;
        if (xloc.u.uvalue != xloc.u.lluvalue) WARN("Cropping integral value\n");
        loc->offset = xloc.u.uvalue;
        return TRUE;
    case DW_FORM_sdata:
        loc->kind = loc_absolute;
        loc->reg = 0;
        if (xloc.u.svalue != xloc.u.llsvalue) WARN("Cropping integral value\n");
        loc->offset = xloc.u.svalue;
        return TRUE;
    case DW_FORM_data8:
        if (ctx->head.version >= 4)
        {
            loc->kind = loc_absolute;
            loc->reg = 0;
            loc->offset = xloc.u.lluvalue;
            return TRUE;
        }
        /* fall through */
    case DW_FORM_sec_offset:
        loc->kind = loc_dwarf2_location_list;
        loc->reg = Wine_DW_no_register;
        loc->offset = xloc.u.lluvalue;
        return TRUE;
    case DW_FORM_block:
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_exprloc:
        break;
    default: FIXME("Unsupported yet form %Ix\n", xloc.form);
        return FALSE;
    }

    /* assume we have a block form */
    if (dw == DW_AT_frame_base && xloc.u.block.size == 1 && *xloc.u.block.ptr == DW_OP_call_frame_cfa)
    {
        loc->kind = loc_dwarf2_frame_cfa;
        loc->reg = Wine_DW_no_register;
        loc->offset = 0;
    }
    else if (xloc.u.block.size)
    {
        dwarf2_traverse_context_t       lctx;
        enum location_error             err;

        lctx.data = xloc.u.block.ptr;
        lctx.end_data = xloc.u.block.ptr + xloc.u.block.size;

        err = compute_location(ctx->module_ctx->module, &ctx->head, &lctx, loc, NULL, frame);
        if (err < 0)
        {
            loc->kind = loc_error;
            loc->reg = err;
        }
        else if (loc->kind == loc_dwarf2_block)
        {
            unsigned*   ptr = pool_alloc(&ctx->module_ctx->module->pool,
                                         sizeof(unsigned) + xloc.u.block.size);
            *ptr = xloc.u.block.size;
            memcpy(ptr + 1, xloc.u.block.ptr, xloc.u.block.size);
            loc->offset = (ULONG_PTR)ptr;
        }
    }
    return TRUE;
}

static struct symt* dwarf2_lookup_type(const dwarf2_debug_info_t* di)
{
    struct attribute attr;
    dwarf2_debug_info_t* type;

    if (!dwarf2_find_attribute(di, DW_AT_type, &attr))
        /* this is only valid if current language of CU is C or C++ */
        return &symt_get_basic(btVoid, 0)->symt;
    if (!(type = dwarf2_jump_to_debug_info(&attr)))
        return &symt_get_basic(btNoType, 0)->symt;

    if (type == di)
    {
        FIXME("Reference to itself\n");
        return &symt_get_basic(btNoType, 0)->symt;
    }
    if (!type->symt)
    {
        /* load the debug info entity */
        dwarf2_load_one_entry(type);
        if (!type->symt)
        {
            FIXME("Unable to load forward reference for tag %Ix\n", type->abbrev->tag);
            return &symt_get_basic(btNoType, 0)->symt;
        }
    }
    return type->symt;
}

static const char* dwarf2_get_cpp_name(dwarf2_debug_info_t* di, const char* name)
{
    char* last;
    struct attribute diname;
    struct attribute spec;

    if (di->abbrev->tag == DW_TAG_compile_unit || di->abbrev->tag == DW_TAG_partial_unit) return name;

    /* if the di is a definition, but has also a (previous) declaration, then scope must
     * be gotten from declaration not definition
     */
    if (dwarf2_find_attribute(di, DW_AT_specification, &spec) && spec.gotten_from == attr_direct)
    {
        di = dwarf2_jump_to_debug_info(&spec);
        if (!di)
        {
            FIXME("Should have found the debug info entry\n");
            return NULL;
        }
    }

    if (!di->unit_ctx->cpp_name)
    {
        di->unit_ctx->cpp_name = pool_alloc(&di->unit_ctx->pool, MAX_SYM_NAME);
        if (!di->unit_ctx->cpp_name) return name;
    }
    last = di->unit_ctx->cpp_name + MAX_SYM_NAME - strlen(name) - 1;
    strcpy(last, name);

    for (di = di->parent; di; di = di->parent)
    {
        switch (di->abbrev->tag)
        {
        case DW_TAG_namespace:
        case DW_TAG_structure_type:
        case DW_TAG_class_type:
        case DW_TAG_interface_type:
        case DW_TAG_union_type:
            if (dwarf2_find_attribute(di, DW_AT_name, &diname))
            {
                size_t  len = strlen(diname.u.string);
                last -= 2 + len;
                if (last < di->unit_ctx->cpp_name)
                {
                    WARN("Too long C++ qualified identifier for %s... using unqualified identifier\n", debugstr_a(name));
                    return name;
                }
                memcpy(last, diname.u.string, len);
                last[len] = last[len + 1] = ':';
            }
            break;
        default:
            break;
        }
    }
    return last;
}

static unsigned dwarf2_get_num_ranges(const dwarf2_debug_info_t* di)
{
    struct attribute            range;

    if (dwarf2_find_attribute(di, DW_AT_ranges, &range))
    {
        dwarf2_traverse_context_t   traverse;
        unsigned num_ranges = 0;

        traverse.data = di->unit_ctx->module_ctx->sections[section_ranges].address + range.u.uvalue;
        traverse.end_data = di->unit_ctx->module_ctx->sections[section_ranges].address +
            di->unit_ctx->module_ctx->sections[section_ranges].size;

        for (num_ranges = 0; traverse.data + 2 * di->unit_ctx->head.word_size < traverse.end_data; num_ranges++)
        {
            ULONG_PTR low = dwarf2_parse_addr_head(&traverse, &di->unit_ctx->head);
            ULONG_PTR high = dwarf2_parse_addr_head(&traverse, &di->unit_ctx->head);
            if (low == 0 && high == 0) break;
            if (low == (di->unit_ctx->head.word_size == 8 ? (~(DWORD64)0u) : (DWORD64)(~0u)))
                FIXME("unsupported yet (base address selection)\n");
        }
        return num_ranges;
    }
    else
    {
        struct attribute            low_pc;
        struct attribute            high_pc;

        return dwarf2_find_attribute(di, DW_AT_low_pc, &low_pc) &&
            dwarf2_find_attribute(di, DW_AT_high_pc, &high_pc) ? 1 : 0;
    }
}

/* nun_ranges must have been gotten from dwarf2_get_num_ranges() */
static BOOL dwarf2_fill_ranges(const dwarf2_debug_info_t* di, struct addr_range* ranges, unsigned num_ranges)
{
    struct attribute            range;

    if (dwarf2_find_attribute(di, DW_AT_ranges, &range))
    {
        dwarf2_traverse_context_t   traverse;
        unsigned                    index;

        traverse.data = di->unit_ctx->module_ctx->sections[section_ranges].address + range.u.uvalue;
        traverse.end_data = di->unit_ctx->module_ctx->sections[section_ranges].address +
            di->unit_ctx->module_ctx->sections[section_ranges].size;

        for (index = 0; traverse.data + 2 * di->unit_ctx->head.word_size < traverse.end_data; index++)
        {
            ULONG_PTR low = dwarf2_parse_addr_head(&traverse, &di->unit_ctx->head);
            ULONG_PTR high = dwarf2_parse_addr_head(&traverse, &di->unit_ctx->head);
            if (low == 0 && high == 0) break;
            if (low == (di->unit_ctx->head.word_size == 8 ? (~(DWORD64)0u) : (DWORD64)(~0u)))
                FIXME("unsupported yet (base address selection)\n");
            if (index >= num_ranges) return FALSE; /* sanity check */
            ranges[index].low = di->unit_ctx->compiland->address + low;
            ranges[index].high = di->unit_ctx->compiland->address + high;
        }
        return index == num_ranges; /* sanity check */
    }
    else
    {
        struct attribute            low_pc;
        struct attribute            high_pc;

        if (num_ranges != 1 || /* sanity check */
            !dwarf2_find_attribute(di, DW_AT_low_pc, &low_pc) ||
            !dwarf2_find_attribute(di, DW_AT_high_pc, &high_pc))
            return FALSE;
        if (di->unit_ctx->head.version >= 4)
            switch (high_pc.form)
            {
            case DW_FORM_addr:
                break;
            case DW_FORM_sdata:
            case DW_FORM_udata:
                if (high_pc.u.uvalue != high_pc.u.lluvalue) WARN("Cropping integral value\n");
                /* fall through */
            case DW_FORM_data1:
            case DW_FORM_data2:
            case DW_FORM_data4:
            case DW_FORM_data8:
                /* From dwarf4 on, when FORM's class is constant, high_pc is an offset from low_pc */
                high_pc.u.uvalue += low_pc.u.uvalue;
                break;
            default:
                FIXME("Unsupported class for high_pc\n");
                break;
            }
        ranges[0].low = di->unit_ctx->module_ctx->load_offset + low_pc.u.uvalue;
        ranges[0].high = di->unit_ctx->module_ctx->load_offset + high_pc.u.uvalue;
    }
    return TRUE;
}

static struct addr_range* dwarf2_get_ranges(const dwarf2_debug_info_t* di, unsigned* num_ranges)
{
    unsigned nr = dwarf2_get_num_ranges(di);
    struct addr_range* ranges;

    if (nr == 0) return NULL;
    ranges = malloc(nr * sizeof(ranges[0]));
    if (!ranges || !dwarf2_fill_ranges(di, ranges, nr)) return NULL;
    *num_ranges = nr;
    return ranges;
}

/******************************************************************
 *		dwarf2_read_one_debug_info
 *
 * Loads into memory one debug info entry, and recursively its children (if any)
 */
static BOOL dwarf2_read_one_debug_info(dwarf2_parse_context_t* ctx,
                                       dwarf2_traverse_context_t* traverse,
                                       dwarf2_debug_info_t* parent_di,
                                       dwarf2_debug_info_t** pdi)
{
    const dwarf2_abbrev_entry_t*abbrev;
    ULONG_PTR                   entry_code;
    ULONG_PTR                   offset;
    dwarf2_debug_info_t*        di;
    dwarf2_debug_info_t*        child;
    dwarf2_debug_info_t**       where;
    dwarf2_abbrev_entry_attr_t* attr;
    unsigned                    i;
    struct attribute            sibling;

    offset = traverse->data - ctx->module_ctx->sections[ctx->section].address;
    entry_code = dwarf2_leb128_as_unsigned(traverse);
    TRACE("found entry_code %Iu at 0x%Ix\n", entry_code, offset);
    if (!entry_code)
    {
        *pdi = NULL;
        return TRUE;
    }
    abbrev = dwarf2_abbrev_table_find_entry(&ctx->abbrev_table, entry_code);
    if (!abbrev)
    {
	WARN("Cannot find abbrev entry for %Iu at 0x%Ix\n", entry_code, offset);
	return FALSE;
    }
    di = sparse_array_add(&ctx->debug_info_table, offset, &ctx->pool);
    if (!di) return FALSE;
    di->abbrev = abbrev;
    di->symt   = NULL;
    di->parent = parent_di;
    di->unit_ctx = ctx;

    if (abbrev->num_attr)
    {
        di->data = pool_alloc(&ctx->pool, abbrev->num_attr * sizeof(const char*));
        for (i = 0, attr = abbrev->attrs; attr; i++, attr = attr->next)
        {
            di->data[i] = traverse->data;
            dwarf2_swallow_attribute(traverse, &ctx->head, attr);
        }
    }
    else di->data = NULL;
    if (abbrev->have_child)
    {
        vector_init(&di->children, sizeof(dwarf2_debug_info_t*), 0);
        while (traverse->data < traverse->end_data)
        {
            if (!dwarf2_read_one_debug_info(ctx, traverse, di, &child)) return FALSE;
            if (!child) break;
            where = vector_add(&di->children, &ctx->pool);
            if (!where) return FALSE;
            *where = child;
        }
    }
    if (dwarf2_find_attribute(di, DW_AT_sibling, &sibling) &&
        traverse->data != ctx->module_ctx->sections[ctx->section].address + sibling.u.uvalue)
    {
        if (sibling.u.uvalue >= ctx->module_ctx->sections[ctx->section].size)
        {
            FIXME("cursor sibling after section end %s: 0x%Ix 0x%x\n",
                  dwarf2_debug_unit_ctx(ctx), sibling.u.uvalue, ctx->module_ctx->sections[ctx->section].size);
            return FALSE;
        }
        WARN("setting cursor for %s to next sibling <0x%Ix>\n",
             dwarf2_debug_traverse_ctx(traverse), sibling.u.uvalue);
        traverse->data = ctx->module_ctx->sections[ctx->section].address + sibling.u.uvalue;
    }
    *pdi = di;
    return TRUE;
}

static struct vector* dwarf2_get_di_children(dwarf2_debug_info_t* di)
{
    struct attribute    spec;

    while (di)
    {
        if (di->abbrev->have_child)
            return &di->children;
        if (!dwarf2_find_attribute(di, DW_AT_specification, &spec)) break;
        if (!(di = dwarf2_jump_to_debug_info(&spec)))
            FIXME("Should have found the debug info entry\n");
    }
    return NULL;
}

/* reconstruct whether integer is long (contains 'long' only once) */
static BOOL is_long(const char* name)
{
    /* we assume name is made only of basic C keywords:
     *    int long short unsigned signed void float double char _Bool _Complex
     */
    const char* p = strstr(name, "long");
    return p && strstr(p + 4, "long") == NULL;
}

static BOOL is_c_language(dwarf2_parse_context_t* unit_ctx)
{
    return unit_ctx->language == DW_LANG_C ||
        unit_ctx->language == DW_LANG_C89 ||
        unit_ctx->language == DW_LANG_C99;
}

static BOOL is_cpp_language(dwarf2_parse_context_t* unit_ctx)
{
    return unit_ctx->language == DW_LANG_C_plus_plus;
}

static struct symt* dwarf2_parse_base_type(dwarf2_debug_info_t* di)
{
    struct attribute name;
    struct attribute size;
    struct attribute encoding;
    enum BasicType bt;
    BOOL c_language, cpp_language;

    if (di->symt) return di->symt;

    TRACE("%s\n", dwarf2_debug_di(di));

    c_language = is_c_language(di->unit_ctx);
    cpp_language = is_cpp_language(di->unit_ctx);

    if (!dwarf2_find_attribute(di, DW_AT_name, &name))
        name.u.string = NULL;
    if (!dwarf2_find_attribute(di, DW_AT_byte_size, &size)) size.u.uvalue = 0;
    if (!dwarf2_find_attribute(di, DW_AT_encoding, &encoding)) encoding.u.uvalue = DW_ATE_void;

    switch (encoding.u.uvalue)
    {
    case DW_ATE_void:           bt = btVoid; break;
    case DW_ATE_address:        bt = btULong; break;
    case DW_ATE_boolean:        bt = btBool; break;
    case DW_ATE_complex_float:  bt = btComplex; break;
    case DW_ATE_float:          bt = btFloat; break;
    case DW_ATE_signed:         bt = ((c_language || cpp_language) && is_long(name.u.string)) ? btLong : btInt; break;
    case DW_ATE_unsigned:
        if ((c_language || cpp_language) && is_long(name.u.string)) bt = btULong;
        else if (cpp_language && !strcmp(name.u.string, "wchar_t")) bt = btWChar;
        else if (cpp_language && !strcmp(name.u.string, "char8_t")) bt = btChar8;
        else if (cpp_language && !strcmp(name.u.string, "char16_t")) bt = btChar16;
        else if (cpp_language && !strcmp(name.u.string, "char32_t")) bt = btChar32;
        else bt = btUInt;
        break;
    /* on Windows, in C, char == signed char, but not in C++ */
    case DW_ATE_signed_char:    bt = (cpp_language && !strcmp(name.u.string, "signed char")) ? btInt : btChar; break;
    case DW_ATE_unsigned_char:  bt = btUInt; break;
    case DW_ATE_UTF:            bt = (size.u.uvalue == 1) ? btChar8 : (size.u.uvalue == 2 ? btChar16 : btChar32); break;
    default:                    bt = btNoType; break;
    }
    di->symt = &symt_get_basic(bt, size.u.uvalue)->symt;

    if (dwarf2_get_di_children(di)) FIXME("Unsupported children\n");
    return di->symt;
}

static struct symt* dwarf2_parse_typedef(dwarf2_debug_info_t* di)
{
    struct symt*        ref_type;
    struct attribute    name;

    if (di->symt) return di->symt;

    TRACE("%s\n", dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(di, DW_AT_name, &name)) name.u.string = NULL;
    ref_type = dwarf2_lookup_type(di);

    if (name.u.string)
    {
        /* Note: The MS C compiler has tweaks for WCHAR support.
         *       Even if WCHAR is a typedef to wchar_t, wchar_t is emitted as btUInt/2 (it's defined as
         *          unsigned short, so far so good), while WCHAR is emitted as btWChar/2).
         */
        if ((is_c_language(di->unit_ctx) || is_cpp_language(di->unit_ctx)) && !strcmp(name.u.string, "WCHAR"))
            ref_type = &symt_get_basic(btWChar, 2)->symt;
        di->symt = &symt_new_typedef(di->unit_ctx->module_ctx->module, ref_type, name.u.string)->symt;
    }
    if (dwarf2_get_di_children(di)) FIXME("Unsupported children\n");
    return di->symt;
}

static struct symt* dwarf2_parse_pointer_type(dwarf2_debug_info_t* di)
{
    struct symt*        ref_type;
    struct attribute    size;

    if (di->symt) return di->symt;

    TRACE("%s\n", dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(di, DW_AT_byte_size, &size)) size.u.uvalue = di->unit_ctx->module_ctx->module->cpu->word_size;
    ref_type = dwarf2_lookup_type(di);
    di->symt = &symt_new_pointer(di->unit_ctx->module_ctx->module, ref_type, size.u.uvalue)->symt;
    if (dwarf2_get_di_children(di)) FIXME("Unsupported children\n");
    return di->symt;
}

static struct symt* dwarf2_parse_subrange_type(dwarf2_debug_info_t* di)
{
    struct symt*        ref_type;
    struct attribute    name;
    struct attribute    dummy;

    if (di->symt) return di->symt;

    TRACE("%s\n", dwarf2_debug_di(di));

    if (dwarf2_find_attribute(di, DW_AT_name, &name)) FIXME("Found name for subrange %s\n", debugstr_a(name.u.string));
    if (dwarf2_find_attribute(di, DW_AT_byte_size, &dummy)) FIXME("Found byte_size %Iu\n", dummy.u.uvalue);
    if (dwarf2_find_attribute(di, DW_AT_bit_size, &dummy)) FIXME("Found bit_size %Iu\n", dummy.u.uvalue);
    /* for now, we don't support the byte_size nor bit_size about the subrange, and pretend the two
     * types are the same (FIXME)
     */
    ref_type = dwarf2_lookup_type(di);
    di->symt = ref_type;
    if (dwarf2_get_di_children(di)) FIXME("Unsupported children\n");
    return di->symt;
}

static struct symt* dwarf2_parse_array_type(dwarf2_debug_info_t* di)
{
    struct symt* ref_type;
    struct symt* idx_type = NULL;
    struct symt* symt = NULL;
    struct attribute min, max, cnt;
    dwarf2_debug_info_t* child;
    unsigned int i, j;
    const struct vector* children;

    if (di->symt) return di->symt;

    TRACE("%s\n", dwarf2_debug_di(di));

    ref_type = dwarf2_lookup_type(di);

    if (!(children = dwarf2_get_di_children(di)))
    {
        /* fake an array with unknown size */
        /* FIXME: int4 even on 64bit machines??? */
        idx_type = &symt_get_basic(btInt, 4)->symt;
        min.u.uvalue = 0;
        cnt.u.uvalue = 0;
    }
    else for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);
        if (child->symt == &symt_get_basic(btNoType, 0)->symt) continue;
        switch (child->abbrev->tag)
        {
        case DW_TAG_subrange_type:
            idx_type = dwarf2_lookup_type(child);
            if (!dwarf2_find_attribute(child, DW_AT_lower_bound, &min))
                min.u.uvalue = 0;
            if (dwarf2_find_attribute(child, DW_AT_upper_bound, &max))
                cnt.u.uvalue = max.u.uvalue + 1 - min.u.uvalue;
            else if (!dwarf2_find_attribute(child, DW_AT_count, &cnt))
                cnt.u.uvalue = 0;
            break;
        case DW_TAG_enumeration_type:
            symt = dwarf2_parse_enumeration_type(child);
            if (symt_check_tag(symt, SymTagEnum))
            {
                struct symt_enum* enum_symt = (struct symt_enum*)symt;
                idx_type = enum_symt->base_type;
                min.u.uvalue = ~0U;
                max.u.uvalue = ~0U;
                for (j = 0; j < enum_symt->vchildren.num_elts; ++j)
                {
                    struct symt** pc = vector_at(&enum_symt->vchildren, j);
                    if (pc && symt_check_tag(*pc, SymTagData))
                    {
                        struct symt_data* elt = (struct symt_data*)(*pc);
                        if (elt->u.value.lVal < min.u.uvalue)
                            min.u.uvalue = elt->u.value.lVal;
                        if (elt->u.value.lVal > max.u.uvalue)
                            max.u.uvalue = elt->u.value.lVal;
                    }
                }
            }
            break;
        default:
            FIXME("Unhandled Tag type 0x%Ix at %s\n",
                  child->abbrev->tag, dwarf2_debug_di(di));
            break;
        }
    }
    di->symt = &symt_new_array(di->unit_ctx->module_ctx->module, min.u.uvalue, cnt.u.uvalue, ref_type, idx_type)->symt;
    return di->symt;
}

static struct symt* dwarf2_parse_const_type(dwarf2_debug_info_t* di)
{
    struct symt* ref_type;

    if (di->symt) return di->symt;

    TRACE("%s\n", dwarf2_debug_di(di));

    ref_type = dwarf2_lookup_type(di);
    if (dwarf2_get_di_children(di)) FIXME("Unsupported children\n");
    di->symt = ref_type;

    return ref_type;
}

static struct symt* dwarf2_parse_volatile_type(dwarf2_debug_info_t* di)
{
    struct symt* ref_type;

    if (di->symt) return di->symt;

    TRACE("%s\n", dwarf2_debug_di(di));

    ref_type = dwarf2_lookup_type(di);
    if (dwarf2_get_di_children(di)) FIXME("Unsupported children\n");
    di->symt = ref_type;

    return ref_type;
}

static struct symt* dwarf2_parse_restrict_type(dwarf2_debug_info_t* di)
{
    struct symt* ref_type;

    if (di->symt) return di->symt;

    TRACE("%s\n", dwarf2_debug_di(di));

    ref_type = dwarf2_lookup_type(di);
    if (dwarf2_get_di_children(di)) FIXME("Unsupported children\n");
    di->symt = ref_type;

    return ref_type;
}

static struct symt* dwarf2_parse_unspecified_type(dwarf2_debug_info_t* di)
{
    struct attribute name;
    struct symt* basic;

    TRACE("%s\n", dwarf2_debug_di(di));

    if (di->symt) return di->symt;

    basic = &symt_get_basic(btVoid, 0)->symt;
    if (dwarf2_find_attribute(di, DW_AT_name, &name))
        /* define the missing type as a typedef to void... */
        di->symt = &symt_new_typedef(di->unit_ctx->module_ctx->module, basic, name.u.string)->symt;
    else /* or use void if it doesn't even have a name */
        di->symt = basic;

    if (dwarf2_get_di_children(di)) FIXME("Unsupported children\n");
    return di->symt;
}

static struct symt* dwarf2_parse_reference_type(dwarf2_debug_info_t* di)
{
    struct symt* ref_type = NULL;

    if (di->symt) return di->symt;

    TRACE("%s\n", dwarf2_debug_di(di));

    ref_type = dwarf2_lookup_type(di);
    /* FIXME: for now, we hard-wire C++ references to pointers */
    di->symt = &symt_new_pointer(di->unit_ctx->module_ctx->module, ref_type,
                                 di->unit_ctx->module_ctx->module->cpu->word_size)->symt;

    if (dwarf2_get_di_children(di)) FIXME("Unsupported children\n");

    return di->symt;
}

static void dwarf2_parse_udt_member(dwarf2_debug_info_t* di,
                                    struct symt_udt* parent)
{
    struct symt* elt_type;
    struct attribute name;
    struct attribute bit_size;
    struct attribute bit_offset;
    struct location  loc;

    assert(parent);

    TRACE("%s\n", dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(di, DW_AT_name, &name)) name.u.string = NULL;
    elt_type = dwarf2_lookup_type(di);
    if (dwarf2_compute_location_attr(di->unit_ctx, di, DW_AT_data_member_location, &loc, NULL))
    {
        if (loc.kind != loc_absolute)
        {
            FIXME("Unexpected offset computation for member %s in %ls!%s\n",
                  debugstr_a(name.u.string), di->unit_ctx->module_ctx->module->modulename, debugstr_a(parent->hash_elt.name));
            loc.offset = 0;
        }
        else
            TRACE("found member_location at %s -> %Iu\n",
                  dwarf2_debug_di(di), loc.offset);
    }
    else
        loc.offset = 0;
    if (!dwarf2_find_attribute(di, DW_AT_bit_size, &bit_size))
        bit_size.u.uvalue = 0;
    if (dwarf2_find_attribute(di, DW_AT_bit_offset, &bit_offset))
    {
        /* FIXME: we should only do this when implementation is LSB (which is
         * the case on i386 processors)
         */
        struct attribute nbytes;
        if (!dwarf2_find_attribute(di, DW_AT_byte_size, &nbytes))
        {
            DWORD64     size;
            nbytes.u.uvalue = symt_get_info(di->unit_ctx->module_ctx->module, elt_type, TI_GET_LENGTH, &size) ?
                (ULONG_PTR)size : 0;
        }
        bit_offset.u.uvalue = nbytes.u.uvalue * 8 - bit_offset.u.uvalue - bit_size.u.uvalue;
    }
    else bit_offset.u.uvalue = 0;
    symt_add_udt_element(di->unit_ctx->module_ctx->module, parent, name.u.string,
                         symt_ptr_to_symref(elt_type),
                         loc.offset, bit_offset.u.uvalue,
                         bit_size.u.uvalue);

    if (dwarf2_get_di_children(di)) FIXME("Unsupported children\n");
}

static struct symt* dwarf2_parse_subprogram(dwarf2_debug_info_t* di);

static struct symt* dwarf2_parse_udt_type(dwarf2_debug_info_t* di,
                                          enum UdtKind udt)
{
    struct attribute    name;
    struct attribute    size;
    struct vector*      children;
    dwarf2_debug_info_t*child;
    unsigned int        i;

    if (di->symt) return di->symt;

    TRACE("%s\n", dwarf2_debug_di(di));

    /* quirk... FIXME provide real support for anonymous UDTs */
    if (!dwarf2_find_attribute(di, DW_AT_name, &name))
        name.u.string = "<unnamed-tag>";
    if (!dwarf2_find_attribute(di, DW_AT_byte_size, &size)) size.u.uvalue = 0;

    di->symt = &symt_new_udt(di->unit_ctx->module_ctx->module, dwarf2_get_cpp_name(di, name.u.string),
                             size.u.uvalue, udt)->symt;

    children = dwarf2_get_di_children(di);
    if (children) for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);

        switch (child->abbrev->tag)
        {
        case DW_TAG_array_type:
            dwarf2_parse_array_type(child);
            break;
        case DW_TAG_member:
            /* FIXME: should I follow the sibling stuff ?? */
            if (symt_check_tag(di->symt, SymTagUDT))
                dwarf2_parse_udt_member(child, (struct symt_udt*)di->symt);
            break;
        case DW_TAG_enumeration_type:
            dwarf2_parse_enumeration_type(child);
            break;
        case DW_TAG_subprogram:
            dwarf2_parse_subprogram(child);
            break;
        case DW_TAG_const_type:
            dwarf2_parse_const_type(child);
            break;
        case DW_TAG_volatile_type:
            dwarf2_parse_volatile_type(child);
            break;
        case DW_TAG_pointer_type:
            dwarf2_parse_pointer_type(child);
            break;
        case DW_TAG_subrange_type:
            dwarf2_parse_subrange_type(child);
            break;
        case DW_TAG_structure_type:
        case DW_TAG_class_type:
        case DW_TAG_union_type:
        case DW_TAG_typedef:
            /* FIXME: we need to handle nested udt definitions */
        case DW_TAG_inheritance:
        case DW_TAG_interface_type:
        case DW_TAG_template_type_param:
        case DW_TAG_template_value_param:
        case DW_TAG_variable:
        case DW_TAG_imported_declaration:
        case DW_TAG_ptr_to_member_type:
        case DW_TAG_GNU_template_template_param:
        case DW_TAG_GNU_template_parameter_pack:
        case DW_TAG_GNU_formal_parameter_pack:
            /* FIXME: some C++ related stuff */
            break;
        default:
            FIXME("Unhandled Tag type 0x%Ix at %s\n",
                  child->abbrev->tag, dwarf2_debug_di(di));
            break;
        }
    }

    return di->symt;
}

static void dwarf2_parse_enumerator(dwarf2_debug_info_t* di,
                                    struct symt_enum* parent)
{
    VARIANT             v;
    struct attribute    name;
    struct attribute    value;

    TRACE("%s\n", dwarf2_debug_di(di));

    V_VT(&v) = VT_EMPTY;

    if (!dwarf2_find_attribute(di, DW_AT_name, &name)) return;
    if (dwarf2_find_attribute(di, DW_AT_const_value, &value) &&
        symt_check_tag(parent->base_type, SymTagBaseType))
    {
        if (!dwarf2_fill_in_variant(di->unit_ctx->module_ctx->module, &v, &value, parent->base_type))
            TRACE("Failed to get variant\n");
    }
    symt_add_enum_element(di->unit_ctx->module_ctx->module, parent, name.u.string, &v);

    if (dwarf2_get_di_children(di)) FIXME("Unsupported children\n");
}

static struct symt* dwarf2_parse_enumeration_type(dwarf2_debug_info_t* di)
{
    struct attribute    name;
    struct attribute    attrtype;
    dwarf2_debug_info_t*ditype;
    struct symt*        type = NULL;
    struct vector*      children;
    dwarf2_debug_info_t*child;
    unsigned int        i;

    if (di->symt) return di->symt;

    TRACE("%s\n", dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(di, DW_AT_name, &name)) name.u.string = NULL;
    if (dwarf2_find_attribute(di, DW_AT_type, &attrtype) && (ditype = dwarf2_jump_to_debug_info(&attrtype)) != NULL)
        type = symt_get_real_type(ditype->symt);
    if (!type || type->tag != SymTagBaseType) /* no type found for this enumeration, construct it from size */
    {
        struct attribute    size;
        struct symt_basic*  basetype;

        if (!dwarf2_find_attribute(di, DW_AT_byte_size, &size)) size.u.uvalue = 4;
        switch (size.u.uvalue)
        {
        case 1: basetype = symt_get_basic(btInt, 1); break;
        case 2: basetype = symt_get_basic(btInt, 2); break;
        default:
        case 4: basetype = symt_get_basic(btInt, 4); break;
        case 8: basetype = symt_get_basic(btInt, 8); break;
        }
        type = &basetype->symt;
    }

    di->symt = &symt_new_enum(di->unit_ctx->module_ctx->module, name.u.string, type)->symt;
    children = dwarf2_get_di_children(di);

    if (children) for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);

        switch (child->abbrev->tag)
        {
        case DW_TAG_enumerator:
            if (symt_check_tag(di->symt, SymTagEnum))
                dwarf2_parse_enumerator(child, (struct symt_enum*)di->symt);
            break;
        default:
            FIXME("Unhandled Tag type 0x%Ix at %s\n",
                  di->abbrev->tag, dwarf2_debug_di(di));
	}
    }
    return di->symt;
}

/* structure used to pass information around when parsing a subprogram */
typedef struct dwarf2_subprogram_s
{
    dwarf2_parse_context_t*     ctx;
    struct symt_function*       top_func;
    struct symt_function*       current_func; /* either symt_function* or symt_inlinesite* */
    struct symt_block*          current_block;
    BOOL                        non_computed_variable;
    struct location             frame;
} dwarf2_subprogram_t;

/******************************************************************
 *		dwarf2_parse_variable
 *
 * Parses any variable (parameter, local/global variable)
 */
static void dwarf2_parse_variable(dwarf2_subprogram_t* subpgm,
                                  dwarf2_debug_info_t* di)
{
    struct symt*        param_type;
    struct attribute    name, value;
    struct location     loc;
    BOOL                is_pmt;

    TRACE("%s\n", dwarf2_debug_di(di));

    is_pmt = !subpgm->current_block && di->abbrev->tag == DW_TAG_formal_parameter;
    param_type = dwarf2_lookup_type(di);
        
    if (!dwarf2_find_attribute(di, DW_AT_name, &name)) {
	/* cannot do much without the name, the functions below won't like it. */
        return;
    }
    if (dwarf2_compute_location_attr(subpgm->ctx, di, DW_AT_location,
                                     &loc, &subpgm->frame))
    {
        struct attribute ext;

	TRACE("found parameter %s (kind=%d, offset=%Id, reg=%d) at %s\n",
              debugstr_a(name.u.string), loc.kind, loc.offset, loc.reg,
              dwarf2_debug_unit_ctx(subpgm->ctx));

        switch (loc.kind)
        {
        case loc_error:
            break;
        case loc_absolute:
            /* it's a global variable */
            if (!dwarf2_find_attribute(di, DW_AT_external, &ext))
                ext.u.uvalue = 0;
            loc.offset += subpgm->ctx->module_ctx->load_offset;
            if (subpgm->top_func)
            {
                if (ext.u.uvalue) WARN("unexpected global inside a function\n");
                symt_add_func_local(subpgm->ctx->module_ctx->module, subpgm->current_func,
                                    DataIsStaticLocal, &loc, subpgm->current_block,
                                    symt_ptr_to_symref(param_type), dwarf2_get_cpp_name(di, name.u.string));
            }
            else
            {
                symt_new_global_variable(subpgm->ctx->module_ctx->module,
                                         ext.u.uvalue ? NULL : subpgm->ctx->compiland,
                                         dwarf2_get_cpp_name(di, name.u.string), !ext.u.uvalue,
                                         loc, 0, symt_ptr_to_symref(param_type));
            }
            break;
        default:
            subpgm->non_computed_variable = TRUE;
            /* fall through */
        case loc_register:
        case loc_regrel:
            /* either a pmt/variable relative to frame pointer or
             * pmt/variable in a register
             */
            if (subpgm->current_func)
                symt_add_func_local(subpgm->ctx->module_ctx->module, subpgm->current_func,
                                    is_pmt ? DataIsParam : DataIsLocal,
                                    &loc, subpgm->current_block, symt_ptr_to_symref(param_type), name.u.string);
            break;
        }
    }
    else if (dwarf2_find_attribute(di, DW_AT_const_value, &value))
    {
        VARIANT v;

        if (!dwarf2_fill_in_variant(subpgm->ctx->module_ctx->module, &v, &value, param_type))
            FIXME("Unsupported form for const value %s (%Ix)\n",
                  debugstr_a(name.u.string), value.form);

        if (subpgm->current_func)
        {
            if (is_pmt) WARN("Constant parameter %s reported as local variable in function '%s'\n",
                             debugstr_a(name.u.string), debugstr_a(subpgm->current_func->hash_elt.name));
            di->symt = &symt_add_func_constant(subpgm->ctx->module_ctx->module,
                                               subpgm->current_func, subpgm->current_block,
                                               symt_ptr_to_symref(param_type), name.u.string, &v)->symt;
        }
        else
            di->symt = &symt_new_constant(subpgm->ctx->module_ctx->module, subpgm->ctx->compiland,
                                          name.u.string, symt_ptr_to_symref(param_type), &v)->symt;
    }
    else
    {
        if (subpgm->current_func)
        {
            /* local variable has been optimized away... report anyway */
            loc.kind = loc_error;
            loc.reg = loc_err_no_location;
            symt_add_func_local(subpgm->ctx->module_ctx->module, subpgm->current_func,
                                is_pmt ? DataIsParam : DataIsLocal,
                                &loc, subpgm->current_block, symt_ptr_to_symref(param_type), name.u.string);
        }
        else
        {
            struct attribute is_decl;
            /* only warn when di doesn't represent a declaration */
            if (!dwarf2_find_attribute(di, DW_AT_declaration, &is_decl) ||
                !is_decl.u.uvalue || is_decl.gotten_from != attr_direct)
                WARN("dropping global variable %s which has been optimized away\n", debugstr_a(name.u.string));
        }
    }
    if (dwarf2_get_di_children(di)) FIXME("Unsupported children\n");
}

static void dwarf2_parse_subprogram_label(dwarf2_subprogram_t* subpgm,
                                          const dwarf2_debug_info_t* di)
{
    struct attribute    name;
    struct attribute    low_pc;
    struct location     loc;

    TRACE("%s\n", dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(di, DW_AT_name, &name))
        name.u.string = NULL;
    if (dwarf2_find_attribute(di, DW_AT_low_pc, &low_pc))
    {
        loc.kind = loc_absolute;
        loc.offset = subpgm->ctx->module_ctx->load_offset + low_pc.u.uvalue - subpgm->top_func->ranges[0].low;
        symt_add_function_point(subpgm->ctx->module_ctx->module, subpgm->top_func, SymTagLabel,
                                &loc, name.u.string);
    }
    else
        WARN("Label %s inside function %s doesn't have an address... don't register it\n",
             debugstr_a(name.u.string), debugstr_a(subpgm->top_func->hash_elt.name));
}

static void dwarf2_parse_subprogram_block(dwarf2_subprogram_t* subpgm,
                                          dwarf2_debug_info_t* di);

static struct symt* dwarf2_parse_subroutine_type(dwarf2_debug_info_t* di);

static void dwarf2_parse_inlined_subroutine(dwarf2_subprogram_t* subpgm,
                                            dwarf2_debug_info_t* di)
{
    struct attribute    name;
    struct symt_function* inlined;
    struct vector*      children;
    dwarf2_debug_info_t*child;
    unsigned int        i;
    unsigned            num_ranges;

    TRACE("%s\n", dwarf2_debug_di(di));

    if (!(num_ranges = dwarf2_get_num_ranges(di)))
    {
        WARN("cannot read ranges\n");
        return;
    }
    if (!dwarf2_find_attribute(di, DW_AT_name, &name))
    {
        FIXME("No name for function... dropping function\n");
        return;
    }

    inlined = symt_new_inlinesite(subpgm->ctx->module_ctx->module,
                                  subpgm->top_func,
                                  subpgm->current_block ? &subpgm->current_block->symt : &subpgm->current_func->symt,
                                  dwarf2_get_cpp_name(di, name.u.string),
                                  symt_ptr_to_symref(dwarf2_parse_subroutine_type(di)), num_ranges);
    subpgm->current_func = inlined;
    subpgm->current_block = NULL;

    if (!dwarf2_fill_ranges(di, inlined->ranges, num_ranges))
    {
        FIXME("Unexpected situation\n");
        inlined->num_ranges = 0;
    }

    children = dwarf2_get_di_children(di);
    if (children) for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);

        switch (child->abbrev->tag)
        {
        case DW_TAG_formal_parameter:
        case DW_TAG_variable:
            dwarf2_parse_variable(subpgm, child);
            break;
        case DW_TAG_lexical_block:
            dwarf2_parse_subprogram_block(subpgm, child);
            break;
        case DW_TAG_inlined_subroutine:
            dwarf2_parse_inlined_subroutine(subpgm, child);
            break;
        case DW_TAG_label:
            dwarf2_parse_subprogram_label(subpgm, child);
            break;
        case DW_TAG_GNU_call_site:
            /* this isn't properly supported by dbghelp interface. skip it for now */
            break;
        default:
            FIXME("Unhandled Tag type 0x%Ix at %s\n",
                  child->abbrev->tag, dwarf2_debug_di(di));
        }
    }
    subpgm->current_block = symt_check_tag(subpgm->current_func->container, SymTagBlock) ?
        (struct symt_block*)subpgm->current_func->container : NULL;
    subpgm->current_func = (struct symt_function*)symt_get_upper_inlined(subpgm->current_func);
}

static void dwarf2_parse_subprogram_block(dwarf2_subprogram_t* subpgm,
					  dwarf2_debug_info_t* di)
{
    unsigned int        num_ranges;
    struct vector*      children;
    dwarf2_debug_info_t*child;
    unsigned int        i;

    TRACE("%s\n", dwarf2_debug_di(di));

    num_ranges = dwarf2_get_num_ranges(di);
    if (!num_ranges)
    {
        WARN("no ranges\n");
        return;
    }

    /* Dwarf tends to keep the structure of the C/C++ program, and emits DW_TAG_lexical_block
     * for every block the in source program.
     * With inlining and other optimizations, code for a block no longer lies in a contiguous
     * chunk of memory. It's hence described with (potentially) multiple chunks of memory.
     * Then each variable of each block is attached to its block. (A)
     *
     * PDB on the other hand no longer emits block information, and merge variable information
     * at function level (actually function and each inline site).
     * For example, if several variables of same name exist in different blocks of a function,
     * only one entry for that name will exist. The information stored in (A) will point
     * to the correct instance as defined by C/C++ scoping rules.
     *
     * (A) in all cases, there is information telling for each address of the function if a
     *     variable is accessible, and if so, how to get its value.
     *
     * DbgHelp only exposes a contiguous chunk of memory for a block.
     *
     * => Store internally all the ranges of addresses in a block, but only expose the size
     *    of the first chunk of memory.
     *    Otherwise, it would break the rule: blocks' chunks don't overlap.
     * Note: This could mislead some programs using the blocks' address and size information.
     *       That's very unlikely to happen (most will use the APIs from DbgHelp, which will
     *       hide this information to the caller).
     */
    subpgm->current_block = symt_open_func_block(subpgm->ctx->module_ctx->module, subpgm->current_func,
                                                 subpgm->current_block, num_ranges);
    if (!dwarf2_fill_ranges(di, subpgm->current_block->ranges, num_ranges))
    {
        FIXME("Unexpected situation\n");
        subpgm->current_block->num_ranges = 0;
    }

    children = dwarf2_get_di_children(di);
    if (children) for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);

        switch (child->abbrev->tag)
        {
        case DW_TAG_inlined_subroutine:
            dwarf2_parse_inlined_subroutine(subpgm, child);
            break;
        case DW_TAG_variable:
            dwarf2_parse_variable(subpgm, child);
            break;
        case DW_TAG_pointer_type:
            dwarf2_parse_pointer_type(child);
            break;
        case DW_TAG_subroutine_type:
            if (!child->symt)
                child->symt = dwarf2_parse_subroutine_type(child);
            break;
        case DW_TAG_const_type:
            dwarf2_parse_const_type(child);
            break;
        case DW_TAG_lexical_block:
            dwarf2_parse_subprogram_block(subpgm, child);
            break;
        case DW_TAG_subprogram:
            /* FIXME: likely a declaration (to be checked)
             * skip it for now
             */
            break;
        case DW_TAG_formal_parameter:
            /* FIXME: likely elements for exception handling (GCC flavor)
             * Skip it for now
             */
            break;
        case DW_TAG_imported_module:
            /* C++ stuff to be silenced (for now) */
            break;
        case DW_TAG_GNU_call_site:
            /* this isn't properly supported by dbghelp interface. skip it for now */
            break;
        case DW_TAG_label:
            dwarf2_parse_subprogram_label(subpgm, child);
            break;
        case DW_TAG_class_type:
        case DW_TAG_structure_type:
        case DW_TAG_union_type:
        case DW_TAG_enumeration_type:
        case DW_TAG_typedef:
            /* the type referred to will be loaded when we need it, so skip it */
            break;
        default:
            FIXME("Unhandled Tag type 0x%Ix at %s\n",
                  child->abbrev->tag, dwarf2_debug_di(di));
        }
    }

    subpgm->current_block = symt_close_func_block(subpgm->ctx->module_ctx->module, subpgm->current_func, subpgm->current_block);
}

static struct symt* dwarf2_parse_subprogram(dwarf2_debug_info_t* di)
{
    struct attribute name;
    struct addr_range* addr_ranges;
    unsigned num_addr_ranges;
    struct attribute is_decl;
    struct attribute inline_flags;
    dwarf2_subprogram_t subpgm;
    struct vector* children;
    dwarf2_debug_info_t* child;
    unsigned int i;

    if (di->symt) return di->symt;

    TRACE("%s\n", dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(di, DW_AT_name, &name))
    {
        WARN("No name for function... dropping function\n");
        return NULL;
    }
    /* if it's an abstract representation of an inline function, there should be
     * a concrete object that we'll handle
     */
    if (dwarf2_find_attribute(di, DW_AT_inline, &inline_flags) &&
        inline_flags.gotten_from == attr_direct &&
        inline_flags.u.uvalue != DW_INL_not_inlined)
    {
        TRACE("Function %s declared as inlined (%Id)... skipping\n",
              debugstr_a(name.u.string), inline_flags.u.uvalue);
        return NULL;
    }

    if (dwarf2_find_attribute(di, DW_AT_declaration, &is_decl) &&
        is_decl.u.uvalue && is_decl.gotten_from == attr_direct)
    {
        /* it's a real declaration, skip it */
        return NULL;
    }
    if ((addr_ranges = dwarf2_get_ranges(di, &num_addr_ranges)) == NULL)
    {
        WARN("cannot get range for %s\n", debugstr_a(name.u.string));
        return NULL;
    }
    /* As functions (defined as inline assembly) get debug info with dwarf
     * (not the case for stabs), we just drop Wine's thunks here...
     * Actual thunks will be created in elf_module from the symbol table
     */
    if (elf_is_in_thunk_area(di->unit_ctx->module_ctx->load_offset + addr_ranges[0].low, di->unit_ctx->module_ctx->thunks) >= 0)
    {
        free(addr_ranges);
        return NULL;
    }

    subpgm.top_func = symt_new_function(di->unit_ctx->module_ctx->module, di->unit_ctx->compiland,
                                        dwarf2_get_cpp_name(di, name.u.string),
                                        addr_ranges[0].low, addr_ranges[0].high - addr_ranges[0].low,
                                        symt_ptr_to_symref(dwarf2_parse_subroutine_type(di)));
    if (num_addr_ranges > 1)
        WARN("Function %s has multiple address ranges, only using the first one\n", debugstr_a(name.u.string));
    free(addr_ranges);

    subpgm.current_func = subpgm.top_func;
    di->symt = &subpgm.top_func->symt;
    subpgm.ctx = di->unit_ctx;
    if (!dwarf2_compute_location_attr(di->unit_ctx, di, DW_AT_frame_base,
                                      &subpgm.frame, NULL))
    {
        /* on stack !! */
        subpgm.frame.kind = loc_regrel;
        subpgm.frame.reg = di->unit_ctx->module_ctx->module->cpu->frame_regno;
        subpgm.frame.offset = 0;
    }
    subpgm.non_computed_variable = FALSE;
    subpgm.current_block = NULL;

    children = dwarf2_get_di_children(di);
    if (children) for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);

        switch (child->abbrev->tag)
        {
        case DW_TAG_variable:
        case DW_TAG_formal_parameter:
            dwarf2_parse_variable(&subpgm, child);
            break;
        case DW_TAG_lexical_block:
            dwarf2_parse_subprogram_block(&subpgm, child);
            break;
        case DW_TAG_inlined_subroutine:
            dwarf2_parse_inlined_subroutine(&subpgm, child);
            break;
        case DW_TAG_pointer_type:
            dwarf2_parse_pointer_type(child);
            break;
        case DW_TAG_const_type:
            dwarf2_parse_const_type(child);
            break;
        case DW_TAG_subprogram:
            /* FIXME: likely a declaration (to be checked)
             * skip it for now
             */
            break;
        case DW_TAG_label:
            dwarf2_parse_subprogram_label(&subpgm, child);
            break;
        case DW_TAG_class_type:
        case DW_TAG_structure_type:
        case DW_TAG_union_type:
        case DW_TAG_enumeration_type:
        case DW_TAG_typedef:
            /* the type referred to will be loaded when we need it, so skip it */
            break;
        case DW_TAG_unspecified_parameters:
        case DW_TAG_template_type_param:
        case DW_TAG_template_value_param:
        case DW_TAG_GNU_call_site:
        case DW_TAG_GNU_template_parameter_pack:
        case DW_TAG_GNU_formal_parameter_pack:
            /* FIXME: no support in dbghelp's internals so far */
            break;
        default:
            FIXME("Unhandled Tag type 0x%Ix at %s\n",
                  child->abbrev->tag, dwarf2_debug_di(di));
	}
    }

    if (subpgm.non_computed_variable || subpgm.frame.kind >= loc_user)
    {
        symt_add_function_point(di->unit_ctx->module_ctx->module, subpgm.top_func, SymTagCustom,
                                &subpgm.frame, NULL);
    }

    return di->symt;
}

static struct symt* dwarf2_parse_subroutine_type(dwarf2_debug_info_t* di)
{
    struct symt* ret_type;
    struct symt_function_signature* sig_type;
    struct vector* children;
    dwarf2_debug_info_t* child;
    unsigned int i;

    TRACE("%s\n", dwarf2_debug_di(di));

    ret_type = dwarf2_lookup_type(di);

    /* FIXME: assuming C source code */
    sig_type = symt_new_function_signature(di->unit_ctx->module_ctx->module, ret_type, CV_CALL_FAR_C);

    children = dwarf2_get_di_children(di);
    if (children) for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);

        switch (child->abbrev->tag)
        {
        case DW_TAG_formal_parameter:
            symt_add_function_signature_parameter(di->unit_ctx->module_ctx->module, sig_type,
                                                  dwarf2_lookup_type(child));
            break;
        case DW_TAG_unspecified_parameters:
            WARN("Unsupported unspecified parameters\n");
            break;
	}
    }

    return &sig_type->symt;
}

static void dwarf2_parse_namespace(dwarf2_debug_info_t* di)
{
    struct vector*          children;
    dwarf2_debug_info_t*    child;
    unsigned int            i;

    if (di->symt) return;

    TRACE("%s\n", dwarf2_debug_di(di));

    di->symt = &symt_get_basic(btVoid, 0)->symt;

    children = dwarf2_get_di_children(di);
    if (children) for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);
        dwarf2_load_one_entry(child);
    }
}

static void dwarf2_parse_imported_unit(dwarf2_debug_info_t* di)
{
    struct attribute imp;

    if (di->symt) return;

    TRACE("%s\n", dwarf2_debug_di(di));

    if (dwarf2_find_attribute(di, DW_AT_import, &imp))
    {
        dwarf2_debug_info_t* jmp = dwarf2_jump_to_debug_info(&imp);
        if (jmp) di->symt = jmp->symt;
        else FIXME("Couldn't load imported CU\n");
    }
    else
        FIXME("Couldn't find import attribute\n");
}

static void dwarf2_load_one_entry(dwarf2_debug_info_t* di)
{
    switch (di->abbrev->tag)
    {
    case DW_TAG_typedef:
        dwarf2_parse_typedef(di);
        break;
    case DW_TAG_base_type:
        dwarf2_parse_base_type(di);
        break;
    case DW_TAG_pointer_type:
        dwarf2_parse_pointer_type(di);
        break;
    case DW_TAG_class_type:
        dwarf2_parse_udt_type(di, UdtClass);
        break;
    case DW_TAG_structure_type:
        dwarf2_parse_udt_type(di, UdtStruct);
        break;
    case DW_TAG_union_type:
        dwarf2_parse_udt_type(di, UdtUnion);
        break;
    case DW_TAG_array_type:
        dwarf2_parse_array_type(di);
        break;
    case DW_TAG_const_type:
        dwarf2_parse_const_type(di);
        break;
    case DW_TAG_volatile_type:
        dwarf2_parse_volatile_type(di);
        break;
    case DW_TAG_restrict_type:
        dwarf2_parse_restrict_type(di);
        break;
    case DW_TAG_unspecified_type:
        dwarf2_parse_unspecified_type(di);
        break;
    case DW_TAG_reference_type:
    case DW_TAG_rvalue_reference_type:
        dwarf2_parse_reference_type(di);
        break;
    case DW_TAG_enumeration_type:
        dwarf2_parse_enumeration_type(di);
        break;
    case DW_TAG_subprogram:
        dwarf2_parse_subprogram(di);
        break;
    case DW_TAG_subroutine_type:
        if (!di->symt)
            di->symt = dwarf2_parse_subroutine_type(di);
        break;
    case DW_TAG_variable:
        {
            dwarf2_subprogram_t subpgm;

            subpgm.ctx = di->unit_ctx;
            subpgm.top_func = subpgm.current_func = NULL;
            subpgm.current_block = NULL;
            subpgm.frame.kind = loc_absolute;
            subpgm.frame.offset = 0;
            subpgm.frame.reg = Wine_DW_no_register;
            dwarf2_parse_variable(&subpgm, di);
        }
        break;
    case DW_TAG_namespace:
        dwarf2_parse_namespace(di);
        break;
    case DW_TAG_subrange_type:
        dwarf2_parse_subrange_type(di);
        break;
    case DW_TAG_imported_unit:
        dwarf2_parse_imported_unit(di);
        break;
    /* keep it silent until we need DW_OP_call_xxx support */
    case DW_TAG_dwarf_procedure:
    /* silence a couple of non-C language defines (mainly C++ but others too) */
    case DW_TAG_imported_module:
    case DW_TAG_imported_declaration:
    case DW_TAG_interface_type:
    case DW_TAG_module:
    case DW_TAG_ptr_to_member_type:
        break;
    default:
        FIXME("Unhandled Tag type 0x%Ix at %s\n",
              di->abbrev->tag, dwarf2_debug_di(di));
    }
}

static void dwarf2_set_line_number(struct module* module, ULONG_PTR address,
                                   const struct vector* v, unsigned file, unsigned line)
{
    struct symt_function*       func;
    struct symt_function*       inlined;
    struct symt_ht*             symt;
    unsigned*                   psrc;

    if (!file || !(psrc = vector_at(v, file - 1))) return;

    TRACE("%s %Ix %s %u\n",
          debugstr_w(module->modulename), address, debugstr_a(source_get(module, *psrc)), line);
    symt = symt_find_symbol_at(module, address);
    if (symt_check_tag(&symt->symt, SymTagFunction))
    {
        func = (struct symt_function*)symt;
        for (inlined = func->next_inlinesite; inlined; inlined = inlined->next_inlinesite)
        {
            int i;
            for (i = 0; i < inlined->num_ranges; ++i)
            {
                if (inlined->ranges[i].low <= address && address < inlined->ranges[i].high)
                {
                    symt_add_func_line(module, inlined, *psrc, line, address);
                    return; /* only add to lowest matching inline site */
                }
            }
        }
        symt_add_func_line(module, func, *psrc, line, address);
    }
}

static BOOL dwarf2_parse_line_numbers(dwarf2_parse_context_t* ctx,
                                      const char* compile_dir,
                                      ULONG_PTR offset)
{
    dwarf2_traverse_context_t   traverse;
    ULONG_PTR                   length;
    unsigned                    insn_size, version, default_stmt;
    unsigned                    line_range, opcode_base;
    int                         line_base;
    unsigned char               offset_size;
    const unsigned char*        opcode_len;
    struct vector               dirs;
    struct vector               files;
    const char**                p;

    /* section with line numbers stripped */
    if (ctx->module_ctx->sections[section_line].address == IMAGE_NO_MAP)
        return FALSE;

    if (offset + 4 > ctx->module_ctx->sections[section_line].size)
    {
        WARN("out of bounds offset\n");
        return FALSE;
    }
    traverse.data = ctx->module_ctx->sections[section_line].address + offset;
    traverse.end_data = ctx->module_ctx->sections[section_line].address + ctx->module_ctx->sections[section_line].size;

    length = dwarf2_parse_3264(&traverse, &offset_size);
    if (offset_size != ctx->head.offset_size)
    {
        WARN("Mismatch in 32/64 bit format\n");
        return FALSE;
    }
    traverse.end_data = traverse.data + length;

    if (traverse.end_data > ctx->module_ctx->sections[section_line].address + ctx->module_ctx->sections[section_line].size)
    {
        WARN("out of bounds header\n");
        return FALSE;
    }
    version = dwarf2_parse_u2(&traverse);
    dwarf2_parse_offset(&traverse, offset_size); /* header_len */
    insn_size = dwarf2_parse_byte(&traverse);
    if (version >= 4)
        dwarf2_parse_byte(&traverse); /* max_operations_per_instructions */
    default_stmt = dwarf2_parse_byte(&traverse);
    line_base = (signed char)dwarf2_parse_byte(&traverse);
    line_range = dwarf2_parse_byte(&traverse);
    opcode_base = dwarf2_parse_byte(&traverse);

    opcode_len = traverse.data;
    traverse.data += opcode_base - 1;

    vector_init(&dirs, sizeof(const char*), 0);
    p = vector_add(&dirs, &ctx->pool);
    *p = compile_dir ? compile_dir : ".";
    while (traverse.data < traverse.end_data && *traverse.data)
    {
        const char*  rel = (const char*)traverse.data;
        unsigned     rellen = strlen(rel);
        TRACE("Got include %s\n", debugstr_a(rel));
        traverse.data += rellen + 1;
        p = vector_add(&dirs, &ctx->pool);

        if (*rel == '/' || !compile_dir || !*compile_dir)
            *p = rel;
        else
        {
           /* include directory relative to compile directory */
           unsigned  baselen = strlen(compile_dir);
           char*     tmp = pool_alloc(&ctx->pool, baselen + 1 + rellen + 1);
           strcpy(tmp, compile_dir);
           if (baselen && tmp[baselen - 1] != '/') tmp[baselen++] = '/';
           strcpy(&tmp[baselen], rel);
           *p = tmp;
        }

    }
    traverse.data++;

    vector_init(&files, sizeof(unsigned), 0);
    while (traverse.data < traverse.end_data && *traverse.data)
    {
        unsigned int    dir_index, mod_time;
        const char*     name;
        const char*     dir;
        unsigned*       psrc;

        name = (const char*)traverse.data;
        traverse.data += strlen(name) + 1;
        dir_index = dwarf2_leb128_as_unsigned(&traverse);
        mod_time = dwarf2_leb128_as_unsigned(&traverse);
        length = dwarf2_leb128_as_unsigned(&traverse);
        dir = *(const char**)vector_at(&dirs, dir_index);
        TRACE("Got file %s/%s (%u,%Iu)\n", debugstr_a(dir), debugstr_a(name), mod_time, length);
        psrc = vector_add(&files, &ctx->pool);
        *psrc = source_new(ctx->module_ctx->module, dir, name);
    }
    traverse.data++;

    while (traverse.data < traverse.end_data && *traverse.data)
    {
        ULONG_PTR address = 0;
        unsigned file = 1;
        unsigned line = 1;
        unsigned is_stmt = default_stmt;
        BOOL end_sequence = FALSE;
        unsigned opcode, extopcode, i;

        while (!end_sequence)
        {
            opcode = dwarf2_parse_byte(&traverse);
            TRACE("Got opcode %x\n", opcode);

            if (opcode >= opcode_base)
            {
                unsigned delta = opcode - opcode_base;

                address += (delta / line_range) * insn_size;
                line += line_base + (delta % line_range);
                dwarf2_set_line_number(ctx->module_ctx->module, address, &files, file, line);
            }
            else
            {
                switch (opcode)
                {
                case DW_LNS_copy:
                    dwarf2_set_line_number(ctx->module_ctx->module, address, &files, file, line);
                    break;
                case DW_LNS_advance_pc:
                    address += insn_size * dwarf2_leb128_as_unsigned(&traverse);
                    break;
                case DW_LNS_advance_line:
                    line += dwarf2_leb128_as_signed(&traverse);
                    break;
                case DW_LNS_set_file:
                    file = dwarf2_leb128_as_unsigned(&traverse);
                    break;
                case DW_LNS_set_column:
                    dwarf2_leb128_as_unsigned(&traverse);
                    break;
                case DW_LNS_negate_stmt:
                    is_stmt = !is_stmt;
                    break;
                case DW_LNS_set_basic_block:
                    break;
                case DW_LNS_const_add_pc:
                    address += ((255 - opcode_base) / line_range) * insn_size;
                    break;
                case DW_LNS_fixed_advance_pc:
                    address += dwarf2_parse_u2(&traverse);
                    break;
                case DW_LNS_set_prologue_end:
                case DW_LNS_set_epilogue_begin:
                    break;
                case DW_LNS_extended_op:
                    dwarf2_leb128_as_unsigned(&traverse);
                    extopcode = dwarf2_parse_byte(&traverse);
                    switch (extopcode)
                    {
                    case DW_LNE_end_sequence:
                        dwarf2_set_line_number(ctx->module_ctx->module, address, &files, file, line);
                        end_sequence = TRUE;
                        break;
                    case DW_LNE_set_address:
                        address = ctx->module_ctx->load_offset + dwarf2_parse_addr_head(&traverse, &ctx->head);
                        break;
                    case DW_LNE_define_file:
                        FIXME("not handled define file %s\n", debugstr_a((char *)traverse.data));
                        traverse.data += strlen((const char *)traverse.data) + 1;
                        dwarf2_leb128_as_unsigned(&traverse);
                        dwarf2_leb128_as_unsigned(&traverse);
                        dwarf2_leb128_as_unsigned(&traverse);
                        break;
                    case DW_LNE_set_discriminator:
                        {
                            unsigned descr;

                            descr = dwarf2_leb128_as_unsigned(&traverse);
                            WARN("not handled discriminator %x\n", descr);
                        }
                        break;
                    default:
                        FIXME("Unsupported extended opcode %x\n", extopcode);
                        break;
                    }
                    break;
                default:
                    WARN("Unsupported opcode %x\n", opcode);
                    for (i = 0; i < opcode_len[opcode]; i++)
                        dwarf2_leb128_as_unsigned(&traverse);
                    break;
                }
            }
        }
    }
    return TRUE;
}

unsigned dwarf2_cache_cuhead(struct dwarf2_module_info_s* module, struct symt_compiland* c, const dwarf2_cuhead_t* head)
{
    dwarf2_cuhead_t* ah;
    unsigned i;
    for (i = 0; i < module->num_cuheads; ++i)
    {
        if (memcmp(module->cuheads[i], head, sizeof(*head)) == 0)
        {
            c->user = module->cuheads[i];
            return TRUE;
        }
    }
    if (!(ah = pool_alloc(&c->container->module->pool, sizeof(*head)))) return FALSE;
    memcpy(ah, head, sizeof(*head));
    module->cuheads = realloc(module->cuheads, ++module->num_cuheads * sizeof(head));
    module->cuheads[module->num_cuheads - 1] = ah;
    c->user = ah;
    return TRUE;
}

static dwarf2_parse_context_t* dwarf2_locate_cu(dwarf2_parse_module_context_t* module_ctx, ULONG_PTR ref)
{
    unsigned i;
    dwarf2_parse_context_t* ctx;
    const BYTE* where;
    for (i = 0; i < module_ctx->unit_contexts.num_elts; ++i)
    {
        ctx = *(dwarf2_parse_context_t**)vector_at(&module_ctx->unit_contexts, i);
        where = module_ctx->sections[ctx->section].address + ref;
        if (where >= ctx->traverse_DIE.data && where < ctx->traverse_DIE.end_data)
            return ctx;
    }
    FIXME("Couldn't find ref 0x%Ix inside sect\n", ref);
    return NULL;
}

static BOOL dwarf2_parse_compilation_unit_head(dwarf2_parse_context_t* ctx,
                                               dwarf2_traverse_context_t* mod_ctx)
{
    dwarf2_traverse_context_t abbrev_ctx;
    const unsigned char* comp_unit_start = mod_ctx->data;
    ULONG_PTR cu_length;
    ULONG_PTR cu_abbrev_offset;
    /* FIXME this is a temporary configuration while adding support for dwarf3&4 bits */
    static LONG max_supported_dwarf_version = 0;

    cu_length = dwarf2_parse_3264(mod_ctx, &ctx->head.offset_size);

    ctx->traverse_DIE.data = mod_ctx->data;
    ctx->traverse_DIE.end_data = mod_ctx->data + cu_length;
    mod_ctx->data += cu_length;
    ctx->head.version = dwarf2_parse_u2(&ctx->traverse_DIE);
    cu_abbrev_offset = dwarf2_parse_offset(&ctx->traverse_DIE, ctx->head.offset_size);
    ctx->head.word_size = dwarf2_parse_byte(&ctx->traverse_DIE);
    ctx->status = UNIT_ERROR;

    TRACE("Compilation Unit Header found at 0x%x:\n",
          (int)(comp_unit_start - ctx->module_ctx->sections[section_debug].address));
    TRACE("- length:        %Iu\n", cu_length);
    TRACE("- version:       %u\n",  ctx->head.version);
    TRACE("- abbrev_offset: %Iu\n", cu_abbrev_offset);
    TRACE("- word_size:     %u\n",  ctx->head.word_size);
    TRACE("- offset_size:   %u\n",  ctx->head.offset_size);

    if (ctx->head.version >= 2 && ctx->head.version <= 5)
        ctx->module_ctx->cu_versions |= DHEXT_FORMAT_DWARF2 << (ctx->head.version - 2);
    if (max_supported_dwarf_version == 0)
    {
        char* env = getenv("DBGHELP_DWARF_VERSION");
        LONG v = env ? atol(env) : 4;
        max_supported_dwarf_version = (v >= 2 && v <= 4) ? v : 4;
    }

    if (ctx->head.version < 2 || ctx->head.version > max_supported_dwarf_version)
    {
        WARN("DWARF version %d isn't supported. Wine dbghelp only supports DWARF 2 up to %lu.\n",
             ctx->head.version, max_supported_dwarf_version);
        return FALSE;
    }

    pool_init(&ctx->pool, 65536);
    ctx->section = section_debug;
    ctx->ref_offset = comp_unit_start - ctx->module_ctx->sections[section_debug].address;
    ctx->cpp_name = NULL;
    ctx->status = UNIT_NOTLOADED;

    abbrev_ctx.data = ctx->module_ctx->sections[section_abbrev].address + cu_abbrev_offset;
    abbrev_ctx.end_data = ctx->module_ctx->sections[section_abbrev].address + ctx->module_ctx->sections[section_abbrev].size;
    dwarf2_parse_abbrev_set(&abbrev_ctx, &ctx->abbrev_table, &ctx->pool);

    sparse_array_init(&ctx->debug_info_table, sizeof(dwarf2_debug_info_t), 128);
    return TRUE;
}

static BOOL dwarf2_parse_compilation_unit(dwarf2_parse_context_t* ctx)
{
    dwarf2_debug_info_t* di;
    dwarf2_traverse_context_t cu_ctx = ctx->traverse_DIE;
    BOOL ret = FALSE;

    switch (ctx->status)
    {
    case UNIT_ERROR: return FALSE;
    case UNIT_BEINGLOADED:
        FIXME("Circular deps on CU\n");
        /* fall through */
    case UNIT_LOADED:
    case UNIT_LOADED_FAIL:
        return TRUE;
    case UNIT_NOTLOADED: break;
    }

    ctx->status = UNIT_BEINGLOADED;
    if (dwarf2_read_one_debug_info(ctx, &cu_ctx, NULL, &di))
    {
        if (di->abbrev->tag == DW_TAG_compile_unit || di->abbrev->tag == DW_TAG_partial_unit)
        {
            struct attribute            name;
            struct vector*              children;
            dwarf2_debug_info_t*        child = NULL;
            unsigned int                i;
            struct attribute            stmt_list, low_pc;
            struct attribute            comp_dir;
            struct attribute            language;

            if (!dwarf2_find_attribute(di, DW_AT_name, &name))
                name.u.string = NULL;

            /* get working directory of current compilation unit */
            if (!dwarf2_find_attribute(di, DW_AT_comp_dir, &comp_dir))
                comp_dir.u.string = NULL;

            if (!dwarf2_find_attribute(di, DW_AT_low_pc, &low_pc))
                low_pc.u.uvalue = 0;

            if (!dwarf2_find_attribute(di, DW_AT_language, &language))
                language.u.uvalue = DW_LANG_C;

            ctx->language = language.u.uvalue;

            ctx->compiland = symt_new_compiland(ctx->module_ctx->module,
                                                source_new(ctx->module_ctx->module, comp_dir.u.string, name.u.string));
            ctx->compiland->address = ctx->module_ctx->load_offset + low_pc.u.uvalue;
            dwarf2_cache_cuhead(ctx->module_ctx->module->format_info[DFI_DWARF]->u.dwarf2_info, ctx->compiland, &ctx->head);
            di->symt = &ctx->compiland->symt;
            children = dwarf2_get_di_children(di);
            if (children) for (i = 0; i < vector_length(children); i++)
            {
                child = *(dwarf2_debug_info_t**)vector_at(children, i);
                dwarf2_load_one_entry(child);
            }
            if ((SymGetOptions() & SYMOPT_LOAD_LINES) && dwarf2_find_attribute(di, DW_AT_stmt_list, &stmt_list))
            {
                if (dwarf2_parse_line_numbers(ctx, comp_dir.u.string, stmt_list.u.uvalue))
                    ctx->module_ctx->module->module.LineNumbers = TRUE;
            }
            ctx->status = UNIT_LOADED;
            ret = TRUE;
        }
        else FIXME("Should have a compilation unit here %Iu\n", di->abbrev->tag);
    }
    if (ctx->status == UNIT_BEINGLOADED) ctx->status = UNIT_LOADED_FAIL;
    return ret;
}

static BOOL dwarf2_lookup_loclist(const struct module_format* modfmt, const dwarf2_cuhead_t* head,
                                  const BYTE* start, ULONG_PTR ip, dwarf2_traverse_context_t* lctx)
{
    DWORD_PTR                   beg, end;
    const BYTE*                 ptr = start;
    DWORD                       len;

    while (ptr < modfmt->u.dwarf2_info->debug_loc.address + modfmt->u.dwarf2_info->debug_loc.size)
    {
        beg = dwarf2_get_addr(ptr, head->word_size); ptr += head->word_size;
        end = dwarf2_get_addr(ptr, head->word_size); ptr += head->word_size;
        if (!beg && !end) break;
        len = dwarf2_get_u2(ptr); ptr += 2;

        if (beg <= ip && ip < end)
        {
            lctx->data = ptr;
            lctx->end_data = ptr + len;
            return TRUE;
        }
        ptr += len;
    }
    WARN("Couldn't find ip in location list\n");
    return FALSE;
}

static const dwarf2_cuhead_t* get_cuhead_from_func(const struct symt_function* func)
{
    if (symt_check_tag(&func->symt, SymTagInlineSite))
        func = symt_get_function_from_inlined((struct symt_function*)func);
    if (symt_check_tag(&func->symt, SymTagFunction) && symt_check_tag(func->container, SymTagCompiland))
    {
        struct symt_compiland* c = (struct symt_compiland*)func->container;
        return (const dwarf2_cuhead_t*)c->user;
    }
    FIXME("Should have a compilation unit head\n");
    return NULL;
}

static enum location_error compute_call_frame_cfa(struct module* module, ULONG_PTR ip, struct location* frame);

static enum location_error loc_compute_frame(const struct module_format* modfmt,
                                             const struct symt_function* func,
                                             DWORD_PTR ip, const dwarf2_cuhead_t* head,
                                             struct location* frame)
{
    struct process             *pcs = modfmt->module->process;
    struct symt**               psym = NULL;
    struct location*            pframe;
    dwarf2_traverse_context_t   lctx;
    enum location_error         err;
    unsigned int                i;

    for (i=0; i<vector_length(&func->vchildren); i++)
    {
        psym = vector_at(&func->vchildren, i);
        if (psym && symt_check_tag(*psym, SymTagCustom))
        {
            pframe = &((struct symt_hierarchy_point*)*psym)->loc;

            /* First, recompute the frame information, if needed */
            switch (pframe->kind)
            {
            case loc_regrel:
            case loc_register:
                *frame = *pframe;
                break;
            case loc_dwarf2_location_list:
                WARN("Searching loclist for %s\n", debugstr_a(func->hash_elt.name));
                if (!dwarf2_lookup_loclist(modfmt, head,
                                           modfmt->u.dwarf2_info->debug_loc.address + pframe->offset,
                                           ip, &lctx))
                    return loc_err_out_of_scope;
                if ((err = compute_location(modfmt->module, head,
                                            &lctx, frame, pcs->handle, NULL)) < 0) return err;
                if (frame->kind >= loc_user)
                {
                    WARN("Couldn't compute runtime frame location\n");
                    return loc_err_too_complex;
                }
                break;
            case loc_dwarf2_frame_cfa:
                err = compute_call_frame_cfa(modfmt->module, ip + ((struct symt_compiland*)func->container)->address, frame);
                if (err < 0) return err;
                break;
            default:
                WARN("Unsupported frame kind %d\n", pframe->kind);
                return loc_err_internal;
            }
            return 0;
        }
    }
    WARN("Couldn't find Custom function point, whilst location list offset is searched\n");
    return loc_err_internal;
}

enum reg_rule
{
    RULE_UNSET,          /* not set at all */
    RULE_UNDEFINED,      /* undefined value */
    RULE_SAME,           /* same value as previous frame */
    RULE_CFA_OFFSET,     /* stored at cfa offset */
    RULE_OTHER_REG,      /* stored in other register */
    RULE_EXPRESSION,     /* address specified by expression */
    RULE_VAL_EXPRESSION  /* value specified by expression */
};

/* make it large enough for all CPUs */
#define NB_FRAME_REGS 64
#define MAX_SAVED_STATES 16

struct frame_state
{
    ULONG_PTR     cfa_offset;
    unsigned char cfa_reg;
    enum reg_rule cfa_rule;
    enum reg_rule rules[NB_FRAME_REGS];
    ULONG_PTR     regs[NB_FRAME_REGS];
};

struct frame_info
{
    ULONG_PTR     ip;
    ULONG_PTR     code_align;
    LONG_PTR      data_align;
    unsigned char retaddr_reg;
    unsigned char fde_encoding;
    unsigned char lsda_encoding;
    unsigned char signal_frame;
    unsigned char aug_z_format;
    unsigned char state_sp;
    struct frame_state state;
    struct frame_state state_stack[MAX_SAVED_STATES];
};

static ULONG_PTR dwarf2_parse_augmentation_ptr(dwarf2_traverse_context_t* ctx, unsigned char encoding, unsigned char word_size)
{
    ULONG_PTR   base;

    if (encoding == DW_EH_PE_omit) return 0;

    switch (encoding & 0xf0)
    {
    case DW_EH_PE_abs:
        base = 0;
        break;
    case DW_EH_PE_pcrel:
        base = (ULONG_PTR)ctx->data;
        break;
    default:
        FIXME("unsupported encoding %02x\n", encoding);
        return 0;
    }

    switch (encoding & 0x0f)
    {
    case DW_EH_PE_native:
        return base + dwarf2_parse_addr(ctx, word_size);
    case DW_EH_PE_leb128:
        return base + dwarf2_leb128_as_unsigned(ctx);
    case DW_EH_PE_data2:
        return base + dwarf2_parse_u2(ctx);
    case DW_EH_PE_data4:
        return base + dwarf2_parse_u4(ctx);
    case DW_EH_PE_data8:
        return base + dwarf2_parse_u8(ctx);
    case DW_EH_PE_signed|DW_EH_PE_leb128:
        return base + dwarf2_leb128_as_signed(ctx);
    case DW_EH_PE_signed|DW_EH_PE_data2:
        return base + (signed short)dwarf2_parse_u2(ctx);
    case DW_EH_PE_signed|DW_EH_PE_data4:
        return base + (signed int)dwarf2_parse_u4(ctx);
    case DW_EH_PE_signed|DW_EH_PE_data8:
        return base + (LONG64)dwarf2_parse_u8(ctx);
    default:
        FIXME("unsupported encoding %02x\n", encoding);
        return 0;
    }
}

static BOOL parse_cie_details(dwarf2_traverse_context_t* ctx, struct frame_info* info, unsigned char word_size)
{
    unsigned char version;
    const char* augmentation;
    const unsigned char* end;
    ULONG_PTR len;

    memset(info, 0, sizeof(*info));
    info->lsda_encoding = DW_EH_PE_omit;
    info->aug_z_format = 0;

    /* parse the CIE first */
    version = dwarf2_parse_byte(ctx);
    if (version != 1 && version != 3 && version != 4)
    {
        FIXME("unknown CIE version %u at %p\n", version, ctx->data - 1);
        return FALSE;
    }
    augmentation = (const char*)ctx->data;
    ctx->data += strlen(augmentation) + 1;

    switch (version)
    {
    case 4:
        /* skip 'address_size' and 'segment_size' */
        ctx->data += 2;
        /* fallthrough */
    case 1:
    case 3:
        info->code_align = dwarf2_leb128_as_unsigned(ctx);
        info->data_align = dwarf2_leb128_as_signed(ctx);
        info->retaddr_reg = version == 1 ? dwarf2_parse_byte(ctx) :dwarf2_leb128_as_unsigned(ctx);
        break;
    default:
        ;
    }
    info->state.cfa_rule = RULE_CFA_OFFSET;

    end = NULL;
    TRACE("\tparsing augmentation %s\n", debugstr_a(augmentation));
    if (*augmentation) do
    {
        switch (*augmentation)
        {
        case 'z':
            len = dwarf2_leb128_as_unsigned(ctx);
            end = ctx->data + len;
            info->aug_z_format = 1;
            continue;
        case 'L':
            info->lsda_encoding = dwarf2_parse_byte(ctx);
            continue;
        case 'P':
        {
            unsigned char encoding = dwarf2_parse_byte(ctx);
            /* throw away the indirect bit, as we don't care for the result */
            encoding &= ~DW_EH_PE_indirect;
            dwarf2_parse_augmentation_ptr(ctx, encoding, word_size); /* handler */
            continue;
        }
        case 'R':
            info->fde_encoding = dwarf2_parse_byte(ctx);
            continue;
        case 'S':
            info->signal_frame = 1;
            continue;
        }
        FIXME("unknown augmentation '%c'\n", *augmentation);
        if (!end) return FALSE;
        break;
    } while (*++augmentation);
    if (end) ctx->data = end;
    return TRUE;
}

static BOOL dwarf2_get_cie(ULONG_PTR addr, struct module* module, DWORD_PTR delta,
                           dwarf2_traverse_context_t* fde_ctx, dwarf2_traverse_context_t* cie_ctx,
                           struct frame_info* info, BOOL in_eh_frame)
{
    const unsigned char*        ptr_blk;
    const unsigned char*        cie_ptr;
    const unsigned char*        last_cie_ptr = (const unsigned char*)~0;
    ULONG_PTR                   len, id;
    ULONG_PTR                   start, range;
    ULONG_PTR                   cie_id;
    const BYTE*                 start_data = fde_ctx->data;
    unsigned char               word_size = module->format_info[DFI_DWARF]->u.dwarf2_info->word_size;
    unsigned char               offset_size;

    /* skip 0-padding at beginning of section (alignment) */
    while (fde_ctx->data + 2 * 4 < fde_ctx->end_data)
    {
        if (dwarf2_parse_u4(fde_ctx))
        {
            fde_ctx->data -= 4;
            break;
        }
    }
    for (; fde_ctx->data + 2 * 4 < fde_ctx->end_data; fde_ctx->data = ptr_blk)
    {
        const unsigned char* st = fde_ctx->data;
        /* find the FDE for address addr (skip CIE) */
        len = dwarf2_parse_3264(fde_ctx, &offset_size);
        cie_id = in_eh_frame ? 0 : (offset_size == 4 ? DW_CIE_ID : (ULONG_PTR)DW64_CIE_ID);
        ptr_blk = fde_ctx->data + len;
        id = dwarf2_parse_offset(fde_ctx, offset_size);
        if (id == cie_id)
        {
            last_cie_ptr = st;
            /* we need some bits out of the CIE in order to parse all contents */
            if (!parse_cie_details(fde_ctx, info, word_size)) return FALSE;
            cie_ctx->data = fde_ctx->data;
            cie_ctx->end_data = ptr_blk;
            continue;
        }
        cie_ptr = (in_eh_frame) ? fde_ctx->data - id - 4 : start_data + id;
        if (cie_ptr != last_cie_ptr)
        {
            last_cie_ptr = cie_ptr;
            cie_ctx->data = cie_ptr;
            cie_ctx->end_data = cie_ptr + (offset_size == 4 ? 4 : 4 + 8);
            cie_ctx->end_data += dwarf2_parse_3264(cie_ctx, &offset_size);

            if (dwarf2_parse_offset(cie_ctx, in_eh_frame ? word_size : offset_size) != cie_id)
            {
                FIXME("wrong CIE pointer at %x from FDE %x\n",
                      (unsigned)(cie_ptr - start_data),
                      (unsigned)(fde_ctx->data - start_data));
                return FALSE;
            }
            if (!parse_cie_details(cie_ctx, info, word_size)) return FALSE;
        }
        start = delta + dwarf2_parse_augmentation_ptr(fde_ctx, info->fde_encoding, word_size);
        range = dwarf2_parse_augmentation_ptr(fde_ctx, info->fde_encoding & 0x0F, word_size);

        if (addr >= start && addr < start + range)
        {
            /* reset the FDE context */
            fde_ctx->end_data = ptr_blk;

            info->ip = start;
            return TRUE;
        }
    }
    return FALSE;
}

static int valid_reg(ULONG_PTR reg)
{
    if (reg >= NB_FRAME_REGS) FIXME("unsupported reg %Ix\n", reg);
    return (reg < NB_FRAME_REGS);
}

static void execute_cfa_instructions(struct module* module, dwarf2_traverse_context_t* ctx,
                                     ULONG_PTR last_ip, struct frame_info *info)
{
    while (ctx->data < ctx->end_data && info->ip <= last_ip + info->signal_frame)
    {
        enum dwarf_call_frame_info op = dwarf2_parse_byte(ctx);

        if (op & 0xc0)
        {
            switch (op & 0xc0)
            {
            case DW_CFA_advance_loc:
            {
                ULONG_PTR offset = (op & 0x3f) * info->code_align;
                TRACE("%Ix: DW_CFA_advance_loc %Iu\n", info->ip, offset);
                info->ip += offset;
                break;
            }
            case DW_CFA_offset:
            {
                ULONG_PTR reg = op & 0x3f;
                LONG_PTR offset = dwarf2_leb128_as_unsigned(ctx) * info->data_align;
                if (!valid_reg(reg)) break;
                TRACE("%Ix: DW_CFA_offset %s, %Id\n",
                      info->ip,
                      module->cpu->fetch_regname(module->cpu->map_dwarf_register(reg, module, TRUE)),
                      offset);
                info->state.regs[reg]  = offset;
                info->state.rules[reg] = RULE_CFA_OFFSET;
                break;
            }
            case DW_CFA_restore:
            {
                ULONG_PTR reg = op & 0x3f;
                if (!valid_reg(reg)) break;
                TRACE("%Ix: DW_CFA_restore %s\n",
                      info->ip,
                      module->cpu->fetch_regname(module->cpu->map_dwarf_register(reg, module, TRUE)));
                info->state.rules[reg] = RULE_UNSET;
                break;
            }
            }
        }
        else switch (op)
        {
        case DW_CFA_nop:
            break;
        case DW_CFA_set_loc:
        {
            ULONG_PTR loc = dwarf2_parse_augmentation_ptr(ctx, info->fde_encoding,
                                                          module->format_info[DFI_DWARF]->u.dwarf2_info->word_size);
            TRACE("%Ix: DW_CFA_set_loc %Ix\n", info->ip, loc);
            info->ip = loc;
            break;
        }
        case DW_CFA_advance_loc1:
        {
            ULONG_PTR offset = dwarf2_parse_byte(ctx) * info->code_align;
            TRACE("%Ix: DW_CFA_advance_loc1 %Iu\n", info->ip, offset);
            info->ip += offset;
            break;
        }
        case DW_CFA_advance_loc2:
        {
            ULONG_PTR offset = dwarf2_parse_u2(ctx) * info->code_align;
            TRACE("%Ix: DW_CFA_advance_loc2 %Iu\n", info->ip, offset);
            info->ip += offset;
            break;
        }
        case DW_CFA_advance_loc4:
        {
            ULONG_PTR offset = dwarf2_parse_u4(ctx) * info->code_align;
            TRACE("%Ix: DW_CFA_advance_loc4 %Iu\n", info->ip, offset);
            info->ip += offset;
            break;
        }
        case DW_CFA_offset_extended:
        case DW_CFA_offset_extended_sf:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            LONG_PTR offset = (op == DW_CFA_offset_extended) ? dwarf2_leb128_as_unsigned(ctx) * info->data_align
                                                             : dwarf2_leb128_as_signed(ctx) * info->data_align;
            if (!valid_reg(reg)) break;
            TRACE("%Ix: DW_CFA_offset_extended %s, %Id\n",
                  info->ip,
                  module->cpu->fetch_regname(module->cpu->map_dwarf_register(reg, module, TRUE)),
                  offset);
            info->state.regs[reg]  = offset;
            info->state.rules[reg] = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_restore_extended:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            if (!valid_reg(reg)) break;
            TRACE("%Ix: DW_CFA_restore_extended %s\n",
                  info->ip,
                  module->cpu->fetch_regname(module->cpu->map_dwarf_register(reg, module, TRUE)));
            info->state.rules[reg] = RULE_UNSET;
            break;
        }
        case DW_CFA_undefined:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            if (!valid_reg(reg)) break;
            TRACE("%Ix: DW_CFA_undefined %s\n",
                  info->ip,
                  module->cpu->fetch_regname(module->cpu->map_dwarf_register(reg, module, TRUE)));
            info->state.rules[reg] = RULE_UNDEFINED;
            break;
        }
        case DW_CFA_same_value:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            if (!valid_reg(reg)) break;
            TRACE("%Ix: DW_CFA_same_value %s\n",
                  info->ip,
                  module->cpu->fetch_regname(module->cpu->map_dwarf_register(reg, module, TRUE)));
            info->state.regs[reg]  = reg;
            info->state.rules[reg] = RULE_SAME;
            break;
        }
        case DW_CFA_register:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            ULONG_PTR reg2 = dwarf2_leb128_as_unsigned(ctx);
            if (!valid_reg(reg) || !valid_reg(reg2)) break;
            TRACE("%Ix: DW_CFA_register %s == %s\n",
                  info->ip,
                  module->cpu->fetch_regname(module->cpu->map_dwarf_register(reg, module, TRUE)),
                  module->cpu->fetch_regname(module->cpu->map_dwarf_register(reg2, module, TRUE)));
            info->state.regs[reg]  = reg2;
            info->state.rules[reg] = RULE_OTHER_REG;
            break;
        }
        case DW_CFA_remember_state:
            TRACE("%Ix: DW_CFA_remember_state\n", info->ip);
            if (info->state_sp >= MAX_SAVED_STATES)
                FIXME("%Ix: DW_CFA_remember_state too many nested saves\n", info->ip);
            else
                info->state_stack[info->state_sp++] = info->state;
            break;
        case DW_CFA_restore_state:
            TRACE("%Ix: DW_CFA_restore_state\n", info->ip);
            if (!info->state_sp)
                FIXME("%Ix: DW_CFA_restore_state without corresponding save\n", info->ip);
            else
                info->state = info->state_stack[--info->state_sp];
            break;
        case DW_CFA_def_cfa:
        case DW_CFA_def_cfa_sf:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            ULONG_PTR offset = (op == DW_CFA_def_cfa) ? dwarf2_leb128_as_unsigned(ctx)
                                                      : dwarf2_leb128_as_signed(ctx) * info->data_align;
            if (!valid_reg(reg)) break;
            TRACE("%Ix: DW_CFA_def_cfa %s, %Id\n",
                  info->ip,
                  module->cpu->fetch_regname(module->cpu->map_dwarf_register(reg, module, TRUE)),
                  offset);
            info->state.cfa_reg    = reg;
            info->state.cfa_offset = offset;
            info->state.cfa_rule   = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_def_cfa_register:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            if (!valid_reg(reg)) break;
            TRACE("%Ix: DW_CFA_def_cfa_register %s\n",
                  info->ip,
                  module->cpu->fetch_regname(module->cpu->map_dwarf_register(reg, module, TRUE)));
            info->state.cfa_reg  = reg;
            info->state.cfa_rule = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_def_cfa_offset:
        case DW_CFA_def_cfa_offset_sf:
        {
            ULONG_PTR offset = (op == DW_CFA_def_cfa_offset) ? dwarf2_leb128_as_unsigned(ctx)
                                                             : dwarf2_leb128_as_signed(ctx) * info->data_align;
            TRACE("%Ix: DW_CFA_def_cfa_offset %Id\n", info->ip, offset);
            info->state.cfa_offset = offset;
            info->state.cfa_rule   = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_def_cfa_expression:
        {
            ULONG_PTR expr = (ULONG_PTR)ctx->data;
            ULONG_PTR len = dwarf2_leb128_as_unsigned(ctx);
            TRACE("%Ix: DW_CFA_def_cfa_expression %Ix-%Ix\n", info->ip, expr, expr+len);
            info->state.cfa_offset = expr;
            info->state.cfa_rule   = RULE_VAL_EXPRESSION;
            ctx->data += len;
            break;
        }
        case DW_CFA_expression:
        case DW_CFA_val_expression:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            ULONG_PTR expr = (ULONG_PTR)ctx->data;
            ULONG_PTR len = dwarf2_leb128_as_unsigned(ctx);
            if (!valid_reg(reg)) break;
            TRACE("%Ix: DW_CFA_%sexpression %s %Ix-%Ix\n",
                  info->ip, (op == DW_CFA_expression) ? "" : "val_",
                  module->cpu->fetch_regname(module->cpu->map_dwarf_register(reg, module, TRUE)),
                  expr, expr + len);
            info->state.regs[reg]  = expr;
            info->state.rules[reg] = (op == DW_CFA_expression) ? RULE_EXPRESSION : RULE_VAL_EXPRESSION;
            ctx->data += len;
            break;
        }
        case DW_CFA_GNU_args_size:
        /* FIXME: should check that GCC is the compiler for this CU */
        {
            ULONG_PTR   args = dwarf2_leb128_as_unsigned(ctx);
            TRACE("%Ix: DW_CFA_GNU_args_size %Iu\n", info->ip, args);
            /* ignored */
            break;
        }
        default:
            FIXME("%Ix: unknown CFA opcode %02x\n", info->ip, op);
            break;
        }
    }
}

/* retrieve a context register from its dwarf number */
static DWORD64 get_context_reg(const struct module* module, struct cpu_stack_walk *csw, union ctx *context,
    ULONG_PTR dw_reg)
{
    unsigned regno = csw->cpu->map_dwarf_register(dw_reg, module, TRUE), sz;
    void* ptr = csw->cpu->fetch_context_reg(context, regno, &sz);

    if (csw->cpu != module->cpu) FIXME("mismatch in cpu\n");
    if (sz == 8)
        return *(DWORD64 *)ptr;
    else if (sz == 4)
        return *(DWORD *)ptr;

    FIXME("unhandled size %d\n", sz);
    return 0;
}

/* set a context register from its dwarf number */
static void set_context_reg(const struct module* module, struct cpu_stack_walk* csw, union ctx *context,
    ULONG_PTR dw_reg, ULONG_PTR val, BOOL isdebuggee)
{
    unsigned regno = csw->cpu->map_dwarf_register(dw_reg, module, TRUE), sz;
    ULONG_PTR* ptr = csw->cpu->fetch_context_reg(context, regno, &sz);

    if (csw->cpu != module->cpu) FIXME("mismatch in cpu\n");
    if (isdebuggee)
    {
        char    tmp[16];

        if (sz > sizeof(tmp))
        {
            FIXME("register %Iu/%u size is too wide: %u\n", dw_reg, regno, sz);
            return;
        }
        if (!sw_read_mem(csw, val, tmp, sz))
        {
            WARN("Couldn't read memory at %p\n", (void*)val);
            return;
        }
        memcpy(ptr, tmp, sz);
    }
    else
    {
        if (sz != sizeof(ULONG_PTR))
        {
            FIXME("assigning to register %Iu/%u of wrong size %u\n", dw_reg, regno, sz);
            return;
        }
        *ptr = val;
    }
}

/* copy a register from one context to another using dwarf number */
static void copy_context_reg(const struct module* module, struct cpu_stack_walk *csw,
    union ctx *dstcontext, ULONG_PTR dwregdst,
    union ctx *srccontext, ULONG_PTR dwregsrc)
{
    unsigned regdstno = csw->cpu->map_dwarf_register(dwregdst, module, TRUE), szdst;
    unsigned regsrcno = csw->cpu->map_dwarf_register(dwregsrc, module, TRUE), szsrc;
    ULONG_PTR* ptrdst = csw->cpu->fetch_context_reg(dstcontext, regdstno, &szdst);
    ULONG_PTR* ptrsrc = csw->cpu->fetch_context_reg(srccontext, regsrcno, &szsrc);

    if (csw->cpu != module->cpu) FIXME("mismatch in cpu\n");
    if (szdst != szsrc)
    {
        FIXME("Cannot copy register %Iu/%u => %Iu/%u because of size mismatch (%u => %u)\n",
              dwregsrc, regsrcno, dwregdst, regdstno, szsrc, szdst);
        return;
    }
    memcpy(ptrdst, ptrsrc, szdst);
}

static ULONG_PTR eval_expression(const struct module* module, struct cpu_stack_walk* csw,
                                 const unsigned char* zp, union ctx *context)
{
    dwarf2_traverse_context_t    ctx;
    ULONG_PTR reg, sz, tmp;
    DWORD64 stack[64];
    int sp = -1;
    ULONG_PTR len;

    if (csw->cpu != module->cpu) FIXME("mismatch in cpu\n");
    ctx.data = zp;
    ctx.end_data = zp + 4;
    len = dwarf2_leb128_as_unsigned(&ctx);
    ctx.end_data = ctx.data + len;

    while (ctx.data < ctx.end_data)
    {
        unsigned char opcode = dwarf2_parse_byte(&ctx);

        if (opcode >= DW_OP_lit0 && opcode <= DW_OP_lit31)
            stack[++sp] = opcode - DW_OP_lit0;
        else if (opcode >= DW_OP_reg0 && opcode <= DW_OP_reg31)
            stack[++sp] = get_context_reg(module, csw, context, opcode - DW_OP_reg0);
        else if (opcode >= DW_OP_breg0 && opcode <= DW_OP_breg31)
            stack[++sp] = get_context_reg(module, csw, context, opcode - DW_OP_breg0)
                          + dwarf2_leb128_as_signed(&ctx);
        else switch (opcode)
        {
        case DW_OP_nop:         break;
        case DW_OP_addr:        stack[++sp] = dwarf2_parse_addr(&ctx, module->format_info[DFI_DWARF]->u.dwarf2_info->word_size); break;
        case DW_OP_const1u:     stack[++sp] = dwarf2_parse_byte(&ctx); break;
        case DW_OP_const1s:     stack[++sp] = (signed char)dwarf2_parse_byte(&ctx); break;
        case DW_OP_const2u:     stack[++sp] = dwarf2_parse_u2(&ctx); break;
        case DW_OP_const2s:     stack[++sp] = (short)dwarf2_parse_u2(&ctx); break;
        case DW_OP_const4u:     stack[++sp] = dwarf2_parse_u4(&ctx); break;
        case DW_OP_const4s:     stack[++sp] = (signed int)dwarf2_parse_u4(&ctx); break;
        case DW_OP_const8u:     stack[++sp] = dwarf2_parse_u8(&ctx); break;
        case DW_OP_const8s:     stack[++sp] = (LONG_PTR)dwarf2_parse_u8(&ctx); break;
        case DW_OP_constu:      stack[++sp] = dwarf2_leb128_as_unsigned(&ctx); break;
        case DW_OP_consts:      stack[++sp] = dwarf2_leb128_as_signed(&ctx); break;
        case DW_OP_deref:
            tmp = 0;
            if (!sw_read_mem(csw, stack[sp], &tmp, module->format_info[DFI_DWARF]->u.dwarf2_info->word_size))
            {
                ERR("Couldn't read memory at %I64x\n", stack[sp]);
                tmp = 0;
            }
            stack[sp] = tmp;
            break;
        case DW_OP_dup:         stack[sp + 1] = stack[sp]; sp++; break;
        case DW_OP_drop:        sp--; break;
        case DW_OP_over:        stack[sp + 1] = stack[sp - 1]; sp++; break;
        case DW_OP_pick:        stack[sp + 1] = stack[sp - dwarf2_parse_byte(&ctx)]; sp++; break;
        case DW_OP_swap:        tmp = stack[sp]; stack[sp] = stack[sp-1]; stack[sp-1] = tmp; break;
        case DW_OP_rot:         tmp = stack[sp]; stack[sp] = stack[sp-1]; stack[sp-1] = stack[sp-2]; stack[sp-2] = tmp; break;
        case DW_OP_abs:         stack[sp] = sizeof(stack[sp]) == 8 ? llabs((INT64)stack[sp]) : abs((INT32)stack[sp]); break;
        case DW_OP_neg:         stack[sp] = -stack[sp]; break;
        case DW_OP_not:         stack[sp] = ~stack[sp]; break;
        case DW_OP_and:         stack[sp-1] &= stack[sp]; sp--; break;
        case DW_OP_or:          stack[sp-1] |= stack[sp]; sp--; break;
        case DW_OP_minus:       stack[sp-1] -= stack[sp]; sp--; break;
        case DW_OP_mul:         stack[sp-1] *= stack[sp]; sp--; break;
        case DW_OP_plus:        stack[sp-1] += stack[sp]; sp--; break;
        case DW_OP_xor:         stack[sp-1] ^= stack[sp]; sp--; break;
        case DW_OP_shl:         stack[sp-1] <<= stack[sp]; sp--; break;
        case DW_OP_shr:         stack[sp-1] >>= stack[sp]; sp--; break;
        case DW_OP_plus_uconst: stack[sp] += dwarf2_leb128_as_unsigned(&ctx); break;
        case DW_OP_shra:        stack[sp-1] = (LONG_PTR)stack[sp-1] / (1 << stack[sp]); sp--; break;
        case DW_OP_div:         stack[sp-1] = (LONG_PTR)stack[sp-1] / (LONG_PTR)stack[sp]; sp--; break;
        case DW_OP_mod:         stack[sp-1] = (LONG_PTR)stack[sp-1] % (LONG_PTR)stack[sp]; sp--; break;
        case DW_OP_ge:          stack[sp-1] = ((LONG_PTR)stack[sp-1] >= (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_gt:          stack[sp-1] = ((LONG_PTR)stack[sp-1] >  (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_le:          stack[sp-1] = ((LONG_PTR)stack[sp-1] <= (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_lt:          stack[sp-1] = ((LONG_PTR)stack[sp-1] <  (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_eq:          stack[sp-1] = (stack[sp-1] == stack[sp]); sp--; break;
        case DW_OP_ne:          stack[sp-1] = (stack[sp-1] != stack[sp]); sp--; break;
        case DW_OP_skip:        tmp = (short)dwarf2_parse_u2(&ctx); ctx.data += tmp; break;
        case DW_OP_bra:         tmp = (short)dwarf2_parse_u2(&ctx); if (!stack[sp--]) ctx.data += tmp; break;
        case DW_OP_GNU_encoded_addr:
            tmp = dwarf2_parse_byte(&ctx);
            stack[++sp] = dwarf2_parse_augmentation_ptr(&ctx, tmp, module->format_info[DFI_DWARF]->u.dwarf2_info->word_size);
            break;
        case DW_OP_regx:
            stack[++sp] = get_context_reg(module, csw, context, dwarf2_leb128_as_unsigned(&ctx));
            break;
        case DW_OP_bregx:
            reg = dwarf2_leb128_as_unsigned(&ctx);
            tmp = dwarf2_leb128_as_signed(&ctx);
            stack[++sp] = get_context_reg(module, csw, context, reg) + tmp;
            break;
        case DW_OP_deref_size:
            sz = dwarf2_parse_byte(&ctx);
            if (!sw_read_mem(csw, stack[sp], &tmp, sz))
            {
                ERR("Couldn't read memory at %I64x\n", stack[sp]);
                tmp = 0;
            }
            /* do integral promotion */
            switch (sz)
            {
            case 1: stack[sp] = *(unsigned char*)&tmp; break;
            case 2: stack[sp] = *(unsigned short*)&tmp; break;
            case 4: stack[sp] = *(unsigned int*)&tmp; break;
            case 8: stack[sp] = tmp; break; /* FIXME: won't work on 32bit platform */
            default: FIXME("Unknown size for deref 0x%Ix\n", sz);
            }
            break;
        default:
            FIXME("unhandled opcode %02x\n", opcode);
        }
    }
    return stack[sp];
}

static void apply_frame_state(const struct module* module, struct cpu_stack_walk* csw,
                              union ctx *context, struct frame_state *state, DWORD64 *cfa)
{
    unsigned int i;
    ULONG_PTR value;
    union ctx new_context = *context;

    if (csw->cpu != module->cpu) FIXME("mismatch in cpu\n");
    switch (state->cfa_rule)
    {
    case RULE_EXPRESSION:
        *cfa = eval_expression(module, csw, (const unsigned char*)state->cfa_offset, context);
        if (!sw_read_mem(csw, *cfa, cfa, csw->cpu->word_size))
        {
            WARN("Couldn't read memory at %I64x\n", *cfa);
            return;
        }
        break;
    case RULE_VAL_EXPRESSION:
        *cfa = eval_expression(module, csw, (const unsigned char*)state->cfa_offset, context);
        break;
    default:
        *cfa = get_context_reg(module, csw, context, state->cfa_reg) + (LONG_PTR)state->cfa_offset;
        break;
    }
    if (!*cfa) return;

    for (i = 0; i < NB_FRAME_REGS; i++)
    {
        switch (state->rules[i])
        {
        case RULE_UNSET:
        case RULE_UNDEFINED:
        case RULE_SAME:
            break;
        case RULE_CFA_OFFSET:
            set_context_reg(module, csw, &new_context, i, *cfa + (LONG_PTR)state->regs[i], TRUE);
            break;
        case RULE_OTHER_REG:
            copy_context_reg(module, csw, &new_context, i, context, state->regs[i]);
            break;
        case RULE_EXPRESSION:
            value = eval_expression(module, csw, (const unsigned char*)state->regs[i], context);
            set_context_reg(module, csw, &new_context, i, value, TRUE);
            break;
        case RULE_VAL_EXPRESSION:
            value = eval_expression(module, csw, (const unsigned char*)state->regs[i], context);
            set_context_reg(module, csw, &new_context, i, value, FALSE);
            break;
        }
    }
    *context = new_context;
}

static BOOL dwarf2_fetch_frame_info(struct module* module, struct cpu* cpu, LONG_PTR ip, struct frame_info* info)
{
    dwarf2_traverse_context_t cie_ctx, fde_ctx;
    struct module_format* modfmt;
    const unsigned char* end;
    DWORD_PTR delta;

    modfmt = module->format_info[DFI_DWARF];
    if (!modfmt) return FALSE;
    memset(info, 0, sizeof(*info));
    fde_ctx.data = modfmt->u.dwarf2_info->eh_frame.address;
    fde_ctx.end_data = fde_ctx.data + modfmt->u.dwarf2_info->eh_frame.size;
    /* let offsets relative to the eh_frame sections be correctly computed, as we'll map
     * in this process the IMAGE section at a different address as the one expected by
     * the image
     */
    delta = module->module.BaseOfImage + modfmt->u.dwarf2_info->eh_frame.rva -
        (DWORD_PTR)modfmt->u.dwarf2_info->eh_frame.address;
    if (!dwarf2_get_cie(ip, module, delta, &fde_ctx, &cie_ctx, info, TRUE))
    {
        fde_ctx.data = modfmt->u.dwarf2_info->debug_frame.address;
        fde_ctx.end_data = fde_ctx.data + modfmt->u.dwarf2_info->debug_frame.size;
        delta = module->reloc_delta;
        if (modfmt->u.dwarf2_info->debug_frame.address == IMAGE_NO_MAP ||
            !dwarf2_get_cie(ip, module, delta, &fde_ctx, &cie_ctx, info, FALSE))
        {
            TRACE("Couldn't find information for %Ix\n", ip);
            return FALSE;
        }
    }

    TRACE("function %Ix/%Ix code_align %Iu data_align %Id retaddr %s\n",
          ip, info->ip, info->code_align, info->data_align,
          cpu->fetch_regname(cpu->map_dwarf_register(info->retaddr_reg, module, TRUE)));

    if (ip != info->ip)
    {
        execute_cfa_instructions(module, &cie_ctx, ip, info);

        if (info->aug_z_format)  /* get length of augmentation data */
        {
            ULONG_PTR len = dwarf2_leb128_as_unsigned(&fde_ctx);
            end = fde_ctx.data + len;
        }
        else end = NULL;
        dwarf2_parse_augmentation_ptr(&fde_ctx, info->lsda_encoding, modfmt->u.dwarf2_info->word_size); /* handler_data */
        if (end) fde_ctx.data = end;

        execute_cfa_instructions(module, &fde_ctx, ip, info);
    }
    return TRUE;
}

/***********************************************************************
 *           dwarf2_virtual_unwind
 *
 */
BOOL dwarf2_virtual_unwind(struct cpu_stack_walk *csw, ULONG_PTR ip,
    union ctx *context, DWORD64 *cfa)
{
    struct module_pair pair;
    struct frame_info info;

    if (!module_init_pair(&pair, csw->hProcess, ip)) return FALSE;
    if (csw->cpu != pair.effective->cpu) FIXME("mismatch in cpu\n");
    if (!dwarf2_fetch_frame_info(pair.effective, csw->cpu, ip, &info)) return FALSE;

    /* if at very beginning of function, return and use default unwinder */
    if (ip == info.ip) return FALSE;

    /* if there is no information about retaddr, use default unwinder */
    if (info.state.rules[info.retaddr_reg] == RULE_UNSET) return FALSE;

    apply_frame_state(pair.effective, csw, context, &info.state, cfa);

    return TRUE;
}

static enum location_error compute_call_frame_cfa(struct module* module, ULONG_PTR ip, struct location* frame)
{
    struct frame_info info;

    if (!dwarf2_fetch_frame_info(module, module->cpu, ip, &info)) return loc_err_internal;

    /* beginning of function, or no available dwarf information ? */
    if (ip == info.ip || info.state.rules[info.retaddr_reg] == RULE_UNSET)
    {
        /* fake the default unwinder */
        frame->kind = loc_regrel;
        frame->reg = module->cpu->frame_regno;
        frame->offset = module->cpu->word_size; /* FIXME stack direction */
    }
    else
    {
        /* we expect to translate the call_frame_cfa into a regrel location...
         * that should cover most of the cases
         */
        switch (info.state.cfa_rule)
        {
        case RULE_EXPRESSION:
            WARN("Too complex expression for frame_CFA resolution (RULE_EXPRESSION)\n");
            return loc_err_too_complex;
        case RULE_VAL_EXPRESSION:
            /* unfortunately, we've seen at least construct like:
             *     cfa := 'breg_x + offset; deref'
             * which is an indirection too much for the DbgHelp API.
             */
            WARN("Too complex expression for frame_CFA resolution (RULE_VAL_EXPRESSION)\n");
            return loc_err_too_complex;
        default:
            frame->kind = loc_regrel;
            frame->reg = module->cpu->map_dwarf_register(info.state.cfa_reg, module, TRUE);
            frame->offset = info.state.cfa_offset;
            break;
        }
    }
    return 0;
}

static void dwarf2_location_compute(const struct module_format* modfmt,
                                    const struct symt_function* func,
                                    struct location* loc)
{
    struct location             frame;
    DWORD_PTR                   ip;
    int                         err;
    dwarf2_traverse_context_t   lctx;
    const dwarf2_cuhead_t*      head = get_cuhead_from_func(func);

    if (!head)
    {
        WARN("We'd expect function %s's container to be a valid compiland with dwarf inforamation\n",
             debugstr_a(func->hash_elt.name));
        err = loc_err_internal;
    }
    else
    {
        struct process *pcs = modfmt->module->process;
        /* instruction pointer relative to compiland's start */
        ip = pcs->localscope_pc - ((struct symt_compiland*)func->container)->address;

        if ((err = loc_compute_frame(modfmt, func, ip, head, &frame)) == 0)
        {
            switch (loc->kind)
            {
            case loc_dwarf2_location_list:
                /* Then, if the variable has a location list, find it !! */
                if (dwarf2_lookup_loclist(modfmt, head,
                                          modfmt->u.dwarf2_info->debug_loc.address + loc->offset,
                                          ip, &lctx))
                    goto do_compute;
                err = loc_err_out_of_scope;
                break;
            case loc_dwarf2_block:
                /* or if we have a copy of an existing block, get ready for it */
                {
                    unsigned*   ptr = (unsigned*)loc->offset;

                    lctx.data = (const BYTE*)(ptr + 1);
                    lctx.end_data = lctx.data + *ptr;
                }
            do_compute:
                /* now get the variable */
                err = compute_location(modfmt->module, head,
                                       &lctx, loc, pcs->handle, &frame);
                break;
            case loc_register:
            case loc_regrel:
                /* nothing to do */
                break;
            default:
                WARN("Unsupported local kind %d\n", loc->kind);
                err = loc_err_internal;
            }
        }
    }
    if (err < 0)
    {
        loc->kind = loc_register;
        loc->reg = err;
    }
}

static void *zalloc(void *priv, uInt items, uInt sz)
{
    return HeapAlloc(GetProcessHeap(), 0, items * sz);
}

static void zfree(void *priv, void *addr)
{
    HeapFree(GetProcessHeap(), 0, addr);
}

static inline BOOL dwarf2_init_zsection(dwarf2_section_t* section,
                                        const char* zsectname,
                                        struct image_section_map* ism)
{
    z_stream z;
    LARGE_INTEGER li;
    int res;
    BOOL ret = FALSE;

    BYTE *addr, *sect = (BYTE *)image_map_section(ism);
    size_t sz = image_get_map_size(ism);

    if (sz <= 12 || memcmp(sect, "ZLIB", 4))
    {
        ERR("invalid compressed section %s\n", debugstr_a(zsectname));
        goto out;
    }

#ifdef WORDS_BIGENDIAN
    li.u.HighPart = *(DWORD*)&sect[4];
    li.u.LowPart = *(DWORD*)&sect[8];
#else
    li.u.HighPart = RtlUlongByteSwap(*(DWORD*)&sect[4]);
    li.u.LowPart = RtlUlongByteSwap(*(DWORD*)&sect[8]);
#endif

    addr = HeapAlloc(GetProcessHeap(), 0, li.QuadPart);
    if (!addr)
        goto out;

    z.next_in = &sect[12];
    z.avail_in = sz - 12;
    z.opaque = NULL;
    z.zalloc = zalloc;
    z.zfree = zfree;

    res = inflateInit(&z);
    if (res != Z_OK)
    {
        FIXME("inflateInit failed with %i / %s\n", res, debugstr_a(z.msg));
        goto out_free;
    }

    do {
        z.next_out = addr + z.total_out;
        z.avail_out = li.QuadPart - z.total_out;
        res = inflate(&z, Z_FINISH);
    } while (z.avail_in && res == Z_STREAM_END);

    if (res != Z_STREAM_END)
    {
        FIXME("Decompression failed with %i / %s\n", res, debugstr_a(z.msg));
        goto out_end;
    }

    ret = TRUE;
    section->compressed = TRUE;
    section->address = addr;
    section->rva = image_get_map_rva(ism);
    section->size = z.total_out;

out_end:
    inflateEnd(&z);
out_free:
    if (!ret)
        HeapFree(GetProcessHeap(), 0, addr);
out:
    image_unmap_section(ism);
    return ret;
}

static inline BOOL dwarf2_init_section(dwarf2_section_t* section, struct image_file_map* fmap,
                                       const char* sectname, const char* zsectname,
                                       struct image_section_map* ism)
{
    struct image_section_map    local_ism;

    if (!ism) ism = &local_ism;

    section->compressed = FALSE;
    if (image_find_section(fmap, sectname, ism))
    {
        section->address = (const BYTE*)image_map_section(ism);
        section->size    = image_get_map_size(ism);
        section->rva     = image_get_map_rva(ism);
        return TRUE;
    }

    section->address = NULL;
    section->size    = 0;
    section->rva     = 0;

    if (zsectname && image_find_section(fmap, zsectname, ism))
    {
        return dwarf2_init_zsection(section, zsectname, ism);
    }

    return FALSE;
}

static inline void dwarf2_fini_section(dwarf2_section_t* section)
{
    if (section->compressed)
        HeapFree(GetProcessHeap(), 0, (void*)section->address);
}

static void dwarf2_module_remove(struct module_format* modfmt)
{
    dwarf2_fini_section(&modfmt->u.dwarf2_info->debug_loc);
    dwarf2_fini_section(&modfmt->u.dwarf2_info->debug_frame);
    free(modfmt->u.dwarf2_info->cuheads);
    HeapFree(GetProcessHeap(), 0, modfmt);
}

static BOOL dwarf2_load_CU_module(dwarf2_parse_module_context_t* module_ctx, struct module* module,
                                  dwarf2_section_t* sections, ULONG_PTR load_offset,
                                  const struct elf_thunk_area* thunks, BOOL is_dwz)
{
    dwarf2_traverse_context_t   mod_ctx;
    unsigned i;

    module_ctx->sections = sections;
    module_ctx->module = module;
    module_ctx->thunks = thunks;
    module_ctx->load_offset = load_offset;
    vector_init(&module_ctx->unit_contexts, sizeof(dwarf2_parse_context_t*), 0);
    module_ctx->cu_versions = 0;

    /* phase I: parse all CU heads */
    mod_ctx.data = sections[section_debug].address;
    mod_ctx.end_data = mod_ctx.data + sections[section_debug].size;
    while (mod_ctx.data < mod_ctx.end_data)
    {
        dwarf2_parse_context_t **punit_ctx = vector_add(&module_ctx->unit_contexts, &module_ctx->module->pool);

        if (!(*punit_ctx = pool_alloc(&module_ctx->module->pool, sizeof(dwarf2_parse_context_t))))
            return FALSE;

        (*punit_ctx)->module_ctx = module_ctx;
        dwarf2_parse_compilation_unit_head(*punit_ctx, &mod_ctx);
    }

    /* phase2: load content of all CU
     * If this is a DWZ alternate module, don't load all debug_info at once
     * wait for main module to ask for them (it's likely it won't need them all)
     * Doing this can lead to a huge performance improvement.
     */
    if (!is_dwz)
        for (i = 0; i < module_ctx->unit_contexts.num_elts; ++i)
            dwarf2_parse_compilation_unit(*(dwarf2_parse_context_t**)vector_at(&module_ctx->unit_contexts, i));

    return TRUE;
}

static dwarf2_dwz_alternate_t* dwarf2_load_dwz(struct image_file_map* fmap, struct module* module)
{
    struct image_file_map* fmap_dwz;
    dwarf2_dwz_alternate_t* dwz;

    fmap_dwz = image_load_debugaltlink(fmap, module);
    if (!fmap_dwz) return NULL;
    if (!(dwz = HeapAlloc(GetProcessHeap(), 0, sizeof(*dwz))))
    {
        image_unmap_file(fmap_dwz);
        HeapFree(GetProcessHeap(), 0, fmap_dwz);
        return NULL;
    }

    dwz->fmap = fmap_dwz;
    dwarf2_init_section(&dwz->sections[section_debug],  fmap_dwz, ".debug_info",   ".zdebug_info",   &dwz->sectmap[section_debug]);
    dwarf2_init_section(&dwz->sections[section_abbrev], fmap_dwz, ".debug_abbrev", ".zdebug_abbrev", &dwz->sectmap[section_abbrev]);
    dwarf2_init_section(&dwz->sections[section_string], fmap_dwz, ".debug_str",    ".zdebug_str",    &dwz->sectmap[section_string]);
    dwarf2_init_section(&dwz->sections[section_line],   fmap_dwz, ".debug_line",   ".zdebug_line",   &dwz->sectmap[section_line]);
    dwarf2_init_section(&dwz->sections[section_ranges], fmap_dwz, ".debug_ranges", ".zdebug_ranges", &dwz->sectmap[section_ranges]);

    dwz->module_ctx.dwz = NULL;
    dwarf2_load_CU_module(&dwz->module_ctx, module, dwz->sections, 0/*FIXME*/, NULL, TRUE);
    return dwz;
}

static void dwarf2_unload_dwz(dwarf2_dwz_alternate_t* dwz)
{
    if (!dwz) return;
    dwarf2_fini_section(&dwz->sections[section_debug]);
    dwarf2_fini_section(&dwz->sections[section_abbrev]);
    dwarf2_fini_section(&dwz->sections[section_string]);
    dwarf2_fini_section(&dwz->sections[section_line]);
    dwarf2_fini_section(&dwz->sections[section_ranges]);

    image_unmap_section(&dwz->sectmap[section_debug]);
    image_unmap_section(&dwz->sectmap[section_abbrev]);
    image_unmap_section(&dwz->sectmap[section_string]);
    image_unmap_section(&dwz->sectmap[section_line]);
    image_unmap_section(&dwz->sectmap[section_ranges]);

    HeapFree(GetProcessHeap(), 0, dwz);
}

static BOOL dwarf2_unload_CU_module(dwarf2_parse_module_context_t* module_ctx)
{
    unsigned i;
    for (i = 0; i < module_ctx->unit_contexts.num_elts; ++i)
    {
        dwarf2_parse_context_t* unit = *(dwarf2_parse_context_t**)vector_at(&module_ctx->unit_contexts, i);
        if (unit && unit->status != UNIT_ERROR)
            pool_destroy(&unit->pool);
        pool_free(&module_ctx->module->pool, unit);
    }
    dwarf2_unload_dwz(module_ctx->dwz);
    return TRUE;
}

static const struct module_format_vtable dwarf2_module_format_vtable =
{
    dwarf2_module_remove,
    dwarf2_location_compute,
};

BOOL dwarf2_parse(struct module* module, ULONG_PTR load_offset,
                  const struct elf_thunk_area* thunks,
                  struct image_file_map* fmap)
{
    dwarf2_section_t    eh_frame, section[section_max];
    struct image_section_map    debug_sect, debug_str_sect, debug_abbrev_sect,
                                debug_line_sect, debug_ranges_sect, eh_frame_sect;
    BOOL                ret = TRUE;
    struct module_format* dwarf2_modfmt;
    dwarf2_parse_module_context_t module_ctx;

    if (!dwarf2_init_section(&eh_frame,                fmap, ".eh_frame",     NULL,             &eh_frame_sect))
        /* lld produces .eh_fram to avoid generating a long name */
        dwarf2_init_section(&eh_frame,                fmap, ".eh_fram",      NULL,             &eh_frame_sect);
    dwarf2_init_section(&section[section_debug],  fmap, ".debug_info",   ".zdebug_info",   &debug_sect);
    dwarf2_init_section(&section[section_abbrev], fmap, ".debug_abbrev", ".zdebug_abbrev", &debug_abbrev_sect);
    dwarf2_init_section(&section[section_string], fmap, ".debug_str",    ".zdebug_str",    &debug_str_sect);
    dwarf2_init_section(&section[section_line],   fmap, ".debug_line",   ".zdebug_line",   &debug_line_sect);
    dwarf2_init_section(&section[section_ranges], fmap, ".debug_ranges", ".zdebug_ranges", &debug_ranges_sect);

    /* to do anything useful we need either .eh_frame or .debug_info */
    if ((!eh_frame.address || eh_frame.address == IMAGE_NO_MAP) &&
        (!section[section_debug].address || section[section_debug].address == IMAGE_NO_MAP))
    {
        ret = FALSE;
        goto leave;
    }

    if (fmap->modtype == DMT_ELF && debug_sect.fmap)
    {
        /* debug info might have a different base address than .so file
         * when elf file is prelinked after splitting off debug info
         * adjust symbol base addresses accordingly
         */
        load_offset += fmap->u.elf.elf_start - debug_sect.fmap->u.elf.elf_start;
    }

    TRACE("Loading Dwarf2 information for %s\n", debugstr_w(module->modulename));

    dwarf2_modfmt = HeapAlloc(GetProcessHeap(), 0,
                              sizeof(*dwarf2_modfmt) + sizeof(*dwarf2_modfmt->u.dwarf2_info));
    if (!dwarf2_modfmt)
    {
        ret = FALSE;
        goto leave;
    }
    dwarf2_modfmt->module = module;
    dwarf2_modfmt->vtable = &dwarf2_module_format_vtable;
    dwarf2_modfmt->u.dwarf2_info = (struct dwarf2_module_info_s*)(dwarf2_modfmt + 1);
    dwarf2_modfmt->u.dwarf2_info->word_size = fmap->addr_size / 8; /* set the word_size for eh_frame parsing */
    dwarf2_modfmt->module->format_info[DFI_DWARF] = dwarf2_modfmt;

    /* As we'll need later some sections' content, we won't unmap these
     * sections upon existing this function
     */
    dwarf2_init_section(&dwarf2_modfmt->u.dwarf2_info->debug_loc,   fmap, ".debug_loc",   ".zdebug_loc",   NULL);
    dwarf2_init_section(&dwarf2_modfmt->u.dwarf2_info->debug_frame, fmap, ".debug_frame", ".zdebug_frame", NULL);
    dwarf2_modfmt->u.dwarf2_info->eh_frame = eh_frame;
    dwarf2_modfmt->u.dwarf2_info->cuheads = NULL;
    dwarf2_modfmt->u.dwarf2_info->num_cuheads = 0;

    module_ctx.dwz = dwarf2_load_dwz(fmap, module);
    dwarf2_load_CU_module(&module_ctx, module, section, load_offset, thunks, FALSE);

    if (module_ctx.cu_versions)
    {
        dwarf2_modfmt->module->module.SymType = SymDia;
        module->debug_format_bitmask |= module_ctx.cu_versions;
        /* FIXME: we could have a finer grain here */
        dwarf2_modfmt->module->module.GlobalSymbols = TRUE;
        dwarf2_modfmt->module->module.TypeInfo = TRUE;
        dwarf2_modfmt->module->module.SourceIndexed = TRUE;
        dwarf2_modfmt->module->module.Publics = TRUE;
    }

    dwarf2_unload_CU_module(&module_ctx);
leave:

    dwarf2_fini_section(&section[section_debug]);
    dwarf2_fini_section(&section[section_abbrev]);
    dwarf2_fini_section(&section[section_string]);
    dwarf2_fini_section(&section[section_line]);
    dwarf2_fini_section(&section[section_ranges]);

    image_unmap_section(&debug_sect);
    image_unmap_section(&debug_abbrev_sect);
    image_unmap_section(&debug_str_sect);
    image_unmap_section(&debug_line_sect);
    image_unmap_section(&debug_ranges_sect);
    if (!ret) image_unmap_section(&eh_frame_sect);

    return ret;
}
