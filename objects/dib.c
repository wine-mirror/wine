/*
 * GDI device-independent bitmaps
 *
 * Copyright 1993,1994  Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "dc.h"
#include "bitmap.h"
#include "callback.h"
#include "palette.h"
#include "stddebug.h"
#include "color.h"
#include "debug.h"

extern void CLIPPING_UpdateGCRegion(DC* );

/***********************************************************************
 *           DIB_GetImageWidthBytes
 *
 * Return the width of an X image in bytes
 */
int DIB_GetImageWidthBytes( int width, int depth )
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
        	fprintf(stderr, "DIB: unsupported depth %d.\n", depth );
	/* fall through */
	case 32:
	        words = width;
    }
    return 4 * words;
}


/***********************************************************************
 *           DIB_BitmapInfoSize
 *
 * Return the size of the bitmap info structure.
 */
int DIB_BitmapInfoSize( BITMAPINFO * info, WORD coloruse )
{
    int colors;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)info;
        colors = (core->bcBitCount != 24) ? 1 << core->bcBitCount : 0;
        return sizeof(BITMAPCOREHEADER) + colors *
             ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
    }
    else  /* assume BITMAPINFOHEADER */
    {
        colors = info->bmiHeader.biClrUsed;
        if (!colors && (info->bmiHeader.biBitCount != 24))
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
static int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, DWORD *width,
                              DWORD *height, WORD *bpp )
{
    if (header->biSize == sizeof(BITMAPINFOHEADER))
    {
        *width  = header->biWidth;
        *height = header->biHeight;
        *bpp    = header->biBitCount;
        return 1;
    }
    if (header->biSize == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)header;
        *width  = core->bcWidth;
        *height = core->bcHeight;
        *bpp    = core->bcBitCount;
        return 0;
    }
    fprintf( stderr, "DIB_GetBitmapInfo: wrong size (%ld) for header\n",
             header->biSize );
    return -1;
}


/***********************************************************************
 *           DIB_BuildColorMap
 *
 * Build the color map from the bitmap palette. Should not be called
 * for a 24-bit deep bitmap.
 */
static int *DIB_BuildColorMap( DC *dc, WORD coloruse, WORD depth,
                               BITMAPINFO *info )
{
    int i, colors;
    BOOL32 isInfo;
    WORD *colorPtr;
    int *colorMapping;

    if ((isInfo = (info->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))))
    {
        colors = info->bmiHeader.biClrUsed;
        if (!colors) colors = 1 << info->bmiHeader.biBitCount;
        colorPtr = (WORD *)info->bmiColors;
    }
    else  /* assume BITMAPCOREINFO */
    {
        colors = 1 << ((BITMAPCOREHEADER *)&info->bmiHeader)->bcBitCount;
        colorPtr = (WORD *)((BITMAPCOREINFO *)info)->bmciColors;
    }
    if (!(colorMapping = (int *)malloc( colors * sizeof(int) ))) return NULL;

    if (coloruse == DIB_RGB_COLORS)
    {
        if (isInfo)
        {
            RGBQUAD * rgb = (RGBQUAD *)colorPtr;
        
            if (depth == 1)  /* Monochrome */
                for (i = 0; i < colors; i++, rgb++)
                    colorMapping[i] = (rgb->rgbRed + rgb->rgbGreen + 
                                       rgb->rgbBlue > 255*3/2);
            else
                for (i = 0; i < colors; i++, rgb++)
                    colorMapping[i] = COLOR_ToPhysical( dc, RGB(rgb->rgbRed,
                                                                rgb->rgbGreen,
                                                                rgb->rgbBlue));
        }
        else
        {
            RGBTRIPLE * rgb = (RGBTRIPLE *)colorPtr;
        
            if (depth == 1)  /* Monochrome */
                for (i = 0; i < colors; i++, rgb++)
                    colorMapping[i] = (rgb->rgbtRed + rgb->rgbtGreen + 
                                       rgb->rgbtBlue > 255*3/2);
            else
                for (i = 0; i < colors; i++, rgb++)
                    colorMapping[i] = COLOR_ToPhysical( dc, RGB(rgb->rgbtRed,
                                                               rgb->rgbtGreen,
                                                               rgb->rgbtBlue));
        }
    }
    else  /* DIB_PAL_COLORS */
    {
        for (i = 0; i < colors; i++, colorPtr++)
            colorMapping[i] = COLOR_ToPhysical( dc, PALETTEINDEX(*colorPtr) );
    }
    return colorMapping;
}


