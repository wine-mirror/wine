/*
 * Tool tip control
 *
 * Copyright 1998 Eric Kohl
 *
 * TODO:
 *   - Unicode support.
 *   - Custom draw support.
 *
 * Testing:
 *   - Run tests using Waite Group Windows95 API Bible Volume 2.
 *     The second cdrom (chapter 3) contains executables activate.exe,
 *     curtool.exe, deltool.exe, enumtools.exe, getinfo.exe, getiptxt.exe,
 *     hittest.exe, needtext.exe, newrect.exe, updtext.exe and winfrpt.exe.
 */

#include "windows.h"
#include "commctrl.h"
#include "tooltips.h"
#include "win.h"
#include "debug.h"

#define ID_TIMERSHOW   1    /* show delay timer */
#define ID_TIMERPOP    2    /* auto pop timer */
#define ID_TIMERLEAVE  3    /* tool leave timer */


extern LPSTR COMCTL32_aSubclass; /* global subclassing atom */

/* property name of tooltip window handle */
/*#define TT_SUBCLASS_PROP "CC32SubclassInfo" */

#define TOOLTIPS_GetInfoPtr(wndPtr) ((TOOLTIPS_INFO *)wndPtr->wExtra[0])


LRESULT CALLBACK
TOOLTIPS_SubclassProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam);


static VOID
TOOLTIPS_Refresh (WND *wndPtr, HDC32 hdc)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    RECT32 rc;
    INT32 oldBkMode;
    HFONT32 hOldFont;
    HBRUSH32 hBrush;
    UINT32 uFlags = DT_EXTERNALLEADING;

    if (infoPtr->nMaxTipWidth > -1)
	uFlags |= DT_WORDBREAK;
    if (wndPtr->dwStyle & TTS_NOPREFIX)
	uFlags |= DT_NOPREFIX;
    GetClientRect32 (wndPtr->hwndSelf, &rc);

    /* fill the background */
    hBrush = CreateSolidBrush32 (infoPtr->clrBk);
    FillRect32 (hdc, &rc, hBrush);
    DeleteObject32 (hBrush);

    /* calculate text rectangle */
    rc.left   += (2 + infoPtr->rcMargin.left);
    rc.top    += (2 + infoPtr->rcMargin.top);
    rc.right  -= (2 + infoPtr->rcMargin.right);
    rc.bottom -= (2 + infoPtr->rcMargin.bottom);

    /* draw text */
    oldBkMode = SetBkMode32 (hdc, TRANSPARENT);
    SetTextColor32 (hdc, infoPtr->clrText);
    hOldFont = SelectObject32 (hdc, infoPtr->hFont);
    DrawText32W (hdc, infoPtr->szTipText, -1, &rc, uFlags);
    SelectObject32 (hdc, hOldFont);
    if (oldBkMode != TRANSPARENT)
	SetBkMode32 (hdc, oldBkMode);
}


static VOID
TOOLTIPS_GetTipText (WND *wndPtr, TOOLTIPS_INFO *infoPtr, INT32 nTool)
{
    TTTOOL_INFO *toolPtr = &infoPtr->tools[nTool];

    if ((toolPtr->hinst) && (HIWORD((UINT32)toolPtr->lpszText) == 0)) {
	/* load a resource */
	TRACE (tooltips, "load res string %x %x\n",
	       toolPtr->hinst, (int)toolPtr->lpszText);
	LoadString32W (toolPtr->hinst, (UINT32)toolPtr->lpszText,
		       infoPtr->szTipText, INFOTIPSIZE);
    }
    else if (toolPtr->lpszText) {
	if (toolPtr->lpszText == LPSTR_TEXTCALLBACK32W) {
	    NMTTDISPINFO32A ttnmdi;

	    /* fill NMHDR struct */
	    ZeroMemory (&ttnmdi, sizeof(NMTTDISPINFO32A));
	    ttnmdi.hdr.hwndFrom = wndPtr->hwndSelf;
	    ttnmdi.hdr.idFrom = toolPtr->uId;
	    ttnmdi.hdr.code = TTN_GETDISPINFO32A;
	    ttnmdi.lpszText = (LPSTR)&ttnmdi.szText;
	    ttnmdi.uFlags = toolPtr->uFlags;
	    ttnmdi.lParam = toolPtr->lParam;

	    TRACE (tooltips, "hdr.idFrom = %x\n", ttnmdi.hdr.idFrom);
	    SendMessage32A (toolPtr->hwnd, WM_NOTIFY,
			    (WPARAM32)toolPtr->uId, (LPARAM)&ttnmdi);

	    if ((ttnmdi.hinst) && (HIWORD((UINT32)ttnmdi.szText) == 0)) {
		LoadString32W (ttnmdi.hinst, (UINT32)ttnmdi.szText,
			       infoPtr->szTipText, INFOTIPSIZE);
		if (ttnmdi.uFlags & TTF_DI_SETITEM) {
		    toolPtr->hinst = ttnmdi.hinst;
		    toolPtr->lpszText = (LPWSTR)ttnmdi.szText;
		}
	    }
	    else if (ttnmdi.szText[0]) {
		lstrcpynAtoW (infoPtr->szTipText, ttnmdi.szText, 80);
		if (ttnmdi.uFlags & TTF_DI_SETITEM) {
		    INT32 len = lstrlen32A (ttnmdi.szText);
		    toolPtr->hinst = 0;
		    toolPtr->lpszText =	COMCTL32_Alloc ((len+1)* sizeof(WCHAR));
		    lstrcpyAtoW (toolPtr->lpszText, ttnmdi.szText);
		}
	    }
	    else if (ttnmdi.lpszText == 0) {
		/* no text available */
		infoPtr->szTipText[0] = L'\0';
	    }
	    else if (ttnmdi.lpszText != LPSTR_TEXTCALLBACK32A) {
		lstrcpynAtoW (infoPtr->szTipText, ttnmdi.lpszText, INFOTIPSIZE);
		if (ttnmdi.uFlags & TTF_DI_SETITEM) {
		    INT32 len = lstrlen32A (ttnmdi.lpszText);
		    toolPtr->hinst = 0;
		    toolPtr->lpszText =	COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
		    lstrcpyAtoW (toolPtr->lpszText, ttnmdi.lpszText);
		}
	    }
	    else {
		ERR (tooltips, "recursive text callback!\n");
		infoPtr->szTipText[0] = '\0';
	    }
	}
	else {
	    /* the item is a usual (unicode) text */
	    lstrcpyn32W (infoPtr->szTipText, toolPtr->lpszText, INFOTIPSIZE);
	}
    }
    else {
	/* no text available */
	infoPtr->szTipText[0] = L'\0';
    }

    TRACE (tooltips, "\"%s\"\n", debugstr_w(infoPtr->szTipText));
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
    TRACE (tooltips, "\"%s\"\n", debugstr_w(infoPtr->szTipText));

    hdc = GetDC32 (wndPtr->hwndSelf);
    hOldFont = SelectObject32 (hdc, infoPtr->hFont);
    DrawText32W (hdc, infoPtr->szTipText, -1, &rc, uFlags);
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
    RECT32 rect;
    SIZE32 size;
    HDC32  hdc;
    NMHDR  hdr;

    if (infoPtr->nTool == -1) {
	TRACE (tooltips, "invalid tool (-1)!\n");
	return;
    }

    infoPtr->nCurrentTool = infoPtr->nTool;

    TRACE (tooltips, "Show tooltip pre %d!\n", infoPtr->nTool);

    TOOLTIPS_GetTipText (wndPtr, infoPtr, infoPtr->nCurrentTool);

    if (infoPtr->szTipText[0] == L'\0') {
	infoPtr->nCurrentTool = -1;
	return;
    }

    TRACE (tooltips, "Show tooltip %d!\n", infoPtr->nCurrentTool);
    toolPtr = &infoPtr->tools[infoPtr->nCurrentTool];

    hdr.hwndFrom = wndPtr->hwndSelf;
    hdr.idFrom = toolPtr->uId;
    hdr.code = TTN_SHOW;
    SendMessage32A (toolPtr->hwnd, WM_NOTIFY,
		    (WPARAM32)toolPtr->uId, (LPARAM)&hdr);

    TRACE (tooltips, "\"%s\"\n", debugstr_w(infoPtr->szTipText));

    TOOLTIPS_CalcTipSize (wndPtr, infoPtr, &size);
    TRACE (tooltips, "size %d - %d\n", size.cx, size.cy);

    if (toolPtr->uFlags & TTF_CENTERTIP) {
	RECT32 rc;

	if (toolPtr->uFlags & TTF_IDISHWND)
	    GetWindowRect32 ((HWND32)toolPtr->uId, &rc);
	else {
	    rc = toolPtr->rect;
	    MapWindowPoints32 (toolPtr->hwnd, (HWND32)0, (LPPOINT32)&rc, 2);
	}
	rect.left = (rc.left + rc.right - size.cx) / 2;
	rect.top  = rc.bottom + 2;
    }
    else {
	GetCursorPos32 ((LPPOINT32)&rect);
	rect.top += 20;
    }

    /* FIXME: check position */

    TRACE (tooltips, "pos %d - %d\n", rect.left, rect.top);

    rect.right = rect.left + size.cx;
    rect.bottom = rect.top + size.cy;

    AdjustWindowRectEx32 (&rect, wndPtr->dwStyle, FALSE, wndPtr->dwExStyle);

    SetWindowPos32 (wndPtr->hwndSelf, HWND_TOP, rect.left, rect.top,
		    rect.right - rect.left, rect.bottom - rect.top,
		    SWP_SHOWWINDOW);

    /* repaint the tooltip */
    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLTIPS_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    SetTimer32 (wndPtr->hwndSelf, ID_TIMERPOP, infoPtr->nAutoPopTime, 0);
}


