/*
 * X11DRV device-independent bitmaps
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

#include "config.h"

#include <X11/Xlib.h>
#ifdef HAVE_LIBXXSHM
#include <X11/extensions/XShm.h>
# ifdef HAVE_SYS_SHM_H
#  include <sys/shm.h>
# endif
# ifdef HAVE_SYS_IPC_H
#  include <sys/ipc.h>
# endif
#endif /* defined(HAVE_LIBXXSHM) */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "bitmap.h"
#include "x11drv.h"
#include "wine/debug.h"
#include "gdi.h"
#include "palette.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitmap);
WINE_DECLARE_DEBUG_CHANNEL(x11drv);

static int ximageDepthTable[32];

/* This structure holds the arguments for DIB_SetImageBits() */
typedef struct
{
    X11DRV_PDEVICE *physDev;
    LPCVOID         bits;
    XImage         *image;
    PALETTEENTRY   *palentry;
    int             lines;
    DWORD           infoWidth;
    WORD            depth;
    WORD            infoBpp;
    WORD            compression;
    RGBQUAD        *colorMap;
    int             nColorMap;
    Drawable        drawable;
    GC              gc;
    int             xSrc;
    int             ySrc;
    int             xDest;
    int             yDest;
    int             width;
    int             height;
    DWORD           rMask;
    DWORD           gMask;
    DWORD           bMask;
    BOOL            useShm;
    int             dibpitch;
} X11DRV_DIB_IMAGEBITS_DESCR;


enum Rle_EscapeCodes
{
  RLE_EOL   = 0, /* End of line */
  RLE_END   = 1, /* End of bitmap */
  RLE_DELTA = 2  /* Delta */
};

/***********************************************************************
 *           X11DRV_DIB_GetXImageWidthBytes
 *
 * Return the width of an X image in bytes
 */
inline static int X11DRV_DIB_GetXImageWidthBytes( int width, int depth )
{
    if (!depth || depth > 32) goto error;

    if (!ximageDepthTable[depth-1])
    {
        XImage *testimage = XCreateImage( gdi_display, visual, depth,
                                          ZPixmap, 0, NULL, 1, 1, 32, 20 );
        if (testimage)
        {
            ximageDepthTable[depth-1] = testimage->bits_per_pixel;
            XDestroyImage( testimage );
        }
        else ximageDepthTable[depth-1] = -1;
    }
    if (ximageDepthTable[depth-1] != -1)
        return (4 * ((width * ximageDepthTable[depth-1] + 31) / 32));

 error:
    WARN( "(%d): Unsupported depth\n", depth );
    return 4 * width;
}


/***********************************************************************
 *           X11DRV_DIB_CreateXImage
 *
 * Create an X image.
 */
XImage *X11DRV_DIB_CreateXImage( int width, int height, int depth )
{
    int width_bytes;
    XImage *image;

    wine_tsx11_lock();
    width_bytes = X11DRV_DIB_GetXImageWidthBytes( width, depth );
    image = XCreateImage( gdi_display, visual, depth, ZPixmap, 0,
                          calloc( height, width_bytes ),
                          width, height, 32, width_bytes );
    wine_tsx11_unlock();
    return image;
}


/***********************************************************************
 *           DIB_GetBitmapInfo
 *
 * Get the info from a bitmap header.
 * Return 1 for INFOHEADER, 0 for COREHEADER, -1 for error.
 */
static int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, DWORD *width,
                              int *height, WORD *bpp, WORD *compr )
{
    if (header->biSize == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)header;
        *width  = core->bcWidth;
        *height = core->bcHeight;
        *bpp    = core->bcBitCount;
        *compr  = 0;
        return 0;
    }
    if (header->biSize >= sizeof(BITMAPINFOHEADER))
    {
        *width  = header->biWidth;
        *height = header->biHeight;
        *bpp    = header->biBitCount;
        *compr  = header->biCompression;
        return 1;
    }
    ERR("(%ld): unknown/wrong size for header\n", header->biSize );
    return -1;
}


/***********************************************************************
 *           X11DRV_DIB_GenColorMap
 *
 * Fills the color map of a bitmap palette. Should not be called
 * for a >8-bit deep bitmap.
 */
int *X11DRV_DIB_GenColorMap( X11DRV_PDEVICE *physDev, int *colorMapping,
                             WORD coloruse, WORD depth, BOOL quads,
                             const void *colorPtr, int start, int end )
{
    int i;

    if (coloruse == DIB_RGB_COLORS)
    {
	int max = 1 << depth;

	if (end > max) end = max;

        if (quads)
        {
            RGBQUAD * rgb = (RGBQUAD *)colorPtr;

            if (depth == 1)  /* Monochrome */
                for (i = start; i < end; i++, rgb++)
                    colorMapping[i] = (rgb->rgbRed + rgb->rgbGreen +
                                       rgb->rgbBlue > 255*3/2);
            else
                for (i = start; i < end; i++, rgb++)
                    colorMapping[i] = X11DRV_PALETTE_ToPhysical( physDev, RGB(rgb->rgbRed,
                                                                rgb->rgbGreen,
                                                                rgb->rgbBlue));
        }
        else
        {
            RGBTRIPLE * rgb = (RGBTRIPLE *)colorPtr;

            if (depth == 1)  /* Monochrome */
                for (i = start; i < end; i++, rgb++)
                    colorMapping[i] = (rgb->rgbtRed + rgb->rgbtGreen +
                                       rgb->rgbtBlue > 255*3/2);
            else
                for (i = start; i < end; i++, rgb++)
                    colorMapping[i] = X11DRV_PALETTE_ToPhysical( physDev, RGB(rgb->rgbtRed,
                                                               rgb->rgbtGreen,
                                                               rgb->rgbtBlue));
        }
    }
    else  /* DIB_PAL_COLORS */
    {
        if (colorPtr) {
            WORD * index = (WORD *)colorPtr;

            for (i = start; i < end; i++, index++)
                colorMapping[i] = X11DRV_PALETTE_ToPhysical( physDev, PALETTEINDEX(*index) );
        } else {
            for (i = start; i < end; i++)
                colorMapping[i] = X11DRV_PALETTE_ToPhysical( physDev, PALETTEINDEX(i) );
        }
    }

    return colorMapping;
}

/***********************************************************************
 *           X11DRV_DIB_BuildColorMap
 *
 * Build the color map from the bitmap palette. Should not be called
 * for a >8-bit deep bitmap.
 */
int *X11DRV_DIB_BuildColorMap( X11DRV_PDEVICE *physDev, WORD coloruse, WORD depth,
                               const BITMAPINFO *info, int *nColors )
{
    int colors;
    BOOL isInfo;
    const void *colorPtr;
    int *colorMapping;

    if ((isInfo = (info->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))))
    {
        colors = info->bmiHeader.biClrUsed;
        if (!colors) colors = 1 << info->bmiHeader.biBitCount;
        colorPtr = info->bmiColors;
    }
    else  /* assume BITMAPCOREINFO */
    {
        colors = 1 << ((BITMAPCOREHEADER *)&info->bmiHeader)->bcBitCount;
        colorPtr = (WORD *)((BITMAPCOREINFO *)info)->bmciColors;
    }

    if (colors > 256)
    {
        ERR("called with >256 colors!\n");
        return NULL;
    }

    /* just so CopyDIBSection doesn't have to create an identity palette */
    if (coloruse == (WORD)-1) colorPtr = NULL;

    if (!(colorMapping = (int *)HeapAlloc(GetProcessHeap(), 0,
                                          colors * sizeof(int) )))
	return NULL;

    *nColors = colors;
    return X11DRV_DIB_GenColorMap( physDev, colorMapping, coloruse, depth,
                                   isInfo, colorPtr, 0, colors);
}


/***********************************************************************
 *           X11DRV_DIB_MapColor
 */
int X11DRV_DIB_MapColor( int *physMap, int nPhysMap, int phys, int oldcol )
{
    int color;

    if ((oldcol < nPhysMap) && (physMap[oldcol] == phys))
        return oldcol;

    for (color = 0; color < nPhysMap; color++)
        if (physMap[color] == phys)
            return color;

    WARN("Strange color %08x\n", phys);
    return 0;
}


/*********************************************************************
 *         X11DRV_DIB_GetNearestIndex
 *
 * Helper for X11DRV_DIB_GetDIBits.
 * Returns the nearest colour table index for a given RGB.
 * Nearest is defined by minimizing the sum of the squares.
 */
static INT X11DRV_DIB_GetNearestIndex(RGBQUAD *colormap, int numColors, BYTE r, BYTE g, BYTE b)
{
    INT i, best = -1, diff, bestdiff = -1;
    RGBQUAD *color;

    for(color = colormap, i = 0; i < numColors; color++, i++) {
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
/*********************************************************************
 *         X11DRV_DIB_MaskToShift
 *
 * Helper for X11DRV_DIB_GetDIBits.
 * Returns the by how many bits to shift a given color so that it is
 * in the proper position.
 */
INT X11DRV_DIB_MaskToShift(DWORD mask)
{
    int shift;

    if (mask==0)
        return 0;

    shift=0;
    while ((mask&1)==0) {
        mask>>=1;
        shift++;
    }
    return shift;
}

/***********************************************************************
 *           X11DRV_DIB_SetImageBits_1
 *
 * SetDIBits for a 1-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_1( int lines, const BYTE *srcbits,
                                DWORD srcwidth, DWORD dstwidth, int left,
                                int *colors, XImage *bmpImage, DWORD linebytes)
{
    int h;
    const BYTE* srcbyte;
    BYTE srcval, extra;
    DWORD i, x;

    if (lines < 0 ) {
        lines = -lines;
        srcbits = srcbits + linebytes * (lines - 1);
        linebytes = -linebytes;
    }

    if ((extra = (left & 7)) != 0) {
        left &= ~7;
        dstwidth += extra;
    }
    srcbits += left >> 3;

    /* ==== pal 1 dib -> any bmp format ==== */
    for (h = lines-1; h >=0; h--) {
        srcbyte=srcbits;
        /* FIXME: should avoid putting x<left pixels (minor speed issue) */
        for (i = dstwidth/8, x = left; i > 0; i--) {
            srcval=*srcbyte++;
            XPutPixel( bmpImage, x++, h, colors[ srcval >> 7] );
            XPutPixel( bmpImage, x++, h, colors[(srcval >> 6) & 1] );
            XPutPixel( bmpImage, x++, h, colors[(srcval >> 5) & 1] );
            XPutPixel( bmpImage, x++, h, colors[(srcval >> 4) & 1] );
            XPutPixel( bmpImage, x++, h, colors[(srcval >> 3) & 1] );
            XPutPixel( bmpImage, x++, h, colors[(srcval >> 2) & 1] );
            XPutPixel( bmpImage, x++, h, colors[(srcval >> 1) & 1] );
            XPutPixel( bmpImage, x++, h, colors[ srcval       & 1] );
        }
        if (dstwidth % 8){
            srcval=*srcbyte;
            switch (dstwidth & 7)
            {
            case 7: XPutPixel(bmpImage, x++, h, colors[srcval >> 7]); srcval<<=1;
            case 6: XPutPixel(bmpImage, x++, h, colors[srcval >> 7]); srcval<<=1;
            case 5: XPutPixel(bmpImage, x++, h, colors[srcval >> 7]); srcval<<=1;
            case 4: XPutPixel(bmpImage, x++, h, colors[srcval >> 7]); srcval<<=1;
            case 3: XPutPixel(bmpImage, x++, h, colors[srcval >> 7]); srcval<<=1;
            case 2: XPutPixel(bmpImage, x++, h, colors[srcval >> 7]); srcval<<=1;
            case 1: XPutPixel(bmpImage, x++, h, colors[srcval >> 7]);
            }
        }
        srcbits += linebytes;
    }
}

/***********************************************************************
 *           X11DRV_DIB_GetImageBits_1
 *
 * GetDIBits for a 1-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_1( int lines, BYTE *dstbits,
				       DWORD dstwidth, DWORD srcwidth,
				       RGBQUAD *colors, PALETTEENTRY *srccolors,
                                XImage *bmpImage, DWORD linebytes )
{
    DWORD x;
    int h;

    if (lines < 0 ) {
        lines = -lines;
        dstbits = dstbits + linebytes * (lines - 1);
        linebytes = -linebytes;
    }

    switch (bmpImage->depth)
    {
    case 1:
    case 4:
        if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors) {
            /* ==== pal 1 or 4 bmp -> pal 1 dib ==== */
            BYTE* dstbyte;

            for (h=lines-1; h>=0; h--) {
                BYTE dstval;
                dstbyte=dstbits;
                dstval=0;
                for (x=0; x<dstwidth; x++) {
                    PALETTEENTRY srcval;
                    srcval=srccolors[XGetPixel(bmpImage, x, h)];
                    dstval|=(X11DRV_DIB_GetNearestIndex
                             (colors, 2,
                              srcval.peRed,
                              srcval.peGreen,
                              srcval.peBlue) << (7 - (x & 7)));
                    if ((x&7)==7) {
                        *dstbyte++=dstval;
                        dstval=0;
                    }
                }
                if ((dstwidth&7)!=0) {
                    *dstbyte=dstval;
                }
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 8:
        if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors) {
            /* ==== pal 8 bmp -> pal 1 dib ==== */
            const void* srcbits;
            const BYTE* srcpixel;
            BYTE* dstbyte;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            for (h=0; h<lines; h++) {
                BYTE dstval;
                srcpixel=srcbits;
                dstbyte=dstbits;
                dstval=0;
                for (x=0; x<dstwidth; x++) {
                    PALETTEENTRY srcval;
                    srcval=srccolors[(int)*srcpixel++];
                    dstval|=(X11DRV_DIB_GetNearestIndex
                             (colors, 2,
                              srcval.peRed,
                              srcval.peGreen,
                              srcval.peBlue) << (7-(x&7)) );
                    if ((x&7)==7) {
                        *dstbyte++=dstval;
                        dstval=0;
                    }
                }
                if ((dstwidth&7)!=0) {
                    *dstbyte=dstval;
                }
                srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 15:
    case 16:
        {
            const void* srcbits;
            const WORD* srcpixel;
            BYTE* dstbyte;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask==0x03e0) {
                if (bmpImage->red_mask==0x7c00) {
                    /* ==== rgb 555 bmp -> pal 1 dib ==== */
                    for (h=0; h<lines; h++) {
                        BYTE dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<dstwidth; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 2,
                                      ((srcval >>  7) & 0xf8) | /* r */
                                      ((srcval >> 12) & 0x07),
                                      ((srcval >>  2) & 0xf8) | /* g */
                                      ((srcval >>  7) & 0x07),
                                      ((srcval <<  3) & 0xf8) | /* b */
                                      ((srcval >>  2) & 0x07) ) << (7-(x&7)) );
                            if ((x&7)==7) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((dstwidth&7)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else if (bmpImage->blue_mask==0x7c00) {
                    /* ==== bgr 555 bmp -> pal 1 dib ==== */
                    for (h=0; h<lines; h++) {
                        WORD dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<dstwidth; x++) {
                            BYTE srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 2,
                                      ((srcval <<  3) & 0xf8) | /* r */
                                      ((srcval >>  2) & 0x07),
                                      ((srcval >>  2) & 0xf8) | /* g */
                                      ((srcval >>  7) & 0x07),
                                      ((srcval >>  7) & 0xf8) | /* b */
                                      ((srcval >> 12) & 0x07) ) << (7-(x&7)) );
                            if ((x&7)==7) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((dstwidth&7)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else {
                    goto notsupported;
                }
            } else if (bmpImage->green_mask==0x07e0) {
                if (bmpImage->red_mask==0xf800) {
                    /* ==== rgb 565 bmp -> pal 1 dib ==== */
                    for (h=0; h<lines; h++) {
                        BYTE dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<dstwidth; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 2,
                                      ((srcval >>  8) & 0xf8) | /* r */
                                      ((srcval >> 13) & 0x07),
                                      ((srcval >>  3) & 0xfc) | /* g */
                                      ((srcval >>  9) & 0x03),
                                      ((srcval <<  3) & 0xf8) | /* b */
                                      ((srcval >>  2) & 0x07) ) << (7-(x&7)) );
                            if ((x&7)==7) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((dstwidth&7)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else if (bmpImage->blue_mask==0xf800) {
                    /* ==== bgr 565 bmp -> pal 1 dib ==== */
                    for (h=0; h<lines; h++) {
                        BYTE dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<dstwidth; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 2,
                                      ((srcval <<  3) & 0xf8) | /* r */
                                      ((srcval >>  2) & 0x07),
                                      ((srcval >>  3) & 0xfc) | /* g */
                                      ((srcval >>  9) & 0x03),
                                      ((srcval >>  8) & 0xf8) | /* b */
                                      ((srcval >> 13) & 0x07) ) << (7-(x&7)) );
                            if ((x&7)==7) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((dstwidth&7)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else {
                    goto notsupported;
                }
            } else {
                goto notsupported;
            }
        }
        break;

    case 24:
    case 32:
        {
            const void* srcbits;
            const BYTE *srcbyte;
            BYTE* dstbyte;
            int bytes_per_pixel;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;
            bytes_per_pixel=(bmpImage->bits_per_pixel==24?3:4);

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (bmpImage->blue_mask==0xff) {
                /* ==== rgb 888 or 0888 bmp -> pal 1 dib ==== */
                for (h=0; h<lines; h++) {
                    BYTE dstval;
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    dstval=0;
                    for (x=0; x<dstwidth; x++) {
                        dstval|=(X11DRV_DIB_GetNearestIndex
                                 (colors, 2,
                                  srcbyte[2],
                                  srcbyte[1],
                                  srcbyte[0]) << (7-(x&7)) );
                        srcbyte+=bytes_per_pixel;
                        if ((x&7)==7) {
                            *dstbyte++=dstval;
                            dstval=0;
                        }
                    }
                    if ((dstwidth&7)!=0) {
                        *dstbyte=dstval;
                    }
                    srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            } else {
                /* ==== bgr 888 or 0888 bmp -> pal 1 dib ==== */
                for (h=0; h<lines; h++) {
                    BYTE dstval;
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    dstval=0;
                    for (x=0; x<dstwidth; x++) {
                        dstval|=(X11DRV_DIB_GetNearestIndex
                                 (colors, 2,
                                  srcbyte[0],
                                  srcbyte[1],
                                  srcbyte[2]) << (7-(x&7)) );
                        srcbyte+=bytes_per_pixel;
                        if ((x&7)==7) {
                            *dstbyte++=dstval;
                            dstval=0;
                        }
                    }
                    if ((dstwidth&7)!=0) {
                        *dstbyte=dstval;
                    }
                    srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            }
        }
        break;

    default:
    notsupported:
        {
            BYTE* dstbyte;
            unsigned long white = (1 << bmpImage->bits_per_pixel) - 1;

            /* ==== any bmp format -> pal 1 dib ==== */
            WARN("from unknown %d bit bitmap (%lx,%lx,%lx) to 1 bit DIB\n",
                  bmpImage->bits_per_pixel, bmpImage->red_mask,
                  bmpImage->green_mask, bmpImage->blue_mask );

            for (h=lines-1; h>=0; h--) {
                BYTE dstval;
                dstbyte=dstbits;
                dstval=0;
                for (x=0; x<dstwidth; x++) {
                    dstval|=(XGetPixel( bmpImage, x, h) >= white) << (7 - (x&7));
                    if ((x&7)==7) {
                        *dstbyte++=dstval;
                        dstval=0;
                    }
                }
                if ((dstwidth&7)!=0) {
                    *dstbyte=dstval;
                }
                dstbits += linebytes;
            }
        }
        break;
    }
}

/***********************************************************************
 *           X11DRV_DIB_SetImageBits_4
 *
 * SetDIBits for a 4-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_4( int lines, const BYTE *srcbits,
                                DWORD srcwidth, DWORD dstwidth, int left,
                                int *colors, XImage *bmpImage, DWORD linebytes)
{
    int h;
    const BYTE* srcbyte;
    DWORD i, x;

    if (lines < 0 ) {
        lines = -lines;
        srcbits = srcbits + linebytes * (lines - 1);
        linebytes = -linebytes;
    }

    if (left & 1) {
        left--;
        dstwidth++;
    }
    srcbits += left >> 1;

    /* ==== pal 4 dib -> any bmp format ==== */
    for (h = lines-1; h >= 0; h--) {
        srcbyte=srcbits;
        for (i = dstwidth/2, x = left; i > 0; i--) {
            BYTE srcval=*srcbyte++;
            XPutPixel( bmpImage, x++, h, colors[srcval >> 4] );
            XPutPixel( bmpImage, x++, h, colors[srcval & 0x0f] );
        }
        if (dstwidth & 1)
            XPutPixel( bmpImage, x, h, colors[*srcbyte >> 4] );
        srcbits += linebytes;
    }
}



/***********************************************************************
 *           X11DRV_DIB_GetImageBits_4
 *
 * GetDIBits for a 4-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_4( int lines, BYTE *dstbits,
				       DWORD srcwidth, DWORD dstwidth,
				       RGBQUAD *colors, PALETTEENTRY *srccolors,
				       XImage *bmpImage, DWORD linebytes )
{
    DWORD x;
    int h;
    BYTE *bits;

    if (lines < 0 )
    {
       lines = -lines;
       dstbits = dstbits + ( linebytes * (lines-1) );
       linebytes = -linebytes;
    }

    bits = dstbits;

    switch (bmpImage->depth) {
    case 1:
    case 4:
        if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors) {
            /* ==== pal 1 or 4 bmp -> pal 4 dib ==== */
            BYTE* dstbyte;

            for (h = lines-1; h >= 0; h--) {
                BYTE dstval;
                dstbyte=dstbits;
                dstval=0;
                for (x = 0; x < dstwidth; x++) {
                    PALETTEENTRY srcval;
                    srcval=srccolors[XGetPixel(bmpImage, x, h)];
                    dstval|=(X11DRV_DIB_GetNearestIndex
                             (colors, 16,
                              srcval.peRed,
                              srcval.peGreen,
                              srcval.peBlue) << (4-((x&1)<<2)));
                    if ((x&1)==1) {
                        *dstbyte++=dstval;
                        dstval=0;
                    }
                }
                if ((dstwidth&1)!=0) {
                    *dstbyte=dstval;
                }
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 8:
        if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors) {
            /* ==== pal 8 bmp -> pal 4 dib ==== */
            const void* srcbits;
            const BYTE *srcpixel;
            BYTE* dstbyte;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;
            for (h=0; h<lines; h++) {
                BYTE dstval;
                srcpixel=srcbits;
                dstbyte=dstbits;
                dstval=0;
                for (x=0; x<dstwidth; x++) {
                    PALETTEENTRY srcval;
                    srcval = srccolors[(int)*srcpixel++];
                    dstval|=(X11DRV_DIB_GetNearestIndex
                             (colors, 16,
                              srcval.peRed,
                              srcval.peGreen,
                              srcval.peBlue) << (4*(1-(x&1))) );
                    if ((x&1)==1) {
                        *dstbyte++=dstval;
                        dstval=0;
                    }
                }
                if ((dstwidth&1)!=0) {
                    *dstbyte=dstval;
                }
                srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 15:
    case 16:
        {
            const void* srcbits;
            const WORD* srcpixel;
            BYTE* dstbyte;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask==0x03e0) {
                if (bmpImage->red_mask==0x7c00) {
                    /* ==== rgb 555 bmp -> pal 4 dib ==== */
                    for (h=0; h<lines; h++) {
                        BYTE dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<dstwidth; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 16,
                                      ((srcval >>  7) & 0xf8) | /* r */
                                      ((srcval >> 12) & 0x07),
                                      ((srcval >>  2) & 0xf8) | /* g */
                                      ((srcval >>  7) & 0x07),
                                      ((srcval <<  3) & 0xf8) | /* b */
                                      ((srcval >>  2) & 0x07) ) << ((1-(x&1))<<2) );
                            if ((x&1)==1) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((dstwidth&1)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else if (bmpImage->blue_mask==0x7c00) {
                    /* ==== bgr 555 bmp -> pal 4 dib ==== */
                    for (h=0; h<lines; h++) {
                        WORD dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<dstwidth; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 16,
                                      ((srcval <<  3) & 0xf8) | /* r */
                                      ((srcval >>  2) & 0x07),
                                      ((srcval >>  2) & 0xf8) | /* g */
                                      ((srcval >>  7) & 0x07),
                                      ((srcval >>  7) & 0xf8) | /* b */
                                      ((srcval >> 12) & 0x07) ) << ((1-(x&1))<<2) );
                            if ((x&1)==1) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((dstwidth&1)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else {
                    goto notsupported;
                }
            } else if (bmpImage->green_mask==0x07e0) {
                if (bmpImage->red_mask==0xf800) {
                    /* ==== rgb 565 bmp -> pal 4 dib ==== */
                    for (h=0; h<lines; h++) {
                        BYTE dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<dstwidth; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 16,
                                      ((srcval >>  8) & 0xf8) | /* r */
                                      ((srcval >> 13) & 0x07),
                                      ((srcval >>  3) & 0xfc) | /* g */
                                      ((srcval >>  9) & 0x03),
                                      ((srcval <<  3) & 0xf8) | /* b */
                                      ((srcval >>  2) & 0x07) ) << ((1-(x&1))<<2) );
                            if ((x&1)==1) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((dstwidth&1)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else if (bmpImage->blue_mask==0xf800) {
                    /* ==== bgr 565 bmp -> pal 4 dib ==== */
                    for (h=0; h<lines; h++) {
                        WORD dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<dstwidth; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 16,
                                      ((srcval <<  3) & 0xf8) | /* r */
                                      ((srcval >>  2) & 0x07),
                                      ((srcval >>  3) & 0xfc) | /* g */
                                      ((srcval >>  9) & 0x03),
                                      ((srcval >>  8) & 0xf8) | /* b */
                                      ((srcval >> 13) & 0x07) ) << ((1-(x&1))<<2) );
                            if ((x&1)==1) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((dstwidth&1)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else {
                    goto notsupported;
                }
            } else {
                goto notsupported;
            }
        }
        break;

    case 24:
        if (bmpImage->bits_per_pixel==24) {
            const void* srcbits;
            const BYTE *srcbyte;
            BYTE* dstbyte;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (bmpImage->blue_mask==0xff) {
                /* ==== rgb 888 bmp -> pal 4 dib ==== */
                for (h=0; h<lines; h++) {
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    for (x=0; x<dstwidth/2; x++) {
                        /* Do 2 pixels at a time */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[2],
                                     srcbyte[1],
                                     srcbyte[0]) << 4) |
                                    X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[5],
                                     srcbyte[4],
                                     srcbyte[3]);
                        srcbyte+=6;
                    }
                    if (dstwidth&1) {
                        /* And the the odd pixel */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[2],
                                     srcbyte[1],
                                     srcbyte[0]) << 4);
                    }
                    srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            } else {
                /* ==== bgr 888 bmp -> pal 4 dib ==== */
                for (h=0; h<lines; h++) {
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    for (x=0; x<dstwidth/2; x++) {
                        /* Do 2 pixels at a time */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[0],
                                     srcbyte[1],
                                     srcbyte[2]) << 4) |
                                    X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[3],
                                     srcbyte[4],
                                     srcbyte[5]);
                        srcbyte+=6;
                    }
                    if (dstwidth&1) {
                        /* And the the odd pixel */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[0],
                                     srcbyte[1],
                                     srcbyte[2]) << 4);
                    }
                    srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            }
            break;
        }
        /* Fall through */

    case 32:
        {
            const void* srcbits;
            const BYTE *srcbyte;
            BYTE* dstbyte;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (bmpImage->blue_mask==0xff) {
                /* ==== rgb 0888 bmp -> pal 4 dib ==== */
                for (h=0; h<lines; h++) {
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    for (x=0; x<dstwidth/2; x++) {
                        /* Do 2 pixels at a time */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[2],
                                     srcbyte[1],
                                     srcbyte[0]) << 4) |
                                    X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[6],
                                     srcbyte[5],
                                     srcbyte[4]);
                        srcbyte+=8;
                    }
                    if (dstwidth&1) {
                        /* And the the odd pixel */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[2],
                                     srcbyte[1],
                                     srcbyte[0]) << 4);
                    }
                    srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            } else {
                /* ==== bgr 0888 bmp -> pal 4 dib ==== */
                for (h=0; h<lines; h++) {
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    for (x=0; x<dstwidth/2; x++) {
                        /* Do 2 pixels at a time */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[0],
                                     srcbyte[1],
                                     srcbyte[2]) << 4) |
                                    X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[4],
                                     srcbyte[5],
                                     srcbyte[6]);
                        srcbyte+=8;
                    }
                    if (dstwidth&1) {
                        /* And the the odd pixel */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[0],
                                     srcbyte[1],
                                     srcbyte[2]) << 4);
                    }
                    srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            }
        }
        break;

    default:
    notsupported:
        {
            BYTE* dstbyte;

            /* ==== any bmp format -> pal 4 dib ==== */
            WARN("from unknown %d bit bitmap (%lx,%lx,%lx) to 4 bit DIB\n",
                  bmpImage->bits_per_pixel, bmpImage->red_mask,
                  bmpImage->green_mask, bmpImage->blue_mask );
            for (h=lines-1; h>=0; h--) {
                dstbyte=dstbits;
                for (x=0; x<(dstwidth & ~1); x+=2) {
                    *dstbyte++=(X11DRV_DIB_MapColor((int*)colors, 16, XGetPixel(bmpImage, x, h), 0) << 4) |
                        X11DRV_DIB_MapColor((int*)colors, 16, XGetPixel(bmpImage, x+1, h), 0);
                }
                if (dstwidth & 1) {
                    *dstbyte=(X11DRV_DIB_MapColor((int *)colors, 16, XGetPixel(bmpImage, x, h), 0) << 4);
                }
                dstbits += linebytes;
            }
        }
        break;
    }
}

