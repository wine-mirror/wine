/*
 * Tool tip control
 *
 * Copyright 1998 Eric Kohl
 *
 * TODO:
 *   - Subclassing.
 *   - Tracking tooltips (under construction).
 *   - TTS_ALWAYSTIP (undefined).
 *   - Unicode support.
 *   - Custom draw support.
 *
 * Testing:
 *   - Run tests using Waite Group Windows95 API Bible Volume 2.
 *     The second cdrom (chapter 3) contains executables activate.exe,
 *     curtool.exe, deltool.exe, enumtools.exe, getinfo.exe, getiptxt.exe,
 *     hittest.exe, needtext.exe, newrect.exe, updtext.exe and winfrpt.exe.
 *
 *   - Activate.exe, deltool.exe, enumtool.exe, getinfo.exe and getiptxt.exe
 *     are the only working examples, since subclassing is not implemented.
 *
 * Fixme:
 *   - The "lParam" variable from NMTTDISPINFO32A is not handled
 *     in TOOLTIPS_GetTipText.
 */

#include "windows.h"
#include "commctrl.h"
#include "tooltips.h"
#include "heap.h"
#include "win.h"
#include "debug.h"


#define ID_TIMER1  1    /* show delay timer */
#define ID_TIMER2  2    /* auto pop timer */
#define ID_TIMER3  3    /* tool leave timer */

#define TOOLTIPS_GetInfoPtr(wndPtr) ((TOOLTIPS_INFO *)wndPtr->wExtra[0])


static VOID
TOOLTIPS_Refresh (WND *wndPtr, HDC32 hdc)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    RECT32 rc;
    INT32 oldBkMode;
    HFONT32 hOldFont;
    UINT32 uFlags = DT_EXTERNALLEADING;

    if (infoPtr->nMaxTipWidth > -1)
	uFlags |= DT_WORDBREAK;
    if (wndPtr->dwStyle & TTS_NOPREFIX)
	uFlags |= DT_NOPREFIX;
    GetClientRect32 (wndPtr->hwndSelf, &rc);
    rc.left   += (2 + infoPtr->rcMargin.left);
    rc.top    += (2 + infoPtr->rcMargin.top);
    rc.right  -= (2 + infoPtr->rcMargin.right);
    rc.bottom -= (2 + infoPtr->rcMargin.bottom);
    oldBkMode = SetBkMode32 (hdc, TRANSPARENT);
    SetTextColor32 (hdc, infoPtr->clrText);
    hOldFont = SelectObject32 (hdc, infoPtr->hFont);
    DrawText32A (hdc, infoPtr->szTipText, -1, &rc, uFlags);
    SelectObject32 (hdc, hOldFont);
    if (oldBkMode != TRANSPARENT)
	SetBkMode32 (hdc, oldBkMode);
}


static VOID
TOOLTIPS_GetTipText (WND *wndPtr, TOOLTIPS_INFO *infoPtr)
{
    TTTOOL_INFO *toolPtr = &infoPtr->tools[infoPtr->nCurrentTool];

    if (toolPtr->hinst) {
	TRACE (tooltips, "get res string %x %x\n",
	       toolPtr->hinst, (int)toolPtr->lpszText);
	LoadString32A (toolPtr->hinst, (UINT32)toolPtr->lpszText,
		       infoPtr->szTipText, INFOTIPSIZE);
    }
    else if (toolPtr->lpszText) {
	if (toolPtr->lpszText == LPSTR_TEXTCALLBACK32A) {
	    NMTTDISPINFO32A ttnmdi;

	    /* fill NMHDR struct */
	    ZeroMemory (&ttnmdi, sizeof(NMTTDISPINFO32A));
	    ttnmdi.hdr.hwndFrom = wndPtr->hwndSelf;
	    ttnmdi.hdr.idFrom = toolPtr->uId;
	    ttnmdi.hdr.code = TTN_GETDISPINFO32A;
	    ttnmdi.uFlags = toolPtr->uFlags;
	    ttnmdi.lpszText = infoPtr->szTipText;

	    TRACE (tooltips, "hdr.idFrom = %x\n", ttnmdi.hdr.idFrom);
	    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
			    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&ttnmdi);

	    if (ttnmdi.hinst) {
		LoadString32A (ttnmdi.hinst, (UINT32)ttnmdi.szText,
			       infoPtr->szTipText, INFOTIPSIZE);
		if (ttnmdi.uFlags & TTF_DI_SETITEM) {
		    toolPtr->hinst = ttnmdi.hinst;
		    toolPtr->lpszText = ttnmdi.szText;
		}
	    }
	    else if (ttnmdi.szText[0]) {
		lstrcpyn32A (infoPtr->szTipText, ttnmdi.szText, 80);
		if (ttnmdi.uFlags & TTF_DI_SETITEM) {
		    INT32 len = lstrlen32A (ttnmdi.szText) + 1;
		    toolPtr->hinst = 0;
		    toolPtr->lpszText =	
			HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, len);
		    lstrcpy32A (toolPtr->lpszText, ttnmdi.szText);
		}
	    }
	    else if (ttnmdi.lpszText == 0) {
		/* no text available */
		infoPtr->szTipText[0] = '\0';
	    }
	    else if (ttnmdi.lpszText != LPSTR_TEXTCALLBACK32A) {
		if (ttnmdi.lpszText != infoPtr->szTipText)
		    lstrcpyn32A (infoPtr->szTipText, ttnmdi.lpszText,
				 INFOTIPSIZE);
		if (ttnmdi.uFlags & TTF_DI_SETITEM) {
		    INT32 len = lstrlen32A (ttnmdi.lpszText) + 1;
		    toolPtr->hinst = 0;
		    toolPtr->lpszText =	
			HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, len);
		    lstrcpy32A (toolPtr->lpszText, ttnmdi.lpszText);
		}
	    }
	    else
		ERR (tooltips, "recursive text callback!\n");
	}
	else
	    lstrcpyn32A (infoPtr->szTipText, toolPtr->lpszText, INFOTIPSIZE);
    }
    else
	/* no text available */
	infoPtr->szTipText[0] = '\0';

    TRACE (tooltips, "\"%s\"\n", infoPtr->szTipText);
}


