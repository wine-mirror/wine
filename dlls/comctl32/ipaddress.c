/*
 * IP Address control
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
 *
 */

#include "windows.h"
#include "commctrl.h"
#include "ipaddress.h"
#include "win.h"
#include "debug.h"


#define IPADDRESS_GetInfoPtr(wndPtr) ((IPADDRESS_INFO *)wndPtr->wExtra[0])






static LRESULT
IPADDRESS_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    IPADDRESS_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (IPADDRESS_INFO *)COMCTL32_Alloc (sizeof(IPADDRESS_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
	ERR (listview, "could not allocate info memory!\n");
	return 0;
    }

    if ((IPADDRESS_INFO*)wndPtr->wExtra[0] != infoPtr) {
	ERR (listview, "pointer assignment error!\n");
	return 0;
    }

    /* initialize info structure */



    return 0;
}


static LRESULT
IPADDRESS_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    IPADDRESS_INFO *infoPtr = IPADDRESS_GetInfoPtr(wndPtr);






    /* free ipaddress info data */
    COMCTL32_Free (infoPtr);

    return 0;
}




LRESULT WINAPI
IPADDRESS_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
//	case IPM_CLEARADDRESS:
//	case IPM_GETADDRESS:
//	case IPM_ISBLANK:
//	case IPM_SETADDRESS:
//	case IPM_SETFOCUS:
//	case IPM_SETRANGE:


	case WM_CREATE:
	    return IPADDRESS_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return IPADDRESS_Destroy (wndPtr, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR (ipaddress, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
IPADDRESS_Register (VOID)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (WC_IPADDRESS32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC32)IPADDRESS_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(IPADDRESS_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = (HBRUSH32)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = WC_IPADDRESS32A;
 
    RegisterClass32A (&wndClass);
}


VOID
IPADDRESS_Unregister (VOID)
{
    if (GlobalFindAtom32A (WC_IPADDRESS32A))
	UnregisterClass32A (WC_IPADDRESS32A, (HINSTANCE32)NULL);
}

