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
#include "ttydrv.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ttydrv);


/**********************************************************************
 *	     TTYDRV_GDI_Initialize
 */
BOOL TTYDRV_GDI_Initialize(void)
{
  return TTYDRV_PALETTE_Initialize();
}

/***********************************************************************
 *	     TTYDRV_DC_CreateDC
 */
BOOL TTYDRV_DC_CreateDC(DC *dc, TTYDRV_PDEVICE **pdev, LPCSTR driver, LPCSTR device,
			LPCSTR output, const DEVMODEA *initData)
{
  TTYDRV_PDEVICE *physDev;

  TRACE("(%p, %s, %s, %s, %p)\n",
    dc, debugstr_a(driver), debugstr_a(device),
    debugstr_a(output), initData);

  physDev = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TTYDRV_PDEVICE));
  if(!physDev) {
    ERR("Can't allocate physDev\n");
    return FALSE;
  }
  *pdev = physDev;
  physDev->hdc = dc->hSelf;
  physDev->org.x = physDev->org.y = 0;

  if(dc->flags & DC_MEMORY){
    physDev->window = NULL;
    physDev->cellWidth = 1;
    physDev->cellHeight = 1;
  } else {
    physDev->window = root_window;
    physDev->cellWidth = cell_width;
    physDev->cellHeight = cell_height;

    dc->bitsPerPixel = 1;
  }

  return TRUE;
}

/***********************************************************************
 *	     TTYDRV_DC_DeleteDC
 */
BOOL TTYDRV_DC_DeleteDC(TTYDRV_PDEVICE *physDev)
{
    TRACE("(%p)\n", physDev->hdc);

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
        return cell_width * screen_cols;
    case VERTRES:
        return cell_height * screen_rows;
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
        FIXME("(%p): unsupported capability %d, will return 0\n", physDev->hdc, cap );
        return 0;
    }
}


/***********************************************************************
 *           GetDCOrgEx    (TTYDRV.@)
 */
BOOL TTYDRV_GetDCOrgEx( TTYDRV_PDEVICE *physDev, LPPOINT pt )
{
    *pt = physDev->org;
    return TRUE;
}


/***********************************************************************
 *           SetDCOrg    (TTYDRV.@)
 */
DWORD TTYDRV_SetDCOrg( TTYDRV_PDEVICE *physDev, INT x, INT y )
{
    DWORD ret = MAKELONG( physDev->org.x, physDev->org.y );
    physDev->org.x = x;
    physDev->org.y = y;
    return ret;
}
