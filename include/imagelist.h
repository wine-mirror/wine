/*
 * ImageList definitions
 *
 * Copyright 1998 Eric Kohl
 */
 
#ifndef __WINE_IMAGELIST_H
#define __WINE_IMAGELIST_H
 
struct _IMAGELIST
{
    HANDLE32  hHeap;
    HBITMAP32 hbmImage;
    HBITMAP32 hbmMask;
    COLORREF  clrBk;
    INT32     cGrow;
    INT32     cMaxImage;
    INT32     cCurImage;
    INT32     cx;
    INT32     cy;
    UINT32    flags;
    INT32     nOvlIdx[4];
};
 
typedef struct _IMAGELIST *HIMAGELIST;
 
 
#endif  /* __WINE_IMAGELIST_H */
