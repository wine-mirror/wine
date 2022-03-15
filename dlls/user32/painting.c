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


/***********************************************************************
 *           free_dce
 *
 * Free a class or window DCE.
 */
void free_dce( struct dce *dce, HWND hwnd )
{
    /* FIXME: move callers to win32u */
    NtUserCallTwoParam( (UINT_PTR)dce, HandleToUlong(hwnd), NtUserFreeDCE );
}


/***********************************************************************
 *   delete_clip_rgn
 */
static void delete_clip_rgn( struct dce *dce )
{
    if (!dce->clip_rgn) return;  /* nothing to do */

    dce->flags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN);
    DeleteObject( dce->clip_rgn );
    dce->clip_rgn = 0;

    /* make it dirty so that the vis rgn gets recomputed next time */
    SetHookFlags( dce->hdc, DCHF_INVALIDATEVISRGN );
}


/***********************************************************************
 *   invalidate_dce
 *
 * It is called from SetWindowPos() - we have to
 * mark as dirty all busy DCEs for windows that have pWnd->parent as
 * an ancestor and whose client rect intersects with specified update
 * rectangle. In addition, pWnd->parent DCEs may need to be updated if
 * DCX_CLIPCHILDREN flag is set.
 */
void invalidate_dce( WND *win, const RECT *extra_rect )
{
    /* FIXME: move callers to win32u */
    NtUserCallTwoParam( (UINT_PTR)win, (UINT_PTR)extra_rect, NtUserInvalidateDCE );
}


/***********************************************************************
 *		release_dc
 *
 * Implementation of ReleaseDC.
 */
static INT release_dc( HWND hwnd, HDC hdc, BOOL end_paint )
{
    struct dce *dce;
    BOOL ret = FALSE;

    TRACE("%p %p\n", hwnd, hdc );

    USER_Lock();
    dce = (struct dce *)GetDCHook( hdc, NULL );
    if (dce && dce->count && dce->hwnd)
    {
        if (!(dce->flags & DCX_NORESETATTRS)) SetHookFlags( dce->hdc, DCHF_RESETDC );
        if (end_paint || (dce->flags & DCX_CACHE)) delete_clip_rgn( dce );
        if (dce->flags & DCX_CACHE)
        {
            dce->count = 0;
            SetHookFlags( dce->hdc, DCHF_DISABLEDC );
        }
        ret = TRUE;
    }
    USER_Unlock();
    return ret;
}


/***********************************************************************
 *           get_update_region
 *
 * Return update region (in screen coordinates) for a window.
 */
static HRGN get_update_region( HWND hwnd, UINT *flags, HWND *child )
{
    HRGN hrgn = 0;
    NTSTATUS status;
    RGNDATA *data;
    size_t size = 256;

    do
    {
        if (!(data = HeapAlloc( GetProcessHeap(), 0, sizeof(*data) + size - 1 )))
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }

        SERVER_START_REQ( get_update_region )
        {
            req->window     = wine_server_user_handle( hwnd );
            req->from_child = wine_server_user_handle( child ? *child : 0 );
            req->flags      = *flags;
            wine_server_set_reply( req, data->Buffer, size );
            if (!(status = wine_server_call( req )))
            {
                size_t reply_size = wine_server_reply_size( reply );
                data->rdh.dwSize   = sizeof(data->rdh);
                data->rdh.iType    = RDH_RECTANGLES;
                data->rdh.nCount   = reply_size / sizeof(RECT);
                data->rdh.nRgnSize = reply_size;
                hrgn = ExtCreateRegion( NULL, data->rdh.dwSize + data->rdh.nRgnSize, data );
                if (child) *child = wine_server_ptr_handle( reply->child );
                *flags = reply->flags;
            }
            else size = reply->total_size;
        }
        SERVER_END_REQ;
        HeapFree( GetProcessHeap(), 0, data );
    } while (status == STATUS_BUFFER_OVERFLOW);

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return hrgn;
}


/***********************************************************************
 *           get_update_flags
 *
 * Get only the update flags, not the update region.
 */
