/*
 * X11 graphics driver initialisation functions
 *
 * Copyright 1996 Alexandre Julliard
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

#include "ts_xlib.h"

#include <string.h>

#include "bitmap.h"
#include "palette.h"
#include "wine/debug.h"
#include "winnt.h"
#include "x11drv.h"
#include "ddrawi.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

const DC_FUNCTIONS *X11DRV_DC_Funcs = NULL;  /* hack */

BITMAP_DRIVER X11DRV_BITMAP_Driver =
{
  X11DRV_DIB_SetDIBits,
  X11DRV_DIB_GetDIBits,
  X11DRV_DIB_DeleteDIBSection,
  X11DRV_DIB_SetDIBColorTable,
  X11DRV_DIB_GetDIBColorTable,
  X11DRV_DIB_Lock,
  X11DRV_DIB_Unlock
};

PALETTE_DRIVER X11DRV_PALETTE_Driver =
{
  X11DRV_PALETTE_SetMapping,
  X11DRV_PALETTE_UpdateMapping,
  X11DRV_PALETTE_IsDark
};


Display *gdi_display;  /* display to use for all GDI functions */


/* a few dynamic device caps */
static int log_pixels_x;  /* pixels per logical inch in x direction */
static int log_pixels_y;  /* pixels per logical inch in y direction */
static int horz_size;     /* horz. size of screen in millimeters */
static int vert_size;     /* vert. size of screen in millimeters */
static int palette_size;
static int text_caps;

/**********************************************************************
 *	     X11DRV_GDI_Initialize
 */
BOOL X11DRV_GDI_Initialize( Display *display )
{
    Screen *screen = DefaultScreenOfDisplay(display);

    gdi_display = display;
    BITMAP_Driver = &X11DRV_BITMAP_Driver;
    PALETTE_Driver = &X11DRV_PALETTE_Driver;

    palette_size = X11DRV_PALETTE_Init();

    if (!X11DRV_BITMAP_Init()) return FALSE;

    /* Initialize XRender */
    X11DRV_XRender_Init();

    /* Initialize fonts and text caps */

    log_pixels_x = MulDiv( WidthOfScreen(screen), 254, WidthMMOfScreen(screen) * 10 );
    log_pixels_y = MulDiv( HeightOfScreen(screen), 254, HeightMMOfScreen(screen) * 10 );
    text_caps = X11DRV_FONT_Init( &log_pixels_x, &log_pixels_y );
    horz_size = MulDiv( screen_width, 254, log_pixels_x * 10 );
    vert_size = MulDiv( screen_height, 254, log_pixels_y * 10 );
    return TRUE;
}

/**********************************************************************
 *	     X11DRV_GDI_Finalize
 */
void X11DRV_GDI_Finalize(void)
{
    X11DRV_PALETTE_Cleanup();
    XCloseDisplay( gdi_display );
    gdi_display = NULL;
}

/**********************************************************************
 *	     X11DRV_CreateDC
 */
BOOL X11DRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                      LPCSTR output, const DEVMODEA* initData )
{
    X11DRV_PDEVICE *physDev;

    if (!X11DRV_DC_Funcs) X11DRV_DC_Funcs = dc->funcs;  /* hack */

    dc->physDev = physDev = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
				       sizeof(*physDev) );
    if(!physDev) {
        ERR("Can't allocate physDev\n");
	return FALSE;
    }

    if (dc->flags & DC_MEMORY)
    {
        BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( dc->hBitmap, BITMAP_MAGIC );
	if (!bmp) 
	{
	    HeapFree( GetProcessHeap(), 0, physDev );
	    return FALSE;
        }
        if (!bmp->physBitmap) X11DRV_CreateBitmap( dc->hBitmap );
        physDev->drawable  = (Pixmap)bmp->physBitmap;
        physDev->gc        = TSXCreateGC( gdi_display, physDev->drawable, 0, NULL );
        dc->bitsPerPixel       = bmp->bitmap.bmBitsPixel;
        dc->totalExtent.left   = 0;
        dc->totalExtent.top    = 0;
        dc->totalExtent.right  = bmp->bitmap.bmWidth;
        dc->totalExtent.bottom = bmp->bitmap.bmHeight;
        GDI_ReleaseObj( dc->hBitmap );
    }
    else
    {
        physDev->drawable  = root_window;
        physDev->gc        = TSXCreateGC( gdi_display, physDev->drawable, 0, NULL );
        dc->bitsPerPixel       = screen_depth;
        dc->totalExtent.left   = 0;
        dc->totalExtent.top    = 0;
        dc->totalExtent.right  = screen_width;
        dc->totalExtent.bottom = screen_height;
    }

    physDev->current_pf   = 0;
    physDev->used_visuals = 0;

    if (!(dc->hVisRgn = CreateRectRgnIndirect( &dc->totalExtent )))
    {
        TSXFreeGC( gdi_display, physDev->gc );
	HeapFree( GetProcessHeap(), 0, physDev );
        return FALSE;
    }

    wine_tsx11_lock();
    XSetGraphicsExposures( gdi_display, physDev->gc, False );
    XSetSubwindowMode( gdi_display, physDev->gc, IncludeInferiors );
    XFlush( gdi_display );
    wine_tsx11_unlock();
    return TRUE;
}