static VOID
TOOLTIPS_CalcTipSize (WND *wndPtr, TOOLTIPS_INFO *infoPtr, LPSIZE32 lpSize)
{
    HDC32 hdc;
    HFONT32 hOldFont;
    UINT32 uFlags = DT_EXTERNALLEADING | DT_CALCRECT;
    RECT32 rc = {0, 0, 0, 0};

    if (infoPtr->nMaxTipWidth > -1) {
	rc.right = infoPtr->nMaxTipWidth;
	uFlags |= DT_WORDBREAK;
    }
    if (wndPtr->dwStyle & TTS_NOPREFIX)
	uFlags |= DT_NOPREFIX;
    TRACE (tooltips, "\"%s\"\n", infoPtr->szTipText);

    hdc = GetDC32 (wndPtr->hwndSelf);
    hOldFont = SelectObject32 (hdc, infoPtr->hFont);
    DrawText32A (hdc, infoPtr->szTipText, -1, &rc, uFlags);
    SelectObject32 (hdc, hOldFont);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    lpSize->cx = rc.right - rc.left + 4 + 
		 infoPtr->rcMargin.left + infoPtr->rcMargin.right;
    lpSize->cy = rc.bottom - rc.top + 4 +
		 infoPtr->rcMargin.bottom + infoPtr->rcMargin.top;
}


static VOID
TOOLTIPS_Show (WND *wndPtr, TOOLTIPS_INFO *infoPtr)
{
    TTTOOL_INFO *toolPtr;
    POINT32 pt;
    SIZE32 size;
    NMHDR hdr;

    if (infoPtr->nTool == -1) {
	TRACE (tooltips, "invalid tool (-1)!\n");
	return;
    }

    infoPtr->nCurrentTool = infoPtr->nTool;

    TRACE (tooltips, "Show tooltip pre %d!\n", infoPtr->nTool);

    TOOLTIPS_GetTipText (wndPtr, infoPtr);

    if (infoPtr->szTipText[0] == '\0') {
	infoPtr->nCurrentTool = -1;
	return;
    }

    TRACE (tooltips, "Show tooltip %d!\n", infoPtr->nTool);

    hdr.hwndFrom = wndPtr->hwndSelf;
    hdr.idFrom = infoPtr->nTool;
    hdr.code = TTN_SHOW;
    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
		    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&hdr);

    TRACE (tooltips, "\"%s\"\n", infoPtr->szTipText);

    toolPtr = &infoPtr->tools[infoPtr->nTool];
    TOOLTIPS_CalcTipSize (wndPtr, infoPtr, &size);
    TRACE (tooltips, "size %d - %d\n", size.cx, size.cy);

    if ((toolPtr->uFlags & TTF_TRACK) && (toolPtr->uFlags & TTF_ABSOLUTE)) {
	pt.x = infoPtr->xTrackPos;
	pt.y = infoPtr->yTrackPos;
    }
    else if (toolPtr->uFlags & TTF_CENTERTIP) {
	RECT32 rect;

	if (toolPtr->uFlags & TTF_IDISHWND)
	    GetWindowRect32 ((HWND32)toolPtr->uId, &rect);
	else {
	    rect = toolPtr->rect;
	    MapWindowPoints32 (toolPtr->hwnd, (HWND32)0, (LPPOINT32)&rect, 2);
	}
	pt.x = (rect.left + rect.right - size.cx) / 2;
	pt.y = rect.bottom + 2;
    }
    else {
	GetCursorPos32 (&pt);
	pt.y += 20;
    }

    TRACE (tooltips, "pos %d - %d\n", pt.x, pt.y);

//    SetWindowPos32 (wndPtr->hwndSelf, HWND_TOP, pt.x, pt.y,
//		    size.cx, size.cy, SWP_SHOWWINDOW);
    SetWindowPos32 (wndPtr->hwndSelf, HWND_TOP, 1, 1,
		    size.cx, size.cy, SWP_SHOWWINDOW);

    SetTimer32 (wndPtr->hwndSelf, ID_TIMER2, infoPtr->nAutoPopTime, 0);
}


