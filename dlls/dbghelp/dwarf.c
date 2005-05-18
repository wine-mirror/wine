/*
 * File dwarf.c - read dwarf2 information from the ELF modules
 *
 * Copyright (C) 2005, Raphael Junqueira
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

typedef struct {
  unsigned char length[4];
  unsigned char version[2];
  unsigned char abbrev_offset[4];
  unsigned char word_size[1];
} dwarf2_comp_unit_stream_t;

typedef struct {
  unsigned long  length;
  unsigned short version;
  unsigned long  abbrev_offset;
  unsigned char  word_size;
} dwarf2_comp_unit_t;

typedef struct {
  unsigned int   length;
  unsigned short version;
  unsigned int   prologue_length;
  unsigned char  min_insn_length;
  unsigned char  default_is_stmt;
  int            line_base;
  unsigned char  line_range;
  unsigned char  opcode_base;
} dwarf2_line_info_t;

typedef enum dwarf_tag_e {
  DW_TAG_padding                = 0x00,
  DW_TAG_array_type             = 0x01,
  DW_TAG_class_type             = 0x02,
  DW_TAG_entry_point            = 0x03,
  DW_TAG_enumeration_type       = 0x04,
  DW_TAG_formal_parameter       = 0x05,
  DW_TAG_imported_declaration   = 0x08,
  DW_TAG_label                  = 0x0a,
  DW_TAG_lexical_block          = 0x0b,
  DW_TAG_member                 = 0x0d,
  DW_TAG_pointer_type           = 0x0f,
  DW_TAG_reference_type         = 0x10,
  DW_TAG_compile_unit           = 0x11,
  DW_TAG_string_type            = 0x12,
  DW_TAG_structure_type         = 0x13,
  DW_TAG_subroutine_type        = 0x15,
  DW_TAG_typedef                = 0x16,
  DW_TAG_union_type             = 0x17,
  DW_TAG_unspecified_parameters = 0x18,
  DW_TAG_variant                = 0x19,
  DW_TAG_common_block           = 0x1a,
  DW_TAG_common_inclusion       = 0x1b,
  DW_TAG_inheritance            = 0x1c,
  DW_TAG_inlined_subroutine     = 0x1d,
  DW_TAG_module                 = 0x1e,
  DW_TAG_ptr_to_member_type     = 0x1f,
  DW_TAG_set_type               = 0x20,
  DW_TAG_subrange_type          = 0x21,
  DW_TAG_with_stmt              = 0x22,
  DW_TAG_access_declaration     = 0x23,
  DW_TAG_base_type              = 0x24,
  DW_TAG_catch_block            = 0x25,
  DW_TAG_const_type             = 0x26,
  DW_TAG_constant               = 0x27,
  DW_TAG_enumerator             = 0x28,
  DW_TAG_file_type              = 0x29,
  DW_TAG_friend                 = 0x2a,
  DW_TAG_namelist               = 0x2b,
  DW_TAG_namelist_item          = 0x2c,
  DW_TAG_packed_type            = 0x2d,
  DW_TAG_subprogram             = 0x2e,
  DW_TAG_template_type_param    = 0x2f,
  DW_TAG_template_value_param   = 0x30,
  DW_TAG_thrown_type            = 0x31,
  DW_TAG_try_block              = 0x32,
  DW_TAG_variant_part           = 0x33,
  DW_TAG_variable               = 0x34,
  DW_TAG_volatile_type          = 0x35,
  /** extensions */
  DW_TAG_MIPS_loop              = 0x4081,
  DW_TAG_format_label           = 0x4101,
  DW_TAG_function_template      = 0x4102,
  DW_TAG_class_template         = 0x4103
} dwarf_tag_t;

