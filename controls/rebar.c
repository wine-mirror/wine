/*
 * Rebar control
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
#include "rebar.h"
#include "heap.h"
#include "win.h"
#include "debug.h"


#define REBAR_GetInfoPtr(wndPtr) ((REBAR_INFO *)wndPtr->wExtra[0])


LRESULT WINAPI
RebarWindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
//	case WM_CREATE:
//	    return REBAR_Create (wndPtr, wParam, lParam);

//	case WM_DESTROY:
//	    return REBAR_Destroy (wndPtr, wParam, lParam);

//	case WM_GETFONT:

//	case WM_MOUSEMOVE:
//	    return REBAR_MouseMove (wndPtr, wParam, lParam);

//	case WM_PAINT:
//	    return REBAR_Paint (wndPtr, wParam);

//	case WM_SETFONT:

//	case WM_TIMER:

//	case WM_WININICHANGE:

	default:
	    if (uMsg >= WM_USER)
		ERR (rebar, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void
REBAR_Register (void)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (REBARCLASSNAME32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS;
    wndClass.lpfnWndProc   = (WNDPROC32)RebarWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(REBAR_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = REBARCLASSNAME32A;
 
    RegisterClass32A (&wndClass);
}