static VOID
TOOLTIPS_Hide (WND *wndPtr, TOOLTIPS_INFO *infoPtr)
{
    NMHDR hdr;

    if (infoPtr->nCurrentTool == -1)
	return;

    TRACE (tooltips, "Hide tooltip %d!\n", infoPtr->nCurrentTool);
    KillTimer32 (wndPtr->hwndSelf, ID_TIMER2);

    hdr.hwndFrom = wndPtr->hwndSelf;
    hdr.idFrom = infoPtr->nCurrentTool;
    hdr.code = TTN_POP;
    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
		    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&hdr);

    infoPtr->nCurrentTool = -1;

    SetWindowPos32 (wndPtr->hwndSelf, HWND_TOP, 0, 0, 0, 0, SWP_HIDEWINDOW);
}


static INT32
TOOLTIPS_GetToolFromInfoA (TOOLTIPS_INFO *infoPtr, LPTTTOOLINFO32A lpToolInfo)
{
    TTTOOL_INFO *toolPtr;
    INT32 nTool;

#if 0
    for (nTool = 0; nTool < infoPtr->uNumTools; nTool++) {
	toolPtr = &infoPtr->tools[nTool];

	if (toolPtr->uFlags & TTF_IDISHWND) {
	    if (lpToolInfo->uId == toolPtr->uId)
		return nTool;
	}
	else {
	    if ((lpToolInfo->hwnd == toolPtr->hwnd) &&
		(lpToolInfo->uId == toolPtr->uId))
		return nTool;
	}
    }
#endif

    for (nTool = 0; nTool < infoPtr->uNumTools; nTool++) {
	toolPtr = &infoPtr->tools[nTool];

	if (!(toolPtr->uFlags & TTF_IDISHWND) && 
	    (lpToolInfo->hwnd == toolPtr->hwnd) &&
	    (lpToolInfo->uId == toolPtr->uId))
	    return nTool;
    }

    for (nTool = 0; nTool < infoPtr->uNumTools; nTool++) {
	toolPtr = &infoPtr->tools[nTool];

	if ((toolPtr->uFlags & TTF_IDISHWND) &&
	    (lpToolInfo->uId == toolPtr->uId))
	    return nTool;
    }

    return -1;
}


static INT32
TOOLTIPS_GetToolFromPoint (TOOLTIPS_INFO *infoPtr, HWND32 hwnd, LPPOINT32 lpPt)
{
    TTTOOL_INFO *toolPtr;
    INT32  nTool;

    for (nTool = 0; nTool < infoPtr->uNumTools; nTool++) {
	toolPtr = &infoPtr->tools[nTool];

	if (toolPtr->uFlags & TTF_IDISHWND) {
	    if ((HWND32)toolPtr->uId == hwnd)
		return nTool;
	}
	else {
	    if (hwnd != toolPtr->hwnd)
		continue;
	    if (!PtInRect32 (&toolPtr->rect, *lpPt))
		continue;
	    return nTool;
	}
    }

    return -1;
}


static BOOL32
TOOLTIPS_CheckTool (WND *wndPtr)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    POINT32 pt;
    HWND32 hwndTool;
    INT32 nTool;

    GetCursorPos32 (&pt);
    hwndTool =
	SendMessage32A (wndPtr->hwndSelf, TTM_WINDOWFROMPOINT, 0, (LPARAM)&pt);
    if (hwndTool == 0)
	return FALSE;

    ScreenToClient32 (hwndTool, &pt);
    nTool = TOOLTIPS_GetToolFromPoint (infoPtr, hwndTool, &pt);
    TRACE (tooltips, "tool %d\n", nTool);

    return (nTool != -1);
}


static LRESULT
TOOLTIPS_Activate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    infoPtr->bActive = (BOOL32)wParam;

    if (infoPtr->bActive)
	TRACE (tooltips, "activate!\n");

    if (!(infoPtr->bActive) && (infoPtr->nCurrentTool != -1)) 
	    TOOLTIPS_Hide (wndPtr, infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_AddTool32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32A lpToolInfo = (LPTTTOOLINFO32A)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo == NULL) return FALSE;

    if (lpToolInfo->uFlags & TTF_SUBCLASS)
	FIXME (tooltips, "subclassing not supported!\n");

    TRACE (tooltips, "add tool (%x) %x %d%s!\n",
	   wndPtr->hwndSelf, lpToolInfo->hwnd, lpToolInfo->uId,
	   (lpToolInfo->uFlags & TTF_IDISHWND) ? " TTF_IDISHWND" : "");

    if (infoPtr->uNumTools == 0) {
	infoPtr->tools =
	    HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
		       sizeof(TTTOOL_INFO));
	toolPtr = infoPtr->tools;
    }
    else {
	TTTOOL_INFO *oldTools = infoPtr->tools;
	infoPtr->tools =
	    HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
		       sizeof(TTTOOL_INFO) * (infoPtr->uNumTools + 1));
	memcpy (infoPtr->tools, oldTools,
		infoPtr->uNumTools * sizeof(TTTOOL_INFO));
	HeapFree (GetProcessHeap (), 0, oldTools);
	toolPtr = &infoPtr->tools[infoPtr->uNumTools];
    }

    infoPtr->uNumTools++;

    /* copy tool data */
    toolPtr->uFlags = lpToolInfo->uFlags;
    toolPtr->hwnd   = lpToolInfo->hwnd;
    toolPtr->uId    = lpToolInfo->uId;
    toolPtr->rect   = lpToolInfo->rect;
    toolPtr->hinst  = lpToolInfo->hinst;

    if (lpToolInfo->hinst) {
	TRACE (tooltips, "add string id %x!\n", (int)lpToolInfo->lpszText);
	toolPtr->lpszText = lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACK32A) {
	    TRACE (tooltips, "add CALLBACK!\n");
	    toolPtr->lpszText = lpToolInfo->lpszText;
	}
	else {
	    INT32 len = lstrlen32A (lpToolInfo->lpszText);
	    TRACE (tooltips, "add text \"%s\"!\n", lpToolInfo->lpszText);
	    toolPtr->lpszText =
		HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, len + 1);
	    lstrcpy32A (toolPtr->lpszText, lpToolInfo->lpszText);
	}
    }

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFO32A))
	toolPtr->lParam = lpToolInfo->lParam;

    return TRUE;
}


