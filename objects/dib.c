/*
 * GDI device-independent bitmaps
 *
 * Copyright 1993,1994  Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include "ts_xlib.h"
#include "ts_xutil.h"
#include "dc.h"
#include "bitmap.h"
#include "callback.h"
#include "palette.h"
#include "color.h"
#include "debug.h"

extern void CLIPPING_UpdateGCRegion(DC* );

static int bitmapDepthTable[] = { 8, 1, 32, 16, 24, 15, 4, 0 };
static int ximageDepthTable[] = { 0, 0, 0,  0,  0,  0,  0 };


/* This structure holds the arguments for DIB_SetImageBits() */
typedef struct
{
    DC               *dc;
    LPCVOID           bits;
    int               lines;
    DWORD             infoWidth;
    WORD              depth;
    WORD              infoBpp;
    const BITMAPINFO *info;
    WORD              coloruse;
    Drawable          drawable;
    GC                gc;
    int               xSrc;
    int               ySrc;
    int               xDest;
    int               yDest;
    int               width;
    int               height;
} DIB_SETIMAGEBITS_DESCR;


/***********************************************************************
 *           DIB_Init
 */
BOOL32 DIB_Init(void)
{
    int		i;
    XImage*	testimage;

    for( i = 0; bitmapDepthTable[i]; i++ )
    {
	 testimage = TSXCreateImage(display, DefaultVisualOfScreen(screen),
			 bitmapDepthTable[i], ZPixmap, 0, NULL, 1, 1, 32, 20 );
	 if( testimage ) ximageDepthTable[i] = testimage->bits_per_pixel;
	 else return FALSE;
	 TSXDestroyImage(testimage);
    }
    return TRUE;
}

/***********************************************************************
 *           DIB_GetXImageWidthBytes
 *
 * Return the width of an X image in bytes
 */
int DIB_GetXImageWidthBytes( int width, int depth )
{
    int		i;

    if (!ximageDepthTable[0]) {
	    DIB_Init();
    }
    for( i = 0; bitmapDepthTable[i] ; i++ )
	 if( bitmapDepthTable[i] == depth )
	     return (4 * ((width * ximageDepthTable[i] + 31)/32));
    
    WARN(bitmap, "(%d): Unsupported depth\n", depth );
    return (4 * width);
}

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
static int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, DWORD *width,
                              int *height, WORD *bpp )
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
    WARN(bitmap, "(%ld): wrong size for header\n", header->biSize );
    return -1;
}


/***********************************************************************
 *           DIB_BuildColorMap
 *
 * Build the color map from the bitmap palette. Should not be called
 * for a 24-bit deep bitmap.
 */
static int *DIB_BuildColorMap( DC *dc, WORD coloruse, WORD depth,
                               const BITMAPINFO *info )
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
    if (!(colorMapping = (int *)HeapAlloc(GetProcessHeap(), 0,
                                          colors * sizeof(int) ))) return NULL;

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
 *           DIB_SetImageBits_1_Line
 *
 * Handles a single line of 1 bit data.
 */
