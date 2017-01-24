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
#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "ntgdi_private.h"
#include "ntuser_private.h"
#include "wine/winbase16.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(driver);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static const struct user_driver_funcs lazy_load_driver;
static struct user_driver_funcs null_user_driver;
static WCHAR driver_load_error[80];

static INT nulldrv_AbortDoc( PHYSDEV dev )
{
    return 0;
}

static BOOL nulldrv_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                         INT xstart, INT ystart, INT xend, INT yend )
{
    return TRUE;
}

static BOOL nulldrv_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                           INT xstart, INT ystart, INT xend, INT yend )
{
    return TRUE;
}

static BOOL nulldrv_CreateCompatibleDC( PHYSDEV orig, PHYSDEV *pdev )
{
    if (!user_driver->dc_funcs.pCreateCompatibleDC) return TRUE;
    return user_driver->dc_funcs.pCreateCompatibleDC( NULL, pdev );
}

static BOOL nulldrv_CreateDC( PHYSDEV *dev, LPCWSTR device, LPCWSTR output,
                              const DEVMODEW *devmode )
{
    assert(0);  /* should never be called */
    return FALSE;
}

static BOOL nulldrv_DeleteDC( PHYSDEV dev )
{
    assert(0);  /* should never be called */
    return TRUE;
}

static BOOL nulldrv_DeleteObject( PHYSDEV dev, HGDIOBJ obj )
{
    return TRUE;
}

static BOOL nulldrv_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    return TRUE;
}

static INT nulldrv_EndDoc( PHYSDEV dev )
{
    return 0;
}

static INT nulldrv_EndPage( PHYSDEV dev )
{
    return 0;
}

static BOOL nulldrv_EnumFonts( PHYSDEV dev, LOGFONTW *logfont, font_enum_proc proc, LPARAM lParam )
{
    return TRUE;
}

static INT nulldrv_ExtEscape( PHYSDEV dev, INT escape, INT in_size, const void *in_data,
                              INT out_size, void *out_data )
{
    return 0;
}

static BOOL nulldrv_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT type )
{
    return TRUE;
}

static BOOL nulldrv_FontIsLinked( PHYSDEV dev )
{
    return FALSE;
}

static UINT nulldrv_GetBoundsRect( PHYSDEV dev, RECT *rect, UINT flags )
{
    return DCB_RESET;
}

static BOOL nulldrv_GetCharABCWidths( PHYSDEV dev, UINT first, UINT count, WCHAR *chars, ABC *abc )
{
    return FALSE;
}

static BOOL nulldrv_GetCharABCWidthsI( PHYSDEV dev, UINT first, UINT count, WORD *indices, LPABC abc )
{
    return FALSE;
}

static BOOL nulldrv_GetCharWidth( PHYSDEV dev, UINT first, UINT count,
                                  const WCHAR *chars, INT *buffer )
{
    return FALSE;
}

static BOOL nulldrv_GetCharWidthInfo( PHYSDEV dev, void *info )
{
    return FALSE;
}

