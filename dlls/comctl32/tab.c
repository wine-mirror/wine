/*
 * Tab control
 *
 * Copyright 1998 Anders Carlsson
 * Copyright 1999 Alex Priem <alexp@sci.kun.nl>
 *
 * TODO:
 *  Image list support
 *  Multiline support
 *  Unicode support
 *  Updown control support
 *  Look and feel
 *
 */

#include <string.h>

#include "commctrl.h"
#include "tab.h"
#include "win.h"
#include "debug.h"


#define TAB_GetInfoPtr(hwnd) ((TAB_INFO *)GetWindowLongA(hwnd,0))

static void TAB_Refresh (HWND hwnd, HDC hdc);

static BOOL
TAB_SendSimpleNotify (HWND hwnd, UINT code)
{
    NMHDR nmhdr;

    nmhdr.hwndFrom = hwnd;
    nmhdr.idFrom = GetWindowLongA(hwnd, GWL_ID);
    nmhdr.code = code;

    return (BOOL) SendMessageA (GetParent (hwnd), WM_NOTIFY,
				    (WPARAM) nmhdr.idFrom, (LPARAM) &nmhdr);
}


static VOID
TAB_RelayEvent (HWND hwndTip, HWND hwndMsg, UINT uMsg,
            WPARAM wParam, LPARAM lParam)
{
    MSG msg;

    msg.hwnd = hwndMsg;
    msg.message = uMsg;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.time = GetMessageTime ();
    msg.pt.x = LOWORD(GetMessagePos ());
    msg.pt.y = HIWORD(GetMessagePos ());

    SendMessageA (hwndTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
}



static LRESULT
TAB_GetCurSel (HWND hwnd)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
 
    return infoPtr->iSelected;
}

static LRESULT
TAB_GetCurFocus (HWND hwnd)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
 
    return infoPtr->uFocus;
}

static LRESULT
TAB_GetToolTips (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

    if (infoPtr == NULL) return 0;
    return infoPtr->hwndToolTip;
}


static LRESULT
TAB_SetCurSel (HWND hwnd,WPARAM wParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
	INT iItem=(INT) wParam;
	INT prevItem;
 
	prevItem=-1;
	if ((iItem >= 0) && (iItem < infoPtr->uNumItem)) {
		prevItem=infoPtr->iSelected;
    	infoPtr->iSelected=iItem;
	}
	return prevItem;
}

static LRESULT
TAB_SetCurFocus (HWND hwnd,WPARAM wParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
	INT iItem=(INT) wParam;
	HDC hdc;
 
	if ((iItem < 0) || (iItem > infoPtr->uNumItem)) return 0;

   	infoPtr->uFocus=iItem;
	if (GetWindowLongA(hwnd, GWL_STYLE) & TCS_BUTTONS) {
		FIXME (tab,"Should set input focus\n");
	} else { 
		if (infoPtr->iSelected != iItem) {
	    	if (TAB_SendSimpleNotify(hwnd, TCN_SELCHANGING)!=TRUE)  {
				infoPtr->iSelected = iItem;
				TAB_SendSimpleNotify(hwnd, TCN_SELCHANGE);
    			hdc = GetDC (hwnd);
    			TAB_Refresh (hwnd, hdc);
    			ReleaseDC (hwnd, hdc);
			}
		}
	}
  return 0;
}

static LRESULT
TAB_SetToolTips (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

    if (infoPtr == NULL) return 0;
    infoPtr->hwndToolTip = (HWND)wParam;
    return 0;
}


static HWND TAB_InternalHitTest (TAB_INFO *infoPtr, POINT pt, 
									UINT *flags)

{
  RECT rect;
  int iCount; 
  
  for (iCount = 0; iCount < infoPtr->uNumItem; iCount++) {
	    rect = infoPtr->items[iCount].rect;
	    if (PtInRect (&rect, pt)) return iCount;
	}
  *flags=TCHT_NOWHERE;
  return -1;
}

