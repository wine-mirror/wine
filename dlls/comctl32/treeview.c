/* Treeview control
 *
 * Copyright 1998 Eric Kohl <ekohl@abo.rhein-zeitung.de>
 * Copyright 1998,1999 Alex Priem <alexp@sci.kun.nl>
 * Copyright 1999 Sylvain St-Germain
 *
 *
 * TODO:
 *   list-handling stuff: sort, sorted insertitem. 
 *           [should be merged with mm-handling stuff as done in listview]
 *   refreshtreeview: 	
		-small array containing info about positions.
 		-better implementation of RefreshItem:
              1) draw lines between parents
              2) draw items
			  3) draw lines from parent<->items.
		-implement partial drawing?
 *  -drag&drop: TVM_CREATEDRAGIMAGE should create drag bitmap.
 *  -scrollbars: horizontal scrollbar doesn't work.
 *  -Unicode messages
 *  -check custom draw
 *  -I_CHILDRENCALLBACK
 *   FIXME: check fontsize. (uRealItemHeight)
 *          test focusItem  (redraw in different color)
			uHotItem
			Edit: needs timer
				  better implementation.
 *   WM_HSCROLL is broken.
 *   use separate routine to get item text/image.
 *  
 *   Separate drawing/calculation.
 *
 * FIXMEs  (for personal use)
    Expand:		-ctlmacro expands twice ->toggle.
   -DblClick:	ctlmacro.exe's NM_DBLCLK seems to go wrong (returns FALSE).
   -treehelper: stack corruption makes big window. 
	     
 */


#include <string.h>
#include "winbase.h"
#include "commctrl.h"
#include "treeview.h"
#include "debug.h"

/* ffs should be in <string.h>. */

/* Defines, since they do not need to return previous state, and nr
 * has no side effects in this file.
 */
#define tv_test_bit(nr,bf)	(((LPBYTE)bf)[nr>>3]&(1<<(nr&7)))
#define tv_set_bit(nr,bf)	((LPBYTE)bf)[nr>>3]|=(1<<(nr&7))
#define tv_clear_bit(nr,bf)	((LPBYTE)bf)[nr>>3]&=~(1<<(nr&7))


#define TREEVIEW_GetInfoPtr(hwnd) \
  ((TREEVIEW_INFO *) GetWindowLongA( hwnd, 0))

static BOOL
TREEVIEW_SendSimpleNotify (HWND hwnd, UINT code);
static BOOL
TREEVIEW_SendTreeviewNotify (HWND hwnd, UINT code, UINT action, 
			HTREEITEM oldItem, HTREEITEM newItem);
static BOOL
TREEVIEW_SendTreeviewDnDNotify (HWND hwnd, UINT code, HTREEITEM dragItem, 
			POINT pt);
static BOOL
TREEVIEW_SendDispInfoNotify (HWND hwnd, TREEVIEW_ITEM *wineItem, 
			UINT code, UINT what);
static BOOL
TREEVIEW_SendCustomDrawNotify (HWND hwnd, DWORD dwDrawStage, HDC hdc,
			RECT rc);
static BOOL
TREEVIEW_SendCustomDrawItemNotify (HWND hwnd, HDC hdc,
            TREEVIEW_ITEM *tvItem, UINT uItemDrawState);
static LRESULT
TREEVIEW_DoSelectItem (HWND hwnd, INT action, HTREEITEM newSelect, INT cause);
static void
TREEVIEW_Refresh (HWND hwnd);

static LRESULT CALLBACK
TREEVIEW_Edit_SubclassProc (HWND hwnd, UINT uMsg, WPARAM wParam, 
							LPARAM lParam);





/* helper functions. Work with the assumption that validity of operands 
   is checked beforehand, and that tree state is valid.  */

/* FIXME: MS documentation says `GetNextVisibleItem' returns NULL 
   if not succesfull'. Probably only applies to derefencing infoPtr
   (ie we are offered a valid treeview structure)
   and not whether there is a next `visible' child. 
   FIXME: check other failures.
 */

/***************************************************************************
 * This method returns the TREEVIEW_ITEM object given the handle
 */
static TREEVIEW_ITEM* TREEVIEW_ValidItem(
  TREEVIEW_INFO *infoPtr,
  HTREEITEM  handle)
{
 
 if ((!handle) || (handle>infoPtr->uMaxHandle)) return NULL;
 if (tv_test_bit ((INT)handle, infoPtr->freeList)) return NULL;

 return & infoPtr->items[(INT)handle];
}

/***************************************************************************
 * This method returns the last expanded child item of a tree node
 */
static TREEVIEW_ITEM *TREEVIEW_GetLastListItem(
  TREEVIEW_INFO *infoPtr,
  TREEVIEW_ITEM *tvItem)

{
  TREEVIEW_ITEM *wineItem = tvItem;

  /* 
   * Get this item last sibling 
   */
  while (wineItem->sibling) 
 	  wineItem=& infoPtr->items [(INT)wineItem->sibling];

  /* 
   * If the last sibling has expanded children, restart.
   */
  if ( ( wineItem->cChildren > 0 ) && ( wineItem->state & TVIS_EXPANDED) )
    return TREEVIEW_GetLastListItem(
             infoPtr, 
             &(infoPtr->items[(INT)wineItem->firstChild]));

  return wineItem;
}

/***************************************************************************
 * This method returns the previous physical item in the list not 
 * considering the tree hierarchy.
 */
static TREEVIEW_ITEM *TREEVIEW_GetPrevListItem(
  TREEVIEW_INFO *infoPtr, 
  TREEVIEW_ITEM *tvItem)
{
  if (tvItem->upsibling) 
  {
    /* 
     * This item has a upsibling, get the last item.  Since, GetLastListItem
     * first looks at siblings, we must feed it with the first child.
     */
    TREEVIEW_ITEM *upItem = &infoPtr->items[(INT)tvItem->upsibling];
    
    if ( ( upItem->cChildren > 0 ) && ( upItem->state & TVIS_EXPANDED) )
      return TREEVIEW_GetLastListItem( 
               infoPtr, 
               &infoPtr->items[(INT)upItem->firstChild]);
    else
      return upItem;
  }
  else
  {
    /*
     * this item does not have a upsibling, get the parent
     */
    if (tvItem->parent) 
      return &infoPtr->items[(INT)tvItem->parent];
  }

  return NULL;
}


/***************************************************************************
 * This method returns the next physical item in the treeview not 
 * considering the tree hierarchy.
 */
static TREEVIEW_ITEM *TREEVIEW_GetNextListItem(
  TREEVIEW_INFO *infoPtr, 
  TREEVIEW_ITEM *tvItem)
{
  TREEVIEW_ITEM *wineItem = NULL;

  /* 
   * If this item has children and is expanded, return the first child
   */
  if ((tvItem->firstChild) && (tvItem->state & TVIS_EXPANDED)) 
		return (& infoPtr->items[(INT)tvItem->firstChild]);


  /*
   * try to get the sibling
   */
  if (tvItem->sibling) 
		return (& infoPtr->items[(INT)tvItem->sibling]);

  /*
   * Otherwise, get the parent's sibling.
   */
  wineItem=tvItem;
  while (wineItem->parent) {
    wineItem=& infoPtr->items [(INT)wineItem->parent];
  	if (wineItem->sibling) 
      return (& infoPtr->items [(INT)wineItem->sibling]);
  } 

  return NULL;  
}

/***************************************************************************
 * This method returns the nth item starting at the given item.  It returns 
 * the last item (or first) we we run out of items.
 *
 * Will scroll backward if count is <0.
 *             forward if count is >0.
 */
static TREEVIEW_ITEM *TREEVIEW_GetListItem(
  TREEVIEW_INFO *infoPtr, 
  TREEVIEW_ITEM *tvItem,
  LONG          count)
{
  TREEVIEW_ITEM *previousItem = NULL;
  TREEVIEW_ITEM *wineItem     = tvItem;
  LONG          iter          = 0;

  if      (count > 0)
  {
    /* Find count item downward */
    while ((iter++ < count) && (wineItem != NULL))
    {
      /* Keep a pointer to the previous in case we ask for more than we got */
      previousItem = wineItem; 
      wineItem     = TREEVIEW_GetNextListItem(infoPtr, wineItem);
    } 

    if (wineItem == NULL)
      wineItem = previousItem;
  }
  else if (count < 0)
  {
    /* Find count item upward */
    while ((iter-- > count) && (wineItem != NULL))
    {
      /* Keep a pointer to the previous in case we ask for more than we got */
      previousItem = wineItem; 
      wineItem = TREEVIEW_GetPrevListItem(infoPtr, wineItem);
    }

    if (wineItem == NULL)
      wineItem = previousItem;
  }
  else
    wineItem = NULL;

  return wineItem;
}

 
/***************************************************************************
 * This method 
 */
static void TREEVIEW_RemoveAllChildren(
  HWND hwnd,
  TREEVIEW_ITEM *parentItem)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
 TREEVIEW_ITEM *killItem;
 INT	kill;
 
 kill=(INT)parentItem->firstChild;
 while (kill) {
 	tv_set_bit ( kill, infoPtr->freeList);
 	killItem=& infoPtr->items[kill];
	if (killItem->pszText!=LPSTR_TEXTCALLBACKA) 
		COMCTL32_Free (killItem->pszText);
 	TREEVIEW_SendTreeviewNotify (hwnd, TVN_DELETEITEM, 0, (HTREEITEM)kill, 0);
	if (killItem->firstChild) 
			TREEVIEW_RemoveAllChildren (hwnd, killItem);
	kill=(INT)killItem->sibling;
 }

 if (parentItem->cChildren>0) {
 	infoPtr->uNumItems -= parentItem->cChildren;
 	parentItem->firstChild = 0;
 	parentItem->cChildren  = 0;
 }

}


static void
TREEVIEW_RemoveItem (HWND hwnd, TREEVIEW_ITEM *wineItem)

{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
 TREEVIEW_ITEM *parentItem, *upsiblingItem, *siblingItem;
 INT iItem;

 iItem=(INT)wineItem->hItem;
 tv_set_bit(iItem,infoPtr->freeList);
 infoPtr->uNumItems--;
 parentItem=NULL;
 if (wineItem->pszText!=LPSTR_TEXTCALLBACKA) 
	COMCTL32_Free (wineItem->pszText);

 TREEVIEW_SendTreeviewNotify (hwnd, TVN_DELETEITEM, 0, (HTREEITEM)iItem, 0);

 if (wineItem->firstChild) 
 	TREEVIEW_RemoveAllChildren (hwnd,wineItem);

 if (wineItem->parent) {
	parentItem=& infoPtr->items [(INT)wineItem->parent];
	switch (parentItem->cChildren) {
		case I_CHILDRENCALLBACK: 
				FIXME (treeview,"we don't handle I_CHILDRENCALLBACK yet\n");
				break;
		case 1:
			parentItem->cChildren=0;
			parentItem->firstChild=0;    
			return;
		default:
			parentItem->cChildren--;
			if ((INT)parentItem->firstChild==iItem) 
				parentItem->firstChild=wineItem->sibling;
		}
 }

 if (iItem==(INT)infoPtr->TopRootItem) 
	infoPtr->TopRootItem=(HTREEITEM)wineItem->sibling;
 if (wineItem->upsibling) {
	upsiblingItem=& infoPtr->items [(INT)wineItem->upsibling];
	upsiblingItem->sibling=wineItem->sibling;
 }
 if (wineItem->sibling) {
	siblingItem=& infoPtr->items [(INT)wineItem->sibling];
	siblingItem->upsibling=wineItem->upsibling;
 }
}





/* Note:TREEVIEW_RemoveTree doesn't remove infoPtr itself */

static void TREEVIEW_RemoveTree (HWND hwnd)
					   
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
 TREEVIEW_ITEM *killItem;
 int i;

 for (i=1; i<=(INT)infoPtr->uMaxHandle; i++) 
	if (!tv_test_bit (i, infoPtr->freeList)) {
		killItem=& infoPtr->items [i];	
		if (killItem->pszText!=LPSTR_TEXTCALLBACKA)
			COMCTL32_Free (killItem->pszText);
		TREEVIEW_SendTreeviewNotify 
					(hwnd, TVN_DELETEITEM, 0, killItem->hItem, 0);
		} 

 if (infoPtr->uNumPtrsAlloced) {
        COMCTL32_Free (infoPtr->items);
        COMCTL32_Free (infoPtr->freeList);
        infoPtr->uNumItems=0;
        infoPtr->uNumPtrsAlloced=0;
        infoPtr->uMaxHandle=0;
    }   
}







static LRESULT
TREEVIEW_GetImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);

  TRACE (treeview,"\n");
  if (infoPtr==NULL) return 0;

  if ((INT)wParam == TVSIL_NORMAL) 
	return (LRESULT) infoPtr->himlNormal;
  if ((INT)wParam == TVSIL_STATE) 
	return (LRESULT) infoPtr->himlState;

  return 0;
}

static LRESULT
TREEVIEW_SetImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
    HIMAGELIST himlTemp;

    TRACE (treeview,"\n");
    switch ((INT)wParam) {
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
TREEVIEW_SetItemHeight (HWND hwnd, WPARAM wParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  INT cx,cy,prevHeight=infoPtr->uItemHeight;
  HDC hdc;

  TRACE (treeview,"\n");
  if (wParam==-1) {
	hdc=GetDC (hwnd);
	infoPtr->uItemHeight=-1;
	return prevHeight;
  }

  ImageList_GetIconSize (infoPtr->himlNormal, &cx, &cy);

  if (wParam>cy) cy=wParam;
  infoPtr->uItemHeight=cy;

  if (!( GetWindowLongA( hwnd, GWL_STYLE) & TVS_NONEVENHEIGHT))
	infoPtr->uItemHeight = (INT) wParam & 0xfffffffe;
  return prevHeight;
}

static LRESULT
TREEVIEW_GetItemHeight (HWND hwnd)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  
  TRACE (treeview,"\n");
  return infoPtr->uItemHeight;
}
  
