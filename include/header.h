/*
 * Header window definitions
 *
 * Copyright 1998 Eric Kohl
 *
 */

#ifndef __WINE_HEADER_H_
#define __WINE_HEADER_H_

#include "commctrl.h"
#include "windef.h"

typedef struct 
{
    INT     cxy;
    HBITMAP hbm;
    LPWSTR    pszText;
    INT     fmt;
    LPARAM    lParam;
    INT     iImage;
    INT     iOrder;		/* see documentation of HD_ITEM */

    BOOL    bDown;		/* is item pressed? (used for drawing) */
    RECT    rect;		/* bounding rectangle of the item */
} HEADER_ITEM;


typedef struct
{
    UINT      uNumItem;	/* number of items (columns) */
    INT       nHeight;	/* height of the header (pixels) */
    HFONT     hFont;		/* handle to the current font */
    HCURSOR   hcurArrow;	/* handle to the arrow cursor */
    HCURSOR   hcurDivider;	/* handle to a cursor (used over dividers) <-|-> */
    HCURSOR   hcurDivopen;	/* handle to a cursor (used over dividers) <-||-> */
    BOOL      bCaptured;	/* Is the mouse captured? */
    BOOL      bPressed;	/* Is a header item pressed (down)? */
    BOOL      bTracking;	/* Is in tracking mode? */
    BOOL      bUnicode;       /* Unicode flag */
    INT       iMoveItem;	/* index of tracked item. (Tracking mode) */
    INT       xTrackOffset;	/* distance between the right side of the tracked item and the cursor */
    INT       xOldTrack;	/* track offset (see above) after the last WM_MOUSEMOVE */
    INT       nOldWidth;	/* width of a sizing item after the last WM_MOUSEMOVE */
    INT       iHotItem;	/* index of hot item (cursor is over this item) */

    HIMAGELIST  himl;		/* handle to a image list (may be 0) */
    HEADER_ITEM *items;		/* pointer to array of HEADER_ITEM's */
    BOOL	bRectsValid;	/* validity flag for bounding rectangles */
    LPINT     pOrder;         /* pointer to order array */
} HEADER_INFO;


extern VOID HEADER_Register (VOID);
extern VOID HEADER_Unregister (VOID);

#endif /* __WINE_HEADER_H_ */