/***********************************************************************
 *           DIB_SetImageBits_1
 *
 * SetDIBits for a 1-bit deep DIB.
 */
static void DIB_SetImageBits_1( DWORD lines, BYTE *bits, DWORD width,
                                int *colors, XImage *bmpImage )
{
    DWORD i, x;
    BYTE pad, pix;

    if (!(width & 31)) pad = 0;
    else pad = ((32 - (width & 31)) + 7) / 8;

    while (lines--)
    {
	for (i = width/8, x = 0; (i > 0); i--)
	{
	    pix = *bits++;
	    XPutPixel( bmpImage, x++, lines, colors[pix >> 7] );
	    XPutPixel( bmpImage, x++, lines, colors[(pix >> 6) & 1] );
	    XPutPixel( bmpImage, x++, lines, colors[(pix >> 5) & 1] );
	    XPutPixel( bmpImage, x++, lines, colors[(pix >> 4) & 1] );
	    XPutPixel( bmpImage, x++, lines, colors[(pix >> 3) & 1] );
	    XPutPixel( bmpImage, x++, lines, colors[(pix >> 2) & 1] );
	    XPutPixel( bmpImage, x++, lines, colors[(pix >> 1) & 1] );
	    XPutPixel( bmpImage, x++, lines, colors[pix & 1] );
	}
	pix = *bits;
	switch(width & 7)
	{
	case 7: XPutPixel( bmpImage, x++, lines, colors[pix >> 7] ); pix <<= 1;
	case 6: XPutPixel( bmpImage, x++, lines, colors[pix >> 7] ); pix <<= 1;
	case 5: XPutPixel( bmpImage, x++, lines, colors[pix >> 7] ); pix <<= 1;
	case 4: XPutPixel( bmpImage, x++, lines, colors[pix >> 7] ); pix <<= 1;
	case 3: XPutPixel( bmpImage, x++, lines, colors[pix >> 7] ); pix <<= 1;
	case 2: XPutPixel( bmpImage, x++, lines, colors[pix >> 7] ); pix <<= 1;
	case 1: XPutPixel( bmpImage, x++, lines, colors[pix >> 7] );
	}
	bits += pad;
    }
}


/***********************************************************************
 *           DIB_SetImageBits_4
 *
 * SetDIBits for a 4-bit deep DIB.
 */
static void DIB_SetImageBits_4( DWORD lines, BYTE *bits, DWORD width,
                                int *colors, XImage *bmpImage )
{
    DWORD i, x;
    BYTE pad;

    if (!(width & 7)) pad = 0;
    else pad = ((8 - (width & 7)) + 1) / 2;

    while (lines--)
    {
	for (i = width/2, x = 0; i > 0; i--)
	{
	    BYTE pix = *bits++;
	    XPutPixel( bmpImage, x++, lines, colors[pix >> 4] );
	    XPutPixel( bmpImage, x++, lines, colors[pix & 0x0f] );
	}
	if (width & 1) XPutPixel( bmpImage, x, lines, colors[*bits >> 4] );
	bits += pad;
    }
}

#define check_xy(x,y) \
	if (x > width) { \
		x = 0; \
		if (lines) \
			lines--; \
	}

/***********************************************************************
 *           DIB_SetImageBits_RLE4
 *
 * SetDIBits for a 4-bit deep compressed DIB.
 */