// << TOOLTIPS_AddTool32W >>


static LRESULT
TOOLTIPS_DelTool32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32A lpToolInfo = (LPTTTOOLINFO32A)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nTool;

    if (lpToolInfo == NULL) return 0;
    if (infoPtr->uNumTools == 0) return 0;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    TRACE (tooltips, "tool %d\n", nTool);

    /* delete text string */
    toolPtr = &infoPtr->tools[nTool]; 
    if ((toolPtr->hinst) && (toolPtr->lpszText)) {
	if (toolPtr->lpszText != LPSTR_TEXTCALLBACK32A)
	    HeapFree (GetProcessHeap (), 0, toolPtr->lpszText);
    }

    /* delete tool from tool list */
    if (infoPtr->uNumTools == 1) {
	HeapFree (GetProcessHeap (), 0, infoPtr->tools);
	infoPtr->tools = NULL;
    }
    else {
	TTTOOL_INFO *oldTools = infoPtr->tools;
	infoPtr->tools =
	    HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
		       sizeof(TTTOOL_INFO) * (infoPtr->uNumTools - 1));

	if (nTool > 0)
	    memcpy (&infoPtr->tools[0], &oldTools[0],
		    nTool * sizeof(TTTOOL_INFO));

	if (nTool < infoPtr->uNumTools - 1)
	    memcpy (&infoPtr->tools[nTool], &oldTools[nTool + 1],
		    (infoPtr->uNumTools - nTool - 1) * sizeof(TTTOOL_INFO));

	HeapFree (GetProcessHeap (), 0, oldTools);
    }

    infoPtr->uNumTools--;

    return 0;
}


// << TOOLTIPS_DelTool32W >>


static LRESULT
TOOLTIPS_EnumTools32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    UINT32 uIndex = (UINT32)wParam;
    LPTTTOOLINFO32A lpToolInfo = (LPTTTOOLINFO32A)lParam;
    TTTOOL_INFO *toolPtr;

    if (uIndex >= infoPtr->uNumTools) return FALSE;
    if (lpToolInfo == NULL) return FALSE;

    TRACE (tooltips, "index=%u\n", uIndex);

    toolPtr = &infoPtr->tools[uIndex];

    /* copy tool data */
    lpToolInfo->uFlags   = toolPtr->uFlags;
    lpToolInfo->hwnd     = toolPtr->hwnd;
    lpToolInfo->uId      = toolPtr->uId;
    lpToolInfo->rect     = toolPtr->rect;
    lpToolInfo->hinst    = toolPtr->hinst;
    lpToolInfo->lpszText = toolPtr->lpszText;

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFO32A))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


// << TOOLTIPS_EnumTools32W >>


static LRESULT
TOOLTIPS_GetCurrentTool32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32A lpti = (LPTTTOOLINFO32A)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpti) {
	if (infoPtr->nCurrentTool > -1) {
	    toolPtr = &infoPtr->tools[infoPtr->nCurrentTool];

	    /* copy tool data */
	    lpti->uFlags   = toolPtr->uFlags;
	    lpti->rect     = toolPtr->rect;
	    lpti->hinst    = toolPtr->hinst;
	    lpti->lpszText = toolPtr->lpszText;

	    if (lpti->cbSize >= sizeof(TTTOOLINFO32A))
		lpti->lParam = toolPtr->lParam;

	    return TRUE;
	}
	else
	    return FALSE;
    }
    else
	return (infoPtr->nCurrentTool != -1);

    return FALSE;
}


// << TOOLTIPS_GetCurrentTool32W >>


static LRESULT
TOOLTIPS_GetDelayTime (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    switch (wParam) {
	case TTDT_AUTOMATIC:
	    return infoPtr->nAutomaticTime;

	case TTDT_RESHOW:
	    return infoPtr->nReshowTime;

	case TTDT_AUTOPOP:
	    return infoPtr->nAutoPopTime;

	case TTDT_INITIAL:
	    return infoPtr->nInitialTime;
    }

    return 0;
}


static LRESULT
TOOLTIPS_GetMargin (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPRECT32 lpRect = (LPRECT32)lParam;

    lpRect->left   = infoPtr->rcMargin.left;
    lpRect->right  = infoPtr->rcMargin.right;
    lpRect->bottom = infoPtr->rcMargin.bottom;
    lpRect->top    = infoPtr->rcMargin.top;

    return 0;
}


static LRESULT
TOOLTIPS_GetMaxTipWidth (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    return infoPtr->nMaxTipWidth;
}