static INT nulldrv_GetDeviceCaps( PHYSDEV dev, INT cap )
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
        DC *dc;

        if (NtGdiGetDeviceCaps( dev->hdc, TECHNOLOGY ) == DT_RASDISPLAY)
        {
            dc = get_nulldrv_dc( dev );
            RtlInitUnicodeString( &display, dc->display );
            return get_display_depth( &display );
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
        RtlInitUnicodeString( &display, dc->display );
        if (NtUserEnumDisplaySettings( &display, ENUM_CURRENT_SETTINGS, &devmode, 0 ) &&
            devmode.dmDisplayFrequency)
            return devmode.dmDisplayFrequency;
        return 1;
    }
    case DESKTOPHORZRES:
        if (NtGdiGetDeviceCaps( dev->hdc, TECHNOLOGY ) == DT_RASDISPLAY)
        {
            RECT rect = get_virtual_screen_rect( 0, MDT_DEFAULT );
            return rect.right - rect.left;
        }
        return NtGdiGetDeviceCaps( dev->hdc, HORZRES );
    case DESKTOPVERTRES:
        if (NtGdiGetDeviceCaps( dev->hdc, TECHNOLOGY ) == DT_RASDISPLAY)
        {
            RECT rect = get_virtual_screen_rect( 0, MDT_DEFAULT );
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
        /* Newer versions of Windows return -1 for 8-bit and higher */
        return (bpp > 4) ? -1 : (1 << bpp);
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

static BOOL nulldrv_GetDeviceGammaRamp( PHYSDEV dev, void *ramp )
{
    RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
    return FALSE;
}

static DWORD nulldrv_GetFontData( PHYSDEV dev, DWORD table, DWORD offset, LPVOID buffer, DWORD length )
{
    return FALSE;
}

static BOOL nulldrv_GetFontRealizationInfo( PHYSDEV dev, void *info )
{
    return FALSE;
}

static DWORD nulldrv_GetFontUnicodeRanges( PHYSDEV dev, LPGLYPHSET glyphs )
{
    return 0;
}

static DWORD nulldrv_GetGlyphIndices( PHYSDEV dev, LPCWSTR str, INT count, LPWORD indices, DWORD flags )
{
    return GDI_ERROR;
}

static DWORD nulldrv_GetGlyphOutline( PHYSDEV dev, UINT ch, UINT format, LPGLYPHMETRICS metrics,
                                      DWORD size, LPVOID buffer, const MAT2 *mat )
{
    return GDI_ERROR;
}

static BOOL nulldrv_GetICMProfile( PHYSDEV dev, BOOL allow_default, LPDWORD size, LPWSTR filename )
{
    return FALSE;
}

static DWORD nulldrv_GetImage( PHYSDEV dev, BITMAPINFO *info, struct gdi_image_bits *bits,
                               struct bitblt_coords *src )
{
    return ERROR_NOT_SUPPORTED;
}

static DWORD nulldrv_GetKerningPairs( PHYSDEV dev, DWORD count, LPKERNINGPAIR pairs )
{
    return 0;
}

static UINT nulldrv_GetOutlineTextMetrics( PHYSDEV dev, UINT size, LPOUTLINETEXTMETRICW otm )
{
    return 0;
}

static UINT nulldrv_GetTextCharsetInfo( PHYSDEV dev, LPFONTSIGNATURE fs, DWORD flags )
{
    return DEFAULT_CHARSET;
}

static BOOL nulldrv_GetTextExtentExPoint( PHYSDEV dev, LPCWSTR str, INT count, INT *dx )
{
    return FALSE;
}

static BOOL nulldrv_GetTextExtentExPointI( PHYSDEV dev, const WORD *indices, INT count, INT *dx )
{
    return FALSE;
}

static INT nulldrv_GetTextFace( PHYSDEV dev, INT size, LPWSTR name )
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

static BOOL nulldrv_GetTextMetrics( PHYSDEV dev, TEXTMETRICW *metrics )
{
    return FALSE;
}

static BOOL nulldrv_LineTo( PHYSDEV dev, INT x, INT y )
{
    return TRUE;
}

static BOOL nulldrv_MoveTo( PHYSDEV dev, INT x, INT y )
{
    return TRUE;
}

static BOOL nulldrv_PaintRgn( PHYSDEV dev, HRGN rgn )
{
    return TRUE;
}

static BOOL nulldrv_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop )
{
    return TRUE;
}

static BOOL nulldrv_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                         INT xstart, INT ystart, INT xend, INT yend )
{
    return TRUE;
}

static BOOL nulldrv_PolyPolygon( PHYSDEV dev, const POINT *points, const INT *counts, UINT polygons )
{
    return TRUE;
}

static BOOL nulldrv_PolyPolyline( PHYSDEV dev, const POINT *points, const DWORD *counts, DWORD lines )
{
    return TRUE;
}

static DWORD nulldrv_PutImage( PHYSDEV dev, HRGN clip, BITMAPINFO *info,
                               const struct gdi_image_bits *bits, struct bitblt_coords *src,
                               struct bitblt_coords *dst, DWORD rop )
{
    return ERROR_SUCCESS;
}

static UINT nulldrv_RealizeDefaultPalette( PHYSDEV dev )
{
    return 0;
}

static UINT nulldrv_RealizePalette( PHYSDEV dev, HPALETTE palette, BOOL primary )
{
    return 0;
}

static BOOL nulldrv_Rectangle( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    return TRUE;
}

static BOOL nulldrv_ResetDC( PHYSDEV dev, const DEVMODEW *devmode )
{
    return FALSE;
}

static BOOL nulldrv_RoundRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                               INT ell_width, INT ell_height )
{
    return TRUE;
}

static HBITMAP nulldrv_SelectBitmap( PHYSDEV dev, HBITMAP bitmap )
{
    return bitmap;
}

static HBRUSH nulldrv_SelectBrush( PHYSDEV dev, HBRUSH brush, const struct brush_pattern *pattern )
{
    return brush;
}

