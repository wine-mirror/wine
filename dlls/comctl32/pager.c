/*
 * Pager control
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
#include "pager.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(pager)


#define PAGER_GetInfoPtr(hwnd) ((PAGER_INFO *)GetWindowLongA(hwnd, 0))


static inline LRESULT
PAGER_ForwardMouse (HWND hwnd, WPARAM wParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);

    infoPtr->bForward = (BOOL)wParam;

    return 0;
}


static inline LRESULT
PAGER_GetBkColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);

    return (LRESULT)infoPtr->clrBk;
}


static inline LRESULT
PAGER_GetBorder (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);

    return (LRESULT)infoPtr->nBorder;
}


static inline LRESULT
PAGER_GetButtonSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);

    return (LRESULT)infoPtr->nButtonSize;
}


static LRESULT
PAGER_GetButtonState (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    /* PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd); */

    FIXME("empty stub!\n");

    return PGF_INVISIBLE;
}


/* << PAGER_GetDropTarget >> */


static inline LRESULT
PAGER_GetPos (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);

    return infoPtr->nPos;
}


static LRESULT
PAGER_RecalcSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    NMPGCALCSIZE nmpgcs;

    if (infoPtr->hwndChild) {
	ZeroMemory (&nmpgcs, sizeof (NMPGCALCSIZE));
	nmpgcs.hdr.hwndFrom = hwnd;
	nmpgcs.hdr.idFrom   = GetWindowLongA (hwnd, GWL_ID);
	nmpgcs.hdr.code = PGN_CALCSIZE;
	nmpgcs.dwFlag = (dwStyle & PGS_HORZ) ? PGF_CALCWIDTH : PGF_CALCHEIGHT;
	SendMessageA (GetParent (hwnd), WM_NOTIFY,
			(WPARAM)nmpgcs.hdr.idFrom, (LPARAM)&nmpgcs);

	infoPtr->nChildSize = (dwStyle & PGS_HORZ) ? nmpgcs.iWidth : nmpgcs.iHeight;


        FIXME("Child size %d\n", infoPtr->nChildSize);


    }

    return 0;
}


static inline LRESULT
PAGER_SetBkColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    COLORREF clrTemp = infoPtr->clrBk;

    infoPtr->clrBk = (COLORREF)lParam;

    /* FIXME: redraw */

    return (LRESULT)clrTemp;
}


static inline LRESULT
PAGER_SetBorder (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    INT nTemp = infoPtr->nBorder;

    infoPtr->nBorder = (INT)lParam;

    /* FIXME: redraw */

    return (LRESULT)nTemp;
}


static inline LRESULT
PAGER_SetButtonSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    INT nTemp = infoPtr->nButtonSize;

    infoPtr->nButtonSize = (INT)lParam;

    FIXME("size=%d\n", infoPtr->nButtonSize);

    /* FIXME: redraw */

    return (LRESULT)nTemp;
}


static inline LRESULT
PAGER_SetChild (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);

    infoPtr->hwndChild = IsWindow ((HWND)lParam) ? (HWND)lParam : 0;

    FIXME("hwnd=%x\n", infoPtr->hwndChild);

    /* FIXME: redraw */
    if (infoPtr->hwndChild) {
	RECT rect;

	GetClientRect (hwnd, &rect);
	SetParent (infoPtr->hwndChild, hwnd);
	SetWindowPos (infoPtr->hwndChild, HWND_TOP,
			0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);

	MoveWindow (infoPtr->hwndChild, 0, 0, rect.right, rect.bottom, TRUE);
    }

    return 0;
}


static inline LRESULT
PAGER_SetPos (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);

    infoPtr->nPos = (INT)lParam;

    FIXME("pos=%d\n", infoPtr->nPos);

    /* FIXME: redraw */
    SetWindowPos (infoPtr->hwndChild, HWND_TOP,
		    0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);

    return 0;
}


static LRESULT
PAGER_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (PAGER_INFO *)COMCTL32_Alloc (sizeof(PAGER_INFO));
    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

    /* set default settings */
    infoPtr->hwndChild = (HWND)NULL;
    infoPtr->clrBk = GetSysColor (COLOR_BTNFACE);
    infoPtr->nBorder = 0;
    infoPtr->nButtonSize = 0;
    infoPtr->nPos = 0;


    return 0;
}


