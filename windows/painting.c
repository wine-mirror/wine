/*
 * Window painting functions
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 *			 1999 Alex Korobka
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "wownt32.h"
#include "wine/unicode.h"
#include "wine/server.h"
#include "user.h"
#include "win.h"
#include "message.h"
#include "dce.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);
WINE_DECLARE_DEBUG_CHANNEL(nonclient);

HPALETTE (WINAPI *pfnGDISelectPalette)(HDC hdc, HPALETTE hpal, WORD bkgnd ) = NULL;
UINT (WINAPI *pfnGDIRealizePalette)(HDC hdc) = NULL;


/***********************************************************************
 *		SelectPalette (Not a Windows API)
 */
HPALETTE WINAPI SelectPalette( HDC hDC, HPALETTE hPal, BOOL bForceBackground )
{
    WORD wBkgPalette = 1;

    if (!bForceBackground && (hPal != GetStockObject(DEFAULT_PALETTE)))
    {
        HWND hwnd = WindowFromDC( hDC );
        if (hwnd)
        {
            HWND hForeground = GetForegroundWindow();
            /* set primary palette if it's related to current active */
            if (hForeground == hwnd || IsChild(hForeground,hwnd)) wBkgPalette = 0;
        }
    }
    return pfnGDISelectPalette( hDC, hPal, wBkgPalette);
}


/***********************************************************************
 *		UserRealizePalette (USER32.@)
 */
UINT WINAPI UserRealizePalette( HDC hDC )
{
    UINT realized = pfnGDIRealizePalette( hDC );

    /* do not send anything if no colors were changed */
    if (realized && IsDCCurrentPalette16( HDC_16(hDC) ))
    {
        /* send palette change notification */
        HWND hWnd = WindowFromDC( hDC );
        if (hWnd) SendMessageTimeoutW( HWND_BROADCAST, WM_PALETTECHANGED, (WPARAM)hWnd, 0,
                                       SMTO_ABORTIFHUNG, 2000, NULL );
    }
    return realized;
}