static LRESULT
TREEVIEW_SetTextColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  COLORREF prevColor=infoPtr->clrText;

  TRACE (treeview,"\n");
  infoPtr->clrText=(COLORREF) lParam;
  return (LRESULT) prevColor;
}

static LRESULT
TREEVIEW_GetBkColor (HWND hwnd)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
	
  TRACE (treeview,"\n");
  return (LRESULT) infoPtr->clrText;
}

static LRESULT
TREEVIEW_SetBkColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  COLORREF prevColor=infoPtr->clrBk;

  TRACE (treeview,"\n");
  infoPtr->clrBk=(COLORREF) lParam;
  return (LRESULT) prevColor;
}

static LRESULT
TREEVIEW_GetTextColor (HWND hwnd)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
	
  TRACE (treeview,"\n");
  return (LRESULT) infoPtr->clrBk;
}


/* cdmode: custom draw mode as received from app. in first NMCUSTOMDRAW 
           notification */

#define TREEVIEW_LEFT_MARGIN 8

static void
TREEVIEW_DrawItem (HWND hwnd, HDC hdc, TREEVIEW_ITEM *wineItem)

{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);

  INT   center,xpos,cx,cy, cditem, drawmode;
  HFONT hOldFont;
  UINT  uTextJustify = DT_LEFT;
  RECT  r;

 
  if (wineItem->state & TVIS_BOLD) 
  	hOldFont = SelectObject (hdc, infoPtr->hBoldFont);
  else 
  	hOldFont = SelectObject (hdc, infoPtr->hFont);

  cditem=0;
  if (infoPtr->cdmode & CDRF_NOTIFYITEMDRAW) {
		drawmode=CDDS_ITEMPREPAINT;

		if (infoPtr->cdmode & CDRF_NOTIFYSUBITEMDRAW) 
      drawmode|=CDDS_SUBITEM;

		cditem=TREEVIEW_SendCustomDrawItemNotify (hwnd, hdc, wineItem, drawmode);

		TRACE (treeview,"cditem:%d\n",cditem);

		if (cditem & CDRF_SKIPDEFAULT) 
			return;
	}

  /* 
   * Set drawing starting points 
   */
  r      = wineItem->rect;               /* this item rectangle */
  center = (r.top+r.bottom)/2;           /* this item vertical center */
  xpos   = r.left + TREEVIEW_LEFT_MARGIN;/* horizontal starting point */

  TRACE(treeview, "Left %3d, Top %3d, Right %3d, Bottom %3d\n", 
    r.left, 
    r.top, 
    r.right, 
    r.bottom);

  /* 
   * Display the tree hierarchy 
   */
  if ( GetWindowLongA( hwnd, GWL_STYLE) & TVS_HASLINES) 
  {
    /* 
     * Write links to parent node 
     * we draw the L starting from the child to the parent
     *
     * points[0] is attached to the current item
     * points[1] is the L corner
     * points[2] is attached to the parent or the up sibling
     */
    if ( GetWindowLongA( hwnd, GWL_STYLE) & TVS_LINESATROOT) 
    {
      TREEVIEW_ITEM *upNode    = NULL;
      TREEVIEW_ITEM *downNode  = NULL;
     	BOOL  hasParentOrSibling = TRUE;
      RECT  upRect             = {0,0,0,0};
      HPEN  hOldPen, hnewPen;
    	POINT points[3], downLink[2];
      /* 
       * determine the target location of the line at root, either be linked
       * to the up sibling or to the parent node.
       */
      if (wineItem->upsibling)
      	upNode  = TREEVIEW_ValidItem (infoPtr, wineItem->upsibling);
      else if (wineItem->parent)
      	upNode  = TREEVIEW_ValidItem (infoPtr, wineItem->parent);
      else
        hasParentOrSibling = FALSE;

      if (upNode)
        upRect = upNode->rect;

      if ( wineItem->iLevel == 0 )
      {
        points[2].x = points[1].x = upRect.left+8;
        points[0].x = points[2].x + 10;
        points[2].y = upRect.bottom-3;      
        points[1].y = points[0].y = center;
      }
      else
      {
        points[2].x = points[1].x = 8 + (20*wineItem->iLevel); 
        points[2].y = ((upNode->cChildren == 0) ? upRect.top : upRect.bottom-1);    
        points[1].y = points[0].y = center;
        points[0].x = points[1].x + 10; 
      }
    
      /* 
       * Check wheather this item has an invisible sibling.  If so, draw a 
       * link to the bottom of the treeview otherwise there will not be any...
       */
      if (wineItem->sibling)
      {
      	downNode = TREEVIEW_ValidItem (infoPtr, wineItem->sibling);
        if (downNode->visible == FALSE)
        {
          downLink[0].x = downLink[1].x = points[2].x;
          downLink[0].y = points[2].y;
          downLink[1].y = infoPtr->uTotalHeight - points[2].y;
        }
      }
  
      /* 
       * Get a doted pen
       */ 
      hnewPen = CreatePen(PS_DOT, 0, GetSysColor(COLOR_WINDOWTEXT) );
      hOldPen = SelectObject( hdc, hnewPen );
  
      if (hasParentOrSibling)
        Polyline (hdc,points,3); 
      else
        Polyline (hdc,points,2); 
      
      /* FIXME: This simple stuff it does not work...
      if ( downNode  && ( downNode->visible == FALSE ))
        Polyline (hdc,downLink,2); 
      */

      DeleteObject(hnewPen);
      SelectObject(hdc, hOldPen);
    }
  }

  /* 
   * Display the (+/-) signs
   */
  if (wineItem->iLevel != 0)/*  update position only for non root node */
    xpos+=(5*wineItem->iLevel);

  if (( GetWindowLongA( hwnd, GWL_STYLE) & TVS_HASBUTTONS) && 
      ( GetWindowLongA( hwnd, GWL_STYLE) & TVS_HASLINES))
  {
	  if ( (wineItem->cChildren) ||
	       (wineItem->cChildren == I_CHILDRENCALLBACK))
    {
      /* Setup expand box coordinate to facilitate the LMBClick handling */
      wineItem->expandBox.left   = xpos-4;
      wineItem->expandBox.top    = center-4;
      wineItem->expandBox.right  = xpos+5;
      wineItem->expandBox.bottom = center+5;

  		Rectangle (
        hdc, 
        wineItem->expandBox.left, 
        wineItem->expandBox.top , 
        wineItem->expandBox.right, 
        wineItem->expandBox.bottom);

  		MoveToEx (hdc, xpos-2, center, NULL);
  		LineTo   (hdc, xpos+3, center);
  
  		if (!(wineItem->state & TVIS_EXPANDED)) {
  			MoveToEx (hdc, xpos,   center-2, NULL);
  			LineTo   (hdc, xpos,   center+3);
  	  }
    }
  }

  /* 
   * Display the image assiciated with this item
   */
  xpos += 13; /* update position */
  if (wineItem->mask & (TVIF_IMAGE|TVIF_SELECTEDIMAGE)) {
    INT        imageIndex;
    HIMAGELIST *himlp = NULL;

	  if (infoPtr->himlNormal) 
      himlp=&infoPtr->himlNormal; /* get the image list */

  	if ( (wineItem->state & TVIS_SELECTED) && 
         (wineItem->iSelectedImage)) { 
        
      /* State images are displayed to the left of the Normal image*/
      if (infoPtr->himlState) 
        himlp=&infoPtr->himlState;

      /* The item is curently selected */
		  if (wineItem->iSelectedImage == I_IMAGECALLBACK) 
  			TREEVIEW_SendDispInfoNotify (
          hwnd, 
          wineItem, 
          TVN_GETDISPINFO, 
          TVIF_SELECTEDIMAGE);

      imageIndex = wineItem->iSelectedImage;

	  } else { 
      /* This item is not selected */
		  if (wineItem->iImage == I_IMAGECALLBACK) 
			  TREEVIEW_SendDispInfoNotify ( 
          hwnd, 
          wineItem, 
          TVN_GETDISPINFO, 
          TVIF_IMAGE);

      imageIndex = wineItem->iImage;
  	}
 
    if (himlp)
    { 
      /* We found an image to display? Draw it. */
     	ImageList_Draw (
        *himlp, 
        wineItem->iImage,
        hdc, 
        xpos-2, 
        r.top+1, 
        ILD_NORMAL);

   	  ImageList_GetIconSize (*himlp, &cx, &cy);
   	  xpos+=cx;
    }
  }

  /* 
   * Display the text assiciated with this item
   */
  r.left=xpos;
  if ((wineItem->mask & TVIF_TEXT) && (wineItem->pszText)) 
  {
    COLORREF oldBkColor = 0;
    COLORREF oldTextColor = 0;
    INT      oldBkMode;

    if (wineItem->state & (TVIS_SELECTED | TVIS_DROPHILITED) ) {
      oldBkMode    = SetBkMode   (hdc, OPAQUE);
		  oldBkColor   = SetBkColor  (hdc, GetSysColor( COLOR_HIGHLIGHT));
		  oldTextColor = SetTextColor(hdc, GetSysColor( COLOR_HIGHLIGHTTEXT));
    } 
    else 
    {
    	oldBkMode    = SetBkMode(hdc, TRANSPARENT);
		  oldTextColor = SetTextColor(hdc, GetSysColor( COLOR_WINDOWTEXT));
    }

    r.left += 3;
    r.right -= 3;
    wineItem->text.left=r.left;
    wineItem->text.right=r.right;

    if (wineItem->pszText== LPSTR_TEXTCALLBACKA) {
      TRACE (treeview,"LPSTR_TEXTCALLBACK\n");
      TREEVIEW_SendDispInfoNotify (hwnd, wineItem, TVN_GETDISPINFO, TVIF_TEXT);
    }

    DrawTextA (
      hdc, 
      wineItem->pszText, 
      lstrlenA(wineItem->pszText), 
      &r, 
      uTextJustify|DT_VCENTER|DT_SINGLELINE);

    /* 
     * FIXME if not always done, the bmp following the selection are not 
     * visible ????
     */
    SetTextColor( hdc, oldTextColor);

    if (oldBkMode != TRANSPARENT)
      SetBkMode(hdc, oldBkMode);
    if (wineItem->state & (TVIS_SELECTED | TVIS_DROPHILITED))
      SetBkColor (hdc, oldBkColor);
  }

  if (cditem & CDRF_NOTIFYPOSTPAINT)
		TREEVIEW_SendCustomDrawItemNotify (hwnd, hdc, wineItem, CDDS_ITEMPOSTPAINT);

  SelectObject (hdc, hOldFont);
}


static LRESULT
TREEVIEW_GetItemRect (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  TREEVIEW_ITEM *wineItem;
  HTREEITEM *iItem;
  LPRECT lpRect;

  TRACE (treeview,"\n");
  if (infoPtr==NULL) return FALSE;

  if (infoPtr->Timer & TV_REFRESH_TIMER_SET)          
		TREEVIEW_Refresh (hwnd);	/* we want a rect for the current view */
  
  iItem = (HTREEITEM *) lParam;
  wineItem = TREEVIEW_ValidItem (infoPtr, *iItem);
  if (!wineItem) return FALSE;

  wineItem=& infoPtr->items[ (INT)*iItem ];
  if (!wineItem->visible) return FALSE; 

  lpRect = (LPRECT)lParam;
  if (lpRect == NULL) return FALSE;
	
  if ((INT) wParam) {
  	lpRect->left	= wineItem->text.left;
	lpRect->right	= wineItem->text.right;
	lpRect->bottom	= wineItem->text.bottom;
	lpRect->top	    = wineItem->text.top;
  } else {
	lpRect->left 	= wineItem->rect.left;
	lpRect->right	= wineItem->rect.right;
	lpRect->bottom  = wineItem->rect.bottom;
	lpRect->top	= wineItem->rect.top;
  }

  TRACE (treeview,"[L:%d R:%d T:%d B:%d]\n", lpRect->left,lpRect->right,
									lpRect->top,lpRect->bottom);
  return TRUE;
}

static LRESULT
TREEVIEW_GetVisibleCount (HWND hwnd,  WPARAM wParam, LPARAM lParam)

{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);

  return (LRESULT) infoPtr->uVisibleHeight / infoPtr->uRealItemHeight;
}



static LRESULT
TREEVIEW_SetItemA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  TREEVIEW_ITEM *wineItem;
  TVITEMEXA *tvItem;
  INT iItem,len;

  tvItem=(LPTVITEMEXA) lParam;
  iItem=(INT)tvItem->hItem;
  TRACE (treeview,"item %d,mask %x\n",iItem,tvItem->mask);

  wineItem = TREEVIEW_ValidItem (infoPtr, (HTREEITEM)iItem);
  if (!wineItem) return FALSE;

  if (tvItem->mask & TVIF_CHILDREN) {
        wineItem->cChildren=tvItem->cChildren;
  }

  if (tvItem->mask & TVIF_IMAGE) {
       wineItem->iImage=tvItem->iImage;
  }

  if (tvItem->mask & TVIF_INTEGRAL) {
        wineItem->iIntegral=tvItem->iIntegral; 
        FIXME (treeview," TVIF_INTEGRAL not supported yet\n");
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
		if (tvItem->pszText!=LPSTR_TEXTCALLBACKA) {
        len=lstrlenA (tvItem->pszText);
        if (len>wineItem->cchTextMax) 
			wineItem->pszText= COMCTL32_ReAlloc (wineItem->pszText, len+1);
        lstrcpynA (wineItem->pszText, tvItem->pszText,len);
		} else {
			if (wineItem->cchTextMax) {
				COMCTL32_Free (wineItem->pszText);
				wineItem->cchTextMax=0;
			}
		wineItem->pszText=LPSTR_TEXTCALLBACKA;
		}
   }

  return TRUE;
}





