/*
 * Trackbar control
 *
 * Copyright 1998 Eric Kohl
 *
 * NOTES
 *   Development in progress. Author needed! Any volunteers?
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - Some messages.
 *   - display code.
 *   - user interaction.
 *   - tic handling.
 *   - All notifications.
 */

#include "windows.h"
#include "commctrl.h"
#include "trackbar.h"
#include "heap.h"
#include "win.h"
#include "debug.h"


#define TRACKBAR_GetInfoPtr(wndPtr) ((TRACKBAR_INFO *)wndPtr->wExtra[0])


static VOID
TRACKBAR_Refresh (WND *wndPtr, HDC32 hdc)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    RECT32 rcClient, rcChannel;

    GetClientRect32 (wndPtr->hwndSelf, &rcClient);

    /* draw channel */
    rcChannel = infoPtr->rcChannel;
    DrawEdge32 (hdc, &rcChannel, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
    if (wndPtr->dwStyle & TBS_ENABLESELRANGE) {
	/* fill the channel */
	HBRUSH32 hbr = CreateSolidBrush32 (RGB(255,255,255));
	FillRect32 (hdc, &rcChannel, hbr);
	DeleteObject32 (hbr);
    }

    /* draw ticks */
    if (!(wndPtr->dwStyle & TBS_NOTICKS)) {

    }

    /* draw thumb */
    if (!(wndPtr->dwStyle & TBS_NOTHUMB)) {

    }

    if (infoPtr->bFocus)
	DrawFocusRect32 (hdc, &rcClient);
}


static VOID
TRACKBAR_Calc (WND *wndPtr, TRACKBAR_INFO *infoPtr, LPRECT32 lpRect)
{
    INT32 cyChannel;

    if (wndPtr->dwStyle & TBS_ENABLESELRANGE)
	cyChannel = MAX(infoPtr->uThumbLen - 8, 4);
    else
	cyChannel = 4;

    /* calculate channel rect */
    if (wndPtr->dwStyle & TBS_VERT) {
	infoPtr->rcChannel.top = lpRect->top + 8;
	infoPtr->rcChannel.bottom = lpRect->bottom - 8;

	if (wndPtr->dwStyle & TBS_BOTH) {
	    infoPtr->rcChannel.left = (lpRect->bottom - cyChannel ) / 2;
	    infoPtr->rcChannel.right = (lpRect->bottom + cyChannel) / 2;
	}
	else if (wndPtr->dwStyle & TBS_LEFT) {
	    infoPtr->rcChannel.left = lpRect->left + 10;
	    infoPtr->rcChannel.right = infoPtr->rcChannel.left + cyChannel;
	}
	else {
	    /* TBS_RIGHT */
	    infoPtr->rcChannel.right = lpRect->right - 10;
	    infoPtr->rcChannel.left = infoPtr->rcChannel.right - cyChannel;
	}
    }
    else {
	infoPtr->rcChannel.left = lpRect->left + 8;
	infoPtr->rcChannel.right = lpRect->right - 8;

	if (wndPtr->dwStyle & TBS_BOTH) {
	    infoPtr->rcChannel.top = (lpRect->bottom - cyChannel ) / 2;
	    infoPtr->rcChannel.bottom = (lpRect->bottom + cyChannel) / 2;
	}
	else if (wndPtr->dwStyle & TBS_TOP) {
	    infoPtr->rcChannel.top = lpRect->top + 10;
	    infoPtr->rcChannel.bottom = infoPtr->rcChannel.top + cyChannel;
	}
	else {
	    /* TBS_BOTTOM */
	    infoPtr->rcChannel.bottom = lpRect->bottom - 10;
	    infoPtr->rcChannel.top = infoPtr->rcChannel.bottom - cyChannel;
	}
    }
}


