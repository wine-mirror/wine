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
#include <string.h>

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
TOOLTIPS_SubclassProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


static VOID
TOOLTIPS_Refresh (WND *wndPtr, HDC hdc)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    RECT rc;
    INT oldBkMode;
    HFONT hOldFont;
    HBRUSH hBrush;
    UINT uFlags = DT_EXTERNALLEADING;

    if (infoPtr->nMaxTipWidth > -1)
	uFlags |= DT_WORDBREAK;
    if (wndPtr->dwStyle & TTS_NOPREFIX)
	uFlags |= DT_NOPREFIX;
    GetClientRect (wndPtr->hwndSelf, &rc);

    /* fill the background */
    hBrush = CreateSolidBrush (infoPtr->clrBk);
    FillRect (hdc, &rc, hBrush);
    DeleteObject (hBrush);

    /* calculate text rectangle */
    rc.left   += (2 + infoPtr->rcMargin.left);
    rc.top    += (2 + infoPtr->rcMargin.top);
    rc.right  -= (2 + infoPtr->rcMargin.right);
    rc.bottom -= (2 + infoPtr->rcMargin.bottom);

    /* draw text */
    oldBkMode = SetBkMode (hdc, TRANSPARENT);
    SetTextColor (hdc, infoPtr->clrText);
    hOldFont = SelectObject (hdc, infoPtr->hFont);
    DrawTextW (hdc, infoPtr->szTipText, -1, &rc, uFlags);
    SelectObject (hdc, hOldFont);
    if (oldBkMode != TRANSPARENT)
	SetBkMode (hdc, oldBkMode);
}


