/*
 * TTY DC driver
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "config.h"

#include "gdi.h"
#include "bitmap.h"
#include "palette.h"
#include "ttydrv.h"
#include "winbase.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ttydrv);

/**********************************************************************/

BITMAP_DRIVER TTYDRV_BITMAP_Driver =
{
  TTYDRV_BITMAP_SetDIBits,
  TTYDRV_BITMAP_GetDIBits,
  TTYDRV_BITMAP_DeleteDIBSection,
  TTYDRV_BITMAP_SetDIBColorTable,
  TTYDRV_BITMAP_GetDIBColorTable,
  TTYDRV_BITMAP_Lock,
  TTYDRV_BITMAP_Unlock
};

PALETTE_DRIVER TTYDRV_PALETTE_Driver = 
{
  TTYDRV_PALETTE_SetMapping,
  TTYDRV_PALETTE_UpdateMapping,
  TTYDRV_PALETTE_IsDark
};

/* FIXME: Adapt to the TTY driver. Copied from the X11 driver */

DeviceCaps TTYDRV_DC_DevCaps = {
/* version */		0, 
/* technology */	DT_RASDISPLAY,
/* size, resolution */	0, 0, 0, 0, 0, 
/* device objects */	1, 16 + 6, 16, 0, 0, 100, 0,	
/* curve caps */	CC_CIRCLES | CC_PIE | CC_CHORD | CC_ELLIPSES |
			CC_WIDE | CC_STYLED | CC_WIDESTYLED | CC_INTERIORS | CC_ROUNDRECT,
/* line caps */		LC_POLYLINE | LC_MARKER | LC_POLYMARKER | LC_WIDE |
			LC_STYLED | LC_WIDESTYLED | LC_INTERIORS,
/* polygon caps */	PC_POLYGON | PC_RECTANGLE | PC_WINDPOLYGON |
			PC_SCANLINE | PC_WIDE | PC_STYLED | PC_WIDESTYLED | PC_INTERIORS,
/* text caps */		0,
/* regions */		CP_REGION,
/* raster caps */	RC_BITBLT | RC_BANDING | RC_SCALING | RC_BITMAP64 |
			RC_DI_BITMAP | RC_DIBTODEV | RC_BIGFONT | RC_STRETCHBLT | RC_STRETCHDIB | RC_DEVBITS,
/* aspects */		36, 36, 51,
/* pad1 */		{ 0 },
/* log pixels */	0, 0, 
/* pad2 */		{ 0 },
/* palette size */	0,
/* ..etc */		0, 0
};

const DC_FUNCTIONS *TTYDRV_DC_Funcs = NULL;  /* hack */

/**********************************************************************
 *	     TTYDRV_GDI_Initialize
 */
BOOL TTYDRV_GDI_Initialize(void)
{
  BITMAP_Driver = &TTYDRV_BITMAP_Driver;
  PALETTE_Driver = &TTYDRV_PALETTE_Driver;

  TTYDRV_DC_DevCaps.version = 0x300;
  TTYDRV_DC_DevCaps.horzSize = 0;    /* FIXME: Screen width in mm */
  TTYDRV_DC_DevCaps.vertSize = 0;    /* FIXME: Screen height in mm */
  TTYDRV_DC_DevCaps.horzRes = 640;   /* FIXME: Screen width in pixel */
  TTYDRV_DC_DevCaps.vertRes = 480;   /* FIXME: Screen height in pixel */
  TTYDRV_DC_DevCaps.bitsPixel = 1;   /* FIXME: Bits per pixel */
  TTYDRV_DC_DevCaps.sizePalette = 256; /* FIXME: ??? */
  
  /* Resolution will be adjusted during the font init */
  
  TTYDRV_DC_DevCaps.logPixelsX = (int) (TTYDRV_DC_DevCaps.horzRes * 25.4 / TTYDRV_DC_DevCaps.horzSize);
  TTYDRV_DC_DevCaps.logPixelsY = (int) (TTYDRV_DC_DevCaps.vertRes * 25.4 / TTYDRV_DC_DevCaps.vertSize);
 
  return TTYDRV_PALETTE_Initialize();
}

/**********************************************************************
 *	     TTYDRV_GDI_Finalize
 */
void TTYDRV_GDI_Finalize(void)
{
    TTYDRV_PALETTE_Finalize();
}

/***********************************************************************
 *	     TTYDRV_DC_CreateDC
 */
BOOL TTYDRV_DC_CreateDC(DC *dc, LPCSTR driver, LPCSTR device,
			LPCSTR output, const DEVMODEA *initData)
{
  TTYDRV_PDEVICE *physDev;
  BITMAPOBJ *bmp;

  TRACE("(%p, %s, %s, %s, %p)\n",
    dc, debugstr_a(driver), debugstr_a(device), 
    debugstr_a(output), initData);

  if (!TTYDRV_DC_Funcs) TTYDRV_DC_Funcs = dc->funcs;  /* hack */

  dc->physDev = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			  sizeof(TTYDRV_PDEVICE));  
  if(!dc->physDev) {
    ERR("Can't allocate physDev\n");
    return FALSE;
  }
  physDev = (TTYDRV_PDEVICE *) dc->physDev;
  
  dc->devCaps = &TTYDRV_DC_DevCaps;

  if(dc->flags & DC_MEMORY){
    physDev->window = NULL;
    physDev->cellWidth = 1;
    physDev->cellHeight = 1;

    TTYDRV_DC_CreateBitmap(dc->hBitmap);
    bmp = (BITMAPOBJ *) GDI_GetObjPtr(dc->hBitmap, BITMAP_MAGIC);
				   
    dc->bitsPerPixel = bmp->bitmap.bmBitsPixel;
    
    dc->totalExtent.left   = 0;
    dc->totalExtent.top    = 0;
    dc->totalExtent.right  = bmp->bitmap.bmWidth;
    dc->totalExtent.bottom = bmp->bitmap.bmHeight;
    dc->hVisRgn            = CreateRectRgnIndirect( &dc->totalExtent );
    
    GDI_ReleaseObj( dc->hBitmap );
  } else {
    physDev->window = root_window;
    physDev->cellWidth = cell_width;
    physDev->cellHeight = cell_height;
    
    dc->bitsPerPixel       = 1;
    dc->totalExtent.left   = 0;
    dc->totalExtent.top    = 0;
    dc->totalExtent.right  = cell_width * screen_cols;
    dc->totalExtent.bottom = cell_height * screen_rows;
    dc->hVisRgn            = CreateRectRgnIndirect( &dc->totalExtent );    
  }

  return TRUE;
}

/***********************************************************************
 *	     TTYDRV_DC_DeleteDC
 */
BOOL TTYDRV_DC_DeleteDC(DC *dc)
{
  TRACE("(%p)\n", dc);

  HeapFree( GetProcessHeap(), 0, dc->physDev );
  dc->physDev = NULL;
  
  return TRUE;
}

/***********************************************************************
 *           TTYDRV_DC_Escape
 */
INT TTYDRV_DC_Escape(DC *dc, INT nEscape, INT cbInput,
		     SEGPTR lpInData, SEGPTR lpOutData)
{
  return 0;
}

/***********************************************************************
 *		TTYDRV_DC_SetDeviceClipping
 */
void TTYDRV_DC_SetDeviceClipping(DC *dc)
{
  TRACE("(%p)\n", dc);
}