static LRESULT
TOOLTIPS_GetText32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32A lpti = (LPTTTOOLINFO32A)lParam;
    INT32 nTool;

    if (!(lpti)) return 0;
    if (lpti->cbSize < sizeof(TTTOOLINFO32A)) return 0;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpti);
    if (nTool == -1) return 0;

    lstrcpy32A (lpti->lpszText, infoPtr->tools[nTool].lpszText);

    return 0;
}


// << TOOLTIPS_GetText32W >>


static LRESULT
TOOLTIPS_GetTipBkColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    return infoPtr->clrBk;
}


static LRESULT
TOOLTIPS_GetTipTextColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    return infoPtr->clrText;
}


static LRESULT
TOOLTIPS_GetToolCount (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    return infoPtr->uNumTools;
}


static LRESULT
TOOLTIPS_GetToolInfo32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32A lpToolInfo = (LPTTTOOLINFO32A)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nTool;

    if (lpToolInfo == NULL) return FALSE;
    if (infoPtr->uNumTools == 0) return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
    if (nTool == -1) return FALSE;

    TRACE (tooltips, "tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool data */
    lpToolInfo->uFlags   = toolPtr->uFlags;
    lpToolInfo->rect     = toolPtr->rect;
    lpToolInfo->hinst    = toolPtr->hinst;
    lpToolInfo->lpszText = toolPtr->lpszText;

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFO32A))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


// << TOOLTIPS_GetToolInfo32W >>


static LRESULT
TOOLTIPS_HitTest32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTHITTESTINFO32A lptthit = (LPTTHITTESTINFO32A)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nTool;

    if (lptthit == 0)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromPoint (infoPtr, lptthit->hwnd, &lptthit->pt);
    if (nTool == -1)
	return FALSE;

    TRACE (tooltips, "tool %d!\n", nTool);

    /* copy tool data */
    toolPtr = &infoPtr->tools[nTool];
    lptthit->ti.cbSize   = sizeof(TTTOOLINFO32A);
    lptthit->ti.uFlags   = toolPtr->uFlags;
    lptthit->ti.hwnd     = toolPtr->hwnd;
    lptthit->ti.uId      = toolPtr->uId;
    lptthit->ti.rect     = toolPtr->rect;
    lptthit->ti.hinst    = toolPtr->hinst;
    lptthit->ti.lpszText = toolPtr->lpszText;
    lptthit->ti.lParam   = toolPtr->lParam;

    return TRUE;
}


// << TOOLTIPS_HitTest32W >>


static LRESULT
TOOLTIPS_NewToolRect32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32A lpti = (LPTTTOOLINFO32A)lParam;
    INT32 nTool;

    if (!(lpti)) return 0;
    if (lpti->cbSize < sizeof(TTTOOLINFO32A)) return 0;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpti);
    if (nTool == -1) return 0;

    infoPtr->tools[nTool].rect = lpti->rect;

    return 0;
}


// << TOOLTIPS_NewToolRect32W >>


static LRESULT
TOOLTIPS_Pop (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    TOOLTIPS_Hide (wndPtr, infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_RelayEvent (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPMSG32 lpMsg = (LPMSG32)lParam;
    POINT32 pt;

    if (lpMsg == NULL) {
	ERR (tooltips, "lpMsg == NULL!\n");
	return 0;
    }

    pt = lpMsg->pt;
    ScreenToClient32 (lpMsg->hwnd, &pt);
    infoPtr->nOldTool = infoPtr->nTool;
    infoPtr->nTool = TOOLTIPS_GetToolFromPoint (infoPtr, lpMsg->hwnd, &pt);
    TRACE (tooltips, "tool (%x) %d %d\n", wndPtr->hwndSelf, infoPtr->nOldTool, infoPtr->nTool);

    switch (lpMsg->message) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	    TOOLTIPS_Hide (wndPtr, infoPtr);
	    break;

	case WM_MOUSEMOVE:
	    TRACE (tooltips, "WM_MOUSEMOVE (%04x %d %d)\n",
		   wndPtr->hwndSelf, pt.x, pt.y);
	    if ((infoPtr->bActive) && (infoPtr->nTool != infoPtr->nOldTool)) {
		if (infoPtr->nOldTool == -1) {
		    SetTimer32 (wndPtr->hwndSelf, ID_TIMER1,
				infoPtr->nInitialTime, 0);
		    TRACE (tooltips, "timer 1 started!\n");
		}
		else {
		    TOOLTIPS_Hide (wndPtr, infoPtr);
		    SetTimer32 (wndPtr->hwndSelf, ID_TIMER1,
				infoPtr->nReshowTime, 0);
		    TRACE (tooltips, "timer 2 started!\n");
		}
	    }
	    if (infoPtr->nCurrentTool != -1) {
		SetTimer32 (wndPtr->hwndSelf, ID_TIMER3, 100, 0);
		TRACE (tooltips, "timer 3 started!\n");
	    }
	    break;
    }

    return 0;
}


static LRESULT
TOOLTIPS_SetDelayTime (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    INT32 nTime = (INT32)LOWORD(lParam);

    switch (wParam) {
	case TTDT_AUTOMATIC:
	    if (nTime == 0) {
		infoPtr->nAutomaticTime = 500;
		infoPtr->nReshowTime    = 100;
		infoPtr->nAutoPopTime   = 5000;
		infoPtr->nInitialTime   = 500;
	    }
	    else {
		infoPtr->nAutomaticTime = nTime;
		infoPtr->nReshowTime    = nTime / 5;
		infoPtr->nAutoPopTime   = nTime * 10;
		infoPtr->nInitialTime   = nTime;
	    }
	    break;

	case TTDT_RESHOW:
	    infoPtr->nReshowTime = nTime;
	    break;

	case TTDT_AUTOPOP:
	    infoPtr->nAutoPopTime = nTime;
	    break;

	case TTDT_INITIAL:
	    infoPtr->nInitialTime = nTime;
	    break;
    }

    return 0;
}


static LRESULT
TOOLTIPS_SetMargin (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPRECT32 lpRect = (LPRECT32)lParam;

    infoPtr->rcMargin.left   = lpRect->left;
    infoPtr->rcMargin.right  = lpRect->right;
    infoPtr->rcMargin.bottom = lpRect->bottom;
    infoPtr->rcMargin.top    = lpRect->top;

    return 0;
}


static LRESULT
TOOLTIPS_SetMaxTipWidth (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    INT32 nTemp = infoPtr->nMaxTipWidth;

    infoPtr->nMaxTipWidth = (INT32)lParam;

    return nTemp;
}


static LRESULT
TOOLTIPS_SetTipBkColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    infoPtr->clrBk = (COLORREF)wParam;

    return 0;
}


