/*
 *	PostScript driver bitmap functions
 *
 * Copyright 1998  Huw D M Davies
 *
 */

#include "gdi.h"
#include "psdrv.h"
#include "debug.h"
#include "bitmap.h"
#include "winbase.h"

DEFAULT_DEBUG_CHANNEL(psdrv)


/***************************************************************************
 *                PSDRV_WriteImageHeader
 *
 * Helper for PSDRV_StretchDIBits
 *
 * BUGS
 *  Uses level 2 PostScript
 */

static BOOL PSDRV_WriteImageHeader(DC *dc, const BITMAPINFO *info, INT xDst,
				   INT yDst, INT widthDst, INT heightDst,
				   INT widthSrc, INT heightSrc)
{
    COLORREF map[256];
    int i;

    switch(info->bmiHeader.biBitCount) {
    case 8:
        PSDRV_WriteIndexColorSpaceBegin(dc, 255);
	for(i = 0; i < 256; i++) {
	    map[i] =  info->bmiColors[i].rgbRed |
	      info->bmiColors[i].rgbGreen << 8 |
	      info->bmiColors[i].rgbBlue << 16;
	}
	PSDRV_WriteRGB(dc, map, 256);
	PSDRV_WriteIndexColorSpaceEnd(dc);
	break;

    case 4:
        PSDRV_WriteIndexColorSpaceBegin(dc, 15);
	for(i = 0; i < 16; i++) {
	    map[i] =  info->bmiColors[i].rgbRed |
	      info->bmiColors[i].rgbGreen << 8 |
	      info->bmiColors[i].rgbBlue << 16;
	}
	PSDRV_WriteRGB(dc, map, 16);
	PSDRV_WriteIndexColorSpaceEnd(dc);
	break;

    case 1:
        PSDRV_WriteIndexColorSpaceBegin(dc, 1);
	for(i = 0; i < 2; i++) {
	    map[i] =  info->bmiColors[i].rgbRed |
	      info->bmiColors[i].rgbGreen << 8 |
	      info->bmiColors[i].rgbBlue << 16;
	}
	PSDRV_WriteRGB(dc, map, 2);
	PSDRV_WriteIndexColorSpaceEnd(dc);
	break;

    case 15:
    case 16:
    case 24:
    case 32:
      {
	PSCOLOR pscol;
	pscol.type = PSCOLOR_RGB;
	pscol.value.rgb.r = pscol.value.rgb.g = pscol.value.rgb.b = 0.0;
        PSDRV_WriteSetColor(dc, &pscol);
        break;
      }

    default:
        FIXME(psdrv, "Not implemented yet\n");
	return FALSE;
	break;
    }

    PSDRV_WriteImageDict(dc, info->bmiHeader.biBitCount, xDst, yDst,
			  widthDst, heightDst, widthSrc, heightSrc);
    return TRUE;
}


/***************************************************************************
 *
 *	PSDRV_StretchDIBits
 *
 * BUGS
 *  Doesn't work correctly if the DIB bits aren't byte aligned at the start of
 *  a line - this affects 1 and 4 bit depths.
 *  Compression not implemented.
 */
INT PSDRV_StretchDIBits( DC *dc, INT xDst, INT yDst, INT widthDst,
			 INT heightDst, INT xSrc, INT ySrc,
			 INT widthSrc, INT heightSrc, const void *bits,
			 const BITMAPINFO *info, UINT wUsage, DWORD dwRop )
{
    DWORD fullSrcWidth;
    INT widthbytes, fullSrcHeight;
    WORD bpp, compression;
    const char *ptr;
    INT line;

    TRACE(psdrv, "%08x (%d,%d %dx%d) -> (%d,%d %dx%d)\n", dc->hSelf,
	  xSrc, ySrc, widthSrc, heightSrc, xDst, yDst, widthDst, heightDst);

    DIB_GetBitmapInfo((const BITMAPINFOHEADER *)info, &fullSrcWidth,
		      &fullSrcHeight, &bpp, &compression);

    widthbytes = DIB_GetDIBWidthBytes(fullSrcWidth, bpp);

    TRACE(psdrv, "full size=%ldx%d bpp=%d compression=%d\n", fullSrcWidth,
	  fullSrcHeight, bpp, compression);


    if(compression != BI_RGB) {
        FIXME(psdrv, "Compression not supported\n");
	return FALSE;
    }


    switch(bpp) {

    case 1:
	PSDRV_WriteGSave(dc);
	PSDRV_WriteImageHeader(dc, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);
	ptr = bits;
	ptr += (ySrc * widthbytes);
	if(xSrc & 7 || widthSrc & 7)
	    FIXME(psdrv, "This won't work...\n");
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteBytes(dc, ptr + xSrc/8, widthSrc/8);
	PSDRV_WriteGRestore(dc);
	break;

    case 4:
	PSDRV_WriteGSave(dc);
	PSDRV_WriteImageHeader(dc, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);
	ptr = bits;
	ptr += (ySrc * widthbytes);
	if(xSrc & 1 || widthSrc & 1)
	    FIXME(psdrv, "This won't work...\n");
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteBytes(dc, ptr + xSrc/2, widthSrc/2);
	PSDRV_WriteGRestore(dc);
	break;

    case 8:
	PSDRV_WriteGSave(dc);
	PSDRV_WriteImageHeader(dc, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);
	ptr = bits;
	ptr += (ySrc * widthbytes);
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteBytes(dc, ptr + xSrc, widthSrc);
	PSDRV_WriteGRestore(dc);
	break;

    case 15:
    case 16:
	PSDRV_WriteGSave(dc);
	PSDRV_WriteImageHeader(dc, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);

	ptr = bits;
	ptr += (ySrc * widthbytes);
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteDIBits16(dc, (WORD *)ptr + xSrc, widthSrc);
	PSDRV_WriteGRestore(dc);
	break;

    case 24:
	PSDRV_WriteGSave(dc);
	PSDRV_WriteImageHeader(dc, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);

	ptr = bits;
	ptr += (ySrc * widthbytes);
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteDIBits24(dc, ptr + xSrc * 3, widthSrc);
	PSDRV_WriteGRestore(dc);
	break;

    case 32:
	PSDRV_WriteGSave(dc);
	PSDRV_WriteImageHeader(dc, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);

	ptr = bits;
	ptr += (ySrc * widthbytes);
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteDIBits32(dc, ptr + xSrc * 3, widthSrc);
	PSDRV_WriteGRestore(dc);
	break;

    default:
        FIXME(psdrv, "Unsupported depth\n");
	return FALSE;

    }

    return TRUE;
}






