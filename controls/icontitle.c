/*
 * Icontitle window class.
 *
 * Copyright 1997 Alex Korobka
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "windows.h"
#include "sysmetrics.h"
#include "win.h"
#include "desktop.h"
#include "graphics.h"
#include "heap.h"

static  LPCSTR	emptyTitleText = "<...>";

	BOOL32	bMultiLineTitle;
	HFONT32	hIconTitleFont;

/***********************************************************************
 *           ICONTITLE_Init
 */
BOOL32 ICONTITLE_Init(void)
{
    LOGFONT32A logFont;

    SystemParametersInfo32A( SPI_GETICONTITLELOGFONT, 0, &logFont, 0 );
    SystemParametersInfo32A( SPI_GETICONTITLEWRAP, 0, &bMultiLineTitle, 0 );
    hIconTitleFont = CreateFontIndirect32A( &logFont );
    return (hIconTitleFont) ? TRUE : FALSE;
}

/***********************************************************************
 *           ICONTITLE_Create
 */
HWND32 ICONTITLE_Create( WND* wnd )
{
    WND* wndPtr;
    HWND32 hWnd;

    if( wnd->dwStyle & WS_CHILD )
	hWnd = CreateWindowEx32A( 0, ICONTITLE_CLASS_ATOM, NULL,
				  WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 1, 1,
				  wnd->parent->hwndSelf, 0, wnd->hInstance, NULL );
    else
	hWnd = CreateWindowEx32A( 0, ICONTITLE_CLASS_ATOM, NULL,
				  WS_CLIPSIBLINGS, 0, 0, 1, 1,
				  wnd->hwndSelf, 0, wnd->hInstance, NULL );
    wndPtr = WIN_FindWndPtr( hWnd );
    if( wndPtr )
    {
	wndPtr->owner = wnd;	/* MDI depends on this */
	wndPtr->dwStyle &= ~(WS_CAPTION | WS_BORDER);
	if( wnd->dwStyle & WS_DISABLED ) wndPtr->dwStyle |= WS_DISABLED;
	return hWnd;
    }
    return 0;
}

/***********************************************************************
 *           ICONTITLE_GetTitlePos
 */
static BOOL32 ICONTITLE_GetTitlePos( WND* wnd, LPRECT32 lpRect )
{
    LPSTR str;
    int length = lstrlen32A( wnd->owner->text );

    if( length )
    {
	str = HeapAlloc( GetProcessHeap(), 0, length + 1 );
	lstrcpy32A( str, wnd->owner->text );
	while( str[length - 1] == ' ' ) /* remove trailing spaces */
	{ 
	    str[--length] = '\0';
	    if( !length )
	    {
		HeapFree( GetProcessHeap(), 0, str );
		break;
	    }
	}
    }
    if( !length ) 
    {
	str = (LPSTR)emptyTitleText;
	length = lstrlen32A( str );
    }

    if( str )
    {
	HDC32 hDC = GetDC32( wnd->hwndSelf );
	if( hDC )
	{
	    HFONT32 hPrevFont = SelectObject32( hDC, hIconTitleFont );

	    SetRect32( lpRect, 0, 0, sysMetrics[SM_CXICONSPACING] -
		       SYSMETRICS_CXBORDER * 2, SYSMETRICS_CYBORDER * 2 );

	    DrawText32A( hDC, str, length, lpRect, DT_CALCRECT |
			 DT_CENTER | DT_NOPREFIX | DT_WORDBREAK |
			 (( bMultiLineTitle ) ? 0 : DT_SINGLELINE) );

	    SelectObject32( hDC, hPrevFont );
	    ReleaseDC32( wnd->hwndSelf, hDC );

	    lpRect->right += 4 * SYSMETRICS_CXBORDER - lpRect->left;
	    lpRect->left = wnd->owner->rectWindow.left + SYSMETRICS_CXICON / 2 -
				      (lpRect->right - lpRect->left) / 2;
	    lpRect->bottom -= lpRect->top;
	    lpRect->top = wnd->owner->rectWindow.top + SYSMETRICS_CYICON;
	}
	if( str != emptyTitleText ) HeapFree( GetProcessHeap(), 0, str );
	return ( hDC ) ? TRUE : FALSE;
    }
    return FALSE;
}

/***********************************************************************
 *           ICONTITLE_Paint
 */