static void DIB_SetImageBits_RLE4( DWORD lines, BYTE *bits, DWORD width,
                                   int *colors, XImage *bmpImage )
{
	int x = 0, c, length;
	BYTE *begin = bits;

        lines--;
	while ((int)lines >= 0)
        {
		length = *bits++;
		if (length) {	/* encoded */
			c = *bits++;
			while (length--) {
				XPutPixel(bmpImage, x++, lines, colors[c >> 4]);
				check_xy(x, y);
				if (length) {
					length--;
					XPutPixel(bmpImage, x++, lines, colors[c & 0xf]);
					check_xy(x, y);
				}
			}
		} else {
			length = *bits++;
			switch (length) {
				case 0: /* eol */
					x = 0;
					lines--;
					continue;

				case 1: /* eopicture */
					return;

				case 2:	/* delta */
					x += *bits++;
					lines -= *bits++;
					continue;

				default: /* absolute */
					while (length--) {
						c = *bits++;
						XPutPixel(bmpImage, x++, lines, colors[c >> 4]);
						check_xy(x, y);
						if (length) {
							length--;
							XPutPixel(bmpImage, x++, lines, colors[c & 0xf]);
							check_xy(x, y);
						}
					}
					if ((bits - begin) & 1)
						bits++;
			}
		}
	}
}

/***********************************************************************
 *           DIB_SetImageBits_8
 *
 * SetDIBits for an 8-bit deep DIB.
 */
static void DIB_SetImageBits_8( DWORD lines, BYTE *bits, DWORD width,
                                int *colors, XImage *bmpImage )
{
    DWORD x;
    BYTE pad = (4 - (width & 3)) & 3;

    while (lines--)
    {
	for (x = 0; x < width; x++)
	    XPutPixel( bmpImage, x, lines, colors[*bits++] );
	bits += pad;
    }
}

/***********************************************************************
 *	      DIB_SetImageBits_RLE8
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

enum Rle8_EscapeCodes		
{
  /* 
   * Apologies for polluting your file's namespace...
   */
  RleEol 	= 0,		/* End of line */
  RleEnd 	= 1,		/* End of bitmap */
  RleDelta	= 2		/* Delta */
};
  
static void DIB_SetImageBits_RLE8( DWORD lines, BYTE *bits, DWORD width,
                                   int *colors, XImage *bmpImage )
{
    int x;			/* X-positon on each line.  Increases. */
    int line;			/* Line #.  Starts at lines-1, decreases */
    BYTE *pIn = bits;		/* Pointer to current position in bits */
    BYTE length;		/* The length pf a run */
    BYTE color_index;		/* index into colors[] as read from bits */
    BYTE escape_code;		/* See enum Rle8_EscapeCodes.*/
    WORD color;			/* value of colour[color_index] */
    
    if (lines == 0)		/* Let's hope this doesn't happen. */
      return;
    
    /*
     * Note that the bitmap data is stored by Windows starting at the
     * bottom line of the bitmap and going upwards.  Within each line,
     * the data is stored left-to-right.  That's the reason why line
     * goes from lines-1 to 0.			[JAY]
     */
    
    x = 0;
    line = lines-1;
    do
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
		color_index = (*pIn++); /* Get the colour index. */
		color = colors[color_index];

		while(length--)
		  XPutPixel(bmpImage, x++, line, color);
	    }
	  else 
	    {    
		/* 
		 * Escape codes (may be an absolute sequence though)
		 */
		escape_code = (*pIn++);
		switch(escape_code)
		  {
		    case RleEol: /* =0, end of line */
		      {
			  x = 0;  
			  line--;  
			  break;
		      }
		      
		    case RleEnd: /* =1, end of bitmap */
		      {
			  /*
			   * Not all RLE8 bitmaps end with this 
			   * code.  For example, Paint Shop Pro 
			   * produces some that don't.  That's (I think)
			   * what caused the previous implementation to 
			   * fail.			[JAY]
			   */
			  line=-1; /* Cause exit from do loop. */
			  break;
		      }
		      
		    case RleDelta: /* =2, a delta */
		      {
			  /* 
			   * Note that deltaing to line 0 
			   * will cause an exit from the loop, 
			   * which may not be what is intended. 
			   * The fact that there is a delta in the bits
			   * almost certainly implies that there is data
			   * to follow.  You may feel that we should 
			   * jump to the top of the loop to avoid exiting
			   * in this case.  
			   *
			   * TODO: Decide what to do here in that case. [JAY]
			   */
			  x 	+= (*pIn++); 
			  line 	-= (*pIn++);
			  if (line == 0)
			    {
			      dprintf_bitmap(stddeb, 
					     "DIB_SetImageBits_RLE8(): "
					     "Delta to last line of bitmap "
					     "(wrongly?) causes loop exit\n");
			    }
			  break;
		      }
		      
		    default:	/* >2, switch to absolute mode */
		      {
			  /* 
			   * Absolute Mode 
			   */
			  length = escape_code;
			  while(length--)
			    {
				color_index = (*pIn++);
				XPutPixel(bmpImage, x++, line, 
					  colors[color_index]);
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
			  if (escape_code & 1) 
			    pIn++; /* Throw away the pad byte. */
			  break;
		      }
		  } /* switch (escape_code) : Escape sequence */
	    }  /* process either an encoded sequence or an escape sequence */
	  
	  /* We expect to come here more than once per line. */
      } while (line >= 0);  /* Do this until the bitmap is filled */
    
    /*
     * Everybody comes here at the end.
     * Check how we exited the loop and print a message if it's a bit odd.
     *						[JAY]
     */
    if ( (*(pIn-2) != 0/*escape*/) || (*(pIn-1)!= RleEnd) )
      {
	dprintf_bitmap(stddeb, "DIB_SetImageBits_RLE8(): End-of-bitmap "
		       "without (strictly) proper escape code.  Last two "
		       "bytes were: %02X %02X.\n",
		       (int)*(pIn-2),
		       (int)*(pIn-1));		 
      }
}  


