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
#include "debugtools.h"
#include "dc.h"
#include "color.h"
#include "callback.h"
#include "selectors.h"
#include "global.h"
#include "xmalloc.h" /* for XCREATEIMAGE macro */

#include "ts_xshm.h"
#include <sys/shm.h>
#include <sys/ipc.h>

DECLARE_DEBUG_CHANNEL(bitmap)
DECLARE_DEBUG_CHANNEL(x11drv)

static int bitmapDepthTable[] = { 8, 1, 32, 16, 24, 15, 4, 0 };
static int ximageDepthTable[] = { 0, 0, 0,  0,  0,  0,  0 };

static int XShmErrorFlag = 0;

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
    
    WARN_(bitmap)("(%d): Unsupported depth\n", depth );
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
        ERR_(bitmap)("called with >256 colors!\n");
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

    WARN_(bitmap)("Strange color %08x\n", phys);
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
 *           X11DRV_DIB_GetImageBits_1
 *
 * GetDIBits for a 1-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_1( int lines, BYTE *dstbits,
				       DWORD dstwidth, DWORD srcwidth,
				       RGBQUAD *colors, PALETTEENTRY *srccolors, 
                                XImage *bmpImage )
{
    DWORD x;
    int h;
    BYTE *bits;

       /* 32 bit aligned */
    DWORD linebytes = ((dstwidth + 31) & ~31) / 8;

    if (lines < 0 ) {
        lines = -lines;
	dstbits = dstbits + linebytes * (lines - 1);
	linebytes = -linebytes;
    }

    bits = dstbits;

    switch(bmpImage->depth) {

    case 1:
       /* ==== monochrome bitmap to monochrome dib ==== */
    case 4:
       /* ==== 4 colormap bitmap to monochrome dib ==== */
       if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors)
	 { 
	   PALETTEENTRY val;

	   for (h = lines - 1; h >= 0; h--) {
	     for (x = 0; x < dstwidth; x++) {
	       val = srccolors[XGetPixel(bmpImage, x, h)];
	       if (!(x&7)) *bits = 0;
	       *bits |= (X11DRV_DIB_GetNearestIndex(colors, 2, 
						    val.peRed,
						    val.peGreen, 
						    val.peBlue) << (7 - (x & 7)));
	       if ((x&7)==7) bits++;
	     }
	     bits = (dstbits += linebytes);
	   }
	 }
       else goto notsupported;
       
       break;
      
    case 8:
      /* ==== 8 colormap bitmap to monochrome dib ==== */
      if ( bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors)
      {
	BYTE *srcpixel;
	PALETTEENTRY val;

       for( h = lines- 1; h >= 0; h-- ) {
	  srcpixel = bmpImage->data + (h*bmpImage->bytes_per_line);
	  for( x = 0; x < dstwidth; x++ ) {
	    if (!(x&7)) *bits = 0;
	    val = srccolors[(int)*srcpixel++];
	    *bits |= ( X11DRV_DIB_GetNearestIndex(colors, 2,
						  val.peRed,
						  val.peGreen,
						  val.peBlue) << (7-(x&7)) );
	    if ((x&7)==7) bits++;
	  }
	  bits = (dstbits += linebytes);
	}
      }
      else goto notsupported;

      break;
      
    case 15:
      {
	LPWORD srcpixel;
	WORD val;
	
	/* ==== 555 BGR bitmap to monochrome dib ==== */
	if (bmpImage->red_mask == 0x7c00 && bmpImage->blue_mask == 0x1f)
	{
	  for( h = lines - 1; h >= 0; h--) {
		srcpixel = (LPWORD)(bmpImage->data + h*bmpImage->bytes_per_line);
		for( x = 0; x < dstwidth; x++) {
		  if (!(x&7)) *bits = 0;
		  val = *srcpixel++;
		  *bits |= (X11DRV_DIB_GetNearestIndex( colors, 2, 
							((val >> 7) & 0xf8) |
							((val >> 12) & 0x7),
							((val >> 2) & 0xf8) |
							((val >> 7) & 0x3),
							((val << 3) & 0xf8) |
							((val >> 2) & 0x7) ) << (7-(x&7)) );
		  if ((x&7)==7) bits++;
		}
		bits = (dstbits += linebytes);
	    }
	}
	/* ==== 555 RGB bitmap to monochrome dib ==== */
	else if (bmpImage->red_mask == 0x001f && bmpImage->blue_mask == 0xf800)
	{
	    for( h = lines - 1; h >= 0; h--) 
	    {
		srcpixel = (LPWORD)(bmpImage->data + h*bmpImage->bytes_per_line);
		for( x = 0; x < dstwidth; x++) {
		  if (!(x&1)) *bits = 0;
		  val = *srcpixel++;
		  *bits |= (X11DRV_DIB_GetNearestIndex( colors, 2, 
							((val << 3) & 0xf8) |
							((val >> 2) & 0x7),
							((val >> 2) & 0xf8) |
							((val >> 7) & 0x3),
							((val >> 7) & 0xf8) |
							((val >> 12) &  0x7) ) << (7-(x&7)) );
		  if ((x&7)==7) bits++;
		}
		bits = (dstbits += linebytes);
	    }
        }
	else goto notsupported;
      }
      break;
 
    case 16:
      {
	LPWORD srcpixel;
	WORD val;

	/* ==== 565 BGR bitmap to monochrome dib ==== */
	if (bmpImage->red_mask == 0xf800 && bmpImage->blue_mask == 0x001f) 
	{
	    for( h = lines - 1; h >= 0; h--) 
	    {
		srcpixel = (LPWORD)(bmpImage->data + h*bmpImage->bytes_per_line);
		for( x = 0; x < dstwidth; x++) {
		  if (!(x&7)) *bits = 0;
		  val = *srcpixel++;
		  *bits |= (X11DRV_DIB_GetNearestIndex( colors, 2, 
							((val >> 8) & 0xf8) |
							((val >> 13) & 0x7),
							((val >> 3) & 0xfc) |
							((val >> 9) & 0x3),
							((val << 3) & 0xf8) |
							((val >> 2) & 0x7) ) << (7-(x&7)) );
		  if ((x&7)==7) bits++;
		}
		bits = (dstbits += linebytes);
	    }
	}
	/* ==== 565 RGB bitmap to monochrome dib ==== */
	else if (bmpImage->red_mask == 0x001f && bmpImage->blue_mask == 0xf800)
	{
	    for( h = lines - 1; h >= 0; h--) 
	    {
		srcpixel = (LPWORD)(bmpImage->data + h*bmpImage->bytes_per_line);
		for( x = 0; x < dstwidth; x++) {
		  if (!(x&7)) *bits = 0;
		  val = *srcpixel++;
		  *bits |= (X11DRV_DIB_GetNearestIndex( colors, 2, 
							((val << 3) & 0xf8) |
							((val >> 2) & 0x7),
							((val >> 3) & 0xfc) |
							((val >> 9) & 0x3),
							((val >> 8) & 0xf8) |
							((val >> 13) &  0x7) ) << (7-(x&7)) );
		  if ((x&7)==7) bits++;
		}
		bits = (dstbits += linebytes);
	    }
        }
	else goto notsupported;
      }
      break;
      
    case 24: 
    case 32:
      {
	BYTE *srcpixel;
 	
	/* ==== 24/32 BGR bitmap to monochrome dib ==== */
	if (bmpImage->red_mask == 0xff0000 && bmpImage->blue_mask == 0xff)
	{	 
	  for (h = lines - 1; h >= 0; h--)
	  {
	    srcpixel = bmpImage->data + h*bmpImage->bytes_per_line;
	    for (x = 0; x < dstwidth; x++, srcpixel+=4) {
	      if (!(x&7)) *bits = 0;
	      *bits |= (X11DRV_DIB_GetNearestIndex(colors, 2, srcpixel[2] , srcpixel[1], srcpixel[0]) << (7-(x&7)) );
	      if ((x&7)==7) bits++;
	    }
	    bits = (dstbits += linebytes);
	  }
	} 
	/* ==== 24/32 RGB bitmap to monochrome dib ==== */
	else if (bmpImage->red_mask == 0xff && bmpImage->blue_mask == 0xff0000)
	{
	  for (h = lines - 1; h >= 0; h--)
	  {
	    srcpixel = bmpImage->data + h*bmpImage->bytes_per_line;
	    for (x = 0; x < dstwidth; x++, srcpixel+=4) { 
                       if (!(x & 7)) *bits = 0;
		       *bits |= (X11DRV_DIB_GetNearestIndex(colors, 2, srcpixel[0] , srcpixel[1], srcpixel[2]) << (7-(x&7)) );
                       if ((x & 7) == 7) bits++;
	    }
	    bits = (dstbits += linebytes);
	  }
	}
	else goto notsupported;
      }
      break;

    default: /* ? bit bmp -> monochrome DIB */
    notsupported:
      {
	unsigned long white = (1 << bmpImage->bits_per_pixel) - 1;

	FIXME_(bitmap)("from %d bit bitmap with mask R,G,B %x,%x,%x to 1 bit DIB\n",
                         bmpImage->bits_per_pixel, (int)bmpImage->red_mask, 
                        (int)bmpImage->green_mask, (int)bmpImage->blue_mask );
      
	for( h = lines - 1; h >= 0; h-- ) {
	  for( x = 0; x < dstwidth; x++ ) {
	    if (!(x&7)) *bits = 0;
	    *bits |= (XGetPixel( bmpImage, x, h) >= white) 
	      << (7 - (x&7));
	    if ((x&7)==7) bits++;
	  }
	  bits = (dstbits += linebytes);
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
static void X11DRV_DIB_GetImageBits_4( int lines, BYTE *dstbits,
				       DWORD srcwidth, DWORD dstwidth,
				       RGBQUAD *colors, PALETTEENTRY *srccolors, 
				       XImage *bmpImage )
{
    DWORD x;
    int h;
    BYTE *bits;
    LPBYTE srcpixel;

    /* 32 bit aligned */
    DWORD linebytes = ((srcwidth+7)&~7)/2;

    if (lines < 0 )
    {
       lines = -lines;
       dstbits = dstbits + ( linebytes * (lines-1) );
       linebytes = -linebytes;
    }

    bits = dstbits;

    switch(bmpImage->depth) {

     case 1:
	/* ==== monochrome bitmap to 4 colormap dib ==== */
     case 4:
       /* ==== 4 colormap bitmap to 4 colormap dib ==== */
       if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors)
	 {
	   PALETTEENTRY val;
	   
       for (h = lines-1; h >= 0; h--) {
	     for (x = 0; x < dstwidth; x++) {
	       if (!(x&1)) *bits = 0;
	       val = srccolors[XGetPixel(bmpImage, x, h)];
	       *bits |= (X11DRV_DIB_GetNearestIndex(colors, 16, 
						    val.peRed,
						    val.peGreen, 
						    val.peBlue) << (4-((x&1)<<2)));
	       if ((x&1)==1) bits++;
           }
	     bits = (dstbits += linebytes);
       }
	 }
       else goto notsupported;
       
       break;
      
    case 8:
      /* ==== 8 colormap bitmap to 4 colormap dib ==== */
      if ( bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors)
      {
	PALETTEENTRY val;

	for( h = lines - 1; h >= 0; h-- ) {
	  srcpixel = bmpImage->data + (h*bmpImage->bytes_per_line);
	  for( x = 0; x < dstwidth; x++ ) {
	    if (!(x&1)) *bits = 0;
	    val = srccolors[(int)*srcpixel++];
	    *bits |= ( X11DRV_DIB_GetNearestIndex(colors, 16,
						  val.peRed,
						  val.peGreen,
						  val.peBlue) << (4*(1-(x&1))) );
	    if ((x&1)==1) bits++;
	  }
	  bits = (dstbits += linebytes);
	}
      }
      else goto notsupported;

      break;
      
    case 15:
      {
	LPWORD srcpixel;
	WORD val;
	
	/* ==== 555 BGR bitmap to 4 colormap dib ==== */
	if (bmpImage->red_mask == 0x7c00 && bmpImage->blue_mask == 0x1f)
	{
	  for( h = lines - 1; h >= 0; h--) {
		srcpixel = (LPWORD)(bmpImage->data + h*bmpImage->bytes_per_line);
		for( x = 0; x < dstwidth; x++) {
		  if (!(x&1)) *bits = 0;
		  val = *srcpixel++;
		  *bits |= (X11DRV_DIB_GetNearestIndex( colors, 16, 
							((val >> 7) & 0xf8) |
							((val >> 12) & 0x7),
							((val >> 2) & 0xf8) |
							((val >> 7) & 0x3),
							((val << 3) & 0xf8) |
							((val >> 2) & 0x7) ) << ((1-(x&1))<<2) );
		  if ((x&1)==1) bits++;
		}
		bits = (dstbits += linebytes);
	    }
	}
	/* ==== 555 RGB bitmap to 4 colormap dib ==== */
	else if (bmpImage->red_mask == 0x001f && bmpImage->blue_mask == 0x7c00)
	{
	    for( h = lines - 1; h >= 0; h--) 
	    {
		srcpixel = (LPWORD)(bmpImage->data + h*bmpImage->bytes_per_line);
		for( x = 0; x < dstwidth; x++) {
		  if (!(x&1)) *bits = 0;
		  val = *srcpixel++;
		  *bits |= (X11DRV_DIB_GetNearestIndex( colors, 16, 
							((val << 3) & 0xf8) |
							((val >> 2) & 0x7),
							((val >> 2) & 0xfc) |
							((val >> 7) & 0x3),
							((val >> 7) & 0xf8) |
							((val >> 12) &  0x7) ) << ((1-(x&1))<<2) );
		  if ((x&1)==1) bits++;
		}
		bits = (dstbits += linebytes);
	    }
        }
	else goto notsupported;
      }
      break;
 
    case 16:
      {
	LPWORD srcpixel;
	WORD val;

	/* ==== 565 BGR bitmap to 4 colormap dib ==== */
	if (bmpImage->red_mask == 0xf800 && bmpImage->blue_mask == 0x001f) 
	{
	    for( h = lines - 1; h >= 0; h--) 
	    {
		srcpixel = (LPWORD)(bmpImage->data + h*bmpImage->bytes_per_line);
		for( x = 0; x < dstwidth; x++) {
		  if (!(x&1)) *bits = 0;
		  val = *srcpixel++;
		  *bits |= (X11DRV_DIB_GetNearestIndex( colors, 16, 
							((val >> 8) & 0xf8) |
							((val >> 13) & 0x7),
							((val >> 3) & 0xfc) |
							((val >> 9) & 0x3),
							((val << 3) & 0xf8) |
							((val >> 2) & 0x7) )  << ((1-(x&1))<<2) );
		  if ((x&1)==1) bits++;
		}
		bits = (dstbits += linebytes);
	    }
	}
	/* ==== 565 RGB bitmap to 4 colormap dib ==== */
	else if (bmpImage->red_mask == 0x001f && bmpImage->blue_mask == 0xf800)
	{
	    for( h = lines - 1; h >= 0; h--) 
	    {
		srcpixel = (LPWORD)(bmpImage->data + h*bmpImage->bytes_per_line);
		for( x = 0; x < dstwidth; x++) {
		  if (!(x&1)) *bits = 0;
		  val = *srcpixel++;
		  *bits |= (X11DRV_DIB_GetNearestIndex( colors, 16, 
							((val << 3) & 0xf8) |
							((val >> 2) & 0x7),
							((val >> 3) & 0xfc) |
							((val >> 9) & 0x3),
							((val >> 8) & 0xf8) |
							((val >> 13) &  0x7) ) << ((1-(x&1))<<2) );
		  if ((x&1)==1) bits++;
		}
		bits = (dstbits += linebytes);
	    }
        }
	else goto notsupported;
      }
      break;
      
    case 24: 
    case 32:
      {
	BYTE *srcpixel;
 	
	/* ==== 24/32 BGR bitmap to 4 colormap dib ==== */
	if (bmpImage->red_mask == 0xff0000 && bmpImage->blue_mask == 0xff)
	{	 
	  for (h = lines - 1; h >= 0; h--)
	  {
	    srcpixel = bmpImage->data + h*bmpImage->bytes_per_line;
	    for (x = 0; x < dstwidth; x+=2, srcpixel+=8) /* 2 pixels at a time */
	      *bits++ = (X11DRV_DIB_GetNearestIndex(colors, 16, srcpixel[2] , srcpixel[1], srcpixel[0]) << 4) |
		         X11DRV_DIB_GetNearestIndex(colors, 16, srcpixel[6] , srcpixel[5], srcpixel[4]);
	    bits = (dstbits += linebytes);
	  }
	} 
	/* ==== 24/32 RGB bitmap to 4 colormap dib ==== */
	else if (bmpImage->red_mask == 0xff && bmpImage->blue_mask == 0xff0000)
	{
	  for (h = lines - 1; h >= 0; h--)
	  {
	    srcpixel = bmpImage->data + h*bmpImage->bytes_per_line;
	    for (x = 0; x < dstwidth; x+=2, srcpixel+=8) /* 2 pixels at a time */
	      *bits++ = (X11DRV_DIB_GetNearestIndex(colors, 16, srcpixel[0] , srcpixel[1], srcpixel[2]) << 4) |
		         X11DRV_DIB_GetNearestIndex(colors, 16, srcpixel[4] , srcpixel[5], srcpixel[6]);
	    bits = (dstbits += linebytes);
	  }
	}
	else goto notsupported;
      }
      break;

    default: /* ? bit bmp -> 4 bit DIB */
    notsupported:
      FIXME_(bitmap)("from %d bit bitmap with mask R,G,B %x,%x,%x to 4 bit DIB\n",
                         bmpImage->bits_per_pixel, (int)bmpImage->red_mask, 
                        (int)bmpImage->green_mask, (int)bmpImage->blue_mask );
      for (h = lines-1; h >= 0; h--) {
	for (x = 0; x < dstwidth/2; x++) {
	  *bits++ = (X11DRV_DIB_MapColor((int *)colors, 16, 
					  XGetPixel( bmpImage, x++, h )) << 4)
	    | (X11DRV_DIB_MapColor((int *)colors, 16,
					XGetPixel( bmpImage, x++, h )) & 0x0f);
           }
           if (dstwidth & 1)
	  *bits = (X11DRV_DIB_MapColor((int *)colors, 16,
					XGetPixel( bmpImage, x++, h )) << 4);
	bits = (dstbits += linebytes);
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
		   FIXME_(x11drv)("x-delta is too large?\n");
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
    int h, color;
    const BYTE *bits;

    /* align to 32 bit */
    DWORD linebytes = (srcwidth + 3) & ~3;

    dstwidth += left;

    if (lines < 0 )
    {
        lines = -lines;
        srcbits = srcbits + ( linebytes * (lines-1) );
        linebytes = -linebytes;
    }

    bits = srcbits + left;

    for (h = lines - 1; h >= 0; h--) {
        for (x = left; x < dstwidth; x++, bits++) {
	    color = colors[*bits];	    
            XPutPixel( bmpImage, x, h, colors[*bits] );
        }
        bits = (srcbits += linebytes) + left;
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
				       XImage *bmpImage )
{
    DWORD x;
    int h;
    BYTE *bits;

    /* align to 32 bit */
    DWORD linebytes = (srcwidth + 3) & ~3;

    if (lines < 0 )
    {
       lines = -lines;
       dstbits = dstbits + ( linebytes * (lines-1) );
       linebytes = -linebytes;
    }

    bits = dstbits;

    /* 
       Hack for now 
       This condition is true when GetImageBits has been called by UpdateDIBSection.
       For now, GetNearestIndex is too slow to support 256 colormaps, so we'll just use for
       for GetDIBits calls. (In somes cases, in a updateDIBSection, the returned colors are bad too)
    */
    if (!srccolors) goto updatesection;

    switch(bmpImage->depth) {

    case 1:
	/* ==== monochrome bitmap to 8 colormap dib ==== */
    case 4:
       /* ==== 4 colormap bitmap to 8 colormap dib ==== */
       if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors)
	 {
	   PALETTEENTRY val;

    for (h = lines - 1; h >= 0; h--) {
	     for (x = 0; x < dstwidth; x++) {
	       val = srccolors[XGetPixel(bmpImage, x, h)];
	       *bits++ = X11DRV_DIB_GetNearestIndex(colors, 256, val.peRed,
						    val.peGreen, val.peBlue);
	     }
	     bits = (dstbits += linebytes);
	   }
	 }
       else goto notsupported;
       
       break;
    
     case 8:
       /* ==== 8 colormap bitmap to 8 colormap dib ==== */
       if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors)
	 {
	   BYTE *srcpixel;
	   PALETTEENTRY val;
	   
	   for (h = lines - 1; h >= 0; h--) {
	     srcpixel = bmpImage->data + h*bmpImage->bytes_per_line;
	     for (x = 0; x < dstwidth; x++) {
	       val = srccolors[(int)*srcpixel++];
	       *bits++ = X11DRV_DIB_GetNearestIndex(colors, 256, val.peRed, 
						    val.peGreen, val.peBlue);
	     }
	     bits = (dstbits += linebytes);
	   }
	 }
       else goto notsupported;
       
       break;

      case 15:
      {
	LPWORD srcpixel;
	WORD val;
	
	/* ==== 555 BGR bitmap to 8 colormap dib ==== */
	if (bmpImage->red_mask == 0x7c00 && bmpImage->blue_mask == 0x001f)
	{
	    for( h = lines - 1; h >= 0; h--) 
	    {
		srcpixel = (LPWORD)(bmpImage->data + h*bmpImage->bytes_per_line);
		for( x = 0; x < dstwidth; x++ )
		{
		  val = *srcpixel++;
		  *bits++ = X11DRV_DIB_GetNearestIndex( colors, 256, 
							((val >> 7) & 0xf8) |
							((val >> 12) & 0x7),
							((val >> 2) & 0xf8) |
							((val >> 7) & 0x3),
							((val << 3) & 0xf8) |
							((val >> 2) & 0x7) );
		}
		bits = (dstbits += linebytes);
	    }
	}
	/* ==== 555 RGB bitmap to 8 colormap dib ==== */
	else if (bmpImage->red_mask == 0x001f && bmpImage->blue_mask == 0x7c00)
	{
	    for( h = lines - 1; h >= 0; h--) 
	    {
		srcpixel = (LPWORD)(bmpImage->data + h*bmpImage->bytes_per_line);
		for( x = 0; x < dstwidth; x++ )
		{
		  val = *srcpixel++;
		  *bits++ = X11DRV_DIB_GetNearestIndex( colors, 256, 
							((val << 3) & 0xf8) |
							((val >> 2) & 0x7),
							((val >> 2) & 0xf8) |
							((val >> 7) & 0x3),
							((val >> 7) & 0xf8) |
							((val >> 12) &  0x7) );
		}
		bits = (dstbits += linebytes);
	    }
	}
	else goto notsupported;
      }
      break;

      case 16:
      {
	LPWORD srcpixel;
	WORD val;
	
	/* ==== 565 BGR bitmap to 8 colormap dib ==== */
	if (bmpImage->red_mask == 0xf800 && bmpImage->blue_mask == 0x001f)
	{
	    for( h = lines - 1; h >= 0; h--) 
	    {
		srcpixel = (LPWORD)(bmpImage->data + h*bmpImage->bytes_per_line);
		for( x = 0; x < dstwidth; x++ )
		{
		  val = *srcpixel++;
		  *bits++ = X11DRV_DIB_GetNearestIndex( colors, 256, 
							((val >> 8) & 0xf8) |
							((val >> 13) & 0x7),
							((val >> 3) & 0xfc) |
							((val >> 9) & 0x3),
							((val << 3) & 0xf8) |
							((val >> 2) & 0x7) );
		}
		bits = (dstbits += linebytes);
	    }
	}
	/* ==== 565 RGB bitmap to 8 colormap dib ==== */
	else if (bmpImage->red_mask == 0x001f && bmpImage->blue_mask == 0xf800)
	{
	    for( h = lines - 1; h >= 0; h--) 
	    {
		srcpixel = (LPWORD)(bmpImage->data + h*bmpImage->bytes_per_line);
		for( x = 0; x < dstwidth; x++ )
		{
		  val = *srcpixel++;
		  *bits++ = X11DRV_DIB_GetNearestIndex( colors, 256, 
							((val << 3) & 0xf8) |
							((val >> 2) & 0x7),
							((val >> 3) & 0x00fc) |
							((val >> 9) & 0x3),
							((val >> 8) & 0x00f8) |
							((val >> 13) &  0x7) );
		}
		bits = (dstbits += linebytes);
	    }
	}
	else goto notsupported;
      }
      break;
      
      case 24: 
      case 32:
      {
	BYTE *srcpixel;
	
	/* ==== 24/32 BGR bitmap to 8 colormap dib ==== */
	if (bmpImage->red_mask == 0xff0000 && bmpImage->blue_mask == 0xff)
	{
	  for (h = lines - 1; h >= 0; h--)
	    {
	      srcpixel = bmpImage->data + h*bmpImage->bytes_per_line;
	      for (x = 0; x < dstwidth; x++, srcpixel+=4)
		*bits++ = X11DRV_DIB_GetNearestIndex(colors, 256, 
						     srcpixel[2] , srcpixel[1], *srcpixel);
	      bits = (dstbits += linebytes);
	    }
	}
	/* ==== 24/32 RGB bitmap to 8 colormap dib ==== */
	else if (bmpImage->red_mask == 0xff && bmpImage->blue_mask == 0xff0000)
	{
	  for (h = lines - 1; h >= 0; h--)
	    {
	      srcpixel = bmpImage->data + h*bmpImage->bytes_per_line;
	      for (x = 0; x < dstwidth; x++, srcpixel+=4)
		*bits++ = X11DRV_DIB_GetNearestIndex(colors, 256, 
						     *srcpixel, srcpixel[1], srcpixel[2]);
	      bits = (dstbits += linebytes);
	    }
	  
	}
	else goto notsupported;
      }
      break;

    default: /* ? bit bmp -> 8 bit DIB */
    notsupported:
      FIXME_(bitmap)("from %d bit bitmap with mask R,G,B %x,%x,%x to 8 bit DIB\n",
                         bmpImage->depth, (int)bmpImage->red_mask, 
                        (int)bmpImage->green_mask, (int)bmpImage->blue_mask );
    updatesection:
      for (h = lines - 1; h >= 0; h--) {
	for (x = 0; x < dstwidth; x++, bits++) {
	  *bits = X11DRV_DIB_MapColor((int *)colors, 256,
                                         XGetPixel( bmpImage, x, h ) );
        }
	bits = (dstbits += linebytes);
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
			      TRACE_(bitmap)("Delta to last line of bitmap "
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
	TRACE_(bitmap)("End-of-bitmap "
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
                                       DC *dc, DWORD rSrc, DWORD gSrc, DWORD bSrc,
                                       XImage *bmpImage )
{
    DWORD x;
    int h;
  
    /* align to 32 bit */
    DWORD linebytes = (srcwidth * 2 + 3) & ~3;

    if (lines < 0 )
    {
        lines = -lines;
        srcbits = srcbits + ( linebytes * (lines-1));
        linebytes = -linebytes;
    }
    	
    switch ( bmpImage->depth )
    {
        case 15:
            /* using same format as XImage */
            if (rSrc == bmpImage->red_mask && gSrc == bmpImage->green_mask && bSrc == bmpImage->blue_mask)
                for (h = lines - 1; h >= 0; h--, srcbits += linebytes)
                    memcpy ( bmpImage->data + h * bmpImage->bytes_per_line + left*2, srcbits + left*2, dstwidth*2 );
            else     /* We need to do a conversion from a 565 dib */
            {
                LPDWORD dstpixel, ptr = (LPDWORD)(srcbits + left*2);
                DWORD val;
                int div = dstwidth % 2;

                for (h = lines - 1; h >= 0; h--) {
                    dstpixel = (LPDWORD) (bmpImage->data + h * bmpImage->bytes_per_line + left*2);
                    for (x = 0; x < dstwidth/2; x++) { /* Do 2 pixels at a time */
                        val = *ptr++;
                        *dstpixel++ =   ((val >> 1) & 0x7fe07fe0) | (val & 0x001f001f); /* Red & Green & Blue */
                    }
                    if (div != 0) /* Odd width? */
                        *dstpixel = ((*(WORD *)ptr >> 1) & 0x7fe0) | (*(WORD *)ptr & 0x001f);
                    ptr = (LPDWORD) ((srcbits += linebytes) + left*2);
                }
            }
            break;

        case 16:
            /* using same format as XImage */
            if (rSrc == bmpImage->red_mask && gSrc == bmpImage->green_mask && bSrc == bmpImage->blue_mask)
                for (h = lines - 1; h >= 0; h--, srcbits += linebytes)
                    memcpy ( bmpImage->data + h * bmpImage->bytes_per_line + left*2, srcbits + left*2, dstwidth*2 );
            else     /* We need to do a conversion from a 555 dib */
            {
                LPDWORD dstpixel, ptr = (LPDWORD)(srcbits + left*2);
                DWORD val;
                int div = dstwidth % 2;

                for (h = lines - 1; h >= 0; h--) {
                    dstpixel = (LPDWORD) (bmpImage->data + h * bmpImage->bytes_per_line + left*2);
                    for (x = 0; x < dstwidth/2; x++) { /* Do 2 pixels at a time */
                        val = *ptr++;
                        *dstpixel++ = ((val << 1) & 0xffc0ffc0) | ((val >> 4) & 0x00200020) | /* Red & Green */
			  (val & 0x001f001f);                             /* Blue */
                    }
                    if (div != 0) /* Odd width? */
                        *dstpixel = ((*(WORD *)ptr << 1) & 0xffc0) | ((*(WORD *)ptr >> 4) & 0x0020)
                                    | (*(WORD *)ptr & 0x001f);
                    ptr = (LPDWORD) ((srcbits += linebytes) + left*2);
                }
            }
            break;

        case 24:
        case 32:
            {
                DWORD  *dstpixel;
                LPWORD ptr = (LPWORD)srcbits + left;
                DWORD val;

		/* ==== 555 BGR dib to 24/32 BGR bitmap ==== */
		if (bmpImage->red_mask == 0xff0000 && bmpImage->blue_mask == 0xff)
                {
                for (h = lines - 1; h >= 0; h--) {
                    dstpixel = (DWORD *) (bmpImage->data + h * bmpImage->bytes_per_line + left*4);
                    for (x = 0; x < dstwidth; x++) {

                        val = *ptr++;
                        *dstpixel++ = ((val << 9) & 0xf80000) | ((val << 4) & 0x070000) | /* Red */
			  ((val << 6) & 0x00f800) | ((val << 1) & 0x000700) | /* Green */
			  ((val << 3) & 0x0000f8) | ((val >> 2) & 0x000007);      /* Blue */
                    }
                    ptr = (LPWORD)(srcbits += linebytes) + left;
                }
	    }
		/* ==== 555 BGR dib to 24/32 RGB bitmap ==== */
		else if (bmpImage->red_mask == 0xff && bmpImage->blue_mask == 0xff0000)
                {
		  for (h = lines - 1; h >= 0; h--) {
                    dstpixel = (DWORD *) (bmpImage->data + h * bmpImage->bytes_per_line + left*4);
                    for (x = 0; x < dstwidth; x++) {

                        val = *ptr++;
                        *dstpixel++ = ((val >> 7) & 0x0000f8) | ((val >> 12) & 0x000007) | /* Red */
			  ((val << 6) & 0x00f800) | ((val << 1) & 0x000700) | /* Green */
			  ((val << 19) & 0xf80000) | ((val >> 14) & 0x070000);      /* Blue */
                    }
                    ptr = (LPWORD)(srcbits += linebytes) + left;
		  }
		}

	    }
            break;

        case 1:
        case 4:
        case 8:
            {
                LPWORD ptr = (LPWORD)srcbits + left;
                WORD val;
                int sc1, sc2;

                /* Set color scaling values */
                if ( rSrc == 0x7c00 ) { sc1 = 7; sc2 = 2; }             /* 555 dib */
                else { sc1 = 8; sc2 = 3; }                              /* 565 dib */

	        for (h = lines - 1; h >= 0; h--) {
                    for (x = left; x < dstwidth+left; x++) {
                        val = *ptr++;
		        XPutPixel( bmpImage, x, h,
                                   X11DRV_PALETTE_ToPhysical(dc, RGB(((val & rSrc) >> sc1),  /* Red */
                                                                     ((val & gSrc) >> sc2),  /* Green */
                                                                     ((val & bSrc) << 3)))); /* Blue */
	            }
	            ptr = (LPWORD) (srcbits += linebytes) + left;
	        }
	    }
            break;

        default:
            FIXME_(bitmap)("16 bit DIB %d bit bitmap\n",
                           bmpImage->bits_per_pixel);
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
					PALETTEENTRY *srccolors, XImage *bmpImage )
{
    DWORD x;
    int h;

    /* align to 32 bit */
    DWORD linebytes = (dstwidth * 2 + 3) & ~3;

    if (lines < 0 )
    {
        lines = -lines;
        dstbits = dstbits + ( linebytes * (lines-1));
        linebytes = -linebytes;
    }

    switch ( bmpImage->depth )
    {
        case 15:
            /* ==== 555 BGR bitmap to 555 BGR dib ==== */
            if (bmpImage->red_mask == 0x7c00 && bmpImage->blue_mask == 0x001f)
                for (h = lines - 1; h >= 0; h--, dstbits += linebytes)
                    memcpy( dstbits, bmpImage->data + h*bmpImage->bytes_per_line, srcwidth*2 );

	    /* ==== 555 RGB bitmap to 555 BGR dib ==== */
            else if (bmpImage->red_mask == 0x001f && bmpImage->blue_mask == 0x7c00)
            {
                LPDWORD srcpixel, ptr = (LPDWORD)dstbits;
                DWORD val;
                int div = srcwidth % 2;

                for (h = lines - 1; h >= 0; h--) {
                    srcpixel = (LPDWORD) (bmpImage->data + h * bmpImage->bytes_per_line);
                    for (x = 0; x < srcwidth/2; x++) {  /* Do 2 pixels at a time */
                        val = *srcpixel++;
                        *ptr++ = ((val << 10) & 0xf800f800) | (val & 0x03e003e0) |        /* Red & Green */
			  ((val >> 10) & 0x001f001f);                                     /* Blue */
                    }
                    if (div != 0) /* Odd width? */
                        *ptr = ((*(WORD *)srcpixel << 1) & 0xffc0) | ((*(WORD *)srcpixel >> 4) & 0x0020) |
                               (*(WORD *)srcpixel & 0x001f);
                    ptr = (LPDWORD)(dstbits += linebytes);
               }
           }
	   else goto notsupported;

           break;

       case 16:
           {
	   LPDWORD srcpixel, ptr = (LPDWORD)dstbits;
	   DWORD val;
	   int div = srcwidth % 2;

           /* ==== 565 BGR bitmap to 555 BGR dib ==== */
           if (bmpImage->red_mask == 0xf800 && bmpImage->blue_mask == 0x001f ) 
	   {
               for (h = lines - 1; h >= 0; h--) {
                   srcpixel = (LPDWORD) (bmpImage->data + h * bmpImage->bytes_per_line);
                   for (x = 0; x < srcwidth/2; x++) {  /* Do 2 pixels at a time */
                       val = *srcpixel++;
                       *ptr++ = ((val >> 1) & 0x7fe07fe0) |                    /* Red & Green */
			 (val & 0x001f001f);                                   /* Blue */
                   }
                   if (div != 0) /* Odd width? */
                       *ptr = ((*(WORD *)srcpixel >> 1) & 0x7fe0) | (*(WORD *)srcpixel & 0x001f);
                   ptr = (LPDWORD) (dstbits += linebytes);
               }
	   }
	   /* ==== 565 RGB bitmap to 555 BGR dib ==== */
           else if (bmpImage->red_mask == 0x001f && bmpImage->blue_mask == 0xf800)
           {
               for (h = lines - 1; h >= 0; h--) {
                   srcpixel = (LPDWORD) (bmpImage->data + h * bmpImage->bytes_per_line);
                   for (x = 0; x < srcwidth/2; x++) {  /* Do 2 pixels at a time */
                       val = *srcpixel++;
                       *ptr++ = ((val << 10) & 0x7c007c00) | ((val >> 1) & 0x03e003e0) |   /* Red & Green */
			 ((val >> 11) & 0x001f001f);                                       /* Blue */
                   }
                   if (div != 0) /* Odd width? */
                       *ptr = ((*(WORD *)srcpixel >> 1) & 0x7fe0) | (*(WORD *)srcpixel & 0x001f);
                   ptr = (LPDWORD) (dstbits += linebytes);
               }
               }
	   else goto notsupported;
           }
           break;

       case 24:
       case 32:
           {
               DWORD  *srcpixel;
               LPWORD ptr = (LPWORD)dstbits;
               DWORD val;

	       /* ==== 24/32 BGR bitmap to 555 BGR dib ==== */
	       if (bmpImage->red_mask == 0xff0000 && bmpImage->blue_mask == 0xff)
	       {
               for (h = lines - 1; h >= 0; h--) {
                   srcpixel = (DWORD *) (bmpImage->data + h * bmpImage->bytes_per_line);
                   for (x = 0; x < srcwidth; x++, ptr++) {
                       val = *srcpixel++;
                       *ptr = ((val >> 9) & 0x7c00) |                               /* Red */
			      ((val >> 6) & 0x03e0) |                               /* Green */
			      ((val >> 3) & 0x001f);                                /* Blue */
                   }
                   ptr = (LPWORD)(dstbits += linebytes);
		   }
	       }
	       /* ==== 24/32 RGB bitmap to 555 BGR dib ==== */
	       else if (bmpImage->red_mask == 0xff && bmpImage->blue_mask == 0xff0000)
	       {
		 for (h = lines - 1; h >= 0; h--) {
                   srcpixel = (DWORD *) (bmpImage->data + h * bmpImage->bytes_per_line);
                   for (x = 0; x < srcwidth; x++, ptr++) {
                       val = *srcpixel++;
                       *ptr = ((val << 7) & 0x7c00) |                              /* Red */
			      ((val >> 6) & 0x03e0) |                              /* Green */
			      ((val >> 19) & 0x001f);                              /* Blue */
                   }
                   ptr = (LPWORD) (dstbits += linebytes);
               }
           }
	       else goto notsupported;
           }
           break;

       case 1:
	    /* ==== monochrome bitmap to 16 BGR dib ==== */
       case 4:
	    /* ==== 4 colormap bitmap to 16 BGR dib ==== */
	    if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors)
           {
	        LPWORD ptr = (LPWORD)dstbits;
		PALETTEENTRY val;

                for (h = lines - 1; h >= 0; h--) {
		  for (x = 0; x < dstwidth; x++) {
		    val = srccolors[XGetPixel(bmpImage, x, h)];
		    *ptr++ = ((val.peRed << 7) & 0x7c00) | 
		             ((val.peGreen << 2) & 0x03e0) |
		             ((val.peBlue >> 3) & 0x001f);
		  }
		  ptr = (LPWORD)(dstbits += linebytes);
                }
            }
	    else goto notsupported;

            break;

        case 8:
	    /* ==== 8 colormap bitmap to 16 BGR dib ==== */
	    if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors)
            {
	        LPWORD ptr = (LPWORD)dstbits;
                BYTE *srcpixel;
		PALETTEENTRY val;

               for (h = lines - 1; h >= 0; h--) {
		  srcpixel = bmpImage->data + h*bmpImage->bytes_per_line;
		  for (x = 0; x < dstwidth; x++) {
		    val = srccolors[(int)*srcpixel++];
		    *ptr++ = ((val.peRed << 7) & 0x7c00) | 
		             ((val.peGreen << 2) & 0x03e0) |
		             ((val.peBlue >> 3) & 0x001f);
                   }
		  ptr = (LPWORD)(dstbits += linebytes);
               }
           }
	    else goto notsupported;

           break;

       default:
       notsupported:
	    {
	      BYTE r,g, b;
	      LPWORD ptr = (LPWORD)dstbits;

	      FIXME_(bitmap)("from %d bit bitmap with mask R,G,B %x,%x,%x to 16 bit DIB\n",
			     bmpImage->depth, (int)bmpImage->red_mask, 
			     (int)bmpImage->green_mask, (int)bmpImage->blue_mask );
	      for (h = lines - 1; h >= 0; h--)
		{
		  for (x = 0; x < dstwidth; x++, ptr++)
		    {
		      COLORREF pixel = X11DRV_PALETTE_ToLogical( XGetPixel( bmpImage, x, h ) );
		      r = (BYTE) GetRValue(pixel);
		      g = (BYTE) GetGValue(pixel);
		      b = (BYTE) GetBValue(pixel);
		      *ptr = ( ((r << 7) & 0x7c00) | ((g << 2) & 0x03e0) | ((b >> 3) & 0x001f) );
		    }
		  ptr = (LPWORD) (dstbits += linebytes);
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
				 DC *dc, XImage *bmpImage )
{
    DWORD x;
    int h;
  
    /* align to 32 bit */
    DWORD linebytes = (srcwidth * 3 + 3) & ~3;
      	
    if (lines < 0 )
    {
        lines = -lines;
        srcbits = srcbits + linebytes * (lines - 1);
        linebytes = -linebytes;
    }

    switch ( bmpImage->depth )
    {
        case 24:
        case 32:
            {
                if( bmpImage->blue_mask == 0xff && bmpImage->red_mask == 0xff0000 )  // packed BGR to unpacked BGR
                {
                    DWORD *dstpixel, val, buf;
                    DWORD *ptr = (DWORD *)(srcbits + left*3);
                    BYTE *bits;
                    int div = dstwidth % 4;
                    int divk;

                    for(h = lines - 1; h >= 0; h--)
                    {
                        dstpixel = (DWORD *) (bmpImage->data + h*bmpImage->bytes_per_line + left*4);

                        for (x = 0; x < dstwidth/4; x++) {   // do 3 dwords source, 4 dwords dest at a time
                            buf = *ptr++;
                            *dstpixel++ = buf&0x00ffffff;                  // b1, g1, r1
                            val = (buf >> 24);                             // b2
                            buf = *ptr++;
                            *dstpixel++ = (val | (buf<<8)) &0x00ffffff;    // g2, r2
                            val = (buf >> 16);                             // b3, g3
                            buf = *ptr++;
                            *dstpixel++ = (val | (buf<<16)) &0x00ffffff;   // r3
                            *dstpixel++ = (buf >> 8);                      // b4, g4, r4
                        }
                        for ( divk=div, bits=(BYTE*)ptr; divk>0; divk--, bits+=3 ) // do remainder
                        {
                            *dstpixel++ = *(DWORD*)bits & 0x00ffffff;      // b, g, r
                        }
                        ptr = (DWORD*)((srcbits+=linebytes)+left*3);
                    }
                }
		else if( bmpImage->blue_mask == 0xff0000 && bmpImage->red_mask == 0xff )  // packed BGR to unpacked RGB
                {
                    DWORD *dstpixel, val, buf;
                    DWORD *ptr = (DWORD *)(srcbits + left*3);
                    BYTE *bits;
                    int div = dstwidth % 4;
                    int divk;

                    for(h = lines - 1; h >= 0; h--)
                    {
                        dstpixel = (DWORD *) (bmpImage->data + h*bmpImage->bytes_per_line + left*4);

                        for (x = 0; x < dstwidth/4; x++) {   // do 3 dwords source, 4 dwords dest at a time
                            buf = *ptr++;
                            *dstpixel++ = ((buf&0xff)<<16) | (buf&0xff00) | ((buf&0xff0000)>>16);  // b1, g1, r1
                            val = ((buf&0xff000000)>>8);                                           // b2
                            buf = *ptr++;
                            *dstpixel++ = val | ((buf&0xff)<<8) | ((buf&0xff00)>>8);               // g2, r2
                            val = (buf&0xff0000) | ((buf&0xff000000)>>16);                         // b3, g3
                            buf = *ptr++;
                            *dstpixel++ = val | (buf&0xff);                                        // r3
                            *dstpixel++ = ((buf&0xff00)<<8) | ((buf&0xff0000)>>8) | (buf>>24);     // b4, g4, r4
                        }
                        for ( divk=div, bits=(BYTE*)ptr; divk>0; divk--, bits+=3 ) // do remainder
                        {
                            buf = *(DWORD*)bits;
                            *dstpixel++ = ((buf&0xff)<<16) | (buf&0xff00) | ((buf&0xff0000)>>16);  // b, g, r
                        }
                        ptr = (DWORD*)((srcbits+=linebytes)+left*3);
                    }
                }
                else
                    goto notsupported;
            }
            break;

        case 15:
            {
                if( bmpImage->blue_mask == 0x7c00 && bmpImage->red_mask == 0x1f )   // BGR888 to RGB555
                {
                    DWORD  *ptr = (DWORD *)(srcbits + left*3), val;
                    LPBYTE bits;
                    LPWORD dstpixel;
                    int div = dstwidth % 4;
                    int divk;

                    for (h = lines - 1; h >= 0; h--) {              //Do 4 pixels at a time
                        dstpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line + left*2);
                        for (x = 0; x < dstwidth/4; x++) {
                            *dstpixel++ = (WORD)((((val = *ptr++) << 7) & 0x7c00) | ((val >> 6) & 0x03e0) | ((val >> 19) & 0x1f));
                            *dstpixel++ = (WORD)(((val >> 17) & 0x7c00) | (((val = *ptr++) << 2) & 0x03e0) | ((val >> 11) & 0x1f));
                            *dstpixel++ = (WORD)(((val >> 9) & 0x07c00) | ((val >> 22) & 0x03e0) | (((val = *ptr++) >> 3) & 0x1f));
                            *dstpixel++ = (WORD)(((val >> 1) & 0x07c00) | ((val >> 14) & 0x03e0) | ((val >> 27) & 0x1f));
                        }
                        for (bits = (LPBYTE)ptr, divk=div; divk > 0; divk--, bits+=3)  //dstwidth not divisible by 4?
                            *dstpixel++ = (((WORD)bits[0] << 7) & 0x07c00) |
                                            (((WORD)bits[1] << 2) & 0x03e0) |
                                            (((WORD)bits[2] >> 3) & 0x001f);
                        ptr = (DWORD *)((srcbits += linebytes) + left * 3);
                    }
                }
                else if( bmpImage->blue_mask == 0x1f && bmpImage->red_mask == 0x7c00 )   // BGR888 to BGR555
                {
                    DWORD  *ptr = (DWORD *)(srcbits + left*3), val;
                    LPBYTE bits;
                    LPWORD dstpixel;
                    int div = dstwidth % 4;
                    int divk;

                    for (h = lines - 1; h >= 0; h--) {              //Do 4 pixels at a time
                        dstpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line + left*2);
                        for (x = 0; x < dstwidth/4; x++) {
                            *dstpixel++ = (WORD)((((val = *ptr++) >> 3) & 0x1f) | ((val >> 6) & 0x03e0) | ((val >> 9) & 0x7c00));
                            *dstpixel++ = (WORD)(((val >> 27) & 0x1f) | (((val = *ptr++) << 2) & 0x03e0) | ((val >> 1) & 0x7c00));
                            *dstpixel++ = (WORD)(((val >> 19) & 0x1f) | ((val >> 22) & 0x03e0) | (((val = *ptr++) << 7) & 0x7c00));
                            *dstpixel++ = (WORD)(((val >> 11) & 0x1f) | ((val >> 14) & 0x03e0) | ((val >> 17) & 0x7c00));
                        }
                        for (bits = (LPBYTE)ptr, divk=div; divk > 0; divk--, bits+=3)  //dstwidth not divisible by 4?
                            *dstpixel++ = (((WORD)bits[2] << 7) & 0x07c00) |
                                            (((WORD)bits[1] << 2) & 0x03e0) |
                                            (((WORD)bits[0] >> 3) & 0x001f);
                        ptr = (DWORD *)((srcbits += linebytes) + left * 3);
                    }
                }
                else
                    goto notsupported;
            }
            break;

        case 16:
            {
                DWORD  *ptr = (DWORD *)(srcbits + left*3), val;
                LPBYTE bits;
                LPWORD dstpixel;
                int div = dstwidth % 4;
                int divk;

                if( bmpImage->blue_mask == 0x001f && bmpImage->red_mask == 0xf800 )    // BGR888 to BGR565
                {
                    for (h = lines - 1; h >= 0; h--) {              //Do 4 pixels at a time
                        dstpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line + left*2);
                        for (x = 0; x < dstwidth/4; x++) {
                            *dstpixel++ = (WORD)((((val = *ptr++) >> 3) & 0x1f) | ((val >> 5) & 0x07e0) | ((val >> 8) & 0xf800));
                            *dstpixel++ = (WORD)(((val >> 27) & 0x1f) | (((val = *ptr++) << 3) & 0x07e0) | ((val) & 0xf800));
                            *dstpixel++ = (WORD)(((val >> 19) & 0x1f) | ((val >> 21) & 0x07e0) | (((val = *ptr++) << 8) & 0xf800));
                            *dstpixel++ = (WORD)(((val >> 11) & 0x1f) | ((val >> 13) & 0x07e0) | ((val >> 16) & 0xf800));
                        }
                        for (   bits = (LPBYTE)ptr, divk=div; divk > 0; divk--, bits+=3)  //dstwidth is not divisible by 4?
                            *dstpixel++ = (((WORD)bits[2] << 8) & 0xf800) |
                                            (((WORD)bits[1] << 3) & 0x07e0) |
                                            (((WORD)bits[0] >> 3) & 0x001f);
                        ptr = (DWORD *)((srcbits += linebytes) + left * 3);
                    }
                }
                else if( bmpImage->blue_mask == 0xf800 && bmpImage->red_mask == 0x001f ) // BGR888 to RGB565
                {
                    for (h = lines - 1; h >= 0; h--) {              //Do 4 pixels at a time
                        dstpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line + left*2);
                        for (x = 0; x < dstwidth/4; x++) {
                            *dstpixel++ = (WORD)((((val = *ptr++) << 8) & 0xf800) | ((val >> 5) & 0x07e0) | ((val >> 19) & 0x1f));
                            *dstpixel++ = (WORD)(((val >> 16) & 0xf800) | (((val = *ptr++) << 3) & 0x07e0) | ((val >> 11) & 0x1f));
                            *dstpixel++ = (WORD)(((val >> 8) & 0xf800) | ((val >> 21) & 0x07e0) | (((val = *ptr++) >> 3) & 0x1f));
                            *dstpixel++ = (WORD)((val & 0xf800) | ((val >> 13) & 0x07e0) | ((val >> 27) & 0x1f));
                        }
                        for (   bits = (LPBYTE)ptr, divk=div; divk > 0; divk--, bits+=3)  //dstwidth is not divisible by 4?
                            *dstpixel++ = (((WORD)bits[0] << 8) & 0xf800) |
                                            (((WORD)bits[1] << 3) & 0x07e0) |
                                            (((WORD)bits[2] >> 3) & 0x001f);
                        ptr = (DWORD *)((srcbits += linebytes) + left * 3);
                    }
                }
                else
                    goto notsupported;
            }
            break;
         
        case 1:
        case 4:
        case 8:
            {
                LPBYTE bits = (LPBYTE)srcbits + left*3;

                for (h = lines - 1; h >= 0; h--) {
                    for (x = left; x < dstwidth+left; x++, bits+=3)
                        XPutPixel( bmpImage, x, h, 
                                   X11DRV_PALETTE_ToPhysical(dc, RGB(bits[2], bits[1], bits[0])));
                    bits = (LPBYTE)(srcbits += linebytes) + left * 3;
                }
            }
            break;

        default:
        notsupported:
            FIXME_(bitmap)("from 24 bit DIB to %d bit bitmap with mask R,G,B %x,%x,%x\n",
                            bmpImage->bits_per_pixel, (int)bmpImage->red_mask, 
                            (int)bmpImage->green_mask, (int)bmpImage->blue_mask );
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
					PALETTEENTRY *srccolors, XImage *bmpImage )
{
    DWORD x, val;
    int h;

    /* align to 32 bit */
    DWORD linebytes = (dstwidth * 3 + 3) & ~3;

    if (lines < 0 )
    {
        lines = -lines;
        dstbits = dstbits + ( linebytes * (lines-1) );
        linebytes = -linebytes;
    }

    switch ( bmpImage->depth )
    {
        case 24:
        case 32:
            {
                    DWORD  *srcpixel, buf;
                    LPBYTE bits;
	    DWORD *ptr=(DWORD *)dstbits;
                    int quotient = dstwidth / 4;
                    int remainder = dstwidth % 4;
                    int remk;

	    /* ==== 24/32 BGR bitmap to 24 BGR dib==== */
	    if( bmpImage->blue_mask == 0xff && bmpImage->red_mask == 0xff0000 )
	    {
                    for(h = lines - 1; h >= 0; h--)
                    {
		  srcpixel = (DWORD *) (bmpImage->data + h * bmpImage->bytes_per_line);

                        for (x = 0; x < quotient; x++) {     /* do 4 dwords source, 3 dwords dest at a time*/
			  buf = ((*srcpixel++)&0x00ffffff);      /* b1, g1, r1*/
			  *ptr++ = buf | ((*srcpixel)<<24);      /* b2 */
			  buf = ((*srcpixel++>>8)&0x0000ffff);   /* g2, r2 */
			  *ptr++ = buf | ((*srcpixel)<<16);      /* b3, g3 */
			  buf = ((*srcpixel++>>16)&0x000000ff);  /* r3 */
			  *ptr++ = buf | ((*srcpixel++)<<8);     /* b4, g4, r4 */
                        }
                        for ( remk=remainder, bits=(BYTE*)ptr; remk>0; remk--, bits+=3 ) /* do remainder */
                        {
                            buf=*srcpixel++;
                            *(WORD*)bits = buf;                    /* b, g */
			  *(bits+2) = buf>>16;                   /* r */
                        }
		  ptr = (DWORD*)(dstbits+=linebytes);
                    }

                }
	    /* ==== 24/32 RGB bitmap to 24 BGR dib ==== */
	    else if( bmpImage->blue_mask == 0xff0000 && bmpImage->red_mask == 0xff )
                {
                    for(h = lines - 1; h >= 0; h--)
                    {
		  srcpixel = (DWORD *) (bmpImage->data + h * bmpImage->bytes_per_line);

                        for (x = 0; x < quotient; x++) {     /* do 4 dwords source, 3 dwords dest at a time */
                            buf = *srcpixel++;
                            val = ((buf&0xff0000)>>16) | (buf&0xff00) | ((buf&0xff)<<16);       /* b1, g1, r1 */
                            buf = *srcpixel++;
                            *ptr++ = val | ((buf&0xff0000)<<8);                                 /* b2 */
                            val = ((buf&0xff00)>>8) | ((buf&0xff)<<8);                          /* g2, r2 */
                            buf = *srcpixel++;
                            *ptr++ = val | (buf&0xff0000) | ((buf&0xff00)<<16);                 /* b3, g3 */
                            val = (buf&0xff);                                                   /* r3 */
                            buf = *srcpixel++;
                            *ptr++ = val | ((buf&0xff0000)>>8) | ((buf&0xff00)<<8) | (buf<<24); /* b4, g4, r4 */
                        }
                        for ( remk=remainder, bits=(BYTE*)ptr; remk>0; remk--, bits+=3 ) /* do remainder */
                        {
                            buf=*srcpixel++;
                            *(WORD*)bits = (buf&0xff00) | ((buf&0xff0000)>>16) ;                /* b, g */
                            *(bits+2) = buf;                                                    /* r */
                        }
		  ptr = (DWORD*)(dstbits+=linebytes);
                    }
                }
	    else goto notsupported;
            }
            break;

        case 15:
            {
                LPWORD srcpixel;
	    LPBYTE bits = dstbits;
                WORD val;

	    /* ==== 555 BGR bitmap to 24 BGR dib ==== */
	    if( bmpImage->blue_mask == 0x1f && bmpImage->red_mask == 0x7c00 )  
                {
                    for (h = lines - 1; h >= 0; h--) {
		  srcpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line);
                        for (x = 0; x < srcwidth; x++, bits += 3) {
                            val = *srcpixel++;
                            bits[2] = (BYTE)(((val >> 7) & 0xf8) | ((val >> 12) & 0x07));           /*Red*/
                            bits[1] = (BYTE)(((val >> 2) & 0xf8) | ((val >> 7) & 0x07));            /*Green*/
                            bits[0] = (BYTE)(((val << 3) & 0xf8) | ((val >> 2) & 0x07));            /*Blue*/
			}
		  bits = (dstbits += linebytes);
                    }
                }
	    /* ==== 555 RGB bitmap to 24 RGB dib==== */
	    else if( bmpImage->blue_mask == 0x7c00 && bmpImage->red_mask == 0x1f )
                {
                    for (h = lines - 1; h >= 0; h--) {
		  srcpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line);
                        for (x = 0; x < srcwidth; x++, bits += 3) {
                            val = *srcpixel++;
                            bits[0] = (BYTE)(((val >> 7) & 0xf8) | ((val >> 12) & 0x07));           /*Red*/
                            bits[1] = (BYTE)(((val >> 2) & 0xf8) | ((val >> 7) & 0x07));            /*Green*/
                            bits[2] = (BYTE)(((val << 3) & 0xf8) | ((val >> 2) & 0x07));            /*Blue*/
                        }
		  bits = (dstbits += linebytes);
                    }
                }
	    else goto notsupported;
            }
            break;

        case 16:
            {
                LPWORD srcpixel;
	    LPBYTE bits = dstbits;
                WORD val;

	    /* ==== 565 BGR bitmap to 24 BGR dib ==== */
	    if( bmpImage->blue_mask == 0x1f && bmpImage->red_mask == 0xf800 )
                {
                    for (h = lines - 1; h >= 0; h--) {
		  srcpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line);
                        for (x = 0; x < srcwidth; x++, bits += 3) {
                            val = *srcpixel++;
                            bits[2] = (BYTE)(((val >> 8) & 0xf8) | ((val >> 13) & 0x07));           /*Red*/
                            bits[1] = (BYTE)(((val >> 3) & 0xfc) | ((val >> 9) & 0x03));            /*Green*/
                            bits[0] = (BYTE)(((val << 3) & 0xf8) | ((val >> 2) & 0x07));            /*Blue*/
                        }
		  bits = (dstbits += linebytes);
                    }
                }
	    /* ==== 565 RGB bitmap to 24 BGR dib ==== */
	    else if( bmpImage->blue_mask == 0xf800 && bmpImage->red_mask == 0x1f )
                {
                    for (h = lines - 1; h >= 0; h--) {
		  srcpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line);
                        for (x = 0; x < srcwidth; x++, bits += 3) {
                            val = *srcpixel++;
                            bits[0] = (BYTE)(((val >> 8) & 0xf8) | ((val >> 13) & 0x07));           /*Red*/
                            bits[1] = (BYTE)(((val >> 3) & 0xfc) | ((val >> 9) & 0x03));            /*Green*/
                            bits[2] = (BYTE)(((val << 3) & 0xf8) | ((val >> 2) & 0x07));            /*Blue*/
                        }
		  bits = (dstbits += linebytes);
                    }
                }
	    else  goto notsupported;
	    }
            break;

        case 1:
	    /* ==== monochrome bitmap to 24 BGR dib ==== */
        case 4:
	    /* ==== 4 colormap bitmap to 24 BGR dib ==== */
	    if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors)
            {
		LPBYTE bits = dstbits;
		PALETTEENTRY val;

                for (h = lines - 1; h >= 0; h--) {
		  for (x = 0; x < dstwidth; x++) {
		    val = srccolors[XGetPixel(bmpImage, x, h)];
		    *bits++ = val.peBlue;
		    *bits++ = val.peGreen;
		    *bits++ = val.peRed;
		  }
		  bits = (dstbits += linebytes);
                }
            }
	    else goto notsupported;

            break;

        case 8:
	    /* ==== 8 colormap bitmap to 24 BGR dib ==== */
	    if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask == 0 && srccolors)
            {
                BYTE *srcpixel;
		LPBYTE bits = dstbits;
		PALETTEENTRY val;

                for (h = lines - 1; h >= 0; h--) {
		  srcpixel = bmpImage->data + h*bmpImage->bytes_per_line;
		  for (x = 0; x < dstwidth; x++ ) {
		    val = srccolors[(int)*srcpixel++];
		    *bits++ = val.peBlue;               /*Blue*/
		    *bits++ = val.peGreen;              /*Green*/
		    *bits++ = val.peRed;                /*Red*/
                    }
		  bits = (dstbits += linebytes);
                }
            }
	    else goto notsupported;

            break;

        default:
        notsupported:
	    {
	      LPBYTE bits = dstbits;

            FIXME_(bitmap)("from %d bit bitmap with mask R,G,B %x,%x,%x to 24 bit DIB\n",
			     bmpImage->depth, (int)bmpImage->red_mask, 
                            (int)bmpImage->green_mask, (int)bmpImage->blue_mask );
	      for (h = lines - 1; h >= 0; h--)
		{
		  for (x = 0; x < dstwidth; x++, bits += 3)
		    {
		      COLORREF pixel = X11DRV_PALETTE_ToLogical( XGetPixel( bmpImage, x, h ) );
		      bits[0] = GetBValue(pixel);
		      bits[1] = GetGValue(pixel);
		      bits[2] = GetRValue(pixel);
		    }
		  bits = (dstbits += linebytes);
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
static void X11DRV_DIB_SetImageBits_32( int lines, const BYTE *srcbits,
                                 DWORD srcwidth, DWORD dstwidth, int left,
					DC *dc, XImage *bmpImage )
{
    DWORD x, *ptr;
    int h;
  
    DWORD linebytes = (srcwidth * 4);

    if (lines < 0 )
    {
       lines = -lines;
       srcbits = srcbits + ( linebytes * (lines-1) );
       linebytes = -linebytes;
    }

    ptr = (DWORD *) srcbits + left;

    switch ( bmpImage->depth )
    {
        case 24:
        case 32:
            /* ==== 32 BGR dib to 24/32 BGR bitmap ==== */
            if (bmpImage->red_mask == 0xff0000 && bmpImage->blue_mask == 0xff) {
                for (h = lines - 1; h >= 0; h--, srcbits+=linebytes) {
                    memcpy( bmpImage->data + h * bmpImage->bytes_per_line, srcbits + left*4, dstwidth*4 );
                }
            }

	    /* ==== 32 BGR dib to 24/32 RGB bitmap ==== */
            else if (bmpImage->red_mask == 0xff && bmpImage->blue_mask == 0xff0000)
            {
                DWORD *dstpixel;

                for (h = lines - 1; h >= 0; h--) {
                    dstpixel = (DWORD *) (bmpImage->data + h * bmpImage->bytes_per_line);
                    for (x = 0; x < dstwidth; x++, ptr++) {
                        *dstpixel++ =   ((*ptr << 16) & 0xff0000) | (*ptr & 0xff00) | ((*ptr >> 16) & 0xff);
                    }
                    ptr = (DWORD *) (srcbits += linebytes) + left;
                }
            }
	    else goto notsupported;

            break;

        case 15:
	    /* ==== 32 BGR dib to 555 BGR bitmap ==== */
	    if (bmpImage->red_mask == 0x7c00 && bmpImage->blue_mask == 0x001f)
            {
                LPWORD  dstpixel;

                for (h = lines - 1; h >= 0; h--) {
                    dstpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line);
                    for (x = 0; x < dstwidth; x++, ptr++) {
                        *dstpixel++ = (WORD) (((*ptr >> 9) & 0x7c00) | ((*ptr >> 6) & 0x03e0) | ((*ptr >> 3) & 0x001f));
                    }
                    ptr = (DWORD *) (srcbits += linebytes) + left;
                }
            }
	    /* ==== 32 BGR dib to 555 RGB bitmap ==== */
	    else if (bmpImage->red_mask == 0x001f && bmpImage->blue_mask == 0x7c00)
            {
                LPWORD  dstpixel;

                for (h = lines - 1; h >= 0; h--) {
                    dstpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line);
                    for (x = 0; x < dstwidth; x++, ptr++) {
                        *dstpixel++ = (WORD) (((*ptr << 7) & 0x7c00) | ((*ptr >> 6) & 0x03e0) | ((*ptr >> 19) & 0x001f));
                    }
                    ptr = (DWORD *) (srcbits += linebytes) + left;
                }
            }
	    else goto notsupported;

            break;

        case 16:
	    /* ==== 32 BGR dib to 565 BGR bitmap ==== */
	    if (bmpImage->red_mask == 0xf800 && bmpImage->blue_mask == 0x001f)
            {
                LPWORD  dstpixel;

                for (h = lines - 1; h >= 0; h--) {
                    dstpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line);
                    for (x = 0; x < dstwidth; x++, ptr++) {
                        *dstpixel++ = (WORD) (((*ptr >> 8) & 0xf800) | ((*ptr >> 5) & 0x07e0) | ((*ptr >> 3) & 0x001f));
                    }
                    ptr = (DWORD *) (srcbits += linebytes) + left;
                }
	    }
	    /* ==== 32 BGR dib to 565 RGB bitmap ==== */
	    else if (bmpImage->red_mask == 0x001f && bmpImage->blue_mask == 0xf800)
            {
                LPWORD dstpixel;

                for (h = lines - 1; h >= 0; h--) {
                    dstpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line);
                    for (x = 0; x < dstwidth; x++, ptr++) {
                        *dstpixel++ = (WORD) (((*ptr << 8) & 0xf800) | ((*ptr >> 5) & 0x07e0) | ((*ptr >> 19) & 0x001f));
                    }
                    ptr = (DWORD *) (srcbits += linebytes) + left;
                }
            }
	    else goto notsupported;

            break;

        case 1:
        case 4:
        case 8:
            {
                LPBYTE bits = (LPBYTE)srcbits + left*4;

                for (h = lines - 1; h >= 0; h--) {
                    for (x = left; x < dstwidth+left; x++, bits += 4)
                        XPutPixel( bmpImage, x, h,
                                   X11DRV_PALETTE_ToPhysical(dc, RGB( bits[2],  bits[1], *bits )));
                    bits = (LPBYTE)(srcbits += linebytes) + left * 4;
                }
            }
            break;

       default:
       notsupported:
            FIXME_(bitmap)("32 bit DIB %d bit bitmap\n",
                            bmpImage->bits_per_pixel);
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
					PALETTEENTRY *srccolors, XImage *bmpImage )
{
    DWORD x;
    int h;
    BYTE *bits;

    /* align to 32 bit */
    DWORD linebytes = (srcwidth * 4);

    if (lines < 0 )
    {
        lines = -lines;
        dstbits = dstbits + ( linebytes * (lines-1) );
        linebytes = -linebytes;
    }

    bits = dstbits;

    switch ( bmpImage->depth )
    {
        case 24:
        case 32:
            /* ==== 24/32 BGR bitmap to 32 BGR dib ==== */
            if ( bmpImage->red_mask == 0xff0000 && bmpImage->blue_mask == 0xff )
                for (h = lines - 1; h >= 0; h--, dstbits+=linebytes)
                    memcpy( dstbits, bmpImage->data + h*bmpImage->bytes_per_line, linebytes );

	    /* ==== 24/32 RGB bitmap to 32 BGR dib ==== */
            else if ( bmpImage->red_mask == 0xff && bmpImage->blue_mask == 0xff0000 )
            {
                LPBYTE srcbits;

                for (h = lines - 1; h >= 0; h--) {
                    srcbits = bmpImage->data + h * bmpImage->bytes_per_line;
                    for (x = 0; x < dstwidth; x++, bits+=4, srcbits+=2) {
                        *(bits + 2) = *srcbits++;
                        *(bits + 1) = *srcbits++;
                        *bits = *srcbits;
                    }
                    bits = (dstbits += linebytes);
                }
            }
            else goto notsupported;
            break;

        case 15:
            {
                LPWORD srcpixel;
                WORD val;

	    /* ==== 555 BGR bitmap to 32 BGR dib ==== */
	    if (bmpImage->red_mask == 0x7c00 && bmpImage->blue_mask == 0x001f)
	    {
	      for (h = lines - 1; h >= 0; h--) {
		srcpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line);
		for (x = 0; x < dstwidth; x++, bits+=2) {
		  val = *srcpixel++;
		  *bits++ = (BYTE)(((val << 3) & 0xf8) | ((val >> 2) & 0x03)); /*Blue*/
		  *bits++ = (BYTE)(((val >> 2) & 0xfc) | ((val >> 8) & 0x03)); /*Green*/
		  *bits = (BYTE)(((val >> 7) & 0xf8) | ((val >> 12) & 0x07));  /*Red*/
		}
		bits = (dstbits += linebytes);
	      }
            }
	    /* ==== 555 RGB bitmap to 32 BGR dib ==== */
	    else if (bmpImage->red_mask == 0x001f && bmpImage->blue_mask == 0x7c00)
	    {
                for (h = lines - 1; h >= 0; h--) {
		srcpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line);
		for (x = 0; x < dstwidth; x++, bits+=2) {
                        val = *srcpixel++;
		  *bits++ = (BYTE)(((val >> 7) & 0xf8) | ((val >> 12) & 0x07));/*Blue*/
		  *bits++ = (BYTE)(((val >> 2) & 0xfc) | ((val >> 8) & 0x03)); /*Green*/
		  *bits = (BYTE)(((val << 3) & 0xf8) | ((val >> 2) & 0x03));   /*Red*/
		}
		bits = (dstbits += linebytes);
                    }
                }
	    else goto notsupported;
            }
            break;

        case 16:
            {
                LPWORD srcpixel;
                WORD val;

	    /* ==== 565 BGR bitmap to 32 BGR dib ==== */
	    if (bmpImage->red_mask == 0xf800 && bmpImage->blue_mask == 0x001f)
	    {
                for (h = lines - 1; h >= 0; h--) {
		srcpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line);
		for (x = 0; x < srcwidth; x++, bits+=2) {
                        val = *srcpixel++;
		  *bits++ = (BYTE)(((val << 3) & 0xf8) | ((val >> 2) & 0x03)); /*Blue*/
		  *bits++ = (BYTE)(((val >> 3) & 0xfc) | ((val >> 9) & 0x03)); /*Green*/
		  *bits = (BYTE)(((val >> 8) & 0xf8) | ((val >> 13) & 0x07));  /*Red*/
		}
		bits = (dstbits += linebytes);
	      }
	    }
	    /* ==== 565 RGB bitmap to 32 BGR dib ==== */
	    else if (bmpImage->red_mask == 0xf800 && bmpImage->blue_mask == 0x001f)
	    {
	      for (h = lines - 1; h >= 0; h--) {
		srcpixel = (LPWORD) (bmpImage->data + h * bmpImage->bytes_per_line);
		for (x = 0; x < srcwidth; x++, bits+=2) {
		  val = *srcpixel++;
		  *bits++ = (BYTE)(((val >> 8) & 0xf8) | ((val >> 13) & 0x07));/*Blue*/
		  *bits++ = (BYTE)(((val >> 3) & 0xfc) | ((val >> 9) & 0x03)); /*Green*/
		  *bits = (BYTE)(((val << 3) & 0xf8) | ((val >> 2) & 0x03));   /*Red*/
		}
		bits = (dstbits += linebytes);
                    }
                }
	    else goto notsupported;
            }
            break;

        case 1:
	    /* ==== monochrome bitmap to 32 BGR dib ==== */
        case 4:
	    /* ==== 4 colormap bitmap to 32 BGR dib ==== */
	    if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors)
            {
		PALETTEENTRY val;

                for (h = lines - 1; h >= 0; h--) {
		  for (x = 0; x < dstwidth; x++) {
		    val = srccolors[XGetPixel(bmpImage, x, h)];
		    *bits++ = val.peBlue;
		    *bits++ = val.peGreen;
		    *bits++ = val.peRed;
		    *bits++ = 0;
		  }
		  bits = (dstbits += linebytes);
                }
            }
	    else goto notsupported;

            break;

        case 8:
	    /* ==== 8 colormap bitmap to 32 BGR dib ==== */
	    if (bmpImage->red_mask==0 && bmpImage->green_mask==0 && bmpImage->blue_mask==0 && srccolors)
            {
                BYTE *srcpixel;
		PALETTEENTRY val;

                for (h = lines - 1; h >= 0; h--) {
		  srcpixel = bmpImage->data + h*bmpImage->bytes_per_line;
		  for (x = 0; x < dstwidth; x++) {
		    val = srccolors[(int)*srcpixel++];
		    *bits++ = val.peBlue;               /*Blue*/
		    *bits++ = val.peGreen;              /*Green*/
		    *bits++ = val.peRed;                /*Red*/
		    *bits++ = 0;
                    }
		  bits = (dstbits += linebytes);
                }
            }
	    else goto notsupported;
            break;

        default:
        notsupported:
	    FIXME_(bitmap)("from %d bit bitmap with mask R,G,B %x,%x,%x to 32 bit DIB\n",
			   bmpImage->depth, (int)bmpImage->red_mask, 
			   (int)bmpImage->green_mask, (int)bmpImage->blue_mask );
	    for (h = lines - 1; h >= 0; h--)
	      {
		for (x = 0; x < dstwidth; x++, bits += 4)
		  {
		    COLORREF pixel = X11DRV_PALETTE_ToLogical( XGetPixel( bmpImage, x, h ) );
		    bits[0] = GetBValue(pixel);
		    bits[1] = GetGValue(pixel);
		    bits[2] = GetRValue(pixel);
		  }
		bits = (dstbits += linebytes);
	      }
            break;
    }
}