typedef enum dwarf_attribute_e {
  DW_AT_sibling              = 0x01,
  DW_AT_location             = 0x02,
  DW_AT_name                 = 0x03,
  DW_AT_ordering             = 0x09,
  DW_AT_subscr_data          = 0x0a,
  DW_AT_byte_size            = 0x0b,
  DW_AT_bit_offset           = 0x0c,
  DW_AT_bit_size             = 0x0d,
  DW_AT_element_list         = 0x0f,
  DW_AT_stmt_list            = 0x10,
  DW_AT_low_pc               = 0x11,
  DW_AT_high_pc              = 0x12,
  DW_AT_language             = 0x13,
  DW_AT_member               = 0x14,
  DW_AT_discr                = 0x15,
  DW_AT_discr_value          = 0x16,
  DW_AT_visibility           = 0x17,
  DW_AT_import               = 0x18,
  DW_AT_string_length        = 0x19,
  DW_AT_common_reference     = 0x1a,
  DW_AT_comp_dir             = 0x1b,
  DW_AT_const_value          = 0x1c,
  DW_AT_containing_type      = 0x1d,
  DW_AT_default_value        = 0x1e,
  DW_AT_inline               = 0x20,
  DW_AT_is_optional          = 0x21,
  DW_AT_lower_bound          = 0x22,
  DW_AT_producer             = 0x25,
  DW_AT_prototyped           = 0x27,
  DW_AT_return_addr          = 0x2a,
  DW_AT_start_scope          = 0x2c,
  DW_AT_stride_size          = 0x2e,
  DW_AT_upper_bound          = 0x2f,
  DW_AT_abstract_origin      = 0x31,
  DW_AT_accessibility        = 0x32,
  DW_AT_address_class        = 0x33,
  DW_AT_artificial           = 0x34,
  DW_AT_base_types           = 0x35,
  DW_AT_calling_convention   = 0x36,
  DW_AT_count                = 0x37,
  DW_AT_data_member_location = 0x38,
  DW_AT_decl_column          = 0x39,
  DW_AT_decl_file            = 0x3a,
  DW_AT_decl_line            = 0x3b,
  DW_AT_declaration          = 0x3c,
  DW_AT_discr_list           = 0x3d,
  DW_AT_encoding             = 0x3e,
  DW_AT_external             = 0x3f,
  DW_AT_frame_base           = 0x40,
  DW_AT_friend               = 0x41,
  DW_AT_identifier_case      = 0x42,
  DW_AT_macro_info           = 0x43,
  DW_AT_namelist_items       = 0x44,
  DW_AT_priority             = 0x45,
  DW_AT_segment              = 0x46,
  DW_AT_specification        = 0x47,
  DW_AT_static_link          = 0x48,
  DW_AT_type                 = 0x49,
  DW_AT_use_location         = 0x4a,
  DW_AT_variable_parameter   = 0x4b,
  DW_AT_virtuality           = 0x4c,
  DW_AT_vtable_elem_location = 0x4d,
  /* extensions */
  DW_AT_MIPS_fde                     = 0x2001,
  DW_AT_MIPS_loop_begin              = 0x2002,
  DW_AT_MIPS_tail_loop_begin         = 0x2003,
  DW_AT_MIPS_epilog_begin            = 0x2004,
  DW_AT_MIPS_loop_unroll_factor      = 0x2005,
  DW_AT_MIPS_software_pipeline_depth = 0x2006,
  DW_AT_MIPS_linkage_name            = 0x2007,
  DW_AT_MIPS_stride                  = 0x2008,
  DW_AT_MIPS_abstract_name           = 0x2009,
  DW_AT_MIPS_clone_origin            = 0x200a,
  DW_AT_MIPS_has_inlines             = 0x200b,
  DW_AT_sf_names                     = 0x2101,
  DW_AT_src_info                     = 0x2102,
  DW_AT_mac_info                     = 0x2103,
  DW_AT_src_coords                   = 0x2104,
  DW_AT_body_begin                   = 0x2105,
  DW_AT_body_end                     = 0x2106
} dwarf_attribute_t;

typedef enum dwarf_form_e {
  DW_FORM_addr      = 0x01,
  DW_FORM_block2    = 0x03,
  DW_FORM_block4    = 0x04,
  DW_FORM_data2     = 0x05,
  DW_FORM_data4     = 0x06,
  DW_FORM_data8     = 0x07,
  DW_FORM_string    = 0x08,
  DW_FORM_block     = 0x09,
  DW_FORM_block1    = 0x0a,
  DW_FORM_data1     = 0x0b,
  DW_FORM_flag      = 0x0c,
  DW_FORM_sdata     = 0x0d,
  DW_FORM_strp      = 0x0e,
  DW_FORM_udata     = 0x0f,
  DW_FORM_ref_addr  = 0x10,
  DW_FORM_ref1      = 0x11,
  DW_FORM_ref2      = 0x12,
  DW_FORM_ref4      = 0x13,
  DW_FORM_ref8      = 0x14,
  DW_FORM_ref_udata = 0x15,
  DW_FORM_indirect  = 0x16
} dwarf_form_t;

/**
 * Parsers
 */

