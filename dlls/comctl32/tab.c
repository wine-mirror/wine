/*
 * Tab control
 *
 * Copyright 1998 Anders Carlsson
 *
 * TODO:
 *  Image list support
 *  Multiline support
 *  Unicode support
 *  Updown control support
 *  Look and feel
 *  Messages to be added in commctrl.h
 *  ...
 */

#include "windows.h"
#include "commctrl.h"
#include "tab.h"
#include "win.h"
#include "debug.h"


#define TAB_GetInfoPtr(wndPtr) ((TAB_INFO *)wndPtr->wExtra[0])






static BOOL32
TAB_SendSimpleNotify (WND *wndPtr, UINT32 code)
{
    NMHDR nmhdr;

    nmhdr.hwndFrom = wndPtr->hwndSelf;
    nmhdr.idFrom = wndPtr->wIDmenu;
    nmhdr.code = code;

    return (BOOL32) SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
				    (WPARAM32) nmhdr.idFrom, (LPARAM) &nmhdr);
}

static LRESULT
TAB_GetCurSel (WND *wndPtr)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
 
    return infoPtr->iSelected;
}


static LRESULT
TAB_SetCurSel (WND *wndPtr,WPARAM32 wParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
	INT32 iItem=(INT32) wParam;
	INT32 prevItem;
 
	prevItem=-1;
	if ((iItem >= 0) && (iItem < infoPtr->uNumItem)) {
		prevItem=infoPtr->iSelected;
    	infoPtr->iSelected=iItem;
	}
	return prevItem;
}

static LRESULT
TAB_LButtonUp (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
    POINT32 pt;
    RECT32 rect;
    INT32 iCount;

    pt.x = (INT32)LOWORD(lParam);
    pt.y = (INT32)HIWORD(lParam);

    GetClientRect32 (wndPtr->hwndSelf, &rect);

    if (PtInRect32 (&rect, pt))
    {
	for (iCount = 0; iCount < infoPtr->uNumItem; iCount++) {
	    rect = infoPtr->items[iCount].rect;
	    if (PtInRect32 (&rect, pt)) {
		TRACE(tab, "On Tab, item %d\n", iCount);
		
		if (infoPtr->iSelected != iCount) {
			infoPtr->iSelected = iCount;

			TAB_SendSimpleNotify(wndPtr, TCN_SELCHANGE);
		}
		
		return 0;
	    }
	}
    }

    return 0;
}


static LRESULT
TAB_AdjustRect (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
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
    RECT32 rect;
    HFONT32 hFont, hOldFont;
    INT32 i, left;
    SIZE32 size;
    HDC32 hdc;

    /* FIXME: Is this needed? */
    GetClientRect32 (wndPtr->hwndSelf, &rect);
    
    hdc = GetDC32(wndPtr->hwndSelf); 
    
    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject32 (SYSTEM_FONT);
    hOldFont = SelectObject32 (hdc, hFont);

    left = rect.left;
    
    for (i = 0; i < infoPtr->uNumItem; i++)
    {
	infoPtr->items[i].rect.left = left;
	infoPtr->items[i].rect.top = infoPtr->rect.top;

	GetTextExtentPoint32A(hdc, 
			     infoPtr->items[i].pszText, 
			     lstrlen32A(infoPtr->items[i].pszText), &size);
	infoPtr->items[i].rect.right = left + size.cx+2*5;
        infoPtr->items[i].rect.bottom = infoPtr->rect.top + 20;
	TRACE(tab, "TextSize: %i - ", size.cx);
	TRACE(tab, "Rect: T %i, L %i, B %i, R %i\n", 
	      infoPtr->items[i].rect.top,
	      infoPtr->items[i].rect.left,
	      infoPtr->items[i].rect.bottom,
	      infoPtr->items[i].rect.right);	
	left += (size.cx + 11);
    }

    SelectObject32 (hdc, hOldFont);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);
}
				 
static void
TAB_DrawItem (WND *wndPtr, HDC32 hdc, INT32 iItem)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
    TAB_ITEM *pti = &infoPtr->items[iItem];
    RECT32 r;
    INT32 oldBkMode;
   
    HPEN32	hwPen  = GetSysColorPen32 (COLOR_3DHILIGHT);
    HPEN32	hbPen  = GetSysColorPen32 (COLOR_BTNSHADOW);
    HPEN32	hsdPen = GetSysColorPen32 (COLOR_BTNTEXT);
    HPEN32	htmpPen = (HPEN32)NULL;

    CopyRect32(&r, &pti->rect);

    htmpPen = hwPen;
    htmpPen = SelectObject32 (hdc, htmpPen);
    MoveToEx32 (hdc, r.left, r.bottom, NULL);
    LineTo32 (hdc, r.left, r.top + 2);
    LineTo32 (hdc, r.left +2, r.top);

    LineTo32 (hdc, r.right -1, r.top);
    htmpPen = SelectObject32 (hdc, htmpPen);

    htmpPen = SelectObject32 (hdc, hbPen);
    MoveToEx32 (hdc, r.right-1, r.top, NULL);
    LineTo32 (hdc,r.right-1, r.bottom-1);
    hbPen = SelectObject32 (hdc, hsdPen);
    MoveToEx32 (hdc, r.right, r.top+1, NULL);
    LineTo32(hdc, r.right,r.bottom);
    hsdPen = SelectObject32(hdc,htmpPen); 

    oldBkMode = SetBkMode32(hdc, TRANSPARENT);
    r.left += 3;
    r.right -= 3;
    SetTextColor32 (hdc, COLOR_BTNTEXT);
    DrawText32A(hdc, pti->pszText, lstrlen32A(pti->pszText),
	&r, DT_LEFT|DT_SINGLELINE|DT_VCENTER);
    if (oldBkMode != TRANSPARENT)
	SetBkMode32(hdc, oldBkMode);
}

