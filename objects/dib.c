/*
 * GDI device-independent bitmaps
 *
 * Copyright 1993,1994  Alexandre Julliard
 *
 * TODO: Still contains some references to X11DRV.
 */

#include "ts_xlib.h"
#include "x11drv.h"

#include <stdlib.h>
#include "dc.h"
#include "bitmap.h"
#include "callback.h"
#include "palette.h"
#include "global.h"
#include "selectors.h"
#include "debug.h"
#include "local.h"
#include "xmalloc.h" /* for XCREATEIMAGE macro */
#include "monitor.h"


/***********************************************************************
 *           DIB_GetDIBWidthBytes
 *
 * Return the width of a DIB bitmap in bytes. DIB bitmap data is 32-bit aligned.
 * http://www.microsoft.com/msdn/sdk/platforms/doc/sdk/win32/struc/src/str01.htm
 */
int DIB_GetDIBWidthBytes( int width, int depth )
{
    int words;

    switch(depth)
    {
	case 1:  words = (width + 31) / 32; break;
	case 4:  words = (width + 7) / 8; break;
	case 8:  words = (width + 3) / 4; break;
	case 15:
	case 16: words = (width + 1) / 2; break;
	case 24: words = (width * 3 + 3)/4; break;

	default:
            WARN(bitmap, "(%d): Unsupported depth\n", depth );
	/* fall through */
	case 32:
	        words = width;
    }
    return 4 * words;
}


/***********************************************************************
 *           DIB_BitmapInfoSize
 *
 * Return the size of the bitmap info structure including color table.
 */
int DIB_BitmapInfoSize( BITMAPINFO * info, WORD coloruse )
{
    int colors;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)info;
        colors = (core->bcBitCount <= 8) ? 1 << core->bcBitCount : 0;
        return sizeof(BITMAPCOREHEADER) + colors *
             ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
    }
    else  /* assume BITMAPINFOHEADER */
    {
        colors = info->bmiHeader.biClrUsed;
        if (!colors && (info->bmiHeader.biBitCount <= 8))
            colors = 1 << info->bmiHeader.biBitCount;
        return sizeof(BITMAPINFOHEADER) + colors *
               ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}


/***********************************************************************
 *           DIB_GetBitmapInfo
 *
 * Get the info from a bitmap header.
 * Return 1 for INFOHEADER, 0 for COREHEADER, -1 for error.
 */
int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, DWORD *width,
                              int *height, WORD *bpp, WORD *compr )
{
    if (header->biSize == sizeof(BITMAPINFOHEADER))
    {
        *width  = header->biWidth;
        *height = header->biHeight;
        *bpp    = header->biBitCount;
        *compr  = header->biCompression;
        return 1;
    }
    if (header->biSize == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)header;
        *width  = core->bcWidth;
        *height = core->bcHeight;
        *bpp    = core->bcBitCount;
        *compr  = 0;
        return 0;
    }
    WARN(bitmap, "(%ld): wrong size for header\n", header->biSize );
    return -1;
}


/***********************************************************************
 *           StretchDIBits16   (GDI.439)
 */
INT16 WINAPI StretchDIBits16(HDC16 hdc, INT16 xDst, INT16 yDst, INT16 widthDst,
                       INT16 heightDst, INT16 xSrc, INT16 ySrc, INT16 widthSrc,
                       INT16 heightSrc, const VOID *bits,
                       const BITMAPINFO *info, UINT16 wUsage, DWORD dwRop )
{
    return (INT16)StretchDIBits( hdc, xDst, yDst, widthDst, heightDst,
                                   xSrc, ySrc, widthSrc, heightSrc, bits,
                                   info, wUsage, dwRop );
}


/***********************************************************************
 *           StretchDIBits32   (GDI32.351)
 */
INT WINAPI StretchDIBits(HDC hdc, INT xDst, INT yDst, INT widthDst,
                       INT heightDst, INT xSrc, INT ySrc, INT widthSrc,
                       INT heightSrc, const void *bits,
                       const BITMAPINFO *info, UINT wUsage, DWORD dwRop )
{
    DC *dc = DC_GetDCPtr( hdc );
    if(!dc) return FALSE;

    if(dc->funcs->pStretchDIBits)
    	   return dc->funcs->pStretchDIBits(dc, xDst, yDst, widthDst, 
					    heightDst, xSrc, ySrc, widthSrc,
					    heightSrc, bits, info, wUsage,
					    dwRop);
    else { /* use StretchBlt32 */
        HBITMAP hBitmap, hOldBitmap;
	HDC hdcMem;
    
	hBitmap = CreateDIBitmap( hdc, &info->bmiHeader, CBM_INIT,
				    bits, info, wUsage );
	hdcMem = CreateCompatibleDC( hdc );
	hOldBitmap = SelectObject( hdcMem, hBitmap );
        /* Origin for DIBitmap is bottom left ! */
        StretchBlt( hdc, xDst, yDst, widthDst, heightDst,
                      hdcMem, xSrc, info->bmiHeader.biHeight - heightSrc - ySrc, 
                      widthSrc, heightSrc, dwRop );
	SelectObject( hdcMem, hOldBitmap );
	DeleteDC( hdcMem );
	DeleteObject( hBitmap );
	return heightSrc;
    }
}


/***********************************************************************
 *           SetDIBits16    (GDI.440)
 */
INT16 WINAPI SetDIBits16( HDC16 hdc, HBITMAP16 hbitmap, UINT16 startscan,
                          UINT16 lines, LPCVOID bits, const BITMAPINFO *info,
                          UINT16 coloruse )
{
    return SetDIBits( hdc, hbitmap, startscan, lines, bits, info, coloruse );
}


/******************************************************************************
 * SetDIBits32 [GDI32.312]  Sets pixels in a bitmap using colors from DIB
 *
 * PARAMS
 *    hdc       [I] Handle to device context
 *    hbitmap   [I] Handle to bitmap
 *    startscan [I] Starting scan line
 *    lines     [I] Number of scan lines
 *    bits      [I] Array of bitmap bits
 *    info      [I] Address of structure with data
 *    coloruse  [I] Type of color indexes to use
 *
 * RETURNS
 *    Success: Number of scan lines copied
 *    Failure: 0
 */
