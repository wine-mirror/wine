/*
 * GDI device-independent bitmaps
 *
 * Copyright 1993,1994  Alexandre Julliard
 *
 */

#include "winbase.h"
#include "bitmap.h"
#include "callback.h"
#include "dc.h"
#include "debugtools.h"
#include "monitor.h"
#include "palette.h"

DEFAULT_DEBUG_CHANNEL(bitmap)

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
            WARN("(%d): Unsupported depth\n", depth );
	/* fall through */
	case 32:
	        words = width;
    }
    return 4 * words;
}

/***********************************************************************
 *           DIB_GetDIBImageBytes
 *
 * Return the number of bytes used to hold the image in a DIB bitmap.
 */
int DIB_GetDIBImageBytes( int width, int height, int depth )
{
    return DIB_GetDIBWidthBytes( width, depth ) * abs( height );
}


/***********************************************************************
 *           DIB_BitmapInfoSize
 *
 * Return the size of the bitmap info structure including color table.
 */
int DIB_BitmapInfoSize( const BITMAPINFO * info, WORD coloruse )
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
    WARN("(%ld): wrong size for header\n", header->biSize );
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
 *           StretchDIBits   (GDI32.351)
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
    else { /* use StretchBlt */
        HBITMAP hBitmap, hOldBitmap;
	HDC hdcMem;

	hBitmap = CreateDIBitmap( hdc, &info->bmiHeader, CBM_INIT,
				    bits, info, wUsage );
	hdcMem = CreateCompatibleDC( hdc );
	hOldBitmap = SelectObject( hdcMem, hBitmap );
        /* Origin for DIBitmap may be bottom left (positive biHeight) or top
           left (negative biHeight) */
        StretchBlt( hdc, xDst, yDst, widthDst, heightDst,
                      hdcMem, xSrc, abs(info->bmiHeader.biHeight) - heightSrc - ySrc, 
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
 * SetDIBits [GDI32.312]  Sets pixels in a bitmap using colors from DIB
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
    DC *dc;
    BITMAPOBJ *bitmap;
    INT result;

    /* Check parameters */
    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *) GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }

    if (!(bitmap = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC )))
    {
        GDI_HEAP_UNLOCK( hdc );
        return 0;
    }

    result = BITMAP_Driver->pSetDIBits(bitmap, dc, startscan, 
				       lines, bits, info, 
				       coloruse, hbitmap);

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
 *           SetDIBitsToDevice   (GDI32.313)
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
        FIXME("unimplemented on hdc %08x\n", hdc);
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
 *           SetDIBColorTable    (GDI32.311)
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
	if (startpos + entries > (1 << dc->w.bitsPerPixel))
	    entries = (1 << dc->w.bitsPerPixel) - startpos;

        if (startpos + entries > palette->logpalette.palNumEntries)
            entries = palette->logpalette.palNumEntries - startpos;

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
 *           GetDIBColorTable    (GDI32.169)
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
 * GetDIBits [GDI32.170]  Retrieves bits of bitmap and copies to buffer
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
    int i;

    if (!info) return 0;
    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }
    if (!(bmp = (BITMAPOBJ *)GDI_GetObjPtr( hbitmap, BITMAP_MAGIC )))
    {
        GDI_HEAP_UNLOCK( hdc );
	return 0;
    }
    if (!(palette = (PALETTEOBJ*)GDI_GetObjPtr( dc->w.hPalette, PALETTE_MAGIC )))
    {
        GDI_HEAP_UNLOCK( hdc );
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

    GDI_HEAP_UNLOCK( dc->w.hPalette );

    if (bits && lines)
    {
        /* If the bitmap object already have a dib section that contains image data, get the bits from it*/
        if(bmp->dib && bmp->dib->dsBm.bmBitsPixel >= 15 && info->bmiHeader.biBitCount >= 15)
        {
            /*FIXME: Only RGB dibs supported for now */
            int srcwidth = bmp->dib->dsBm.bmWidth, srcwidthb = bmp->dib->dsBm.bmWidthBytes;
            int dstwidthb = DIB_GetDIBWidthBytes( info->bmiHeader.biWidth, info->bmiHeader.biBitCount );
            LPBYTE dbits = bits, sbits = (LPBYTE) bmp->dib->dsBm.bmBits + (startscan * srcwidthb);
            int x, y;

            if ((info->bmiHeader.biHeight < 0) ^ (bmp->dib->dsBmih.biHeight < 0))
            {
                dbits = (LPBYTE)bits + (dstwidthb * (lines-1));
                dstwidthb = -dstwidthb;
            }

            switch( info->bmiHeader.biBitCount ) {

	    case 15:
            case 16: /* 16 bpp dstDIB */
                {
                    LPWORD dstbits = (LPWORD)dbits;
                    WORD rmask = 0x7c00, gmask= 0x03e0, bmask = 0x001f;

                    /* FIXME: BI_BITFIELDS not supported yet */

                    switch(bmp->dib->dsBm.bmBitsPixel) {

                    case 16: /* 16 bpp srcDIB -> 16 bpp dstDIB */
                        {
                            /* FIXME: BI_BITFIELDS not supported yet */
                            for (y = 0; y < lines; y++, dbits+=dstwidthb, sbits+=srcwidthb)
                                memcpy(dbits, sbits, srcwidthb);
                        }
                        break;

                    case 24: /* 24 bpp srcDIB -> 16 bpp dstDIB */
                        {
                            LPBYTE srcbits = sbits;

                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < srcwidth; x++ )
                                    *dstbits++ = ((*srcbits++ >> 3) & bmask) |
                                                 (((WORD)*srcbits++ << 2) & gmask) |
                                                 (((WORD)*srcbits++ << 7) & rmask);
                                dstbits = (LPWORD)(dbits+=dstwidthb);
                                srcbits = (sbits += srcwidthb);
                            }
                        }
                        break;

                    case 32: /* 32 bpp srcDIB -> 16 bpp dstDIB */
                        {
                            LPDWORD srcbits = (LPDWORD)sbits;
                            DWORD val;

                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < srcwidth; x++ ) {
                                    val = *srcbits++;
                                    *dstbits++ = (WORD)(((val >> 3) & bmask) | ((val >> 6) & gmask) |
                                                       ((val >> 9) & rmask));
                                }
                                dstbits = (LPWORD)(dbits+=dstwidthb);
                                srcbits = (LPDWORD)(sbits+=srcwidthb);
                            }
                        }
                        break;

                    default: /* ? bit bmp -> 16 bit DIB */
                        FIXME("15/16 bit DIB %d bit bitmap\n",
                        bmp->bitmap.bmBitsPixel);
                        break;
                    }
                }
                break;

            case 24: /* 24 bpp dstDIB */
                {
                    LPBYTE dstbits = dbits;

                    switch(bmp->dib->dsBm.bmBitsPixel) {

                    case 16: /* 16 bpp srcDIB -> 24 bpp dstDIB */
                        {
                            LPWORD srcbits = (LPWORD)sbits;
                            WORD val;

                            /* FIXME: BI_BITFIELDS not supported yet */
                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < srcwidth; x++ ) {
                                    val = *srcbits++;
                                    *dstbits++ = (BYTE)(((val << 3) & 0xf8) | ((val >> 2) & 0x07));
                                    *dstbits++ = (BYTE)(((val >> 2) & 0xf8) | ((val >> 7) & 0x07));
                                    *dstbits++ = (BYTE)(((val >> 7) & 0xf8) | ((val >> 12) & 0x07));
                                }
                                dstbits = (LPBYTE)(dbits+=dstwidthb);
                                srcbits = (LPWORD)(sbits+=srcwidthb);
                            }
                        }
                        break;

                    case 24: /* 24 bpp srcDIB -> 24 bpp dstDIB */
                        {
                            for (y = 0; y < lines; y++, dbits+=dstwidthb, sbits+=srcwidthb)
                                memcpy(dbits, sbits, srcwidthb);
                        }
                        break;

                    case 32: /* 32 bpp srcDIB -> 24 bpp dstDIB */
                        {
                            LPBYTE srcbits = (LPBYTE)sbits;

                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < srcwidth; x++, srcbits++ ) {
                                    *dstbits++ = *srcbits++;
                                    *dstbits++ = *srcbits++;
                                    *dstbits++ = *srcbits++;
                                }
                                dstbits=(LPBYTE)(dbits+=dstwidthb);
                                srcbits = (LPBYTE)(sbits+=srcwidthb);
                            }
                        }
                        break;

                    default: /* ? bit bmp -> 24 bit DIB */
                        FIXME("24 bit DIB %d bit bitmap\n",
                              bmp->bitmap.bmBitsPixel);
                        break;
                    }
                }
                break;

            case 32: /* 32 bpp dstDIB */
                {
                    LPDWORD dstbits = (LPDWORD)dbits;

                    /* FIXME: BI_BITFIELDS not supported yet */

                    switch(bmp->dib->dsBm.bmBitsPixel) {
                        case 16: /* 16 bpp srcDIB -> 32 bpp dstDIB */
                        {
                            LPWORD srcbits = (LPWORD)sbits;
                            DWORD val;

                            /* FIXME: BI_BITFIELDS not supported yet */
                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < srcwidth; x++ ) {
                                    val = (DWORD)*srcbits++;
                                    *dstbits++ = ((val << 3) & 0xf8) | ((val >> 2) & 0x07) |
                                                 ((val << 6) & 0xf800) | ((val << 1) & 0x0700) |
                                                 ((val << 9) & 0xf80000) | ((val << 4) & 0x070000);
                                }
                                dstbits=(LPDWORD)(dbits+=dstwidthb);
                                srcbits=(LPWORD)(sbits+=srcwidthb);
                            }
                        }
                        break;

                    case 24: /* 24 bpp srcDIB -> 32 bpp dstDIB */
                        {
                            LPBYTE srcbits = sbits;

                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < srcwidth; x++, srcbits+=3 )
                                    *dstbits++ = ((DWORD)*srcbits) & 0x00ffffff; 
                                dstbits=(LPDWORD)(dbits+=dstwidthb);
                                srcbits=(sbits+=srcwidthb);
                            }
                        }
                        break;

                    case 32: /* 32 bpp srcDIB -> 16 bpp dstDIB */
                        {
                            /* FIXME: BI_BITFIELDS not supported yet */
                            for (y = 0; y < lines; y++, dbits+=dstwidthb, sbits+=srcwidthb)
                                memcpy(dbits, sbits, srcwidthb);
                        }
                        break;

                    default: /* ? bit bmp -> 16 bit DIB */
                        FIXME("15/16 bit DIB %d bit bitmap\n",
                        bmp->bitmap.bmBitsPixel);
                        break;
                    }
                }
                break;

            default: /* ? bit DIB */
                FIXME("Unsupported DIB depth %d\n", info->bmiHeader.biBitCount);
                break;
            }
        }
        /* Otherwise, get bits from the XImage */
        else if(!BITMAP_Driver->pGetDIBits(bmp, dc, startscan, lines, bits, info, coloruse, hbitmap))
        {
	    GDI_HEAP_UNLOCK( hdc );
	    GDI_HEAP_UNLOCK( hbitmap );

	    return FALSE;
	}
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
	    info->bmiHeader.biSizeImage = 
                             DIB_GetDIBImageBytes( bmp->bitmap.bmWidth,
						   bmp->bitmap.bmHeight,
						   bmp->bitmap.bmBitsPixel );
	    info->bmiHeader.biCompression = 0;
	}
	else
	{
	    info->bmiHeader.biSizeImage = DIB_GetDIBImageBytes(
					       info->bmiHeader.biWidth,
					       info->bmiHeader.biHeight,
					       info->bmiHeader.biBitCount );
	}
    }

    TRACE("biSizeImage = %ld, biWidth = %ld, biHeight = %ld\n",
	  info->bmiHeader.biSizeImage, info->bmiHeader.biWidth,
	  info->bmiHeader.biHeight);

    GDI_HEAP_UNLOCK( hdc );
    GDI_HEAP_UNLOCK( hbitmap );

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
 *           CreateDIBitmap    (GDI32.37)
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
    /* colors, which are black followed by white, nothing else.  */
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

	    /* Check if the first color of the colormap is black */ 
	    if ((col == RGB(0,0,0)))
            {
                rgb++;
                col = RGB( rgb->rgbRed, rgb->rgbGreen, rgb->rgbBlue );
		/* If the second color is white, create a monochrome bitmap */
                fColor =  (col != RGB(0xff,0xff,0xff));
            }
	    /* Note : If the first color of the colormap is white 
	       followed by black, we have to create a color bitmap. 
	       If we don't the white will be displayed in black later on!*/ 
            else fColor = TRUE;
        }
        else if (data->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
        {
            RGBTRIPLE *rgb = ((BITMAPCOREINFO *)data)->bmciColors;
            DWORD col = RGB( rgb->rgbtRed, rgb->rgbtGreen, rgb->rgbtBlue );
            if ((col == RGB(0,0,0)))
            {
                rgb++;
                col = RGB( rgb->rgbtRed, rgb->rgbtGreen, rgb->rgbtBlue );
                fColor = (col != RGB(0xff,0xff,0xff));
            }
            else fColor = TRUE;
        }
        else
        {
            WARN("(%ld): wrong size for data\n",
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
 *           CreateDIBSection16    (GDI.489)
 */
HBITMAP16 WINAPI CreateDIBSection16 (HDC16 hdc, BITMAPINFO *bmi, UINT16 usage,
				     SEGPTR *bits, HANDLE section,
				     DWORD offset)
{
    HBITMAP16 hbitmap;
    DC *dc = (DC *) GDI_GetObjPtr(hdc, DC_MAGIC);
    if(!dc) dc = (DC *) GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
    if(!dc) return (HBITMAP16) NULL;

    hbitmap = dc->funcs->pCreateDIBSection16(dc, bmi, usage, bits, section, offset, 0);

    GDI_HEAP_UNLOCK(hdc);

    return hbitmap;
}

/***********************************************************************
 *           DIB_CreateDIBSection
 */
HBITMAP DIB_CreateDIBSection(HDC hdc, BITMAPINFO *bmi, UINT usage,
			     LPVOID *bits, HANDLE section,
			     DWORD offset, DWORD ovr_pitch)
{
    HBITMAP hbitmap;
    DC *dc = (DC *) GDI_GetObjPtr(hdc, DC_MAGIC);
    if(!dc) dc = (DC *) GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
    if(!dc) return (HBITMAP) NULL;

    hbitmap = dc->funcs->pCreateDIBSection(dc, bmi, usage, bits, section, offset, ovr_pitch);

    GDI_HEAP_UNLOCK(hdc);

    return hbitmap;
}

/***********************************************************************
 *           CreateDIBSection    (GDI32.36)
 */
HBITMAP WINAPI CreateDIBSection(HDC hdc, BITMAPINFO *bmi, UINT usage,
				LPVOID *bits, HANDLE section,
				DWORD offset)
{
    return DIB_CreateDIBSection(hdc, bmi, usage, bits, section, offset, 0);
}

/***********************************************************************
 *           DIB_DeleteDIBSection
 */
void DIB_DeleteDIBSection( BITMAPOBJ *bmp )
{
    if (bmp && bmp->dib)
    {
        DIBSECTION *dib = bmp->dib;

        if (dib->dsBm.bmBits)
        {
            if (dib->dshSection)
                UnmapViewOfFile(dib->dsBm.bmBits);
            else if (!dib->dsOffset)
                VirtualFree(dib->dsBm.bmBits, 0L, MEM_RELEASE );
        }

	BITMAP_Driver->pDeleteDIBSection(bmp);

        HeapFree(GetProcessHeap(), 0, dib);
        bmp->dib = NULL;
    }
}

/***********************************************************************
 *           DIB_CreateDIBFromBitmap
 *  Allocates a packed DIB and copies the bitmap data into it.
 */
HGLOBAL DIB_CreateDIBFromBitmap(HDC hdc, HBITMAP hBmp)
{
    BITMAPOBJ *pBmp = NULL;
    HGLOBAL hPackedDIB = 0;
    LPBYTE pPackedDIB = NULL;
    LPBITMAPINFOHEADER pbmiHeader = NULL;
    unsigned int width, height, depth, cDataSize = 0, cPackedSize = 0,
                 OffsetBits = 0, nLinesCopied = 0;

    /* Get a pointer to the BITMAPOBJ structure */
    pBmp = (BITMAPOBJ *)GDI_GetObjPtr( hBmp, BITMAP_MAGIC );

    /* Get the bitmap dimensions */
    width = pBmp->bitmap.bmWidth;
    height = pBmp->bitmap.bmHeight;
    depth = pBmp->bitmap.bmBitsPixel;
                 
    /*
     * A packed DIB contains a BITMAPINFO structure followed immediately by
     * an optional color palette and the pixel data.
     */

    /* Calculate the size of the packed DIB */
    cDataSize = DIB_GetDIBImageBytes( width, height, depth );
    cPackedSize = sizeof(BITMAPINFOHEADER)
                  + ( (depth <= 8) ? (sizeof(RGBQUAD) * (1 << depth)) : 0 )
                  + cDataSize;
    /* Get the offset to the bits */
    OffsetBits = cPackedSize - cDataSize;

    /* Allocate the packed DIB */
    TRACE("\tAllocating packed DIB of size %d\n", cPackedSize);
    hPackedDIB = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE /*| GMEM_ZEROINIT*/,
                             cPackedSize );
    if ( !hPackedDIB )
    {
        WARN("Could not allocate packed DIB!\n");
        goto END;
    }
    
    /* A packed DIB starts with a BITMAPINFOHEADER */
    pPackedDIB = (LPBYTE)GlobalLock(hPackedDIB);
    pbmiHeader = (LPBITMAPINFOHEADER)pPackedDIB;

    /* Init the BITMAPINFOHEADER */
    pbmiHeader->biSize = sizeof(BITMAPINFOHEADER);
    pbmiHeader->biWidth = width;
    pbmiHeader->biHeight = height;
    pbmiHeader->biPlanes = 1;
    pbmiHeader->biBitCount = depth;
    pbmiHeader->biCompression = BI_RGB;
    pbmiHeader->biSizeImage = 0;
    pbmiHeader->biXPelsPerMeter = pbmiHeader->biYPelsPerMeter = 0;
    pbmiHeader->biClrUsed = 0;
    pbmiHeader->biClrImportant = 0;

    /* Retrieve the DIB bits from the bitmap and fill in the
     * DIB color table if present */
    
    nLinesCopied = GetDIBits(hdc,                       /* Handle to device context */
                             hBmp,                      /* Handle to bitmap */
                             0,                         /* First scan line to set in dest bitmap */
                             height,                    /* Number of scan lines to copy */
                             pPackedDIB + OffsetBits,   /* [out] Address of array for bitmap bits */
                             (LPBITMAPINFO) pbmiHeader, /* [out] Address of BITMAPINFO structure */
                             0);                        /* RGB or palette index */
    GlobalUnlock(hPackedDIB);

    /* Cleanup if GetDIBits failed */
    if (nLinesCopied != height)
    {
        TRACE("\tGetDIBits returned %d. Actual lines=%d\n", nLinesCopied, height);
        GlobalFree(hPackedDIB);
        hPackedDIB = 0;
    }

END:
    return hPackedDIB;
}
