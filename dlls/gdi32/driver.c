/*
 * Graphics driver management functions
 *
 * Copyright 1994 Bob Amstadt
 * Copyright 1996, 2001 Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ddrawgdi.h"
#include "wine/winbase16.h"
#include "winuser.h"
#include "winternl.h"
#include "ddk/d3dkmthk.h"

#include "gdi_private.h"
#include "wine/unicode.h"
#include "wine/list.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(driver);

struct graphics_driver
{
    struct list                entry;
    HMODULE                    module;  /* module handle */
    const struct gdi_dc_funcs *funcs;
};

struct d3dkmt_adapter
{
    D3DKMT_HANDLE handle;               /* Kernel mode graphics adapter handle */
    INT ordinal;                        /* Graphics adapter ordinal */
    struct list entry;                  /* List entry */
};

struct d3dkmt_device
{
    D3DKMT_HANDLE handle;               /* Kernel mode graphics device handle*/
    struct list entry;                  /* List entry */
};

static struct list drivers = LIST_INIT( drivers );
static struct graphics_driver *display_driver;

static struct list d3dkmt_adapters = LIST_INIT( d3dkmt_adapters );
static struct list d3dkmt_devices = LIST_INIT( d3dkmt_devices );

const struct gdi_dc_funcs *font_driver = NULL;

static CRITICAL_SECTION driver_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &driver_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": driver_section") }
};
static CRITICAL_SECTION driver_section = { &critsect_debug, -1, 0, 0, 0, 0 };

static HWND (WINAPI *pGetDesktopWindow)(void);
static INT (WINAPI *pGetSystemMetrics)(INT);
static DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);

/**********************************************************************
 *	     create_driver
 *
 * Allocate and fill the driver structure for a given module.
 */
static struct graphics_driver *create_driver( HMODULE module )
{
    static const struct gdi_dc_funcs empty_funcs;
    const struct gdi_dc_funcs *funcs = NULL;
    struct graphics_driver *driver;

    if (!(driver = HeapAlloc( GetProcessHeap(), 0, sizeof(*driver)))) return NULL;
    driver->module = module;

    if (module)
    {
        const struct gdi_dc_funcs * (CDECL *wine_get_gdi_driver)( unsigned int version );

        if ((wine_get_gdi_driver = (void *)GetProcAddress( module, "wine_get_gdi_driver" )))
            funcs = wine_get_gdi_driver( WINE_GDI_DRIVER_VERSION );
    }
    if (!funcs) funcs = &empty_funcs;
    driver->funcs = funcs;
    return driver;
}


/**********************************************************************
 *	     get_display_driver
 *
 * Special case for loading the display driver: get the name from the config file
 */
static const struct gdi_dc_funcs *get_display_driver(void)
{
    if (!display_driver)
    {
        HMODULE user32 = LoadLibraryA( "user32.dll" );
        pGetDesktopWindow = (void *)GetProcAddress( user32, "GetDesktopWindow" );

        if (!pGetDesktopWindow() || !display_driver)
        {
            WARN( "failed to load the display driver, falling back to null driver\n" );
            __wine_set_display_driver( 0 );
        }
    }
    return display_driver->funcs;
}


/**********************************************************************
 *	     is_display_device
 */
static BOOL is_display_device( LPCWSTR name )
{
    static const WCHAR display_deviceW[] = {'\\','\\','.','\\','D','I','S','P','L','A','Y'};
    const WCHAR *p = name;

    if (strncmpiW( name, display_deviceW, sizeof(display_deviceW) / sizeof(WCHAR) ))
        return FALSE;

    p += sizeof(display_deviceW) / sizeof(WCHAR);

    if (!isdigitW( *p++ ))
        return FALSE;

    for (; *p; p++)
    {
        if (!isdigitW( *p ))
            return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *	     DRIVER_load_driver
 */
const struct gdi_dc_funcs *DRIVER_load_driver( LPCWSTR name )
{
    HMODULE module;
    struct graphics_driver *driver, *new_driver;
    static const WCHAR displayW[] = { 'd','i','s','p','l','a','y',0 };

    /* display driver is a special case */
    if (!strcmpiW( name, displayW ) || is_display_device( name )) return get_display_driver();

    if ((module = GetModuleHandleW( name )))
    {
        if (display_driver && display_driver->module == module) return display_driver->funcs;

        EnterCriticalSection( &driver_section );
        LIST_FOR_EACH_ENTRY( driver, &drivers, struct graphics_driver, entry )
        {
            if (driver->module == module) goto done;
        }
        LeaveCriticalSection( &driver_section );
    }

    if (!(module = LoadLibraryW( name ))) return NULL;

    if (!(new_driver = create_driver( module )))
    {
        FreeLibrary( module );
        return NULL;
    }

    /* check if someone else added it in the meantime */
    EnterCriticalSection( &driver_section );
    LIST_FOR_EACH_ENTRY( driver, &drivers, struct graphics_driver, entry )
    {
        if (driver->module != module) continue;
        FreeLibrary( module );
        HeapFree( GetProcessHeap(), 0, new_driver );
        goto done;
    }
    driver = new_driver;
    list_add_head( &drivers, &driver->entry );
    TRACE( "loaded driver %p for %s\n", driver, debugstr_w(name) );
done:
    LeaveCriticalSection( &driver_section );
    return driver->funcs;
}


/***********************************************************************
 *           __wine_set_display_driver    (GDI32.@)
 */
void CDECL __wine_set_display_driver( HMODULE module )
{
    struct graphics_driver *driver;
    HMODULE user32;

    if (!(driver = create_driver( module )))
    {
        ERR( "Could not create graphics driver\n" );
        ExitProcess(1);
    }
    if (InterlockedCompareExchangePointer( (void **)&display_driver, driver, NULL ))
        HeapFree( GetProcessHeap(), 0, driver );

    user32 = LoadLibraryA( "user32.dll" );
    pGetSystemMetrics = (void *)GetProcAddress( user32, "GetSystemMetrics" );
    pSetThreadDpiAwarenessContext = (void *)GetProcAddress( user32, "SetThreadDpiAwarenessContext" );
}


static INT CDECL nulldrv_AbortDoc( PHYSDEV dev )
{
    return 0;
}

static BOOL CDECL nulldrv_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                               INT xstart, INT ystart, INT xend, INT yend )
{
    return TRUE;
}

static BOOL CDECL nulldrv_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                                 INT xstart, INT ystart, INT xend, INT yend )
{
    return TRUE;
}

