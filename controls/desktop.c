/*
 * Desktop window class.
 *
 * Copyright 1994 Alexandre Julliard
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "win.h"
#include "desktop.h"
#include "graphics.h"
#include "heap.h"


/***********************************************************************
 *           DESKTOP_LoadBitmap
 *
 * Load a bitmap from a file. Used by SetDeskWallPaper().
 */
static HBITMAP32 DESKTOP_LoadBitmap( HDC32 hdc, const char *filename )
{
    BITMAPFILEHEADER *fileHeader;
    BITMAPINFO *bitmapInfo;
    HBITMAP32 hbitmap;
    HFILE32 file;
    LPSTR buffer;
    LONG size;

    /* Read all the file into memory */

    if ((file = _lopen32( filename, OF_READ )) == HFILE_ERROR32)
    {
        UINT32 len = GetWindowsDirectory32A( NULL, 0 );
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0,
                                  len + strlen(filename) + 2 )))
            return 0;
        GetWindowsDirectory32A( buffer, len + 1 );
        strcat( buffer, "\\" );
        strcat( buffer, filename );
        file = _lopen32( buffer, OF_READ );
        HeapFree( GetProcessHeap(), 0, buffer );
    }
    if (file == HFILE_ERROR32) return 0;
    size = _llseek32( file, 0, 2 );
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size )))
    {
	_lclose32( file );
	return 0;
    }
    _llseek32( file, 0, 0 );
    size = _lread32( file, buffer, size );
    _lclose32( file );
    fileHeader = (BITMAPFILEHEADER *)buffer;
    bitmapInfo = (BITMAPINFO *)(buffer + sizeof(BITMAPFILEHEADER));
    
      /* Check header content */
    if ((fileHeader->bfType != 0x4d42) || (size < fileHeader->bfSize))
    {
	HeapFree( GetProcessHeap(), 0, buffer );
	return 0;
    }
    hbitmap = CreateDIBitmap32( hdc, &bitmapInfo->bmiHeader, CBM_INIT,
                                buffer + fileHeader->bfOffBits,
                                bitmapInfo, DIB_RGB_COLORS );
    HeapFree( GetProcessHeap(), 0, buffer );
    return hbitmap;
}


/***********************************************************************
 *           DESKTOP_DoEraseBkgnd
 *
 * Handle the WM_ERASEBKGND message.
 */
static LRESULT DESKTOP_DoEraseBkgnd( HWND32 hwnd, HDC32 hdc,
                                     DESKTOPINFO *infoPtr )
{
    RECT32 rect;
    WND*   Wnd = WIN_FindWndPtr( hwnd );

    if (Wnd->hrgnUpdate > 1) DeleteObject32( Wnd->hrgnUpdate );
    Wnd->hrgnUpdate = 0;

    GetClientRect32( hwnd, &rect );    

    /* Paint desktop pattern (only if wall paper does not cover everything) */

    if (!infoPtr->hbitmapWallPaper || 
	(!infoPtr->fTileWallPaper && ((infoPtr->bitmapSize.cx < rect.right) ||
	 (infoPtr->bitmapSize.cy < rect.bottom))))
    {
	  /* Set colors in case pattern is a monochrome bitmap */
	SetBkColor32( hdc, RGB(0,0,0) );
	SetTextColor32( hdc, GetSysColor32(COLOR_BACKGROUND) );
	FillRect32( hdc, &rect, infoPtr->hbrushPattern );
    }

      /* Paint wall paper */

    if (infoPtr->hbitmapWallPaper)
    {
	INT32 x, y;

	if (infoPtr->fTileWallPaper)
	{
	    for (y = 0; y < rect.bottom; y += infoPtr->bitmapSize.cy)
		for (x = 0; x < rect.right; x += infoPtr->bitmapSize.cx)
		    GRAPH_DrawBitmap( hdc, infoPtr->hbitmapWallPaper,
				      x, y, 0, 0, 
				      infoPtr->bitmapSize.cx,
				      infoPtr->bitmapSize.cy, FALSE );
	}
	else
	{
	    x = (rect.left + rect.right - infoPtr->bitmapSize.cx) / 2;
	    y = (rect.top + rect.bottom - infoPtr->bitmapSize.cy) / 2;
	    if (x < 0) x = 0;
	    if (y < 0) y = 0;
	    GRAPH_DrawBitmap( hdc, infoPtr->hbitmapWallPaper, 
			      x, y, 0, 0, infoPtr->bitmapSize.cx, 
			      infoPtr->bitmapSize.cy, FALSE );
	}
    }

    return 1;
}


