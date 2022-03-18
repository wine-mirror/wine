/*
 * Window painting functions
 *
 * Copyright 1993, 1994, 1995, 2001, 2004, 2005, 2008 Alexandre Julliard
 * Copyright 1996, 1997, 1999 Alex Korobka
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "user_private.h"
#include "win.h"
#include "controls.h"
#include "wine/server.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);


/*************************************************************************
 *             fix_caret
 *
 * Helper for ScrollWindowEx:
 * If the return value is 0, no special caret handling is necessary.
 * Otherwise the return value is the handle of the window that owns the
 * caret. Its caret needs to be hidden during the scroll operation and
 * moved to new_caret_pos if move_caret is TRUE.
 */
static HWND fix_caret(HWND hWnd, const RECT *scroll_rect, INT dx, INT dy,
                     UINT flags, LPBOOL move_caret, LPPOINT new_caret_pos)
{
    GUITHREADINFO info;
    RECT rect, mapped_rcCaret;

    info.cbSize = sizeof(info);
    if (!NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info )) return 0;
    if (!info.hwndCaret) return 0;
    
    mapped_rcCaret = info.rcCaret;
    if (info.hwndCaret == hWnd)
    {
        /* The caret needs to be moved along with scrolling even if it's
         * outside the visible area. Otherwise, when the caret is scrolled
         * out from the view, the position won't get updated anymore and
         * the caret will never scroll back again. */
        *move_caret = TRUE;
        new_caret_pos->x = info.rcCaret.left + dx;
        new_caret_pos->y = info.rcCaret.top + dy;
    }
    else
    {
        *move_caret = FALSE;
        if (!(flags & SW_SCROLLCHILDREN) || !IsChild(hWnd, info.hwndCaret))
            return 0;
        MapWindowPoints(info.hwndCaret, hWnd, (LPPOINT)&mapped_rcCaret, 2);
    }

    /* If the caret is not in the src/dest rects, all is fine done. */
    if (!IntersectRect(&rect, scroll_rect, &mapped_rcCaret))
    {
        rect = *scroll_rect;
        OffsetRect(&rect, dx, dy);
        if (!IntersectRect(&rect, &rect, &mapped_rcCaret))
            return 0;
    }

    /* Indicate that the caret needs to be updated during the scrolling. */
    return info.hwndCaret;
}


/***********************************************************************
 *		GetDC (USER32.@)
 *
 * Get a device context.
 *
 * RETURNS
 *	Success: Handle to the device context
 *	Failure: NULL.
 */
HDC WINAPI GetDC( HWND hwnd )
{
    if (!hwnd) return NtUserGetDCEx( 0, 0, DCX_CACHE | DCX_WINDOW );
    return NtUserGetDCEx( hwnd, 0, DCX_USESTYLE );
}


/***********************************************************************
 *		GetWindowDC (USER32.@)
 */
HDC WINAPI GetWindowDC( HWND hwnd )
{
    return NtUserGetDCEx( hwnd, 0, DCX_USESTYLE | DCX_WINDOW );
}


/***********************************************************************
 *		LockWindowUpdate (USER32.@)
 *
 * Enables or disables painting in the chosen window.
 *
 * PARAMS
 *  hwnd [I] handle to a window.
 *
 * RETURNS
 *  If successful, returns nonzero value. Otherwise,
 *  returns 0.
 *
 * NOTES
 *  You can lock only one window at a time.
 */
BOOL WINAPI LockWindowUpdate( HWND hwnd )
{
    static HWND lockedWnd;

    FIXME("(%p), partial stub!\n",hwnd);

    USER_Lock();
    if (lockedWnd)
    {
        if (!hwnd)
        {
            /* Unlock lockedWnd */
            /* FIXME: Do something */
        }
        else
        {
            /* Attempted to lock a second window */
            /* Return FALSE and do nothing */
            USER_Unlock();
            return FALSE;
        }
    }
    lockedWnd = hwnd;
    USER_Unlock();
    return TRUE;
}


/***********************************************************************
 *		UpdateWindow (USER32.@)
 */
BOOL WINAPI UpdateWindow( HWND hwnd )
{
    if (!hwnd)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

    return NtUserRedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );
}


/***********************************************************************
 *		InvalidateRgn (USER32.@)
 */
BOOL WINAPI InvalidateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    if (!hwnd)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

    return NtUserRedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}


