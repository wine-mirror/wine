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
    INT32     cxy;
    HBITMAP32 hbm;
    LPSTR     pszText;
    INT32     fmt;
    LPARAM    lParam;
    INT32     iImage;
    INT32     iOrder;		/* see documentation of HD_ITEM */

    BOOL32    bDown;		/* is item pressed? (used for drawing) */
    RECT32    rect;		/* bounding rectangle of the item */
} HEADER_ITEM;


typedef struct
{
    UINT32      uNumItem;	/* number of items (columns) */
    INT32       nHeight;	/* height of the header (pixels) */
    HFONT32     hFont;		/* handle to the current font */
    HCURSOR32   hcurArrow;	/* handle to the arrow cursor */
    HCURSOR32   hcurDivider;	/* handle to a cursor (used over dividers) <-|-> */
    HCURSOR32   hcurDivopen;	/* handle to a cursor (used over dividers) <-||-> */
    BOOL32      bCaptured;	/* Is the mouse captured? */
    BOOL32      bPressed;	/* Is a header item pressed (down)? */
    BOOL32      bTracking;	/* Is in tracking mode? */
    INT32       iMoveItem;	/* index of tracked item. (Tracking mode) */
    INT32       xTrackOffset;	/* distance between the right side of the tracked item and the cursor */
    INT32       xOldTrack;	/* track offset (see above) after the last WM_MOUSEMOVE */
    INT32       nOldWidth;	/* width of a sizing item after the last WM_MOUSEMOVE */
    INT32       iHotItem;	/* index of hot item (cursor is over this item) */

    HIMAGELIST  himl;		/* handle to a image list (may be 0) */
    HEADER_ITEM *items;		/* pointer to array of HEADER_ITEM's */
} HEADER_INFO;


extern void HEADER_Register (void);

#endif /* __WINE_HEADER_H_ */
