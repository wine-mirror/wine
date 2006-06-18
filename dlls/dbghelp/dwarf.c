/*
 * File dwarf.c - read dwarf2 information from the ELF modules
 *
 * Copyright (C) 2005, Raphael Junqueira
 * Copyright (C) 2006, Eric Pouech
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

#include "config.h"

#include <sys/types.h>
#include <fcntl.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdio.h>
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"

#include "dbghelp_private.h"

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
  const BYTE* x = (const BYTE*)ptr;

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

typedef struct dwarf2_abbrev_entry_attr_s {
  unsigned long attribute;
  unsigned long form;
  struct dwarf2_abbrev_entry_attr_s* next;
} dwarf2_abbrev_entry_attr_t;

typedef struct dwarf2_abbrev_entry_s
{
    unsigned long entry_code;
    unsigned long tag;
    unsigned char have_child;
    unsigned num_attr;
    dwarf2_abbrev_entry_attr_t* attrs;
} dwarf2_abbrev_entry_t;

struct dwarf2_block
{
    unsigned                    size;
    const unsigned char*        ptr;
};

union attribute
{
    unsigned long                   uvalue;
    long                            svalue;
    const char*                     string;
    struct dwarf2_block*            block;
};

typedef struct dwarf2_debug_info_s
{
    unsigned long               offset;
    const dwarf2_abbrev_entry_t*abbrev;
    struct symt*                symt;
    union attribute*            attributes;
    struct vector               children;
} dwarf2_debug_info_t;


typedef struct dwarf2_section_s
{
    const unsigned char*        address;
    unsigned                    size;
} dwarf2_section_t;

enum dwarf2_sections {section_debug, section_string, section_abbrev, section_line, section_max};

typedef struct dwarf2_traverse_context_s
{
    const dwarf2_section_t*     sections;
    unsigned                    section;
    const unsigned char*        data;
    const unsigned char*        start_data;
    const unsigned char*        end_data;
    unsigned long               offset;
    unsigned char               word_size;
} dwarf2_traverse_context_t;

typedef struct dwarf2_parse_context_s
{
    struct pool                 pool;
    struct module*              module;
    struct sparse_array         abbrev_table;
    struct sparse_array         debug_info_table;
    unsigned char               word_size;
} dwarf2_parse_context_t;

/* forward declarations */
static struct symt* dwarf2_parse_enumeration_type(dwarf2_parse_context_t* ctx, dwarf2_debug_info_t* entry);

static unsigned char dwarf2_parse_byte(dwarf2_traverse_context_t* ctx)
{
  unsigned char uvalue = *(const unsigned char*) ctx->data;
  ctx->data += 1;
  return uvalue;
}

static unsigned short dwarf2_parse_u2(dwarf2_traverse_context_t* ctx)
{
  unsigned short uvalue = *(const unsigned short*) ctx->data;
  ctx->data += 2;
  return uvalue;
}

static unsigned long dwarf2_parse_u4(dwarf2_traverse_context_t* ctx)
{
  unsigned long uvalue = *(const unsigned int*) ctx->data;
  ctx->data += 4;
  return uvalue;
}

static unsigned long dwarf2_leb128_as_unsigned(dwarf2_traverse_context_t* ctx)
{
  unsigned long ret = 0;
  unsigned char byte;
  unsigned shift = 0;

  assert( NULL != ctx );

  while (1) {
    byte = dwarf2_parse_byte(ctx);
    ret |= (byte & 0x7f) << shift;
    shift += 7;
    if (0 == (byte & 0x80)) { break ; }
  }

  return ret;
}

static long dwarf2_leb128_as_signed(dwarf2_traverse_context_t* ctx)
{
  long ret = 0;
  unsigned char byte;
  unsigned shift = 0;
  const unsigned size = sizeof(int) * 8;

  assert( NULL != ctx );

  while (1) {
    byte = dwarf2_parse_byte(ctx);
    ret |= (byte & 0x7f) << shift;
    shift += 7;
    if (0 == (byte & 0x80)) { break ; }
  }
  /* as spec: sign bit of byte is 2nd high order bit (80x40)
   *  -> 0x80 is used as flag.
   */
  if ((shift < size) && (byte & 0x40)) {
    ret |= - (1 << shift);
  }
  return ret;
}

static unsigned long dwarf2_parse_addr(dwarf2_traverse_context_t* ctx)
{
    unsigned long ret;

    switch (ctx->word_size)
    {
    case 4:
        ret = dwarf2_parse_u4(ctx);
        break;
    default:
        FIXME("Unsupported Word Size %u\n", ctx->word_size);
        ret = 0;
    }
    return ret;
}

static const char* dwarf2_debug_traverse_ctx(const dwarf2_traverse_context_t* ctx) 
{
    return wine_dbg_sprintf("ctx(0x%x)", ctx->data - ctx->sections[ctx->section].address); 
}

static const char* dwarf2_debug_ctx(const dwarf2_parse_context_t* ctx) 
{
    return wine_dbg_sprintf("ctx(%p,%s)", ctx, ctx->module->module.ModuleName);
}

static const char* dwarf2_debug_di(dwarf2_debug_info_t* di) 
{
    return wine_dbg_sprintf("debug_info(offset:0x%lx,abbrev:%p,symt:%p)",
                            di->offset, di->abbrev, di->symt);
}

static dwarf2_abbrev_entry_t*
dwarf2_abbrev_table_find_entry(struct sparse_array* abbrev_table,
                               unsigned long entry_code)
{
    assert( NULL != abbrev_table );
    return sparse_array_find(abbrev_table, entry_code);
}

static void dwarf2_parse_abbrev_set(dwarf2_traverse_context_t* abbrev_ctx, 
                                    struct sparse_array* abbrev_table,
                                    struct pool* pool)
{
    unsigned long entry_code;
    dwarf2_abbrev_entry_t* abbrev_entry;
    dwarf2_abbrev_entry_attr_t* new = NULL;
    dwarf2_abbrev_entry_attr_t* last = NULL;
    unsigned long attribute;
    unsigned long form;

    assert( NULL != abbrev_ctx );

    TRACE("%s, end at %p\n",
          dwarf2_debug_traverse_ctx(abbrev_ctx), abbrev_ctx->end_data); 

