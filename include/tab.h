/*
 * Tab control class extra info
 *
 * Copyright 1998 Anders Carlsson
 */

#ifndef __WINE_TAB_H  
#define __WINE_TAB_H

typedef struct tagTAB_ITEM
{
	UINT32  mask;
	DWORD	dwState;
    LPSTR	pszText;
	INT32   cchTextMax;
    INT32	iImage;
    LPARAM	lParam;
    RECT32   	rect;		/* bounding rectangle of the item */
} TAB_ITEM;

typedef struct tagTAB_INFO
{
    UINT32	    uNumItem;	/* number of tab items */
    INT32	    nHeight;	/* height of the tab row */
    HFONT32	    hFont;		/* handle to the current font */
    HCURSOR32	hcurArrow;	/* handle to the current cursor */
	HIMAGELIST  himl;       /* handle to a image list (may be 0) */
	UINT32		cchTextMax;
    INT32	    iSelected;	/* the currently selected item */
    TAB_ITEM	*items;		/* pointer to an array of TAB_ITEM's */
    RECT32	    rect;
} TAB_INFO;


extern VOID TAB_Register (VOID);
extern VOID TAB_Unregister (VOID);

#endif  /* __WINE_TAB_H */
