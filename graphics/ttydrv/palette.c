/*
 * TTY palette driver
 *
 * Copyright 1999 Patrik Stridvall
 */

#include <stdlib.h>

#include "color.h"
#include "debugtools.h"
#include "palette.h"
#include "ttydrv.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

/**********************************************************************/

extern DeviceCaps TTYDRV_DC_DevCaps;

extern PALETTEENTRY *COLOR_sysPal;
extern int COLOR_gapStart;
extern int COLOR_gapEnd;
extern int COLOR_gapFilled;
extern int COLOR_max;

extern const PALETTEENTRY COLOR_sysPalTemplate[NB_RESERVED_COLORS]; 

/***********************************************************************
 *	     TTYDRV_PALETTE_Initialize
 */
BOOL TTYDRV_PALETTE_Initialize(void)
{
  int i;

  TRACE("(void)\n");

  TTYDRV_DC_DevCaps.sizePalette = 256;

  COLOR_sysPal = (PALETTEENTRY *) HeapAlloc(GetProcessHeap(), 0, sizeof(PALETTEENTRY) * TTYDRV_DC_DevCaps.sizePalette);
  if(COLOR_sysPal == NULL) {
    WARN("No memory to create system palette!");
    return FALSE;
  }

  for(i=0; i < TTYDRV_DC_DevCaps.sizePalette; i++ ) {
    const PALETTEENTRY *src;
    PALETTEENTRY *dst = &COLOR_sysPal[i];

    if(i < NB_RESERVED_COLORS/2) {
      src = &COLOR_sysPalTemplate[i];
    } else if(i >= TTYDRV_DC_DevCaps.sizePalette - NB_RESERVED_COLORS/2) {
      src = &COLOR_sysPalTemplate[NB_RESERVED_COLORS + i - TTYDRV_DC_DevCaps.sizePalette];
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
 *	     TTYDRV_PALETTE_Finalize
 *
 */
void TTYDRV_PALETTE_Finalize(void)
{
  TRACE("(void)\n");
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
