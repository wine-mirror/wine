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

#if 0
#pragma makedep unix
#endif

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "wine/winbase16.h"
#include "winuser.h"

#include "ntgdi_private.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(driver);

struct d3dkmt_adapter
{
    D3DKMT_HANDLE handle;               /* Kernel mode graphics adapter handle */
    struct list entry;                  /* List entry */
};

struct d3dkmt_device
{
    D3DKMT_HANDLE handle;               /* Kernel mode graphics device handle*/
    struct list entry;                  /* List entry */
};

static const struct user_driver_funcs lazy_load_driver;

static struct list d3dkmt_adapters = LIST_INIT( d3dkmt_adapters );
static struct list d3dkmt_devices = LIST_INIT( d3dkmt_devices );

static pthread_mutex_t driver_lock = PTHREAD_MUTEX_INITIALIZER;

/**********************************************************************
 *	     get_display_driver
 *
 * Special case for loading the display driver: get the name from the config file
 */
const struct gdi_dc_funcs *get_display_driver(void)
{
    if (user_driver == &lazy_load_driver)
    {
        if (!user_callbacks || !user_callbacks->pGetDesktopWindow() ||
            user_driver == &lazy_load_driver)
        {
            static struct user_driver_funcs empty_funcs;
            WARN( "failed to load the display driver, falling back to null driver\n" );
            __wine_set_display_driver( &empty_funcs, WINE_GDI_DRIVER_VERSION );
        }
    }
    return &user_driver->dc_funcs;
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
    if (!user_driver->dc_funcs.pCreateCompatibleDC) return TRUE;
    return user_driver->dc_funcs.pCreateCompatibleDC( NULL, pdev );
}

static BOOL CDECL nulldrv_CreateDC( PHYSDEV *dev, LPCWSTR device, LPCWSTR output,
                                    const DEVMODEW *devmode )
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

static UINT CDECL nulldrv_GetBoundsRect( PHYSDEV dev, RECT *rect, UINT flags )
{
    return DCB_RESET;
}

static BOOL CDECL nulldrv_GetCharABCWidths( PHYSDEV dev, UINT first, UINT count,
                                            WCHAR *chars, ABC *abc )
{
    return FALSE;
}

static BOOL CDECL nulldrv_GetCharABCWidthsI( PHYSDEV dev, UINT first, UINT count, WORD *indices, LPABC abc )
{
    return FALSE;
}