static void
TREEVIEW_Refresh (HWND hwnd)

{
    TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
	TEXTMETRICA tm;
	HBRUSH hbrBk;
    RECT rect;
	HDC hdc;
    INT iItem, indent, x, y, cx, height, itemHeight;
    INT viewtop,viewbottom,viewleft,viewright;
    TREEVIEW_ITEM *wineItem, *prevItem;

    TRACE (treeview,"\n");

	hdc=GetDC (hwnd);

    if (infoPtr->Timer & TV_REFRESH_TIMER_SET) {
		KillTimer (hwnd, TV_REFRESH_TIMER);
		infoPtr->Timer &= ~TV_REFRESH_TIMER_SET;
    }

    
    GetClientRect (hwnd, &rect);
    if ((rect.left-rect.right ==0) || (rect.top-rect.bottom==0)) return;

    infoPtr->cdmode=TREEVIEW_SendCustomDrawNotify 
						(hwnd, CDDS_PREPAINT, hdc, rect);

	if (infoPtr->cdmode==CDRF_SKIPDEFAULT) {
		  ReleaseDC (hwnd, hdc);
		  return;
	}

	infoPtr->uVisibleHeight= rect.bottom-rect.top;
	infoPtr->uVisibleWidth= rect.right-rect.left;

    viewtop=infoPtr->cy;
    viewbottom=infoPtr->cy + rect.bottom-rect.top;
    viewleft=infoPtr->cx;
    viewright=infoPtr->cx + rect.right-rect.left;



    /* draw background */
    
    hbrBk = GetSysColorBrush (COLOR_WINDOW);
    FillRect(hdc, &rect, hbrBk);


    iItem=(INT)infoPtr->TopRootItem;
    infoPtr->firstVisible=0;
    wineItem=NULL;
    indent=0;
    x=y=0;
    TRACE (treeview, "[%d %d %d %d]\n",viewtop,viewbottom,viewleft,viewright);

    while (iItem) {
		prevItem=wineItem;
        wineItem= & infoPtr->items[iItem];
		wineItem->iLevel=indent;

        ImageList_GetIconSize (infoPtr->himlNormal, &cx, &itemHeight);
        if (infoPtr->uItemHeight>itemHeight)
		    itemHeight=infoPtr->uItemHeight;

	    GetTextMetricsA (hdc, &tm);
 	    if ((tm.tmHeight + tm.tmExternalLeading) > itemHeight)
		     itemHeight=tm.tmHeight + tm.tmExternalLeading;

        infoPtr->uRealItemHeight=itemHeight;	


/* FIXME: remove this in later stage  */
/*
		if (wineItem->pszText!=LPSTR_TEXTCALLBACK32A) 
		TRACE (treeview, "%d %d [%d %d %d %d] (%s)\n",y,x,
			wineItem->rect.top, wineItem->rect.bottom,
			wineItem->rect.left, wineItem->rect.right,
			wineItem->pszText);
		else 
		TRACE (treeview, "%d [%d %d %d %d] (CALLBACK)\n",
				wineItem->hItem,
				wineItem->rect.top, wineItem->rect.bottom,
				wineItem->rect.left, wineItem->rect.right);
*/

		height=itemHeight * wineItem->iIntegral +1;
		if ((y >= viewtop) && (y <= viewbottom) &&
	    	(x >= viewleft  ) && (x <= viewright)) {
				wineItem->visible = TRUE;
        		wineItem->rect.top = y - infoPtr->cy + rect.top;
        		wineItem->rect.bottom = wineItem->rect.top + height ;
         		wineItem->rect.left = x - infoPtr->cx + rect.left;
        		wineItem->rect.right = rect.right;
				wineItem->text.left = wineItem->rect.left;
				wineItem->text.top  = wineItem->rect.top;
				wineItem->text.right= wineItem->rect.right;
				wineItem->text.bottom=wineItem->rect.bottom;
			if (!infoPtr->firstVisible)
				infoPtr->firstVisible=wineItem->hItem;
       		TREEVIEW_DrawItem (hwnd, hdc, wineItem);
		}
		else {
			wineItem->visible   = FALSE;
			wineItem->rect.left = wineItem->rect.top    = 0;
			wineItem->rect.right= wineItem->rect.bottom = 0;
			wineItem->text.left = wineItem->text.top    = 0;
			wineItem->text.right= wineItem->text.bottom = 0;
 		}

		/* look up next item */
	
		if ((wineItem->firstChild) && (wineItem->state & TVIS_EXPANDED)) {
			iItem=(INT)wineItem->firstChild;
			indent++;
			x+=infoPtr->uIndent;
			if (x>infoPtr->uTotalWidth) 	
				infoPtr->uTotalWidth=x;
		}
		else {
			iItem=(INT)wineItem->sibling;
			while ((!iItem) && (indent>0)) {
				indent--;
				x-=infoPtr->uIndent;
				prevItem=wineItem;
				wineItem=&infoPtr->items[(INT)wineItem->parent];
				iItem=(INT)wineItem->sibling;
			}
		}
        y +=height;
    }				/* while */

/* FIXME: infoPtr->uTotalWidth should also take item label into account */
/* FIXME: or should query item sizes (ie check CDRF_NEWFONT) */

    infoPtr->uTotalHeight=y;
    if (y >= (viewbottom-viewtop)) {
 		if (!(infoPtr->uInternalStatus & TV_VSCROLL))
			ShowScrollBar (hwnd, SB_VERT, TRUE);
		infoPtr->uInternalStatus |=TV_VSCROLL;
 		SetScrollRange (hwnd, SB_VERT, 0, 
					y - infoPtr->uVisibleHeight, FALSE);
		SetScrollPos (hwnd, SB_VERT, infoPtr->cy, TRUE);
	}
    else {
		if (infoPtr->uInternalStatus & TV_VSCROLL) 
			ShowScrollBar (hwnd, SB_VERT, FALSE);
		infoPtr->uInternalStatus &= ~TV_VSCROLL;
	}


	if (infoPtr->cdmode & CDRF_NOTIFYPOSTPAINT) 
    	infoPtr->cdmode=TREEVIEW_SendCustomDrawNotify 
								(hwnd, CDDS_POSTPAINT, hdc, rect);

    ReleaseDC (hwnd, hdc);
    TRACE (treeview,"done\n");
}


static LRESULT 
TREEVIEW_HandleTimer (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);

  TRACE (treeview, " %d\n",wParam);
  if (!infoPtr) return FALSE;

  switch (wParam) {
	case TV_REFRESH_TIMER:
		KillTimer (hwnd, TV_REFRESH_TIMER);
		infoPtr->Timer &= ~TV_REFRESH_TIMER_SET;
		SendMessageA (hwnd, WM_PAINT, 0, 0);
		return 0;
	case TV_EDIT_TIMER:
		KillTimer (hwnd, TV_EDIT_TIMER);
		infoPtr->Timer &= ~TV_EDIT_TIMER_SET;
		return 0;
	default:
		ERR (treeview,"got unknown timer\n");
 }
		
 return 1;
}


static void
TREEVIEW_QueueRefresh (HWND hwnd)

{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);

 TRACE (treeview,"\n");
 if (infoPtr->Timer & TV_REFRESH_TIMER_SET) {
	KillTimer (hwnd, TV_REFRESH_TIMER);
 }

 SetTimer (hwnd, TV_REFRESH_TIMER, TV_REFRESH_DELAY, 0);
 infoPtr->Timer|=TV_REFRESH_TIMER_SET;
}



static LRESULT
TREEVIEW_GetItemA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  LPTVITEMEXA    tvItem;
  TREEVIEW_ITEM *wineItem;
  INT         iItem;

  tvItem=(LPTVITEMEXA) lParam;
  iItem=(INT)tvItem->hItem;
  TRACE (treeview,"item %d<%p>, txt %p, img %p, action %x\n", iItem,
tvItem, tvItem->pszText, & tvItem->iImage, tvItem->mask);

  wineItem = TREEVIEW_ValidItem (infoPtr, (HTREEITEM)iItem);
  if (!wineItem) return FALSE;

   if (tvItem->mask & TVIF_CHILDREN) {
		if (TVIF_CHILDREN==I_CHILDRENCALLBACK) 
			FIXME (treeview,"I_CHILDRENCALLBACK not supported\n");
        tvItem->cChildren=wineItem->cChildren;
   }

   if (tvItem->mask & TVIF_HANDLE) {
        tvItem->hItem=wineItem->hItem;
   }

   if (tvItem->mask & TVIF_IMAGE) {
        tvItem->iImage=wineItem->iImage;
   }

   if (tvItem->mask & TVIF_INTEGRAL) {
        tvItem->iIntegral=wineItem->iIntegral; 
		FIXME (treeview," TVIF_INTEGRAL not supported yet\n");
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
	if (wineItem->pszText == LPSTR_TEXTCALLBACKA) {
	    tvItem->pszText = LPSTR_TEXTCALLBACKA;  /* FIXME:send notification? */
		ERR (treeview," GetItem called with LPSTR_TEXTCALLBACK\n");
	}
	else if (wineItem->pszText) {
	    lstrcpynA (tvItem->pszText, wineItem->pszText, tvItem->cchTextMax);
	}
   }

  return TRUE;
}



/* FIXME: check implementation of TVGN_NEXT/TVGN_NEXTVISIBLE */

static LRESULT
TREEVIEW_GetNextItem (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  TREEVIEW_ITEM *wineItem, *returnItem;
  INT iItem, retval, flag;


  if (!infoPtr) return FALSE;
  flag  = (INT) wParam;
  iItem = (INT) lParam;
  retval=0;
  switch (flag) {
	case TVGN_ROOT: retval=(INT)infoPtr->TopRootItem;
					break;
	case TVGN_CARET:retval=(INT)infoPtr->selectedItem;
					break;
	case TVGN_FIRSTVISIBLE: 
         			TREEVIEW_Refresh (hwnd);       
/* FIXME:we should only recalculate, not redraw */
					retval=(INT)infoPtr->firstVisible;
					break;
	case TVGN_DROPHILITE:
					retval=(INT)infoPtr->dropItem;
					break;
	}
  if (retval) {
  		TRACE (treeview,"flags:%x, returns %u\n", flag, retval);
		return retval;
  }
 
  wineItem = TREEVIEW_ValidItem (infoPtr, (HTREEITEM)iItem);
  returnItem = NULL;
  if (!wineItem) return FALSE;

  switch (flag)	{
	case TVGN_NEXT: retval=(INT)wineItem->sibling;
					break;
	case TVGN_PREVIOUS:	
					retval=(INT)wineItem->upsibling;
					break;
	case TVGN_PARENT:
					retval=(INT)wineItem->parent;
					break;
	case TVGN_CHILD:
					retval=(INT)wineItem->firstChild;
					break;
	case TVGN_LASTVISIBLE:  
					returnItem=TREEVIEW_GetLastListItem (infoPtr,wineItem);
					break;
	case TVGN_NEXTVISIBLE:  
					returnItem=TREEVIEW_GetNextListItem (infoPtr,wineItem);
					break;
	case TVGN_PREVIOUSVISIBLE: 
					returnItem=TREEVIEW_GetPrevListItem (infoPtr, wineItem);
					break;
	default:		FIXME (treeview,"Unknown msg %x,item %x\n", flag,iItem);
					break;
	}

  if (returnItem) {
		  TRACE (treeview,"flags:%x, item %d;returns %d\n", flag, iItem,
							(INT)returnItem->hItem);
		  return (INT)returnItem->hItem;
  }

  TRACE (treeview,"flags:%x, item %d;returns %d\n", flag, iItem,retval);
  return retval;
}


static LRESULT
TREEVIEW_GetCount (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);

 TRACE (treeview," %d\n",infoPtr->uNumItems);
 return (LRESULT) infoPtr->uNumItems;
}

/***************************************************************************
 * This method does the chaining of the insertion of a treeview item 
 * before an item.
 */
static void TREEVIEW_InsertBefore(
    TREEVIEW_INFO *infoPtr,
    TREEVIEW_ITEM *newItem, 
    TREEVIEW_ITEM *sibling,
    TREEVIEW_ITEM *parent)
{
  HTREEITEM     siblingHandle   = 0;
  HTREEITEM     upSiblingHandle = 0;
  TREEVIEW_ITEM *upSibling      = NULL;

  if (newItem == NULL)
    ERR(treeview, "NULL newItem, impossible condition\n");

  if (parent == NULL)
    ERR(treeview, "NULL parent, impossible condition\n");

  if (sibling != NULL) /* Insert before this sibling for this parent */
  { 
    /* Store the new item sibling up sibling and sibling tem handle */
    siblingHandle   = sibling->hItem;
    upSiblingHandle = sibling->upsibling;
    /* As well as a pointer to the upsibling sibling object */
    if ( (INT)sibling->upsibling != 0 )
      upSibling = &infoPtr->items[(INT)sibling->upsibling];
  
    /* Adjust the sibling pointer */
    sibling->upsibling = newItem->hItem;
    
    /* Adjust the new item pointers */
    newItem->upsibling = upSiblingHandle;
    newItem->sibling   = siblingHandle;
    
    /* Adjust the up sibling pointer */
    if ( upSibling != NULL )        
      upSibling->sibling = newItem->hItem;
    else
      /* this item is the first child of this parent, adjust parent pointers */
      parent->firstChild = newItem->hItem;
  }
  else /* Insert as first child of this parent */
    parent->firstChild = newItem->hItem;
}