/***********************************************************************
 *           DesktopWndProc
 *
 * Window procedure for the desktop window.
 */
LRESULT WINAPI DesktopWndProc( HWND32 hwnd, UINT32 message,
                               WPARAM32 wParam, LPARAM lParam )
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
	SetDeskWallPaper32( (LPSTR)-1 );
	break;
	
    case WM_ERASEBKGND:
	if (rootWindow == DefaultRootWindow(display)) return 1;
	return DESKTOP_DoEraseBkgnd( hwnd, (HDC32)wParam, infoPtr );

    case WM_SYSCOMMAND:
	if ((wParam & 0xfff0) != SC_CLOSE) return 0;
	ExitWindows16( 0, 0 ); 

    case WM_SETCURSOR:
        return (LRESULT)SetCursor16( LoadCursor16( 0, IDC_ARROW ) );
    }
    
    return 0;
}


/***********************************************************************
 *           SetDeskPattern   (USER.279)
 */
BOOL16 WINAPI SetDeskPattern(void)
{
    char buffer[100];
    GetProfileString32A( "desktop", "Pattern", "(None)", buffer, 100 );
    return DESKTOP_SetPattern( buffer );
}


/***********************************************************************
 *           SetDeskWallPaper16   (USER.285)
 */
BOOL16 WINAPI SetDeskWallPaper16( LPCSTR filename )
{
    return SetDeskWallPaper32( filename );
}


/***********************************************************************
 *           SetDeskWallPaper32   (USER32.475)
 *
 * FIXME: is there a unicode version?
 */
BOOL32 WINAPI SetDeskWallPaper32( LPCSTR filename )
{
    HBITMAP32 hbitmap;
    HDC32 hdc;
    char buffer[256];
    WND *wndPtr = WIN_GetDesktop();
    DESKTOPINFO *infoPtr = (DESKTOPINFO *)wndPtr->wExtra;

    if (filename == (LPSTR)-1)
    {
	GetProfileString32A( "desktop", "WallPaper", "(None)", buffer, 256 );
	filename = buffer;
    }
    hdc = GetDC32( 0 );
    hbitmap = DESKTOP_LoadBitmap( hdc, filename );
    ReleaseDC32( 0, hdc );
    if (infoPtr->hbitmapWallPaper) DeleteObject32( infoPtr->hbitmapWallPaper );
    infoPtr->hbitmapWallPaper = hbitmap;
    infoPtr->fTileWallPaper = GetProfileInt32A( "desktop", "TileWallPaper", 0 );
    if (hbitmap)
    {
	BITMAP32 bmp;
	GetObject32A( hbitmap, sizeof(bmp), &bmp );
	infoPtr->bitmapSize.cx = (bmp.bmWidth != 0) ? bmp.bmWidth : 1;
	infoPtr->bitmapSize.cy = (bmp.bmHeight != 0) ? bmp.bmHeight : 1;
    }
    return TRUE;
}


/***********************************************************************
 *           DESKTOP_SetPattern
 *
 * Set the desktop pattern.
 */
BOOL32 DESKTOP_SetPattern( LPCSTR pattern )
{
    WND *wndPtr = WIN_GetDesktop();
    DESKTOPINFO *infoPtr = (DESKTOPINFO *)wndPtr->wExtra;
    int pat[8];

    if (infoPtr->hbrushPattern) DeleteObject32( infoPtr->hbrushPattern );
    memset( pat, 0, sizeof(pat) );
    if (pattern && sscanf( pattern, " %d %d %d %d %d %d %d %d",
			   &pat[0], &pat[1], &pat[2], &pat[3],
			   &pat[4], &pat[5], &pat[6], &pat[7] ))
    {
	WORD pattern[8];
	HBITMAP32 hbitmap;
	int i;

	for (i = 0; i < 8; i++) pattern[i] = pat[i] & 0xffff;
	hbitmap = CreateBitmap32( 8, 8, 1, 1, (LPSTR)pattern );
	infoPtr->hbrushPattern = CreatePatternBrush32( hbitmap );
	DeleteObject32( hbitmap );
    }
    else infoPtr->hbrushPattern = CreateSolidBrush32( GetSysColor32(COLOR_BACKGROUND) );
    return TRUE;
}