INT WINAPI SetDIBits( HDC hdc, HBITMAP hbitmap, UINT startscan,
                          UINT lines, LPCVOID bits, const BITMAPINFO *info,
                          UINT coloruse )
{
    DIB_SETIMAGEBITS_DESCR descr;
    BITMAPOBJ * bmp;
    int height, tmpheight;
    INT result;

      /* Check parameters */

    descr.dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!descr.dc) 
    {
	descr.dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!descr.dc) return 0;
    }
    if (!(bmp = (BITMAPOBJ *)GDI_GetObjPtr( hbitmap, BITMAP_MAGIC )))
    {
        GDI_HEAP_UNLOCK( hdc );
        return 0;
    }
    if (DIB_GetBitmapInfo( &info->bmiHeader, &descr.infoWidth, &height,
                           &descr.infoBpp, &descr.compression ) == -1)
    {
        GDI_HEAP_UNLOCK( hbitmap );
        GDI_HEAP_UNLOCK( hdc );
        return 0;
    }
    tmpheight = height;
    if (height < 0) height = -height;
    if (!lines || (startscan >= height))
    {
        GDI_HEAP_UNLOCK( hbitmap );
        GDI_HEAP_UNLOCK( hdc );
        return 0;
    }
    if (startscan + lines > height) lines = height - startscan;

    if (descr.infoBpp <= 8)
    {
        descr.colorMap = X11DRV_DIB_BuildColorMap( descr.dc, coloruse,
						   bmp->bitmap.bmBitsPixel,
						   info, &descr.nColorMap );
        if (!descr.colorMap)
        {
            GDI_HEAP_UNLOCK( hbitmap );
            GDI_HEAP_UNLOCK( hdc );
            return 0;
        } 
    } else
    	descr.colorMap = 0;

    /* HACK for now */
    if(!bmp->DDBitmap)
        X11DRV_CreateBitmap(hbitmap);
{
    X11DRV_PHYSBITMAP *pbitmap = bmp->DDBitmap->physBitmap;


    descr.bits      = bits;
    descr.image     = NULL;
    descr.lines     = tmpheight >= 0 ? lines : -lines;
    descr.depth     = bmp->bitmap.bmBitsPixel;
    descr.drawable  = pbitmap->pixmap;
    descr.gc        = BITMAP_GC(bmp);
    descr.xSrc      = 0;
    descr.ySrc      = 0;
    descr.xDest     = 0;
    descr.yDest     = height - startscan - lines;
    descr.width     = bmp->bitmap.bmWidth;
    descr.height    = lines;
}
    EnterCriticalSection( &X11DRV_CritSection );
    result = CALL_LARGE_STACK( X11DRV_DIB_SetImageBits, &descr );
    LeaveCriticalSection( &X11DRV_CritSection );

    if (descr.colorMap) HeapFree(GetProcessHeap(), 0, descr.colorMap);

    GDI_HEAP_UNLOCK( hdc );
    GDI_HEAP_UNLOCK( hbitmap );
    return result;
}


/***********************************************************************
 *           SetDIBitsToDevice16    (GDI.443)
 */
INT16 WINAPI SetDIBitsToDevice16(HDC16 hdc, INT16 xDest, INT16 yDest, INT16 cx,
                           INT16 cy, INT16 xSrc, INT16 ySrc, UINT16 startscan,
                           UINT16 lines, LPCVOID bits, const BITMAPINFO *info,
                           UINT16 coloruse )
{
    return SetDIBitsToDevice( hdc, xDest, yDest, cx, cy, xSrc, ySrc,
                                startscan, lines, bits, info, coloruse );
}


/***********************************************************************
 *           SetDIBitsToDevice32   (GDI32.313)
 */
INT WINAPI SetDIBitsToDevice(HDC hdc, INT xDest, INT yDest, DWORD cx,
                           DWORD cy, INT xSrc, INT ySrc, UINT startscan,
                           UINT lines, LPCVOID bits, const BITMAPINFO *info,
                           UINT coloruse )
{
    INT ret;
    DC *dc;

    if (!(dc = DC_GetDCPtr( hdc ))) return 0;

    if(dc->funcs->pSetDIBitsToDevice)
        ret = dc->funcs->pSetDIBitsToDevice( dc, xDest, yDest, cx, cy, xSrc,
					     ySrc, startscan, lines, bits,
					     info, coloruse );
    else {
        FIXME(bitmap, "unimplemented on hdc %08x\n", hdc);
	ret = 0;
    }

    GDI_HEAP_UNLOCK( hdc );
    return ret;
}

/***********************************************************************
 *           SetDIBColorTable16    (GDI.602)
 */
UINT16 WINAPI SetDIBColorTable16( HDC16 hdc, UINT16 startpos, UINT16 entries,
				  RGBQUAD *colors )
{
    return SetDIBColorTable( hdc, startpos, entries, colors );
}

/***********************************************************************
 *           SetDIBColorTable32    (GDI32.311)
 */
UINT WINAPI SetDIBColorTable( HDC hdc, UINT startpos, UINT entries,
				  RGBQUAD *colors )
{
    DC * dc;
    PALETTEENTRY * palEntry;
    PALETTEOBJ * palette;
    RGBQUAD *end;

    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }

    if (!(palette = (PALETTEOBJ*)GDI_GetObjPtr( dc->w.hPalette, PALETTE_MAGIC )))
    {
        return 0;
    }

    /* Transfer color info */
    
    if (dc->w.bitsPerPixel <= 8) {
	palEntry = palette->logpalette.palPalEntry + startpos;
	if (startpos + entries > (1 << dc->w.bitsPerPixel)) {
	    entries = (1 << dc->w.bitsPerPixel) - startpos;
	}
	for (end = colors + entries; colors < end; palEntry++, colors++)
	{
	    palEntry->peRed   = colors->rgbRed;
	    palEntry->peGreen = colors->rgbGreen;
	    palEntry->peBlue  = colors->rgbBlue;
	}
    } else {
	entries = 0;
    }
    GDI_HEAP_UNLOCK( dc->w.hPalette );
    return entries;
}

/***********************************************************************
 *           GetDIBColorTable16    (GDI.603)
 */
UINT16 WINAPI GetDIBColorTable16( HDC16 hdc, UINT16 startpos, UINT16 entries,
				  RGBQUAD *colors )
{
    return GetDIBColorTable( hdc, startpos, entries, colors );
}

/***********************************************************************
 *           GetDIBColorTable32    (GDI32.169)
 */
UINT WINAPI GetDIBColorTable( HDC hdc, UINT startpos, UINT entries,
				  RGBQUAD *colors )
{
    DC * dc;
    PALETTEENTRY * palEntry;
    PALETTEOBJ * palette;
    RGBQUAD *end;

    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }

    if (!(palette = (PALETTEOBJ*)GDI_GetObjPtr( dc->w.hPalette, PALETTE_MAGIC )))
    {
        return 0;
    }

    /* Transfer color info */
    
    if (dc->w.bitsPerPixel <= 8) {
	palEntry = palette->logpalette.palPalEntry + startpos;
	if (startpos + entries > (1 << dc->w.bitsPerPixel)) {
	    entries = (1 << dc->w.bitsPerPixel) - startpos;
	}
	for (end = colors + entries; colors < end; palEntry++, colors++)
	{
	    colors->rgbRed      = palEntry->peRed;
	    colors->rgbGreen    = palEntry->peGreen;
	    colors->rgbBlue     = palEntry->peBlue;
	    colors->rgbReserved = 0;
	}
    } else {
	entries = 0;
    }
    GDI_HEAP_UNLOCK( dc->w.hPalette );
    return entries;
}

