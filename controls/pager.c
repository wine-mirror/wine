/*
 * Pager control
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
#include "pager.h"
#include "heap.h"
#include "win.h"
#include "debug.h"


#define PAGER_GetInfoPtr(wndPtr) ((PAGER_INFO *)wndPtr->wExtra[0])


LRESULT WINAPI
PagerWindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
//	case WM_CREATE:
//	    return PAGER_Create (wndPtr, wParam, lParam);

//	case WM_DESTROY:
//	    return PAGER_Destroy (wndPtr, wParam, lParam);

//	case WM_MOUSEMOVE:
//	    return PAGER_MouseMove (wndPtr, wParam, lParam);

//	case WM_PAINT:
//	    return PAGER_Paint (wndPtr, wParam);


	default:
	    if (uMsg >= WM_USER)
		ERR (pager, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void
PAGER_Register (void)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (WC_PAGESCROLLER32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS;
    wndClass.lpfnWndProc   = (WNDPROC32)PagerWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(PAGER_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = WC_PAGESCROLLER32A;
 
    RegisterClass32A (&wndClass);
}