static LRESULT
TAB_HitTest (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
	LPTCHITTESTINFO lptest=(LPTCHITTESTINFO) lParam;
 
    return TAB_InternalHitTest (infoPtr,lptest->pt,&lptest->flags);
}


static LRESULT
TAB_LButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

	if (infoPtr->hwndToolTip)
    TAB_RelayEvent (infoPtr->hwndToolTip, hwnd,
                WM_LBUTTONDOWN, wParam, lParam);

	if (GetWindowLongA(hwnd, GWL_STYLE) & TCS_FOCUSONBUTTONDOWN ) {
		SetFocus (hwnd);
	}
	return 0;
}

static LRESULT
TAB_LButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
    POINT pt;
    INT newItem,dummy;
	HDC hdc;

	if (infoPtr->hwndToolTip)
    TAB_RelayEvent (infoPtr->hwndToolTip, hwnd,
                WM_LBUTTONDOWN, wParam, lParam);

    pt.x = (INT)LOWORD(lParam);
    pt.y = (INT)HIWORD(lParam);

	newItem=TAB_InternalHitTest (infoPtr,pt,&dummy);

	TRACE(tab, "On Tab, item %d\n", newItem);
		
	if (infoPtr->iSelected != newItem) {
	    if (TAB_SendSimpleNotify(hwnd, TCN_SELCHANGING)!=TRUE)  {
			infoPtr->iSelected = newItem;
			TAB_SendSimpleNotify(hwnd, TCN_SELCHANGE);
    		hdc = GetDC (hwnd);
    		TAB_Refresh (hwnd, hdc);
    		ReleaseDC (hwnd, hdc);
		}
	}
	TAB_SendSimpleNotify(hwnd, NM_CLICK);

    return 0;
}

static LRESULT
TAB_RButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	TAB_SendSimpleNotify(hwnd, NM_RCLICK);
	return 0;
}

static LRESULT
TAB_MouseMove (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

    if (infoPtr->hwndToolTip)
    TAB_RelayEvent (infoPtr->hwndToolTip, hwnd,
                WM_LBUTTONDOWN, wParam, lParam);
	return 0;
}

static LRESULT
TAB_AdjustRect (HWND hwnd, WPARAM wParam, LPARAM lParam)
{

	if (wParam==TRUE) {
		FIXME (tab,"Should set display rectangle\n");
	} else {
		FIXME (tab,"Should set window rectangle\n");
	}
	
	return 0;
}

static void 
TAB_SetItemBounds (HWND hwnd)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
    RECT rect;
    HFONT hFont, hOldFont;
    INT i, left;
    SIZE size;
    HDC hdc;

    /* FIXME: Is this needed? */
    GetClientRect (hwnd, &rect);
	left += (size.cx + 11);
    
    hdc = GetDC(hwnd); 
    
    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);
    hOldFont = SelectObject (hdc, hFont);

    left = rect.left;
    
    for (i = 0; i < infoPtr->uNumItem; i++)
    {
	if (GetWindowLongA(hwnd, GWL_STYLE) & TCS_BOTTOM) {
    	infoPtr->items[i].rect.bottom = rect.bottom;
		infoPtr->items[i].rect.top    = rect.bottom-20;
	} else {
		infoPtr->items[i].rect.top    = rect.top;
    	infoPtr->items[i].rect.bottom = rect.top + 20;
	}
	infoPtr->items[i].rect.left = left;

	GetTextExtentPoint32A(hdc, 
			     infoPtr->items[i].pszText, 
			     lstrlenA(infoPtr->items[i].pszText), &size);
	infoPtr->items[i].rect.right = left + size.cx+2*5;
	TRACE(tab, "TextSize: %i\n ", size.cx);
	TRACE(tab, "Rect: T %i, L %i, B %i, R %i\n", 
	      infoPtr->items[i].rect.top,
	      infoPtr->items[i].rect.left,
	      infoPtr->items[i].rect.bottom,
	      infoPtr->items[i].rect.right);	
	left += (size.cx + 11);
    }

    SelectObject (hdc, hOldFont);
    ReleaseDC (hwnd, hdc);
}
				 