static VOID
TOOLTIPS_Hide (WND *wndPtr, TOOLTIPS_INFO *infoPtr)
{
    TTTOOL_INFO *toolPtr;
    NMHDR hdr;

    if (infoPtr->nCurrentTool == -1)
	return;

    toolPtr = &infoPtr->tools[infoPtr->nCurrentTool];
    TRACE (tooltips, "Hide tooltip %d!\n", infoPtr->nCurrentTool);
    KillTimer32 (wndPtr->hwndSelf, ID_TIMERPOP);

    hdr.hwndFrom = wndPtr->hwndSelf;
    hdr.idFrom = toolPtr->uId;
    hdr.code = TTN_POP;
    SendMessage32A (toolPtr->hwnd, WM_NOTIFY,
		    (WPARAM32)toolPtr->uId, (LPARAM)&hdr);

    infoPtr->nCurrentTool = -1;

    SetWindowPos32 (wndPtr->hwndSelf, HWND_TOP, 0, 0, 0, 0,
		    SWP_NOZORDER | SWP_HIDEWINDOW);
}


static VOID
TOOLTIPS_TrackShow (WND *wndPtr, TOOLTIPS_INFO *infoPtr)
{
    TTTOOL_INFO *toolPtr;
    RECT32 rect;
    SIZE32 size;
    HDC32  hdc;
    NMHDR hdr;

    if (infoPtr->nTrackTool == -1) {
	TRACE (tooltips, "invalid tracking tool (-1)!\n");
	return;
    }

    TRACE (tooltips, "show tracking tooltip pre %d!\n", infoPtr->nTrackTool);

    TOOLTIPS_GetTipText (wndPtr, infoPtr, infoPtr->nTrackTool);

    if (infoPtr->szTipText[0] == L'\0') {
	infoPtr->nTrackTool = -1;
	return;
    }

    TRACE (tooltips, "show tracking tooltip %d!\n", infoPtr->nTrackTool);
    toolPtr = &infoPtr->tools[infoPtr->nTrackTool];

    hdr.hwndFrom = wndPtr->hwndSelf;
    hdr.idFrom = toolPtr->uId;
    hdr.code = TTN_SHOW;
    SendMessage32A (toolPtr->hwnd, WM_NOTIFY,
		    (WPARAM32)toolPtr->uId, (LPARAM)&hdr);

    TRACE (tooltips, "\"%s\"\n", debugstr_w(infoPtr->szTipText));

    TOOLTIPS_CalcTipSize (wndPtr, infoPtr, &size);
    TRACE (tooltips, "size %d - %d\n", size.cx, size.cy);

    if (toolPtr->uFlags & TTF_ABSOLUTE) {
	rect.left = infoPtr->xTrackPos;
	rect.top  = infoPtr->yTrackPos;

	if (toolPtr->uFlags & TTF_CENTERTIP) {
	    rect.left -= (size.cx / 2);
	    rect.top  -= (size.cy / 2);
	}
    }
    else {
	RECT32 rcTool;

	if (toolPtr->uFlags & TTF_IDISHWND)
	    GetWindowRect32 ((HWND32)toolPtr->uId, &rcTool);
	else {
	    rcTool = toolPtr->rect;
	    MapWindowPoints32 (toolPtr->hwnd, (HWND32)0, (LPPOINT32)&rcTool, 2);
	}

	GetCursorPos32 ((LPPOINT32)&rect);
	rect.top += 20;

	if (toolPtr->uFlags & TTF_CENTERTIP) {
	    rect.left -= (size.cx / 2);
	    rect.top  -= (size.cy / 2);
	}

	/* smart placement */
	if ((rect.left + size.cx > rcTool.left) && (rect.left < rcTool.right) &&
	    (rect.top + size.cy > rcTool.top) && (rect.top < rcTool.bottom))
	    rect.left = rcTool.right;
    }

    TRACE (tooltips, "pos %d - %d\n", rect.left, rect.top);

    rect.right = rect.left + size.cx;
    rect.bottom = rect.top + size.cy;

    AdjustWindowRectEx32 (&rect, wndPtr->dwStyle, FALSE, wndPtr->dwExStyle);

    SetWindowPos32 (wndPtr->hwndSelf, HWND_TOP, rect.left, rect.top,
		    rect.right - rect.left, rect.bottom - rect.top,
		    SWP_SHOWWINDOW);

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLTIPS_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);
}


static VOID
TOOLTIPS_TrackHide (WND *wndPtr, TOOLTIPS_INFO *infoPtr)
{
    TTTOOL_INFO *toolPtr;
    NMHDR hdr;

    if (infoPtr->nTrackTool == -1)
	return;

    toolPtr = &infoPtr->tools[infoPtr->nTrackTool];
    TRACE (tooltips, "hide tracking tooltip %d!\n", infoPtr->nTrackTool);

    hdr.hwndFrom = wndPtr->hwndSelf;
    hdr.idFrom = toolPtr->uId;
    hdr.code = TTN_POP;
    SendMessage32A (toolPtr->hwnd, WM_NOTIFY,
		    (WPARAM32)toolPtr->uId, (LPARAM)&hdr);

    SetWindowPos32 (wndPtr->hwndSelf, HWND_TOP, 0, 0, 0, 0,
		    SWP_NOZORDER | SWP_HIDEWINDOW);
}


