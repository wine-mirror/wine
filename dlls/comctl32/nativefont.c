/*
 * Native Font control
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

#include "commctrl.h"
#include "nativefont.h"
#include "win.h"
#include "debug.h"


#define NATIVEFONT_GetInfoPtr(wndPtr) ((NATIVEFONT_INFO *)wndPtr->wExtra[0])




static LRESULT
NATIVEFONT_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    NATIVEFONT_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (NATIVEFONT_INFO *)COMCTL32_Alloc (sizeof(NATIVEFONT_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
	ERR (listview, "could not allocate info memory!\n");
	return 0;
    }

    if ((NATIVEFONT_INFO*)wndPtr->wExtra[0] != infoPtr) {
	ERR (listview, "pointer assignment error!\n");
	return 0;
    }

    /* initialize info structure */


    return 0;
}


static LRESULT
NATIVEFONT_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    NATIVEFONT_INFO *infoPtr = NATIVEFONT_GetInfoPtr(wndPtr);




    /* free comboex info data */
    COMCTL32_Free (infoPtr);

    return 0;
}



LRESULT WINAPI
NATIVEFONT_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {

	case WM_CREATE:
	    return NATIVEFONT_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return NATIVEFONT_Destroy (wndPtr, wParam, lParam);

	default:
	    ERR (nativefont, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
NATIVEFONT_Register (VOID)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (WC_NATIVEFONTCTL32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC32)NATIVEFONT_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(NATIVEFONT_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = (HBRUSH32)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = WC_NATIVEFONTCTL32A;
 
    RegisterClass32A (&wndClass);
}


VOID
NATIVEFONT_Unregister (VOID)
{
    if (GlobalFindAtom32A (WC_NATIVEFONTCTL32A))
	UnregisterClass32A (WC_NATIVEFONTCTL32A, (HINSTANCE32)NULL);
}

