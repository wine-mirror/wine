/*
 * GDI device independent bitmaps
 *
 * Copyright 1993 Alexandre Julliard
 *
static char Copyright[] = "Copyright  Alexandre Julliard, 1993";
*/
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "dc.h"
#include "gdi.h"
#include "bitmap.h"
#include "icon.h"
#include "stddebug.h"
#include "color.h"
#include "debug.h"

static int get_bpp(int depth)
{
	switch(depth) {
		case 4:
		case 8:	
			return 1;
		case 15:
		case 16:
			return 2;
		case 24:
			return 4;
		default:
			fprintf(stderr, "DIB: unsupported depth %d!\n", depth);
			exit(1);
	}		
}

/***********************************************************************
 *           DIB_BitmapInfoSize
 *
 * Return the size of the bitmap info structure.
 */
int DIB_BitmapInfoSize( BITMAPINFO * info, WORD coloruse )
{
    int size = info->bmiHeader.biClrUsed;
    if (!size && (info->bmiHeader.biBitCount != 24))
	size = 1 << info->bmiHeader.biBitCount;
    if (coloruse == DIB_RGB_COLORS) 
	size = info->bmiHeader.biSize + size * sizeof(RGBQUAD);
    else
	size = info->bmiHeader.biSize + size * sizeof(WORD);
    return size;
}


/***********************************************************************
 *           DIB_DIBmpToImage
 *
 * Create an XImage pointing to the bitmap data.
 */
static XImage *DIB_DIBmpToImage( BITMAPINFOHEADER * bmp, void * bmpData )
{
    extern void _XInitImageFuncPtrs( XImage* );
    XImage * image;
    int bytesPerLine = bmp->biWidth * get_bpp(bmp->biBitCount);

    image = XCreateImage( display, DefaultVisualOfScreen( screen ),
			  bmp->biBitCount, ZPixmap, 0, bmpData,
			  bmp->biWidth, bmp->biHeight, 32, bytesPerLine );
    if (!image) return 0;
    image->byte_order = MSBFirst;
    image->bitmap_bit_order = MSBFirst;
    image->bitmap_unit = 16;
    _XInitImageFuncPtrs(image);
    return image;
}


/***********************************************************************
 *           DIB_SetImageBits_1
 *
 * SetDIBits for a 1-bit deep DIB.
 */
static void DIB_SetImageBits_1( WORD lines, BYTE *bits, WORD width,
                                int *colors, XImage *bmpImage )
{
    WORD i, x;
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
static void DIB_SetImageBits_4( WORD lines, BYTE *bits, WORD width,
                                int *colors, XImage *bmpImage )
{
    WORD i, x;
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
static void DIB_SetImageBits_RLE4( WORD lines, BYTE *bits, WORD width,
                                   int *colors, XImage *bmpImage )
{
	int x = 0, c, length;
	BYTE *begin = bits;
	
	lines--;
	while (1) {
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
static void DIB_SetImageBits_8( WORD lines, BYTE *bits, WORD width,
                                int *colors, XImage *bmpImage )
{
    WORD x;
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
  
static void DIB_SetImageBits_RLE8(WORD lines, 
				  BYTE *bits, 
				  WORD width,
				  int *colors, 
				  XImage *bmpImage)
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
			  line=0; /* Cause exit from do loop. */
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
					     "(wrongly??) causes loop exit\n");
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
			    (*pIn++); /* Get and throw away the pad byte. */
			  break;
		      }
		  } /* switch (escape_code) : Escape sequence */
	    }  /* process either an encoded sequence or an escape sequence */
	  
	  /* We expect to come here more than once per line. */
      } while (line > 0);  /* Do this until the bitmap is filled */
    
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
static void DIB_SetImageBits_24( WORD lines, BYTE *bits, WORD width,
				 DC *dc, XImage *bmpImage )
{
    WORD x;
    BYTE pad = (4 - ((width*3) & 3)) & 3;

    while (lines--)
    {
	for (x = 0; x < width; x++, bits += 3)
	{
	    XPutPixel( bmpImage, x, lines,
		       COLOR_ToPhysical( dc, RGB(bits[0],bits[1],bits[2]) ));
	}
	bits += pad;
    }
}


/***********************************************************************
 *           DIB_SetImageBits
 *
 * Transfer the bits to an X image.
 * Helper function for SetDIBits() and SetDIBitsToDevice().
 */