static LRESULT
PAGER_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);




    /* free pager info data */
    COMCTL32_Free (infoPtr);
    SetWindowLongA (hwnd, 0, 0);
    return 0;
}


static LRESULT
PAGER_EraseBackground (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    HBRUSH hBrush = CreateSolidBrush (infoPtr->clrBk);
    RECT rect;

    GetClientRect (hwnd, &rect);
    FillRect ((HDC)wParam, &rect, hBrush);
    DeleteObject (hBrush);

/*    return TRUE; */
    return FALSE;
}


static LRESULT
PAGER_MouseMove (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    /* PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd); */

    TRACE("stub!\n");

    return 0;
}


/* << PAGER_Paint >> */


static LRESULT
PAGER_Size (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    RECT rect;

    GetClientRect (hwnd, &rect);
    if (infoPtr->hwndChild) {
	SetWindowPos (infoPtr->hwndChild, HWND_TOP, rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top,
			SWP_SHOWWINDOW);
/*	MoveWindow (infoPtr->hwndChild, 1, 1, rect.right - 2, rect.bottom-2, TRUE); */
/*	UpdateWindow (infoPtr->hwndChild); */

    }
/*    FillRect ((HDC)wParam, &rect, hBrush); */
/*    DeleteObject (hBrush); */
    return TRUE;
}



static LRESULT WINAPI
PAGER_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("hwnd=%x msg=%x wparam=%x lparam=%lx\n", hwnd, uMsg, wParam, lParam);
    if (!PAGER_GetInfoPtr (hwnd) && (uMsg != WM_CREATE))
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    switch (uMsg)
    {
	case PGM_FORWARDMOUSE:
	    return PAGER_ForwardMouse (hwnd, wParam);

	case PGM_GETBKCOLOR:
	    return PAGER_GetBkColor (hwnd, wParam, lParam);

	case PGM_GETBORDER:
	    return PAGER_GetBorder (hwnd, wParam, lParam);

	case PGM_GETBUTTONSIZE:
	    return PAGER_GetButtonSize (hwnd, wParam, lParam);

	case PGM_GETBUTTONSTATE:
	    return PAGER_GetButtonState (hwnd, wParam, lParam);

/*	case PGM_GETDROPTARGET: */

	case PGM_GETPOS:
	    return PAGER_SetPos (hwnd, wParam, lParam);

	case PGM_RECALCSIZE:
	    return PAGER_RecalcSize (hwnd, wParam, lParam);

	case PGM_SETBKCOLOR:
	    return PAGER_SetBkColor (hwnd, wParam, lParam);

	case PGM_SETBORDER:
	    return PAGER_SetBorder (hwnd, wParam, lParam);

	case PGM_SETBUTTONSIZE:
	    return PAGER_SetButtonSize (hwnd, wParam, lParam);

	case PGM_SETCHILD:
	    return PAGER_SetChild (hwnd, wParam, lParam);

	case PGM_SETPOS:
	    return PAGER_SetPos (hwnd, wParam, lParam);

	case WM_CREATE:
	    return PAGER_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return PAGER_Destroy (hwnd, wParam, lParam);

	case WM_ERASEBKGND:
	    return PAGER_EraseBackground (hwnd, wParam, lParam);

	case WM_MOUSEMOVE:
	    return PAGER_MouseMove (hwnd, wParam, lParam);

	case WM_NOTIFY:
	case WM_COMMAND:
	    return SendMessageA (GetParent (hwnd), uMsg, wParam, lParam);

/*	case WM_PAINT: */
/*	    return PAGER_Paint (hwnd, wParam); */

	case WM_SIZE:
	    return PAGER_Size (hwnd, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR("unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
PAGER_Register (void)
{
    WNDCLASSA wndClass;

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
PAGER_Unregister (void)
{
    UnregisterClassA (WC_PAGESCROLLERA, (HINSTANCE)NULL);
}