static HFONT nulldrv_SelectFont( PHYSDEV dev, HFONT font, UINT *aa_flags )
{
    return font;
}

static HPEN nulldrv_SelectPen( PHYSDEV dev, HPEN pen, const struct brush_pattern *pattern )
{
    return pen;
}

static COLORREF nulldrv_SetBkColor( PHYSDEV dev, COLORREF color )
{
    return color;
}

static UINT nulldrv_SetBoundsRect( PHYSDEV dev, RECT *rect, UINT flags )
{
    return DCB_RESET;
}

static COLORREF nulldrv_SetDCBrushColor( PHYSDEV dev, COLORREF color )
{
    return color;
}

static COLORREF nulldrv_SetDCPenColor( PHYSDEV dev, COLORREF color )
{
    return color;
}

static void nulldrv_SetDeviceClipping( PHYSDEV dev, HRGN rgn )
{
}

static BOOL nulldrv_SetDeviceGammaRamp( PHYSDEV dev, void *ramp )
{
    RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
    return FALSE;
}

static COLORREF nulldrv_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color )
{
    return color;
}

static COLORREF nulldrv_SetTextColor( PHYSDEV dev, COLORREF color )
{
    return color;
}

static INT nulldrv_StartDoc( PHYSDEV dev, const DOCINFOW *info )
{
    return 0;
}

static INT nulldrv_StartPage( PHYSDEV dev )
{
    return 1;
}

static BOOL nulldrv_UnrealizePalette( HPALETTE palette )
{
    return FALSE;
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

    GDI_PRIORITY_NULL_DRV               /* priority */
};


/**********************************************************************
 * Null user driver
 *
 * These are fallbacks for entry points that are not implemented in the real driver.
 */

static BOOL nulldrv_ActivateKeyboardLayout( HKL layout, UINT flags )
{
    return TRUE;
}

static void nulldrv_Beep(void)
{
}

static UINT nulldrv_GetKeyboardLayoutList( INT size, HKL *layouts )
{
    return ~0; /* use default implementation */
}

static INT nulldrv_GetKeyNameText( LONG lparam, LPWSTR buffer, INT size )
{
    return -1; /* use default implementation */
}

static UINT nulldrv_MapVirtualKeyEx( UINT code, UINT type, HKL layout )
{
    return -1; /* use default implementation */
}

static BOOL nulldrv_RegisterHotKey( HWND hwnd, UINT modifiers, UINT vk )
{
    return TRUE;
}

static INT nulldrv_ToUnicodeEx( UINT virt, UINT scan, const BYTE *state, LPWSTR str,
                                int size, UINT flags, HKL layout )
{
    return -2; /* use default implementation */
}

static void nulldrv_UnregisterHotKey( HWND hwnd, UINT modifiers, UINT vk )
{
}

static SHORT nulldrv_VkKeyScanEx( WCHAR ch, HKL layout )
{
    return -256; /* use default implementation */
}

static const KBDTABLES *nulldrv_KbdLayerDescriptor( HKL layout )
{
    return NULL;
}

static void nulldrv_ReleaseKbdTables( const KBDTABLES *tables )
{
}

static UINT nulldrv_ImeProcessKey( HIMC himc, UINT wparam, UINT lparam, const BYTE *state )
{
    return 0;
}

static void nulldrv_NotifyIMEStatus( HWND hwnd, UINT status )
{
}

static BOOL nulldrv_SetIMECompositionRect( HWND hwnd, RECT rect )
{
    return FALSE;
}

static LRESULT nulldrv_DesktopWindowProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return default_window_proc( hwnd, msg, wparam, lparam, FALSE );
}

static void nulldrv_DestroyCursorIcon( HCURSOR cursor )
{
}

static void nulldrv_SetCursor( HWND hwnd, HCURSOR cursor )
{
}

static BOOL nulldrv_GetCursorPos( LPPOINT pt )
{
    return TRUE;
}

static BOOL nulldrv_SetCursorPos( INT x, INT y )
{
    return TRUE;
}

static BOOL nulldrv_ClipCursor( const RECT *clip, BOOL reset )
{
    return TRUE;
}

static LRESULT nulldrv_NotifyIcon( HWND hwnd, UINT msg, NOTIFYICONDATAW *data )
{
    return -1;
}

static void nulldrv_CleanupIcons( HWND hwnd )
{
}

static void nulldrv_SystrayDockInit( HWND hwnd )
{
}

static BOOL nulldrv_SystrayDockInsert( HWND hwnd, UINT cx, UINT cy, void *icon )
{
    return FALSE;
}

