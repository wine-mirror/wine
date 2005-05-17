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

typedef struct dwarf2_parse_context_s {
  const unsigned char* data;
} dwarf2_parse_context_t;

unsigned long dwarf2_leb128_as_unsigned(dwarf2_parse_context_t* ctx)
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

long dwarf2_leb128_as_signed(dwarf2_parse_context_t* ctx)
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

BOOL dwarf2_parse(struct module* module, unsigned long load_offset,
		  const unsigned char* debug, unsigned int debug_size,
		  const unsigned char* abbrev, unsigned int abbrev_size,
		  const unsigned char* str, unsigned int str_sz)
{
  return FALSE;
}
