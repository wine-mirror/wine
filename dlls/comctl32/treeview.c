/* Treeview control
 *
 * Copyright 1998 Eric Kohl <ekohl@abo.rhein-zeitung.de>
 * Copyright 1998 Alex Priem <alexp@sci.kun.nl>
 *
 *
 * TODO:
 *   - Nearly all notifications.
 * 
 *   list-handling stuff: sort, sorted insertitem.
 *
 *   refreshtreeview: 	
		-small array containing info about positions.
 		-better implementation of DrawItem (connecting lines).
		-implement partial drawing?
 *   Expand:		-ctlmacro expands twice ->toggle.
 *  -drag&drop.
 *  -scrollbars.
 *  -Unicode messages
 *  -TVITEMEX 
 *
 * FIXMEs:  
   -GetNextItem: add flag for traversing visible items 
   -DblClick:	ctlmacro.exe's NM_DBLCLK seems to go wrong (returns FALSE).
	     
 */

#include "windows.h"
#include "commctrl.h"
#include "treeview.h"
#include "heap.h"
#include "win.h"
#include "debug.h"
#include <asm/bitops.h>      /* FIXME: linux specific */

static int TREEVIEW_Timer;



#define TREEVIEW_GetInfoPtr(wndPtr) ((TREEVIEW_INFO *)wndPtr->wExtra[0])



#include <asm/bitops.h>      /* FIXME: linux specific */

static int TREEVIEW_Timer;

  
  #define TREEVIEW_GetInfoPtr(wndPtr) ((TREEVIEW_INFO *)wndPtr->wExtra[0])
  
  
  

static BOOL32
TREEVIEW_SendSimpleNotify (WND *wndPtr, UINT32 code);
static BOOL32
TREEVIEW_SendTreeviewNotify (WND *wndPtr, UINT32 code, UINT32 action, 
			INT32 oldItem, INT32 newItem, POINT32 pt);
static LRESULT
TREEVIEW_SelectItem (WND *wndPtr, WPARAM32 wParam, LPARAM lParam);
static void
TREEVIEW_Refresh (WND *wndPtr, HDC32 hdc);





/* helper functions. Work with the assumption that validity of operands 
   is checked beforehand */


static TREEVIEW_ITEM *
TREEVIEW_ValidItem (TREEVIEW_INFO *infoPtr,int  handle)
{
 
 if ((!handle) || (handle>infoPtr->uMaxHandle)) return NULL;
 if (test_bit (handle, infoPtr->freeList)) return NULL;

 return & infoPtr->items[handle];
}



static TREEVIEW_ITEM *TREEVIEW_GetPrevListItem (TREEVIEW_INFO *infoPtr, 
					TREEVIEW_ITEM *tvItem)

{
 TREEVIEW_ITEM *wineItem;

 if (tvItem->upsibling) 
		return (& infoPtr->items[tvItem->upsibling]);

 wineItem=tvItem;
 while (wineItem->parent) {
	wineItem=& infoPtr->items[wineItem->parent];
	if (wineItem->upsibling) 
                return (& infoPtr->items[wineItem->upsibling]);
 } 

 return NULL;
}

static TREEVIEW_ITEM *TREEVIEW_GetNextListItem (TREEVIEW_INFO *infoPtr, 
					TREEVIEW_ITEM *tvItem)

{
 TREEVIEW_ITEM *wineItem;

 if (tvItem->sibling) 
		return (& infoPtr->items[tvItem->sibling]);

 wineItem=tvItem;
 while (wineItem->parent) {
	wineItem=& infoPtr->items [wineItem->parent];
	if (wineItem->sibling) 
                return (& infoPtr->items [wineItem->sibling]);
 } 

 return NULL;
}

static TREEVIEW_ITEM *TREEVIEW_GetLastListItem (TREEVIEW_INFO *infoPtr)

{
  TREEVIEW_ITEM *wineItem;
  
 wineItem=NULL;
 if (infoPtr->TopRootItem) 
	wineItem=& infoPtr->items [infoPtr->TopRootItem];
 while (wineItem->sibling) 
	wineItem=& infoPtr->items [wineItem->sibling];

 return wineItem;
}
	
 


static void
TREEVIEW_RemoveItem (TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *wineItem)

{
 TREEVIEW_ITEM *parentItem, *upsiblingItem, *siblingItem;
 INT32 iItem;

 iItem=wineItem->hItem;
 set_bit ( iItem & 31, &infoPtr->freeList[iItem >>5]);
 infoPtr->uNumItems--;
 parentItem=NULL;
 if (wineItem->pszText!=LPSTR_TEXTCALLBACK32A) 
	HeapFree (GetProcessHeap (), 0, wineItem->pszText);

 if (wineItem->parent) {
	parentItem=& infoPtr->items[ wineItem->parent];
	if (parentItem->cChildren==1) {
		parentItem->cChildren=0;
		parentItem->firstChild=0;    
		return;
	} else {
		parentItem->cChildren--;
		if (parentItem->firstChild==iItem) 
			parentItem->firstChild=wineItem->sibling;
		}
 }

 if (iItem==infoPtr->TopRootItem) 
	infoPtr->TopRootItem=wineItem->sibling;
 if (wineItem->upsibling) {
	upsiblingItem=& infoPtr->items [wineItem->upsibling];
	upsiblingItem->sibling=wineItem->sibling;
 }
 if (wineItem->sibling) {
	siblingItem=& infoPtr->items [wineItem->sibling];
	siblingItem->upsibling=wineItem->upsibling;
 }
}



static void TREEVIEW_RemoveAllChildren (TREEVIEW_INFO *infoPtr, 
					   TREEVIEW_ITEM *parentItem)

{
 TREEVIEW_ITEM *killItem;
 INT32	kill;
 
 kill=parentItem->firstChild;
 while (kill) {
 	set_bit ( kill & 31, &infoPtr->freeList[kill >>5]);
 	killItem=& infoPtr->items[kill];
	if (killItem->pszText!=LPSTR_TEXTCALLBACK32A) 
		HeapFree (GetProcessHeap (), 0, killItem->pszText);
	kill=killItem->sibling;
 }
 infoPtr->uNumItems -= parentItem->cChildren;
 parentItem->firstChild = 0;
 parentItem->cChildren = 0;
}



static void TREEVIEW_RemoveTree (TREEVIEW_INFO *infoPtr)
					   

{
 TREEVIEW_ITEM *killItem;
 int i;

	/* bummer: if we didn't delete anything, test_bit is overhead */
 
    for (i=1; i<=infoPtr->uMaxHandle; i++) 
	if (!test_bit (i, infoPtr->freeList)) {
		killItem=& infoPtr->items [i];	
		if (killItem->pszText!=LPSTR_TEXTCALLBACK32A)
			HeapFree (GetProcessHeap (), 0, killItem->pszText);
	} 

    if (infoPtr->uNumPtrsAlloced) {
        HeapFree (GetProcessHeap (), 0, infoPtr->items);
        HeapFree (GetProcessHeap (), 0, infoPtr->freeList);
        infoPtr->uNumItems=0;
        infoPtr->uNumPtrsAlloced=0;
        infoPtr->uMaxHandle=0;
    }   

	/* this function doesn't remove infoPtr itself */
}










static LRESULT
TREEVIEW_GetImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);

  TRACE (treeview,"\n");

  if (infoPtr==NULL) return 0;

  if ((INT32)wParam == TVSIL_NORMAL) 
	return (LRESULT) infoPtr->himlNormal;
  if ((INT32)wParam == TVSIL_STATE) 
	return (LRESULT) infoPtr->himlState;

  return 0;
}