static void nulldrv_SystrayDockClear( HWND hwnd )
{
}

static BOOL nulldrv_SystrayDockRemove( HWND hwnd )
{
    return FALSE;
}

static void nulldrv_UpdateClipboard(void)
{
}

static LONG nulldrv_ChangeDisplaySettings( LPDEVMODEW displays, LPCWSTR primary_name, HWND hwnd,
                                           DWORD flags, LPVOID lparam )
{
    return DISP_CHANGE_SUCCESSFUL;
}

static UINT nulldrv_UpdateDisplayDevices( const struct gdi_device_manager *manager, void *param )
{
    return STATUS_NOT_IMPLEMENTED;
}

static BOOL nulldrv_CreateDesktop( const WCHAR *name, UINT width, UINT height )
{
    return TRUE;
}

static BOOL nodrv_CreateWindow( HWND hwnd )
{
    static int warned;
    HWND parent = NtUserGetAncestor( hwnd, GA_PARENT );

    /* HWND_MESSAGE windows don't need a graphics driver */
    if (!parent || parent == UlongToHandle( NtUserGetThreadInfo()->msg_window )) return TRUE;
    if (warned++) return FALSE;

    ERR_(winediag)( "Application tried to create a window, but no driver could be loaded.\n" );
    if (driver_load_error[0]) ERR_(winediag)( "%s\n", debugstr_w(driver_load_error) );
    return FALSE;
}

static BOOL nulldrv_CreateWindow( HWND hwnd )
{
    return TRUE;
}

static void nulldrv_DestroyWindow( HWND hwnd )
{
}

static void nulldrv_FlashWindowEx( FLASHWINFO *info )
{
}

static void nulldrv_GetDC( HDC hdc, HWND hwnd, HWND top_win, const RECT *win_rect,
                           const RECT *top_rect, DWORD flags )
{
}

static BOOL nulldrv_ProcessEvents( DWORD mask )
{
    return FALSE;
}

static void nulldrv_ReleaseDC( HWND hwnd, HDC hdc )
{
}

static BOOL nulldrv_ScrollDC( HDC hdc, INT dx, INT dy, HRGN update )
{
    RECT rect;

    NtGdiGetAppClipBox( hdc, &rect );
    return NtGdiBitBlt( hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                        hdc, rect.left - dx, rect.top - dy, SRCCOPY, 0, 0 );
}

static void nulldrv_SetCapture( HWND hwnd, UINT flags )
{
}

static void nulldrv_SetDesktopWindow( HWND hwnd )
{
}

static void nulldrv_SetFocus( HWND hwnd )
{
}

static void nulldrv_SetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags )
{
}

static void nulldrv_SetParent( HWND hwnd, HWND parent, HWND old_parent )
{
}

static void nulldrv_SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
}

static void nulldrv_SetWindowIcon( HWND hwnd, UINT type, HICON icon )
{
}

static void nulldrv_SetWindowStyle( HWND hwnd, INT offset, STYLESTRUCT *style )
{
}

static void nulldrv_SetWindowText( HWND hwnd, LPCWSTR text )
{
}

static UINT nulldrv_ShowWindow( HWND hwnd, INT cmd, RECT *rect, UINT swp )
{
    return ~0; /* use default implementation */
}

static LRESULT nulldrv_SysCommand( HWND hwnd, WPARAM wparam, LPARAM lparam, const POINT *pos )
{
    return -1;
}

static void nulldrv_UpdateLayeredWindow( HWND hwnd, BYTE alpha, UINT flags )
{
}

static LRESULT nulldrv_WindowMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return 0;
}

static BOOL nulldrv_WindowPosChanging( HWND hwnd, UINT swp_flags, BOOL shaped, const struct window_rects *rects )
{
    return TRUE;
}

static BOOL nulldrv_GetWindowStyleMasks( HWND hwnd, UINT style, UINT ex_style, UINT *style_mask, UINT *ex_style_mask )
{
    return FALSE;
}

static BOOL nulldrv_GetWindowStateUpdates( HWND hwnd, UINT *state_cmd, UINT *config_cmd, RECT *rect )
{
    return FALSE;
}

static BOOL nulldrv_CreateWindowSurface( HWND hwnd, BOOL layered, const RECT *surface_rect, struct window_surface **surface )
{
    return FALSE;
}

static void nulldrv_MoveWindowBits( HWND hwnd, const struct window_rects *old_rects,
                                    const struct window_rects *new_rects, const RECT *valid_rects )
{
}