static VOID
TRACKBAR_AlignBuddies (WND *wndPtr, TRACKBAR_INFO *infoPtr)
{
    HWND32 hwndParent = GetParent32 (wndPtr->hwndSelf);
    RECT32 rcSelf, rcBuddy;
    INT32 x, y;

    GetWindowRect32 (wndPtr->hwndSelf, &rcSelf);
    MapWindowPoints32 (HWND_DESKTOP, hwndParent, (LPPOINT32)&rcSelf, 2);

    /* align buddy left or above */
    if (infoPtr->hwndBuddyLA) {
	GetWindowRect32 (infoPtr->hwndBuddyLA, &rcBuddy);
	MapWindowPoints32 (HWND_DESKTOP, hwndParent, (LPPOINT32)&rcBuddy, 2);

	if (wndPtr->dwStyle & TBS_VERT) {
	    x = (infoPtr->rcChannel.right + infoPtr->rcChannel.left) / 2 -
		(rcBuddy.right - rcBuddy.left) / 2 + rcSelf.left;
	    y = rcSelf.top - (rcBuddy.bottom - rcBuddy.top);
	}
	else {
	    x = rcSelf.left - (rcBuddy.right - rcBuddy.left);
	    y = (infoPtr->rcChannel.bottom + infoPtr->rcChannel.top) / 2 -
		(rcBuddy.bottom - rcBuddy.top) / 2 + rcSelf.top;
	}

	SetWindowPos32 (infoPtr->hwndBuddyLA, 0, x, y, 0, 0,
			SWP_NOZORDER | SWP_NOSIZE);
    }


    /* align buddy right or below */
    if (infoPtr->hwndBuddyRB) {
	GetWindowRect32 (infoPtr->hwndBuddyRB, &rcBuddy);
	MapWindowPoints32 (HWND_DESKTOP, hwndParent, (LPPOINT32)&rcBuddy, 2);

	if (wndPtr->dwStyle & TBS_VERT) {
	    x = (infoPtr->rcChannel.right + infoPtr->rcChannel.left) / 2 -
		(rcBuddy.right - rcBuddy.left) / 2 + rcSelf.left;
	    y = rcSelf.bottom;
	}
	else {
	    x = rcSelf.right;
	    y = (infoPtr->rcChannel.bottom + infoPtr->rcChannel.top) / 2 -
		(rcBuddy.bottom - rcBuddy.top) / 2 + rcSelf.top;
	}
	SetWindowPos32 (infoPtr->hwndBuddyRB, 0, x, y, 0, 0,
			SWP_NOZORDER | SWP_NOSIZE);
    }
}


static LRESULT
TRACKBAR_ClearSel (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    infoPtr->nSelMin = 0;
    infoPtr->nSelMax = 0;

    if ((BOOL32)wParam) {
	HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
	TRACKBAR_Refresh (wndPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_ClearTics (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    if (infoPtr->tics) {
	FIXME (trackbar, "is this correct??\n");
	HeapFree (GetProcessHeap (), 0, infoPtr->tics);
	infoPtr->tics = NULL;
	infoPtr->uNumTics = 2;
    }

    if (wParam) {
	HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
	TRACKBAR_Refresh (wndPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_GetBuddy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    if (wParam)
	/* buddy is left or above */
	return (LRESULT)infoPtr->hwndBuddyLA;

    /* buddy is right or below */
    return (LRESULT) infoPtr->hwndBuddyRB;
}


static LRESULT
TRACKBAR_GetChannelRect (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    LPRECT32 lprc = (LPRECT32)lParam;

    if (lprc == NULL)
	return 0;

    lprc->left   = infoPtr->rcChannel.left;
    lprc->right  = infoPtr->rcChannel.right;
    lprc->bottom = infoPtr->rcChannel.bottom;
    lprc->top    = infoPtr->rcChannel.top;

    return 0;
}


static LRESULT
TRACKBAR_GetLineSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->nLineSize;
}


static LRESULT
TRACKBAR_GetNumTics (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    if (wndPtr->dwStyle & TBS_NOTICKS)
	return 0;

    return infoPtr->uNumTics;
}


static LRESULT
TRACKBAR_GetPageSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->nPageSize;
}


static LRESULT
TRACKBAR_GetPos (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->nPos;
}


// << TRACKBAR_GetPTics >>


static LRESULT
TRACKBAR_GetRangeMax (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->nRangeMax;
}


static LRESULT
TRACKBAR_GetRangeMin (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->nRangeMin;
}


static LRESULT
TRACKBAR_GetSelEnd (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->nSelMax;
}


static LRESULT
TRACKBAR_GetSelStart (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->nSelMin;
}


static LRESULT
TRACKBAR_GetThumbLength (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->uThumbLen;
}



// << TRACKBAR_GetThumbRect >>
//	case TBM_GETTIC:
//	case TBM_GETTICPOS:


static LRESULT
TRACKBAR_GetToolTips (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    if (wndPtr->dwStyle & TBS_TOOLTIPS)
	return (LRESULT)infoPtr->hwndToolTip;
    return 0;
}


//	case TBM_GETUNICODEFORMAT:


static LRESULT
TRACKBAR_SetBuddy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    HWND32 hwndTemp;

    if (wParam) {
	/* buddy is left or above */
	hwndTemp = infoPtr->hwndBuddyLA;
	infoPtr->hwndBuddyLA = (HWND32)lParam;

	FIXME (trackbar, "move buddy!\n");
    }
    else {
	/* buddy is right or below */
	hwndTemp = infoPtr->hwndBuddyRB;
	infoPtr->hwndBuddyRB = (HWND32)lParam;

	FIXME (trackbar, "move buddy!\n");
    }

    TRACKBAR_AlignBuddies (wndPtr, infoPtr);

    return (LRESULT)hwndTemp;
}


static LRESULT
TRACKBAR_SetLineSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    INT32 nTemp = infoPtr->nLineSize;

    infoPtr->nLineSize = (INT32)lParam;

    return nTemp;
}


static LRESULT
TRACKBAR_SetPageSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    INT32 nTemp = infoPtr->nPageSize;

    infoPtr->nPageSize = (INT32)lParam;

    return nTemp;
}


