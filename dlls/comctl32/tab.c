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


#define TAB_GetInfoPtr(wndPtr) ((TAB_INFO *)wndPtr->wExtra[0])




static void TAB_Refresh (WND *wndPtr, HDC hdc);


static BOOL
TAB_SendSimpleNotify (WND *wndPtr, UINT code)
{
    NMHDR nmhdr;

    nmhdr.hwndFrom = wndPtr->hwndSelf;
    nmhdr.idFrom = wndPtr->wIDmenu;
    nmhdr.code = code;

    return (BOOL) SendMessageA (GetParent (wndPtr->hwndSelf), WM_NOTIFY,
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
TAB_GetCurSel (WND *wndPtr)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
 
    return infoPtr->iSelected;
}

static LRESULT
TAB_GetCurFocus (WND *wndPtr)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
 
    return infoPtr->uFocus;
}

static LRESULT
TAB_GetToolTips (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);

    if (infoPtr == NULL) return 0;
    return infoPtr->hwndToolTip;
}


static LRESULT
TAB_SetCurSel (WND *wndPtr,WPARAM wParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
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
TAB_SetCurFocus (WND *wndPtr,WPARAM wParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
	INT iItem=(INT) wParam;
	HDC hdc;
 
	if ((iItem < 0) || (iItem > infoPtr->uNumItem)) return 0;

   	infoPtr->uFocus=iItem;
	if (wndPtr->dwStyle & TCS_BUTTONS) {
		FIXME (tab,"Should set input focus\n");
	} else { 
		if (infoPtr->iSelected != iItem) {
	    	if (TAB_SendSimpleNotify(wndPtr, TCN_SELCHANGING)!=TRUE)  {
				infoPtr->iSelected = iItem;
				TAB_SendSimpleNotify(wndPtr, TCN_SELCHANGE);
    			hdc = GetDC (wndPtr->hwndSelf);
    			TAB_Refresh (wndPtr, hdc);
    			ReleaseDC (wndPtr->hwndSelf, hdc);
			}
		}
	}
  return 0;
}

static LRESULT
TAB_SetToolTips (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);

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
TAB_HitTest (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
	LPTCHITTESTINFO lptest=(LPTCHITTESTINFO) lParam;
 
    return TAB_InternalHitTest (infoPtr,lptest->pt,&lptest->flags);
}


static LRESULT
TAB_LButtonDown (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);

	if (infoPtr->hwndToolTip)
    TAB_RelayEvent (infoPtr->hwndToolTip, wndPtr->hwndSelf,
                WM_LBUTTONDOWN, wParam, lParam);

	if (wndPtr->dwStyle & TCS_FOCUSONBUTTONDOWN ) {
		SetFocus (wndPtr->hwndSelf);
	}
	return 0;
}

static LRESULT
TAB_LButtonUp (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
    POINT pt;
    INT newItem,dummy;
	HDC hdc;

	if (infoPtr->hwndToolTip)
    TAB_RelayEvent (infoPtr->hwndToolTip, wndPtr->hwndSelf,
                WM_LBUTTONDOWN, wParam, lParam);

    pt.x = (INT)LOWORD(lParam);
    pt.y = (INT)HIWORD(lParam);

	newItem=TAB_InternalHitTest (infoPtr,pt,&dummy);
	if (!newItem) return 0;
	TRACE(tab, "On Tab, item %d\n", newItem);
		
	if (infoPtr->iSelected != newItem) {
	    if (TAB_SendSimpleNotify(wndPtr, TCN_SELCHANGING)!=TRUE)  {
			infoPtr->iSelected = newItem;
			TAB_SendSimpleNotify(wndPtr, TCN_SELCHANGE);
    		hdc = GetDC (wndPtr->hwndSelf);
    		TAB_Refresh (wndPtr, hdc);
    		ReleaseDC (wndPtr->hwndSelf, hdc);
		}
	}
	TAB_SendSimpleNotify(wndPtr, NM_CLICK);

    return 0;
}

static LRESULT
TAB_RButtonDown (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	TAB_SendSimpleNotify(wndPtr, NM_RCLICK);
	return 0;
}

