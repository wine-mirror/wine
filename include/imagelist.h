/*
 * ImageList definitions
 *
 * Copyright 1998 Eric Kohl
 */
 
#ifndef __WINE_IMAGELIST_H
#define __WINE_IMAGELIST_H

#include "wingdi.h"

struct _IMAGELIST
{
    HBITMAP hbmImage;
    HBITMAP hbmMask;
    HBRUSH  hbrBlend25;
    HBRUSH  hbrBlend50;
    COLORREF  clrBk;
    COLORREF  clrFg;
    INT     cInitial;
    INT     cGrow;
    INT     cMaxImage;
    INT     cCurImage;
    INT     cx;
    INT     cy;
    UINT    flags;
    UINT    uBitsPixel;
    INT     nOvlIdx[15];
};
 
typedef struct _IMAGELIST *HIMAGELIST;


/* header used by ImageList_Read() and ImageList_Write() */

typedef struct _ILHEAD
{
    USHORT usMagic;
    USHORT usParam1;
    INT    Param2;
    INT    Param3;
    INT    Param4;
    INT    Param5;
    INT    Param6;
    INT    Param7;
} ILHEAD;

#endif  /* __WINE_IMAGELIST_H */

