/*
 * Flat Scrollbar control
 *
 * Copyright 1998 Eric Kohl
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

#include "commctrl.h"
#include "flatsb.h" 
#include "win.h"
#include "debug.h"


#define FlatSB_GetInfoPtr(wndPtr) ((FLATSB_INFO*)wndPtr->wExtra[0])


BOOL32 WINAPI 
FlatSB_EnableScrollBar(HWND32 hwnd, INT32 dummy, UINT32 dummy2)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

BOOL32 WINAPI 
FlatSB_ShowScrollBar(HWND32 hwnd, INT32 code, BOOL32 flag)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

BOOL32 WINAPI 
FlatSB_GetScrollRange(HWND32 hwnd, INT32 code, LPINT32 min, LPINT32 max)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

BOOL32 WINAPI 
FlatSB_GetScrollInfo(HWND32 hwnd, INT32 code, LPSCROLLINFO info)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

INT32 WINAPI 
FlatSB_GetScrollPos(HWND32 hwnd, INT32 code)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

BOOL32 WINAPI 
FlatSB_GetScrollProp(HWND32 hwnd, INT32 propIndex, LPINT32 prop)
{
    FIXME(commctrl,"stub\n");
    return 0;
}


INT32 WINAPI 
FlatSB_SetScrollPos(HWND32 hwnd, INT32 code, INT32 pos, BOOL32 fRedraw)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

INT32 WINAPI 
FlatSB_SetScrollInfo(HWND32 hwnd, INT32 code, LPSCROLLINFO info, BOOL32 fRedraw)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

INT32 WINAPI 
FlatSB_SetScrollRange(HWND32 hwnd, INT32 code, INT32 min, INT32 max, BOOL32 fRedraw)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

BOOL32 WINAPI 
FlatSB_SetScrollProp(HWND32 hwnd, UINT32 index, INT32 newValue, BOOL32 flag)
{
    FIXME(commctrl,"stub\n");
    return 0;
}


BOOL32 WINAPI InitializeFlatSB(HWND32 hwnd)
{
    FIXME(commctrl,"stub\n");
    return 0;
}

HRESULT WINAPI UninitializeFlatSB(HWND32 hwnd)
{
    FIXME(commctrl,"stub\n");
    return 0;
}



static LRESULT
FlatSB_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    return 0;
}


static LRESULT
FlatSB_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    return 0;
}




LRESULT WINAPI
FlatSB_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {

	case WM_CREATE:
	    return FlatSB_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return FlatSB_Destroy (wndPtr, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR (datetime, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
FLATSB_Register (VOID)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (FLATSB_CLASS32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC32)FlatSB_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(FLATSB_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = (HBRUSH32)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = FLATSB_CLASS32A;
 
    RegisterClass32A (&wndClass);
}


VOID
FLATSB_Unregister (VOID)
{
    if (GlobalFindAtom32A (FLATSB_CLASS32A))
	UnregisterClass32A (FLATSB_CLASS32A, (HINSTANCE32)NULL);
}