typedef struct dwarf2_parse_context_s {
  const unsigned char* data;
  const unsigned char* end_data;
  unsigned long offset;
  unsigned char word_size;
} dwarf2_parse_context_t;

static unsigned long dwarf2_leb128_as_unsigned(dwarf2_parse_context_t* ctx)
{
  unsigned long ret = 0;
  unsigned char byte;
  unsigned shift = 0;

  assert( NULL != ctx );

  while (1) {
    byte = *(ctx->data);
    ctx->data++;
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
    byte = *(ctx->data);
    ctx->data++;
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

typedef struct dwarf2_abbrev_entry_attr_s {
  unsigned long attribute;
  unsigned long form;
  struct dwarf2_abbrev_entry_attr_s* next;
} dwarf2_abbrev_entry_attr_t;

typedef struct dwarf2_abbrev_entry_s {
  unsigned long entry_code;
  unsigned long tag;
  unsigned char have_child;
  dwarf2_abbrev_entry_attr_t* attrs;
  struct dwarf2_abbrev_entry_s* next;
} dwarf2_abbrev_entry_t;

typedef struct dwarf2_abbrev_table_s {
  dwarf2_abbrev_entry_t* first;
  unsigned n_entries;
} dwarf2_abbrev_table_t;

dwarf2_abbrev_entry_attr_t* dwarf2_abbrev_entry_add_attr(dwarf2_abbrev_entry_t* abbrev_entry, unsigned long attribute, unsigned long form)
{
  dwarf2_abbrev_entry_attr_t* ret = NULL;
  dwarf2_abbrev_entry_attr_t* it = NULL;

  assert( NULL != abbrev_entry );
  ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(dwarf2_abbrev_entry_attr_t));
  assert( NULL != ret );

  ret->attribute = attribute;
  ret->form      = form;

  ret->next = NULL;
  if (NULL == abbrev_entry->attrs) {
    abbrev_entry->attrs = ret;
  } else {
    for (it = abbrev_entry->attrs; NULL != it->next; it = it->next) ;
    it->next = ret;
  }
  return ret;
}

dwarf2_abbrev_entry_t* dwarf2_abbrev_table_add_entry(dwarf2_abbrev_table_t* abbrev_table, unsigned long entry_code, unsigned long tag, unsigned char have_child)
{
  dwarf2_abbrev_entry_t* ret = NULL;

  assert( NULL != abbrev_table );
  ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(dwarf2_abbrev_entry_t));
  assert( NULL != ret );

  TRACE("(%p,%u) entry_code(%lu) tag(0x%lx) have_child(%u) -> %p\n", abbrev_table, abbrev_table->n_entries, entry_code, tag, have_child, ret);

  ret->entry_code = entry_code;
  ret->tag        = tag;
  ret->have_child = have_child;
  ret->attrs      = NULL;

  ret->next       = abbrev_table->first;
  abbrev_table->first = ret;
  abbrev_table->n_entries++;
  return ret;
}

dwarf2_abbrev_entry_t* dwarf2_abbrev_table_find_entry(dwarf2_abbrev_table_t* abbrev_table, unsigned long entry_code)
{
  dwarf2_abbrev_entry_t* ret = NULL;

  assert( NULL != abbrev_table );
  for (ret = abbrev_table->first; ret; ret = ret->next) {
    if (ret->entry_code == entry_code) { break ; }
  }
  return ret;
}

void dwarf2_abbrev_table_free(dwarf2_abbrev_table_t* abbrev_table)
{
  dwarf2_abbrev_entry_t* entry = NULL;
  dwarf2_abbrev_entry_t* next_entry = NULL;
  assert( NULL != abbrev_table );
  for (entry = abbrev_table->first; NULL != entry; entry = next_entry) {
    dwarf2_abbrev_entry_attr_t* attr = NULL;
    dwarf2_abbrev_entry_attr_t* next_attr = NULL;
    for (attr = entry->attrs; NULL != attr; attr = next_attr) {
      next_attr = attr->next;
      HeapFree(GetProcessHeap(), 0, attr);
    }
    next_entry = entry->next;
    HeapFree(GetProcessHeap(), 0, entry);
  }
  abbrev_table->first = NULL;
  abbrev_table->n_entries = 0;
}

