/*
 * TTY DC driver
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "config.h"

#include "gdi.h"
#include "bitmap.h"
#include "dc.h"
#include "ttydrv.h"
#include "winbase.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ttydrv);

/**********************************************************************/

extern DeviceCaps TTYDRV_DC_DevCaps;

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

  dc->physDev = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			  sizeof(TTYDRV_PDEVICE));  
  if(!dc->physDev) {
    ERR("Can't allocate physDev\n");
    return FALSE;
  }
  physDev = (TTYDRV_PDEVICE *) dc->physDev;
  
  dc->w.devCaps = &TTYDRV_DC_DevCaps;

  if(dc->w.flags & DC_MEMORY){
    physDev->window = NULL;
    physDev->cellWidth = 1;
    physDev->cellHeight = 1;

    TTYDRV_DC_CreateBitmap(dc->w.hBitmap);
    bmp = (BITMAPOBJ *) GDI_GetObjPtr(dc->w.hBitmap, BITMAP_MAGIC);
				   
    dc->w.bitsPerPixel = bmp->bitmap.bmBitsPixel;
    
    dc->w.totalExtent.left   = 0;
    dc->w.totalExtent.top    = 0;
    dc->w.totalExtent.right  = bmp->bitmap.bmWidth;
    dc->w.totalExtent.bottom = bmp->bitmap.bmHeight;
    dc->w.hVisRgn            = CreateRectRgnIndirect( &dc->w.totalExtent );
    
    GDI_HEAP_UNLOCK( dc->w.hBitmap );
  } else {
    physDev->window = TTYDRV_GetRootWindow();
    physDev->cellWidth = cell_width;
    physDev->cellHeight = cell_height;
    
    dc->w.bitsPerPixel       = 1;
    dc->w.totalExtent.left   = 0;
    dc->w.totalExtent.top    = 0;
    dc->w.totalExtent.right  = cell_width * screen_cols;
    dc->w.totalExtent.bottom = cell_height * screen_rows;
    dc->w.hVisRgn            = CreateRectRgnIndirect( &dc->w.totalExtent );    
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