static int DIB_SetImageBits( DC *dc, WORD lines, WORD depth, LPSTR bits,
			     BITMAPINFO *info, WORD coloruse,
			     Drawable drawable, GC gc, int xSrc, int ySrc,
			     int xDest, int yDest, int width, int height )
{
    int *colorMapping;
    XImage *bmpImage;
    void *bmpData;
    int i, colors, widthBytes;

      /* Build the color mapping table */

    if (info->bmiHeader.biBitCount == 24) colorMapping = NULL;
    else
    {
	colors = info->bmiHeader.biClrUsed;
	if (!colors) colors = 1 << info->bmiHeader.biBitCount;
	if (!(colorMapping = (int *)malloc( colors * sizeof(int) )))
	    return 0;
	if (coloruse == DIB_RGB_COLORS)
	{
	    RGBQUAD * rgbPtr = info->bmiColors;
	    for (i = 0; i < colors; i++, rgbPtr++)
		colorMapping[i] = COLOR_ToPhysical( dc, RGB(rgbPtr->rgbRed,
							    rgbPtr->rgbGreen,
							    rgbPtr->rgbBlue) );
	}
	else
	{
	    WORD * index = (WORD *)info->bmiColors;
	    for (i = 0; i < colors; i++, index++)
		colorMapping[i] = COLOR_ToPhysical( dc, PALETTEINDEX(*index) );
	}
    }

      /* Transfer the pixels */
    widthBytes = info->bmiHeader.biWidth * get_bpp(depth);

    bmpData  = malloc( lines * widthBytes );
    bmpImage = XCreateImage( display, DefaultVisualOfScreen(screen),
			     depth, ZPixmap, 0, bmpData,
			     info->bmiHeader.biWidth, lines, 32, widthBytes );

    switch(info->bmiHeader.biBitCount)
    {
    case 1:
	DIB_SetImageBits_1( lines, bits, info->bmiHeader.biWidth,
			    colorMapping, bmpImage );
	break;
    case 4:
	if (info->bmiHeader.biCompression)
		DIB_SetImageBits_RLE4( lines, bits, info->bmiHeader.biWidth,
			    colorMapping, bmpImage );
	else	
		DIB_SetImageBits_4( lines, bits, info->bmiHeader.biWidth,
			    colorMapping, bmpImage );
	break;
    case 8:
	if (info->bmiHeader.biCompression)
		DIB_SetImageBits_RLE8( lines, bits, info->bmiHeader.biWidth,
			    colorMapping, bmpImage );
	else
		DIB_SetImageBits_8( lines, bits, info->bmiHeader.biWidth,
			    colorMapping, bmpImage );
	break;
    case 24:
	DIB_SetImageBits_24( lines, bits, info->bmiHeader.biWidth,
			     dc, bmpImage );
	break;
    }
    if (colorMapping) free(colorMapping);

    XPutImage( display, drawable, gc, bmpImage, xSrc, ySrc,
	       xDest, yDest, width, height );
    XDestroyImage( bmpImage );
    return lines;
}


/***********************************************************************
 *           SetDIBits    (GDI.440)
 */
int SetDIBits( HDC hdc, HBITMAP hbitmap, WORD startscan, WORD lines,
	       LPSTR bits, BITMAPINFO * info, WORD coloruse )
{
    DC * dc;
    BITMAPOBJ * bmp;

      /* Check parameters */

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    if (!(bmp = (BITMAPOBJ *)GDI_GetObjPtr( hbitmap, BITMAP_MAGIC )))
	return 0;
    if (!lines || (startscan >= (WORD)info->bmiHeader.biHeight)) return 0;
    if (startscan+lines > info->bmiHeader.biHeight)
	lines = info->bmiHeader.biHeight - startscan;

    return DIB_SetImageBits( dc, lines, bmp->bitmap.bmBitsPixel,
			     bits, info, coloruse, bmp->pixmap, BITMAP_GC(bmp),
			     0, 0, 0, startscan, bmp->bitmap.bmWidth, lines );
}


/***********************************************************************
 *           SetDIBitsToDevice    (GDI.443)
 */
int SetDIBitsToDevice( HDC hdc, short xDest, short yDest, WORD cx, WORD cy,
		       WORD xSrc, WORD ySrc, WORD startscan, WORD lines,
		       LPSTR bits, BITMAPINFO * info, WORD coloruse )
{
    DC * dc;

      /* Check parameters */

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    if (!lines || (startscan >= info->bmiHeader.biHeight)) return 0;
    if (startscan+lines > info->bmiHeader.biHeight)
	lines = info->bmiHeader.biHeight - startscan;
    if (ySrc < startscan) ySrc = startscan;
    else if (ySrc >= startscan+lines) return 0;
    if (xSrc >= info->bmiHeader.biWidth) return 0;
    if (ySrc+cy >= startscan+lines) cy = startscan + lines - ySrc;
    if (xSrc+cx >= info->bmiHeader.biWidth) cx = info->bmiHeader.biWidth-xSrc;
    if (!cx || !cy) return 0;

    DC_SetupGCForText( dc );  /* To have the correct colors */
    XSetFunction( display, dc->u.x.gc, DC_XROPfunction[dc->w.ROPmode-1] );
    return DIB_SetImageBits( dc, lines, dc->w.bitsPerPixel,
			     bits, info, coloruse,
			     dc->u.x.drawable, dc->u.x.gc,
			     xSrc, ySrc - startscan,
			     dc->w.DCOrgX + XLPTODP( dc, xDest ),
			     dc->w.DCOrgY + YLPTODP( dc, yDest ),
			     cx, cy );
}


