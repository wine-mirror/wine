/*
 * Static control
 *
 * Copyright  David W. Metcalfe, 1993
 *
 */

static char Copyright[] = "Copyright  David W. Metcalfe, 1993";

#include <windows.h>
#include "win.h"
#include "user.h"

LONG StaticWndProc(HWND hWnd, WORD uMsg, WORD wParam, LONG lParam);

static LONG PaintTextfn(HWND hwnd);
static LONG PaintRectfn(HWND hwnd);
static LONG PaintFramefn(HWND hwnd);
static LONG PaintIconfn(HWND hwnd);


static COLORREF color_windowframe, color_background, color_window,
                                                     color_windowtext;

#define NOTIFY_PARENT(hWndCntrl, wNotifyCode) \
	SendMessage(GetParent(hWndCntrl), WM_COMMAND, \
		    GetDlgCtrlID(hWndCntrl), MAKELPARAM(hWndCntrl, wNotifyCode));
#define DIM(array)	((sizeof array)/(sizeof array[0]))

typedef struct
{
    LONG (*paintfn)();
} STATICFN;

#define MAX_STATIC_TYPE  12

static STATICFN staticfn[MAX_STATIC_TYPE] =
{
    { (LONG(*)())PaintTextfn },                    /* SS_LEFT */
    { (LONG(*)())PaintTextfn },                    /* SS_CENTER */
    { (LONG(*)())PaintTextfn },                    /* SS_RIGHT */
    { (LONG(*)())PaintIconfn },                    /* SS_ICON */
    { (LONG(*)())PaintRectfn },                    /* SS_BLACKRECT */
    { (LONG(*)())PaintRectfn },                    /* SS_GRAYRECT */
    { (LONG(*)())PaintRectfn },                    /* SS_WHITERECT */
    { (LONG(*)())PaintFramefn },                   /* SS_BLACKFRAME */
    { (LONG(*)())PaintFramefn },                   /* SS_GRAYFRAME */
    { (LONG(*)())PaintFramefn },                   /* SS_WHITEFRAME */
    { (LONG(*)())PaintTextfn },                    /* SS_SIMPLE */
    { (LONG(*)())PaintTextfn }                     /* SS_LEFTNOWORDWRAP */
};


LONG StaticWndProc(HWND hWnd, WORD uMsg, WORD wParam, LONG lParam)
{
	LONG lResult = 0;
	HDC hDC;
	RECT rc;
	LPSTR textPtr;

	WND *wndPtr = WIN_FindWndPtr(hWnd);
	LONG style = wndPtr->dwStyle & 0x0000000F;

	switch (uMsg) {
	case WM_ENABLE:
	    InvalidateRect(hWnd, NULL, FALSE);
	    break;

	case WM_CREATE:
	    if (style < 0L || style >= (LONG)DIM(staticfn)) {
		lResult = -1L;
		break;
		}
	    /* initialise colours */
	    color_windowframe  = GetSysColor(COLOR_WINDOWFRAME);
	    color_background   = GetSysColor(COLOR_BACKGROUND);
	    color_window       = GetSysColor(COLOR_WINDOW);
	    color_windowtext   = GetSysColor(COLOR_WINDOWTEXT);
	    lResult = 0L;
	    if (style == SS_ICON) {
/*
		SetWindowPos(hWnd, (HWND)NULL, 0, 0, 32, 32,
				SWP_NOZORDER | SWP_NOMOVE);
*/
		}
	    break;

	case WM_PAINT:
	    if (staticfn[style].paintfn)
		(staticfn[style].paintfn)(hWnd);
	    break;

	case WM_SYSCOLORCHANGE:
	    color_windowframe  = GetSysColor(COLOR_WINDOWFRAME);
	    color_background   = GetSysColor(COLOR_BACKGROUND);
	    color_window       = GetSysColor(COLOR_WINDOW);
	    color_windowtext   = GetSysColor(COLOR_WINDOWTEXT);
	    InvalidateRect(hWnd, NULL, TRUE);
	    break;

	case WM_SETTEXT:
	    if (wndPtr->hText)
		USER_HEAP_FREE(wndPtr->hText);

	    wndPtr->hText = USER_HEAP_ALLOC(GMEM_MOVEABLE, 
					    strlen((LPSTR)lParam) + 1);
	    textPtr = (LPSTR)USER_HEAP_ADDR(wndPtr->hText);
	    strcpy(textPtr, (LPSTR)lParam);
	    InvalidateRect(hWnd, NULL, TRUE);
	    break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CHAR:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
	    return(SendMessage(wndPtr->hwndParent, uMsg, wParam, lParam));

	default:
		lResult = DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}

	GlobalUnlock(hWnd);
	return lResult;
}


