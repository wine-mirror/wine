/*
 * ImageList definitions
 *
 * Copyright 1998 Eric Kohl
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_IMAGELIST_H
#define __WINE_IMAGELIST_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

struct _IMAGELIST
{
    const struct IImageListVtbl *lpVtbl; /* 00: IImageList vtable */
    LONG        ref;                     /* 04: reference count */

    DWORD       magic;                   /* 08: 'SAMX' */
    INT         cCurImage;               /* 0C: ImageCount */
    INT         cMaxImage;               /* 10: maximages */
    INT         cGrow;                   /* 14: cGrow */
    INT         cx;                      /* 18: cx */
    INT         cy;                      /* 1C: cy */
    DWORD       x4;
    UINT        flags;                   /* 24: flags */
    COLORREF    clrFg;                   /* 28: foreground color */
    COLORREF    clrBk;                   /* 2C: background color */


    HBITMAP     hbmImage;                /* 30: images Bitmap */
    HBITMAP     hbmMask;                 /* 34: masks  Bitmap */
    HDC         hdcImage;                /* 38: images MemDC  */
    HDC         hdcMask;                 /* 3C: masks  MemDC  */
    INT         nOvlIdx[15];             /* 40: overlay images index */

    /* not yet found out */
    HBRUSH  hbrBlend25;
    HBRUSH  hbrBlend50;
    INT     cInitial;
    UINT    uBitsPixel;
};

#define IMAGELIST_MAGIC 0x53414D58

/* Header used by ImageList_Read() and ImageList_Write() */
#include "pshpack2.h"
typedef struct _ILHEAD
{
    USHORT	usMagic;
    USHORT	usVersion;
    WORD	cCurImage;
    WORD	cMaxImage;
    WORD	cGrow;
    WORD	cx;
    WORD	cy;
    COLORREF	bkcolor;
    WORD	flags;
    SHORT	ovls[4];
} ILHEAD;
#include "poppack.h"

#endif  /* __WINE_IMAGELIST_H */