static BOOL CDECL nulldrv_CreateCompatibleDC( PHYSDEV orig, PHYSDEV *pdev )
{
    if (!display_driver || !display_driver->funcs->pCreateCompatibleDC) return TRUE;
    return display_driver->funcs->pCreateCompatibleDC( NULL, pdev );
}

static BOOL CDECL nulldrv_CreateDC( PHYSDEV *dev, LPCWSTR driver, LPCWSTR device,
                                    LPCWSTR output, const DEVMODEW *devmode )
{
    assert(0);  /* should never be called */
    return FALSE;
}

static BOOL CDECL nulldrv_DeleteDC( PHYSDEV dev )
{
    assert(0);  /* should never be called */
    return TRUE;
}

static BOOL CDECL nulldrv_DeleteObject( PHYSDEV dev, HGDIOBJ obj )
{
    return TRUE;
}

static DWORD CDECL nulldrv_DeviceCapabilities( LPSTR buffer, LPCSTR device, LPCSTR port,
                                               WORD cap, LPSTR output, DEVMODEA *devmode )
{
    return -1;
}

static BOOL CDECL nulldrv_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    return TRUE;
}

static INT CDECL nulldrv_EndDoc( PHYSDEV dev )
{
    return 0;
}

static INT CDECL nulldrv_EndPage( PHYSDEV dev )
{
    return 0;
}

static BOOL CDECL nulldrv_EnumFonts( PHYSDEV dev, LOGFONTW *logfont, FONTENUMPROCW proc, LPARAM lParam )
{
    return TRUE;
}

static INT CDECL nulldrv_EnumICMProfiles( PHYSDEV dev, ICMENUMPROCW func, LPARAM lparam )
{
    return -1;
}

static INT CDECL nulldrv_ExtDeviceMode( LPSTR buffer, HWND hwnd, DEVMODEA *output, LPSTR device,
                                        LPSTR port, DEVMODEA *input, LPSTR profile, DWORD mode )
{
    return -1;
}

static INT CDECL nulldrv_ExtEscape( PHYSDEV dev, INT escape, INT in_size, const void *in_data,
                                    INT out_size, void *out_data )
{
    return 0;
}

static BOOL CDECL nulldrv_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT type )
{
    return TRUE;
}

static BOOL CDECL nulldrv_FontIsLinked( PHYSDEV dev )
{
    return FALSE;
}

static BOOL CDECL nulldrv_GdiComment( PHYSDEV dev, UINT size, const BYTE *data )
{
    return FALSE;
}

static UINT CDECL nulldrv_GetBoundsRect( PHYSDEV dev, RECT *rect, UINT flags )
{
    return DCB_RESET;
}

static BOOL CDECL nulldrv_GetCharABCWidths( PHYSDEV dev, UINT first, UINT last, LPABC abc )
{
    return FALSE;
}

static BOOL CDECL nulldrv_GetCharABCWidthsI( PHYSDEV dev, UINT first, UINT count, WORD *indices, LPABC abc )
{
    return FALSE;
}

static BOOL CDECL nulldrv_GetCharWidth( PHYSDEV dev, UINT first, UINT last, INT *buffer )
{
    return FALSE;
}

static BOOL CDECL nulldrv_GetCharWidthInfo( PHYSDEV dev, void *info )
{
    return FALSE;
}

static INT CDECL nulldrv_GetDeviceCaps( PHYSDEV dev, INT cap )
{
    int bpp;

    switch (cap)
    {
    case DRIVERVERSION:   return 0x4000;
    case TECHNOLOGY:      return DT_RASDISPLAY;
    case HORZSIZE:        return MulDiv( GetDeviceCaps( dev->hdc, HORZRES ), 254,
                                         GetDeviceCaps( dev->hdc, LOGPIXELSX ) * 10 );
    case VERTSIZE:        return MulDiv( GetDeviceCaps( dev->hdc, VERTRES ), 254,
                                         GetDeviceCaps( dev->hdc, LOGPIXELSY ) * 10 );
    case HORZRES:         return pGetSystemMetrics ? pGetSystemMetrics( SM_CXSCREEN ) : 640;
    case VERTRES:         return pGetSystemMetrics ? pGetSystemMetrics( SM_CYSCREEN ) : 480;
    case BITSPIXEL:       return 32;
    case PLANES:          return 1;
    case NUMBRUSHES:      return -1;
    case NUMPENS:         return -1;
    case NUMMARKERS:      return 0;
    case NUMFONTS:        return 0;
    case PDEVICESIZE:     return 0;
    case CURVECAPS:       return (CC_CIRCLES | CC_PIE | CC_CHORD | CC_ELLIPSES | CC_WIDE |
                                  CC_STYLED | CC_WIDESTYLED | CC_INTERIORS | CC_ROUNDRECT);
    case LINECAPS:        return (LC_POLYLINE | LC_MARKER | LC_POLYMARKER | LC_WIDE |
                                  LC_STYLED | LC_WIDESTYLED | LC_INTERIORS);
    case POLYGONALCAPS:   return (PC_POLYGON | PC_RECTANGLE | PC_WINDPOLYGON | PC_SCANLINE |
                                  PC_WIDE | PC_STYLED | PC_WIDESTYLED | PC_INTERIORS);
    case TEXTCAPS:        return (TC_OP_CHARACTER | TC_OP_STROKE | TC_CP_STROKE |
                                  TC_CR_ANY | TC_SF_X_YINDEP | TC_SA_DOUBLE | TC_SA_INTEGER |
                                  TC_SA_CONTIN | TC_UA_ABLE | TC_SO_ABLE | TC_RA_ABLE | TC_VA_ABLE);
    case CLIPCAPS:        return CP_RECTANGLE;
    case RASTERCAPS:      return (RC_BITBLT | RC_BITMAP64 | RC_GDI20_OUTPUT | RC_DI_BITMAP | RC_DIBTODEV |
                                  RC_BIGFONT | RC_STRETCHBLT | RC_FLOODFILL | RC_STRETCHDIB | RC_DEVBITS |
                                  (GetDeviceCaps( dev->hdc, SIZEPALETTE ) ? RC_PALETTE : 0));
    case ASPECTX:         return 36;
    case ASPECTY:         return 36;
    case ASPECTXY:        return (int)(hypot( GetDeviceCaps( dev->hdc, ASPECTX ),
                                              GetDeviceCaps( dev->hdc, ASPECTY )) + 0.5);
    case CAPS1:           return 0;
    case SIZEPALETTE:     return 0;
    case NUMRESERVED:     return 20;
    case PHYSICALWIDTH:   return 0;
    case PHYSICALHEIGHT:  return 0;
    case PHYSICALOFFSETX: return 0;
    case PHYSICALOFFSETY: return 0;
    case SCALINGFACTORX:  return 0;
    case SCALINGFACTORY:  return 0;
    case VREFRESH:        return GetDeviceCaps( dev->hdc, TECHNOLOGY ) == DT_RASDISPLAY ? 1 : 0;
    case DESKTOPHORZRES:
        if (GetDeviceCaps( dev->hdc, TECHNOLOGY ) == DT_RASDISPLAY && pGetSystemMetrics)
        {
            DPI_AWARENESS_CONTEXT context;
            UINT ret;
            context = pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
            ret = pGetSystemMetrics( SM_CXVIRTUALSCREEN );
            pSetThreadDpiAwarenessContext( context );
            return ret;
        }
        return GetDeviceCaps( dev->hdc, HORZRES );
    case DESKTOPVERTRES:
        if (GetDeviceCaps( dev->hdc, TECHNOLOGY ) == DT_RASDISPLAY && pGetSystemMetrics)
        {
            DPI_AWARENESS_CONTEXT context;
            UINT ret;
            context = pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
            ret = pGetSystemMetrics( SM_CYVIRTUALSCREEN );
            pSetThreadDpiAwarenessContext( context );
            return ret;
        }
        return GetDeviceCaps( dev->hdc, VERTRES );
    case BLTALIGNMENT:    return 0;
    case SHADEBLENDCAPS:  return 0;
    case COLORMGMTCAPS:   return 0;
    case LOGPIXELSX:
    case LOGPIXELSY:      return get_system_dpi();
    case NUMCOLORS:
        bpp = GetDeviceCaps( dev->hdc, BITSPIXEL );
        return (bpp > 8) ? -1 : (1 << bpp);
    case COLORRES:
        /* The observed correspondence between BITSPIXEL and COLORRES is:
         * BITSPIXEL: 8  -> COLORRES: 18
         * BITSPIXEL: 16 -> COLORRES: 16
         * BITSPIXEL: 24 -> COLORRES: 24
         * BITSPIXEL: 32 -> COLORRES: 24 */
        bpp = GetDeviceCaps( dev->hdc, BITSPIXEL );
        return (bpp <= 8) ? 18 : min( 24, bpp );
    default:
        FIXME("(%p): unsupported capability %d, will return 0\n", dev->hdc, cap );
        return 0;
    }
}