static void
TAB_DrawBorder (WND *wndPtr, HDC32 hdc)
{
    HPEN32 htmPen;
    HPEN32 hwPen  = GetSysColorPen32 (COLOR_3DHILIGHT);
    HPEN32 hbPen  = GetSysColorPen32 (COLOR_3DDKSHADOW);
    HPEN32 hShade = GetSysColorPen32 (COLOR_BTNSHADOW);

    RECT32 rect;

    htmPen = SelectObject32 (hdc, hwPen);
    GetClientRect32 (wndPtr->hwndSelf, &rect);

    MoveToEx32 (hdc, rect.left, rect.bottom, NULL);
    LineTo32 (hdc, rect.left, rect.top+20); 

    LineTo32 (hdc, rect.right, rect.top+20);

    hwPen = SelectObject32 (hdc, htmPen);
    LineTo32 (hdc, rect.right, rect.bottom );
    LineTo32 (hdc, rect.left, rect.bottom);
    hbPen = SelectObject32 (hdc, hShade );
    MoveToEx32 (hdc, rect.right-1, rect.top+20, NULL);
    LineTo32 (hdc, rect.right-1, rect.bottom-1);
    LineTo32 (hdc, rect.left, rect.bottom-1);
    hShade = SelectObject32(hdc, hShade);
}

    
static void
TAB_Refresh (WND *wndPtr, HDC32 hdc)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
    HFONT32 hOldFont;
    INT32 i;

    TAB_DrawBorder (wndPtr, hdc);

	hOldFont = SelectObject32 (hdc, infoPtr->hFont);
    for (i = 0; i < infoPtr->uNumItem; i++) {
	TAB_DrawItem (wndPtr, hdc, i);
    }
	SelectObject32 (hdc, hOldFont);
}

static LRESULT
TAB_Paint (WND *wndPtr, WPARAM32 wParam)
{
    HDC32 hdc;
    PAINTSTRUCT32 ps;
    
    hdc = wParam== 0 ? BeginPaint32 (wndPtr->hwndSelf, &ps) : (HDC32)wParam;
    TAB_Refresh (wndPtr, hdc);
    
    if(!wParam)
	EndPaint32 (wndPtr->hwndSelf, &ps);
    return 0;
}

static LRESULT
TAB_InsertItem (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
    TCITEM32A *pti;
    HDC32  hdc;
    INT32 iItem, len;

    pti = (TCITEM32A *)lParam;
    iItem = (INT32)wParam;

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
	len = lstrlen32A (pti->pszText);
	infoPtr->items[iItem].pszText = COMCTL32_Alloc (len+1);
	lstrcpy32A (infoPtr->items[iItem].pszText, pti->pszText);
	infoPtr->items[iItem].cchTextMax = pti->cchTextMax;
    }

    if (pti->mask & TCIF_IMAGE)
	infoPtr->items[iItem].iImage = pti->iImage;

    if (pti->mask & TCIF_PARAM)
	infoPtr->items[iItem].lParam = pti->lParam;

    hdc = GetDC32 (wndPtr->hwndSelf);
    TAB_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    TRACE(tab, "[%04x]: added item %d '%s'\n",
		wndPtr->hwndSelf, iItem, infoPtr->items[iItem].pszText);

    TAB_SetItemBounds(wndPtr);
    return iItem;
}

static LRESULT 
TAB_SetItem32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
  TCITEM32A *tabItem; 
  TAB_ITEM *wineItem; 
  INT32    iItem,len;

  iItem=(INT32) wParam;
  tabItem=(LPTCITEM32A ) lParam;
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
	 len=lstrlen32A (tabItem->pszText);
	 if (len>wineItem->cchTextMax) 
		 wineItem->pszText= COMCTL32_ReAlloc (wineItem->pszText, len+1);
	 lstrcpyn32A (wineItem->pszText, tabItem->pszText, len);
  }

	return TRUE;
}

static LRESULT 
TAB_GetItemCount (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
   TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);

   return infoPtr->uNumItem;
}


