/*
 * GDI device-independent bitmaps
 *
 * Copyright 1993,1994  Alexandre Julliard
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "bitmap.h"
#include "selectors.h"
#include "gdi.h"
#include "wownt32.h"
#include "wine/debug.h"
#include "palette.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitmap);

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
 * Return 1 for INFOHEADER, 0 for COREHEADER,
 * 4 for V4HEADER, 5 for V5HEADER, -1 for error.
 */
static int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, DWORD *width,
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
    if (header->biSize == sizeof(BITMAPV4HEADER))
    {
        BITMAPV4HEADER *v4hdr = (BITMAPV4HEADER *)header;
        *width  = v4hdr->bV4Width;
        *height = v4hdr->bV4Height;
        *bpp    = v4hdr->bV4BitCount;
        *compr  = v4hdr->bV4V4Compression;
        return 4;
    }
    if (header->biSize == sizeof(BITMAPV5HEADER))
    {
        BITMAPV5HEADER *v5hdr = (BITMAPV5HEADER *)header;
        *width  = v5hdr->bV5Width;
        *height = v5hdr->bV5Height;
        *bpp    = v5hdr->bV5BitCount;
        *compr  = v5hdr->bV5Compression;
        return 5;
    }
    ERR("(%ld): unknown/wrong size for header\n", header->biSize );
    return -1;
}


/***********************************************************************
 *           StretchDIBits   (GDI32.@)
 */
INT WINAPI StretchDIBits(HDC hdc, INT xDst, INT yDst, INT widthDst,
                       INT heightDst, INT xSrc, INT ySrc, INT widthSrc,
                       INT heightSrc, const void *bits,
                       const BITMAPINFO *info, UINT wUsage, DWORD dwRop )
{
    DC *dc;

    if (!bits || !info)
	return 0;

    dc = DC_GetDCUpdate( hdc );
    if(!dc) return FALSE;
    
    if(dc->funcs->pStretchDIBits)
    {
        heightSrc = dc->funcs->pStretchDIBits(dc->physDev, xDst, yDst, widthDst,
                                              heightDst, xSrc, ySrc, widthSrc,
                                              heightSrc, bits, info, wUsage, dwRop);
        GDI_ReleaseObj( hdc );
    }
    else /* use StretchBlt */
    {
        HBITMAP hBitmap, hOldBitmap;
	HDC hdcMem;

        GDI_ReleaseObj( hdc );
	hdcMem = CreateCompatibleDC( hdc );
	if (info->bmiHeader.biCompression == BI_RLE4 ||
	    info->bmiHeader.biCompression == BI_RLE8) {

	   /* when RLE compression is used, there may be some gaps (ie the DIB doesn't
	    * contain all the rectangle described in bmiHeader, but only part of it.
	    * This mean that those undescribed pixels must be left untouched.
	    * So, we first copy on a memory bitmap the current content of the
	    * destination rectangle, blit the DIB bits on top of it - hence leaving
	    * the gaps untouched -, and blitting the rectangle back.
	    * This insure that gaps are untouched on the destination rectangle
	    * Not doing so leads to trashed images (the gaps contain what was on the
	    * memory bitmap => generally black or garbage)
	    * Unfortunately, RLE DIBs without gaps will be slowed down. But this is
	    * another speed vs correctness issue. Anyway, if speed is needed, then the
	    * pStretchDIBits function shall be implemented.
	    * ericP (2000/09/09)
	    */
	   hBitmap = CreateCompatibleBitmap(hdc, info->bmiHeader.biWidth,
					    info->bmiHeader.biHeight);
	   hOldBitmap = SelectObject( hdcMem, hBitmap );

	   /* copy existing bitmap from destination dc */
	   StretchBlt( hdcMem, xSrc, abs(info->bmiHeader.biHeight) - heightSrc - ySrc,
		       widthSrc, heightSrc, hdc, xDst, yDst, widthDst, heightDst,
		       dwRop );
	   SetDIBits(hdcMem, hBitmap, 0, info->bmiHeader.biHeight, bits,
		     info, DIB_RGB_COLORS);

	} else {
	   hBitmap = CreateDIBitmap( hdc, &info->bmiHeader, CBM_INIT,
				     bits, info, wUsage );
	   hOldBitmap = SelectObject( hdcMem, hBitmap );
	}

        /* Origin for DIBitmap may be bottom left (positive biHeight) or top
           left (negative biHeight) */
        StretchBlt( hdc, xDst, yDst, widthDst, heightDst,
		    hdcMem, xSrc, abs(info->bmiHeader.biHeight) - heightSrc - ySrc,
		    widthSrc, heightSrc, dwRop );
	SelectObject( hdcMem, hOldBitmap );
	DeleteDC( hdcMem );
	DeleteObject( hBitmap );
    }
    return heightSrc;
}


