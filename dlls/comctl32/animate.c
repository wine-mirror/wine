/*
 * Animation control
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

#include "windows.h"
#include "winnt.h"
#include "winbase.h"
#include "commctrl.h"
#include "animate.h"
#include "win.h"
#include "debug.h"


#define ANIMATE_GetInfoPtr(wndPtr) ((ANIMATE_INFO *)wndPtr->wExtra[0])


static BOOL32
ANIMATE_LoadRes32A (ANIMATE_INFO *infoPtr, HINSTANCE32 hInst, LPSTR lpName)
{
    HRSRC32 hrsrc;
    HGLOBAL32 handle;

    hrsrc = FindResource32A (hInst, lpName, "AVI");
    if (!hrsrc)
	return FALSE;

    handle = LoadResource32 (hInst, hrsrc);
    if (!handle)
	return FALSE;

    infoPtr->lpAvi = LockResource32 (handle);
    if (!infoPtr->lpAvi)
	return FALSE;

    return TRUE;
}


static BOOL32
ANIMATE_LoadFile32A (ANIMATE_INFO *infoPtr, LPSTR lpName)
{
    HANDLE32 handle;

    infoPtr->hFile =
	CreateFile32A (lpName, GENERIC_READ, 0, NULL, OPEN_EXISTING,
		       FILE_ATTRIBUTE_NORMAL, 0);
    if (!infoPtr->hFile)
	return FALSE;

    handle =
	CreateFileMapping32A (infoPtr->hFile, NULL, PAGE_READONLY | SEC_COMMIT,
			      0, 0, NULL);
    if (!handle) {
	CloseHandle (infoPtr->hFile);
	infoPtr->hFile = 0;
	return FALSE;
    }

    infoPtr->lpAvi = MapViewOfFile (handle, FILE_MAP_READ, 0, 0, 0);
    if (!infoPtr->lpAvi) {
	CloseHandle (infoPtr->hFile);
	infoPtr->hFile = 0;
	return FALSE;
    }

    return TRUE;
}


static VOID
ANIMATE_Free (ANIMATE_INFO *infoPtr)
{
    if (infoPtr->hFile) {
	UnmapViewOfFile (infoPtr->lpAvi);
	CloseHandle (infoPtr->hFile);
	infoPtr->lpAvi = NULL;
    }
    else {
	GlobalFree32 (infoPtr->lpAvi);
	infoPtr->lpAvi = NULL;
    }
}


static VOID
ANIMATE_GetAviInfo (infoPtr)
{


}


static LRESULT
ANIMATE_Open32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr = ANIMATE_GetInfoPtr(wndPtr);
    HINSTANCE32 hInstance = (HINSTANCE32)wParam;

    ANIMATE_Free (infoPtr);

    if (!lParam) {
	TRACE (animate, "closing avi!\n");
	return TRUE;
    }
    
    if (HIWORD(lParam)) {
	FIXME (animate, "(\"%s\") empty stub!\n", (LPSTR)lParam);

	if (ANIMATE_LoadRes32A (infoPtr, hInstance, (LPSTR)lParam)) {

	    FIXME (animate, "AVI resource found!\n");

	}
	else {
	    FIXME (animate, "No AVI resource found!\n");
	    if (ANIMATE_LoadFile32A (infoPtr, (LPSTR)lParam)) {
		FIXME (animate, "AVI file found!\n");
	    }
	    else {
		FIXME (animate, "No AVI file found!\n");
		return FALSE;
	    }
	}
    }
    else {
	FIXME (animate, "(%u) empty stub!\n", (WORD)LOWORD(lParam));

	if (ANIMATE_LoadRes32A (infoPtr, hInstance,
				MAKEINTRESOURCE32A((INT32)lParam))) {
	    FIXME (animate, "AVI resource found!\n");
	}
	else {
	    FIXME (animate, "No AVI resource found!\n");
	    return FALSE;
	}
    }

    ANIMATE_GetAviInfo (infoPtr);

    return TRUE;
}


// << ANIMATE_Open32W >>


static LRESULT
ANIMATE_Play (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr = ANIMATE_GetInfoPtr(wndPtr);
    INT32 nFrom   = (INT32)LOWORD(lParam);
    INT32 nTo     = (INT32)HIWORD(lParam);
    INT32 nRepeat = (INT32)wParam;

#if 0
    /* nothing opened */
    if (...)
	return FALSE;
#endif
    
    if (nRepeat == -1) {

	FIXME (animate, "(loop from=%d to=%d) empty stub!\n",
	       nFrom, nTo);

    }
    else {

	FIXME (animate, "(repeat=%d from=%d to=%d) empty stub!\n",
	       nRepeat, nFrom, nTo);

    }


    return TRUE;
}


static LRESULT
ANIMATE_Stop (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr = ANIMATE_GetInfoPtr(wndPtr);

#if 0
    /* nothing opened */
    if (...)
	return FALSE;
#endif
    
    return TRUE;
}



static LRESULT
ANIMATE_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (ANIMATE_INFO *)COMCTL32_Alloc (sizeof(ANIMATE_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
	ERR (animate, "could not allocate info memory!\n");
	return 0;
    }

    if ((ANIMATE_INFO*)wndPtr->wExtra[0] != infoPtr) {
	ERR (animate, "pointer assignment error!\n");
	return 0;
    }

    /* set default settings */


    return 0;
}


static LRESULT
ANIMATE_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr = ANIMATE_GetInfoPtr(wndPtr);


    /* free avi data */
    ANIMATE_Free (infoPtr);

    /* free animate info data */
    COMCTL32_Free (infoPtr);

    return 0;
}


#if 0
static LRESULT
ANIMATE_EraseBackground (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr = ANIMATE_GetInfoPtr(wndPtr);
/*
    HBRUSH32 hBrush = CreateSolidBrush32 (infoPtr->clrBk);
    RECT32 rect;

    GetClientRect32 (wndPtr->hwndSelf, &rect);
    FillRect32 ((HDC32)wParam, &rect, hBrush);
    DeleteObject32 (hBrush);
*/
    return TRUE;
}
#endif



LRESULT WINAPI
ANIMATE_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
	case ACM_OPEN32A:
	    return ANIMATE_Open32A (wndPtr, wParam, lParam);

//	case ACM_OPEN32W:
//	    return ANIMATE_Open32W (wndPtr, wParam, lParam);

        case ACM_PLAY:
            return ANIMATE_Play (wndPtr, wParam, lParam);

	case ACM_STOP:
	    return ANIMATE_Stop (wndPtr, wParam, lParam);


	case WM_CREATE:
	    return ANIMATE_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return ANIMATE_Destroy (wndPtr, wParam, lParam);

//	case WM_ERASEBKGND:
//	    return ANIMATE_EraseBackground (wndPtr, wParam, lParam);

//	case WM_NCCREATE:
//	case WM_NCHITTEST:
//	case WM_PAINT:
//	case WM_SIZE:
//	case WM_STYLECHANGED:
//	case WM_TIMER:

	default:
	    if (uMsg >= WM_USER)
		ERR (animate, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void
ANIMATE_Register (void)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (ANIMATE_CLASS32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC32)ANIMATE_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(ANIMATE_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = (HBRUSH32)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = ANIMATE_CLASS32A;
 
    RegisterClass32A (&wndClass);
}
