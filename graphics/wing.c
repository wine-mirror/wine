/*
 * WinG support
 *
 * Started by Robert Pouliot <krynos@clic.net>
 */

#include "config.h"

#include "ts_xlib.h"
#ifdef HAVE_LIBXXSHM
#include <sys/types.h>
#include <sys/ipc.h>
#ifndef __EMX__
#include <sys/shm.h>
#endif /* !defined(__EMX__) */
#include "ts_xshm.h"
#endif /* defined(HAVE_LIBXXSHM) */
#include "x11drv.h"

#include "bitmap.h"
#include "palette.h"
#include "dc.h"
#include "debug.h"
#include "gdi.h"
#include "heap.h"
#include "selectors.h"
#include "monitor.h"
#include "wintypes.h"
#include "xmalloc.h"

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
 *          WinGCreateDC16	(WING.1001)
 */
HDC16 WINAPI WinGCreateDC16(void)
{
    TRACE(wing, "(void)\n");
	return CreateCompatibleDC16(0);
}

/***********************************************************************
 *  WinGRecommendDIBFormat16    (WING.1002)
 */
BOOL16 WINAPI WinGRecommendDIBFormat16(BITMAPINFO *bmpi)
{
    TRACE(wing, "(%p)\n", bmpi);
    if (!bmpi)
	return FALSE;

    bmpi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpi->bmiHeader.biWidth = 320;
    bmpi->bmiHeader.biHeight = -1;
    bmpi->bmiHeader.biPlanes = 1;
    bmpi->bmiHeader.biBitCount = MONITOR_GetDepth(&MONITOR_PrimaryMonitor);
    bmpi->bmiHeader.biCompression = BI_RGB;
    bmpi->bmiHeader.biSizeImage = 0;
    bmpi->bmiHeader.biXPelsPerMeter = 0;
    bmpi->bmiHeader.biYPelsPerMeter = 0;
    bmpi->bmiHeader.biClrUsed = 0;
    bmpi->bmiHeader.biClrImportant = 0;

    return TRUE;
  }

/***********************************************************************
 *        WinGCreateBitmap16    (WING.1003)
 */
HBITMAP16 WINAPI WinGCreateBitmap16(HDC16 hdc, BITMAPINFO *bmpi,
                                    SEGPTR *bits)
{
    TRACE(wing, "(%d,%p,%p)\n", hdc, bmpi, bits);
    TRACE(wing, ": create %ldx%ldx%d bitmap\n", bmpi->bmiHeader.biWidth,
	  bmpi->bmiHeader.biHeight, bmpi->bmiHeader.biPlanes);
    return CreateDIBSection16(hdc, bmpi, 0, bits, 0, 0);
		} 

/***********************************************************************
 *  WinGGetDIBPointer   (WING.1004)
 */
SEGPTR WINAPI WinGGetDIBPointer16(HBITMAP16 hWinGBitmap, BITMAPINFO* bmpi)
{
  BITMAPOBJ*	bmp = (BITMAPOBJ *) GDI_GetObjPtr( hWinGBitmap, BITMAP_MAGIC );

    TRACE(wing, "(%d,%p)\n", hWinGBitmap, bmpi);
    if (!bmp) return (SEGPTR)NULL;

    if (bmpi)
	FIXME(wing, ": Todo - implement setting BITMAPINFO\n");

    return PTR_SEG_OFF_TO_SEGPTR(bmp->dib->selector, 0);
}

/***********************************************************************
 *  WinGSetDIBColorTable   (WING.1004)
 */
UINT16 WINAPI WinGSetDIBColorTable16(HDC16 hdc, UINT16 start, UINT16 num,
                                     RGBQUAD *colors)
{
    TRACE(wing, "(%d,%d,%d,%p)\n", hdc, start, num, colors);
    return SetDIBColorTable16(hdc, start, num, colors);
}

/***********************************************************************
 *  WinGGetDIBColorTable16   (WING.1005)
 */
UINT16 WINAPI WinGGetDIBColorTable16(HDC16 hdc, UINT16 start, UINT16 num,
				     RGBQUAD *colors)
{
    TRACE(wing, "(%d,%d,%d,%p)\n", hdc, start, num, colors);
    return GetDIBColorTable16(hdc, start, num, colors);
}

/***********************************************************************
 *  WinGCreateHalfTonePalette16   (WING.1007)
 */
HPALETTE16 WINAPI WinGCreateHalfTonePalette16(void)
{
    TRACE(wing, "(void)\n");
    return CreateHalftonePalette16(GetDC16(0));
}

/***********************************************************************
 *  WinGCreateHalfToneBrush16   (WING.1008)
 */
HBRUSH16 WINAPI WinGCreateHalfToneBrush16(HDC16 winDC, COLORREF col,
                                            WING_DITHER_TYPE type)
{
    TRACE(wing, "(%d,%ld,%d)\n", winDC, col, type);
    return CreateSolidBrush16(col);
}

/***********************************************************************
 *  WinGStretchBlt16   (WING.1009)
 */
BOOL16 WINAPI WinGStretchBlt16(HDC16 destDC, INT16 xDest, INT16 yDest,
                               INT16 widDest, INT16 heiDest,
                               HDC16 srcDC, INT16 xSrc, INT16 ySrc,
                               INT16 widSrc, INT16 heiSrc)
{
    TRACE(wing, "(%d,%d,...)\n", destDC, srcDC);
    return StretchBlt16(destDC, xDest, yDest, widDest, heiDest, srcDC,
			xSrc, ySrc, widSrc, heiSrc, SRCCOPY);
}

/***********************************************************************
 *  WinGBitBlt16   (WING.1010)
 */
BOOL16 WINAPI WinGBitBlt16(HDC16 destDC, INT16 xDest, INT16 yDest,
                           INT16 widDest, INT16 heiDest, HDC16 srcDC,
                           INT16 xSrc, INT16 ySrc)
{
    TRACE(wing, "(%d,%d,...)\n", destDC, srcDC);
    return BitBlt16(destDC, xDest, yDest, widDest, heiDest, srcDC,
		    xSrc, ySrc, SRCCOPY);
}