/******************************************************************************
 * SetDIBits [GDI32.@]  Sets pixels in a bitmap using colors from DIB
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
    INT result = 0;

    if (!(bitmap = GDI_GetObjPtr( hbitmap, BITMAP_MAGIC ))) return 0;

    if (!(dc = DC_GetDCUpdate( hdc )))
    {
        if (coloruse == DIB_RGB_COLORS) FIXME( "shouldn't require a DC for DIB_RGB_COLORS\n" );
        GDI_ReleaseObj( hbitmap );
        return 0;
    }

    if (!bitmap->funcs && !BITMAP_SetOwnerDC( hbitmap, dc )) goto done;

    if (bitmap->funcs && bitmap->funcs->pSetDIBits)
        result = bitmap->funcs->pSetDIBits( dc->physDev, hbitmap, startscan, lines,
                                            bits, info, coloruse );
    else
        result = lines;

 done:
    GDI_ReleaseObj( hdc );
    GDI_ReleaseObj( hbitmap );
    return result;
}


/***********************************************************************
 *           SetDIBitsToDevice   (GDI32.@)
 */
INT WINAPI SetDIBitsToDevice(HDC hdc, INT xDest, INT yDest, DWORD cx,
                           DWORD cy, INT xSrc, INT ySrc, UINT startscan,
                           UINT lines, LPCVOID bits, const BITMAPINFO *info,
                           UINT coloruse )
{
    INT ret;
    DC *dc;

    if (!(dc = DC_GetDCUpdate( hdc ))) return 0;

    if(dc->funcs->pSetDIBitsToDevice)
        ret = dc->funcs->pSetDIBitsToDevice( dc->physDev, xDest, yDest, cx, cy, xSrc,
					     ySrc, startscan, lines, bits,
					     info, coloruse );
    else {
        FIXME("unimplemented on hdc %p\n", hdc);
	ret = 0;
    }

    GDI_ReleaseObj( hdc );
    return ret;
}

/***********************************************************************
 *           SetDIBColorTable    (GDI32.@)
 */
UINT WINAPI SetDIBColorTable( HDC hdc, UINT startpos, UINT entries, RGBQUAD *colors )
{
    DC * dc;
    UINT result = 0;

    if (!(dc = DC_GetDCUpdate( hdc ))) return 0;

    if (dc->funcs->pSetDIBColorTable)
        result = dc->funcs->pSetDIBColorTable(dc->physDev, startpos, entries, colors);

    GDI_ReleaseObj( hdc );
    return result;
}


/***********************************************************************
 *           GetDIBColorTable    (GDI32.@)
 */
UINT WINAPI GetDIBColorTable( HDC hdc, UINT startpos, UINT entries, RGBQUAD *colors )
{
    DC * dc;
    UINT result = 0;

    if (!(dc = DC_GetDCUpdate( hdc ))) return 0;

    if (dc->funcs->pGetDIBColorTable)
        result = dc->funcs->pGetDIBColorTable(dc->physDev, startpos, entries, colors);

    GDI_ReleaseObj( hdc );
    return result;
}

/* FIXME the following two structs should be combined with __sysPalTemplate in
   objects/color.c - this should happen after de-X11-ing both of these
   files.
   NB. RGBQUAD and PALETTEENTRY have different orderings of red, green
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


/******************************************************************************
 * GetDIBits [GDI32.@]  Retrieves bits of bitmap and copies to buffer
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
    if (!(dc = DC_GetDCUpdate( hdc ))) return 0;
    if (!(bmp = (BITMAPOBJ *)GDI_GetObjPtr( hbitmap, BITMAP_MAGIC )))
    {
        GDI_ReleaseObj( hdc );
	return 0;
    }
    if (!(palette = (PALETTEOBJ*)GDI_GetObjPtr( dc->hPalette, PALETTE_MAGIC )))
    {
        GDI_ReleaseObj( hdc );
        GDI_ReleaseObj( hbitmap );
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

    GDI_ReleaseObj( dc->hPalette );

    if (bits && lines)
    {
        /* If the bitmap object already have a dib section that contains image data, get the bits from it */
        if(bmp->dib && bmp->dib->dsBm.bmBitsPixel >= 15 && info->bmiHeader.biBitCount >= 15)
        {
            /*FIXME: Only RGB dibs supported for now */
            unsigned int srcwidth = bmp->dib->dsBm.bmWidth, srcwidthb = bmp->dib->dsBm.bmWidthBytes;
            int dstwidthb = DIB_GetDIBWidthBytes( info->bmiHeader.biWidth, info->bmiHeader.biBitCount );
            LPBYTE dbits = bits, sbits = (LPBYTE) bmp->dib->dsBm.bmBits + (startscan * srcwidthb);
            unsigned int x, y;

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
                                for( x = 0; x < srcwidth; x++, srcbits += 3)
                                    *dstbits++ = ((srcbits[0] >> 3) & bmask) |
                                                 (((WORD)srcbits[1] << 2) & gmask) |
                                                 (((WORD)srcbits[2] << 7) & rmask);

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
        else
        {
            if (!bmp->funcs && !BITMAP_SetOwnerDC( hbitmap, dc )) lines = 0;
            else
            {
                if (bmp->funcs && bmp->funcs->pGetDIBits)
                    lines = bmp->funcs->pGetDIBits( dc->physDev, hbitmap, startscan,
                                                    lines, bits, info, coloruse );
                else
                    lines = 0;  /* FIXME: should copy from bmp->bitmap.bmBits */
            }
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

    GDI_ReleaseObj( hdc );
    GDI_ReleaseObj( hbitmap );

    return lines;
}


/***********************************************************************
 *           CreateDIBitmap    (GDI32.@)
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
    DC *dc;

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
        else if (data->bmiHeader.biSize == sizeof(BITMAPV4HEADER))
        { /* FIXME: correct ? */
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
        else
        {
            ERR("(%ld): wrong/unknown size for data\n",
                     data->bmiHeader.biSize );
            return 0;
        }
    }

    /* Now create the bitmap */

    if (!(dc = DC_GetDCPtr( hdc ))) return 0;

    if (fColor)
        handle = CreateBitmap( width, height, GetDeviceCaps( hdc, PLANES ),
                               GetDeviceCaps( hdc, BITSPIXEL ), NULL );
    else handle = CreateBitmap( width, height, 1, 1, NULL );

    if (handle)
    {
        if (init == CBM_INIT) SetDIBits( hdc, handle, 0, height, bits, data, coloruse );
        else if (!BITMAP_SetOwnerDC( handle, dc ))
        {
            DeleteObject( handle );
            handle = 0;
        }
    }

    GDI_ReleaseObj( hdc );
    return handle;
}

