/*
 * GDI bitmap definitions
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#ifndef __WINE_BITMAP_H
#define __WINE_BITMAP_H

#include "gdi.h"
#include "xmalloc.h"

#ifdef PRELIMINARY_WING16_SUPPORT
/* FIXME: this doesn't belong here */
#include "ts_xshm.h"

typedef struct
{
  XShmSegmentInfo	si;
  SEGPTR		bits;
} __ShmBitmapCtl;

#endif

  /* GDI logical bitmap object */
typedef struct
{
    GDIOBJHDR   header;
    BITMAP16    bitmap;
    Pixmap      pixmap;
    SIZE16      size;   /* For SetBitmapDimension() */

    /* For device-independent bitmaps: */
    DIBSECTION *dibSection;
    RGBQUAD    *colorMap;
    int         nColorMap;

} BITMAPOBJ;

  /* GCs used for B&W and color bitmap operations */
extern GC BITMAP_monoGC, BITMAP_colorGC;

#define BITMAP_GC(bmp) \
  (((bmp)->bitmap.bmBitsPixel == 1) ? BITMAP_monoGC : BITMAP_colorGC)

#define BITMAP_WIDTH_BYTES(width,bpp) \
    (((bpp) == 24) ? (width) * 4 : ( ((bpp) == 15) ? (width) * 2 : \
				    ((width) * (bpp) + 15) / 16 * 2 ))

#define XCREATEIMAGE(image,width,height,bpp) \
{ \
    int width_bytes = DIB_GetXImageWidthBytes( (width), (bpp) ); \
    (image) = TSXCreateImage(display, DefaultVisualOfScreen(screen), \
                           (bpp), ZPixmap, 0, xmalloc( (height)*width_bytes ),\
                           (width), (height), 32, width_bytes ); \
}

  /* objects/bitmap.c */
extern BOOL32  BITMAP_Init(void);
extern INT16   BITMAP_GetObject16( BITMAPOBJ * bmp, INT16 count, LPVOID buffer );
extern INT32   BITMAP_GetObject32( BITMAPOBJ * bmp, INT32 count, LPVOID buffer );
extern BOOL32  BITMAP_DeleteObject( HBITMAP16 hbitmap, BITMAPOBJ * bitmap );
extern XImage *BITMAP_GetXImage( const BITMAPOBJ *bmp );
extern INT32   BITMAP_GetBitsPadding( int width, int depth );
extern INT32   BITMAP_GetBitsWidth( int width, int depth );

  /* objects/dib.c */
extern int DIB_GetDIBWidthBytes( int width, int depth );
extern int DIB_GetXImageWidthBytes( int width, int depth );
extern int DIB_BitmapInfoSize( BITMAPINFO * info, WORD coloruse );
extern void DIB_UpdateDIBSection( DC *dc, BOOL32 toDIB );


  /* objects/oembitmap.c */
extern BOOL32 OBM_Init(void);
extern HBITMAP16 OBM_LoadBitmap( WORD id );
extern HGLOBAL16 OBM_LoadCursorIcon( WORD id, BOOL32 fCursor );

#endif  /* __WINE_BITMAP_H */