dwarf2_abbrev_table_t* dwarf2_parse_abbrev_set(dwarf2_parse_context_t* abbrev_ctx)
{
  dwarf2_abbrev_table_t* abbrev_table = NULL;

  TRACE("beginning at %p, end at %p\n",  abbrev_ctx->data, abbrev_ctx->end_data);

  assert( NULL != abbrev_ctx );
  abbrev_table = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(dwarf2_abbrev_table_t));
  assert( NULL != abbrev_table );

  while (abbrev_ctx->data < abbrev_ctx->end_data) {
    unsigned long entry_code;
    unsigned long tag;
    unsigned char have_child;
    dwarf2_abbrev_entry_t* abbrev_entry;

    TRACE("now at %p\n",  abbrev_ctx->data);
    entry_code = dwarf2_leb128_as_unsigned(abbrev_ctx);
    TRACE("found entry_code %lu\n", entry_code);
    if (0 == entry_code) {
      TRACE("NULL entry code at %p\n", abbrev_ctx->data);
      break ;
    }
    tag = dwarf2_leb128_as_unsigned(abbrev_ctx);
    have_child = *(abbrev_ctx->data);
    abbrev_ctx->data++;

    abbrev_entry = dwarf2_abbrev_table_add_entry(abbrev_table, entry_code, tag, have_child);
    assert( NULL != abbrev_entry );
    while (1) {
      unsigned long attribute;
      unsigned long form;
      attribute = dwarf2_leb128_as_unsigned(abbrev_ctx);
      form = dwarf2_leb128_as_unsigned(abbrev_ctx);
      if (0 == attribute) break;
      dwarf2_abbrev_entry_add_attr(abbrev_entry, attribute, form);
    }
  }

  TRACE("found %u entries\n", abbrev_table->n_entries);

  return abbrev_table;
}

static const char* dwarf2_parse_attr_as_string(dwarf2_abbrev_entry_attr_t* attr,
					       dwarf2_parse_context_t* ctx)
{
  const char* ret = (const char*) ctx->data;
  ctx->data += strlen(ret) + 1;
  return ret;
}

static void dwarf2_parse_attr(dwarf2_abbrev_entry_attr_t* attr,
			      dwarf2_parse_context_t* ctx)
{
  const unsigned long attribute = attr->attribute;
  const unsigned long form = attr->form;
  unsigned long  uvalue = 0;

  TRACE("(attr:0x%lx,form:0x%lx)\n", attribute, form);

  switch (form) {
  case DW_FORM_ref_addr:
  case DW_FORM_addr:
    ctx->data += ctx->word_size;
    break;

  case DW_FORM_ref1:
  case DW_FORM_flag:
  case DW_FORM_data1:
    ctx->data++;
    break;

  case DW_FORM_ref2:
  case DW_FORM_data2:
    ctx->data += 2;
    break;

  case DW_FORM_ref4:
  case DW_FORM_data4:
    ctx->data += 4;
    break;

  case DW_FORM_ref8:
  case DW_FORM_data8:
    ctx->data += 8;
    break;

  case DW_FORM_sdata:
    uvalue = dwarf2_leb128_as_signed(ctx);
    break;

  case DW_FORM_ref_udata:
  case DW_FORM_udata:
    uvalue = dwarf2_leb128_as_unsigned(ctx);
    break;

  case DW_FORM_string:
    dwarf2_parse_attr_as_string(attr, ctx);
    break;
  case DW_FORM_strp:
    FIXME("Unsupported indirect string format (in .debug_str)\n");
    ctx->data += 4;
    break;

  case DW_FORM_block:
    uvalue = dwarf2_leb128_as_unsigned(ctx);
    ctx->data += uvalue;
    break;

  case DW_FORM_block1:
    uvalue = *(unsigned char*) ctx->data;
    ctx->data += 1 + uvalue;
    break;

  case DW_FORM_block2:
    uvalue = *(unsigned short*) ctx->data;
    ctx->data += 2 + uvalue;
    break;

  case DW_FORM_block4:
    uvalue = *(unsigned int*) ctx->data;
    ctx->data += 4 + uvalue;
    break;

  default:
    break;
  }
  switch (form) {
  }

}


static void dwarf2_parse_compiland(struct module* module,
				   dwarf2_abbrev_entry_t* entry,
				   dwarf2_parse_context_t* ctx)
{
  struct symt_compiland* compiland = NULL;
  const char* name = NULL;
  dwarf2_abbrev_entry_attr_t* attr = NULL;

  TRACE("beginning at %p, for %lu\n", ctx->data, entry->entry_code);

  for (attr = entry->attrs; NULL != attr; attr = attr->next) {
    /*
     *TRACE("at %p\n", ctx->data);
     *dump(ctx->data, 64);
     */
    switch (attr->attribute) {
    case DW_AT_name:
      name = dwarf2_parse_attr_as_string(attr, ctx);
      TRACE("found name %s\n", name);
      break;
    default:
      dwarf2_parse_attr(attr, ctx);
    }
    if (NULL != name) {
      compiland = symt_new_compiland(module, name);
    }
  }
}