static void nulldrv_WindowPosChanged( HWND hwnd, HWND insert_after, HWND owner_hint, UINT swp_flags, BOOL fullscreen,
                                      const struct window_rects *new_rects, struct window_surface *surface )
{
}

static BOOL nulldrv_SystemParametersInfo( UINT action, UINT int_param, void *ptr_param, UINT flags )
{
    return FALSE;
}

static UINT nulldrv_VulkanInit( UINT version, void *vulkan_handle, const struct vulkan_driver_funcs **driver_funcs )
{
    return STATUS_NOT_IMPLEMENTED;
}

static struct opengl_funcs *nulldrv_wine_get_wgl_driver( UINT version )
{
    return (void *)-1;
}

static void nulldrv_ThreadDetach( void )
{
}

static const WCHAR guid_key_prefixW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','y','s','t','e','m',
    '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
    '\\','C','o','n','t','r','o','l',
    '\\','V','i','d','e','o','\\','{'
};
static const WCHAR guid_key_suffixW[] = {'}','\\','0','0','0','0'};

static BOOL load_desktop_driver( HWND hwnd )
{
    static const WCHAR guid_nullW[] = {'0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0','0','-',
                                       '0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0','0',0};
    WCHAR key[ARRAYSIZE(guid_key_prefixW) + 40 + ARRAYSIZE(guid_key_suffixW)], *ptr;
    char buf[4096];
    KEY_VALUE_PARTIAL_INFORMATION *info = (void *)buf;
    ATOM_BASIC_INFORMATION *abi = (ATOM_BASIC_INFORMATION *)buf;
    BOOL ret = FALSE;
    HKEY hkey;
    DWORD size;
    UINT guid_atom;

    static const WCHAR prop_nameW[] =
        {'_','_','w','i','n','e','_','d','i','s','p','l','a','y','_','d','e','v','i','c','e',
         '_','g','u','i','d',0};

    user_check_not_lock();

    asciiz_to_unicode( driver_load_error, "The explorer process failed to start." );  /* default error */
    /* wait for graphics driver to be ready */
    send_message( hwnd, WM_NULL, 0, 0 );

    guid_atom = HandleToULong( NtUserGetProp( hwnd, prop_nameW ));
    memcpy( key, guid_key_prefixW, sizeof(guid_key_prefixW) );
    ptr = key + ARRAYSIZE(guid_key_prefixW);
    if (NtQueryInformationAtom( guid_atom, AtomBasicInformation, buf, sizeof(buf), NULL ))
    {
        wcscpy( ptr, guid_nullW );
        ptr += ARRAY_SIZE(guid_nullW) - 1;
    }
    else
    {
        memcpy( ptr, abi->Name, abi->NameLength );
        ptr += abi->NameLength / sizeof(WCHAR);
    }
    memcpy( ptr, guid_key_suffixW, sizeof(guid_key_suffixW) );
    ptr += ARRAY_SIZE(guid_key_suffixW);

    if (!(hkey = reg_open_key( NULL, key, (ptr - key) * sizeof(WCHAR) ))) return FALSE;

    if ((size = query_reg_ascii_value( hkey, "GraphicsDriver", info, sizeof(buf) )))
    {
        static const WCHAR nullW[] = {'n','u','l','l',0};
        TRACE( "trying driver %s\n", debugstr_wn( (const WCHAR *)info->Data,
                                                  info->DataLength / sizeof(WCHAR) ));
        if (info->DataLength != sizeof(nullW) || memcmp( info->Data, nullW, sizeof(nullW) ))
        {
            void *ret_ptr;
            ULONG ret_len;
            ret = !KeUserModeCallback( NtUserLoadDriver, info->Data, info->DataLength, &ret_ptr, &ret_len );
        }
        else
        {
            __wine_set_user_driver( &null_user_driver, WINE_GDI_DRIVER_VERSION );
            ret = TRUE;
        }
    }
    else if ((size = query_reg_ascii_value( hkey, "DriverError", info, sizeof(buf) )))
    {
        memcpy( driver_load_error, info->Data, min( info->DataLength, sizeof(driver_load_error) ));
        driver_load_error[ARRAYSIZE(driver_load_error) - 1] = 0;
    }

    NtClose( hkey );
    return ret;
}

/**********************************************************************
 * Lazy loading user driver
 *
 * Initial driver used before another driver is loaded.
 * Each entry point simply loads the real driver and chains to it.
 */

