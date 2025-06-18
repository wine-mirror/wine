/*
 * WinG support
 *
 * Copyright (C) Robert Pouliot <krynos@clic.net>
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wownt32.h"
#include "wine/wingdi16.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wing);

struct dib_segptr_bits
{
    struct list entry;
    HBITMAP   bmp;
    WORD      sel;
    WORD      count;
};

static struct list dib_segptr_list = LIST_INIT( dib_segptr_list );

/* remove saved bits for bitmaps that no longer exist */
static void cleanup_segptr_bits(void)
{
    unsigned int i;
    struct dib_segptr_bits *bits, *next;

    LIST_FOR_EACH_ENTRY_SAFE( bits, next, &dib_segptr_list, struct dib_segptr_bits, entry )
    {
        if (GetObjectType( bits->bmp ) == OBJ_BITMAP) continue;
        for (i = 0; i < bits->count; i++) FreeSelector16( bits->sel + (i << __AHSHIFT) );
        list_remove( &bits->entry );
        HeapFree( GetProcessHeap(), 0, bits );
    }
}

static SEGPTR alloc_segptr_bits( HBITMAP bmp, void *bits32 )
{
    DIBSECTION dib;
    unsigned int i, size;
    struct dib_segptr_bits *bits;

    cleanup_segptr_bits();

    if (!(bits = HeapAlloc( GetProcessHeap(), 0, sizeof(*bits) ))) return 0;

    GetObjectW( bmp, sizeof(dib), &dib );
    size = dib.dsBm.bmHeight * dib.dsBm.bmWidthBytes;

    /* calculate number of sel's needed for size with 64K steps */
    bits->bmp   = bmp;
    bits->count = (size + 0xffff) / 0x10000;
    bits->sel   = AllocSelectorArray16( bits->count );

    for (i = 0; i < bits->count; i++)
    {
        SetSelectorBase(bits->sel + (i << __AHSHIFT), (DWORD)bits32 + i * 0x10000);
        SetSelectorLimit16(bits->sel + (i << __AHSHIFT), size - 1); /* yep, limit is correct */
        size -= 0x10000;
    }
    list_add_head( &dib_segptr_list, &bits->entry );
    return MAKESEGPTR( bits->sel, 0 );
}

/*************************************************************************
 * WING {WING}
 *
 * The Windows Game dll provides a number of functions designed to allow
 * programmers to bypass Gdi and write directly to video memory. The intention
 * was to bolster the use of Windows as a gaming platform and remove the
 * need for Dos based games using 32 bit Dos extenders.
 *
 * This initial approach could not compete with the performance of Dos games
 * (such as Doom and Warcraft) at the time, and so this dll was eventually
 * superseded by DirectX. It should not be used by new applications, and is
 * provided only for compatibility with older Windows programs.
 */

typedef enum WING_DITHER_TYPE
{
  WING_DISPERSED_4x4, WING_DISPERSED_8x8, WING_CLUSTERED_4x4
} WING_DITHER_TYPE;

/***********************************************************************
 *          WinGCreateDC	(WING.1001)
 *
 * Create a new WinG device context.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Success: A handle to the created device context.
 *  Failure: A NULL handle.
 */
HDC16 WINAPI WinGCreateDC16(void)
{
    TRACE("(void)\n");
    return HDC_16( CreateCompatibleDC( 0 ));
}

/***********************************************************************
 *  WinGRecommendDIBFormat    (WING.1002)
 *
 * Get the recommended format of bitmaps for the current display.
 *
 * PARAMS
 *  bmpi [O] Destination for format information
 *
 * RETURNS
 *  Success: TRUE. bmpi is filled with the best (fastest) bitmap format
 *  Failure: FALSE, if bmpi is NULL.
 */
BOOL16 WINAPI WinGRecommendDIBFormat16(BITMAPINFO *bmpi)
{
    TRACE("(%p)\n", bmpi);

    if (!bmpi)
	return FALSE;

    bmpi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpi->bmiHeader.biWidth = 320;
    bmpi->bmiHeader.biHeight = -1;
    bmpi->bmiHeader.biPlanes = 1;
    bmpi->bmiHeader.biBitCount = 8;
    bmpi->bmiHeader.biCompression = BI_RGB;
    bmpi->bmiHeader.biSizeImage = 0;
    bmpi->bmiHeader.biXPelsPerMeter = 0;
    bmpi->bmiHeader.biYPelsPerMeter = 0;
    bmpi->bmiHeader.biClrUsed = 0;
    bmpi->bmiHeader.biClrImportant = 0;

    return TRUE;
}

/***********************************************************************
 *        WinGCreateBitmap    (WING.1003)
 *
 * Create a new WinG bitmap.
 *
 * PARAMS
 *  hdc  [I] WinG device context
 *  bmpi [I] Information about the bitmap
 *  bits [I] Location of the bitmap image data
 *
 * RETURNS
 *  Success: A handle to the created bitmap.
 *  Failure: A NULL handle.
 */
