/*
 * Treeview class extra info
 *
 * Copyright 1998 Eric Kohl
 * Copyright 1998 Alex Priem
 */

#ifndef __WINE_TREEVIEW_H
#define __WINE_TREEVIEW_H

#define MINIMUM_INDENT 10
#define TV_REFRESH_DELAY 100     /* 100 ms delay between two refreshes */
#define TV_DEFAULTITEMHEIGHT 16
#define TVITEM_ALLOC	16	/* default nr of items to allocate at first try */


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
	int iLevel;			/* indentation level:0=root level */
	COLORREF clrText;
	
	int parent;	    /* handle to parent or 0 if at root*/
	int firstChild;     /* handle to first child or 0 if no child*/
	int sibling;        /* handle to next item in list, 0 if last */
	int upsibling;      /* handle to previous item in list, 0 if first */
	int visible;
    RECT32 rect;
	RECT32 text;
} TREEVIEW_ITEM;



typedef struct tagTREEVIEW_INFO
{
    UINT32	uInternalStatus;		
	UINT32  bAutoSize;			/* merge with uInternalStatus */
	INT32	Timer;
    UINT32  uNumItems;	/* number of valid TREEVIEW_ITEMs */
    UINT32	uNumPtrsAlloced; 
    UINT32	uMaxHandle;	/* needed for delete_item */
    HTREEITEM 	TopRootItem;	/* handle to first item in treeview */
	INT32	cdmode;		/* last custom draw setting */

	UINT32	uItemHeight;		/* item height, -1 for default item height */
	UINT32 	uRealItemHeight;	/* current item height in pixels */
	UINT32	uVisibleHeight;	    /* visible height of treeview in pixels */
    UINT32	uTotalHeight;		/* total height of treeview in pixels */
	UINT32	uVisibleWidth;	    
    UINT32	uTotalWidth;	
    UINT32	uIndent;			/* indentation in pixels */
    HTREEITEM	selectedItem;   /* handle to selected item or 0 if none */
	HTREEITEM	focusItem;		/* handle to item that has focus, 0 if none */
	HTREEITEM	hotItem;		/* handle currently under cursor, 0 if none */
    HTREEITEM	editItem;   /* handle to item currently editted, 0 if none */
    HTREEITEM   firstVisible;	/* handle to first visible item */
    HTREEITEM   dropItem;	/* handle to item selected by drag cursor */
	HIMAGELIST  dragList;	/* Bitmap of dragged item */
    INT32		cx,cy;		/* current x/y place in list */
    COLORREF	clrBk;		
    COLORREF	clrText;
    HFONT32 	hFont;
    HFONT32 	hBoldFont;
	HWND32		hwndToolTip;
	HWND32		hwndEdit;
	WNDPROC32   wpEditOrig;     /* needed for subclassing edit control */

    HIMAGELIST	himlNormal;	
    HIMAGELIST	himlState;
    TREEVIEW_ITEM *items;	/* itemlist */
    INT32		*freeList;	/* bitmap indicating which elements are valid */
				/* 1=valid, 0=free; 	*/
				/* size of list= uNumPtrsAlloced/32 */

} TREEVIEW_INFO;



/* bitflags for infoPtr->uInternalStatus */

#define TV_HSCROLL 	0x01    /* treeview too large to fit in window */
#define TV_VSCROLL 	0x02	/* (horizontal/vertical) */
#define TV_LDRAG		0x04	/* Lbutton pushed to start drag */
#define TV_LDRAGGING	0x08	/* Lbutton pushed, mouse moved.  */
#define TV_RDRAG		0x10	/* dito Rbutton */
#define TV_RDRAGGING	0x20	

/* bitflags for infoPtr->timer */

#define TV_REFRESH_TIMER 1	
#define TV_EDIT_TIMER    2
#define TV_REFRESH_TIMER_SET 1  
#define TV_EDIT_TIMER_SET 2  


extern VOID TREEVIEW_Register (VOID);
extern VOID TREEVIEW_Unregister (VOID);

#endif  /* __WINE_TREEVIEW_H */