static void
TAB_DrawItem (HWND hwnd, HDC hdc, INT iItem)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
    TAB_ITEM *pti = &infoPtr->items[iItem];
    RECT r;
    INT oldBkMode,cx,cy;

    HBRUSH hbr = CreateSolidBrush(GetSysColor(COLOR_BACKGROUND));
   
    HPEN	hwPen  = GetSysColorPen (COLOR_3DHILIGHT);
    HPEN	hbPen  = GetSysColorPen (COLOR_BTNSHADOW);
    HPEN	hsdPen = GetSysColorPen (COLOR_BTNTEXT);
    HPEN	htmpPen = (HPEN)NULL;

    CopyRect(&r, &pti->rect);

/* demo */
    FillRect(hdc, &r, hbr);
	
	


    htmpPen = hwPen;
    htmpPen = SelectObject (hdc, htmpPen);
	if (GetWindowLongA(hwnd, GWL_STYLE) & TCS_BOTTOM) {
    	MoveToEx (hdc, r.left, r.top, NULL);
    	LineTo (hdc, r.left, r.bottom - 2);
    	LineTo (hdc, r.left +2, r.bottom);
    	LineTo (hdc, r.right -1, r.bottom);
    	htmpPen = SelectObject (hdc, hbPen); 
    	MoveToEx (hdc, r.right-1, r.top, NULL);
    	LineTo (hdc,r.right-1, r.bottom-1);
    	hbPen = SelectObject (hdc, hsdPen);
    	MoveToEx (hdc, r.right, r.top+1, NULL);
    	LineTo(hdc, r.right,r.bottom);
    } else {
    	MoveToEx (hdc, r.left, r.bottom, NULL);
    	LineTo (hdc, r.left, r.top + 2);
    	LineTo (hdc, r.left +2, r.top);
    	LineTo (hdc, r.right -1, r.top);
    	htmpPen = SelectObject (hdc, hbPen); 
    	MoveToEx (hdc, r.right-1, r.bottom, NULL);
    	LineTo (hdc,r.right-1, r.top+1);
    	hbPen = SelectObject (hdc, hsdPen);
    	MoveToEx (hdc, r.right, r.bottom-1, NULL);
    	LineTo(hdc, r.right,r.top);
	}

   	hsdPen = SelectObject(hdc,htmpPen); 

    oldBkMode = SetBkMode(hdc, TRANSPARENT); 
    r.left += 3;
    r.right -= 3;

	if (infoPtr->himl) {
		ImageList_Draw (infoPtr->himl, iItem, hdc, 
						r.left, r.top+1, ILD_NORMAL);
		ImageList_GetIconSize (infoPtr->himl, &cx, &cy);
		r.left+=cx+3;
		}
    SetTextColor (hdc, COLOR_BTNTEXT);
    DrawTextA(hdc, pti->pszText, lstrlenA(pti->pszText),
	&r, DT_LEFT|DT_SINGLELINE|DT_VCENTER);
    if (oldBkMode != TRANSPARENT)
	SetBkMode(hdc, oldBkMode);
}

static void
TAB_DrawBorder (HWND hwnd, HDC hdc)
{
    HPEN htmPen;
    HPEN hwPen  = GetSysColorPen (COLOR_3DHILIGHT);
    HPEN hbPen  = GetSysColorPen (COLOR_3DDKSHADOW);
    HPEN hShade = GetSysColorPen (COLOR_BTNSHADOW);

    RECT rect;

    htmPen = SelectObject (hdc, hwPen);
    GetClientRect (hwnd, &rect);

    MoveToEx (hdc, rect.left, rect.bottom, NULL);
    LineTo (hdc, rect.left, rect.top+20); 

    LineTo (hdc, rect.right, rect.top+20);

    hwPen = SelectObject (hdc, htmPen);
    LineTo (hdc, rect.right, rect.bottom );
    LineTo (hdc, rect.left, rect.bottom);
    hbPen = SelectObject (hdc, hShade );
    MoveToEx (hdc, rect.right-1, rect.top+20, NULL);
    LineTo (hdc, rect.right-1, rect.bottom-1);
    LineTo (hdc, rect.left, rect.bottom-1);
    hShade = SelectObject(hdc, hShade);
}

    
static void
TAB_Refresh (HWND hwnd, HDC hdc)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
    HFONT hOldFont;
    INT i;

	if (!infoPtr->DoRedraw) return;

    TAB_DrawBorder (hwnd, hdc);

	hOldFont = SelectObject (hdc, infoPtr->hFont);
    for (i = 0; i < infoPtr->uNumItem; i++) {
	TAB_DrawItem (hwnd, hdc, i);
    }
	SelectObject (hdc, hOldFont);
}

