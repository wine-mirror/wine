/*
 *	PostScript driver bitmap functions
 *
 * Copyright 1998  Huw D M Davies
 *
 */

#include "windows.h"
#include "gdi.h"
#include "psdrv.h"
#include "debug.h"


/***************************************************************************
 *
 *	PSDRV_StretchDIBits
 */
INT32 PSDRV_StretchDIBits( DC *dc, INT32 xDst, INT32 yDst, INT32 widthDst,
			   INT32 heightDst, INT32 xSrc, INT32 ySrc,
			   INT32 widthSrc, INT32 heightSrc, const void *bits,
			   const BITMAPINFO *info, UINT32 wUsage, DWORD dwRop )
{
    TRACE(psdrv, "(%d,%d %dx%d) -> (%d,%d %dx%d) on %08x. %d colour bits\n",
	  xSrc, ySrc, widthSrc, heightSrc, xDst, yDst, widthDst, heightDst,
	  dc->hSelf, info->bmiHeader.biBitCount);


    FIXME(psdrv, "stub\n");
    return FALSE;
}
