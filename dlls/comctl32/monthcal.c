/*
 * Month calendar control
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

#include "commctrl.h"
#include "monthcal.h"
#include "win.h"
#include "debug.h"


#define MONTHCAL_GetInfoPtr(wndPtr) ((MONTHCAL_INFO *)wndPtr->wExtra[0])






static LRESULT
MONTHCAL_Create (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    MONTHCAL_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (MONTHCAL_INFO *)COMCTL32_Alloc (sizeof(MONTHCAL_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
	ERR (monthcal, "could not allocate info memory!\n");
	return 0;
    }

    if ((MONTHCAL_INFO*)wndPtr->wExtra[0] != infoPtr) {
	ERR (monthcal, "pointer assignment error!\n");
	return 0;
    }

    /* initialize info structure */



    return 0;
}


static LRESULT
MONTHCAL_Destroy (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(wndPtr);






    /* free ipaddress info data */
    COMCTL32_Free (infoPtr);

    return 0;
}




LRESULT WINAPI
MONTHCAL_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {


	case WM_CREATE:
	    return MONTHCAL_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return MONTHCAL_Destroy (wndPtr, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR (monthcal, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
MONTHCAL_Register (VOID)
{
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (MONTHCAL_CLASSA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC)MONTHCAL_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(MONTHCAL_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = MONTHCAL_CLASSA;
 
    RegisterClassA (&wndClass);
}


VOID
MONTHCAL_Unregister (VOID)
{
    if (GlobalFindAtomA (MONTHCAL_CLASSA))
	UnregisterClassA (MONTHCAL_CLASSA, (HINSTANCE)NULL);
}

