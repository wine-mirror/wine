/*
 * Listview control
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
#include "listview.h"
#include "heap.h"
#include "win.h"
#include "debug.h"


#define LISTVIEW_GetInfoPtr(wndPtr) ((LISTVIEW_INFO *)wndPtr->wExtra[0])


static VOID
LISTVIEW_Refresh (WND *wndPtr, HDC32 hdc)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);



}



static LRESULT
LISTVIEW_SetBkColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    if (!(infoPtr)) return FALSE;

    /* set background color */
    infoPtr->clrBk = (COLORREF)lParam;

    return TRUE;
}


static LRESULT
LISTVIEW_SetImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    fprintf (stderr, "LISTVIEW: SetImageList (0x%08x 0x%08lx)\n",
	     wParam, lParam);

    return 0;
}


static LRESULT
LISTVIEW_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    /* initialize info structure */
    infoPtr->clrBk = CLR_NONE;

    return 0;
}


static LRESULT
LISTVIEW_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);


    return 0;
}


static LRESULT
LISTVIEW_EraseBackground (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    if (infoPtr->clrBk == CLR_NONE) {
	return SendMessage32A (GetParent32 (wndPtr->hwndSelf),
			       WM_ERASEBKGND, wParam, lParam);
    }
    else {
	RECT32 rect;
	HBRUSH32 hBrush = CreateSolidBrush32 (infoPtr->clrBk);
	GetClientRect32 (wndPtr->hwndSelf, &rect);
	FillRect32 ((HDC32)wParam, &rect, hBrush);
	DeleteObject32 (hBrush);
	return FALSE;
    }
    return FALSE;
}


static LRESULT
LISTVIEW_NCCreate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (LISTVIEW_INFO *)HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
                                   sizeof(LISTVIEW_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
	ERR (listview, "could not allocate info memory!\n");
	return 0;
    }

    if ((LISTVIEW_INFO*)wndPtr->wExtra[0] != infoPtr) {
	ERR (listview, "pointer assignment error!\n");
	return 0;
    }

    return DefWindowProc32A (wndPtr->hwndSelf, WM_NCCREATE, wParam, lParam);
}


static LRESULT
LISTVIEW_NCDestroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);




    /* free list view info data */
    HeapFree (GetProcessHeap (), 0, infoPtr);

    return 0;
}


static LRESULT
LISTVIEW_Paint (WND *wndPtr, WPARAM32 wParam)
{
    HDC32 hdc;
    PAINTSTRUCT32 ps;

    hdc = wParam==0 ? BeginPaint32 (wndPtr->hwndSelf, &ps) : (HDC32)wParam;
    LISTVIEW_Refresh (wndPtr, hdc);
    if (!wParam)
	EndPaint32 (wndPtr->hwndSelf, &ps);
    return 0;
}


LRESULT WINAPI
ListviewWindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {

	case LVM_SETBKCOLOR:
	    return LISTVIEW_SetBkColor (wndPtr, wParam, lParam);

	
	case LVM_SETIMAGELIST:
	    return LISTVIEW_SetImageList (wndPtr, wParam, lParam);




//	case WM_CHAR:
//	case WM_COMMAND:

	case WM_CREATE:
	    return LISTVIEW_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return LISTVIEW_Destroy (wndPtr, wParam, lParam);

	case WM_ERASEBKGND:
	    return LISTVIEW_EraseBackground (wndPtr, wParam, lParam);

//	case WM_GETDLGCODE:
//	case WM_GETFONT:
//	case WM_HSCROLL:

//	case WM_MOUSEMOVE:
//	    return LISTVIEW_MouseMove (wndPtr, wParam, lParam);

	case WM_NCCREATE:
	    return LISTVIEW_NCCreate (wndPtr, wParam, lParam);

	case WM_NCDESTROY:
	    return LISTVIEW_NCDestroy (wndPtr, wParam, lParam);

//	case WM_NOTIFY:

	case WM_PAINT:
	    return LISTVIEW_Paint (wndPtr, wParam);

//	case WM_RBUTTONDOWN:
//	case WM_SETFOCUS:
//	case WM_SETFONT:
//	case WM_SETREDRAW:
//	case WM_TIMER:
//	case WM_VSCROLL:
//	case WM_WINDOWPOSCHANGED:
//	case WM_WININICHANGE:

	default:
	    if (uMsg >= WM_USER)
		ERR (listview, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void
LISTVIEW_Register (void)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (WC_LISTVIEW32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC32)ListviewWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(LISTVIEW_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = (HBRUSH32)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = WC_LISTVIEW32A;
 
    RegisterClass32A (&wndClass);
}