static LRESULT
TRACKBAR_SetPos (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    infoPtr->nPos = (INT32)HIWORD(lParam);

    if (infoPtr->nPos < infoPtr->nRangeMin)
	infoPtr->nPos = infoPtr->nRangeMin;

    if (infoPtr->nPos > infoPtr->nRangeMax)
	infoPtr->nPos = infoPtr->nRangeMax;

    if (wParam) {
	HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
	TRACKBAR_Refresh (wndPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_SetRange (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    infoPtr->nRangeMin = (INT32)LOWORD(lParam);
    infoPtr->nRangeMax = (INT32)HIWORD(lParam);

    if (infoPtr->nPos < infoPtr->nRangeMin)
	infoPtr->nPos = infoPtr->nRangeMin;

    if (infoPtr->nPos > infoPtr->nRangeMax)
	infoPtr->nPos = infoPtr->nRangeMax;

    if (wParam) {
	HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
	TRACKBAR_Refresh (wndPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_SetRangeMax (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    infoPtr->nRangeMax = (INT32)lParam;
    if (infoPtr->nPos > infoPtr->nRangeMax)
	infoPtr->nPos = infoPtr->nRangeMax;

    if (wParam) {
	HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
	TRACKBAR_Refresh (wndPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_SetRangeMin (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    infoPtr->nRangeMin = (INT32)lParam;
    if (infoPtr->nPos < infoPtr->nRangeMin)
	infoPtr->nPos = infoPtr->nRangeMin;

    if (wParam) {
	HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
	TRACKBAR_Refresh (wndPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_SetSel (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    if (!wndPtr->dwStyle & TBS_ENABLESELRANGE)
	return 0;

    infoPtr->nSelMin = (INT32)LOWORD(lParam);
    infoPtr->nSelMax = (INT32)HIWORD(lParam);

    if (infoPtr->nSelMin < infoPtr->nRangeMin)
	infoPtr->nSelMin = infoPtr->nRangeMin;
    if (infoPtr->nSelMax > infoPtr->nRangeMax)
	infoPtr->nSelMax = infoPtr->nRangeMax;

    if (wParam) {
	HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
	TRACKBAR_Refresh (wndPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_SetSelEnd (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    if (!wndPtr->dwStyle & TBS_ENABLESELRANGE)
	return 0;

    infoPtr->nSelMax = (INT32)lParam;
    if (infoPtr->nSelMax > infoPtr->nRangeMax)
	infoPtr->nSelMax = infoPtr->nRangeMax;

    if (wParam) {
	HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
	TRACKBAR_Refresh (wndPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_SetSelStart (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    if (!wndPtr->dwStyle & TBS_ENABLESELRANGE)
	return 0;

    infoPtr->nSelMin = (INT32)lParam;
    if (infoPtr->nSelMin < infoPtr->nRangeMin)
	infoPtr->nSelMin = infoPtr->nRangeMin;

    if (wParam) {
	HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
	TRACKBAR_Refresh (wndPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_SetThumbLength (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    if (wndPtr->dwStyle & TBS_FIXEDLENGTH)
	infoPtr->uThumbLen = (UINT32)wParam;

    return 0;
}


static LRESULT
TRACKBAR_SetTic (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    INT32 nPos = (INT32)lParam;

    if (nPos < infoPtr->nRangeMin)
	return FALSE;
    if (nPos > infoPtr->nRangeMax)
	return FALSE;

    FIXME (trackbar, "%d - empty stub!\n", nPos);

    return TRUE;
}


//	case TBM_SETTICFREQ:


static LRESULT
TRACKBAR_SetTipSide (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    INT32 fTemp = infoPtr->fLocation;

    infoPtr->fLocation = (INT32)wParam;

    return fTemp;
}


static LRESULT
TRACKBAR_SetToolTips (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    infoPtr->hwndToolTip = (HWND32)wParam;

    return 0;
}


//	case TBM_SETUNICODEFORMAT:




static LRESULT
TRACKBAR_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr;

    infoPtr = (TRACKBAR_INFO *)HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
					  sizeof(TRACKBAR_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;


    /* default values */
    infoPtr->nRangeMin = 0;
    infoPtr->nRangeMax = 100;
    infoPtr->nLineSize = 1;
    infoPtr->nPageSize = 20;
    infoPtr->nSelMin   = 0;
    infoPtr->nSelMax   = 0;
    infoPtr->nPos      = 0;
    infoPtr->uThumbLen = 23;   /* initial thumb length */
    infoPtr->uNumTics  = 2;    /* default start and end tic */


    return 0;
}


static LRESULT
TRACKBAR_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);




    HeapFree (GetProcessHeap (), 0, infoPtr);

    return 0;
}


static LRESULT
TRACKBAR_KillFocus (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    HDC32 hdc;

    infoPtr->bFocus = FALSE;
    hdc = GetDC32 (wndPtr->hwndSelf);
    TRACKBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);
    InvalidateRect32 (wndPtr->hwndSelf, NULL, TRUE);

    return 0;
}


static LRESULT
TRACKBAR_LButtonDown (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    SetFocus32 (wndPtr->hwndSelf);

    return 0;
}


static LRESULT
TRACKBAR_Paint (WND *wndPtr, WPARAM32 wParam)
{
    HDC32 hdc;
    PAINTSTRUCT32 ps;

    hdc = wParam==0 ? BeginPaint32 (wndPtr->hwndSelf, &ps) : (HDC32)wParam;
    TRACKBAR_Refresh (wndPtr, hdc);
    if(!wParam)
	EndPaint32 (wndPtr->hwndSelf, &ps);
    return 0;
}


static LRESULT
TRACKBAR_SetFocus (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    HDC32 hdc;

    infoPtr->bFocus = TRUE;

    hdc = GetDC32 (wndPtr->hwndSelf);
    TRACKBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return 0;
}


static LRESULT
TRACKBAR_Size (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    RECT32 rcClient;

    GetClientRect32 (wndPtr->hwndSelf, &rcClient);

    TRACKBAR_Calc (wndPtr, infoPtr, &rcClient);
    TRACKBAR_AlignBuddies (wndPtr, infoPtr);

    return 0;
}


// << TRACKBAR_Timer >>
// << TRACKBAR_WinIniChange >>


LRESULT WINAPI
TRACKBAR_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
	case TBM_CLEARSEL:
	    return TRACKBAR_ClearSel (wndPtr, wParam, lParam);

	case TBM_CLEARTICS:
	    return TRACKBAR_ClearTics (wndPtr, wParam, lParam);

	case TBM_GETBUDDY:
	    return TRACKBAR_GetBuddy (wndPtr, wParam, lParam);

	case TBM_GETCHANNELRECT:
	    return TRACKBAR_GetChannelRect (wndPtr, wParam, lParam);

	case TBM_GETLINESIZE:
	    return TRACKBAR_GetLineSize (wndPtr, wParam, lParam);

	case TBM_GETNUMTICS:
	    return TRACKBAR_GetNumTics (wndPtr, wParam, lParam);

	case TBM_GETPAGESIZE:
	    return TRACKBAR_GetPageSize (wndPtr, wParam, lParam);

	case TBM_GETPOS:
	    return TRACKBAR_GetPos (wndPtr, wParam, lParam);

//	case TBM_GETPTICS:

	case TBM_GETRANGEMAX:
	    return TRACKBAR_GetRangeMax (wndPtr, wParam, lParam);

	case TBM_GETRANGEMIN:
	    return TRACKBAR_GetRangeMin (wndPtr, wParam, lParam);

	case TBM_GETSELEND:
	    return TRACKBAR_GetSelEnd (wndPtr, wParam, lParam);

	case TBM_GETSELSTART:
	    return TRACKBAR_GetSelStart (wndPtr, wParam, lParam);

	case TBM_GETTHUMBLENGTH:
	    return TRACKBAR_GetThumbLength (wndPtr, wParam, lParam);

//	case TBM_GETTHUMBRECT:
//	case TBM_GETTIC:
//	case TBM_GETTICPOS:

	case TBM_GETTOOLTIPS:
	    return TRACKBAR_GetToolTips (wndPtr, wParam, lParam);

//	case TBM_GETUNICODEFORMAT:

	case TBM_SETBUDDY:
	    return TRACKBAR_SetBuddy (wndPtr, wParam, lParam);

	case TBM_SETLINESIZE:
	    return TRACKBAR_SetLineSize (wndPtr, wParam, lParam);

	case TBM_SETPAGESIZE:
	    return TRACKBAR_SetPageSize (wndPtr, wParam, lParam);

	case TBM_SETPOS:
	    return TRACKBAR_SetPos (wndPtr, wParam, lParam);

	case TBM_SETRANGE:
	    return TRACKBAR_SetRange (wndPtr, wParam, lParam);

	case TBM_SETRANGEMAX:
	    return TRACKBAR_SetRangeMax (wndPtr, wParam, lParam);

	case TBM_SETRANGEMIN:
	    return TRACKBAR_SetRangeMin (wndPtr, wParam, lParam);

	case TBM_SETSEL:
	    return TRACKBAR_SetSel (wndPtr, wParam, lParam);

	case TBM_SETSELEND:
	    return TRACKBAR_SetSelEnd (wndPtr, wParam, lParam);

	case TBM_SETSELSTART:
	    return TRACKBAR_SetSelStart (wndPtr, wParam, lParam);

	case TBM_SETTHUMBLENGTH:
	    return TRACKBAR_SetThumbLength (wndPtr, wParam, lParam);

	case TBM_SETTIC:
	    return TRACKBAR_SetTic (wndPtr, wParam, lParam);

//	case TBM_SETTICFREQ:

	case TBM_SETTIPSIDE:
	    return TRACKBAR_SetTipSide (wndPtr, wParam, lParam);

	case TBM_SETTOOLTIPS:
	    return TRACKBAR_SetToolTips (wndPtr, wParam, lParam);

//	case TBM_SETUNICODEFORMAT:


//	case WM_CAPTURECHANGED:

	case WM_CREATE:
	    return TRACKBAR_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return TRACKBAR_Destroy (wndPtr, wParam, lParam);

//	case WM_ENABLE:

//	case WM_ERASEBKGND:
//	    return 0;

	case WM_GETDLGCODE:
	    return DLGC_WANTARROWS;

//	case WM_KEYDOWN:

//	case WM_KEYUP:

	case WM_KILLFOCUS:
	    return TRACKBAR_KillFocus (wndPtr, wParam, lParam);

	case WM_LBUTTONDOWN:
	    return TRACKBAR_LButtonDown (wndPtr, wParam, lParam);

//	case WM_LBUTTONUP:

//	case WM_MOUSEMOVE:
//	    return TRACKBAR_MouseMove (wndPtr, wParam, lParam);

	case WM_PAINT:
	    return TRACKBAR_Paint (wndPtr, wParam);

	case WM_SETFOCUS:
	    return TRACKBAR_SetFocus (wndPtr, wParam, lParam);

	case WM_SIZE:
	    return TRACKBAR_Size (wndPtr, wParam, lParam);

//	case WM_TIMER:

//	case WM_WININICHANGE:

	default:
	    if (uMsg >= WM_USER)
		ERR (trackbar, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void
TRACKBAR_Register (void)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (TRACKBAR_CLASS32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC32)TRACKBAR_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TRACKBAR_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = (HBRUSH32)(COLOR_3DFACE + 1);
    wndClass.lpszClassName = TRACKBAR_CLASS32A;
 
    RegisterClass32A (&wndClass);
}