static INT32
TOOLTIPS_GetToolFromInfoA (TOOLTIPS_INFO *infoPtr, LPTTTOOLINFO32A lpToolInfo)
{
    TTTOOL_INFO *toolPtr;
    INT32 nTool;

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
TOOLTIPS_GetToolFromInfoW (TOOLTIPS_INFO *infoPtr, LPTTTOOLINFO32W lpToolInfo)
{
    TTTOOL_INFO *toolPtr;
    INT32 nTool;

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

	if (!(toolPtr->uFlags & TTF_IDISHWND)) {
	    if (hwnd != toolPtr->hwnd)
		continue;
	    if (!PtInRect32 (&toolPtr->rect, *lpPt))
		continue;
	    return nTool;
	}
    }

    for (nTool = 0; nTool < infoPtr->uNumTools; nTool++) {
	toolPtr = &infoPtr->tools[nTool];

	if (toolPtr->uFlags & TTF_IDISHWND) {
	    if ((HWND32)toolPtr->uId == hwnd)
		return nTool;
	}
    }

    return -1;
}


static INT32
TOOLTIPS_GetToolFromMessage (TOOLTIPS_INFO *infoPtr, HWND32 hwndTool)
{
    DWORD   dwPos;
    POINT32 pt;

    dwPos = GetMessagePos ();
    pt.x = (INT32)LOWORD(dwPos);
    pt.y = (INT32)HIWORD(dwPos);
    ScreenToClient32 (hwndTool, &pt);

    return TOOLTIPS_GetToolFromPoint (infoPtr, hwndTool, &pt);
}


static BOOL32
TOOLTIPS_IsWindowActive (HWND32 hwnd)
{
    HWND32 hwndActive = GetActiveWindow32 ();
    if (!hwndActive)
	return FALSE;
    if (hwndActive == hwnd)
	return TRUE;
    return IsChild32 (hwndActive, hwnd);
}


static INT32
TOOLTIPS_CheckTool (WND *wndPtr, BOOL32 bShowTest)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    POINT32 pt;
    HWND32 hwndTool;
    INT32 nTool;

    GetCursorPos32 (&pt);
    hwndTool =
	SendMessage32A (wndPtr->hwndSelf, TTM_WINDOWFROMPOINT, 0, (LPARAM)&pt);
    if (hwndTool == 0)
	return -1;

    ScreenToClient32 (hwndTool, &pt);
    nTool = TOOLTIPS_GetToolFromPoint (infoPtr, hwndTool, &pt);
    if (nTool == -1)
	return -1;

    if (!(wndPtr->dwStyle & TTS_ALWAYSTIP) && bShowTest) {
	if (!TOOLTIPS_IsWindowActive (wndPtr->owner->hwndSelf))
	    return -1;
    }

    TRACE (tooltips, "tool %d\n", nTool);

    return nTool;
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

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32A)
	return FALSE;

    TRACE (tooltips, "add tool (%x) %x %d%s!\n",
	   wndPtr->hwndSelf, lpToolInfo->hwnd, lpToolInfo->uId,
	   (lpToolInfo->uFlags & TTF_IDISHWND) ? " TTF_IDISHWND" : "");

    if (infoPtr->uNumTools == 0) {
	infoPtr->tools = COMCTL32_Alloc (sizeof(TTTOOL_INFO));
	toolPtr = infoPtr->tools;
    }
    else {
	TTTOOL_INFO *oldTools = infoPtr->tools;
	infoPtr->tools =
	    COMCTL32_Alloc (sizeof(TTTOOL_INFO) * (infoPtr->uNumTools + 1));
	memcpy (infoPtr->tools, oldTools,
		infoPtr->uNumTools * sizeof(TTTOOL_INFO));
	COMCTL32_Free (oldTools);
	toolPtr = &infoPtr->tools[infoPtr->uNumTools];
    }

    infoPtr->uNumTools++;

    /* copy tool data */
    toolPtr->uFlags = lpToolInfo->uFlags;
    toolPtr->hwnd   = lpToolInfo->hwnd;
    toolPtr->uId    = lpToolInfo->uId;
    toolPtr->rect   = lpToolInfo->rect;
    toolPtr->hinst  = lpToolInfo->hinst;

    if ((lpToolInfo->hinst) && (HIWORD((INT32)lpToolInfo->lpszText) == 0)) {
	TRACE (tooltips, "add string id %x!\n", (int)lpToolInfo->lpszText);
	toolPtr->lpszText = (LPWSTR)lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACK32A) {
	    TRACE (tooltips, "add CALLBACK!\n");
	    toolPtr->lpszText = LPSTR_TEXTCALLBACK32W;
	}
	else {
	    INT32 len = lstrlen32A (lpToolInfo->lpszText);
	    TRACE (tooltips, "add text \"%s\"!\n", lpToolInfo->lpszText);
	    toolPtr->lpszText =	COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    lstrcpyAtoW (toolPtr->lpszText, lpToolInfo->lpszText);
	}
    }

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFO32A))
	toolPtr->lParam = lpToolInfo->lParam;

    /* install subclassing hook */
    if (toolPtr->uFlags & TTF_SUBCLASS) {
	if (toolPtr->uFlags & TTF_IDISHWND) {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetProp32A ((HWND32)toolPtr->uId, COMCTL32_aSubclass);
	    if (lpttsi == NULL) {
		lpttsi =
		    (LPTT_SUBCLASS_INFO)COMCTL32_Alloc (sizeof(TT_SUBCLASS_INFO));
		lpttsi->wpOrigProc = 
		    (WNDPROC32)SetWindowLong32A ((HWND32)toolPtr->uId,
		    GWL_WNDPROC,(LONG)TOOLTIPS_SubclassProc);
		lpttsi->hwndToolTip = wndPtr->hwndSelf;
		lpttsi->uRefCount++;
		SetProp32A ((HWND32)toolPtr->uId, COMCTL32_aSubclass,
			    (HANDLE32)lpttsi);
	    }
	    else
		WARN (tooltips, "A window tool must only be listed once!\n");
	}
	else {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetProp32A (toolPtr->hwnd, COMCTL32_aSubclass);
	    if (lpttsi == NULL) {
		lpttsi =
		    (LPTT_SUBCLASS_INFO)COMCTL32_Alloc (sizeof(TT_SUBCLASS_INFO));
		lpttsi->wpOrigProc = 
		    (WNDPROC32)SetWindowLong32A (toolPtr->hwnd,
		    GWL_WNDPROC,(LONG)TOOLTIPS_SubclassProc);
		lpttsi->hwndToolTip = wndPtr->hwndSelf;
		lpttsi->uRefCount++;
		SetProp32A (toolPtr->hwnd, COMCTL32_aSubclass, (HANDLE32)lpttsi);
	    }
	    else
		lpttsi->uRefCount++;
	}
	TRACE (tooltips, "subclassing installed!\n");
    }

    return TRUE;
}


