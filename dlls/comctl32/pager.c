/*
 * Pager control
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

#include "commctrl.h"
#include "pager.h"
#include "win.h"
#include "debug.h"


#define PAGER_GetInfoPtr(wndPtr) ((PAGER_INFO *)wndPtr->wExtra[0])


static __inline__ LRESULT
PAGER_ForwardMouse (WND *wndPtr, WPARAM wParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr);

    infoPtr->bForward = (BOOL)wParam;

    return 0;
}


static __inline__ LRESULT
PAGER_GetBkColor (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr);

    return (LRESULT)infoPtr->clrBk;
}


static __inline__ LRESULT
PAGER_GetBorder (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr);

    return (LRESULT)infoPtr->nBorder;
}


static __inline__ LRESULT
PAGER_GetButtonSize (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr);

    return (LRESULT)infoPtr->nButtonSize;
}


static LRESULT
PAGER_GetButtonState (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    /* PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr); */

    FIXME (pager, "empty stub!\n");

    return PGF_INVISIBLE;
}


/* << PAGER_GetDropTarget >> */


static __inline__ LRESULT
PAGER_GetPos (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr);

    return infoPtr->nPos;
}


static LRESULT
PAGER_RecalcSize (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr);
    NMPGCALCSIZE nmpgcs;

    if (infoPtr->hwndChild) {
	ZeroMemory (&nmpgcs, sizeof (NMPGCALCSIZE));
	nmpgcs.hdr.hwndFrom = wndPtr->hwndSelf;
	nmpgcs.hdr.idFrom = wndPtr->wIDmenu;
	nmpgcs.hdr.code = PGN_CALCSIZE;
	nmpgcs.dwFlag =
	     (wndPtr->dwStyle & PGS_HORZ) ? PGF_CALCWIDTH : PGF_CALCHEIGHT;
	SendMessageA (GetParent (wndPtr->hwndSelf), WM_NOTIFY,
			(WPARAM)wndPtr->wIDmenu, (LPARAM)&nmpgcs);

	infoPtr->nChildSize =
	     (wndPtr->dwStyle & PGS_HORZ) ? nmpgcs.iWidth : nmpgcs.iHeight;


        FIXME (pager, "Child size %d\n", infoPtr->nChildSize);


    }

    return 0;
}


static __inline__ LRESULT
PAGER_SetBkColor (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr);
    COLORREF clrTemp = infoPtr->clrBk;

    infoPtr->clrBk = (COLORREF)lParam;

    /* FIXME: redraw */

    return (LRESULT)clrTemp;
}


static __inline__ LRESULT
PAGER_SetBorder (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr);
    INT nTemp = infoPtr->nBorder;

    infoPtr->nBorder = (INT)lParam;

    /* FIXME: redraw */

    return (LRESULT)nTemp;
}


static __inline__ LRESULT
PAGER_SetButtonSize (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr);
    INT nTemp = infoPtr->nButtonSize;

    infoPtr->nButtonSize = (INT)lParam;

    FIXME (pager, "size=%d\n", infoPtr->nButtonSize);

    /* FIXME: redraw */

    return (LRESULT)nTemp;
}


static __inline__ LRESULT
PAGER_SetChild (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr);

    infoPtr->hwndChild = IsWindow ((HWND)lParam) ? (HWND)lParam : 0;

    FIXME (pager, "hwnd=%x\n", infoPtr->hwndChild);

    /* FIXME: redraw */
    if (infoPtr->hwndChild) {
	SetParent (infoPtr->hwndChild, wndPtr->hwndSelf);
	SetWindowPos (infoPtr->hwndChild, HWND_TOP,
			0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
    }

    return 0;
}


static __inline__ LRESULT
PAGER_SetPos (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr);

    infoPtr->nPos = (INT)lParam;

    FIXME (pager, "pos=%d\n", infoPtr->nPos);

    /* FIXME: redraw */
    SetWindowPos (infoPtr->hwndChild, HWND_TOP,
		    0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);

    return 0;
}