static BOOL CDECL nulldrv_GetCharWidth( PHYSDEV dev, UINT first, UINT count,
                                        const WCHAR *chars, INT *buffer )
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
    case HORZSIZE:        return muldiv( NtGdiGetDeviceCaps( dev->hdc, HORZRES ), 254,
                                         NtGdiGetDeviceCaps( dev->hdc, LOGPIXELSX ) * 10 );
    case VERTSIZE:        return muldiv( NtGdiGetDeviceCaps( dev->hdc, VERTRES ), 254,
                                         NtGdiGetDeviceCaps( dev->hdc, LOGPIXELSY ) * 10 );
    case HORZRES:
    {
        DC *dc = get_nulldrv_dc( dev );
        RECT rect;
        int ret;

        if (dc->display[0])
        {
            rect = get_display_rect( dc->display );
            if (!IsRectEmpty( &rect )) return rect.right - rect.left;
        }

        ret = get_system_metrics( SM_CXSCREEN );
        return ret ? ret : 640;
    }
    case VERTRES:
    {
        DC *dc = get_nulldrv_dc( dev );
        RECT rect;
        int ret;

        if (dc->display[0])
        {
            rect = get_display_rect( dc->display );
            if (!IsRectEmpty( &rect )) return rect.bottom - rect.top;
        }

        ret = get_system_metrics( SM_CYSCREEN );
        return ret ? ret : 480;
    }
    case BITSPIXEL:
    {
        UNICODE_STRING display;
        DEVMODEW devmode;
        DC *dc;

        if (NtGdiGetDeviceCaps( dev->hdc, TECHNOLOGY ) == DT_RASDISPLAY)
        {
            dc = get_nulldrv_dc( dev );
            memset( &devmode, 0, sizeof(devmode) );
            devmode.dmSize = sizeof(devmode);
            init_unicode_string( &display, dc->display );
            if (NtUserEnumDisplaySettings( &display, ENUM_CURRENT_SETTINGS, &devmode, 0 ) &&
                (devmode.dmFields & DM_BITSPERPEL) && devmode.dmBitsPerPel)
                return devmode.dmBitsPerPel;
        }
        return 32;
    }
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
                                  (NtGdiGetDeviceCaps( dev->hdc, SIZEPALETTE ) ? RC_PALETTE : 0));
    case ASPECTX:         return 36;
    case ASPECTY:         return 36;
    case ASPECTXY:        return (int)(hypot( NtGdiGetDeviceCaps( dev->hdc, ASPECTX ),
                                              NtGdiGetDeviceCaps( dev->hdc, ASPECTY )) + 0.5);
    case CAPS1:           return 0;
    case SIZEPALETTE:     return 0;
    case NUMRESERVED:     return 20;
    case PHYSICALWIDTH:   return 0;
    case PHYSICALHEIGHT:  return 0;
    case PHYSICALOFFSETX: return 0;
    case PHYSICALOFFSETY: return 0;
    case SCALINGFACTORX:  return 0;
    case SCALINGFACTORY:  return 0;
    case VREFRESH:
    {
        UNICODE_STRING display;
        DEVMODEW devmode;
        DC *dc;

        if (NtGdiGetDeviceCaps( dev->hdc, TECHNOLOGY ) != DT_RASDISPLAY)
            return 0;

        dc = get_nulldrv_dc( dev );

        memset( &devmode, 0, sizeof(devmode) );
        devmode.dmSize = sizeof(devmode);
        init_unicode_string( &display, dc->display );
        if (NtUserEnumDisplaySettings( &display, ENUM_CURRENT_SETTINGS, &devmode, 0 ) &&
            devmode.dmDisplayFrequency)
            return devmode.dmDisplayFrequency;
        return 1;
    }
    case DESKTOPHORZRES:
        if (NtGdiGetDeviceCaps( dev->hdc, TECHNOLOGY ) == DT_RASDISPLAY)
        {
            RECT rect = get_virtual_screen_rect( 0 );
            return rect.right - rect.left;
        }
        return NtGdiGetDeviceCaps( dev->hdc, HORZRES );
    case DESKTOPVERTRES:
        if (NtGdiGetDeviceCaps( dev->hdc, TECHNOLOGY ) == DT_RASDISPLAY)
        {
            RECT rect = get_virtual_screen_rect( 0 );
            return rect.bottom - rect.top;
        }
        return NtGdiGetDeviceCaps( dev->hdc, VERTRES );
    case BLTALIGNMENT:    return 0;
    case SHADEBLENDCAPS:  return 0;
    case COLORMGMTCAPS:   return 0;
    case LOGPIXELSX:
    case LOGPIXELSY:      return get_system_dpi();
    case NUMCOLORS:
        bpp = NtGdiGetDeviceCaps( dev->hdc, BITSPIXEL );
        return (bpp > 8) ? -1 : (1 << bpp);
    case COLORRES:
        /* The observed correspondence between BITSPIXEL and COLORRES is:
         * BITSPIXEL: 8  -> COLORRES: 18
         * BITSPIXEL: 16 -> COLORRES: 16
         * BITSPIXEL: 24 -> COLORRES: 24
         * BITSPIXEL: 32 -> COLORRES: 24 */
        bpp = NtGdiGetDeviceCaps( dev->hdc, BITSPIXEL );
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

