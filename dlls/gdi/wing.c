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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "wine/winuser16.h"
#include "bitmap.h"
#include "wine/debug.h"
#include "palette.h"
#include "windef.h"
#include "wownt32.h"

WINE_DEFAULT_DEBUG_CHANNEL(wing);


typedef enum WING_DITHER_TYPE
{
  WING_DISPERSED_4x4, WING_DISPERSED_8x8, WING_CLUSTERED_4x4
} WING_DITHER_TYPE;

/*
 * WinG DIB bitmaps can be selected into DC and then scribbled upon
 * by GDI functions. They can also be changed directly. This gives us
 * three choices
 *	- use original WinG 16-bit DLL
 *		requires working 16-bit driver interface
 * 	- implement DIB graphics driver from scratch
 *		see wing.zip size
 *	- use shared pixmaps
 *		won't work with some videocards and/or videomodes
 * 961208 - AK
 */

/***********************************************************************
 *          WinGCreateDC	(WING.1001)
 */
HDC16 WINAPI WinGCreateDC16(void)
{
    TRACE("(void)\n");
	return CreateCompatibleDC16(0);
}

/***********************************************************************
 *  WinGRecommendDIBFormat    (WING.1002)
 */
BOOL16 WINAPI WinGRecommendDIBFormat16(BITMAPINFO *bmpi)
{
    HDC hdc;
    TRACE("(%p)\n", bmpi);
    if (!bmpi)
	return FALSE;

    hdc = CreateDCA( "DISPLAY", NULL, NULL, NULL );
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
    DeleteDC(hdc);
    return TRUE;
}

/***********************************************************************
 *        WinGCreateBitmap    (WING.1003)
 */
HBITMAP16 WINAPI WinGCreateBitmap16(HDC16 hdc, BITMAPINFO *bmpi,
                                    SEGPTR *bits)
{
    TRACE("(%d,%p,%p)\n", hdc, bmpi, bits);
    TRACE(": create %ldx%ldx%d bitmap\n", bmpi->bmiHeader.biWidth,
	  bmpi->bmiHeader.biHeight, bmpi->bmiHeader.biPlanes);
    return CreateDIBSection16(hdc, bmpi, 0, bits, 0, 0);
}

/***********************************************************************
 *  WinGGetDIBPointer   (WING.1004)
 */
SEGPTR WINAPI WinGGetDIBPointer16(HBITMAP16 hWinGBitmap, BITMAPINFO* bmpi)
{
    BITMAPOBJ* bmp = (BITMAPOBJ *) GDI_GetObjPtr( HBITMAP_32(hWinGBitmap),
						  BITMAP_MAGIC );
    SEGPTR res = 0;

    TRACE("(%d,%p)\n", hWinGBitmap, bmpi);
    if (!bmp) return 0;

    if (bmpi) FIXME(": Todo - implement setting BITMAPINFO\n");

    res = bmp->segptr_bits;
    GDI_ReleaseObj( HBITMAP_32(hWinGBitmap) );
    return res;
}

/***********************************************************************
 *  WinGSetDIBColorTable   (WING.1006)
 */
UINT16 WINAPI WinGSetDIBColorTable16(HDC16 hdc, UINT16 start, UINT16 num,
                                     RGBQUAD *colors)
{
    TRACE("(%d,%d,%d,%p)\n", hdc, start, num, colors);
    return SetDIBColorTable16(hdc, start, num, colors);
}

/***********************************************************************
 *  WinGGetDIBColorTable   (WING.1005)
 */
UINT16 WINAPI WinGGetDIBColorTable16(HDC16 hdc, UINT16 start, UINT16 num,
				     RGBQUAD *colors)
{
    TRACE("(%d,%d,%d,%p)\n", hdc, start, num, colors);
    return GetDIBColorTable16(hdc, start, num, colors);
}

/***********************************************************************
 *  WinGCreateHalfTonePalette   (WING.1007)
 */
HPALETTE16 WINAPI WinGCreateHalfTonePalette16(void)
{
    HDC16 hdc = CreateCompatibleDC16(0);
    HPALETTE16 ret = CreateHalftonePalette16(hdc);
    TRACE("(void)\n");
    DeleteDC16(hdc);
    return ret;
}

/***********************************************************************
 *  WinGCreateHalfToneBrush   (WING.1008)
 */
HBRUSH16 WINAPI WinGCreateHalfToneBrush16(HDC16 winDC, COLORREF col,
                                            WING_DITHER_TYPE type)
{
    TRACE("(%d,%ld,%d)\n", winDC, col, type);
    return CreateSolidBrush16(col);
}

/***********************************************************************
 *  WinGStretchBlt   (WING.1009)
 */
BOOL16 WINAPI WinGStretchBlt16(HDC16 destDC, INT16 xDest, INT16 yDest,
                               INT16 widDest, INT16 heiDest,
                               HDC16 srcDC, INT16 xSrc, INT16 ySrc,
                               INT16 widSrc, INT16 heiSrc)
{
    BOOL16 retval;
    TRACE("(%d,%d,...)\n", destDC, srcDC);
    SetStretchBltMode16 ( destDC, COLORONCOLOR );
    retval=StretchBlt16(destDC, xDest, yDest, widDest, heiDest, srcDC,
			xSrc, ySrc, widSrc, heiSrc, SRCCOPY);
    SetStretchBltMode16 ( destDC, BLACKONWHITE );
    return retval;
}

/***********************************************************************
 *  WinGBitBlt   (WING.1010)
 */
BOOL16 WINAPI WinGBitBlt16(HDC16 destDC, INT16 xDest, INT16 yDest,
                           INT16 widDest, INT16 heiDest, HDC16 srcDC,
                           INT16 xSrc, INT16 ySrc)
{
    TRACE("(%d,%d,...)\n", destDC, srcDC);
    return BitBlt16(destDC, xDest, yDest, widDest, heiDest, srcDC,
		    xSrc, ySrc, SRCCOPY);
}