static LRESULT
TREEVIEW_SetImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
    HIMAGELIST himlTemp;

    switch ((INT32)wParam) {
	case TVSIL_NORMAL:
	    himlTemp = infoPtr->himlNormal;
	    infoPtr->himlNormal = (HIMAGELIST)lParam;
	    return (LRESULT)himlTemp;

	case TVSIL_STATE:
	    himlTemp = infoPtr->himlState;
	    infoPtr->himlState = (HIMAGELIST)lParam;
	    return (LRESULT)himlTemp;
    }

    return (LRESULT)NULL;
}



static LRESULT
TREEVIEW_SetItemHeight (WND *wndPtr, WPARAM32 wParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
  INT32 prevHeight=infoPtr->uItemHeight;
  HDC32 hdc;
  TEXTMETRIC32A tm;


  if (wParam==-1) {
	hdc=GetDC32 (wndPtr->hwndSelf);
	infoPtr->uItemHeight=-1;
	GetTextMetrics32A (hdc, &tm);
    infoPtr->uRealItemHeight= tm.tmHeight + tm.tmExternalLeading;
	ReleaseDC32 (wndPtr->hwndSelf, hdc);
	return prevHeight;
  }

	/* FIXME: check wParam > imagelist height */

  if (!(wndPtr->dwStyle & TVS_NONEVENHEIGHT))
	infoPtr->uItemHeight = (INT32) wParam & 0xfffffffe;
  return prevHeight;
}

static LRESULT
TREEVIEW_GetItemHeight (WND *wndPtr)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
  
  return infoPtr->uItemHeight;
}
  
static LRESULT
TREEVIEW_SetTextColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
  COLORREF prevColor=infoPtr->clrText;

  infoPtr->clrText=(COLORREF) lParam;
  return (LRESULT) prevColor;
}

static LRESULT
TREEVIEW_GetTextColor (WND *wndPtr)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
	
  return (LRESULT) infoPtr->clrText;
}


static INT32
TREEVIEW_DrawItem (WND *wndPtr, HDC32 hdc, TREEVIEW_ITEM *wineItem, 
		   TREEVIEW_ITEM *upperItem, int indent)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
  INT32  oldBkMode,center,xpos;
  COLORREF oldBkColor;
  UINT32 uTextJustify = DT_LEFT;
  HPEN32 hOldPen, hnewPen,hRootPen;
  RECT32 r,upper;
  
  hnewPen = CreatePen32(PS_DOT, 0, GetSysColor32(COLOR_WINDOWTEXT) );
  hOldPen = SelectObject32( hdc, hnewPen );
 
  r=wineItem->rect;
  if (upperItem) 
	upper=upperItem->rect;
  else {
	upper.top=0;
	upper.left=8;
  }
  center=(r.top+r.bottom)/2;
  xpos=r.left+8;

  if (wndPtr->dwStyle & TVS_HASLINES) {
	POINT32 points[3];
	if ((wndPtr->dwStyle & TVS_LINESATROOT) && (indent==0)) {
		points[0].y=points[1].y=center;
		points[2].y=upper.top;
		points[1].x=points[2].x=upper.left;
		points[0].x=upper.left+12;
		points[2].y+=5;

 		Polyline32 (hdc,points,3);
	}
	else {
		points[0].y=points[1].y=center;
                points[2].y=upper.top;
                points[1].x=points[2].x=upper.left+13;
                points[0].x=upper.left+25;
                points[2].y+=5;
 		Polyline32 (hdc,points,3);
	}
 }

  DeleteObject32(hnewPen);
  SelectObject32(hdc, hOldPen);

  if ((wndPtr->dwStyle & TVS_HASBUTTONS) && (wineItem->cChildren)) {
/*
  	hRootPen = CreatePen32(PS_SOLID, 0, GetSysColor32(COLOR_WINDOW) );
  	SelectObject32( hdc, hRootPen );
*/

	Rectangle32 (hdc, xpos-4, center-4, xpos+5, center+5);
	MoveToEx32 (hdc, xpos-2, center, NULL);
	LineTo32   (hdc, xpos+3, center);
	if (!(wineItem->state & TVIS_EXPANDED)) {
		MoveToEx32 (hdc, xpos,   center-2, NULL);
		LineTo32   (hdc, xpos,   center+3);
	}
 /* 	DeleteObject32(hRootPen); */
        }


  xpos+=13;

  if (wineItem->mask & TVIF_IMAGE) {
	if (wineItem->iImage!=I_IMAGECALLBACK) {
  		if (infoPtr->himlNormal) {
  			ImageList_Draw (infoPtr->himlNormal,wineItem->iImage, hdc,
                      			xpos-2, r.top+1, ILD_NORMAL);
  			xpos+=15;
		}
	}
  }

  r.left=xpos;
  if ((wineItem->mask & TVIF_TEXT) && (wineItem->pszText)) {
	    if (wineItem->state & TVIS_SELECTED) {
            	oldBkMode = SetBkMode32(hdc, OPAQUE);
		oldBkColor= SetBkColor32 (hdc, GetSysColor32( COLOR_HIGHLIGHT));
		SetTextColor32 (hdc, GetSysColor32(COLOR_HIGHLIGHTTEXT));
	    }
	    else {
            	oldBkMode = SetBkMode32(hdc, TRANSPARENT);
	    }
            r.left += 3;
            r.right -= 3;
			if (infoPtr->clrText==-1)
            	SetTextColor32 (hdc, COLOR_BTNTEXT);
			else 
				SetTextColor32 (hdc, infoPtr->clrText);  /* FIXME: retval */
            DrawText32A(hdc, wineItem->pszText, lstrlen32A(wineItem->pszText),
                  &r, uTextJustify|DT_VCENTER|DT_SINGLELINE);
            if (oldBkMode != TRANSPARENT)
                SetBkMode32(hdc, oldBkMode);
	    if (wineItem->state & TVIS_SELECTED)
		SetBkColor32 (hdc, oldBkColor);
        }

 return wineItem->rect.right;
}







static LRESULT
TREEVIEW_GetItemRect (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
  TREEVIEW_ITEM *wineItem;
  INT32 iItem;
  LPRECT32 lpRect;

  TRACE (treeview,"\n");
  if (infoPtr==NULL) return FALSE;
  
  iItem = (INT32)lParam;
  wineItem = TREEVIEW_ValidItem (infoPtr, iItem);
  if (!wineItem) return FALSE;

  wineItem=& infoPtr->items[ iItem ];
  if (!wineItem->visible) return FALSE;

  lpRect = (LPRECT32)lParam;
  if (lpRect == NULL) return FALSE;
	
  if ((INT32) wParam) {
  	lpRect->left	= wineItem->text.left;
	lpRect->right	= wineItem->text.right;
	lpRect->bottom	= wineItem->text.bottom;
	lpRect->top	= wineItem->text.top;
  } else {
	lpRect->left 	= wineItem->rect.left;
	lpRect->right	= wineItem->rect.right;
	lpRect->bottom  = wineItem->rect.bottom;
	lpRect->top	= wineItem->rect.top;
  }

  return TRUE;
}



static LRESULT
TREEVIEW_GetVisibleCount (WND *wndPtr,  WPARAM32 wParam, LPARAM lParam)

{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);

  TRACE (treeview,"\n");

  return (LRESULT) infoPtr->uVisibleHeight / infoPtr->uRealItemHeight;
}