static void load_display_driver(void)
{
    USEROBJECTFLAGS flags;
    HWINSTA winstation;

    if (!load_desktop_driver( get_desktop_window() ) || user_driver == &lazy_load_driver)
    {
        winstation = NtUserGetProcessWindowStation();
        if (!NtUserGetObjectInformation( winstation, UOI_FLAGS, &flags, sizeof(flags), NULL )
            || (flags.dwFlags & WSF_VISIBLE))
            null_user_driver.pCreateWindow = nodrv_CreateWindow;

        __wine_set_user_driver( &null_user_driver, WINE_GDI_DRIVER_VERSION );
    }
}

static const struct user_driver_funcs *load_driver(void)
{
    load_display_driver();
    update_display_cache( FALSE );
    return user_driver;
}

void init_display_driver(void)
{
    if (user_driver == &lazy_load_driver) load_display_driver();
}

/**********************************************************************
 *           get_display_driver
 */
const struct gdi_dc_funcs *get_display_driver(void)
{
    if (user_driver == &lazy_load_driver) load_driver();
    return &user_driver->dc_funcs;
}

static BOOL loaderdrv_ActivateKeyboardLayout( HKL layout, UINT flags )
{
    return load_driver()->pActivateKeyboardLayout( layout, flags );
}

static void loaderdrv_Beep(void)
{
    load_driver()->pBeep();
}

static INT loaderdrv_GetKeyNameText( LONG lparam, LPWSTR buffer, INT size )
{
    return load_driver()->pGetKeyNameText( lparam, buffer, size );
}

static UINT loaderdrv_GetKeyboardLayoutList( INT size, HKL *layouts )
{
    return load_driver()->pGetKeyboardLayoutList( size, layouts );
}

static UINT loaderdrv_MapVirtualKeyEx( UINT code, UINT type, HKL layout )
{
    return load_driver()->pMapVirtualKeyEx( code, type, layout );
}

static INT loaderdrv_ToUnicodeEx( UINT virt, UINT scan, const BYTE *state, LPWSTR str,
                                        int size, UINT flags, HKL layout )
{
    return load_driver()->pToUnicodeEx( virt, scan, state, str, size, flags, layout );
}

static BOOL loaderdrv_RegisterHotKey( HWND hwnd, UINT modifiers, UINT vk )
{
    return load_driver()->pRegisterHotKey( hwnd, modifiers, vk );
}

static void loaderdrv_UnregisterHotKey( HWND hwnd, UINT modifiers, UINT vk )
{
    load_driver()->pUnregisterHotKey( hwnd, modifiers, vk );
}

static SHORT loaderdrv_VkKeyScanEx( WCHAR ch, HKL layout )
{
    return load_driver()->pVkKeyScanEx( ch, layout );
}

static const KBDTABLES *loaderdrv_KbdLayerDescriptor( HKL layout )
{
    return load_driver()->pKbdLayerDescriptor( layout );
}

static void loaderdrv_ReleaseKbdTables( const KBDTABLES *tables )
{
    return load_driver()->pReleaseKbdTables( tables );
}

static UINT loaderdrv_ImeProcessKey( HIMC himc, UINT wparam, UINT lparam, const BYTE *state )
{
    return load_driver()->pImeProcessKey( himc, wparam, lparam, state );
}

static void loaderdrv_NotifyIMEStatus( HWND hwnd, UINT status )
{
    return load_driver()->pNotifyIMEStatus( hwnd, status );
}

static BOOL loaderdrv_SetIMECompositionRect( HWND hwnd, RECT rect )
{
    return load_driver()->pSetIMECompositionRect( hwnd, rect );
}

static LONG loaderdrv_ChangeDisplaySettings( LPDEVMODEW displays, LPCWSTR primary_name, HWND hwnd,
                                             DWORD flags, LPVOID lparam )
{
    return load_driver()->pChangeDisplaySettings( displays, primary_name, hwnd, flags, lparam );
}

static void loaderdrv_SetCursor( HWND hwnd, HCURSOR cursor )
{
    load_driver()->pSetCursor( hwnd, cursor );
}

static BOOL loaderdrv_GetCursorPos( POINT *pt )
{
    return load_driver()->pGetCursorPos( pt );
}

static BOOL loaderdrv_SetCursorPos( INT x, INT y )
{
    return load_driver()->pSetCursorPos( x, y );
}

static BOOL loaderdrv_ClipCursor( const RECT *clip, BOOL reset )
{
    return load_driver()->pClipCursor( clip, reset );
}

static LRESULT loaderdrv_NotifyIcon( HWND hwnd, UINT msg, NOTIFYICONDATAW *data )
{
    return load_driver()->pNotifyIcon( hwnd, msg, data );
}

