/*
 * Window painting functions
 *
 * Copyright 1993, 1994, 1995, 2001, 2004 Alexandre Julliard
 * Copyright 1999 Alex Korobka
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
#include "ntstatus.h"
#include "winuser.h"
#include "wine/server.h"
#include "win.h"
#include "dce.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);


/***********************************************************************
 *           dump_rdw_flags
 */
static void dump_rdw_flags(UINT flags)
{
    TRACE("flags:");
    if (flags & RDW_INVALIDATE) TRACE(" RDW_INVALIDATE");
    if (flags & RDW_INTERNALPAINT) TRACE(" RDW_INTERNALPAINT");
    if (flags & RDW_ERASE) TRACE(" RDW_ERASE");
    if (flags & RDW_VALIDATE) TRACE(" RDW_VALIDATE");
    if (flags & RDW_NOINTERNALPAINT) TRACE(" RDW_NOINTERNALPAINT");
    if (flags & RDW_NOERASE) TRACE(" RDW_NOERASE");
    if (flags & RDW_NOCHILDREN) TRACE(" RDW_NOCHILDREN");
    if (flags & RDW_ALLCHILDREN) TRACE(" RDW_ALLCHILDREN");
    if (flags & RDW_UPDATENOW) TRACE(" RDW_UPDATENOW");
    if (flags & RDW_ERASENOW) TRACE(" RDW_ERASENOW");
    if (flags & RDW_FRAME) TRACE(" RDW_FRAME");
    if (flags & RDW_NOFRAME) TRACE(" RDW_NOFRAME");

#define RDW_FLAGS \
    (RDW_INVALIDATE | \
    RDW_INTERNALPAINT | \
    RDW_ERASE | \
    RDW_VALIDATE | \
    RDW_NOINTERNALPAINT | \
    RDW_NOERASE | \
    RDW_NOCHILDREN | \
    RDW_ALLCHILDREN | \
    RDW_UPDATENOW | \
    RDW_ERASENOW | \
    RDW_FRAME | \
    RDW_NOFRAME)

    if (flags & ~RDW_FLAGS) TRACE(" %04x", flags & ~RDW_FLAGS);
    TRACE("\n");
#undef RDW_FLAGS
}


/***********************************************************************
 *           get_update_region
 *
 * Return update region for a window.
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
            req->window = hwnd;
            req->flags  = *flags;
            wine_server_set_reply( req, data->Buffer, size );
            if (!(status = wine_server_call( req )))
            {
                size_t reply_size = wine_server_reply_size( reply );
                data->rdh.dwSize   = sizeof(data->rdh);
                data->rdh.iType    = RDH_RECTANGLES;
                data->rdh.nCount   = reply_size / sizeof(RECT);
                data->rdh.nRgnSize = reply_size;
                hrgn = ExtCreateRegion( NULL, size, data );
                if (child) *child = reply->child;
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
        req->window = hwnd;
        req->flags  = *flags | UPDATE_NOREGION;
        if ((ret = !wine_server_call_err( req )))
        {
            if (child) *child = reply->child;
            *flags = reply->flags;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           redraw_window_rects
 *
 * Redraw part of a window.
 */
