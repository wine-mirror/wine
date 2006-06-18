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
 *      o local variables
 *      o unspecified parameters
 *      o inlined functions
 *      o line numbers
 * - Udt
 *      o proper types loading (addresses, bitfield, nesting)
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

typedef struct dwarf2_parse_context_s {
  struct pool pool;
  struct module* module;
  struct sparse_array abbrev_table;
  struct sparse_array debug_info_table;
  const unsigned char* data_stream;
  const unsigned char* data;
  const unsigned char* start_data;
  const unsigned char* end_data;
  const unsigned char* str_section;
  unsigned long offset;
  unsigned char word_size;
} dwarf2_parse_context_t;

/* forward declarations */
static struct symt* dwarf2_parse_enumeration_type(dwarf2_parse_context_t* ctx, dwarf2_debug_info_t* entry);

static unsigned char dwarf2_parse_byte(dwarf2_parse_context_t* ctx)
{
  unsigned char uvalue = *(const unsigned char*) ctx->data;
  ctx->data += 1;
  return uvalue;
}

static unsigned short dwarf2_parse_u2(dwarf2_parse_context_t* ctx)
{
  unsigned short uvalue = *(const unsigned short*) ctx->data;
  ctx->data += 2;
  return uvalue;
}

static unsigned long dwarf2_parse_u4(dwarf2_parse_context_t* ctx)
{
  unsigned long uvalue = *(const unsigned int*) ctx->data;
  ctx->data += 4;
  return uvalue;
}

static unsigned long dwarf2_leb128_as_unsigned(dwarf2_parse_context_t* ctx)
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

static long dwarf2_leb128_as_signed(dwarf2_parse_context_t* ctx)
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

static unsigned long dwarf2_parse_addr(dwarf2_parse_context_t* ctx)
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