HBITMAP16 WINAPI WinGCreateBitmap16(HDC16 hdc, BITMAPINFO *bmpi, SEGPTR *bits)
{
    LPVOID bits32;
    HBITMAP hbitmap;

    TRACE("(%d,%p,%p): create %ldx%ldx%d bitmap\n", hdc, bmpi, bits,
          bmpi->bmiHeader.biWidth, bmpi->bmiHeader.biHeight, bmpi->bmiHeader.biPlanes);

    hbitmap = CreateDIBSection( HDC_32(hdc), bmpi, DIB_RGB_COLORS, &bits32, 0, 0 );
    if (hbitmap)
    {
        SEGPTR segptr = alloc_segptr_bits( hbitmap, bits32 );
        if (bits) *bits = segptr;
    }
    return HBITMAP_16(hbitmap);
}

/***********************************************************************
 *  WinGGetDIBPointer   (WING.1004)
 */
SEGPTR WINAPI WinGGetDIBPointer16(HBITMAP16 hWinGBitmap, BITMAPINFO* bmpi)
{
    struct dib_segptr_bits *bits;

    if (bmpi) FIXME( "%04x %p: setting BITMAPINFO not supported\n", hWinGBitmap, bmpi );

    LIST_FOR_EACH_ENTRY( bits, &dib_segptr_list, struct dib_segptr_bits, entry )
        if (HBITMAP_16(bits->bmp) == hWinGBitmap) return MAKESEGPTR( bits->sel, 0 );

    return 0;
}

/***********************************************************************
 *  WinGSetDIBColorTable   (WING.1006)
 *
 * Set all or part of the color table for a WinG device context.
 *
 * PARAMS
 *  hdc    [I] WinG device context
 *  start  [I] Start color
 *  num    [I] Number of entries to set
 *  colors [I] Array of color data
 *
 * RETURNS
 *  The number of entries set.
 */
UINT16 WINAPI WinGSetDIBColorTable16(HDC16 hdc, UINT16 start, UINT16 num, RGBQUAD *colors)
{
    TRACE("(%d,%d,%d,%p)\n", hdc, start, num, colors);
    return SetDIBColorTable( HDC_32(hdc), start, num, colors );
}

/***********************************************************************
 *  WinGGetDIBColorTable   (WING.1005)
 *
 * Get all or part of the color table for a WinG device context.
 *
 * PARAMS
 *  hdc    [I] WinG device context
 *  start  [I] Start color
 *  num    [I] Number of entries to set
 *  colors [O] Destination for the array of color data
 *
 * RETURNS
 *  The number of entries retrieved.
 */
UINT16 WINAPI WinGGetDIBColorTable16(HDC16 hdc, UINT16 start, UINT16 num, RGBQUAD *colors)
{
    TRACE("(%d,%d,%d,%p)\n", hdc, start, num, colors);
    return GetDIBColorTable( HDC_32(hdc), start, num, colors );
}

/***********************************************************************
 *  WinGCreateHalfTonePalette   (WING.1007)
 *
 * Create a half tone palette.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Success: A handle to the created palette.
 *  Failure: A NULL handle.
 */
HPALETTE16 WINAPI WinGCreateHalfTonePalette16(void)
{
    HDC hdc = CreateCompatibleDC(0);
    HPALETTE16 ret = HPALETTE_16( CreateHalftonePalette( hdc ));
    TRACE("(void)\n");
    DeleteDC( hdc );
    return ret;
}

/***********************************************************************
 *  WinGCreateHalfToneBrush   (WING.1008)
 *
 * Create a half tone brush for a WinG device context.
 *
 * PARAMS
 *  winDC [I] WinG device context
 *  col   [I] Color
 *  type  [I] Desired dithering type.
 *
 * RETURNS
 *  Success: A handle to the created brush.
 *  Failure: A NULL handle.
 */
HBRUSH16 WINAPI WinGCreateHalfToneBrush16(HDC16 winDC, COLORREF col,
                                            WING_DITHER_TYPE type)
{
    TRACE("(%d,%ld,%d)\n", winDC, col, type);
    return HBRUSH_16( CreateSolidBrush( col ));
}

/***********************************************************************
 *  WinGStretchBlt   (WING.1009)
 *
 * See StretchBlt16.
 */
BOOL16 WINAPI WinGStretchBlt16(HDC16 destDC, INT16 xDest, INT16 yDest,
                               INT16 widDest, INT16 heiDest,
                               HDC16 srcDC, INT16 xSrc, INT16 ySrc,
                               INT16 widSrc, INT16 heiSrc)
{
    BOOL retval;
    TRACE("(%d,%d,...)\n", destDC, srcDC);
    SetStretchBltMode( HDC_32(destDC), COLORONCOLOR );
    retval = StretchBlt( HDC_32(destDC), xDest, yDest, widDest, heiDest,
                         HDC_32(srcDC), xSrc, ySrc, widSrc, heiSrc, SRCCOPY );
    SetStretchBltMode( HDC_32(destDC), BLACKONWHITE );
    return retval;
}

/***********************************************************************
 *  WinGBitBlt   (WING.1010)
 *
 * See BitBlt16.
 */
BOOL16 WINAPI WinGBitBlt16(HDC16 destDC, INT16 xDest, INT16 yDest,
                           INT16 widDest, INT16 heiDest, HDC16 srcDC,
                           INT16 xSrc, INT16 ySrc)
{
    TRACE("(%d,%d,...)\n", destDC, srcDC);
    return BitBlt( HDC_32(destDC), xDest, yDest, widDest, heiDest, HDC_32(srcDC), xSrc, ySrc, SRCCOPY );
}