static LRESULT
PAGER_Create (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (PAGER_INFO *)COMCTL32_Alloc (sizeof(PAGER_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
	ERR (pager, "could not allocate info memory!\n");
	return 0;
    }

    if ((PAGER_INFO*)wndPtr->wExtra[0] != infoPtr) {
	ERR (pager, "pointer assignment error!\n");
	return 0;
    }

    /* set default settings */
    infoPtr->hwndChild = (HWND)NULL;
    infoPtr->clrBk = GetSysColor (COLOR_BTNFACE);
    infoPtr->nBorder = 0;
    infoPtr->nButtonSize = 0;
    infoPtr->nPos = 0;


    return 0;
}


static LRESULT
PAGER_Destroy (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr);




    /* free pager info data */
    COMCTL32_Free (infoPtr);

    return 0;
}


static LRESULT
PAGER_EraseBackground (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr);
    HBRUSH hBrush = CreateSolidBrush (infoPtr->clrBk);
    RECT rect;

    GetClientRect (wndPtr->hwndSelf, &rect);
    FillRect ((HDC)wParam, &rect, hBrush);
    DeleteObject (hBrush);

/*    return TRUE; */
    return FALSE;
}


static LRESULT
PAGER_MouseMove (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    /* PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr); */

    TRACE (pager, "stub!\n");

    return 0;
}


/* << PAGER_Paint >> */


static LRESULT
PAGER_Size (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr(wndPtr);
    RECT rect;

    GetClientRect (wndPtr->hwndSelf, &rect);
    if (infoPtr->hwndChild) {
	SetWindowPos (infoPtr->hwndChild, HWND_TOP, rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top,
			SWP_SHOWWINDOW);
/*	MoveWindow32 (infoPtr->hwndChild, 1, 1, rect.right - 2, rect.bottom-2, TRUE); */
/*	UpdateWindow32 (infoPtr->hwndChild); */

    }
/*    FillRect32 ((HDC32)wParam, &rect, hBrush); */
/*    DeleteObject32 (hBrush); */
    return TRUE;
}



LRESULT WINAPI
PAGER_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
	case PGM_FORWARDMOUSE:
	    return PAGER_ForwardMouse (wndPtr, wParam);

	case PGM_GETBKCOLOR:
	    return PAGER_GetBkColor (wndPtr, wParam, lParam);

	case PGM_GETBORDER:
	    return PAGER_GetBorder (wndPtr, wParam, lParam);

	case PGM_GETBUTTONSIZE:
	    return PAGER_GetButtonSize (wndPtr, wParam, lParam);

	case PGM_GETBUTTONSTATE:
	    return PAGER_GetButtonState (wndPtr, wParam, lParam);

/*	case PGM_GETDROPTARGET: */

	case PGM_GETPOS:
	    return PAGER_SetPos (wndPtr, wParam, lParam);

	case PGM_RECALCSIZE:
	    return PAGER_RecalcSize (wndPtr, wParam, lParam);

	case PGM_SETBKCOLOR:
	    return PAGER_SetBkColor (wndPtr, wParam, lParam);

	case PGM_SETBORDER:
	    return PAGER_SetBorder (wndPtr, wParam, lParam);

	case PGM_SETBUTTONSIZE:
	    return PAGER_SetButtonSize (wndPtr, wParam, lParam);

	case PGM_SETCHILD:
	    return PAGER_SetChild (wndPtr, wParam, lParam);

	case PGM_SETPOS:
	    return PAGER_SetPos (wndPtr, wParam, lParam);

	case WM_CREATE:
	    return PAGER_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return PAGER_Destroy (wndPtr, wParam, lParam);

	case WM_ERASEBKGND:
	    return PAGER_EraseBackground (wndPtr, wParam, lParam);

	case WM_MOUSEMOVE:
	    return PAGER_MouseMove (wndPtr, wParam, lParam);

	case WM_NOTIFY:
	case WM_COMMAND:
	    return SendMessageA (wndPtr->parent->hwndSelf, uMsg, wParam, lParam);

/*	case WM_PAINT: */
/*	    return PAGER_Paint (wndPtr, wParam); */

	case WM_SIZE:
	    return PAGER_Size (wndPtr, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR (pager, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
PAGER_Register (VOID)
{
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (WC_PAGESCROLLERA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS;
    wndClass.lpfnWndProc   = (WNDPROC)PAGER_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(PAGER_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = WC_PAGESCROLLERA;
 
    RegisterClassA (&wndClass);
}


VOID
PAGER_Unregister (VOID)
{
    if (GlobalFindAtomA (WC_PAGESCROLLERA))
	UnregisterClassA (WC_PAGESCROLLERA, (HINSTANCE)NULL);
}