static BOOL CDECL nulldrv_GetDeviceGammaRamp( PHYSDEV dev, void *ramp )
{
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}

static DWORD CDECL nulldrv_GetFontData( PHYSDEV dev, DWORD table, DWORD offset, LPVOID buffer, DWORD length )
{
    return FALSE;
}

static BOOL CDECL nulldrv_GetFontRealizationInfo( PHYSDEV dev, void *info )
{
    return FALSE;
}

static DWORD CDECL nulldrv_GetFontUnicodeRanges( PHYSDEV dev, LPGLYPHSET glyphs )
{
    return 0;
}

static DWORD CDECL nulldrv_GetGlyphIndices( PHYSDEV dev, LPCWSTR str, INT count, LPWORD indices, DWORD flags )
{
    return GDI_ERROR;
}

static DWORD CDECL nulldrv_GetGlyphOutline( PHYSDEV dev, UINT ch, UINT format, LPGLYPHMETRICS metrics,
                                            DWORD size, LPVOID buffer, const MAT2 *mat )
{
    return GDI_ERROR;
}

static BOOL CDECL nulldrv_GetICMProfile( PHYSDEV dev, LPDWORD size, LPWSTR filename )
{
    return FALSE;
}

static DWORD CDECL nulldrv_GetImage( PHYSDEV dev, BITMAPINFO *info, struct gdi_image_bits *bits,
                                     struct bitblt_coords *src )
{
    return ERROR_NOT_SUPPORTED;
}

static DWORD CDECL nulldrv_GetKerningPairs( PHYSDEV dev, DWORD count, LPKERNINGPAIR pairs )
{
    return 0;
}

static UINT CDECL nulldrv_GetOutlineTextMetrics( PHYSDEV dev, UINT size, LPOUTLINETEXTMETRICW otm )
{
    return 0;
}

static UINT CDECL nulldrv_GetTextCharsetInfo( PHYSDEV dev, LPFONTSIGNATURE fs, DWORD flags )
{
    return DEFAULT_CHARSET;
}

static BOOL CDECL nulldrv_GetTextExtentExPoint( PHYSDEV dev, LPCWSTR str, INT count, INT *dx )
{
    return FALSE;
}

static BOOL CDECL nulldrv_GetTextExtentExPointI( PHYSDEV dev, const WORD *indices, INT count, INT *dx )
{
    return FALSE;
}

static INT CDECL nulldrv_GetTextFace( PHYSDEV dev, INT size, LPWSTR name )
{
    INT ret = 0;
    LOGFONTW font;
    DC *dc = get_nulldrv_dc( dev );

    if (GetObjectW( dc->hFont, sizeof(font), &font ))
    {
        ret = strlenW( font.lfFaceName ) + 1;
        if (name)
        {
            lstrcpynW( name, font.lfFaceName, size );
            ret = min( size, ret );
        }
    }
    return ret;
}

static BOOL CDECL nulldrv_GetTextMetrics( PHYSDEV dev, TEXTMETRICW *metrics )
{
    return FALSE;
}

static BOOL CDECL nulldrv_LineTo( PHYSDEV dev, INT x, INT y )
{
    return TRUE;
}

static BOOL CDECL nulldrv_MoveTo( PHYSDEV dev, INT x, INT y )
{
    return TRUE;
}

static BOOL CDECL nulldrv_PaintRgn( PHYSDEV dev, HRGN rgn )
{
    return TRUE;
}

static BOOL CDECL nulldrv_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop )
{
    return TRUE;
}

static BOOL CDECL nulldrv_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                               INT xstart, INT ystart, INT xend, INT yend )
{
    return TRUE;
}

static BOOL CDECL nulldrv_PolyPolygon( PHYSDEV dev, const POINT *points, const INT *counts, UINT polygons )
{
    return TRUE;
}

static BOOL CDECL nulldrv_PolyPolyline( PHYSDEV dev, const POINT *points, const DWORD *counts, DWORD lines )
{
    return TRUE;
}

static BOOL CDECL nulldrv_Polygon( PHYSDEV dev, const POINT *points, INT count )
{
    INT counts[1] = { count };

    return PolyPolygon( dev->hdc, points, counts, 1 );
}

