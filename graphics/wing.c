/*
 * WingG support
 *
 * Started by Robert Pouliot <krynos@clic.net>
 */

#include "gdi.h"
#include "stddebug.h"
#include "debug.h"

/* I dunno if this structure can be put here... Maybe copyright, I'm no lawyer... */
typedef enum WING_DITHER_TYPE
{
    WING_DISPERSED_4x4,
    WING_DISPERSED_8x8,

    WING_CLUSTERED_4x4

} WING_DITHER_TYPE;

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
        fprintf(stdnimp,"WinGRecommendDIBFormat: empty stub!\n");
	return 0;
}

/***********************************************************************
 *        WinGCreateBitmap16    (WING.1003)
 */
HBITMAP16 WinGCreateBitmap16(HDC16 winDC, BITMAPINFO *header, void **bits)
{
        fprintf(stdnimp,"WinGCreateBitmap: empty stub! (expect failure)\n");
	*bits=0;
	return 0;
}

/***********************************************************************
 *  WinGCreateHalfTonePalette16   (WING.1007)
 */
HPALETTE16 WinGCreateHalfTonePalette16(void)
{
        fprintf(stdnimp,"WinGCreateHalfTonePalette: empty stub!\n");
	return 0;
}

/***********************************************************************
 *  WinGCreateHalfToneBrush16   (WING.1008)
 */
HPALETTE16 WinGCreateHalfToneBrush16(HDC16 winDC, COLORREF col, WING_DITHER_TYPE type)
{
        fprintf(stdnimp,"WinGCreateHalfToneBrush: empty stub!\n");
	return 0;
}

