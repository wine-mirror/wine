/*
 * Scroll windows and DCs
 *
 * Copyright  David W. Metcalfe, 1993
 *	      Alex Korobka       1995,1996
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

#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "winuser.h"
#include "user_private.h"
#include "win.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(scroll);

/*************************************************************************
 *             fix_caret
 */
static HWND fix_caret(HWND hWnd, LPRECT lprc, UINT flags)
{
    GUITHREADINFO info;

    if (!GetGUIThreadInfo( GetCurrentThreadId(), &info )) return 0;
    if (!info.hwndCaret) return 0;
    if (info.hwndCaret == hWnd ||
        ((flags & SW_SCROLLCHILDREN) && IsChild(hWnd, info.hwndCaret)))
    {
        POINT pt;
        pt.x = info.rcCaret.left;
        pt.y = info.rcCaret.top;
        MapWindowPoints( info.hwndCaret, hWnd, (LPPOINT)&info.rcCaret, 2 );
        if( IntersectRect(lprc, lprc, &info.rcCaret) )
        {
            HideCaret(0);
            lprc->left = pt.x;
            lprc->top = pt.y;
            return info.hwndCaret;
        }
    }
    return 0;
}

/*************************************************************************
 *		ScrollWindowEx (USER32.@)
 *
 * Note: contrary to what the doc says, pixels that are scrolled from the
 *      outside of clipRect to the inside are NOT painted.
 *
 */