static BOOL CDECL nulldrv_Polyline( PHYSDEV dev, const POINT *points, INT count )
{
    DWORD counts[1] = { count };

    if (count < 0) return FALSE;
    return PolyPolyline( dev->hdc, points, counts, 1 );
}

static DWORD CDECL nulldrv_PutImage( PHYSDEV dev, HRGN clip, BITMAPINFO *info,
                                     const struct gdi_image_bits *bits, struct bitblt_coords *src,
                                     struct bitblt_coords *dst, DWORD rop )
{
    return ERROR_SUCCESS;
}

static UINT CDECL nulldrv_RealizeDefaultPalette( PHYSDEV dev )
{
    return 0;
}

static UINT CDECL nulldrv_RealizePalette( PHYSDEV dev, HPALETTE palette, BOOL primary )
{
    return 0;
}

static BOOL CDECL nulldrv_Rectangle( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    return TRUE;
}

static HDC CDECL nulldrv_ResetDC( PHYSDEV dev, const DEVMODEW *devmode )
{
    return 0;
}

static BOOL CDECL nulldrv_RoundRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                                     INT ell_width, INT ell_height )
{
    return TRUE;
}

static HBITMAP CDECL nulldrv_SelectBitmap( PHYSDEV dev, HBITMAP bitmap )
{
    return bitmap;
}

static HBRUSH CDECL nulldrv_SelectBrush( PHYSDEV dev, HBRUSH brush, const struct brush_pattern *pattern )
{
    return brush;
}

static HPALETTE CDECL nulldrv_SelectPalette( PHYSDEV dev, HPALETTE palette, BOOL bkgnd )
{
    return palette;
}

static HPEN CDECL nulldrv_SelectPen( PHYSDEV dev, HPEN pen, const struct brush_pattern *pattern )
{
    return pen;
}

static INT CDECL nulldrv_SetArcDirection( PHYSDEV dev, INT dir )
{
    return dir;
}

static COLORREF CDECL nulldrv_SetBkColor( PHYSDEV dev, COLORREF color )
{
    return color;
}

static INT CDECL nulldrv_SetBkMode( PHYSDEV dev, INT mode )
{
    return mode;
}

static UINT CDECL nulldrv_SetBoundsRect( PHYSDEV dev, RECT *rect, UINT flags )
{
    return DCB_RESET;
}

static COLORREF CDECL nulldrv_SetDCBrushColor( PHYSDEV dev, COLORREF color )
{
    return color;
}

static COLORREF CDECL nulldrv_SetDCPenColor( PHYSDEV dev, COLORREF color )
{
    return color;
}

static void CDECL nulldrv_SetDeviceClipping( PHYSDEV dev, HRGN rgn )
{
}

static DWORD CDECL nulldrv_SetLayout( PHYSDEV dev, DWORD layout )
{
    return layout;
}

static BOOL CDECL nulldrv_SetDeviceGammaRamp( PHYSDEV dev, void *ramp )
{
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}

static DWORD CDECL nulldrv_SetMapperFlags( PHYSDEV dev, DWORD flags )
{
    return flags;
}

static COLORREF CDECL nulldrv_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color )
{
    return color;
}

static INT CDECL nulldrv_SetPolyFillMode( PHYSDEV dev, INT mode )
{
    return mode;
}

static INT CDECL nulldrv_SetROP2( PHYSDEV dev, INT rop )
{
    return rop;
}

static INT CDECL nulldrv_SetRelAbs( PHYSDEV dev, INT mode )
{
    return mode;
}

static INT CDECL nulldrv_SetStretchBltMode( PHYSDEV dev, INT mode )
{
    return mode;
}

static UINT CDECL nulldrv_SetTextAlign( PHYSDEV dev, UINT align )
{
    return align;
}

static INT CDECL nulldrv_SetTextCharacterExtra( PHYSDEV dev, INT extra )
{
    return extra;
}

static COLORREF CDECL nulldrv_SetTextColor( PHYSDEV dev, COLORREF color )
{
    return color;
}

static BOOL CDECL nulldrv_SetTextJustification( PHYSDEV dev, INT extra, INT breaks )
{
    return TRUE;
}

static INT CDECL nulldrv_StartDoc( PHYSDEV dev, const DOCINFOW *info )
{
    return 0;
}

static INT CDECL nulldrv_StartPage( PHYSDEV dev )
{
    return 1;
}

static BOOL CDECL nulldrv_UnrealizePalette( HPALETTE palette )
{
    return FALSE;
}

static struct opengl_funcs * CDECL nulldrv_wine_get_wgl_driver( PHYSDEV dev, UINT version )
{
    return (void *)-1;
}

static const struct vulkan_funcs * CDECL nulldrv_wine_get_vulkan_driver( PHYSDEV dev, UINT version )
{
    return NULL;
}