/***************************************************************************
 * This method does the chaining of the insertion of a treeview item 
 * after an item.
 */
static void TREEVIEW_InsertAfter(
    TREEVIEW_INFO *infoPtr,
    TREEVIEW_ITEM *newItem, 
    TREEVIEW_ITEM *upSibling,
    TREEVIEW_ITEM *parent)
{
  HTREEITEM     upSiblingHandle = 0;
  HTREEITEM     siblingHandle   = 0;
  TREEVIEW_ITEM *sibling        = NULL;

  if (newItem == NULL)
    ERR(treeview, "NULL newItem, impossible condition\n");

  if (parent == NULL)
    ERR(treeview, "NULL parent, impossible condition\n");

  if (upSibling != NULL) /* Insert after this upsibling for this parent */
  { 
    /* Store the new item up sibling and sibling item handle */
    upSiblingHandle = upSibling->hItem;
    siblingHandle   = upSibling->sibling;
    /* As well as a pointer to the upsibling sibling object */
    if ( (INT)upSibling->sibling != 0 )
      sibling = &infoPtr->items[(INT)upSibling->sibling];
  
    /* Adjust the up sibling pointer */
    upSibling->sibling = newItem->hItem;
    
    /* Adjust the new item pointers */
    newItem->upsibling = upSiblingHandle;
    newItem->sibling   = siblingHandle;
    
    /* Adjust the sibling pointer */
    if ( sibling != NULL )        
      sibling->upsibling = newItem->hItem; 
    /*
    else 
      newItem is the last of the level, nothing else to do 
    */
  }
  else /* Insert as first child of this parent */
    parent->firstChild = newItem->hItem;
}

/***************************************************************************
 * This method is used to abstract the comparaison of two items during 
 * the insertion or a request for a sort.
 */
static INT TREEVIEW_CompareItems(
  TREEVIEW_INFO *infoPtr, 
  TREEVIEW_ITEM *first,   
  TREEVIEW_ITEM *second)
{

  if ((first == NULL) || (second == NULL))
    ERR( treeview, 
      "Invalid parameters passed infoPtr=%p, first=%p, second=%p.\n",
      infoPtr,
      first,
      second);

  if (infoPtr->pCallBackSort == NULL)
  {
    /* Simply sort based on the label comparaison */
    return strcmp(first->pszText, second->pszText);
  }
  else
  {
    /* use the callback method setup in tree view struct */
    if( (first->lParam == 0) || (second->lParam == 0))
      ERR( treeview, "Invalid lParam, first=%08lx, second=%08lx.\n", 
        first->lParam, 
        second->lParam);

    return (infoPtr->pCallBackSort->lpfnCompare)(
              first->lParam,
              second->lParam, 
              infoPtr->pCallBackSort->lParam);
  }
}

/***************************************************************************
 * Forward the DPA local callback to the treeview owner callback
 */
static INT TREEVIEW_CallBackCompare( 
  LPVOID first, 
  LPVOID second, 
  LPARAM tvInfoPtr)
{
  /* Forward the call to the client define callback */
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr((HWND)tvInfoPtr);
  return (infoPtr->pCallBackSort->lpfnCompare)(
    ((TREEVIEW_ITEM*)first)->lParam,
    ((TREEVIEW_ITEM*)second)->lParam,
    infoPtr->pCallBackSort->lParam);
}

/***************************************************************************
 * Setup the treeview structure with regards of the sort method
 * and sort the children of the TV item specified in lParam
 */
LRESULT WINAPI TREEVIEW_SortChildrenCB(
  HWND   hwnd, 
  WPARAM wParam, 
  LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  TREEVIEW_ITEM *sortMe  = NULL; /* Node for which we sort the children */

  /* Obtain the TVSORTBC struct */
  infoPtr->pCallBackSort = (LPTVSORTCB)lParam;

  /* Obtain the parent node to sort */  
  sortMe = &infoPtr->items[ (INT)infoPtr->pCallBackSort->hParent ];

  /* Make sure there is something to sort */
  if ( sortMe->cChildren > 1 ) 
  {
    /* pointer organization */
    HDPA          sortList   = DPA_Create(sortMe->cChildren);
    HTREEITEM     itemHandle = sortMe->firstChild;
    TREEVIEW_ITEM *itemPtr   = & infoPtr->items[ (INT)itemHandle ];

    /* TREEVIEW_ITEM rechaining */
    INT  count     = 0;
    VOID *item     = 0;
    VOID *nextItem = 0;
    VOID *prevItem = 0;

    /* Build the list of item to sort */
    do 
    {
      DPA_InsertPtr(
        sortList,              /* the list */
        sortMe->cChildren+1,   /* force the insertion to be an append */
        itemPtr);              /* the ptr to store */   

      /* Get the next sibling */
      itemHandle = itemPtr->sibling;
      itemPtr    = & infoPtr->items[ (INT)itemHandle ];
    } while ( itemHandle != NULL );

    /* let DPA perform the sort activity */
    DPA_Sort(
      sortList,                  /* what  */ 
      TREEVIEW_CallBackCompare,  /* how   */
      hwnd);                     /* owner */

    /* 
     * Reorganized TREEVIEW_ITEM structures. 
     * Note that we know we have at least two elements.
     */

    /* Get the first item and get ready to start... */
    item = DPA_GetPtr(sortList, count++);    
    while ( (nextItem = DPA_GetPtr(sortList, count++)) != NULL )
    {
      /* link the two current item toghether */
      ((TREEVIEW_ITEM*)item)->sibling       = ((TREEVIEW_ITEM*)nextItem)->hItem;
      ((TREEVIEW_ITEM*)nextItem)->upsibling = ((TREEVIEW_ITEM*)item)->hItem;

      if (prevItem == NULL) /* this is the first item, update the parent */
      {
        sortMe->firstChild                = ((TREEVIEW_ITEM*)item)->hItem;
        ((TREEVIEW_ITEM*)item)->upsibling = NULL;
      }
      else                  /* fix the back chaining */
      {
        ((TREEVIEW_ITEM*)item)->upsibling = ((TREEVIEW_ITEM*)prevItem)->hItem;
      }

      /* get ready for the next one */
      prevItem = item; 
      item     = nextItem;
    }

    /* the last item is pointed to by item and never has a sibling */
    ((TREEVIEW_ITEM*)item)->sibling = NULL; 

    DPA_Destroy(sortList);

    return TRUE;
  }
  return FALSE;
}


/* the method used below isn't the most memory-friendly, but it avoids 
   a lot of memory reallocations */ 

/* BTW: we waste handle 0; 0 is not an allowed handle. */

static LRESULT
TREEVIEW_InsertItemA (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  TVINSERTSTRUCTA  *ptdi;
  TVITEMEXA 	*tvItem;
  TREEVIEW_ITEM *wineItem, *parentItem, *prevsib, *sibItem;
  INT		iItem,listItems,i,len;
 
  /* Item to insert */
  ptdi = (LPTVINSERTSTRUCTA) lParam;

	/* check if memory is available */

  if (infoPtr->uNumPtrsAlloced==0) {
        infoPtr->items = COMCTL32_Alloc (TVITEM_ALLOC*sizeof (TREEVIEW_ITEM));
        infoPtr->freeList= COMCTL32_Alloc ((1+(TVITEM_ALLOC>>5)) * sizeof (INT));
        infoPtr->uNumPtrsAlloced=TVITEM_ALLOC;
	infoPtr->TopRootItem=(HTREEITEM)1;
   }

  /* 
   * Reallocate contiguous space for items 
   */
  if (infoPtr->uNumItems == (infoPtr->uNumPtrsAlloced-1) ) {
   	TREEVIEW_ITEM *oldItems = infoPtr->items;
	INT *oldfreeList = infoPtr->freeList;

	infoPtr->uNumPtrsAlloced*=2;
    infoPtr->items = COMCTL32_Alloc (infoPtr->uNumPtrsAlloced*sizeof (TREEVIEW_ITEM));
    infoPtr->freeList= COMCTL32_Alloc ((1+(infoPtr->uNumPtrsAlloced>>5))*sizeof (INT));

    memcpy (&infoPtr->items[0], &oldItems[0],
                    infoPtr->uNumPtrsAlloced/2 * sizeof(TREEVIEW_ITEM));
    memcpy (&infoPtr->freeList[0], &oldfreeList[0],
                    infoPtr->uNumPtrsAlloced>>6 * sizeof(INT));

    COMCTL32_Free (oldItems);  
    COMCTL32_Free (oldfreeList);  
   }

  /* 
   * Reset infoPtr structure with new stat according to current TV picture
   */
  iItem=0;
  infoPtr->uNumItems++;
  if ((INT)infoPtr->uMaxHandle==(infoPtr->uNumItems-1))  { 
  	iItem=infoPtr->uNumItems;
  	infoPtr->uMaxHandle = (HTREEITEM)((INT)infoPtr->uMaxHandle + 1);
  } else {					 /* check freelist */
  	for (i=0; i<infoPtr->uNumPtrsAlloced>>5; i++) {
  		if (infoPtr->freeList[i]) {
  			iItem=ffs (infoPtr->freeList[i])-1;
  			tv_clear_bit(iItem,&infoPtr->freeList[i]);
   			iItem+=i<<5;
  			break;
  		}
    } 
  }

  /* little performace enhancement... */  
  if (TRACE_ON(treeview)) { 
    for (i=0; i<infoPtr->uNumPtrsAlloced>>5; i++) 
	    TRACE (treeview,"%8x\n",infoPtr->freeList[i]);
  }

  if (!iItem) ERR (treeview, "Argh -- can't find free item.\n");

  /* 
   * Find the parent item of the new item 
   */  
  tvItem= & ptdi->DUMMYUNIONNAME.itemex;
  wineItem=& infoPtr->items[iItem];

  if ((ptdi->hParent==TVI_ROOT) || (ptdi->hParent==0)) {
    parentItem       = NULL;
    wineItem->parent = 0; 
    sibItem          = &infoPtr->items [(INT)infoPtr->TopRootItem];
    listItems        = infoPtr->uNumItems;
  }
  else  {
  	parentItem = &infoPtr->items[(INT)ptdi->hParent];
  
    /* Do the insertion here it if it's the only item of this parent */
  	if (!parentItem->firstChild) 
  		parentItem->firstChild=(HTREEITEM)iItem;
  
  	wineItem->parent = ptdi->hParent;
  	sibItem          = &infoPtr->items [(INT)parentItem->firstChild];
  	parentItem->cChildren++;
  	listItems        = parentItem->cChildren;
  }

  
  /* NOTE: I am moving some setup of the wineItem object that was initialy 
   *       done at the end of the function since some of the values are 
   *       required by the Callback sorting 
   */

  if (tvItem->mask & TVIF_TEXT) 
  {
    /*
     * Setup the item text stuff here since it's required by the Sort method
     * when the insertion are ordered
     */
    if (tvItem->pszText!=LPSTR_TEXTCALLBACKA) 
    {
      TRACE (treeview,"(%p,%s)\n", &tvItem->pszText, tvItem->pszText); 
      len = lstrlenA (tvItem->pszText)+1;
      wineItem->pszText= COMCTL32_Alloc (len+1);
      lstrcpyA (wineItem->pszText, tvItem->pszText);
      wineItem->cchTextMax=len;
    }
    else 
    {
      TRACE (treeview,"LPSTR_TEXTCALLBACK\n");
      wineItem->pszText = LPSTR_TEXTCALLBACKA;
      wineItem->cchTextMax = 0;
    }
  }

  if (tvItem->mask & TVIF_PARAM) 
    wineItem->lParam=tvItem->lParam;


  wineItem->upsibling=0;  /* needed in case we're the first item in a list */ 
  wineItem->sibling=0;     
  wineItem->firstChild=0;
  wineItem->hItem=(HTREEITEM)iItem;

  if (listItems>1) {
     prevsib=NULL;

     switch ((INT)ptdi->hInsertAfter) {
		case TVI_FIRST: 
			if (wineItem->parent) {
				wineItem->sibling=parentItem->firstChild;
				parentItem->firstChild=(HTREEITEM)iItem;
			} else {
				wineItem->sibling=infoPtr->TopRootItem;
				infoPtr->TopRootItem=(HTREEITEM)iItem;
			}
			sibItem->upsibling=(HTREEITEM)iItem;
			break;

		case TVI_SORT:  
  	  if (sibItem==wineItem) 
        /* 
         * This item is the first child of the level and it 
         * has already been inserted 
         */                
        break; 
      else
      {
        TREEVIEW_ITEM *aChild        = 
          &infoPtr->items[(INT)parentItem->firstChild];
  
        TREEVIEW_ITEM *previousChild = NULL;
        BOOL bItemInserted           = FALSE;
  
        /* Iterate the parent children to see where we fit in */
        while ( aChild != NULL )
        {
          INT comp = strcmp(wineItem->pszText, aChild->pszText);
          if ( comp < 0 )  /* we are smaller than the current one */
          {
            TREEVIEW_InsertBefore(infoPtr, wineItem, aChild, parentItem);
            bItemInserted = TRUE;
            break;
          }
          else if ( comp > 0 )  /* we are bigger than the current one */
          {
            previousChild = aChild;
            aChild = (aChild->sibling == 0)  /* This will help us to exit   */
                        ? NULL               /* if there is no more sibling */
                        : &infoPtr->items[(INT)aChild->sibling];
  
            /* Look at the next item */
            continue;
          }
          else if ( comp == 0 )
          {
            /* 
             * An item with this name is already existing, therefore,  
             * we add after the one we found 
             */
            TREEVIEW_InsertAfter(infoPtr, wineItem, aChild, parentItem);
            bItemInserted = TRUE;
            break;
          }
        }
      
        /* 
         * we reach the end of the child list and the item as not
         * yet been inserted, therefore, insert it after the last child.
         */
        if ( (! bItemInserted ) && (aChild == NULL) )
          TREEVIEW_InsertAfter(infoPtr, wineItem, previousChild, parentItem);
  
        break;
      }


		case TVI_LAST:  
			if (sibItem==wineItem) break;
			while (sibItem->sibling) {
				prevsib=sibItem;
				sibItem=&infoPtr->items [(INT)sibItem->sibling];
			}
			sibItem->sibling=(HTREEITEM)iItem;
			wineItem->upsibling=sibItem->hItem;
			break;
		default:
			while ((sibItem->sibling) && (sibItem->hItem!=ptdi->hInsertAfter))
				{
				prevsib=sibItem;
                sibItem=&infoPtr->items [(INT)sibItem->sibling];
              }
			if (sibItem->hItem!=ptdi->hInsertAfter) {
			 ERR (treeview, "tried to insert item after nonexisting handle.\n");
			 break;
			}
			prevsib=sibItem;
			if (sibItem->sibling) {
            	sibItem=&infoPtr->items [(INT)sibItem->sibling];
				sibItem->upsibling=(HTREEITEM)iItem;
				wineItem->sibling=sibItem->hItem;
			}
			prevsib->sibling=(HTREEITEM)iItem;
			wineItem->upsibling=prevsib->hItem;
			break;
   	}
   }	


/* Fill in info structure */

   TRACE (treeview,"new item %d; parent %d, mask %x\n", iItem, 
			(INT)wineItem->parent,tvItem->mask);

   wineItem->mask=tvItem->mask;
   wineItem->iIntegral=1; 

   if (tvItem->mask & TVIF_CHILDREN) {
	 wineItem->cChildren=tvItem->cChildren;
	 if (tvItem->cChildren==I_CHILDRENCALLBACK) 
			FIXME (treeview," I_CHILDRENCALLBACK not supported\n");
	}

  wineItem->expandBox.left   = 0; /* Initialize the expandBox */
  wineItem->expandBox.top    = 0;
  wineItem->expandBox.right  = 0;
  wineItem->expandBox.bottom = 0;

   if (tvItem->mask & TVIF_IMAGE) 
	wineItem->iImage=tvItem->iImage;

		/* If the application sets TVIF_INTEGRAL without
			supplying a TVITEMEX structure, it's toast */

   if (tvItem->mask & TVIF_INTEGRAL) 
   		wineItem->iIntegral=tvItem->iIntegral;   

   if (tvItem->mask & TVIF_SELECTEDIMAGE) 
	wineItem->iSelectedImage=tvItem->iSelectedImage;

   if (tvItem->mask & TVIF_STATE) {
	wineItem->state=tvItem->state;
	wineItem->stateMask=tvItem->stateMask;
   }


   TREEVIEW_QueueRefresh (hwnd);

   return (LRESULT) iItem;
}