static VOID
TOOLTIPS_GetTipText (WND *wndPtr, TOOLTIPS_INFO *infoPtr, INT nTool)
{
    TTTOOL_INFO *toolPtr = &infoPtr->tools[nTool];

    if ((toolPtr->hinst) && (HIWORD((UINT)toolPtr->lpszText) == 0)) {
	/* load a resource */
	TRACE (tooltips, "load res string %x %x\n",
	       toolPtr->hinst, (int)toolPtr->lpszText);
	LoadStringW (toolPtr->hinst, (UINT)toolPtr->lpszText,
		       infoPtr->szTipText, INFOTIPSIZE);
    }
    else if (toolPtr->lpszText) {
	if (toolPtr->lpszText == LPSTR_TEXTCALLBACKW) {
	    NMTTDISPINFOA ttnmdi;

	    /* fill NMHDR struct */
	    ZeroMemory (&ttnmdi, sizeof(NMTTDISPINFOA));
	    ttnmdi.hdr.hwndFrom = wndPtr->hwndSelf;
	    ttnmdi.hdr.idFrom = toolPtr->uId;
	    ttnmdi.hdr.code = TTN_GETDISPINFOA;
	    ttnmdi.lpszText = (LPSTR)&ttnmdi.szText;
	    ttnmdi.uFlags = toolPtr->uFlags;
	    ttnmdi.lParam = toolPtr->lParam;

	    TRACE (tooltips, "hdr.idFrom = %x\n", ttnmdi.hdr.idFrom);
	    SendMessageA (toolPtr->hwnd, WM_NOTIFY,
			    (WPARAM)toolPtr->uId, (LPARAM)&ttnmdi);

	    if ((ttnmdi.hinst) && (HIWORD((UINT)ttnmdi.szText) == 0)) {
		LoadStringW (ttnmdi.hinst, (UINT)ttnmdi.szText,
			       infoPtr->szTipText, INFOTIPSIZE);
		if (ttnmdi.uFlags & TTF_DI_SETITEM) {
		    toolPtr->hinst = ttnmdi.hinst;
		    toolPtr->lpszText = (LPWSTR)ttnmdi.szText;
		}
	    }
	    else if (ttnmdi.szText[0]) {
		lstrcpynAtoW (infoPtr->szTipText, ttnmdi.szText, 80);
		if (ttnmdi.uFlags & TTF_DI_SETITEM) {
		    INT len = lstrlenA (ttnmdi.szText);
		    toolPtr->hinst = 0;
		    toolPtr->lpszText =	COMCTL32_Alloc ((len+1)* sizeof(WCHAR));
		    lstrcpyAtoW (toolPtr->lpszText, ttnmdi.szText);
		}
	    }
	    else if (ttnmdi.lpszText == 0) {
		/* no text available */
		infoPtr->szTipText[0] = L'\0';
	    }
	    else if (ttnmdi.lpszText != LPSTR_TEXTCALLBACKA) {
		lstrcpynAtoW (infoPtr->szTipText, ttnmdi.lpszText, INFOTIPSIZE);
		if (ttnmdi.uFlags & TTF_DI_SETITEM) {
		    INT len = lstrlenA (ttnmdi.lpszText);
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
	    lstrcpynW (infoPtr->szTipText, toolPtr->lpszText, INFOTIPSIZE);
	}
    }
    else {
	/* no text available */
	infoPtr->szTipText[0] = L'\0';
    }

    TRACE (tooltips, "\"%s\"\n", debugstr_w(infoPtr->szTipText));
}


static VOID
TOOLTIPS_CalcTipSize (WND *wndPtr, TOOLTIPS_INFO *infoPtr, LPSIZE lpSize)
{
    HDC hdc;
    HFONT hOldFont;
    UINT uFlags = DT_EXTERNALLEADING | DT_CALCRECT;
    RECT rc = {0, 0, 0, 0};

    if (infoPtr->nMaxTipWidth > -1) {
	rc.right = infoPtr->nMaxTipWidth;
	uFlags |= DT_WORDBREAK;
    }
    if (wndPtr->dwStyle & TTS_NOPREFIX)
	uFlags |= DT_NOPREFIX;
    TRACE (tooltips, "\"%s\"\n", debugstr_w(infoPtr->szTipText));

    hdc = GetDC (wndPtr->hwndSelf);
    hOldFont = SelectObject (hdc, infoPtr->hFont);
    DrawTextW (hdc, infoPtr->szTipText, -1, &rc, uFlags);
    SelectObject (hdc, hOldFont);
    ReleaseDC (wndPtr->hwndSelf, hdc);

    lpSize->cx = rc.right - rc.left + 4 + 
		 infoPtr->rcMargin.left + infoPtr->rcMargin.right;
    lpSize->cy = rc.bottom - rc.top + 4 +
		 infoPtr->rcMargin.bottom + infoPtr->rcMargin.top;
}


static VOID
TOOLTIPS_Show (WND *wndPtr, TOOLTIPS_INFO *infoPtr)
{
    TTTOOL_INFO *toolPtr;
    RECT rect;
    SIZE size;
    HDC  hdc;
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
    SendMessageA (toolPtr->hwnd, WM_NOTIFY,
		    (WPARAM)toolPtr->uId, (LPARAM)&hdr);

    TRACE (tooltips, "\"%s\"\n", debugstr_w(infoPtr->szTipText));

    TOOLTIPS_CalcTipSize (wndPtr, infoPtr, &size);
    TRACE (tooltips, "size %d - %d\n", size.cx, size.cy);

    if (toolPtr->uFlags & TTF_CENTERTIP) {
	RECT rc;

	if (toolPtr->uFlags & TTF_IDISHWND)
	    GetWindowRect ((HWND)toolPtr->uId, &rc);
	else {
	    rc = toolPtr->rect;
	    MapWindowPoints (toolPtr->hwnd, (HWND)0, (LPPOINT)&rc, 2);
	}
	rect.left = (rc.left + rc.right - size.cx) / 2;
	rect.top  = rc.bottom + 2;
    }
    else {
	GetCursorPos ((LPPOINT)&rect);
	rect.top += 20;
    }

    /* FIXME: check position */

    TRACE (tooltips, "pos %d - %d\n", rect.left, rect.top);

    rect.right = rect.left + size.cx;
    rect.bottom = rect.top + size.cy;

    AdjustWindowRectEx (&rect, wndPtr->dwStyle, FALSE, wndPtr->dwExStyle);

    SetWindowPos (wndPtr->hwndSelf, HWND_TOP, rect.left, rect.top,
		    rect.right - rect.left, rect.bottom - rect.top,
		    SWP_SHOWWINDOW);

    /* repaint the tooltip */
    hdc = GetDC (wndPtr->hwndSelf);
    TOOLTIPS_Refresh (wndPtr, hdc);
    ReleaseDC (wndPtr->hwndSelf, hdc);

    SetTimer (wndPtr->hwndSelf, ID_TIMERPOP, infoPtr->nAutoPopTime, 0);
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
    KillTimer (wndPtr->hwndSelf, ID_TIMERPOP);

    hdr.hwndFrom = wndPtr->hwndSelf;
    hdr.idFrom = toolPtr->uId;
    hdr.code = TTN_POP;
    SendMessageA (toolPtr->hwnd, WM_NOTIFY,
		    (WPARAM)toolPtr->uId, (LPARAM)&hdr);

    infoPtr->nCurrentTool = -1;

    SetWindowPos (wndPtr->hwndSelf, HWND_TOP, 0, 0, 0, 0,
		    SWP_NOZORDER | SWP_HIDEWINDOW);
}


static VOID
TOOLTIPS_TrackShow (WND *wndPtr, TOOLTIPS_INFO *infoPtr)
{
    TTTOOL_INFO *toolPtr;
    RECT rect;
    SIZE size;
    HDC  hdc;
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
    SendMessageA (toolPtr->hwnd, WM_NOTIFY,
		    (WPARAM)toolPtr->uId, (LPARAM)&hdr);

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
	RECT rcTool;

	if (toolPtr->uFlags & TTF_IDISHWND)
	    GetWindowRect ((HWND)toolPtr->uId, &rcTool);
	else {
	    rcTool = toolPtr->rect;
	    MapWindowPoints (toolPtr->hwnd, (HWND)0, (LPPOINT)&rcTool, 2);
	}

	GetCursorPos ((LPPOINT)&rect);
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

    AdjustWindowRectEx (&rect, wndPtr->dwStyle, FALSE, wndPtr->dwExStyle);

    SetWindowPos (wndPtr->hwndSelf, HWND_TOP, rect.left, rect.top,
		    rect.right - rect.left, rect.bottom - rect.top,
		    SWP_SHOWWINDOW);

    hdc = GetDC (wndPtr->hwndSelf);
    TOOLTIPS_Refresh (wndPtr, hdc);
    ReleaseDC (wndPtr->hwndSelf, hdc);
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
    SendMessageA (toolPtr->hwnd, WM_NOTIFY,
		    (WPARAM)toolPtr->uId, (LPARAM)&hdr);

    SetWindowPos (wndPtr->hwndSelf, HWND_TOP, 0, 0, 0, 0,
		    SWP_NOZORDER | SWP_HIDEWINDOW);
}


static INT
TOOLTIPS_GetToolFromInfoA (TOOLTIPS_INFO *infoPtr, LPTTTOOLINFOA lpToolInfo)
{
    TTTOOL_INFO *toolPtr;
    INT nTool;

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


static INT
TOOLTIPS_GetToolFromInfoW (TOOLTIPS_INFO *infoPtr, LPTTTOOLINFOW lpToolInfo)
{
    TTTOOL_INFO *toolPtr;
    INT nTool;

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


static INT
TOOLTIPS_GetToolFromPoint (TOOLTIPS_INFO *infoPtr, HWND hwnd, LPPOINT lpPt)
{
    TTTOOL_INFO *toolPtr;
    INT  nTool;

    for (nTool = 0; nTool < infoPtr->uNumTools; nTool++) {
	toolPtr = &infoPtr->tools[nTool];

	if (!(toolPtr->uFlags & TTF_IDISHWND)) {
	    if (hwnd != toolPtr->hwnd)
		continue;
	    if (!PtInRect (&toolPtr->rect, *lpPt))
		continue;
	    return nTool;
	}
    }

    for (nTool = 0; nTool < infoPtr->uNumTools; nTool++) {
	toolPtr = &infoPtr->tools[nTool];

	if (toolPtr->uFlags & TTF_IDISHWND) {
	    if ((HWND)toolPtr->uId == hwnd)
		return nTool;
	}
    }

    return -1;
}


static INT
TOOLTIPS_GetToolFromMessage (TOOLTIPS_INFO *infoPtr, HWND hwndTool)
{
    DWORD   dwPos;
    POINT pt;

    dwPos = GetMessagePos ();
    pt.x = (INT)LOWORD(dwPos);
    pt.y = (INT)HIWORD(dwPos);
    ScreenToClient (hwndTool, &pt);

    return TOOLTIPS_GetToolFromPoint (infoPtr, hwndTool, &pt);
}


static BOOL
TOOLTIPS_IsWindowActive (HWND hwnd)
{
    HWND hwndActive = GetActiveWindow ();
    if (!hwndActive)
	return FALSE;
    if (hwndActive == hwnd)
	return TRUE;
    return IsChild (hwndActive, hwnd);
}


static INT
TOOLTIPS_CheckTool (WND *wndPtr, BOOL bShowTest)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    POINT pt;
    HWND hwndTool;
    INT nTool;

    GetCursorPos (&pt);
    hwndTool =
	SendMessageA (wndPtr->hwndSelf, TTM_WINDOWFROMPOINT, 0, (LPARAM)&pt);
    if (hwndTool == 0)
	return -1;

    ScreenToClient (hwndTool, &pt);
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
TOOLTIPS_Activate (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    infoPtr->bActive = (BOOL)wParam;

    if (infoPtr->bActive)
	TRACE (tooltips, "activate!\n");

    if (!(infoPtr->bActive) && (infoPtr->nCurrentTool != -1)) 
	TOOLTIPS_Hide (wndPtr, infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_AddToolA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEA)
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

    if ((lpToolInfo->hinst) && (HIWORD((INT)lpToolInfo->lpszText) == 0)) {
	TRACE (tooltips, "add string id %x!\n", (int)lpToolInfo->lpszText);
	toolPtr->lpszText = (LPWSTR)lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACKA) {
	    TRACE (tooltips, "add CALLBACK!\n");
	    toolPtr->lpszText = LPSTR_TEXTCALLBACKW;
	}
	else {
	    INT len = lstrlenA (lpToolInfo->lpszText);
	    TRACE (tooltips, "add text \"%s\"!\n", lpToolInfo->lpszText);
	    toolPtr->lpszText =	COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    lstrcpyAtoW (toolPtr->lpszText, lpToolInfo->lpszText);
	}
    }

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOA))
	toolPtr->lParam = lpToolInfo->lParam;

    /* install subclassing hook */
    if (toolPtr->uFlags & TTF_SUBCLASS) {
	if (toolPtr->uFlags & TTF_IDISHWND) {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetPropA ((HWND)toolPtr->uId, COMCTL32_aSubclass);
	    if (lpttsi == NULL) {
		lpttsi =
		    (LPTT_SUBCLASS_INFO)COMCTL32_Alloc (sizeof(TT_SUBCLASS_INFO));
		lpttsi->wpOrigProc = 
		    (WNDPROC)SetWindowLongA ((HWND)toolPtr->uId,
		    GWL_WNDPROC,(LONG)TOOLTIPS_SubclassProc);
		lpttsi->hwndToolTip = wndPtr->hwndSelf;
		lpttsi->uRefCount++;
		SetPropA ((HWND)toolPtr->uId, COMCTL32_aSubclass,
			    (HANDLE)lpttsi);
	    }
	    else
		WARN (tooltips, "A window tool must only be listed once!\n");
	}
	else {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetPropA (toolPtr->hwnd, COMCTL32_aSubclass);
	    if (lpttsi == NULL) {
		lpttsi =
		    (LPTT_SUBCLASS_INFO)COMCTL32_Alloc (sizeof(TT_SUBCLASS_INFO));
		lpttsi->wpOrigProc = 
		    (WNDPROC)SetWindowLongA (toolPtr->hwnd,
		    GWL_WNDPROC,(LONG)TOOLTIPS_SubclassProc);
		lpttsi->hwndToolTip = wndPtr->hwndSelf;
		lpttsi->uRefCount++;
		SetPropA (toolPtr->hwnd, COMCTL32_aSubclass, (HANDLE)lpttsi);
	    }
	    else
		lpttsi->uRefCount++;
	}
	TRACE (tooltips, "subclassing installed!\n");
    }

    return TRUE;
}


static LRESULT
TOOLTIPS_AddToolW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEW)
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

    if ((lpToolInfo->hinst) && (HIWORD((INT)lpToolInfo->lpszText) == 0)) {
	TRACE (tooltips, "add string id %x!\n", (int)lpToolInfo->lpszText);
	toolPtr->lpszText = (LPWSTR)lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACKW) {
	    TRACE (tooltips, "add CALLBACK!\n");
	    toolPtr->lpszText = LPSTR_TEXTCALLBACKW;
	}
	else {
	    INT len = lstrlenW (lpToolInfo->lpszText);
	    TRACE (tooltips, "add text \"%s\"!\n",
		   debugstr_w(lpToolInfo->lpszText));
	    toolPtr->lpszText =	COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    lstrcpyW (toolPtr->lpszText, lpToolInfo->lpszText);
	}
    }

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOW))
	toolPtr->lParam = lpToolInfo->lParam;

    /* install subclassing hook */
    if (toolPtr->uFlags & TTF_SUBCLASS) {
	if (toolPtr->uFlags & TTF_IDISHWND) {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetPropA ((HWND)toolPtr->uId, COMCTL32_aSubclass);
	    if (lpttsi == NULL) {
		lpttsi =
		    (LPTT_SUBCLASS_INFO)COMCTL32_Alloc (sizeof(TT_SUBCLASS_INFO));
		lpttsi->wpOrigProc = 
		    (WNDPROC)SetWindowLongA ((HWND)toolPtr->uId,
		    GWL_WNDPROC,(LONG)TOOLTIPS_SubclassProc);
		lpttsi->hwndToolTip = wndPtr->hwndSelf;
		lpttsi->uRefCount++;
		SetPropA ((HWND)toolPtr->uId, COMCTL32_aSubclass,
			    (HANDLE)lpttsi);
	    }
	    else
		WARN (tooltips, "A window tool must only be listed once!\n");
	}
	else {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetPropA (toolPtr->hwnd, COMCTL32_aSubclass);
	    if (lpttsi == NULL) {
		lpttsi =
		    (LPTT_SUBCLASS_INFO)COMCTL32_Alloc (sizeof(TT_SUBCLASS_INFO));
		lpttsi->wpOrigProc = 
		    (WNDPROC)SetWindowLongA (toolPtr->hwnd,
		    GWL_WNDPROC,(LONG)TOOLTIPS_SubclassProc);
		lpttsi->hwndToolTip = wndPtr->hwndSelf;
		lpttsi->uRefCount++;
		SetPropA (toolPtr->hwnd, COMCTL32_aSubclass, (HANDLE)lpttsi);
	    }
	    else
		lpttsi->uRefCount++;
	}
	TRACE (tooltips, "subclassing installed!\n");
    }

    return TRUE;
}


