/*
 * Static control
 *
 * Copyright  David W. Metcalfe, 1993
 *
static char Copyright[] = "Copyright  David W. Metcalfe, 1993";
*/

#include <stdio.h>
#include <windows.h>
#include "win.h"
#include "user.h"
#include "static.h"
#include "icon.h"

extern void DEFWND_SetText( HWND hwnd, LPSTR text );  /* windows/defwnd.c */

static void PaintTextfn( HWND hwnd, HDC hdc );
static void PaintRectfn( HWND hwnd, HDC hdc );
static void PaintFramefn( HWND hwnd, HDC hdc );
static void PaintIconfn( HWND hwnd, HDC hdc );


static COLORREF color_windowframe, color_background, color_window;


typedef void (*pfPaint)(HWND, HDC);

#define LAST_STATIC_TYPE  SS_LEFTNOWORDWRAP

static pfPaint staticPaintFunc[LAST_STATIC_TYPE+1] =
{
    PaintTextfn,             /* SS_LEFT */
    PaintTextfn,             /* SS_CENTER */
    PaintTextfn,             /* SS_RIGHT */
    PaintIconfn,             /* SS_ICON */
    PaintRectfn,             /* SS_BLACKRECT */
    PaintRectfn,             /* SS_GRAYRECT */
    PaintRectfn,             /* SS_WHITERECT */
    PaintFramefn,            /* SS_BLACKFRAME */
    PaintFramefn,            /* SS_GRAYFRAME */
    PaintFramefn,            /* SS_WHITEFRAME */
    NULL,                    /* Not defined */
    PaintTextfn,             /* SS_SIMPLE */
    PaintTextfn              /* SS_LEFTNOWORDWRAP */
};


/***********************************************************************
 *           STATIC_SetIcon
 *
 * Set the icon for an SS_ICON control.
 */
static void STATIC_SetIcon( HWND hwnd, HICON hicon )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    if ((wndPtr->dwStyle & 0x0f) != SS_ICON) return;
    if (infoPtr->hIcon) DestroyIcon( infoPtr->hIcon );
    infoPtr->hIcon = hicon;
    if (hicon)
    {
        ICONALLOC *icon = (ICONALLOC *) GlobalLock( hicon );
        SetWindowPos( hwnd, 0, 0, 0,
                     icon->descriptor.Width, icon->descriptor.Height,
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
        GlobalUnlock( hicon );
    }
}


/***********************************************************************
 *           StaticWndProc
 */
