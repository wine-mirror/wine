/*
 * Header window definitions
 *
 * Copyright 1998 Eric Kohl
 *
 */

#ifndef __WINE_HEADER_H_
#define __WINE_HEADER_H_

typedef struct 
{
    UINT32    mask;
    INT32     cxy;
    HBITMAP32 hbm;
    LPSTR     pszText;
    INT32     cchTextMax;
    INT32     fmt;
    LPARAM    lParam;
    INT32     iImage;
    INT32     iOrder;

    BOOL32    bDown;
    RECT32    rect;
} HEADER_ITEM;


typedef struct
{
    UINT32      uNumItem;
    INT32       nHeight;
    HFONT32     hFont;
    HCURSOR32   hcurArrow;
    HCURSOR32   hcurDivider;
    HCURSOR32   hcurDivopen;
    BOOL32      bCaptured;
    BOOL32      bPressed;
    BOOL32      bTracking;
    INT32       iMoveItem;
    INT32       xTrackOffset;
    INT32       xOldTrack;
    INT32       nOldWidth;
    INT32       iHotItem;

    HIMAGELIST  himl;
    HEADER_ITEM *items;
} HEADER_INFO;


extern void HEADER_Register (void);

#endif /* __WINE_HEADER_H_ */
