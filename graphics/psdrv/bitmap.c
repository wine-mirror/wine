/*
 *	PostScript driver bitmap functions
 *
 * Copyright 1998  Huw D M Davies
 *
 */

#include "gdi.h"
#include "psdrv.h"
#include "debug.h"


/***************************************************************************
 *
 *	PSDRV_StretchDIBits
 */
INT PSDRV_StretchDIBits( DC *dc, INT xDst, INT yDst, INT widthDst,
			   INT heightDst, INT xSrc, INT ySrc,
			   INT widthSrc, INT heightSrc, const void *bits,
			   const BITMAPINFO *info, UINT wUsage, DWORD dwRop )
{
    TRACE(psdrv, "(%d,%d %dx%d) -> (%d,%d %dx%d) on %08x. %d colour bits\n",
	  xSrc, ySrc, widthSrc, heightSrc, xDst, yDst, widthDst, heightDst,
	  dc->hSelf, info->bmiHeader.biBitCount);


    FIXME(psdrv, "stub\n");
    return FALSE;
}