LONG StaticWndProc(HWND hWnd, WORD uMsg, WORD wParam, LONG lParam)
{
	LONG lResult = 0;
	WND *wndPtr = WIN_FindWndPtr(hWnd);
	LONG style = wndPtr->dwStyle & 0x0000000F;
        STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

	switch (uMsg) {
	case WM_ENABLE:
	    InvalidateRect(hWnd, NULL, FALSE);
	    break;

        case WM_NCCREATE:
	    if (style == SS_ICON)
            {
		CREATESTRUCT * createStruct = (CREATESTRUCT *)lParam;
		if (createStruct->lpszName)
                    STATIC_SetIcon( hWnd, LoadIcon( createStruct->hInstance,
                                                    createStruct->lpszName ));
                return 1;
            }
            return DefWindowProc(hWnd, uMsg, wParam, lParam);

	case WM_CREATE:
	    if (style < 0L || style > LAST_STATIC_TYPE)
            {
                fprintf( stderr, "STATIC: Unknown style 0x%02lx\n", style );
		lResult = -1L;
		break;
            }
	    /* initialise colours */
	    color_windowframe  = GetSysColor(COLOR_WINDOWFRAME);
	    color_background   = GetSysColor(COLOR_BACKGROUND);
	    color_window       = GetSysColor(COLOR_WINDOW);
	    break;

        case WM_NCDESTROY:
            if (style == SS_ICON)
                STATIC_SetIcon( hWnd, 0 );  /* Destroy the current icon */
            else 
                lResult = DefWindowProc(hWnd, uMsg, wParam, lParam);
            break;

	case WM_PAINT:
            {
                PAINTSTRUCT ps;
                BeginPaint( hWnd, &ps );
                if (staticPaintFunc[style])
                    (staticPaintFunc[style])( hWnd, ps.hdc );
                EndPaint( hWnd, &ps );
            }
	    break;

	case WM_SYSCOLORCHANGE:
	    color_windowframe  = GetSysColor(COLOR_WINDOWFRAME);
	    color_background   = GetSysColor(COLOR_BACKGROUND);
	    color_window       = GetSysColor(COLOR_WINDOW);
	    InvalidateRect(hWnd, NULL, TRUE);
	    break;

	case WM_SETTEXT:
	    DEFWND_SetText( hWnd, (LPSTR)lParam );
	    InvalidateRect( hWnd, NULL, FALSE );
	    UpdateWindow( hWnd );
	    break;

        case WM_SETFONT:
            if (style == SS_ICON) return 0;
            infoPtr->hFont = wParam;
            if (LOWORD(lParam))
            {
                InvalidateRect( hWnd, NULL, FALSE );
                UpdateWindow( hWnd );
            }
            break;

        case WM_GETFONT:
            return infoPtr->hFont;

	case WM_NCHITTEST:
	    return HTTRANSPARENT;

        case WM_GETDLGCODE:
            return DLGC_STATIC;

	case STM_GETICON:
	    return infoPtr->hIcon;

	case STM_SETICON:
            STATIC_SetIcon( hWnd, wParam );
            InvalidateRect( hWnd, NULL, FALSE );
            UpdateWindow( hWnd );
	    return 0;

	default:
		lResult = DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}

	return lResult;
}


static void PaintTextfn( HWND hwnd, HDC hdc )
{
    RECT rc;
    HBRUSH hBrush;
    char *text;
    WORD wFormat;

    WND *wndPtr = WIN_FindWndPtr(hwnd);
    LONG style = wndPtr->dwStyle;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    GetClientRect(hwnd, &rc);
    text = USER_HEAP_ADDR( wndPtr->hText );

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

    if (infoPtr->hFont) SelectObject( hdc, infoPtr->hFont );
    hBrush = SendMessage( wndPtr->hwndParent, WM_CTLCOLOR, (WORD)hdc,
                          MAKELONG(hwnd, CTLCOLOR_STATIC));
    if (hBrush == (HBRUSH)NULL) hBrush = GetStockObject(WHITE_BRUSH);
    FillRect(hdc, &rc, hBrush);
    if (text)
	DrawText(hdc, text, -1, &rc, wFormat);
}

static void PaintRectfn( HWND hwnd, HDC hdc )
{
    RECT rc;
    HBRUSH hBrush;

    WND *wndPtr = WIN_FindWndPtr(hwnd);

    GetClientRect(hwnd, &rc);
    
    switch (wndPtr->dwStyle & 0x0000000F)
    {
    case SS_BLACKRECT:
	hBrush = CreateSolidBrush(color_windowframe);
	break;

    case SS_GRAYRECT:
	hBrush = CreateSolidBrush(color_background);
	break;

    case SS_WHITERECT:
	hBrush = CreateSolidBrush(color_window);
	break;
    }
    FillRect( hdc, &rc, hBrush );
}

static void PaintFramefn( HWND hwnd, HDC hdc )
{
    RECT rc;
    HPEN hOldPen, hPen;
    HBRUSH hOldBrush, hBrush;

    WND *wndPtr = WIN_FindWndPtr(hwnd);

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
}


static void PaintIconfn( HWND hwnd, HDC hdc )
{
    RECT 	rc;
    HBRUSH      hbrush;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    GetClientRect(hwnd, &rc);
    hbrush = SendMessage( wndPtr->hwndParent, WM_CTLCOLOR, hdc,
                          MAKELONG(hwnd, CTLCOLOR_STATIC));
    FillRect( hdc, &rc, hbrush );
    if (infoPtr->hIcon) DrawIcon( hdc, rc.left, rc.top, infoPtr->hIcon );
}