/***********************************************************************
 *           DIB_SetImageBits_24
 *
 * SetDIBits for a 24-bit deep DIB.
 */
static void DIB_SetImageBits_24( DWORD lines, BYTE *bits, DWORD width,
				 DC *dc, XImage *bmpImage )
{
    DWORD x;
    BYTE pad = (4 - ((width*3) & 3)) & 3;

    /* "bits" order is reversed for some reason */

    while (lines--)
    {
	for (x = 0; x < width; x++, bits += 3)
	    XPutPixel( bmpImage, x, lines, 
		       COLOR_ToPhysical(dc, RGB(bits[2],bits[1],bits[0])) );

	bits += pad;
    }
}


/***********************************************************************
 *           DIB_SetImageBits
 *
 * Transfer the bits to an X image.
 * Helper function for SetDIBits() and SetDIBitsToDevice().
 */
static int DIB_SetImageBits( DC *dc, DWORD lines, WORD depth, LPSTR bits,
                             DWORD infoWidth, WORD infoBpp,
                             BITMAPINFO *info, WORD coloruse,
			     Drawable drawable, GC gc, int xSrc, int ySrc,
			     int xDest, int yDest, int width, int height )
{
    int *colorMapping;
    XImage *bmpImage;
    DWORD compression = 0;

    if (info->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
        compression = info->bmiHeader.biCompression;

      /* Build the color mapping table */

    if (infoBpp == 24) colorMapping = NULL;
    else
        if (!(colorMapping = DIB_BuildColorMap( dc, coloruse, depth, info )))
            return 0;

    if( dc->w.flags & DC_DIRTY ) CLIPPING_UpdateGCRegion(dc);

      /* Transfer the pixels */
    XCREATEIMAGE(bmpImage, infoWidth, lines, depth );

    switch(infoBpp)
    {
    case 1:
	DIB_SetImageBits_1( lines, bits, infoWidth,
			    colorMapping, bmpImage );
	break;
    case 4:
	if (compression) DIB_SetImageBits_RLE4( lines, bits, infoWidth,
                                                colorMapping, bmpImage );
	else DIB_SetImageBits_4( lines, bits, infoWidth,
                                 colorMapping, bmpImage );
	break;
    case 8:
	if (compression) DIB_SetImageBits_RLE8( lines, bits, infoWidth,
                                                colorMapping, bmpImage );
	else DIB_SetImageBits_8( lines, bits, infoWidth,
                                 colorMapping, bmpImage );
	break;
    case 24:
	DIB_SetImageBits_24( lines, bits, infoWidth, dc, bmpImage );
	break;
    default:
        fprintf( stderr, "Invalid depth %d for SetDIBits!\n", infoBpp );
        break;
    }
    if (colorMapping) free(colorMapping);
    XPutImage( display, drawable, gc, bmpImage, xSrc, ySrc,
	       xDest, yDest, width, height );
    XDestroyImage( bmpImage );
    return lines;
}


/***********************************************************************
 *           StretchDIBits16   (GDI.439)
 */
INT16 StretchDIBits16( HDC16 hdc, INT16 xDst, INT16 yDst, INT16 widthDst,
                       INT16 heightDst, INT16 xSrc, INT16 ySrc, INT16 widthSrc,
                       INT16 heightSrc, const VOID *bits,
                       const BITMAPINFO *info, UINT16 wUsage, DWORD dwRop )
{
    return (INT16)StretchDIBits32( hdc, xDst, yDst, widthDst, heightDst,
                                   xSrc, ySrc, widthSrc, heightSrc, bits,
                                   info, wUsage, dwRop );
}


/***********************************************************************
 *           StretchDIBits32   (GDI32.351)
 */
INT32 StretchDIBits32( HDC32 hdc, INT32 xDst, INT32 yDst, INT32 widthDst,
                       INT32 heightDst, INT32 xSrc, INT32 ySrc, INT32 widthSrc,
                       INT32 heightSrc, const void *bits,
                       const BITMAPINFO *info, UINT32 wUsage, DWORD dwRop )
{
    HBITMAP32 hBitmap, hOldBitmap;
    HDC32 hdcMem;

    hBitmap = CreateDIBitmap32( hdc, &info->bmiHeader, CBM_INIT,
                                bits, info, wUsage );
    hdcMem = CreateCompatibleDC32( hdc );
    hOldBitmap = SelectObject32( hdcMem, hBitmap );
    StretchBlt32( hdc, xDst, yDst, widthDst, heightDst,
                  hdcMem, xSrc, ySrc, widthSrc, heightSrc, dwRop );
    SelectObject32( hdcMem, hOldBitmap );
    DeleteDC32( hdcMem );
    DeleteObject32( hBitmap );
    return heightSrc;
}


/***********************************************************************
 *           SetDIBits16    (GDI.440)
 */
INT16 SetDIBits16( HDC16 hdc, HBITMAP16 hbitmap, UINT16 startscan,
                   UINT16 lines, LPCVOID bits, const BITMAPINFO *info,
                   UINT16 coloruse )
{
    return SetDIBits32( hdc, hbitmap, startscan, lines, bits, info, coloruse );
}


/***********************************************************************
 *           SetDIBits32   (GDI32.312)
 */
INT32 SetDIBits32( HDC32 hdc, HBITMAP32 hbitmap, UINT32 startscan,
                   UINT32 lines, LPCVOID bits, const BITMAPINFO *info,
                   UINT32 coloruse )
{
    DC * dc;
    BITMAPOBJ * bmp;
    DWORD width, height;
    WORD bpp;

      /* Check parameters */

    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }
    if (!(bmp = (BITMAPOBJ *)GDI_GetObjPtr( hbitmap, BITMAP_MAGIC )))
	return 0;
    if (DIB_GetBitmapInfo( &info->bmiHeader, &width, &height, &bpp ) == -1)
        return 0;
    if (!lines || (startscan >= (WORD)height)) return 0;
    if (startscan + lines > height) lines = height - startscan;

    return CallTo32_LargeStack( (int(*)())DIB_SetImageBits, 16,
                                dc, lines, bmp->bitmap.bmBitsPixel,
                                bits, width, bpp, info,
                                coloruse, bmp->pixmap, BITMAP_GC(bmp), 0, 0, 0,
                                height - startscan - lines,
                                bmp->bitmap.bmWidth, lines );
}