static LRESULT 
TAB_GetItem32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
   TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
   TCITEM32A *tabItem;
   TAB_ITEM *wineItem;
   INT32    iItem;

  iItem=(INT32) wParam;
  tabItem=(LPTCITEM32A) lParam;
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
	 lstrcpyn32A (tabItem->pszText, wineItem->pszText, tabItem->cchTextMax);

	return TRUE;
}

static LRESULT 
TAB_DeleteItem (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
   TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);

	FIXME (tab,"stub \n");
	return TRUE;
}
static LRESULT 
TAB_DeleteAllItems (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
   TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);

	COMCTL32_Free (infoPtr->items);
	infoPtr->uNumItem=0;
	
	return TRUE;
}


static LRESULT
TAB_GetFont (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);

  TRACE (tab,"\n");
  return (LRESULT)infoPtr->hFont;
}

static LRESULT
TAB_SetFont (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)

{
 TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
 TEXTMETRIC32A tm;
 HFONT32 hFont, hOldFont;
 HDC32 hdc;

 TRACE (tab,"%x %lx\n",wParam, lParam);

 infoPtr->hFont = (HFONT32)wParam;

 hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject32 (SYSTEM_FONT);

 hdc = GetDC32 (0);
 hOldFont = SelectObject32 (hdc, hFont);
 GetTextMetrics32A (hdc, &tm);
 infoPtr->nHeight= tm.tmHeight + tm.tmExternalLeading;
 SelectObject32 (hdc, hOldFont);

 if (lParam) TAB_Refresh (wndPtr,hdc);
 ReleaseDC32 (0, hdc);

 return 0;
}


static LRESULT
TAB_GetImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);

  TRACE (tab,"\n");
  return (LRESULT)infoPtr->himl;
}

static LRESULT
TAB_SetImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
    HIMAGELIST himlPrev;

    TRACE (tab,"\n");
    himlPrev = infoPtr->himl;
  	infoPtr->himl= (HIMAGELIST)lParam;
    return (LRESULT)himlPrev;
}

static LRESULT 
TAB_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr;

    infoPtr = (TAB_INFO *)COMCTL32_Alloc (sizeof(TAB_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;
   
    infoPtr->uNumItem = 0;
    infoPtr->hFont = 0;
    infoPtr->items = 0;
    infoPtr->hcurArrow = LoadCursor32A (0, IDC_ARROW32A);
    infoPtr->iSelected = -1;  
  
    TRACE(tab, "Created tab control, hwnd [%04x]\n", wndPtr->hwndSelf); 
    return 0;
}

static LRESULT
TAB_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(wndPtr);
    INT32 iItem;

    if (infoPtr->items) {
	for (iItem = 0; iItem < infoPtr->uNumItem; iItem++) {
	    if (infoPtr->items[iItem].pszText)
		COMCTL32_Free (infoPtr->items[iItem].pszText);
	}
	COMCTL32_Free (infoPtr->items);
    }

    COMCTL32_Free (infoPtr);
 
    return 0;
}

LRESULT WINAPI
TAB_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
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

	case TCM_GETITEM32A:
	   return TAB_GetItem32A (wndPtr, wParam, lParam);

	case TCM_GETITEM32W:
	   FIXME (tab, "Unimplemented msg TCM_GETITEM32W\n");
	   return 0;

	case TCM_SETITEM32A:
	   return TAB_SetItem32A (wndPtr, wParam, lParam);

	case TCM_SETITEM32W:
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

	case TCM_SETCURSEL:
	   return TAB_SetCurSel (wndPtr, wParam);

	case TCM_INSERTITEM32A:
	   return TAB_InsertItem (wndPtr, wParam, lParam);

	case TCM_INSERTITEM32W:
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
	   FIXME (tab, "Unimplemented msg TCM_GETTOOLTIPS\n");
	   return 0;

	case TCM_SETTOOLTIPS:
	   FIXME (tab, "Unimplemented msg TCM_SETTOOLTIPS\n");
	   return 0;

	case TCM_GETCURFOCUS:
	   FIXME (tab, "Unimplemented msg TCM_GETCURFOCUS\n");
	   return 0;

	case TCM_SETCURFOCUS:
	   FIXME (tab, "Unimplemented msg TCM_SETCURFOCUS\n");
	   return 0;

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

	case WM_LBUTTONUP:
	    return TAB_LButtonUp (wndPtr, wParam, lParam);

	case WM_PAINT:
	    return TAB_Paint (wndPtr, wParam);
	
	
	default:
	    if (uMsg >= WM_USER)
		ERR (tab, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
TAB_Register (VOID)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (WC_TABCONTROL32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS;
    wndClass.lpfnWndProc   = (WNDPROC32)TAB_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TAB_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = WC_TABCONTROL32A;
 
    RegisterClass32A (&wndClass);
}


VOID
TAB_Unregister (VOID)
{
    if (GlobalFindAtom32A (WC_TABCONTROL32A))
	UnregisterClass32A (WC_TABCONTROL32A, (HINSTANCE32)NULL);
}