/***********************************************************************
 *           GetDIBits    (GDI.441)
 */
int GetDIBits( HDC hdc, HBITMAP hbitmap, WORD startscan, WORD lines,
	       LPSTR bits, BITMAPINFO * info, WORD coloruse )
{
    DC * dc;
    BITMAPOBJ * bmp;
    PALETTEENTRY * palEntry;
    PALETTEOBJ * palette;
    XImage * bmpImage, * dibImage;
    int i, x, y;
        
    if (!lines) return 0;
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
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
    
      /* Transfer the pixels (very slow...) */

    if (bits)
    {	
	bmpImage = XGetImage( display, bmp->pixmap, 0, 0, bmp->bitmap.bmWidth,
			      bmp->bitmap.bmHeight, AllPlanes, ZPixmap );
	dibImage = DIB_DIBmpToImage( &info->bmiHeader, bits );

	for (y = 0; y < lines; y++)
	{
	    for (x = 0; x < info->bmiHeader.biWidth; x++)
	    {
		XPutPixel( dibImage, x, y,
		  XGetPixel(bmpImage, x, bmp->bitmap.bmHeight-startscan-y-1) );
		
	    }
	}
	
	dibImage->data = NULL;
	XDestroyImage( dibImage );
	XDestroyImage( bmpImage );
    }
    return lines;
}


/***********************************************************************
 *           CreateDIBitmap    (GDI.442)
 */
HBITMAP CreateDIBitmap( HDC hdc, BITMAPINFOHEADER * header, DWORD init,
		        LPSTR bits, BITMAPINFO * data, WORD coloruse )
{
    HBITMAP handle;
    
    handle = CreateCompatibleBitmap( hdc, header->biWidth, header->biHeight );
    if (!handle) return 0;
    if (init == CBM_INIT) SetDIBits( hdc, handle, 0, header->biHeight,
				    bits, data, coloruse );
    return handle;
}

/***********************************************************************
 *           DrawIcon    (USER.84)
 */
BOOL DrawIcon(HDC hDC, short x, short y, HICON hIcon)
{
    ICONALLOC	*lpico;
    BITMAP	bm;
    HBITMAP	hBitTemp;
    HDC		hMemDC;

    dprintf_icon(stddeb,"DrawIcon(%04X, %d, %d, %04X) \n", hDC, x, y, hIcon);
    if (hIcon == (HICON)NULL) return FALSE;
    lpico = (ICONALLOC *)GlobalLock(hIcon);
    GetObject(lpico->hBitmap, sizeof(BITMAP), (LPSTR)&bm);
    dprintf_icon(stddeb,"DrawIcon / x=%d y=%d\n", x, y);
    dprintf_icon(stddeb,"DrawIcon / icon Width=%d\n", 
		 (int)lpico->descriptor.Width);
    dprintf_icon(stddeb,"DrawIcon / icon Height=%d\n", 
		 (int)lpico->descriptor.Height);
    dprintf_icon(stddeb,"DrawIcon / icon ColorCount=%d\n", 
		 (int)lpico->descriptor.ColorCount);
    dprintf_icon(stddeb,"DrawIcon / icon icoDIBSize=%lX\n", 
		 (DWORD)lpico->descriptor.icoDIBSize);
    dprintf_icon(stddeb,"DrawIcon / icon icoDIBOffset=%lX\n", 
		 (DWORD)lpico->descriptor.icoDIBOffset);
    dprintf_icon(stddeb,"DrawIcon / bitmap bmWidth=%d bmHeight=%d\n", 
		 bm.bmWidth, bm.bmHeight);
    hMemDC = CreateCompatibleDC(hDC);
    if(debugging_icon){
    hBitTemp = SelectObject(hMemDC, lpico->hBitmap);
    BitBlt(hDC, x, y, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
    SelectObject(hMemDC, lpico->hBitMask);
    BitBlt(hDC, x, y + bm.bmHeight, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
    }else{
    hBitTemp = SelectObject(hMemDC, lpico->hBitMask);
    BitBlt(hDC, x, y, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCAND);
    SelectObject(hMemDC, lpico->hBitmap);
    BitBlt(hDC, x, y, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCPAINT);
    }
    SelectObject( hMemDC, hBitTemp );
    DeleteDC(hMemDC);
    return TRUE;
}

