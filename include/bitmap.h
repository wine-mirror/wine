/*
 * GDI bitmap definitions
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#ifndef __WINE_BITMAP_H
#define __WINE_BITMAP_H

#include "gdi.h"

  /* GDI logical bitmap object */
typedef struct
{
    GDIOBJHDR   header;
    BITMAP16    bitmap;
    Pixmap      pixmap;
    SIZE16      size;   /* For SetBitmapDimension() */
} BITMAPOBJ;

  /* GCs used for B&W and color bitmap operations */
extern GC BITMAP_monoGC, BITMAP_colorGC;

#define BITMAP_GC(bmp) \
  (((bmp)->bitmap.bmBitsPixel == 1) ? BITMAP_monoGC : BITMAP_colorGC)

#define BITMAP_WIDTH_BYTES(width,bpp) \
    (((bpp) == 24) ? (width) * 4 : ((width) * (bpp) + 15) / 16 * 2)

#define XCREATEIMAGE(image,width,height,bpp) \
{ \
    int width_bytes = DIB_GetImageWidthBytes( (width), (bpp) ); \
    (image) = XCreateImage(display, DefaultVisualOfScreen(screen), \
                           (bpp), ZPixmap, 0, xmalloc( (height)*width_bytes ),\
                           (width), (height), 32, width_bytes ); \
}

  /* objects/bitmap.c */
extern BOOL BITMAP_Init(void);
extern INT16 BITMAP_GetObject16( BITMAPOBJ * bmp, INT16 count, LPVOID buffer );
extern INT32 BITMAP_GetObject32( BITMAPOBJ * bmp, INT32 count, LPVOID buffer );
extern BOOL BITMAP_DeleteObject( HBITMAP hbitmap, BITMAPOBJ * bitmap );
extern HBITMAP BITMAP_SelectObject( DC * dc, HBITMAP hbitmap,
                                    BITMAPOBJ * bmp );

  /* objects/dib.c */
extern int DIB_GetImageWidthBytes( int width, int depth );
extern int DIB_BitmapInfoSize( BITMAPINFO * info, WORD coloruse );

  /* objects/oembitmap.c */
extern HBITMAP OBM_LoadBitmap( WORD id );
extern HANDLE OBM_LoadCursorIcon( WORD id, BOOL fCursor );

#endif  /* __WINE_BITMAP_H */