    sparse_array_init(abbrev_table, sizeof(dwarf2_abbrev_entry_t), 32);
    while (abbrev_ctx->data < abbrev_ctx->end_data)
    {
        TRACE("now at %s\n", dwarf2_debug_traverse_ctx(abbrev_ctx)); 
        entry_code = dwarf2_leb128_as_unsigned(abbrev_ctx);
        TRACE("found entry_code %lu\n", entry_code);
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

        TRACE("table:(%p,#%u) entry_code(%lu) tag(0x%lx) have_child(%u) -> %p\n",
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

static void dwarf2_parse_attr_into_di(struct pool* pool,
                                      dwarf2_traverse_context_t* ctx,
                                      const dwarf2_abbrev_entry_attr_t* abbrev_attr,
                                      union attribute* attr)

{
    TRACE("(attr:0x%lx,form:0x%lx)\n", abbrev_attr->attribute, abbrev_attr->form);

    switch (abbrev_attr->form) {
    case DW_FORM_ref_addr:
    case DW_FORM_addr:
        attr->uvalue = dwarf2_parse_addr(ctx);
        TRACE("addr<0x%lx>\n", attr->uvalue);
        break;

    case DW_FORM_flag:
        attr->uvalue = dwarf2_parse_byte(ctx);
        TRACE("flag<0x%lx>\n", attr->uvalue);
        break;

    case DW_FORM_data1:
        attr->uvalue = dwarf2_parse_byte(ctx);
        TRACE("data1<%lu>\n", attr->uvalue);
        break;

    case DW_FORM_data2:
        attr->uvalue = dwarf2_parse_u2(ctx);
        TRACE("data2<%lu>\n", attr->uvalue);
        break;

    case DW_FORM_data4:
        attr->uvalue = dwarf2_parse_u4(ctx);
        TRACE("data4<%lu>\n", attr->uvalue);
        break;

    case DW_FORM_data8:
        FIXME("Unhandled 64bits support\n");
        ctx->data += 8;
        break;

    case DW_FORM_ref1:
        attr->uvalue = ctx->offset + dwarf2_parse_byte(ctx);
        TRACE("ref1<0x%lx>\n", attr->uvalue);
        break;

    case DW_FORM_ref2:
        attr->uvalue = ctx->offset + dwarf2_parse_u2(ctx);
        TRACE("ref2<0x%lx>\n", attr->uvalue);
        break;

    case DW_FORM_ref4:
        attr->uvalue = ctx->offset + dwarf2_parse_u4(ctx);
        TRACE("ref4<0x%lx>\n", attr->uvalue);
        break;
    
    case DW_FORM_ref8:
        FIXME("Unhandled 64 bit support\n");
        ctx->data += 8;
        break;

    case DW_FORM_sdata:
        attr->svalue = dwarf2_leb128_as_signed(ctx);
        break;

    case DW_FORM_ref_udata:
        attr->uvalue = dwarf2_leb128_as_unsigned(ctx);
        break;

    case DW_FORM_udata:
        attr->uvalue = dwarf2_leb128_as_unsigned(ctx);
        break;

    case DW_FORM_string:
        attr->string = (const char*)ctx->data;
        ctx->data += strlen(attr->string) + 1;
        TRACE("string<%s>\n", attr->string);
        break;

    case DW_FORM_strp:
        {
            unsigned long offset = dwarf2_parse_u4(ctx);
            attr->string = (const char*)ctx->sections[section_string].address + offset;
        }
        TRACE("strp<%s>\n", attr->string);
        break;
    case DW_FORM_block:
        attr->block = pool_alloc(pool, sizeof(struct dwarf2_block));
        attr->block->size = dwarf2_leb128_as_unsigned(ctx);
        attr->block->ptr  = ctx->data;
        ctx->data += attr->block->size;
        break;

    case DW_FORM_block1:
        attr->block = pool_alloc(pool, sizeof(struct dwarf2_block));
        attr->block->size = dwarf2_parse_byte(ctx);
        attr->block->ptr  = ctx->data;
        ctx->data += attr->block->size;
        break;

    case DW_FORM_block2:
        attr->block = pool_alloc(pool, sizeof(struct dwarf2_block));
        attr->block->size = dwarf2_parse_u2(ctx);
        attr->block->ptr  = ctx->data;
        ctx->data += attr->block->size;
        break;

    case DW_FORM_block4:
        attr->block = pool_alloc(pool, sizeof(struct dwarf2_block));
        attr->block->size = dwarf2_parse_u4(ctx);
        attr->block->ptr  = ctx->data;
        ctx->data += attr->block->size;
        break;

    default:
        FIXME("Unhandled attribute form %lx\n", abbrev_attr->form);
        break;
    }
}

static BOOL dwarf2_find_attribute(const dwarf2_debug_info_t* di,
                                  unsigned at, union attribute* attr)
{
    unsigned                    i;
    dwarf2_abbrev_entry_attr_t* abbrev_attr;

    for (i = 0, abbrev_attr = di->abbrev->attrs; abbrev_attr; i++, abbrev_attr = abbrev_attr->next)
    {
        if (abbrev_attr->attribute == at)
        {
            *attr = di->attributes[i];
            return TRUE;
        }
    }
    return FALSE;
}

static void dwarf2_find_name(dwarf2_parse_context_t* ctx,
                             const dwarf2_debug_info_t* di,
                             union attribute* attr, const char* pfx)
{
    static      int index;

    if (!dwarf2_find_attribute(di, DW_AT_name, attr))
    {
        char* tmp = pool_alloc(&ctx->pool, strlen(pfx) + 16);
        if (tmp) sprintf(tmp, "%s_%d", pfx, index++);
        attr->string = tmp;
    }
}

static void dwarf2_load_one_entry(dwarf2_parse_context_t*, dwarf2_debug_info_t*,
                                  struct symt_compiland*);

#define Wine_DW_no_register     -1
#define Wine_DW_frame_register  -2

static unsigned long dwarf2_compute_location(dwarf2_parse_context_t* ctx,
                                             struct dwarf2_block* block,
                                             int* in_register)
{
    unsigned long loc[64];
    unsigned stk;

    if (in_register) *in_register = Wine_DW_no_register;
    loc[stk = 0] = 0;

    if (block->size)
    {
        dwarf2_traverse_context_t  lctx;
        unsigned char op;
        BOOL piece_found = FALSE;

        lctx.data = block->ptr;
        lctx.end_data = block->ptr + block->size;
        lctx.word_size = ctx->word_size;

        while (lctx.data < lctx.end_data)
        {
            op = dwarf2_parse_byte(&lctx);
            switch (op)
            {
            case DW_OP_addr:    loc[++stk] = dwarf2_parse_addr(&lctx); break;
            case DW_OP_const1u: loc[++stk] = dwarf2_parse_byte(&lctx); break;
            case DW_OP_const1s: loc[++stk] = (long)(signed char)dwarf2_parse_byte(&lctx); break;
            case DW_OP_const2u: loc[++stk] = dwarf2_parse_u2(&lctx); break;
            case DW_OP_const2s: loc[++stk] = (long)(short)dwarf2_parse_u2(&lctx); break;
            case DW_OP_const4u: loc[++stk] = dwarf2_parse_u4(&lctx); break;
            case DW_OP_const4s: loc[++stk] = dwarf2_parse_u4(&lctx); break;
            case DW_OP_constu:  loc[++stk] = dwarf2_leb128_as_unsigned(&lctx); break;
            case DW_OP_consts:  loc[++stk] = dwarf2_leb128_as_signed(&lctx); break;
            case DW_OP_plus_uconst:
                                loc[stk] += dwarf2_leb128_as_unsigned(&lctx); break;
            case DW_OP_reg0:  case DW_OP_reg1:  case DW_OP_reg2:  case DW_OP_reg3:
            case DW_OP_reg4:  case DW_OP_reg5:  case DW_OP_reg6:  case DW_OP_reg7:
            case DW_OP_reg8:  case DW_OP_reg9:  case DW_OP_reg10: case DW_OP_reg11:
            case DW_OP_reg12: case DW_OP_reg13: case DW_OP_reg14: case DW_OP_reg15:
            case DW_OP_reg16: case DW_OP_reg17: case DW_OP_reg18: case DW_OP_reg19:
            case DW_OP_reg20: case DW_OP_reg21: case DW_OP_reg22: case DW_OP_reg23:
            case DW_OP_reg24: case DW_OP_reg25: case DW_OP_reg26: case DW_OP_reg27:
            case DW_OP_reg28: case DW_OP_reg29: case DW_OP_reg30: case DW_OP_reg31:
                if (in_register)
                {
                    /* dbghelp APIs don't know how to cope with this anyway
                     * (for example 'long long' stored in two registers)
                     * FIXME: We should tell winedbg how to deal with it (sigh)
                     */
                    if (!piece_found || (op - DW_OP_reg0 != *in_register + 1))
                    {
                        if (*in_register != Wine_DW_no_register)
                            FIXME("Only supporting one reg (%d -> %d)\n", 
                                  *in_register, op - DW_OP_reg0);
                        *in_register = op - DW_OP_reg0;
                    }
                }
                else FIXME("Found register, while not expecting it\n");
                break;
            case DW_OP_fbreg:
                if (in_register)
                {
                    if (*in_register != Wine_DW_no_register)
                        FIXME("Only supporting one reg (%d -> -2)\n", *in_register);
                    *in_register = Wine_DW_frame_register;
                }
                else FIXME("Found register, while not expecting it\n");
                loc[++stk] = dwarf2_leb128_as_signed(&lctx);
                break;
            case DW_OP_piece:
                {
                    unsigned sz = dwarf2_leb128_as_unsigned(&lctx);
                    WARN("Not handling OP_piece directly (size=%d)\n", sz);
                    piece_found = TRUE;
                }
                break;
            default:
                FIXME("Unhandled attr op: %x\n", op);
                return loc[stk];
            }
        }
    }
    return loc[stk];
}

static struct symt* dwarf2_lookup_type(dwarf2_parse_context_t* ctx,
                                       const dwarf2_debug_info_t* di)
{
    union attribute     attr;

    if (dwarf2_find_attribute(di, DW_AT_type, &attr))
    {
        dwarf2_debug_info_t*    type;
        
        type = sparse_array_find(&ctx->debug_info_table, attr.uvalue);
        if (!type) FIXME("Unable to find back reference to type %lx\n", attr.uvalue);
        if (!type->symt)
        {
            /* load the debug info entity */
            dwarf2_load_one_entry(ctx, type, NULL);
        }
        return type->symt;
    }
    return NULL;
}

/******************************************************************
 *		dwarf2_read_one_debug_info
 *
 * Loads into memory one debug info entry, and recursively its children (if any)
 */
static BOOL dwarf2_read_one_debug_info(dwarf2_parse_context_t* ctx,
                                       dwarf2_traverse_context_t* traverse,
                                       dwarf2_debug_info_t** pdi)
{
    const dwarf2_abbrev_entry_t*abbrev;
    unsigned long               entry_code;
    unsigned long               offset;
    dwarf2_debug_info_t*        di;
    dwarf2_debug_info_t*        child;
    dwarf2_debug_info_t**       where;
    dwarf2_abbrev_entry_attr_t* attr;
    unsigned                    i;
    union attribute             sibling;

    offset = traverse->data - traverse->sections[traverse->section].address;
    entry_code = dwarf2_leb128_as_unsigned(traverse);
    TRACE("found entry_code %lu at 0x%lx\n", entry_code, offset);
    if (!entry_code)
    {
        *pdi = NULL;
        return TRUE;
    }
    abbrev = dwarf2_abbrev_table_find_entry(&ctx->abbrev_table, entry_code);
    if (!abbrev)
    {
	WARN("Cannot find abbrev entry for %lu at 0x%lx\n", entry_code, offset);
	return FALSE;
    }
    di = sparse_array_add(&ctx->debug_info_table, offset, &ctx->pool);
    if (!di) return FALSE;
    di->offset = offset;
    di->abbrev = abbrev;
    di->symt   = NULL;

    if (abbrev->num_attr)
    {
        di->attributes = pool_alloc(&ctx->pool,
                                    abbrev->num_attr * sizeof(union attribute));
        for (i = 0, attr = abbrev->attrs; attr; i++, attr = attr->next)
        {
            dwarf2_parse_attr_into_di(&ctx->pool, traverse, attr, &di->attributes[i]);
        }
    }
    else di->attributes = NULL;
    if (abbrev->have_child)
    {
        vector_init(&di->children, sizeof(dwarf2_debug_info_t*), 16);
        while (traverse->data < traverse->end_data)
        {
            if (!dwarf2_read_one_debug_info(ctx, traverse, &child)) return FALSE;
            if (!child) break;
            where = vector_add(&di->children, &ctx->pool);
            if (!where) return FALSE;
            *where = child;
        }
    }
    if (dwarf2_find_attribute(di, DW_AT_sibling, &sibling) &&
        traverse->data != traverse->sections[traverse->section].address + sibling.uvalue)
    {
        WARN("setting cursor for %s to next sibling <0x%lx>\n",
             dwarf2_debug_traverse_ctx(traverse), sibling.uvalue);
        traverse->data = traverse->sections[traverse->section].address + sibling.uvalue;
    }
    *pdi = di;
    return TRUE;
}

static struct symt* dwarf2_parse_base_type(dwarf2_parse_context_t* ctx,
                                           dwarf2_debug_info_t* di)
{
    union attribute name;
    union attribute size;
    union attribute encoding;
    enum BasicType bt;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    dwarf2_find_name(ctx, di, &name, "base_type");
    if (!dwarf2_find_attribute(di, DW_AT_byte_size, &size)) size.uvalue = 0;
    if (!dwarf2_find_attribute(di, DW_AT_encoding, &encoding)) encoding.uvalue = DW_ATE_void;

    switch (encoding.uvalue)
    {
    case DW_ATE_void:           bt = btVoid; break;
    case DW_ATE_address:        bt = btULong; break;
    case DW_ATE_boolean:        bt = btBool; break;
    case DW_ATE_complex_float:  bt = btComplex; break;
    case DW_ATE_float:          bt = btFloat; break;
    case DW_ATE_signed:         bt = btInt; break;
    case DW_ATE_unsigned:       bt = btUInt; break;
    case DW_ATE_signed_char:    bt = btChar; break;
    case DW_ATE_unsigned_char:  bt = btChar; break;
    default:                    bt = btNoType; break;
    }
    di->symt = &symt_new_basic(ctx->module, bt, name.string, size.uvalue)->symt;
    if (di->abbrev->have_child) FIXME("Unsupported children\n");
    return di->symt;
}

static struct symt* dwarf2_parse_typedef(dwarf2_parse_context_t* ctx,
                                         dwarf2_debug_info_t* di)
{
    struct symt* ref_type;
    union attribute name;

    if (di->symt) return di->symt;

    TRACE("%s, for %lu\n", dwarf2_debug_ctx(ctx), di->abbrev->entry_code); 

    dwarf2_find_name(ctx, di, &name, "typedef");
    ref_type = dwarf2_lookup_type(ctx, di);

    if (name.string)
    {
        di->symt = &symt_new_typedef(ctx->module, ref_type, name.string)->symt;
    }
    if (di->abbrev->have_child) FIXME("Unsupported children\n");
    return di->symt;
}

static struct symt* dwarf2_parse_pointer_type(dwarf2_parse_context_t* ctx,
                                              dwarf2_debug_info_t* di)
{
    struct symt* ref_type;
    union attribute size;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    if (!dwarf2_find_attribute(di, DW_AT_byte_size, &size)) size.uvalue = 0;
    ref_type = dwarf2_lookup_type(ctx, di);

    di->symt = &symt_new_pointer(ctx->module, ref_type)->symt;
    if (di->abbrev->have_child) FIXME("Unsupported children\n");
    return di->symt;
}

static struct symt* dwarf2_parse_array_type(dwarf2_parse_context_t* ctx,
                                            dwarf2_debug_info_t* di)
{
    struct symt* ref_type;
    struct symt* idx_type = NULL;
    union attribute min, max, cnt;
    dwarf2_debug_info_t** pchild = NULL;
    dwarf2_debug_info_t* child;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    if (!di->abbrev->have_child)
    {
        FIXME("array without range information\n");
        return NULL;
    }
    ref_type = dwarf2_lookup_type(ctx, di);

    while ((pchild = vector_iter_up(&di->children, pchild)))
    {
        child = *pchild;
        switch (child->abbrev->tag)
        {
        case DW_TAG_subrange_type:
            idx_type = dwarf2_lookup_type(ctx, child);
            if (!dwarf2_find_attribute(child, DW_AT_lower_bound, &min))
                min.uvalue = 0;
            if (!dwarf2_find_attribute(child, DW_AT_upper_bound, &max))
                max.uvalue = 0;
            if (dwarf2_find_attribute(child, DW_AT_count, &cnt))
                max.uvalue = min.uvalue + cnt.uvalue;
            break;
        default:
            FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                  child->abbrev->tag, dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));
            break;
        }
    }
    di->symt = &symt_new_array(ctx->module, min.uvalue, max.uvalue, ref_type, idx_type)->symt;
    return di->symt;
}

static struct symt* dwarf2_parse_const_type(dwarf2_parse_context_t* ctx,
                                            dwarf2_debug_info_t* di)
{
    struct symt* ref_type;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    ref_type = dwarf2_lookup_type(ctx, di);
    if (di->abbrev->have_child) FIXME("Unsupported children\n");
    di->symt = ref_type;

    return ref_type;
}

static struct symt* dwarf2_parse_reference_type(dwarf2_parse_context_t* ctx,
                                                dwarf2_debug_info_t* di)
{
    struct symt* ref_type = NULL;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    ref_type = dwarf2_lookup_type(ctx, di);
    /* FIXME: for now, we hard-wire C++ references to pointers */
    di->symt = &symt_new_pointer(ctx->module, ref_type)->symt;

    if (di->abbrev->have_child) FIXME("Unsupported children\n");

    return di->symt;
}

static void dwarf2_parse_udt_member(dwarf2_parse_context_t* ctx,
                                    dwarf2_debug_info_t* di,
                                    struct symt_udt* parent)
{
    struct symt* elt_type;
    union attribute name;
    union attribute loc;
    unsigned long offset = 0;
    union attribute bit_size;
    union attribute bit_offset;