/***********************************************************************
 *           SetDIBitsToDevice16    (GDI.443)
 */
INT16 SetDIBitsToDevice16( HDC16 hdc, INT16 xDest, INT16 yDest, INT16 cx,
                           INT16 cy, INT16 xSrc, INT16 ySrc, UINT16 startscan,
                           UINT16 lines, LPCVOID bits, const BITMAPINFO *info,
                           UINT16 coloruse )
{
    return SetDIBitsToDevice32( hdc, xDest, yDest, cx, cy, xSrc, ySrc,
                                startscan, lines, bits, info, coloruse );
}


/***********************************************************************
 *           SetDIBitsToDevice32   (GDI32.313)
 */
INT32 SetDIBitsToDevice32( HDC32 hdc, INT32 xDest, INT32 yDest, DWORD cx,
                           DWORD cy, INT32 xSrc, INT32 ySrc, UINT32 startscan,
                           UINT32 lines, LPCVOID bits, const BITMAPINFO *info,
                           UINT32 coloruse )
{
    DC * dc;
    DWORD width, height;
    WORD bpp;

      /* Check parameters */

    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }
    if (DIB_GetBitmapInfo( &info->bmiHeader, &width, &height, &bpp ) == -1)
        return 0;
    if (!lines || (startscan >= height)) return 0;
    if (startscan + lines > height) lines = height - startscan;
    if (ySrc < startscan) ySrc = startscan;
    else if (ySrc >= startscan + lines) return 0;
    if (xSrc >= width) return 0;
    if (ySrc + cy >= startscan + lines) cy = startscan + lines - ySrc;
    if (xSrc + cx >= width) cx = width - xSrc;
    if (!cx || !cy) return 0;

    DC_SetupGCForText( dc );  /* To have the correct colors */
    XSetFunction( display, dc->u.x.gc, DC_XROPfunction[dc->w.ROPmode-1] );
    return CallTo32_LargeStack( (int(*)())DIB_SetImageBits, 16,
                                dc, lines, dc->w.bitsPerPixel, bits, width,
                                bpp, info, coloruse,
                                dc->u.x.drawable, dc->u.x.gc,
                                xSrc, ySrc - startscan,
                                dc->w.DCOrgX + XLPTODP( dc, xDest ),
                                dc->w.DCOrgY + YLPTODP( dc, yDest ),
                                cx, cy );
}



