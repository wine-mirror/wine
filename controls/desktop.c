/*
 * Desktop window class.
 *
 * Copyright 1994 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1994";

#include <stdio.h>
#include <string.h>
#include "win.h"
#include "desktop.h"


/***********************************************************************
 *           DESKTOP_DoEraseBkgnd
 *
 * Handle the WM_ERASEBKGND message.
 */
static LONG DESKTOP_DoEraseBkgnd( HWND hwnd, HDC hdc, DESKTOPINFO *infoPtr )
{
    RECT rect;

      /* Set colors in case pattern is a monochrome bitmap */
    SetBkColor( hdc, RGB(0,0,0) );
    SetTextColor( hdc, GetSysColor(COLOR_BACKGROUND) );
    GetClientRect( hwnd, &rect );    
    FillRect( hdc, &rect, infoPtr->hbrushPattern );
    return 1;
}


/***********************************************************************
 *           DesktopWndProc
 *
 * Window procedure for the desktop window.
 */
LONG DesktopWndProc ( HWND hwnd, WORD message, WORD wParam, LONG lParam )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    DESKTOPINFO *infoPtr = (DESKTOPINFO *)wndPtr->wExtra;

      /* Most messages are ignored (we DON'T call DefWindowProc) */

    switch(message)
    {
	/* Warning: this message is sent directly by                     */
	/* WIN_CreateDesktopWindow() and does not contain a valid lParam */
    case WM_NCCREATE:
	infoPtr->hbrushPattern = 0;
	infoPtr->hbitmapWallPaper = 0;
	SetDeskPattern();
	break;
	
    case WM_ERASEBKGND:
	if (rootWindow == DefaultRootWindow(display)) return 1;
	return DESKTOP_DoEraseBkgnd( hwnd, (HDC)wParam, infoPtr );
    }
    
    return 0;
}


/***********************************************************************
 *           SetDeskPattern   (USER.279)
 */
BOOL SetDeskPattern()
{
    char buffer[100];
    GetProfileString( "desktop", "Pattern", "(None)", buffer, 100 );
    return DESKTOP_SetPattern( buffer );
}


/***********************************************************************
 *           SetDeskWallPaper   (USER.285)
 */
BOOL SetDeskWallPaper( LPSTR filename )
{
    return TRUE;
}


/***********************************************************************
 *           DESKTOP_SetPattern
 *
 * Set the desktop pattern.
 */
BOOL DESKTOP_SetPattern(char *pattern )
{
    WND *wndPtr = WIN_FindWndPtr( GetDesktopWindow() );
    DESKTOPINFO *infoPtr = (DESKTOPINFO *)wndPtr->wExtra;
    int pat[8];

    if (infoPtr->hbrushPattern) DeleteObject( infoPtr->hbrushPattern );
    memset( pat, 0, sizeof(pat) );
    if (pattern && sscanf( pattern, " %d %d %d %d %d %d %d %d",
			   &pat[0], &pat[1], &pat[2], &pat[3],
			   &pat[4], &pat[5], &pat[6], &pat[7] ))
    {
	WORD pattern[8];
	HBITMAP hbitmap;
	int i;

	for (i = 0; i < 8; i++) pattern[i] = pat[i] & 0xffff;
	hbitmap = CreateBitmap( 8, 8, 1, 1, pattern );
	infoPtr->hbrushPattern = CreatePatternBrush( hbitmap );
	DeleteObject( hbitmap );
    }
    else infoPtr->hbrushPattern = CreateSolidBrush( GetSysColor(COLOR_BACKGROUND) );
    return TRUE;
}

