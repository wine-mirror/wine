/*
 *	PostScript driver bitmap functions
 *
 * Copyright 1998  Huw D M Davies
 *
 */

#include "gdi.h"
#include "psdrv.h"
#include "debugtools.h"
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
        FIXME("Not implemented yet\n");
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

    TRACE("%08x (%d,%d %dx%d) -> (%d,%d %dx%d)\n", dc->hSelf,
	  xSrc, ySrc, widthSrc, heightSrc, xDst, yDst, widthDst, heightDst);

    DIB_GetBitmapInfo((const BITMAPINFOHEADER *)info, &fullSrcWidth,
		      &fullSrcHeight, &bpp, &compression);

    widthbytes = DIB_GetDIBWidthBytes(fullSrcWidth, bpp);

    TRACE("full size=%ldx%d bpp=%d compression=%d\n", fullSrcWidth,
	  fullSrcHeight, bpp, compression);


    if(compression != BI_RGB) {
        FIXME("Compression not supported\n");
	return FALSE;
    }

    xDst = XLPTODP(dc, xDst);
    yDst = YLPTODP(dc, yDst);
    widthDst = XLSTODS(dc, widthDst);
    heightDst = YLSTODS(dc, heightDst);

    switch(bpp) {

    case 1:
	PSDRV_WriteGSave(dc);
	PSDRV_WriteImageHeader(dc, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);
	ptr = bits;
	ptr += (ySrc * widthbytes);
	if(xSrc & 7 || widthSrc & 7)
	    FIXME("This won't work...\n");
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteBytes(dc, ptr + xSrc/8, widthSrc/8);
	break;

    case 4:
	PSDRV_WriteGSave(dc);
	PSDRV_WriteImageHeader(dc, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);
	ptr = bits;
	ptr += (ySrc * widthbytes);
	if(xSrc & 1 || widthSrc & 1)
	    FIXME("This won't work...\n");
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteBytes(dc, ptr + xSrc/2, widthSrc/2);
	break;

    case 8:
	PSDRV_WriteGSave(dc);
	PSDRV_WriteImageHeader(dc, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);
	ptr = bits;
	ptr += (ySrc * widthbytes);
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteBytes(dc, ptr + xSrc, widthSrc);
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
	break;

    case 24:
	PSDRV_WriteGSave(dc);
	PSDRV_WriteImageHeader(dc, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);

	ptr = bits;
	ptr += (ySrc * widthbytes);
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteDIBits24(dc, ptr + xSrc * 3, widthSrc);
	break;

    case 32:
	PSDRV_WriteGSave(dc);
	PSDRV_WriteImageHeader(dc, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);

	ptr = bits;
	ptr += (ySrc * widthbytes);
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteDIBits32(dc, ptr + xSrc * 3, widthSrc);
	break;

    default:
        FIXME("Unsupported depth\n");
	return FALSE;

    }
    PSDRV_WriteSpool(dc, "%\n", 2);  /* some versions of ghostscript seem to
					eat one too many chars after the image
					operator */
    PSDRV_WriteGRestore(dc);
    return TRUE;
}