/***********************************************************************
 *           GetDIBits16    (GDI.441)
 */
INT16 GetDIBits16( HDC16 hdc, HBITMAP16 hbitmap, UINT16 startscan,
                   UINT16 lines, LPSTR bits, BITMAPINFO * info,
                   UINT16 coloruse )
{
    return GetDIBits32( hdc, hbitmap, startscan, lines, bits, info, coloruse );
}


/***********************************************************************
 *           GetDIBits32    (GDI32.170)
 */
INT32 GetDIBits32( HDC32 hdc, HBITMAP32 hbitmap, UINT32 startscan,
                   UINT32 lines, LPSTR bits, BITMAPINFO * info,
                   UINT32 coloruse )
{
    DC * dc;
    BITMAPOBJ * bmp;
    PALETTEENTRY * palEntry;
    PALETTEOBJ * palette;
    XImage * bmpImage, * dibImage;
    int i, x, y;
        
    if (!lines) return 0;
    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }
    if (!(bmp = (BITMAPOBJ *)GDI_GetObjPtr( hbitmap, BITMAP_MAGIC )))
	return 0;
    if (!(palette = (PALETTEOBJ*)GDI_GetObjPtr( dc->w.hPalette, PALETTE_MAGIC )))
	return 0;

      /* Transfer color info */
    
    palEntry = palette->logpalette.palPalEntry;
    for (i = 0; i < info->bmiHeader.biClrUsed; i++, palEntry++)
    {
	if (coloruse == DIB_RGB_COLORS)
	{
	    info->bmiColors[i].rgbRed      = palEntry->peRed;
	    info->bmiColors[i].rgbGreen    = palEntry->peGreen;
	    info->bmiColors[i].rgbBlue     = palEntry->peBlue;
	    info->bmiColors[i].rgbReserved = 0;
	}
	else ((WORD *)info->bmiColors)[i] = (WORD)i;
    }
    
    if (bits)
    {	
	BYTE*	bbits = bits;
	int	pad, yend, xend = bmp->bitmap.bmWidth;

        dprintf_bitmap(stddeb, "GetDIBits: %u scanlines of (%i,%i) -> (%i,%i) starting from %u\n",
			    lines, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
			    (int)info->bmiHeader.biWidth, (int)info->bmiHeader.biHeight, startscan );

	/* adjust number of scanlines to copy */

	if( lines > info->bmiHeader.biHeight ) lines = info->bmiHeader.biHeight;
	yend = startscan + lines;
	if( startscan >= bmp->bitmap.bmHeight ) 
	    return FALSE;
	if( yend > bmp->bitmap.bmHeight ) yend = bmp->bitmap.bmHeight;

	/* adjust scanline width */

	pad = info->bmiHeader.biWidth - bmp->bitmap.bmWidth;
	if( pad < 0 ) 
	{
	    /* bitmap is wider than DIB, copy only a part */

	    pad = 0; xend = info->bmiHeader.biWidth;
	}

	bmpImage = (XImage *)CallTo32_LargeStack( (int (*)())XGetImage, 8, 
		               display, bmp->pixmap, 0, 0, bmp->bitmap.bmWidth,
		               bmp->bitmap.bmHeight, AllPlanes, ZPixmap );

	switch( info->bmiHeader.biBitCount )
	{
	   case 8:

		pad += (4 - (info->bmiHeader.biWidth & 3)) & 3;
		for( y = yend - 1; (int)y >= (int)startscan; y-- )
		{
		   for( x = 0; x < xend; x++ )
			*bbits++ = XGetPixel( bmpImage, x, y );
		   bbits += pad;
		}
		break;

/* add more bpp-specific shortcuts here */

	   default:

		dibImage = XCreateImage(display, DefaultVisualOfScreen( screen ),
			   info->bmiHeader.biBitCount, ZPixmap, 0, bits,
			   info->bmiHeader.biWidth, info->bmiHeader.biHeight,
			   32, DIB_GetImageWidthBytes( info->bmiHeader.biWidth,
                                                info->bmiHeader.biBitCount ) );
		if( dibImage )
		{
		    extern void _XInitImageFuncPtrs( XImage* );

		    dibImage->byte_order = MSBFirst;
		    dibImage->bitmap_bit_order = MSBFirst;
		    dibImage->bitmap_unit = 16;
		    _XInitImageFuncPtrs( dibImage );

		    for (y = yend - 1; (int)y >= (int)startscan; y--)
		        for (x = 0; x < xend; x++)
			    XPutPixel( dibImage, x, yend - y + 1, 
				       XGetPixel( bmpImage, x, y ));
		    dibImage->data = NULL;
		    XDestroyImage( dibImage );
		}
	}

	XDestroyImage( bmpImage );

	info->bmiHeader.biCompression = 0;
    }
    else if( info->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER) ) 
    {
	/* fill in struct members */
	
	info->bmiHeader.biWidth = bmp->bitmap.bmWidth;
	info->bmiHeader.biHeight = bmp->bitmap.bmHeight;
	info->bmiHeader.biPlanes = 1;
	info->bmiHeader.biBitCount = bmp->bitmap.bmBitsPixel;
	info->bmiHeader.biSizeImage = bmp->bitmap.bmHeight *
                             DIB_GetImageWidthBytes( bmp->bitmap.bmWidth,
                                                     bmp->bitmap.bmBitsPixel );
	info->bmiHeader.biCompression = 0;
    }

    return lines;
}


