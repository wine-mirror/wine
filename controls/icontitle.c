/*
 * Icontitle window class.
 *
 * Copyright 1997 Alex Korobka
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winuser16.h"
#include "wine/unicode.h"
#include "controls.h"
#include "win.h"

static BOOL bMultiLineTitle;
static HFONT hIconTitleFont;

static LRESULT WINAPI IconTitleWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

/*********************************************************************
 * icon title class descriptor
 */
const struct builtin_class_descr ICONTITLE_builtin_class =
{
    ICONTITLE_CLASS_ATOM, /* name */
    CS_GLOBALCLASS,       /* style */
    NULL,                 /* procA (winproc is Unicode only) */
    IconTitleWndProc,     /* procW */
    0,                    /* extra */
    IDC_ARROWA,           /* cursor */
    0                     /* brush */
};



/***********************************************************************
 *           ICONTITLE_Create
 */
HWND ICONTITLE_Create( HWND owner )
{
    WND* wndPtr;
    HWND hWnd;
    HINSTANCE instance = GetWindowLongA( owner, GWL_HINSTANCE );

    if( GetWindowLongA( owner, GWL_STYLE ) & WS_CHILD )
	hWnd = CreateWindowExA( 0, ICONTITLE_CLASS_ATOM, NULL,
				  WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 1, 1,
				  GetParent(owner), 0, instance, NULL );
    else
	hWnd = CreateWindowExA( 0, ICONTITLE_CLASS_ATOM, NULL,
				  WS_CLIPSIBLINGS, 0, 0, 1, 1,
				  owner, 0, instance, NULL );
    wndPtr = WIN_FindWndPtr( hWnd );
    if( wndPtr )
    {
        WND *wnd = WIN_FindWndPtr(owner);
	wndPtr->owner = wnd;	/* MDI depends on this */
	wndPtr->dwStyle &= ~(WS_CAPTION | WS_BORDER);
        if (!IsWindowEnabled(owner)) wndPtr->dwStyle |= WS_DISABLED;
        WIN_ReleaseWndPtr(wndPtr);
        WIN_ReleaseWndPtr(wnd);
	return hWnd;
    }
    return 0;
}

/***********************************************************************
 *           ICONTITLE_SetTitlePos
 */
static BOOL ICONTITLE_SetTitlePos( HWND hwnd, HWND owner )
{
    static WCHAR emptyTitleText[] = {'<','.','.','.','>',0};
    WCHAR str[80];
    HDC hDC;
    HFONT hPrevFont;
    RECT rect;
    INT cx, cy;
    POINT pt;

    int length = GetWindowTextW( owner, str, sizeof(str)/sizeof(WCHAR) );

    while (length && str[length - 1] == ' ') /* remove trailing spaces */
        str[--length] = 0;

    if( !length )
    {
        strcpyW( str, emptyTitleText );
        length = strlenW( str );
    }

    if (!(hDC = GetDC( hwnd ))) return FALSE;

    hPrevFont = SelectObject( hDC, hIconTitleFont );

    SetRect( &rect, 0, 0, GetSystemMetrics(SM_CXICONSPACING) -
             GetSystemMetrics(SM_CXBORDER) * 2,
             GetSystemMetrics(SM_CYBORDER) * 2 );

    DrawTextW( hDC, str, length, &rect, DT_CALCRECT | DT_CENTER | DT_NOPREFIX | DT_WORDBREAK |
               (( bMultiLineTitle ) ? 0 : DT_SINGLELINE) );

    SelectObject( hDC, hPrevFont );
    ReleaseDC( hwnd, hDC );

    cx = rect.right - rect.left +  4 * GetSystemMetrics(SM_CXBORDER);
    cy = rect.bottom - rect.top;

    pt.x = (GetSystemMetrics(SM_CXICON) - cx) / 2;
    pt.y = GetSystemMetrics(SM_CYICON);

    /* point is relative to owner, make it relative to parent */
    MapWindowPoints( owner, GetParent(hwnd), &pt, 1 );

    SetWindowPos( hwnd, owner, pt.x, pt.y, cx, cy, SWP_NOACTIVATE );
    return TRUE;
}

/***********************************************************************
 *           ICONTITLE_Paint
 */