static LRESULT
TREEVIEW_SetItem (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
  TREEVIEW_ITEM *wineItem;
  TV_ITEM *tvItem;
  INT32 iItem,len;

  TRACE (treeview,"\n");
  tvItem=(LPTVITEM) lParam;
  iItem=tvItem->hItem;

  wineItem = TREEVIEW_ValidItem (infoPtr, iItem);
  if (!wineItem) return FALSE;

  if (tvItem->mask & TVIF_CHILDREN) {
        wineItem->cChildren=tvItem->cChildren;
  }

  if (tvItem->mask & TVIF_IMAGE) {
       wineItem->iImage=tvItem->iImage;
  }

  if (tvItem->mask & TVIF_INTEGRAL) {
/*        wineItem->iIntegral=tvItem->iIntegral; */
  }

  if (tvItem->mask & TVIF_PARAM) {
        wineItem->lParam=tvItem->lParam;
  }

  if (tvItem->mask & TVIF_SELECTEDIMAGE) {
        wineItem->iSelectedImage=tvItem->iSelectedImage;
  }

  if (tvItem->mask & TVIF_STATE) {
        wineItem->state=tvItem->state & tvItem->stateMask;
  }

  if (tvItem->mask & TVIF_TEXT) {
        len=tvItem->cchTextMax;
        if (len>wineItem->cchTextMax) {
		HeapFree (GetProcessHeap (), 0, wineItem->pszText);
                wineItem->pszText= HeapAlloc (GetProcessHeap (), 
				HEAP_ZERO_MEMORY, len+1);
	}
        lstrcpyn32A (wineItem->pszText, tvItem->pszText,len);
   }

  return TRUE;
}





static void
TREEVIEW_Refresh (WND *wndPtr, HDC32 hdc)

{
    TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
    HFONT32 hFont, hOldFont;
    RECT32 rect;
    HBRUSH32 hbrBk;
    INT32 iItem, indent, x, y, height;
    INT32 viewtop,viewbottom,viewleft,viewright;
    TREEVIEW_ITEM *wineItem, *prevItem;

    TRACE (treeview,"\n");

    if (TREEVIEW_Timer & TV_REFRESH_TIMER_SET) {
		KillTimer32 (wndPtr->hwndSelf, TV_REFRESH_TIMER);
		TREEVIEW_Timer &= ~TV_REFRESH_TIMER_SET;
    }

    
    GetClientRect32 (wndPtr->hwndSelf, &rect);
    if ((rect.left-rect.right ==0) || (rect.top-rect.bottom==0)) return;
    viewtop=infoPtr->cy;
    viewbottom=infoPtr->cy + rect.bottom-rect.top;
    viewleft=infoPtr->cx;
    viewright=infoPtr->cx + rect.right-rect.left;

	infoPtr->uVisibleHeight=viewbottom - viewtop;

    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject32 (DEFAULT_GUI_FONT);
    hOldFont = SelectObject32 (hdc, hFont);

    /* draw background */
    hbrBk = GetSysColorBrush32(COLOR_WINDOW);
    FillRect32(hdc, &rect, hbrBk);


    iItem=infoPtr->TopRootItem;
    infoPtr->firstVisible=0;
    wineItem=NULL;
    indent=0;
    x=y=0;
    TRACE (treeview, "[%d %d %d %d]\n",viewtop,viewbottom,viewleft,viewright);

    while (iItem) {
		prevItem=wineItem;
        wineItem= & infoPtr->items[iItem];

		TRACE (treeview, "%d %d [%d %d %d %d] (%s)\n",y,x,
			wineItem->rect.top, wineItem->rect.bottom,
			wineItem->rect.left, wineItem->rect.right,
			wineItem->pszText);

		height=infoPtr->uRealItemHeight * wineItem->iIntegral;
		if ((y >= viewtop) && (y <= viewbottom) &&
	    	(x >= viewleft  ) && (x <= viewright)) {
        		wineItem->rect.top = y - infoPtr->cy + rect.top;
        		wineItem->rect.bottom = wineItem->rect.top + height ;
         		wineItem->rect.left = x - infoPtr->cx + rect.left;
        		wineItem->rect.right = rect.right;
			if (!infoPtr->firstVisible)
				infoPtr->firstVisible=wineItem->hItem;
        		TREEVIEW_DrawItem (wndPtr, hdc, wineItem, prevItem, indent);
		}
		else {
			wineItem->rect.top  = wineItem->rect.bottom = -1;
			wineItem->rect.left = wineItem->rect.right = -1;
 		}

		/* look up next item */
	
		if ((wineItem->firstChild) && (wineItem->state & TVIS_EXPANDED)) {
			iItem=wineItem->firstChild;
			indent++;
			x+=infoPtr->uIndent;
		}
		else {
			iItem=wineItem->sibling;
			while ((!iItem) && (indent>0)) {
				indent--;
				x-=infoPtr->uIndent;
				prevItem=wineItem;
				wineItem=&infoPtr->items[wineItem->parent];
				iItem=wineItem->sibling;
			}
		}
        y +=height;
    }				/* while */

    infoPtr->uTotalHeight=y;
    if (y >= (viewbottom-viewtop)) {
 		if (!(infoPtr->uInternalStatus & TV_VSCROLL))
			ShowScrollBar32 (wndPtr->hwndSelf, SB_VERT, TRUE);
		infoPtr->uInternalStatus |=TV_VSCROLL;
 		SetScrollRange32 (wndPtr->hwndSelf, SB_VERT, 0, 
					y - infoPtr->uVisibleHeight, FALSE);
		SetScrollPos32 (wndPtr->hwndSelf, SB_VERT, infoPtr->cy, TRUE);
	}
    else {
		if (infoPtr->uInternalStatus & TV_VSCROLL) 
			ShowScrollBar32 (wndPtr->hwndSelf, SB_VERT, FALSE);
		infoPtr->uInternalStatus &= ~TV_VSCROLL;
	}


    SelectObject32 (hdc, hOldFont);
}


static LRESULT 
TREEVIEW_HandleTimer ( WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
  HDC32 hdc;

  if (!infoPtr) return FALSE;
 
  TRACE (treeview, "timer\n");

  switch (wParam) {
	case TV_REFRESH_TIMER:
		KillTimer32 (wndPtr->hwndSelf, TV_REFRESH_TIMER);
		TREEVIEW_Timer &= ~TV_REFRESH_TIMER_SET;
		hdc=GetDC32 (wndPtr->hwndSelf);
		TREEVIEW_Refresh (wndPtr, hdc);
		ReleaseDC32 (wndPtr->hwndSelf, hdc);
		return 0;
	case TV_EDIT_TIMER:
		KillTimer32 (wndPtr->hwndSelf, TV_EDIT_TIMER);
		TREEVIEW_Timer &= ~TV_EDIT_TIMER_SET;
		return 0;
 }
		
 return 1;
}


static void
TREEVIEW_QueueRefresh (WND *wndPtr)

{
 
 TRACE (treeview,"queued\n");
 if (TREEVIEW_Timer & TV_REFRESH_TIMER_SET) {
	KillTimer32 (wndPtr->hwndSelf, TV_REFRESH_TIMER);
 }

 SetTimer32 (wndPtr->hwndSelf, TV_REFRESH_TIMER, TV_REFRESH_DELAY, 0);
 TREEVIEW_Timer|=TV_REFRESH_TIMER_SET;
}