const struct gdi_dc_funcs null_driver =
{
    nulldrv_AbortDoc,                   /* pAbortDoc */
    nulldrv_AbortPath,                  /* pAbortPath */
    nulldrv_AlphaBlend,                 /* pAlphaBlend */
    nulldrv_AngleArc,                   /* pAngleArc */
    nulldrv_Arc,                        /* pArc */
    nulldrv_ArcTo,                      /* pArcTo */
    nulldrv_BeginPath,                  /* pBeginPath */
    nulldrv_BlendImage,                 /* pBlendImage */
    nulldrv_Chord,                      /* pChord */
    nulldrv_CloseFigure,                /* pCloseFigure */
    nulldrv_CreateCompatibleDC,         /* pCreateCompatibleDC */
    nulldrv_CreateDC,                   /* pCreateDC */
    nulldrv_DeleteDC,                   /* pDeleteDC */
    nulldrv_DeleteObject,               /* pDeleteObject */
    nulldrv_DeviceCapabilities,         /* pDeviceCapabilities */
    nulldrv_Ellipse,                    /* pEllipse */
    nulldrv_EndDoc,                     /* pEndDoc */
    nulldrv_EndPage,                    /* pEndPage */
    nulldrv_EndPath,                    /* pEndPath */
    nulldrv_EnumFonts,                  /* pEnumFonts */
    nulldrv_EnumICMProfiles,            /* pEnumICMProfiles */
    nulldrv_ExcludeClipRect,            /* pExcludeClipRect */
    nulldrv_ExtDeviceMode,              /* pExtDeviceMode */
    nulldrv_ExtEscape,                  /* pExtEscape */
    nulldrv_ExtFloodFill,               /* pExtFloodFill */
    nulldrv_ExtSelectClipRgn,           /* pExtSelectClipRgn */
    nulldrv_ExtTextOut,                 /* pExtTextOut */
    nulldrv_FillPath,                   /* pFillPath */
    nulldrv_FillRgn,                    /* pFillRgn */
    nulldrv_FlattenPath,                /* pFlattenPath */
    nulldrv_FontIsLinked,               /* pFontIsLinked */
    nulldrv_FrameRgn,                   /* pFrameRgn */
    nulldrv_GdiComment,                 /* pGdiComment */
    nulldrv_GetBoundsRect,              /* pGetBoundsRect */
    nulldrv_GetCharABCWidths,           /* pGetCharABCWidths */
    nulldrv_GetCharABCWidthsI,          /* pGetCharABCWidthsI */
    nulldrv_GetCharWidth,               /* pGetCharWidth */
    nulldrv_GetCharWidthInfo,           /* pGetCharWidthInfo */
    nulldrv_GetDeviceCaps,              /* pGetDeviceCaps */
    nulldrv_GetDeviceGammaRamp,         /* pGetDeviceGammaRamp */
    nulldrv_GetFontData,                /* pGetFontData */
    nulldrv_GetFontRealizationInfo,     /* pGetFontRealizationInfo */
    nulldrv_GetFontUnicodeRanges,       /* pGetFontUnicodeRanges */
    nulldrv_GetGlyphIndices,            /* pGetGlyphIndices */
    nulldrv_GetGlyphOutline,            /* pGetGlyphOutline */
    nulldrv_GetICMProfile,              /* pGetICMProfile */
    nulldrv_GetImage,                   /* pGetImage */
    nulldrv_GetKerningPairs,            /* pGetKerningPairs */
    nulldrv_GetNearestColor,            /* pGetNearestColor */
    nulldrv_GetOutlineTextMetrics,      /* pGetOutlineTextMetrics */
    nulldrv_GetPixel,                   /* pGetPixel */
    nulldrv_GetSystemPaletteEntries,    /* pGetSystemPaletteEntries */
    nulldrv_GetTextCharsetInfo,         /* pGetTextCharsetInfo */
    nulldrv_GetTextExtentExPoint,       /* pGetTextExtentExPoint */
    nulldrv_GetTextExtentExPointI,      /* pGetTextExtentExPointI */
    nulldrv_GetTextFace,                /* pGetTextFace */
    nulldrv_GetTextMetrics,             /* pGetTextMetrics */
    nulldrv_GradientFill,               /* pGradientFill */
    nulldrv_IntersectClipRect,          /* pIntersectClipRect */
    nulldrv_InvertRgn,                  /* pInvertRgn */
    nulldrv_LineTo,                     /* pLineTo */
    nulldrv_ModifyWorldTransform,       /* pModifyWorldTransform */
    nulldrv_MoveTo,                     /* pMoveTo */
    nulldrv_OffsetClipRgn,              /* pOffsetClipRgn */
    nulldrv_OffsetViewportOrgEx,        /* pOffsetViewportOrg */
    nulldrv_OffsetWindowOrgEx,          /* pOffsetWindowOrg */
    nulldrv_PaintRgn,                   /* pPaintRgn */
    nulldrv_PatBlt,                     /* pPatBlt */
    nulldrv_Pie,                        /* pPie */
    nulldrv_PolyBezier,                 /* pPolyBezier */
    nulldrv_PolyBezierTo,               /* pPolyBezierTo */
    nulldrv_PolyDraw,                   /* pPolyDraw */
    nulldrv_PolyPolygon,                /* pPolyPolygon */
    nulldrv_PolyPolyline,               /* pPolyPolyline */
    nulldrv_Polygon,                    /* pPolygon */
    nulldrv_Polyline,                   /* pPolyline */
    nulldrv_PolylineTo,                 /* pPolylineTo */
    nulldrv_PutImage,                   /* pPutImage */
    nulldrv_RealizeDefaultPalette,      /* pRealizeDefaultPalette */
    nulldrv_RealizePalette,             /* pRealizePalette */
    nulldrv_Rectangle,                  /* pRectangle */
    nulldrv_ResetDC,                    /* pResetDC */
    nulldrv_RestoreDC,                  /* pRestoreDC */
    nulldrv_RoundRect,                  /* pRoundRect */
    nulldrv_SaveDC,                     /* pSaveDC */
    nulldrv_ScaleViewportExtEx,         /* pScaleViewportExt */
    nulldrv_ScaleWindowExtEx,           /* pScaleWindowExt */
    nulldrv_SelectBitmap,               /* pSelectBitmap */
    nulldrv_SelectBrush,                /* pSelectBrush */
    nulldrv_SelectClipPath,             /* pSelectClipPath */
    nulldrv_SelectFont,                 /* pSelectFont */
    nulldrv_SelectPalette,              /* pSelectPalette */
    nulldrv_SelectPen,                  /* pSelectPen */
    nulldrv_SetArcDirection,            /* pSetArcDirection */
    nulldrv_SetBkColor,                 /* pSetBkColor */
    nulldrv_SetBkMode,                  /* pSetBkMode */
    nulldrv_SetBoundsRect,              /* pSetBoundsRect */
    nulldrv_SetDCBrushColor,            /* pSetDCBrushColor */
    nulldrv_SetDCPenColor,              /* pSetDCPenColor */
    nulldrv_SetDIBitsToDevice,          /* pSetDIBitsToDevice */
    nulldrv_SetDeviceClipping,          /* pSetDeviceClipping */
    nulldrv_SetDeviceGammaRamp,         /* pSetDeviceGammaRamp */
    nulldrv_SetLayout,                  /* pSetLayout */
    nulldrv_SetMapMode,                 /* pSetMapMode */
    nulldrv_SetMapperFlags,             /* pSetMapperFlags */
    nulldrv_SetPixel,                   /* pSetPixel */
    nulldrv_SetPolyFillMode,            /* pSetPolyFillMode */
    nulldrv_SetROP2,                    /* pSetROP2 */
    nulldrv_SetRelAbs,                  /* pSetRelAbs */
    nulldrv_SetStretchBltMode,          /* pSetStretchBltMode */
    nulldrv_SetTextAlign,               /* pSetTextAlign */
    nulldrv_SetTextCharacterExtra,      /* pSetTextCharacterExtra */
    nulldrv_SetTextColor,               /* pSetTextColor */
    nulldrv_SetTextJustification,       /* pSetTextJustification */
    nulldrv_SetViewportExtEx,           /* pSetViewportExt */
    nulldrv_SetViewportOrgEx,           /* pSetViewportOrg */
    nulldrv_SetWindowExtEx,             /* pSetWindowExt */
    nulldrv_SetWindowOrgEx,             /* pSetWindowOrg */
    nulldrv_SetWorldTransform,          /* pSetWorldTransform */
    nulldrv_StartDoc,                   /* pStartDoc */
    nulldrv_StartPage,                  /* pStartPage */
    nulldrv_StretchBlt,                 /* pStretchBlt */
    nulldrv_StretchDIBits,              /* pStretchDIBits */
    nulldrv_StrokeAndFillPath,          /* pStrokeAndFillPath */
    nulldrv_StrokePath,                 /* pStrokePath */
    nulldrv_UnrealizePalette,           /* pUnrealizePalette */
    nulldrv_WidenPath,                  /* pWidenPath */
    nulldrv_wine_get_wgl_driver,        /* wine_get_wgl_driver */
    nulldrv_wine_get_vulkan_driver,     /* wine_get_vulkan_driver */

    GDI_PRIORITY_NULL_DRV               /* priority */
};