    assert(parent);

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    dwarf2_find_name(ctx, di, &name, "udt_member");
    elt_type = dwarf2_lookup_type(ctx, di);
    if (dwarf2_find_attribute(di, DW_AT_data_member_location, &loc))
    {
	TRACE("found member_location at %s\n", dwarf2_debug_ctx(ctx));
        offset = dwarf2_compute_location(ctx, loc.block, NULL);
        TRACE("found offset:%lu\n", offset); 		  
    }
    if (!dwarf2_find_attribute(di, DW_AT_bit_size, &bit_size))   bit_size.uvalue = 0;
    if (dwarf2_find_attribute(di, DW_AT_bit_offset, &bit_offset))
    {
        /* FIXME: we should only do this when implementation is LSB (which is
         * the case on i386 processors)
         */
        union attribute nbytes;
        if (!dwarf2_find_attribute(di, DW_AT_byte_size, &nbytes))
        {
            DWORD64     size;
            nbytes.uvalue = symt_get_info(elt_type, TI_GET_LENGTH, &size) ? (unsigned long)size : 0;
        }
        bit_offset.uvalue = nbytes.uvalue * 8 - bit_offset.uvalue - bit_size.uvalue;
    }
    else bit_offset.uvalue = 0;
    symt_add_udt_element(ctx->module, parent, name.string, elt_type,    
                         (offset << 3) + bit_offset.uvalue, bit_size.uvalue);