/* FIXME the following two structs should be combined with __sysPalTemplate in
   objects/color.c - this should happen after de-X11-ing both of these
   files.
   NB. RGBQUAD and PALETTENTRY have different orderings of red, green
   and blue - sigh */

static RGBQUAD EGAColors[16] = { 
/* rgbBlue, rgbGreen, rgbRed, rgbReserverd */
    { 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x80, 0x80, 0x00 },
    { 0x80, 0x00, 0x00, 0x00 },
    { 0x80, 0x00, 0x80, 0x00 },
    { 0x80, 0x80, 0x00, 0x00 },
    { 0x80, 0x80, 0x80, 0x00 },
    { 0xc0, 0xc0, 0xc0, 0x00 },
    { 0x00, 0x00, 0xff, 0x00 },
    { 0x00, 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0xff, 0x00 },
    { 0xff, 0x00, 0x00, 0x00 },
    { 0xff, 0x00, 0xff, 0x00 },
    { 0xff, 0xff, 0x00, 0x00 },
    { 0xff, 0xff, 0xff, 0x00 }
};


static RGBQUAD DefLogPalette[20] = { /* Copy of Default Logical Palette */
/* rgbBlue, rgbGreen, rgbRed, rgbReserverd */
    { 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x80, 0x80, 0x00 },
    { 0x80, 0x00, 0x00, 0x00 },
    { 0x80, 0x00, 0x80, 0x00 },
    { 0x80, 0x80, 0x00, 0x00 },
    { 0xc0, 0xc0, 0xc0, 0x00 },
    { 0xc0, 0xdc, 0xc0, 0x00 },
    { 0xf0, 0xca, 0xa6, 0x00 },
    { 0xf0, 0xfb, 0xff, 0x00 },
    { 0xa4, 0xa0, 0xa0, 0x00 },
    { 0x80, 0x80, 0x80, 0x00 },
    { 0x00, 0x00, 0xf0, 0x00 },
    { 0x00, 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0xff, 0x00 },
    { 0xff, 0x00, 0x00, 0x00 },
    { 0xff, 0x00, 0xff, 0x00 },
    { 0xff, 0xff, 0x00, 0x00 },
    { 0xff, 0xff, 0xff, 0x00 }
};

/*********************************************************************
 *         DIB_GetNearestIndex
 *
 * Helper for GetDIBits.
 * Returns the nearest colour table index for a given RGB.
 * Nearest is defined by minimizing the sum of the squares.
 */
static INT DIB_GetNearestIndex(BITMAPINFO *info, BYTE r, BYTE g, BYTE b)
{
    INT i, best = -1, diff, bestdiff = -1;
    RGBQUAD *color;

    for(color = info->bmiColors, i = 0; i < (1 << info->bmiHeader.biBitCount);
	color++, i++) {
        diff = (r - color->rgbRed) * (r - color->rgbRed) +
	       (g - color->rgbGreen) * (g - color->rgbGreen) +
	       (b - color->rgbBlue) * (b - color->rgbBlue);
	if(diff == 0)
	    return i;
	if(best == -1 || diff < bestdiff) {
	    best = i;
	    bestdiff = diff;
	}
    }
    return best;
}


/***********************************************************************
 *           GetDIBits16    (GDI.441)
 */
INT16 WINAPI GetDIBits16( HDC16 hdc, HBITMAP16 hbitmap, UINT16 startscan,
                          UINT16 lines, LPVOID bits, BITMAPINFO * info,
                          UINT16 coloruse )
{
    return GetDIBits( hdc, hbitmap, startscan, lines, bits, info, coloruse );
}


/******************************************************************************
 * GetDIBits32 [GDI32.170]  Retrieves bits of bitmap and copies to buffer
 *
 * RETURNS
 *    Success: Number of scan lines copied from bitmap
 *    Failure: 0
 *
 * http://www.microsoft.com/msdn/sdk/platforms/doc/sdk/win32/func/src/f30_14.htm
 */