static LRESULT
TOOLTIPS_AddTool32W (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32W lpToolInfo = (LPTTTOOLINFO32W)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32W)
	return FALSE;

    TRACE (tooltips, "add tool (%x) %x %d%s!\n",
	   wndPtr->hwndSelf, lpToolInfo->hwnd, lpToolInfo->uId,
	   (lpToolInfo->uFlags & TTF_IDISHWND) ? " TTF_IDISHWND" : "");

    if (infoPtr->uNumTools == 0) {
	infoPtr->tools = COMCTL32_Alloc (sizeof(TTTOOL_INFO));
	toolPtr = infoPtr->tools;
    }
    else {
	TTTOOL_INFO *oldTools = infoPtr->tools;
	infoPtr->tools =
	    COMCTL32_Alloc (sizeof(TTTOOL_INFO) * (infoPtr->uNumTools + 1));
	memcpy (infoPtr->tools, oldTools,
		infoPtr->uNumTools * sizeof(TTTOOL_INFO));
	COMCTL32_Free (oldTools);
	toolPtr = &infoPtr->tools[infoPtr->uNumTools];
    }

    infoPtr->uNumTools++;

    /* copy tool data */
    toolPtr->uFlags = lpToolInfo->uFlags;
    toolPtr->hwnd   = lpToolInfo->hwnd;
    toolPtr->uId    = lpToolInfo->uId;
    toolPtr->rect   = lpToolInfo->rect;
    toolPtr->hinst  = lpToolInfo->hinst;

    if ((lpToolInfo->hinst) && (HIWORD((INT32)lpToolInfo->lpszText) == 0)) {
	TRACE (tooltips, "add string id %x!\n", (int)lpToolInfo->lpszText);
	toolPtr->lpszText = (LPWSTR)lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACK32W) {
	    TRACE (tooltips, "add CALLBACK!\n");
	    toolPtr->lpszText = LPSTR_TEXTCALLBACK32W;
	}
	else {
	    INT32 len = lstrlen32W (lpToolInfo->lpszText);
	    TRACE (tooltips, "add text \"%s\"!\n",
		   debugstr_w(lpToolInfo->lpszText));
	    toolPtr->lpszText =	COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    lstrcpy32W (toolPtr->lpszText, lpToolInfo->lpszText);
	}
    }

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFO32W))
	toolPtr->lParam = lpToolInfo->lParam;

    /* install subclassing hook */
    if (toolPtr->uFlags & TTF_SUBCLASS) {
	if (toolPtr->uFlags & TTF_IDISHWND) {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetProp32A ((HWND32)toolPtr->uId, COMCTL32_aSubclass);
	    if (lpttsi == NULL) {
		lpttsi =
		    (LPTT_SUBCLASS_INFO)COMCTL32_Alloc (sizeof(TT_SUBCLASS_INFO));
		lpttsi->wpOrigProc = 
		    (WNDPROC32)SetWindowLong32A ((HWND32)toolPtr->uId,
		    GWL_WNDPROC,(LONG)TOOLTIPS_SubclassProc);
		lpttsi->hwndToolTip = wndPtr->hwndSelf;
		lpttsi->uRefCount++;
		SetProp32A ((HWND32)toolPtr->uId, COMCTL32_aSubclass,
			    (HANDLE32)lpttsi);
	    }
	    else
		WARN (tooltips, "A window tool must only be listed once!\n");
	}
	else {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetProp32A (toolPtr->hwnd, COMCTL32_aSubclass);
	    if (lpttsi == NULL) {
		lpttsi =
		    (LPTT_SUBCLASS_INFO)COMCTL32_Alloc (sizeof(TT_SUBCLASS_INFO));
		lpttsi->wpOrigProc = 
		    (WNDPROC32)SetWindowLong32A (toolPtr->hwnd,
		    GWL_WNDPROC,(LONG)TOOLTIPS_SubclassProc);
		lpttsi->hwndToolTip = wndPtr->hwndSelf;
		lpttsi->uRefCount++;
		SetProp32A (toolPtr->hwnd, COMCTL32_aSubclass, (HANDLE32)lpttsi);
	    }
	    else
		lpttsi->uRefCount++;
	}
	TRACE (tooltips, "subclassing installed!\n");
    }

    return TRUE;
}


static LRESULT
TOOLTIPS_DelTool32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32A lpToolInfo = (LPTTTOOLINFO32A)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32A)
	return 0;
    if (infoPtr->uNumTools == 0)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    TRACE (tooltips, "tool %d\n", nTool);

    /* delete text string */
    toolPtr = &infoPtr->tools[nTool]; 
    if ((toolPtr->hinst) && (toolPtr->lpszText)) {
	if (toolPtr->lpszText != LPSTR_TEXTCALLBACK32W)
	    COMCTL32_Free (toolPtr->lpszText);
    }

    /* remove subclassing */
    if (toolPtr->uFlags & TTF_SUBCLASS) {
	if (toolPtr->uFlags & TTF_IDISHWND) {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetProp32A ((HWND32)toolPtr->uId, COMCTL32_aSubclass);
	    if (lpttsi) {
		SetWindowLong32A ((HWND32)toolPtr->uId, GWL_WNDPROC,
				  (LONG)lpttsi->wpOrigProc);
		RemoveProp32A ((HWND32)toolPtr->uId, COMCTL32_aSubclass);
		COMCTL32_Free (&lpttsi);
	    }
	    else
		ERR (tooltips, "Invalid data handle!\n");
	}
	else {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetProp32A (toolPtr->hwnd, COMCTL32_aSubclass);
	    if (lpttsi) {
		if (lpttsi->uRefCount == 1) {
		    SetWindowLong32A ((HWND32)toolPtr->uId, GWL_WNDPROC,
				      (LONG)lpttsi->wpOrigProc);
		    RemoveProp32A ((HWND32)toolPtr->uId, COMCTL32_aSubclass);
		    COMCTL32_Free (&lpttsi);
		}
		else
		    lpttsi->uRefCount--;
	    }
	    else
		ERR (tooltips, "Invalid data handle!\n");
	}
    }

    /* delete tool from tool list */
    if (infoPtr->uNumTools == 1) {
	COMCTL32_Free (infoPtr->tools);
	infoPtr->tools = NULL;
    }
    else {
	TTTOOL_INFO *oldTools = infoPtr->tools;
	infoPtr->tools =
	    COMCTL32_Alloc (sizeof(TTTOOL_INFO) * (infoPtr->uNumTools - 1));

	if (nTool > 0)
	    memcpy (&infoPtr->tools[0], &oldTools[0],
		    nTool * sizeof(TTTOOL_INFO));

	if (nTool < infoPtr->uNumTools - 1)
	    memcpy (&infoPtr->tools[nTool], &oldTools[nTool + 1],
		    (infoPtr->uNumTools - nTool - 1) * sizeof(TTTOOL_INFO));

	COMCTL32_Free (oldTools);
    }

    infoPtr->uNumTools--;

    return 0;
}


static LRESULT
TOOLTIPS_DelTool32W (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32W lpToolInfo = (LPTTTOOLINFO32W)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32W)
	return 0;
    if (infoPtr->uNumTools == 0)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    TRACE (tooltips, "tool %d\n", nTool);

    /* delete text string */
    toolPtr = &infoPtr->tools[nTool]; 
    if ((toolPtr->hinst) && (toolPtr->lpszText)) {
	if (toolPtr->lpszText != LPSTR_TEXTCALLBACK32W)
	    COMCTL32_Free (toolPtr->lpszText);
    }

    /* remove subclassing */
    if (toolPtr->uFlags & TTF_SUBCLASS) {
	if (toolPtr->uFlags & TTF_IDISHWND) {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetProp32A ((HWND32)toolPtr->uId, COMCTL32_aSubclass);
	    if (lpttsi) {
		SetWindowLong32A ((HWND32)toolPtr->uId, GWL_WNDPROC,
				  (LONG)lpttsi->wpOrigProc);
		RemoveProp32A ((HWND32)toolPtr->uId, COMCTL32_aSubclass);
		COMCTL32_Free (&lpttsi);
	    }
	    else
		ERR (tooltips, "Invalid data handle!\n");
	}
	else {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetProp32A (toolPtr->hwnd, COMCTL32_aSubclass);
	    if (lpttsi) {
		if (lpttsi->uRefCount == 1) {
		    SetWindowLong32A ((HWND32)toolPtr->uId, GWL_WNDPROC,
				      (LONG)lpttsi->wpOrigProc);
		    RemoveProp32A ((HWND32)toolPtr->uId, COMCTL32_aSubclass);
		    COMCTL32_Free (&lpttsi);
		}
		else
		    lpttsi->uRefCount--;
	    }
	    else
		ERR (tooltips, "Invalid data handle!\n");
	}
    }

    /* delete tool from tool list */
    if (infoPtr->uNumTools == 1) {
	COMCTL32_Free (infoPtr->tools);
	infoPtr->tools = NULL;
    }
    else {
	TTTOOL_INFO *oldTools = infoPtr->tools;
	infoPtr->tools =
	    COMCTL32_Alloc (sizeof(TTTOOL_INFO) * (infoPtr->uNumTools - 1));

	if (nTool > 0)
	    memcpy (&infoPtr->tools[0], &oldTools[0],
		    nTool * sizeof(TTTOOL_INFO));

	if (nTool < infoPtr->uNumTools - 1)
	    memcpy (&infoPtr->tools[nTool], &oldTools[nTool + 1],
		    (infoPtr->uNumTools - nTool - 1) * sizeof(TTTOOL_INFO));

	COMCTL32_Free (oldTools);
    }

    infoPtr->uNumTools--;

    return 0;
}


