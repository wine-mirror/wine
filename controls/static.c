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

extern void DEFWND_SetText( HWND hwnd, LPSTR text );  /* windows/defwnd.c */

static void PaintTextfn( HWND hwnd, HDC hdc );
static void PaintRectfn( HWND hwnd, HDC hdc );
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
    PaintRectfn,             /* SS_BLACKFRAME */
    PaintRectfn,             /* SS_GRAYFRAME */
    PaintRectfn,             /* SS_WHITEFRAME */
    NULL,                    /* Not defined */
    PaintTextfn,             /* SS_SIMPLE */
    PaintTextfn              /* SS_LEFTNOWORDWRAP */
};


/***********************************************************************
 *           STATIC_SetIcon
 *
 * Set the icon for an SS_ICON control.
 */
static HICON STATIC_SetIcon( HWND hwnd, HICON hicon )
{
    HICON prevIcon;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    if ((wndPtr->dwStyle & 0x0f) != SS_ICON) return 0;
    prevIcon = infoPtr->hIcon;
    infoPtr->hIcon = hicon;
    if (hicon)
    {
        CURSORICONINFO *info = (CURSORICONINFO *) GlobalLock( hicon );
        SetWindowPos( hwnd, 0, 0, 0, info->nWidth, info->nHeight,
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
        GlobalUnlock( hicon );
    }
    return prevIcon;
}


/***********************************************************************
 *           StaticWndProc
 */
LONG StaticWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
		CREATESTRUCT * createStruct = (CREATESTRUCT *)PTR_SEG_TO_LIN(lParam);
		if (createStruct->lpszName)
                {
                    HICON hicon = LoadIcon( createStruct->hInstance,
                                            createStruct->lpszName );
                    if (!hicon)  /* Try OEM icon (FIXME: is this right?) */
                        hicon = LoadIcon( 0, createStruct->lpszName );
                    STATIC_SetIcon( hWnd, hicon );
                }
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
                DestroyIcon( STATIC_SetIcon( hWnd, 0 ) );
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
	    if (style == SS_ICON)
	        /* FIXME : should we also return the previous hIcon here ??? */
                STATIC_SetIcon( hWnd, LoadIcon( wndPtr->hInstance,
                                                (SEGPTR)lParam ));
            else
                DEFWND_SetText( hWnd, (LPSTR)PTR_SEG_TO_LIN(lParam) );
	    InvalidateRect( hWnd, NULL, FALSE );
	    UpdateWindow( hWnd );
	    break;

        case WM_SETFONT:
            if (style == SS_ICON) return 0;
            infoPtr->hFont = (HFONT)wParam;
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
            lResult = STATIC_SetIcon( hWnd, (HICON)wParam );
            InvalidateRect( hWnd, NULL, FALSE );
            UpdateWindow( hWnd );
	    break;

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
    text = USER_HEAP_LIN_ADDR( wndPtr->hText );

    switch (style & 0x0000000F)
    {
    case SS_LEFT:
	wFormat = DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK | DT_NOCLIP;
	break;

    case SS_CENTER:
	wFormat = DT_CENTER | DT_EXPANDTABS | DT_WORDBREAK | DT_NOCLIP;
	break;

    case SS_RIGHT:
	wFormat = DT_RIGHT | DT_EXPANDTABS | DT_WORDBREAK | DT_NOCLIP;
	break;

    case SS_SIMPLE:
	wFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP;
	break;

    case SS_LEFTNOWORDWRAP:
	wFormat = DT_LEFT | DT_SINGLELINE | DT_EXPANDTABS | DT_VCENTER | DT_NOCLIP;
	break;

    default:
        return;
    }

    if (style & SS_NOPREFIX)
	wFormat |= DT_NOPREFIX;

    if (infoPtr->hFont) SelectObject( hdc, infoPtr->hFont );
#ifdef WINELIB32
    hBrush = SendMessage( GetParent(hwnd), WM_CTLCOLORSTATIC, hdc, hwnd );
#else
    hBrush = SendMessage( GetParent(hwnd), WM_CTLCOLOR, (WORD)hdc,
                          MAKELONG(hwnd, CTLCOLOR_STATIC));
#endif
    if (!hBrush) hBrush = GetStockObject(WHITE_BRUSH);
    FillRect(hdc, &rc, hBrush);
    if (text) DrawText( hdc, text, -1, &rc, wFormat );
}

static void PaintRectfn( HWND hwnd, HDC hdc )
{
    RECT rc;
    HBRUSH hBrush;

    WND *wndPtr = WIN_FindWndPtr(hwnd);

    GetClientRect(hwnd, &rc);
    
    switch (wndPtr->dwStyle & 0x0f)
    {
    case SS_BLACKRECT:
	hBrush = CreateSolidBrush(color_windowframe);
        FillRect( hdc, &rc, hBrush );
	break;
    case SS_GRAYRECT:
	hBrush = CreateSolidBrush(color_background);
        FillRect( hdc, &rc, hBrush );
	break;
    case SS_WHITERECT:
	hBrush = CreateSolidBrush(color_window);
        FillRect( hdc, &rc, hBrush );
	break;
    case SS_BLACKFRAME:
	hBrush = CreateSolidBrush(color_windowframe);
        FrameRect( hdc, &rc, hBrush );
	break;
    case SS_GRAYFRAME:
	hBrush = CreateSolidBrush(color_background);
        FrameRect( hdc, &rc, hBrush );
	break;
    case SS_WHITEFRAME:
	hBrush = CreateSolidBrush(color_window);
        FrameRect( hdc, &rc, hBrush );
	break;
    default:
        return;
    }
    DeleteObject( hBrush );
}


static void PaintIconfn( HWND hwnd, HDC hdc )
{
    RECT 	rc;
    HBRUSH      hbrush;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    GetClientRect(hwnd, &rc);
#ifdef WINELIB32
    hbrush = SendMessage( GetParent(hwnd), WM_CTLCOLORSTATIC, hdc, hwnd );
#else
    hbrush = SendMessage( GetParent(hwnd), WM_CTLCOLOR, hdc,
                          MAKELONG(hwnd, CTLCOLOR_STATIC));
#endif
    FillRect( hdc, &rc, hbrush );
    if (infoPtr->hIcon) DrawIcon( hdc, rc.left, rc.top, infoPtr->hIcon );
}