INT WINAPI GetDIBits(
    HDC hdc,         /* [in]  Handle to device context */
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    UINT startscan,  /* [in]  First scan line to set in dest bitmap */
    UINT lines,      /* [in]  Number of scan lines to copy */
    LPVOID bits,       /* [out] Address of array for bitmap bits */
    BITMAPINFO * info, /* [out] Address of structure with bitmap data */
    UINT coloruse)   /* [in]  RGB or palette index */
{
    DC * dc;
    BITMAPOBJ * bmp;
    PALETTEENTRY * palEntry;
    PALETTEOBJ * palette;
    XImage * bmpImage;
    int i, x, y;

    if (!lines) return 0;
    if (!info) return 0;
    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }
    if (!(bmp = (BITMAPOBJ *)GDI_GetObjPtr( hbitmap, BITMAP_MAGIC )))
	return 0;
    if (!(palette = (PALETTEOBJ*)GDI_GetObjPtr( dc->w.hPalette, PALETTE_MAGIC )))
    {
        GDI_HEAP_UNLOCK( hbitmap );
        return 0;
    }

    /* Transfer color info */

    if (info->bmiHeader.biBitCount <= 8 && info->bmiHeader.biBitCount > 0 ) {

	info->bmiHeader.biClrUsed = 0;

	if(info->bmiHeader.biBitCount >= bmp->bitmap.bmBitsPixel) {
	    palEntry = palette->logpalette.palPalEntry;
	    for (i = 0; i < (1 << bmp->bitmap.bmBitsPixel); i++, palEntry++) {
		if (coloruse == DIB_RGB_COLORS) {
		    info->bmiColors[i].rgbRed      = palEntry->peRed;
		    info->bmiColors[i].rgbGreen    = palEntry->peGreen;
		    info->bmiColors[i].rgbBlue     = palEntry->peBlue;
		    info->bmiColors[i].rgbReserved = 0;
		}
		else ((WORD *)info->bmiColors)[i] = (WORD)i;
	    }
	} else {
	    switch (info->bmiHeader.biBitCount) {
	    case 1:
	        info->bmiColors[0].rgbRed = info->bmiColors[0].rgbGreen =
		  info->bmiColors[0].rgbBlue = 0;
		info->bmiColors[0].rgbReserved = 0;
		info->bmiColors[1].rgbRed = info->bmiColors[1].rgbGreen =
		  info->bmiColors[1].rgbBlue = 0xff;
		info->bmiColors[1].rgbReserved = 0;
		break;

	    case 4:
	        memcpy(info->bmiColors, EGAColors, sizeof(EGAColors));
		break;

	    case 8:
	        {
	        INT r, g, b;
		RGBQUAD *color;

		memcpy(info->bmiColors, DefLogPalette,
		       10 * sizeof(RGBQUAD));
		memcpy(info->bmiColors + 246, DefLogPalette + 10,
		       10 * sizeof(RGBQUAD));
		color = info->bmiColors + 10;
		for(r = 0; r <= 5; r++) /* FIXME */
		    for(g = 0; g <= 5; g++)
		        for(b = 0; b <= 5; b++) {
			    color->rgbRed =   (r * 0xff) / 5;
			    color->rgbGreen = (g * 0xff) / 5;
			    color->rgbBlue =  (b * 0xff) / 5;
			    color->rgbReserved = 0;
			    color++;
			}
		}
	    }
	}
    }

    if (bits)
    {	
	BYTE    *bbits = (BYTE*)bits, *linestart;
	int	dstwidth, yend, xend = bmp->bitmap.bmWidth;

        TRACE(bitmap, "%u scanlines of (%i,%i) -> (%i,%i) starting from %u\n",
	      lines, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
	      (int)info->bmiHeader.biWidth, (int)info->bmiHeader.biHeight,
	      startscan );

	/* adjust number of scanlines to copy */

	if( lines > info->bmiHeader.biHeight )
	    lines = info->bmiHeader.biHeight;

	yend = startscan + lines;
	if( startscan >= bmp->bitmap.bmHeight ) 
        {
            GDI_HEAP_UNLOCK( hbitmap );
            GDI_HEAP_UNLOCK( dc->w.hPalette );
            return FALSE;
        }
	if( yend > bmp->bitmap.bmHeight ) yend = bmp->bitmap.bmHeight;

	/* adjust scanline width */

	if(bmp->bitmap.bmWidth > info->bmiHeader.biWidth)
            xend = info->bmiHeader.biWidth;

	/* HACK for now */
	if(!bmp->DDBitmap)
	    X11DRV_CreateBitmap(hbitmap);

	dstwidth = DIB_GetDIBWidthBytes( info->bmiHeader.biWidth,
					 info->bmiHeader.biBitCount );

        EnterCriticalSection( &X11DRV_CritSection );
	bmpImage = (XImage *)CALL_LARGE_STACK( X11DRV_BITMAP_GetXImage, bmp );

	linestart = bbits;
	switch( info->bmiHeader.biBitCount ) {

	case 1: /* 1 bit DIB */
	    {
	        unsigned long white = (1 << bmp->bitmap.bmBitsPixel) - 1;

		for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        if (!(x&7)) *bbits = 0;
			*bbits |= (XGetPixel( bmpImage, x, y) >= white) 
			  << (7 - (x&7));
			if ((x&7)==7) bbits++;
		    }
		    bbits = (linestart += dstwidth);
		}
	    }
	    break;


	case 4: /* 4 bit DIB */
	    switch(bmp->bitmap.bmBitsPixel) {

	    case 1: /* 1/4 bit bmp -> 4 bit DIB */
	    case 4:
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        if (!(x&1)) *bbits = 0;
			*bbits |= XGetPixel( bmpImage, x, y)<<(4*(1-(x&1)));
			if ((x&1)==1) bbits++;
		    }
		    bbits = (linestart += dstwidth);
		}
		break;

	    case 8: /* 8 bit bmp -> 4 bit DIB */
	        palEntry = palette->logpalette.palPalEntry;
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        unsigned long pixel = XGetPixel( bmpImage, x, y );
			if (!(x&1)) *bbits = 0;
			*bbits |= ( DIB_GetNearestIndex(info,
				    palEntry[pixel].peRed,
				    palEntry[pixel].peGreen,
				    palEntry[pixel].peBlue )
					  << (4*(1-(x&1))) );
			if ((x&1)==1) bbits++;
		    }
		    bbits = (linestart += dstwidth);
		}
		break;

	    case 15: /* 15/16 bit bmp -> 4 bit DIB */
	    case 16:
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        unsigned long pixel = XGetPixel( bmpImage, x, y );
			if (!(x&1)) *bbits = 0;
			*bbits |= ( DIB_GetNearestIndex(info,
				     ((pixel << 3) & 0xf8) |
				     ((pixel >> 2) &  0x7),
				     ((pixel >> 2) & 0xf8) |
				     ((pixel >> 7) & 0x7),
				     ((pixel >> 7) & 0xf8) |
				     ((pixel >> 12) & 0x7) ) 
					  << (4*(1-(x&1))) );
			if ((x&1)==1) bbits++;
		    }
		    bbits = (linestart += dstwidth);
		}
		break;

	    case 24: /* 24/32 bit bmp -> 4 bit DIB */
	    case 32:
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        unsigned long pixel = XGetPixel( bmpImage, x, y );
			if (!(x&1)) *bbits = 0;
			*bbits |= ( DIB_GetNearestIndex( info,
				     (pixel >> 16) & 0xff,
				     (pixel >>  8) & 0xff,
				      pixel & 0xff )
					  << (4*(1-(x&1))) );
			if ((x&1)==1) bbits++;
		    }
		    bbits = (linestart += dstwidth);
		}
		break;

	    default: /* ? bit bmp -> 4 bit DIB */
	        FIXME(bitmap, "4 bit DIB %d bit bitmap\n",
				  bmp->bitmap.bmBitsPixel);
		break;
	    }
	    break;


	case 8: /* 8 bit DIB */
	    switch(bmp->bitmap.bmBitsPixel) {

	    case 1: /* 1/4/8 bit bmp -> 8 bit DIB */
	    case 4:
	    case 8:
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ )
		        *bbits++ = XGetPixel( bmpImage, x, y );
		    bbits = (linestart += dstwidth);
		}
		break;

	    case 15: /* 15/16 bit bmp -> 8 bit DIB */
	    case 16:
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        unsigned long pixel = XGetPixel( bmpImage, x, y );
		        *bbits++ = DIB_GetNearestIndex( info, 
					 ((pixel << 3) & 0xf8) |
					 ((pixel >> 2) &  0x7),
					 ((pixel >> 2) & 0xf8) |
					 ((pixel >> 7) & 0x7),
					 ((pixel >> 7) & 0xf8) |
					 ((pixel >> 12) & 0x7) );
		    }
		    bbits = (linestart += dstwidth);
		}
		break;

	    case 24: /* 24/32 bit bmp -> 8 bit DIB */
	    case 32:
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        unsigned long pixel = XGetPixel( bmpImage, x, y );
		        *bbits++ = DIB_GetNearestIndex( info,
					 (pixel >> 16) & 0xff,
					 (pixel >>  8) & 0xff,
					  pixel & 0xff );
		    }
		    bbits = (linestart += dstwidth);
		}
		break;

	    default: /* ? bit bmp -> 8 bit DIB */
	        FIXME(bitmap, "8 bit DIB %d bit bitmap\n",
				  bmp->bitmap.bmBitsPixel);
		break;
	    }
	    break;


	case 15: /* 15/16 bit DIB */
	case 16:
	    switch(bmp->bitmap.bmBitsPixel) {

	    case 15: /* 15/16 bit bmp -> 16 bit DIB */
	    case 16:
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        unsigned long pixel=XGetPixel( bmpImage, x, y);
			*bbits++ = pixel & 0xff;
			*bbits++ = (pixel >> 8) & 0xff;
		    }
		    bbits = (linestart += dstwidth);
		}
		break;
		   
	    case 24: /* 24/32 bit bmp -> 16 bit DIB */
	    case 32:
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        unsigned long pixel=XGetPixel( bmpImage, x, y);
			*bbits++ = ((pixel >> 6) & 0xe0) |
			           ((pixel >> 3) & 0x1f);
			*bbits++ = ((pixel >> 17) & 0x7c) |
			           ((pixel >> 14) & 0x3);
		    }
		    bbits = (linestart += dstwidth);
		}
		break;

	    case 1: /* 1/4/8 bit bmp -> 16 bit DIB */
	    case 4:
	    case 8:
	        palEntry = palette->logpalette.palPalEntry;
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        unsigned long pixel=XGetPixel( bmpImage, x, y);
			*bbits++ = ((palEntry[pixel].peBlue >> 3) & 0x1f) |
			           ((palEntry[pixel].peGreen << 2) & 0xe0); 
			*bbits++ = ((palEntry[pixel].peGreen >> 6) & 0x3) |
			           ((palEntry[pixel].peRed >> 1) & 0x7c);
		    }
		    bbits = (linestart += dstwidth);
		}
		break;

	    default: /* ? bit bmp -> 16 bit DIB */
	        FIXME(bitmap, "15/16 bit DIB %d bit bitmap\n",
				  bmp->bitmap.bmBitsPixel);
		break;
	    }
	    break;


	case 24: /* 24 bit DIB */
	    switch(bmp->bitmap.bmBitsPixel) {

	    case 24: /* 24/32 bit bmp -> 24 bit DIB */
	    case 32:
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        unsigned long pixel=XGetPixel( bmpImage, x, y);
			*bbits++ = (pixel >>16) & 0xff;
			*bbits++ = (pixel >> 8) & 0xff;
			*bbits++ =  pixel       & 0xff;
		    }
		    bbits = (linestart += dstwidth);
		}
		break;

	    case 15: /* 15/16 bit bmp -> 24 bit DIB */
	    case 16:
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        unsigned long pixel=XGetPixel( bmpImage, x, y);
		        *bbits++ = ((pixel >> 7) & 0xf8) |
			           ((pixel >> 12) & 0x7);
			*bbits++ = ((pixel >> 2) & 0xf8) |
			           ((pixel >> 7) & 0x7);
			*bbits++ = ((pixel << 3) & 0xf8) |
			           ((pixel >> 2) & 0x7);
		    }
		    bbits = (linestart += dstwidth);
		}
		break;

	    case 1: /* 1/4/8 bit bmp -> 24 bit DIB */
	    case 4:
	    case 8:
	        palEntry = palette->logpalette.palPalEntry;
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        unsigned long pixel=XGetPixel( bmpImage, x, y);
			*bbits++ = palEntry[pixel].peBlue;
			*bbits++ = palEntry[pixel].peGreen;
			*bbits++ = palEntry[pixel].peRed;
		    }
		    bbits = (linestart += dstwidth);
		}
		break;

	    default: /* ? bit bmp -> 24 bit DIB */
	        FIXME(bitmap, "24 bit DIB %d bit bitmap\n",
		      bmp->bitmap.bmBitsPixel);
		break;
	    }
	    break;


	case 32: /* 32 bit DIB */
	    switch(bmp->bitmap.bmBitsPixel) {

	    case 24: /* 24/32 bit bmp -> 32 bit DIB */
	    case 32:
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        unsigned long pixel=XGetPixel( bmpImage, x, y);
			*bbits++ = (pixel >>16) & 0xff;
			*bbits++ = (pixel >> 8) & 0xff;
			*bbits++ =  pixel       & 0xff;
			*bbits++ = 0;
		    }
		    bbits = (linestart += dstwidth);
		}
		break;

	    case 15: /* 15/16 bit bmp -> 32 bit DIB */
	    case 16:
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        unsigned long pixel=XGetPixel( bmpImage, x, y);
		        *bbits++ = ((pixel >> 7) & 0xf8) |
			           ((pixel >> 12) & 0x7);
			*bbits++ = ((pixel >> 2) & 0xf8) |
			           ((pixel >> 7) & 0x7);
			*bbits++ = ((pixel << 3) & 0xf8) |
			           ((pixel >> 2) & 0x7);
			*bbits++ = 0;
		    }
		    bbits = (linestart += dstwidth);
		}
		break;

	    case 1: /* 1/4/8 bit bmp -> 32 bit DIB */
	    case 4:
	    case 8:
	        palEntry = palette->logpalette.palPalEntry;
	        for( y = yend - 1; (int)y >= (int)startscan; y-- ) {
		    for( x = 0; x < xend; x++ ) {
		        unsigned long pixel=XGetPixel( bmpImage, x, y);
			*bbits++ = palEntry[pixel].peBlue;
			*bbits++ = palEntry[pixel].peGreen;
			*bbits++ = palEntry[pixel].peRed;
			*bbits++ = 0;
		    }
		    bbits = (linestart += dstwidth);
		}
		break;

	    default: /* ? bit bmp -> 32 bit DIB */
	        FIXME(bitmap, "32 bit DIB %d bit bitmap\n",
		      bmp->bitmap.bmBitsPixel);
		break;
	    }
	    break;


	default: /* ? bit DIB */
	    FIXME(bitmap,"Unsupported DIB depth %d\n",
		  info->bmiHeader.biBitCount);
	    break;
	}

	XDestroyImage( bmpImage );
        LeaveCriticalSection( &X11DRV_CritSection );

	if(info->bmiHeader.biSizeImage == 0) /* Fill in biSizeImage */
	    info->bmiHeader.biSizeImage = info->bmiHeader.biHeight *
	                    DIB_GetDIBWidthBytes( info->bmiHeader.biWidth,
						  info->bmiHeader.biBitCount );

	if(bbits - (BYTE *)bits > info->bmiHeader.biSizeImage)
	    ERR(bitmap, "Buffer overrun. Please investigate.\n");

	info->bmiHeader.biCompression = 0;
    }
    else if( info->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER) ) 
    {
	/* fill in struct members */

        if( info->bmiHeader.biBitCount == 0)
	{
	    info->bmiHeader.biWidth = bmp->bitmap.bmWidth;
	    info->bmiHeader.biHeight = bmp->bitmap.bmHeight;
	    info->bmiHeader.biPlanes = 1;
	    info->bmiHeader.biBitCount = bmp->bitmap.bmBitsPixel;
	    info->bmiHeader.biSizeImage = bmp->bitmap.bmHeight *
                             DIB_GetDIBWidthBytes( bmp->bitmap.bmWidth,
                                                   bmp->bitmap.bmBitsPixel );
	    info->bmiHeader.biCompression = 0;
	}
	else
	{
	    info->bmiHeader.biSizeImage = info->bmiHeader.biHeight *
	                    DIB_GetDIBWidthBytes( info->bmiHeader.biWidth,
						  info->bmiHeader.biBitCount );
	}
    }

    TRACE(bitmap, "biSizeImage = %ld, biWidth = %ld, biHeight = %ld\n",
	  info->bmiHeader.biSizeImage, info->bmiHeader.biWidth,
	  info->bmiHeader.biHeight);
    GDI_HEAP_UNLOCK( hbitmap );
    GDI_HEAP_UNLOCK( dc->w.hPalette );
    return lines;
}