static BOOL CDECL nulldrv_GetICMProfile( PHYSDEV dev, BOOL allow_default, LPDWORD size, LPWSTR filename )
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

    if (NtGdiExtGetObjectW( dc->hFont, sizeof(font), &font ))
    {
        ret = lstrlenW( font.lfFaceName ) + 1;
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

static BOOL CDECL nulldrv_ResetDC( PHYSDEV dev, const DEVMODEW *devmode )
{
    return FALSE;
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

static HFONT CDECL nulldrv_SelectFont( PHYSDEV dev, HFONT font, UINT *aa_flags )
{
    return font;
}

static HPEN CDECL nulldrv_SelectPen( PHYSDEV dev, HPEN pen, const struct brush_pattern *pattern )
{
    return pen;
}

static COLORREF CDECL nulldrv_SetBkColor( PHYSDEV dev, COLORREF color )
{
    return color;
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

static BOOL CDECL nulldrv_SetDeviceGammaRamp( PHYSDEV dev, void *ramp )
{
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}

static COLORREF CDECL nulldrv_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color )
{
    return color;
}

static COLORREF CDECL nulldrv_SetTextColor( PHYSDEV dev, COLORREF color )
{
    return color;
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

static NTSTATUS CDECL nulldrv_D3DKMTCheckVidPnExclusiveOwnership( const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *desc )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static NTSTATUS CDECL nulldrv_D3DKMTSetVidPnSourceOwner( const D3DKMT_SETVIDPNSOURCEOWNER *desc )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static struct opengl_funcs * CDECL nulldrv_wine_get_wgl_driver( PHYSDEV dev, UINT version )
{
    return (void *)-1;
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
    nulldrv_Ellipse,                    /* pEllipse */
    nulldrv_EndDoc,                     /* pEndDoc */
    nulldrv_EndPage,                    /* pEndPage */
    nulldrv_EndPath,                    /* pEndPath */
    nulldrv_EnumFonts,                  /* pEnumFonts */
    nulldrv_ExtEscape,                  /* pExtEscape */
    nulldrv_ExtFloodFill,               /* pExtFloodFill */
    nulldrv_ExtTextOut,                 /* pExtTextOut */
    nulldrv_FillPath,                   /* pFillPath */
    nulldrv_FillRgn,                    /* pFillRgn */
    nulldrv_FontIsLinked,               /* pFontIsLinked */
    nulldrv_FrameRgn,                   /* pFrameRgn */
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
    nulldrv_InvertRgn,                  /* pInvertRgn */
    nulldrv_LineTo,                     /* pLineTo */
    nulldrv_MoveTo,                     /* pMoveTo */
    nulldrv_PaintRgn,                   /* pPaintRgn */
    nulldrv_PatBlt,                     /* pPatBlt */
    nulldrv_Pie,                        /* pPie */
    nulldrv_PolyBezier,                 /* pPolyBezier */
    nulldrv_PolyBezierTo,               /* pPolyBezierTo */
    nulldrv_PolyDraw,                   /* pPolyDraw */
    nulldrv_PolyPolygon,                /* pPolyPolygon */
    nulldrv_PolyPolyline,               /* pPolyPolyline */
    nulldrv_PolylineTo,                 /* pPolylineTo */
    nulldrv_PutImage,                   /* pPutImage */
    nulldrv_RealizeDefaultPalette,      /* pRealizeDefaultPalette */
    nulldrv_RealizePalette,             /* pRealizePalette */
    nulldrv_Rectangle,                  /* pRectangle */
    nulldrv_ResetDC,                    /* pResetDC */
    nulldrv_RoundRect,                  /* pRoundRect */
    nulldrv_SelectBitmap,               /* pSelectBitmap */
    nulldrv_SelectBrush,                /* pSelectBrush */
    nulldrv_SelectFont,                 /* pSelectFont */
    nulldrv_SelectPen,                  /* pSelectPen */
    nulldrv_SetBkColor,                 /* pSetBkColor */
    nulldrv_SetBoundsRect,              /* pSetBoundsRect */
    nulldrv_SetDCBrushColor,            /* pSetDCBrushColor */
    nulldrv_SetDCPenColor,              /* pSetDCPenColor */
    nulldrv_SetDIBitsToDevice,          /* pSetDIBitsToDevice */
    nulldrv_SetDeviceClipping,          /* pSetDeviceClipping */
    nulldrv_SetDeviceGammaRamp,         /* pSetDeviceGammaRamp */
    nulldrv_SetPixel,                   /* pSetPixel */
    nulldrv_SetTextColor,               /* pSetTextColor */
    nulldrv_StartDoc,                   /* pStartDoc */
    nulldrv_StartPage,                  /* pStartPage */
    nulldrv_StretchBlt,                 /* pStretchBlt */
    nulldrv_StretchDIBits,              /* pStretchDIBits */
    nulldrv_StrokeAndFillPath,          /* pStrokeAndFillPath */
    nulldrv_StrokePath,                 /* pStrokePath */
    nulldrv_UnrealizePalette,           /* pUnrealizePalette */
    nulldrv_D3DKMTCheckVidPnExclusiveOwnership, /* pD3DKMTCheckVidPnExclusiveOwnership */
    nulldrv_D3DKMTSetVidPnSourceOwner,  /* pD3DKMTSetVidPnSourceOwner */
    nulldrv_wine_get_wgl_driver,        /* wine_get_wgl_driver */

    GDI_PRIORITY_NULL_DRV               /* priority */
};


/**********************************************************************
 * Null user driver
 *
 * These are fallbacks for entry points that are not implemented in the real driver.
 */

static BOOL CDECL nulldrv_ActivateKeyboardLayout( HKL layout, UINT flags )
{
    return TRUE;
}

static void CDECL nulldrv_Beep(void)
{
}

static UINT CDECL nulldrv_GetKeyboardLayoutList( INT size, HKL *layouts )
{
    return ~0; /* use default implementation */
}

static INT CDECL nulldrv_GetKeyNameText( LONG lparam, LPWSTR buffer, INT size )
{
    return -1; /* use default implementation */
}

static UINT CDECL nulldrv_MapVirtualKeyEx( UINT code, UINT type, HKL layout )
{
    return -1; /* use default implementation */
}

static BOOL CDECL nulldrv_RegisterHotKey( HWND hwnd, UINT modifiers, UINT vk )
{
    return TRUE;
}

static INT CDECL nulldrv_ToUnicodeEx( UINT virt, UINT scan, const BYTE *state, LPWSTR str,
                                      int size, UINT flags, HKL layout )
{
    return -2; /* use default implementation */
}

static void CDECL nulldrv_UnregisterHotKey( HWND hwnd, UINT modifiers, UINT vk )
{
}

static SHORT CDECL nulldrv_VkKeyScanEx( WCHAR ch, HKL layout )
{
    return -256; /* use default implementation */
}

static void CDECL nulldrv_DestroyCursorIcon( HCURSOR cursor )
{
}

static void CDECL nulldrv_SetCursor( HCURSOR cursor )
{
}

static BOOL CDECL nulldrv_GetCursorPos( LPPOINT pt )
{
    return TRUE;
}

static BOOL CDECL nulldrv_SetCursorPos( INT x, INT y )
{
    return TRUE;
}

static BOOL CDECL nulldrv_ClipCursor( LPCRECT clip )
{
    return TRUE;
}

static void CDECL nulldrv_UpdateClipboard(void)
{
}

static LONG CDECL nulldrv_ChangeDisplaySettingsEx( LPCWSTR name, LPDEVMODEW mode, HWND hwnd,
                                                   DWORD flags, LPVOID lparam )
{
    return DISP_CHANGE_FAILED;
}

static BOOL CDECL nulldrv_EnumDisplaySettingsEx( LPCWSTR name, DWORD num, LPDEVMODEW mode, DWORD flags )
{
    return FALSE;
}

static void CDECL nulldrv_UpdateDisplayDevices( const struct gdi_device_manager *manager,
                                                BOOL force, void *param )
{
    manager->add_monitor( NULL, param ); /* use virtual monitor */
}

static BOOL CDECL nulldrv_CreateDesktopWindow( HWND hwnd )
{
    return TRUE;
}

static BOOL CDECL nulldrv_CreateWindow( HWND hwnd )
{
    return TRUE;
}

static void CDECL nulldrv_DestroyWindow( HWND hwnd )
{
}

static void CDECL nulldrv_FlashWindowEx( FLASHWINFO *info )
{
}

static void CDECL nulldrv_GetDC( HDC hdc, HWND hwnd, HWND top_win, const RECT *win_rect,
                                 const RECT *top_rect, DWORD flags )
{
}

/* helper for kernel32->ntdll timeout format conversion */
static inline LARGE_INTEGER *get_nt_timeout( LARGE_INTEGER *time, DWORD timeout )
{
    if (timeout == INFINITE) return NULL;
    time->QuadPart = (ULONGLONG)timeout * -10000;
    return time;
}

static HANDLE normalize_std_handle( HANDLE handle )
{
    if (handle == (HANDLE)STD_INPUT_HANDLE)
        return NtCurrentTeb()->Peb->ProcessParameters->hStdInput;
    if (handle == (HANDLE)STD_OUTPUT_HANDLE)
        return NtCurrentTeb()->Peb->ProcessParameters->hStdOutput;
    if (handle == (HANDLE)STD_ERROR_HANDLE)
        return NtCurrentTeb()->Peb->ProcessParameters->hStdError;

    return handle;
}

static DWORD CDECL nulldrv_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles, DWORD timeout,
                                                        DWORD mask, DWORD flags )
{
    NTSTATUS status;
    HANDLE hloc[MAXIMUM_WAIT_OBJECTS];
    LARGE_INTEGER time;
    unsigned int i;

    if (!count && !timeout) return WAIT_TIMEOUT;
    if (count > MAXIMUM_WAIT_OBJECTS)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return WAIT_FAILED;
    }
    for (i = 0; i < count; i++) hloc[i] = normalize_std_handle( handles[i] );

    status = NtWaitForMultipleObjects( count, hloc, !(flags & MWMO_WAITALL), !!(flags & MWMO_ALERTABLE),
                                       get_nt_timeout( &time, timeout ) );
    if (HIWORD(status))  /* is it an error code? */
    {
        SetLastError( RtlNtStatusToDosError(status) );
        status = WAIT_FAILED;
    }
    return status;
}

static void CDECL nulldrv_ReleaseDC( HWND hwnd, HDC hdc )
{
}

static BOOL CDECL nulldrv_ScrollDC( HDC hdc, INT dx, INT dy, HRGN update )
{
    RECT rect;

    NtGdiGetAppClipBox( hdc, &rect );
    return NtGdiBitBlt( hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                        hdc, rect.left - dx, rect.top - dy, SRCCOPY, 0, 0 );
}

static void CDECL nulldrv_SetCapture( HWND hwnd, UINT flags )
{
}

static void CDECL nulldrv_SetFocus( HWND hwnd )
{
}

static void CDECL nulldrv_SetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags )
{
}