static LRESULT
TOOLTIPS_SetTipTextColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    infoPtr->clrText = (COLORREF)wParam;

    return 0;
}


static LRESULT
TOOLTIPS_SetToolInfo32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32A lpToolInfo = (LPTTTOOLINFO32A)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nTool;

    if (lpToolInfo == NULL) return 0;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    TRACE (tooltips, "tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool data */
    toolPtr->uFlags = lpToolInfo->uFlags;
    toolPtr->hwnd   = lpToolInfo->hwnd;
    toolPtr->uId    = lpToolInfo->uId;
    toolPtr->rect   = lpToolInfo->rect;
    toolPtr->hinst  = lpToolInfo->hinst;

    if (lpToolInfo->hinst) {
	toolPtr->lpszText = lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACK32A)
	    toolPtr->lpszText = lpToolInfo->lpszText;
	else {
	    INT32 len = lstrlen32A (lpToolInfo->lpszText);
	    HeapFree (GetProcessHeap (), 0, toolPtr->lpszText);
	    toolPtr->lpszText =
		HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, len + 1);
	    lstrcpy32A (toolPtr->lpszText, lpToolInfo->lpszText);
	}
    }

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFO32A))
	toolPtr->lParam = lpToolInfo->lParam;

    return 0;
}


// << TOOLTIPS_SetToolInfo32W >>


static LRESULT
TOOLTIPS_TrackActivate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32A lpToolInfo = (LPTTTOOLINFO32A)lParam;

    if ((BOOL32)wParam) {
	/* activate */
	infoPtr->nTrackTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
	if (infoPtr->nTrackTool != -1) {
	    infoPtr->bTrackActive = TRUE;

	    /* FIXME : show tool tip */
	}
    }
    else {
	/* deactivate */
	/* FIXME : hide tool tip */

	infoPtr->bTrackActive = FALSE;
	infoPtr->nTrackTool = -1;
    }

    return 0;
}


static LRESULT
TOOLTIPS_TrackPosition (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    infoPtr->xTrackPos = (INT32)LOWORD(lParam);
    infoPtr->yTrackPos = (INT32)HIWORD(lParam);

    if (infoPtr->bTrackActive) {

	FIXME (tooltips, "set position!\n");
    }

    return 0;
}


static LRESULT
TOOLTIPS_Update (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    if (infoPtr->nCurrentTool != -1)
	UpdateWindow32 (wndPtr->hwndSelf);

    return 0;
}


static LRESULT
TOOLTIPS_UpdateTipText32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32A lpToolInfo = (LPTTTOOLINFO32A)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nTool;

    if (lpToolInfo == NULL) return 0;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    TRACE (tooltips, "tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool text */
    toolPtr->hinst  = lpToolInfo->hinst;

    if (lpToolInfo->hinst) {
	toolPtr->lpszText = lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACK32A)
	    toolPtr->lpszText = lpToolInfo->lpszText;
	else {
	    INT32 len = lstrlen32A (lpToolInfo->lpszText);
	    HeapFree (GetProcessHeap (), 0, toolPtr->lpszText);
	    toolPtr->lpszText =
		HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, len + 1);
	    lstrcpy32A (toolPtr->lpszText, lpToolInfo->lpszText);
	}
    }

    return 0;
}


// << TOOLTIPS_UpdateTipText32W >>


static LRESULT
TOOLTIPS_WindowFromPoint (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    return WindowFromPoint32 (*((LPPOINT32)lParam));
}