static const char* dwarf2_debug_ctx(const dwarf2_parse_context_t* ctx) 
{
  /*return wine_dbg_sprintf("ctx(0x%x,%u)", ctx->data - ctx->start_data, ctx->level); */
  return wine_dbg_sprintf("ctx(0x%x)", ctx->data - ctx->data_stream); 
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

static void dwarf2_parse_abbrev_set(dwarf2_parse_context_t* abbrev_ctx, 
                                    struct sparse_array* abbrev_table,
                                    struct pool* pool)
{
    unsigned long entry_code;
    dwarf2_abbrev_entry_t* abbrev_entry;
    dwarf2_abbrev_entry_attr_t* new = NULL;
    dwarf2_abbrev_entry_attr_t* last = NULL;
    unsigned long attribute;
    unsigned long form;

    TRACE("%s, end at %p\n", dwarf2_debug_ctx(abbrev_ctx), abbrev_ctx->end_data); 

    assert( NULL != abbrev_ctx );

    sparse_array_init(abbrev_table, sizeof(dwarf2_abbrev_entry_t), 32);
    while (abbrev_ctx->data < abbrev_ctx->end_data)
    {
        TRACE("now at %s\n", dwarf2_debug_ctx(abbrev_ctx)); 
        entry_code = dwarf2_leb128_as_unsigned(abbrev_ctx);
        TRACE("found entry_code %lu\n", entry_code);
        if (!entry_code)
        {
            TRACE("NULL entry code at %s\n", dwarf2_debug_ctx(abbrev_ctx)); 
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

static void dwarf2_parse_attr_into_di(dwarf2_parse_context_t* ctx,
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
            attr->string = (const char*)ctx->str_section + offset;
        }
        TRACE("strp<%s>\n", attr->string);
        break;
    case DW_FORM_block:
        attr->block = pool_alloc(&ctx->pool, sizeof(struct dwarf2_block));
        attr->block->size = dwarf2_leb128_as_unsigned(ctx);
        attr->block->ptr  = ctx->data;
        ctx->data += attr->block->size;
        break;

    case DW_FORM_block1:
        attr->block = pool_alloc(&ctx->pool, sizeof(struct dwarf2_block));
        attr->block->size = dwarf2_parse_byte(ctx);
        attr->block->ptr  = ctx->data;
        ctx->data += attr->block->size;
        break;

    case DW_FORM_block2:
        attr->block = pool_alloc(&ctx->pool, sizeof(struct dwarf2_block));
        attr->block->size = dwarf2_parse_u2(ctx);
        attr->block->ptr  = ctx->data;
        ctx->data += attr->block->size;
        break;

    case DW_FORM_block4:
        attr->block = pool_alloc(&ctx->pool, sizeof(struct dwarf2_block));
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
        /* FIXME: would require better definition of contexts */
        dwarf2_parse_context_t  lctx;
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
            case DW_OP_const1s: loc[++stk] = (long)(char)dwarf2_parse_byte(&lctx); break;
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

    offset = ctx->data - ctx->data_stream;
    entry_code = dwarf2_leb128_as_unsigned(ctx);
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
            dwarf2_parse_attr_into_di(ctx, attr, &di->attributes[i]);
        }
    }
    else di->attributes = NULL;
    if (abbrev->have_child)
    {
        vector_init(&di->children, sizeof(dwarf2_debug_info_t*), 16);
        while (ctx->data < ctx->end_data)
        {
            if (!dwarf2_read_one_debug_info(ctx, &child)) return FALSE;
            if (!child) break;
            where = vector_add(&di->children, &ctx->pool);
            if (!where) return FALSE;
            *where = child;
        }
    }
    if (dwarf2_find_attribute(di, DW_AT_sibling, &sibling) &&
        ctx->data != ctx->data_stream + sibling.uvalue)
    {
        WARN("setting cursor for %s to next sibling <0x%lx>\n",
             dwarf2_debug_ctx(ctx), sibling.uvalue);
        ctx->data = ctx->data_stream + sibling.uvalue;
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
    
    symt_add_udt_element(ctx->module, parent, name.string, elt_type, offset, 0);

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

static void dwarf2_parse_variable(dwarf2_parse_context_t* ctx,
				  dwarf2_debug_info_t* di)
{
    struct symt* var_type;
    union attribute name;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    var_type = dwarf2_lookup_type(ctx, di);
    dwarf2_find_name(ctx, di, &name, "variable");
    /* FIXME: still lots to do !!! */

    if (di->abbrev->have_child) FIXME("Unsupported children\n");
}

static void dwarf2_parse_subprogram_parameter(dwarf2_parse_context_t* ctx,
					      dwarf2_debug_info_t* di, 
					      struct symt_function_signature* sig_type)
{
    struct symt* param_type;
    union attribute name;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    param_type = dwarf2_lookup_type(ctx, di);
    dwarf2_find_name(ctx, di, &name, "parameter");
    /* DW_AT_location */
    if (sig_type) symt_add_function_signature_parameter(ctx->module, sig_type, param_type);
    /* FIXME: add variable to func_type */
    if (di->abbrev->have_child) FIXME("Unsupported children\n");
}

static void dwarf2_parse_subprogram_label(dwarf2_parse_context_t* ctx,
                                          struct symt_function* func,
                                          dwarf2_debug_info_t* di)
{
    union attribute name;
    union attribute low_pc;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(di, DW_AT_low_pc, &low_pc)) low_pc.uvalue = 0;
    dwarf2_find_name(ctx, di, &name, "label");

    symt_add_function_point(ctx->module, func, SymTagLabel,
                            ctx->module->module.BaseOfImage + low_pc.uvalue, name.string);
}

static void dwarf2_parse_subprogram_block(dwarf2_parse_context_t* ctx, 
					  dwarf2_debug_info_t* di, 
					  struct symt_function* func);

static void dwarf2_parse_inlined_subroutine(dwarf2_parse_context_t* ctx,
                                            dwarf2_debug_info_t* di)
{
    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

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
                dwarf2_parse_variable(ctx, child);
                break;
            case DW_TAG_lexical_block:
                /* FIXME:
                   dwarf2_parse_subprogram_block(ctx, child, func);
                */
                break;
            case DW_TAG_inlined_subroutine:
                /* FIXME */
                dwarf2_parse_inlined_subroutine(ctx, child);
                break;
            case DW_TAG_label:
                /* FIXME
                 * dwarf2_parse_subprogram_label(ctx, func, child);
                 */
                break;
            default:
                FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                      child->abbrev->tag, dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));
            }
        }
    }
}

static void dwarf2_parse_subprogram_block(dwarf2_parse_context_t* ctx, 
					  dwarf2_debug_info_t* di, 
					  struct symt_function* func)
{
    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    /* if (!dwarf2_find_attribute(di, DW_AT_ranges, ??)): what to do ? */

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
                dwarf2_parse_inlined_subroutine(ctx, child);
                break;
            case DW_TAG_variable:
                dwarf2_parse_variable(ctx, child);
                break;
            case DW_TAG_lexical_block:
                dwarf2_parse_subprogram_block(ctx, child, func);
                break;
            case DW_TAG_subprogram:
                /* FIXME: likely a declaration (to be checked)
                 * skip it for now
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
                      child->abbrev->tag, dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));
            }
        }
    }
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
    struct symt* ret_type;
    struct symt_function* func = NULL;
    struct symt_function_signature* sig_type;

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
        func = symt_new_function(ctx->module, compiland, name.string,
                                 low_pc.uvalue, high_pc.uvalue - low_pc.uvalue,
                                 &sig_type->symt);
        di->symt = &func->symt;
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
            case DW_TAG_formal_parameter:
                dwarf2_parse_subprogram_parameter(ctx, child, sig_type);
                break;
            case DW_TAG_lexical_block:
                dwarf2_parse_subprogram_block(ctx, child, func);
                break;
            case DW_TAG_variable:
                dwarf2_parse_variable(ctx, child);
                break;
            case DW_TAG_inlined_subroutine:
                dwarf2_parse_inlined_subroutine(ctx, child);
                break;
            case DW_TAG_subprogram:
                /* FIXME: likely a declaration (to be checked)
                 * skip it for now
                 */
                break;
            case DW_TAG_label:
                dwarf2_parse_subprogram_label(ctx, func, child);
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