static LRESULT
TAB_SetRedraw (HWND hwnd, WPARAM wParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
	
	infoPtr->DoRedraw=(BOOL) wParam;
	return 0;
}

static LRESULT
TAB_Paint (HWND hwnd, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    
    hdc = wParam== 0 ? BeginPaint (hwnd, &ps) : (HDC)wParam;
    TAB_Refresh (hwnd, hdc);
    
    if(!wParam)
	EndPaint (hwnd, &ps);
    return 0;
}

static LRESULT
TAB_InsertItem (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
    TCITEMA *pti;
    HDC  hdc;
    INT iItem, len;
    RECT rect;

    GetClientRect (hwnd, &rect);
	TRACE(tab, "Rect: %x T %i, L %i, B %i, R %i\n", hwnd,
	      rect.top, rect.left, rect.bottom, rect.right);	

    pti = (TCITEMA *)lParam;
    iItem = (INT)wParam;

    if (iItem < 0) return -1;
    if (iItem > infoPtr->uNumItem)
	iItem = infoPtr->uNumItem;

    if (infoPtr->uNumItem == 0) {
	infoPtr->items = COMCTL32_Alloc (sizeof (TAB_ITEM));
	infoPtr->uNumItem++;
    }
    else {
	TAB_ITEM *oldItems = infoPtr->items;

	infoPtr->uNumItem++;
	infoPtr->items = COMCTL32_Alloc (sizeof (TAB_ITEM) * infoPtr->uNumItem);
	
	/* pre insert copy */
	if (iItem > 0) {
	    memcpy (&infoPtr->items[0], &oldItems[0],
		    iItem * sizeof(TAB_ITEM));
	}

	/* post insert copy */
	if (iItem < infoPtr->uNumItem - 1) {
	    memcpy (&infoPtr->items[iItem+1], &oldItems[iItem],
		    (infoPtr->uNumItem - iItem - 1) * sizeof(TAB_ITEM));

	}
	
	COMCTL32_Free (oldItems);
    }

    infoPtr->items[iItem].mask = pti->mask;
    if (pti->mask & TCIF_TEXT) {
	len = lstrlenA (pti->pszText);
	infoPtr->items[iItem].pszText = COMCTL32_Alloc (len+1);
	lstrcpyA (infoPtr->items[iItem].pszText, pti->pszText);
	infoPtr->items[iItem].cchTextMax = pti->cchTextMax;
    }

    if (pti->mask & TCIF_IMAGE)
	infoPtr->items[iItem].iImage = pti->iImage;

    if (pti->mask & TCIF_PARAM)
	infoPtr->items[iItem].lParam = pti->lParam;

    hdc = GetDC (hwnd);
    TAB_Refresh (hwnd, hdc);
    ReleaseDC (hwnd, hdc);

    TRACE(tab, "[%04x]: added item %d '%s'\n",
		hwnd, iItem, infoPtr->items[iItem].pszText);

    TAB_SetItemBounds(hwnd);
    return iItem;
}

