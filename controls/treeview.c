/*
 * Treeview control
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
#include "treeview.h"
#include "heap.h"
#include "win.h"
#include "debug.h"


#define TREEVIEW_GetInfoPtr(wndPtr) ((TREEVIEW_INFO *)wndPtr->wExtra[0])


LRESULT WINAPI
TreeviewWindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
//	case WM_CREATE:
//	    return TREEVIEW_Create (wndPtr, wParam, lParam);

//	case WM_DESTROY:
//	    return TREEVIEW_Destroy (wndPtr, wParam, lParam);

//	case WM_PAINT:
//	    return TREEVIEW_Paint (wndPtr, wParam);

//	case WM_SETFONT:

//	case WM_TIMER:

//	case WM_WININICHANGE:

	default:
	    if (uMsg >= WM_USER)
		ERR (treeview, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void
TREEVIEW_Register (void)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (WC_TREEVIEW32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC32)TreeviewWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TREEVIEW_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = WC_TREEVIEW32A;
 
    RegisterClass32A (&wndClass);
}

