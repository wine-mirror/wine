/*
 * X11DRV device-independent bitmaps
 *
 * Copyright 1993,1994  Alexandre Julliard
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include "ts_xlib.h"
#include "ts_xutil.h"

#include "wintypes.h"
#include "bitmap.h"
#include "x11drv.h"
#include "debug.h"
#include "dc.h"
#include "color.h"
#include "callback.h"
#include "xmalloc.h"

static int bitmapDepthTable[] = { 8, 1, 32, 16, 24, 15, 4, 0 };
static int ximageDepthTable[] = { 0, 0, 0,  0,  0,  0,  0 };

/***********************************************************************
 *           X11DRV_DIB_Init
 */
BOOL32 X11DRV_DIB_Init(void)
{
    int		i;
    XImage*	testimage;

    for( i = 0; bitmapDepthTable[i]; i++ )
    {
	 testimage = TSXCreateImage(display, DefaultVisualOfScreen(X11DRV_GetXScreen()),
			 bitmapDepthTable[i], ZPixmap, 0, NULL, 1, 1, 32, 20 );
	 if( testimage ) ximageDepthTable[i] = testimage->bits_per_pixel;
	 else return FALSE;
	 TSXDestroyImage(testimage);
    }
    return TRUE;
}


/***********************************************************************
 *           X11DRV_DIB_GetXImageWidthBytes
 *
 * Return the width of an X image in bytes
 */
int X11DRV_DIB_GetXImageWidthBytes( int width, int depth )
{
    int		i;

    if (!ximageDepthTable[0]) {
	    X11DRV_DIB_Init();
    }
    for( i = 0; bitmapDepthTable[i] ; i++ )
	 if( bitmapDepthTable[i] == depth )
	     return (4 * ((width * ximageDepthTable[i] + 31)/32));
    
    WARN(bitmap, "(%d): Unsupported depth\n", depth );
    return (4 * width);
}

/***********************************************************************
 *           X11DRV_DIB_BuildColorMap
 *
 * Build the color map from the bitmap palette. Should not be called
 * for a >8-bit deep bitmap.
 */
int *X11DRV_DIB_BuildColorMap( DC *dc, WORD coloruse, WORD depth, 
                               const BITMAPINFO *info, int *nColors )
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

    if (colors > 256)
    {
        ERR(bitmap, "called with >256 colors!\n");
        return NULL;
    }

    if (!(colorMapping = (int *)HeapAlloc(GetProcessHeap(), 0,
                                          colors * sizeof(int) ))) 
	return NULL;

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

    *nColors = colors;
    return colorMapping;
}


/***********************************************************************
 *           X11DRV_DIB_MapColor
 */
int X11DRV_DIB_MapColor( int *physMap, int nPhysMap, int phys )
{
    int color;

    for (color = 0; color < nPhysMap; color++)
        if (physMap[color] == phys)
            return color;

    WARN(bitmap, "Strange color %08x\n", phys);
    return 0;
}



/***********************************************************************
 *           X11DRV_DIB_SetImageBits_1_Line
 *
 * Handles a single line of 1 bit data.
 */
