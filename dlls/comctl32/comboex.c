/*
 * ComboBoxEx control
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
 * FIXME:
 *   - should include "combo.h" 
 */

#include "windows.h"
#include "commctrl.h"
#include "comboex.h"
#include "win.h"
#include "debug.h"

#define ID_CB_EDIT    1001

#define COMBOEX_GetInfoPtr(wndPtr) ((COMBOEX_INFO *)wndPtr->wExtra[0])


// << COMBOEX_DeleteItem >>


__inline__ static LRESULT
COMBOEX_GetComboControl (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr(wndPtr);

    TRACE (comboex, "\n");

    return (LRESULT)infoPtr->hwndCombo;
}


__inline__ static LRESULT
COMBOEX_GetEditControl (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr(wndPtr);

    if ((wndPtr->dwStyle & CBS_DROPDOWNLIST) != CBS_DROPDOWN)
	return 0;

    FIXME (comboex, "-- 0x%x\n", GetDlgItem32 (infoPtr->hwndCombo, ID_CB_EDIT));

    return (LRESULT)GetDlgItem32 (infoPtr->hwndCombo, ID_CB_EDIT);
}


__inline__ static LRESULT
COMBOEX_GetExtendedStyle (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr(wndPtr);

    return (LRESULT)infoPtr->dwExtStyle;
}


__inline__ static LRESULT
COMBOEX_GetImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr(wndPtr);

    TRACE (comboex, "(0x%08x 0x%08lx)\n", wParam, lParam);

    return (LRESULT)infoPtr->himl;
}




static LRESULT
COMBOEX_InsertItem32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr(wndPtr);

    FIXME (comboex, "(0x%08x 0x%08lx)\n", wParam, lParam);

    return -1;
}



static LRESULT
COMBOEX_SetExtendedStyle (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr(wndPtr);
    DWORD dwTemp;

    TRACE (comboex, "(0x%08x 0x%08lx)\n", wParam, lParam);

    dwTemp = infoPtr->dwExtStyle;

    if ((DWORD)wParam) {
	infoPtr->dwExtStyle = (infoPtr->dwExtStyle & ~(DWORD)wParam) | (DWORD)lParam;
    }
    else
	infoPtr->dwExtStyle = (DWORD)lParam;

    /* FIXME: repaint?? */

    return (LRESULT)dwTemp;
}


__inline__ static LRESULT
COMBOEX_SetImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr(wndPtr);
    HIMAGELIST himlTemp;

    TRACE (comboex, "(0x%08x 0x%08lx)\n", wParam, lParam);

    himlTemp = infoPtr->himl;
    infoPtr->himl = (HIMAGELIST)lParam;

    return (LRESULT)himlTemp;
}




static LRESULT
COMBOEX_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr;
    DWORD dwComboStyle;

    /* allocate memory for info structure */
    infoPtr = (COMBOEX_INFO *)COMCTL32_Alloc (sizeof(COMBOEX_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
	ERR (listview, "could not allocate info memory!\n");
	return 0;
    }

    if ((COMBOEX_INFO*)wndPtr->wExtra[0] != infoPtr) {
	ERR (listview, "pointer assignment error!\n");
	return 0;
    }


    /* initialize info structure */


    /* create combo box */
    dwComboStyle = 
	wndPtr->dwStyle & (CBS_SIMPLE|CBS_DROPDOWN|CBS_DROPDOWNLIST|WS_CHILD);

    infoPtr->hwndCombo =
	CreateWindow32A ("ComboBox", "",
			 WS_CHILD | WS_VISIBLE | CBS_OWNERDRAWFIXED | dwComboStyle,
			 0, 0, 0, 0, wndPtr->hwndSelf, (HMENU32)1,
			 wndPtr->hInstance, NULL);


    return 0;
}


static LRESULT
COMBOEX_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr(wndPtr);


    if (infoPtr->hwndCombo)
	DestroyWindow32 (infoPtr->hwndCombo);




    /* free comboex info data */
    COMCTL32_Free (infoPtr);

    return 0;
}


static LRESULT
COMBOEX_Size (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr(wndPtr);
    RECT32 rect;

    GetClientRect32 (wndPtr->hwndSelf, &rect);

    MoveWindow32 (infoPtr->hwndCombo, 0, 0, rect.right -rect.left,
		  rect.bottom - rect.top, TRUE);

    return 0;
}


LRESULT WINAPI
COMBOEX_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
//	case CBEM_DELETEITEM:

	case CBEM_GETCOMBOCONTROL:
	    return COMBOEX_GetComboControl (wndPtr, wParam, lParam);

	case CBEM_GETEDITCONTROL:
	    return COMBOEX_GetEditControl (wndPtr, wParam, lParam);

	case CBEM_GETEXTENDEDSTYLE:
	    return COMBOEX_GetExtendedStyle (wndPtr, wParam, lParam);

	case CBEM_GETIMAGELIST:
	    return COMBOEX_GetImageList (wndPtr, wParam, lParam);

//	case CBEM_GETITEM32A:
//	case CBEM_GETITEM32W:
//	case CBEM_GETUNICODEFORMAT:
//	case CBEM_HASEDITCHANGED:

	case CBEM_INSERTITEM32A:
	    return COMBOEX_InsertItem32A (wndPtr, wParam, lParam);

//	case CBEM_INSERTITEM32W:

	case CBEM_SETEXTENDEDSTYLE:
	    return COMBOEX_SetExtendedStyle (wndPtr, wParam, lParam);

	case CBEM_SETIMAGELIST:
	    return COMBOEX_SetImageList (wndPtr, wParam, lParam);

//	case CBEM_SETITEM32A:
//	case CBEM_SETITEM32W:
//	case CBEM_SETUNICODEFORMAT:


	case WM_CREATE:
	    return COMBOEX_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return COMBOEX_Destroy (wndPtr, wParam, lParam);

	case WM_SIZE:
	    return COMBOEX_Size (wndPtr, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR (comboex, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
COMBOEX_Register (VOID)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (WC_COMBOBOXEX32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC32)COMBOEX_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(COMBOEX_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = (HBRUSH32)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = WC_COMBOBOXEX32A;
 
    RegisterClass32A (&wndClass);
}


VOID
COMBOEX_Unregister (VOID)
{
    if (GlobalFindAtom32A (WC_COMBOBOXEX32A))
	UnregisterClass32A (WC_COMBOBOXEX32A, (HINSTANCE32)NULL);
}

