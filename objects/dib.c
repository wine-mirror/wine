/*
 * GDI device independent bitmaps
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "gdi.h"
#include "bitmap.h"
#include "icon.h"

extern WORD COLOR_ToPhysical( DC *dc, COLORREF color );  /* color.c */

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
    int bytesPerLine = (bmp->biWidth * bmp->biBitCount + 31) / 32 * 4;
    
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
			        WORD *colors, XImage *bmpImage )
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
			        WORD *colors, XImage *bmpImage )
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


/***********************************************************************
 *           DIB_SetImageBits_8
 *
 * SetDIBits for an 8-bit deep DIB.
 */
static void DIB_SetImageBits_8( WORD lines, BYTE *bits, WORD width,
			        WORD *colors, XImage *bmpImage )
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
    WORD *colorMapping;
    XImage *bmpImage;
    void *bmpData;
    int i, colors, widthBytes;

      /* Build the color mapping table */

    if (info->bmiHeader.biBitCount == 24) colorMapping = NULL;
    else
    {
	colors = info->bmiHeader.biClrUsed;
	if (!colors) colors = 1 << info->bmiHeader.biBitCount;
	if (!(colorMapping = (WORD *)malloc( colors * sizeof(WORD) )))
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

    widthBytes = (info->bmiHeader.biWidth * depth + 31) / 32 * 4;
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
	DIB_SetImageBits_4( lines, bits, info->bmiHeader.biWidth,
			    colorMapping, bmpImage );
	break;
    case 8:
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

    DC_SetupGCForText( dc );  /* To have the correct ROP */
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
    HDC		hMemDC2;
#ifdef DEBUG_ICON
    printf("DrawIcon(%04X, %d, %d, %04X) \n", hDC, x, y, hIcon);
#endif
    if (hIcon == (HICON)NULL) return FALSE;
    lpico = (ICONALLOC *)GlobalLock(hIcon);
    GetObject(lpico->hBitmap, sizeof(BITMAP), (LPSTR)&bm);
#ifdef DEBUG_ICON
    printf("DrawIcon / x=%d y=%d\n", x, y);
    printf("DrawIcon / icon Width=%d\n", (int)lpico->descriptor.Width);
    printf("DrawIcon / icon Height=%d\n", (int)lpico->descriptor.Height);
    printf("DrawIcon / icon ColorCount=%d\n", (int)lpico->descriptor.ColorCount);
    printf("DrawIcon / icon icoDIBSize=%lX\n", (DWORD)lpico->descriptor.icoDIBSize);
    printf("DrawIcon / icon icoDIBOffset=%lX\n", (DWORD)lpico->descriptor.icoDIBOffset);
    printf("DrawIcon / bitmap bmWidth=%d bmHeight=%d\n", bm.bmWidth, bm.bmHeight);
#endif
    hMemDC = CreateCompatibleDC(hDC);
#ifdef DEBUG_ICON
    SelectObject(hMemDC, lpico->hBitmap);
    BitBlt(hDC, x, y, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
    SelectObject(hMemDC, lpico->hBitMask);
    BitBlt(hDC, x, y + bm.bmHeight, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
#else
    SelectObject(hMemDC, lpico->hBitMask);
    BitBlt(hDC, x, y, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCAND);
    SelectObject(hMemDC, lpico->hBitmap);
    BitBlt(hDC, x, y, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCPAINT);
#endif
    DeleteDC(hMemDC);
    return TRUE;
}

