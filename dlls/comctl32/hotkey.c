/*
 * Hotkey control
 *
 * Copyright 1998 Eric Kohl
 *
 * NOTES
 *   Development in progress. An author is needed! Any volunteers?
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - Some messages.
 *   - Display code.
 */

#include "windows.h"
#include "commctrl.h"
#include "hotkey.h"
#include "win.h"
#include "debug.h"


#define HOTKEY_GetInfoPtr(wndPtr) ((HOTKEY_INFO *)wndPtr->wExtra[0])


// << HOTHEY_GetHotKey >>
// << HOTHEY_SetHotKey >>
// << HOTHEY_SetRules >>



// << HOTKEY_Char >>


static LRESULT
HOTKEY_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HOTKEY_INFO *infoPtr;
    TEXTMETRIC32A tm;
    HDC32 hdc;

    /* allocate memory for info structure */
    infoPtr = (HOTKEY_INFO *)COMCTL32_Alloc (sizeof(HOTKEY_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
	ERR (listview, "could not allocate info memory!\n");
	return 0;
    }

    if ((HOTKEY_INFO*)wndPtr->wExtra[0] != infoPtr) {
	ERR (listview, "pointer assignment error!\n");
	return 0;
    }


    /* initialize info structure */

    /* get default font height */
    hdc = GetDC32 (wndPtr->hwndSelf);
    GetTextMetrics32A (hdc, &tm);
    infoPtr->nHeight = tm.tmHeight;
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return 0;
}


static LRESULT
HOTKEY_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HOTKEY_INFO *infoPtr = HOTKEY_GetInfoPtr(wndPtr);



    /* free hotkey info data */
    COMCTL32_Free (infoPtr);

    return 0;
}


static LRESULT
HOTKEY_EraseBackground (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HOTKEY_INFO *infoPtr = HOTKEY_GetInfoPtr(wndPtr);
    HBRUSH32 hBrush;
    RECT32   rc;

    hBrush =
	(HBRUSH32)SendMessage32A (wndPtr->parent->hwndSelf, WM_CTLCOLOREDIT,
				  wParam, (LPARAM)wndPtr->hwndSelf);
    if (hBrush)
	hBrush = (HBRUSH32)GetStockObject32 (WHITE_BRUSH);
    GetClientRect32 (wndPtr->hwndSelf, &rc);

    FillRect32 ((HDC32)wParam, &rc, hBrush);

    return -1;
}


__inline__ static LRESULT
HOTKEY_GetFont (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HOTKEY_INFO *infoPtr = HOTKEY_GetInfoPtr(wndPtr);

    return infoPtr->hFont;
}


static LRESULT
HOTKEY_KeyDown (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HOTKEY_INFO *infoPtr = HOTKEY_GetInfoPtr(wndPtr);

    switch (wParam) {
	case VK_RETURN:
	case VK_TAB:
	case VK_SPACE:
	case VK_DELETE:
	case VK_ESCAPE:
	case VK_BACK:
	    return DefWindowProc32A (wndPtr->hwndSelf, WM_KEYDOWN, wParam, lParam);

	case VK_SHIFT:
	case VK_CONTROL:
	case VK_MENU:
	    FIXME (hotkey, "modifier key pressed!\n");
	    break;

	default:
	    FIXME (hotkey, " %d\n", wParam);
	    break;
    }

    return TRUE;
}


static LRESULT
HOTKEY_KeyUp (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HOTKEY_INFO *infoPtr = HOTKEY_GetInfoPtr(wndPtr);

    FIXME (hotkey, " %d\n", wParam);

    return 0;
}


static LRESULT
HOTKEY_KillFocus (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HOTKEY_INFO *infoPtr = HOTKEY_GetInfoPtr(wndPtr);

    infoPtr->bFocus = FALSE;
    DestroyCaret32 ();

    return 0;
}


static LRESULT
HOTKEY_LButtonDown (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
//    HOTKEY_INFO *infoPtr = HOTKEY_GetInfoPtr(wndPtr);

    SetFocus32 (wndPtr->hwndSelf);

    return 0;
}


__inline__ static LRESULT
HOTKEY_NCCreate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    wndPtr->dwExStyle |= WS_EX_CLIENTEDGE;

    return DefWindowProc32A (wndPtr->hwndSelf, WM_NCCREATE, wParam, lParam);
}