static LRESULT
TREEVIEW_DeleteItem (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  INT iItem;
  TREEVIEW_ITEM *wineItem;

  TRACE (treeview,"\n");
  if (!infoPtr) return FALSE;

  if (lParam == (INT)TVI_ROOT) {
	TREEVIEW_RemoveTree (hwnd);
  } else {
  	iItem= (INT) lParam;
  	wineItem = TREEVIEW_ValidItem (infoPtr, (HTREEITEM)iItem);
  	if (!wineItem) return FALSE;
    TRACE (treeview,"%s\n",wineItem->pszText);
	TREEVIEW_RemoveItem (hwnd, wineItem);
  }

  TREEVIEW_QueueRefresh (hwnd);

  return TRUE;
}



static LRESULT
TREEVIEW_GetIndent (HWND hwnd)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);

 TRACE (treeview,"\n");
 return infoPtr->uIndent;
}

static LRESULT
TREEVIEW_SetIndent (HWND hwnd, WPARAM wParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  INT newIndent;
   
  TRACE (treeview,"\n");
  newIndent=(INT) wParam;
  if (newIndent < MINIMUM_INDENT) newIndent=MINIMUM_INDENT;
  infoPtr->uIndent=newIndent;
  
  return 0;
}

static LRESULT
TREEVIEW_GetToolTips (HWND hwnd)

{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);

 TRACE (treeview,"\n");
 return infoPtr->hwndToolTip;
}


static LRESULT
TREEVIEW_SetToolTips (HWND hwnd, WPARAM wParam)

{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
 HWND prevToolTip;

 TRACE (treeview,"\n");
 prevToolTip=infoPtr->hwndToolTip;
 infoPtr->hwndToolTip= (HWND) wParam;

 return prevToolTip;
}


LRESULT CALLBACK
TREEVIEW_GetEditControl (HWND hwnd)

{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);

 return infoPtr->hwndEdit;
}

LRESULT CALLBACK
TREEVIEW_Edit_SubclassProc (HWND hwnd, UINT uMsg, WPARAM wParam, 
							LPARAM lParam)
{
  switch (uMsg) {
   case WM_ERASEBKGND: {
           RECT rc;
           HDC  hdc = (HDC) wParam;
           GetClientRect (hwnd, &rc);
           Rectangle (hdc, rc.left, rc.top, rc.right, rc.bottom);
           return -1;
		}
   case WM_GETDLGCODE:
            return DLGC_WANTARROWS | DLGC_WANTALLKEYS;
   default:
			 return DefWindowProcA (hwnd, uMsg, wParam, lParam);
  }
  return 0;
}


/* should handle edit control messages here */

static LRESULT
TREEVIEW_Command (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
  TRACE (treeview, "%x %ld\n",wParam, lParam);
 
  switch (HIWORD(wParam)) {
		case EN_UPDATE:
			 FIXME (treeview, "got EN_UPDATE.\n");
			 break;
		case EN_KILLFOCUS:
			 FIXME (treeview, "got EN_KILLFOCUS.\n");
			 break;
		default:
			 return SendMessageA (GetParent (hwnd), 
										WM_COMMAND, wParam, lParam);
	}
 return 0;
}




static LRESULT
TREEVIEW_Size (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);

  if (infoPtr->bAutoSize) 
  {
    infoPtr->bAutoSize = FALSE;
    return 0;
  }
  infoPtr->bAutoSize = TRUE;

  if (wParam == SIZE_RESTORED)  
  {
    infoPtr->uTotalWidth  = LOWORD (lParam);
  	infoPtr->uTotalHeight = HIWORD (lParam);
  } else {
  	FIXME (treeview,"WM_SIZE flag %x %lx not handled\n", wParam, lParam);
  }

  TREEVIEW_QueueRefresh (hwnd);
  return 0;
}



static LRESULT
TREEVIEW_StyleChanged (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  LPSTYLESTRUCT lpss=(LPSTYLESTRUCT) lParam;

  TRACE (treeview,"(%x %lx)\n",wParam,lParam);
  
  if (wParam & (GWL_STYLE)) 
	 SetWindowLongA( hwnd, GWL_STYLE, lpss->styleNew);
  if (wParam & (GWL_EXSTYLE)) 
	 SetWindowLongA( hwnd, GWL_STYLE, lpss->styleNew);

  return 0;
}

static LRESULT
TREEVIEW_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TREEVIEW_INFO *infoPtr;
 	LOGFONTA logFont;
    TEXTMETRICA tm;
	HDC hdc;
  
    TRACE (treeview,"wnd %x\n",hwnd);
      /* allocate memory for info structure */
      infoPtr = (TREEVIEW_INFO *) COMCTL32_Alloc (sizeof(TREEVIEW_INFO));

     SetWindowLongA( hwnd, 0, (DWORD)infoPtr);

    if (infoPtr == NULL) {
		ERR (treeview, "could not allocate info memory!\n");
		return 0;
    }

    if ((TREEVIEW_INFO*) GetWindowLongA( hwnd, 0) != infoPtr) {
		ERR (treeview, "pointer assignment error!\n");
		return 0;
    }

	hdc=GetDC (hwnd);

    /* set default settings */
    infoPtr->uInternalStatus=0;
    infoPtr->uNumItems=0;
    infoPtr->clrBk = GetSysColor (COLOR_WINDOW);
    infoPtr->clrText = GetSysColor (COLOR_BTNTEXT);
    infoPtr->cy = 0;
    infoPtr->cx = 0;
    infoPtr->uIndent = 15;
    infoPtr->himlNormal = NULL;
    infoPtr->himlState = NULL;
	infoPtr->uItemHeight = -1;
    GetTextMetricsA (hdc, &tm);
    infoPtr->hFont = GetStockObject (DEFAULT_GUI_FONT);
	GetObjectA (infoPtr->hFont, sizeof (LOGFONTA), &logFont);
	logFont.lfWeight=FW_BOLD;
    infoPtr->hBoldFont = CreateFontIndirectA (&logFont);
    
    infoPtr->items = NULL;
    infoPtr->selectedItem=0;
    infoPtr->clrText=-1;	/* use system color */
    infoPtr->dropItem=0;
    infoPtr->pCallBackSort=NULL;

/*
    infoPtr->hwndNotify = GetParent32 (hwnd);
    infoPtr->bTransparent = ( GetWindowLongA( hwnd, GWL_STYLE) & TBSTYLE_FLAT);
*/

	infoPtr->hwndToolTip=0;
    if (!( GetWindowLongA( hwnd, GWL_STYLE) & TVS_NOTOOLTIPS)) {   /* Create tooltip control */
		TTTOOLINFOA ti;

		infoPtr->hwndToolTip =  
			CreateWindowExA (0, TOOLTIPS_CLASSA, NULL, 0,
                   CW_USEDEFAULT, CW_USEDEFAULT,
                   CW_USEDEFAULT, CW_USEDEFAULT,
                   hwnd, 0, 0, 0);

        /* Send NM_TOOLTIPSCREATED notification */
        if (infoPtr->hwndToolTip) {
            NMTOOLTIPSCREATED nmttc;

            nmttc.hdr.hwndFrom = hwnd;
            nmttc.hdr.idFrom =  GetWindowLongA( hwnd, GWL_ID);
            nmttc.hdr.code = NM_TOOLTIPSCREATED;
            nmttc.hwndToolTips = infoPtr->hwndToolTip;

            SendMessageA (GetParent (hwnd), WM_NOTIFY,
                (WPARAM) GetWindowLongA( hwnd, GWL_ID), (LPARAM)&nmttc);
        }

		ZeroMemory (&ti, sizeof(TTTOOLINFOA));
        ti.cbSize   = sizeof(TTTOOLINFOA);
        ti.uFlags   = TTF_IDISHWND | TTF_TRACK | TTF_TRANSPARENT ;
        ti.hwnd     = hwnd;
        ti.uId      = 0;
        ti.lpszText = "Test"; /* LPSTR_TEXTCALLBACK; */
        SetRectEmpty (&ti.rect);

        SendMessageA (infoPtr->hwndToolTip, TTM_ADDTOOLA, 0, (LPARAM)&ti);
    }


 	infoPtr->hwndEdit = CreateWindowExA ( 0, "EDIT",NULL,
                 WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_WANTRETURN,
                 0, 0, 0, 0,
                 hwnd, 0,0,0);
				/* FIXME:   (HMENU)IDTVEDIT, pcs->hInstance, 0); */

    SendMessageA ( infoPtr->hwndEdit, WM_SETFONT, infoPtr->hFont, FALSE);
	infoPtr->wpEditOrig= (WNDPROC)
                    SetWindowLongA (infoPtr->hwndEdit,GWL_WNDPROC, 
					(LONG) TREEVIEW_Edit_SubclassProc);

    ReleaseDC (hwnd, hdc);

    return 0;
}



static LRESULT 
TREEVIEW_Destroy (HWND hwnd) 
{
   TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
     
   TRACE (treeview,"\n");
   TREEVIEW_RemoveTree (hwnd);
   if (infoPtr->Timer & TV_REFRESH_TIMER_SET) 
        KillTimer (hwnd, TV_REFRESH_TIMER);
   if (infoPtr->hwndToolTip) 
		DestroyWindow (infoPtr->hwndToolTip);

   COMCTL32_Free (infoPtr);
   return 0;
}


static LRESULT
TREEVIEW_Paint (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    TRACE (treeview,"\n");
    hdc = wParam==0 ? BeginPaint (hwnd, &ps) : (HDC)wParam;
    TREEVIEW_Refresh (hwnd);
    if(!wParam)
        EndPaint (hwnd, &ps);
    TRACE (treeview,"done\n");
      
    return DefWindowProcA (hwnd, WM_PAINT, wParam, lParam);
}

static LRESULT
TREEVIEW_SetFocus (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
   TREEVIEW_SendSimpleNotify (hwnd, NM_SETFOCUS);
   SendMessageA (hwnd, WM_PAINT, 0, 0);
   return 0;
}

static LRESULT
TREEVIEW_KillFocus (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
   TREEVIEW_SendSimpleNotify (hwnd, NM_KILLFOCUS);
   SendMessageA (hwnd, WM_PAINT, 0, 0);
   return 0;
}

