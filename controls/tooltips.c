/*
 * Tool tip control
 *
 * Copyright 1998 Eric Kohl
 *
 * TODO:
 *   - Some messages.
 *   - All notifications.
 *
 * Testing:
 *   - Run tests using Waite Group Windows95 API Bible Volume 2.
 *     The second cdrom contains executables enumtools.exe ...
 *   - additional features.
 *
 * FIXME:
 *   - Display code.
 *   - Missing Unicode support.
 */

#include "windows.h"
#include "commctrl.h"
#include "tooltips.h"
#include "heap.h"
#include "win.h"
#include "debug.h"


#define ID_TIMER1  1
#define ID_TIMER2  2

#define TOOLTIPS_GetInfoPtr(wndPtr) ((TOOLTIPS_INFO *)wndPtr->wExtra[0])


static VOID
TOOLTIPS_Refresh (WND *wndPtr, HDC32 hdc)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    RECT32 rc;
    INT32 oldBkMode;
    HFONT32 hOldFont;

    GetClientRect32 (wndPtr->hwndSelf, &rc);
    oldBkMode = SetBkMode32 (hdc, TRANSPARENT);
    SetTextColor32 (hdc, infoPtr->clrText);
    hOldFont = SelectObject32 (hdc, infoPtr->hFont);
    DrawText32A (hdc, infoPtr->szTipText, -1, &rc, DT_EXTERNALLEADING);
    SelectObject32 (hdc, hOldFont);
    if (oldBkMode != TRANSPARENT)
	SetBkMode32 (hdc, oldBkMode);
}


static VOID
TOOLTIPS_GetTipText (WND *wndPtr, TOOLTIPS_INFO *infoPtr)
{
    TTTOOL_INFO *toolPtr = &infoPtr->tools[infoPtr->nCurrentTool];

    if (toolPtr->hinst) {
	LoadString32A (toolPtr->hinst, (UINT32)toolPtr->lpszText,
		       infoPtr->szTipText, INFOTIPSIZE);
    }
    else if (toolPtr->lpszText) {
	if (toolPtr->lpszText == LPSTR_TEXTCALLBACK32A) {
	    NMTTDISPINFOA ttnmdi;

	    /* fill NMHDR struct */
	    ttnmdi.hdr.hwndFrom = wndPtr->hwndSelf;
	    ttnmdi.hdr.idFrom = infoPtr->nCurrentTool;
	    ttnmdi.hdr.code = TTN_GETDISPINFOA;

	    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
			    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&ttnmdi);

	    /* FIXME: partial */
	    lstrcpyn32A (infoPtr->szTipText, ttnmdi.szText, INFOTIPSIZE);

	}
	else
	    lstrcpyn32A (infoPtr->szTipText, toolPtr->lpszText, INFOTIPSIZE);
    }
}


static VOID
TOOLTIPS_CalcTipSize (WND *wndPtr, TOOLTIPS_INFO *infoPtr, LPSIZE32 lpSize)
{
    HDC32 hdc;
    HFONT32 hOldFont;
    RECT32 rc;

    hdc = GetDC32 (wndPtr->hwndSelf);
    hOldFont = SelectObject32 (hdc, infoPtr->hFont);
    DrawText32A (hdc, infoPtr->szTipText, -1, &rc, DT_EXTERNALLEADING | DT_CALCRECT);
    SelectObject32 (hdc, hOldFont);
    ReleaseDC32 (hdc, wndPtr->hwndSelf);

    lpSize->cx = rc.right - rc.left + 4;
    lpSize->cy = rc.bottom - rc.top + 4;
}


