/*
 * TTY DC driver
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

#include "gdi.h"
#include "bitmap.h"
#include "palette.h"
#include "ttydrv.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ttydrv);

/**********************************************************************/

PALETTE_DRIVER TTYDRV_PALETTE_Driver = 
{
  TTYDRV_PALETTE_SetMapping,
  TTYDRV_PALETTE_UpdateMapping,
  TTYDRV_PALETTE_IsDark
};

const DC_FUNCTIONS *TTYDRV_DC_Funcs = NULL;  /* hack */

/**********************************************************************
 *	     TTYDRV_GDI_Initialize
 */
BOOL TTYDRV_GDI_Initialize(void)
{
  PALETTE_Driver = &TTYDRV_PALETTE_Driver;

  return TTYDRV_PALETTE_Initialize();
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
  physDev->hdc = dc->hSelf;
  physDev->dc = dc;

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
BOOL TTYDRV_DC_DeleteDC(TTYDRV_PDEVICE *physDev)
{
    TRACE("(%x)\n", physDev->hdc);

    physDev->dc->physDev = NULL;
    HeapFree( GetProcessHeap(), 0, physDev );
    return TRUE;
}


/***********************************************************************
 *           GetDeviceCaps    (TTYDRV.@)
 */
INT TTYDRV_GetDeviceCaps( TTYDRV_PDEVICE *physDev, INT cap )
{
    switch(cap)
    {
    case DRIVERVERSION:
        return 0x300;
    case TECHNOLOGY:
        return DT_RASDISPLAY;
    case HORZSIZE:
        return 0;    /* FIXME: Screen width in mm */
    case VERTSIZE:
        return 0;    /* FIXME: Screen height in mm */
    case HORZRES:
        return 640;  /* FIXME: Screen width in pixel */
    case VERTRES:
        return 480;  /* FIXME: Screen height in pixel */
    case BITSPIXEL:
        return 1;    /* FIXME */
    case PLANES:
        return 1;
    case NUMBRUSHES:
        return 16 + 6;
    case NUMPENS:
        return 16;
    case NUMMARKERS:
        return 0;
    case NUMFONTS:
        return 0;
    case NUMCOLORS:
        return 100;
    case PDEVICESIZE:
        return sizeof(TTYDRV_PDEVICE);
    case CURVECAPS:
        return (CC_CIRCLES | CC_PIE | CC_CHORD | CC_ELLIPSES | CC_WIDE |
                CC_STYLED | CC_WIDESTYLED | CC_INTERIORS | CC_ROUNDRECT);
    case LINECAPS:
        return (LC_POLYLINE | LC_MARKER | LC_POLYMARKER | LC_WIDE |
                LC_STYLED | LC_WIDESTYLED | LC_INTERIORS);
    case POLYGONALCAPS:
        return (PC_POLYGON | PC_RECTANGLE | PC_WINDPOLYGON |
                PC_SCANLINE | PC_WIDE | PC_STYLED | PC_WIDESTYLED | PC_INTERIORS);
    case TEXTCAPS:
        return 0;
    case CLIPCAPS:
        return CP_REGION;
    case RASTERCAPS:
        return (RC_BITBLT | RC_BANDING | RC_SCALING | RC_BITMAP64 | RC_DI_BITMAP |
                RC_DIBTODEV | RC_BIGFONT | RC_STRETCHBLT | RC_STRETCHDIB | RC_DEVBITS);
    case ASPECTX:
    case ASPECTY:
        return 36;
    case ASPECTXY:
        return 51;
    case LOGPIXELSX:
    case LOGPIXELSY:
        return 72;  /* FIXME */
    case SIZEPALETTE:
        return 256;  /* FIXME */
    case NUMRESERVED:
        return 0;
    case COLORRES:
        return 0;
    case PHYSICALWIDTH:
    case PHYSICALHEIGHT:
    case PHYSICALOFFSETX:
    case PHYSICALOFFSETY:
    case SCALINGFACTORX:
    case SCALINGFACTORY:
    case VREFRESH:
    case DESKTOPVERTRES:
    case DESKTOPHORZRES:
    case BTLALIGNMENT:
        return 0;
    default:
        FIXME("(%04x): unsupported capability %d, will return 0\n", physDev->hdc, cap );
        return 0;
    }
}
