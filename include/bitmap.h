/*
 * GDI bitmap definitions
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#ifndef BITMAP_H
#define BITMAP_H

#include <stdlib.h>
#include <X11/Xlib.h>
#include "windows.h"

  /* objects/bitmap.c */
extern BOOL BITMAP_Init(void);

  /* objects/dib.c */
extern int DIB_GetImageWidthBytes( int width, int depth );
extern int DIB_BitmapInfoSize( BITMAPINFO * info, WORD coloruse );

  /* GCs used for B&W and color bitmap operations */
extern GC BITMAP_monoGC, BITMAP_colorGC;

#define BITMAP_GC(bmp) \
  (((bmp)->bitmap.bmBitsPixel == 1) ? BITMAP_monoGC : BITMAP_colorGC)

#define XCREATEIMAGE(image,width,height,bpp) \
{ \
    int width_bytes = DIB_GetImageWidthBytes( (width), (bpp) ); \
    (image) = XCreateImage(display, DefaultVisualOfScreen(screen), \
                           (bpp), ZPixmap, 0, malloc( (height)*width_bytes ), \
                           (width), (height), 32, width_bytes ); \
}

#endif  /* BITMAP_H */