static LRESULT 
TAB_SetItemA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  TCITEMA *tabItem; 
  TAB_ITEM *wineItem; 
  INT    iItem,len;

  iItem=(INT) wParam;
  tabItem=(LPTCITEMA ) lParam;
  TRACE (tab,"%d %p\n",iItem, tabItem);
  if ((iItem<0) || (iItem>infoPtr->uNumItem)) return FALSE;

  wineItem=& infoPtr->items[iItem];

  if (tabItem->mask & TCIF_IMAGE) 
		wineItem->iImage=tabItem->iImage;

  if (tabItem->mask & TCIF_PARAM) 
		wineItem->lParam=tabItem->lParam;

  if (tabItem->mask & TCIF_RTLREADING) 
		FIXME (tab,"TCIF_RTLREADING\n");

  if (tabItem->mask & TCIF_STATE) 
		wineItem->dwState=tabItem->dwState;

  if (tabItem->mask & TCIF_TEXT) {
	 len=lstrlenA (tabItem->pszText);
	 if (len>wineItem->cchTextMax) 
		 wineItem->pszText= COMCTL32_ReAlloc (wineItem->pszText, len+1);
	 lstrcpynA (wineItem->pszText, tabItem->pszText, len);
  }

	return TRUE;
}

static LRESULT 
TAB_GetItemCount (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
   TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

   return infoPtr->uNumItem;
}


static LRESULT 
TAB_GetItemA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
   TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
   TCITEMA *tabItem;
   TAB_ITEM *wineItem;
   INT    iItem;

  iItem=(INT) wParam;
  tabItem=(LPTCITEMA) lParam;
  TRACE (tab,"\n");
  if ((iItem<0) || (iItem>infoPtr->uNumItem)) return FALSE;

  wineItem=& infoPtr->items[iItem];

  if (tabItem->mask & TCIF_IMAGE) 
		tabItem->iImage=wineItem->iImage;

  if (tabItem->mask & TCIF_PARAM) 
		tabItem->lParam=wineItem->lParam;

  if (tabItem->mask & TCIF_RTLREADING) 
		FIXME (tab, "TCIF_RTLREADING\n");

  if (tabItem->mask & TCIF_STATE) 
		tabItem->dwState=wineItem->dwState;

  if (tabItem->mask & TCIF_TEXT) 
	 lstrcpynA (tabItem->pszText, wineItem->pszText, tabItem->cchTextMax);

	return TRUE;
}

static LRESULT 
TAB_DeleteItem (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	FIXME (tab,"stub \n");
	return TRUE;
}

static LRESULT 
TAB_DeleteAllItems (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
   TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

	COMCTL32_Free (infoPtr->items);
	infoPtr->uNumItem=0;
	
	return TRUE;
}


static LRESULT
TAB_GetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

  TRACE (tab,"\n");
  return (LRESULT)infoPtr->hFont;
}

static LRESULT
TAB_SetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
 TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
 TEXTMETRICA tm;
 HFONT hFont, hOldFont;
 HDC hdc;

 TRACE (tab,"%x %lx\n",wParam, lParam);

 infoPtr->hFont = (HFONT)wParam;

 hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);

 hdc = GetDC (0);
 hOldFont = SelectObject (hdc, hFont);
 GetTextMetricsA (hdc, &tm);
 infoPtr->nHeight= tm.tmHeight + tm.tmExternalLeading;
 SelectObject (hdc, hOldFont);

 if (lParam) TAB_Refresh (hwnd,hdc);
 ReleaseDC (0, hdc);

 return 0;
}


static LRESULT
TAB_GetImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

  TRACE (tab,"\n");
  return (LRESULT)infoPtr->himl;
}

static LRESULT
TAB_SetImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
    HIMAGELIST himlPrev;

    TRACE (tab,"\n");
    himlPrev = infoPtr->himl;
  	infoPtr->himl= (HIMAGELIST)lParam;
    return (LRESULT)himlPrev;
}