    if (di->abbrev->have_child) FIXME("Unsupported children\n");
}

static struct symt* dwarf2_parse_udt_type(dwarf2_parse_context_t* ctx,
                                          dwarf2_debug_info_t* di,
                                          enum UdtKind udt)
{
    union attribute name;
    union attribute size;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    dwarf2_find_name(ctx, di, &name, "udt");
    if (!dwarf2_find_attribute(di, DW_AT_byte_size, &size)) size.uvalue = 0;

    di->symt = &symt_new_udt(ctx->module, name.string, size.uvalue, udt)->symt;

    if (di->abbrev->have_child) /** any interest to not have child ? */
    {
        dwarf2_debug_info_t**    pchild = NULL;
        dwarf2_debug_info_t*    child;

        while ((pchild = vector_iter_up(&di->children, pchild)))
        {
            child = *pchild;

            switch (child->abbrev->tag)
            {
            case DW_TAG_member:
                /* FIXME: should I follow the sibling stuff ?? */
                dwarf2_parse_udt_member(ctx, child, (struct symt_udt*)di->symt);
                break;
            case DW_TAG_enumeration_type:
                dwarf2_parse_enumeration_type(ctx, child);
                break;
            case DW_TAG_structure_type:
            case DW_TAG_class_type:
            case DW_TAG_union_type:
                /* FIXME: we need to handle nested udt definitions */
                break;
            default:
                FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                      child->abbrev->tag, dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));
                break;
            }
        }
    }

    return di->symt;
}