/*****************************************************************************
 *      DRIVER_GetDriverName
 *
 */
BOOL DRIVER_GetDriverName( LPCWSTR device, LPWSTR driver, DWORD size )
{
    static const WCHAR displayW[] = { 'd','i','s','p','l','a','y',0 };
    static const WCHAR devicesW[] = { 'd','e','v','i','c','e','s',0 };
    static const WCHAR empty_strW[] = { 0 };
    WCHAR *p;

    /* display is a special case */
    if (!strcmpiW( device, displayW ) ||
        is_display_device( device ))
    {
        lstrcpynW( driver, displayW, size );
        return TRUE;
    }

    size = GetProfileStringW(devicesW, device, empty_strW, driver, size);
    if(!size) {
        WARN("Unable to find %s in [devices] section of win.ini\n", debugstr_w(device));
        return FALSE;
    }
    p = strchrW(driver, ',');
    if(!p)
    {
        WARN("%s entry in [devices] section of win.ini is malformed.\n", debugstr_w(device));
        return FALSE;
    }
    *p = 0;
    TRACE("Found %s for %s\n", debugstr_w(driver), debugstr_w(device));
    return TRUE;
}


/***********************************************************************
 *           GdiConvertToDevmodeW    (GDI32.@)
 */
DEVMODEW * WINAPI GdiConvertToDevmodeW(const DEVMODEA *dmA)
{
    DEVMODEW *dmW;
    WORD dmW_size, dmA_size;

    dmA_size = dmA->dmSize;

    /* this is the minimal dmSize that XP accepts */
    if (dmA_size < FIELD_OFFSET(DEVMODEA, dmFields))
        return NULL;

    if (dmA_size > sizeof(DEVMODEA))
        dmA_size = sizeof(DEVMODEA);

    dmW_size = dmA_size + CCHDEVICENAME;
    if (dmA_size >= FIELD_OFFSET(DEVMODEA, dmFormName) + CCHFORMNAME)
        dmW_size += CCHFORMNAME;

    dmW = HeapAlloc(GetProcessHeap(), 0, dmW_size + dmA->dmDriverExtra);
    if (!dmW) return NULL;

    MultiByteToWideChar(CP_ACP, 0, (const char*) dmA->dmDeviceName, -1,
                                   dmW->dmDeviceName, CCHDEVICENAME);
    /* copy slightly more, to avoid long computations */
    memcpy(&dmW->dmSpecVersion, &dmA->dmSpecVersion, dmA_size - CCHDEVICENAME);

    if (dmA_size >= FIELD_OFFSET(DEVMODEA, dmFormName) + CCHFORMNAME)
    {
        if (dmA->dmFields & DM_FORMNAME)
            MultiByteToWideChar(CP_ACP, 0, (const char*) dmA->dmFormName, -1,
                                       dmW->dmFormName, CCHFORMNAME);
        else
            dmW->dmFormName[0] = 0;

        if (dmA_size > FIELD_OFFSET(DEVMODEA, dmLogPixels))
            memcpy(&dmW->dmLogPixels, &dmA->dmLogPixels, dmA_size - FIELD_OFFSET(DEVMODEA, dmLogPixels));
    }

    if (dmA->dmDriverExtra)
        memcpy((char *)dmW + dmW_size, (const char *)dmA + dmA_size, dmA->dmDriverExtra);

    dmW->dmSize = dmW_size;

    return dmW;
}


/*****************************************************************************
 *      @ [GDI32.100]
 *
 * This should thunk to 16-bit and simply call the proc with the given args.
 */
INT WINAPI GDI_CallDevInstall16( FARPROC16 lpfnDevInstallProc, HWND hWnd,
                                 LPSTR lpModelName, LPSTR OldPort, LPSTR NewPort )
{
    FIXME("(%p, %p, %s, %s, %s)\n", lpfnDevInstallProc, hWnd, lpModelName, OldPort, NewPort );
    return -1;
}

/*****************************************************************************
 *      @ [GDI32.101]
 *
 * This should load the correct driver for lpszDevice and calls this driver's
 * ExtDeviceModePropSheet proc.
 *
 * Note: The driver calls a callback routine for each property sheet page; these
 * pages are supposed to be filled into the structure pointed to by lpPropSheet.
 * The layout of this structure is:
 *
 * struct
 * {
 *   DWORD  nPages;
 *   DWORD  unknown;
 *   HPROPSHEETPAGE  pages[10];
 * };
 */
INT WINAPI GDI_CallExtDeviceModePropSheet16( HWND hWnd, LPCSTR lpszDevice,
                                             LPCSTR lpszPort, LPVOID lpPropSheet )
{
    FIXME("(%p, %s, %s, %p)\n", hWnd, lpszDevice, lpszPort, lpPropSheet );
    return -1;
}

/*****************************************************************************
 *      @ [GDI32.102]
 *
 * This should load the correct driver for lpszDevice and call this driver's
 * ExtDeviceMode proc.
 *
 * FIXME: convert ExtDeviceMode to unicode in the driver interface
 */