static void CDECL nulldrv_SetParent( HWND hwnd, HWND parent, HWND old_parent )
{
}

static void CDECL nulldrv_SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
}

static void CDECL nulldrv_SetWindowIcon( HWND hwnd, UINT type, HICON icon )
{
}

static void CDECL nulldrv_SetWindowStyle( HWND hwnd, INT offset, STYLESTRUCT *style )
{
}

static void CDECL nulldrv_SetWindowText( HWND hwnd, LPCWSTR text )
{
}

static UINT CDECL nulldrv_ShowWindow( HWND hwnd, INT cmd, RECT *rect, UINT swp )
{
    return ~0; /* use default implementation */
}

static LRESULT CDECL nulldrv_SysCommand( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    return -1;
}

static BOOL CDECL nulldrv_UpdateLayeredWindow( HWND hwnd, const UPDATELAYEREDWINDOWINFO *info,
                                               const RECT *window_rect )
{
    return TRUE;
}

static LRESULT CDECL nulldrv_WindowMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return 0;
}

static BOOL CDECL nulldrv_WindowPosChanging( HWND hwnd, HWND insert_after, UINT swp_flags,
                                             const RECT *window_rect, const RECT *client_rect,
                                             RECT *visible_rect, struct window_surface **surface )
{
    return FALSE;
}