static LRESULT
TREEVIEW_EraseBackground (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
    HBRUSH hBrush = CreateSolidBrush (infoPtr->clrBk);
    RECT rect;

    TRACE (treeview,"\n");
    GetClientRect (hwnd, &rect);
    FillRect ((HDC)wParam, &rect, hBrush);
    DeleteObject (hBrush);
    return TRUE;
}





  
/* Notifications */

  



static BOOL
TREEVIEW_SendSimpleNotify (HWND hwnd, UINT code)
{
    NMHDR nmhdr;

    TRACE (treeview, "%x\n",code);
    nmhdr.hwndFrom = hwnd;
    nmhdr.idFrom   =  GetWindowLongA( hwnd, GWL_ID);
    nmhdr.code     = code;

    return (BOOL) SendMessageA (GetParent (hwnd), WM_NOTIFY,
                                   (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);
}



static BOOL
TREEVIEW_SendTreeviewNotify (HWND hwnd, UINT code, UINT action, 
			HTREEITEM oldItem, HTREEITEM newItem)

{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  NMTREEVIEWA nmhdr;
  TREEVIEW_ITEM  *wineItem;

  TRACE (treeview,"code:%x action:%x olditem:%x newitem:%x\n",
		  code,action,(INT)oldItem,(INT)newItem);
  nmhdr.hdr.hwndFrom = hwnd;
  nmhdr.hdr.idFrom =  GetWindowLongA( hwnd, GWL_ID);
  nmhdr.hdr.code = code;
  nmhdr.action = action;
  if (oldItem) {
  	wineItem=& infoPtr->items[(INT)oldItem];
  	nmhdr.itemOld.mask 		= wineItem->mask;
  	nmhdr.itemOld.hItem		= wineItem->hItem;
  	nmhdr.itemOld.state		= wineItem->state;
  	nmhdr.itemOld.stateMask	= wineItem->stateMask;
  	nmhdr.itemOld.iImage 	= wineItem->iImage;
  	nmhdr.itemOld.pszText 	= wineItem->pszText;
  	nmhdr.itemOld.cchTextMax= wineItem->cchTextMax;
  	nmhdr.itemOld.iImage 	= wineItem->iImage;
  	nmhdr.itemOld.iSelectedImage 	= wineItem->iSelectedImage;
  	nmhdr.itemOld.cChildren = wineItem->cChildren;
  	nmhdr.itemOld.lParam	= wineItem->lParam;
  }

  if (newItem) {
  	wineItem=& infoPtr->items[(INT)newItem];
  	nmhdr.itemNew.mask 		= wineItem->mask;
  	nmhdr.itemNew.hItem		= wineItem->hItem;
  	nmhdr.itemNew.state		= wineItem->state;
  	nmhdr.itemNew.stateMask	= wineItem->stateMask;
  	nmhdr.itemNew.iImage 	= wineItem->iImage;
  	nmhdr.itemNew.pszText 	= wineItem->pszText;
  	nmhdr.itemNew.cchTextMax= wineItem->cchTextMax;
  	nmhdr.itemNew.iImage 	= wineItem->iImage;
  	nmhdr.itemNew.iSelectedImage 	= wineItem->iSelectedImage;
  	nmhdr.itemNew.cChildren = wineItem->cChildren;
  	nmhdr.itemNew.lParam	= wineItem->lParam;
  }

  nmhdr.ptDrag.x = 0;
  nmhdr.ptDrag.y = 0;

  return (BOOL)SendMessageA (GetParent (hwnd), WM_NOTIFY,
                                   (WPARAM) GetWindowLongA( hwnd, GWL_ID), (LPARAM)&nmhdr);

}

static BOOL
TREEVIEW_SendTreeviewDnDNotify (HWND hwnd, UINT code, HTREEITEM dragItem, 
								POINT pt)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  NMTREEVIEWA nmhdr;
  TREEVIEW_ITEM  *wineItem;

  TRACE (treeview,"code:%x dragitem:%x\n", code,(INT)dragItem);

  nmhdr.hdr.hwndFrom = hwnd;
  nmhdr.hdr.idFrom =  GetWindowLongA( hwnd, GWL_ID);
  nmhdr.hdr.code = code;
  nmhdr.action = 0;
  wineItem=& infoPtr->items[(INT)dragItem];
  nmhdr.itemNew.mask 	= wineItem->mask;
  nmhdr.itemNew.hItem	= wineItem->hItem;
  nmhdr.itemNew.state	= wineItem->state;
  nmhdr.itemNew.lParam	= wineItem->lParam;

  nmhdr.ptDrag.x = pt.x;
  nmhdr.ptDrag.y = pt.y;

  return (BOOL)SendMessageA (GetParent (hwnd), WM_NOTIFY,
                                   (WPARAM) GetWindowLongA( hwnd, GWL_ID), (LPARAM)&nmhdr);

}



static BOOL
TREEVIEW_SendDispInfoNotify (HWND hwnd, TREEVIEW_ITEM *wineItem, 
								UINT code, UINT what)
{
  NMTVDISPINFOA tvdi;
  BOOL retval;
  char *buf;

  TRACE (treeview,"item %d, action %x\n",(INT)wineItem->hItem,what);

  tvdi.hdr.hwndFrom	= hwnd;
  tvdi.hdr.idFrom	=  GetWindowLongA( hwnd, GWL_ID);
  tvdi.hdr.code		= code;
  tvdi.item.mask	= what;
  tvdi.item.hItem	= wineItem->hItem;
  tvdi.item.state	= wineItem->state;
  tvdi.item.lParam	= wineItem->lParam;
  tvdi.item.pszText = COMCTL32_Alloc (128*sizeof(char));
  buf = tvdi.item.pszText;
  retval=(BOOL)SendMessageA (GetParent (hwnd), WM_NOTIFY,
                                   (WPARAM) GetWindowLongA( hwnd, GWL_ID), (LPARAM)&tvdi);
  if (what & TVIF_TEXT) {
		wineItem->pszText        = tvdi.item.pszText;
		if (buf==tvdi.item.pszText) {
			wineItem->cchTextMax = 128;
		} else { 
			TRACE (treeview,"user-supplied buffer\n");
			COMCTL32_Free (buf);
			wineItem->cchTextMax = 0;
		}
	}
  if (what & TVIF_SELECTEDIMAGE) 
		wineItem->iSelectedImage = tvdi.item.iSelectedImage;
  if (what & TVIF_IMAGE) 
		wineItem->iImage         = tvdi.item.iImage;
  if (what & TVIF_CHILDREN) 
		wineItem->cChildren      = tvdi.item.cChildren;

 return retval;
}



static BOOL
TREEVIEW_SendCustomDrawNotify (HWND hwnd, DWORD dwDrawStage, HDC hdc,
			RECT rc)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  NMTVCUSTOMDRAW nmcdhdr;
  LPNMCUSTOMDRAW nmcd;

  TRACE (treeview,"drawstage:%lx hdc:%x\n", dwDrawStage, hdc);

  nmcd= & nmcdhdr.nmcd;
  nmcd->hdr.hwndFrom = hwnd;
  nmcd->hdr.idFrom =  GetWindowLongA( hwnd, GWL_ID);
  nmcd->hdr.code   = NM_CUSTOMDRAW;
  nmcd->dwDrawStage= dwDrawStage;
  nmcd->hdc		   = hdc;
  nmcd->rc.left    = rc.left;
  nmcd->rc.right   = rc.right;
  nmcd->rc.bottom  = rc.bottom;
  nmcd->rc.top     = rc.top;
  nmcd->dwItemSpec = 0;
  nmcd->uItemState = 0;
  nmcd->lItemlParam= 0;
  nmcdhdr.clrText  = infoPtr->clrText;
  nmcdhdr.clrTextBk= infoPtr->clrBk;
  nmcdhdr.iLevel   = 0;

  return (BOOL)SendMessageA (GetParent (hwnd), WM_NOTIFY,
                               (WPARAM) GetWindowLongA( hwnd, GWL_ID), (LPARAM)&nmcdhdr);

}



/* FIXME: need to find out when the flags in uItemState need to be set */

static BOOL
TREEVIEW_SendCustomDrawItemNotify (HWND hwnd, HDC hdc,
  			TREEVIEW_ITEM *wineItem, UINT uItemDrawState)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
 NMTVCUSTOMDRAW nmcdhdr;
 LPNMCUSTOMDRAW nmcd;
 DWORD dwDrawStage,dwItemSpec;
 UINT uItemState;
 
 dwDrawStage=CDDS_ITEM | uItemDrawState;
 dwItemSpec=(DWORD)wineItem->hItem;
 uItemState=0;
 if (wineItem->hItem==infoPtr->selectedItem) uItemState|=CDIS_SELECTED;
 if (wineItem->hItem==infoPtr->focusItem)	 uItemState|=CDIS_FOCUS;
 if (wineItem->hItem==infoPtr->hotItem)      uItemState|=CDIS_HOT;

 nmcd= & nmcdhdr.nmcd;
 nmcd->hdr.hwndFrom = hwnd;
 nmcd->hdr.idFrom =  GetWindowLongA( hwnd, GWL_ID);
 nmcd->hdr.code   = NM_CUSTOMDRAW;
 nmcd->dwDrawStage= dwDrawStage;
 nmcd->hdc		  = hdc;
 nmcd->rc.left    = wineItem->rect.left;
 nmcd->rc.right   = wineItem->rect.right;
 nmcd->rc.bottom  = wineItem->rect.bottom;
 nmcd->rc.top     = wineItem->rect.top;
 nmcd->dwItemSpec = dwItemSpec;
 nmcd->uItemState = uItemState;
 nmcd->lItemlParam= wineItem->lParam;

 nmcdhdr.clrText  = infoPtr->clrText;
 nmcdhdr.clrTextBk= infoPtr->clrBk;
 nmcdhdr.iLevel   = wineItem->iLevel;

 TRACE (treeview,"drawstage:%lx hdc:%x item:%lx, itemstate:%x\n",
		  dwDrawStage, hdc, dwItemSpec, uItemState);

 return (BOOL)SendMessageA (GetParent (hwnd), WM_NOTIFY,
                               (WPARAM) GetWindowLongA( hwnd, GWL_ID), (LPARAM)&nmcdhdr);
}



/* Note:If the specified item is the child of a collapsed parent item,
   the parent's list of child items is (recursively) expanded to reveal the 
   specified item. This is mentioned for TREEVIEW_SelectItem; don't 
   know if it also applies here.
*/

static LRESULT
TREEVIEW_Expand (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  TREEVIEW_ITEM *wineItem;
  UINT flag;
  INT expand;
  
  flag = (UINT) wParam;
  expand = (INT) lParam;
  TRACE (treeview,"flags:%x item:%x\n", expand, wParam);
  wineItem = TREEVIEW_ValidItem (infoPtr, (HTREEITEM)expand);

  if (!wineItem) 
    return 0;
  if (!wineItem->cChildren) 
    return 0;

  if (wineItem->cChildren==I_CHILDRENCALLBACK) {
    FIXME (treeview,"we don't handle I_CHILDRENCALLBACK yet\n");
    return 0;
  }

  if (flag == TVE_TOGGLE) {    /* FIXME: check exact behaviour here */
   flag &= ~TVE_TOGGLE;    /* ie: bitwise ops or 'case' ops */
   if (wineItem->state & TVIS_EXPANDED) 
     flag |= TVE_COLLAPSE;
   else
     flag |= TVE_EXPAND;
  }

  switch (flag) 
  {
    case TVE_COLLAPSERESET: 
      if (!wineItem->state & TVIS_EXPANDED) 
        return 0;

       wineItem->state &= ~(TVIS_EXPANDEDONCE | TVIS_EXPANDED);
       TREEVIEW_RemoveAllChildren (hwnd, wineItem);
       break;

    case TVE_COLLAPSE: 
      if (!wineItem->state & TVIS_EXPANDED) 
        return 0;

      wineItem->state &= ~TVIS_EXPANDED;
      break;

    case TVE_EXPAND: 
      if (wineItem->state & TVIS_EXPANDED) 
        return 0;

     
      if (!(wineItem->state & TVIS_EXPANDEDONCE))  
      { 
        /* this item has never been expanded */
        if (TREEVIEW_SendTreeviewNotify (
              hwnd, 
              TVN_ITEMEXPANDING, 
              TVE_EXPAND, 
              0, 
              (HTREEITEM)expand))
            return FALSE;   /* FIXME: OK? */

        wineItem->state |= TVIS_EXPANDED | TVIS_EXPANDEDONCE;
        TREEVIEW_SendTreeviewNotify (
          hwnd, 
          TVN_ITEMEXPANDED, 
          TVE_EXPAND, 
          0, 
          (HTREEITEM)expand);
      }
      else
      {
        /* this item has already been expanded */
        wineItem->state |= TVIS_EXPANDED;
      }
      break;

    case TVE_EXPANDPARTIAL:
      FIXME (treeview, "TVE_EXPANDPARTIAL not implemented\n");
      wineItem->state ^=TVIS_EXPANDED;
      wineItem->state |=TVIS_EXPANDEDONCE;
      break;
  }
  
  TREEVIEW_QueueRefresh (hwnd);
  return TRUE;
}



static TREEVIEW_ITEM *
TREEVIEW_HitTestPoint (HWND hwnd, POINT pt)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
 TREEVIEW_ITEM *wineItem;
 RECT rect;

 GetClientRect (hwnd, &rect);

 if (!infoPtr->firstVisible) return NULL;

 wineItem=&infoPtr->items [(INT)infoPtr->firstVisible];

 while ((wineItem!=NULL) && (pt.y > wineItem->rect.bottom))
       wineItem=TREEVIEW_GetNextListItem (infoPtr,wineItem);
	
 if (!wineItem) 
	return NULL;

 return wineItem;
}