/***********************************************************************
 *           X11DRV_DIB_SetImageBits
 *
 * Transfer the bits to an X image.
 * Helper function for SetDIBits() and SetDIBitsToDevice().
 * The Xlib critical section must be entered before calling this function.
 */
int X11DRV_DIB_SetImageBits( const X11DRV_DIB_IMAGEBITS_DESCR *descr )
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
				   descr->width, descr->xSrc, (int *)(descr->colorMap),
				   bmpImage );
	break;
    case 4:
        if (descr->compression) {
	    XGetSubImage( display, descr->drawable, descr->xDest, descr->yDest,
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
				       bmpImage );
	break;
    case 8:
        if (descr->compression) {
	    XGetSubImage( display, descr->drawable, descr->xDest, descr->yDest,
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
				       bmpImage );
	break;
    case 15:
    case 16:
	X11DRV_DIB_SetImageBits_16( descr->lines, descr->bits,
				    descr->infoWidth, descr->width,
                                   descr->xSrc, descr->dc,
                                   descr->rMask, descr->gMask, descr->bMask,
                                   bmpImage);
	break;
    case 24:
	X11DRV_DIB_SetImageBits_24( descr->lines, descr->bits,
				    descr->infoWidth, descr->width,
				    descr->xSrc, descr->dc, bmpImage );
	break;
    case 32:
	X11DRV_DIB_SetImageBits_32( descr->lines, descr->bits,
				    descr->infoWidth, descr->width,
                                   descr->xSrc, descr->dc,
                                   bmpImage);
	break;
    default:
        WARN_(bitmap)("(%d): Invalid depth\n", descr->infoBpp );
        break;
    }

    if (descr->useShm)
    {
        XShmPutImage( display, descr->drawable, descr->gc, bmpImage,
                      descr->xSrc, descr->ySrc, descr->xDest, descr->yDest,
                      descr->width, descr->height, FALSE );
        XSync( display, 0 );
    }
    else
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
int X11DRV_DIB_GetImageBits( const X11DRV_DIB_IMAGEBITS_DESCR *descr )
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
          X11DRV_DIB_GetImageBits_1( descr->lines,(LPVOID)descr->bits, 
				     descr->infoWidth, descr->width,
				     descr->colorMap, descr->palentry, 
                                     bmpImage );
       break;

    case 4:
       if (descr->compression)
	   FIXME_(bitmap)("Compression not yet supported!\n");
       else
	   X11DRV_DIB_GetImageBits_4( descr->lines,(LPVOID)descr->bits, 
				      descr->infoWidth, descr->width, 
				      descr->colorMap, descr->palentry, 
				      bmpImage );
       break;

    case 8:
       if (descr->compression)
	   FIXME_(bitmap)("Compression not yet supported!\n");
       else
	   X11DRV_DIB_GetImageBits_8( descr->lines, (LPVOID)descr->bits,
				      descr->infoWidth, descr->width,
				      descr->colorMap, descr->palentry,
				      bmpImage );
       break;
    case 15:
    case 16:
       X11DRV_DIB_GetImageBits_16( descr->lines, (LPVOID)descr->bits,
				   descr->infoWidth,descr->width,
				   descr->palentry, bmpImage );
       break;

    case 24:
       X11DRV_DIB_GetImageBits_24( descr->lines, (LPVOID)descr->bits,
				   descr->infoWidth,descr->width,
				   descr->palentry, bmpImage );
       break;

    case 32:
       X11DRV_DIB_GetImageBits_32( descr->lines, (LPVOID)descr->bits,
				   descr->infoWidth, descr->width,
				   descr->palentry, bmpImage );
       break;

    default:
        WARN_(bitmap)("(%d): Invalid depth\n", descr->infoBpp );
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
    X11DRV_DIB_IMAGEBITS_DESCR descr;
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

    switch (descr.infoBpp)
    {
       case 1:
       case 4:
       case 8:
               descr.colorMap = (RGBQUAD *)X11DRV_DIB_BuildColorMap( 
                                            coloruse == DIB_PAL_COLORS ? dc : NULL, coloruse,
                                            dc->w.bitsPerPixel, info, &descr.nColorMap );
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
               descr.rMask = descr.gMask = descr.bMask = 0;
               descr.colorMap = 0;
               break;

       case 32:
               descr.rMask = (descr.compression == BI_BITFIELDS) ? *(DWORD *)info->bmiColors : 0xff0000;
               descr.gMask = (descr.compression == BI_BITFIELDS) ?  *((DWORD *)info->bmiColors + 1) : 0xff00;
               descr.bMask = (descr.compression == BI_BITFIELDS) ?  *((DWORD *)info->bmiColors + 2) : 0xff;
               descr.colorMap = 0;
               break;
    }

    descr.dc        = dc;
    descr.bits      = bits;
    descr.image     = NULL;
    descr.palentry  = NULL;
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
    descr.useShm    = FALSE;

    EnterCriticalSection( &X11DRV_CritSection );
    result = CALL_LARGE_STACK( X11DRV_DIB_SetImageBits, &descr );
    LeaveCriticalSection( &X11DRV_CritSection );

    if (descr.infoBpp <= 8)
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
  X11DRV_DIB_IMAGEBITS_DESCR descr;
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

  switch (descr.infoBpp)
  {
       case 1:
       case 4:
       case 8:
	       descr.colorMap = (RGBQUAD *)X11DRV_DIB_BuildColorMap(
                        coloruse == DIB_PAL_COLORS ? descr.dc : NULL, coloruse,
                                                          bmp->bitmap.bmBitsPixel,
                                                          info, &descr.nColorMap );
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
               descr.rMask = descr.gMask = descr.bMask = 0;
               descr.colorMap = 0;
               break;

       case 32:
               descr.rMask = (descr.compression == BI_BITFIELDS) ? *(DWORD *)info->bmiColors : 0xff0000;
               descr.gMask = (descr.compression == BI_BITFIELDS) ?  *((DWORD *)info->bmiColors + 1) : 0xff00;
               descr.bMask = (descr.compression == BI_BITFIELDS) ?  *((DWORD *)info->bmiColors + 2) : 0xff;
               descr.colorMap = 0;
               break;

       default: break;
  }

  /* HACK for now */
  if(!bmp->DDBitmap)
    X11DRV_CreateBitmap(hbitmap);

  pbitmap = bmp->DDBitmap->physBitmap;
  
  descr.bits      = bits;
  descr.image     = NULL;
  descr.palentry  = NULL;
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
  descr.useShm    = FALSE;
  
  EnterCriticalSection( &X11DRV_CritSection );
  result = CALL_LARGE_STACK( X11DRV_DIB_SetImageBits, &descr );
  LeaveCriticalSection( &X11DRV_CritSection );
  
  if (descr.colorMap) HeapFree(GetProcessHeap(), 0, descr.colorMap);

  return result;
}