static void DIB_SetImageBits_1_Line(DWORD dstwidth, int left, int *colors,
				    XImage *bmpImage, int h, const BYTE *bits)
{
    BYTE pix;
    DWORD i, x;

    dstwidth += left; bits += left >> 3;

    /* FIXME: should avoid putting x<left pixels (minor speed issue) */
    for (i = dstwidth/8, x = left&~7; (i > 0); i--)
    {
	pix = *bits++;
	XPutPixel( bmpImage, x++, h, colors[pix >> 7] );
	XPutPixel( bmpImage, x++, h, colors[(pix >> 6) & 1] );
	XPutPixel( bmpImage, x++, h, colors[(pix >> 5) & 1] );
	XPutPixel( bmpImage, x++, h, colors[(pix >> 4) & 1] );
	XPutPixel( bmpImage, x++, h, colors[(pix >> 3) & 1] );
	XPutPixel( bmpImage, x++, h, colors[(pix >> 2) & 1] );
	XPutPixel( bmpImage, x++, h, colors[(pix >> 1) & 1] );
	XPutPixel( bmpImage, x++, h, colors[pix & 1] );
    }
    pix = *bits;
    switch(dstwidth & 7)
    {
    case 7: XPutPixel( bmpImage, x++, h, colors[pix >> 7] ); pix <<= 1;
    case 6: XPutPixel( bmpImage, x++, h, colors[pix >> 7] ); pix <<= 1;
    case 5: XPutPixel( bmpImage, x++, h, colors[pix >> 7] ); pix <<= 1;
    case 4: XPutPixel( bmpImage, x++, h, colors[pix >> 7] ); pix <<= 1;
    case 3: XPutPixel( bmpImage, x++, h, colors[pix >> 7] ); pix <<= 1;
    case 2: XPutPixel( bmpImage, x++, h, colors[pix >> 7] ); pix <<= 1;
    case 1: XPutPixel( bmpImage, x++, h, colors[pix >> 7] );
    }
}

/***********************************************************************
 *           DIB_SetImageBits_1
 *
 * SetDIBits for a 1-bit deep DIB.
 */
static void DIB_SetImageBits_1( int lines, const BYTE *srcbits,
                                DWORD srcwidth, DWORD dstwidth, int left,
                                int *colors, XImage *bmpImage )
{
    int h;

    /* 32 bit aligned */
    DWORD linebytes = ((srcwidth + 31) & ~31) / 8;

    if (lines > 0) {
	for (h = lines-1; h >=0; h--) {
	    DIB_SetImageBits_1_Line(dstwidth, left, colors, bmpImage, h, srcbits);
	    srcbits += linebytes;
	}
    } else {
	lines = -lines;
	for (h = 0; h < lines; h++) {
	    DIB_SetImageBits_1_Line(dstwidth, left, colors, bmpImage, h, srcbits);
	    srcbits += linebytes;
	}
    }
}


/***********************************************************************
 *           DIB_SetImageBits_4
 *
 * SetDIBits for a 4-bit deep DIB.
 */