static LRESULT
TREEVIEW_HitTest (HWND hwnd, LPARAM lParam)
{
  LPTVHITTESTINFO lpht=(LPTVHITTESTINFO) lParam;
  TREEVIEW_ITEM *wineItem;
  RECT rect;
  UINT status,x,y;

  GetClientRect (hwnd, &rect);
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

  wineItem=TREEVIEW_HitTestPoint (hwnd, lpht->pt);
  if (!wineItem) {  
    lpht->flags=TVHT_NOWHERE;
    return 0;
  }

  /* FIXME: implement other flags
   * Assign the appropriate flags depending on the click location 
   * Intitialize flags before to "|=" it... 
   */
  lpht->flags=0;

  if (x < wineItem->expandBox.left) 
  {
    lpht->flags |= TVHT_ONITEMINDENT;
  } 
  else if ( ( x >= wineItem->expandBox.left) && 
            ( x <= wineItem->expandBox.right))
  {
    lpht->flags |= TVHT_ONITEMBUTTON;
  }
  else if (x < wineItem->rect.right) 
  {
    lpht->flags |= TVHT_ONITEMLABEL;    
  } 
  else
  {
    lpht->flags|=TVHT_ONITEMRIGHT;
  }
 
  lpht->hItem=wineItem->hItem;

  return (LRESULT) wineItem->hItem;
}




LRESULT
TREEVIEW_LButtonDoubleClick (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TREEVIEW_ITEM *wineItem;
  POINT pt;

  TRACE (treeview,"\n");
  pt.x = (INT)LOWORD(lParam);
  pt.y = (INT)HIWORD(lParam);
  SetFocus (hwnd);

  wineItem=TREEVIEW_HitTestPoint (hwnd, pt);
  if (!wineItem) return 0;
  TRACE (treeview,"item %d \n",(INT)wineItem->hItem);
 
  if (TREEVIEW_SendSimpleNotify (hwnd, NM_DBLCLK)!=TRUE) {     /* FIXME!*/
  	TREEVIEW_Expand (hwnd, (WPARAM) TVE_TOGGLE, (LPARAM) wineItem->hItem);
 }
 return TRUE;
}



static LRESULT
TREEVIEW_LButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  INT iItem;
  TVHITTESTINFO ht;

  ht.pt.x = (INT)LOWORD(lParam);
  ht.pt.y = (INT)HIWORD(lParam);

  SetFocus (hwnd);
  iItem=TREEVIEW_HitTest (hwnd, (LPARAM) &ht);
  TRACE (treeview,"item %d \n",iItem);
  if (ht.flags & TVHT_ONITEMBUTTON) {
	TREEVIEW_Expand (hwnd, (WPARAM) TVE_TOGGLE, (LPARAM) iItem);
  }

  infoPtr->uInternalStatus|=TV_LDRAG;
	
  if (TREEVIEW_DoSelectItem (hwnd, TVGN_CARET, (HTREEITEM)iItem, TVC_BYMOUSE))
	 return 0;

  
 return 0;
}

static LRESULT
TREEVIEW_LButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
 TREEVIEW_ITEM *editItem;
 INT ret;
 POINT pt;

 pt.x = (INT)LOWORD(lParam);
 pt.y = (INT)HIWORD(lParam);

 TRACE (treeview,"\n");
 if (TREEVIEW_SendSimpleNotify (hwnd, NM_CLICK)) return 0;
 editItem=TREEVIEW_HitTestPoint (hwnd, pt);    
 if (!editItem) return 0;

 infoPtr->uInternalStatus &= ~(TV_LDRAG | TV_LDRAGGING);

 if ( GetWindowLongA( hwnd, GWL_STYLE) & TVS_EDITLABELS) {
		RECT *r;
		ret=TREEVIEW_SendDispInfoNotify (hwnd, editItem, 
											TVN_BEGINLABELEDIT, 0);
		if (ret) return 0;
		printf ("edit started..\n");
		r=& editItem->rect;
		infoPtr->editItem=editItem->hItem;
		SetWindowPos ( infoPtr->hwndEdit, HWND_TOP, r->left, r->top,
                           r->right - r->left + 5,
                           r->bottom - r->top + 2,
                           SWP_SHOWWINDOW );
		SetFocus (infoPtr->hwndEdit);
		SetWindowTextA ( infoPtr->hwndEdit, editItem->pszText );
        SendMessageA ( infoPtr->hwndEdit, EM_SETSEL, 0, -1 );
	}


 
 return 0;
}


static LRESULT
TREEVIEW_RButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);

 TRACE (treeview,"\n");
 infoPtr->uInternalStatus|=TV_RDRAG;
 return 0;
}

static LRESULT
TREEVIEW_RButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);

 TRACE (treeview,"\n");
 if (TREEVIEW_SendSimpleNotify (hwnd, NM_RCLICK)) return 0;
 infoPtr->uInternalStatus&= ~(TV_RDRAG | TV_RDRAGGING);
 return 0;
}


static LRESULT
TREEVIEW_MouseMove (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
 TREEVIEW_ITEM *hotItem;
 POINT pt;

 pt.x=(INT) LOWORD (lParam);
 pt.y=(INT) HIWORD (lParam);
 hotItem=TREEVIEW_HitTestPoint (hwnd, pt);
 if (!hotItem) return 0;
 infoPtr->focusItem=hotItem->hItem;

 if ( GetWindowLongA( hwnd, GWL_STYLE) & TVS_DISABLEDRAGDROP) return 0;

 if (infoPtr->uInternalStatus & TV_LDRAG) {
	TREEVIEW_SendTreeviewDnDNotify (hwnd, TVN_BEGINDRAG, hotItem->hItem, pt);
	infoPtr->uInternalStatus &= ~TV_LDRAG;
	infoPtr->uInternalStatus |= TV_LDRAGGING;
	infoPtr->dropItem=hotItem->hItem;
	return 0;
 }

 if (infoPtr->uInternalStatus & TV_RDRAG) {
	TREEVIEW_SendTreeviewDnDNotify (hwnd, TVN_BEGINRDRAG, hotItem->hItem, pt);
	infoPtr->uInternalStatus &= ~TV_RDRAG;
	infoPtr->uInternalStatus |= TV_RDRAGGING;
	infoPtr->dropItem=hotItem->hItem;
	return 0;
 }
 
 return 0;
}


static LRESULT
TREEVIEW_CreateDragImage (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
 TREEVIEW_ITEM *dragItem;
 INT cx,cy;
 HDC    hdc,htopdc;
 HWND hwtop;
 HBITMAP hbmp,hOldbmp;
 SIZE  size;
 RECT  rc;
 HFONT hOldFont;
 char    *itemtxt;
 
 TRACE (treeview,"\n");
 if (!(infoPtr->himlNormal))  return 0;
 dragItem=TREEVIEW_ValidItem (infoPtr, (HTREEITEM) lParam);
 
 if (!dragItem) return 0;
 itemtxt=dragItem->pszText;

 hwtop=GetDesktopWindow ();
 htopdc= GetDC (hwtop);
 hdc=CreateCompatibleDC (htopdc); 
 
 hOldFont=SelectObject (hdc, infoPtr->hFont);
 GetTextExtentPoint32A (hdc, itemtxt, lstrlenA (itemtxt), &size);
 TRACE (treeview,"%d %d %s %d\n",size.cx,size.cy,itemtxt,lstrlenA(itemtxt));
 hbmp=CreateCompatibleBitmap (htopdc, size.cx, size.cy);
 hOldbmp=SelectObject (hdc, hbmp);

 ImageList_GetIconSize (infoPtr->himlNormal, &cx, &cy);
 size.cx+=cx;
 if (cy>size.cy) size.cy=cy;

 infoPtr->dragList=ImageList_Create (size.cx, size.cy, ILC_COLOR, 10, 10);
 ImageList_Draw (infoPtr->himlNormal, dragItem->iImage, hdc, 0, 0, ILD_NORMAL);

/*
 ImageList_GetImageInfo (infoPtr->himlNormal, dragItem->hItem, &iminfo);
 ImageList_AddMasked (infoPtr->dragList, iminfo.hbmImage, CLR_DEFAULT);
*/

/* draw item text */

 SetRect (&rc, cx, 0, size.cx,size.cy);
 DrawTextA (hdc, itemtxt, lstrlenA (itemtxt), &rc, DT_LEFT);
 SelectObject (hdc, hOldFont);
 SelectObject (hdc, hOldbmp);

 ImageList_Add (infoPtr->dragList, hbmp, 0);

 DeleteDC (hdc);
 DeleteObject (hbmp);
 ReleaseDC (hwtop, htopdc);

 return (LRESULT)infoPtr->dragList;
}



/* FIXME: handle NM_KILLFocus enzo */

static LRESULT
TREEVIEW_DoSelectItem (HWND hwnd, INT action, HTREEITEM newSelect, INT cause)

{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
 TREEVIEW_ITEM *prevItem,*wineItem, *parentItem;
 INT prevSelect;

  TRACE (treeview,"item %x, flag %x, cause %x\n", (INT)newSelect, action, cause);
  wineItem = TREEVIEW_ValidItem (infoPtr, (HTREEITEM)newSelect);

  if (wineItem) {
	if (wineItem->parent) {
  	parentItem=TREEVIEW_ValidItem (infoPtr, wineItem->parent);
	if (!(parentItem->state & TVIS_EXPANDED)) 
		TREEVIEW_Expand (hwnd, TVE_EXPAND, (LPARAM) wineItem->parent);
  	}
  }

  switch (action) {
	case TVGN_CARET: 
  		prevSelect=(INT)infoPtr->selectedItem;
		if ((HTREEITEM)prevSelect==newSelect) return FALSE;
  		prevItem= TREEVIEW_ValidItem (infoPtr, (HTREEITEM)prevSelect);
		if (newSelect) 
	    	if (TREEVIEW_SendTreeviewNotify (hwnd, TVN_SELCHANGING, 
							cause, (HTREEITEM)prevSelect, (HTREEITEM)newSelect)) 
			return FALSE;       /* FIXME: OK? */
		
	    if (prevItem) prevItem->state &= ~TVIS_SELECTED;
  		infoPtr->selectedItem=(HTREEITEM)newSelect;
		if (wineItem) wineItem->state |=TVIS_SELECTED;
		if (newSelect)
			TREEVIEW_SendTreeviewNotify (hwnd, TVN_SELCHANGED, 
				cause, (HTREEITEM)prevSelect, (HTREEITEM)newSelect);
		break;
	case TVGN_DROPHILITE: 
  		prevItem= TREEVIEW_ValidItem (infoPtr, infoPtr->dropItem);
		if (prevItem) prevItem->state &= ~TVIS_DROPHILITED;
		infoPtr->dropItem=(HTREEITEM)newSelect;
		if (wineItem) wineItem->state |=TVIS_DROPHILITED;
		break;
	case TVGN_FIRSTVISIBLE:
		FIXME (treeview, "FIRSTVISIBLE not implemented\n");
		break;
 }
 
 TREEVIEW_QueueRefresh (hwnd);
  
 return TRUE;
}


static LRESULT
TREEVIEW_SelectItem (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
 return TREEVIEW_DoSelectItem (hwnd, wParam, (HTREEITEM) lParam, TVC_UNKNOWN);
}



   
static LRESULT
TREEVIEW_GetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);

 TRACE (treeview,"%x\n",infoPtr->hFont);
 return infoPtr->hFont;
}

static LRESULT
TREEVIEW_SetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
 TEXTMETRICA tm;
 LOGFONTA logFont;
 HFONT hFont, hOldFont;
 INT height;
 HDC hdc;

 TRACE (treeview,"%x %lx\n",wParam, lParam);
 
 infoPtr->hFont = (HFONT)wParam;

 hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);

 GetObjectA (infoPtr->hFont, sizeof (LOGFONTA), &logFont);
 logFont.lfWeight=FW_BOLD;
 infoPtr->hBoldFont = CreateFontIndirectA (&logFont);

 hdc = GetDC (0);
 hOldFont = SelectObject (hdc, hFont);
 GetTextMetricsA (hdc, &tm);
 height= tm.tmHeight + tm.tmExternalLeading;
 if (height>infoPtr->uRealItemHeight) 
 	infoPtr->uRealItemHeight=height;
 SelectObject (hdc, hOldFont);
 ReleaseDC (0, hdc);

 if (lParam) 	
 	TREEVIEW_QueueRefresh (hwnd);
 
 return 0;
}



static LRESULT
TREEVIEW_VScroll (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
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
  
  TREEVIEW_QueueRefresh (hwnd);
  return TRUE;
}