static LRESULT
TAB_MouseMove (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);

    if (infoPtr->hwndToolTip)
    TAB_RelayEvent (infoPtr->hwndToolTip, wndPtr->hwndSelf,
                WM_LBUTTONDOWN, wParam, lParam);
	return 0;
}

static LRESULT
TAB_AdjustRect (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{

	if (wParam==TRUE) {
		FIXME (tab,"Should set display rectangle\n");
	} else {
		FIXME (tab,"Should set window rectangle\n");
	}
	
	return 0;
}

static void 
TAB_SetItemBounds (WND *wndPtr)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
    RECT rect;
    HFONT hFont, hOldFont;
    INT i, left;
    SIZE size;
    HDC hdc;

    /* FIXME: Is this needed? */
    GetClientRect (wndPtr->hwndSelf, &rect);
	left += (size.cx + 11);
    
    hdc = GetDC(wndPtr->hwndSelf); 
    
    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);
    hOldFont = SelectObject (hdc, hFont);

    left = rect.left;
    
    for (i = 0; i < infoPtr->uNumItem; i++)
    {
	if (wndPtr->dwStyle & TCS_BOTTOM) {
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
    ReleaseDC (wndPtr->hwndSelf, hdc);
}
				 
static void
TAB_DrawItem (WND *wndPtr, HDC hdc, INT iItem)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
    TAB_ITEM *pti = &infoPtr->items[iItem];
    RECT r;
    INT oldBkMode,cx,cy;
  HBRUSH hbr = CreateSolidBrush (COLOR_BACKGROUND);
   
    HPEN	hwPen  = GetSysColorPen (COLOR_3DHILIGHT);
    HPEN	hbPen  = GetSysColorPen (COLOR_BTNSHADOW);
    HPEN	hsdPen = GetSysColorPen (COLOR_BTNTEXT);
    HPEN	htmpPen = (HPEN)NULL;

    CopyRect(&r, &pti->rect);

/* demo */
    FillRect(hdc, &r, hbr);
	
	


    htmpPen = hwPen;
    htmpPen = SelectObject (hdc, htmpPen);
	if (wndPtr->dwStyle & TCS_BOTTOM) {
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
TAB_DrawBorder (WND *wndPtr, HDC hdc)
{
    HPEN htmPen;
    HPEN hwPen  = GetSysColorPen (COLOR_3DHILIGHT);
    HPEN hbPen  = GetSysColorPen (COLOR_3DDKSHADOW);
    HPEN hShade = GetSysColorPen (COLOR_BTNSHADOW);

    RECT rect;

    htmPen = SelectObject (hdc, hwPen);
    GetClientRect (wndPtr->hwndSelf, &rect);

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
TAB_Refresh (WND *wndPtr, HDC hdc)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
    HFONT hOldFont;
    INT i;

	if (!infoPtr->DoRedraw) return;

    TAB_DrawBorder (wndPtr, hdc);

	hOldFont = SelectObject (hdc, infoPtr->hFont);
    for (i = 0; i < infoPtr->uNumItem; i++) {
	TAB_DrawItem (wndPtr, hdc, i);
    }
	SelectObject (hdc, hOldFont);
}

static LRESULT
TAB_SetRedraw (WND *wndPtr, WPARAM wParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
	
	infoPtr->DoRedraw=(BOOL) wParam;
	return 0;
}

static LRESULT
TAB_Paint (WND *wndPtr, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    
    hdc = wParam== 0 ? BeginPaint (wndPtr->hwndSelf, &ps) : (HDC)wParam;
    TAB_Refresh (wndPtr, hdc);
    
    if(!wParam)
	EndPaint (wndPtr->hwndSelf, &ps);
    return 0;
}

static LRESULT
TAB_InsertItem (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
    TCITEMA *pti;
    HDC  hdc;
    INT iItem, len;
    RECT rect;

    GetClientRect (wndPtr->hwndSelf, &rect);
	TRACE(tab, "Rect: %x T %i, L %i, B %i, R %i\n", wndPtr->hwndSelf,
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

    hdc = GetDC (wndPtr->hwndSelf);
    TAB_Refresh (wndPtr, hdc);
    ReleaseDC (wndPtr->hwndSelf, hdc);

    TRACE(tab, "[%04x]: added item %d '%s'\n",
		wndPtr->hwndSelf, iItem, infoPtr->items[iItem].pszText);

    TAB_SetItemBounds(wndPtr);
    return iItem;
}

static LRESULT 
TAB_SetItemA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
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
TAB_GetItemCount (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
   TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);

   return infoPtr->uNumItem;
}


static LRESULT 
TAB_GetItemA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
   TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
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
TAB_DeleteItem (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
	FIXME (tab,"stub \n");
	return TRUE;
}

static LRESULT 
TAB_DeleteAllItems (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
   TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);

	COMCTL32_Free (infoPtr->items);
	infoPtr->uNumItem=0;
	
	return TRUE;
}


static LRESULT
TAB_GetFont (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);

  TRACE (tab,"\n");
  return (LRESULT)infoPtr->hFont;
}