static BOOL redraw_window_rects( HWND hwnd, UINT flags, const RECT *rects, UINT count )
{
    BOOL ret;

    SERVER_START_REQ( redraw_window )
    {
        req->window = hwnd;
        req->flags  = flags;
        wine_server_add_data( req, rects, count * sizeof(RECT) );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           send_ncpaint
 *
 * Send a WM_NCPAINT message if needed, and return the resulting update region.
 * Helper for erase_now and BeginPaint.
 */
static HRGN send_ncpaint( HWND hwnd, HWND *child, UINT *flags )
{
    HRGN whole_rgn = get_update_region( hwnd, flags, child );
    HRGN client_rgn = 0;

    if (child) hwnd = *child;

    if (whole_rgn)
    {
        RECT client, update;
        INT type;
        WND *win = WIN_GetPtr( hwnd );

        if (!win || win == WND_OTHER_PROCESS)
        {
            DeleteObject( whole_rgn );
            return 0;
        }

        /* check if update rgn overlaps with nonclient area */
        type = GetRgnBox( whole_rgn, &update );
        client = win->rectClient;
        OffsetRect( &client, -win->rectWindow.left, -win->rectWindow.top );

        if ((*flags & UPDATE_NONCLIENT) ||
            update.left < client.left || update.top < client.top ||
            update.right > client.right || update.bottom > client.bottom)
        {
            client_rgn = CreateRectRgnIndirect( &client );
            CombineRgn( client_rgn, client_rgn, whole_rgn, RGN_AND );

            /* check if update rgn contains complete nonclient area */
            if (type == SIMPLEREGION && update.left == 0 && update.top == 0 &&
                update.right == win->rectWindow.right - win->rectWindow.left &&
                update.bottom == win->rectWindow.bottom - win->rectWindow.top)
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
        /* map client region to client coordinates */
        OffsetRgn( client_rgn, win->rectWindow.left - win->rectClient.left,
                   win->rectWindow.top - win->rectClient.top );
        WIN_ReleasePtr( win );

        if (whole_rgn) /* NOTE: WM_NCPAINT allows wParam to be 1 */
        {
            if (*flags & UPDATE_NONCLIENT) SendMessageW( hwnd, WM_NCPAINT, (WPARAM)whole_rgn, 0 );
            if (whole_rgn > (HRGN)1) DeleteObject( whole_rgn );
        }
    }
    return client_rgn;
}


/***********************************************************************
 *           send_erase
 *
 * Send a WM_ERASEBKGND message if needed, and optionally return the DC for painting.
 * If a DC is requested, the region is selected into it.
 * Helper for erase_now and BeginPaint.
 */
static BOOL send_erase( HWND hwnd, UINT flags, HRGN client_rgn,
                        RECT *clip_rect, HDC *hdc_ret )
{
    BOOL need_erase = FALSE;
    HDC hdc;

    if (hdc_ret || (flags & UPDATE_ERASE))
    {
        UINT dcx_flags = DCX_INTERSECTRGN | DCX_WINDOWPAINT | DCX_USESTYLE;
        if (IsIconic(hwnd)) dcx_flags |= DCX_WINDOW;

        if ((hdc = GetDCEx( hwnd, client_rgn, dcx_flags )))
        {
            INT type = GetClipBox( hdc, clip_rect );

            if (flags & UPDATE_ERASE)
            {
                /* don't erase if the clip box is empty */
                if (type != NULLREGION)
                    need_erase = !SendMessageW( hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0 );
            }
            if (!hdc_ret)
            {
                if (need_erase && hwnd != GetDesktopWindow())  /* FIXME: mark it as needing erase again */
                    RedrawWindow( hwnd, NULL, client_rgn, RDW_INVALIDATE | RDW_ERASE | RDW_NOCHILDREN );
                ReleaseDC( hwnd, hdc );
            }
        }

        if (hdc_ret) *hdc_ret = hdc;
    }
    return need_erase;
}


/***********************************************************************
 *           erase_now
 *
 * Implementation of RDW_ERASENOW behavior.
 */
static void erase_now( HWND hwnd, UINT rdw_flags )
{
    HWND child;
    HRGN hrgn;

    /* loop while we find a child to repaint */
    for (;;)
    {
        RECT rect;
        UINT flags = UPDATE_NONCLIENT | UPDATE_ERASE;

        if (rdw_flags & RDW_NOCHILDREN) flags |= UPDATE_NOCHILDREN;
        else if (rdw_flags & RDW_ALLCHILDREN) flags |= UPDATE_ALLCHILDREN;

        if (!(hrgn = send_ncpaint( hwnd, &child, &flags ))) break;
        send_erase( child, flags, hrgn, &rect, NULL );
        DeleteObject( hrgn );

        if (!flags) break;  /* nothing more to do */
        if (rdw_flags & RDW_NOCHILDREN) break;
    }
}


/***********************************************************************
 *           update_now
 *
 * Implementation of RDW_UPDATENOW behavior.
 *
 * FIXME: Windows uses WM_SYNCPAINT to cut down the number of intertask
 * SendMessage() calls. This is a comment inside DefWindowProc() source
 * from 16-bit SDK:
 *
 *   This message avoids lots of inter-app message traffic
 *   by switching to the other task and continuing the
 *   recursion there.
 *
 * wParam         = flags
 * LOWORD(lParam) = hrgnClip
 * HIWORD(lParam) = hwndSkip  (not used; always NULL)
 *
 */
static void update_now( HWND hwnd, UINT rdw_flags )
{
    HWND child;

    /* desktop window never gets WM_PAINT, only WM_ERASEBKGND */
    if (hwnd == GetDesktopWindow()) erase_now( hwnd, rdw_flags | RDW_NOCHILDREN );

    /* loop while we find a child to repaint */
    for (;;)
    {
        UINT flags = UPDATE_PAINT | UPDATE_INTERNALPAINT;

        if (rdw_flags & RDW_NOCHILDREN) flags |= UPDATE_NOCHILDREN;
        else if (rdw_flags & RDW_ALLCHILDREN) flags |= UPDATE_ALLCHILDREN;

        if (!get_update_flags( hwnd, &child, &flags )) break;
        if (!flags) break;  /* nothing more to do */

        SendMessageW( child, WM_PAINT, 0, 0 );

        if (rdw_flags & RDW_NOCHILDREN) break;
    }
}


/***********************************************************************
 *		BeginPaint (USER32.@)
 */
HDC WINAPI BeginPaint( HWND hwnd, PAINTSTRUCT *lps )
{
    HWND full_handle;
    HRGN hrgn;
    UINT flags = UPDATE_NONCLIENT | UPDATE_ERASE | UPDATE_PAINT | UPDATE_INTERNALPAINT | UPDATE_NOCHILDREN;

    if (!lps) return 0;

    if (!(full_handle = WIN_IsCurrentThread( hwnd )))
    {
        if (IsWindow(hwnd))
            FIXME( "window %p belongs to other thread\n", hwnd );
        return 0;
    }
    hwnd = full_handle;

    HideCaret( hwnd );

    if (!(hrgn = send_ncpaint( hwnd, NULL, &flags ))) return 0;

    lps->fErase = send_erase( hwnd, flags, hrgn, &lps->rcPaint, &lps->hdc );
    if (!lps->hdc) DeleteObject( hrgn );

    TRACE("hdc = %p box = (%ld,%ld - %ld,%ld), fErase = %d\n",
          lps->hdc, lps->rcPaint.left, lps->rcPaint.top, lps->rcPaint.right, lps->rcPaint.bottom,
          lps->fErase);

    return lps->hdc;
}


/***********************************************************************
 *		EndPaint (USER32.@)
 */
BOOL WINAPI EndPaint( HWND hwnd, const PAINTSTRUCT *lps )
{
    if (!lps) return FALSE;

    ReleaseDC( hwnd, lps->hdc );
    ShowCaret( hwnd );
    return TRUE;
}


/***********************************************************************
 *		RedrawWindow (USER32.@)
 */
BOOL WINAPI RedrawWindow( HWND hwnd, const RECT *rect, HRGN hrgn, UINT flags )
{
    BOOL ret;

    if (!hwnd) hwnd = GetDesktopWindow();

    /* check if the window or its parents are visible/not minimized */

    if (!WIN_IsWindowDrawable( hwnd, !(flags & RDW_FRAME) )) return TRUE;

    /* process pending events and messages before painting */
    if (flags & RDW_UPDATENOW)
        MsgWaitForMultipleObjects( 0, NULL, FALSE, 0, QS_ALLINPUT );

    if (TRACE_ON(win))
    {
        if (hrgn)
        {
            RECT r;
            GetRgnBox( hrgn, &r );
            TRACE( "%p region %p box %s ", hwnd, hrgn, wine_dbgstr_rect(&r) );
        }
        else if (rect)
            TRACE( "%p rect %s ", hwnd, wine_dbgstr_rect(rect) );
        else
            TRACE( "%p whole window ", hwnd );

        dump_rdw_flags(flags);
    }

    if (rect && !hrgn)
    {
        ret = redraw_window_rects( hwnd, flags, rect, 1 );
    }
    else if (!hrgn)
    {
        ret = redraw_window_rects( hwnd, flags, NULL, 0 );
    }
    else  /* need to build a list of the region rectangles */
    {
        DWORD size;
        RGNDATA *data = NULL;
        static const RECT empty;

        if (!(size = GetRegionData( hrgn, 0, NULL ))) return FALSE;
        if (!(data = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
        GetRegionData( hrgn, size, data );
        if (!data->rdh.nCount)  /* empty region -> use a single all-zero rectangle */
            ret = redraw_window_rects( hwnd, flags, &empty, 1 );
        else
            ret = redraw_window_rects( hwnd, flags, (const RECT *)data->Buffer, data->rdh.nCount );
        HeapFree( GetProcessHeap(), 0, data );
    }

    if (flags & RDW_UPDATENOW) update_now( hwnd, flags );
    else if (flags & RDW_ERASENOW) erase_now( hwnd, flags );

    return ret;
}


/***********************************************************************
 *		UpdateWindow (USER32.@)
 */
BOOL WINAPI UpdateWindow( HWND hwnd )
{
    return RedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );
}


/***********************************************************************
 *		InvalidateRgn (USER32.@)
 */
BOOL WINAPI InvalidateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    return RedrawWindow(hwnd, NULL, hrgn, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}


/***********************************************************************
 *		InvalidateRect (USER32.@)
 */
BOOL WINAPI InvalidateRect( HWND hwnd, const RECT *rect, BOOL erase )
{
    return RedrawWindow( hwnd, rect, 0, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}


/***********************************************************************
 *		ValidateRgn (USER32.@)
 */
BOOL WINAPI ValidateRgn( HWND hwnd, HRGN hrgn )
{
    return RedrawWindow( hwnd, NULL, hrgn, RDW_VALIDATE | RDW_NOCHILDREN );
}


/***********************************************************************
 *		ValidateRect (USER32.@)
 */
BOOL WINAPI ValidateRect( HWND hwnd, const RECT *rect )
{
    return RedrawWindow( hwnd, rect, 0, RDW_VALIDATE | RDW_NOCHILDREN );
}


/***********************************************************************
 *		GetUpdateRgn (USER32.@)
 */
INT WINAPI GetUpdateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    INT retval = ERROR;
    UINT flags = UPDATE_NOCHILDREN;
    HRGN update_rgn;

    if (erase) flags |= UPDATE_NONCLIENT | UPDATE_ERASE;

    if ((update_rgn = send_ncpaint( hwnd, NULL, &flags )))
    {
        RECT rect;
        send_erase( hwnd, flags, update_rgn, &rect, NULL );
        retval = CombineRgn( hrgn, update_rgn, 0, RGN_COPY );
        DeleteObject( update_rgn );
    }
    return retval;
}


/***********************************************************************
 *		GetUpdateRect (USER32.@)
 */
BOOL WINAPI GetUpdateRect( HWND hwnd, LPRECT rect, BOOL erase )
{
    HDC hdc;
    RECT dummy;
    UINT flags = UPDATE_NOCHILDREN;
    HRGN update_rgn;

    if (erase) flags |= UPDATE_NONCLIENT | UPDATE_ERASE;

    if (!(update_rgn = send_ncpaint( hwnd, NULL, &flags ))) return FALSE;

    if (rect) GetRgnBox( update_rgn, rect );

    send_erase( hwnd, flags, update_rgn, &dummy, &hdc );
    if (hdc)
    {
        if (rect) DPtoLP( hdc, (LPPOINT)rect, 2 );
        ReleaseDC( hwnd, hdc );
    }
    else DeleteObject( update_rgn );

    /* check if we still have an update region */
    flags = UPDATE_PAINT | UPDATE_NOCHILDREN;
    return (get_update_flags( hwnd, NULL, &flags ) && (flags & UPDATE_PAINT));
}


/***********************************************************************
 *		ExcludeUpdateRgn (USER32.@)
 */
INT WINAPI ExcludeUpdateRgn( HDC hdc, HWND hwnd )
{
    HRGN update_rgn = CreateRectRgn( 0, 0, 0, 0 );
    INT ret = GetUpdateRgn( hwnd, update_rgn, FALSE );

    if (ret != ERROR)
    {
        POINT pt;

        GetDCOrgEx( hdc, &pt );
        MapWindowPoints( 0, hwnd, &pt, 1 );
        OffsetRgn( update_rgn, -pt.x, -pt.y );
        ret = ExtSelectClipRgn( hdc, update_rgn, RGN_DIFF );
        DeleteObject( update_rgn );
    }
    return ret;
}