static VOID
TOOLTIPS_Show (WND *wndPtr, TOOLTIPS_INFO *infoPtr)
{
    POINT32 pt;
    SIZE32 size;

    infoPtr->nCurrentTool = infoPtr->nTool;
    FIXME (tooltips, "Show tooltip %d!\n", infoPtr->nCurrentTool);

    GetCursorPos32 (&pt);

    TOOLTIPS_GetTipText (wndPtr, infoPtr);
    TRACE (tooltips, "\"%s\"\n", infoPtr->szTipText);

    TOOLTIPS_CalcTipSize (wndPtr, infoPtr, &size);

    SetWindowPos32 (wndPtr->hwndSelf, HWND_TOP, pt.x, pt.y + 20,
		    size.cx, size.cy, SWP_SHOWWINDOW);

    SetTimer32 (wndPtr->hwndSelf, ID_TIMER2, infoPtr->nAutoPopTime, 0);
}


static VOID
TOOLTIPS_Hide (WND *wndPtr, TOOLTIPS_INFO *infoPtr)
{
    if (infoPtr->nCurrentTool == -1)
	return;

    FIXME (tooltips, "Hide tooltip %d!\n", infoPtr->nCurrentTool);
    KillTimer32 (wndPtr->hwndSelf, ID_TIMER2);
    infoPtr->nCurrentTool = -1;
    SetWindowPos32 (wndPtr->hwndSelf, HWND_TOP, 0, 0, 0, 0, SWP_HIDEWINDOW);
}


static INT32
TOOLTIPS_GetIndexFromInfoA (TOOLTIPS_INFO *infoPtr, LPTTTOOLINFOA lpToolInfo)
{
    TTTOOL_INFO *toolPtr;
    INT32 nIndex;

    for (nIndex = 0; nIndex < infoPtr->uNumTools; nIndex++) {
	toolPtr = &infoPtr->tools[nIndex];

	if ((lpToolInfo->hwnd == toolPtr->hwnd) &&
	    (lpToolInfo->uId == toolPtr->uId))
	    return nIndex;
    }

    return -1;
}


static INT32
TOOLTIPS_GetIndexFromPoint (TOOLTIPS_INFO *infoPtr, HWND32 hwnd, LPPOINT32 lpPt)
{
    TTTOOL_INFO *toolPtr;
    INT32  nIndex;

    for (nIndex = 0; nIndex < infoPtr->uNumTools; nIndex++) {
	toolPtr = &infoPtr->tools[nIndex];

	if (toolPtr->uFlags & TTF_IDISHWND) {
	    if ((HWND32)toolPtr->uId == hwnd)
		return nIndex;
	}
	else {
	    if (hwnd != toolPtr->hwnd)
		continue;
	    if (!PtInRect32 (&toolPtr->rect, *lpPt))
		continue;
	    return nIndex;
	}
    }

    return -1;
}


static LRESULT
TOOLTIPS_Activate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    infoPtr->bActive = (BOOL32)wParam;

    if (infoPtr->bActive) {
	FIXME (tooltips, "activated!\n");
    }
    else {
	FIXME (tooltips, "deactivated!\n");
    }

    return 0;
}


static LRESULT
TOOLTIPS_AddTool32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo == NULL) return FALSE;

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
	toolPtr->lpszText = lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACK32A)
	    toolPtr->lpszText = lpToolInfo->lpszText;
	else {
	    INT32 len = lstrlen32A (lpToolInfo->lpszText);
	    toolPtr->lpszText =
		HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, len + 1);
	    lstrcpy32A (toolPtr->lpszText, lpToolInfo->lpszText);
	}
    }

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOA))
	toolPtr->lParam = lpToolInfo->lParam;

    return TRUE;
}


// << TOOLTIPS_AddTool32W >>


static LRESULT
TOOLTIPS_DelTool32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nIndex;

    if (lpToolInfo == NULL) return 0;
    if (infoPtr->uNumTools == 0) return 0;

    nIndex = TOOLTIPS_GetIndexFromInfoA (infoPtr, lpToolInfo);
    if (nIndex == -1) return 0;

    TRACE (tooltips, "index=%d\n", nIndex);

    /* delete text string */
    toolPtr = &infoPtr->tools[nIndex]; 
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

	if (nIndex > 0)
	    memcpy (&infoPtr->tools[0], &oldTools[0],
		    nIndex * sizeof(TTTOOL_INFO));

	if (nIndex < infoPtr->uNumTools - 1)
	    memcpy (&infoPtr->tools[nIndex], &oldTools[nIndex + 1],
		    (infoPtr->uNumTools - nIndex - 1) * sizeof(TTTOOL_INFO));

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
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
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

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOA))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


