/*
 * Static control
 *
 * Copyright  David W. Metcalfe, 1993
 *
 */

#include <stdio.h>
#include <windows.h>
#include "win.h"
#include "static.h"

static void STATIC_PaintTextfn( WND *wndPtr, HDC hdc );
static void STATIC_PaintRectfn( WND *wndPtr, HDC hdc );
static void STATIC_PaintIconfn( WND *wndPtr, HDC hdc );


static COLORREF color_windowframe, color_background, color_window;


typedef void (*pfPaint)( WND *, HDC);

#define LAST_STATIC_TYPE  SS_LEFTNOWORDWRAP

static pfPaint staticPaintFunc[LAST_STATIC_TYPE+1] =
{
    STATIC_PaintTextfn,      /* SS_LEFT */
    STATIC_PaintTextfn,      /* SS_CENTER */
    STATIC_PaintTextfn,      /* SS_RIGHT */
    STATIC_PaintIconfn,      /* SS_ICON */
    STATIC_PaintRectfn,      /* SS_BLACKRECT */
    STATIC_PaintRectfn,      /* SS_GRAYRECT */
    STATIC_PaintRectfn,      /* SS_WHITERECT */
    STATIC_PaintRectfn,      /* SS_BLACKFRAME */
    STATIC_PaintRectfn,      /* SS_GRAYFRAME */
    STATIC_PaintRectfn,      /* SS_WHITEFRAME */
    NULL,                    /* Not defined */
    STATIC_PaintTextfn,      /* SS_SIMPLE */
    STATIC_PaintTextfn       /* SS_LEFTNOWORDWRAP */
};


/***********************************************************************
 *           STATIC_SetIcon
 *
 * Set the icon for an SS_ICON control.
 */
static HICON16 STATIC_SetIcon( WND *wndPtr, HICON16 hicon )
{
    HICON16 prevIcon;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    if ((wndPtr->dwStyle & 0x0f) != SS_ICON) return 0;
    prevIcon = infoPtr->hIcon;
    infoPtr->hIcon = hicon;
    if (hicon)
    {
        CURSORICONINFO *info = (CURSORICONINFO *) GlobalLock16( hicon );
        SetWindowPos( wndPtr->hwndSelf, 0, 0, 0, info->nWidth, info->nHeight,
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
        GlobalUnlock16( hicon );
    }
    return prevIcon;
}


/***********************************************************************
 *           StaticWndProc
 */
LRESULT StaticWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    LONG style = wndPtr->dwStyle & 0x0000000F;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    switch (uMsg)
    {
	case WM_ENABLE:
	    InvalidateRect32( hWnd, NULL, FALSE );
	    break;

        case WM_NCCREATE:
	    if (style == SS_ICON)
            {
		CREATESTRUCT16 *cs = (CREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam);
		if (cs->lpszName)
                {
                    HICON16 hicon = LoadIcon16( cs->hInstance, cs->lpszName );
                    if (!hicon)  /* Try OEM icon (FIXME: is this right?) */
                        hicon = LoadIcon16( 0, cs->lpszName );
                    STATIC_SetIcon( wndPtr, hicon );
                }
                return 1;
            }
            return DefWindowProc16(hWnd, uMsg, wParam, lParam);

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
                DestroyIcon( STATIC_SetIcon( wndPtr, 0 ) );
            else 
                lResult = DefWindowProc16(hWnd, uMsg, wParam, lParam);
            break;

	case WM_PAINT:
            {
                PAINTSTRUCT16 ps;
                BeginPaint16( hWnd, &ps );
                if (staticPaintFunc[style])
                    (staticPaintFunc[style])( wndPtr, ps.hdc );
                EndPaint16( hWnd, &ps );
            }
	    break;

	case WM_SYSCOLORCHANGE:
	    color_windowframe  = GetSysColor(COLOR_WINDOWFRAME);
	    color_background   = GetSysColor(COLOR_BACKGROUND);
	    color_window       = GetSysColor(COLOR_WINDOW);
	    InvalidateRect32( hWnd, NULL, TRUE );
	    break;

	case WM_SETTEXT:
	    if (style == SS_ICON)
	        /* FIXME : should we also return the previous hIcon here ??? */
                STATIC_SetIcon( wndPtr, LoadIcon16( wndPtr->hInstance,
                                                  (SEGPTR)lParam ));
            else
                DEFWND_SetText( wndPtr, (LPSTR)PTR_SEG_TO_LIN(lParam) );
	    InvalidateRect32( hWnd, NULL, FALSE );
	    UpdateWindow( hWnd );
	    break;

        case WM_SETFONT:
            if (style == SS_ICON) return 0;
            infoPtr->hFont = (HFONT)wParam;
            if (LOWORD(lParam))
            {
                InvalidateRect32( hWnd, NULL, FALSE );
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
            lResult = STATIC_SetIcon( wndPtr, (HICON16)wParam );
            InvalidateRect32( hWnd, NULL, FALSE );
            UpdateWindow( hWnd );
	    break;

	default:
		lResult = DefWindowProc16(hWnd, uMsg, wParam, lParam);
		break;
	}

	return lResult;
}


static void STATIC_PaintTextfn( WND *wndPtr, HDC hdc )
{
    RECT16 rc;
    HBRUSH hBrush;
    WORD wFormat;

    LONG style = wndPtr->dwStyle;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    GetClientRect16( wndPtr->hwndSelf, &rc);

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
    hBrush = SendMessage32A( GetParent(wndPtr->hwndSelf), WM_CTLCOLORSTATIC,
                             hdc, wndPtr->hwndSelf );
    if (!hBrush) hBrush = GetStockObject(WHITE_BRUSH);
    FillRect16(hdc, &rc, hBrush);
    if (wndPtr->text) DrawText16( hdc, wndPtr->text, -1, &rc, wFormat );
}

static void STATIC_PaintRectfn( WND *wndPtr, HDC hdc )
{
    RECT16 rc;
    HBRUSH hBrush;

    GetClientRect16( wndPtr->hwndSelf, &rc);
    
    switch (wndPtr->dwStyle & 0x0f)
    {
    case SS_BLACKRECT:
	hBrush = CreateSolidBrush(color_windowframe);
        FillRect16( hdc, &rc, hBrush );
	break;
    case SS_GRAYRECT:
	hBrush = CreateSolidBrush(color_background);
        FillRect16( hdc, &rc, hBrush );
	break;
    case SS_WHITERECT:
	hBrush = CreateSolidBrush(color_window);
        FillRect16( hdc, &rc, hBrush );
	break;
    case SS_BLACKFRAME:
	hBrush = CreateSolidBrush(color_windowframe);
        FrameRect16( hdc, &rc, hBrush );
	break;
    case SS_GRAYFRAME:
	hBrush = CreateSolidBrush(color_background);
        FrameRect16( hdc, &rc, hBrush );
	break;
    case SS_WHITEFRAME:
	hBrush = CreateSolidBrush(color_window);
        FrameRect16( hdc, &rc, hBrush );
	break;
    default:
        return;
    }
    DeleteObject( hBrush );
}


static void STATIC_PaintIconfn( WND *wndPtr, HDC hdc )
{
    RECT16 rc;
    HBRUSH      hbrush;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    GetClientRect16( wndPtr->hwndSelf, &rc);
    hbrush = SendMessage32A( GetParent(wndPtr->hwndSelf), WM_CTLCOLORSTATIC,
                             hdc, wndPtr->hwndSelf );
    FillRect16( hdc, &rc, hbrush );
    if (infoPtr->hIcon) DrawIcon( hdc, rc.left, rc.top, infoPtr->hIcon );
}
