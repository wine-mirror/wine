/*
 * Tool tip control
 *
 * Copyright 1998 Eric Kohl
 *
 * NOTES
 *   PLEASE don't try to improve or change this code right now. Many
 *   features are still missing, but I'm working on it. I want to avoid
 *   any confusion. This note will be removed as soon as most of the
 *   features are implemented.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - Most messages.
 *   - All notifications.
 *
 * Testing:
 *   - Run tests using Waite Group Windows95 API Bible Volume 2.
 *     The second cdrom contains executables enumtools.exe ...
 *   - additional features.
 *
 * FIXME:
 *   - DelTool32A incomplete.
 *   - GetCurrentTool32A incomplete.
 */

#include "windows.h"
#include "commctrl.h"
#include "tooltips.h"
#include "heap.h"
#include "win.h"
#include "debug.h"


#define TOOLTIPS_GetInfoPtr(wndPtr) ((TOOLTIPS_INFO *)wndPtr->wExtra[0])


static INT32
TOOLTIPS_GetIndexFromInfoA (TOOLTIPS_INFO *infoPtr, LPTOOLINFOA lpToolInfo)
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

/*
static INT32
TOOLTIPS_GetIndexFromPoint (TOOLTIPS_INFO *infoPtr, LPPOINT32 lpPt)
{
    TTTOOL_INFO *toolPtr;
    INT32 nIndex;

    for (nIndex = 0; nIndex < infoPtr->uNumTools; nIndex++) {
	toolPtr = &infoPtr->tools[nIndex];

	if (lpToolInfo->uFlags & TTF_IDISHWND) {
	    if (PtInRect (
	}
	else {

	    return nIndex;
	}
    }

    return -1;
}
*/

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
    LPTOOLINFOA lpToolInfo = (LPTOOLINFOA)lParam;
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

    if (lpToolInfo->cbSize >= sizeof(TOOLINFOA))
	toolPtr->lParam = lpToolInfo->lParam;


    return TRUE;
}


// << TOOLTIPS_AddTool32W >>


static LRESULT
TOOLTIPS_DelTool32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTOOLINFOA lpToolInfo = (LPTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nIndex;

    if (lpToolInfo == NULL) return 0;
    if (infoPtr->uNumTools == 0) return 0;

    nIndex = TOOLTIPS_GetIndexFromInfoA (infoPtr, lpToolInfo);
    if (nIndex == -1) return 0;

    TRACE (tooltips, "index=%d\n", nIndex);


/*    delete tool from tool list */
/*
    toolPtr = &infoPtr->tools[nIndex]; 

    // delete text string
    if ((toolPtr->hinst) && (toolPtr->lpszText)) {
	if (toolPtr->lpszText != LPSTR_TEXTCALLBACK32A)
	    HeapFree (GetProcessHeap (), 0, toolPtr->lpszText);
    }


*/
    return 0;
}


// << TOOLTIPS_DelTool32W >>


static LRESULT
TOOLTIPS_EnumTools32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    UINT32 uIndex = (UINT32)wParam;
    LPTOOLINFOA lpToolInfo = (LPTOOLINFOA)lParam;
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

    if (lpToolInfo->cbSize >= sizeof(TOOLINFOA))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


// << TOOLTIPS_EnumTools32W >>


static LRESULT
TOOLTIPS_GetCurrentTool32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTOOLINFOA lpti = (LPTOOLINFOA)lParam;

    if (lpti) {
	if (infoPtr->iCurrentTool > -1) {
	    /* FIXME */

	    return TRUE;
	}
	else
	    return FALSE;
    }
    else
	return (infoPtr->iCurrentTool != -1);

    return FALSE;
}


// << TOOLTIPS_GetCurrentTool32W >>
// << TOOLTIPS_GetDelayTime >>
// << TOOLTIPS_GetMargin >>


static LRESULT
TOOLTIPS_GetMaxTipWidth (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    return infoPtr->iMaxTipWidth;
}


static LRESULT
TOOLTIPS_GetText32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTOOLINFOA lpti = (LPTOOLINFOA)lParam;
    INT32 nIndex;

    if (!(lpti)) return 0;
    if (lpti->cbSize < sizeof(TOOLINFOA)) return 0;

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
    LPTOOLINFOA lpToolInfo = (LPTOOLINFOA)lParam;
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

    if (lpToolInfo->cbSize >= sizeof(TOOLINFOA))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