static LRESULT
TOOLTIPS_EnumTools32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    UINT32 uIndex = (UINT32)wParam;
    LPTTTOOLINFO32A lpToolInfo = (LPTTTOOLINFO32A)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32A)
	return FALSE;
    if (uIndex >= infoPtr->uNumTools)
	return FALSE;

    TRACE (tooltips, "index=%u\n", uIndex);

    toolPtr = &infoPtr->tools[uIndex];

    /* copy tool data */
    lpToolInfo->uFlags   = toolPtr->uFlags;
    lpToolInfo->hwnd     = toolPtr->hwnd;
    lpToolInfo->uId      = toolPtr->uId;
    lpToolInfo->rect     = toolPtr->rect;
    lpToolInfo->hinst    = toolPtr->hinst;
/*    lpToolInfo->lpszText = toolPtr->lpszText; */
    lpToolInfo->lpszText = NULL;  /* FIXME */

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFO32A))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


static LRESULT
TOOLTIPS_EnumTools32W (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    UINT32 uIndex = (UINT32)wParam;
    LPTTTOOLINFO32W lpToolInfo = (LPTTTOOLINFO32W)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32W)
	return FALSE;
    if (uIndex >= infoPtr->uNumTools)
	return FALSE;

    TRACE (tooltips, "index=%u\n", uIndex);

    toolPtr = &infoPtr->tools[uIndex];

    /* copy tool data */
    lpToolInfo->uFlags   = toolPtr->uFlags;
    lpToolInfo->hwnd     = toolPtr->hwnd;
    lpToolInfo->uId      = toolPtr->uId;
    lpToolInfo->rect     = toolPtr->rect;
    lpToolInfo->hinst    = toolPtr->hinst;
/*    lpToolInfo->lpszText = toolPtr->lpszText; */
    lpToolInfo->lpszText = NULL;  /* FIXME */

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFO32W))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


static LRESULT
TOOLTIPS_GetCurrentTool32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32A lpToolInfo = (LPTTTOOLINFO32A)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32A)
	return FALSE;

    if (lpToolInfo) {
	if (infoPtr->nCurrentTool > -1) {
	    toolPtr = &infoPtr->tools[infoPtr->nCurrentTool];

	    /* copy tool data */
	    lpToolInfo->uFlags   = toolPtr->uFlags;
	    lpToolInfo->rect     = toolPtr->rect;
	    lpToolInfo->hinst    = toolPtr->hinst;
/*	    lpToolInfo->lpszText = toolPtr->lpszText; */
	    lpToolInfo->lpszText = NULL;  /* FIXME */

	    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFO32A))
		lpToolInfo->lParam = toolPtr->lParam;

	    return TRUE;
	}
	else
	    return FALSE;
    }
    else
	return (infoPtr->nCurrentTool != -1);

    return FALSE;
}


static LRESULT
TOOLTIPS_GetCurrentTool32W (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32W lpToolInfo = (LPTTTOOLINFO32W)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32W)
	return FALSE;

    if (lpToolInfo) {
	if (infoPtr->nCurrentTool > -1) {
	    toolPtr = &infoPtr->tools[infoPtr->nCurrentTool];

	    /* copy tool data */
	    lpToolInfo->uFlags   = toolPtr->uFlags;
	    lpToolInfo->rect     = toolPtr->rect;
	    lpToolInfo->hinst    = toolPtr->hinst;
/*	    lpToolInfo->lpszText = toolPtr->lpszText; */
	    lpToolInfo->lpszText = NULL;  /* FIXME */

	    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFO32W))
		lpToolInfo->lParam = toolPtr->lParam;

	    return TRUE;
	}
	else
	    return FALSE;
    }
    else
	return (infoPtr->nCurrentTool != -1);

    return FALSE;
}


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


__inline__ static LRESULT
TOOLTIPS_GetMaxTipWidth (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    return infoPtr->nMaxTipWidth;
}


static LRESULT
TOOLTIPS_GetText32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32A lpToolInfo = (LPTTTOOLINFO32A)lParam;
    INT32 nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32A)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    lstrcpyWtoA (lpToolInfo->lpszText, infoPtr->tools[nTool].lpszText);

    return 0;
}


static LRESULT
TOOLTIPS_GetText32W (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32W lpToolInfo = (LPTTTOOLINFO32W)lParam;
    INT32 nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32W)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    lstrcpy32W (lpToolInfo->lpszText, infoPtr->tools[nTool].lpszText);

    return 0;
}


__inline__ static LRESULT
TOOLTIPS_GetTipBkColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    return infoPtr->clrBk;
}


__inline__ static LRESULT
TOOLTIPS_GetTipTextColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    return infoPtr->clrText;
}


__inline__ static LRESULT
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

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32A)
	return FALSE;
    if (infoPtr->uNumTools == 0)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
    if (nTool == -1)
	return FALSE;

    TRACE (tooltips, "tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool data */
    lpToolInfo->uFlags   = toolPtr->uFlags;
    lpToolInfo->rect     = toolPtr->rect;
    lpToolInfo->hinst    = toolPtr->hinst;
/*    lpToolInfo->lpszText = toolPtr->lpszText; */
    lpToolInfo->lpszText = NULL;  /* FIXME */

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFO32A))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


static LRESULT
TOOLTIPS_GetToolInfo32W (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32W lpToolInfo = (LPTTTOOLINFO32W)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nTool;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32W)
	return FALSE;
    if (infoPtr->uNumTools == 0)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpToolInfo);
    if (nTool == -1)
	return FALSE;

    TRACE (tooltips, "tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool data */
    lpToolInfo->uFlags   = toolPtr->uFlags;
    lpToolInfo->rect     = toolPtr->rect;
    lpToolInfo->hinst    = toolPtr->hinst;
/*    lpToolInfo->lpszText = toolPtr->lpszText; */
    lpToolInfo->lpszText = NULL;  /* FIXME */

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFO32W))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


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
    if (lptthit->ti.cbSize >= sizeof(TTTOOLINFO32A)) {
	toolPtr = &infoPtr->tools[nTool];

	lptthit->ti.uFlags   = toolPtr->uFlags;
	lptthit->ti.hwnd     = toolPtr->hwnd;
	lptthit->ti.uId      = toolPtr->uId;
	lptthit->ti.rect     = toolPtr->rect;
	lptthit->ti.hinst    = toolPtr->hinst;
/*	lptthit->ti.lpszText = toolPtr->lpszText; */
	lptthit->ti.lpszText = NULL;  /* FIXME */
	lptthit->ti.lParam   = toolPtr->lParam;
    }

    return TRUE;
}