/***********************************************************************
 *           X11DRV_DIB_GetDIBits
 */
INT X11DRV_DIB_GetDIBits(
  BITMAPOBJ *bmp, DC *dc, UINT startscan, 
  UINT lines, LPVOID bits, BITMAPINFO *info,
  UINT coloruse, HBITMAP hbitmap)
{
  X11DRV_DIBSECTION *dib = (X11DRV_DIBSECTION *) bmp->dib;
  X11DRV_DIB_IMAGEBITS_DESCR descr;
  X11DRV_PHYSBITMAP *pbitmap;
  PALETTEOBJ * palette;
  
  TRACE_(bitmap)("%u scanlines of (%i,%i) -> (%i,%i) starting from %u\n",
	lines, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
	(int)info->bmiHeader.biWidth, (int)info->bmiHeader.biHeight,
	startscan );

  if (!(palette = (PALETTEOBJ*)GDI_GetObjPtr( dc->w.hPalette, PALETTE_MAGIC )))
      return 0;

  if( lines > info->bmiHeader.biHeight )  lines = info->bmiHeader.biHeight;
  if( startscan >= bmp->bitmap.bmHeight ) return FALSE;
  
  if (DIB_GetBitmapInfo( &info->bmiHeader, &descr.infoWidth, &descr.lines,
                        &descr.infoBpp, &descr.compression ) == -1)
      return FALSE;

  switch (descr.infoBpp)
  {
      case 1:
      case 4:
      case 8:
      case 24:
          descr.rMask = descr.gMask = descr.bMask = 0;
          break;
      case 15:
      case 16:
          descr.rMask = (descr.compression == BI_BITFIELDS) ? *(DWORD *)info->bmiColors : 0x7c00;
          descr.gMask = (descr.compression == BI_BITFIELDS) ? *((DWORD *)info->bmiColors + 1) : 0x03e0;
          descr.bMask = (descr.compression == BI_BITFIELDS) ? *((DWORD *)info->bmiColors + 2) : 0x001f;
          break;
  
      case 32:
          descr.rMask = (descr.compression == BI_BITFIELDS) ? *(DWORD *)info->bmiColors : 0xff0000;
          descr.gMask = (descr.compression == BI_BITFIELDS) ? *((DWORD *)info->bmiColors + 1) : 0xff00;
          descr.bMask = (descr.compression == BI_BITFIELDS) ? *((DWORD *)info->bmiColors + 2) : 0xff;
          break;
  }

  /* Hack for now */
  if(!bmp->DDBitmap)
    X11DRV_CreateBitmap(hbitmap);

  pbitmap = bmp->DDBitmap->physBitmap;

  descr.dc        = dc;
  descr.palentry  = palette->logpalette.palPalEntry;
  descr.bits      = bits;
  descr.lines     = lines;
  descr.depth     = bmp->bitmap.bmBitsPixel;
  descr.drawable  = pbitmap->pixmap;
  descr.gc        = BITMAP_GC(bmp);
  descr.xSrc      = 0;
  descr.ySrc      = startscan;
  descr.xDest     = 0;
  descr.yDest     = 0;
  descr.width     = bmp->bitmap.bmWidth;
  descr.height    = bmp->bitmap.bmHeight;
  descr.colorMap  = info->bmiColors;

  if (dib)
    descr.useShm = (dib->shminfo.shmid != -1);
  else
    descr.useShm = FALSE;

  EnterCriticalSection( &X11DRV_CritSection );

  descr.image  = (XImage *)CALL_LARGE_STACK( X11DRV_BITMAP_GetXImage, bmp );
  CALL_LARGE_STACK( X11DRV_DIB_GetImageBits, &descr );

  LeaveCriticalSection( &X11DRV_CritSection );
  
  if(info->bmiHeader.biSizeImage == 0) /* Fill in biSizeImage */
      info->bmiHeader.biSizeImage = DIB_GetDIBImageBytes(
					 info->bmiHeader.biWidth,
					 info->bmiHeader.biHeight,
					 info->bmiHeader.biBitCount );

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
    TRACE_(bitmap)("Changed protection from %ld to %ld\n", 
                  old_prot, new_prot);
}