static void dwarf2_parse_enumerator(dwarf2_parse_context_t* ctx,
                                    dwarf2_debug_info_t* di,
                                    struct symt_enum* parent)
{
    union attribute name;
    union attribute value;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    dwarf2_find_name(ctx, di, &name, "enum_value");
    if (!dwarf2_find_attribute(di, DW_AT_const_value, &value)) value.svalue = 0;
    symt_add_enum_element(ctx->module, parent, name.string, value.svalue);

    if (di->abbrev->have_child) FIXME("Unsupported children\n");
}

static struct symt* dwarf2_parse_enumeration_type(dwarf2_parse_context_t* ctx,
                                                  dwarf2_debug_info_t* di)
{
    union attribute name;
    union attribute size;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    dwarf2_find_name(ctx, di, &name, "enum");
    if (!dwarf2_find_attribute(di, DW_AT_byte_size, &size)) size.uvalue = 0;

    di->symt = &symt_new_enum(ctx->module, name.string)->symt;

    if (di->abbrev->have_child) /* any interest to not have child ? */
    {
        dwarf2_debug_info_t**   pchild = NULL;
        dwarf2_debug_info_t*    child;

        /* FIXME: should we use the sibling stuff ?? */
        while ((pchild = vector_iter_up(&di->children, pchild)))
        {
            child = *pchild;

            switch (child->abbrev->tag)
            {
            case DW_TAG_enumerator:
                dwarf2_parse_enumerator(ctx, child, (struct symt_enum*)di->symt);
                break;
            default:
                FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                      di->abbrev->tag, dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));
            }
	}
    }
    return di->symt;
}

static unsigned dwarf2_map_register(int regno)
{
    unsigned    reg;

    switch (regno)
    {
    case Wine_DW_no_register: FIXME("What the heck\n"); reg = 0; break;
    /* FIXME: this is a dirty hack */
    case Wine_DW_frame_register: reg = 0;          break;
    case  0: reg = CV_REG_EAX; break;
    case  1: reg = CV_REG_ECX; break;
    case  2: reg = CV_REG_EDX; break;
    case  3: reg = CV_REG_EBX; break;
    case  4: reg = CV_REG_ESP; break;
    case  5: reg = CV_REG_EBP; break;
    case  6: reg = CV_REG_ESI; break;
    case  7: reg = CV_REG_EDI; break;
    case  8: reg = CV_REG_EIP; break;
    case  9: reg = CV_REG_EFLAGS; break;
    case 10: reg = CV_REG_CS;  break;
    case 11: reg = CV_REG_SS;  break;
    case 12: reg = CV_REG_DS;  break;
    case 13: reg = CV_REG_ES;  break;
    case 14: reg = CV_REG_FS;  break;
    case 15: reg = CV_REG_GS;  break;
    case 16: case 17: case 18: case 19:
    case 20: case 21: case 22: case 23:
        reg = CV_REG_ST0 + regno - 16; break;
    case 24: reg = CV_REG_CTRL; break;
    case 25: reg = CV_REG_STAT; break;
    case 26: reg = CV_REG_TAG; break;
/*
reg: fiseg 27
reg: fioff 28
reg: foseg 29
reg: fooff 30
reg: fop   31
*/
    case 32: case 33: case 34: case 35:
    case 36: case 37: case 38: case 39:
        reg = CV_REG_XMM0 + regno - 32; break;
    case 40: reg = CV_REG_MXCSR; break;
    default:
        FIXME("Don't know how to map register %d\n", regno);
        return 0;
    }
    return reg;
}

/* structure used to pass information around when parsing a subprogram */
typedef struct dwarf2_subprogram_s
{
    dwarf2_parse_context_t*     ctx;
    struct symt_compiland*      compiland;
    struct symt_function*       func;
    long                        frame_offset;
    int                         frame_reg;
} dwarf2_subprogram_t;

/******************************************************************
 *		dwarf2_parse_variable
 *
 * Parses any variable (parameter, local/global variable)
 */
static void dwarf2_parse_variable(dwarf2_subprogram_t* subpgm,
                                  struct symt_block* block,
                                  dwarf2_debug_info_t* di)
{
    struct symt* param_type;
    union attribute loc;
    BOOL is_pmt = di->abbrev->tag == DW_TAG_formal_parameter;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(subpgm->ctx), dwarf2_debug_di(di));

    param_type = dwarf2_lookup_type(subpgm->ctx, di);
    if (dwarf2_find_attribute(di, DW_AT_location, &loc))
    {
        union attribute name;
        union attribute ext;
        long offset;
        int in_reg;

        dwarf2_find_name(subpgm->ctx, di, &name, "parameter");
        offset = dwarf2_compute_location(subpgm->ctx, loc.block, &in_reg);
	TRACE("found parameter %s/%ld (reg=%d) at %s\n",
              name.string, offset, in_reg, dwarf2_debug_ctx(subpgm->ctx));
        switch (in_reg)
        {
        case Wine_DW_no_register:
            /* it's a global variable */
            /* FIXME: we don't handle it's scope yet */
            if (!dwarf2_find_attribute(di, DW_AT_external, &ext))
                ext.uvalue = 0;
            symt_new_global_variable(subpgm->ctx->module, subpgm->compiland,
                                     name.string, !ext.uvalue,
                                     subpgm->ctx->module->module.BaseOfImage + offset,
                                     0, param_type);
            break;
        case Wine_DW_frame_register:
            in_reg = subpgm->frame_reg;
            offset += subpgm->frame_offset;
            /* fall through */
        default:
            /* either a pmt/variable relative to frame pointer or
             * pmt/variable in a register
             */
            symt_add_func_local(subpgm->ctx->module, subpgm->func, 
                                is_pmt ? DataIsParam : DataIsLocal,
                                dwarf2_map_register(in_reg), offset,
                                block, param_type, name.string);
            break;
        }
    }
    if (is_pmt && subpgm->func && subpgm->func->type)
        symt_add_function_signature_parameter(subpgm->ctx->module,
                                              (struct symt_function_signature*)subpgm->func->type,
                                              param_type);

    if (di->abbrev->have_child) FIXME("Unsupported children\n");
}