// << TOOLTIPS_EnumTools32W >>


static LRESULT
TOOLTIPS_GetCurrentTool32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOA lpti = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpti) {
	if (infoPtr->nCurrentTool > -1) {
	    toolPtr = &infoPtr->tools[infoPtr->nCurrentTool];

	    /* copy tool data */
	    lpti->uFlags   = toolPtr->uFlags;
	    lpti->rect     = toolPtr->rect;
	    lpti->hinst    = toolPtr->hinst;
	    lpti->lpszText = toolPtr->lpszText;

	    if (lpti->cbSize >= sizeof(TTTOOLINFOA))
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
    LPTTTOOLINFOA lpti = (LPTTTOOLINFOA)lParam;
    INT32 nIndex;

    if (!(lpti)) return 0;
    if (lpti->cbSize < sizeof(TTTOOLINFOA)) return 0;

    nIndex = TOOLTIPS_GetIndexFromInfoA (infoPtr, lpti);
    if (nIndex == -1) return 0;

    lstrcpy32A (lpti->lpszText, infoPtr->tools[nIndex].lpszText);

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
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nIndex;

    if (lpToolInfo == NULL) return FALSE;
    if (infoPtr->uNumTools == 0) return FALSE;

    nIndex = TOOLTIPS_GetIndexFromInfoA (infoPtr, lpToolInfo);
    if (nIndex == -1) return FALSE;

    TRACE (tooltips, "index=%d\n", nIndex);

    toolPtr = &infoPtr->tools[nIndex];

    /* copy tool data */
    lpToolInfo->uFlags   = toolPtr->uFlags;
    lpToolInfo->rect     = toolPtr->rect;
    lpToolInfo->hinst    = toolPtr->hinst;
    lpToolInfo->lpszText = toolPtr->lpszText;

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOA))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


// << TOOLTIPS_GetToolInfo32W >>


static LRESULT
TOOLTIPS_HitTest32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTHITTESTINFOA lptthit = (LPTTHITTESTINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nTool;

    if (lptthit == 0)
	return FALSE;

    nTool = TOOLTIPS_GetIndexFromPoint (infoPtr, lptthit->hwnd, &lptthit->pt);
    if (nTool == -1)
	return FALSE;

    TRACE (tooltips, "nTool = %d!\n", nTool);

    /* copy tool data */
    toolPtr = &infoPtr->tools[nTool];
    lptthit->ti.cbSize   = sizeof(TTTOOLINFOA);
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
    LPTTTOOLINFOA lpti = (LPTTTOOLINFOA)lParam;
    INT32 nIndex;

    if (!(lpti)) return 0;
    if (lpti->cbSize < sizeof(TTTOOLINFOA)) return 0;

    nIndex = TOOLTIPS_GetIndexFromInfoA (infoPtr, lpti);
    if (nIndex == -1) return 0;

    infoPtr->tools[nIndex].rect = lpti->rect;

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
	WARN (tooltips, "lpMsg == NULL!\n");
	return 0;
    }

    pt = lpMsg->pt;
    ScreenToClient32 (lpMsg->hwnd, &pt);
    infoPtr->nOldTool = infoPtr->nTool;
    infoPtr->nTool = TOOLTIPS_GetIndexFromPoint (infoPtr, lpMsg->hwnd, &pt);
    TRACE (tooltips, "nTool = %d\n", infoPtr->nTool);

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
	    TRACE (tooltips, "WM_MOUSEMOVE (%d %d)\n", pt.x, pt.y);
	    if (infoPtr->nTool != infoPtr->nOldTool) {
		if (infoPtr->nOldTool == -1)
		    SetTimer32 (wndPtr->hwndSelf, ID_TIMER1, infoPtr->nInitialTime, 0);
		else
		    TOOLTIPS_Hide (wndPtr, infoPtr);
		    SetTimer32 (wndPtr->hwndSelf, ID_TIMER1, infoPtr->nReshowTime, 0);
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
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nIndex;

    if (lpToolInfo == NULL) return 0;

    nIndex = TOOLTIPS_GetIndexFromInfoA (infoPtr, lpToolInfo);
    if (nIndex == -1) return 0;

    TRACE (tooltips, "nIndex=%d\n", nIndex);

    toolPtr = &infoPtr->tools[nIndex];

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

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOA))
	toolPtr->lParam = lpToolInfo->lParam;

    return 0;
}