/***********************************************************************
 *           CreateDIBitmap16    (GDI.442)
 */
HBITMAP16 CreateDIBitmap16( HDC16 hdc, const BITMAPINFOHEADER * header,
                            DWORD init, LPCVOID bits, const BITMAPINFO * data,
                            UINT16 coloruse )
{
    return CreateDIBitmap32( hdc, header, init, bits, data, coloruse );
}


/***********************************************************************
 *           CreateDIBitmap32    (GDI32.37)
 */
HBITMAP32 CreateDIBitmap32( HDC32 hdc, const BITMAPINFOHEADER *header,
                            DWORD init, LPCVOID bits, const BITMAPINFO *data,
                            UINT32 coloruse )
{
    HBITMAP32 handle;
    BOOL32 fColor;
    DWORD width, height;
    WORD bpp;

    if (DIB_GetBitmapInfo( header, &width, &height, &bpp ) == -1) return 0;

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
            fprintf( stderr, "CreateDIBitmap: wrong size (%ld) for data\n",
                     data->bmiHeader.biSize );
            return 0;
        }
    }

    /* Now create the bitmap */

    handle = fColor ? CreateCompatibleBitmap32( hdc, width, height ) :
                      CreateBitmap32( width, height, 1, 1, NULL );
    if (!handle) return 0;

    if (init == CBM_INIT)
        SetDIBits32( hdc, handle, 0, height, bits, data, coloruse );
    return handle;
}
