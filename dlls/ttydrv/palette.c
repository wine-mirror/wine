/*
 * TTY palette driver
 *
 * Copyright 1999 Patrik Stridvall
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

#include <stdlib.h>

#include "palette.h"
#include "winbase.h"
#include "ttydrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ttydrv);

/**********************************************************************/

static PALETTEENTRY *COLOR_sysPal;

static int palette_size = 256;  /* FIXME */

/***********************************************************************
 *	     TTYDRV_PALETTE_Initialize
 */
BOOL TTYDRV_PALETTE_Initialize(void)
{
  int i;
  PALETTEENTRY sys_pal_template[NB_RESERVED_COLORS];

  TRACE("(void)\n");

  COLOR_sysPal = (PALETTEENTRY *) HeapAlloc(GetProcessHeap(), 0, sizeof(PALETTEENTRY) * palette_size);
  if(COLOR_sysPal == NULL) {
    WARN("No memory to create system palette!\n");
    return FALSE;
  }

  GetPaletteEntries( GetStockObject(DEFAULT_PALETTE), 0, NB_RESERVED_COLORS, sys_pal_template );

  for(i=0; i < palette_size; i++ ) {
    const PALETTEENTRY *src;
    PALETTEENTRY *dst = &COLOR_sysPal[i];

    if(i < NB_RESERVED_COLORS/2) {
      src = &sys_pal_template[i];
    } else if(i >= palette_size - NB_RESERVED_COLORS/2) {
      src = &sys_pal_template[NB_RESERVED_COLORS + i - palette_size];
    } else {
      PALETTEENTRY pe = { 0, 0, 0, 0 };
      src = &pe;
    }

    if((src->peRed + src->peGreen + src->peBlue) <= 0xB0) {
      dst->peRed = 0;
      dst->peGreen = 0;
      dst->peBlue = 0;
      dst->peFlags = PC_SYS_USED;
    } else {
      dst->peRed = 255;
      dst->peGreen= 255;
      dst->peBlue = 255;
      dst->peFlags = PC_SYS_USED;
    }
  }

  return TRUE;
}


/***********************************************************************
 *               GetSystemPaletteEntries   (TTYDRV.@)
 */
UINT TTYDRV_GetSystemPaletteEntries( TTYDRV_PDEVICE *dev, UINT start, UINT count,
                                     LPPALETTEENTRY entries )
{
    UINT i;

    if (!entries) return palette_size;
    if (start >= palette_size) return 0;
    if (start + count >= palette_size) count = palette_size - start;

    for (i = 0; i < count; i++)
    {
        entries[i].peRed   = COLOR_sysPal[start + i].peRed;
        entries[i].peGreen = COLOR_sysPal[start + i].peGreen;
        entries[i].peBlue  = COLOR_sysPal[start + i].peBlue;
        entries[i].peFlags = 0;
        TRACE("\tidx(%02x) -> RGB(%08lx)\n", start + i, *(COLORREF*)(entries + i) );
    }
    return count;
}