// << TOOLTIPS_SetToolInfo32W >>
// << TOOLTIPS_TrackActive >>
// << TOOLTIPS_TrackPosition >>
// << TOOLTIPS_Update >>
// << TOOLTIPS_UpdateTipText32A >>
// << TOOLTIPS_UpdateTipText32W >>
// << TOOLTIPS_WindowFromPoint >>


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
    infoPtr->clrBk   = GetSysColor32 (COLOR_INFOBK);
    infoPtr->clrText = GetSysColor32 (COLOR_INFOTEXT);

    SystemParametersInfo32A( SPI_GETICONTITLELOGFONT, 0, &logFont, 0 );
    infoPtr->hFont = CreateFontIndirect32A( &logFont );

    infoPtr->nMaxTipWidth = -1;
    infoPtr->nTool = -1;
    infoPtr->nOldTool = -1;
    infoPtr->nCurrentTool = -1;

    infoPtr->nAutomaticTime = 500;
    infoPtr->nReshowTime    = 100;
    infoPtr->nAutoPopTime   = 5000;
    infoPtr->nInitialTime   = 500;

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

    if (LOWORD(lParam)) {
	/* redraw */

    }

    return 0;
}


static LRESULT
TOOLTIPS_Timer (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    switch (wParam)
    {
	case ID_TIMER1:
	    KillTimer32 (wndPtr->hwndSelf, ID_TIMER1);
	    TOOLTIPS_Show (wndPtr, infoPtr);
	    break;

	case ID_TIMER2:
	    TOOLTIPS_Hide (wndPtr, infoPtr);
	    break;
    }
    return 0;
}


// << TOOLTIPS_WinIniChange >>


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
//	case TTM_TRACKACTIVE:			/* 4.70 */
//	case TTM_TRACKPOSITION:			/* 4.70 */
//	case TTM_UPDATE:			/* 4.71 */
//	case TTM_UPDATETIPTEXT32A:
//	case TTM_UPDATETIPTEXT32W:
//	case TTM_WINDOWFROMPOINT:


	case WM_CREATE:
	    return TOOLTIPS_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return TOOLTIPS_Destroy (wndPtr, wParam, lParam);

	case WM_ERASEBKGND:
	    return TOOLTIPS_EraseBackground (wndPtr, wParam, lParam);

	case WM_GETFONT:
	    return TOOLTIPS_GetFont (wndPtr, wParam, lParam);

	case WM_MOUSEMOVE:
	    return TOOLTIPS_MouseMove (wndPtr, wParam, lParam);

	case WM_NCCREATE:
	    return TOOLTIPS_NcCreate (wndPtr, wParam, lParam);

	case WM_PAINT:
	    return TOOLTIPS_Paint (wndPtr, wParam, lParam);

	case WM_SETFONT:
	    return TOOLTIPS_SetFont (wndPtr, wParam, lParam);

	case WM_TIMER:
	    return TOOLTIPS_Timer (wndPtr, wParam, lParam);

//	case WM_WININICHANGE:

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

