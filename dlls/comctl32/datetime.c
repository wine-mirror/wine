/*
 * Date and time picker control
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
#include "datetime.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(datetime)


#define DATETIME_GetInfoPtr(hwnd) ((DATETIME_INFO *)GetWindowLongA (hwnd, 0))






static LRESULT
DATETIME_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DATETIME_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (DATETIME_INFO *)COMCTL32_Alloc (sizeof(DATETIME_INFO));
    if (infoPtr == NULL) {
	ERR (datetime, "could not allocate info memory!\n");
	return 0;
    }

    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);


    /* initialize info structure */



    return 0;
}


static LRESULT
DATETIME_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);

    /* free datetime info data */
    COMCTL32_Free (infoPtr);

    return 0;
}




LRESULT WINAPI
DATETIME_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
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

    case DTM_SETFORMATA:
        FIXME (datetime, "Unimplemented msg DTM_SETFORMAT32A\n");
        return 1;

    case DTM_SETFORMATW:
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
	    return DATETIME_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return DATETIME_Destroy (hwnd, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR (datetime, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
DATETIME_Register (VOID)
{
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (DATETIMEPICK_CLASSA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC)DATETIME_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(DATETIME_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = DATETIMEPICK_CLASSA;
 
    RegisterClassA (&wndClass);
}


VOID
DATETIME_Unregister (VOID)
{
    if (GlobalFindAtomA (DATETIMEPICK_CLASSA))
	UnregisterClassA (DATETIMEPICK_CLASSA, (HINSTANCE)NULL);
}