static BOOL get_update_flags( HWND hwnd, HWND *child, UINT *flags )
{
    BOOL ret;

    SERVER_START_REQ( get_update_region )
    {
        req->window     = wine_server_user_handle( hwnd );
        req->from_child = wine_server_user_handle( child ? *child : 0 );
        req->flags      = *flags | UPDATE_NOREGION;
        if ((ret = !wine_server_call_err( req )))
        {
            if (child) *child = wine_server_ptr_handle( reply->child );
            *flags = reply->flags;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           send_ncpaint
 *
 * Send a WM_NCPAINT message if needed, and return the resulting update region (in screen coords).
 * Helper for erase_now and BeginPaint.
 */
static HRGN send_ncpaint( HWND hwnd, HWND *child, UINT *flags )
{
    HRGN whole_rgn = get_update_region( hwnd, flags, child );
    HRGN client_rgn = 0;
    DWORD style;

    if (child) hwnd = *child;

    if (hwnd == GetDesktopWindow()) return whole_rgn;

    if (whole_rgn)
    {
        DPI_AWARENESS_CONTEXT context;
        RECT client, window, update;
        INT type;

        context = SetThreadDpiAwarenessContext( GetWindowDpiAwarenessContext( hwnd ));

        /* check if update rgn overlaps with nonclient area */
        type = GetRgnBox( whole_rgn, &update );
        WIN_GetRectangles( hwnd, COORDS_SCREEN, &window, &client );

        if ((*flags & UPDATE_NONCLIENT) ||
            update.left < client.left || update.top < client.top ||
            update.right > client.right || update.bottom > client.bottom)
        {
            client_rgn = CreateRectRgnIndirect( &client );
            CombineRgn( client_rgn, client_rgn, whole_rgn, RGN_AND );

            /* check if update rgn contains complete nonclient area */
            if (type == SIMPLEREGION && EqualRect( &window, &update ))
            {
                DeleteObject( whole_rgn );
                whole_rgn = (HRGN)1;
            }
        }
        else
        {
            client_rgn = whole_rgn;
            whole_rgn = 0;
        }

        if (whole_rgn) /* NOTE: WM_NCPAINT allows wParam to be 1 */
        {
            if (*flags & UPDATE_NONCLIENT)
            {
                /* Mark standard scroll bars as not painted before sending WM_NCPAINT */
                style = GetWindowLongW( hwnd, GWL_STYLE );
                if (style & WS_HSCROLL)
                    SCROLL_SetStandardScrollPainted( hwnd, SB_HORZ, FALSE );
                if (style & WS_VSCROLL)
                    SCROLL_SetStandardScrollPainted( hwnd, SB_VERT, FALSE );

                SendMessageW( hwnd, WM_NCPAINT, (WPARAM)whole_rgn, 0 );
            }
            if (whole_rgn > (HRGN)1) DeleteObject( whole_rgn );
        }
        SetThreadDpiAwarenessContext( context );
    }
    return client_rgn;
}


/***********************************************************************
 *           send_erase
 *
 * Send a WM_ERASEBKGND message if needed, and optionally return the DC for painting.
 * If a DC is requested, the region is selected into it. In all cases the region is deleted.
 * Helper for erase_now and BeginPaint.
 */
static BOOL send_erase( HWND hwnd, UINT flags, HRGN client_rgn,
                        RECT *clip_rect, HDC *hdc_ret )
{
    BOOL need_erase = (flags & UPDATE_DELAYED_ERASE) != 0;
    HDC hdc = 0;
    RECT dummy;

    if (!clip_rect) clip_rect = &dummy;
    if (hdc_ret || (flags & UPDATE_ERASE))
    {
        UINT dcx_flags = DCX_INTERSECTRGN | DCX_USESTYLE;
        if (IsIconic(hwnd)) dcx_flags |= DCX_WINDOW;

        if ((hdc = NtUserGetDCEx( hwnd, client_rgn, dcx_flags )))
        {
            INT type = GetClipBox( hdc, clip_rect );

            if (flags & UPDATE_ERASE)
            {
                /* don't erase if the clip box is empty */
                if (type != NULLREGION)
                    need_erase = !SendMessageW( hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0 );
            }
            if (!hdc_ret) release_dc( hwnd, hdc, TRUE );
        }

        if (hdc_ret) *hdc_ret = hdc;
    }
    if (!hdc) DeleteObject( client_rgn );
    return need_erase;
}


/***********************************************************************
 *           erase_now
 *
 * Implementation of RDW_ERASENOW behavior.
 */
void erase_now( HWND hwnd, UINT rdw_flags )
{
    HWND child = 0;
    HRGN hrgn;
    BOOL need_erase = FALSE;

    /* loop while we find a child to repaint */
    for (;;)
    {
        UINT flags = UPDATE_NONCLIENT | UPDATE_ERASE;

        if (rdw_flags & RDW_NOCHILDREN) flags |= UPDATE_NOCHILDREN;
        else if (rdw_flags & RDW_ALLCHILDREN) flags |= UPDATE_ALLCHILDREN;
        if (need_erase) flags |= UPDATE_DELAYED_ERASE;

        if (!(hrgn = send_ncpaint( hwnd, &child, &flags ))) break;
        need_erase = send_erase( child, flags, hrgn, NULL, NULL );

        if (!flags) break;  /* nothing more to do */
        if ((rdw_flags & RDW_NOCHILDREN) && !need_erase) break;
    }
}


/***********************************************************************
 *           copy_bits_from_surface
 *
 * Copy bits from a window surface; helper for move_window_bits and move_window_bits_parent.
 */
static void copy_bits_from_surface( HWND hwnd, struct window_surface *surface,
                                    const RECT *dst, const RECT *src )
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    void *bits;
    UINT flags = UPDATE_NOCHILDREN | UPDATE_CLIPCHILDREN;
    HRGN rgn = get_update_region( hwnd, &flags, NULL );
    HDC hdc = NtUserGetDCEx( hwnd, rgn, DCX_CACHE | DCX_WINDOW | DCX_EXCLUDERGN );

    bits = surface->funcs->get_info( surface, info );
    surface->funcs->lock( surface );
    SetDIBitsToDevice( hdc, dst->left, dst->top, dst->right - dst->left, dst->bottom - dst->top,
                       src->left - surface->rect.left, surface->rect.bottom - src->bottom,
                       0, surface->rect.bottom - surface->rect.top,
                       bits, info, DIB_RGB_COLORS );
    surface->funcs->unlock( surface );
    NtUserReleaseDC( hwnd, hdc );
}


/***********************************************************************
 *           move_window_bits
 *
 * Move the window bits when a window is resized or its surface recreated.
 */
void move_window_bits( HWND hwnd, struct window_surface *old_surface,
                       struct window_surface *new_surface,
                       const RECT *visible_rect, const RECT *old_visible_rect,
                       const RECT *window_rect, const RECT *valid_rects )
{
    RECT dst = valid_rects[0];
    RECT src = valid_rects[1];

    if (new_surface != old_surface ||
        src.left - old_visible_rect->left != dst.left - visible_rect->left ||
        src.top - old_visible_rect->top != dst.top - visible_rect->top)
    {
        TRACE( "copying %s -> %s\n", wine_dbgstr_rect( &src ), wine_dbgstr_rect( &dst ));
        OffsetRect( &src, -old_visible_rect->left, -old_visible_rect->top );
        OffsetRect( &dst, -window_rect->left, -window_rect->top );
        copy_bits_from_surface( hwnd, old_surface, &dst, &src );
    }
}


/***********************************************************************
 *		move_window_bits_parent
 *
 * Move the window bits in the parent surface when a child is moved.
 */
void move_window_bits_parent( HWND hwnd, HWND parent, const RECT *window_rect, const RECT *valid_rects )
{
    struct window_surface *surface;
    RECT dst = valid_rects[0];
    RECT src = valid_rects[1];
    WND *win;

    if (src.left == dst.left && src.top == dst.top) return;

    if (!(win = WIN_GetPtr( parent ))) return;
    if (win == WND_DESKTOP || win == WND_OTHER_PROCESS) return;
    if (!(surface = win->surface))
    {
        WIN_ReleasePtr( win );
        return;
    }

    TRACE( "copying %s -> %s\n", wine_dbgstr_rect( &src ), wine_dbgstr_rect( &dst ));
    MapWindowPoints( NtUserGetAncestor( hwnd, GA_PARENT ), parent, (POINT *)&src, 2 );
    OffsetRect( &src, win->client_rect.left - win->visible_rect.left,
                win->client_rect.top - win->visible_rect.top );
    OffsetRect( &dst, -window_rect->left, -window_rect->top );
    window_surface_add_ref( surface );
    WIN_ReleasePtr( win );

    copy_bits_from_surface( hwnd, surface, &dst, &src );
    window_surface_release( surface );
}


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
 *		GetUpdateRect (USER32.@)
 */
BOOL WINAPI GetUpdateRect( HWND hwnd, LPRECT rect, BOOL erase )
{
    DPI_AWARENESS_CONTEXT context;
    UINT flags = UPDATE_NOCHILDREN;
    HRGN update_rgn;
    BOOL need_erase;

    if (erase) flags |= UPDATE_NONCLIENT | UPDATE_ERASE;

    if (!(update_rgn = send_ncpaint( hwnd, NULL, &flags ))) return FALSE;

    if (rect)
    {
        if (GetRgnBox( update_rgn, rect ) != NULLREGION)
        {
            HDC hdc = NtUserGetDCEx( hwnd, 0, DCX_USESTYLE );
            DWORD layout = SetLayout( hdc, 0 );  /* MapWindowPoints mirrors already */
            context = SetThreadDpiAwarenessContext( GetWindowDpiAwarenessContext( hwnd ));
            MapWindowPoints( 0, hwnd, (LPPOINT)rect, 2 );
            SetThreadDpiAwarenessContext( context );
            *rect = rect_win_to_thread_dpi( hwnd, *rect );
            DPtoLP( hdc, (LPPOINT)rect, 2 );
            SetLayout( hdc, layout );
            NtUserReleaseDC( hwnd, hdc );
        }
    }
    need_erase = send_erase( hwnd, flags, update_rgn, NULL, NULL );

    /* check if we still have an update region */
    flags = UPDATE_PAINT | UPDATE_NOCHILDREN;
    if (need_erase) flags |= UPDATE_DELAYED_ERASE;
    return (get_update_flags( hwnd, NULL, &flags ) && (flags & UPDATE_PAINT));
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
