/*
 * WingG support
 *
 * Started by Robert Pouliot <krynos@clic.net>
 */

#include "gdi.h"
#include "windows.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *          WingCreateDC16	(WING.1001)
 */
HDC16 WinGCreateDC16(void)
{
	/* FIXME: Probably wrong... */
	return CreateDC("DISPLAY", NULL, NULL, NULL);
}


/***********************************************************************
 *  WinGRecommendDIBFormat16    (WING.1002)
 */
BOOL16 WinGRecommendDIBFormat16(BITMAPINFO *fmt)
{
	HDC16 i=GetDC16(0);

	fmt->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	fmt->bmiHeader.biWidth=0;
	fmt->bmiHeader.biHeight=1; /* The important part */
	fmt->bmiHeader.biPlanes=GetDeviceCaps(i, PLANES);
	fmt->bmiHeader.biBitCount=GetDeviceCaps(i, BITSPIXEL);
	fmt->bmiHeader.biCompression=BI_RGB;
	fmt->bmiHeader.biSizeImage=0;
	fmt->bmiHeader.biXPelsPerMeter=1000/25.4*GetDeviceCaps(i,LOGPIXELSX);
	fmt->bmiHeader.biYPelsPerMeter=1000/25.4*GetDeviceCaps(i,LOGPIXELSY);
	fmt->bmiHeader.biClrUsed=0;
	fmt->bmiHeader.biClrImportant=0;
	ReleaseDC16(0, i);
	return 1;
}

/***********************************************************************
 *        WinGCreateBitmap16    (WING.1003)
 */
HBITMAP16 WinGCreateBitmap16(HDC16 winDC, BITMAPINFO *header, void **bits)
{
        fprintf(stdnimp,"WinGCreateBitmap: almost empty stub! (expect failure)\n");
	/* Assume RGB color */
	if(bits==NULL)
		return CreateDIBitmap(winDC, header, 0, bits, header, 0);
	else
		return CreateDIBitmap(winDC, header, 1, bits, header, 0);
	return 0;
}

/***********************************************************************
 *  WinGGetDIBPointer16   (WING.1004)
 */
void* WinGGetDIBPointer16(HBITMAP16 bmap, BITMAPINFO *header)
{
        fprintf(stdnimp,"WinGGetDIBPointer16: empty stub!\n");
	return NULL;
}

/***********************************************************************
 *  WinGGetDIBColorTable16   (WING.1005)
 */
UINT16 WinGGetDIBColorTable16(HDC16 winDC, UINT16 start, UINT16 numentry, 
                            RGBQUAD* colors)
{
	return GetPaletteEntries(winDC, start, numentry, colors);
}

/***********************************************************************
 *  WinGSetDIBColorTable16   (WING.1006)
 */
UINT16 WinGSetDIBColorTable16(HDC16 winDC, UINT16 start, UINT16 numentry,
                              RGBQUAD* colors)
{
	return SetPaletteEntries(winDC, start, numentry, colors);
}


/***********************************************************************
 *  WinGCreateHalfTonePalette16   (WING.1007)
 */
HPALETTE16 WinGCreateHalfTonePalette16(void)
{
        fprintf(stdnimp,"WinGCreateHalfTonePalette16: empty stub!\n");
	return 0;
}

/***********************************************************************
 *  WinGCreateHalfToneBrush16   (WING.1008)
 */
HPALETTE16 WinGCreateHalfToneBrush16(HDC16 winDC, COLORREF col, INT16 dithertype)
{
        fprintf(stdnimp,"WinGCreateHalfToneBrush16: empty stub!\n");
	return 0;
}

/***********************************************************************
 *  WinGStretchBlt16   (WING.1009)
 */
BOOL16 WinGStretchBlt16(HDC16 destDC, INT16 xDest, INT16 yDest, INT16 widDest, 
                        INT16 heiDest, HDC16 srcDC, INT16 xSrc, INT16 ySrc, 
			INT16 widSrc, INT16 heiSrc)
{

	return StretchBlt16(destDC, xDest, yDest, widDest, heiDest, srcDC, xSrc, ySrc, widSrc, heiSrc, SRCCOPY);
/*        fprintf(stdnimp,"WinGStretchBlt16: empty stub!\n");*/
/*	return 0; */
}

/***********************************************************************
 *  WinGBitBlt16   (WING.1010)
 */
BOOL16 WinGBitBlt16(HDC16 destDC, INT16 xDest, INT16 yDest, INT16 widDest,
                    INT16 heiDest, HDC16 srcDC, INT16 xSrc, INT16 ySrc)
{
	return BitBlt16(destDC, xDest, yDest, widDest, heiDest, srcDC, xSrc, ySrc, SRCCOPY);
/*        fprintf(stdnimp,"WinGBitBlt16: empty stub!\n");*/
/*	return 0;*/
}

