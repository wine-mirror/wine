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

#include "color.h"
#include "wine/debug.h"
#include "palette.h"
#include "winbase.h"
#include "ttydrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(ttydrv);

/**********************************************************************/

extern PALETTEENTRY *COLOR_sysPal;
extern int COLOR_gapStart;
extern int COLOR_gapEnd;
extern int COLOR_gapFilled;
extern int COLOR_max;

static int palette_size = 256;  /* FIXME */

extern const PALETTEENTRY COLOR_sysPalTemplate[NB_RESERVED_COLORS]; 

/***********************************************************************
 *	     TTYDRV_PALETTE_Initialize
 */
BOOL TTYDRV_PALETTE_Initialize(void)
{
  int i;

  TRACE("(void)\n");

  COLOR_sysPal = (PALETTEENTRY *) HeapAlloc(GetProcessHeap(), 0, sizeof(PALETTEENTRY) * palette_size);
  if(COLOR_sysPal == NULL) {
    WARN("No memory to create system palette!\n");
    return FALSE;
  }

  for(i=0; i < palette_size; i++ ) {
    const PALETTEENTRY *src;
    PALETTEENTRY *dst = &COLOR_sysPal[i];

    if(i < NB_RESERVED_COLORS/2) {
      src = &COLOR_sysPalTemplate[i];
    } else if(i >= palette_size - NB_RESERVED_COLORS/2) {
      src = &COLOR_sysPalTemplate[NB_RESERVED_COLORS + i - palette_size];
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

  COLOR_gapStart = NB_RESERVED_COLORS/2;
  COLOR_gapEnd = NB_RESERVED_COLORS/2;

  return TRUE;
}

/***********************************************************************
 *           TTYDRV_PALETTE_SetMapping
 */
int TTYDRV_PALETTE_SetMapping(
  PALETTEOBJ *palPtr, UINT uStart, UINT uNum, BOOL mapOnly)
{
  FIXME("(%p, %u, %u, %d): stub\n", palPtr, uStart, uNum, mapOnly);

  return 0;
}

/***********************************************************************
 *           TTYDRV_PALETTE_UpdateMapping
 */
int TTYDRV_PALETTE_UpdateMapping(PALETTEOBJ *palPtr)
{
  TRACE("(%p)\n", palPtr);

  return 0;
}

/***********************************************************************
 *           TTYDRV_PALETTE_IsDark
 */
int TTYDRV_PALETTE_IsDark(int pixel)
{
  FIXME("(%d): stub\n", pixel);

  return 0;
}