static void loaderdrv_CleanupIcons( HWND hwnd )
{
    load_driver()->pCleanupIcons( hwnd );
}

static void loaderdrv_SystrayDockInit( HWND hwnd )
{
    load_driver()->pSystrayDockInit( hwnd );
}

static BOOL loaderdrv_SystrayDockInsert( HWND hwnd, UINT cx, UINT cy, void *icon )
{
    return load_driver()->pSystrayDockInsert( hwnd, cx, cy, icon );
}

static void loaderdrv_SystrayDockClear( HWND hwnd )
{
    load_driver()->pSystrayDockClear( hwnd );
}

static BOOL loaderdrv_SystrayDockRemove( HWND hwnd )
{
    return load_driver()->pSystrayDockRemove( hwnd );
}

static LRESULT nulldrv_ClipboardWindowProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return 0;
}

static void loaderdrv_UpdateClipboard(void)
{
    load_driver()->pUpdateClipboard();
}

static UINT loaderdrv_UpdateDisplayDevices( const struct gdi_device_manager *manager, void *param )
{
    return load_driver()->pUpdateDisplayDevices( manager, param );
}

static BOOL loaderdrv_CreateDesktop( const WCHAR *name, UINT width, UINT height )
{
    return load_driver()->pCreateDesktop( name, width, height );
}

static BOOL loaderdrv_CreateWindow( HWND hwnd )
{
    return load_driver()->pCreateWindow( hwnd );
}

static void loaderdrv_GetDC( HDC hdc, HWND hwnd, HWND top_win, const RECT *win_rect,
                             const RECT *top_rect, DWORD flags )
{
    load_driver()->pGetDC( hdc, hwnd, top_win, win_rect, top_rect, flags );
}

static void loaderdrv_FlashWindowEx( FLASHWINFO *info )
{
    load_driver()->pFlashWindowEx( info );
}

static void loaderdrv_SetDesktopWindow( HWND hwnd )
{
    load_driver()->pSetDesktopWindow( hwnd );
}

static void loaderdrv_SetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags )
{
    load_driver()->pSetLayeredWindowAttributes( hwnd, key, alpha, flags );
}

static void loaderdrv_SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
    load_driver()->pSetWindowRgn( hwnd, hrgn, redraw );
}

static void loaderdrv_UpdateLayeredWindow( HWND hwnd, BYTE alpha, UINT flags )
{
    load_driver()->pUpdateLayeredWindow( hwnd, alpha, flags );
}

static UINT loaderdrv_VulkanInit( UINT version, void *vulkan_handle, const struct vulkan_driver_funcs **driver_funcs )
{
    return load_driver()->pVulkanInit( version, vulkan_handle, driver_funcs );
}

static const struct user_driver_funcs lazy_load_driver =
{
    { NULL },
    /* keyboard functions */
    loaderdrv_ActivateKeyboardLayout,
    loaderdrv_Beep,
    loaderdrv_GetKeyNameText,
    loaderdrv_GetKeyboardLayoutList,
    loaderdrv_MapVirtualKeyEx,
    loaderdrv_RegisterHotKey,
    loaderdrv_ToUnicodeEx,
    loaderdrv_UnregisterHotKey,
    loaderdrv_VkKeyScanEx,
    loaderdrv_KbdLayerDescriptor,
    loaderdrv_ReleaseKbdTables,
    loaderdrv_ImeProcessKey,
    loaderdrv_NotifyIMEStatus,
    loaderdrv_SetIMECompositionRect,
    /* cursor/icon functions */
    nulldrv_DestroyCursorIcon,
    loaderdrv_SetCursor,
    loaderdrv_GetCursorPos,
    loaderdrv_SetCursorPos,
    loaderdrv_ClipCursor,
    /* systray functions */
    loaderdrv_NotifyIcon,
    loaderdrv_CleanupIcons,
    loaderdrv_SystrayDockInit,
    loaderdrv_SystrayDockInsert,
    loaderdrv_SystrayDockClear,
    loaderdrv_SystrayDockRemove,
    /* clipboard functions */
    nulldrv_ClipboardWindowProc,
    loaderdrv_UpdateClipboard,
    /* display modes */
    loaderdrv_ChangeDisplaySettings,
    loaderdrv_UpdateDisplayDevices,
    /* windowing functions */
    loaderdrv_CreateDesktop,
    loaderdrv_CreateWindow,
    nulldrv_DesktopWindowProc,
    nulldrv_DestroyWindow,
    loaderdrv_FlashWindowEx,
    loaderdrv_GetDC,
    nulldrv_ProcessEvents,
    nulldrv_ReleaseDC,
    nulldrv_ScrollDC,
    nulldrv_SetCapture,
    loaderdrv_SetDesktopWindow,
    nulldrv_SetFocus,
    loaderdrv_SetLayeredWindowAttributes,
    nulldrv_SetParent,
    loaderdrv_SetWindowRgn,
    nulldrv_SetWindowIcon,
    nulldrv_SetWindowStyle,
    nulldrv_SetWindowText,
    nulldrv_ShowWindow,
    nulldrv_SysCommand,
    loaderdrv_UpdateLayeredWindow,
    nulldrv_WindowMessage,
    nulldrv_WindowPosChanging,
    nulldrv_GetWindowStyleMasks,
    nulldrv_GetWindowStateUpdates,
    nulldrv_CreateWindowSurface,
    nulldrv_MoveWindowBits,
    nulldrv_WindowPosChanged,
    /* system parameters */
    nulldrv_SystemParametersInfo,
    /* vulkan support */
    loaderdrv_VulkanInit,
    /* opengl support */
    nulldrv_wine_get_wgl_driver,
    /* thread management */
    nulldrv_ThreadDetach,
};