static LRESULT
TOOLTIPS_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr;
    LOGFONT32A logFont;

    /* allocate memory for info structure */
    infoPtr = (TOOLTIPS_INFO *)HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
					  sizeof(TOOLTIPS_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
	ERR (tooltips, "could not allocate info memory!\n");
	return 0;
    }

    /* initialize info structure */
    infoPtr->bActive = TRUE;
    infoPtr->bTrackActive = FALSE;
    infoPtr->clrBk   = GetSysColor32 (COLOR_INFOBK);
    infoPtr->clrText = GetSysColor32 (COLOR_INFOTEXT);

    SystemParametersInfo32A( SPI_GETICONTITLELOGFONT, 0, &logFont, 0 );
    infoPtr->hFont = CreateFontIndirect32A( &logFont );

    infoPtr->nMaxTipWidth = -1;
    infoPtr->nTool = -1;
    infoPtr->nOldTool = -1;
    infoPtr->nCurrentTool = -1;
    infoPtr->nTrackTool = -1;

    infoPtr->nAutomaticTime = 500;
    infoPtr->nReshowTime    = 100;
    infoPtr->nAutoPopTime   = 5000;
    infoPtr->nInitialTime   = 500;

    SetWindowPos32 (wndPtr->hwndSelf, HWND_TOP, 0, 0, 0, 0, SWP_HIDEWINDOW);

    return 0;
}