static LRESULT
TAB_SetFont (WND *wndPtr, WPARAM wParam, LPARAM lParam)

{
 TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
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

 if (lParam) TAB_Refresh (wndPtr,hdc);
 ReleaseDC (0, hdc);

 return 0;
}


static LRESULT
TAB_GetImageList (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);

  TRACE (tab,"\n");
  return (LRESULT)infoPtr->himl;
}

static LRESULT
TAB_SetImageList (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
    HIMAGELIST himlPrev;

    TRACE (tab,"\n");
    himlPrev = infoPtr->himl;
  	infoPtr->himl= (HIMAGELIST)lParam;
    return (LRESULT)himlPrev;
}


static LRESULT
TAB_Size (WND *wndPtr, WPARAM wParam, LPARAM lParam)

{
  RECT parent_rect;
  HWND parent;
  HDC hdc;
  UINT uPosFlags,cx,cy;

  uPosFlags=0;
  if (!wParam) {
  	parent = GetParent (wndPtr->hwndSelf);
  	GetClientRect(parent, &parent_rect);
  	cx=LOWORD (lParam);
  	cy=HIWORD (lParam);
  	if (wndPtr->dwStyle & CCS_NORESIZE) 
        uPosFlags |= (SWP_NOSIZE | SWP_NOMOVE);

  	SetWindowPos (wndPtr->hwndSelf, 0, parent_rect.left, parent_rect.top,
            cx, cy, uPosFlags | SWP_NOZORDER);
	} else {
    FIXME (tab,"WM_SIZE flag %x %lx not handled\n", wParam, lParam);
  } 

  TAB_SetItemBounds (wndPtr);
  hdc = GetDC (wndPtr->hwndSelf);
  TAB_Refresh (wndPtr, hdc);
  ReleaseDC (wndPtr->hwndSelf, hdc);

  return 0;
}