static void CDECL nulldrv_WindowPosChanged( HWND hwnd, HWND insert_after, UINT swp_flags,
                                            const RECT *window_rect, const RECT *client_rect,
                                            const RECT *visible_rect, const RECT *valid_rects,
                                            struct window_surface *surface )
{
}

static BOOL CDECL nulldrv_SystemParametersInfo( UINT action, UINT int_param, void *ptr_param, UINT flags )
{
    return FALSE;
}

static const struct vulkan_funcs * CDECL nulldrv_wine_get_vulkan_driver( UINT version )
{
    return NULL;
}

static void CDECL nulldrv_ThreadDetach( void )
{
}

/**********************************************************************
 * Lazy loading user driver
 *
 * Initial driver used before another driver is loaded.
 * Each entry point simply loads the real driver and chains to it.
 */

static const struct user_driver_funcs *load_driver(void)
{
    get_display_driver();
    return user_driver;
}

static BOOL CDECL loaderdrv_ActivateKeyboardLayout( HKL layout, UINT flags )
{
    return load_driver()->pActivateKeyboardLayout( layout, flags );
}

static INT CDECL loaderdrv_GetKeyNameText( LONG lparam, LPWSTR buffer, INT size )
{
    return load_driver()->pGetKeyNameText( lparam, buffer, size );
}

static UINT CDECL loaderdrv_GetKeyboardLayoutList( INT size, HKL *layouts )
{
    return load_driver()->pGetKeyboardLayoutList( size, layouts );
}

static UINT CDECL loaderdrv_MapVirtualKeyEx( UINT code, UINT type, HKL layout )
{
    return load_driver()->pMapVirtualKeyEx( code, type, layout );
}

static INT CDECL loaderdrv_ToUnicodeEx( UINT virt, UINT scan, const BYTE *state, LPWSTR str,
                                        int size, UINT flags, HKL layout )
{
    return load_driver()->pToUnicodeEx( virt, scan, state, str, size, flags, layout );
}

static void CDECL loaderdrv_UnregisterHotKey( HWND hwnd, UINT modifiers, UINT vk )
{
    load_driver()->pUnregisterHotKey( hwnd, modifiers, vk );
}

static SHORT CDECL loaderdrv_VkKeyScanEx( WCHAR ch, HKL layout )
{
    return load_driver()->pVkKeyScanEx( ch, layout );
}

static LONG CDECL loaderdrv_ChangeDisplaySettingsEx( LPCWSTR name, LPDEVMODEW mode, HWND hwnd,
                                                     DWORD flags, LPVOID lparam )
{
    return load_driver()->pChangeDisplaySettingsEx( name, mode, hwnd, flags, lparam );
}

