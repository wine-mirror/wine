/*
 * Date and time picker control
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

#include "win.h"
#include "commctrl.h"
#include "datetime.h"
#include "debug.h"


#define DATETIME_GetInfoPtr(wndPtr) ((DATETIME_INFO *)wndPtr->wExtra[0])






static LRESULT
DATETIME_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    DATETIME_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (DATETIME_INFO *)COMCTL32_Alloc (sizeof(DATETIME_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
	ERR (datetime, "could not allocate info memory!\n");
	return 0;
    }

    if ((DATETIME_INFO*)wndPtr->wExtra[0] != infoPtr) {
	ERR (datetime, "pointer assignment error!\n");
	return 0;
    }

    /* initialize info structure */



    return 0;
}


static LRESULT
DATETIME_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr(wndPtr);

    /* free datetime info data */
    COMCTL32_Free (infoPtr);

    return 0;
}




LRESULT WINAPI
DATETIME_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {

    case DTM_GETSYSTEMTIME:
        FIXME (datetime, "Unimplemented msg DTM_GETSYSTEMTIME\n");
        return GDT_VALID;

    case DTM_SETSYSTEMTIME:
        FIXME (datetime, "Unimplemented msg DTM_SETSYSTEMTIME\n");
        return 1;

    case DTM_GETRANGE:
        FIXME (datetime, "Unimplemented msg DTM_GETRANGE\n");
        return 0;

    case DTM_SETRANGE:
        FIXME (datetime, "Unimplemented msg DTM_SETRANGE\n");
        return 1;

    case DTM_SETFORMAT32A:
        FIXME (datetime, "Unimplemented msg DTM_SETFORMAT32A\n");
        return 1;

    case DTM_SETFORMAT32W:
        FIXME (datetime, "Unimplemented msg DTM_SETFORMAT32W\n");
        return 1;

    case DTM_SETMCCOLOR:
        FIXME (datetime, "Unimplemented msg DTM_SETMCCOLOR\n");
        return 0;

    case DTM_GETMCCOLOR:
        FIXME (datetime, "Unimplemented msg DTM_GETMCCOLOR\n");
        return 0;

    case DTM_GETMONTHCAL:
        FIXME (datetime, "Unimplemented msg DTM_GETMONTHCAL\n");
        return 0;

    case DTM_SETMCFONT:
        FIXME (datetime, "Unimplemented msg DTM_SETMCFONT\n");
        return 0;

    case DTM_GETMCFONT:
        FIXME (datetime, "Unimplemented msg DTM_GETMCFONT\n");
        return 0;

	case WM_CREATE:
	    return DATETIME_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return DATETIME_Destroy (wndPtr, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR (datetime, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
DATETIME_Register (VOID)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (DATETIMEPICK_CLASS32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC32)DATETIME_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(DATETIME_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = (HBRUSH32)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = DATETIMEPICK_CLASS32A;
 
    RegisterClass32A (&wndClass);
}


VOID
DATETIME_Unregister (VOID)
{
    if (GlobalFindAtom32A (DATETIMEPICK_CLASS32A))
	UnregisterClass32A (DATETIMEPICK_CLASS32A, (HINSTANCE32)NULL);
}

