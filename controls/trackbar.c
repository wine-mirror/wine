/*
 * Trackbar control
 *
 * Copyright 1998 Eric Kohl
 *
 * NOTES
 *   This is just a dummy control. An author is needed! Any volunteers?
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - All messages.
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
    RECT32 rect;

    GetClientRect32 (wndPtr->hwndSelf, &rect);

    /* draw channel */
    DrawEdge32 (hdc, &infoPtr->rcChannel, EDGE_SUNKEN, BF_RECT);

    /* draw thumb */
    if (!(wndPtr->dwStyle & TBS_NOTHUMB)) {

    }

    /* draw ticks */
    if (!(wndPtr->dwStyle & TBS_NOTICKS)) {

    }

    if (infoPtr->bFocus)
	DrawFocusRect32 (hdc, &rect);
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


// << TRACKBAR_ClearTics >>
// << TRACKBAR_GetChannelRect >>


static LRESULT
TRACKBAR_GetLineSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->nLineSize;
}


// << TRACKBAR_GetNumTics >>


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

    return infoPtr->nThumbLen;
}






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
    infoPtr->nThumbLen = 23;   /* initial thumb length */


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
    RECT32 rect;

    GetClientRect32 (wndPtr->hwndSelf, &rect);

    /* calculate channel rect */
    if (wndPtr->dwStyle & TBS_VERT) {
	infoPtr->rcChannel.top = rect.top + 8;
	infoPtr->rcChannel.bottom = rect.bottom - 8;

	/* FIXME */
	infoPtr->rcChannel.left = rect.left + 10;
	infoPtr->rcChannel.right = rect.left + 14;

    }
    else {
	infoPtr->rcChannel.left = rect.left + 8;
	infoPtr->rcChannel.right = rect.right - 8;

	/* FIXME */
	if (wndPtr->dwStyle & TBS_BOTH) {
	    infoPtr->rcChannel.top = rect.bottom / 2 - 2;
	    infoPtr->rcChannel.bottom = rect.bottom / 2 + 2;

	}
	else if (wndPtr->dwStyle & TBS_TOP) {
	    infoPtr->rcChannel.top = rect.top + 10;
	    infoPtr->rcChannel.bottom = rect.top + 14;

	}
	else {
	    /* TBS_BOTTOM */
	    infoPtr->rcChannel.top = rect.bottom - 14;
	    infoPtr->rcChannel.bottom = rect.bottom - 10;
	}
    }

    return 0;
}


// << TRACKBAR_Timer >>
// << TRACKBAR_WinIniChange >>


LRESULT WINAPI
TrackbarWindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
	case TBM_CLEARSEL:
	    return TRACKBAR_ClearSel (wndPtr, wParam, lParam);

//	case TBM_CLEARTICS:
//	case TBM_GETBUDDY:
//	case TBM_GETCHANNELRECT:

	case TBM_GETLINESIZE:
	    return TRACKBAR_GetLineSize (wndPtr, wParam, lParam);

//	case TBM_GETNUMTICS:

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
//	case TBM_GETTOOLTIPS:
//	case TBM_GETUNICODEFORMAT:
//	case TBM_SETBUDDY:
//	case TBM_SETPAGESIZE:
//	case TBM_SETPOS:
//	case TBM_SETRANGE:
//	case TBM_SETRANGEMAX:
//	case TBM_SETRANGEMIN:
//	case TBM_SETSEL:
//	case TBM_SETSELEND:
//	case TBM_SETSELSTART:
//	case TBM_SETTHUMBLENGTH:
//	case TBM_SETTIC:
//	case TBM_SETTICFREQ:
//	case TBM_SETTIPSIDE:
//	case TBM_SETTOOLTIPS:
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
    wndClass.lpfnWndProc   = (WNDPROC32)TrackbarWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TRACKBAR_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = (HBRUSH32)(COLOR_3DFACE + 1);
    wndClass.lpszClassName = TRACKBAR_CLASS32A;
 
    RegisterClass32A (&wndClass);
}