/***********************************************************************
 *           X11DRV_DIB_SetImageBits_RLE4
 *
 * SetDIBits for a 4-bit deep compressed DIB.
 */
static void X11DRV_DIB_SetImageBits_RLE4( int lines, const BYTE *bits,
					  DWORD width, DWORD dstwidth,
					  int left, int *colors,
					  XImage *bmpImage )
{
    int x = 0, y = lines - 1, c, length;
    const BYTE *begin = bits;

    while (y >= 0)
    {
        length = *bits++;
	if (length) {	/* encoded */
	    c = *bits++;
	    while (length--) {
                if (x >= width) break;
                XPutPixel(bmpImage, x++, y, colors[c >> 4]);
                if (!length--) break;
                if (x >= width) break;
                XPutPixel(bmpImage, x++, y, colors[c & 0xf]);
	    }
	} else {
	    length = *bits++;
	    switch (length)
            {
            case RLE_EOL:
                x = 0;
                y--;
                break;

            case RLE_END:
	        return;

            case RLE_DELTA:
                x += *bits++;
                y -= *bits++;
                break;

	    default: /* absolute */
	        while (length--) {
		    c = *bits++;
                    if (x < width) XPutPixel(bmpImage, x++, y, colors[c >> 4]);
                    if (!length--) break;
                    if (x < width) XPutPixel(bmpImage, x++, y, colors[c & 0xf]);
		}
		if ((bits - begin) & 1)
		    bits++;
	    }
	}
    }
}



/***********************************************************************
 *           X11DRV_DIB_SetImageBits_8
 *
 * SetDIBits for an 8-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_8( int lines, const BYTE *srcbits,
				DWORD srcwidth, DWORD dstwidth, int left,
                                const int *colors, XImage *bmpImage,
				DWORD linebytes )
{
    DWORD x;
    int h;
    const BYTE* srcbyte;
    BYTE* dstbits;

    if (lines < 0 )
    {
        lines = -lines;
        srcbits = srcbits + linebytes * (lines-1);
        linebytes = -linebytes;
    }
    srcbits += left;
    srcbyte = srcbits;

    switch (bmpImage->depth) {
    case 15:
    case 16:
#if defined(__i386__) && defined(__GNUC__)
	/* Some X servers might have 32 bit/ 16bit deep pixel */
	if (lines && dstwidth && (bmpImage->bits_per_pixel == 16) &&
            (ImageByteOrder(gdi_display)==LSBFirst) )
	{
	    dstbits=bmpImage->data+left*2+(lines-1)*bmpImage->bytes_per_line;
	    /* FIXME: Does this really handle all these cases correctly? */
	    /* ==== pal 8 dib -> rgb or bgr 555 or 565 bmp ==== */
	    for (h = lines ; h--; ) {
		int _cl1,_cl2; /* temp outputs for asm below */
		/* Borrowed from DirectDraw */
		__asm__ __volatile__(
		"xor %%eax,%%eax\n"
		"cld\n"
		"1:\n"
		"    lodsb\n"
		"    movw (%%edx,%%eax,4),%%ax\n"
		"    stosw\n"
		"      xor %%eax,%%eax\n"
		"    loop 1b\n"
		:"=S" (srcbyte), "=D" (_cl1), "=c" (_cl2)
		:"S" (srcbyte),
		 "D" (dstbits),
		 "c" (dstwidth),
		 "d" (colors)
		:"eax", "cc", "memory"
		);
		srcbyte = (srcbits += linebytes);
		dstbits -= bmpImage->bytes_per_line;
	    }
	    return;
	}
	break;
#endif
    case 24:
    case 32:
#if defined(__i386__) && defined(__GNUC__)
	if (lines && dstwidth && (bmpImage->bits_per_pixel == 32) &&
            (ImageByteOrder(gdi_display)==LSBFirst) )
	{
	    dstbits=bmpImage->data+left*4+(lines-1)*bmpImage->bytes_per_line;
	    /* FIXME: Does this really handle both cases correctly? */
	    /* ==== pal 8 dib -> rgb or bgr 0888 bmp ==== */
	    for (h = lines ; h--; ) {
		int _cl1,_cl2; /* temp outputs for asm below */
		/* Borrowed from DirectDraw */
		__asm__ __volatile__(
		"xor %%eax,%%eax\n"
		"cld\n"
		"1:\n"
		"    lodsb\n"
		"    movl (%%edx,%%eax,4),%%eax\n"
		"    stosl\n"
		"      xor %%eax,%%eax\n"
		"    loop 1b\n"
		:"=S" (srcbyte), "=D" (_cl1), "=c" (_cl2)
		:"S" (srcbyte),
		 "D" (dstbits),
		 "c" (dstwidth),
		 "d" (colors)
		:"eax", "cc", "memory"
		);
		srcbyte = (srcbits += linebytes);
		dstbits -= bmpImage->bytes_per_line;
	    }
	    return;
	}
	break;
#endif
    default:
        break; /* use slow generic case below */
    }

    /* ==== pal 8 dib -> any bmp format ==== */
    for (h=lines-1; h>=0; h--) {
        for (x=left; x<dstwidth+left; x++) {
            XPutPixel(bmpImage, x, h, colors[*srcbyte++]);
        }
        srcbyte = (srcbits += linebytes);
    }
}