static LRESULT
TOOLTIPS_DelToolA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEA)
	return 0;
    if (infoPtr->uNumTools == 0)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    TRACE (tooltips, "tool %d\n", nTool);

    /* delete text string */
    toolPtr = &infoPtr->tools[nTool]; 
    if ((toolPtr->hinst) && (toolPtr->lpszText)) {
	if (toolPtr->lpszText != LPSTR_TEXTCALLBACKW)
	    COMCTL32_Free (toolPtr->lpszText);
    }

    /* remove subclassing */
    if (toolPtr->uFlags & TTF_SUBCLASS) {
	if (toolPtr->uFlags & TTF_IDISHWND) {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetPropA ((HWND)toolPtr->uId, COMCTL32_aSubclass);
	    if (lpttsi) {
		SetWindowLongA ((HWND)toolPtr->uId, GWL_WNDPROC,
				  (LONG)lpttsi->wpOrigProc);
		RemovePropA ((HWND)toolPtr->uId, COMCTL32_aSubclass);
		COMCTL32_Free (&lpttsi);
	    }
	    else
		ERR (tooltips, "Invalid data handle!\n");
	}
	else {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetPropA (toolPtr->hwnd, COMCTL32_aSubclass);
	    if (lpttsi) {
		if (lpttsi->uRefCount == 1) {
		    SetWindowLongA ((HWND)toolPtr->uId, GWL_WNDPROC,
				      (LONG)lpttsi->wpOrigProc);
		    RemovePropA ((HWND)toolPtr->uId, COMCTL32_aSubclass);
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
TOOLTIPS_DelToolW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEW)
	return 0;
    if (infoPtr->uNumTools == 0)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    TRACE (tooltips, "tool %d\n", nTool);

    /* delete text string */
    toolPtr = &infoPtr->tools[nTool]; 
    if ((toolPtr->hinst) && (toolPtr->lpszText)) {
	if (toolPtr->lpszText != LPSTR_TEXTCALLBACKW)
	    COMCTL32_Free (toolPtr->lpszText);
    }

    /* remove subclassing */
    if (toolPtr->uFlags & TTF_SUBCLASS) {
	if (toolPtr->uFlags & TTF_IDISHWND) {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetPropA ((HWND)toolPtr->uId, COMCTL32_aSubclass);
	    if (lpttsi) {
		SetWindowLongA ((HWND)toolPtr->uId, GWL_WNDPROC,
				  (LONG)lpttsi->wpOrigProc);
		RemovePropA ((HWND)toolPtr->uId, COMCTL32_aSubclass);
		COMCTL32_Free (&lpttsi);
	    }
	    else
		ERR (tooltips, "Invalid data handle!\n");
	}
	else {
	    LPTT_SUBCLASS_INFO lpttsi =
		(LPTT_SUBCLASS_INFO)GetPropA (toolPtr->hwnd, COMCTL32_aSubclass);
	    if (lpttsi) {
		if (lpttsi->uRefCount == 1) {
		    SetWindowLongA ((HWND)toolPtr->uId, GWL_WNDPROC,
				      (LONG)lpttsi->wpOrigProc);
		    RemovePropA ((HWND)toolPtr->uId, COMCTL32_aSubclass);
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
TOOLTIPS_EnumToolsA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    UINT uIndex = (UINT)wParam;
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEA)
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

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOA))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


static LRESULT
TOOLTIPS_EnumToolsW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    UINT uIndex = (UINT)wParam;
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEW)
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

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOW))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


static LRESULT
TOOLTIPS_GetCurrentToolA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEA)
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

	    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOA))
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
TOOLTIPS_GetCurrentToolW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEW)
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

	    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOW))
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
TOOLTIPS_GetDelayTime (WND *wndPtr, WPARAM wParam, LPARAM lParam)
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
TOOLTIPS_GetMargin (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPRECT lpRect = (LPRECT)lParam;

    lpRect->left   = infoPtr->rcMargin.left;
    lpRect->right  = infoPtr->rcMargin.right;
    lpRect->bottom = infoPtr->rcMargin.bottom;
    lpRect->top    = infoPtr->rcMargin.top;

    return 0;
}