    symt_normalize_function(ctx->module, func);

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

BOOL dwarf2_parse(struct module* module, unsigned long load_offset,
		  const unsigned char* debug, unsigned int debug_size,
		  const unsigned char* abbrev, unsigned int abbrev_size,
		  const unsigned char* str, unsigned int str_sz)
{
  const unsigned char* comp_unit_cursor = debug;
  const unsigned char* end_debug = debug + debug_size;

  while (comp_unit_cursor < end_debug) {
    const dwarf2_comp_unit_stream_t* comp_unit_stream;
    dwarf2_comp_unit_t comp_unit;
    dwarf2_parse_context_t ctx;
    dwarf2_parse_context_t abbrev_ctx;
    dwarf2_debug_info_t* di;
    
    comp_unit_stream = (const dwarf2_comp_unit_stream_t*) comp_unit_cursor;

    comp_unit.length = *(unsigned long*)  comp_unit_stream->length;
    comp_unit.version = *(unsigned short*) comp_unit_stream->version;
    comp_unit.abbrev_offset = *(unsigned long*) comp_unit_stream->abbrev_offset;
    comp_unit.word_size = *(unsigned char*) comp_unit_stream->word_size;

    TRACE("Compilation Unit Herder found at 0x%x:\n", comp_unit_cursor - debug);
    TRACE("- length:        %lu\n", comp_unit.length);
    TRACE("- version:       %u\n",  comp_unit.version);
    TRACE("- abbrev_offset: %lu\n", comp_unit.abbrev_offset);
    TRACE("- word_size:     %u\n",  comp_unit.word_size);

    pool_init(&ctx.pool, 65536);
    ctx.module = module;
    ctx.data_stream = debug;
    ctx.data = ctx.start_data = comp_unit_cursor + sizeof(dwarf2_comp_unit_stream_t);
    ctx.offset = comp_unit_cursor - debug;
    ctx.word_size = comp_unit.word_size;
    ctx.str_section = str;

    comp_unit_cursor += comp_unit.length + sizeof(unsigned);
    ctx.end_data = comp_unit_cursor;

    if (2 != comp_unit.version) {
      WARN("%u DWARF version unsupported. Wine dbghelp only support DWARF 2.\n", comp_unit.version);
      continue ;
    }

    abbrev_ctx.data_stream = abbrev;
    abbrev_ctx.data = abbrev_ctx.start_data = abbrev + comp_unit.abbrev_offset;
    abbrev_ctx.end_data = abbrev + abbrev_size;
    abbrev_ctx.offset = comp_unit.abbrev_offset;
    abbrev_ctx.str_section = str;
    dwarf2_parse_abbrev_set(&abbrev_ctx, &ctx.abbrev_table, &ctx.pool);

    sparse_array_init(&ctx.debug_info_table, sizeof(dwarf2_debug_info_t), 128);
    dwarf2_read_one_debug_info(&ctx, &di);
    ctx.data = ctx.start_data; /* FIXME */

    if (di->abbrev->tag == DW_TAG_compile_unit)
    {
        union attribute             name;
        dwarf2_debug_info_t**       pdi = NULL;

        TRACE("beginning at 0x%lx, for %lu\n", di->offset, di->abbrev->entry_code); 

        dwarf2_find_name(&ctx, di, &name, "compiland");
        di->symt = &symt_new_compiland(module, name.string)->symt;

        if (di->abbrev->have_child)
        {
            while ((pdi = vector_iter_up(&di->children, pdi)))
            {
                dwarf2_load_one_entry(&ctx, *pdi, (struct symt_compiland*)di->symt);
            }
        }
    }
    else FIXME("Should have a compilation unit here\n");
    pool_destroy(&ctx.pool);
  }
  
  module->module.SymType = SymDia;
  return TRUE;
}