/**********************************************************************
 *	     X11DRV_DeleteDC
 */
BOOL X11DRV_DeleteDC( DC *dc )
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    if(physDev->xrender)
      X11DRV_XRender_DeleteDC(dc);
    wine_tsx11_lock();
    XFreeGC( gdi_display, physDev->gc );
    while (physDev->used_visuals-- > 0)
        XFree(physDev->visuals[physDev->used_visuals]);
    wine_tsx11_unlock();
    HeapFree( GetProcessHeap(), 0, physDev );
    dc->physDev = NULL;
    return TRUE;
}


/***********************************************************************
 *           GetDeviceCaps    (X11DRV.@)
 */
INT X11DRV_GetDeviceCaps( DC *dc, INT cap )
{
    switch(cap)
    {
    case DRIVERVERSION:
        return 0x300;
    case TECHNOLOGY:
        return DT_RASDISPLAY;
    case HORZSIZE:
        return horz_size;
    case VERTSIZE:
        return vert_size;
    case HORZRES:
        return screen_width;
    case VERTRES:
        return screen_height;
    case BITSPIXEL:
        return screen_depth;
    case PLANES:
        return 1;
    case NUMBRUSHES:
        return -1;
    case NUMPENS:
        return -1;
    case NUMMARKERS:
        return 0;
    case NUMFONTS:
        return 0;
    case NUMCOLORS:
        /* MSDN: Number of entries in the device's color table, if the device has
         * a color depth of no more than 8 bits per pixel.For devices with greater
         * color depths, -1 is returned. */
        return (screen_depth > 8) ? -1 : (1 << screen_depth);
    case PDEVICESIZE:
        return sizeof(X11DRV_PDEVICE);
    case CURVECAPS:
        return (CC_CIRCLES | CC_PIE | CC_CHORD | CC_ELLIPSES | CC_WIDE |
                CC_STYLED | CC_WIDESTYLED | CC_INTERIORS | CC_ROUNDRECT);
    case LINECAPS:
        return (LC_POLYLINE | LC_MARKER | LC_POLYMARKER | LC_WIDE |
                LC_STYLED | LC_WIDESTYLED | LC_INTERIORS);
    case POLYGONALCAPS:
        return (PC_POLYGON | PC_RECTANGLE | PC_WINDPOLYGON | PC_SCANLINE |
                PC_WIDE | PC_STYLED | PC_WIDESTYLED | PC_INTERIORS);
    case TEXTCAPS:
        return text_caps;
    case CLIPCAPS:
        return CP_REGION;
    case RASTERCAPS:
        return (RC_BITBLT | RC_BANDING | RC_SCALING | RC_BITMAP64 | RC_DI_BITMAP |
                RC_DIBTODEV | RC_BIGFONT | RC_STRETCHBLT | RC_STRETCHDIB | RC_DEVBITS |
                (palette_size ? RC_PALETTE : 0));
    case ASPECTX:
    case ASPECTY:
        return 36;
    case ASPECTXY:
        return 51;
    case LOGPIXELSX:
        return log_pixels_x;
    case LOGPIXELSY:
        return log_pixels_y;
    case CAPS1:
        FIXME("(%04x): CAPS1 is unimplemented, will return 0\n", dc->hSelf );
        /* please see wingdi.h for the possible bit-flag values that need
           to be returned. also, see 
           http://msdn.microsoft.com/library/ddkdoc/win95ddk/graphcnt_1m0p.htm */
        return 0;
    case SIZEPALETTE:
        return palette_size;
    case NUMRESERVED:
    case COLORRES:
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
        FIXME("(%04x): unsupported capability %d, will return 0\n", dc->hSelf, cap );
        return 0;
    }
}


/**********************************************************************
 *           ExtEscape  (X11DRV.@)
 */
INT X11DRV_ExtEscape( DC *dc, INT escape, INT in_count, LPCVOID in_data,
                      INT out_count, LPVOID out_data )
{
    switch(escape)
    {
    case QUERYESCSUPPORT:
        if (in_data)
        {
            switch (*(INT *)in_data)
            {
            case DCICOMMAND:
                return DD_HAL_VERSION;
            }
        }
        break;

    case DCICOMMAND:
        if (in_data)
        {
            const DCICMD *lpCmd = in_data;
            if (lpCmd->dwVersion != DD_VERSION) break;
            return X11DRV_DCICommand(in_count, lpCmd, out_data);
        }
        break;
    }
    return 0;
}
