/*
 * GDI bitmap definitions
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#ifndef __WINE_BITMAP_H
#define __WINE_BITMAP_H

#include <X11/Xlib.h>

#include "gdi.h"

  /* Additional info for DIB section objects */
typedef struct
{
    /* Windows DIB section */
    DIBSECTION  dibSection;

    /* Mapping status */
    enum { DIB_NoHandler, DIB_InSync, DIB_AppMod, DIB_GdiMod } status;

    /* Color map info */
    int         nColorMap;
    int        *colorMap;

    /* Cached XImage */
    XImage     *image;

    /* Selector for 16-bit access to bits */
    WORD selector;

} DIBSECTIONOBJ;

/* Flags used for BitmapBits. We only use the first two at the moment */

#define DDB_SET			1
#define DDB_GET			2
#define DDB_COPY		4
#define DDB_SETWITHFILLER	8

typedef struct {
    const struct tagDC_FUNCS *funcs; /* DC function table */
    void	 *physBitmap; /* ptr to device specific data */
} DDBITMAP;

  /* GDI logical bitmap object */
typedef struct tagBITMAPOBJ
{
    GDIOBJHDR   header;
    BITMAP32    bitmap;
    SIZE32      size;   /* For SetBitmapDimension() */

    DDBITMAP	*DDBitmap;

    /* For device-independent bitmaps: */
    DIBSECTIONOBJ *dib;

} BITMAPOBJ;

  /* objects/bitmap.c */
extern INT16   BITMAP_GetObject16( BITMAPOBJ * bmp, INT16 count, LPVOID buffer );
extern INT32   BITMAP_GetObject32( BITMAPOBJ * bmp, INT32 count, LPVOID buffer );
extern BOOL32  BITMAP_DeleteObject( HBITMAP16 hbitmap, BITMAPOBJ * bitmap );
extern INT32   BITMAP_GetPadding( INT32 width, INT32 depth );
extern INT32   BITMAP_GetWidthBytes( INT32 width, INT32 depth );
extern HBITMAP32 BITMAP_LoadBitmap32W(HINSTANCE32 instance,LPCWSTR name,
  UINT32 loadflags);
extern HBITMAP32 BITMAP_CopyBitmap( HBITMAP32 hbitmap );

  /* objects/dib.c */
extern int DIB_GetDIBWidthBytes( int width, int depth );
extern int DIB_BitmapInfoSize( BITMAPINFO * info, WORD coloruse );
extern int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, DWORD *width,
                              int *height, WORD *bpp, WORD *compr );
extern void DIB_UpdateDIBSection( DC *dc, BOOL32 toDIB );
extern void DIB_DeleteDIBSection( BITMAPOBJ *bmp );
extern void DIB_SelectDIBSection( DC *dc, BITMAPOBJ *bmp );
extern void DIB_FixColorsToLoadflags(BITMAPINFO * bmi, UINT32 loadflags,
  BYTE pix);

#endif  /* __WINE_BITMAP_H */