// << TOOLTIPS_GetToolInfo32W >>
// << TOOLTIPS_HitTest32A >>
// << TOOLTIPS_HitTest32W >>


static LRESULT
TOOLTIPS_NewToolRect32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPTOOLINFOA lpti = (LPTOOLINFOA)lParam;
    INT32 nIndex;

    if (!(lpti)) return 0;
    if (lpti->cbSize < sizeof(TOOLINFOA)) return 0;

    nIndex = TOOLTIPS_GetIndexFromInfoA (infoPtr, lpti);
    if (nIndex == -1) return 0;

    infoPtr->tools[nIndex].rect = lpti->rect;

    return 0;
}


// << TOOLTIPS_NewToolRect32W >>
// << TOOLTIPS_Pop >>


static LRESULT
TOOLTIPS_RelayEvent (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    LPMSG16 lpMsg = (LPMSG16)lParam;

    if (lpMsg == NULL) return 0;

    if (lpMsg->message == WM_MOUSEMOVE) {
	FIXME (tooltips, "WM_MOUSEMOVE (%d %d)\n", lpMsg->pt.x, lpMsg->pt.y);
    }

//    FIXME (tooltips, "empty stub!\n");

    return 0;
}


// << TOOLTIPS_SetDelayTime >>
// << TOOLTIPS_SetMargin >>


static LRESULT
TOOLTIPS_SetMaxTipWidth (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);
    INT32 iTemp = infoPtr->iMaxTipWidth;

    infoPtr->iMaxTipWidth = (INT32)lParam;

    return iTemp;
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
    LPTOOLINFOA lpToolInfo = (LPTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT32 nIndex;

    if (lpToolInfo == NULL) return 0;

    nIndex = TOOLTIPS_GetIndexFromInfoA (infoPtr, lpToolInfo);
    if (nIndex == -1) return 0;

    TRACE (tooltips, "index=%d\n", nIndex);

    toolPtr = &infoPtr->tools[nIndex];

    /* copy tool data */
    toolPtr->uFlags = lpToolInfo->uFlags;
    toolPtr->hwnd   = lpToolInfo->hwnd;
    toolPtr->uId    = lpToolInfo->uId;
    toolPtr->rect   = lpToolInfo->rect;
    toolPtr->hinst  = lpToolInfo->hinst;

    if (lpToolInfo->lpszText) {
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

    if (lpToolInfo->cbSize >= sizeof(TOOLINFOA))
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
    infoPtr->hFont   = NULL;
    infoPtr->iMaxTipWidth = -1;
    infoPtr->iCurrentTool = -1;

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
TOOLTIPS_GetFont (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(wndPtr);

    return infoPtr->hFont;
}


// << TOOLTIPS_MouseMove >>
// << TOOLTIPS_Paint >>


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


// << TOOLTIPS_Timer >>
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
//	case TTM_GETDELAYTIME:			/* 4.70 */
//	case TTM_GETMARGIN:			/* 4.70 */

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
//	case TTM_HITTEST32A:
//	case TTM_HITTEST32W:

	case TTM_NEWTOOLRECT32A:
	    return TOOLTIPS_NewToolRect32A (wndPtr, wParam, lParam);

//	case TTM_NEWTOOLRECT32W:
//	case TTM_POP:				/* 4.70 */

	case TTM_RELAYEVENT:
	    return TOOLTIPS_RelayEvent (wndPtr, wParam, lParam);

//	case TTM_SETDELAYTIME:			/* 4.70 */
//	case TTM_SETMARGIN:			/* 4.70 */

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

	case WM_GETFONT:
	    return TOOLTIPS_GetFont (wndPtr, wParam, lParam);

//	case WM_MOUSEMOVE:
//	    return TOOLTIPS_MouseMove (wndPtr, wParam, lParam);

//	case WM_PAINT:
//	    return TOOLTIPS_Paint (wndPtr, wParam);

	case WM_SETFONT:
	    return TOOLTIPS_SetFont (wndPtr, wParam, lParam);

//	case WM_TIMER:

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