INT WINAPI GDI_CallExtDeviceMode16( HWND hwnd,
                                    LPDEVMODEA lpdmOutput, LPSTR lpszDevice,
                                    LPSTR lpszPort, LPDEVMODEA lpdmInput,
                                    LPSTR lpszProfile, DWORD fwMode )
{
    WCHAR deviceW[300];
    WCHAR bufW[300];
    char buf[300];
    HDC hdc;
    DC *dc;
    INT ret = -1;

    TRACE("(%p, %p, %s, %s, %p, %s, %d)\n",
          hwnd, lpdmOutput, lpszDevice, lpszPort, lpdmInput, lpszProfile, fwMode );

    if (!lpszDevice) return -1;
    if (!MultiByteToWideChar(CP_ACP, 0, lpszDevice, -1, deviceW, 300)) return -1;

    if(!DRIVER_GetDriverName( deviceW, bufW, 300 )) return -1;

    if (!WideCharToMultiByte(CP_ACP, 0, bufW, -1, buf, 300, NULL, NULL)) return -1;

    if (!(hdc = CreateICA( buf, lpszDevice, lpszPort, NULL ))) return -1;

    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pExtDeviceMode );
        ret = physdev->funcs->pExtDeviceMode( buf, hwnd, lpdmOutput, lpszDevice, lpszPort,
                                              lpdmInput, lpszProfile, fwMode );
	release_dc_ptr( dc );
    }
    DeleteDC( hdc );
    return ret;
}

/****************************************************************************
 *      @ [GDI32.103]
 *
 * This should load the correct driver for lpszDevice and calls this driver's
 * AdvancedSetupDialog proc.
 */
INT WINAPI GDI_CallAdvancedSetupDialog16( HWND hwnd, LPSTR lpszDevice,
                                          LPDEVMODEA devin, LPDEVMODEA devout )
{
    TRACE("(%p, %s, %p, %p)\n", hwnd, lpszDevice, devin, devout );
    return -1;
}

/*****************************************************************************
 *      @ [GDI32.104]
 *
 * This should load the correct driver for lpszDevice and calls this driver's
 * DeviceCapabilities proc.
 *
 * FIXME: convert DeviceCapabilities to unicode in the driver interface
 */
DWORD WINAPI GDI_CallDeviceCapabilities16( LPCSTR lpszDevice, LPCSTR lpszPort,
                                           WORD fwCapability, LPSTR lpszOutput,
                                           LPDEVMODEA lpdm )
{
    WCHAR deviceW[300];
    WCHAR bufW[300];
    char buf[300];
    HDC hdc;
    DC *dc;
    INT ret = -1;

    TRACE("(%s, %s, %d, %p, %p)\n", lpszDevice, lpszPort, fwCapability, lpszOutput, lpdm );

    if (!lpszDevice) return -1;
    if (!MultiByteToWideChar(CP_ACP, 0, lpszDevice, -1, deviceW, 300)) return -1;

    if(!DRIVER_GetDriverName( deviceW, bufW, 300 )) return -1;

    if (!WideCharToMultiByte(CP_ACP, 0, bufW, -1, buf, 300, NULL, NULL)) return -1;

    if (!(hdc = CreateICA( buf, lpszDevice, lpszPort, NULL ))) return -1;

    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pDeviceCapabilities );
        ret = physdev->funcs->pDeviceCapabilities( buf, lpszDevice, lpszPort,
                                                   fwCapability, lpszOutput, lpdm );
        release_dc_ptr( dc );
    }
    DeleteDC( hdc );
    return ret;
}


/************************************************************************
 *             Escape  [GDI32.@]
 */
INT WINAPI Escape( HDC hdc, INT escape, INT in_count, LPCSTR in_data, LPVOID out_data )
{
    INT ret;
    POINT *pt;

    switch (escape)
    {
    case ABORTDOC:
        return AbortDoc( hdc );

    case ENDDOC:
        return EndDoc( hdc );

    case GETPHYSPAGESIZE:
        pt = out_data;
        pt->x = GetDeviceCaps( hdc, PHYSICALWIDTH );
        pt->y = GetDeviceCaps( hdc, PHYSICALHEIGHT );
        return 1;

    case GETPRINTINGOFFSET:
        pt = out_data;
        pt->x = GetDeviceCaps( hdc, PHYSICALOFFSETX );
        pt->y = GetDeviceCaps( hdc, PHYSICALOFFSETY );
        return 1;

    case GETSCALINGFACTOR:
        pt = out_data;
        pt->x = GetDeviceCaps( hdc, SCALINGFACTORX );
        pt->y = GetDeviceCaps( hdc, SCALINGFACTORY );
        return 1;

    case NEWFRAME:
        return EndPage( hdc );

    case SETABORTPROC:
        return SetAbortProc( hdc, (ABORTPROC)in_data );

    case STARTDOC:
        {
            DOCINFOA doc;
            char *name = NULL;

            /* in_data may not be 0 terminated so we must copy it */
            if (in_data)
            {
                name = HeapAlloc( GetProcessHeap(), 0, in_count+1 );
                memcpy( name, in_data, in_count );
                name[in_count] = 0;
            }
            /* out_data is actually a pointer to the DocInfo structure and used as
             * a second input parameter */
            if (out_data) doc = *(DOCINFOA *)out_data;
            else
            {
                doc.cbSize = sizeof(doc);
                doc.lpszOutput = NULL;
                doc.lpszDatatype = NULL;
                doc.fwType = 0;
            }
            doc.lpszDocName = name;
            ret = StartDocA( hdc, &doc );
            HeapFree( GetProcessHeap(), 0, name );
            if (ret > 0) ret = StartPage( hdc );
            return ret;
        }

    case QUERYESCSUPPORT:
        {
            DWORD code;

            if (in_count < sizeof(SHORT)) return 0;
            code = (in_count < sizeof(DWORD)) ? *(const USHORT *)in_data : *(const DWORD *)in_data;
            switch (code)
            {
            case ABORTDOC:
            case ENDDOC:
            case GETPHYSPAGESIZE:
            case GETPRINTINGOFFSET:
            case GETSCALINGFACTOR:
            case NEWFRAME:
            case QUERYESCSUPPORT:
            case SETABORTPROC:
            case STARTDOC:
                return TRUE;
            }
            break;
        }
    }

    /* if not handled internally, pass it to the driver */
    return ExtEscape( hdc, escape, in_count, in_data, 0, out_data );
}


/******************************************************************************
 *		ExtEscape	[GDI32.@]
 *
 * Access capabilities of a particular device that are not available through GDI.
 *
 * PARAMS
 *    hdc         [I] Handle to device context
 *    nEscape     [I] Escape function
 *    cbInput     [I] Number of bytes in input structure
 *    lpszInData  [I] Pointer to input structure
 *    cbOutput    [I] Number of bytes in output structure
 *    lpszOutData [O] Pointer to output structure
 *
 * RETURNS
 *    Success: >0
 *    Not implemented: 0
 *    Failure: <0
 */
INT WINAPI ExtEscape( HDC hdc, INT nEscape, INT cbInput, LPCSTR lpszInData,
                      INT cbOutput, LPSTR lpszOutData )
{
    PHYSDEV physdev;
    INT ret;
    DC * dc = get_dc_ptr( hdc );

    if (!dc) return 0;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pExtEscape );
    ret = physdev->funcs->pExtEscape( physdev, nEscape, cbInput, lpszInData, cbOutput, lpszOutData );
    release_dc_ptr( dc );
    return ret;
}