/***********************************************************************
 *           CreateDIBitmap16    (GDI.442)
 */
HBITMAP16 WINAPI CreateDIBitmap16( HDC16 hdc, const BITMAPINFOHEADER * header,
                            DWORD init, LPCVOID bits, const BITMAPINFO * data,
                            UINT16 coloruse )
{
    return CreateDIBitmap( hdc, header, init, bits, data, coloruse );
}


/***********************************************************************
 *           CreateDIBitmap32    (GDI32.37)
 */
HBITMAP WINAPI CreateDIBitmap( HDC hdc, const BITMAPINFOHEADER *header,
                            DWORD init, LPCVOID bits, const BITMAPINFO *data,
                            UINT coloruse )
{
    HBITMAP handle;
    BOOL fColor;
    DWORD width;
    int height;
    WORD bpp;
    WORD compr;

    if (DIB_GetBitmapInfo( header, &width, &height, &bpp, &compr ) == -1) return 0;
    if (height < 0) height = -height;

    /* Check if we should create a monochrome or color bitmap. */
    /* We create a monochrome bitmap only if it has exactly 2  */
    /* colors, which are either black or white, nothing else.  */
    /* In all other cases, we create a color bitmap.           */

    if (bpp != 1) fColor = TRUE;
    else if ((coloruse != DIB_RGB_COLORS) ||
             (init != CBM_INIT) || !data) fColor = FALSE;
    else
    {
        if (data->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
        {
            RGBQUAD *rgb = data->bmiColors;
            DWORD col = RGB( rgb->rgbRed, rgb->rgbGreen, rgb->rgbBlue );
            if ((col == RGB(0,0,0)) || (col == RGB(0xff,0xff,0xff)))
            {
                rgb++;
                col = RGB( rgb->rgbRed, rgb->rgbGreen, rgb->rgbBlue );
                fColor = ((col != RGB(0,0,0)) && (col != RGB(0xff,0xff,0xff)));
            }
            else fColor = TRUE;
        }
        else if (data->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
        {
            RGBTRIPLE *rgb = ((BITMAPCOREINFO *)data)->bmciColors;
            DWORD col = RGB( rgb->rgbtRed, rgb->rgbtGreen, rgb->rgbtBlue );
            if ((col == RGB(0,0,0)) || (col == RGB(0xff,0xff,0xff)))
            {
                rgb++;
                col = RGB( rgb->rgbtRed, rgb->rgbtGreen, rgb->rgbtBlue );
                fColor = ((col != RGB(0,0,0)) && (col != RGB(0xff,0xff,0xff)));
            }
            else fColor = TRUE;
        }
        else
        {
            WARN(bitmap, "(%ld): wrong size for data\n",
                     data->bmiHeader.biSize );
            return 0;
        }
    }

    /* Now create the bitmap */

    handle = fColor ? CreateBitmap( width, height, 1, MONITOR_GetDepth(&MONITOR_PrimaryMonitor), NULL ) :
                      CreateBitmap( width, height, 1, 1, NULL );
    if (!handle) return 0;

    if (init == CBM_INIT)
        SetDIBits( hdc, handle, 0, height, bits, data, coloruse );
    return handle;
}


/***********************************************************************
 *           DIB_DoProtectDIBSection
 */
static void DIB_DoProtectDIBSection( BITMAPOBJ *bmp, DWORD new_prot )
{
    DIBSECTION *dib = &bmp->dib->dibSection;
    INT effHeight = dib->dsBm.bmHeight >= 0? dib->dsBm.bmHeight
                                             : -dib->dsBm.bmHeight;
    INT totalSize = dib->dsBmih.biSizeImage? dib->dsBmih.biSizeImage
                         : dib->dsBm.bmWidthBytes * effHeight;
    DWORD old_prot;

    VirtualProtect(dib->dsBm.bmBits, totalSize, new_prot, &old_prot);
    TRACE(bitmap, "Changed protection from %ld to %ld\n", 
                  old_prot, new_prot);
}

/***********************************************************************
 *           DIB_DoUpdateDIBSection
 */
static void DIB_DoUpdateDIBSection( BITMAPOBJ *bmp, BOOL toDIB )
{
    DIBSECTIONOBJ *dib = bmp->dib;
    DIB_SETIMAGEBITS_DESCR descr;

    if (DIB_GetBitmapInfo( &dib->dibSection.dsBmih, &descr.infoWidth, &descr.lines,
                           &descr.infoBpp, &descr.compression ) == -1)
	return;

    descr.dc        = NULL;
    descr.image     = dib->image;
    descr.colorMap  = dib->colorMap;
    descr.nColorMap = dib->nColorMap;
    descr.bits      = dib->dibSection.dsBm.bmBits;
    descr.depth     = bmp->bitmap.bmBitsPixel;
    
    /* Hack for now */
    descr.drawable  = ((X11DRV_PHYSBITMAP *)bmp->DDBitmap->physBitmap)->pixmap;
    descr.gc        = BITMAP_GC(bmp);
    descr.xSrc      = 0;
    descr.ySrc      = 0;
    descr.xDest     = 0;
    descr.yDest     = 0;
    descr.width     = bmp->bitmap.bmWidth;
    descr.height    = bmp->bitmap.bmHeight;

    if (toDIB)
    {
        TRACE(bitmap, "Copying from Pixmap to DIB bits\n");
        EnterCriticalSection( &X11DRV_CritSection );
        CALL_LARGE_STACK( X11DRV_DIB_GetImageBits, &descr );
        LeaveCriticalSection( &X11DRV_CritSection );
    }
    else
    {
	TRACE(bitmap, "Copying from DIB bits to Pixmap\n"); 
        EnterCriticalSection( &X11DRV_CritSection );
        CALL_LARGE_STACK( X11DRV_DIB_SetImageBits, &descr );
        LeaveCriticalSection( &X11DRV_CritSection );
    }
}

/***********************************************************************
 *           DIB_FaultHandler
 */
static BOOL DIB_FaultHandler( LPVOID res, LPCVOID addr )
{
    BOOL handled = FALSE;
    BITMAPOBJ *bmp;

    bmp = (BITMAPOBJ *)GDI_GetObjPtr( (HBITMAP)res, BITMAP_MAGIC );
    if (!bmp) return FALSE;

    if (bmp->dib)
        switch (bmp->dib->status)
        {
        case DIB_GdiMod:
            TRACE( bitmap, "called in status DIB_GdiMod\n" );
            DIB_DoProtectDIBSection( bmp, PAGE_READWRITE );
            DIB_DoUpdateDIBSection( bmp, TRUE );
            DIB_DoProtectDIBSection( bmp, PAGE_READONLY );
            bmp->dib->status = DIB_InSync;
            handled = TRUE;
            break;

        case DIB_InSync:
            TRACE( bitmap, "called in status DIB_InSync\n" );
            DIB_DoProtectDIBSection( bmp, PAGE_READWRITE );
            bmp->dib->status = DIB_AppMod;
            handled = TRUE;
            break;

        case DIB_AppMod:
            FIXME( bitmap, "called in status DIB_AppMod: "
                           "this can't happen!\n" );
            break;

        case DIB_NoHandler:
            FIXME( bitmap, "called in status DIB_NoHandler: "
                           "this can't happen!\n" );
            break;
        }

    GDI_HEAP_UNLOCK( (HBITMAP)res );
    return handled;
}

/***********************************************************************
 *           DIB_UpdateDIBSection
 */
void DIB_UpdateDIBSection( DC *dc, BOOL toDIB )
{
    BITMAPOBJ *bmp;

    /* Ensure this is a Compatible DC that has a DIB section selected */

    if (!dc) return;
    if (!(dc->w.flags & DC_MEMORY)) return;

    bmp = (BITMAPOBJ *)GDI_GetObjPtr( dc->w.hBitmap, BITMAP_MAGIC );
    if (!bmp) return;

    if (!bmp->dib)
    {
	GDI_HEAP_UNLOCK(dc->w.hBitmap);
	return;
    }


    if (!toDIB)
    {
        /* Prepare for access to the DIB by GDI functions */

        switch (bmp->dib->status)
        {
        default:
        case DIB_NoHandler:
            DIB_DoUpdateDIBSection( bmp, FALSE );
            break;

        case DIB_GdiMod:
            TRACE( bitmap, "fromDIB called in status DIB_GdiMod\n" );
            /* nothing to do */
            break;

        case DIB_InSync:
            TRACE( bitmap, "fromDIB called in status DIB_InSync\n" );
            /* nothing to do */
            break;

        case DIB_AppMod:
            TRACE( bitmap, "fromDIB called in status DIB_AppMod\n" );
            DIB_DoUpdateDIBSection( bmp, FALSE );
            DIB_DoProtectDIBSection( bmp, PAGE_READONLY );
            bmp->dib->status = DIB_InSync;
            break;
        }
    }
    else
    {
        /* Acknowledge write access to the DIB by GDI functions */

        switch (bmp->dib->status)
        {
        default:
        case DIB_NoHandler:
            DIB_DoUpdateDIBSection( bmp, TRUE );
            break;

        case DIB_GdiMod:
            TRACE( bitmap, "  toDIB called in status DIB_GdiMod\n" );
            /* nothing to do */
            break;

        case DIB_InSync:
            TRACE( bitmap, "  toDIB called in status DIB_InSync\n" );
            DIB_DoProtectDIBSection( bmp, PAGE_NOACCESS );
            bmp->dib->status = DIB_GdiMod;
            break;

        case DIB_AppMod:
            FIXME( bitmap, "  toDIB called in status DIB_AppMod: "
                           "this can't happen!\n" );
            break;
        }
    }

  
    GDI_HEAP_UNLOCK(dc->w.hBitmap);
}

/***********************************************************************
 *           CreateDIBSection16    (GDI.489)
 */
HBITMAP16 WINAPI CreateDIBSection16 (HDC16 hdc, BITMAPINFO *bmi, UINT16 usage,
				     SEGPTR *bits, HANDLE section,
				     DWORD offset)
{
    HBITMAP res = CreateDIBSection(hdc, bmi, usage, NULL, section,
				       offset);

    if ( res )
    {
	BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr(res, BITMAP_MAGIC);
	if ( bmp && bmp->dib )
	{
	    DIBSECTION *dib = &bmp->dib->dibSection;
	    INT height = dib->dsBm.bmHeight >= 0 ?
		dib->dsBm.bmHeight : -dib->dsBm.bmHeight;
	    INT size = dib->dsBmih.biSizeImage ?
		dib->dsBmih.biSizeImage : dib->dsBm.bmWidthBytes * height;
	    if ( dib->dsBm.bmBits )
	    {
		bmp->dib->selector = 
		    SELECTOR_AllocBlock( dib->dsBm.bmBits, size, 
					 SEGMENT_DATA, FALSE, FALSE );
	    }
	    printf("ptr = %p, size =%d, selector = %04x, segptr = %ld\n",
		   dib->dsBm.bmBits, size, bmp->dib->selector,
		   PTR_SEG_OFF_TO_SEGPTR(bmp->dib->selector, 0));
}
	GDI_HEAP_UNLOCK( res );

	if ( bits ) 
	    *bits = PTR_SEG_OFF_TO_SEGPTR( bmp->dib->selector, 0 );
    }

    return res;
}

/***********************************************************************
 *           CreateDIBSection32    (GDI32.36)
 */
HBITMAP WINAPI CreateDIBSection (HDC hdc, BITMAPINFO *bmi, UINT usage,
				     LPVOID *bits,HANDLE section,
				     DWORD offset)
{
    HBITMAP res = 0;
    BITMAPOBJ *bmp = NULL;
    DIBSECTIONOBJ *dib = NULL;
    int *colorMap = NULL;
    int nColorMap;

    /* Fill BITMAP32 structure with DIB data */
    BITMAPINFOHEADER *bi = &bmi->bmiHeader;
    INT effHeight, totalSize;
    BITMAP bm;

    TRACE(bitmap, "format (%ld,%ld), planes %d, bpp %d, size %ld, colors %ld (%s)\n",
	  bi->biWidth, bi->biHeight, bi->biPlanes, bi->biBitCount,
	  bi->biSizeImage, bi->biClrUsed, usage == DIB_PAL_COLORS? "PAL" : "RGB");

    bm.bmType = 0;
    bm.bmWidth = bi->biWidth;
    bm.bmHeight = bi->biHeight;
    bm.bmWidthBytes = DIB_GetDIBWidthBytes(bm.bmWidth, bi->biBitCount);
    bm.bmPlanes = bi->biPlanes;
    bm.bmBitsPixel = bi->biBitCount;
    bm.bmBits = NULL;

    /* Get storage location for DIB bits */
    effHeight = bm.bmHeight >= 0 ? bm.bmHeight : -bm.bmHeight;
    totalSize = bi->biSizeImage? bi->biSizeImage : bm.bmWidthBytes * effHeight;

    if (section)
        bm.bmBits = MapViewOfFile(section, FILE_MAP_ALL_ACCESS, 
                                  0L, offset, totalSize);
    else
	bm.bmBits = VirtualAlloc(NULL, totalSize, 
                                 MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    /* Create Color Map */
    if (bm.bmBits && bm.bmBitsPixel <= 8)
    {
        DC *dc = hdc? (DC *)GDI_GetObjPtr(hdc, DC_MAGIC) : NULL;
        if (hdc && !dc) dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);

        if (!hdc || dc)
            colorMap = X11DRV_DIB_BuildColorMap( dc, usage, bm.bmBitsPixel,
						 bmi, &nColorMap );
        GDI_HEAP_UNLOCK(hdc);
    }

    /* Allocate Memory for DIB and fill structure */
    if (bm.bmBits)
	dib = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DIBSECTIONOBJ));
    if (dib)
    {
	dib->dibSection.dsBm = bm;
	dib->dibSection.dsBmih = *bi;
	/* FIXME: dib->dibSection.dsBitfields ??? */
	dib->dibSection.dshSection = section;
	dib->dibSection.dsOffset = offset;

        dib->status    = DIB_NoHandler;
	dib->selector  = 0;
        
	dib->nColorMap = nColorMap;
        dib->colorMap  = colorMap;
    }

    /* Create Device Dependent Bitmap and add DIB pointer */
    if (dib) 
    {
       res = CreateDIBitmap(hdc, bi, 0, NULL, bmi, usage);
       if (res)
       {
           bmp = (BITMAPOBJ *) GDI_GetObjPtr(res, BITMAP_MAGIC);
           if (bmp)
           {
               bmp->dib = dib;
               /* HACK for now */
               if(!bmp->DDBitmap)
                   X11DRV_CreateBitmap(res); 
           }
       }
    }

    /* Create XImage */
    if (dib && bmp)
        XCREATEIMAGE( dib->image, bm.bmWidth, effHeight, bmp->bitmap.bmBitsPixel );

    /* Clean up in case of errors */
    if (!res || !bmp || !dib || !bm.bmBits || (bm.bmBitsPixel <= 8 && !colorMap))
    {
        TRACE(bitmap, "got an error res=%08x, bmp=%p, dib=%p, bm.bmBits=%p\n",
              res, bmp, dib, bm.bmBits);
	if (bm.bmBits)
        {
            if (section)
                UnmapViewOfFile(bm.bmBits), bm.bmBits = NULL;
            else
                VirtualFree(bm.bmBits, MEM_RELEASE, 0L), bm.bmBits = NULL;
        }

        if (dib && dib->image) { XDestroyImage(dib->image); dib->image = NULL; }
	if (colorMap) { HeapFree(GetProcessHeap(), 0, colorMap); colorMap = NULL; }
	if (dib) { HeapFree(GetProcessHeap(), 0, dib); dib = NULL; }
	if (res) { DeleteObject(res); res = 0; }
    }

    /* Install fault handler, if possible */
    if (bm.bmBits)
    {
        if (VIRTUAL_SetFaultHandler(bm.bmBits, DIB_FaultHandler, (LPVOID)res))
        {
            DIB_DoProtectDIBSection( bmp, PAGE_READONLY );
            if (dib) dib->status = DIB_InSync;
        }
    }

    /* Return BITMAP handle and storage location */
    if (res) GDI_HEAP_UNLOCK(res);
    if (bm.bmBits && bits) *bits = bm.bmBits;
    return res;
}

