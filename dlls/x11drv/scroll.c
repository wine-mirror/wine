/*
 * Scroll windows and DCs
 *
 * Copyright 1993  David W. Metcalfe
 * Copyright 1995, 1996 Alex Korobka
 * Copyright 2001 Alexandre Julliard
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

#include <stdarg.h>
#include <X11/Xlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "x11drv.h"
#include "win.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(scroll);


/*************************************************************************
 *		ScrollWindowEx   (X11DRV.@)
 *
 * Note: contrary to what the doc says, pixels that are scrolled from the
 *      outside of clipRect to the inside are NOT painted.
 *
 * Parameter are the same as in ScrollWindowEx, with the additional
 * requirement that rect and clipRect are _valid_ pointers, to
 * rectangles _within_ the client are. Moreover, there is something
 * to scroll.
 */
INT X11DRV_ScrollWindowEx( HWND hwnd, INT dx, INT dy,
                           const RECT *rect, const RECT *clipRect,
                           HRGN hrgnUpdate, LPRECT rcUpdate, UINT flags )
{
    INT   retVal;
    BOOL  bOwnRgn = TRUE;
    BOOL  bUpdate = (rcUpdate || hrgnUpdate || flags & (SW_INVALIDATE | SW_ERASE));
    HRGN  hrgnTemp;
    HDC   hDC;
    RECT  rc, cliprc;

    TRACE( "%p, %d,%d hrgnUpdate=%p rcUpdate = %p %s %04x\n",
           hwnd, dx, dy, hrgnUpdate, rcUpdate, wine_dbgstr_rect(rect), flags );
    TRACE( "clipRect = %s\n", wine_dbgstr_rect(clipRect));

    GetClientRect(hwnd, &rc);
    if (rect) IntersectRect(&rc, &rc, rect);

    if (clipRect) IntersectRect(&cliprc,&rc,clipRect);
    else cliprc = rc;

    if( hrgnUpdate ) bOwnRgn = FALSE;
    else if( bUpdate ) hrgnUpdate = CreateRectRgn( 0, 0, 0, 0 );

    hDC = GetDCEx( hwnd, 0, DCX_CACHE | DCX_USESTYLE );
    if (hDC)
    {
        HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
        X11DRV_StartGraphicsExposures( hDC );
        ScrollDC( hDC, dx, dy, &rc, &cliprc, hrgnUpdate, rcUpdate );
        X11DRV_EndGraphicsExposures( hDC, hrgn );
        ReleaseDC( hwnd, hDC );
        if (bUpdate) CombineRgn( hrgnUpdate, hrgnUpdate, hrgn, RGN_OR );
        else RedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE | RDW_ERASE );
        DeleteObject( hrgn );
    }

    /* Take into account the fact that some damage may have occurred during the scroll */
    hrgnTemp = CreateRectRgn( 0, 0, 0, 0 );
    retVal = GetUpdateRgn( hwnd, hrgnTemp, FALSE );
    if (retVal != NULLREGION)
    {
        HRGN hrgnClip = CreateRectRgnIndirect(&cliprc);
        OffsetRgn( hrgnTemp, dx, dy );
        CombineRgn( hrgnTemp, hrgnTemp, hrgnClip, RGN_AND );
        RedrawWindow( hwnd, NULL, hrgnTemp, RDW_INVALIDATE | RDW_ERASE );
        DeleteObject( hrgnClip );
    }
    DeleteObject( hrgnTemp );

    if( flags & SW_SCROLLCHILDREN )
    {
        HWND *list = WIN_ListChildren( hwnd );
        if (list)
        {
            int i;
            RECT r, dummy;
            for (i = 0; list[i]; i++)
            {
                GetWindowRect( list[i], &r );
                MapWindowPoints( 0, hwnd, (POINT *)&r, 2 );
                if (!rect || IntersectRect(&dummy, &r, &rc))
                    SetWindowPos( list[i], 0, r.left + dx, r.top  + dy, 0, 0,
                                  SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE |
                                  SWP_NOREDRAW | SWP_DEFERERASE );
            }
            HeapFree( GetProcessHeap(), 0, list );
        }
    }

    if( flags & (SW_INVALIDATE | SW_ERASE) )
        RedrawWindow( hwnd, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ERASE |
                      ((flags & SW_ERASE) ? RDW_ERASENOW : 0) |
                      ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : 0 ) );

    if( bOwnRgn && hrgnUpdate ) DeleteObject( hrgnUpdate );
    
    return retVal;
}