BOOL dwarf2_parse(struct module* module, unsigned long load_offset,
		  const unsigned char* debug, unsigned int debug_size,
		  const unsigned char* abbrev, unsigned int abbrev_size,
		  const unsigned char* str, unsigned int str_sz)
{
  const unsigned char* comp_unit_cursor = debug;
  const unsigned char* end_debug = debug + debug_size;
  BOOL bRet = TRUE;

  while (comp_unit_cursor < end_debug) {
    dwarf2_abbrev_table_t* abbrev_table;
    dwarf2_comp_unit_stream_t* comp_unit_stream;
    dwarf2_comp_unit_t comp_unit;
    dwarf2_parse_context_t ctx;
    dwarf2_parse_context_t abbrev_ctx;

    comp_unit_stream = (dwarf2_comp_unit_stream_t*) comp_unit_cursor;

    comp_unit.length = *(unsigned long*)  comp_unit_stream->length;
    comp_unit.version = *(unsigned short*) comp_unit_stream->version;
    comp_unit.abbrev_offset = *(unsigned long*) comp_unit_stream->abbrev_offset;
    comp_unit.word_size = *(unsigned char*) comp_unit_stream->word_size;

    TRACE("Compilation Unit Herder found at 0x%x:\n", comp_unit_cursor - debug);
    TRACE("- length:        %lu\n", comp_unit.length);
    TRACE("- version:       %u\n",  comp_unit.version);
    TRACE("- abbrev_offset: %lu\n", comp_unit.abbrev_offset);
    TRACE("- word_size:     %u\n",  comp_unit.word_size);

    ctx.data = comp_unit_cursor + sizeof(dwarf2_comp_unit_stream_t);
    ctx.offset = comp_unit_cursor - debug;
    ctx.word_size = comp_unit.word_size;

    comp_unit_cursor += comp_unit.length + sizeof(unsigned);
    ctx.end_data = comp_unit_cursor;

    if (2 != comp_unit.version) {
      WARN("%u DWARF version unsupported. Wine dbghelp only support DWARF 2.\n", comp_unit.version);
      continue ;
    }

    abbrev_ctx.data = abbrev + comp_unit.abbrev_offset;
    abbrev_ctx.end_data = abbrev + abbrev_size;
    abbrev_ctx.offset = comp_unit.abbrev_offset;
    abbrev_table = dwarf2_parse_abbrev_set(&abbrev_ctx);

    while (ctx.data < ctx.end_data) {
      unsigned long entry_code;
      dwarf2_abbrev_entry_t* entry = NULL;

      dump(ctx.data, 16);
      entry_code = dwarf2_leb128_as_unsigned(&ctx);
      TRACE("found entry_code %lu\n", entry_code);
      if (0 == entry_code) {
	continue ;
      }
      entry = dwarf2_abbrev_table_find_entry(abbrev_table, entry_code);
      if (NULL == entry) {
	WARN("Cannot find abbrev entry for %lu\n", entry_code);
	dwarf2_abbrev_table_free(abbrev_table);
	return FALSE;
      }

      switch (entry->tag) {
      case DW_TAG_compile_unit:
	dwarf2_parse_compiland(module, entry, &ctx);
	break;
      case DW_TAG_base_type:
      case DW_TAG_subprogram:
      case DW_TAG_formal_parameter:
      case DW_TAG_typedef:
      case DW_TAG_pointer_type:
      case DW_TAG_reference_type:
      case DW_TAG_structure_type:
      case DW_TAG_inheritance:
      case DW_TAG_member:
      case DW_TAG_enumeration_type:
      case DW_TAG_enumerator:
      default:
	{
	  dwarf2_abbrev_entry_attr_t* attr;
	  for (attr = entry->attrs; NULL != attr; attr = attr->next) {
	    dwarf2_parse_attr(attr, &ctx);
	  }
	}
	break;
      }
    }
    dwarf2_abbrev_table_free(abbrev_table);
  }
  return bRet;
}

BOOL dwarf2_parse_lines(struct module* module, unsigned long load_offset,
			const unsigned char* debug_line, unsigned int debug_line_size)
{
  return FALSE;
}
