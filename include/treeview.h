/*
 * Treeview class extra info
 *
 * Copyright 1998 Eric Kohl
 * Copyright 1998 Alex Priem
 */

#ifndef __WINE_TREEVIEW_H
#define __WINE_TREEVIEW_H

#define MINIMUM_INDENT 10


#define TVITEM_ALLOC	16
#define TV_REFRESH_TIMER 1	
#define TV_EDIT_TIMER    2
#define TV_REFRESH_TIMER_SET 1  
#define TV_EDIT_TIMER_SET 2  
#define TV_REFRESH_DELAY 100     /* 100 ms delay between two refreshes */
#define TV_DEFAULTITEMHEIGHT 15

/* internal structures */

typedef struct {
    UINT32 mask;
    HTREEITEM hItem;
    UINT32 state;
    UINT32 stateMask;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int cChildren;
    LPARAM lParam;
    int iIntegral;
	int clrText;
	
	int parent;	    /* handle to parent or 0 if at root*/
	int firstChild;     /* handle to first child or 0 if no child*/
	int sibling;        /* handle to next item in list, 0 if last */
	int upsibling;      /* handle to previous item in list, 0 if first */
	int visible;
        RECT32 rect;
	RECT32 text;
} TREEVIEW_ITEM;


#define TV_HSCROLL 	0x01    /* treeview too large to fit in window */
#define TV_VSCROLL 	0x02	/* (horizontal/vertical) */

typedef struct tagTREEVIEW_INFO
{
    UINT32	uInternalStatus;		
    UINT32  uNumItems;	/* number of valid TREEVIEW_ITEMs */
    UINT32	uNumPtrsAlloced; 
    UINT32	uMaxHandle;	/* needed for delete_item */
    HTREEITEM 	TopRootItem;	/* handle to first item in treeview */

	UINT32	uItemHeight;		/* item height, -1 for default item height */
	UINT32 	uRealItemHeight;	/* real item height in pixels */
	UINT32	uVisibleHeight;	    /* visible height of treeview in pixels */
    UINT32	uTotalHeight;		/* total height of treeview in pixels */
    UINT32	uIndent;		/* indentation in pixels */
    HTREEITEM	selectedItem;   /* handle to selected item or 0 if none */
    HTREEITEM   firstVisible;	/* handle to first visible item */
    HTREEITEM   dropItem;	/* handle to item selected by drag cursor */
    INT32	cx,cy;		/* current x/y place in list */
    COLORREF clrBk;		
    COLORREF clrText;
    HFONT32     hFont;

    HIMAGELIST himlNormal;	
    HIMAGELIST himlState;
    TREEVIEW_ITEM *items;	/* itemlist */
    INT32	*freeList;	/* bitmap indicating which elements are valid */
				/* 1=valid, 0=free; 	*/
				/* size of list= uNumPtrsAlloced/32 */

} TREEVIEW_INFO;


extern void TREEVIEW_Register (void);

#endif  /* __WINE_TREEVIEW_H */
