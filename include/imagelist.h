/*
 * ImageList definitions
 *
 * Copyright 1998 Eric Kohl
 */
 
#ifndef __WINE_IMAGELIST_H
#define __WINE_IMAGELIST_H
 
struct _IMAGELIST
{
    HBITMAP32 hbmImage;
    HBITMAP32 hbmMask;
    HBRUSH32  hbrBlend25;
    HBRUSH32  hbrBlend50;
    COLORREF  clrBk;
    INT32     cInitial;
    INT32     cGrow;
    INT32     cMaxImage;
    INT32     cCurImage;
    INT32     cx;
    INT32     cy;
    UINT32    flags;
    UINT32    uBitsPixel;
    INT32     nOvlIdx[15];
};
 
typedef struct _IMAGELIST *HIMAGELIST;
 
 
#endif  /* __WINE_IMAGELIST_H */