static LRESULT
TOOLTIPS_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    /* free tools */
    if (infoPtr->tools) {
	INT32 i;
	for (i = 0; i < infoPtr->uNumTools; i++) {
	    if ((infoPtr->tools[i].hinst) && (infoPtr->tools[i].lpszText)) {
		if (infoPtr->tools[i].lpszText != LPSTR_TEXTCALLBACK32A)
		    HeapFree (GetProcessHeap (), 0, infoPtr->tools[i].lpszText);
	    }
	}
	HeapFree (GetProcessHeap (), 0, infoPtr->tools);
    }

    /* delete font */
    DeleteObject32 (infoPtr->hFont);

    /* free tool tips info data */
    HeapFree (GetProcessHeap (), 0, infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_EraseBackground (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    RECT32 rect;
    HBRUSH32 hBrush;

    hBrush = CreateSolidBrush32 (infoPtr->clrBk);
    GetClientRect32 (wndPtr->hwndSelf, &rect);
    FillRect32 ((HDC32)wParam, &rect, hBrush);
    DeleteObject32 (hBrush);

    return FALSE;
}


static LRESULT
TOOLTIPS_GetFont (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    return infoPtr->hFont;
}


static LRESULT
TOOLTIPS_MouseMove (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    TOOLTIPS_Hide (wndPtr, infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_NcCreate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    wndPtr->dwStyle &= 0x0000FFFF;
    wndPtr->dwStyle |= (WS_POPUP | WS_BORDER);

    return TRUE;
}


static LRESULT
TOOLTIPS_Paint (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HDC32 hdc;
    PAINTSTRUCT32 ps;

    hdc = wParam==0 ? BeginPaint32 (wndPtr->hwndSelf, &ps) : (HDC32)wParam;
    TOOLTIPS_Refresh (wndPtr, hdc);
    if (!wParam)
	EndPaint32 (wndPtr->hwndSelf, &ps);
    return 0;
}


static LRESULT
TOOLTIPS_SetFont (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    infoPtr->hFont = (HFONT32)wParam;

    if ((LOWORD(lParam)) & (infoPtr->nCurrentTool != -1)) {
	FIXME (tooltips, "full redraw needed!\n");
    }

    return 0;
}


static LRESULT
TOOLTIPS_Timer (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    TRACE (tooltips, "timer %d (%x) expired!\n", wParam, wndPtr->hwndSelf);

    switch (wParam)
    {
	case ID_TIMER1:
	    KillTimer32 (wndPtr->hwndSelf, ID_TIMER1);
	    TOOLTIPS_Show (wndPtr, infoPtr);
	    break;

	case ID_TIMER2:
	    TOOLTIPS_Hide (wndPtr, infoPtr);
	    break;

	case ID_TIMER3:
	    KillTimer32 (wndPtr->hwndSelf, ID_TIMER3);
	    if (TOOLTIPS_CheckTool (wndPtr) == FALSE) {
		infoPtr->nTool = -1;
		infoPtr->nOldTool = -1;
		TOOLTIPS_Hide (wndPtr, infoPtr);
	    }
	    break;
    }
    return 0;
}


static LRESULT
TOOLTIPS_WinIniChange (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LOGFONT32A logFont;

    infoPtr->clrBk   = GetSysColor32 (COLOR_INFOBK);
    infoPtr->clrText = GetSysColor32 (COLOR_INFOTEXT);

    DeleteObject32 (infoPtr->hFont);
    SystemParametersInfo32A( SPI_GETICONTITLELOGFONT, 0, &logFont, 0 );
    infoPtr->hFont = CreateFontIndirect32A( &logFont );

    return 0;
}


LRESULT WINAPI
ToolTipsWindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
	case TTM_ACTIVATE:
	    return TOOLTIPS_Activate (wndPtr, wParam, lParam);

	case TTM_ADDTOOL32A:
	    return TOOLTIPS_AddTool32A (wndPtr, wParam, lParam);

//	case TTM_ADDTOOL32W:

	case TTM_DELTOOL32A:
	    return TOOLTIPS_DelTool32A (wndPtr, wParam, lParam);

//	case TTM_DELTOOL32W:

	case TTM_ENUMTOOLS32A:
	    return TOOLTIPS_EnumTools32A (wndPtr, wParam, lParam);

//	case TTM_ENUMTOOLS32W:

	case TTM_GETCURRENTTOOL32A:
	    return TOOLTIPS_GetCurrentTool32A (wndPtr, wParam, lParam);

//	case TTM_GETCURRENTTOOL32W:

	case TTM_GETDELAYTIME:
	    return TOOLTIPS_GetDelayTime (wndPtr, wParam, lParam);

	case TTM_GETMARGIN:
	    return TOOLTIPS_GetMargin (wndPtr, wParam, lParam);

	case TTM_GETMAXTIPWIDTH:
	    return TOOLTIPS_GetMaxTipWidth (wndPtr, wParam, lParam);

	case TTM_GETTEXT32A:
	    return TOOLTIPS_GetText32A (wndPtr, wParam, lParam);

//	case TTM_GETTEXT32W:

	case TTM_GETTIPBKCOLOR:
	    return TOOLTIPS_GetTipBkColor (wndPtr, wParam, lParam);

	case TTM_GETTIPTEXTCOLOR:
	    return TOOLTIPS_GetTipTextColor (wndPtr, wParam, lParam);

	case TTM_GETTOOLCOUNT:
	    return TOOLTIPS_GetToolCount (wndPtr, wParam, lParam);

	case TTM_GETTOOLINFO32A:
	    return TOOLTIPS_GetToolInfo32A (wndPtr, wParam, lParam);

//	case TTM_GETTOOLINFO32W:

	case TTM_HITTEST32A:
	    return TOOLTIPS_HitTest32A (wndPtr, wParam, lParam);

//	case TTM_HITTEST32W:

	case TTM_NEWTOOLRECT32A:
	    return TOOLTIPS_NewToolRect32A (wndPtr, wParam, lParam);

//	case TTM_NEWTOOLRECT32W:

	case TTM_POP:
	    return TOOLTIPS_Pop (wndPtr, wParam, lParam);

	case TTM_RELAYEVENT:
	    return TOOLTIPS_RelayEvent (wndPtr, wParam, lParam);

	case TTM_SETDELAYTIME:
	    return TOOLTIPS_SetDelayTime (wndPtr, wParam, lParam);

	case TTM_SETMARGIN:
	    return TOOLTIPS_SetMargin (wndPtr, wParam, lParam);

	case TTM_SETMAXTIPWIDTH:
	    return TOOLTIPS_SetMaxTipWidth (wndPtr, wParam, lParam);

	case TTM_SETTIPBKCOLOR:
	    return TOOLTIPS_SetTipBkColor (wndPtr, wParam, lParam);

	case TTM_SETTIPTEXTCOLOR:
	    return TOOLTIPS_SetTipTextColor (wndPtr, wParam, lParam);

	case TTM_SETTOOLINFO32A:
	    return TOOLTIPS_SetToolInfo32A (wndPtr, wParam, lParam);

//	case TTM_SETTOOLINFO32W:

	case TTM_TRACKACTIVATE:
	    return TOOLTIPS_TrackActivate (wndPtr, wParam, lParam);

	case TTM_TRACKPOSITION:
	    return TOOLTIPS_TrackPosition (wndPtr, wParam, lParam);

	case TTM_UPDATE:
	    return TOOLTIPS_Update (wndPtr, wParam, lParam);

	case TTM_UPDATETIPTEXT32A:
	    return TOOLTIPS_UpdateTipText32A (wndPtr, wParam, lParam);

//	case TTM_UPDATETIPTEXT32W:

	case TTM_WINDOWFROMPOINT:
	    return TOOLTIPS_WindowFromPoint (wndPtr, wParam, lParam);


	case WM_CREATE:
	    return TOOLTIPS_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return TOOLTIPS_Destroy (wndPtr, wParam, lParam);

	case WM_ERASEBKGND:
	    return TOOLTIPS_EraseBackground (wndPtr, wParam, lParam);

	case WM_GETFONT:
	    return TOOLTIPS_GetFont (wndPtr, wParam, lParam);

//	case WM_GETTEXT:
//	case WM_GETTEXTLENGTH:
//	case WM_LBUTTONDOWN:
//	case WM_MBUTTONDOWN:

	case WM_MOUSEMOVE:
	    return TOOLTIPS_MouseMove (wndPtr, wParam, lParam);

	case WM_NCCREATE:
	    return TOOLTIPS_NcCreate (wndPtr, wParam, lParam);

//	case WM_NCHITTEST:
//	case WM_NOTIFYFORMAT:

	case WM_PAINT:
	    return TOOLTIPS_Paint (wndPtr, wParam, lParam);

//	case WM_PRINTCLIENT:
//	case WM_RBUTTONDOWN:

	case WM_SETFONT:
	    return TOOLTIPS_SetFont (wndPtr, wParam, lParam);

//	case WM_STYLECHANGED:

	case WM_TIMER:
	    return TOOLTIPS_Timer (wndPtr, wParam, lParam);

	case WM_WININICHANGE:
	    return TOOLTIPS_WinIniChange (wndPtr, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR (tooltips, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void
TOOLTIPS_Register (void)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (TOOLTIPS_CLASS32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS;
    wndClass.lpfnWndProc   = (WNDPROC32)ToolTipsWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TOOLTIPS_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = TOOLTIPS_CLASS32A;
 
    RegisterClass32A (&wndClass);
}