__inline__ static LRESULT
TOOLTIPS_GetMaxTipWidth (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    return infoPtr->nMaxTipWidth;
}


static LRESULT
TOOLTIPS_GetTextA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEA)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    lstrcpyWtoA (lpToolInfo->lpszText, infoPtr->tools[nTool].lpszText);

    return 0;
}


static LRESULT
TOOLTIPS_GetTextW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEW)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    lstrcpyW (lpToolInfo->lpszText, infoPtr->tools[nTool].lpszText);

    return 0;
}


__inline__ static LRESULT
TOOLTIPS_GetTipBkColor (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    return infoPtr->clrBk;
}


__inline__ static LRESULT
TOOLTIPS_GetTipTextColor (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    return infoPtr->clrText;
}


__inline__ static LRESULT
TOOLTIPS_GetToolCount (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    return infoPtr->uNumTools;
}


static LRESULT
TOOLTIPS_GetToolInfoA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEA)
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

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOA))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


static LRESULT
TOOLTIPS_GetToolInfoW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEW)
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

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOW))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


static LRESULT
TOOLTIPS_HitTestA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTHITTESTINFOA lptthit = (LPTTHITTESTINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lptthit == 0)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromPoint (infoPtr, lptthit->hwnd, &lptthit->pt);
    if (nTool == -1)
	return FALSE;

    TRACE (tooltips, "tool %d!\n", nTool);

    /* copy tool data */
    if (lptthit->ti.cbSize >= sizeof(TTTOOLINFOA)) {
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
TOOLTIPS_HitTestW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTHITTESTINFOW lptthit = (LPTTHITTESTINFOW)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lptthit == 0)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromPoint (infoPtr, lptthit->hwnd, &lptthit->pt);
    if (nTool == -1)
	return FALSE;

    TRACE (tooltips, "tool %d!\n", nTool);

    /* copy tool data */
    if (lptthit->ti.cbSize >= sizeof(TTTOOLINFOW)) {
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
TOOLTIPS_NewToolRectA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOA lpti = (LPTTTOOLINFOA)lParam;
    INT nTool;

    if (lpti == NULL)
	return 0;
    if (lpti->cbSize < TTTOOLINFO_V1_SIZEA)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpti);
    if (nTool == -1) return 0;

    infoPtr->tools[nTool].rect = lpti->rect;

    return 0;
}