/***********************************************************************
 *           X11DRV_DIB_GetImageBits_8
 *
 * GetDIBits for an 8-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_8( int lines, BYTE *dstbits,
				       DWORD srcwidth, DWORD dstwidth,
				       RGBQUAD *colors, PALETTEENTRY *srccolors,
				       XImage *bmpImage, DWORD linebytes )
{
    DWORD x;
    int h;
    BYTE* dstbyte;

    if (lines < 0 )
    {
       lines = -lines;
       dstbits = dstbits + ( linebytes * (lines-1) );
       linebytes = -linebytes;
    }

    /*
     * Hack for now
     * This condition is true when GetImageBits has been called by
     * UpdateDIBSection. For now, GetNearestIndex is too slow to support
     * 256 colormaps, so we'll just use for for GetDIBits calls.
     * (In somes cases, in a updateDIBSection, the returned colors are bad too)
     */
    if (!srccolors) goto updatesection;

    switch (bmpImage->depth) {
    case 1:
    case 4:
        if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors) {

            /* ==== pal 1 bmp -> pal 8 dib ==== */
            /* ==== pal 4 bmp -> pal 8 dib ==== */
            for (h=lines-1; h>=0; h--) {
                dstbyte=dstbits;
                for (x=0; x<dstwidth; x++) {
                    PALETTEENTRY srcval;
                    srcval=srccolors[XGetPixel(bmpImage, x, h)];
                    *dstbyte++=X11DRV_DIB_GetNearestIndex(colors, 256,
                                                          srcval.peRed,
                                                          srcval.peGreen,
                                                          srcval.peBlue);
                }
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 8:
       if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors) {
            /* ==== pal 8 bmp -> pal 8 dib ==== */
           const void* srcbits;
           const BYTE* srcpixel;

           srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;
           for (h=0; h<lines; h++) {
               srcpixel=srcbits;
               dstbyte=dstbits;
               for (x = 0; x < dstwidth; x++) {
                   PALETTEENTRY srcval;
                   srcval=srccolors[(int)*srcpixel++];
                   *dstbyte++=X11DRV_DIB_GetNearestIndex(colors, 256,
                                                         srcval.peRed,
                                                         srcval.peGreen,
                                                         srcval.peBlue);
               }
               srcbits = (char*)srcbits - bmpImage->bytes_per_line;
               dstbits += linebytes;
           }
       } else {
           goto notsupported;
       }
       break;

    case 15:
    case 16:
        {
            const void* srcbits;
            const WORD* srcpixel;
            BYTE* dstbyte;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask==0x03e0) {
                if (bmpImage->red_mask==0x7c00) {
                    /* ==== rgb 555 bmp -> pal 8 dib ==== */
                    for (h=0; h<lines; h++) {
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        for (x=0; x<dstwidth; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            *dstbyte++=X11DRV_DIB_GetNearestIndex
                                (colors, 256,
                                 ((srcval >>  7) & 0xf8) | /* r */
                                 ((srcval >> 12) & 0x07),
                                 ((srcval >>  2) & 0xf8) | /* g */
                                 ((srcval >>  7) & 0x07),
                                 ((srcval <<  3) & 0xf8) | /* b */
                                 ((srcval >>  2) & 0x07) );
                        }
                        srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else if (bmpImage->blue_mask==0x7c00) {
                    /* ==== bgr 555 bmp -> pal 8 dib ==== */
                    for (h=0; h<lines; h++) {
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        for (x=0; x<dstwidth; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            *dstbyte++=X11DRV_DIB_GetNearestIndex
                                (colors, 256,
                                 ((srcval <<  3) & 0xf8) | /* r */
                                 ((srcval >>  2) & 0x07),
                                 ((srcval >>  2) & 0xf8) | /* g */
                                 ((srcval >>  7) & 0x07),
                                 ((srcval >>  7) & 0xf8) | /* b */
                                 ((srcval >> 12) & 0x07) );
                        }
                        srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else {
                    goto notsupported;
                }
            } else if (bmpImage->green_mask==0x07e0) {
                if (bmpImage->red_mask==0xf800) {
                    /* ==== rgb 565 bmp -> pal 8 dib ==== */
                    for (h=0; h<lines; h++) {
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        for (x=0; x<dstwidth; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            *dstbyte++=X11DRV_DIB_GetNearestIndex
                                (colors, 256,
                                 ((srcval >>  8) & 0xf8) | /* r */
                                 ((srcval >> 13) & 0x07),
                                 ((srcval >>  3) & 0xfc) | /* g */
                                 ((srcval >>  9) & 0x03),
                                 ((srcval <<  3) & 0xf8) | /* b */
                                 ((srcval >>  2) & 0x07) );
                        }
                        srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else if (bmpImage->blue_mask==0xf800) {
                    /* ==== bgr 565 bmp -> pal 8 dib ==== */
                    for (h=0; h<lines; h++) {
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        for (x=0; x<dstwidth; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            *dstbyte++=X11DRV_DIB_GetNearestIndex
                                (colors, 256,
                                 ((srcval <<  3) & 0xf8) | /* r */
                                 ((srcval >>  2) & 0x07),
                                 ((srcval >>  3) & 0xfc) | /* g */
                                 ((srcval >>  9) & 0x03),
                                 ((srcval >>  8) & 0xf8) | /* b */
                                 ((srcval >> 13) & 0x07) );
                        }
                        srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else {
                    goto notsupported;
                }
            } else {
                goto notsupported;
            }
        }
        break;

    case 24:
    case 32:
        {
            const void* srcbits;
            const BYTE *srcbyte;
            BYTE* dstbyte;
            int bytes_per_pixel;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;
            bytes_per_pixel=(bmpImage->bits_per_pixel==24?3:4);

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (bmpImage->blue_mask==0xff) {
                /* ==== rgb 888 or 0888 bmp -> pal 8 dib ==== */
                for (h=0; h<lines; h++) {
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    for (x=0; x<dstwidth; x++) {
                        *dstbyte++=X11DRV_DIB_GetNearestIndex
                            (colors, 256,
                             srcbyte[2],
                             srcbyte[1],
                             srcbyte[0]);
                        srcbyte+=bytes_per_pixel;
                    }
                    srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            } else {
                /* ==== bgr 888 or 0888 bmp -> pal 8 dib ==== */
                for (h=0; h<lines; h++) {
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    for (x=0; x<dstwidth; x++) {
                        *dstbyte++=X11DRV_DIB_GetNearestIndex
                            (colors, 256,
                             srcbyte[0],
                             srcbyte[1],
                             srcbyte[2]);
                        srcbyte+=bytes_per_pixel;
                    }
                    srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            }
        }
        break;

    default:
    notsupported:
        WARN("from unknown %d bit bitmap (%lx,%lx,%lx) to 8 bit DIB\n",
              bmpImage->depth, bmpImage->red_mask,
              bmpImage->green_mask, bmpImage->blue_mask );
    updatesection:
        /* ==== any bmp format -> pal 8 dib ==== */
        for (h=lines-1; h>=0; h--) {
            dstbyte=dstbits;
            for (x=0; x<dstwidth; x++) {
                *dstbyte=X11DRV_DIB_MapColor
                    ((int*)colors, 256,
                     XGetPixel(bmpImage, x, h), *dstbyte);
                dstbyte++;
            }
            dstbits += linebytes;
        }
        break;
    }
}

/***********************************************************************
 *	      X11DRV_DIB_SetImageBits_RLE8
 *
 * SetDIBits for an 8-bit deep compressed DIB.
 *
 * This function rewritten 941113 by James Youngman.  WINE blew out when I
 * first ran it because my desktop wallpaper is a (large) RLE8 bitmap.
 *
 * This was because the algorithm assumed that all RLE8 bitmaps end with the
 * 'End of bitmap' escape code.  This code is very much laxer in what it
 * allows to end the expansion.  Possibly too lax.  See the note by
 * case RleDelta.  BTW, MS's documentation implies that a correct RLE8
 * bitmap should end with RleEnd, but on the other hand, software exists
 * that produces ones that don't and Windows 3.1 doesn't complain a bit
 * about it.
 *
 * (No) apologies for my English spelling.  [Emacs users: c-indent-level=4].
 *			James A. Youngman <mbcstjy@afs.man.ac.uk>
 *						[JAY]
 */
static void X11DRV_DIB_SetImageBits_RLE8( int lines, const BYTE *bits,
					  DWORD width, DWORD dstwidth,
					  int left, int *colors,
					  XImage *bmpImage )
{
    int x;			/* X-positon on each line.  Increases. */
    int y;			/* Line #.  Starts at lines-1, decreases */
    const BYTE *pIn = bits;     /* Pointer to current position in bits */
    BYTE length;		/* The length pf a run */
    BYTE escape_code;		/* See enum Rle8_EscapeCodes.*/

    /*
     * Note that the bitmap data is stored by Windows starting at the
     * bottom line of the bitmap and going upwards.  Within each line,
     * the data is stored left-to-right.  That's the reason why line
     * goes from lines-1 to 0.			[JAY]
     */

    x = 0;
    y = lines - 1;
    while (y >= 0)
    {
        length = *pIn++;

        /*
         * If the length byte is not zero (which is the escape value),
         * We have a run of length pixels all the same colour.  The colour
         * index is stored next.
         *
         * If the length byte is zero, we need to read the next byte to
         * know what to do.			[JAY]
         */
        if (length != 0)
        {
            /*
             * [Run-Length] Encoded mode
             */
            int color = colors[*pIn++];
            while (length-- && x < dstwidth) XPutPixel(bmpImage, x++, y, color);
        }
        else
        {
            /*
             * Escape codes (may be an absolute sequence though)
             */
            escape_code = (*pIn++);
            switch(escape_code)
            {
            case RLE_EOL:
                x = 0;
                y--;
                break;

            case RLE_END:
                /* Not all RLE8 bitmaps end with this code.  For
                 * example, Paint Shop Pro produces some that don't.
                 * That's (I think) what caused the previous
                 * implementation to fail.  [JAY]
                 */
                return;

            case RLE_DELTA:
                x += (*pIn++);
                y -= (*pIn++);
                break;

            default:  /* switch to absolute mode */
                length = escape_code;
                while (length--)
                {
                    int color = colors[*pIn++];
                    if (x >= dstwidth)
                    {
                        pIn += length;
                        break;
                    }
                    XPutPixel(bmpImage, x++, y, color);
                }
                /*
                 * If you think for a moment you'll realise that the
                 * only time we could ever possibly read an odd
                 * number of bytes is when there is a 0x00 (escape),
                 * a value >0x02 (absolute mode) and then an odd-
                 * length run.  Therefore this is the only place we
                 * need to worry about it.  Everywhere else the
                 * bytes are always read in pairs.  [JAY]
                 */
                if (escape_code & 1) pIn++; /* Throw away the pad byte. */
                break;
            } /* switch (escape_code) : Escape sequence */
        }
    }
}


/***********************************************************************
 *           X11DRV_DIB_SetImageBits_16
 *
 * SetDIBits for a 16-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_16( int lines, const BYTE *srcbits,
                                 DWORD srcwidth, DWORD dstwidth, int left,
                                       X11DRV_PDEVICE *physDev, DWORD rSrc, DWORD gSrc, DWORD bSrc,
                                       XImage *bmpImage, DWORD linebytes )
{
    DWORD x;
    int h;
    const dib_conversions *convs = (bmpImage->byte_order == LSBFirst) ? &dib_normal : &dib_dst_byteswap;

    if (lines < 0 )
    {
        lines = -lines;
        srcbits = srcbits + ( linebytes * (lines-1));
        linebytes = -linebytes;
    }

    switch (bmpImage->depth)
    {
    case 15:
    case 16:
        {
            char* dstbits;

            srcbits=srcbits+left*2;
            dstbits=bmpImage->data+left*2+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask==0x03e0) {
                if (gSrc==bmpImage->green_mask) {
                    if (rSrc==bmpImage->red_mask) {
                        /* ==== rgb 555 dib -> rgb 555 bmp ==== */
                        /* ==== bgr 555 dib -> bgr 555 bmp ==== */
                        convs->Convert_5x5_asis
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else if (rSrc==bmpImage->blue_mask) {
                        /* ==== rgb 555 dib -> bgr 555 bmp ==== */
                        /* ==== bgr 555 dib -> rgb 555 bmp ==== */
                        convs->Convert_555_reverse
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    }
                } else {
                    if (rSrc==bmpImage->red_mask || bSrc==bmpImage->blue_mask) {
                        /* ==== rgb 565 dib -> rgb 555 bmp ==== */
                        /* ==== bgr 565 dib -> bgr 555 bmp ==== */
                        convs->Convert_565_to_555_asis
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else {
                        /* ==== rgb 565 dib -> bgr 555 bmp ==== */
                        /* ==== bgr 565 dib -> rgb 555 bmp ==== */
                        convs->Convert_565_to_555_reverse
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    }
                }
            } else if (bmpImage->green_mask==0x07e0) {
                if (gSrc==bmpImage->green_mask) {
                    if (rSrc==bmpImage->red_mask) {
                        /* ==== rgb 565 dib -> rgb 565 bmp ==== */
                        /* ==== bgr 565 dib -> bgr 565 bmp ==== */
                        convs->Convert_5x5_asis
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else {
                        /* ==== rgb 565 dib -> bgr 565 bmp ==== */
                        /* ==== bgr 565 dib -> rgb 565 bmp ==== */
                        convs->Convert_565_reverse
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    }
                } else {
                    if (rSrc==bmpImage->red_mask || bSrc==bmpImage->blue_mask) {
                        /* ==== rgb 555 dib -> rgb 565 bmp ==== */
                        /* ==== bgr 555 dib -> bgr 565 bmp ==== */
                        convs->Convert_555_to_565_asis
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else {
                        /* ==== rgb 555 dib -> bgr 565 bmp ==== */
                        /* ==== bgr 555 dib -> rgb 565 bmp ==== */
                        convs->Convert_555_to_565_reverse
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    }
                }
            } else {
                goto notsupported;
            }
        }
        break;

    case 24:
        if (bmpImage->bits_per_pixel==24) {
            char* dstbits;

            srcbits=srcbits+left*2;
            dstbits=bmpImage->data+left*3+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if ((rSrc==0x1f && bmpImage->red_mask==0xff) ||
                       (bSrc==0x1f && bmpImage->blue_mask==0xff)) {
                if (gSrc==0x03e0) {
                    /* ==== rgb 555 dib -> rgb 888 bmp ==== */
                    /* ==== bgr 555 dib -> bgr 888 bmp ==== */
                    convs->Convert_555_to_888_asis
                        (dstwidth,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else {
                    /* ==== rgb 565 dib -> rgb 888 bmp ==== */
                    /* ==== bgr 565 dib -> bgr 888 bmp ==== */
                    convs->Convert_565_to_888_asis
                        (dstwidth,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                }
            } else {
                if (gSrc==0x03e0) {
                    /* ==== rgb 555 dib -> bgr 888 bmp ==== */
                    /* ==== bgr 555 dib -> rgb 888 bmp ==== */
                    convs->Convert_555_to_888_reverse
                        (dstwidth,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else {
                    /* ==== rgb 565 dib -> bgr 888 bmp ==== */
                    /* ==== bgr 565 dib -> rgb 888 bmp ==== */
                    convs->Convert_565_to_888_reverse
                        (dstwidth,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                }
            }
            break;
        }
        /* Fall through */

    case 32:
        {
            char* dstbits;

            srcbits=srcbits+left*2;
            dstbits=bmpImage->data+left*4+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if ((rSrc==0x1f && bmpImage->red_mask==0xff) ||
                       (bSrc==0x1f && bmpImage->blue_mask==0xff)) {
                if (gSrc==0x03e0) {
                    /* ==== rgb 555 dib -> rgb 0888 bmp ==== */
                    /* ==== bgr 555 dib -> bgr 0888 bmp ==== */
                    convs->Convert_555_to_0888_asis
                        (dstwidth,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else {
                    /* ==== rgb 565 dib -> rgb 0888 bmp ==== */
                    /* ==== bgr 565 dib -> bgr 0888 bmp ==== */
                    convs->Convert_565_to_0888_asis
                        (dstwidth,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                }
            } else {
                if (gSrc==0x03e0) {
                    /* ==== rgb 555 dib -> bgr 0888 bmp ==== */
                    /* ==== bgr 555 dib -> rgb 0888 bmp ==== */
                    convs->Convert_555_to_0888_reverse
                        (dstwidth,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else {
                    /* ==== rgb 565 dib -> bgr 0888 bmp ==== */
                    /* ==== bgr 565 dib -> rgb 0888 bmp ==== */
                    convs->Convert_565_to_0888_reverse
                        (dstwidth,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                }
            }
        }
        break;

    default:
    notsupported:
        WARN("from 16 bit DIB (%lx,%lx,%lx) to unknown %d bit bitmap (%lx,%lx,%lx)\n",
              rSrc, gSrc, bSrc, bmpImage->bits_per_pixel, bmpImage->red_mask,
              bmpImage->green_mask, bmpImage->blue_mask );
        /* fall through */
    case 1:
    case 4:
    case 8:
        {
            /* ==== rgb or bgr 555 or 565 dib -> pal 1, 4 or 8 ==== */
            const WORD* srcpixel;
            int rShift1,gShift1,bShift1;
            int rShift2,gShift2,bShift2;
            BYTE gMask1,gMask2;

            /* Set color scaling values */
            rShift1=16+X11DRV_DIB_MaskToShift(rSrc)-3;
            gShift1=16+X11DRV_DIB_MaskToShift(gSrc)-3;
            bShift1=16+X11DRV_DIB_MaskToShift(bSrc)-3;
            rShift2=rShift1+5;
            gShift2=gShift1+5;
            bShift2=bShift1+5;
            if (gSrc==0x03e0) {
                /* Green has 5 bits, like the others */
                gMask1=0xf8;
                gMask2=0x07;
            } else {
                /* Green has 6 bits, not 5. Compensate. */
                gShift1++;
                gShift2+=2;
                gMask1=0xfc;
                gMask2=0x03;
            }

            srcbits+=2*left;

            /* We could split it into four separate cases to optimize
             * but it is probably not worth it.
             */
            for (h=lines-1; h>=0; h--) {
                srcpixel=(const WORD*)srcbits;
                for (x=left; x<dstwidth+left; x++) {
                    DWORD srcval;
                    BYTE red,green,blue;
                    srcval=*srcpixel++ << 16;
                    red=  ((srcval >> rShift1) & 0xf8) |
                        ((srcval >> rShift2) & 0x07);
                    green=((srcval >> gShift1) & gMask1) |
                        ((srcval >> gShift2) & gMask2);
                    blue= ((srcval >> bShift1) & 0xf8) |
                        ((srcval >> bShift2) & 0x07);
                    XPutPixel(bmpImage, x, h,
                              X11DRV_PALETTE_ToPhysical
                              (physDev, RGB(red,green,blue)));
                }
                srcbits += linebytes;
            }
        }
        break;
    }
}


/***********************************************************************
 *           X11DRV_DIB_GetImageBits_16
 *
 * GetDIBits for an 16-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_16( int lines, BYTE *dstbits,
					DWORD dstwidth, DWORD srcwidth,
					PALETTEENTRY *srccolors,
					DWORD rDst, DWORD gDst, DWORD bDst,
					XImage *bmpImage, DWORD dibpitch )
{
    DWORD x;
    int h;
    const dib_conversions *convs = (bmpImage->byte_order == LSBFirst) ? &dib_normal : &dib_src_byteswap;

    DWORD linebytes = dibpitch;

    if (lines < 0 )
    {
        lines = -lines;
        dstbits = dstbits + ( linebytes * (lines-1));
        linebytes = -linebytes;
    }

    switch (bmpImage->depth)
    {
    case 15:
    case 16:
        {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask==0x03e0) {
                if (gDst==bmpImage->green_mask) {
                    if (rDst==bmpImage->red_mask) {
                        /* ==== rgb 555 bmp -> rgb 555 dib ==== */
                        /* ==== bgr 555 bmp -> bgr 555 dib ==== */
                        convs->Convert_5x5_asis
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        /* ==== rgb 555 bmp -> bgr 555 dib ==== */
                        /* ==== bgr 555 bmp -> rgb 555 dib ==== */
                        convs->Convert_555_reverse
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    }
                } else {
                    if (rDst==bmpImage->red_mask || bDst==bmpImage->blue_mask) {
                        /* ==== rgb 555 bmp -> rgb 565 dib ==== */
                        /* ==== bgr 555 bmp -> bgr 565 dib ==== */
                        convs->Convert_555_to_565_asis
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        /* ==== rgb 555 bmp -> bgr 565 dib ==== */
                        /* ==== bgr 555 bmp -> rgb 565 dib ==== */
                        convs->Convert_555_to_565_reverse
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    }
                }
            } else if (bmpImage->green_mask==0x07e0) {
                if (gDst==bmpImage->green_mask) {
                    if (rDst == bmpImage->red_mask) {
                        /* ==== rgb 565 bmp -> rgb 565 dib ==== */
                        /* ==== bgr 565 bmp -> bgr 565 dib ==== */
                        convs->Convert_5x5_asis
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        /* ==== rgb 565 bmp -> bgr 565 dib ==== */
                        /* ==== bgr 565 bmp -> rgb 565 dib ==== */
                        convs->Convert_565_reverse
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    }
                } else {
                    if (rDst==bmpImage->red_mask || bDst==bmpImage->blue_mask) {
                        /* ==== rgb 565 bmp -> rgb 555 dib ==== */
                        /* ==== bgr 565 bmp -> bgr 555 dib ==== */
                        convs->Convert_565_to_555_asis
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        /* ==== rgb 565 bmp -> bgr 555 dib ==== */
                        /* ==== bgr 565 bmp -> rgb 555 dib ==== */
                        convs->Convert_565_to_555_reverse
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    }
                }
            } else {
                goto notsupported;
            }
        }
        break;

    case 24:
        if (bmpImage->bits_per_pixel == 24) {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if ((rDst==0x1f && bmpImage->red_mask==0xff) ||
                    (bDst==0x1f && bmpImage->blue_mask==0xff)) {
                if (gDst==0x03e0) {
                    /* ==== rgb 888 bmp -> rgb 555 dib ==== */
                    /* ==== bgr 888 bmp -> bgr 555 dib ==== */
                    convs->Convert_888_to_555_asis
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else {
                    /* ==== rgb 888 bmp -> rgb 565 dib ==== */
                    /* ==== rgb 888 bmp -> rgb 565 dib ==== */
                    convs->Convert_888_to_565_asis
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                }
            } else {
                if (gDst==0x03e0) {
                    /* ==== rgb 888 bmp -> bgr 555 dib ==== */
                    /* ==== bgr 888 bmp -> rgb 555 dib ==== */
                    convs->Convert_888_to_555_reverse
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else {
                    /* ==== rgb 888 bmp -> bgr 565 dib ==== */
                    /* ==== bgr 888 bmp -> rgb 565 dib ==== */
                    convs->Convert_888_to_565_reverse
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                }
            }
            break;
        }
        /* Fall through */

    case 32:
        {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if ((rDst==0x1f && bmpImage->red_mask==0xff) ||
                       (bDst==0x1f && bmpImage->blue_mask==0xff)) {
                if (gDst==0x03e0) {
                    /* ==== rgb 0888 bmp -> rgb 555 dib ==== */
                    /* ==== bgr 0888 bmp -> bgr 555 dib ==== */
                    convs->Convert_0888_to_555_asis
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else {
                    /* ==== rgb 0888 bmp -> rgb 565 dib ==== */
                    /* ==== bgr 0888 bmp -> bgr 565 dib ==== */
                    convs->Convert_0888_to_565_asis
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                }
            } else {
                if (gDst==0x03e0) {
                    /* ==== rgb 0888 bmp -> bgr 555 dib ==== */
                    /* ==== bgr 0888 bmp -> rgb 555 dib ==== */
                    convs->Convert_0888_to_555_reverse
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else {
                    /* ==== rgb 0888 bmp -> bgr 565 dib ==== */
                    /* ==== bgr 0888 bmp -> rgb 565 dib ==== */
                    convs->Convert_0888_to_565_reverse
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                }
            }
        }
        break;

    case 1:
    case 4:
        if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors) {
            /* ==== pal 1 or 4 bmp -> rgb or bgr 555 or 565 dib ==== */
            int rShift,gShift,bShift;
            WORD* dstpixel;

            /* Shift everything 16 bits left so that all shifts are >0,
             * even for BGR DIBs. Then a single >> 16 will bring everything
             * back into place.
             */
            rShift=16+X11DRV_DIB_MaskToShift(rDst)-3;
            gShift=16+X11DRV_DIB_MaskToShift(gDst)-3;
            bShift=16+X11DRV_DIB_MaskToShift(bDst)-3;
            if (gDst==0x07e0) {
                /* 6 bits for the green */
                gShift++;
            }
            rDst=rDst << 16;
            gDst=gDst << 16;
            bDst=bDst << 16;
            for (h = lines - 1; h >= 0; h--) {
                dstpixel=(LPWORD)dstbits;
                for (x = 0; x < dstwidth; x++) {
                    PALETTEENTRY srcval;
                    DWORD dstval;
                    srcval=srccolors[XGetPixel(bmpImage, x, h)];
                    dstval=((srcval.peRed   << rShift) & rDst) |
                           ((srcval.peGreen << gShift) & gDst) |
                           ((srcval.peBlue  << bShift) & bDst);
                    *dstpixel++=dstval >> 16;
                }
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 8:
        if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors) {
            /* ==== pal 8 bmp -> rgb or bgr 555 or 565 dib ==== */
            int rShift,gShift,bShift;
            const BYTE* srcbits;
            const BYTE* srcpixel;
            WORD* dstpixel;

            /* Shift everything 16 bits left so that all shifts are >0,
             * even for BGR DIBs. Then a single >> 16 will bring everything
             * back into place.
             */
            rShift=16+X11DRV_DIB_MaskToShift(rDst)-3;
            gShift=16+X11DRV_DIB_MaskToShift(gDst)-3;
            bShift=16+X11DRV_DIB_MaskToShift(bDst)-3;
            if (gDst==0x07e0) {
                /* 6 bits for the green */
                gShift++;
            }
            rDst=rDst << 16;
            gDst=gDst << 16;
            bDst=bDst << 16;
            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;
            for (h=0; h<lines; h++) {
                srcpixel=srcbits;
                dstpixel=(LPWORD)dstbits;
                for (x = 0; x < dstwidth; x++) {
                    PALETTEENTRY srcval;
                    DWORD dstval;
                    srcval=srccolors[(int)*srcpixel++];
                    dstval=((srcval.peRed   << rShift) & rDst) |
                           ((srcval.peGreen << gShift) & gDst) |
                           ((srcval.peBlue  << bShift) & bDst);
                    *dstpixel++=dstval >> 16;
                }
                srcbits -= bmpImage->bytes_per_line;
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    default:
    notsupported:
        {
            /* ==== any bmp format -> rgb or bgr 555 or 565 dib ==== */
            int rShift,gShift,bShift;
            WORD* dstpixel;

            WARN("from unknown %d bit bitmap (%lx,%lx,%lx) to 16 bit DIB (%lx,%lx,%lx)\n",
                  bmpImage->depth, bmpImage->red_mask,
                  bmpImage->green_mask, bmpImage->blue_mask,
                  rDst, gDst, bDst);

            /* Shift everything 16 bits left so that all shifts are >0,
             * even for BGR DIBs. Then a single >> 16 will bring everything
             * back into place.
             */
            rShift=16+X11DRV_DIB_MaskToShift(rDst)-3;
            gShift=16+X11DRV_DIB_MaskToShift(gDst)-3;
            bShift=16+X11DRV_DIB_MaskToShift(bDst)-3;
            if (gDst==0x07e0) {
                /* 6 bits for the green */
                gShift++;
            }
            rDst=rDst << 16;
            gDst=gDst << 16;
            bDst=bDst << 16;
            for (h = lines - 1; h >= 0; h--) {
                dstpixel=(LPWORD)dstbits;
                for (x = 0; x < dstwidth; x++) {
                    COLORREF srcval;
                    DWORD dstval;
                    srcval=X11DRV_PALETTE_ToLogical(XGetPixel(bmpImage, x, h));
                    dstval=((GetRValue(srcval) << rShift) & rDst) |
                           ((GetGValue(srcval) << gShift) & gDst) |
                           ((GetBValue(srcval) << bShift) & bDst);
                    *dstpixel++=dstval >> 16;
                }
                dstbits += linebytes;
            }
        }
        break;
    }
}


/***********************************************************************
 *           X11DRV_DIB_SetImageBits_24
 *
 * SetDIBits for a 24-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_24( int lines, const BYTE *srcbits,
                                 DWORD srcwidth, DWORD dstwidth, int left,
                                 X11DRV_PDEVICE *physDev,
                                 DWORD rSrc, DWORD gSrc, DWORD bSrc,
                                 XImage *bmpImage, DWORD linebytes )
{
    DWORD x;
    int h;
    const dib_conversions *convs = (bmpImage->byte_order == LSBFirst) ? &dib_normal : &dib_dst_byteswap;

    if (lines < 0 )
    {
        lines = -lines;
        srcbits = srcbits + linebytes * (lines - 1);
        linebytes = -linebytes;
    }

    switch (bmpImage->depth)
    {
    case 24:
        if (bmpImage->bits_per_pixel==24) {
            char* dstbits;

            srcbits=srcbits+left*3;
            dstbits=bmpImage->data+left*3+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (rSrc==bmpImage->red_mask) {
                /* ==== rgb 888 dib -> rgb 888 bmp ==== */
                /* ==== bgr 888 dib -> bgr 888 bmp ==== */
                convs->Convert_888_asis
                    (dstwidth,lines,
                     srcbits,linebytes,
                     dstbits,-bmpImage->bytes_per_line);
            } else {
                /* ==== rgb 888 dib -> bgr 888 bmp ==== */
                /* ==== bgr 888 dib -> rgb 888 bmp ==== */
                convs->Convert_888_reverse
                    (dstwidth,lines,
                     srcbits,linebytes,
                     dstbits,-bmpImage->bytes_per_line);
            }
            break;
        }
        /* fall through */

    case 32:
        {
            char* dstbits;

            srcbits=srcbits+left*3;
            dstbits=bmpImage->data+left*4+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (rSrc==bmpImage->red_mask) {
                /* ==== rgb 888 dib -> rgb 0888 bmp ==== */
                /* ==== bgr 888 dib -> bgr 0888 bmp ==== */
                convs->Convert_888_to_0888_asis
                    (dstwidth,lines,
                     srcbits,linebytes,
                     dstbits,-bmpImage->bytes_per_line);
            } else {
                /* ==== rgb 888 dib -> bgr 0888 bmp ==== */
                /* ==== bgr 888 dib -> rgb 0888 bmp ==== */
                convs->Convert_888_to_0888_reverse
                    (dstwidth,lines,
                     srcbits,linebytes,
                     dstbits,-bmpImage->bytes_per_line);
            }
            break;
        }

    case 15:
    case 16:
        {
            char* dstbits;

            srcbits=srcbits+left*3;
            dstbits=bmpImage->data+left*2+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask==0x03e0) {
                if ((rSrc==0xff0000 && bmpImage->red_mask==0x7f00) ||
                    (bSrc==0xff0000 && bmpImage->blue_mask==0x7f00)) {
                    /* ==== rgb 888 dib -> rgb 555 bmp ==== */
                    /* ==== bgr 888 dib -> bgr 555 bmp ==== */
                    convs->Convert_888_to_555_asis
                        (dstwidth,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else if ((rSrc==0xff && bmpImage->red_mask==0x7f00) ||
                           (bSrc==0xff && bmpImage->blue_mask==0x7f00)) {
                    /* ==== rgb 888 dib -> bgr 555 bmp ==== */
                    /* ==== bgr 888 dib -> rgb 555 bmp ==== */
                    convs->Convert_888_to_555_reverse
                        (dstwidth,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else {
                    goto notsupported;
                }
            } else if (bmpImage->green_mask==0x07e0) {
                if ((rSrc==0xff0000 && bmpImage->red_mask==0xf800) ||
                    (bSrc==0xff0000 && bmpImage->blue_mask==0xf800)) {
                    /* ==== rgb 888 dib -> rgb 565 bmp ==== */
                    /* ==== bgr 888 dib -> bgr 565 bmp ==== */
                    convs->Convert_888_to_565_asis
                        (dstwidth,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else if ((rSrc==0xff && bmpImage->red_mask==0xf800) ||
                           (bSrc==0xff && bmpImage->blue_mask==0xf800)) {
                    /* ==== rgb 888 dib -> bgr 565 bmp ==== */
                    /* ==== bgr 888 dib -> rgb 565 bmp ==== */
                    convs->Convert_888_to_565_reverse
                        (dstwidth,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else {
                    goto notsupported;
                }
            } else {
                goto notsupported;
            }
        }
        break;

    default:
    notsupported:
        WARN("from 24 bit DIB (%lx,%lx,%lx) to unknown %d bit bitmap (%lx,%lx,%lx)\n",
              rSrc, gSrc, bSrc, bmpImage->bits_per_pixel, bmpImage->red_mask,
              bmpImage->green_mask, bmpImage->blue_mask );
        /* fall through */
    case 1:
    case 4:
    case 8:
        {
            /* ==== rgb 888 dib -> any bmp bormat ==== */
            const BYTE* srcbyte;

            /* Windows only supports one 24bpp DIB format: RGB888 */
            srcbits+=left*3;
            for (h = lines - 1; h >= 0; h--) {
                srcbyte=(const BYTE*)srcbits;
                for (x = left; x < dstwidth+left; x++) {
                    XPutPixel(bmpImage, x, h,
                              X11DRV_PALETTE_ToPhysical
                              (physDev, RGB(srcbyte[2], srcbyte[1], srcbyte[0])));
                    srcbyte+=3;
                }
                srcbits += linebytes;
            }
        }
        break;
    }
}


/***********************************************************************
 *           X11DRV_DIB_GetImageBits_24
 *
 * GetDIBits for an 24-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_24( int lines, BYTE *dstbits,
					DWORD dstwidth, DWORD srcwidth,
					PALETTEENTRY *srccolors,
                                        DWORD rDst, DWORD gDst, DWORD bDst,
					XImage *bmpImage, DWORD linebytes )
{
    DWORD x;
    int h;
    const dib_conversions *convs = (bmpImage->byte_order == LSBFirst) ? &dib_normal : &dib_src_byteswap;

    if (lines < 0 )
    {
        lines = -lines;
        dstbits = dstbits + ( linebytes * (lines-1) );
        linebytes = -linebytes;
    }

    switch (bmpImage->depth)
    {
    case 24:
        if (bmpImage->bits_per_pixel==24) {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (rDst==bmpImage->red_mask) {
                /* ==== rgb 888 bmp -> rgb 888 dib ==== */
                /* ==== bgr 888 bmp -> bgr 888 dib ==== */
                convs->Convert_888_asis
                    (dstwidth,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes);
            } else {
                /* ==== rgb 888 bmp -> bgr 888 dib ==== */
                /* ==== bgr 888 bmp -> rgb 888 dib ==== */
                convs->Convert_888_reverse
                    (dstwidth,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes);
            }
            break;
        }
        /* fall through */

    case 32:
        {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (rDst==bmpImage->red_mask) {
                /* ==== rgb 888 bmp -> rgb 0888 dib ==== */
                /* ==== bgr 888 bmp -> bgr 0888 dib ==== */
                convs->Convert_0888_to_888_asis
                    (dstwidth,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes);
            } else {
                /* ==== rgb 888 bmp -> bgr 0888 dib ==== */
                /* ==== bgr 888 bmp -> rgb 0888 dib ==== */
                convs->Convert_0888_to_888_reverse
                    (dstwidth,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes);
            }
            break;
        }

    case 15:
    case 16:
        {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask==0x03e0) {
                if ((rDst==0xff0000 && bmpImage->red_mask==0x7f00) ||
                    (bDst==0xff0000 && bmpImage->blue_mask==0x7f00)) {
                    /* ==== rgb 555 bmp -> rgb 888 dib ==== */
                    /* ==== bgr 555 bmp -> bgr 888 dib ==== */
                    convs->Convert_555_to_888_asis
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else if ((rDst==0xff && bmpImage->red_mask==0x7f00) ||
                           (bDst==0xff && bmpImage->blue_mask==0x7f00)) {
                    /* ==== rgb 555 bmp -> bgr 888 dib ==== */
                    /* ==== bgr 555 bmp -> rgb 888 dib ==== */
                    convs->Convert_555_to_888_reverse
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else {
                    goto notsupported;
                }
            } else if (bmpImage->green_mask==0x07e0) {
                if ((rDst==0xff0000 && bmpImage->red_mask==0xf800) ||
                    (bDst==0xff0000 && bmpImage->blue_mask==0xf800)) {
                    /* ==== rgb 565 bmp -> rgb 888 dib ==== */
                    /* ==== bgr 565 bmp -> bgr 888 dib ==== */
                    convs->Convert_565_to_888_asis
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else if ((rDst==0xff && bmpImage->red_mask==0xf800) ||
                           (bDst==0xff && bmpImage->blue_mask==0xf800)) {
                    /* ==== rgb 565 bmp -> bgr 888 dib ==== */
                    /* ==== bgr 565 bmp -> rgb 888 dib ==== */
                    convs->Convert_565_to_888_reverse
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else {
                    goto notsupported;
                }
            } else {
                goto notsupported;
            }
        }
        break;

    case 1:
    case 4:
        if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors) {
            /* ==== pal 1 or 4 bmp -> rgb 888 dib ==== */
            BYTE* dstbyte;

            /* Windows only supports one 24bpp DIB format: rgb 888 */
            for (h = lines - 1; h >= 0; h--) {
                dstbyte=dstbits;
                for (x = 0; x < dstwidth; x++) {
                    PALETTEENTRY srcval;
                    srcval=srccolors[XGetPixel(bmpImage, x, h)];
                    dstbyte[0]=srcval.peBlue;
                    dstbyte[1]=srcval.peGreen;
                    dstbyte[2]=srcval.peRed;
                    dstbyte+=3;
                }
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 8:
        if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask == 0 && srccolors) {
            /* ==== pal 8 bmp -> rgb 888 dib ==== */
            const void* srcbits;
            const BYTE* srcpixel;
            BYTE* dstbyte;

            /* Windows only supports one 24bpp DIB format: rgb 888 */
            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;
            for (h = lines - 1; h >= 0; h--) {
                srcpixel=srcbits;
                dstbyte=dstbits;
                for (x = 0; x < dstwidth; x++ ) {
                    PALETTEENTRY srcval;
                    srcval=srccolors[(int)*srcpixel++];
                    dstbyte[0]=srcval.peBlue;
                    dstbyte[1]=srcval.peGreen;
                    dstbyte[2]=srcval.peRed;
                    dstbyte+=3;
                }
                srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    default:
    notsupported:
        {
            /* ==== any bmp format -> 888 dib ==== */
            BYTE* dstbyte;

            WARN("from unknown %d bit bitmap (%lx,%lx,%lx) to 24 bit DIB (%lx,%lx,%lx)\n",
                  bmpImage->depth, bmpImage->red_mask,
                  bmpImage->green_mask, bmpImage->blue_mask,
                  rDst, gDst, bDst );

            /* Windows only supports one 24bpp DIB format: rgb 888 */
            for (h = lines - 1; h >= 0; h--) {
                dstbyte=dstbits;
                for (x = 0; x < dstwidth; x++) {
                    COLORREF srcval=X11DRV_PALETTE_ToLogical
                        (XGetPixel( bmpImage, x, h ));
                    dstbyte[0]=GetBValue(srcval);
                    dstbyte[1]=GetGValue(srcval);
                    dstbyte[2]=GetRValue(srcval);
                    dstbyte+=3;
                }
                dstbits += linebytes;
            }
        }
        break;
    }
}


/***********************************************************************
 *           X11DRV_DIB_SetImageBits_32
 *
 * SetDIBits for a 32-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_32(int lines, const BYTE *srcbits,
                                       DWORD srcwidth, DWORD dstwidth, int left,
                                       X11DRV_PDEVICE *physDev,
                                       DWORD rSrc, DWORD gSrc, DWORD bSrc,
                                       XImage *bmpImage,
                                       DWORD linebytes)
{
    DWORD x, *ptr;
    int h;
    const dib_conversions *convs = (bmpImage->byte_order == LSBFirst) ? &dib_normal : &dib_dst_byteswap;

    if (lines < 0 )
    {
       lines = -lines;
       srcbits = srcbits + ( linebytes * (lines-1) );
       linebytes = -linebytes;
    }

    ptr = (DWORD *) srcbits + left;

    switch (bmpImage->depth)
    {
    case 24:
        if (bmpImage->bits_per_pixel==24) {
            char* dstbits;

            srcbits=srcbits+left*4;
            dstbits=bmpImage->data+left*3+(lines-1)*bmpImage->bytes_per_line;

            if (rSrc==bmpImage->red_mask && gSrc==bmpImage->green_mask && bSrc==bmpImage->blue_mask) {
                /* ==== rgb 0888 dib -> rgb 888 bmp ==== */
                /* ==== bgr 0888 dib -> bgr 888 bmp ==== */
                convs->Convert_0888_to_888_asis
                    (dstwidth,lines,
                     srcbits,linebytes,
                     dstbits,-bmpImage->bytes_per_line);
            } else if (bmpImage->green_mask!=0x00ff00 ||
                       (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
                /* the tests below assume sane bmpImage masks */
            } else if (rSrc==bmpImage->blue_mask && gSrc==bmpImage->green_mask && bSrc==bmpImage->red_mask) {
                /* ==== rgb 0888 dib -> bgr 888 bmp ==== */
                /* ==== bgr 0888 dib -> rgb 888 bmp ==== */
                convs->Convert_0888_to_888_reverse
                    (dstwidth,lines,
                     srcbits,linebytes,
                     dstbits,-bmpImage->bytes_per_line);
            } else if (bmpImage->blue_mask==0xff) {
                /* ==== any 0888 dib -> rgb 888 bmp ==== */
                convs->Convert_any0888_to_rgb888
                    (dstwidth,lines,
                     srcbits,linebytes,
                     rSrc,gSrc,bSrc,
                     dstbits,-bmpImage->bytes_per_line);
            } else {
                /* ==== any 0888 dib -> bgr 888 bmp ==== */
                convs->Convert_any0888_to_bgr888
                    (dstwidth,lines,
                     srcbits,linebytes,
                     rSrc,gSrc,bSrc,
                     dstbits,-bmpImage->bytes_per_line);
            }
            break;
        }
        /* fall through */

    case 32:
        {
            char* dstbits;

            srcbits=srcbits+left*4;
            dstbits=bmpImage->data+left*4+(lines-1)*bmpImage->bytes_per_line;

            if (gSrc==bmpImage->green_mask) {
                if (rSrc==bmpImage->red_mask && bSrc==bmpImage->blue_mask) {
                    /* ==== rgb 0888 dib -> rgb 0888 bmp ==== */
                    /* ==== bgr 0888 dib -> bgr 0888 bmp ==== */
                    convs->Convert_0888_asis
                        (dstwidth,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else if (bmpImage->green_mask!=0x00ff00 ||
                           (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                    goto notsupported;
                    /* the tests below assume sane bmpImage masks */
                } else if (rSrc==bmpImage->blue_mask && bSrc==bmpImage->red_mask) {
                    /* ==== rgb 0888 dib -> bgr 0888 bmp ==== */
                    /* ==== bgr 0888 dib -> rgb 0888 bmp ==== */
                    convs->Convert_0888_reverse
                        (dstwidth,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else {
                    /* ==== any 0888 dib -> any 0888 bmp ==== */
                    convs->Convert_0888_any
                        (dstwidth,lines,
                         srcbits,linebytes,
                         rSrc,gSrc,bSrc,
                         dstbits,-bmpImage->bytes_per_line,
                         bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask);
                }
            } else if (bmpImage->green_mask!=0x00ff00 ||
                       (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
                /* the tests below assume sane bmpImage masks */
            } else {
                /* ==== any 0888 dib -> any 0888 bmp ==== */
                convs->Convert_0888_any
                    (dstwidth,lines,
                     srcbits,linebytes,
                     rSrc,gSrc,bSrc,
                     dstbits,-bmpImage->bytes_per_line,
                     bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask);
            }
        }
        break;

    case 15:
    case 16:
        {
            char* dstbits;

            srcbits=srcbits+left*4;
            dstbits=bmpImage->data+left*2+(lines-1)*bmpImage->bytes_per_line;

            if (rSrc==0xff0000 && gSrc==0x00ff00 && bSrc==0x0000ff) {
                if (bmpImage->green_mask==0x03e0) {
                    if (bmpImage->red_mask==0x7f00) {
                        /* ==== rgb 0888 dib -> rgb 555 bmp ==== */
                        convs->Convert_0888_to_555_asis
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else if (bmpImage->blue_mask==0x7f00) {
                        /* ==== rgb 0888 dib -> bgr 555 bmp ==== */
                        convs->Convert_0888_to_555_reverse
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else {
                        goto notsupported;
                    }
                } else if (bmpImage->green_mask==0x07e0) {
                    if (bmpImage->red_mask==0xf800) {
                        /* ==== rgb 0888 dib -> rgb 565 bmp ==== */
                        convs->Convert_0888_to_565_asis
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else if (bmpImage->blue_mask==0xf800) {
                        /* ==== rgb 0888 dib -> bgr 565 bmp ==== */
                        convs->Convert_0888_to_565_reverse
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else {
                        goto notsupported;
                    }
                } else {
                    goto notsupported;
                }
            } else if (rSrc==0x0000ff && gSrc==0x00ff00 && bSrc==0xff0000) {
                if (bmpImage->green_mask==0x03e0) {
                    if (bmpImage->blue_mask==0x7f00) {
                        /* ==== bgr 0888 dib -> bgr 555 bmp ==== */
                        convs->Convert_0888_to_555_asis
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else if (bmpImage->red_mask==0x7f00) {
                        /* ==== bgr 0888 dib -> rgb 555 bmp ==== */
                        convs->Convert_0888_to_555_reverse
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else {
                        goto notsupported;
                    }
                } else if (bmpImage->green_mask==0x07e0) {
                    if (bmpImage->blue_mask==0xf800) {
                        /* ==== bgr 0888 dib -> bgr 565 bmp ==== */
                        convs->Convert_0888_to_565_asis
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else if (bmpImage->red_mask==0xf800) {
                        /* ==== bgr 0888 dib -> rgb 565 bmp ==== */
                        convs->Convert_0888_to_565_reverse
                            (dstwidth,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else {
                        goto notsupported;
                    }
                } else {
                    goto notsupported;
                }
            } else {
                if (bmpImage->green_mask==0x03e0 &&
                    (bmpImage->red_mask==0x7f00 ||
                     bmpImage->blue_mask==0x7f00)) {
                    /* ==== any 0888 dib -> rgb or bgr 555 bmp ==== */
                    convs->Convert_any0888_to_5x5
                        (dstwidth,lines,
                         srcbits,linebytes,
                         rSrc,gSrc,bSrc,
                         dstbits,-bmpImage->bytes_per_line,
                         bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask);
                } else if (bmpImage->green_mask==0x07e0 &&
                           (bmpImage->red_mask==0xf800 ||
                            bmpImage->blue_mask==0xf800)) {
                    /* ==== any 0888 dib -> rgb or bgr 565 bmp ==== */
                    convs->Convert_any0888_to_5x5
                        (dstwidth,lines,
                         srcbits,linebytes,
                         rSrc,gSrc,bSrc,
                         dstbits,-bmpImage->bytes_per_line,
                         bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask);
                } else {
                    goto notsupported;
                }
            }
        }
        break;

    default:
    notsupported:
        WARN("from 32 bit DIB (%lx,%lx,%lx) to unknown %d bit bitmap (%lx,%lx,%lx)\n",
              rSrc, gSrc, bSrc, bmpImage->bits_per_pixel, bmpImage->red_mask,
              bmpImage->green_mask, bmpImage->blue_mask );
        /* fall through */
    case 1:
    case 4:
    case 8:
        {
            /* ==== any 0888 dib -> pal 1, 4 or 8 bmp ==== */
            const DWORD* srcpixel;
            int rShift,gShift,bShift;

            rShift=X11DRV_DIB_MaskToShift(rSrc);
            gShift=X11DRV_DIB_MaskToShift(gSrc);
            bShift=X11DRV_DIB_MaskToShift(bSrc);
            srcbits+=left*4;
            for (h = lines - 1; h >= 0; h--) {
                srcpixel=(const DWORD*)srcbits;
                for (x = left; x < dstwidth+left; x++) {
                    DWORD srcvalue;
                    BYTE red,green,blue;
                    srcvalue=*srcpixel++;
                    red=  (srcvalue >> rShift) & 0xff;
                    green=(srcvalue >> gShift) & 0xff;
                    blue= (srcvalue >> bShift) & 0xff;
                    XPutPixel(bmpImage, x, h, X11DRV_PALETTE_ToPhysical
                              (physDev, RGB(red,green,blue)));
                }
                srcbits += linebytes;
            }
        }
        break;
    }

}

/***********************************************************************
 *           X11DRV_DIB_GetImageBits_32
 *
 * GetDIBits for an 32-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_32( int lines, BYTE *dstbits,
					DWORD dstwidth, DWORD srcwidth,
					PALETTEENTRY *srccolors,
					DWORD rDst, DWORD gDst, DWORD bDst,
					XImage *bmpImage, DWORD linebytes )
{
    DWORD x;
    int h;
    BYTE *bits;
    const dib_conversions *convs = (bmpImage->byte_order == LSBFirst) ? &dib_normal : &dib_src_byteswap;

    if (lines < 0 )
    {
        lines = -lines;
        dstbits = dstbits + ( linebytes * (lines-1) );
        linebytes = -linebytes;
    }

    bits = dstbits;

    switch (bmpImage->depth)
    {
    case 24:
        if (bmpImage->bits_per_pixel==24) {
            const void* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (rDst==bmpImage->red_mask && gDst==bmpImage->green_mask && bDst==bmpImage->blue_mask) {
                /* ==== rgb 888 bmp -> rgb 0888 dib ==== */
                /* ==== bgr 888 bmp -> bgr 0888 dib ==== */
                convs->Convert_888_to_0888_asis
                    (dstwidth,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes);
            } else if (bmpImage->green_mask!=0x00ff00 ||
                       (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
                /* the tests below assume sane bmpImage masks */
            } else if (rDst==bmpImage->blue_mask && gDst==bmpImage->green_mask && bDst==bmpImage->red_mask) {
                /* ==== rgb 888 bmp -> bgr 0888 dib ==== */
                /* ==== bgr 888 bmp -> rgb 0888 dib ==== */
                convs->Convert_888_to_0888_reverse
                    (dstwidth,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes);
            } else if (bmpImage->blue_mask==0xff) {
                /* ==== rgb 888 bmp -> any 0888 dib ==== */
                convs->Convert_rgb888_to_any0888
                    (dstwidth,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes,
                     rDst,gDst,bDst);
            } else {
                /* ==== bgr 888 bmp -> any 0888 dib ==== */
                convs->Convert_bgr888_to_any0888
                    (dstwidth,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes,
                     rDst,gDst,bDst);
            }
            break;
        }
        /* fall through */

    case 32:
        {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (gDst==bmpImage->green_mask) {
                if (rDst==bmpImage->red_mask && bDst==bmpImage->blue_mask) {
                    /* ==== rgb 0888 bmp -> rgb 0888 dib ==== */
                    /* ==== bgr 0888 bmp -> bgr 0888 dib ==== */
                    convs->Convert_0888_asis
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else if (bmpImage->green_mask!=0x00ff00 ||
                           (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                    goto notsupported;
                    /* the tests below assume sane bmpImage masks */
                } else if (rDst==bmpImage->blue_mask && bDst==bmpImage->red_mask) {
                    /* ==== rgb 0888 bmp -> bgr 0888 dib ==== */
                    /* ==== bgr 0888 bmp -> rgb 0888 dib ==== */
                    convs->Convert_0888_reverse
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else {
                    /* ==== any 0888 bmp -> any 0888 dib ==== */
                    convs->Convert_0888_any
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask,
                         dstbits,linebytes,
                         rDst,gDst,bDst);
                }
            } else if (bmpImage->green_mask!=0x00ff00 ||
                       (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
                /* the tests below assume sane bmpImage masks */
            } else {
                /* ==== any 0888 bmp -> any 0888 dib ==== */
                convs->Convert_0888_any
                    (dstwidth,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask,
                     dstbits,linebytes,
                     rDst,gDst,bDst);
            }
        }
        break;

    case 15:
    case 16:
        {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (rDst==0xff0000 && gDst==0x00ff00 && bDst==0x0000ff) {
                if (bmpImage->green_mask==0x03e0) {
                    if (bmpImage->red_mask==0x7f00) {
                        /* ==== rgb 555 bmp -> rgb 0888 dib ==== */
                        convs->Convert_555_to_0888_asis
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else if (bmpImage->blue_mask==0x7f00) {
                        /* ==== bgr 555 bmp -> rgb 0888 dib ==== */
                        convs->Convert_555_to_0888_reverse
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        goto notsupported;
                    }
                } else if (bmpImage->green_mask==0x07e0) {
                    if (bmpImage->red_mask==0xf800) {
                        /* ==== rgb 565 bmp -> rgb 0888 dib ==== */
                        convs->Convert_565_to_0888_asis
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else if (bmpImage->blue_mask==0xf800) {
                        /* ==== bgr 565 bmp -> rgb 0888 dib ==== */
                        convs->Convert_565_to_0888_reverse
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        goto notsupported;
                    }
                } else {
                    goto notsupported;
                }
            } else if (rDst==0x0000ff && gDst==0x00ff00 && bDst==0xff0000) {
                if (bmpImage->green_mask==0x03e0) {
                    if (bmpImage->blue_mask==0x7f00) {
                        /* ==== bgr 555 bmp -> bgr 0888 dib ==== */
                        convs->Convert_555_to_0888_asis
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else if (bmpImage->red_mask==0x7f00) {
                        /* ==== rgb 555 bmp -> bgr 0888 dib ==== */
                        convs->Convert_555_to_0888_reverse
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        goto notsupported;
                    }
                } else if (bmpImage->green_mask==0x07e0) {
                    if (bmpImage->blue_mask==0xf800) {
                        /* ==== bgr 565 bmp -> bgr 0888 dib ==== */
                        convs->Convert_565_to_0888_asis
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else if (bmpImage->red_mask==0xf800) {
                        /* ==== rgb 565 bmp -> bgr 0888 dib ==== */
                        convs->Convert_565_to_0888_reverse
                            (dstwidth,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        goto notsupported;
                    }
                } else {
                    goto notsupported;
                }
            } else {
                if (bmpImage->green_mask==0x03e0 &&
                    (bmpImage->red_mask==0x7f00 ||
                     bmpImage->blue_mask==0x7f00)) {
                    /* ==== rgb or bgr 555 bmp -> any 0888 dib ==== */
                    convs->Convert_5x5_to_any0888
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask,
                         dstbits,linebytes,
                         rDst,gDst,bDst);
                } else if (bmpImage->green_mask==0x07e0 &&
                           (bmpImage->red_mask==0xf800 ||
                            bmpImage->blue_mask==0xf800)) {
                    /* ==== rgb or bgr 565 bmp -> any 0888 dib ==== */
                    convs->Convert_5x5_to_any0888
                        (dstwidth,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask,
                         dstbits,linebytes,
                         rDst,gDst,bDst);
                } else {
                    goto notsupported;
                }
            }
        }
        break;

    case 1:
    case 4:
        if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors) {
            /* ==== pal 1 or 4 bmp -> any 0888 dib ==== */
            int rShift,gShift,bShift;
            DWORD* dstpixel;

            rShift=X11DRV_DIB_MaskToShift(rDst);
            gShift=X11DRV_DIB_MaskToShift(gDst);
            bShift=X11DRV_DIB_MaskToShift(bDst);
            for (h = lines - 1; h >= 0; h--) {
                dstpixel=(DWORD*)dstbits;
                for (x = 0; x < dstwidth; x++) {
                    PALETTEENTRY srcval;
                    srcval = srccolors[XGetPixel(bmpImage, x, h)];
                    *dstpixel++=(srcval.peRed   << rShift) |
                                (srcval.peGreen << gShift) |
                                (srcval.peBlue  << bShift);
                }
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 8:
        if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors) {
            /* ==== pal 8 bmp -> any 0888 dib ==== */
            int rShift,gShift,bShift;
            const void* srcbits;
            const BYTE* srcpixel;
            DWORD* dstpixel;

            rShift=X11DRV_DIB_MaskToShift(rDst);
            gShift=X11DRV_DIB_MaskToShift(gDst);
            bShift=X11DRV_DIB_MaskToShift(bDst);
            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;
            for (h = lines - 1; h >= 0; h--) {
                srcpixel=srcbits;
                dstpixel=(DWORD*)dstbits;
                for (x = 0; x < dstwidth; x++) {
                    PALETTEENTRY srcval;
                    srcval=srccolors[(int)*srcpixel++];
                    *dstpixel++=(srcval.peRed   << rShift) |
                                (srcval.peGreen << gShift) |
                                (srcval.peBlue  << bShift);
                }
                srcbits = (char*)srcbits - bmpImage->bytes_per_line;
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    default:
    notsupported:
        {
            /* ==== any bmp format -> any 0888 dib ==== */
            int rShift,gShift,bShift;
            DWORD* dstpixel;

            WARN("from unknown %d bit bitmap (%lx,%lx,%lx) to 32 bit DIB (%lx,%lx,%lx)\n",
                  bmpImage->depth, bmpImage->red_mask,
                  bmpImage->green_mask, bmpImage->blue_mask,
                  rDst,gDst,bDst);

            rShift=X11DRV_DIB_MaskToShift(rDst);
            gShift=X11DRV_DIB_MaskToShift(gDst);
            bShift=X11DRV_DIB_MaskToShift(bDst);
            for (h = lines - 1; h >= 0; h--) {
                dstpixel=(DWORD*)dstbits;
                for (x = 0; x < dstwidth; x++) {
                    COLORREF srcval;
                    srcval=X11DRV_PALETTE_ToLogical(XGetPixel(bmpImage, x, h));
                    *dstpixel++=(GetRValue(srcval) << rShift) |
                                (GetGValue(srcval) << gShift) |
                                (GetBValue(srcval) << bShift);
                }
                dstbits += linebytes;
            }
        }
        break;
    }
}

/***********************************************************************
 *           X11DRV_DIB_SetImageBits
 *
 * Transfer the bits to an X image.
 * Helper function for SetDIBits() and SetDIBitsToDevice().
 */
static int X11DRV_DIB_SetImageBits( const X11DRV_DIB_IMAGEBITS_DESCR *descr )
{
    int lines = descr->lines >= 0 ? descr->lines : -descr->lines;
    XImage *bmpImage;

    wine_tsx11_lock();
    if (descr->image)
        bmpImage = descr->image;
    else {
        bmpImage = XCreateImage( gdi_display, visual, descr->depth, ZPixmap, 0, NULL,
				 descr->infoWidth, lines, 32, 0 );
	bmpImage->data = calloc( lines, bmpImage->bytes_per_line );
        if(bmpImage->data == NULL) {
            ERR("Out of memory!\n");
            XDestroyImage( bmpImage );
            wine_tsx11_unlock();
            return lines;
        }
    }

    TRACE("Dib: depth=%d r=%lx g=%lx b=%lx\n",
          descr->infoBpp,descr->rMask,descr->gMask,descr->bMask);
    TRACE("Bmp: depth=%d/%d r=%lx g=%lx b=%lx\n",
          bmpImage->depth,bmpImage->bits_per_pixel,
          bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask);

      /* Transfer the pixels */
    switch(descr->infoBpp)
    {
    case 1:
	X11DRV_DIB_SetImageBits_1( descr->lines, descr->bits, descr->infoWidth,
				   descr->width, descr->xSrc, (int *)(descr->colorMap),
				   bmpImage, descr->dibpitch );
	break;
    case 4:
        if (descr->compression) {
	    XGetSubImage( gdi_display, descr->drawable, descr->xDest, descr->yDest,
			  descr->width, descr->height, AllPlanes, ZPixmap,
			  bmpImage, descr->xSrc, descr->ySrc );

	    X11DRV_DIB_SetImageBits_RLE4( descr->lines, descr->bits,
					  descr->infoWidth, descr->width,
					  descr->xSrc, (int *)(descr->colorMap),
					  bmpImage );
	} else
	    X11DRV_DIB_SetImageBits_4( descr->lines, descr->bits,
				       descr->infoWidth, descr->width,
				       descr->xSrc, (int*)(descr->colorMap),
				       bmpImage, descr->dibpitch );
	break;
    case 8:
        if (descr->compression) {
	    XGetSubImage( gdi_display, descr->drawable, descr->xDest, descr->yDest,
			  descr->width, descr->height, AllPlanes, ZPixmap,
			  bmpImage, descr->xSrc, descr->ySrc );
	    X11DRV_DIB_SetImageBits_RLE8( descr->lines, descr->bits,
					  descr->infoWidth, descr->width,
					  descr->xSrc, (int *)(descr->colorMap),
					  bmpImage );
	} else
	    X11DRV_DIB_SetImageBits_8( descr->lines, descr->bits,
				       descr->infoWidth, descr->width,
				       descr->xSrc, (int *)(descr->colorMap),
				       bmpImage, descr->dibpitch );
	break;
    case 15:
    case 16:
	X11DRV_DIB_SetImageBits_16( descr->lines, descr->bits,
				    descr->infoWidth, descr->width,
                                   descr->xSrc, descr->physDev,
                                   descr->rMask, descr->gMask, descr->bMask,
                                   bmpImage, descr->dibpitch);
	break;
    case 24:
	X11DRV_DIB_SetImageBits_24( descr->lines, descr->bits,
				    descr->infoWidth, descr->width,
				    descr->xSrc, descr->physDev,
                                    descr->rMask, descr->gMask, descr->bMask,
				    bmpImage, descr->dibpitch);
	break;
    case 32:
	X11DRV_DIB_SetImageBits_32( descr->lines, descr->bits,
				    descr->infoWidth, descr->width,
                                   descr->xSrc, descr->physDev,
                                   descr->rMask, descr->gMask, descr->bMask,
                                   bmpImage, descr->dibpitch);
	break;
    default:
        WARN("(%d): Invalid depth\n", descr->infoBpp );
        break;
    }

    TRACE("XPutImage(%ld,%p,%p,%d,%d,%d,%d,%d,%d)\n",
     descr->drawable, descr->gc, bmpImage,
     descr->xSrc, descr->ySrc, descr->xDest, descr->yDest,
     descr->width, descr->height);
#ifdef HAVE_LIBXXSHM
    if (descr->useShm)
    {
        XShmPutImage( gdi_display, descr->drawable, descr->gc, bmpImage,
                      descr->xSrc, descr->ySrc, descr->xDest, descr->yDest,
                      descr->width, descr->height, FALSE );
        XSync( gdi_display, 0 );
    }
    else
#endif
        XPutImage( gdi_display, descr->drawable, descr->gc, bmpImage,
		   descr->xSrc, descr->ySrc, descr->xDest, descr->yDest,
		   descr->width, descr->height );

    if (!descr->image) XDestroyImage( bmpImage );
    wine_tsx11_unlock();
    return lines;
}

/***********************************************************************
 *           X11DRV_DIB_GetImageBits
 *
 * Transfer the bits from an X image.
 */
static int X11DRV_DIB_GetImageBits( const X11DRV_DIB_IMAGEBITS_DESCR *descr )
{
    int lines = descr->lines >= 0 ? descr->lines : -descr->lines;
    XImage *bmpImage;

    wine_tsx11_lock();
   if (descr->image)
        bmpImage = descr->image;
    else {
        bmpImage = XCreateImage( gdi_display, visual, descr->depth, ZPixmap, 0, NULL,
				 descr->infoWidth, lines, 32, 0 );
	bmpImage->data = calloc( lines, bmpImage->bytes_per_line );
        if(bmpImage->data == NULL) {
            ERR("Out of memory!\n");
            XDestroyImage( bmpImage );
            wine_tsx11_unlock();
            return lines;
        }
    }

#ifdef HAVE_LIBXXSHM
    if (descr->useShm)
    {
        int saveRed, saveGreen, saveBlue;

        TRACE("XShmGetImage(%p, %ld, %p, %d, %d, %ld)\n",
                            gdi_display, descr->drawable, bmpImage,
                            descr->xSrc, descr->ySrc, AllPlanes);

 	/* We must save and restore the bmpImage's masks in order
 	 * to preserve them across the call to XShmGetImage, which
 	 * decides to eleminate them since it doesn't happen to know
 	 * what the format of the image is supposed to be, even though
 	 * we do. */
        saveRed = bmpImage->red_mask;
        saveBlue= bmpImage->blue_mask;
        saveGreen = bmpImage->green_mask;

        XShmGetImage( gdi_display, descr->drawable, bmpImage,
                      descr->xSrc, descr->ySrc, AllPlanes);

        bmpImage->red_mask = saveRed;
        bmpImage->blue_mask = saveBlue;
        bmpImage->green_mask = saveGreen;
    }
    else
#endif /* HAVE_LIBXXSHM */
    {
        TRACE("XGetSubImage(%p,%ld,%d,%d,%d,%d,%ld,%d,%p,%d,%d)\n",
              gdi_display, descr->drawable, descr->xSrc, descr->ySrc, descr->width,
              lines, AllPlanes, ZPixmap, bmpImage, descr->xDest, descr->yDest);
        XGetSubImage( gdi_display, descr->drawable, descr->xSrc, descr->ySrc,
                      descr->width, lines, AllPlanes, ZPixmap,
                      bmpImage, descr->xDest, descr->yDest );
    }

    TRACE("Dib: depth=%2d r=%lx g=%lx b=%lx\n",
          descr->infoBpp,descr->rMask,descr->gMask,descr->bMask);
    TRACE("Bmp: depth=%2d/%2d r=%lx g=%lx b=%lx\n",
          bmpImage->depth,bmpImage->bits_per_pixel,
          bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask);
      /* Transfer the pixels */
    switch(descr->infoBpp)
    {
    case 1:
          X11DRV_DIB_GetImageBits_1( descr->lines,(LPVOID)descr->bits,
				     descr->infoWidth, descr->width,
				     descr->colorMap, descr->palentry,
                                     bmpImage, descr->dibpitch );
       break;

    case 4:
       if (descr->compression)
	   FIXME("Compression not yet supported!\n");
       else
	   X11DRV_DIB_GetImageBits_4( descr->lines,(LPVOID)descr->bits,
				      descr->infoWidth, descr->width,
				      descr->colorMap, descr->palentry,
				      bmpImage, descr->dibpitch );
       break;

    case 8:
       if (descr->compression)
	   FIXME("Compression not yet supported!\n");
       else
	   X11DRV_DIB_GetImageBits_8( descr->lines, (LPVOID)descr->bits,
				      descr->infoWidth, descr->width,
				      descr->colorMap, descr->palentry,
				      bmpImage, descr->dibpitch );
       break;
    case 15:
    case 16:
       X11DRV_DIB_GetImageBits_16( descr->lines, (LPVOID)descr->bits,
				   descr->infoWidth,descr->width,
				   descr->palentry,
				   descr->rMask, descr->gMask, descr->bMask,
				   bmpImage, descr->dibpitch );
       break;

    case 24:
       X11DRV_DIB_GetImageBits_24( descr->lines, (LPVOID)descr->bits,
				   descr->infoWidth,descr->width,
				   descr->palentry,
				   descr->rMask, descr->gMask, descr->bMask,
                                   bmpImage, descr->dibpitch);
       break;

    case 32:
       X11DRV_DIB_GetImageBits_32( descr->lines, (LPVOID)descr->bits,
				   descr->infoWidth, descr->width,
				   descr->palentry,
                                   descr->rMask, descr->gMask, descr->bMask,
                                   bmpImage, descr->dibpitch);
       break;

    default:
        WARN("(%d): Invalid depth\n", descr->infoBpp );
        break;
    }

    if (!descr->image) XDestroyImage( bmpImage );
    wine_tsx11_unlock();
    return lines;
}

/*************************************************************************
 *		X11DRV_SetDIBitsToDevice
 *
 */
INT X11DRV_SetDIBitsToDevice( X11DRV_PDEVICE *physDev, INT xDest, INT yDest, DWORD cx,
				DWORD cy, INT xSrc, INT ySrc,
				UINT startscan, UINT lines, LPCVOID bits,
				const BITMAPINFO *info, UINT coloruse )
{
    X11DRV_DIB_IMAGEBITS_DESCR descr;
    DWORD width;
    INT result;
    int height;
    BOOL top_down;
    POINT pt;
    DC *dc = physDev->dc;

    if (DIB_GetBitmapInfo( &info->bmiHeader, &width, &height,
			   &descr.infoBpp, &descr.compression ) == -1)
        return 0;
    top_down = (height < 0);
    if (top_down) height = -height;

    pt.x = xDest;
    pt.y = yDest;
    LPtoDP(physDev->hdc, &pt, 1);

    if (!lines || (startscan >= height)) return 0;
    if (!top_down && startscan + lines > height) lines = height - startscan;

    /* make xSrc,ySrc point to the upper-left corner, not the lower-left one,
     * and clamp all values to fit inside [startscan,startscan+lines]
     */
    if (ySrc + cy <= startscan + lines)
    {
        INT y = startscan + lines - (ySrc + cy);
        if (ySrc < startscan) cy -= (startscan - ySrc);
        if (!top_down)
        {
            /* avoid getting unnecessary lines */
            ySrc = 0;
            if (y >= lines) return 0;
            lines -= y;
        }
        else
        {
            if (y >= lines) return lines;
            ySrc = y;  /* need to get all lines in top down mode */
        }
    }
    else
    {
        if (ySrc >= startscan + lines) return lines;
        pt.y += ySrc + cy - (startscan + lines);
        cy = startscan + lines - ySrc;
        ySrc = 0;
        if (cy > lines) cy = lines;
    }
    if (xSrc >= width) return lines;
    if (xSrc + cx >= width) cx = width - xSrc;
    if (!cx || !cy) return lines;

    X11DRV_SetupGCForText( physDev );  /* To have the correct colors */
    wine_tsx11_lock();
    XSetFunction(gdi_display, physDev->gc, X11DRV_XROPfunction[dc->ROPmode-1]);
    wine_tsx11_unlock();

    switch (descr.infoBpp)
    {
       case 1:
       case 4:
       case 8:
               descr.colorMap = (RGBQUAD *)X11DRV_DIB_BuildColorMap(
                                            coloruse == DIB_PAL_COLORS ? physDev : NULL, coloruse,
                                            dc->bitsPerPixel, info, &descr.nColorMap );
               if (!descr.colorMap) return 0;
               descr.rMask = descr.gMask = descr.bMask = 0;
               break;
       case 15:
       case 16:
               descr.rMask = (descr.compression == BI_BITFIELDS) ? *(DWORD *)info->bmiColors : 0x7c00;
               descr.gMask = (descr.compression == BI_BITFIELDS) ?  *((DWORD *)info->bmiColors + 1) : 0x03e0;
               descr.bMask = (descr.compression == BI_BITFIELDS) ?  *((DWORD *)info->bmiColors + 2) : 0x001f;
               descr.colorMap = 0;
               break;

       case 24:
       case 32:
               descr.rMask = (descr.compression == BI_BITFIELDS) ? *(DWORD *)info->bmiColors       : 0xff0000;
               descr.gMask = (descr.compression == BI_BITFIELDS) ? *((DWORD *)info->bmiColors + 1) : 0x00ff00;
               descr.bMask = (descr.compression == BI_BITFIELDS) ? *((DWORD *)info->bmiColors + 2) : 0x0000ff;
               descr.colorMap = 0;
               break;
    }

    descr.physDev   = physDev;
    descr.bits      = bits;
    descr.image     = NULL;
    descr.palentry  = NULL;
    descr.lines     = top_down ? -lines : lines;
    descr.infoWidth = width;
    descr.depth     = dc->bitsPerPixel;
    descr.drawable  = physDev->drawable;
    descr.gc        = physDev->gc;
    descr.xSrc      = xSrc;
    descr.ySrc      = ySrc;
    descr.xDest     = physDev->org.x + pt.x;
    descr.yDest     = physDev->org.y + pt.y;
    descr.width     = cx;
    descr.height    = cy;
    descr.useShm    = FALSE;
    descr.dibpitch  = ((width * descr.infoBpp + 31) &~31) / 8;

    result = X11DRV_DIB_SetImageBits( &descr );

    if (descr.infoBpp <= 8)
       HeapFree(GetProcessHeap(), 0, descr.colorMap);
    return result;
}

/***********************************************************************
 *           SetDIBits   (X11DRV.@)
 */
INT X11DRV_SetDIBits( X11DRV_PDEVICE *physDev, HBITMAP hbitmap, UINT startscan,
                      UINT lines, LPCVOID bits, const BITMAPINFO *info, UINT coloruse )
{
  X11DRV_DIB_IMAGEBITS_DESCR descr;
  BITMAPOBJ *bmp;
  int height, tmpheight;
  INT result;

  descr.physDev = physDev;

  if (DIB_GetBitmapInfo( &info->bmiHeader, &descr.infoWidth, &height,
			 &descr.infoBpp, &descr.compression ) == -1)
      return 0;

  tmpheight = height;
  if (height < 0) height = -height;
  if (!lines || (startscan >= height))
      return 0;

  if (!(bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC ))) return 0;

  if (startscan + lines > height) lines = height - startscan;

  switch (descr.infoBpp)
  {
       case 1:
       case 4:
       case 8:
	       descr.colorMap = (RGBQUAD *)X11DRV_DIB_BuildColorMap(
                        coloruse == DIB_PAL_COLORS ? descr.physDev : NULL, coloruse,
                                                          bmp->bitmap.bmBitsPixel,
                                                          info, &descr.nColorMap );
               if (!descr.colorMap)
               {
                   GDI_ReleaseObj( hbitmap );
                   return 0;
               }
               descr.rMask = descr.gMask = descr.bMask = 0;
               break;
       case 15:
       case 16:
               descr.rMask = (descr.compression == BI_BITFIELDS) ? *(DWORD *)info->bmiColors : 0x7c00;
               descr.gMask = (descr.compression == BI_BITFIELDS) ? *((DWORD *)info->bmiColors + 1) : 0x03e0;
               descr.bMask = (descr.compression == BI_BITFIELDS) ? *((DWORD *)info->bmiColors + 2) : 0x001f;
               descr.colorMap = 0;
               break;

       case 24:
       case 32:
               descr.rMask = (descr.compression == BI_BITFIELDS) ? *(DWORD *)info->bmiColors       : 0xff0000;
               descr.gMask = (descr.compression == BI_BITFIELDS) ? *((DWORD *)info->bmiColors + 1) : 0x00ff00;
               descr.bMask = (descr.compression == BI_BITFIELDS) ? *((DWORD *)info->bmiColors + 2) : 0x0000ff;
               descr.colorMap = 0;
               break;

       default: break;
  }

  descr.bits      = bits;
  descr.image     = NULL;
  descr.palentry  = NULL;
  descr.lines     = tmpheight >= 0 ? lines : -lines;
  descr.depth     = bmp->bitmap.bmBitsPixel;
  descr.drawable  = (Pixmap)bmp->physBitmap;
  descr.gc        = BITMAP_GC(bmp);
  descr.xSrc      = 0;
  descr.ySrc      = 0;
  descr.xDest     = 0;
  descr.yDest     = height - startscan - lines;
  descr.width     = bmp->bitmap.bmWidth;
  descr.height    = lines;
  descr.useShm    = FALSE;
  descr.dibpitch  = ((descr.infoWidth * descr.infoBpp + 31) &~31) / 8;
  X11DRV_DIB_Lock(bmp, DIB_Status_GdiMod, FALSE);
  result = X11DRV_DIB_SetImageBits( &descr );
  X11DRV_DIB_Unlock(bmp, TRUE);

  if (descr.colorMap) HeapFree(GetProcessHeap(), 0, descr.colorMap);

  GDI_ReleaseObj( hbitmap );
  return result;
}

/***********************************************************************
 *           GetDIBits   (X11DRV.@)
 */
INT X11DRV_GetDIBits( X11DRV_PDEVICE *physDev, HBITMAP hbitmap, UINT startscan, UINT lines,
                      LPVOID bits, BITMAPINFO *info, UINT coloruse )
{
  X11DRV_DIBSECTION *dib;
  X11DRV_DIB_IMAGEBITS_DESCR descr;
  PALETTEOBJ * palette;
  BITMAPOBJ *bmp;
  int height;
  DC *dc = physDev->dc;

  if (!(palette = (PALETTEOBJ*)GDI_GetObjPtr( dc->hPalette, PALETTE_MAGIC )))
      return 0;
  if (!(bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC )))
  {
      GDI_ReleaseObj( dc->hPalette );
      return 0;
  }
  dib = (X11DRV_DIBSECTION *) bmp->dib;

  TRACE("%u scanlines of (%i,%i) -> (%i,%i) starting from %u\n",
	lines, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
	(int)info->bmiHeader.biWidth, (int)info->bmiHeader.biHeight,
        startscan );

  if( lines > bmp->bitmap.bmHeight ) lines = bmp->bitmap.bmHeight;

  height = info->bmiHeader.biHeight;
  if (height < 0) height = -height;
  if( lines > height ) lines = height;
  /* Top-down images have a negative biHeight, the scanlines of theses images
   * were inverted in X11DRV_DIB_GetImageBits_xx
   * To prevent this we simply change the sign of lines
   * (the number of scan lines to copy).
   * Negative lines are correctly handled by X11DRV_DIB_GetImageBits_xx.
   */
  if( info->bmiHeader.biHeight < 0 && lines > 0) lines = -lines;

  if( startscan >= bmp->bitmap.bmHeight )
  {
      lines = 0;
      goto done;
  }

  if (DIB_GetBitmapInfo( &info->bmiHeader, &descr.infoWidth, &descr.lines,
                        &descr.infoBpp, &descr.compression ) == -1)
  {
      lines = 0;
      goto done;
  }

  switch (descr.infoBpp)
  {
      case 1:
      case 4:
      case 8:
          descr.rMask= descr.gMask = descr.bMask = 0;
          break;
      case 15:
      case 16:
          descr.rMask = (descr.compression == BI_BITFIELDS) ? *(DWORD *)info->bmiColors : 0x7c00;
          descr.gMask = (descr.compression == BI_BITFIELDS) ?  *((DWORD *)info->bmiColors + 1) : 0x03e0;
          descr.bMask = (descr.compression == BI_BITFIELDS) ?  *((DWORD *)info->bmiColors + 2) : 0x001f;
          break;
      case 24:
      case 32:
          descr.rMask = (descr.compression == BI_BITFIELDS) ? *(DWORD *)info->bmiColors : 0xff0000;
          descr.gMask = (descr.compression == BI_BITFIELDS) ?  *((DWORD *)info->bmiColors + 1) : 0x00ff00;
          descr.bMask = (descr.compression == BI_BITFIELDS) ?  *((DWORD *)info->bmiColors + 2) : 0x0000ff;
          break;
  }

  descr.physDev   = physDev;
  descr.palentry  = palette->logpalette.palPalEntry;
  descr.bits      = bits;
  descr.image     = NULL;
  descr.lines     = lines;
  descr.depth     = bmp->bitmap.bmBitsPixel;
  descr.drawable  = (Pixmap)bmp->physBitmap;
  descr.gc        = BITMAP_GC(bmp);
  descr.width     = bmp->bitmap.bmWidth;
  descr.height    = bmp->bitmap.bmHeight;
  descr.colorMap  = info->bmiColors;
  descr.xDest     = 0;
  descr.yDest     = 0;
  descr.xSrc      = 0;

  if (descr.lines > 0)
  {
     descr.ySrc = (descr.height-1) - (startscan + (lines-1));
  }
  else
  {
     descr.ySrc = startscan;
  }
#ifdef HAVE_LIBXXSHM
  descr.useShm = dib ? (dib->shminfo.shmid != -1) : FALSE;
#else
  descr.useShm = FALSE;
#endif
  descr.dibpitch = dib ? (dib->dibSection.dsBm.bmWidthBytes)
		       : (((descr.infoWidth * descr.infoBpp + 31) &~31) / 8);

  X11DRV_DIB_Lock(bmp, DIB_Status_GdiMod, FALSE);
  X11DRV_DIB_GetImageBits( &descr );
  X11DRV_DIB_Unlock(bmp, TRUE);

  if(info->bmiHeader.biSizeImage == 0) /* Fill in biSizeImage */
      info->bmiHeader.biSizeImage = DIB_GetDIBImageBytes(
					 info->bmiHeader.biWidth,
					 info->bmiHeader.biHeight,
					 info->bmiHeader.biBitCount );

  if (descr.compression == BI_BITFIELDS)
  {
    *(DWORD *)info->bmiColors = descr.rMask;
    *((DWORD *)info->bmiColors+1) = descr.gMask;
    *((DWORD *)info->bmiColors+2) = descr.bMask;
  }
  else
  {
    /* if RLE or JPEG compression were supported,
     * this line would be invalid. */
    info->bmiHeader.biCompression = 0;
  }

done:
  GDI_ReleaseObj( dc->hPalette );
  GDI_ReleaseObj( hbitmap );
  return lines;
}

/***********************************************************************
 *           DIB_DoProtectDIBSection
 */
static void X11DRV_DIB_DoProtectDIBSection( BITMAPOBJ *bmp, DWORD new_prot )
{
    DIBSECTION *dib = bmp->dib;
    INT effHeight = dib->dsBm.bmHeight >= 0? dib->dsBm.bmHeight
                                             : -dib->dsBm.bmHeight;
    /* use the biSizeImage data as the memory size only if we're dealing with a
       compressed image where the value is set.  Otherwise, calculate based on
       width * height */
    INT totalSize = dib->dsBmih.biSizeImage && dib->dsBmih.biCompression != BI_RGB
                         ? dib->dsBmih.biSizeImage
                         : dib->dsBm.bmWidthBytes * effHeight;
    DWORD old_prot;

    VirtualProtect(dib->dsBm.bmBits, totalSize, new_prot, &old_prot);
    TRACE("Changed protection from %ld to %ld\n", old_prot, new_prot);
}

/***********************************************************************
 *           X11DRV_DIB_DoUpdateDIBSection
 */
static void X11DRV_DIB_DoCopyDIBSection(BITMAPOBJ *bmp, BOOL toDIB,
					void *colorMap, int nColorMap,
					Drawable dest,
					DWORD xSrc, DWORD ySrc,
					DWORD xDest, DWORD yDest,
					DWORD width, DWORD height)
{
  X11DRV_DIBSECTION *dib = (X11DRV_DIBSECTION *) bmp->dib;
  X11DRV_DIB_IMAGEBITS_DESCR descr;

  if (DIB_GetBitmapInfo( &dib->dibSection.dsBmih, &descr.infoWidth, &descr.lines,
			 &descr.infoBpp, &descr.compression ) == -1)
    return;

  descr.physDev   = NULL;
  descr.palentry  = NULL;
  descr.image     = dib->image;
  descr.colorMap  = colorMap;
  descr.nColorMap = nColorMap;
  descr.bits      = dib->dibSection.dsBm.bmBits;
  descr.depth     = bmp->bitmap.bmBitsPixel;

  switch (descr.infoBpp)
  {
    case 1:
    case 4:
    case 8:
      descr.rMask = descr.gMask = descr.bMask = 0;
      break;
    case 15:
    case 16:
      descr.rMask = (descr.compression == BI_BITFIELDS) ? dib->dibSection.dsBitfields[0] : 0x7c00;
      descr.gMask = (descr.compression == BI_BITFIELDS) ? dib->dibSection.dsBitfields[1] : 0x03e0;
      descr.bMask = (descr.compression == BI_BITFIELDS) ? dib->dibSection.dsBitfields[2] : 0x001f;
      break;

    case 24:
    case 32:
      descr.rMask = (descr.compression == BI_BITFIELDS) ? dib->dibSection.dsBitfields[0] : 0xff0000;
      descr.gMask = (descr.compression == BI_BITFIELDS) ? dib->dibSection.dsBitfields[1] : 0x00ff00;
      descr.bMask = (descr.compression == BI_BITFIELDS) ? dib->dibSection.dsBitfields[2] : 0x0000ff;
      break;
  }

  /* Hack for now */
  descr.drawable  = dest;
  descr.gc        = BITMAP_GC(bmp);
  descr.xSrc      = xSrc;
  descr.ySrc      = ySrc;
  descr.xDest     = xDest;
  descr.yDest     = yDest;
  descr.width     = width;
  descr.height    = height;
#ifdef HAVE_LIBXXSHM
  descr.useShm = (dib->shminfo.shmid != -1);
#else
  descr.useShm = FALSE;
#endif
  descr.dibpitch = dib->dibSection.dsBm.bmWidthBytes;

  if (toDIB)
    {
      TRACE("Copying from Pixmap to DIB bits\n");
      X11DRV_DIB_GetImageBits( &descr );
    }
  else
    {
      TRACE("Copying from DIB bits to Pixmap\n");
      X11DRV_DIB_SetImageBits( &descr );
    }
}

/***********************************************************************
 *           X11DRV_DIB_CopyDIBSection
 */
void X11DRV_DIB_CopyDIBSection(X11DRV_PDEVICE *physDevSrc, X11DRV_PDEVICE *physDevDst,
                               DWORD xSrc, DWORD ySrc, DWORD xDest, DWORD yDest,
                               DWORD width, DWORD height)
{
  BITMAPOBJ *bmp;
  DC *dcSrc = physDevSrc->dc;
  DC *dcDst = physDevDst->dc;
  int nColorMap = 0, *colorMap = NULL, aColorMap = FALSE;

  TRACE("(%p,%p,%ld,%ld,%ld,%ld,%ld,%ld)\n", dcSrc, dcDst,
    xSrc, ySrc, xDest, yDest, width, height);
  /* this function is meant as an optimization for BitBlt,
   * not to be called otherwise */
  if (GetObjectType( physDevSrc->hdc ) != OBJ_MEMDC) {
    ERR("called for non-memory source DC!?\n");
    return;
  }

  bmp = (BITMAPOBJ *)GDI_GetObjPtr( dcSrc->hBitmap, BITMAP_MAGIC );
  if (!(bmp && bmp->dib)) {
    ERR("called for non-DIBSection!?\n");
    GDI_ReleaseObj( dcSrc->hBitmap );
    return;
  }
  /* while BitBlt should already have made sure we only get
   * positive values, we should check for oversize values */
  if ((xSrc < bmp->bitmap.bmWidth) &&
      (ySrc < bmp->bitmap.bmHeight)) {
    if (xSrc + width > bmp->bitmap.bmWidth)
      width = bmp->bitmap.bmWidth - xSrc;
    if (ySrc + height > bmp->bitmap.bmHeight)
      height = bmp->bitmap.bmHeight - ySrc;
    /* if the source bitmap is 8bpp or less, we're supposed to use the
     * DC's palette for color conversion (not the DIB color table) */
    if (bmp->dib->dsBm.bmBitsPixel <= 8) {
      X11DRV_DIBSECTION *dib = (X11DRV_DIBSECTION *) bmp->dib;
      if ((!dcSrc->hPalette) ||
	  (dcSrc->hPalette == GetStockObject(DEFAULT_PALETTE))) {
	/* HACK: no palette has been set in the source DC,
	 * use the DIB colormap instead - this is necessary in some
	 * cases since we need to do depth conversion in some places
	 * where real Windows can just copy data straight over */
	colorMap = dib->colorMap;
	nColorMap = dib->nColorMap;
      } else {
	colorMap = X11DRV_DIB_BuildColorMap( physDevSrc, (WORD)-1,
					     bmp->dib->dsBm.bmBitsPixel,
					     (BITMAPINFO*)&(bmp->dib->dsBmih),
					     &nColorMap );
	if (colorMap) aColorMap = TRUE;
      }
    }
    /* perform the copy */
    X11DRV_DIB_DoCopyDIBSection(bmp, FALSE, colorMap, nColorMap,
				physDevDst->drawable, xSrc, ySrc,
                                physDevDst->org.x + xDest, physDevDst->org.y + yDest,
				width, height);
    /* free color mapping */
    if (aColorMap)
      HeapFree(GetProcessHeap(), 0, colorMap);
  }
  GDI_ReleaseObj( dcSrc->hBitmap );
}

/***********************************************************************
 *           X11DRV_DIB_DoUpdateDIBSection
 */
static void X11DRV_DIB_DoUpdateDIBSection(BITMAPOBJ *bmp, BOOL toDIB)
{
  X11DRV_DIBSECTION *dib = (X11DRV_DIBSECTION *) bmp->dib;
  X11DRV_DIB_DoCopyDIBSection(bmp, toDIB, dib->colorMap, dib->nColorMap,
			      (Drawable)bmp->physBitmap, 0, 0, 0, 0,
			      bmp->bitmap.bmWidth, bmp->bitmap.bmHeight);
}

/***********************************************************************
 *           X11DRV_DIB_FaultHandler
 */
static BOOL X11DRV_DIB_FaultHandler( LPVOID res, LPCVOID addr )
{
  BITMAPOBJ *bmp;
  INT state;

  bmp = (BITMAPOBJ *)GDI_GetObjPtr( (HBITMAP)res, BITMAP_MAGIC );
  if (!bmp) return FALSE;

  state = X11DRV_DIB_Lock(bmp, DIB_Status_None, FALSE);
  if (state != DIB_Status_InSync) {
    /* no way to tell whether app needs read or write yet,
     * try read first */
    X11DRV_DIB_Coerce(bmp, DIB_Status_InSync, FALSE);
  } else {
    /* hm, apparently the app must have write access */
    X11DRV_DIB_Coerce(bmp, DIB_Status_AppMod, FALSE);
  }
  X11DRV_DIB_Unlock(bmp, TRUE);

  GDI_ReleaseObj( (HBITMAP)res );
  return TRUE;
}

/***********************************************************************
 *           X11DRV_DIB_Coerce
 */
INT X11DRV_DIB_Coerce(BITMAPOBJ *bmp, INT req, BOOL lossy)
{
  X11DRV_DIBSECTION *dib = (X11DRV_DIBSECTION *) bmp->dib;
  INT ret = DIB_Status_None;

  if (dib) {
    EnterCriticalSection(&(dib->lock));
    ret = dib->status;
    switch (req) {
    case DIB_Status_GdiMod:
      /* GDI access - request to draw on pixmap */
      switch (dib->status)
      {
        default:
        case DIB_Status_None:
	  dib->p_status = DIB_Status_GdiMod;
	  X11DRV_DIB_DoUpdateDIBSection( bmp, FALSE );
	  break;

        case DIB_Status_GdiMod:
	  TRACE("GdiMod requested in status GdiMod\n" );
	  break;

        case DIB_Status_InSync:
	  TRACE("GdiMod requested in status InSync\n" );
	  X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_NOACCESS );
	  dib->status = DIB_Status_GdiMod;
	  dib->p_status = DIB_Status_InSync;
	  break;

	case DIB_Status_AuxMod:
	  TRACE("GdiMod requested in status AuxMod\n" );
	  if (lossy) dib->status = DIB_Status_GdiMod;
	  else (*dib->copy_aux)(dib->aux_ctx, DIB_Status_GdiMod);
	  dib->p_status = DIB_Status_AuxMod;
	  if (dib->status != DIB_Status_AppMod) {
	    X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_NOACCESS );
	    break;
	  }
	  /* fall through if copy_aux() had to change to AppMod state */

        case DIB_Status_AppMod:
	  TRACE("GdiMod requested in status AppMod\n" );
	  if (!lossy) {
	    /* make it readonly to avoid app changing data while we copy */
	    X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READONLY );
	    X11DRV_DIB_DoUpdateDIBSection( bmp, FALSE );
	  }
	  X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_NOACCESS );
	  dib->p_status = DIB_Status_AppMod;
	  dib->status = DIB_Status_GdiMod;
	  break;
      }
      break;

    case DIB_Status_InSync:
      /* App access - request access to read DIB surface */
      /* (typically called from signal handler) */
      switch (dib->status)
      {
        default:
        case DIB_Status_None:
	  /* shouldn't happen from signal handler */
	  break;

	case DIB_Status_AuxMod:
	  TRACE("InSync requested in status AuxMod\n" );
	  if (lossy) dib->status = DIB_Status_InSync;
	  else {
	    X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READWRITE );
	    (*dib->copy_aux)(dib->aux_ctx, DIB_Status_InSync);
	  }
	  if (dib->status != DIB_Status_GdiMod) {
	    X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READONLY );
	    break;
	  }
	  /* fall through if copy_aux() had to change to GdiMod state */

	case DIB_Status_GdiMod:
	  TRACE("InSync requested in status GdiMod\n" );
	  if (!lossy) {
	    X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READWRITE );
	    X11DRV_DIB_DoUpdateDIBSection( bmp, TRUE );
	  }
	  X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READONLY );
	  dib->status = DIB_Status_InSync;
	  break;

        case DIB_Status_InSync:
	  TRACE("InSync requested in status InSync\n" );
	  /* shouldn't happen from signal handler */
	  break;

        case DIB_Status_AppMod:
	  TRACE("InSync requested in status AppMod\n" );
	  /* no reason to do anything here, and this
	   * shouldn't happen from signal handler */
	  break;
      }
      break;

    case DIB_Status_AppMod:
      /* App access - request access to write DIB surface */
      /* (typically called from signal handler) */
      switch (dib->status)
      {
        default:
        case DIB_Status_None:
	  /* shouldn't happen from signal handler */
	  break;

	case DIB_Status_AuxMod:
	  TRACE("AppMod requested in status AuxMod\n" );
	  X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READWRITE );
	  if (lossy) dib->status = DIB_Status_AppMod;
	  else (*dib->copy_aux)(dib->aux_ctx, DIB_Status_AppMod);
	  if (dib->status != DIB_Status_GdiMod)
	    break;
	  /* fall through if copy_aux() had to change to GdiMod state */

	case DIB_Status_GdiMod:
	  TRACE("AppMod requested in status GdiMod\n" );
	  X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READWRITE );
	  if (!lossy) X11DRV_DIB_DoUpdateDIBSection( bmp, TRUE );
	  dib->status = DIB_Status_AppMod;
	  break;

        case DIB_Status_InSync:
	  TRACE("AppMod requested in status InSync\n" );
	  X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READWRITE );
	  dib->status = DIB_Status_AppMod;
	  break;

        case DIB_Status_AppMod:
	  TRACE("AppMod requested in status AppMod\n" );
	  /* shouldn't happen from signal handler */
	  break;
      }
      break;

    case DIB_Status_AuxMod:
      if (dib->status == DIB_Status_None) {
	dib->p_status = req;
      } else {
	if (dib->status != DIB_Status_AuxMod)
	  dib->p_status = dib->status;
	dib->status = DIB_Status_AuxMod;
      }
      break;
      /* it is up to the caller to do the copy/conversion, probably
       * using the return value to decide where to copy from */
    }
    LeaveCriticalSection(&(dib->lock));
  }
  return ret;
}

/***********************************************************************
 *           X11DRV_DIB_Lock
 */
INT X11DRV_DIB_Lock(BITMAPOBJ *bmp, INT req, BOOL lossy)
{
  X11DRV_DIBSECTION *dib = (X11DRV_DIBSECTION *) bmp->dib;
  INT ret = DIB_Status_None;

  if (dib) {
    TRACE("Locking %p from thread %04lx\n", bmp, GetCurrentThreadId());
    EnterCriticalSection(&(dib->lock));
    ret = dib->status;
    if (req != DIB_Status_None)
      X11DRV_DIB_Coerce(bmp, req, lossy);
  }
  return ret;
}

/***********************************************************************
 *           X11DRV_DIB_Unlock
 */
void X11DRV_DIB_Unlock(BITMAPOBJ *bmp, BOOL commit)
{
  X11DRV_DIBSECTION *dib = (X11DRV_DIBSECTION *) bmp->dib;

  if (dib) {
    switch (dib->status)
    {
      default:
      case DIB_Status_None:
	/* in case anyone is wondering, this is the "signal handler doesn't
	 * work" case, where we always have to be ready for app access */
	if (commit) {
	  switch (dib->p_status)
	  {
	    case DIB_Status_AuxMod:
	      TRACE("Unlocking and syncing from AuxMod\n" );
	      (*dib->copy_aux)(dib->aux_ctx, DIB_Status_AppMod);
	      if (dib->status != DIB_Status_None) {
		dib->p_status = dib->status;
		dib->status = DIB_Status_None;
	      }
	      if (dib->p_status != DIB_Status_GdiMod)
		break;
	      /* fall through if copy_aux() had to change to GdiMod state */

	    case DIB_Status_GdiMod:
	      TRACE("Unlocking and syncing from GdiMod\n" );
	      X11DRV_DIB_DoUpdateDIBSection( bmp, TRUE );
	      break;

	    default:
	      TRACE("Unlocking without needing to sync\n" );
	      break;
	  }
	}
	else TRACE("Unlocking with no changes\n");
	dib->p_status = DIB_Status_None;
	break;

      case DIB_Status_GdiMod:
	TRACE("Unlocking in status GdiMod\n" );
	/* DIB was protected in Coerce */
	if (!commit) {
	  /* no commit, revert to InSync if applicable */
	  if ((dib->p_status == DIB_Status_InSync) ||
	      (dib->p_status == DIB_Status_AppMod)) {
	    X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READONLY );
	    dib->status = DIB_Status_InSync;
	  }
	}
	break;

      case DIB_Status_InSync:
	TRACE("Unlocking in status InSync\n" );
	/* DIB was already protected in Coerce */
	break;

      case DIB_Status_AppMod:
	TRACE("Unlocking in status AppMod\n" );
	/* DIB was already protected in Coerce */
	/* this case is ordinary only called from the signal handler,
	 * so we don't bother to check for !commit */
	break;

      case DIB_Status_AuxMod:
	TRACE("Unlocking in status AuxMod\n" );
	if (commit) {
	  /* DIB may need protection now */
	  if ((dib->p_status == DIB_Status_InSync) ||
	      (dib->p_status == DIB_Status_AppMod))
	    X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_NOACCESS );
	} else {
	  /* no commit, revert to previous state */
	  if (dib->p_status != DIB_Status_None)
	    dib->status = dib->p_status;
	  /* no protections changed */
	}
	dib->p_status = DIB_Status_None;
	break;
    }
    LeaveCriticalSection(&(dib->lock));
    TRACE("Unlocked %p\n", bmp);
  }
}

/***********************************************************************
 *           X11DRV_CoerceDIBSection2
 */
INT X11DRV_CoerceDIBSection2(HBITMAP hBmp, INT req, BOOL lossy)
{
  BITMAPOBJ *bmp;
  INT ret;

  bmp = (BITMAPOBJ *)GDI_GetObjPtr( hBmp, BITMAP_MAGIC );
  if (!bmp) return DIB_Status_None;
  ret = X11DRV_DIB_Coerce(bmp, req, lossy);
  GDI_ReleaseObj( hBmp );
  return ret;
}

/***********************************************************************
 *           X11DRV_LockDIBSection2
 */
INT X11DRV_LockDIBSection2(HBITMAP hBmp, INT req, BOOL lossy)
{
  BITMAPOBJ *bmp;
  INT ret;

  bmp = (BITMAPOBJ *)GDI_GetObjPtr( hBmp, BITMAP_MAGIC );
  if (!bmp) return DIB_Status_None;
  ret = X11DRV_DIB_Lock(bmp, req, lossy);
  GDI_ReleaseObj( hBmp );
  return ret;
}

/***********************************************************************
 *           X11DRV_UnlockDIBSection2
 */
void X11DRV_UnlockDIBSection2(HBITMAP hBmp, BOOL commit)
{
  BITMAPOBJ *bmp;

  bmp = (BITMAPOBJ *)GDI_GetObjPtr( hBmp, BITMAP_MAGIC );
  if (!bmp) return;
  X11DRV_DIB_Unlock(bmp, commit);
  GDI_ReleaseObj( hBmp );
}

/***********************************************************************
 *           X11DRV_CoerceDIBSection
 */
INT X11DRV_CoerceDIBSection(X11DRV_PDEVICE *physDev, INT req, BOOL lossy)
{
  if (!physDev) return DIB_Status_None;
  return X11DRV_CoerceDIBSection2( physDev->dc->hBitmap, req, lossy );
}

/***********************************************************************
 *           X11DRV_LockDIBSection
 */
INT X11DRV_LockDIBSection(X11DRV_PDEVICE *physDev, INT req, BOOL lossy)
{
  if (!physDev) return DIB_Status_None;
  if (GetObjectType( physDev->hdc ) != OBJ_MEMDC) return DIB_Status_None;

  return X11DRV_LockDIBSection2( physDev->dc->hBitmap, req, lossy );
}

/***********************************************************************
 *           X11DRV_UnlockDIBSection
 */
void X11DRV_UnlockDIBSection(X11DRV_PDEVICE *physDev, BOOL commit)
{
  if (!physDev) return;
  if (GetObjectType( physDev->hdc ) != OBJ_MEMDC) return;

  X11DRV_UnlockDIBSection2( physDev->dc->hBitmap, commit );
}


#ifdef HAVE_LIBXXSHM
/***********************************************************************
 *           X11DRV_XShmErrorHandler
 *
 */
static int XShmErrorHandler( Display *dpy, XErrorEvent *event, void *arg )
{
    return 1;  /* FIXME: should check event contents */
}

/***********************************************************************
 *           X11DRV_XShmCreateImage
 *
 */
static XImage *X11DRV_XShmCreateImage( int width, int height, int bpp,
                                       XShmSegmentInfo* shminfo)
{
    XImage *image;

    image = XShmCreateImage(gdi_display, visual, bpp, ZPixmap, NULL, shminfo, width, height);
    if (image)
    {
        shminfo->shmid = shmget(IPC_PRIVATE, image->bytes_per_line * height,
                                  IPC_CREAT|0700);
        if( shminfo->shmid != -1 )
        {
            shminfo->shmaddr = image->data = shmat(shminfo->shmid, 0, 0);
            if( shminfo->shmaddr != (char*)-1 )
            {
                BOOL ok;

                shminfo->readOnly = FALSE;
                X11DRV_expect_error( gdi_display, XShmErrorHandler, NULL );
                ok = (XShmAttach( gdi_display, shminfo ) != 0);
                XSync( gdi_display, False );
                if (X11DRV_check_error()) ok = FALSE;
                if (ok)
                {
                    shmctl(shminfo->shmid, IPC_RMID, 0);
                    return image; /* Success! */
                }
                /* An error occurred */
                shmdt(shminfo->shmaddr);
            }
            shmctl(shminfo->shmid, IPC_RMID, 0);
        }
        XFlush(gdi_display);
        XDestroyImage(image);
        image = NULL;
    }
    return image;
}
#endif /* HAVE_LIBXXSHM */


/***********************************************************************
 *           X11DRV_DIB_CreateDIBSection
 */
HBITMAP X11DRV_DIB_CreateDIBSection(
  X11DRV_PDEVICE *physDev, BITMAPINFO *bmi, UINT usage,
  LPVOID *bits, HANDLE section,
  DWORD offset, DWORD ovr_pitch)
{
  HBITMAP res = 0;
  BITMAPOBJ *bmp = NULL;
  X11DRV_DIBSECTION *dib = NULL;
  int *colorMap = NULL;
  int nColorMap;

  /* Fill BITMAP32 structure with DIB data */
  BITMAPINFOHEADER *bi = &bmi->bmiHeader;
  INT effHeight, totalSize;
  BITMAP bm;
  LPVOID mapBits = NULL;

  TRACE("format (%ld,%ld), planes %d, bpp %d, size %ld, colors %ld (%s)\n",
	bi->biWidth, bi->biHeight, bi->biPlanes, bi->biBitCount,
	bi->biSizeImage, bi->biClrUsed, usage == DIB_PAL_COLORS? "PAL" : "RGB");

  effHeight = bi->biHeight >= 0 ? bi->biHeight : -bi->biHeight;
  bm.bmType = 0;
  bm.bmWidth = bi->biWidth;
  bm.bmHeight = effHeight;
  bm.bmWidthBytes = ovr_pitch ? ovr_pitch
			      : DIB_GetDIBWidthBytes(bm.bmWidth, bi->biBitCount);
  bm.bmPlanes = bi->biPlanes;
  bm.bmBitsPixel = bi->biBitCount;
  bm.bmBits = NULL;

  /* Get storage location for DIB bits.  Only use biSizeImage if it's valid and
     we're dealing with a compressed bitmap.  Otherwise, use width * height. */
  totalSize = bi->biSizeImage && bi->biCompression != BI_RGB
    ? bi->biSizeImage : bm.bmWidthBytes * effHeight;

  if (section)
  {
      SYSTEM_INFO SystemInfo;
      DWORD mapOffset;
      INT mapSize;

      GetSystemInfo( &SystemInfo );
      mapOffset = offset - (offset % SystemInfo.dwAllocationGranularity);
      mapSize = totalSize + (offset - mapOffset);
      mapBits = MapViewOfFile( section,
			       FILE_MAP_ALL_ACCESS,
			       0L,
			       mapOffset,
			       mapSize );
      bm.bmBits = (char *)mapBits + (offset - mapOffset);
  }
  else if (ovr_pitch && offset)
    bm.bmBits = (LPVOID) offset;
  else {
    offset = 0;
    bm.bmBits = VirtualAlloc(NULL, totalSize,
			     MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  }

  /* Create Color Map */
  if (bm.bmBits && bm.bmBitsPixel <= 8)
      colorMap = X11DRV_DIB_BuildColorMap( usage == DIB_PAL_COLORS? physDev : NULL,
                                           usage, bm.bmBitsPixel, bmi, &nColorMap );

  /* Allocate Memory for DIB and fill structure */
  if (bm.bmBits)
    dib = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(X11DRV_DIBSECTION));
  if (dib)
    {
      dib->dibSection.dsBm = bm;
      dib->dibSection.dsBmih = *bi;
      dib->dibSection.dsBmih.biSizeImage = totalSize;

      /* Set dsBitfields values */
       if ( usage == DIB_PAL_COLORS || bi->biBitCount <= 8)
       {
           dib->dibSection.dsBitfields[0] = dib->dibSection.dsBitfields[1] = dib->dibSection.dsBitfields[2] = 0;
       }
       else switch( bi->biBitCount )
       {
           case 15:
           case 16:
               dib->dibSection.dsBitfields[0] = (bi->biCompression == BI_BITFIELDS) ? *(DWORD *)bmi->bmiColors : 0x7c00;
               dib->dibSection.dsBitfields[1] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 1) : 0x03e0;
               dib->dibSection.dsBitfields[2] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 2) : 0x001f;
               break;

           case 24:
           case 32:
               dib->dibSection.dsBitfields[0] = (bi->biCompression == BI_BITFIELDS) ? *(DWORD *)bmi->bmiColors       : 0xff0000;
               dib->dibSection.dsBitfields[1] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 1) : 0x00ff00;
               dib->dibSection.dsBitfields[2] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 2) : 0x0000ff;
               break;
       }
      dib->dibSection.dshSection = section;
      dib->dibSection.dsOffset = offset;

      dib->status    = DIB_Status_None;
      dib->nColorMap = nColorMap;
      dib->colorMap  = colorMap;
    }

  /* Create Device Dependent Bitmap and add DIB pointer */
  if (dib)
    {
      res = CreateDIBitmap(physDev->hdc, bi, 0, NULL, bmi, usage);
      if (res)
	{
	  bmp = (BITMAPOBJ *) GDI_GetObjPtr(res, BITMAP_MAGIC);
	  if (bmp) bmp->dib = (DIBSECTION *) dib;
	}
    }

  /* Create XImage */
  if (dib && bmp)
  {
      wine_tsx11_lock();
#ifdef HAVE_LIBXXSHM
      if (XShmQueryExtension(gdi_display) &&
          (dib->image = X11DRV_XShmCreateImage( bm.bmWidth, effHeight,
                                                bmp->bitmap.bmBitsPixel, &dib->shminfo )) )
      {
	; /* Created Image */
      } else {
          dib->image = X11DRV_DIB_CreateXImage( bm.bmWidth, effHeight, bmp->bitmap.bmBitsPixel );
          dib->shminfo.shmid = -1;
      }
#else
      dib->image = X11DRV_DIB_CreateXImage( bm.bmWidth, effHeight, bmp->bitmap.bmBitsPixel );
#endif
      wine_tsx11_unlock();
  }

  /* Clean up in case of errors */
  if (!res || !bmp || !dib || !bm.bmBits || (bm.bmBitsPixel <= 8 && !colorMap))
    {
      TRACE("got an error res=%p, bmp=%p, dib=%p, bm.bmBits=%p\n",
	    res, bmp, dib, bm.bmBits);
      if (bm.bmBits)
        {
	  if (section)
	    UnmapViewOfFile(mapBits), bm.bmBits = NULL;
	  else if (!offset)
	    VirtualFree(bm.bmBits, 0L, MEM_RELEASE), bm.bmBits = NULL;
        }

      if (dib && dib->image) { XDestroyImage(dib->image); dib->image = NULL; }
      if (colorMap) { HeapFree(GetProcessHeap(), 0, colorMap); colorMap = NULL; }
      if (dib) { HeapFree(GetProcessHeap(), 0, dib); dib = NULL; }
      if (bmp) { GDI_ReleaseObj(res); bmp = NULL; }
      if (res) { DeleteObject(res); res = 0; }
    }
  else if (bm.bmBits)
    {
      extern BOOL VIRTUAL_SetFaultHandler(LPCVOID addr, BOOL (*proc)(LPVOID, LPCVOID), LPVOID arg);
      /* Install fault handler, if possible */
      InitializeCriticalSection(&(dib->lock));
      if (VIRTUAL_SetFaultHandler(bm.bmBits, X11DRV_DIB_FaultHandler, (LPVOID)res))
        {
          X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READWRITE );
          if (dib) dib->status = DIB_Status_AppMod;
        }
    }

  /* Return BITMAP handle and storage location */
  if (bmp) GDI_ReleaseObj(res);
  if (bm.bmBits && bits) *bits = bm.bmBits;
  return res;
}

/***********************************************************************
 *           X11DRV_DIB_DeleteDIBSection
 */
void X11DRV_DIB_DeleteDIBSection(BITMAPOBJ *bmp)
{
  X11DRV_DIBSECTION *dib = (X11DRV_DIBSECTION *) bmp->dib;

  if (dib->image)
  {
      wine_tsx11_lock();
#ifdef HAVE_LIBXXSHM
      if (dib->shminfo.shmid != -1)
      {
          XShmDetach (gdi_display, &(dib->shminfo));
          XDestroyImage (dib->image);
          shmdt (dib->shminfo.shmaddr);
          dib->shminfo.shmid = -1;
      }
      else
#endif
          XDestroyImage( dib->image );
      wine_tsx11_unlock();
  }

  if (dib->colorMap)
    HeapFree(GetProcessHeap(), 0, dib->colorMap);

  DeleteCriticalSection(&(dib->lock));
}

/***********************************************************************
 *           SetDIBColorTable   (X11DRV.@)
 */
UINT X11DRV_SetDIBColorTable( X11DRV_PDEVICE *physDev, UINT start, UINT count, const RGBQUAD *colors )
{
    BITMAPOBJ * bmp;
    X11DRV_DIBSECTION *dib;
    UINT ret = 0;

    if (!(bmp = (BITMAPOBJ*)GDI_GetObjPtr( physDev->dc->hBitmap, BITMAP_MAGIC ))) return 0;
    dib = (X11DRV_DIBSECTION *) bmp->dib;

    if (dib && dib->colorMap) {
        UINT end = count + start;
        if (end > dib->nColorMap) end = dib->nColorMap;
        /*
         * Changing color table might change the mapping between
         * DIB colors and X11 colors and thus alter the visible state
         * of the bitmap object.
         */
        X11DRV_DIB_Lock(bmp, DIB_Status_AppMod, FALSE);
        X11DRV_DIB_GenColorMap( physDev, dib->colorMap, DIB_RGB_COLORS,
                                dib->dibSection.dsBm.bmBitsPixel,
                                TRUE, colors, start, end );
        X11DRV_DIB_Unlock(bmp, TRUE);
        ret = end - start;
    }
    GDI_ReleaseObj( physDev->dc->hBitmap );
    return ret;
}

/***********************************************************************
 *           GetDIBColorTable   (X11DRV.@)
 */
UINT X11DRV_GetDIBColorTable( X11DRV_PDEVICE *physDev, UINT start, UINT count, RGBQUAD *colors )
{
    BITMAPOBJ * bmp;
    X11DRV_DIBSECTION *dib;
    UINT ret = 0;

    if (!(bmp = (BITMAPOBJ*)GDI_GetObjPtr( physDev->dc->hBitmap, BITMAP_MAGIC ))) return 0;
    dib = (X11DRV_DIBSECTION *) bmp->dib;

    if (dib && dib->colorMap) {
        UINT i, end = count + start;
        if (end > dib->nColorMap) end = dib->nColorMap;
        for (i = start; i < end; i++,colors++) {
            COLORREF col = X11DRV_PALETTE_ToLogical( dib->colorMap[i] );
            colors->rgbBlue  = GetBValue(col);
            colors->rgbGreen = GetGValue(col);
            colors->rgbRed   = GetRValue(col);
            colors->rgbReserved = 0;
        }
        ret = end-start;
    }
    GDI_ReleaseObj( physDev->dc->hBitmap );
    return ret;
}


/**************************************************************************
 *	        X11DRV_DIB_CreateDIBFromPixmap
 *
 *  Allocates a packed DIB and copies the Pixmap data into it.
 *  If bDeletePixmap is TRUE, the Pixmap passed in is deleted after the conversion.
 */
HGLOBAL X11DRV_DIB_CreateDIBFromPixmap(Pixmap pixmap, HDC hdc, BOOL bDeletePixmap)
{
    HBITMAP hBmp = 0;
    BITMAPOBJ *pBmp = NULL;
    HGLOBAL hPackedDIB = 0;

    /* Allocates an HBITMAP which references the Pixmap passed to us */
    hBmp = X11DRV_BITMAP_CreateBitmapHeaderFromPixmap(pixmap);
    if (!hBmp)
    {
        TRACE("\tCould not create bitmap header for Pixmap\n");
        goto END;
    }

    /*
     * Create a packed DIB from the Pixmap wrapper bitmap created above.
     * A packed DIB contains a BITMAPINFO structure followed immediately by
     * an optional color palette and the pixel data.
     */
    hPackedDIB = DIB_CreateDIBFromBitmap(hdc, hBmp);

    /* Get a pointer to the BITMAPOBJ structure */
    pBmp = (BITMAPOBJ *)GDI_GetObjPtr( hBmp, BITMAP_MAGIC );

    /* We can now get rid of the HBITMAP wrapper we created earlier.
     * Note: Simply calling DeleteObject will free the embedded Pixmap as well.
     */
    if (!bDeletePixmap)
    {
        /* Clear the physBitmap to prevent the Pixmap from being deleted by DeleteObject */
        pBmp->physBitmap = NULL;
        pBmp->funcs = NULL;
    }
    GDI_ReleaseObj( hBmp );
    DeleteObject(hBmp);

END:
    TRACE("\tReturning packed DIB %p\n", hPackedDIB);
    return hPackedDIB;
}


/**************************************************************************
 *	           X11DRV_DIB_CreatePixmapFromDIB
 *
 *    Creates a Pixmap from a packed DIB
 */
Pixmap X11DRV_DIB_CreatePixmapFromDIB( HGLOBAL hPackedDIB, HDC hdc )
{
    Pixmap pixmap = None;
    HBITMAP hBmp = 0;
    BITMAPOBJ *pBmp = NULL;
    LPBYTE pPackedDIB = NULL;
    LPBITMAPINFO pbmi = NULL;
    LPBITMAPINFOHEADER pbmiHeader = NULL;
    LPBYTE pbits = NULL;

    /* Get a pointer to the packed DIB's data  */
    pPackedDIB = (LPBYTE)GlobalLock(hPackedDIB);
    pbmiHeader = (LPBITMAPINFOHEADER)pPackedDIB;
    pbmi = (LPBITMAPINFO)pPackedDIB;
    pbits = (LPBYTE)(pPackedDIB
                     + DIB_BitmapInfoSize( (LPBITMAPINFO)pbmiHeader, DIB_RGB_COLORS ));

    /* Create a DDB from the DIB */

    hBmp = CreateDIBitmap(hdc,
                          pbmiHeader,
                          CBM_INIT,
                          (LPVOID)pbits,
                          pbmi,
                          DIB_RGB_COLORS);

    GlobalUnlock(hPackedDIB);

    TRACE("CreateDIBitmap returned %p\n", hBmp);

    /* Retrieve the internal Pixmap from the DDB */

    pBmp = (BITMAPOBJ *) GDI_GetObjPtr( hBmp, BITMAP_MAGIC );

    pixmap = (Pixmap)pBmp->physBitmap;
    /* clear the physBitmap so that we can steal its pixmap */
    pBmp->physBitmap = NULL;
    pBmp->funcs = NULL;

    /* Delete the DDB we created earlier now that we have stolen its pixmap */
    GDI_ReleaseObj( hBmp );
    DeleteObject(hBmp);

    TRACE("\tReturning Pixmap %ld\n", pixmap);
    return pixmap;
}