static BOOL32 ICONTITLE_Paint( WND* wnd, HDC32 hDC, BOOL32 bActive )
{
    HFONT32 hPrevFont;
    HBRUSH32 hBrush = 0;
    COLORREF textColor = 0;

    if( bActive )
    {
	hBrush = GetSysColorBrush32(COLOR_ACTIVECAPTION);
	textColor = GetSysColor32(COLOR_CAPTIONTEXT);
    }
    else 
    {
	if( wnd->dwStyle & WS_CHILD ) 
	{ 
	    hBrush = wnd->parent->class->hbrBackground;
	    if( hBrush )
	    {
		INT32 level;
		LOGBRUSH32 logBrush;
		GetObject32A( hBrush, sizeof(logBrush), &logBrush );
		level = GetRValue(logBrush.lbColor) +
			   GetGValue(logBrush.lbColor) +
			      GetBValue(logBrush.lbColor);
		if( level < (0x7F * 3) )
		    textColor = RGB( 0xFF, 0xFF, 0xFF );
	    }
	    else
		hBrush = GetStockObject32( WHITE_BRUSH );
	}
	else
	{
	    hBrush = GetStockObject32( BLACK_BRUSH );
	    textColor = RGB( 0xFF, 0xFF, 0xFF );    
	}
    }

    FillWindow( wnd->parent->hwndSelf, wnd->hwndSelf, hDC, hBrush );

    hPrevFont = SelectObject32( hDC, hIconTitleFont );
    if( hPrevFont )
    {
        RECT32  rect;
	INT32	length;
	char	buffer[80];

	rect.left = rect.top = 0;
	rect.right = wnd->rectWindow.right - wnd->rectWindow.left;
	rect.bottom = wnd->rectWindow.bottom - wnd->rectWindow.top;

	length = GetWindowText32A( wnd->owner->hwndSelf, buffer, 80 );
        SetTextColor32( hDC, textColor );
        SetBkMode32( hDC, TRANSPARENT );
	
	DrawText32A( hDC, buffer, length, &rect, DT_CENTER | DT_NOPREFIX |
		     DT_WORDBREAK | ((bMultiLineTitle) ? 0 : DT_SINGLELINE) ); 

	SelectObject32( hDC, hPrevFont );
    }
    return ( hPrevFont ) ? TRUE : FALSE;
}

/***********************************************************************
 *           IconTitleWndProc
 */
LRESULT WINAPI IconTitleWndProc( HWND32 hWnd, UINT32 msg,
                                 WPARAM32 wParam, LPARAM lParam )
{
    WND *wnd = WIN_FindWndPtr( hWnd );

    switch( msg )
    {
	case WM_NCHITTEST:
	     return HTCAPTION;

	case WM_NCMOUSEMOVE:
	case WM_NCLBUTTONDBLCLK:
	     return SendMessage32A( wnd->owner->hwndSelf, msg, wParam, lParam );	

	case WM_ACTIVATE:
	     if( wParam ) SetActiveWindow32( wnd->owner->hwndSelf );
	     /* fall through */

	case WM_CLOSE:
	     return 0;

	case WM_SHOWWINDOW:
	     if( wnd && wParam )
	     {
		 RECT32 titleRect;

		 ICONTITLE_GetTitlePos( wnd, &titleRect );
		 if( wnd->owner->next != wnd )	/* keep icon title behind the owner */
		     SetWindowPos32( hWnd, wnd->owner->hwndSelf, 
				     titleRect.left, titleRect.top,
				     titleRect.right, titleRect.bottom, SWP_NOACTIVATE );
		 else
		     SetWindowPos32( hWnd, 0, titleRect.left, titleRect.top,
				     titleRect.right, titleRect.bottom, 
				     SWP_NOACTIVATE | SWP_NOZORDER );
	     }
	     return 0;

	case WM_ERASEBKGND:
	     if( wnd )
	     {
		 WND* iconWnd = wnd->owner;

		 if( iconWnd->dwStyle & WS_CHILD )
		     lParam = SendMessage32A( iconWnd->hwndSelf, WM_ISACTIVEICON, 0, 0 );
		 else
		     lParam = (iconWnd->hwndSelf == GetActiveWindow16());

		 if( ICONTITLE_Paint( wnd, (HDC32)wParam, (BOOL32)lParam ) )
		     ValidateRect32( hWnd, NULL );
	         return 1;
	     }
    }

    return DefWindowProc32A( hWnd, msg, wParam, lParam );
}