/***********************************************************************
 *           CreateDIBSection    (GDI.489)
 */
HBITMAP16 WINAPI CreateDIBSection16 (HDC16 hdc, BITMAPINFO *bmi, UINT16 usage,
                                     SEGPTR *bits16, HANDLE section, DWORD offset)
{
    LPVOID bits32;
    HBITMAP hbitmap;

    hbitmap = CreateDIBSection( HDC_32(hdc), bmi, usage, &bits32, section, offset );
    if (hbitmap)
    {
        BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr(hbitmap, BITMAP_MAGIC);
        if (bmp && bmp->dib && bits32)
        {
            BITMAPINFOHEADER *bi = &bmi->bmiHeader;
            INT height = bi->biHeight >= 0 ? bi->biHeight : -bi->biHeight;
            INT width_bytes = DIB_GetDIBWidthBytes(bi->biWidth, bi->biBitCount);
            INT size  = (bi->biSizeImage && bi->biCompression != BI_RGB) ?
                         bi->biSizeImage : width_bytes * height;

            /* calculate number of sel's needed for size with 64K steps */
            WORD count = (size + 0xffff) / 0x10000;
            WORD sel = AllocSelectorArray16(count);
            int i;

            for (i = 0; i < count; i++)
            {
                SetSelectorBase(sel + (i << __AHSHIFT), (DWORD)bits32 + i * 0x10000);
                SetSelectorLimit16(sel + (i << __AHSHIFT), size - 1); /* yep, limit is correct */
                size -= 0x10000;
            }
            bmp->segptr_bits = MAKESEGPTR( sel, 0 );
            if (bits16) *bits16 = bmp->segptr_bits;
        }
        if (bmp) GDI_ReleaseObj( hbitmap );
    }
    return HBITMAP_16(hbitmap);
}

/***********************************************************************
 *           DIB_CreateDIBSection
 */
HBITMAP DIB_CreateDIBSection(HDC hdc, BITMAPINFO *bmi, UINT usage,
			     LPVOID *bits, HANDLE section,
			     DWORD offset, DWORD ovr_pitch)
{
    HBITMAP hbitmap = 0;
    DC *dc;
    BOOL bDesktopDC = FALSE;

    /* If the reference hdc is null, take the desktop dc */
    if (hdc == 0)
    {
        hdc = CreateCompatibleDC(0);
        bDesktopDC = TRUE;
    }

    if ((dc = DC_GetDCPtr( hdc )))
    {
        hbitmap = dc->funcs->pCreateDIBSection(dc->physDev, bmi, usage, bits, section, offset, ovr_pitch);
        GDI_ReleaseObj(hdc);
    }

    if (bDesktopDC)
      DeleteDC(hdc);

    return hbitmap;
}

/***********************************************************************
 *           CreateDIBSection    (GDI32.@)
 */
HBITMAP WINAPI CreateDIBSection(HDC hdc, BITMAPINFO *bmi, UINT usage,
				LPVOID *bits, HANDLE section,
				DWORD offset)
{
    return DIB_CreateDIBSection(hdc, bmi, usage, bits, section, offset, 0);
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
    if (!pBmp) return hPackedDIB;

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
    GDI_ReleaseObj( hBmp );
    return hPackedDIB;
}