/*******************************************************************
 *      DrawEscape [GDI32.@]
 *
 *
 */
INT WINAPI DrawEscape(HDC hdc, INT nEscape, INT cbInput, LPCSTR lpszInData)
{
    FIXME("DrawEscape, stub\n");
    return 0;
}

/*******************************************************************
 *      NamedEscape [GDI32.@]
 */
INT WINAPI NamedEscape( HDC hdc, LPCWSTR pDriver, INT nEscape, INT cbInput, LPCSTR lpszInData,
                        INT cbOutput, LPSTR lpszOutData )
{
    FIXME("(%p, %s, %d, %d, %p, %d, %p)\n",
          hdc, wine_dbgstr_w(pDriver), nEscape, cbInput, lpszInData, cbOutput,
          lpszOutData);
    return 0;
}

/*******************************************************************
 *      DdQueryDisplaySettingsUniqueness [GDI32.@]
 *      GdiEntry13                       [GDI32.@]
 */
ULONG WINAPI DdQueryDisplaySettingsUniqueness(VOID)
{
    static int warn_once;

    if (!warn_once++)
        FIXME("stub\n");
    return 0;
}

/******************************************************************************
 *		D3DKMTOpenAdapterFromHdc [GDI32.@]
 */
NTSTATUS WINAPI D3DKMTOpenAdapterFromHdc( void *pData )
{
    FIXME("(%p): stub\n", pData);
    return STATUS_NO_MEMORY;
}

/******************************************************************************
 *		D3DKMTEscape [GDI32.@]
 */
NTSTATUS WINAPI D3DKMTEscape( const void *pData )
{
    FIXME("(%p): stub\n", pData);
    return STATUS_NO_MEMORY;
}

/******************************************************************************
 *		D3DKMTCloseAdapter [GDI32.@]
 */
NTSTATUS WINAPI D3DKMTCloseAdapter( const D3DKMT_CLOSEADAPTER *desc )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    struct d3dkmt_adapter *adapter;

    TRACE("(%p)\n", desc);

    if (!desc || !desc->hAdapter)
        return STATUS_INVALID_PARAMETER;

    EnterCriticalSection( &driver_section );
    LIST_FOR_EACH_ENTRY( adapter, &d3dkmt_adapters, struct d3dkmt_adapter, entry )
    {
        if (adapter->handle == desc->hAdapter)
        {
            list_remove( &adapter->entry );
            heap_free( adapter );
            status = STATUS_SUCCESS;
            break;
        }
    }
    LeaveCriticalSection( &driver_section );

    return status;
}

/******************************************************************************
 *		D3DKMTOpenAdapterFromGdiDisplayName [GDI32.@]
 */
NTSTATUS WINAPI D3DKMTOpenAdapterFromGdiDisplayName( D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME *desc )
{
    static const WCHAR display1W[] = {'\\','\\','.','\\','D','I','S','P','L','A','Y','1',0};
    static D3DKMT_HANDLE handle_start = 0;
    struct d3dkmt_adapter *adapter;

    TRACE("(%p) semi-stub\n", desc);

    if (!desc)
        return STATUS_UNSUCCESSFUL;

    /* FIXME: Support multiple monitors */
    if (lstrcmpiW( desc->DeviceName, display1W ))
    {
        FIXME("%s is unsupported\n", wine_dbgstr_w( desc->DeviceName ));
        return STATUS_UNSUCCESSFUL;
    }

    adapter = heap_alloc( sizeof( *adapter ) );
    if (!adapter)
        return STATUS_NO_MEMORY;

    EnterCriticalSection( &driver_section );
    /* D3DKMT_HANDLE is UINT, so we can't use pointer as handle */
    adapter->handle = ++handle_start;
    adapter->ordinal = 0;
    list_add_tail( &d3dkmt_adapters, &adapter->entry );
    LeaveCriticalSection( &driver_section );

    desc->hAdapter = handle_start;
    /* FIXME: Support AdapterLuid */
    desc->AdapterLuid.LowPart = 0;
    desc->AdapterLuid.HighPart = 0;
    desc->VidPnSourceId = 0;
    return STATUS_SUCCESS;
}

/******************************************************************************
 *		D3DKMTCreateDevice [GDI32.@]
 */
NTSTATUS WINAPI D3DKMTCreateDevice( D3DKMT_CREATEDEVICE *desc )
{
    static D3DKMT_HANDLE handle_start = 0;
    struct d3dkmt_adapter *adapter;
    struct d3dkmt_device *device;
    BOOL found = FALSE;

    TRACE("(%p)\n", desc);

    if (!desc)
        return STATUS_INVALID_PARAMETER;

    EnterCriticalSection( &driver_section );
    LIST_FOR_EACH_ENTRY( adapter, &d3dkmt_adapters, struct d3dkmt_adapter, entry )
    {
        if (adapter->handle == desc->hAdapter)
        {
            found = TRUE;
            break;
        }
    }
    LeaveCriticalSection( &driver_section );

    if (!found)
        return STATUS_INVALID_PARAMETER;

    if (desc->Flags.LegacyMode || desc->Flags.RequestVSync || desc->Flags.DisableGpuTimeout)
        FIXME("Flags unsupported.\n");

    device = heap_alloc_zero( sizeof( *device ) );
    if (!device)
        return STATUS_NO_MEMORY;

    EnterCriticalSection( &driver_section );
    device->handle = ++handle_start;
    list_add_tail( &d3dkmt_devices, &device->entry );
    LeaveCriticalSection( &driver_section );

    desc->hDevice = device->handle;
    return STATUS_SUCCESS;
}

/******************************************************************************
 *		D3DKMTDestroyDevice [GDI32.@]
 */
NTSTATUS WINAPI D3DKMTDestroyDevice( const D3DKMT_DESTROYDEVICE *desc )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    struct d3dkmt_device *device;

    TRACE("(%p)\n", desc);

    if (!desc || !desc->hDevice)
        return STATUS_INVALID_PARAMETER;

    EnterCriticalSection( &driver_section );
    LIST_FOR_EACH_ENTRY( device, &d3dkmt_devices, struct d3dkmt_device, entry )
    {
        if (device->handle == desc->hDevice)
        {
            list_remove( &device->entry );
            heap_free( device );
            status = STATUS_SUCCESS;
            break;
        }
    }
    LeaveCriticalSection( &driver_section );

    return status;
}
