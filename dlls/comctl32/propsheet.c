/*
 * Property Sheets
 *
 * Copyright 1998 Francis Beaudet
 *
 * TODO:
 *   - All the functions are simply stubs
 *
 */

#include "commctrl.h"
#include "prsht.h"
#include "propsheet.h"
#include "win.h"
#include "debug.h"


LRESULT WINAPI
PROPSHEET_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);







/*****************************************************************
 *            PropertySheet32A   (COMCTL32.84)(COMCTL32.83)
 */
INT WINAPI PropertySheetA(LPCPROPSHEETHEADERA lppsh)
{
	HWND hwnd;

    FIXME(propsheet, "(%p): stub\n", lppsh);

	if (lppsh->dwFlags & PSH_MODELESS) {
        hwnd = CreateDialogParamA ( lppsh->hInstance, WC_PROPSHEETA,
                    lppsh->hwndParent, (DLGPROC)PROPSHEET_WindowProc,
		    (LPARAM) lppsh );
		ShowWindow (hwnd, TRUE);
	} else {
		hwnd =  DialogBoxParamA ( lppsh->hInstance, WC_PROPSHEETA,
                    lppsh->hwndParent, (DLGPROC)PROPSHEET_WindowProc,
		    (LPARAM) lppsh );
	}
    return hwnd;
}

/*****************************************************************
 *            PropertySheet32W   (COMCTL32.85)
 */
INT WINAPI PropertySheetW(LPCPROPSHEETHEADERW propertySheetHeader)
{
    FIXME(propsheet, "(%p): stub\n", propertySheetHeader);

    return -1;
}





/*****************************************************************
 *            CreatePropertySheetPage32A   (COMCTL32.19)(COMCTL32.18)
 */
HPROPSHEETPAGE WINAPI CreatePropertySheetPageA(LPCPROPSHEETPAGEA lpPropSheetPage)
{
    FIXME(propsheet, "(%p): stub\n", lpPropSheetPage);

    return 0;
}

/*****************************************************************
 *            CreatePropertySheetPage32W   (COMCTL32.20)
 */
HPROPSHEETPAGE WINAPI CreatePropertySheetPageW(LPCPROPSHEETPAGEW lpPropSheetPage)
{
    FIXME(propsheet, "(%p): stub\n", lpPropSheetPage);

    return 0;
}

/*****************************************************************
 *            DestroyPropertySheetPage32   (COMCTL32.24)
 */
BOOL WINAPI DestroyPropertySheetPage(HPROPSHEETPAGE hPropPage)
{
    FIXME(propsheet, "(0x%08lx): stub\n", (DWORD)hPropPage);
    return FALSE;
}



LRESULT WINAPI
PROPSHEET_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    /* WND *wndPtr = WIN_FindWndPtr(hwnd); */

    switch (uMsg) {
       case PSM_SETCURSEL:
           FIXME (propsheet, "Unimplemented msg PSM_SETCURSEL\n");
           return 0;
       case PSM_REMOVEPAGE:
           FIXME (propsheet, "Unimplemented msg PSM_REMOVEPAGE\n");
           return 0;
       case PSM_ADDPAGE:
           FIXME (propsheet, "Unimplemented msg PSM_ADDPAGE\n");
           return 0;
       case PSM_CHANGED:
           FIXME (propsheet, "Unimplemented msg PSM_CHANGED\n");
           return 0;
       case PSM_RESTARTWINDOWS:
           FIXME (propsheet, "Unimplemented msg PSM_RESTARTWINDOWS\n");
           return 0;
       case PSM_REBOOTSYSTEM:
           FIXME (propsheet, "Unimplemented msg PSM_REBOOTSYSTEM\n");
           return 0;
       case PSM_CANCELTOCLOSE:
           FIXME (propsheet, "Unimplemented msg PSM_CANCELTOCLOSE\n");
           return 0;
       case PSM_QUERYSIBLINGS:
           FIXME (propsheet, "Unimplemented msg PSM_QUERYSIBLINGS\n");
           return 0;
       case PSM_UNCHANGED:
           FIXME (propsheet, "Unimplemented msg PSM_UNCHANGED\n");
           return 0;
       case PSM_APPLY:
           FIXME (propsheet, "Unimplemented msg PSM_APPLY\n");
           return 0;
       case PSM_SETTITLEA:
           FIXME (propsheet, "Unimplemented msg PSM_SETTITLE32A\n");
           return 0;
       case PSM_SETTITLEW:
           FIXME (propsheet, "Unimplemented msg PSM_SETTITLE32W\n");
           return 0;
       case PSM_SETWIZBUTTONS:
           FIXME (propsheet, "Unimplemented msg PSM_SETWIZBUTTONS\n");
           return 0;
       case PSM_PRESSBUTTON:
           FIXME (propsheet, "Unimplemented msg PSM_PRESSBUTTON\n");
           return 0;
       case PSM_SETCURSELID:
           FIXME (propsheet, "Unimplemented msg PSM_SETCURSELID\n");
           return 0;
       case PSM_SETFINISHTEXTA:
           FIXME (propsheet, "Unimplemented msg PSM_SETFINISHTEXT32A\n");
           return 0;
       case PSM_SETFINISHTEXTW:
           FIXME (propsheet, "Unimplemented msg PSM_SETFINISHTEXT32W\n");
           return 0;
       case PSM_GETTABCONTROL:
           FIXME (propsheet, "Unimplemented msg PSM_GETTABCONTROL\n");
           return 0;
       case PSM_ISDIALOGMESSAGE:
           FIXME (propsheet, "Unimplemented msg PSM_ISDIALOGMESSAGE\n");
           return 0;
       case PSM_GETCURRENTPAGEHWND:
           FIXME (propsheet, "Unimplemented msg PSM_GETCURRENTPAGEHWND\n");
           return 0;

       default:
        if (uMsg >= WM_USER)
	    ERR (propsheet, "unknown msg %04x wp=%08x lp=%08lx\n",
		 uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
}


VOID
PROPSHEET_Register (VOID)
{
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (WC_PROPSHEETA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS;
    wndClass.lpfnWndProc   = (WNDPROC)PROPSHEET_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(PROPSHEET_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = WC_PROPSHEETA;

    RegisterClassA (&wndClass);
}


VOID
PROPSHEET_UnRegister (VOID)
{
    if (GlobalFindAtomA (WC_PROPSHEETA))
        UnregisterClassA (WC_PROPSHEETA, (HINSTANCE)NULL);
}
