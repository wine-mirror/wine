/*
 * GDI bitmap definitions
 *
 * Copyright 1993, 1994  Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_BITMAP_H
#define __WINE_BITMAP_H

#include "gdi.h"

struct tagGDI_BITMAP_DRIVER;

/* Flags used for BitmapBits. We only use the first two at the moment */

#define DDB_SET			1
#define DDB_GET			2
#define DDB_COPY		4
#define DDB_SETWITHFILLER	8

/* DIB Section sync state */
enum { DIB_Status_None, DIB_Status_InSync, DIB_Status_GdiMod, DIB_Status_AppMod, DIB_Status_AuxMod };

  /* GDI logical bitmap object */
typedef struct tagBITMAPOBJ
{
    GDIOBJHDR   header;
    BITMAP      bitmap;
    SIZE        size;   /* For SetBitmapDimension() */
    const struct tagDC_FUNCS *funcs; /* DC function table */
    void	*physBitmap; /* ptr to device specific data */
    /* For device-independent bitmaps: */
    DIBSECTION *dib;
    SEGPTR      segptr_bits;  /* segptr to DIB bits */
} BITMAPOBJ;

typedef struct tagBITMAP_DRIVER
{
  INT  (*pSetDIBits)(struct tagBITMAPOBJ *,struct tagDC *,UINT,UINT,LPCVOID,const BITMAPINFO *,UINT,HBITMAP);
  INT  (*pGetDIBits)(struct tagBITMAPOBJ *,struct tagDC *,UINT,UINT,LPVOID,BITMAPINFO *,UINT,HBITMAP);
  VOID (*pDeleteDIBSection)(struct tagBITMAPOBJ *);
  UINT (*pSetDIBColorTable)(struct tagBITMAPOBJ *,struct tagDC *,UINT,UINT,const RGBQUAD *);
  UINT (*pGetDIBColorTable)(struct tagBITMAPOBJ *,struct tagDC *,UINT,UINT,RGBQUAD *);
  INT  (*pLockDIB)(struct tagBITMAPOBJ *,INT,BOOL);
  VOID (*pUnlockDIB)(struct tagBITMAPOBJ *,BOOL);
} BITMAP_DRIVER;

extern BITMAP_DRIVER *BITMAP_Driver;

  /* objects/bitmap.c */
extern INT16   BITMAP_GetObject16( BITMAPOBJ * bmp, INT16 count, LPVOID buffer );
extern INT   BITMAP_GetObject( BITMAPOBJ * bmp, INT count, LPVOID buffer );
extern BOOL  BITMAP_DeleteObject( HBITMAP16 hbitmap, BITMAPOBJ * bitmap );
extern INT   BITMAP_GetWidthBytes( INT width, INT depth );
extern HBITMAP BITMAP_CopyBitmap( HBITMAP hbitmap );

  /* objects/dib.c */
extern int DIB_GetDIBWidthBytes( int width, int depth );
extern int DIB_GetDIBImageBytes( int width, int height, int depth );
extern int DIB_BitmapInfoSize( const BITMAPINFO * info, WORD coloruse );
extern int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, DWORD *width,
                              int *height, WORD *bpp, WORD *compr );
extern HBITMAP DIB_CreateDIBSection( HDC hdc, BITMAPINFO *bmi, UINT usage, LPVOID *bits,
                                     HANDLE section, DWORD offset, DWORD ovr_pitch );
extern void DIB_UpdateDIBSection( DC *dc, BOOL toDIB );
extern void DIB_DeleteDIBSection( BITMAPOBJ *bmp );
extern void DIB_SelectDIBSection( DC *dc, BITMAPOBJ *bmp );
extern HGLOBAL DIB_CreateDIBFromBitmap(HDC hdc, HBITMAP hBmp);

#endif  /* __WINE_BITMAP_H */