static void dwarf2_parse_subprogram_label(dwarf2_subprogram_t* subpgm,
                                          dwarf2_debug_info_t* di)
{
    union attribute name;
    union attribute low_pc;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(subpgm->ctx), dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(di, DW_AT_low_pc, &low_pc)) low_pc.uvalue = 0;
    dwarf2_find_name(subpgm->ctx, di, &name, "label");

    symt_add_function_point(subpgm->ctx->module, subpgm->func, SymTagLabel,
                            subpgm->ctx->module->module.BaseOfImage + low_pc.uvalue, name.string);
}

static void dwarf2_parse_subprogram_block(dwarf2_subprogram_t* subpgm,
                                          struct symt_block* block_parent,
					  dwarf2_debug_info_t* di);

static void dwarf2_parse_inlined_subroutine(dwarf2_subprogram_t* subpgm,
                                            dwarf2_debug_info_t* di)
{
    TRACE("%s, for %s\n", dwarf2_debug_ctx(subpgm->ctx), dwarf2_debug_di(di));

    /* FIXME: attributes to handle:
       DW_AT_low_pc:
       DW_AT_high_pc:
       DW_AT_name:
    */

    if (di->abbrev->have_child) /** any interest to not have child ? */
    {
        dwarf2_debug_info_t**   pchild = NULL;
        dwarf2_debug_info_t*    child;

        while ((pchild = vector_iter_up(&di->children, pchild)))
        {
            child = *pchild;

            switch (child->abbrev->tag)
            {
            case DW_TAG_formal_parameter:
                /* FIXME: this is not properly supported yet
                 * dwarf2_parse_subprogram_parameter(ctx, child, NULL);
                 */
                break;
            case DW_TAG_variable:
                /* FIXME:
                 * dwarf2_parse_variable(ctx, child);
                 */
                break;
            case DW_TAG_lexical_block:
                /* FIXME:
                   dwarf2_parse_subprogram_block(ctx, child, func);
                */
                break;
            case DW_TAG_inlined_subroutine:
                /* FIXME */
                dwarf2_parse_inlined_subroutine(subpgm, child);
                break;
            case DW_TAG_label:
                dwarf2_parse_subprogram_label(subpgm, child);
                break;
            default:
                FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                      child->abbrev->tag, dwarf2_debug_ctx(subpgm->ctx),
                      dwarf2_debug_di(di));
            }
        }
    }
}

static void dwarf2_parse_subprogram_block(dwarf2_subprogram_t* subpgm, 
                                          struct symt_block* parent_block,
					  dwarf2_debug_info_t* di)
{
    struct symt_block*  block;
    union attribute     low_pc;
    union attribute     high_pc;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(subpgm->ctx), dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(di, DW_AT_low_pc, &low_pc)) low_pc.uvalue = 0;
    if (!dwarf2_find_attribute(di, DW_AT_high_pc, &high_pc)) high_pc.uvalue = 0;

    block = symt_open_func_block(subpgm->ctx->module, subpgm->func, parent_block,
                                 low_pc.uvalue, high_pc.uvalue - low_pc.uvalue);

    if (di->abbrev->have_child) /** any interest to not have child ? */
    {
        dwarf2_debug_info_t**   pchild = NULL;
        dwarf2_debug_info_t*    child;

        while ((pchild = vector_iter_up(&di->children, pchild)))
        {
            child = *pchild;

            switch (child->abbrev->tag)
            {
            case DW_TAG_inlined_subroutine:
                dwarf2_parse_inlined_subroutine(subpgm, child);
                break;
            case DW_TAG_variable:
                dwarf2_parse_variable(subpgm, block, child);
                break;
            case DW_TAG_lexical_block:
                dwarf2_parse_subprogram_block(subpgm, block, child);
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
            case DW_TAG_class_type:
            case DW_TAG_structure_type:
            case DW_TAG_union_type:
            case DW_TAG_enumeration_type:
                /* the type referred to will be loaded when we need it, so skip it */
                break;
            default:
                FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                      child->abbrev->tag, dwarf2_debug_ctx(subpgm->ctx), dwarf2_debug_di(di));
            }
        }
    }

    symt_close_func_block(subpgm->ctx->module, subpgm->func, block, 0);
}