static LRESULT
TAB_Size (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
  HDC hdc;
/* I'm not really sure what the following code was meant to do.
   This is what it is doing:
   When WM_SIZE is sent with SIZE_RESTORED, the control
   gets positioned in the top left corner.

  RECT parent_rect;
  HWND parent;
  UINT uPosFlags,cx,cy;

  uPosFlags=0;
  if (!wParam) {
  	parent = GetParent (hwnd);
  	GetClientRect(parent, &parent_rect);
  	cx=LOWORD (lParam);
  	cy=HIWORD (lParam);
  	if (GetWindowLongA(hwnd, GWL_STYLE) & CCS_NORESIZE) 
        uPosFlags |= (SWP_NOSIZE | SWP_NOMOVE);

  	SetWindowPos (hwnd, 0, parent_rect.left, parent_rect.top,
            cx, cy, uPosFlags | SWP_NOZORDER);
	} else {*/
    FIXME (tab,"WM_SIZE flag %x %lx not handled\n", wParam, lParam);
/*  } */

  TAB_SetItemBounds (hwnd);
  hdc = GetDC (hwnd);
  TAB_Refresh (hwnd, hdc);
  ReleaseDC (hwnd, hdc);

  return 0;
}


static LRESULT 
TAB_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr;

    infoPtr = (TAB_INFO *)COMCTL32_Alloc (sizeof(TAB_INFO));
    SetWindowLongA(hwnd, 0, (DWORD)infoPtr);
   
    infoPtr->uNumItem = 0;
    infoPtr->hFont = 0;
    infoPtr->items = 0;
    infoPtr->hcurArrow = LoadCursorA (0, IDC_ARROWA);
    infoPtr->iSelected = -1;  
	infoPtr->hwndToolTip=0;
    infoPtr->DoRedraw = TRUE;
  
    TRACE(tab, "Created tab control, hwnd [%04x]\n", hwnd); 
    if (GetWindowLongA(hwnd, GWL_STYLE) & TCS_TOOLTIPS) {
    /* Create tooltip control */
    infoPtr->hwndToolTip =
        CreateWindowExA (0, TOOLTIPS_CLASSA, NULL, 0,
                   CW_USEDEFAULT, CW_USEDEFAULT,
                   CW_USEDEFAULT, CW_USEDEFAULT,
                   hwnd, 0, 0, 0);

    /* Send NM_TOOLTIPSCREATED notification */
    if (infoPtr->hwndToolTip) {
        NMTOOLTIPSCREATED nmttc;

        nmttc.hdr.hwndFrom = hwnd;
        nmttc.hdr.idFrom = GetWindowLongA(hwnd, GWL_ID);
        nmttc.hdr.code = NM_TOOLTIPSCREATED;
        nmttc.hwndToolTips = infoPtr->hwndToolTip;

        SendMessageA (GetParent (hwnd), WM_NOTIFY,
                (WPARAM)GetWindowLongA(hwnd, GWL_ID), (LPARAM)&nmttc);
    }
    }


    return 0;
}

static LRESULT
TAB_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
    INT iItem;

    if (infoPtr->items) {
	for (iItem = 0; iItem < infoPtr->uNumItem; iItem++) {
	    if (infoPtr->items[iItem].pszText)
		COMCTL32_Free (infoPtr->items[iItem].pszText);
	}
	COMCTL32_Free (infoPtr->items);
    }

	if (infoPtr->hwndToolTip) 
		  DestroyWindow (infoPtr->hwndToolTip);

    COMCTL32_Free (infoPtr);
    return 0;
}