/***********************************************************************
 *           X11DRV_DIB_DoUpdateDIBSection
 */
static void X11DRV_DIB_DoUpdateDIBSection(BITMAPOBJ *bmp, BOOL toDIB)
{
  X11DRV_DIBSECTION *dib = (X11DRV_DIBSECTION *) bmp->dib;
  X11DRV_DIB_IMAGEBITS_DESCR descr;
  
  if (DIB_GetBitmapInfo( &dib->dibSection.dsBmih, &descr.infoWidth, &descr.lines,
			 &descr.infoBpp, &descr.compression ) == -1)
    return;

  descr.dc        = NULL;
  descr.palentry  = NULL;
  descr.image     = dib->image;
  descr.colorMap  = (RGBQUAD *)dib->colorMap;
  descr.nColorMap = dib->nColorMap;
  descr.bits      = dib->dibSection.dsBm.bmBits;
  descr.depth     = bmp->bitmap.bmBitsPixel;
  
  switch (descr.infoBpp)
  {
    case 1:
    case 4:
    case 8:
    case 24:
      descr.rMask = descr.gMask = descr.bMask = 0;
      break;
    case 15:
    case 16:
      descr.rMask = (descr.compression == BI_BITFIELDS) ? dib->dibSection.dsBitfields[0] : 0x7c00;
      descr.gMask = (descr.compression == BI_BITFIELDS) ? dib->dibSection.dsBitfields[1] : 0x03e0;
      descr.bMask = (descr.compression == BI_BITFIELDS) ? dib->dibSection.dsBitfields[2] : 0x001f;
      break;

    case 32:
      descr.rMask = (descr.compression == BI_BITFIELDS) ? dib->dibSection.dsBitfields[0] : 0xff;
      descr.gMask = (descr.compression == BI_BITFIELDS) ? dib->dibSection.dsBitfields[1] : 0xff00;
      descr.bMask = (descr.compression == BI_BITFIELDS) ? dib->dibSection.dsBitfields[2] : 0xff0000;
      break;
  }

  /* Hack for now */
  descr.drawable  = ((X11DRV_PHYSBITMAP *)bmp->DDBitmap->physBitmap)->pixmap;
  descr.gc        = BITMAP_GC(bmp);
  descr.xSrc      = 0;
  descr.ySrc      = 0;
  descr.xDest     = 0;
  descr.yDest     = 0;
  descr.width     = bmp->bitmap.bmWidth;
  descr.height    = bmp->bitmap.bmHeight;
  descr.useShm = (dib->shminfo.shmid != -1);

  if (toDIB)
    {
      TRACE_(bitmap)("Copying from Pixmap to DIB bits\n");
      EnterCriticalSection( &X11DRV_CritSection );
      CALL_LARGE_STACK( X11DRV_DIB_GetImageBits, &descr );
      LeaveCriticalSection( &X11DRV_CritSection );
    }
  else
    {
      TRACE_(bitmap)("Copying from DIB bits to Pixmap\n"); 
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
	TRACE_(bitmap)("called in status DIB_GdiMod\n" );
	X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READWRITE );
	X11DRV_DIB_DoUpdateDIBSection( bmp, TRUE );
	X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READONLY );
	((X11DRV_DIBSECTION *) bmp->dib)->status = X11DRV_DIB_InSync;
	handled = TRUE;
	break;
	
      case X11DRV_DIB_InSync:
	TRACE_(bitmap)("called in status X11DRV_DIB_InSync\n" );
	X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_READWRITE );
	((X11DRV_DIBSECTION *) bmp->dib)->status = X11DRV_DIB_AppMod;
	handled = TRUE;
	break;
	
      case X11DRV_DIB_AppMod:
	FIXME_(bitmap)("called in status X11DRV_DIB_AppMod: "
	       "this can't happen!\n" );
	break;
	
      case X11DRV_DIB_NoHandler:
	FIXME_(bitmap)("called in status DIB_NoHandler: "
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
	  TRACE_(bitmap)("fromDIB called in status X11DRV_DIB_GdiMod\n" );
	  /* nothing to do */
	  break;
	  
        case X11DRV_DIB_InSync:
	  TRACE_(bitmap)("fromDIB called in status X11DRV_DIB_InSync\n" );
	  /* nothing to do */
	  break;
	  
        case X11DRV_DIB_AppMod:
	  TRACE_(bitmap)("fromDIB called in status X11DRV_DIB_AppMod\n" );
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
	  TRACE_(bitmap)("  toDIB called in status X11DRV_DIB_GdiMod\n" );
	  /* nothing to do */
	  break;
	  
        case X11DRV_DIB_InSync:
	  TRACE_(bitmap)("  toDIB called in status X11DRV_DIB_InSync\n" );
	  X11DRV_DIB_DoProtectDIBSection( bmp, PAGE_NOACCESS );
	  ((X11DRV_DIBSECTION *) bmp->dib)->status = X11DRV_DIB_GdiMod;
	  break;
	  
        case X11DRV_DIB_AppMod:
	  FIXME_(bitmap)("  toDIB called in status X11DRV_DIB_AppMod: "
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
	  TRACE_(bitmap)("ptr = %p, size =%d, selector = %04x, segptr = %ld\n",
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
 *           X11DRV_XShmErrorHandler
 *
 */
static int XShmErrorHandler(Display *dpy, XErrorEvent *event) 
{
    XShmErrorFlag = 1;
    return 0;
}

/***********************************************************************
 *           X11DRV_XShmCreateImage
 *
 */

extern BOOL X11DRV_XShmCreateImage(XImage** image, int width, int height, int bpp,
		                                   XShmSegmentInfo* shminfo)
{
    int (*WineXHandler)(Display *, XErrorEvent *);
     
    *image = TSXShmCreateImage(display, DefaultVisualOfScreen(X11DRV_GetXScreen()),
                                bpp, ZPixmap, NULL, shminfo, width, height);
    if( *image != NULL ) 
    {
        EnterCriticalSection( &X11DRV_CritSection );
        shminfo->shmid = shmget(IPC_PRIVATE, (*image)->bytes_per_line * height,
                                  IPC_CREAT|0700);
        if( shminfo->shmid != -1 )
        {
            shminfo->shmaddr = (*image)->data = shmat(shminfo->shmid, 0, 0);
            if( shminfo->shmaddr != (char*)-1 )
            {
                shminfo->readOnly = FALSE;
                if( TSXShmAttach( display, shminfo ) != 0)
                {
		  /* Reset the error flag */
                    XShmErrorFlag = 0;
                    WineXHandler = XSetErrorHandler(XShmErrorHandler);
                    XSync( display, 0 );

                    if (!XShmErrorFlag)
                    {
                        XSetErrorHandler(WineXHandler);
                        LeaveCriticalSection( &X11DRV_CritSection );

                        return TRUE; /* Success! */
                    }    
                    /* An error occured */
                    XShmErrorFlag = 0;
                    XSetErrorHandler(WineXHandler);
                }
                shmdt(shminfo->shmaddr);
            }
            shmctl(shminfo->shmid, IPC_RMID, 0);
        }        
        XFlush(display);
        XDestroyImage(*image);
        LeaveCriticalSection( &X11DRV_CritSection );
    }
    return FALSE;
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
  
  TRACE_(bitmap)("format (%ld,%ld), planes %d, bpp %d, size %ld, colors %ld (%s)\n",
	bi->biWidth, bi->biHeight, bi->biPlanes, bi->biBitCount,
	bi->biSizeImage, bi->biClrUsed, usage == DIB_PAL_COLORS? "PAL" : "RGB");
  
  effHeight = bi->biHeight >= 0 ? bi->biHeight : -bi->biHeight;
  bm.bmType = 0;
  bm.bmWidth = bi->biWidth;
  bm.bmHeight = effHeight;
  bm.bmWidthBytes = DIB_GetDIBWidthBytes(bm.bmWidth, bi->biBitCount);
  bm.bmPlanes = bi->biPlanes;
  bm.bmBitsPixel = bi->biBitCount;
  bm.bmBits = NULL;
  
  /* Get storage location for DIB bits */
  totalSize = bi->biSizeImage? bi->biSizeImage : bm.bmWidthBytes * effHeight;
  
  if (section)
    bm.bmBits = MapViewOfFile(section, FILE_MAP_ALL_ACCESS, 
			      0L, offset, totalSize);
  else
    bm.bmBits = VirtualAlloc(NULL, totalSize, 
			     MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  
  /* Create Color Map */
  if (bm.bmBits && bm.bmBitsPixel <= 8)
      colorMap = X11DRV_DIB_BuildColorMap( usage == DIB_PAL_COLORS? dc : NULL, 
				usage, bm.bmBitsPixel, bmi, &nColorMap );

  /* Allocate Memory for DIB and fill structure */
  if (bm.bmBits)
    dib = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(X11DRV_DIBSECTION));
  if (dib)
    {
      dib->dibSection.dsBm = bm;
      dib->dibSection.dsBmih = *bi;

      /* Set dsBitfields values */
       if ( usage == DIB_PAL_COLORS || bi->biBitCount <= 8)
       {
           dib->dibSection.dsBitfields[0] = dib->dibSection.dsBitfields[1] = dib->dibSection.dsBitfields[2] = 0;
       }
       else switch( bi->biBitCount )
       {
           case 16:
               dib->dibSection.dsBitfields[0] = (bi->biCompression == BI_BITFIELDS) ? *(DWORD *)bmi->bmiColors : 0x7c00;
               dib->dibSection.dsBitfields[1] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 1) : 0x03e0;
               dib->dibSection.dsBitfields[2] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 2) : 0x001f;
               break;

           case 24:
               dib->dibSection.dsBitfields[0] = 0xff;
               dib->dibSection.dsBitfields[1] = 0xff00;
               dib->dibSection.dsBitfields[2] = 0xff0000;
               break;

           case 32:
               dib->dibSection.dsBitfields[0] = (bi->biCompression == BI_BITFIELDS) ? *(DWORD *)bmi->bmiColors : 0xff;
               dib->dibSection.dsBitfields[1] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 1) : 0xff00;
               dib->dibSection.dsBitfields[2] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 2) : 0xff0000;
               break;
       }
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
  {
      if (TSXShmQueryExtension(display) &&
          X11DRV_XShmCreateImage( &dib->image, bm.bmWidth, effHeight,
                                  bmp->bitmap.bmBitsPixel, &dib->shminfo ) )
      {
	; /* Created Image */
      } else {
          XCREATEIMAGE( dib->image, bm.bmWidth, effHeight, bmp->bitmap.bmBitsPixel );
          dib->shminfo.shmid = -1;
      }
  }
  
  /* Clean up in case of errors */
  if (!res || !bmp || !dib || !bm.bmBits || (bm.bmBitsPixel <= 8 && !colorMap))
    {
      TRACE_(bitmap)("got an error res=%08x, bmp=%p, dib=%p, bm.bmBits=%p\n",
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
  {
      if (dib->shminfo.shmid != -1)
      {
          TSXShmDetach (display, &(dib->shminfo));
          XDestroyImage (dib->image);
          shmdt (dib->shminfo.shmaddr);
          shmctl (dib->shminfo.shmid, IPC_RMID, 0);
          dib->shminfo.shmid = -1;
      }
      else
          XDestroyImage( dib->image );
  }
  
  if (dib->colorMap)
    HeapFree(GetProcessHeap(), 0, dib->colorMap);
  
  if (dib->selector)
    {
      WORD count = (GET_SEL_LIMIT( dib->selector ) >> 16) + 1;
      SELECTOR_FreeBlock( dib->selector, count );
    }
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
        TRACE_(bitmap)("\tCould not create bitmap header for Pixmap\n");
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
        /* Manually free the DDBitmap internals to prevent the Pixmap 
         * from being deleted by DeleteObject.
         */
        HeapFree( GetProcessHeap(), 0, pBmp->DDBitmap->physBitmap );
        HeapFree( GetProcessHeap(), 0, pBmp->DDBitmap );
        pBmp->DDBitmap = NULL;
    }
    DeleteObject(hBmp);  
    
END:
    TRACE_(bitmap)("\tReturning packed DIB %x\n", hPackedDIB);
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

    TRACE_(bitmap)("CreateDIBitmap returned %x\n", hBmp);

    /* Retrieve the internal Pixmap from the DDB */
     
    pBmp = (BITMAPOBJ *) GDI_GetObjPtr( hBmp, BITMAP_MAGIC );

    if (pBmp->DDBitmap && pBmp->DDBitmap->physBitmap)
    {
        pixmap = ((X11DRV_PHYSBITMAP *)(pBmp->DDBitmap->physBitmap))->pixmap;
        if (!pixmap)
            TRACE_(bitmap)("NULL Pixmap in DDBitmap->physBitmap!\n");
        
        /* Manually free the BITMAPOBJ internals so that we can steal its pixmap */
        HeapFree( GetProcessHeap(), 0, pBmp->DDBitmap->physBitmap );
        HeapFree( GetProcessHeap(), 0, pBmp->DDBitmap );
        pBmp->DDBitmap = NULL;   /* Its not a DDB anymore */
    }

    /* Delete the DDB we created earlier now that we have stolen its pixmap */
    DeleteObject(hBmp);
    
    TRACE_(bitmap)("\tReturning Pixmap %ld\n", pixmap);
    return pixmap;
}

#endif /* !defined(X_DISPLAY_MISSING) */