/***********************************************************************
 *		InvalidateRect (USER32.@)
 *
 * MSDN: if hwnd parameter is NULL, InvalidateRect invalidates and redraws
 * all windows and sends WM_ERASEBKGND and WM_NCPAINT.
 */
BOOL WINAPI InvalidateRect( HWND hwnd, const RECT *rect, BOOL erase )
{
    UINT flags = RDW_INVALIDATE | (erase ? RDW_ERASE : 0);

    if (!hwnd)
    {
        flags = RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ERASENOW;
        rect = NULL;
    }

    return NtUserRedrawWindow( hwnd, rect, 0, flags );
}


/***********************************************************************
 *		ValidateRgn (USER32.@)
 */
BOOL WINAPI ValidateRgn( HWND hwnd, HRGN hrgn )
{
    if (!hwnd)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

    return NtUserRedrawWindow( hwnd, NULL, hrgn, RDW_VALIDATE );
}


/***********************************************************************
 *		ValidateRect (USER32.@)
 *
 * MSDN: if hwnd parameter is NULL, ValidateRect invalidates and redraws
 * all windows and sends WM_ERASEBKGND and WM_NCPAINT.
 */
BOOL WINAPI ValidateRect( HWND hwnd, const RECT *rect )
{
    UINT flags = RDW_VALIDATE;

    if (!hwnd)
    {
        flags = RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ERASENOW;
        rect = NULL;
    }

    return NtUserRedrawWindow( hwnd, rect, 0, flags );
}


/***********************************************************************
 *		ExcludeUpdateRgn (USER32.@)
 */
INT WINAPI ExcludeUpdateRgn( HDC hdc, HWND hwnd )
{
    HRGN update_rgn = CreateRectRgn( 0, 0, 0, 0 );
    INT ret = NtUserGetUpdateRgn( hwnd, update_rgn, FALSE );

    if (ret != ERROR)
    {
        DPI_AWARENESS_CONTEXT context;
        POINT pt;

        context = SetThreadDpiAwarenessContext( GetWindowDpiAwarenessContext( hwnd ));
        GetDCOrgEx( hdc, &pt );
        MapWindowPoints( 0, hwnd, &pt, 1 );
        OffsetRgn( update_rgn, -pt.x, -pt.y );
        ret = ExtSelectClipRgn( hdc, update_rgn, RGN_DIFF );
        SetThreadDpiAwarenessContext( context );
    }
    DeleteObject( update_rgn );
    return ret;
}