static void X11DRV_DIB_SetImageBits_1_Line(DWORD dstwidth, int left, int *colors,
				    XImage *bmpImage, int h, const BYTE *bits)
{
    BYTE pix, extra;
    DWORD i, x;

    if((extra = (left & 7)) != 0) {
        left &= ~7;
	dstwidth += extra;
    }

    bits += left >> 3;

    /* FIXME: should avoid putting x<left pixels (minor speed issue) */
    for (i = dstwidth/8, x = left; i > 0; i--)
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
 *           X11DRV_DIB_SetImageBits_1
 *
 * SetDIBits for a 1-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_1( int lines, const BYTE *srcbits,
                                DWORD srcwidth, DWORD dstwidth, int left,
                                int *colors, XImage *bmpImage )
{
    int h;

    /* 32 bit aligned */
    DWORD linebytes = ((srcwidth + 31) & ~31) / 8;

    if (lines > 0) {
	for (h = lines-1; h >=0; h--) {
	    X11DRV_DIB_SetImageBits_1_Line(dstwidth, left, colors, bmpImage, h,
					   srcbits);
	    srcbits += linebytes;
	}
    } else {
	lines = -lines;
	for (h = 0; h < lines; h++) {
	    X11DRV_DIB_SetImageBits_1_Line(dstwidth, left, colors, bmpImage, h,
					   srcbits);
	    srcbits += linebytes;
	}
    }
}


/***********************************************************************
 *           X11DRV_DIB_SetImageBits_4
 *
 * SetDIBits for a 4-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_4( int lines, const BYTE *srcbits,
                                DWORD srcwidth, DWORD dstwidth, int left,
                                int *colors, XImage *bmpImage )
{
    DWORD i, x;
    int h;
    const BYTE *bits = srcbits + (left >> 1);
  
    /* 32 bit aligned */
    DWORD linebytes = ((srcwidth+7)&~7)/2;

    if(left & 1) {
        left--;
	dstwidth++;
    }

    if (lines > 0) {
	for (h = lines-1; h >= 0; h--) {
	    for (i = dstwidth/2, x = left; i > 0; i--) {
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
	    for (i = dstwidth/2, x = left; i > 0; i--) {
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



/***********************************************************************
 *           X11DRV_DIB_GetImageBits_4
 *
 * GetDIBits for a 4-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_4( int lines, BYTE *srcbits,
                                DWORD srcwidth, DWORD dstwidth, int left,
                                int *colors, int nColors, XImage *bmpImage )
{
    DWORD i, x;
    int h;
    BYTE *bits = srcbits + (left >> 1);

    /* 32 bit aligned */
    DWORD linebytes = ((srcwidth+7)&~7)/2;

    if(left & 1) {
        left--;
	dstwidth++;
    }

    /* FIXME: should avoid putting x<left pixels (minor speed issue) */
    if (lines > 0) {
       for (h = lines-1; h >= 0; h--) {
           for (i = dstwidth/2, x = left; i > 0; i--) {
               *bits++ = (X11DRV_DIB_MapColor( colors, nColors, 
					XGetPixel( bmpImage, x++, h )) << 4)
		       | (X11DRV_DIB_MapColor( colors, nColors,
					XGetPixel( bmpImage, x++, h )) & 0x0f);
           }
           if (dstwidth & 1)
               *bits = (X11DRV_DIB_MapColor( colors, nColors,
					XGetPixel( bmpImage, x++, h )) << 4);
           srcbits += linebytes;
           bits     = srcbits + (left >> 1);
       }
    } else {
       lines = -lines;
       for (h = 0; h < lines; h++) {
           for (i = dstwidth/2, x = left; i > 0; i--) {
               *bits++ = (X11DRV_DIB_MapColor( colors, nColors,
					XGetPixel( bmpImage, x++, h ))
			  << 4)
		       | (X11DRV_DIB_MapColor( colors, nColors,
					XGetPixel( bmpImage, x++, h )) & 0x0f);
           }
           if (dstwidth & 1)
               *bits = (X11DRV_DIB_MapColor( colors, nColors,
					XGetPixel( bmpImage, x++, h )) << 4);
           srcbits += linebytes;
           bits     = srcbits + (left >> 1);
       }
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
    int x = 0, c, length;
    const BYTE *begin = bits;

    lines--;

    while ((int)lines >= 0) {
        length = *bits++;
	if (length) {	/* encoded */
	    c = *bits++;
	    while (length--) {
	        XPutPixel(bmpImage, x++, lines, colors[c >>4]);
		if(x >= width) {
		    x = 0;
		    if(--lines < 0)
		        return;
		}
		if (length) {
		    length--;
		    XPutPixel(bmpImage, x++, lines, colors[c & 0xf]);
		    if(x >= width) {
		        x = 0;
			if(--lines < 0)
			    return;
		    }
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

	    case 2: /* delta */
	        x += *bits++;
		if(x >= width) {
		   FIXME(x11drv, "x-delta is too large?\n");
		   return;
		}
		lines -= *bits++;
		continue;

	    default: /* absolute */
	        while (length--) {
		    c = *bits++;
		    XPutPixel(bmpImage, x++, lines, colors[c >> 4]);
		    if(x >= width) {
		        x = 0;
			if(--lines < 0)
			    return;
		    }
		    if (length) {
		        length--;
			XPutPixel(bmpImage, x++, lines, colors[c & 0xf]);
			if(x >= width) {
			    x = 0;
			    if(--lines < 0)
			        return;
			}
		    }
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
                                int *colors, XImage *bmpImage )
{
    DWORD x;
    int h;
    const BYTE *bits = srcbits + left;

    /* align to 32 bit */
    DWORD linebytes = (srcwidth + 3) & ~3;

    dstwidth += left;

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
 *           X11DRV_DIB_GetImageBits_8
 *
 * GetDIBits for an 8-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_8( int lines, BYTE *srcbits,
                                DWORD srcwidth, DWORD dstwidth, int left,
                                int *colors, int nColors, XImage *bmpImage )
{
    DWORD x;
    int h;
    BYTE *bits = srcbits + left;

    /* align to 32 bit */
    DWORD linebytes = (srcwidth + 3) & ~3;

    dstwidth += left;

    if (lines > 0) {
       for (h = lines - 1; h >= 0; h--) {
           for (x = left; x < dstwidth; x++, bits++) {
               if ( XGetPixel( bmpImage, x, h ) != colors[*bits] )
                   *bits = X11DRV_DIB_MapColor( colors, nColors,
                                          XGetPixel( bmpImage, x, h ) );
           }
           bits = (srcbits += linebytes) + left;
       }
    } else {
       lines = -lines;
       for (h = 0; h < lines; h++) {
           for (x = left; x < dstwidth; x++, bits++) {
               if ( XGetPixel( bmpImage, x, h ) != colors[*bits] )
                   *bits = X11DRV_DIB_MapColor( colors, nColors,
                                         XGetPixel( bmpImage, x, h ) );
           }
           bits = (srcbits += linebytes) + left;
       }
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

enum Rle8_EscapeCodes		
{
  /* 
   * Apologies for polluting your file's namespace...
   */
  RleEol 	= 0,		/* End of line */
  RleEnd 	= 1,		/* End of bitmap */
  RleDelta	= 2		/* Delta */
};
  
static void X11DRV_DIB_SetImageBits_RLE8( int lines, const BYTE *bits,
					  DWORD width, DWORD dstwidth,
					  int left, int *colors,
					  XImage *bmpImage )
{
    int x;			/* X-positon on each line.  Increases. */
    int line;			/* Line #.  Starts at lines-1, decreases */
    const BYTE *pIn = bits;     /* Pointer to current position in bits */
    BYTE length;		/* The length pf a run */
    BYTE color_index;		/* index into colors[] as read from bits */
    BYTE escape_code;		/* See enum Rle8_EscapeCodes.*/
    int color;			/* value of colour[color_index] */
    
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
 *           X11DRV_DIB_SetImageBits_16
 *
 * SetDIBits for a 16-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_16( int lines, const BYTE *srcbits,
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
 *           X11DRV_DIB_GetImageBits_16
 *
 * GetDIBits for an 16-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_16( int lines, BYTE *srcbits,
                                 DWORD srcwidth, DWORD dstwidth, int left,
                                 XImage *bmpImage )
{
    DWORD x;
    LPWORD ptr;
    int h;
    BYTE r, g, b;

    /* align to 32 bit */
    DWORD linebytes = (srcwidth * 2 + 3) & ~3;

    dstwidth += left;

    ptr = (LPWORD) srcbits + left;
    if (lines > 0) {
       for (h = lines - 1; h >= 0; h--)
        {
           for (x = left; x < dstwidth; x++, ptr++)
            {
                COLORREF pixel = COLOR_ToLogical( XGetPixel( bmpImage, x, h ) );
               r = (BYTE) GetRValue(pixel);
               g = (BYTE) GetGValue(pixel);
               b = (BYTE) GetBValue(pixel);
                *ptr = ( ((r << 7) & 0x7c00) | ((g << 2) & 0x03e0) | ((b >> 3) & 0x001f) );
           }
           ptr = (LPWORD) (srcbits += linebytes) + left;
       }
    } else {
       lines = -lines;
       for (h = 0; h < lines; h++)
        {
           for (x = left; x < dstwidth; x++, ptr++)
            {
                COLORREF pixel = COLOR_ToLogical( XGetPixel( bmpImage, x, h ) );
               r = (BYTE) GetRValue(pixel);
               g = (BYTE) GetGValue(pixel);
               b = (BYTE) GetBValue(pixel);
                *ptr = ( ((r << 7) & 0x7c00) | ((g << 2) & 0x03e0) | ((b >> 3) & 0x001f) );
           }

           ptr = (LPWORD) (srcbits += linebytes) + left;
       }
    }
}



/***********************************************************************
 *           X11DRV_DIB_SetImageBits_24
 *
 * SetDIBits for a 24-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_24( int lines, const BYTE *srcbits,
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
 *           X11DRV_DIB_GetImageBits_24
 *
 * GetDIBits for an 24-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_24( int lines, BYTE *srcbits,
                                 DWORD srcwidth, DWORD dstwidth, int left,
                                 XImage *bmpImage )
{
    DWORD x;
    int h;
    BYTE *bits = srcbits + (left * 3);

    /* align to 32 bit */
    DWORD linebytes = (srcwidth * 3 + 3) & ~3;

    dstwidth += left;

    if (lines > 0) {
       for (h = lines - 1; h >= 0; h--)
        {
           for (x = left; x < dstwidth; x++, bits += 3)
            {
                COLORREF pixel = COLOR_ToLogical( XGetPixel( bmpImage, x, h ) );
                bits[0] = GetRValue(pixel);
                bits[1] = GetGValue(pixel);
                bits[2] = GetBValue(pixel);
           }
           bits = (srcbits += linebytes) + (left * 3);
       }
    } else {
       lines = -lines;
       for (h = 0; h < lines; h++)
        {
           for (x = left; x < dstwidth; x++, bits += 3)
            {
                COLORREF pixel = COLOR_ToLogical( XGetPixel( bmpImage, x, h ) );
                bits[0] = GetRValue(pixel);
                bits[1] = GetGValue(pixel);
                bits[2] = GetBValue(pixel);
           }

           bits = (srcbits += linebytes) + (left * 3);
       }
    }
}


/***********************************************************************
 *           X11DRV_DIB_SetImageBits_32
 *
 * SetDIBits for a 32-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_32( int lines, const BYTE *srcbits,
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
 *           X11DRV_DIB_GetImageBits_32
 *
 * GetDIBits for an 32-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_32( int lines, BYTE *srcbits,
                                 DWORD srcwidth, DWORD dstwidth, int left,
                                 XImage *bmpImage )
{
    DWORD x;
    int h;
    BYTE *bits = srcbits + (left * 4);

    /* align to 32 bit */
    DWORD linebytes = (srcwidth * 4);

    dstwidth += left;

    if (lines > 0) {
       for (h = lines - 1; h >= 0; h--)
        {
           for (x = left; x < dstwidth; x++, bits += 4)
            {
                COLORREF pixel = COLOR_ToLogical( XGetPixel( bmpImage, x, h ) );
                bits[0] = GetRValue(pixel);
                bits[1] = GetGValue(pixel);
                bits[2] = GetBValue(pixel);
           }
           bits = (srcbits += linebytes) + (left * 4);
       }
    } else {
       lines = -lines;
       for (h = 0; h < lines; h++)
        {
           for (x = left; x < dstwidth; x++, bits += 4)
            {
                COLORREF pixel = COLOR_ToLogical( XGetPixel( bmpImage, x, h ) );
                bits[0] = GetRValue(pixel);
                bits[1] = GetGValue(pixel);
                bits[2] = GetBValue(pixel);
           }

           bits = (srcbits += linebytes) + (left * 4);
       }
    }
}


/***********************************************************************
 *           X11DRV_DIB_SetImageBits
 *
 * Transfer the bits to an X image.
 * Helper function for SetDIBits() and SetDIBitsToDevice().
 * The Xlib critical section must be entered before calling this function.
 */
int X11DRV_DIB_SetImageBits( const DIB_SETIMAGEBITS_DESCR *descr )
{
    int lines = descr->lines >= 0 ? descr->lines : -descr->lines;
    XImage *bmpImage;

    if ( descr->dc && descr->dc->w.flags & DC_DIRTY ) 
        CLIPPING_UpdateGCRegion( descr->dc );

    if (descr->image)
        bmpImage = descr->image;
    else {
        bmpImage = XCreateImage( display,
				 DefaultVisualOfScreen(X11DRV_GetXScreen()),
				 descr->depth, ZPixmap, 0, NULL,
				 descr->infoWidth, lines, 32, 0 );
	bmpImage->data = xcalloc( bmpImage->bytes_per_line * lines );
    }

      /* Transfer the pixels */
    switch(descr->infoBpp)
    {
    case 1:
	X11DRV_DIB_SetImageBits_1( descr->lines, descr->bits, descr->infoWidth,
				   descr->width, descr->xSrc, descr->colorMap,
				   bmpImage );
	break;
    case 4:
	if (descr->compression)
	    X11DRV_DIB_SetImageBits_RLE4( descr->lines, descr->bits,
					  descr->infoWidth, descr->width,
					  descr->xSrc, descr->colorMap,
					  bmpImage );
	else
	    X11DRV_DIB_SetImageBits_4( descr->lines, descr->bits,
				       descr->infoWidth, descr->width,
				       descr->xSrc, descr->colorMap,
				       bmpImage );
	break;
    case 8:
	if (descr->compression)
	    X11DRV_DIB_SetImageBits_RLE8( descr->lines, descr->bits,
					  descr->infoWidth, descr->width,
					  descr->xSrc,
					  descr->colorMap, bmpImage );
	else
	    X11DRV_DIB_SetImageBits_8( descr->lines, descr->bits,
				       descr->infoWidth, descr->width,
				       descr->xSrc, descr->colorMap,
				       bmpImage );
	break;
    case 15:
    case 16:
	X11DRV_DIB_SetImageBits_16( descr->lines, descr->bits,
				    descr->infoWidth, descr->width,
				    descr->xSrc, descr->dc, bmpImage);
	break;
    case 24:
	X11DRV_DIB_SetImageBits_24( descr->lines, descr->bits,
				    descr->infoWidth, descr->width,
				    descr->xSrc, descr->dc, bmpImage );
	break;
    case 32:
	X11DRV_DIB_SetImageBits_32( descr->lines, descr->bits,
				    descr->infoWidth, descr->width,
				    descr->xSrc, descr->dc, bmpImage);
	break;
    default:
        WARN(bitmap, "(%d): Invalid depth\n", descr->infoBpp );
        break;
    }

    XPutImage( display, descr->drawable, descr->gc, bmpImage,
               descr->xSrc, descr->ySrc, descr->xDest, descr->yDest,
               descr->width, descr->height );

    if (!descr->image) XDestroyImage( bmpImage );
    return lines;
}

/***********************************************************************
 *           X11DRV_DIB_GetImageBits
 *
 * Transfer the bits from an X image.
 * The Xlib critical section must be entered before calling this function.
 */
int X11DRV_DIB_GetImageBits( const DIB_SETIMAGEBITS_DESCR *descr )
{
    int lines = descr->lines >= 0 ? descr->lines : -descr->lines;
    XImage *bmpImage;

    if (descr->image)
        bmpImage = descr->image;
    else {
        bmpImage = XCreateImage( display,
				 DefaultVisualOfScreen(X11DRV_GetXScreen()),
				 descr->depth, ZPixmap, 0, NULL,
				 descr->infoWidth, lines, 32, 0 );
	bmpImage->data = xcalloc( bmpImage->bytes_per_line * lines );
    }

    XGetSubImage( display, descr->drawable, descr->xDest, descr->yDest,
                  descr->width, descr->height, AllPlanes, ZPixmap,
                  bmpImage, descr->xSrc, descr->ySrc );

      /* Transfer the pixels */
    switch(descr->infoBpp)
    {
    case 1:
        FIXME(bitmap, "Depth 1 not yet supported!\n");
       break;

    case 4:
       if (descr->compression)
	   FIXME(bitmap, "Compression not yet supported!\n");
       else
	   X11DRV_DIB_GetImageBits_4( descr->lines, 
				      (LPVOID)descr->bits, descr->infoWidth,
				      descr->width, descr->xSrc,
				      descr->colorMap, descr->nColorMap,
				      bmpImage );
       break;

    case 8:
       if (descr->compression)
	   FIXME(bitmap, "Compression not yet supported!\n");
       else
	   X11DRV_DIB_GetImageBits_8( descr->lines, (LPVOID)descr->bits,
				      descr->infoWidth, descr->width,
				      descr->xSrc, descr->colorMap,
				      descr->nColorMap, bmpImage );
       break;

    case 15:
    case 16:
       X11DRV_DIB_GetImageBits_16( descr->lines, (LPVOID)descr->bits,
				   descr->infoWidth,
				   descr->width, descr->xSrc, bmpImage );
       break;

    case 24:
       X11DRV_DIB_GetImageBits_24( descr->lines, (LPVOID)descr->bits,
				   descr->infoWidth,
				   descr->width, descr->xSrc, bmpImage );
       break;

    case 32:
       X11DRV_DIB_GetImageBits_32( descr->lines, (LPVOID)descr->bits,
				   descr->infoWidth,
				   descr->width, descr->xSrc, bmpImage );
       break;

    default:
        WARN(bitmap, "(%d): Invalid depth\n", descr->infoBpp );
        break;
    }

    if (!descr->image) XDestroyImage( bmpImage );
    return lines;
}

/*************************************************************************
 *		X11DRV_SetDIBitsToDevice
 *
 */
INT32 X11DRV_SetDIBitsToDevice( DC *dc, INT32 xDest, INT32 yDest, DWORD cx,
				DWORD cy, INT32 xSrc, INT32 ySrc,
				UINT32 startscan, UINT32 lines, LPCVOID bits,
				const BITMAPINFO *info, UINT32 coloruse )
{
    DIB_SETIMAGEBITS_DESCR descr;
    DWORD width, oldcy = cy;
    INT32 result;
    int height, tmpheight;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;


    if (DIB_GetBitmapInfo( &info->bmiHeader, &width, &height, 
			   &descr.infoBpp, &descr.compression ) == -1)
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

    X11DRV_SetupGCForText( dc );  /* To have the correct colors */
    TSXSetFunction(display, physDev->gc, X11DRV_XROPfunction[dc->w.ROPmode-1]);

    if (descr.infoBpp <= 8)
    {
        descr.colorMap = X11DRV_DIB_BuildColorMap( dc, coloruse,
						   dc->w.bitsPerPixel,
						   info, &descr.nColorMap );
        if (!descr.colorMap)
            return 0;
    }

    descr.dc        = dc;
    descr.bits      = bits;
    descr.image     = NULL;
    descr.lines     = tmpheight >= 0 ? lines : -lines;
    descr.infoWidth = width;
    descr.depth     = dc->w.bitsPerPixel;
    descr.drawable  = physDev->drawable;
    descr.gc        = physDev->gc;
    descr.xSrc      = xSrc;
    descr.ySrc      = tmpheight >= 0 ? lines-(ySrc-startscan)-cy+(oldcy-cy) 
                                     : ySrc - startscan;
    descr.xDest     = dc->w.DCOrgX + XLPTODP( dc, xDest );
    descr.yDest     = dc->w.DCOrgY + YLPTODP( dc, yDest ) +
                                     (tmpheight >= 0 ? oldcy-cy : 0);
    descr.width     = cx;
    descr.height    = cy;

    EnterCriticalSection( &X11DRV_CritSection );
    result = CALL_LARGE_STACK( X11DRV_DIB_SetImageBits, &descr );
    LeaveCriticalSection( &X11DRV_CritSection );

    HeapFree(GetProcessHeap(), 0, descr.colorMap);
    return result;
}

#endif /* !defined(X_DISPLAY_MISSING) */
