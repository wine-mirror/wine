/*
 * Flat Scrollbar control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 1998 Alex Priem
 *
 * NOTES
 *   This is just a dummy control. An author is needed! Any volunteers?
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - All messages.
 *   - All notifications.
 *
 */

#include "winbase.h"
#include "commctrl.h"
#include "flatsb.h" 
#include "debug.h"


#define FlatSB_GetInfoPtr(hwnd) ((FLATSB_INFO*)GetWindowLongA (hwnd, 0))


BOOL WINAPI 
FlatSB_EnableScrollBar(HWND hwnd, INT dummy, UINT dummy2)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

BOOL WINAPI 
FlatSB_ShowScrollBar(HWND hwnd, INT code, BOOL flag)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

BOOL WINAPI 
FlatSB_GetScrollRange(HWND hwnd, INT code, LPINT min, LPINT max)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

BOOL WINAPI 
FlatSB_GetScrollInfo(HWND hwnd, INT code, LPSCROLLINFO info)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

INT WINAPI 
FlatSB_GetScrollPos(HWND hwnd, INT code)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

BOOL WINAPI 
FlatSB_GetScrollProp(HWND hwnd, INT propIndex, LPINT prop)
{
    FIXME(commctrl,"stub\n");
    return 0;
}


INT WINAPI 
FlatSB_SetScrollPos(HWND hwnd, INT code, INT pos, BOOL fRedraw)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

INT WINAPI 
FlatSB_SetScrollInfo(HWND hwnd, INT code, LPSCROLLINFO info, BOOL fRedraw)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

INT WINAPI 
FlatSB_SetScrollRange(HWND hwnd, INT code, INT min, INT max, BOOL fRedraw)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

BOOL WINAPI 
FlatSB_SetScrollProp(HWND hwnd, UINT index, INT newValue, BOOL flag)
{
    FIXME(commctrl,"stub\n");
    return 0;
}


BOOL WINAPI InitializeFlatSB(HWND hwnd)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

HRESULT WINAPI UninitializeFlatSB(HWND hwnd)
{
    FIXME(commctrl,"stub\n");
    return 0;
}



static LRESULT
FlatSB_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    return 0;
}


static LRESULT
FlatSB_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    return 0;
}




LRESULT WINAPI
FlatSB_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {

	case WM_CREATE:
	    return FlatSB_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return FlatSB_Destroy (hwnd, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR (datetime, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
FLATSB_Register (VOID)
{
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (FLATSB_CLASSA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC)FlatSB_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(FLATSB_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = FLATSB_CLASSA;
 
    RegisterClassA (&wndClass);
}


VOID
FLATSB_Unregister (VOID)
{
    if (GlobalFindAtomA (FLATSB_CLASSA))
	UnregisterClassA (FLATSB_CLASSA, (HINSTANCE)NULL);
}