/***********************************************************************
 *           DIB_DeleteDIBSection
 */
void DIB_DeleteDIBSection( BITMAPOBJ *bmp )
{
    if (bmp && bmp->dib)
    {
        DIBSECTIONOBJ *dib = bmp->dib;

        if (dib->dibSection.dsBm.bmBits)
        {
            if (dib->dibSection.dshSection)
                UnmapViewOfFile(dib->dibSection.dsBm.bmBits);
            else
                VirtualFree(dib->dibSection.dsBm.bmBits, MEM_RELEASE, 0L);
        }

        if (dib->image) 
            XDestroyImage( dib->image );

        if (dib->colorMap)
            HeapFree(GetProcessHeap(), 0, dib->colorMap);

        if (dib->selector)
        {
            WORD count = (GET_SEL_LIMIT( dib->selector ) >> 16) + 1;
            SELECTOR_FreeBlock( dib->selector, count );
        }

        HeapFree(GetProcessHeap(), 0, dib);
        bmp->dib = NULL;
    }
}

/***********************************************************************
 *           DIB_FixColorsToLoadflags
 *
 * Change color table entries when LR_LOADTRANSPARENT or LR_LOADMAP3DCOLORS
 * are in loadflags
 */
void DIB_FixColorsToLoadflags(BITMAPINFO * bmi, UINT loadflags, BYTE pix)
{
  int colors;
  COLORREF c_W, c_S, c_F, c_L, c_C;
  int incr,i;
  RGBQUAD *ptr;

  if (bmi->bmiHeader.biBitCount > 8) return;
  if (bmi->bmiHeader.biSize == sizeof(BITMAPINFOHEADER)) incr = 4;
  else if (bmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER)) incr = 3;
  else {
    WARN(bitmap, "Wrong bitmap header size!\n");
    return;
  }
  colors = bmi->bmiHeader.biClrUsed;
  if (!colors && (bmi->bmiHeader.biBitCount <= 8))
    colors = 1 << bmi->bmiHeader.biBitCount;
  c_W = GetSysColor(COLOR_WINDOW);
  c_S = GetSysColor(COLOR_3DSHADOW);
  c_F = GetSysColor(COLOR_3DFACE);
  c_L = GetSysColor(COLOR_3DLIGHT);
  if (loadflags & LR_LOADTRANSPARENT) {
    switch (bmi->bmiHeader.biBitCount) {
      case 1: pix = pix >> 7; break;
      case 4: pix = pix >> 4; break;
      case 8: break;
      default: 
        WARN(bitmap, "(%d): Unsupported depth\n", bmi->bmiHeader.biBitCount); 
	return;
    }
    if (pix >= colors) {
      WARN(bitmap, "pixel has color index greater than biClrUsed!\n");
      return;
    }
    if (loadflags & LR_LOADMAP3DCOLORS) c_W = c_F;
    ptr = (RGBQUAD*)((char*)bmi->bmiColors+pix*incr);
    ptr->rgbBlue = GetBValue(c_W);
    ptr->rgbGreen = GetGValue(c_W);
    ptr->rgbRed = GetRValue(c_W);
  }
  if (loadflags & LR_LOADMAP3DCOLORS)
    for (i=0; i<colors; i++) {
      ptr = (RGBQUAD*)((char*)bmi->bmiColors+i*incr);
      c_C = RGB(ptr->rgbRed, ptr->rgbGreen, ptr->rgbBlue);
      if (c_C == RGB(128, 128, 128)) { 
	ptr->rgbRed = GetRValue(c_S);
	ptr->rgbGreen = GetGValue(c_S);
	ptr->rgbBlue = GetBValue(c_S);
      } else if (c_C == RGB(192, 192, 192)) { 
	ptr->rgbRed = GetRValue(c_F);
	ptr->rgbGreen = GetGValue(c_F);
	ptr->rgbBlue = GetBValue(c_F);
      } else if (c_C == RGB(223, 223, 223)) { 
	ptr->rgbRed = GetRValue(c_L);
	ptr->rgbGreen = GetGValue(c_L);
	ptr->rgbBlue = GetBValue(c_L);
      } 
    }
}