static BOOL ICONTITLE_Paint( HWND hwnd, HWND owner, HDC hDC, BOOL bActive )
{
    HFONT hPrevFont;
    HBRUSH hBrush = 0;
    COLORREF textColor = 0;

    if( bActive )
    {
	hBrush = GetSysColorBrush(COLOR_ACTIVECAPTION);
	textColor = GetSysColor(COLOR_CAPTIONTEXT);
    }
    else 
    {
        if( GetWindowLongA( hwnd, GWL_STYLE ) & WS_CHILD )
	{ 
	    hBrush = (HBRUSH) GetClassLongA(hwnd, GCL_HBRBACKGROUND);
	    if( hBrush )
	    {
		INT level;
		LOGBRUSH logBrush;
		GetObjectA( hBrush, sizeof(logBrush), &logBrush );
		level = GetRValue(logBrush.lbColor) +
			   GetGValue(logBrush.lbColor) +
			      GetBValue(logBrush.lbColor);
		if( level < (0x7F * 3) )
		    textColor = RGB( 0xFF, 0xFF, 0xFF );
	    }
	    else
		hBrush = GetStockObject( WHITE_BRUSH );
	}
	else
	{
	    hBrush = GetStockObject( BLACK_BRUSH );
	    textColor = RGB( 0xFF, 0xFF, 0xFF );    
	}
    }

    FillWindow16( GetParent(hwnd), hwnd, hDC, hBrush );

    hPrevFont = SelectObject( hDC, hIconTitleFont );
    if( hPrevFont )
    {
        RECT  rect;
	INT	length;
	WCHAR buffer[80];

        GetClientRect( hwnd, &rect );

	length = GetWindowTextW( owner, buffer, 80 );
        SetTextColor( hDC, textColor );
        SetBkMode( hDC, TRANSPARENT );

        DrawTextW( hDC, buffer, length, &rect, DT_CENTER | DT_NOPREFIX |
                   DT_WORDBREAK | ((bMultiLineTitle) ? 0 : DT_SINGLELINE) );

	SelectObject( hDC, hPrevFont );
    }
    return ( hPrevFont ) ? TRUE : FALSE;
}

/***********************************************************************
 *           IconTitleWndProc
 */
LRESULT WINAPI IconTitleWndProc( HWND hWnd, UINT msg,
                                 WPARAM wParam, LPARAM lParam )
{
    LRESULT retvalue;
    HWND owner = GetWindow( hWnd, GW_OWNER );
    WND *wnd = WIN_FindWndPtr( hWnd );

    if( !wnd )
      return 0;

    switch( msg )
    {
        case WM_CREATE:
            if (!hIconTitleFont)
            {
                LOGFONTA logFont;
                SystemParametersInfoA( SPI_GETICONTITLELOGFONT, 0, &logFont, 0 );
                SystemParametersInfoA( SPI_GETICONTITLEWRAP, 0, &bMultiLineTitle, 0 );
                hIconTitleFont = CreateFontIndirectA( &logFont );
            }
            retvalue = (hIconTitleFont) ? 0 : -1;
            goto END;
	case WM_NCHITTEST:
	     retvalue = HTCAPTION;
             goto END;
	case WM_NCMOUSEMOVE:
	case WM_NCLBUTTONDBLCLK:
	     retvalue = SendMessageW( owner, msg, wParam, lParam );
             goto END;
	case WM_ACTIVATE:
	     if( wParam ) SetActiveWindow( owner );
	     /* fall through */

	case WM_CLOSE:
	     retvalue = 0;
             goto END;
	case WM_SHOWWINDOW:
	     if( wnd && wParam ) ICONTITLE_SetTitlePos( hWnd, owner );
	     retvalue = 0;
             goto END;
	case WM_ERASEBKGND:
	     if( wnd )
	     {
		 if( GetWindowLongA( owner, GWL_STYLE ) & WS_CHILD )
		     lParam = SendMessageA( owner, WM_ISACTIVEICON, 0, 0 );
		 else
		     lParam = (owner == GetActiveWindow());
		 if( ICONTITLE_Paint( hWnd, owner, (HDC)wParam, (BOOL)lParam ) )
		     ValidateRect( hWnd, NULL );
                 retvalue = 1;
                 goto END;
	     }
    }

    retvalue = DefWindowProcW( hWnd, msg, wParam, lParam );
END:
    WIN_ReleaseWndPtr(wnd);
    return retvalue;
}