static LRESULT
TOOLTIPS_HitTest32W (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTHITTESTINFO32W lptthit = (LPTTHITTESTINFO32W)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nTool;

    if (lptthit == 0)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromPoint (infoPtr, lptthit->hwnd, &lptthit->pt);
    if (nTool == -1)
	return FALSE;

    TRACE (tooltips, "tool %d!\n", nTool);

    /* copy tool data */
    if (lptthit->ti.cbSize >= sizeof(TTTOOLINFO32W)) {
	toolPtr = &infoPtr->tools[nTool];

	lptthit->ti.uFlags   = toolPtr->uFlags;
	lptthit->ti.hwnd     = toolPtr->hwnd;
	lptthit->ti.uId      = toolPtr->uId;
	lptthit->ti.rect     = toolPtr->rect;
	lptthit->ti.hinst    = toolPtr->hinst;
/*	lptthit->ti.lpszText = toolPtr->lpszText; */
	lptthit->ti.lpszText = NULL;  /* FIXME */
	lptthit->ti.lParam   = toolPtr->lParam;
    }

    return TRUE;
}


static LRESULT
TOOLTIPS_NewToolRect32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32A lpti = (LPTTTOOLINFO32A)lParam;
    INT32 nTool;

    if (lpti == NULL)
	return 0;
    if (lpti->cbSize < TTTOOLINFO_V1_SIZE32A)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpti);
    if (nTool == -1) return 0;

    infoPtr->tools[nTool].rect = lpti->rect;

    return 0;
}


static LRESULT
TOOLTIPS_NewToolRect32W (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32W lpti = (LPTTTOOLINFO32W)lParam;
    INT32 nTool;

    if (lpti == NULL)
	return 0;
    if (lpti->cbSize < TTTOOLINFO_V1_SIZE32W)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpti);
    if (nTool == -1) return 0;

    infoPtr->tools[nTool].rect = lpti->rect;

    return 0;
}


__inline__ static LRESULT
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

    if (lParam == 0) {
	ERR (tooltips, "lpMsg == NULL!\n");
	return 0;
    }

    switch (lpMsg->message) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	    pt = lpMsg->pt;
	    ScreenToClient32 (lpMsg->hwnd, &pt);
	    infoPtr->nOldTool = infoPtr->nTool;
	    infoPtr->nTool = TOOLTIPS_GetToolFromPoint (infoPtr, lpMsg->hwnd, &pt);
	    TRACE (tooltips, "tool (%x) %d %d\n",
		   wndPtr->hwndSelf, infoPtr->nOldTool, infoPtr->nTool);
	    TOOLTIPS_Hide (wndPtr, infoPtr);
	    break;

	case WM_MOUSEMOVE:
	    pt = lpMsg->pt;
	    ScreenToClient32 (lpMsg->hwnd, &pt);
	    infoPtr->nOldTool = infoPtr->nTool;
	    infoPtr->nTool = TOOLTIPS_GetToolFromPoint (infoPtr, lpMsg->hwnd, &pt);
	    TRACE (tooltips, "tool (%x) %d %d\n",
		   wndPtr->hwndSelf, infoPtr->nOldTool, infoPtr->nTool);
	    TRACE (tooltips, "WM_MOUSEMOVE (%04x %d %d)\n",
		   wndPtr->hwndSelf, pt.x, pt.y);
	    if ((infoPtr->bActive) && (infoPtr->nTool != infoPtr->nOldTool)) {
		if (infoPtr->nOldTool == -1) {
		    SetTimer32 (wndPtr->hwndSelf, ID_TIMERSHOW,
				infoPtr->nInitialTime, 0);
		    TRACE (tooltips, "timer 1 started!\n");
		}
		else {
		    TOOLTIPS_Hide (wndPtr, infoPtr);
		    SetTimer32 (wndPtr->hwndSelf, ID_TIMERSHOW,
				infoPtr->nReshowTime, 0);
		    TRACE (tooltips, "timer 2 started!\n");
		}
	    }
	    if (infoPtr->nCurrentTool != -1) {
		SetTimer32 (wndPtr->hwndSelf, ID_TIMERLEAVE, 100, 0);
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


__inline__ static LRESULT
TOOLTIPS_SetMaxTipWidth (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    INT32 nTemp = infoPtr->nMaxTipWidth;

    infoPtr->nMaxTipWidth = (INT32)lParam;

    return nTemp;
}


__inline__ static LRESULT
TOOLTIPS_SetTipBkColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    infoPtr->clrBk = (COLORREF)wParam;

    return 0;
}


__inline__ static LRESULT
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

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32A)
	return 0;

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

    if ((lpToolInfo->hinst) && (HIWORD((INT32)lpToolInfo->lpszText) == 0)) {
	TRACE (tooltips, "set string id %x!\n", (INT32)lpToolInfo->lpszText);
	toolPtr->lpszText = (LPWSTR)lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACK32A)
	    toolPtr->lpszText = LPSTR_TEXTCALLBACK32W;
	else {
	    if (toolPtr->lpszText) {
		COMCTL32_Free (toolPtr->lpszText);
		toolPtr->lpszText = NULL;
	    }
	    if (lpToolInfo->lpszText) {
		INT32 len = lstrlen32A (lpToolInfo->lpszText);
		toolPtr->lpszText = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
		lstrcpyAtoW (toolPtr->lpszText, lpToolInfo->lpszText);
	    }
	}
    }

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFO32A))
	toolPtr->lParam = lpToolInfo->lParam;

    return 0;
}


static LRESULT
TOOLTIPS_SetToolInfo32W (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32W lpToolInfo = (LPTTTOOLINFO32W)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32W)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    TRACE (tooltips, "tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool data */
    toolPtr->uFlags = lpToolInfo->uFlags;
    toolPtr->hwnd   = lpToolInfo->hwnd;
    toolPtr->uId    = lpToolInfo->uId;
    toolPtr->rect   = lpToolInfo->rect;
    toolPtr->hinst  = lpToolInfo->hinst;

    if ((lpToolInfo->hinst) && (HIWORD((INT32)lpToolInfo->lpszText) == 0)) {
	TRACE (tooltips, "set string id %x!\n", (INT32)lpToolInfo->lpszText);
	toolPtr->lpszText = lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACK32W)
	    toolPtr->lpszText = LPSTR_TEXTCALLBACK32W;
	else {
	    if (toolPtr->lpszText) {
		COMCTL32_Free (toolPtr->lpszText);
		toolPtr->lpszText = NULL;
	    }
	    if (lpToolInfo->lpszText) {
		INT32 len = lstrlen32W (lpToolInfo->lpszText);
		toolPtr->lpszText = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
		lstrcpy32W (toolPtr->lpszText, lpToolInfo->lpszText);
	    }
	}
    }

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFO32W))
	toolPtr->lParam = lpToolInfo->lParam;

    return 0;
}


