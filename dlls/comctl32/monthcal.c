/*
 * Month calendar control
 *
 * Copyright 1998, 1999 Eric Kohl
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
#include "monthcal.h"
#include "debug.h"


#define MONTHCAL_GetInfoPtr(hwnd) ((MONTHCAL_INFO *)GetWindowLongA (hwnd, 0))






static LRESULT
MONTHCAL_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    MONTHCAL_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (MONTHCAL_INFO *)COMCTL32_Alloc (sizeof(MONTHCAL_INFO));
    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);


    /* initialize info structure */



    return 0;
}


static LRESULT
MONTHCAL_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr (hwnd);






    /* free ipaddress info data */
    COMCTL32_Free (infoPtr);

    return 0;
}




LRESULT WINAPI
MONTHCAL_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {


	case WM_CREATE:
	    return MONTHCAL_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return MONTHCAL_Destroy (hwnd, wParam, lParam);

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

