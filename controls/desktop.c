/*
 * Desktop window class.
 *
 * Copyright 1994 Alexandre Julliard

static char Copyright[] = "Copyright  Alexandre Julliard, 1994";
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "win.h"
#include "desktop.h"
#include "dos_fs.h"
#include "graphics.h"


/***********************************************************************
 *           DESKTOP_LoadBitmap
 *
 * Load a bitmap from a file. Used by SetDeskWallPaper().
 */
static HBITMAP DESKTOP_LoadBitmap( HDC hdc, char *filename )
{
    BITMAPFILEHEADER *fileHeader;
    BITMAPINFO *bitmapInfo;
    HBITMAP hbitmap;
    char *unixFileName, *buffer;
    int file;
    long size;

      /* Read all the file into memory */

    if (!(unixFileName = DOS_GetUnixFileName( filename ))) return 0;
    if ((file = open( unixFileName, O_RDONLY )) == -1) return 0;
    size = lseek( file, 0, SEEK_END );
    if (!(buffer = (char *)malloc( size )))
    {
	close( file );
	return 0;
    }
    lseek( file, 0, SEEK_SET );
    size = read( file, buffer, size );
    close( file );
    fileHeader = (BITMAPFILEHEADER *)buffer;
    bitmapInfo = (BITMAPINFO *)(buffer + sizeof(BITMAPFILEHEADER));
    
      /* Check header content */
    if ((fileHeader->bfType != 0x4d42) || (size < fileHeader->bfSize))
    {
	free( buffer );
	return 0;
    }
    hbitmap = CreateDIBitmap( hdc, &bitmapInfo->bmiHeader, CBM_INIT,
			      buffer + fileHeader->bfOffBits,
			      bitmapInfo, DIB_RGB_COLORS );
    free( buffer );
    return hbitmap;
}


/***********************************************************************
 *           DESKTOP_DoEraseBkgnd
 *
 * Handle the WM_ERASEBKGND message.
 */
static LONG DESKTOP_DoEraseBkgnd( HWND hwnd, HDC hdc, DESKTOPINFO *infoPtr )
{
    RECT rect;
    GetClientRect( hwnd, &rect );    

    /* Paint desktop pattern (only if wall paper does not cover everything) */

    if (!infoPtr->hbitmapWallPaper || 
	(!infoPtr->fTileWallPaper && ((infoPtr->bitmapSize.cx < rect.right) ||
	 (infoPtr->bitmapSize.cy < rect.bottom))))
    {
	  /* Set colors in case pattern is a monochrome bitmap */
	SetBkColor( hdc, RGB(0,0,0) );
	SetTextColor( hdc, GetSysColor(COLOR_BACKGROUND) );
	FillRect( hdc, &rect, infoPtr->hbrushPattern );
    }

      /* Paint wall paper */

    if (infoPtr->hbitmapWallPaper)
    {
	int x, y;

	if (infoPtr->fTileWallPaper)
	{
	    for (y = 0; y < rect.bottom; y += infoPtr->bitmapSize.cy)
		for (x = 0; x < rect.right; x += infoPtr->bitmapSize.cx)
		    GRAPH_DrawBitmap( hdc, infoPtr->hbitmapWallPaper,
				      x, y, 0, 0, 
				      infoPtr->bitmapSize.cx,
				      infoPtr->bitmapSize.cy );
	}
	else
	{
	    x = (rect.left + rect.right - infoPtr->bitmapSize.cx) / 2;
	    y = (rect.top + rect.bottom - infoPtr->bitmapSize.cy) / 2;
	    if (x < 0) x = 0;
	    if (y < 0) y = 0;
	    GRAPH_DrawBitmap( hdc, infoPtr->hbitmapWallPaper, x, y, 0, 0, 
                              infoPtr->bitmapSize.cx, infoPtr->bitmapSize.cy );
	}
    }

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
	SetDeskWallPaper( (LPSTR)-1 );
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
BOOL SetDeskPattern(void)
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
    HBITMAP hbitmap;
    HDC hdc;
    char buffer[256];
    WND *wndPtr = WIN_FindWndPtr( GetDesktopWindow() );
    DESKTOPINFO *infoPtr = (DESKTOPINFO *)wndPtr->wExtra;

    if (filename == (LPSTR)-1)
    {
	GetProfileString( "desktop", "WallPaper", "(None)", buffer, 256 );
	filename = buffer;
    }
    hdc = GetDC( 0 );
    hbitmap = DESKTOP_LoadBitmap( hdc, filename );
    ReleaseDC( 0, hdc );
    if (infoPtr->hbitmapWallPaper) DeleteObject( infoPtr->hbitmapWallPaper );
    infoPtr->hbitmapWallPaper = hbitmap;
    infoPtr->fTileWallPaper = GetProfileInt( "desktop", "TileWallPaper", 0 );
    if (hbitmap)
    {
	BITMAP bmp;
	GetObject( hbitmap, sizeof(bmp), (LPSTR)&bmp );
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
	hbitmap = CreateBitmap( 8, 8, 1, 1, (LPSTR)pattern );
	infoPtr->hbrushPattern = CreatePatternBrush( hbitmap );
	DeleteObject( hbitmap );
    }
    else infoPtr->hbrushPattern = CreateSolidBrush( GetSysColor(COLOR_BACKGROUND) );
    return TRUE;
}