static INT scroll_window( HWND hwnd, INT dx, INT dy, const RECT *rect, const RECT *clipRect,
                          HRGN hrgnUpdate, LPRECT rcUpdate, UINT flags, BOOL is_ex )
{
    INT   retVal = NULLREGION;
    BOOL  bOwnRgn = TRUE;
    BOOL  bUpdate = (rcUpdate || hrgnUpdate || flags & (SW_INVALIDATE | SW_ERASE));
    int rdw_flags;
    HRGN  hrgnTemp;
    HRGN  hrgnWinupd = 0;
    HDC   hDC;
    RECT  rc, cliprc;
    HWND hwndCaret = NULL;
    BOOL moveCaret = FALSE;
    POINT newCaretPos;

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

    if (clipRect) IntersectRect(&cliprc,&rc,clipRect);
    else cliprc = rc;

    if (rect) IntersectRect(&rc, &rc, rect);

    if( hrgnUpdate ) bOwnRgn = FALSE;
    else if( bUpdate ) hrgnUpdate = CreateRectRgn( 0, 0, 0, 0 );

    newCaretPos.x = newCaretPos.y = 0;

    if( !IsRectEmpty(&cliprc) && (dx || dy)) {
        DWORD dcxflags = 0;
        DWORD style = GetWindowLongW( hwnd, GWL_STYLE );

        hwndCaret = fix_caret(hwnd, &rc, dx, dy, flags, &moveCaret, &newCaretPos);
        if (hwndCaret)
            HideCaret(hwndCaret);

        if (is_ex) dcxflags |= DCX_CACHE;
        if( style & WS_CLIPSIBLINGS) dcxflags |= DCX_CLIPSIBLINGS;
        if( GetClassLongW( hwnd, GCL_STYLE ) & CS_PARENTDC)
            dcxflags |= DCX_PARENTCLIP;
        if( !(flags & SW_SCROLLCHILDREN) && (style & WS_CLIPCHILDREN))
            dcxflags |= DCX_CLIPCHILDREN;
        hDC = NtUserGetDCEx( hwnd, 0, dcxflags);
        if (hDC)
        {
            NtUserScrollDC( hDC, dx, dy, &rc, &cliprc, hrgnUpdate, rcUpdate );

            NtUserReleaseDC( hwnd, hDC );

            if (!bUpdate)
                NtUserRedrawWindow( hwnd, NULL, hrgnUpdate, rdw_flags);
        }

        /* If the windows has an update region, this must be
         * scrolled as well. Keep a copy in hrgnWinupd
         * to be added to hrngUpdate at the end. */
        hrgnTemp = CreateRectRgn( 0, 0, 0, 0 );
        retVal = NtUserGetUpdateRgn( hwnd, hrgnTemp, FALSE );
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
            NtUserRedrawWindow( hwnd, NULL, hrgnTemp, rdw_flags);

           /* Catch the case where the scrolling amount exceeds the size of the
            * original window. This generated a second update area that is the
            * location where the original scrolled content would end up.
            * This second region is not returned by the ScrollDC and sets
            * ScrollWindowEx apart from just a ScrollDC.
            *
            * This has been verified with testing on windows.
            */
            if (abs(dx) > abs(rc.right - rc.left) ||
                abs(dy) > abs(rc.bottom - rc.top))
            {
                SetRectRgn( hrgnTemp, rc.left + dx, rc.top + dy, rc.right+dx, rc.bottom + dy);
                CombineRgn( hrgnTemp, hrgnTemp, hrgnClip, RGN_AND );
                CombineRgn( hrgnUpdate, hrgnUpdate, hrgnTemp, RGN_OR );

                if (rcUpdate)
                {
                    RECT rcTemp;
                    GetRgnBox( hrgnTemp, &rcTemp );
                    UnionRect( rcUpdate, rcUpdate, &rcTemp );
                }

                if( !bOwnRgn)
                    CombineRgn( hrgnWinupd, hrgnWinupd, hrgnTemp, RGN_OR );
            }
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
                WIN_GetRectangles( list[i], COORDS_PARENT, &r, NULL );
                if (!rect || IntersectRect(&dummy, &r, rect))
                    NtUserSetWindowPos( list[i], 0, r.left + dx, r.top  + dy, 0, 0,
                                        SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE |
                                        SWP_NOREDRAW | SWP_DEFERERASE );
            }
            HeapFree( GetProcessHeap(), 0, list );
        }
    }

    if( flags & (SW_INVALIDATE | SW_ERASE) )
        NtUserRedrawWindow( hwnd, NULL, hrgnUpdate, rdw_flags |
                            ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : 0 ) );

    if( hrgnWinupd) {
        CombineRgn( hrgnUpdate, hrgnUpdate, hrgnWinupd, RGN_OR);
        DeleteObject( hrgnWinupd);
    }

    if( moveCaret )
        SetCaretPos( newCaretPos.x, newCaretPos.y );
    if( hwndCaret )
        ShowCaret( hwndCaret );

    if( bOwnRgn && hrgnUpdate ) DeleteObject( hrgnUpdate );

    return retVal;
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
    return scroll_window( hwnd, dx, dy, rect, clipRect, hrgnUpdate, rcUpdate, flags, TRUE );
}

/*************************************************************************
 *		ScrollWindow (USER32.@)
 *
 */
BOOL WINAPI ScrollWindow( HWND hwnd, INT dx, INT dy,
                          const RECT *rect, const RECT *clipRect )
{
    return scroll_window( hwnd, dx, dy, rect, clipRect, 0, NULL,
                          SW_INVALIDATE | SW_ERASE | (rect ? 0 : SW_SCROLLCHILDREN), FALSE ) != ERROR;
}

/************************************************************************
 *		PrintWindow (USER32.@)
 *
 */
BOOL WINAPI PrintWindow(HWND hwnd, HDC hdcBlt, UINT nFlags)
{
    UINT flags = PRF_CHILDREN | PRF_ERASEBKGND | PRF_OWNED | PRF_CLIENT;
    if(!(nFlags & PW_CLIENTONLY))
    {
        flags |= PRF_NONCLIENT;
    }
    SendMessageW(hwnd, WM_PRINT, (WPARAM)hdcBlt, flags);
    return TRUE;
}
