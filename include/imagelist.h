/*
 * ImageList definitions
 *
 * Copyright 1998 Eric Kohl
 */
 
#ifndef __WINE_IMAGELIST_H
#define __WINE_IMAGELIST_H


#include "wingdi.h"

#include "pshpack1.h"

/* the ones with offsets at the end are the same as in Windows */
struct _IMAGELIST
{
    DWORD	magic; 			/* 00:	'LMIH' */
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

typedef struct _IMAGELIST *HIMAGELIST;

/* Header used by ImageList_Read() and ImageList_Write() */
typedef struct _ILHEAD
{
    USHORT	usMagic;
    USHORT	usVersion;
    WORD	cCurImage;
    WORD	cMaxImage;
    WORD	grow;	/* unclear */
    WORD	cx;
    WORD	cy;
    COLORREF	bkcolor;
    WORD	flags;
    WORD	ovls[4];
} ILHEAD;

#include "poppack.h"
#endif  /* __WINE_IMAGELIST_H */