INT WINAPI ScrollWindowEx( HWND hwnd, INT dx, INT dy,
                               const RECT *rect, const RECT *clipRect,
                               HRGN hrgnUpdate, LPRECT rcUpdate,
                               UINT flags )
{
    INT   retVal = NULLREGION;
    BOOL  bOwnRgn = TRUE;
    BOOL  bUpdate = (rcUpdate || hrgnUpdate || flags & (SW_INVALIDATE | SW_ERASE));
    int rdw_flags;
    HRGN  hrgnTemp;
    HRGN  hrgnWinupd = 0;
    HDC   hDC;
    RECT  rc, cliprc;
    RECT caretrc;
    HWND hwndCaret = NULL;

    TRACE( "%p, %d,%d hrgnUpdate=%p rcUpdate = %p %s %04x\n",
           hwnd, dx, dy, hrgnUpdate, rcUpdate, wine_dbgstr_rect(rect), flags );
    TRACE( "clipRect = %s\n", wine_dbgstr_rect(clipRect));
    if( flags & ~( SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE))
        FIXME("some flags (%04x) are unhandled\n", flags);

    rdw_flags = (flags & SW_ERASE) && (flags & SW_INVALIDATE) ?
                                RDW_INVALIDATE | RDW_ERASE  : RDW_INVALIDATE ;

    if (!WIN_IsWindowDrawable( hwnd, TRUE )) return ERROR;
    hwnd = WIN_GetFullHandle( hwnd );

    GetClientRect(hwnd, &rc);
    if (rect) IntersectRect(&rc, &rc, rect);

    if (clipRect) IntersectRect(&cliprc,&rc,clipRect);
    else cliprc = rc;

    if( hrgnUpdate ) bOwnRgn = FALSE;
    else if( bUpdate ) hrgnUpdate = CreateRectRgn( 0, 0, 0, 0 );

    if( !IsRectEmpty(&cliprc) && (dx || dy)) {
        DWORD dcxflags = DCX_CACHE;
        DWORD style = GetWindowLongW( hwnd, GWL_STYLE );
        caretrc = rc;
        hwndCaret = fix_caret(hwnd, &caretrc, flags);

        if( style & WS_CLIPSIBLINGS) dcxflags |= DCX_CLIPSIBLINGS;
        if( GetClassLongW( hwnd, GCL_STYLE ) & CS_PARENTDC)
            dcxflags |= DCX_PARENTCLIP;
        if( !(flags & SW_SCROLLCHILDREN) && (style & WS_CLIPCHILDREN))
            dcxflags |= DCX_CLIPCHILDREN;
        hDC = GetDCEx( hwnd, 0, dcxflags);
        if (hDC)
        {
            ScrollDC( hDC, dx, dy, &rc, &cliprc, hrgnUpdate, rcUpdate );

            ReleaseDC( hwnd, hDC );

            if (!bUpdate)
                RedrawWindow( hwnd, NULL, hrgnUpdate, rdw_flags);
        }

        /* If the windows has an update region, this must be
         * scrolled as well. Keep a copy in hrgnWinupd 
         * to be added to hrngUpdate at the end. */
        hrgnTemp = CreateRectRgn( 0, 0, 0, 0 );
        retVal = GetUpdateRgn( hwnd, hrgnTemp, FALSE );
        if (retVal != NULLREGION)
        {
            HRGN hrgnClip = CreateRectRgnIndirect(&cliprc);
            if( !bOwnRgn) {
                hrgnWinupd = CreateRectRgn( 0, 0, 0, 0);
                CombineRgn( hrgnWinupd, hrgnTemp, 0, RGN_COPY);
            }
            OffsetRgn( hrgnTemp, dx, dy );
            CombineRgn( hrgnTemp, hrgnTemp, hrgnClip, RGN_AND );
            if( !bOwnRgn)
                CombineRgn( hrgnWinupd, hrgnWinupd, hrgnTemp, RGN_OR );
            RedrawWindow( hwnd, NULL, hrgnTemp, rdw_flags);
            DeleteObject( hrgnClip );
        }
        DeleteObject( hrgnTemp );
    } else {
        /* nothing was scrolled */
        if( !bOwnRgn) 
            SetRectRgn( hrgnUpdate, 0, 0, 0, 0 );
        SetRectEmpty( rcUpdate);
    }

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
                if (!rect || IntersectRect(&dummy, &r, rect))
                    SetWindowPos( list[i], 0, r.left + dx, r.top  + dy, 0, 0,
                                  SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE |
                                  SWP_NOREDRAW | SWP_DEFERERASE );
            }
            HeapFree( GetProcessHeap(), 0, list );
        }
    }

    if( flags & (SW_INVALIDATE | SW_ERASE) )
        RedrawWindow( hwnd, NULL, hrgnUpdate, rdw_flags |
                      ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : 0 ) );

    if( hrgnWinupd) { 
        CombineRgn( hrgnUpdate, hrgnUpdate, hrgnWinupd, RGN_OR);
        DeleteObject( hrgnWinupd);
    }

    if( hwndCaret ) {
        SetCaretPos( caretrc.left + dx, caretrc.top + dy );
        ShowCaret(hwndCaret);
    }
    
    if( bOwnRgn && hrgnUpdate ) DeleteObject( hrgnUpdate );

    return retVal;
}

/*************************************************************************
 *		ScrollWindow (USER32.@)
 *
 */
BOOL WINAPI ScrollWindow( HWND hwnd, INT dx, INT dy,
                              const RECT *rect, const RECT *clipRect )
{
    return
        (ERROR != ScrollWindowEx( hwnd, dx, dy, rect, clipRect, 0, NULL,
                                    (rect ? 0 : SW_SCROLLCHILDREN) |
                                    SW_INVALIDATE | SW_ERASE ));
}

/*************************************************************************
 *		ScrollDC (USER32.@)
 *
 * dx, dy, lprcScroll and lprcClip are all in logical coordinates (msdn is
 * wrong) hrgnUpdate is returned in device coordinates with rcUpdate in
 * logical coordinates.
 */
BOOL WINAPI ScrollDC( HDC hdc, INT dx, INT dy, const RECT *lprcScroll,
                      const RECT *lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate )

{
    if (USER_Driver.pScrollDC)
        return USER_Driver.pScrollDC( hdc, dx, dy, lprcScroll, lprcClip, hrgnUpdate, lprcUpdate );
    return FALSE;
}