static void DIB_SetImageBits_4( int lines, const BYTE *srcbits,
                                DWORD srcwidth, DWORD dstwidth, int left,
                                int *colors, XImage *bmpImage )
{
    DWORD i, x;
    int h;
    const BYTE *bits = srcbits + (left >> 1);
  
    /* 32 bit aligned */
    DWORD linebytes = ((srcwidth+7)&~7)/2;

    dstwidth += left;

    /* FIXME: should avoid putting x<left pixels (minor speed issue) */
    if (lines > 0) {
	for (h = lines-1; h >= 0; h--) {
	    for (i = dstwidth/2, x = left&~1; i > 0; i--) {
		BYTE pix = *bits++;
		XPutPixel( bmpImage, x++, h, colors[pix >> 4] );
		XPutPixel( bmpImage, x++, h, colors[pix & 0x0f] );
	    }
	    if (dstwidth & 1) XPutPixel( bmpImage, x, h, colors[*bits >> 4] );
	    srcbits += linebytes;
	    bits	 = srcbits + (left >> 1);
	}
    } else {
	lines = -lines;
	for (h = 0; h < lines; h++) {
	    for (i = dstwidth/2, x = left&~1; i > 0; i--) {
		BYTE pix = *bits++;
		XPutPixel( bmpImage, x++, h, colors[pix >> 4] );
		XPutPixel( bmpImage, x++, h, colors[pix & 0x0f] );
	    }
	    if (dstwidth & 1) XPutPixel( bmpImage, x, h, colors[*bits >> 4] );
	    srcbits += linebytes;
	    bits	 = srcbits + (left >> 1);
	}
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
static void DIB_SetImageBits_RLE4( int lines, const BYTE *bits, DWORD width,
                                DWORD dstwidth, int left, int *colors, XImage *bmpImage )
{
	int x = 0, c, length;
	const BYTE *begin = bits;

        dstwidth += left; /* FIXME: avoid putting x<left pixels */

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
static void DIB_SetImageBits_8( int lines, const BYTE *srcbits,
				DWORD srcwidth, DWORD dstwidth, int left,
                                int *colors, XImage *bmpImage )
{
    DWORD x;
    int h;
    const BYTE *bits = srcbits + left;

    /* align to 32 bit */
    DWORD linebytes = (srcwidth + 3) & ~3;

    dstwidth+=left;

    if (lines > 0) {
	for (h = lines - 1; h >= 0; h--) {
	    for (x = left; x < dstwidth; x++, bits++) {
		XPutPixel( bmpImage, x, h, colors[*bits] );
	    }
	    bits = (srcbits += linebytes) + left;
	}
    } else {
	lines = -lines;
	for (h = 0; h < lines; h++) {
	    for (x = left; x < dstwidth; x++, bits++) {
		XPutPixel( bmpImage, x, h, colors[*bits] );
	    }
	    bits = (srcbits += linebytes) + left;
	}
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
  
static void DIB_SetImageBits_RLE8( int lines, const BYTE *bits, DWORD width,
                                DWORD dstwidth, int left, int *colors, XImage *bmpImage )
{
    int x;			/* X-positon on each line.  Increases. */
    int line;			/* Line #.  Starts at lines-1, decreases */
    const BYTE *pIn = bits;     /* Pointer to current position in bits */
    BYTE length;		/* The length pf a run */
    BYTE color_index;		/* index into colors[] as read from bits */
    BYTE escape_code;		/* See enum Rle8_EscapeCodes.*/
    WORD color;			/* value of colour[color_index] */
    
    if (lines == 0)		/* Let's hope this doesn't happen. */
      return;
    
    dstwidth += left; /* FIXME: avoid putting x<left pixels */

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
			      TRACE(bitmap, "Delta to last line of bitmap "
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
	TRACE(bitmap, "End-of-bitmap "
		       "without (strictly) proper escape code.  Last two "
		       "bytes were: %02X %02X.\n",
		       (int)*(pIn-2),
		       (int)*(pIn-1));		 
      }
}  


/***********************************************************************
 *           DIB_SetImageBits_16
 *
 * SetDIBits for a 16-bit deep DIB.
 */
static void DIB_SetImageBits_16( int lines, const BYTE *srcbits,
                                 DWORD srcwidth, DWORD dstwidth, int left,
				 DC *dc, XImage *bmpImage )
{
    DWORD x;
    LPWORD ptr;
    WORD val;
    int h;
    BYTE r, g, b;
  
    /* align to 32 bit */
    DWORD linebytes = (srcwidth * 2 + 3) & ~3;

    dstwidth += left;

    ptr = (LPWORD) srcbits + left;
    if (lines > 0) {
	for (h = lines - 1; h >= 0; h--) {
	    for (x = left; x < dstwidth; x++, ptr++) {
		val = *ptr;
		r = (BYTE) ((val & 0x7c00) >> 7);
		g = (BYTE) ((val & 0x03e0) >> 2);
		b = (BYTE) ((val & 0x001f) << 3);
		XPutPixel( bmpImage, x, h,
			   COLOR_ToPhysical(dc, RGB(r,g,b)) );
	    }
	    ptr = (LPWORD) (srcbits += linebytes) + left;
	}
    } else {
	lines = -lines;
	for (h = 0; h < lines; h++) {
	    for (x = left; x < dstwidth; x++, ptr++) {
		val = *ptr;
		r = (BYTE) ((val & 0x7c00) >> 7);
		g = (BYTE) ((val & 0x03e0) >> 2);
		b = (BYTE) ((val & 0x001f) << 3);
		XPutPixel( bmpImage, x, h,
			   COLOR_ToPhysical(dc, RGB(r,g,b)) );
	    }
	    ptr = (LPWORD) (srcbits += linebytes) + left;
	}
    }
}


/***********************************************************************
 *           DIB_SetImageBits_24
 *
 * SetDIBits for a 24-bit deep DIB.
 */
static void DIB_SetImageBits_24( int lines, const BYTE *srcbits,
                                 DWORD srcwidth, DWORD dstwidth, int left,
				 DC *dc, XImage *bmpImage )
{
    DWORD x;
    const BYTE *bits = srcbits + left * 3;
    int h;
  
    /* align to 32 bit */
    DWORD linebytes = (srcwidth * 3 + 3) & ~3;

    dstwidth += left;

    /* "bits" order is reversed for some reason */

    if (lines > 0) {
	for (h = lines - 1; h >= 0; h--) {
	    for (x = left; x < dstwidth; x++, bits += 3) {
		XPutPixel( bmpImage, x, h, 
			   COLOR_ToPhysical(dc, RGB(bits[2],bits[1],bits[0])));
	    }
	    bits = (srcbits += linebytes) + left * 3;
	}
    } else {
	lines = -lines;
	for (h = 0; h < lines; h++) {
	    for (x = left; x < dstwidth; x++, bits += 3) {
		XPutPixel( bmpImage, x, h,
			   COLOR_ToPhysical(dc, RGB(bits[2],bits[1],bits[0])));
	    }
	    bits = (srcbits += linebytes) + left * 3;
	}
    }
}


/***********************************************************************
 *           DIB_SetImageBits_32
 *
 * SetDIBits for a 32-bit deep DIB.
 */
static void DIB_SetImageBits_32( int lines, const BYTE *srcbits,
                                 DWORD srcwidth, DWORD dstwidth, int left,
				 DC *dc, XImage *bmpImage )
{
    DWORD x;
    const BYTE *bits = srcbits + left * 4;
    int h;
  
    DWORD linebytes = (srcwidth * 4);

    dstwidth += left;

    if (lines > 0) {
	for (h = lines - 1; h >= 0; h--) {
	    for (x = left; x < dstwidth; x++, bits += 4) {
		XPutPixel( bmpImage, x, h, 
			   COLOR_ToPhysical(dc, RGB(bits[2],bits[1],bits[0])));
	    }
	    bits = (srcbits += linebytes) + left * 4;
	}
    } else {
	lines = -lines;
	for (h = 0; h < lines; h++) {
	    for (x = left; x < dstwidth; x++, bits += 4) {
		XPutPixel( bmpImage, x, h,
			   COLOR_ToPhysical(dc, RGB(bits[2],bits[1],bits[0])));
	    }
	    bits = (srcbits += linebytes) + left * 4;
	}
    }
}


/***********************************************************************
 *           DIB_SetImageBits
 *
 * Transfer the bits to an X image.
 * Helper function for SetDIBits() and SetDIBitsToDevice().
 * The Xlib critical section must be entered before calling this function.
 */
static int DIB_SetImageBits( const DIB_SETIMAGEBITS_DESCR *descr )
{
    int *colorMapping;
    XImage *bmpImage;
    DWORD compression = 0;
    int lines;

    if (descr->info->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
        compression = descr->info->bmiHeader.biCompression;

      /* Build the color mapping table */

    if (descr->infoBpp > 8) colorMapping = NULL;
    else if (!(colorMapping = DIB_BuildColorMap( descr->dc, descr->coloruse,
                                                 descr->depth, descr->info )))
        return 0;

    if( descr->dc->w.flags & DC_DIRTY ) CLIPPING_UpdateGCRegion(descr->dc);

      /* Transfer the pixels */
    lines = descr->lines;
    if (lines < 0) lines = -lines;
    XCREATEIMAGE(bmpImage, descr->infoWidth, lines, descr->depth );

    switch(descr->infoBpp)
    {
    case 1:
	DIB_SetImageBits_1( descr->lines, descr->bits, descr->infoWidth,
			    descr->width, descr->xSrc, colorMapping, bmpImage );
	break;
    case 4:
	if (compression) DIB_SetImageBits_RLE4( descr->lines, descr->bits,
                                                descr->infoWidth, descr->width, descr->xSrc,
                                                colorMapping, bmpImage );
	else DIB_SetImageBits_4( descr->lines, descr->bits, descr->infoWidth,
                                 descr->width, descr->xSrc, colorMapping, bmpImage );
	break;
    case 8:
	if (compression) DIB_SetImageBits_RLE8( descr->lines, descr->bits,
                                                descr->infoWidth, descr->width, descr->xSrc,
                                                colorMapping, bmpImage );
	else DIB_SetImageBits_8( descr->lines, descr->bits, descr->infoWidth,
                                 descr->width, descr->xSrc, colorMapping, bmpImage );
	break;
    case 15:
    case 16:
	DIB_SetImageBits_16( descr->lines, descr->bits, descr->infoWidth,
			     descr->width, descr->xSrc, descr->dc, bmpImage);
	break;
    case 24:
	DIB_SetImageBits_24( descr->lines, descr->bits, descr->infoWidth,
                             descr->width, descr->xSrc, descr->dc, bmpImage );
	break;
    case 32:
	DIB_SetImageBits_32( descr->lines, descr->bits, descr->infoWidth,
                             descr->width, descr->xSrc, descr->dc, bmpImage);
	break;
    default:
        WARN(bitmap, "(%d): Invalid depth\n", descr->infoBpp );
        break;
    }
    if (colorMapping) HeapFree( GetProcessHeap(), 0, colorMapping );
    XPutImage( display, descr->drawable, descr->gc, bmpImage,
               descr->xSrc, descr->ySrc, descr->xDest, descr->yDest,
               descr->width, descr->height );
    XDestroyImage( bmpImage );
    return lines;
}


/***********************************************************************
 *           StretchDIBits16   (GDI.439)
 */
INT16 WINAPI StretchDIBits16(HDC16 hdc, INT16 xDst, INT16 yDst, INT16 widthDst,
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
INT32 WINAPI StretchDIBits32(HDC32 hdc, INT32 xDst, INT32 yDst, INT32 widthDst,
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
INT16 WINAPI SetDIBits16( HDC16 hdc, HBITMAP16 hbitmap, UINT16 startscan,
                          UINT16 lines, LPCVOID bits, const BITMAPINFO *info,
                          UINT16 coloruse )
{
    return SetDIBits32( hdc, hbitmap, startscan, lines, bits, info, coloruse );
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
INT32 WINAPI SetDIBits32( HDC32 hdc, HBITMAP32 hbitmap, UINT32 startscan,
                          UINT32 lines, LPCVOID bits, const BITMAPINFO *info,
                          UINT32 coloruse )
{
    DIB_SETIMAGEBITS_DESCR descr;
    BITMAPOBJ * bmp;
    int height, tmpheight;
    INT32 result;

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
                           &descr.infoBpp ) == -1)
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

    descr.bits      = bits;
    descr.lines     = tmpheight >= 0 ? lines : -lines;
    descr.depth     = bmp->bitmap.bmBitsPixel;
    descr.info      = info;
    descr.coloruse  = coloruse;
    descr.drawable  = bmp->pixmap;
    descr.gc        = BITMAP_GC(bmp);
    descr.xSrc      = 0;
    descr.ySrc      = 0;
    descr.xDest     = 0;
    descr.yDest     = height - startscan - lines;
    descr.width     = bmp->bitmap.bmWidth;
    descr.height    = lines;

    EnterCriticalSection( &X11DRV_CritSection );
    result = CALL_LARGE_STACK( DIB_SetImageBits, &descr );
    LeaveCriticalSection( &X11DRV_CritSection );

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
    return SetDIBitsToDevice32( hdc, xDest, yDest, cx, cy, xSrc, ySrc,
                                startscan, lines, bits, info, coloruse );
}


/***********************************************************************
 *           SetDIBitsToDevice32   (GDI32.313)
 */
INT32 WINAPI SetDIBitsToDevice32(HDC32 hdc, INT32 xDest, INT32 yDest, DWORD cx,
                           DWORD cy, INT32 xSrc, INT32 ySrc, UINT32 startscan,
                           UINT32 lines, LPCVOID bits, const BITMAPINFO *info,
                           UINT32 coloruse )
{
    DIB_SETIMAGEBITS_DESCR descr;
    DC * dc;
    DWORD width, oldcy = cy;
    INT32 result;
    int height, tmpheight;

      /* Check parameters */

    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }
    if (DIB_GetBitmapInfo( &info->bmiHeader, &width,
                           &height, &descr.infoBpp ) == -1)
        return 0;
    tmpheight = height;
    if (height < 0) height = -height;
    if (!lines || (startscan >= height)) return 0;
    if (startscan + lines > height) lines = height - startscan;
    if (ySrc < startscan) ySrc = startscan;
    else if (ySrc >= startscan + lines) return 0;
    if (xSrc >= width) return 0;
    if (ySrc + cy >= startscan + lines) cy = startscan + lines - ySrc;
    if (xSrc + cx >= width) cx = width - xSrc;
    if (!cx || !cy) return 0;

    DC_SetupGCForText( dc );  /* To have the correct colors */
    TSXSetFunction( display, dc->u.x.gc, DC_XROPfunction[dc->w.ROPmode-1] );

    descr.dc        = dc;
    descr.bits      = bits;
    descr.lines     = tmpheight >= 0 ? lines : -lines;
    descr.infoWidth = width;
    descr.depth     = dc->w.bitsPerPixel;
    descr.info      = info;
    descr.coloruse  = coloruse;
    descr.drawable  = dc->u.x.drawable;
    descr.gc        = dc->u.x.gc;
    descr.xSrc      = xSrc;
    descr.ySrc      = tmpheight >= 0 ? lines-(ySrc-startscan)-cy+(oldcy-cy) : ySrc - startscan;
    descr.xDest     = dc->w.DCOrgX + XLPTODP( dc, xDest );
    descr.yDest     = dc->w.DCOrgY + YLPTODP( dc, yDest ) + (tmpheight >= 0 ? oldcy-cy : 0);
    descr.width     = cx;
    descr.height    = cy;

    EnterCriticalSection( &X11DRV_CritSection );
    result = CALL_LARGE_STACK( DIB_SetImageBits, &descr );
    LeaveCriticalSection( &X11DRV_CritSection );
    return result;
}

/***********************************************************************
 *           SetDIBColorTable32    (GDI32.311)
 */
UINT32 WINAPI SetDIBColorTable32( HDC32 hdc, UINT32 startpos, UINT32 entries,
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
 *           GetDIBColorTable32    (GDI32.169)
 */
UINT32 WINAPI GetDIBColorTable32( HDC32 hdc, UINT32 startpos, UINT32 entries,
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

/***********************************************************************
 *           GetDIBits16    (GDI.441)
 */
INT16 WINAPI GetDIBits16( HDC16 hdc, HBITMAP16 hbitmap, UINT16 startscan,
                          UINT16 lines, LPSTR bits, BITMAPINFO * info,
                          UINT16 coloruse )
{
    return GetDIBits32( hdc, hbitmap, startscan, lines, bits, info, coloruse );
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
INT32 WINAPI GetDIBits32(
    HDC32 hdc,         /* [in]  Handle to device context */
    HBITMAP32 hbitmap, /* [in]  Handle to bitmap */
    UINT32 startscan,  /* [in]  First scan line to set in dest bitmap */
    UINT32 lines,      /* [in]  Number of scan lines to copy */
    LPSTR bits,        /* [out] Address of array for bitmap bits */
    BITMAPINFO * info, /* [out] Address of structure with bitmap data */
    UINT32 coloruse)   /* [in]  RGB or palette index */
{
    DC * dc;
    BITMAPOBJ * bmp;
    PALETTEENTRY * palEntry;
    PALETTEOBJ * palette;
    XImage * bmpImage;
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
    {
        GDI_HEAP_UNLOCK( hbitmap );
        return 0;
    }

      /* Transfer color info */
    
    if (info->bmiHeader.biBitCount<=8) {
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
    }
    
    if (bits)
    {	
	BYTE*	bbits = bits;
	int	pad, yend, xend = bmp->bitmap.bmWidth;

        TRACE(bitmap, "%u scanlines of (%i,%i) -> (%i,%i) starting from %u\n",
			    lines, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
			    (int)info->bmiHeader.biWidth, (int)info->bmiHeader.biHeight, startscan );

	/* adjust number of scanlines to copy */

	if( lines > info->bmiHeader.biHeight ) lines = info->bmiHeader.biHeight;
	yend = startscan + lines;
	if( startscan >= bmp->bitmap.bmHeight ) 
        {
            GDI_HEAP_UNLOCK( hbitmap );
            GDI_HEAP_UNLOCK( dc->w.hPalette );
            return FALSE;
        }
	if( yend > bmp->bitmap.bmHeight ) yend = bmp->bitmap.bmHeight;

	/* adjust scanline width */

	pad = info->bmiHeader.biWidth - bmp->bitmap.bmWidth;
	if( pad < 0 ) 
	{
	    /* bitmap is wider than DIB, copy only a part */

	    pad = 0;
            xend = info->bmiHeader.biWidth;
	}

        EnterCriticalSection( &X11DRV_CritSection );
	bmpImage = (XImage *)CALL_LARGE_STACK( BITMAP_GetXImage, bmp );

	switch( info->bmiHeader.biBitCount )
	{
	   case 8:
		/* pad up to 32 bit */
		pad += (4 - (info->bmiHeader.biWidth & 3)) & 3;
		for( y = yend - 1; (int)y >= (int)startscan; y-- )
		{
		   for( x = 0; x < xend; x++ )
			*bbits++ = XGetPixel( bmpImage, x, y );
		   bbits += pad;
		}
		break;
	   case 1:
	   	pad += ((32 - (info->bmiHeader.biWidth & 31)) / 8) & 3;
		for( y = yend - 1; (int)y >= (int)startscan; y-- )
		{
		   *bbits = 0;
		   for( x = 0; x < xend; x++ ) {
		   	
			*bbits |= XGetPixel( bmpImage, x, y)<<(7-(x&7));
			if ((x&7)==7) {
			    bbits++;
			    *bbits=0;
			}
		   }
		   bbits += pad;
		}
		break;
	   case 4:
	   	pad += ((8 - (info->bmiHeader.biWidth & 7)) / 2) & 3;
		for( y = yend - 1; (int)y >= (int)startscan; y-- )
		{
		   *bbits = 0;
		   for( x = 0; x < xend; x++ ) {
		   	
			*bbits |= XGetPixel( bmpImage, x, y)<<(4*(1-(x&1)));
			if ((x&1)==1) {
			    bbits++;
			    *bbits=0;
			}
		   }
		   bbits += pad;
		}
		break;
	   case 15:
           case 16:
	   	pad += (4 - ((info->bmiHeader.biWidth*2) & 3)) & 3;
		for( y = yend - 1; (int)y >= (int)startscan; y-- )
		{
		   *bbits = 0;
		   for( x = 0; x < xend; x++ ) {
		   	unsigned long pixel=XGetPixel( bmpImage, x, y);
			*bbits++ = pixel & 0xff;
			*bbits++ = (pixel >> 8) & 0xff;
		   }
		   bbits += pad;
		}
		break;
	   case 24:
	   	pad += (4 - ((info->bmiHeader.biWidth*3) & 3)) & 3;
		for( y = yend - 1; (int)y >= (int)startscan; y-- )
		{
		   *bbits = 0;
		   for( x = 0; x < xend; x++ ) {
		   	unsigned long pixel=XGetPixel( bmpImage, x, y);
			*bbits++ = (pixel >>16) & 0xff;
			*bbits++ = (pixel >> 8) & 0xff;
			*bbits++ =  pixel       & 0xff;
		   }
		   bbits += pad;
		}
		break;
	   case 32:
		for( y = yend - 1; (int)y >= (int)startscan; y-- )
		{
		   *bbits = 0;
		   for( x = 0; x < xend; x++ ) {
		   	unsigned long pixel=XGetPixel( bmpImage, x, y);
			*bbits++ = (pixel >>16) & 0xff;
			*bbits++ = (pixel >> 8) & 0xff;
			*bbits++ =  pixel       & 0xff;
		   }
		}
		break;
	   default:
	   	WARN(bitmap,"Unsupported depth %d\n",
                   info->bmiHeader.biBitCount);
	   	break;
	}

	XDestroyImage( bmpImage );
        LeaveCriticalSection( &X11DRV_CritSection );

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
                             DIB_GetDIBWidthBytes( bmp->bitmap.bmWidth,
                                                   bmp->bitmap.bmBitsPixel );
	info->bmiHeader.biCompression = 0;
    }

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
    return CreateDIBitmap32( hdc, header, init, bits, data, coloruse );
}


/***********************************************************************
 *           CreateDIBitmap32    (GDI32.37)
 */
HBITMAP32 WINAPI CreateDIBitmap32( HDC32 hdc, const BITMAPINFOHEADER *header,
                            DWORD init, LPCVOID bits, const BITMAPINFO *data,
                            UINT32 coloruse )
{
    HBITMAP32 handle;
    BOOL32 fColor;
    DWORD width;
    int height;
    WORD bpp;

    if (DIB_GetBitmapInfo( header, &width, &height, &bpp ) == -1) return 0;
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

    handle = fColor ? CreateCompatibleBitmap32( hdc, width, height ) :
                      CreateBitmap32( width, height, 1, 1, NULL );
    if (!handle) return 0;

    if (init == CBM_INIT)
        SetDIBits32( hdc, handle, 0, height, bits, data, coloruse );
    return handle;
}

/***********************************************************************
 *           CreateDIBSection16    (GDI.489)
 */
HBITMAP16 WINAPI CreateDIBSection16 (HDC16 hdc, BITMAPINFO *bmi, UINT16 usage,
				     LPVOID **bits, HANDLE16 section,
				     DWORD offset)
{
  return CreateDIBSection32 (hdc, bmi, usage, bits, section, offset);
}

/***********************************************************************
 *           CreateDIBSection32    (GDI32.36)
 */
HBITMAP32 WINAPI CreateDIBSection32 (HDC32 hdc, BITMAPINFO *bmi, UINT32 usage,
				     LPVOID **bits,HANDLE32 section,
				     DWORD offset)
{
  HBITMAP32 res = 0;

  FIXME(bitmap,
	  "(%d,[w=%ld,h=%ld],%d,%p,0x%08x,%ld),semistub\n",
	  hdc,bmi->bmiHeader.biWidth,bmi->bmiHeader.biHeight,
	  usage,bits,section,offset);

  if (bmi->bmiHeader.biHeight < 0 ) bmi->bmiHeader.biHeight = -bmi->bmiHeader.biHeight;
  if (bmi->bmiHeader.biWidth < 0 ) bmi->bmiHeader.biWidth = -bmi->bmiHeader.biWidth;
  /* FIXME.  The following line isn't quite right.  */
  res = CreateDIBitmap32 (hdc, &bmi->bmiHeader, 0, NULL, bmi, 0);
  if (res)
    {
      BITMAP32 bmp;
      if (GetObject32A (res, sizeof (bmp), &bmp))
	{
            /* FIXME: this is wrong! (bmBits is always NULL) */
            if (bits) *bits = bmp.bmBits;
	    /* hmpf */
	    TRACE(bitmap,"allocating %ld bytes of memory\n",
                bmi->bmiHeader.biWidth*bmi->bmiHeader.biHeight*4);
	    if (bits) *bits = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
                bmi->bmiHeader.biWidth*bmi->bmiHeader.biHeight*4);
            return res;
	}
    }

  /* Error.  */
  if (res) DeleteObject32 (res);
  if (bits) *bits = NULL;
  return 0;
}