static LRESULT 
TAB_Create (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr;

    infoPtr = (TAB_INFO *)COMCTL32_Alloc (sizeof(TAB_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;
   
    infoPtr->uNumItem = 0;
    infoPtr->hFont = 0;
    infoPtr->items = 0;
    infoPtr->hcurArrow = LoadCursorA (0, IDC_ARROWA);
    infoPtr->iSelected = -1;  
	infoPtr->hwndToolTip=0;
  
    TRACE(tab, "Created tab control, hwnd [%04x]\n", wndPtr->hwndSelf); 
    if (wndPtr->dwStyle & TCS_TOOLTIPS) {
    /* Create tooltip control */
    infoPtr->hwndToolTip =
        CreateWindowExA (0, TOOLTIPS_CLASSA, NULL, 0,
                   CW_USEDEFAULT, CW_USEDEFAULT,
                   CW_USEDEFAULT, CW_USEDEFAULT,
                   wndPtr->hwndSelf, 0, 0, 0);

    /* Send NM_TOOLTIPSCREATED notification */
    if (infoPtr->hwndToolTip) {
        NMTOOLTIPSCREATED nmttc;

        nmttc.hdr.hwndFrom = wndPtr->hwndSelf;
        nmttc.hdr.idFrom = wndPtr->wIDmenu;
        nmttc.hdr.code = NM_TOOLTIPSCREATED;
        nmttc.hwndToolTips = infoPtr->hwndToolTip;

        SendMessageA (GetParent (wndPtr->hwndSelf), WM_NOTIFY,
                (WPARAM)wndPtr->wIDmenu, (LPARAM)&nmttc);
    }
    }


    return 0;
}

static LRESULT
TAB_Destroy (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
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
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {

	case TCM_GETIMAGELIST:
	   return TAB_GetImageList (wndPtr, wParam, lParam);

	case TCM_SETIMAGELIST:
	   return TAB_SetImageList (wndPtr, wParam, lParam);
	
	case TCM_GETITEMCOUNT:
	    return TAB_GetItemCount (wndPtr, wParam, lParam);

	case TCM_GETITEMA:
	   return TAB_GetItemA (wndPtr, wParam, lParam);

	case TCM_GETITEMW:
	   FIXME (tab, "Unimplemented msg TCM_GETITEM32W\n");
	   return 0;

	case TCM_SETITEMA:
	   return TAB_SetItemA (wndPtr, wParam, lParam);

	case TCM_SETITEMW:
	   FIXME (tab, "Unimplemented msg TCM_GETITEM32W\n");
	   return 0;

	case TCM_DELETEITEM:
	   return TAB_DeleteItem (wndPtr, wParam, lParam);

	case TCM_DELETEALLITEMS:
	   return TAB_DeleteAllItems (wndPtr, wParam, lParam);

	case TCM_GETITEMRECT:
	   FIXME (tab, "Unimplemented msg TCM_GETITEMRECT\n");
	   return 0;

	case TCM_GETCURSEL:
	   return TAB_GetCurSel (wndPtr);

	case TCM_HITTEST:
	   return TAB_HitTest (wndPtr, wParam, lParam);

	case TCM_SETCURSEL:
	   return TAB_SetCurSel (wndPtr, wParam);

	case TCM_INSERTITEMA:
	   return TAB_InsertItem (wndPtr, wParam, lParam);

	case TCM_INSERTITEMW:
	   FIXME (tab, "Unimplemented msg TCM_INSERTITEM32W\n");
	   return 0;

	case TCM_SETITEMEXTRA:
	   FIXME (tab, "Unimplemented msg TCM_SETITEMEXTRA\n");
	   return 0;

	case TCM_ADJUSTRECT:
	   return TAB_AdjustRect (wndPtr, wParam, lParam);

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
	   return TAB_GetToolTips (wndPtr, wParam, lParam);

	case TCM_SETTOOLTIPS:
	   return TAB_SetToolTips (wndPtr, wParam, lParam);

	case TCM_GETCURFOCUS:
    	return TAB_GetCurFocus (wndPtr);

	case TCM_SETCURFOCUS:
    	return TAB_SetCurFocus (wndPtr, wParam);

	case TCM_SETMINTTABWIDTH:
	   FIXME (tab, "Unimplemented msg TCM_SETMINTTABWIDTH\n");
	   return 0;

	case TCM_DESELECTALL:
	   FIXME (tab, "Unimplemented msg TCM_DESELECTALL\n");
	   return 0;

	case WM_GETFONT:
	    return TAB_GetFont (wndPtr, wParam, lParam);

	case WM_SETFONT:
	    return TAB_SetFont (wndPtr, wParam, lParam);

	case WM_CREATE:
	    return TAB_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return TAB_Destroy (wndPtr, wParam, lParam);

    case WM_GETDLGCODE:
            return DLGC_WANTARROWS | DLGC_WANTCHARS;

	case WM_LBUTTONDOWN:
	    return TAB_LButtonDown (wndPtr, wParam, lParam);

	case WM_LBUTTONUP:
	    return TAB_LButtonUp (wndPtr, wParam, lParam);

	case WM_RBUTTONDOWN:
	    return TAB_RButtonDown (wndPtr, wParam, lParam);

	case WM_MOUSEMOVE:
	    return TAB_MouseMove (wndPtr, wParam, lParam);

	case WM_PAINT:
	    return TAB_Paint (wndPtr, wParam);
	case WM_SIZE:
		return TAB_Size (wndPtr, wParam, lParam);
	
	case WM_SETREDRAW:
	    return TAB_SetRedraw (wndPtr, wParam);
	
	
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