static LRESULT
HOTKEY_SetFocus (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HOTKEY_INFO *infoPtr = HOTKEY_GetInfoPtr(wndPtr);

    infoPtr->bFocus = TRUE;


    CreateCaret32 (wndPtr->hwndSelf, (HBITMAP32)0, 1, infoPtr->nHeight);

    SetCaretPos32 (1, 1);

    ShowCaret32 (wndPtr->hwndSelf);


    return 0;
}


__inline__ static LRESULT
HOTKEY_SetFont (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HOTKEY_INFO *infoPtr = HOTKEY_GetInfoPtr(wndPtr);
    TEXTMETRIC32A tm;
    HDC32 hdc;
    HFONT32 hOldFont = 0;

    infoPtr->hFont = (HFONT32)wParam;

    hdc = GetDC32 (wndPtr->hwndSelf);
    if (infoPtr->hFont)
	hOldFont = SelectObject32 (hdc, infoPtr->hFont);

    GetTextMetrics32A (hdc, &tm);
    infoPtr->nHeight = tm.tmHeight;

    if (infoPtr->hFont)
	SelectObject32 (hdc, hOldFont);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    if (LOWORD(lParam)) {

	FIXME (hotkey, "force redraw!\n");

    }

    return 0;
}


static LRESULT
HOTKEY_SysKeyDown (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HOTKEY_INFO *infoPtr = HOTKEY_GetInfoPtr(wndPtr);

    switch (wParam) {
	case VK_RETURN:
	case VK_TAB:
	case VK_SPACE:
	case VK_DELETE:
	case VK_ESCAPE:
	case VK_BACK:
	    return DefWindowProc32A (wndPtr->hwndSelf, WM_SYSKEYDOWN, wParam, lParam);

	case VK_SHIFT:
	case VK_CONTROL:
	case VK_MENU:
	    FIXME (hotkey, "modifier key pressed!\n");
	    break;

	default:
	    FIXME (hotkey, " %d\n", wParam);
	    break;
    }

    return TRUE;
}


static LRESULT
HOTKEY_SysKeyUp (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HOTKEY_INFO *infoPtr = HOTKEY_GetInfoPtr(wndPtr);

    FIXME (hotkey, " %d\n", wParam);

    return 0;
}



LRESULT WINAPI
HOTKEY_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
//	case HKM_GETHOTKEY:
//	case HKM_SETHOTKEY:
//	case HKM_SETRULES:

//	case WM_CHAR:

	case WM_CREATE:
	    return HOTKEY_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return HOTKEY_Destroy (wndPtr, wParam, lParam);

	case WM_ERASEBKGND:
	    return HOTKEY_EraseBackground (wndPtr, wParam, lParam);

	case WM_GETDLGCODE:
	    return DLGC_WANTCHARS | DLGC_WANTARROWS;

	case WM_GETFONT:
	    return HOTKEY_GetFont (wndPtr, wParam, lParam);

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	    return HOTKEY_KeyDown (wndPtr, wParam, lParam);

	case WM_KEYUP:
	case WM_SYSKEYUP:
	    return HOTKEY_KeyUp (wndPtr, wParam, lParam);

	case WM_KILLFOCUS:
	    return HOTKEY_KillFocus (wndPtr, wParam, lParam);

	case WM_LBUTTONDOWN:
	    return HOTKEY_LButtonDown (wndPtr, wParam, lParam);

	case WM_NCCREATE:
	    return HOTKEY_NCCreate (wndPtr, wParam, lParam);

//	case WM_PAINT:

	case WM_SETFOCUS:
	    return HOTKEY_SetFocus (wndPtr, wParam, lParam);

	case WM_SETFONT:
	    return HOTKEY_SetFont (wndPtr, wParam, lParam);

//	case WM_SYSCHAR:

	default:
	    if (uMsg >= WM_USER)
		ERR (hotkey, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
HOTKEY_Register (VOID)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (HOTKEY_CLASS32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC32)HOTKEY_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(HOTKEY_INFO *);
    wndClass.hCursor       = 0;
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = HOTKEY_CLASS32A;
 
    RegisterClass32A (&wndClass);
}


VOID
HOTKEY_Unregister (VOID)
{
    if (GlobalFindAtom32A (HOTKEY_CLASS32A))
	UnregisterClass32A (HOTKEY_CLASS32A, (HINSTANCE32)NULL);
}

