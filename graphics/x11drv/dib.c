/*
 * X11DRV device-independent bitmaps
 *
 * Copyright 1993,1994  Alexandre Julliard
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include "ts_xlib.h"
#include "ts_xutil.h"

#include "windef.h"
#include "bitmap.h"
#include "x11drv.h"
#include "debug.h"
#include "dc.h"
#include "color.h"
#include "callback.h"
#include "selectors.h"
#include "global.h"
#include "xmalloc.h" /* for XCREATEIMAGE macro */

static int bitmapDepthTable[] = { 8, 1, 32, 16, 24, 15, 4, 0 };
static int ximageDepthTable[] = { 0, 0, 0,  0,  0,  0,  0 };

/***********************************************************************
 *           X11DRV_DIB_Init
 */
BOOL X11DRV_DIB_Init(void)
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
    BOOL isInfo;
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
                    colorMapping[i] = X11DRV_PALETTE_ToPhysical( dc, RGB(rgb->rgbRed,
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
                    colorMapping[i] = X11DRV_PALETTE_ToPhysical( dc, RGB(rgb->rgbtRed,
                                                               rgb->rgbtGreen,
                                                               rgb->rgbtBlue));
        }
    }
    else  /* DIB_PAL_COLORS */
    {
        for (i = 0; i < colors; i++, colorPtr++)
            colorMapping[i] = X11DRV_PALETTE_ToPhysical( dc, PALETTEINDEX(*colorPtr) );
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
		if(x >= width) {
		    x = 0;
		    if(--lines < 0)
		        return;
		}
	        XPutPixel(bmpImage, x++, lines, colors[c >>4]);
		if (length) {
		    length--;
		    if(x >= width) {
		        x = 0;
			if(--lines < 0)
			    return;
		    }
		    XPutPixel(bmpImage, x++, lines, colors[c & 0xf]);
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
		    if(x >= width) {
		        x = 0;
			if(--lines < 0)
			    return;
		    }
		    XPutPixel(bmpImage, x++, lines, colors[c >> 4]);
		    if (length) {
		        length--;
			if(x >= width) {
			    x = 0;
			    if(--lines < 0)
			        return;
			}
			XPutPixel(bmpImage, x++, lines, colors[c & 0xf]);
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

    if (bmpImage->format == ZPixmap)
    {
        unsigned short indA, indB, indC, indD;
        BYTE *imageBits;
    	
        switch (bmpImage->bits_per_pixel)
        {
        case 16:    		
            indA = (bmpImage->byte_order == LSBFirst) ? 1 : 0;
            indB = (indA == 1) ? 0 : 1;

            if (lines > 0) {
                imageBits = (BYTE *)(bmpImage->data + (lines - 1)*bmpImage->bytes_per_line);
                for (h = lines - 1; h >= 0; h--) {
                    for (x = left; x < dstwidth; x++, ptr++) {
                        val = *ptr;
                        imageBits[(x << 1) + indA] = (BYTE)((val >> 7) & 0x00ff);
                        imageBits[(x << 1) + indB] = (BYTE)(((val << 1) & 0x00c0) | (val & 0x001f));
                    }
                    ptr = (LPWORD)(srcbits += linebytes) + left;
                    imageBits -= bmpImage->bytes_per_line;
                }
            } else {
                lines = -lines;
                imageBits = (BYTE *)bmpImage->data;
                for (h = 0; h < lines; h++) {
                    for (x = left; x < dstwidth; x++, ptr++) {
                        val = *ptr;
                        imageBits[(x << 1) + indA] = (BYTE)((val >> 7) & 0x00ff);
                        imageBits[(x << 1) + indB] = (BYTE)(((val << 1) & 0x00c0) | (val & 0x001f));
                    }
                    ptr = (LPWORD)(srcbits += linebytes) + left;
                    imageBits -= bmpImage->bytes_per_line;
                }
            }
            return;

        case 32:
            indA = (bmpImage->byte_order == LSBFirst) ? 3 : 0;
            indB = (indA == 3) ? 2 : 1;
            indC = (indB == 2) ? 1 : 2;             	 		
            indD = (indC == 1) ? 0 : 3;

            if (lines > 0) {
                imageBits = (BYTE *)(bmpImage->data + (lines - 1)*bmpImage->bytes_per_line);
                for (h = lines - 1; h >= 0; h--) {
                    for (x = left; x < dstwidth; x++, ptr ++) {
                        val = *ptr;
                        imageBits[(x << 2) + indA] = 0x00; /* a */
                        imageBits[(x << 2) + indB] = (BYTE)(((val >> 7) & 0x00f8) | ((val >> 12) & 0x0007));  /* red */
                        imageBits[(x << 2) + indC] = (BYTE)(((val >> 2) & 0x00f8) | ((val >> 7) & 0x0007));   /* green */
                        imageBits[(x << 2) + indD] = (BYTE)(((val << 3) & 0x00f8) | ((val >> 2) & 0x0007));   /* blue */
                    }
                    ptr = (LPWORD)(srcbits += linebytes) + left;
                    imageBits -= bmpImage->bytes_per_line;
                }
            } else {
                lines = -lines;
                imageBits = (BYTE *)(bmpImage->data + (lines - 1)*bmpImage->bytes_per_line);
                for (h = 0; h < lines; h++) {
                    for (x = left; x < dstwidth; x++, ptr ++) {
                        val = *ptr;
                        imageBits[(x << 2) + indA] = 0x00;
                        imageBits[(x << 2) + indB] = (BYTE)(((val >> 7) & 0x00f8) | ((val >> 12) & 0x0007));
                        imageBits[(x << 2) + indC] = (BYTE)(((val >> 2) & 0x00f8) | ((val >> 7) & 0x0007));
                        imageBits[(x << 2) + indD] = (BYTE)(((val << 3) & 0x00f8) | ((val >> 2) & 0x0007));
                    }
                    ptr = (LPWORD)(srcbits += linebytes) + left;
                    imageBits -= bmpImage->bytes_per_line;
                }
            }
            return;
	}
    }

    /* Not standard format or Not RGB */
    if (lines > 0) {
	for (h = lines - 1; h >= 0; h--) {
	    for (x = left; x < dstwidth; x++, ptr++) {
		val = *ptr;
		r = (BYTE) ((val & 0x7c00) >> 7);
		g = (BYTE) ((val & 0x03e0) >> 2);
		b = (BYTE) ((val & 0x001f) << 3);
		XPutPixel( bmpImage, x, h,
			   X11DRV_PALETTE_ToPhysical(dc, RGB(r,g,b)) );
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
			   X11DRV_PALETTE_ToPhysical(dc, RGB(r,g,b)) );
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
                COLORREF pixel = X11DRV_PALETTE_ToLogical( XGetPixel( bmpImage, x, h ) );
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
                COLORREF pixel = X11DRV_PALETTE_ToLogical( XGetPixel( bmpImage, x, h ) );
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

    if (bmpImage->format == ZPixmap)
    {
        unsigned short indA, indB, indC, indD;
        BYTE *imageBits;
      	
        switch (bmpImage->bits_per_pixel)
    	{
        case 16:    		
            indA = (bmpImage->byte_order == LSBFirst) ? 1 : 0;
            indB = (indA == 1) ? 0 : 1;

            if (lines > 0) {
                imageBits = (BYTE *)(bmpImage->data + (lines - 1)*bmpImage->bytes_per_line);
                for (h = lines - 1; h >= 0; h--) {
                    for (x = left; x < dstwidth; x++, bits += 3) {
                        imageBits[(x << 1) + indA] = (bits[0] & 0xf8) | ((bits[1] >> 5) & 0x07);
                        imageBits[(x << 1) + indB] = ((bits[1] << 3) & 0xc0) | ((bits[2] >> 3) & 0x1f);
                    }
                    bits = (srcbits += linebytes) + left * 3;
                    imageBits -= bmpImage->bytes_per_line;
                }
            } else {
                lines = -lines;
                imageBits = (BYTE *)bmpImage->data;
                for (h = 0; h < lines; h++) {
                    for (x = left; x < dstwidth; x++, bits += 3) {
                        imageBits[(x << 1) + indA] = (bits[0] & 0xf8) | ((bits[1] >> 5) & 0x07);
                        imageBits[(x << 1) + indB] = ((bits[1] << 3) & 0xc0) | ((bits[2] >> 3) & 0x1f);
                    }
                    bits = (srcbits += linebytes) + left * 3;
                    imageBits += bmpImage->bytes_per_line;
                }
            }
            return;

        case 32:
            indA = (bmpImage->byte_order == LSBFirst) ? 3 : 0;
            indB = (indA == 3) ? 2 : 1;
            indC = (indB == 2) ? 1 : 2;
            indD = (indC == 1) ? 0 : 3;

            if (lines > 0) {
                imageBits = (BYTE *)(bmpImage->data + (lines - 1)*bmpImage->bytes_per_line);
                for (h = lines - 1; h >= 0; h--) {
                    for (x = left; x < dstwidth; x++, bits += 3) {
                        imageBits[(x << 2) + indA] = 0x00;	/*a*/
                        imageBits[(x << 2) + indB] = bits[2];   /*red*/
                        imageBits[(x << 2) + indC] = bits[1];   /*green*/
                        imageBits[(x << 2) + indD] = bits[0];   /*blue*/
                    }
                    bits = (srcbits += linebytes) + left * 3;
                    imageBits -= bmpImage->bytes_per_line;
                }
            } else {
                lines = -lines;
                imageBits = (BYTE *)(bmpImage->data + (lines - 1)*bmpImage->bytes_per_line);
                for (h = 0; h < lines; h++) {
                    for (x = left; x < dstwidth; x++, bits += 3) {
                        imageBits[(x << 2) + indA] = 0x00;
                        imageBits[(x << 2) + indB] = bits[2];
                        imageBits[(x << 2) + indC] = bits[1];
                        imageBits[(x << 2) + indD] = bits[0];
                    }
                    bits = (srcbits += linebytes) + left * 3;
                    imageBits += bmpImage->bytes_per_line;
                }
            }
            return;
	}
    }
	
    /* Not standard format or Not RGB */
    if (lines > 0) {
	for (h = lines - 1; h >= 0; h--) {
	    for (x = left; x < dstwidth; x++, bits += 3) {
		XPutPixel( bmpImage, x, h, 
			   X11DRV_PALETTE_ToPhysical(dc, RGB(bits[2],bits[1],bits[0])));
	    }
	    bits = (srcbits += linebytes) + left * 3;
	}
    } else {
	lines = -lines;
	for (h = 0; h < lines; h++) {
	    for (x = left; x < dstwidth; x++, bits += 3) {
		XPutPixel( bmpImage, x, h,
			   X11DRV_PALETTE_ToPhysical(dc, RGB(bits[2],bits[1],bits[0])));
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
                COLORREF pixel = X11DRV_PALETTE_ToLogical( XGetPixel( bmpImage, x, h ) );
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
                COLORREF pixel = X11DRV_PALETTE_ToLogical( XGetPixel( bmpImage, x, h ) );
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

    if (bmpImage->format == ZPixmap)
    {
        unsigned short indA, indB, indC, indD;
        BYTE *imageBits;

        switch (bmpImage->bits_per_pixel)
        {
        case 16:    		
            indA = (bmpImage->byte_order == LSBFirst) ? 1 : 0;
            indB = (indA == 1) ? 0 : 1;

            if (lines > 0) {
                imageBits = (BYTE *)(bmpImage->data + (lines - 1)*bmpImage->bytes_per_line);
                for (h = lines - 1; h >= 0; h--) {
                    for (x = left; x < dstwidth; x++, bits += 4) {
                        imageBits[(x << 1) + indA] = (bits[0] & 0xf8) | ((bits[1] >> 5) & 0x07);
                        imageBits[(x << 1) + indB] = ((bits[1] << 3) & 0xc0) | ((bits[2] >> 3) & 0x1f);
                    }
                    bits = (srcbits += linebytes) + left * 4;
                    imageBits -= bmpImage->bytes_per_line;
                }
            } else {
                lines = -lines;
                imageBits = (BYTE *)bmpImage->data;
                for (h = 0; h < lines; h++) {
                    for (x = left; x < dstwidth; x++, bits += 4) {
                        imageBits[(x << 1) + indA] = (bits[0] & 0xf8) | ((bits[1] >> 5) & 0x07);
                        imageBits[(x << 1) + indB] = ((bits[1] << 3) & 0xc0) | ((bits[2] >> 3) & 0x1f);
                    }
                    bits = (srcbits += linebytes) + left * 4;
                    imageBits += bmpImage->bytes_per_line;
                }
            }
            return;

        case 32:
            indA = (bmpImage->byte_order == LSBFirst) ? 3 : 0;
            indB = (indA == 3) ? 2 : 1;
            indC = (indB == 2) ? 1 : 2;
            indD = (indC == 1) ? 0 : 3;

            if (lines > 0) {
                imageBits = (BYTE *)(bmpImage->data + (lines - 1)*bmpImage->bytes_per_line);
                for (h = lines - 1; h >= 0; h--) {
                    for (x = left; x < dstwidth; x++, bits += 4) {
                        imageBits[(x << 2) + indA] = 0x00;	/*a*/
                        imageBits[(x << 2) + indB] = bits[2];   /*red*/
                        imageBits[(x << 2) + indC] = bits[1];   /*green*/
                        imageBits[(x << 2) + indD] = bits[0];   /*blue*/
                    }
                    bits = (srcbits += linebytes) + left * 4;
                    imageBits -= bmpImage->bytes_per_line;
                }
            } else {
                lines = -lines;
                imageBits = (BYTE *)(bmpImage->data + (lines - 1)*bmpImage->bytes_per_line);
                for (h = 0; h < lines; h++) {
                    for (x = left; x < dstwidth; x++, bits += 4) {
                        imageBits[(x << 2) + indA] = 0x00;
                        imageBits[(x << 2) + indB] = bits[2];
                        imageBits[(x << 2) + indC] = bits[1];
                        imageBits[(x << 2) + indD] = bits[0];
                    }
                    bits = (srcbits += linebytes) + left * 4;
                    imageBits += bmpImage->bytes_per_line;
                }
            }
            return;
	}
    }

    if (lines > 0) {
	for (h = lines - 1; h >= 0; h--) {
	    for (x = left; x < dstwidth; x++, bits += 4) {
		XPutPixel( bmpImage, x, h, 
			   X11DRV_PALETTE_ToPhysical(dc, RGB(bits[2],bits[1],bits[0])));
	    }
	    bits = (srcbits += linebytes) + left * 4;
	}
    } else {
	lines = -lines;
	for (h = 0; h < lines; h++) {
	    for (x = left; x < dstwidth; x++, bits += 4) {
		XPutPixel( bmpImage, x, h,
			   X11DRV_PALETTE_ToPhysical(dc, RGB(bits[2],bits[1],bits[0])));
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
                COLORREF pixel = X11DRV_PALETTE_ToLogical( XGetPixel( bmpImage, x, h ) );
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
                COLORREF pixel = X11DRV_PALETTE_ToLogical( XGetPixel( bmpImage, x, h ) );
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
int X11DRV_DIB_SetImageBits( const X11DRV_DIB_SETIMAGEBITS_DESCR *descr )
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
int X11DRV_DIB_GetImageBits( const X11DRV_DIB_SETIMAGEBITS_DESCR *descr )
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
INT X11DRV_SetDIBitsToDevice( DC *dc, INT xDest, INT yDest, DWORD cx,
				DWORD cy, INT xSrc, INT ySrc,
				UINT startscan, UINT lines, LPCVOID bits,
				const BITMAPINFO *info, UINT coloruse )
{
    X11DRV_DIB_SETIMAGEBITS_DESCR descr;
    DWORD width, oldcy = cy;
    INT result;
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

/***********************************************************************
 *           X11DRV_DIB_SetDIBits
 */
INT X11DRV_DIB_SetDIBits(
  BITMAPOBJ *bmp, DC *dc, UINT startscan,
  UINT lines, LPCVOID bits, const BITMAPINFO *info,
  UINT coloruse, HBITMAP hbitmap)
{
  X11DRV_DIB_SETIMAGEBITS_DESCR descr;
  X11DRV_PHYSBITMAP *pbitmap;
  int height, tmpheight;
  INT result;

  descr.dc = dc;

  if (DIB_GetBitmapInfo( &info->bmiHeader, &descr.infoWidth, &height,
			 &descr.infoBpp, &descr.compression ) == -1)
      return 0;

  tmpheight = height;
  if (height < 0) height = -height;
  if (!lines || (startscan >= height))
      return 0;

  if (startscan + lines > height) lines = height - startscan;

  if (descr.infoBpp <= 8)
    {
      descr.colorMap = X11DRV_DIB_BuildColorMap( descr.dc, coloruse,
						 bmp->bitmap.bmBitsPixel,
						 info, &descr.nColorMap );
      if (!descr.colorMap)
        {
	  return 0;
        } 
    } else
      descr.colorMap = 0;

  /* HACK for now */
  if(!bmp->DDBitmap)
    X11DRV_CreateBitmap(hbitmap);

  pbitmap = bmp->DDBitmap->physBitmap;
  
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
  
  EnterCriticalSection( &X11DRV_CritSection );
  result = CALL_LARGE_STACK( X11DRV_DIB_SetImageBits, &descr );
  LeaveCriticalSection( &X11DRV_CritSection );
  
  if (descr.colorMap) HeapFree(GetProcessHeap(), 0, descr.colorMap);

  return result;
}


/*********************************************************************
 *         X11DRV_DIB_GetNearestIndex
 *
 * Helper for X11DRV_DIB_GetDIBits.
 * Returns the nearest colour table index for a given RGB.
 * Nearest is defined by minimizing the sum of the squares.
 */
static INT X11DRV_DIB_GetNearestIndex(BITMAPINFO *info, BYTE r, BYTE g, BYTE b)
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
 *           X11DRV_DIB_GetDIBits
 */
INT X11DRV_DIB_GetDIBits(
  BITMAPOBJ *bmp, DC *dc, UINT startscan, 
  UINT lines, LPVOID bits, BITMAPINFO *info,
  UINT coloruse, HBITMAP hbitmap)
{
  XImage * bmpImage;
  int x, y;
  PALETTEENTRY * palEntry;
  PALETTEOBJ * palette;
  BYTE    *bbits = (BYTE*)bits, *linestart;
  int	dstwidth, yend, xend = bmp->bitmap.bmWidth;
  
  TRACE(bitmap, "%u scanlines of (%i,%i) -> (%i,%i) starting from %u\n",
	lines, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
	(int)info->bmiHeader.biWidth, (int)info->bmiHeader.biHeight,
	startscan );

  if (!(palette = (PALETTEOBJ*)GDI_GetObjPtr( dc->w.hPalette, PALETTE_MAGIC )))
      return 0;

  /* adjust number of scanlines to copy */
  
  if( lines > info->bmiHeader.biHeight )
    lines = info->bmiHeader.biHeight;
  
  yend = startscan + lines;
  if( startscan >= bmp->bitmap.bmHeight ) 
    {
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
	  *bbits |= ( X11DRV_DIB_GetNearestIndex(info,
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
	  *bbits |= ( X11DRV_DIB_GetNearestIndex(info,
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
	  *bbits |= ( X11DRV_DIB_GetNearestIndex( info,
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
	  *bbits++ = X11DRV_DIB_GetNearestIndex( info, 
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
	  *bbits++ = X11DRV_DIB_GetNearestIndex( info,
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
    info->bmiHeader.biSizeImage = DIB_GetDIBImageBytes(
					 info->bmiHeader.biWidth,
					 info->bmiHeader.biHeight,
					 info->bmiHeader.biBitCount );

  if(bbits - (BYTE *)bits > info->bmiHeader.biSizeImage)
    ERR(bitmap, "Buffer overrun. Please investigate.\n");

  info->bmiHeader.biCompression = 0;

  GDI_HEAP_UNLOCK( dc->w.hPalette );
  
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
    INT totalSize = dib->dsBmih.biSizeImage? dib->dsBmih.biSizeImage
                         : dib->dsBm.bmWidthBytes * effHeight;
    DWORD old_prot;

    VirtualProtect(dib->dsBm.bmBits, totalSize, new_prot, &old_prot);
    TRACE(bitmap, "Changed protection from %ld to %ld\n", 
                  old_prot, new_prot);
}

/***********************************************************************
 *           X11DRV_DIB_DoUpdateDIBSection
 */
static void X11DRV_DIB_DoUpdateDIBSection(BITMAPOBJ *bmp, BOOL toDIB)
{
  X11DRV_DIBSECTION *dib = (X11DRV_DIBSECTION *) bmp->dib;
  X11DRV_DIB_SETIMAGEBITS_DESCR descr;
  
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
 *           X11DRV_DIB_FaultHandler
 */
static BOOL X11DRV_DIB_FaultHandler( LPVOID res, LPCVOID addr )
{
  BOOL handled = FALSE;
  BITMAPOBJ *bmp;
  
  bmp = (BITMAPOBJ *)GDI_GetObjPtr( (HBITMAP)res, BITMAP_MAGIC );
  if (!bmp) return FALSE;
  
  if (bmp->dib)
    switch (((X11DRV_DIBSECTION *) bmp->dib)->status)
      {
      case X11DRV_DIB_GdiMod:
	TRACE( bitmap, "called in status DIB_GdiMod\n" );
	X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READWRITE );
	X11DRV_DIB_DoUpdateDIBSection( bmp, TRUE );
	X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READONLY );
	((X11DRV_DIBSECTION *) bmp->dib)->status = X11DRV_DIB_InSync;
	handled = TRUE;
	break;
	
      case X11DRV_DIB_InSync:
	TRACE( bitmap, "called in status X11DRV_DIB_InSync\n" );
	X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READWRITE );
	((X11DRV_DIBSECTION *) bmp->dib)->status = X11DRV_DIB_AppMod;
	handled = TRUE;
	break;
	
      case X11DRV_DIB_AppMod:
	FIXME( bitmap, "called in status X11DRV_DIB_AppMod: "
	       "this can't happen!\n" );
	break;
	
      case X11DRV_DIB_NoHandler:
	FIXME( bitmap, "called in status DIB_NoHandler: "
	       "this can't happen!\n" );
	break;
      }
  
  GDI_HEAP_UNLOCK( (HBITMAP)res );
  return handled;
}

/***********************************************************************
 *           X11DRV_DIB_UpdateDIBSection
 */
void X11DRV_DIB_UpdateDIBSection(DC *dc, BOOL toDIB)
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
      
      switch (((X11DRV_DIBSECTION *) bmp->dib)->status)
        {
        default:
        case X11DRV_DIB_NoHandler:
	  X11DRV_DIB_DoUpdateDIBSection( bmp, FALSE );
	  break;
	  
        case X11DRV_DIB_GdiMod:
	  TRACE( bitmap, "fromDIB called in status X11DRV_DIB_GdiMod\n" );
	  /* nothing to do */
	  break;
	  
        case X11DRV_DIB_InSync:
	  TRACE( bitmap, "fromDIB called in status X11DRV_DIB_InSync\n" );
	  /* nothing to do */
	  break;
	  
        case X11DRV_DIB_AppMod:
	  TRACE( bitmap, "fromDIB called in status X11DRV_DIB_AppMod\n" );
	  X11DRV_DIB_DoUpdateDIBSection( bmp, FALSE );
	  X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READONLY );
	  ((X11DRV_DIBSECTION *) bmp->dib)->status = X11DRV_DIB_InSync;
	  break;
        }
    }
  else
    {
      /* Acknowledge write access to the DIB by GDI functions */
      
      switch (((X11DRV_DIBSECTION *) bmp->dib)->status)
        {
        default:
        case X11DRV_DIB_NoHandler:
	  X11DRV_DIB_DoUpdateDIBSection( bmp, TRUE );
	  break;
	  
        case X11DRV_DIB_GdiMod:
	  TRACE( bitmap, "  toDIB called in status X11DRV_DIB_GdiMod\n" );
	  /* nothing to do */
	  break;
	  
        case X11DRV_DIB_InSync:
	  TRACE( bitmap, "  toDIB called in status X11DRV_DIB_InSync\n" );
	  X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_NOACCESS );
	  ((X11DRV_DIBSECTION *) bmp->dib)->status = X11DRV_DIB_GdiMod;
	  break;
	  
        case X11DRV_DIB_AppMod:
	  FIXME( bitmap, "  toDIB called in status X11DRV_DIB_AppMod: "
		 "this can't happen!\n" );
	  break;
        }
    }

    GDI_HEAP_UNLOCK(dc->w.hBitmap);
}

/***********************************************************************
 *           X11DRV_DIB_CreateDIBSection16
 */
HBITMAP16 X11DRV_DIB_CreateDIBSection16(
  DC *dc, BITMAPINFO *bmi, UINT16 usage,
  SEGPTR *bits, HANDLE section,
  DWORD offset)
{
  HBITMAP res = X11DRV_DIB_CreateDIBSection(dc, bmi, usage, NULL, 
					    section, offset);
  if ( res )
    {
      BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr(res, BITMAP_MAGIC);
      if ( bmp && bmp->dib )
	{
	  DIBSECTION *dib = bmp->dib;
	  INT height = dib->dsBm.bmHeight >= 0 ?
	    dib->dsBm.bmHeight : -dib->dsBm.bmHeight;
	  INT size = dib->dsBmih.biSizeImage ?
	    dib->dsBmih.biSizeImage : dib->dsBm.bmWidthBytes * height;
	  if ( dib->dsBm.bmBits )
	    {
	      ((X11DRV_DIBSECTION *) bmp->dib)->selector = 
		SELECTOR_AllocBlock( dib->dsBm.bmBits, size, 
				     SEGMENT_DATA, FALSE, FALSE );
	    }
	  printf("ptr = %p, size =%d, selector = %04x, segptr = %ld\n",
		 dib->dsBm.bmBits, size, ((X11DRV_DIBSECTION *) bmp->dib)->selector,
		 PTR_SEG_OFF_TO_SEGPTR(((X11DRV_DIBSECTION *) bmp->dib)->selector, 0));
	}
      GDI_HEAP_UNLOCK( res );
      
      if ( bits ) 
	*bits = PTR_SEG_OFF_TO_SEGPTR( ((X11DRV_DIBSECTION *) bmp->dib)->selector, 0 );
    }

    return res;
}


/***********************************************************************
 *           X11DRV_DIB_CreateDIBSection
 */
HBITMAP X11DRV_DIB_CreateDIBSection(
  DC *dc, BITMAPINFO *bmi, UINT usage,
  LPVOID *bits, HANDLE section,
  DWORD offset)
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
      if(dc)
	colorMap = X11DRV_DIB_BuildColorMap( dc, usage, bm.bmBitsPixel,
					     bmi, &nColorMap );
    }

  /* Allocate Memory for DIB and fill structure */
  if (bm.bmBits)
    dib = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(X11DRV_DIBSECTION));
  if (dib)
    {
      dib->dibSection.dsBm = bm;
      dib->dibSection.dsBmih = *bi;
      /* FIXME: dib->dibSection.dsBitfields ??? */
      dib->dibSection.dshSection = section;
      dib->dibSection.dsOffset = offset;
      
      dib->status    = X11DRV_DIB_NoHandler;
      dib->selector  = 0;
      
      dib->nColorMap = nColorMap;
      dib->colorMap  = colorMap;
    }
  
  /* Create Device Dependent Bitmap and add DIB pointer */
  if (dib) 
    {
      res = CreateDIBitmap(dc->hSelf, bi, 0, NULL, bmi, usage);
      if (res)
	{
	  bmp = (BITMAPOBJ *) GDI_GetObjPtr(res, BITMAP_MAGIC);
	  if (bmp)
	    {
	      bmp->dib = (DIBSECTION *) dib;
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
      if (VIRTUAL_SetFaultHandler(bm.bmBits, X11DRV_DIB_FaultHandler, (LPVOID)res))
        {
	  X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READONLY );
	  if (dib) dib->status = X11DRV_DIB_InSync;
        }
    }

  /* Return BITMAP handle and storage location */
  if (res) GDI_HEAP_UNLOCK(res);
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
    XDestroyImage( dib->image );
  
  if (dib->colorMap)
    HeapFree(GetProcessHeap(), 0, dib->colorMap);
  
  if (dib->selector)
    {
      WORD count = (GET_SEL_LIMIT( dib->selector ) >> 16) + 1;
      SELECTOR_FreeBlock( dib->selector, count );
    }
}


#endif /* !defined(X_DISPLAY_MISSING) */