static LRESULT
TREEVIEW_GetItem (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
  LPTVITEM      tvItem;
  TREEVIEW_ITEM *wineItem;
  INT32         iItem,len;

  TRACE (treeview,"\n");
  tvItem=(LPTVITEM) lParam;
  iItem=tvItem->hItem;

  wineItem = TREEVIEW_ValidItem (infoPtr, iItem);
  if (!wineItem) return FALSE;


   if (tvItem->mask & TVIF_CHILDREN) {
        tvItem->cChildren=wineItem->cChildren;
   }

   if (tvItem->mask & TVIF_HANDLE) {
        tvItem->hItem=wineItem->hItem;
   }

   if (tvItem->mask & TVIF_IMAGE) {
        tvItem->iImage=wineItem->iImage;
   }

   if (tvItem->mask & TVIF_INTEGRAL) {
/*        tvItem->iIntegral=wineItem->iIntegral; */
   }

   if (tvItem->mask & TVIF_PARAM) {
        tvItem->lParam=wineItem->lParam;
   }

   if (tvItem->mask & TVIF_SELECTEDIMAGE) {
        tvItem->iSelectedImage=wineItem->iSelectedImage;
   }

   if (tvItem->mask & TVIF_STATE) {
        tvItem->state=wineItem->state & tvItem->stateMask;
   }

   if (tvItem->mask & TVIF_TEXT) {
	len=wineItem->cchTextMax;
	if (wineItem->cchTextMax>tvItem->cchTextMax) 
		len=tvItem->cchTextMax-1;
        lstrcpyn32A (tvItem->pszText, tvItem->pszText,len);
   }

  return TRUE;
}



static LRESULT
TREEVIEW_GetNextItem32 (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)

{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
  TREEVIEW_ITEM *wineItem;
  INT32 iItem, flag;


  TRACE (treeview,"item:%lu, flags:%x\n", lParam, wParam);
  if (!infoPtr) return FALSE;

  flag= (INT32) wParam;
  switch (flag) {
	case TVGN_ROOT: 	return (LRESULT) infoPtr->TopRootItem;
	case TVGN_CARET: 	return (LRESULT) infoPtr->selectedItem;
	case TVGN_FIRSTVISIBLE: return (LRESULT) infoPtr->firstVisible;
	case TVGN_DROPHILITE:	return (LRESULT) infoPtr->dropItem;
	}
	
  iItem= (INT32) lParam;
  wineItem = TREEVIEW_ValidItem (infoPtr, iItem);
  if (!wineItem) return FALSE;

  switch (flag)	{
	case TVGN_NEXT: 	return (LRESULT) wineItem->sibling;
	case TVGN_PREVIOUS: 	return (LRESULT) wineItem->upsibling;
	case TVGN_PARENT: 	return (LRESULT) wineItem->parent;
	case TVGN_CHILD: 	return (LRESULT) wineItem->firstChild;
	case TVGN_LASTVISIBLE:  FIXME (treeview,"TVGN_LASTVISIBLE not implemented\n");
				return 0;
	case TVGN_NEXTVISIBLE:  wineItem=TREEVIEW_GetNextListItem 
						(infoPtr,wineItem);
				if (wineItem) 
					return (LRESULT) wineItem->hItem;
				else
					return (LRESULT) 0;
	case TVGN_PREVIOUSVISIBLE: wineItem=TREEVIEW_GetPrevListItem
						(infoPtr, wineItem);
				if (wineItem) 
					return (LRESULT) wineItem->hItem;
				else
					return (LRESULT) 0;
	default:	FIXME (treeview,"Unknown msg %x,item %x\n", flag,iItem);
	}

 return 0;
}


static LRESULT
TREEVIEW_GetCount (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);

 return (LRESULT) infoPtr->uNumItems;
}




/* the method used below isn't the most memory-friendly, but it avoids 
   a lot of memory reallocations */ 

/* BTW: we waste handle 0; 0 is not an allowed handle. Fix this by
        decreasing infoptr->items with 1, and increasing it by 1 if 
        it is referenced in mm-handling stuff? */

static LRESULT
TREEVIEW_InsertItem32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)

{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
  TVINSERTSTRUCT  *ptdi;
  TV_ITEM 	*tvItem;
  TREEVIEW_ITEM *wineItem, *parentItem, *prevsib, *sibItem;
  INT32		iItem,listItems,i,len;
  
  TRACE (treeview,"\n");
  ptdi = (TVINSERTSTRUCT *) lParam;

	/* check if memory is available */

  if (infoPtr->uNumPtrsAlloced==0) {
        infoPtr->items = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
                                    TVITEM_ALLOC*sizeof (TREEVIEW_ITEM));
        infoPtr->freeList= HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
                                    (1+(TVITEM_ALLOC>>5)) *sizeof (INT32));
        infoPtr->uNumPtrsAlloced=TVITEM_ALLOC;
	infoPtr->TopRootItem=1;
   }

  if (infoPtr->uNumItems == (infoPtr->uNumPtrsAlloced-1) ) {
   	TREEVIEW_ITEM *oldItems = infoPtr->items;
	INT32 *oldfreeList = infoPtr->freeList;

	infoPtr->uNumPtrsAlloced*=2;
        infoPtr->items = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
                              infoPtr->uNumPtrsAlloced*sizeof (TREEVIEW_ITEM));
        infoPtr->freeList= HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
                              (1+(infoPtr->uNumPtrsAlloced>>5))*sizeof (INT32));

        memcpy (&infoPtr->items[0], &oldItems[0],
                    infoPtr->uNumPtrsAlloced/2 * sizeof(TREEVIEW_ITEM));
        memcpy (&infoPtr->freeList[0], &oldfreeList[0],
                    infoPtr->uNumPtrsAlloced>>6 * sizeof(INT32));

         HeapFree (GetProcessHeap (), 0, oldItems);  
         HeapFree (GetProcessHeap (), 0, oldfreeList);  
    }

  iItem=0;
  infoPtr->uNumItems++;

  if (infoPtr->uMaxHandle==(infoPtr->uNumItems-1))  { 
  	iItem=infoPtr->uNumItems;
  	infoPtr->uMaxHandle++;
  } 
  else {					 /* check freelist */
	for (i=0; i<infoPtr->uNumPtrsAlloced>>5; i++) {
		if (infoPtr->freeList[i]) {
			iItem=ffs (infoPtr->freeList[i]);
			clear_bit (iItem & 31, & infoPtr->freeList[i]);
			break;
		}
  	 } 
  }
  if (!iItem) ERR (treeview, "Argh -- can't find free item.\n");
  
  tvItem= & ptdi->item;
  wineItem=& infoPtr->items[iItem];



  if ((ptdi->hParent==TVI_ROOT) || (ptdi->hParent==0)) {
	parentItem=NULL;
	wineItem->parent=0; 
	sibItem=&infoPtr->items [infoPtr->TopRootItem];
	listItems=infoPtr->uNumItems;
  }
  else  {
	parentItem= &infoPtr->items[ptdi->hParent];
	if (!parentItem->firstChild) 
		parentItem->firstChild=iItem;
	wineItem->parent=ptdi->hParent;
	sibItem=&infoPtr->items [parentItem->firstChild];
	parentItem->cChildren++;
	listItems=parentItem->cChildren;
  }

  wineItem->upsibling=0;  /* needed in case we're the first item in a list */ 
  wineItem->sibling=0;     
  wineItem->firstChild=0;

  if (listItems>1) {
     prevsib=NULL;
     switch (ptdi->hInsertAfter) {
		case TVI_FIRST: wineItem->sibling=infoPtr->TopRootItem;
			infoPtr->TopRootItem=iItem;
			break;
		case TVI_LAST:  
			while (sibItem->sibling) {
				prevsib=sibItem;
				sibItem=&infoPtr->items [sibItem->sibling];
			}
			sibItem->sibling=iItem;
			if (prevsib!=NULL) 
				wineItem->upsibling=prevsib->hItem;
			else
  				wineItem->sibling=0; 	/* terminate list */
			break;
		case TVI_SORT:  
			FIXME (treeview, "Sorted insert not implemented yet\n");
			break;
		default:	
			while ((sibItem->sibling) && (sibItem->sibling!=iItem)) {
				prevsib=sibItem;
                sibItem=&infoPtr->items [sibItem->sibling];
            }
			if (sibItem->sibling) 
				WARN (treeview, "Buggy program tried to insert item after nonexisting handle.");
			sibItem->upsibling=iItem;
			wineItem->sibling=sibItem->hItem;
			if (prevsib!=NULL) 
				wineItem->upsibling=prevsib->hItem;
			break;
   	}
   }	