static BOOL CDECL loaderdrv_EnumDisplaySettingsEx( LPCWSTR name, DWORD num, LPDEVMODEW mode, DWORD flags )
{
    return load_driver()->pEnumDisplaySettingsEx( name, num, mode, flags );
}

static void CDECL loaderdrv_SetCursor( HCURSOR cursor )
{
    load_driver()->pSetCursor( cursor );
}

static void CDECL loaderdrv_UpdateClipboard(void)
{
    load_driver()->pUpdateClipboard();
}

static void CDECL loaderdrv_UpdateDisplayDevices( const struct gdi_device_manager *manager,
                                                  BOOL force, void *param )
{
    load_driver()->pUpdateDisplayDevices( manager, force, param );
}

static const struct vulkan_funcs * CDECL loaderdrv_wine_get_vulkan_driver( UINT version )
{
    return load_driver()->pwine_get_vulkan_driver( version );
}

static const struct user_driver_funcs lazy_load_driver =
{
    .pActivateKeyboardLayout = loaderdrv_ActivateKeyboardLayout,
    .pGetKeyNameText = loaderdrv_GetKeyNameText,
    .pGetKeyboardLayoutList = loaderdrv_GetKeyboardLayoutList,
    .pMapVirtualKeyEx = loaderdrv_MapVirtualKeyEx,
    .pToUnicodeEx = loaderdrv_ToUnicodeEx,
    .pUnregisterHotKey = loaderdrv_UnregisterHotKey,
    .pVkKeyScanEx = loaderdrv_VkKeyScanEx,
    .pChangeDisplaySettingsEx = loaderdrv_ChangeDisplaySettingsEx,
    .pEnumDisplaySettingsEx = loaderdrv_EnumDisplaySettingsEx,
    .pUpdateDisplayDevices = loaderdrv_UpdateDisplayDevices,
    .pSetCursor = loaderdrv_SetCursor,
    .pUpdateClipboard = loaderdrv_UpdateClipboard,
    .pScrollDC = nulldrv_ScrollDC,
    .pSystemParametersInfo = nulldrv_SystemParametersInfo,
    .pwine_get_vulkan_driver = loaderdrv_wine_get_vulkan_driver,
};

const struct user_driver_funcs *user_driver = &lazy_load_driver;

/******************************************************************************
 *	     __wine_set_display_driver   (win32u.@)
 */