static struct symt* dwarf2_parse_subprogram(dwarf2_parse_context_t* ctx,
                                            dwarf2_debug_info_t* di,
                                            struct symt_compiland* compiland)
{
    union attribute name;
    union attribute low_pc;
    union attribute high_pc;
    union attribute is_decl;
    union attribute inline_flags;
    union attribute frame;
    struct symt* ret_type;
    struct symt_function_signature* sig_type;
    dwarf2_subprogram_t subpgm;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(di, DW_AT_low_pc, &low_pc)) low_pc.uvalue = 0;
    if (!dwarf2_find_attribute(di, DW_AT_high_pc, &high_pc)) high_pc.uvalue = 0;
    if (!dwarf2_find_attribute(di, DW_AT_declaration, &is_decl)) is_decl.uvalue = 0;
    if (!dwarf2_find_attribute(di, DW_AT_inline, &inline_flags)) inline_flags.uvalue = 0;
    dwarf2_find_name(ctx, di, &name, "subprogram");
    ret_type = dwarf2_lookup_type(ctx, di);

    /* FIXME: assuming C source code */
    sig_type = symt_new_function_signature(ctx->module, ret_type, CV_CALL_FAR_C);
    if (!is_decl.uvalue)
    {
        subpgm.func = symt_new_function(ctx->module, compiland, name.string,
                                        ctx->module->module.BaseOfImage + low_pc.uvalue,
                                        high_pc.uvalue - low_pc.uvalue,
                                        &sig_type->symt);
        di->symt = &subpgm.func->symt;
    }
    else subpgm.func = NULL;

    subpgm.ctx = ctx;
    subpgm.compiland = compiland;
    if (dwarf2_find_attribute(di, DW_AT_frame_base, &frame))
    {
        subpgm.frame_offset = dwarf2_compute_location(ctx, frame.block, &subpgm.frame_reg);
        TRACE("For %s got %ld/%d\n", name.string, subpgm.frame_offset, subpgm.frame_reg);
    }
    else /* on stack !! */
    {
        subpgm.frame_reg = 0;
        subpgm.frame_offset = 0;
    }

    if (di->abbrev->have_child) /** any interest to not have child ? */
    {
        dwarf2_debug_info_t**   pchild = NULL;
        dwarf2_debug_info_t*    child;

        while ((pchild = vector_iter_up(&di->children, pchild)))
        {
            child = *pchild;

            switch (child->abbrev->tag)
            {
            case DW_TAG_variable:
            case DW_TAG_formal_parameter:
                dwarf2_parse_variable(&subpgm, NULL, child);
                break;
            case DW_TAG_lexical_block:
                dwarf2_parse_subprogram_block(&subpgm, NULL, child);
                break;
            case DW_TAG_inlined_subroutine:
                dwarf2_parse_inlined_subroutine(&subpgm, child);
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
                /* FIXME: no support in dbghelp's internals so far */
                break;
            default:
                FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                      child->abbrev->tag, dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));
            }
	}
    }

    symt_normalize_function(subpgm.ctx->module, subpgm.func);

    return di->symt;
}

static void dwarf2_load_one_entry(dwarf2_parse_context_t* ctx,
                                  dwarf2_debug_info_t* di,
                                  struct symt_compiland* compiland)
{
    switch (di->abbrev->tag)
    {
    case DW_TAG_typedef:
        dwarf2_parse_typedef(ctx, di);
        break;
    case DW_TAG_base_type:
        dwarf2_parse_base_type(ctx, di);
        break;
    case DW_TAG_pointer_type:
        dwarf2_parse_pointer_type(ctx, di);
        break;
    case DW_TAG_class_type:
        dwarf2_parse_udt_type(ctx, di, UdtClass);
        break;
    case DW_TAG_structure_type:
        dwarf2_parse_udt_type(ctx, di, UdtStruct);
        break;
    case DW_TAG_union_type:
        dwarf2_parse_udt_type(ctx, di, UdtUnion);
        break;
    case DW_TAG_array_type:
        dwarf2_parse_array_type(ctx, di);
        break;
    case DW_TAG_const_type:
        dwarf2_parse_const_type(ctx, di);
        break;
    case DW_TAG_reference_type:
        dwarf2_parse_reference_type(ctx, di);
        break;
    case DW_TAG_enumeration_type:
        dwarf2_parse_enumeration_type(ctx, di);
        break;
    case DW_TAG_subprogram:
        dwarf2_parse_subprogram(ctx, di, compiland);
        break;
    default:
        WARN("Unhandled Tag type 0x%lx at %s, for %lu\n",
             di->abbrev->tag, dwarf2_debug_ctx(ctx), di->abbrev->entry_code); 
    }
}

static void dwarf2_set_line_number(struct module* module, unsigned long address,
                                   struct vector* v, unsigned file, unsigned line)
{
    struct symt_function*       func;
    int                         idx;
    unsigned*                   psrc;

    if (!file || !(psrc = vector_at(v, file - 1))) return;

    TRACE("%s %lx %s %u\n", module->module.ModuleName, address, source_get(module, *psrc), line);
    if ((idx = symt_find_nearest(module, address)) == -1 ||
        module->addr_sorttab[idx]->symt.tag != SymTagFunction) return;
    func = (struct symt_function*)module->addr_sorttab[idx];
    symt_add_func_line(module, func, *psrc, line, address - func->address);
}

