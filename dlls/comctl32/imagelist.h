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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_IMAGELIST_H
#define __WINE_IMAGELIST_H

#include "windef.h"
#include "wingdi.h"

#include "pshpack1.h"

/* the ones with offsets at the end are the same as in Windows */
struct _IMAGELIST
{
    DWORD	magic; 			/* 00:	'SAMX' */
    INT		cCurImage;		/* 04: ImageCount */
    INT		cMaxImage;		/* 08: maximages */
    DWORD	x3;
    INT		cx;			/* 10: cx */
    INT		cy;			/* 14: cy */
    DWORD	x4;
    UINT	flags;			/* 1c: flags */
    DWORD   	x5;
    COLORREF	clrBk;			/* 24: bkcolor */


    /* not yet found out */
    HBITMAP hbmImage;
    HBITMAP hbmMask;
    HBRUSH  hbrBlend25;
    HBRUSH  hbrBlend50;
    COLORREF  clrFg;
    INT     cInitial;
    INT     cGrow;
    UINT    uBitsPixel;
    INT     nOvlIdx[15];
};

#define IMAGELIST_MAGIC 0x53414D58

/* Header used by ImageList_Read() and ImageList_Write() */
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