static LRESULT
TOOLTIPS_TrackActivate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32A lpToolInfo = (LPTTTOOLINFO32A)lParam;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32A)
	return FALSE;

    if ((BOOL32)wParam) {
	/* activate */
	infoPtr->nTrackTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
	if (infoPtr->nTrackTool != -1) {
	    TRACE (tooltips, "activated!\n");
	    infoPtr->bTrackActive = TRUE;
	    TOOLTIPS_TrackShow (wndPtr, infoPtr);
	}
    }
    else {
	/* deactivate */
	TOOLTIPS_TrackHide (wndPtr, infoPtr);

	infoPtr->bTrackActive = FALSE;
	infoPtr->nTrackTool = -1;

	TRACE (tooltips, "deactivated!\n");
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
	TRACE (tooltips, "[%d %d]\n",
	       infoPtr->xTrackPos, infoPtr->yTrackPos);

	TOOLTIPS_TrackShow (wndPtr, infoPtr);
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

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32A)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    TRACE (tooltips, "tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool text */
    toolPtr->hinst  = lpToolInfo->hinst;

    if ((lpToolInfo->hinst) && (HIWORD((INT32)lpToolInfo->lpszText) == 0)){
	toolPtr->lpszText = (LPWSTR)lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACK32A)
	    toolPtr->lpszText = LPSTR_TEXTCALLBACK32W;
	else {
	    if (toolPtr->lpszText) {
		COMCTL32_Free (toolPtr->lpszText);
		toolPtr->lpszText = NULL;
	    }
	    if (lpToolInfo->lpszText) {
		INT32 len = lstrlen32A (lpToolInfo->lpszText);
		toolPtr->lpszText = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
		lstrcpyAtoW (toolPtr->lpszText, lpToolInfo->lpszText);
	    }
	}
    }

    /* force repaint */
    if (infoPtr->bActive)
	TOOLTIPS_Show (wndPtr, infoPtr);
    else if (infoPtr->bTrackActive)
	TOOLTIPS_TrackShow (wndPtr, infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_UpdateTipText32W (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFO32W lpToolInfo = (LPTTTOOLINFO32W)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZE32W)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpToolInfo);
    if (nTool == -1)
	return 0;

    TRACE (tooltips, "tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool text */
    toolPtr->hinst  = lpToolInfo->hinst;

    if ((lpToolInfo->hinst) && (HIWORD((INT32)lpToolInfo->lpszText) == 0)){
	toolPtr->lpszText = lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACK32W)
	    toolPtr->lpszText = LPSTR_TEXTCALLBACK32W;
	else {
	    if (toolPtr->lpszText) {
		COMCTL32_Free (toolPtr->lpszText);
		toolPtr->lpszText = NULL;
	    }
	    if (lpToolInfo->lpszText) {
		INT32 len = lstrlen32W (lpToolInfo->lpszText);
		toolPtr->lpszText = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
		lstrcpy32W (toolPtr->lpszText, lpToolInfo->lpszText);
	    }
	}
    }

    /* force repaint */
    if (infoPtr->bActive)
	TOOLTIPS_Show (wndPtr, infoPtr);
    else if (infoPtr->bTrackActive)
	TOOLTIPS_TrackShow (wndPtr, infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_WindowFromPoint (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    return WindowFromPoint32 (*((LPPOINT32)lParam));
}



static LRESULT
TOOLTIPS_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr;
    NONCLIENTMETRICS32A nclm;
    INT32 nResult;

    /* allocate memory for info structure */
    infoPtr = (TOOLTIPS_INFO *)COMCTL32_Alloc (sizeof(TOOLTIPS_INFO));
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

    nclm.cbSize = sizeof(NONCLIENTMETRICS32A);
    SystemParametersInfo32A (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
    infoPtr->hFont = CreateFontIndirect32A (&nclm.lfStatusFont);

    infoPtr->nMaxTipWidth = -1;
    infoPtr->nTool = -1;
    infoPtr->nOldTool = -1;
    infoPtr->nCurrentTool = -1;
    infoPtr->nTrackTool = -1;

    infoPtr->nAutomaticTime = 500;
    infoPtr->nReshowTime    = 100;
    infoPtr->nAutoPopTime   = 5000;
    infoPtr->nInitialTime   = 500;

    nResult =
	(INT32) SendMessage32A (wndPtr->parent->hwndSelf, WM_NOTIFYFORMAT,
				(WPARAM32)wndPtr->hwndSelf, (LPARAM)NF_QUERY);
    if (nResult == NFR_ANSI)
	FIXME (tooltips, " -- WM_NOTIFYFORMAT returns: NFR_ANSI\n");
    else if (nResult == NFR_UNICODE)
	FIXME (tooltips, " -- WM_NOTIFYFORMAT returns: NFR_UNICODE\n");
    else
	FIXME (tooltips, " -- WM_NOTIFYFORMAT returns: error!\n");

    SetWindowPos32 (wndPtr->hwndSelf, HWND_TOP, 0, 0, 0, 0,
		    SWP_NOZORDER | SWP_HIDEWINDOW);

    return 0;
}


static LRESULT
TOOLTIPS_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    TTTOOL_INFO *toolPtr;
    INT32 i;

    /* free tools */
    if (infoPtr->tools) {
	for (i = 0; i < infoPtr->uNumTools; i++) {
	    toolPtr = &infoPtr->tools[i];
	    if ((toolPtr->hinst) && (toolPtr->lpszText)) {
		if (toolPtr->lpszText != LPSTR_TEXTCALLBACK32W)
		    COMCTL32_Free (toolPtr->lpszText);
	    }

	    /* remove subclassing */
	    if (toolPtr->uFlags & TTF_SUBCLASS) {
		LPTT_SUBCLASS_INFO lpttsi;

		if (toolPtr->uFlags & TTF_IDISHWND)
		    lpttsi = (LPTT_SUBCLASS_INFO)GetProp32A ((HWND32)toolPtr->uId, COMCTL32_aSubclass);
		else
		    lpttsi = (LPTT_SUBCLASS_INFO)GetProp32A (toolPtr->hwnd, COMCTL32_aSubclass);

		if (lpttsi) {
		    SetWindowLong32A ((HWND32)toolPtr->uId, GWL_WNDPROC,
				      (LONG)lpttsi->wpOrigProc);
		    RemoveProp32A ((HWND32)toolPtr->uId, COMCTL32_aSubclass);
		    COMCTL32_Free (&lpttsi);
		}
	    }
	}
	COMCTL32_Free (infoPtr->tools);
    }

    /* delete font */
    DeleteObject32 (infoPtr->hFont);

    /* free tool tips info data */
    COMCTL32_Free (infoPtr);

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
TOOLTIPS_MouseMessage (WND *wndPtr, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    TOOLTIPS_Hide (wndPtr, infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_NCCreate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    wndPtr->dwStyle &= 0x0000FFFF;
    wndPtr->dwStyle |= (WS_POPUP | WS_BORDER | WS_CLIPSIBLINGS);

    return TRUE;
}


static LRESULT
TOOLTIPS_NCHitTest (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    INT32 nTool = (infoPtr->bTrackActive) ? infoPtr->nTrackTool : infoPtr->nTool;

    TRACE (tooltips, " nTool=%d\n", nTool);

    if ((nTool > -1) && (nTool < infoPtr->uNumTools)) {
	if (infoPtr->tools[nTool].uFlags & TTF_TRANSPARENT) {
	    TRACE (tooltips, "-- in transparent mode!\n");
	    return HTTRANSPARENT;
	}
    }

    return DefWindowProc32A (wndPtr->hwndSelf, WM_NCHITTEST, wParam, lParam);
}


static LRESULT
TOOLTIPS_Paint (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HDC32 hdc;
    PAINTSTRUCT32 ps;

    hdc = (wParam == 0) ? BeginPaint32 (wndPtr->hwndSelf, &ps) : (HDC32)wParam;
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
	case ID_TIMERSHOW:
	    KillTimer32 (wndPtr->hwndSelf, ID_TIMERSHOW);
	    if (TOOLTIPS_CheckTool (wndPtr, TRUE) == infoPtr->nTool)
		TOOLTIPS_Show (wndPtr, infoPtr);
	    break;

	case ID_TIMERPOP:
	    TOOLTIPS_Hide (wndPtr, infoPtr);
	    break;

	case ID_TIMERLEAVE:
	    KillTimer32 (wndPtr->hwndSelf, ID_TIMERLEAVE);
	    if (TOOLTIPS_CheckTool (wndPtr, FALSE) == -1) {
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
    NONCLIENTMETRICS32A nclm;

    infoPtr->clrBk   = GetSysColor32 (COLOR_INFOBK);
    infoPtr->clrText = GetSysColor32 (COLOR_INFOTEXT);

    DeleteObject32 (infoPtr->hFont);
    nclm.cbSize = sizeof(NONCLIENTMETRICS32A);
    SystemParametersInfo32A (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
    infoPtr->hFont = CreateFontIndirect32A (&nclm.lfStatusFont);

    return 0;
}


LRESULT CALLBACK
TOOLTIPS_SubclassProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    LPTT_SUBCLASS_INFO lpttsi =
	(LPTT_SUBCLASS_INFO)GetProp32A (hwnd, COMCTL32_aSubclass);
    WND *wndPtr;
    TOOLTIPS_INFO *infoPtr;
    UINT32 nTool;

    switch (uMsg) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	    {
		wndPtr = WIN_FindWndPtr(lpttsi->hwndToolTip);
		infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
		nTool = TOOLTIPS_GetToolFromMessage (infoPtr, hwnd);

		TRACE (tooltips, "subclassed mouse message %04x\n", uMsg);
		infoPtr->nOldTool = infoPtr->nTool;
		infoPtr->nTool = nTool;
		TOOLTIPS_Hide (wndPtr, infoPtr);
	    }
	    break;

	case WM_MOUSEMOVE:
	    {
		wndPtr = WIN_FindWndPtr(lpttsi->hwndToolTip);
		infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
		nTool = TOOLTIPS_GetToolFromMessage (infoPtr, hwnd);

		TRACE (tooltips, "subclassed WM_MOUSEMOVE\n");
		infoPtr->nOldTool = infoPtr->nTool;
		infoPtr->nTool = nTool;

		if ((infoPtr->bActive) &&
		    (infoPtr->nTool != infoPtr->nOldTool)) {
		    if (infoPtr->nOldTool == -1) {
			SetTimer32 (wndPtr->hwndSelf, ID_TIMERSHOW,
				    infoPtr->nInitialTime, 0);
			TRACE (tooltips, "timer 1 started!\n");
		    }
		    else {
			TOOLTIPS_Hide (wndPtr, infoPtr);
			SetTimer32 (wndPtr->hwndSelf, ID_TIMERSHOW,
				infoPtr->nReshowTime, 0);
			TRACE (tooltips, "timer 2 started!\n");
		    }
		}
		if (infoPtr->nCurrentTool != -1) {
		    SetTimer32 (wndPtr->hwndSelf, ID_TIMERLEAVE, 100, 0);
		    TRACE (tooltips, "timer 3 started!\n");
		}
	    }
	    break;
    }

    return CallWindowProc32A (lpttsi->wpOrigProc, hwnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK
TOOLTIPS_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
	case TTM_ACTIVATE:
	    return TOOLTIPS_Activate (wndPtr, wParam, lParam);

	case TTM_ADDTOOL32A:
	    return TOOLTIPS_AddTool32A (wndPtr, wParam, lParam);

	case TTM_ADDTOOL32W:
	    return TOOLTIPS_AddTool32W (wndPtr, wParam, lParam);

	case TTM_DELTOOL32A:
	    return TOOLTIPS_DelTool32A (wndPtr, wParam, lParam);

	case TTM_DELTOOL32W:
	    return TOOLTIPS_DelTool32W (wndPtr, wParam, lParam);

	case TTM_ENUMTOOLS32A:
	    return TOOLTIPS_EnumTools32A (wndPtr, wParam, lParam);

	case TTM_ENUMTOOLS32W:
	    return TOOLTIPS_EnumTools32W (wndPtr, wParam, lParam);

	case TTM_GETCURRENTTOOL32A:
	    return TOOLTIPS_GetCurrentTool32A (wndPtr, wParam, lParam);

	case TTM_GETCURRENTTOOL32W:
	    return TOOLTIPS_GetCurrentTool32W (wndPtr, wParam, lParam);

	case TTM_GETDELAYTIME:
	    return TOOLTIPS_GetDelayTime (wndPtr, wParam, lParam);

	case TTM_GETMARGIN:
	    return TOOLTIPS_GetMargin (wndPtr, wParam, lParam);

	case TTM_GETMAXTIPWIDTH:
	    return TOOLTIPS_GetMaxTipWidth (wndPtr, wParam, lParam);

	case TTM_GETTEXT32A:
	    return TOOLTIPS_GetText32A (wndPtr, wParam, lParam);

	case TTM_GETTEXT32W:
	    return TOOLTIPS_GetText32W (wndPtr, wParam, lParam);

	case TTM_GETTIPBKCOLOR:
	    return TOOLTIPS_GetTipBkColor (wndPtr, wParam, lParam);

	case TTM_GETTIPTEXTCOLOR:
	    return TOOLTIPS_GetTipTextColor (wndPtr, wParam, lParam);

	case TTM_GETTOOLCOUNT:
	    return TOOLTIPS_GetToolCount (wndPtr, wParam, lParam);

	case TTM_GETTOOLINFO32A:
	    return TOOLTIPS_GetToolInfo32A (wndPtr, wParam, lParam);

	case TTM_GETTOOLINFO32W:
	    return TOOLTIPS_GetToolInfo32W (wndPtr, wParam, lParam);

	case TTM_HITTEST32A:
	    return TOOLTIPS_HitTest32A (wndPtr, wParam, lParam);

	case TTM_HITTEST32W:
	    return TOOLTIPS_HitTest32W (wndPtr, wParam, lParam);

	case TTM_NEWTOOLRECT32A:
	    return TOOLTIPS_NewToolRect32A (wndPtr, wParam, lParam);

	case TTM_NEWTOOLRECT32W:
	    return TOOLTIPS_NewToolRect32W (wndPtr, wParam, lParam);

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

	case TTM_SETTOOLINFO32W:
	    return TOOLTIPS_SetToolInfo32W (wndPtr, wParam, lParam);

	case TTM_TRACKACTIVATE:
	    return TOOLTIPS_TrackActivate (wndPtr, wParam, lParam);

	case TTM_TRACKPOSITION:
	    return TOOLTIPS_TrackPosition (wndPtr, wParam, lParam);

	case TTM_UPDATE:
	    return TOOLTIPS_Update (wndPtr, wParam, lParam);

	case TTM_UPDATETIPTEXT32A:
	    return TOOLTIPS_UpdateTipText32A (wndPtr, wParam, lParam);

	case TTM_UPDATETIPTEXT32W:
	    return TOOLTIPS_UpdateTipText32W (wndPtr, wParam, lParam);

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

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
	    return TOOLTIPS_MouseMessage (wndPtr, uMsg, wParam, lParam);

	case WM_NCCREATE:
	    return TOOLTIPS_NCCreate (wndPtr, wParam, lParam);

	case WM_NCHITTEST:
	    return TOOLTIPS_NCHitTest (wndPtr, wParam, lParam);

/*	case WM_NOTIFYFORMAT: */
/*	    return TOOLTIPS_NotifyFormat (wndPtr, wParam, lParam); */

	case WM_PAINT:
	    return TOOLTIPS_Paint (wndPtr, wParam, lParam);

	case WM_SETFONT:
	    return TOOLTIPS_SetFont (wndPtr, wParam, lParam);

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


VOID
TOOLTIPS_Register (VOID)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (TOOLTIPS_CLASS32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS;
    wndClass.lpfnWndProc   = (WNDPROC32)TOOLTIPS_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TOOLTIPS_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = TOOLTIPS_CLASS32A;
 
    RegisterClass32A (&wndClass);
}


VOID
TOOLTIPS_Unregister (VOID)
{
    if (GlobalFindAtom32A (TOOLTIPS_CLASS32A))
	UnregisterClass32A (TOOLTIPS_CLASS32A, (HINSTANCE32)NULL);
}