static void dwarf2_parse_line_numbers(const dwarf2_section_t* sections,        
                                      dwarf2_parse_context_t* ctx,
                                      unsigned long offset)
{
    dwarf2_traverse_context_t   traverse;
    unsigned long               length;
    unsigned                    version, header_len, insn_size, default_stmt;
    unsigned                    line_range, opcode_base;
    int                         line_base;
    const unsigned char*        opcode_len;
    struct vector               dirs;
    struct vector               files;
    const char**                p;

    traverse.sections = sections;
    traverse.section = section_line;
    traverse.data = sections[section_line].address + offset;
    traverse.start_data = traverse.data;
    traverse.end_data = traverse.data + 4;
    traverse.offset = offset;
    traverse.word_size = ctx->word_size;

    length = dwarf2_parse_u4(&traverse);
    traverse.end_data = traverse.start_data + length;

    version = dwarf2_parse_u2(&traverse);
    header_len = dwarf2_parse_u4(&traverse);
    insn_size = dwarf2_parse_byte(&traverse);
    default_stmt = dwarf2_parse_byte(&traverse);
    line_base = (signed char)dwarf2_parse_byte(&traverse);
    line_range = dwarf2_parse_byte(&traverse);
    opcode_base = dwarf2_parse_byte(&traverse);

    opcode_len = traverse.data;
    traverse.data += opcode_base - 1;

    vector_init(&dirs, sizeof(const char*), 4);
    p = vector_add(&dirs, &ctx->pool);
    *p = ".";
    while (*traverse.data)
    {
        TRACE("Got include %s\n", (const char*)traverse.data);
        p = vector_add(&dirs, &ctx->pool);
        *p = (const char *)traverse.data;
        traverse.data += strlen((const char *)traverse.data) + 1;
    }
    traverse.data++;

    vector_init(&files, sizeof(unsigned), 16);
    while (*traverse.data)
    {
        unsigned int    dir_index, mod_time, length;
        const char*     name;
        const char*     dir;
        unsigned*       psrc;

        name = (const char*)traverse.data;
        traverse.data += strlen(name) + 1;
        dir_index = dwarf2_leb128_as_unsigned(&traverse);
        mod_time = dwarf2_leb128_as_unsigned(&traverse);
        length = dwarf2_leb128_as_unsigned(&traverse);
        dir = *(const char**)vector_at(&dirs, dir_index);
        TRACE("Got file %s/%s (%u,%u)\n", dir, name, mod_time, length);
        psrc = vector_add(&files, &ctx->pool);
        *psrc = source_new(ctx->module, dir, name);
    }
    traverse.data++;

    while (traverse.data < traverse.end_data)
    {
        unsigned long address = 0;
        unsigned file = 1;
        unsigned line = 1;
        unsigned is_stmt = default_stmt;
        BOOL basic_block = FALSE, end_sequence = FALSE;
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
                basic_block = TRUE;
                dwarf2_set_line_number(ctx->module, address, &files, file, line);
            }
            else
            {
                switch (opcode)
                {
                case DW_LNS_copy:
                    basic_block = FALSE;
                    dwarf2_set_line_number(ctx->module, address, &files, file, line);
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
                    basic_block = 1;
                    break;
                case DW_LNS_const_add_pc:
                    address += ((255 - opcode_base) / line_range) * insn_size;
                    break;
                case DW_LNS_fixed_advance_pc:
                    address += dwarf2_parse_u2(&traverse);
                    break;
                case DW_LNS_extended_op:
                    dwarf2_leb128_as_unsigned(&traverse);
                    extopcode = dwarf2_parse_byte(&traverse);
                    switch (extopcode)
                    {
                    case DW_LNE_end_sequence:
                        dwarf2_set_line_number(ctx->module, address, &files, file, line);
                        end_sequence = TRUE;
                        break;
                    case DW_LNE_set_address:
                        address = ctx->module->module.BaseOfImage + dwarf2_parse_addr(&traverse);
                        break;
                    case DW_LNE_define_file:
                        FIXME("not handled %s\n", traverse.data);
                        traverse.data += strlen((const char *)traverse.data) + 1;
                        dwarf2_leb128_as_unsigned(&traverse);
                        dwarf2_leb128_as_unsigned(&traverse);
                        dwarf2_leb128_as_unsigned(&traverse);
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
}

static BOOL dwarf2_parse_compilation_unit(const dwarf2_section_t* sections,
                                          const dwarf2_comp_unit_t* comp_unit,
                                          struct module* module,
                                          const unsigned char* comp_unit_cursor)
{
    dwarf2_parse_context_t ctx;
    dwarf2_traverse_context_t traverse;
    dwarf2_traverse_context_t abbrev_ctx;
    dwarf2_debug_info_t* di;
    BOOL ret = FALSE;

    TRACE("Compilation Unit Header found at 0x%x:\n",
          comp_unit_cursor - sections[section_debug].address);
    TRACE("- length:        %lu\n", comp_unit->length);
    TRACE("- version:       %u\n",  comp_unit->version);
    TRACE("- abbrev_offset: %lu\n", comp_unit->abbrev_offset);
    TRACE("- word_size:     %u\n",  comp_unit->word_size);

    if (comp_unit->version != 2)
    {
        WARN("%u DWARF version unsupported. Wine dbghelp only support DWARF 2.\n",
             comp_unit->version);
        return FALSE;
    }

    pool_init(&ctx.pool, 65536);
    ctx.module = module;
    ctx.word_size = comp_unit->word_size;

    traverse.sections = sections;
    traverse.section = section_debug;
    traverse.start_data = comp_unit_cursor + sizeof(dwarf2_comp_unit_stream_t);
    traverse.data = traverse.start_data;
    traverse.offset = comp_unit_cursor - sections[section_debug].address;
    traverse.word_size = comp_unit->word_size;
    traverse.end_data = comp_unit_cursor + comp_unit->length + sizeof(unsigned);

    abbrev_ctx.sections = sections;
    abbrev_ctx.section = section_abbrev;
    abbrev_ctx.start_data = sections[section_abbrev].address + comp_unit->abbrev_offset;
    abbrev_ctx.data = abbrev_ctx.start_data;
    abbrev_ctx.end_data = sections[section_abbrev].address + sections[section_abbrev].size;
    abbrev_ctx.offset = comp_unit->abbrev_offset;
    abbrev_ctx.word_size = comp_unit->word_size;
    dwarf2_parse_abbrev_set(&abbrev_ctx, &ctx.abbrev_table, &ctx.pool);

    sparse_array_init(&ctx.debug_info_table, sizeof(dwarf2_debug_info_t), 128);
    dwarf2_read_one_debug_info(&ctx, &traverse, &di);

    if (di->abbrev->tag == DW_TAG_compile_unit)
    {
        union attribute             name;
        dwarf2_debug_info_t**       pdi = NULL;
        union attribute             stmt_list;

        TRACE("beginning at 0x%lx, for %lu\n", di->offset, di->abbrev->entry_code); 

        dwarf2_find_name(&ctx, di, &name, "compiland");
        di->symt = &symt_new_compiland(module, source_new(module, NULL, name.string))->symt;

        if (di->abbrev->have_child)
        {
            while ((pdi = vector_iter_up(&di->children, pdi)))
            {
                dwarf2_load_one_entry(&ctx, *pdi, (struct symt_compiland*)di->symt);
            }
        }
        if (dwarf2_find_attribute(di, DW_AT_stmt_list, &stmt_list))
        {
            dwarf2_parse_line_numbers(sections, &ctx, stmt_list.uvalue);
        }
        ret = TRUE;
    }
    else FIXME("Should have a compilation unit here\n");
    pool_destroy(&ctx.pool);
    return ret;
}

BOOL dwarf2_parse(struct module* module, unsigned long load_offset,
		  const unsigned char* debug, unsigned int debug_size,
		  const unsigned char* abbrev, unsigned int abbrev_size,
		  const unsigned char* str, unsigned int str_size,
		  const unsigned char* line, unsigned int line_size)
{
    dwarf2_section_t    section[section_max];
    const unsigned char*comp_unit_cursor = debug;
    const unsigned char*end_debug = debug + debug_size;

    section[section_debug].address = debug;
    section[section_debug].size = debug_size;
    section[section_abbrev].address = abbrev;
    section[section_abbrev].size = abbrev_size;
    section[section_string].address = str;
    section[section_string].size = str_size;
    section[section_line].address = line;
    section[section_line].size = line_size;

    while (comp_unit_cursor < end_debug)
    {
        const dwarf2_comp_unit_stream_t* comp_unit_stream;
        dwarf2_comp_unit_t comp_unit;
    
        comp_unit_stream = (const dwarf2_comp_unit_stream_t*) comp_unit_cursor;
        comp_unit.length = *(unsigned long*)  comp_unit_stream->length;
        comp_unit.version = *(unsigned short*) comp_unit_stream->version;
        comp_unit.abbrev_offset = *(unsigned long*) comp_unit_stream->abbrev_offset;
        comp_unit.word_size = *(unsigned char*) comp_unit_stream->word_size;

        dwarf2_parse_compilation_unit(section, &comp_unit, module, comp_unit_cursor);
        comp_unit_cursor += comp_unit.length + sizeof(unsigned);
    }
    module->module.SymType = SymDia;
    return TRUE;
}
