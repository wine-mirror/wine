/*
 * Desktop window class.
 *
 * Copyright 1994 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "windef.h"
#include "wingdi.h"
#include "user.h"
#include "controls.h"
#include "wine/winuser16.h"

static HBRUSH hbrushPattern;
static HBITMAP hbitmapWallPaper;
static SIZE bitmapSize;
static BOOL fTileWallPaper;

static LRESULT WINAPI DesktopWndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );


/*********************************************************************
 * desktop class descriptor
 */
const struct builtin_class_descr DESKTOP_builtin_class =
{
    DESKTOP_CLASS_ATOM,   /* name */
    CS_GLOBALCLASS,       /* style */
    NULL,                 /* procA (winproc is Unicode only) */
    DesktopWndProc,       /* procW */
    0,                    /* extra */
    IDC_ARROWA,           /* cursor */
    COLOR_BACKGROUND+1    /* brush */
};


/***********************************************************************
 *           DESKTOP_LoadBitmap
 *
 * Load a bitmap from a file. Used by SetDeskWallPaper().
 */
static HBITMAP DESKTOP_LoadBitmap( HDC hdc, const char *filename )
{
    BITMAPFILEHEADER *fileHeader;
    BITMAPINFO *bitmapInfo;
    HBITMAP hbitmap;
    HFILE file;
    LPSTR buffer;
    LONG size;

    /* Read all the file into memory */

    if ((file = _lopen( filename, OF_READ )) == HFILE_ERROR)
    {
        UINT len = GetWindowsDirectoryA( NULL, 0 );
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0,
                                  len + strlen(filename) + 2 )))
            return 0;
        GetWindowsDirectoryA( buffer, len + 1 );
        strcat( buffer, "\\" );
        strcat( buffer, filename );
        file = _lopen( buffer, OF_READ );
        HeapFree( GetProcessHeap(), 0, buffer );
    }
    if (file == HFILE_ERROR) return 0;
    size = _llseek( file, 0, 2 );
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size )))
    {
	_lclose( file );
	return 0;
    }
    _llseek( file, 0, 0 );
    size = _lread( file, buffer, size );
    _lclose( file );
    fileHeader = (BITMAPFILEHEADER *)buffer;
    bitmapInfo = (BITMAPINFO *)(buffer + sizeof(BITMAPFILEHEADER));
    
      /* Check header content */
    if ((fileHeader->bfType != 0x4d42) || (size < fileHeader->bfSize))
    {
	HeapFree( GetProcessHeap(), 0, buffer );
	return 0;
    }
    hbitmap = CreateDIBitmap( hdc, &bitmapInfo->bmiHeader, CBM_INIT,
                                buffer + fileHeader->bfOffBits,
                                bitmapInfo, DIB_RGB_COLORS );
    HeapFree( GetProcessHeap(), 0, buffer );
    return hbitmap;
}



/***********************************************************************
 *           DesktopWndProc
 */
static LRESULT WINAPI DesktopWndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    LRESULT retvalue = 0;

    if (message == WM_NCCREATE)
    {
        hbrushPattern = 0;
        hbitmapWallPaper = 0;
        SetDeskPattern();
        SetDeskWallPaper( (LPSTR)-1 );
        retvalue = 1;
    }
    /* all other messages are ignored */
    return retvalue;
}

/***********************************************************************
 *           PaintDesktop   (USER32.@)
 *
 */
BOOL WINAPI PaintDesktop(HDC hdc)
{
    HWND hwnd = GetDesktopWindow();

    /* check for an owning thread; otherwise don't paint anything (non-desktop mode) */
    if (GetWindowThreadProcessId( hwnd, NULL ))
    {
        RECT rect;

        GetClientRect( hwnd, &rect );

        /* Paint desktop pattern (only if wall paper does not cover everything) */

        if (!hbitmapWallPaper ||
            (!fTileWallPaper && ((bitmapSize.cx < rect.right) || (bitmapSize.cy < rect.bottom))))
        {
            HBRUSH brush = hbrushPattern;
            if (!brush) brush = GetClassLongA( hwnd, GCL_HBRBACKGROUND );
            /* Set colors in case pattern is a monochrome bitmap */
            SetBkColor( hdc, RGB(0,0,0) );
            SetTextColor( hdc, GetSysColor(COLOR_BACKGROUND) );
            FillRect( hdc, &rect, brush );
        }

        /* Paint wall paper */

        if (hbitmapWallPaper)
        {
            INT x, y;
            HDC hMemDC = CreateCompatibleDC( hdc );

            SelectObject( hMemDC, hbitmapWallPaper );

            if (fTileWallPaper)
            {
                for (y = 0; y < rect.bottom; y += bitmapSize.cy)
                    for (x = 0; x < rect.right; x += bitmapSize.cx)
                        BitBlt( hdc, x, y, bitmapSize.cx, bitmapSize.cy, hMemDC, 0, 0, SRCCOPY );
            }
            else
            {
                x = (rect.left + rect.right - bitmapSize.cx) / 2;
                y = (rect.top + rect.bottom - bitmapSize.cy) / 2;
                if (x < 0) x = 0;
                if (y < 0) y = 0;
                BitBlt( hdc, x, y, bitmapSize.cx, bitmapSize.cy, hMemDC, 0, 0, SRCCOPY );
            }
            DeleteDC( hMemDC );
        }
    }
    return TRUE;
}

/***********************************************************************
 *           OldSetDeskPattern   (USER.279)
 */
BOOL16 WINAPI SetDeskPattern(void)
{
    char buffer[100];
    GetProfileStringA( "desktop", "Pattern", "(None)", buffer, 100 );
    return DESKTOP_SetPattern( buffer );
}


/***********************************************************************
 *           SetDeskWallPaper   (USER.285)
 */
BOOL16 WINAPI SetDeskWallPaper16( LPCSTR filename )
{
    return SetDeskWallPaper( filename );
}


/***********************************************************************
 *           SetDeskWallPaper   (USER32.@)
 *
 * FIXME: is there a unicode version?
 */
BOOL WINAPI SetDeskWallPaper( LPCSTR filename )
{
    HBITMAP hbitmap;
    HDC hdc;
    char buffer[256];

    if (filename == (LPSTR)-1)
    {
	GetProfileStringA( "desktop", "WallPaper", "(None)", buffer, 256 );
	filename = buffer;
    }
    hdc = GetDC( 0 );
    hbitmap = DESKTOP_LoadBitmap( hdc, filename );
    ReleaseDC( 0, hdc );
    if (hbitmapWallPaper) DeleteObject( hbitmapWallPaper );
    hbitmapWallPaper = hbitmap;
    fTileWallPaper = GetProfileIntA( "desktop", "TileWallPaper", 0 );
    if (hbitmap)
    {
	BITMAP bmp;
	GetObjectA( hbitmap, sizeof(bmp), &bmp );
	bitmapSize.cx = (bmp.bmWidth != 0) ? bmp.bmWidth : 1;
	bitmapSize.cy = (bmp.bmHeight != 0) ? bmp.bmHeight : 1;
    }
    return TRUE;
}


/***********************************************************************
 *           DESKTOP_SetPattern
 *
 * Set the desktop pattern.
 */
BOOL DESKTOP_SetPattern( LPCSTR pattern )
{
    int pat[8];

    if (hbrushPattern) DeleteObject( hbrushPattern );
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
	hbrushPattern = CreatePatternBrush( hbitmap );
	DeleteObject( hbitmap );
    }
    else hbrushPattern = 0;
    return TRUE;
}