LRESULT WINAPI
TAB_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {

	case TCM_GETIMAGELIST:
	   return TAB_GetImageList (hwnd, wParam, lParam);

	case TCM_SETIMAGELIST:
	   return TAB_SetImageList (hwnd, wParam, lParam);
	
	case TCM_GETITEMCOUNT:
	    return TAB_GetItemCount (hwnd, wParam, lParam);

	case TCM_GETITEMA:
	   return TAB_GetItemA (hwnd, wParam, lParam);

	case TCM_GETITEMW:
	   FIXME (tab, "Unimplemented msg TCM_GETITEM32W\n");
	   return 0;

	case TCM_SETITEMA:
	   return TAB_SetItemA (hwnd, wParam, lParam);

	case TCM_SETITEMW:
	   FIXME (tab, "Unimplemented msg TCM_GETITEM32W\n");
	   return 0;

	case TCM_DELETEITEM:
	   return TAB_DeleteItem (hwnd, wParam, lParam);

	case TCM_DELETEALLITEMS:
	   return TAB_DeleteAllItems (hwnd, wParam, lParam);

	case TCM_GETITEMRECT:
	   FIXME (tab, "Unimplemented msg TCM_GETITEMRECT\n");
	   return 0;

	case TCM_GETCURSEL:
	   return TAB_GetCurSel (hwnd);

	case TCM_HITTEST:
	   return TAB_HitTest (hwnd, wParam, lParam);

	case TCM_SETCURSEL:
	   return TAB_SetCurSel (hwnd, wParam);

	case TCM_INSERTITEMA:
	   return TAB_InsertItem (hwnd, wParam, lParam);

	case TCM_INSERTITEMW:
	   FIXME (tab, "Unimplemented msg TCM_INSERTITEM32W\n");
	   return 0;

	case TCM_SETITEMEXTRA:
	   FIXME (tab, "Unimplemented msg TCM_SETITEMEXTRA\n");
	   return 0;

	case TCM_ADJUSTRECT:
	   return TAB_AdjustRect (hwnd, wParam, lParam);

	case TCM_SETITEMSIZE:
	   FIXME (tab, "Unimplemented msg TCM_SETITEMSIZE\n");
	   return 0;

	case TCM_REMOVEIMAGE:
	   FIXME (tab, "Unimplemented msg TCM_REMOVEIMAGE\n");
	   return 0;

	case TCM_SETPADDING:
	   FIXME (tab, "Unimplemented msg TCM_SETPADDING\n");
	   return 0;

	case TCM_GETROWCOUNT:
	   FIXME (tab, "Unimplemented msg TCM_GETROWCOUNT\n");
	   return 0;

	case TCM_GETTOOLTIPS:
	   return TAB_GetToolTips (hwnd, wParam, lParam);

	case TCM_SETTOOLTIPS:
	   return TAB_SetToolTips (hwnd, wParam, lParam);

	case TCM_GETCURFOCUS:
    	return TAB_GetCurFocus (hwnd);

	case TCM_SETCURFOCUS:
    	return TAB_SetCurFocus (hwnd, wParam);

	case TCM_SETMINTTABWIDTH:
	   FIXME (tab, "Unimplemented msg TCM_SETMINTTABWIDTH\n");
	   return 0;

	case TCM_DESELECTALL:
	   FIXME (tab, "Unimplemented msg TCM_DESELECTALL\n");
	   return 0;

	case WM_GETFONT:
	    return TAB_GetFont (hwnd, wParam, lParam);

	case WM_SETFONT:
	    return TAB_SetFont (hwnd, wParam, lParam);

	case WM_CREATE:
	    return TAB_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return TAB_Destroy (hwnd, wParam, lParam);

    case WM_GETDLGCODE:
            return DLGC_WANTARROWS | DLGC_WANTCHARS;

	case WM_LBUTTONDOWN:
	    return TAB_LButtonDown (hwnd, wParam, lParam);

	case WM_LBUTTONUP:
	    return TAB_LButtonUp (hwnd, wParam, lParam);

	case WM_RBUTTONDOWN:
	    return TAB_RButtonDown (hwnd, wParam, lParam);

	case WM_MOUSEMOVE:
	    return TAB_MouseMove (hwnd, wParam, lParam);

	case WM_PAINT:
	    return TAB_Paint (hwnd, wParam);
	case WM_SIZE:
		return TAB_Size (hwnd, wParam, lParam);
	
	case WM_SETREDRAW:
	    return TAB_SetRedraw (hwnd, wParam);
	
	
	default:
	    if (uMsg >= WM_USER)
		ERR (tab, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
TAB_Register (VOID)
{
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (WC_TABCONTROLA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS;
    wndClass.lpfnWndProc   = (WNDPROC)TAB_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TAB_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = WC_TABCONTROLA;
 
    RegisterClassA (&wndClass);
}


VOID
TAB_Unregister (VOID)
{
    if (GlobalFindAtomA (WC_TABCONTROLA))
	UnregisterClassA (WC_TABCONTROLA, (HINSTANCE)NULL);
}