void CDECL __wine_set_display_driver( struct user_driver_funcs *driver, UINT version )
{
    if (version != WINE_GDI_DRIVER_VERSION)
    {
        ERR( "version mismatch, driver wants %u but win32u has %u\n",
             version, WINE_GDI_DRIVER_VERSION );
        return;
    }

#define SET_USER_FUNC(name) \
    do { if (!driver->p##name) driver->p##name = nulldrv_##name; } while(0)

    SET_USER_FUNC(ActivateKeyboardLayout);
    SET_USER_FUNC(Beep);
    SET_USER_FUNC(GetKeyNameText);
    SET_USER_FUNC(GetKeyboardLayoutList);
    SET_USER_FUNC(MapVirtualKeyEx);
    SET_USER_FUNC(RegisterHotKey);
    SET_USER_FUNC(ToUnicodeEx);
    SET_USER_FUNC(UnregisterHotKey);
    SET_USER_FUNC(VkKeyScanEx);
    SET_USER_FUNC(DestroyCursorIcon);
    SET_USER_FUNC(SetCursor);
    SET_USER_FUNC(GetCursorPos);
    SET_USER_FUNC(SetCursorPos);
    SET_USER_FUNC(ClipCursor);
    SET_USER_FUNC(UpdateClipboard);
    SET_USER_FUNC(ChangeDisplaySettingsEx);
    SET_USER_FUNC(EnumDisplaySettingsEx);
    SET_USER_FUNC(UpdateDisplayDevices);
    SET_USER_FUNC(CreateDesktopWindow);
    SET_USER_FUNC(CreateWindow);
    SET_USER_FUNC(DestroyWindow);
    SET_USER_FUNC(FlashWindowEx);
    SET_USER_FUNC(GetDC);
    SET_USER_FUNC(MsgWaitForMultipleObjectsEx);
    SET_USER_FUNC(ReleaseDC);
    SET_USER_FUNC(ScrollDC);
    SET_USER_FUNC(SetCapture);
    SET_USER_FUNC(SetFocus);
    SET_USER_FUNC(SetLayeredWindowAttributes);
    SET_USER_FUNC(SetParent);
    SET_USER_FUNC(SetWindowRgn);
    SET_USER_FUNC(SetWindowIcon);
    SET_USER_FUNC(SetWindowStyle);
    SET_USER_FUNC(SetWindowText);
    SET_USER_FUNC(ShowWindow);
    SET_USER_FUNC(SysCommand);
    SET_USER_FUNC(UpdateLayeredWindow);
    SET_USER_FUNC(WindowMessage);
    SET_USER_FUNC(WindowPosChanging);
    SET_USER_FUNC(WindowPosChanged);
    SET_USER_FUNC(SystemParametersInfo);
    SET_USER_FUNC(wine_get_vulkan_driver);
    SET_USER_FUNC(ThreadDetach);
#undef SET_USER_FUNC

    InterlockedExchangePointer( (void **)&user_driver, driver );
}

/******************************************************************************
 *		NtGdiExtEscape   (win32u.@)
 *
 * Access capabilities of a particular device that are not available through GDI.
 */
INT WINAPI NtGdiExtEscape( HDC hdc, WCHAR *driver, int driver_id, INT escape, INT input_size,
                           const char *input, INT output_size, char *output )
{
    PHYSDEV physdev;
    INT ret;
    DC * dc = get_dc_ptr( hdc );

    if (!dc) return 0;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pExtEscape );
    ret = physdev->funcs->pExtEscape( physdev, escape, input_size, input, output_size, output );
    release_dc_ptr( dc );
    return ret;
}


/******************************************************************************
 *           NtGdiDdDDIOpenAdapterFromHdc    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenAdapterFromHdc( D3DKMT_OPENADAPTERFROMHDC *desc )
{
    FIXME( "(%p): stub\n", desc );
    return STATUS_NO_MEMORY;
}

/******************************************************************************
 *           NtGdiDdDDIEscape    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIEscape( const D3DKMT_ESCAPE *desc )
{
    FIXME( "(%p): stub\n", desc );
    return STATUS_NO_MEMORY;
}

/******************************************************************************
 *           NtGdiDdDDICloseAdapter    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICloseAdapter( const D3DKMT_CLOSEADAPTER *desc )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    struct d3dkmt_adapter *adapter;

    TRACE("(%p)\n", desc);

    if (!desc || !desc->hAdapter)
        return STATUS_INVALID_PARAMETER;

    pthread_mutex_lock( &driver_lock );
    LIST_FOR_EACH_ENTRY( adapter, &d3dkmt_adapters, struct d3dkmt_adapter, entry )
    {
        if (adapter->handle == desc->hAdapter)
        {
            list_remove( &adapter->entry );
            free( adapter );
            status = STATUS_SUCCESS;
            break;
        }
    }
    pthread_mutex_unlock( &driver_lock );

    return status;
}

/******************************************************************************
 *           NtGdiDdDDIOpenAdapterFromDeviceName    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenAdapterFromDeviceName( D3DKMT_OPENADAPTERFROMDEVICENAME *desc )
{
    D3DKMT_OPENADAPTERFROMLUID desc_luid;
    NTSTATUS status;

    FIXME( "desc %p stub.\n", desc );

    if (!desc || !desc->pDeviceName) return STATUS_INVALID_PARAMETER;

    memset( &desc_luid, 0, sizeof( desc_luid ));
    if ((status = NtGdiDdDDIOpenAdapterFromLuid( &desc_luid ))) return status;

    desc->AdapterLuid = desc_luid.AdapterLuid;
    desc->hAdapter = desc_luid.hAdapter;
    return STATUS_SUCCESS;
}

/******************************************************************************
 *           NtGdiDdDDIOpenAdapterFromLuid    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIOpenAdapterFromLuid( D3DKMT_OPENADAPTERFROMLUID *desc )
{
    static D3DKMT_HANDLE handle_start = 0;
    struct d3dkmt_adapter *adapter;

    if (!(adapter = malloc( sizeof( *adapter ) ))) return STATUS_NO_MEMORY;

    pthread_mutex_lock( &driver_lock );
    desc->hAdapter = adapter->handle = ++handle_start;
    list_add_tail( &d3dkmt_adapters, &adapter->entry );
    pthread_mutex_unlock( &driver_lock );
    return STATUS_SUCCESS;
}

/******************************************************************************
 *           NtGdiDdDDICreateDevice    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICreateDevice( D3DKMT_CREATEDEVICE *desc )
{
    static D3DKMT_HANDLE handle_start = 0;
    struct d3dkmt_adapter *adapter;
    struct d3dkmt_device *device;
    BOOL found = FALSE;

    TRACE("(%p)\n", desc);

    if (!desc)
        return STATUS_INVALID_PARAMETER;

    pthread_mutex_lock( &driver_lock );
    LIST_FOR_EACH_ENTRY( adapter, &d3dkmt_adapters, struct d3dkmt_adapter, entry )
    {
        if (adapter->handle == desc->hAdapter)
        {
            found = TRUE;
            break;
        }
    }
    pthread_mutex_unlock( &driver_lock );

    if (!found)
        return STATUS_INVALID_PARAMETER;

    if (desc->Flags.LegacyMode || desc->Flags.RequestVSync || desc->Flags.DisableGpuTimeout)
        FIXME("Flags unsupported.\n");

    device = calloc( 1, sizeof( *device ) );
    if (!device)
        return STATUS_NO_MEMORY;

    pthread_mutex_lock( &driver_lock );
    device->handle = ++handle_start;
    list_add_tail( &d3dkmt_devices, &device->entry );
    pthread_mutex_unlock( &driver_lock );

    desc->hDevice = device->handle;
    return STATUS_SUCCESS;
}

/******************************************************************************
 *           NtGdiDdDDIDestroyDevice    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIDestroyDevice( const D3DKMT_DESTROYDEVICE *desc )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    D3DKMT_SETVIDPNSOURCEOWNER set_owner_desc;
    struct d3dkmt_device *device;

    TRACE("(%p)\n", desc);

    if (!desc || !desc->hDevice)
        return STATUS_INVALID_PARAMETER;

    pthread_mutex_lock( &driver_lock );
    LIST_FOR_EACH_ENTRY( device, &d3dkmt_devices, struct d3dkmt_device, entry )
    {
        if (device->handle == desc->hDevice)
        {
            memset( &set_owner_desc, 0, sizeof(set_owner_desc) );
            set_owner_desc.hDevice = desc->hDevice;
            NtGdiDdDDISetVidPnSourceOwner( &set_owner_desc );
            list_remove( &device->entry );
            free( device );
            status = STATUS_SUCCESS;
            break;
        }
    }
    pthread_mutex_unlock( &driver_lock );

    return status;
}

/******************************************************************************
 *           NtGdiDdDDIQueryStatistics    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIQueryStatistics( D3DKMT_QUERYSTATISTICS *stats )
{
    FIXME("(%p): stub\n", stats);
    return STATUS_SUCCESS;
}

/******************************************************************************
 *           NtGdiDdDDISetQueuedLimit    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDISetQueuedLimit( D3DKMT_SETQUEUEDLIMIT *desc )
{
    FIXME( "(%p): stub\n", desc );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *           NtGdiDdDDISetVidPnSourceOwner    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDISetVidPnSourceOwner( const D3DKMT_SETVIDPNSOURCEOWNER *desc )
{
    TRACE("(%p)\n", desc);

    if (!get_display_driver()->pD3DKMTSetVidPnSourceOwner)
        return STATUS_PROCEDURE_NOT_FOUND;

    if (!desc || !desc->hDevice || (desc->VidPnSourceCount && (!desc->pType || !desc->pVidPnSourceId)))
        return STATUS_INVALID_PARAMETER;

    /* Store the VidPN source ownership info in the graphics driver because
     * the graphics driver needs to change ownership sometimes. For example,
     * when a new window is moved to a VidPN source with an exclusive owner,
     * such an exclusive owner will be released before showing the new window */
    return get_display_driver()->pD3DKMTSetVidPnSourceOwner( desc );
}

/******************************************************************************
 *           NtGdiDdDDICheckVidPnExclusiveOwnership    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICheckVidPnExclusiveOwnership( const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *desc )
{
    TRACE("(%p)\n", desc);

    if (!get_display_driver()->pD3DKMTCheckVidPnExclusiveOwnership)
        return STATUS_PROCEDURE_NOT_FOUND;

    if (!desc || !desc->hAdapter)
        return STATUS_INVALID_PARAMETER;

    return get_display_driver()->pD3DKMTCheckVidPnExclusiveOwnership( desc );
}

/***********************************************************************
 *      __wine_get_wgl_driver  (win32u.@)
 */
struct opengl_funcs * CDECL __wine_get_wgl_driver( HDC hdc, UINT version )
{
    struct opengl_funcs *ret = NULL;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, wine_get_wgl_driver );
        ret = physdev->funcs->wine_get_wgl_driver( physdev, version );
        release_dc_ptr( dc );
    }
    return ret;
}