static LRESULT
TREEVIEW_HScroll (HWND hwnd, WPARAM wParam, LPARAM lParam) 
{
  TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
  int maxWidth;

  TRACE (treeview,"wp %lx, lp %x\n", lParam, wParam);
	
  if (!infoPtr->uInternalStatus & TV_HSCROLL) return FALSE;

  switch (LOWORD (wParam)) {
	case SB_LINEUP: 
			if (!infoPtr->cx) return FALSE;
			infoPtr->cx -= infoPtr->uRealItemHeight;
			if (infoPtr->cx < 0) infoPtr->cx=0;
			break;
	case SB_LINEDOWN: 
			maxWidth=infoPtr->uTotalWidth-infoPtr->uVisibleWidth;
			if (infoPtr->cx == maxWidth) return FALSE;
			infoPtr->cx += infoPtr->uRealItemHeight; /*FIXME */
			if (infoPtr->cx > maxWidth) 
				infoPtr->cx = maxWidth;
			break;
	case SB_PAGEUP:	
			if (!infoPtr->cx) return FALSE;
			infoPtr->cx -= infoPtr->uVisibleWidth;
			if (infoPtr->cx < 0) infoPtr->cx=0;
			break;
	case SB_PAGEDOWN:
			maxWidth=infoPtr->uTotalWidth-infoPtr->uVisibleWidth;
			if (infoPtr->cx == maxWidth) return FALSE;
			infoPtr->cx += infoPtr->uVisibleWidth;
            if (infoPtr->cx > maxWidth)
                infoPtr->cx = maxWidth;
			break;
	case SB_THUMBTRACK: 
			infoPtr->cx = HIWORD (wParam);
			break;
			
  }
  
  TREEVIEW_QueueRefresh (hwnd);
  return TRUE;
}


/* FIXME: does KEYDOWN also send notifications?? If so, use 
   TREEVIEW_DoSelectItem.
*/
static LRESULT
TREEVIEW_KeyDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
 TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);
 TREEVIEW_ITEM *prevItem,*newItem;
 int prevSelect;


 TRACE (treeview,"%x %lx\n",wParam, lParam);
 prevSelect=(INT)infoPtr->selectedItem;
 if (!prevSelect) return FALSE;

 prevItem= TREEVIEW_ValidItem (infoPtr, (HTREEITEM)prevSelect);
 
 newItem=NULL;
 switch (wParam) {
	case VK_UP: 
		newItem=TREEVIEW_GetPrevListItem (infoPtr, prevItem);

		if (!newItem) 
			newItem=& infoPtr->items[(INT)infoPtr->TopRootItem];

    if (! newItem->visible)
      TREEVIEW_VScroll(hwnd, SB_LINEUP, 0);

		break;

	case VK_DOWN: 
		newItem=TREEVIEW_GetNextListItem (infoPtr, prevItem);

		if (!newItem) 
      newItem=prevItem;

    if (! newItem->visible)
      TREEVIEW_VScroll(hwnd, SB_LINEDOWN, 0);
		break;

	case VK_HOME:
		newItem=& infoPtr->items[(INT)infoPtr->TopRootItem];
    infoPtr->cy = 0;
		break;

	case VK_END:
		newItem=& infoPtr->items[(INT)infoPtr->TopRootItem];
		newItem=TREEVIEW_GetLastListItem (infoPtr, newItem);

    if (! newItem->visible)
      infoPtr->cy = infoPtr->uTotalHeight-infoPtr->uVisibleHeight; 

		break;

	case VK_LEFT:
    if ( (prevItem->cChildren > 0) && (prevItem->state & TVIS_EXPANDED) )
    {
      TREEVIEW_Expand(hwnd, TVE_COLLAPSE, prevSelect );
    }
    else if ((INT)prevItem->parent) 
    {
      newItem = (& infoPtr->items[(INT)prevItem->parent]);
      if (! newItem->visible) 
        /* FIXME find a way to make this item the first visible... */
        newItem = NULL; 
    }

    break;

	case VK_RIGHT:
    if ( ( prevItem->cChildren > 0)  || 
         ( prevItem->cChildren == I_CHILDRENCALLBACK))
    {
      if (! (prevItem->state & TVIS_EXPANDED))
        TREEVIEW_Expand(hwnd, TVE_EXPAND, prevSelect );
      else
        newItem = (& infoPtr->items[(INT)prevItem->firstChild]);
    }
    break;

  case VK_ADD:
    if (! (prevItem->state & TVIS_EXPANDED))
      TREEVIEW_Expand(hwnd, TVE_EXPAND, prevSelect );
    break;

  case VK_SUBTRACT:
    if (prevItem->state & TVIS_EXPANDED)
      TREEVIEW_Expand(hwnd, TVE_COLLAPSE, prevSelect );
    break;

  case VK_PRIOR:
    
		newItem=TREEVIEW_GetListItem(
              infoPtr, 
              prevItem,
              -1*(TREEVIEW_GetVisibleCount(hwnd,0,0)-3));
		if (!newItem) 
      newItem=prevItem;
  
    if (! newItem->visible)
      TREEVIEW_VScroll(hwnd, SB_PAGEUP, 0);

		break;

  case VK_NEXT:
		newItem=TREEVIEW_GetListItem(
              infoPtr, 
              prevItem,
              TREEVIEW_GetVisibleCount(hwnd,0,0)-3);

		if (!newItem) 
      newItem=prevItem;

    if (! newItem->visible)
      TREEVIEW_VScroll(hwnd, SB_PAGEDOWN, 0);

		break;

	case VK_BACK:

	case VK_RETURN:

  default:
		FIXME (treeview, "%x not implemented\n", wParam);
		break;
 }

 if (!newItem) return FALSE;

 if (prevItem!=newItem) {
 	prevItem->state &= ~TVIS_SELECTED;
 	newItem->state |= TVIS_SELECTED;
 	infoPtr->selectedItem=newItem->hItem;
 	TREEVIEW_QueueRefresh (hwnd);
 	return TRUE;
 }

 return FALSE;
}





LRESULT WINAPI
TREEVIEW_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    	case TVM_INSERTITEMA:
          return TREEVIEW_InsertItemA (hwnd, wParam, lParam);

    	case TVM_INSERTITEMW:
      		FIXME (treeview, "Unimplemented msg TVM_INSERTITEM32W\n");
      		return 0;

    	case TVM_DELETEITEM:
      		return TREEVIEW_DeleteItem (hwnd, wParam, lParam);

    	case TVM_EXPAND:
      		return TREEVIEW_Expand (hwnd, wParam, lParam);

    	case TVM_GETITEMRECT:
      		return TREEVIEW_GetItemRect (hwnd, wParam, lParam);

    	case TVM_GETCOUNT:
      		return TREEVIEW_GetCount (hwnd, wParam, lParam);

    	case TVM_GETINDENT:
      		return TREEVIEW_GetIndent (hwnd);

    	case TVM_SETINDENT:
      		return TREEVIEW_SetIndent (hwnd, wParam);

    	case TVM_GETIMAGELIST:
      		return TREEVIEW_GetImageList (hwnd, wParam, lParam);

		case TVM_SETIMAGELIST:
	    	return TREEVIEW_SetImageList (hwnd, wParam, lParam);

    	case TVM_GETNEXTITEM:
      		return TREEVIEW_GetNextItem (hwnd, wParam, lParam);

    	case TVM_SELECTITEM:
      		return TREEVIEW_SelectItem (hwnd, wParam, lParam);

    	case TVM_GETITEMA:
      		return TREEVIEW_GetItemA (hwnd, wParam, lParam);

    	case TVM_GETITEMW:
      		FIXME (treeview, "Unimplemented msg TVM_GETITEM32W\n");
      		return 0;

    	case TVM_SETITEMA:
      		return TREEVIEW_SetItemA (hwnd, wParam, lParam);

    	case TVM_SETITEMW:
      		FIXME (treeview, "Unimplemented msg TVM_SETITEMW\n");
      		return 0;

    	case TVM_EDITLABELA:
      		FIXME (treeview, "Unimplemented msg TVM_EDITLABEL32A \n");
      		return 0;

    	case TVM_EDITLABELW:
      		FIXME (treeview, "Unimplemented msg TVM_EDITLABEL32W \n");
      		return 0;

    	case TVM_GETEDITCONTROL:
      		return TREEVIEW_GetEditControl (hwnd);

    	case TVM_GETVISIBLECOUNT:
      		return TREEVIEW_GetVisibleCount (hwnd, wParam, lParam);

    	case TVM_HITTEST:
      		return TREEVIEW_HitTest (hwnd, lParam);

    	case TVM_CREATEDRAGIMAGE:
      		return TREEVIEW_CreateDragImage (hwnd, wParam, lParam);
  
    	case TVM_SORTCHILDREN:
      		FIXME (treeview, "Unimplemented msg TVM_SORTCHILDREN\n");
      		return 0;
  
    	case TVM_ENSUREVISIBLE:
      		FIXME (treeview, "Unimplemented msg TVM_ENSUREVISIBLE\n");
      		return 0;
  
    	case TVM_SORTCHILDRENCB:
      		return TREEVIEW_SortChildrenCB(hwnd, wParam, lParam);
  
    	case TVM_ENDEDITLABELNOW:
      		FIXME (treeview, "Unimplemented msg TVM_ENDEDITLABELNOW\n");
      		return 0;
  
    	case TVM_GETISEARCHSTRINGA:
      		FIXME (treeview, "Unimplemented msg TVM_GETISEARCHSTRING32A\n");
      		return 0;
  
    	case TVM_GETISEARCHSTRINGW:
      		FIXME (treeview, "Unimplemented msg TVM_GETISEARCHSTRING32W\n");
      		return 0;
  
    	case TVM_GETTOOLTIPS:
      		return TREEVIEW_GetToolTips (hwnd);

    	case TVM_SETTOOLTIPS:
      		return TREEVIEW_SetToolTips (hwnd, wParam);
  
    	case TVM_SETINSERTMARK:
      		FIXME (treeview, "Unimplemented msg TVM_SETINSERTMARK\n");
      		return 0;
  
    	case TVM_SETITEMHEIGHT:
      		return TREEVIEW_SetItemHeight (hwnd, wParam);
  
    	case TVM_GETITEMHEIGHT:
      		return TREEVIEW_GetItemHeight (hwnd);
  
    	case TVM_SETBKCOLOR:
      		return TREEVIEW_SetBkColor (hwnd, wParam, lParam);
	
    	case TVM_SETTEXTCOLOR:
      		return TREEVIEW_SetTextColor (hwnd, wParam, lParam);
  
    	case TVM_GETBKCOLOR:
      		return TREEVIEW_GetBkColor (hwnd);
  
    	case TVM_GETTEXTCOLOR:
      		return TREEVIEW_GetTextColor (hwnd);
  
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
  
		case WM_COMMAND: 
			 return TREEVIEW_Command (hwnd, wParam, lParam);
  
		case WM_CREATE:
			return TREEVIEW_Create (hwnd, wParam, lParam);
  
		case WM_DESTROY:
			return TREEVIEW_Destroy (hwnd);
  
/*		case WM_ENABLE: */
  
		case WM_ERASEBKGND:
			return TREEVIEW_EraseBackground (hwnd, wParam, lParam);
  
		case WM_GETDLGCODE:
	    	return DLGC_WANTARROWS | DLGC_WANTCHARS;
  
		case WM_PAINT:
	    	return TREEVIEW_Paint (hwnd, wParam, lParam);
  
		case WM_GETFONT:
	    	return TREEVIEW_GetFont (hwnd, wParam, lParam);

		case WM_SETFONT:
	    	return TREEVIEW_SetFont (hwnd, wParam, lParam);
  
		case WM_KEYDOWN:
			return TREEVIEW_KeyDown (hwnd, wParam, lParam);
  
  
		case WM_SETFOCUS: 
			return TREEVIEW_SetFocus (hwnd, wParam, lParam);

		case WM_KILLFOCUS: 
			return TREEVIEW_KillFocus (hwnd, wParam, lParam);
  
  
		case WM_LBUTTONDOWN:
			return TREEVIEW_LButtonDown (hwnd, wParam, lParam);

		case WM_LBUTTONUP:
			return TREEVIEW_LButtonUp (hwnd, wParam, lParam);
  
		case WM_LBUTTONDBLCLK:
			return TREEVIEW_LButtonDoubleClick (hwnd, wParam, lParam);
  
		case WM_RBUTTONDOWN:
			return TREEVIEW_RButtonDown (hwnd, wParam, lParam);

		case WM_RBUTTONUP:
			return TREEVIEW_RButtonUp (hwnd, wParam, lParam);

		case WM_MOUSEMOVE:
			return TREEVIEW_MouseMove (hwnd, wParam, lParam);
  
  
/*		case WM_SYSCOLORCHANGE: */
		case WM_STYLECHANGED: 
			return TREEVIEW_StyleChanged (hwnd, wParam, lParam);

/*		case WM_SETREDRAW: */
  
		case WM_TIMER:
			return TREEVIEW_HandleTimer (hwnd, wParam, lParam);
 
		case WM_SIZE: 
			return TREEVIEW_Size (hwnd, wParam,lParam);

		case WM_HSCROLL: 
			return TREEVIEW_HScroll (hwnd, wParam, lParam);
		case WM_VSCROLL: 
			return TREEVIEW_VScroll (hwnd, wParam, lParam);
  
		case WM_DRAWITEM:
			printf ("drawItem\n");
			return DefWindowProcA (hwnd, uMsg, wParam, lParam);
  
		default:
	    	if (uMsg >= WM_USER)
		FIXME (treeview, "Unknown msg %04x wp=%08x lp=%08lx\n",
  		     uMsg, wParam, lParam);
  	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
      }
    return 0;
}


VOID
TREEVIEW_Register (VOID)
{
    WNDCLASSA wndClass;

    TRACE (treeview,"\n");

    if (GlobalFindAtomA (WC_TREEVIEWA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC)TREEVIEW_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TREEVIEW_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = WC_TREEVIEWA;
 
    RegisterClassA (&wndClass);
}


VOID
TREEVIEW_Unregister (VOID)
{
    if (GlobalFindAtomA (WC_TREEVIEWA))
	UnregisterClassA (WC_TREEVIEWA, (HINSTANCE)NULL);
}

