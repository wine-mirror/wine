/*
 * GDI device independent bitmaps
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdio.h>
#include <stdlib.h>

#include "gdi.h"
#include "icon.h"


extern XImage * BITMAP_BmpToImage( BITMAP *, void * );


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
XImage * DIB_DIBmpToImage( BITMAPINFOHEADER * bmp, void * bmpData )
{
    XImage * image;
    int bytesPerLine = (bmp->biWidth * bmp->biBitCount + 31) / 32 * 4;
    
    image = XCreateImage( XT_display, DefaultVisualOfScreen( XT_screen ),
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
 *           SetDIBits    (GDI.440)
 */
int SetDIBits( HDC hdc, HBITMAP hbitmap, WORD startscan, WORD lines,
	       LPSTR bits, BITMAPINFO * info, WORD coloruse )
{
    DC * dc;
    BITMAPOBJ * bmpObj;
    BITMAP * bmp;
    WORD * colorMapping;
    RGBQUAD * rgbPtr;
    XImage * bmpImage, * dibImage;
    int i, x, y, pixel, colors;
        
    if (!lines) return 0;
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    if (!(bmpObj = (BITMAPOBJ *)GDI_GetObjPtr( hbitmap, BITMAP_MAGIC )))
	return 0;
    if (!(bmp = (BITMAP *) GlobalLock( bmpObj->hBitmap ))) return 0;

      /* Build the color mapping table */

    if (info->bmiHeader.biBitCount == 24) colorMapping = NULL;
    else if (coloruse == DIB_RGB_COLORS)
    {
	colors = info->bmiHeader.biClrUsed;
	if (!colors) colors = 1 << info->bmiHeader.biBitCount;
	if (!(colorMapping = (WORD *)malloc( colors * sizeof(WORD) )))
	{
	    GlobalUnlock( bmpObj->hBitmap );
	    return 0;
	}
	for (i = 0, rgbPtr = info->bmiColors; i < colors; i++, rgbPtr++)
	    colorMapping[i] = GetNearestPaletteIndex( dc->w.hPalette, 
						     RGB(rgbPtr->rgbRed,
							 rgbPtr->rgbGreen,
							 rgbPtr->rgbBlue) );
    }
    else colorMapping = (WORD *)info->bmiColors;

      /* Transfer the pixels (very slow...) */

    bmpImage = BITMAP_BmpToImage( bmp, ((char *)bmp) + sizeof(BITMAP) );
    dibImage = DIB_DIBmpToImage( &info->bmiHeader, bits );

    for (y = 0; y < lines; y++)
    {
	for (x = 0; x < info->bmiHeader.biWidth; x++)
	{
	    pixel = XGetPixel( dibImage, x, y );
	    if (colorMapping) pixel = colorMapping[pixel];
	    else pixel = GetNearestPaletteIndex(dc->w.hPalette,(COLORREF)pixel);
	    XPutPixel( bmpImage, x, bmp->bmHeight - startscan - y - 1, pixel );
	}
    }

    bmpImage->data = NULL;
    dibImage->data = NULL;
    XDestroyImage( bmpImage );
    XDestroyImage( dibImage );

    if (colorMapping && (coloruse == DIB_RGB_COLORS)) free(colorMapping);
    
    GlobalUnlock( bmpObj->hBitmap );
    return lines;
}


/***********************************************************************
 *           GetDIBits    (GDI.441)
 */
int GetDIBits( HDC hdc, HBITMAP hbitmap, WORD startscan, WORD lines,
	       LPSTR bits, BITMAPINFO * info, WORD coloruse )
{
    DC * dc;
    BITMAPOBJ * bmpObj;
    BITMAP * bmp;
    PALETTEENTRY * palEntry;
    PALETTEOBJ * palette;
    XImage * bmpImage, * dibImage;
    int i, x, y;
        
    if (!lines) return 0;
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    if (!(bmpObj = (BITMAPOBJ *)GDI_GetObjPtr( hbitmap, BITMAP_MAGIC )))
	return 0;
    if (!(palette = (PALETTEOBJ*)GDI_GetObjPtr( dc->w.hPalette, PALETTE_MAGIC )))
	return 0;
    if (!(bmp = (BITMAP *) GlobalLock( bmpObj->hBitmap ))) return 0;

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
	bmpImage = BITMAP_BmpToImage( bmp, ((char *)bmp) + sizeof(BITMAP) );
	dibImage = DIB_DIBmpToImage( &info->bmiHeader, bits );

	for (y = 0; y < lines; y++)
	{
	    for (x = 0; x < info->bmiHeader.biWidth; x++)
	    {
		XPutPixel( dibImage, x, y,
		         XGetPixel(bmpImage, x, bmp->bmHeight-startscan-y-1) );
		
	    }
	}
	
	bmpImage->data = NULL;
	dibImage->data = NULL;
	XDestroyImage( bmpImage );
	XDestroyImage( dibImage );
    }

    GlobalUnlock( bmpObj->hBitmap );
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