/* Fill in info structure */

   wineItem->mask=tvItem->mask;
   wineItem->hItem=iItem;
   wineItem->iIntegral=1; 

   if (tvItem->mask & TVIF_CHILDREN)
	 wineItem->cChildren=tvItem->cChildren;

   if (tvItem->mask & TVIF_IMAGE) 
	wineItem->iImage=tvItem->iImage;

/*   if (tvItem->mask & TVIF_INTEGRAL) 
   	wineItem->iIntegral=tvItem->iIntegral;  */
   

   if (tvItem->mask & TVIF_PARAM) 
	wineItem->lParam=tvItem->lParam;

   if (tvItem->mask & TVIF_SELECTEDIMAGE) 
	wineItem->iSelectedImage=tvItem->iSelectedImage;

   if (tvItem->mask & TVIF_STATE) {
	wineItem->state=tvItem->state;
	wineItem->stateMask=tvItem->stateMask;
   }

   if (tvItem->mask & TVIF_TEXT) {
   	TRACE (treeview,"(%s)\n", tvItem->pszText); 
	if (tvItem->pszText!=LPSTR_TEXTCALLBACK32A) {
		len = lstrlen32A (tvItem->pszText)+1;
		wineItem->pszText=
			HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, len+1);
		lstrcpy32A (wineItem->pszText, tvItem->pszText);
		wineItem->cchTextMax=len;
	}
   }

   TREEVIEW_QueueRefresh (wndPtr);

   return (LRESULT) iItem;
}



static LRESULT
TREEVIEW_DeleteItem (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
  INT32 iItem;
  POINT32 pt;
  TREEVIEW_ITEM *wineItem;

  TRACE (treeview,"\n");
  if (!infoPtr) return FALSE;

  if ((INT32) lParam == TVI_ROOT) {
	TREEVIEW_RemoveTree (infoPtr);
  } else {
  	iItem= (INT32) lParam;
  	wineItem = TREEVIEW_ValidItem (infoPtr, iItem);
  	if (!wineItem) return FALSE;
	TREEVIEW_SendTreeviewNotify (wndPtr, TVN_DELETEITEM, 0, iItem, 0, pt);
	TREEVIEW_RemoveItem (infoPtr, wineItem);
  }

  TREEVIEW_QueueRefresh (wndPtr);

  return TRUE;
}


static LRESULT
TREEVIEW_GetIndent (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);

 return infoPtr->uIndent;
}

static LRESULT
TREEVIEW_SetIndent (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
  INT32 newIndent;
   
  newIndent=(INT32) wParam;
  if (newIndent < MINIMUM_INDENT) newIndent=MINIMUM_INDENT;
  infoPtr->uIndent=newIndent;
  
  return 0;
}





static LRESULT
TREEVIEW_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TREEVIEW_INFO *infoPtr;
	HDC32 hdc;
    TEXTMETRIC32A tm;
  
    TRACE (treeview,"\n");
      /* allocate memory for info structure */
      infoPtr = (TREEVIEW_INFO *)HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
                                     sizeof(TREEVIEW_INFO));

    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
		ERR (treeview, "could not allocate info memory!\n");
		return 0;
    }

    if ((TREEVIEW_INFO*)wndPtr->wExtra[0] != infoPtr) {
		ERR (treeview, "pointer assignment error!\n");
		return 0;
    }

	hdc=GetDC32 (wndPtr->hwndSelf);

    /* set default settings */
    infoPtr->uInternalStatus=0;
    infoPtr->uNumItems=0;
    infoPtr->clrBk = GetSysColor32 (COLOR_WINDOW);
    infoPtr->clrText = GetSysColor32 (COLOR_BTNTEXT);
    infoPtr->cy = 0;
    infoPtr->cx = 0;
    infoPtr->uIndent = 15;
    infoPtr->himlNormal = NULL;
    infoPtr->himlState = NULL;
	infoPtr->uItemHeight = -1;
    GetTextMetrics32A (hdc, &tm);
    infoPtr->uRealItemHeight= tm.tmHeight + tm.tmExternalLeading;

    infoPtr->items = NULL;
    infoPtr->selectedItem=0;
    infoPtr->clrText=-1;	/* use system color */
    infoPtr->dropItem=0;

/*
    infoPtr->hwndNotify = GetParent32 (wndPtr->hwndSelf);
    infoPtr->bTransparent = (wndPtr->dwStyle & TBSTYLE_FLAT);
*/

    if (wndPtr->dwStyle & TBSTYLE_TOOLTIPS) {
        /* Create tooltip control */
//      infoPtr->hwndToolTip = CreateWindowEx32A (....);

        /* Send TV_TOOLTIPSCREATED notification */

    }
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return 0;
}



static LRESULT 
TREEVIEW_Destroy (WND *wndPtr) 
{
   TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
     
   TREEVIEW_RemoveTree (infoPtr);

   HeapFree (GetProcessHeap (), 0, infoPtr);

   return 0;
}


static LRESULT
TREEVIEW_Paint (WND *wndPtr, WPARAM32 wParam)
{
    HDC32 hdc;
    PAINTSTRUCT32 ps;

    hdc = wParam==0 ? BeginPaint32 (wndPtr->hwndSelf, &ps) : (HDC32)wParam;
    TREEVIEW_QueueRefresh (wndPtr);
    if(!wParam)
        EndPaint32 (wndPtr->hwndSelf, &ps);
    return 0;
}



static LRESULT
TREEVIEW_EraseBackground (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
    HBRUSH32 hBrush = CreateSolidBrush32 (infoPtr->clrBk);
    RECT32 rect;

    TRACE (treeview,"\n");
    GetClientRect32 (wndPtr->hwndSelf, &rect);
    FillRect32 ((HDC32)wParam, &rect, hBrush);
    DeleteObject32 (hBrush);
    return TRUE;
}





  



static BOOL32
TREEVIEW_SendSimpleNotify (WND *wndPtr, UINT32 code)
{
    NMHDR nmhdr;

    TRACE (treeview, "%x\n",code);
    nmhdr.hwndFrom = wndPtr->hwndSelf;
    nmhdr.idFrom   = wndPtr->wIDmenu;
    nmhdr.code     = code;

    return (BOOL32) SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
                                   (WPARAM32)nmhdr.idFrom, (LPARAM)&nmhdr);
}