static LONG PaintTextfn(HWND hwnd)
{
    PAINTSTRUCT ps;
    RECT rc;
    HDC hdc;
    HBRUSH hBrush;
    HANDLE hText;
    char *text;
    int textlen;
    WORD wFormat;

    WND *wndPtr = WIN_FindWndPtr(hwnd);
    LONG style = wndPtr->dwStyle;

    hdc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rc);

    textlen = GetWindowTextLength(hwnd);
    hText = USER_HEAP_ALLOC(0, textlen+1);
    text = USER_HEAP_ADDR(hText);
    GetWindowText(hwnd, text, textlen+1);

    switch (style & 0x0000000F)
    {
    case SS_LEFT:
	wFormat = DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK;
	break;

    case SS_CENTER:
	wFormat = DT_CENTER | DT_EXPANDTABS | DT_WORDBREAK;
	break;

    case SS_RIGHT:
	wFormat = DT_RIGHT | DT_EXPANDTABS | DT_WORDBREAK;
	break;

    case SS_SIMPLE:
	wFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER;
	break;

    case SS_LEFTNOWORDWRAP:
	wFormat = DT_LEFT | DT_SINGLELINE | DT_EXPANDTABS | DT_VCENTER;
	break;
    }

    if (style & SS_NOPREFIX)
	wFormat |= DT_NOPREFIX;

    hBrush = SendMessage(GetParent(hwnd), WM_CTLCOLOR, (WORD)hdc,
		    MAKELONG(hwnd, CTLCOLOR_STATIC));
    if (hBrush == (HBRUSH)NULL) hBrush = GetStockObject(WHITE_BRUSH);
    FillRect(hdc, &rc, hBrush);
    DrawText(hdc, text, textlen, &rc, wFormat);

    USER_HEAP_FREE(hText);
    GlobalUnlock(hwnd);
    EndPaint(hwnd, &ps);
}

static LONG PaintRectfn(HWND hwnd)
{
    PAINTSTRUCT ps;
    RECT rc;
    HDC hdc;
    HPEN hOldPen, hPen;
    HBRUSH hOldBrush, hBrush;

    WND *wndPtr = WIN_FindWndPtr(hwnd);

    hdc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rc);
    
    switch (wndPtr->dwStyle & 0x0000000F)
    {
    case SS_BLACKRECT:
	hPen = CreatePen(PS_SOLID, 1, color_windowframe);
	hBrush = CreateSolidBrush(color_windowframe);
	break;

    case SS_GRAYRECT:
	hPen = CreatePen(PS_SOLID, 1, color_background);
	hBrush = CreateSolidBrush(color_background);
	break;

    case SS_WHITERECT:
	hPen = CreatePen(PS_SOLID, 1, color_window);
	hBrush = CreateSolidBrush(color_window);
	break;
    }

    hOldPen = (HPEN)SelectObject(hdc, (HANDLE)hPen);
    hOldBrush = (HBRUSH)SelectObject(hdc, (HANDLE)hBrush);
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    SelectObject(hdc, (HANDLE)hOldPen);
    SelectObject(hdc, (HANDLE)hOldBrush);
    DeleteObject((HANDLE)hPen);
    DeleteObject((HANDLE)hBrush);

    GlobalUnlock(hwnd);
    EndPaint(hwnd, &ps);
}

static LONG PaintFramefn(HWND hwnd)
{
    PAINTSTRUCT ps;
    RECT rc;
    HDC hdc;
    HPEN hOldPen, hPen;
    HBRUSH hOldBrush, hBrush;

    WND *wndPtr = WIN_FindWndPtr(hwnd);

    hdc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rc);
    
    switch (wndPtr->dwStyle & 0x0000000F)
    {
    case SS_BLACKFRAME:
	hPen = CreatePen(PS_SOLID, 1, color_windowframe);
	break;

    case SS_GRAYFRAME:
	hPen = CreatePen(PS_SOLID, 1, color_background);
	break;

    case SS_WHITEFRAME:
	hPen = CreatePen(PS_SOLID, 1, color_window);
	break;
    }

    hBrush = CreateSolidBrush(color_window);
    hOldPen = (HPEN)SelectObject(hdc, (HANDLE)hPen);
    hOldBrush = (HBRUSH)SelectObject(hdc, (HANDLE)hBrush);
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    SelectObject(hdc, (HANDLE)hOldPen);
    SelectObject(hdc, (HANDLE)hOldBrush);
    DeleteObject((HANDLE)hPen);
    DeleteObject((HANDLE)hBrush);

    GlobalUnlock(hwnd);
    EndPaint(hwnd, &ps);
}


static LONG PaintIconfn(HWND hwnd)
{
    WND 	*wndPtr;
    PAINTSTRUCT ps;
    RECT 	rc;
    HDC 	hdc;
    LPSTR	textPtr;
    HICON	hIcon;

    wndPtr = WIN_FindWndPtr(hwnd);
    hdc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, GetStockObject(WHITE_BRUSH));
    textPtr = (LPSTR)USER_HEAP_ADDR(wndPtr->hText);
    printf("SS_ICON : textPtr='%s' / left=%d top=%d right=%d bottom=%d \n", 
    		textPtr, rc.left, rc.top, rc.right, rc.bottom);
/*
    SetWindowPos(hwnd, (HWND)NULL, 0, 0, 32, 32,
		SWP_NOZORDER | SWP_NOMOVE);
    GetClientRect(hwnd, &rc);
    printf("SS_ICON : textPtr='%s' / left=%d top=%d right=%d bottom=%d \n", 
    		textPtr, rc.left, rc.top, rc.right, rc.bottom);
*/
    hIcon = LoadIcon(wndPtr->hInstance, textPtr);
    DrawIcon(hdc, rc.left, rc.top, hIcon);
    EndPaint(hwnd, &ps);
    GlobalUnlock(hwnd);
}