static LRESULT
TOOLTIPS_NewToolRectW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOW lpti = (LPTTTOOLINFOW)lParam;
    INT nTool;

    if (lpti == NULL)
	return 0;
    if (lpti->cbSize < TTTOOLINFO_V1_SIZEW)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpti);
    if (nTool == -1) return 0;

    infoPtr->tools[nTool].rect = lpti->rect;

    return 0;
}


__inline__ static LRESULT
TOOLTIPS_Pop (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    TOOLTIPS_Hide (wndPtr, infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_RelayEvent (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPMSG lpMsg = (LPMSG)lParam;
    POINT pt;

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
	    ScreenToClient (lpMsg->hwnd, &pt);
	    infoPtr->nOldTool = infoPtr->nTool;
	    infoPtr->nTool = TOOLTIPS_GetToolFromPoint (infoPtr, lpMsg->hwnd, &pt);
	    TRACE (tooltips, "tool (%x) %d %d\n",
		   wndPtr->hwndSelf, infoPtr->nOldTool, infoPtr->nTool);
	    TOOLTIPS_Hide (wndPtr, infoPtr);
	    break;

	case WM_MOUSEMOVE:
	    pt = lpMsg->pt;
	    ScreenToClient (lpMsg->hwnd, &pt);
	    infoPtr->nOldTool = infoPtr->nTool;
	    infoPtr->nTool = TOOLTIPS_GetToolFromPoint (infoPtr, lpMsg->hwnd, &pt);
	    TRACE (tooltips, "tool (%x) %d %d\n",
		   wndPtr->hwndSelf, infoPtr->nOldTool, infoPtr->nTool);
	    TRACE (tooltips, "WM_MOUSEMOVE (%04x %ld %ld)\n",
		   wndPtr->hwndSelf, pt.x, pt.y);
	    if ((infoPtr->bActive) && (infoPtr->nTool != infoPtr->nOldTool)) {
		if (infoPtr->nOldTool == -1) {
		    SetTimer (wndPtr->hwndSelf, ID_TIMERSHOW,
				infoPtr->nInitialTime, 0);
		    TRACE (tooltips, "timer 1 started!\n");
		}
		else {
		    TOOLTIPS_Hide (wndPtr, infoPtr);
		    SetTimer (wndPtr->hwndSelf, ID_TIMERSHOW,
				infoPtr->nReshowTime, 0);
		    TRACE (tooltips, "timer 2 started!\n");
		}
	    }
	    if (infoPtr->nCurrentTool != -1) {
		SetTimer (wndPtr->hwndSelf, ID_TIMERLEAVE, 100, 0);
		TRACE (tooltips, "timer 3 started!\n");
	    }
	    break;
    }

    return 0;
}


static LRESULT
TOOLTIPS_SetDelayTime (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    INT nTime = (INT)LOWORD(lParam);

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
TOOLTIPS_SetMargin (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPRECT lpRect = (LPRECT)lParam;

    infoPtr->rcMargin.left   = lpRect->left;
    infoPtr->rcMargin.right  = lpRect->right;
    infoPtr->rcMargin.bottom = lpRect->bottom;
    infoPtr->rcMargin.top    = lpRect->top;

    return 0;
}


__inline__ static LRESULT
TOOLTIPS_SetMaxTipWidth (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    INT nTemp = infoPtr->nMaxTipWidth;

    infoPtr->nMaxTipWidth = (INT)lParam;

    return nTemp;
}


__inline__ static LRESULT
TOOLTIPS_SetTipBkColor (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    infoPtr->clrBk = (COLORREF)wParam;

    return 0;
}


__inline__ static LRESULT
TOOLTIPS_SetTipTextColor (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    infoPtr->clrText = (COLORREF)wParam;

    return 0;
}


static LRESULT
TOOLTIPS_SetToolInfoA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEA)
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

    if ((lpToolInfo->hinst) && (HIWORD((INT)lpToolInfo->lpszText) == 0)) {
	TRACE (tooltips, "set string id %x!\n", (INT)lpToolInfo->lpszText);
	toolPtr->lpszText = (LPWSTR)lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACKA)
	    toolPtr->lpszText = LPSTR_TEXTCALLBACKW;
	else {
	    if (toolPtr->lpszText) {
		COMCTL32_Free (toolPtr->lpszText);
		toolPtr->lpszText = NULL;
	    }
	    if (lpToolInfo->lpszText) {
		INT len = lstrlenA (lpToolInfo->lpszText);
		toolPtr->lpszText = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
		lstrcpyAtoW (toolPtr->lpszText, lpToolInfo->lpszText);
	    }
	}
    }

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOA))
	toolPtr->lParam = lpToolInfo->lParam;

    return 0;
}