static BOOL32
TREEVIEW_SendTreeviewNotify (WND *wndPtr, UINT32 code, UINT32 action, 
			INT32 oldItem, INT32 newItem, POINT32 pt)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
  NMTREEVIEW nmhdr;
  TREEVIEW_ITEM  *wineItem;

  TRACE (treeview,"code:%x action:%x olditem:%x newitem:%x\n",
		  code,action,oldItem,newItem);
  nmhdr.hdr.hwndFrom = wndPtr->hwndSelf;
  nmhdr.hdr.idFrom = wndPtr->wIDmenu;
  nmhdr.hdr.code = code;
  nmhdr.action = action;
  if (oldItem) {
  	wineItem=& infoPtr->items[oldItem];
  	nmhdr.itemOld.mask 		= wineItem->mask;
  	nmhdr.itemOld.hItem		= wineItem->hItem;
  	nmhdr.itemOld.state		= wineItem->state;
  	nmhdr.itemOld.stateMask	= wineItem->stateMask;
  	nmhdr.itemOld.iImage 		= wineItem->iImage;
  	nmhdr.itemOld.pszText 	= wineItem->pszText;
  	nmhdr.itemOld.cchTextMax 	= wineItem->cchTextMax;
  	nmhdr.itemOld.iImage 		= wineItem->iImage;
  	nmhdr.itemOld.iSelectedImage 	= wineItem->iSelectedImage;
  	nmhdr.itemOld.cChildren 	= wineItem->cChildren;
  	nmhdr.itemOld.lParam		= wineItem->lParam;
  }

  if (newItem) {
  	wineItem=& infoPtr->items[newItem];
  	nmhdr.itemNew.mask 		= wineItem->mask;
  	nmhdr.itemNew.hItem		= wineItem->hItem;
  	nmhdr.itemNew.state		= wineItem->state;
  	nmhdr.itemNew.stateMask	= wineItem->stateMask;
  	nmhdr.itemNew.iImage 		= wineItem->iImage;
  	nmhdr.itemNew.pszText 	= wineItem->pszText;
  	nmhdr.itemNew.cchTextMax 	= wineItem->cchTextMax;
  	nmhdr.itemNew.iImage 		= wineItem->iImage;
  	nmhdr.itemNew.iSelectedImage 	= wineItem->iSelectedImage;
  	nmhdr.itemNew.cChildren 	= wineItem->cChildren;
  	nmhdr.itemNew.lParam		= wineItem->lParam;
  }

  nmhdr.ptDrag.x = pt.x;
  nmhdr.ptDrag.y = pt.y;

  return (BOOL32)SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
                                   (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmhdr);

}




static LRESULT
TREEVIEW_Expand (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
 TREEVIEW_ITEM *wineItem;
 UINT32 flag;
 INT32 expandItem;
 POINT32 pt;
 
 flag= (UINT32) wParam;
 expandItem= (INT32) lParam;
 TRACE (treeview,"flags:%x item:%x\n", expandItem, wParam);
 wineItem = TREEVIEW_ValidItem (infoPtr, expandItem);
 if (!wineItem) return 0;
 if (!wineItem->cChildren) return 0;

 if (flag & TVE_TOGGLE) {		/* FIXME: check exact behaviour here */
	flag &= ~TVE_TOGGLE;		/* ie: bitwise ops or 'case' ops */
	if (wineItem->state & TVIS_EXPANDED) 
		flag |= TVE_COLLAPSE;
	else
		flag |= TVE_EXPAND;
 }

 switch (flag) {
    case TVE_COLLAPSERESET: 
   		if (!wineItem->state & TVIS_EXPANDED) return 0;
		wineItem->state &= ~(TVIS_EXPANDEDONCE | TVIS_EXPANDED);
		TREEVIEW_RemoveAllChildren (infoPtr, wineItem);
		break;

    case TVE_COLLAPSE: 
		if (!wineItem->state & TVIS_EXPANDED) return 0;
		wineItem->state &= ~TVIS_EXPANDED;
		break;

    case TVE_EXPAND: 
		if (wineItem->state & TVIS_EXPANDED) return 0;
		if (!(wineItem->state & TVIS_EXPANDEDONCE)) {
    		if (TREEVIEW_SendTreeviewNotify (wndPtr, TVN_ITEMEXPANDING, 
											0, 0, expandItem, pt))
					return FALSE; 	/* FIXME: OK? */
		wineItem->state |= TVIS_EXPANDED | TVIS_EXPANDEDONCE;
    	TREEVIEW_SendTreeviewNotify (wndPtr, TVN_ITEMEXPANDED, 
											0, 0, expandItem, pt);
 	}
	wineItem->state |= TVIS_EXPANDED;
	break;
   case TVE_EXPANDPARTIAL:
		FIXME (treeview, "TVE_EXPANDPARTIAL not implemented\n");
		wineItem->state ^=TVIS_EXPANDED;
		wineItem->state |=TVIS_EXPANDEDONCE;
		break;
  }
 
 TREEVIEW_QueueRefresh (wndPtr);

 return TRUE;
}



static HTREEITEM
TREEVIEW_HitTest (WND *wndPtr, LPTVHITTESTINFO lpht)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
 TREEVIEW_ITEM *wineItem;
 RECT32 rect;
 UINT32 status,x,y;
 


 GetClientRect32 (wndPtr->hwndSelf, &rect);
 TRACE (treeview,"(%d,%d)\n",lpht->pt.x, lpht->pt.y);

 status=0;
 x=lpht->pt.x;
 y=lpht->pt.y;
 if (x < rect.left)  status|=TVHT_TOLEFT;
 if (x > rect.right) status|=TVHT_TORIGHT;
 if (y < rect.top )  status|=TVHT_ABOVE;
 if (y > rect.bottom) status|=TVHT_BELOW;
 if (status) {
	lpht->flags=status;
	return 0;
 }

 if (!infoPtr->firstVisible) WARN (treeview,"Can't fetch first visible item");
 wineItem=&infoPtr->items [infoPtr->firstVisible];

 while ((wineItem!=NULL) && (y > wineItem->rect.bottom))
       wineItem=TREEVIEW_GetNextListItem (infoPtr,wineItem);
	
 if (wineItem==NULL) {
	lpht->flags=TVHT_NOWHERE;
	return 0;
 }

 if (x>wineItem->rect.right) {
	lpht->flags|=TVHT_ONITEMRIGHT;
	return wineItem->hItem;
 }
 
	
 if (x<wineItem->rect.left+10) lpht->flags|=TVHT_ONITEMBUTTON;

 lpht->flags=TVHT_ONITEMLABEL;    /* FIXME: implement other flags */
	

 lpht->hItem=wineItem->hItem;
 return wineItem->hItem;
}


static LRESULT
TREEVIEW_HitTest32 (WND *wndPtr, LPARAM lParam)
{
 
  return (LRESULT) TREEVIEW_HitTest (wndPtr, (LPTVHITTESTINFO) lParam);
}




LRESULT
TREEVIEW_LButtonDoubleClick (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
  TREEVIEW_ITEM *wineItem;
  INT32 iItem;
  TVHITTESTINFO ht;

  TRACE (treeview,"\n");
  ht.pt.x = (INT32)LOWORD(lParam);
  ht.pt.y = (INT32)HIWORD(lParam);
  SetFocus32 (wndPtr->hwndSelf);

  iItem=TREEVIEW_HitTest (wndPtr, &ht);
  TRACE (treeview,"item %d \n",iItem);
  wineItem=TREEVIEW_ValidItem (infoPtr, iItem);
  if (!wineItem) return 0;
 
  if (TREEVIEW_SendSimpleNotify (wndPtr, NM_DBLCLK)!=TRUE) {     /* FIXME!*/
	wineItem->state &= ~TVIS_EXPANDEDONCE;
	TREEVIEW_Expand (wndPtr, (WPARAM32) TVE_TOGGLE, (LPARAM) iItem);
 }
 return TRUE;
}



