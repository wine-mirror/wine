/*
 * Animation control
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
 */


#include "winbase.h"
#include "commctrl.h"
#include "animate.h"
#include "debug.h"

#define ANIMATE_GetInfoPtr(hwnd) ((ANIMATE_INFO *)GetWindowLongA (hwnd, 0))


static BOOL
ANIMATE_LoadResA (ANIMATE_INFO *infoPtr, HINSTANCE hInst, LPSTR lpName)
{
    HRSRC hrsrc;
    HGLOBAL handle;

    hrsrc = FindResourceA (hInst, lpName, "AVI");
    if (!hrsrc)
	return FALSE;

    handle = LoadResource (hInst, hrsrc);
    if (!handle)
	return FALSE;

    infoPtr->lpAvi = LockResource (handle);
    if (!infoPtr->lpAvi)
	return FALSE;

    return TRUE;
}


static BOOL
ANIMATE_LoadFileA (ANIMATE_INFO *infoPtr, LPSTR lpName)
{
    HANDLE handle;

    infoPtr->hFile =
	CreateFileA (lpName, GENERIC_READ, 0, NULL, OPEN_EXISTING,
		       FILE_ATTRIBUTE_NORMAL, 0);
    if (!infoPtr->hFile)
	return FALSE;

    handle =
	CreateFileMappingA (infoPtr->hFile, NULL, PAGE_READONLY | SEC_COMMIT,
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
	GlobalFree ((HGLOBAL)infoPtr->lpAvi);
	infoPtr->lpAvi = NULL;
    }
}


static VOID
ANIMATE_GetAviInfo (infoPtr)
{


}


static LRESULT
ANIMATE_OpenA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr = ANIMATE_GetInfoPtr(hwnd);
    HINSTANCE hInstance = (HINSTANCE)wParam;

    ANIMATE_Free (infoPtr);

    if (!lParam) {
	TRACE (animate, "closing avi!\n");
	return TRUE;
    }
    
    if (HIWORD(lParam)) {
	FIXME (animate, "(\"%s\") empty stub!\n", (LPSTR)lParam);

	if (ANIMATE_LoadResA (infoPtr, hInstance, (LPSTR)lParam)) {

	    FIXME (animate, "AVI resource found!\n");

	}
	else {
	    FIXME (animate, "No AVI resource found!\n");
	    if (ANIMATE_LoadFileA (infoPtr, (LPSTR)lParam)) {
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

	if (ANIMATE_LoadResA (infoPtr, hInstance,
				MAKEINTRESOURCEA((INT)lParam))) {
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


/* << ANIMATE_Open32W >> */


static LRESULT
ANIMATE_Play (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    /* ANIMATE_INFO *infoPtr = ANIMATE_GetInfoPtr(hwnd); */
    INT nFrom   = (INT)LOWORD(lParam);
    INT nTo     = (INT)HIWORD(lParam);
    INT nRepeat = (INT)wParam;

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
ANIMATE_Stop (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    /* ANIMATE_INFO *infoPtr = ANIMATE_GetInfoPtr(hwnd); */

#if 0
    /* nothing opened */
    if (...)
	return FALSE;
#endif
    
    return TRUE;
}



static LRESULT
ANIMATE_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (ANIMATE_INFO *)COMCTL32_Alloc (sizeof(ANIMATE_INFO));
    if (!infoPtr) {
	ERR (animate, "could not allocate info memory!\n");
	return 0;
    }

    /* store pointer to info structure */
    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);


    /* set default settings */


    return 0;
}


static LRESULT
ANIMATE_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr = ANIMATE_GetInfoPtr(hwnd);


    /* free avi data */
    ANIMATE_Free (infoPtr);

    /* free animate info data */
    COMCTL32_Free (infoPtr);

    return 0;
}


#if 0
static LRESULT
ANIMATE_EraseBackground (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr = ANIMATE_GetInfoPtr(hwnd);
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
ANIMATE_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
	case ACM_OPENA:
	    return ANIMATE_OpenA (hwnd, wParam, lParam);

/*	case ACM_OPEN32W: */
/*	    return ANIMATE_Open32W (hwnd, wParam, lParam); */

        case ACM_PLAY:
            return ANIMATE_Play (hwnd, wParam, lParam);

	case ACM_STOP:
	    return ANIMATE_Stop (hwnd, wParam, lParam);


	case WM_CREATE:
	    return ANIMATE_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return ANIMATE_Destroy (hwnd, wParam, lParam);

/*	case WM_ERASEBKGND: */
/*	    return ANIMATE_EraseBackground (hwnd, wParam, lParam); */

/*	case WM_NCCREATE: */
/*	case WM_NCHITTEST: */
/*	case WM_PAINT: */
/*	case WM_SIZE: */
/*	case WM_STYLECHANGED: */
/*	case WM_TIMER: */

	default:
	    if (uMsg >= WM_USER)
		ERR (animate, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
ANIMATE_Register (VOID)
{
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (ANIMATE_CLASSA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC)ANIMATE_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(ANIMATE_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = ANIMATE_CLASSA;
 
    RegisterClassA (&wndClass);
}


VOID
ANIMATE_Unregister (VOID)
{
    if (GlobalFindAtomA (ANIMATE_CLASSA))
	UnregisterClassA (ANIMATE_CLASSA, (HINSTANCE)NULL);
}