const struct user_driver_funcs *user_driver = &lazy_load_driver;

/******************************************************************************
 *	     __wine_set_user_driver   (win32u.so)
 */
void __wine_set_user_driver( const struct user_driver_funcs *funcs, UINT version )
{
    struct user_driver_funcs *driver, *prev;

    if (version != WINE_GDI_DRIVER_VERSION)
    {
        ERR( "version mismatch, driver wants %u but win32u has %u\n",
             version, WINE_GDI_DRIVER_VERSION );
        return;
    }

    if (!funcs)
    {
        prev = InterlockedExchangePointer( (void **)&user_driver, (void *)&lazy_load_driver );
        if (prev != &lazy_load_driver)
            free( prev );
        return;
    }

    driver = malloc( sizeof(*driver) );
    *driver = *funcs;

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
    SET_USER_FUNC(KbdLayerDescriptor);
    SET_USER_FUNC(ReleaseKbdTables);
    SET_USER_FUNC(ImeProcessKey);
    SET_USER_FUNC(NotifyIMEStatus);
    SET_USER_FUNC(SetIMECompositionRect);
    SET_USER_FUNC(DestroyCursorIcon);
    SET_USER_FUNC(SetCursor);
    SET_USER_FUNC(GetCursorPos);
    SET_USER_FUNC(SetCursorPos);
    SET_USER_FUNC(ClipCursor);
    SET_USER_FUNC(NotifyIcon);
    SET_USER_FUNC(CleanupIcons);
    SET_USER_FUNC(SystrayDockInit);
    SET_USER_FUNC(SystrayDockInsert);
    SET_USER_FUNC(SystrayDockClear);
    SET_USER_FUNC(SystrayDockRemove);
    SET_USER_FUNC(ClipboardWindowProc);
    SET_USER_FUNC(UpdateClipboard);
    SET_USER_FUNC(ChangeDisplaySettings);
    SET_USER_FUNC(UpdateDisplayDevices);
    SET_USER_FUNC(CreateDesktop);
    SET_USER_FUNC(CreateWindow);
    SET_USER_FUNC(DesktopWindowProc);
    SET_USER_FUNC(DestroyWindow);
    SET_USER_FUNC(FlashWindowEx);
    SET_USER_FUNC(GetDC);
    SET_USER_FUNC(ProcessEvents);
    SET_USER_FUNC(ReleaseDC);
    SET_USER_FUNC(ScrollDC);
    SET_USER_FUNC(SetCapture);
    SET_USER_FUNC(SetDesktopWindow);
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
    SET_USER_FUNC(GetWindowStyleMasks);
    SET_USER_FUNC(GetWindowStateUpdates);
    SET_USER_FUNC(CreateWindowSurface);
    SET_USER_FUNC(MoveWindowBits);
    SET_USER_FUNC(WindowPosChanged);
    SET_USER_FUNC(SystemParametersInfo);
    SET_USER_FUNC(VulkanInit);
    SET_USER_FUNC(wine_get_wgl_driver);
    SET_USER_FUNC(ThreadDetach);
#undef SET_USER_FUNC

    prev = InterlockedCompareExchangePointer( (void **)&user_driver, driver, (void *)&lazy_load_driver );
    if (prev != &lazy_load_driver)
    {
        /* another thread beat us to it */
        free( driver );
        driver = prev;
    }
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