static LRESULT
TREEVIEW_LButtonDown (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
  INT32 iItem;
  TVHITTESTINFO ht;

  TRACE (treeview,"\n");
  ht.pt.x = (INT32)LOWORD(lParam);
  ht.pt.y = (INT32)HIWORD(lParam);

  SetFocus32 (wndPtr->hwndSelf);
  iItem=TREEVIEW_HitTest (wndPtr, &ht);
  TRACE (treeview,"item %d \n",iItem);
  if (ht.flags & TVHT_ONITEMBUTTON) {
	TREEVIEW_Expand (wndPtr, (WPARAM32) TVE_TOGGLE, (LPARAM) iItem);
  }
	
  if (TREEVIEW_SelectItem (wndPtr, (WPARAM32) TVGN_CARET, (LPARAM) iItem))
	 return 0;

  
 return 0;
}


static LRESULT
TREEVIEW_RButtonDown (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{

 return 0;
}




/* FIXME: If the specified item is the child of a collapsed parent item,
expand parent's list of child items to reveal the specified item.
*/

static LRESULT
TREEVIEW_SelectItem (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
 TREEVIEW_ITEM *prevItem,*wineItem;
 INT32 action,prevSelect, newSelect;
 POINT32 dummy;

  TRACE (treeview,"item %lx, flag %x\n", lParam, wParam);
  newSelect= (INT32) lParam;
  wineItem = TREEVIEW_ValidItem (infoPtr, newSelect);
  if (!wineItem) return FALSE;
  prevSelect=infoPtr->selectedItem;
  prevItem= TREEVIEW_ValidItem (infoPtr, prevSelect);
  dummy.x=0;
  dummy.y=0;

  action= (INT32) wParam;

  switch (action) {
	case TVGN_CARET: 
	    if (TREEVIEW_SendTreeviewNotify (wndPtr, TVN_SELCHANGING, TVC_BYMOUSE, 
										prevSelect, newSelect,dummy)) 
			return FALSE;       /* FIXME: OK? */
		
	    if (prevItem) prevItem->state &= ~TVIS_SELECTED;
  		infoPtr->selectedItem=newSelect;
		wineItem->state |=TVIS_SELECTED;
		TREEVIEW_SendTreeviewNotify (wndPtr, TVN_SELCHANGED, 
				TVC_BYMOUSE, prevSelect, newSelect, dummy);
		break;
	case TVGN_DROPHILITE: 
		FIXME (treeview, "DROPHILITE not implemented");
		break;
	case TVGN_FIRSTVISIBLE:
		FIXME (treeview, "FIRSTVISIBLE not implemented");
		break;
 }
 
 TREEVIEW_QueueRefresh (wndPtr);
  
 return TRUE;
}



/* FIXME: does KEYDOWN also send notifications?? If so, use 
   TREEVIEW_SelectItem.
*/
   

static LRESULT
TREEVIEW_KeyDown (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
 TREEVIEW_ITEM *prevItem,*newItem;
 int prevSelect;


 TRACE (treeview,"%x %lx",wParam, lParam);
 prevSelect=infoPtr->selectedItem;
 if (!prevSelect) return FALSE;

 prevItem= TREEVIEW_ValidItem (infoPtr, prevSelect);
 
 newItem=NULL;
 switch (wParam) {
	case VK_UP: 
		newItem=TREEVIEW_GetPrevListItem (infoPtr, prevItem);
		if (!newItem) 
			newItem=& infoPtr->items[infoPtr->TopRootItem];
		break;
	case VK_DOWN: 
		newItem=TREEVIEW_GetNextListItem (infoPtr, prevItem);
		if (!newItem) newItem=prevItem;
		break;
	case VK_HOME:
		newItem=& infoPtr->items[infoPtr->TopRootItem];
		break;
	case VK_END:
		newItem=TREEVIEW_GetLastListItem (infoPtr);
		break;
	case VK_PRIOR:
	case VK_NEXT:
	case VK_BACK:
	case VK_RETURN:
		FIXME (treeview, "%x not implemented\n", wParam);
		break;
 }

 if (!newItem) return FALSE;

 if (prevItem!=newItem) {
 	prevItem->state &= ~TVIS_SELECTED;
 	newItem->state |= TVIS_SELECTED;
 	infoPtr->selectedItem=newItem->hItem;
 	TREEVIEW_QueueRefresh (wndPtr);
 	return TRUE;
 }

 return FALSE;
}



static LRESULT
TREEVIEW_VScroll (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)

{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);
  int maxHeight;

  TRACE (treeview,"wp %x, lp %lx\n", wParam, lParam);
  if (!infoPtr->uInternalStatus & TV_VSCROLL) return FALSE;

  switch (LOWORD (wParam)) {
	case SB_LINEUP: 
			if (!infoPtr->cy) return FALSE;
			infoPtr->cy -= infoPtr->uRealItemHeight;
			if (infoPtr->cy < 0) infoPtr->cy=0;
			break;
	case SB_LINEDOWN: 
			maxHeight=infoPtr->uTotalHeight-infoPtr->uVisibleHeight;
			if (infoPtr->cy == maxHeight) return FALSE;
			infoPtr->cy += infoPtr->uRealItemHeight;
			if (infoPtr->cy > maxHeight) 
				infoPtr->cy = maxHeight;
			break;
	case SB_PAGEUP:	
			if (!infoPtr->cy) return FALSE;
			infoPtr->cy -= infoPtr->uVisibleHeight;
			if (infoPtr->cy < 0) infoPtr->cy=0;
			break;
	case SB_PAGEDOWN:
			maxHeight=infoPtr->uTotalHeight-infoPtr->uVisibleHeight;
			if (infoPtr->cy == maxHeight) return FALSE;
			infoPtr->cy += infoPtr->uVisibleHeight;
            if (infoPtr->cy > maxHeight)
                infoPtr->cy = maxHeight;
			break;
	case SB_THUMBTRACK: 
			infoPtr->cy = HIWORD (wParam);
			break;
			
  }
  
  TREEVIEW_QueueRefresh (wndPtr);
  return TRUE;
}