static LRESULT
TOOLTIPS_SetToolInfoW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEW)
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

    if ((lpToolInfo->hinst) && (HIWORD((INT)lpToolInfo->lpszText) == 0)) {
	TRACE (tooltips, "set string id %x!\n", (INT)lpToolInfo->lpszText);
	toolPtr->lpszText = lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACKW)
	    toolPtr->lpszText = LPSTR_TEXTCALLBACKW;
	else {
	    if (toolPtr->lpszText) {
		COMCTL32_Free (toolPtr->lpszText);
		toolPtr->lpszText = NULL;
	    }
	    if (lpToolInfo->lpszText) {
		INT len = lstrlenW (lpToolInfo->lpszText);
		toolPtr->lpszText = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
		lstrcpyW (toolPtr->lpszText, lpToolInfo->lpszText);
	    }
	}
    }

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOW))
	toolPtr->lParam = lpToolInfo->lParam;

    return 0;
}


static LRESULT
TOOLTIPS_TrackActivate (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEA)
	return FALSE;

    if ((BOOL)wParam) {
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
TOOLTIPS_TrackPosition (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    infoPtr->xTrackPos = (INT)LOWORD(lParam);
    infoPtr->yTrackPos = (INT)HIWORD(lParam);

    if (infoPtr->bTrackActive) {
	TRACE (tooltips, "[%d %d]\n",
	       infoPtr->xTrackPos, infoPtr->yTrackPos);

	TOOLTIPS_TrackShow (wndPtr, infoPtr);
    }

    return 0;
}


static LRESULT
TOOLTIPS_Update (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    if (infoPtr->nCurrentTool != -1)
	UpdateWindow (wndPtr->hwndSelf);

    return 0;
}


static LRESULT
TOOLTIPS_UpdateTipTextA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEA)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    TRACE (tooltips, "tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool text */
    toolPtr->hinst  = lpToolInfo->hinst;

    if ((lpToolInfo->hinst) && (HIWORD((INT)lpToolInfo->lpszText) == 0)){
	toolPtr->lpszText = (LPWSTR)lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACKA)
	    toolPtr->lpszText = LPSTR_TEXTCALLBACKW;
	else {
	    if (toolPtr->lpszText) {
		COMCTL32_Free (toolPtr->lpszText);
		toolPtr->lpszText = NULL;
	    }
	    if (lpToolInfo->lpszText) {
		INT len = lstrlenA (lpToolInfo->lpszText);
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
TOOLTIPS_UpdateTipTextW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFO_V1_SIZEW)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpToolInfo);
    if (nTool == -1)
	return 0;

    TRACE (tooltips, "tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool text */
    toolPtr->hinst  = lpToolInfo->hinst;

    if ((lpToolInfo->hinst) && (HIWORD((INT)lpToolInfo->lpszText) == 0)){
	toolPtr->lpszText = lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACKW)
	    toolPtr->lpszText = LPSTR_TEXTCALLBACKW;
	else {
	    if (toolPtr->lpszText) {
		COMCTL32_Free (toolPtr->lpszText);
		toolPtr->lpszText = NULL;
	    }
	    if (lpToolInfo->lpszText) {
		INT len = lstrlenW (lpToolInfo->lpszText);
		toolPtr->lpszText = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
		lstrcpyW (toolPtr->lpszText, lpToolInfo->lpszText);
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
TOOLTIPS_WindowFromPoint (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    return WindowFromPoint (*((LPPOINT)lParam));
}



static LRESULT
TOOLTIPS_Create (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr;
    NONCLIENTMETRICSA nclm;
    INT nResult;

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
    infoPtr->clrBk   = GetSysColor (COLOR_INFOBK);
    infoPtr->clrText = GetSysColor (COLOR_INFOTEXT);

    nclm.cbSize = sizeof(NONCLIENTMETRICSA);
    SystemParametersInfoA (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
    infoPtr->hFont = CreateFontIndirectA (&nclm.lfStatusFont);

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
	(INT) SendMessageA (wndPtr->parent->hwndSelf, WM_NOTIFYFORMAT,
				(WPARAM)wndPtr->hwndSelf, (LPARAM)NF_QUERY);
    if (nResult == NFR_ANSI)
	FIXME (tooltips, " -- WM_NOTIFYFORMAT returns: NFR_ANSI\n");
    else if (nResult == NFR_UNICODE)
	FIXME (tooltips, " -- WM_NOTIFYFORMAT returns: NFR_UNICODE\n");
    else
	FIXME (tooltips, " -- WM_NOTIFYFORMAT returns: error!\n");

    SetWindowPos (wndPtr->hwndSelf, HWND_TOP, 0, 0, 0, 0,
		    SWP_NOZORDER | SWP_HIDEWINDOW);

    return 0;
}


static LRESULT
TOOLTIPS_Destroy (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    TTTOOL_INFO *toolPtr;
    INT i;

    /* free tools */
    if (infoPtr->tools) {
	for (i = 0; i < infoPtr->uNumTools; i++) {
	    toolPtr = &infoPtr->tools[i];
	    if ((toolPtr->hinst) && (toolPtr->lpszText)) {
		if (toolPtr->lpszText != LPSTR_TEXTCALLBACKW)
		    COMCTL32_Free (toolPtr->lpszText);
	    }

	    /* remove subclassing */
	    if (toolPtr->uFlags & TTF_SUBCLASS) {
		LPTT_SUBCLASS_INFO lpttsi;

		if (toolPtr->uFlags & TTF_IDISHWND)
		    lpttsi = (LPTT_SUBCLASS_INFO)GetPropA ((HWND)toolPtr->uId, COMCTL32_aSubclass);
		else
		    lpttsi = (LPTT_SUBCLASS_INFO)GetPropA (toolPtr->hwnd, COMCTL32_aSubclass);

		if (lpttsi) {
		    SetWindowLongA ((HWND)toolPtr->uId, GWL_WNDPROC,
				      (LONG)lpttsi->wpOrigProc);
		    RemovePropA ((HWND)toolPtr->uId, COMCTL32_aSubclass);
		    COMCTL32_Free (&lpttsi);
		}
	    }
	}
	COMCTL32_Free (infoPtr->tools);
    }

    /* delete font */
    DeleteObject (infoPtr->hFont);

    /* free tool tips info data */
    COMCTL32_Free (infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_EraseBackground (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    RECT rect;
    HBRUSH hBrush;

    hBrush = CreateSolidBrush (infoPtr->clrBk);
    GetClientRect (wndPtr->hwndSelf, &rect);
    FillRect ((HDC)wParam, &rect, hBrush);
    DeleteObject (hBrush);

    return FALSE;
}


static LRESULT
TOOLTIPS_GetFont (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    return infoPtr->hFont;
}


static LRESULT
TOOLTIPS_MouseMessage (WND *wndPtr, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    TOOLTIPS_Hide (wndPtr, infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_NCCreate (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    wndPtr->dwStyle &= 0x0000FFFF;
    wndPtr->dwStyle |= (WS_POPUP | WS_BORDER | WS_CLIPSIBLINGS);

    return TRUE;
}


static LRESULT
TOOLTIPS_NCHitTest (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    INT nTool = (infoPtr->bTrackActive) ? infoPtr->nTrackTool : infoPtr->nTool;

    TRACE (tooltips, " nTool=%d\n", nTool);

    if ((nTool > -1) && (nTool < infoPtr->uNumTools)) {
	if (infoPtr->tools[nTool].uFlags & TTF_TRANSPARENT) {
	    TRACE (tooltips, "-- in transparent mode!\n");
	    return HTTRANSPARENT;
	}
    }

    return DefWindowProcA (wndPtr->hwndSelf, WM_NCHITTEST, wParam, lParam);
}


static LRESULT
TOOLTIPS_Paint (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = (wParam == 0) ? BeginPaint (wndPtr->hwndSelf, &ps) : (HDC)wParam;
    TOOLTIPS_Refresh (wndPtr, hdc);
    if (!wParam)
	EndPaint (wndPtr->hwndSelf, &ps);
    return 0;
}


static LRESULT
TOOLTIPS_SetFont (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    infoPtr->hFont = (HFONT)wParam;

    if ((LOWORD(lParam)) & (infoPtr->nCurrentTool != -1)) {
	FIXME (tooltips, "full redraw needed!\n");
    }

    return 0;
}


static LRESULT
TOOLTIPS_Timer (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    TRACE (tooltips, "timer %d (%x) expired!\n", wParam, wndPtr->hwndSelf);

    switch (wParam)
    {
	case ID_TIMERSHOW:
	    KillTimer (wndPtr->hwndSelf, ID_TIMERSHOW);
	    if (TOOLTIPS_CheckTool (wndPtr, TRUE) == infoPtr->nTool)
		TOOLTIPS_Show (wndPtr, infoPtr);
	    break;

	case ID_TIMERPOP:
	    TOOLTIPS_Hide (wndPtr, infoPtr);
	    break;

	case ID_TIMERLEAVE:
	    KillTimer (wndPtr->hwndSelf, ID_TIMERLEAVE);
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
TOOLTIPS_WinIniChange (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    NONCLIENTMETRICSA nclm;

    infoPtr->clrBk   = GetSysColor (COLOR_INFOBK);
    infoPtr->clrText = GetSysColor (COLOR_INFOTEXT);

    DeleteObject (infoPtr->hFont);
    nclm.cbSize = sizeof(NONCLIENTMETRICSA);
    SystemParametersInfoA (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
    infoPtr->hFont = CreateFontIndirectA (&nclm.lfStatusFont);

    return 0;
}


LRESULT CALLBACK
TOOLTIPS_SubclassProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPTT_SUBCLASS_INFO lpttsi =
	(LPTT_SUBCLASS_INFO)GetPropA (hwnd, COMCTL32_aSubclass);
    WND *wndPtr;
    TOOLTIPS_INFO *infoPtr;
    UINT nTool;

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
			SetTimer (wndPtr->hwndSelf, ID_TIMERSHOW,
				    infoPtr->nInitialTime, 0);
			TRACE (tooltips, "timer 1 started!\n");
		    }
		    else {
			TOOLTIPS_Hide (wndPtr, infoPtr);
			SetTimer (wndPtr->hwndSelf, ID_TIMERSHOW,
				infoPtr->nReshowTime, 0);
			TRACE (tooltips, "timer 2 started!\n");
		    }
		}
		if (infoPtr->nCurrentTool != -1) {
		    SetTimer (wndPtr->hwndSelf, ID_TIMERLEAVE, 100, 0);
		    TRACE (tooltips, "timer 3 started!\n");
		}
	    }
	    break;
    }

    return CallWindowProcA (lpttsi->wpOrigProc, hwnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK
TOOLTIPS_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
	case TTM_ACTIVATE:
	    return TOOLTIPS_Activate (wndPtr, wParam, lParam);

	case TTM_ADDTOOLA:
	    return TOOLTIPS_AddToolA (wndPtr, wParam, lParam);

	case TTM_ADDTOOLW:
	    return TOOLTIPS_AddToolW (wndPtr, wParam, lParam);

	case TTM_DELTOOLA:
	    return TOOLTIPS_DelToolA (wndPtr, wParam, lParam);

	case TTM_DELTOOLW:
	    return TOOLTIPS_DelToolW (wndPtr, wParam, lParam);

	case TTM_ENUMTOOLSA:
	    return TOOLTIPS_EnumToolsA (wndPtr, wParam, lParam);

	case TTM_ENUMTOOLSW:
	    return TOOLTIPS_EnumToolsW (wndPtr, wParam, lParam);

	case TTM_GETCURRENTTOOLA:
	    return TOOLTIPS_GetCurrentToolA (wndPtr, wParam, lParam);

	case TTM_GETCURRENTTOOLW:
	    return TOOLTIPS_GetCurrentToolW (wndPtr, wParam, lParam);

	case TTM_GETDELAYTIME:
	    return TOOLTIPS_GetDelayTime (wndPtr, wParam, lParam);

	case TTM_GETMARGIN:
	    return TOOLTIPS_GetMargin (wndPtr, wParam, lParam);

	case TTM_GETMAXTIPWIDTH:
	    return TOOLTIPS_GetMaxTipWidth (wndPtr, wParam, lParam);

	case TTM_GETTEXTA:
	    return TOOLTIPS_GetTextA (wndPtr, wParam, lParam);

	case TTM_GETTEXTW:
	    return TOOLTIPS_GetTextW (wndPtr, wParam, lParam);

	case TTM_GETTIPBKCOLOR:
	    return TOOLTIPS_GetTipBkColor (wndPtr, wParam, lParam);

	case TTM_GETTIPTEXTCOLOR:
	    return TOOLTIPS_GetTipTextColor (wndPtr, wParam, lParam);

	case TTM_GETTOOLCOUNT:
	    return TOOLTIPS_GetToolCount (wndPtr, wParam, lParam);

	case TTM_GETTOOLINFOA:
	    return TOOLTIPS_GetToolInfoA (wndPtr, wParam, lParam);

	case TTM_GETTOOLINFOW:
	    return TOOLTIPS_GetToolInfoW (wndPtr, wParam, lParam);

	case TTM_HITTESTA:
	    return TOOLTIPS_HitTestA (wndPtr, wParam, lParam);

	case TTM_HITTESTW:
	    return TOOLTIPS_HitTestW (wndPtr, wParam, lParam);

	case TTM_NEWTOOLRECTA:
	    return TOOLTIPS_NewToolRectA (wndPtr, wParam, lParam);

	case TTM_NEWTOOLRECTW:
	    return TOOLTIPS_NewToolRectW (wndPtr, wParam, lParam);

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

	case TTM_SETTOOLINFOA:
	    return TOOLTIPS_SetToolInfoA (wndPtr, wParam, lParam);

	case TTM_SETTOOLINFOW:
	    return TOOLTIPS_SetToolInfoW (wndPtr, wParam, lParam);

	case TTM_TRACKACTIVATE:
	    return TOOLTIPS_TrackActivate (wndPtr, wParam, lParam);

	case TTM_TRACKPOSITION:
	    return TOOLTIPS_TrackPosition (wndPtr, wParam, lParam);

	case TTM_UPDATE:
	    return TOOLTIPS_Update (wndPtr, wParam, lParam);

	case TTM_UPDATETIPTEXTA:
	    return TOOLTIPS_UpdateTipTextA (wndPtr, wParam, lParam);

	case TTM_UPDATETIPTEXTW:
	    return TOOLTIPS_UpdateTipTextW (wndPtr, wParam, lParam);

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
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
TOOLTIPS_Register (VOID)
{
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (TOOLTIPS_CLASSA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS;
    wndClass.lpfnWndProc   = (WNDPROC)TOOLTIPS_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TOOLTIPS_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = TOOLTIPS_CLASSA;
 
    RegisterClassA (&wndClass);
}


VOID
TOOLTIPS_Unregister (VOID)
{
    if (GlobalFindAtomA (TOOLTIPS_CLASSA))
	UnregisterClassA (TOOLTIPS_CLASSA, (HINSTANCE)NULL);
}