static LRESULT
TREEVIEW_HScroll (WND *wndPtr, WPARAM32 wParam, LPARAM lParam) 
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(wndPtr);

  TRACE (treeview,"wp %lx, lp %x\n", lParam, wParam);
	
  if (!infoPtr->uInternalStatus & TV_HSCROLL) return FALSE;
  return TRUE;
}




  LRESULT WINAPI
  TREEVIEW_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
  {
      WND *wndPtr = WIN_FindWndPtr(hwnd);
  
  
    switch (uMsg) {
    	case TVM_INSERTITEM32A:
          return TREEVIEW_InsertItem32A (wndPtr, wParam, lParam);

    	case TVM_INSERTITEM32W:
      		FIXME (treeview, "Unimplemented msg TVM_INSERTITEM32W\n");
      		return 0;

    	case TVM_DELETEITEM:
      		return TREEVIEW_DeleteItem (wndPtr, wParam, lParam);

    	case TVM_EXPAND:
      		return TREEVIEW_Expand (wndPtr, wParam, lParam);

    	case TVM_GETITEMRECT:
      		return TREEVIEW_GetItemRect (wndPtr, wParam, lParam);

    	case TVM_GETCOUNT:
      		return TREEVIEW_GetCount (wndPtr, wParam, lParam);

    	case TVM_GETINDENT:
      		return TREEVIEW_GetIndent (wndPtr, wParam, lParam);

    	case TVM_SETINDENT:
      		return TREEVIEW_SetIndent (wndPtr, wParam, lParam);

    	case TVM_GETIMAGELIST:
      		return TREEVIEW_GetImageList (wndPtr, wParam, lParam);

		case TVM_SETIMAGELIST:
	    	return TREEVIEW_SetImageList (wndPtr, wParam, lParam);

    	case TVM_GETNEXTITEM:
      		return TREEVIEW_GetNextItem32 (wndPtr, wParam, lParam);

    	case TVM_SELECTITEM:
      		return TREEVIEW_SelectItem (wndPtr, wParam, lParam);

    	case TVM_GETITEM32A:
      		return TREEVIEW_GetItem (wndPtr, wParam, lParam);

    	case TVM_GETITEM32W:
      		FIXME (treeview, "Unimplemented msg TVM_GETITEM32W\n");
      		return 0;

    	case TVM_SETITEM32A:
      		return TREEVIEW_SetItem (wndPtr, wParam, lParam);

    	case TVM_SETITEM32W:
      		FIXME (treeview, "Unimplemented msg TVM_SETITEMW\n");
      		return 0;

    	case TVM_EDITLABEL32A:
      		FIXME (treeview, "Unimplemented msg TVM_EDITLABEL32A \n");
      		return 0;

    	case TVM_EDITLABEL32W:
      		FIXME (treeview, "Unimplemented msg TVM_EDITLABEL32W \n");
      		return 0;

    	case TVM_GETEDITCONTROL:
      		FIXME (treeview, "Unimplemented msg TVM_GETEDITCONTROL\n");
      		return 0;

    	case TVM_GETVISIBLECOUNT:
      		return TREEVIEW_GetVisibleCount (wndPtr, wParam, lParam);

    	case TVM_HITTEST:
      		return TREEVIEW_HitTest32 (wndPtr, lParam);

    	case TVM_CREATEDRAGIMAGE:
      		FIXME (treeview, "Unimplemented msg TVM_CREATEDRAGIMAGE\n");
      		return 0;
  
    	case TVM_SORTCHILDREN:
      		FIXME (treeview, "Unimplemented msg TVM_SORTCHILDREN\n");
      		return 0;
  
    	case TVM_ENSUREVISIBLE:
      		FIXME (treeview, "Unimplemented msg TVM_ENSUREVISIBLE\n");
      		return 0;
  
    	case TVM_SORTCHILDRENCB:
      		FIXME (treeview, "Unimplemented msg TVM_SORTCHILDRENCB\n");
      		return 0;
  
    	case TVM_ENDEDITLABELNOW:
      		FIXME (treeview, "Unimplemented msg TVM_ENDEDITLABELNOW\n");
      		return 0;
  
    	case TVM_GETISEARCHSTRING32A:
      		FIXME (treeview, "Unimplemented msg TVM_GETISEARCHSTRING32A\n");
      		return 0;
  
    	case TVM_GETISEARCHSTRING32W:
      		FIXME (treeview, "Unimplemented msg TVM_GETISEARCHSTRING32W\n");
      		return 0;
  
    	case TVM_SETTOOLTIPS:
      		FIXME (treeview, "Unimplemented msg TVM_SETTOOLTIPS\n");
      		return 0;
  
    	case TVM_GETTOOLTIPS:
      		FIXME (treeview, "Unimplemented msg TVM_GETTOOLTIPS\n");
      		return 0;
  
    	case TVM_SETINSERTMARK:
      		FIXME (treeview, "Unimplemented msg TVM_SETINSERTMARK\n");
      		return 0;
  
    	case TVM_SETITEMHEIGHT:
      		return TREEVIEW_SetItemHeight (wndPtr, wParam);
  
    	case TVM_GETITEMHEIGHT:
      		return TREEVIEW_GetItemHeight (wndPtr);
  
    	case TVM_SETBKCOLOR:
      		FIXME (treeview, "Unimplemented msg TVM_SETBKCOLOR\n");
      		return 0;
	
    	case TVM_SETTEXTCOLOR:
      		return TREEVIEW_SetTextColor (wndPtr, wParam, lParam);
  
    	case TVM_GETBKCOLOR:
      		FIXME (treeview, "Unimplemented msg TVM_GETBKCOLOR\n");
      		return 0;
  
    	case TVM_GETTEXTCOLOR:
      		return TREEVIEW_GetTextColor (wndPtr);
  
    	case TVM_SETSCROLLTIME:
      		FIXME (treeview, "Unimplemented msg TVM_SETSCROLLTIME\n");
      		return 0;
  
    	case TVM_GETSCROLLTIME:
      		FIXME (treeview, "Unimplemented msg TVM_GETSCROLLTIME\n");
      		return 0;
  
    	case TVM_SETINSERTMARKCOLOR:
      		FIXME (treeview, "Unimplemented msg TVM_SETINSERTMARKCOLOR\n");
      		return 0;
  
    	case TVM_SETUNICODEFORMAT:
      		FIXME (treeview, "Unimplemented msg TVM_SETUNICODEFORMAT\n");
      		return 0;
  
    	case TVM_GETUNICODEFORMAT:
      		FIXME (treeview, "Unimplemented msg TVM_GETUNICODEFORMAT\n");
      		return 0;
  
//		case WM_COMMAND:
  
		case WM_CREATE:
			return TREEVIEW_Create (wndPtr, wParam, lParam);
  
		case WM_DESTROY:
			return TREEVIEW_Destroy (wndPtr);
  
//		case WM_ENABLE:
  
		case WM_ERASEBKGND:
	    	return TREEVIEW_EraseBackground (wndPtr, wParam, lParam);
  
		case WM_GETDLGCODE:
	    	return DLGC_WANTARROWS | DLGC_WANTCHARS;
  
		case WM_PAINT:
	    	return TREEVIEW_Paint (wndPtr, wParam);
  
//		case WM_GETFONT:
//		case WM_SETFONT:
  
		case WM_KEYDOWN:
			return TREEVIEW_KeyDown (wndPtr, wParam, lParam);
  
  
//		case WM_KILLFOCUS:
//		case WM_SETFOCUS:
  
  
		case WM_LBUTTONDOWN:
			return TREEVIEW_LButtonDown (wndPtr, wParam, lParam);
  
		case WM_LBUTTONDBLCLK:
			return TREEVIEW_LButtonDoubleClick (wndPtr, wParam, lParam);
  
		case WM_RBUTTONDOWN:
			return TREEVIEW_RButtonDown (wndPtr, wParam, lParam);
  
  
//		case WM_SYSCOLORCHANGE:
//		case WM_STYLECHANGED:
//		case WM_SETREDRAW:
  
		case WM_TIMER:
			return TREEVIEW_HandleTimer (wndPtr, wParam, lParam);
  
//		case WM_SIZE:
		case WM_HSCROLL: 
			return TREEVIEW_HScroll (wndPtr, wParam, lParam);
		case WM_VSCROLL: 
			return TREEVIEW_VScroll (wndPtr, wParam, lParam);
  
		default:
	    	if (uMsg >= WM_USER)
		FIXME (treeview, "Unknown msg %04x wp=%08x lp=%08lx\n",
  		     uMsg, wParam, lParam);
  	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
      }
    return 0;
}


void
TREEVIEW_Register (void)
{
    WNDCLASS32A wndClass;

    TRACE (treeview,"\n");

    if (GlobalFindAtom32A (WC_TREEVIEW32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC32)TREEVIEW_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TREEVIEW_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = WC_TREEVIEW32A;
 
    RegisterClass32A (&wndClass);
}

