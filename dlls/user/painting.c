/*
 * Window painting functions
 *
 * Copyright 1993, 1994, 1995, 2001 Alexandre Julliard
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
#include <string.h>

#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "wine/server.h"
#include "win.h"
#include "dce.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);

/***********************************************************************
 *           add_paint_count
 *
 * Add an increment (normally 1 or -1) to the current paint count of a window.
 */
static void add_paint_count( HWND hwnd, int incr )
{
    SERVER_START_REQ( inc_window_paint_count )
    {
        req->handle = hwnd;
        req->incr   = incr;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *           copy_rgn
 *
 * copy a region, doing the right thing if the source region is 0 or 1
 */
static HRGN copy_rgn( HRGN hSrc )
{
    if (hSrc > (HRGN)1)
    {
        HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( hrgn, hSrc, 0, RGN_COPY );
        return hrgn;
    }
    return hSrc;
}


/***********************************************************************
 *           get_update_regions
 *
 * Return update regions for the whole window and for the client area.
 */
static void get_update_regions( WND *win, HRGN *whole_rgn, HRGN *client_rgn )
{
    if (win->hrgnUpdate > (HRGN)1)
    {
        RECT client, update;

        /* check if update rgn overlaps with nonclient area */
        GetRgnBox( win->hrgnUpdate, &update );
        client = win->rectClient;
        OffsetRect( &client, -win->rectWindow.left, -win->rectWindow.top );

        if (update.left < client.left || update.top < client.top ||
            update.right > client.right || update.bottom > client.bottom)
        {
            *whole_rgn = copy_rgn( win->hrgnUpdate );
            *client_rgn = CreateRectRgnIndirect( &client );
            if (CombineRgn( *client_rgn, *client_rgn, win->hrgnUpdate, RGN_AND ) == NULLREGION)
            {
                DeleteObject( *client_rgn );
                *client_rgn = 0;
            }
        }
        else
        {
            *whole_rgn = 0;
            *client_rgn = copy_rgn( win->hrgnUpdate );
        }
    }
    else
    {
        *client_rgn = *whole_rgn = win->hrgnUpdate;  /* 0 or 1 */
    }
}


/***********************************************************************
 *           begin_ncpaint
 *
 * Send a WM_NCPAINT message from inside BeginPaint().
 * Returns update region cropped to client rectangle (and in client coords),
 * and clears window update region and internal paint flag.
 */
static HRGN begin_ncpaint( HWND hwnd )
{
    HRGN whole_rgn, client_rgn;
    WND *wnd = WIN_GetPtr( hwnd );

    if (!wnd || wnd == WND_OTHER_PROCESS) return 0;

    TRACE("hwnd %p [%p] ncf %i\n",
          hwnd, wnd->hrgnUpdate, wnd->flags & WIN_NEEDS_NCPAINT);

    get_update_regions( wnd, &whole_rgn, &client_rgn );

    if (whole_rgn) /* NOTE: WM_NCPAINT allows wParam to be 1 */
    {
        WIN_ReleasePtr( wnd );
        SendMessageA( hwnd, WM_NCPAINT, (WPARAM)whole_rgn, 0 );
        if (whole_rgn > (HRGN)1) DeleteObject( whole_rgn );
        /* make sure the window still exists before continuing */
        if (!(wnd = WIN_GetPtr( hwnd )) || wnd == WND_OTHER_PROCESS)
        {
            if (client_rgn > (HRGN)1) DeleteObject( client_rgn );
            return 0;
        }
    }

    if (wnd->hrgnUpdate || (wnd->flags & WIN_INTERNAL_PAINT)) add_paint_count( hwnd, -1 );
    if (wnd->hrgnUpdate > (HRGN)1) DeleteObject( wnd->hrgnUpdate );
    wnd->hrgnUpdate = 0;
    wnd->flags &= ~(WIN_INTERNAL_PAINT | WIN_NEEDS_NCPAINT | WIN_NEEDS_BEGINPAINT);
    if (client_rgn > (HRGN)1) OffsetRgn( client_rgn, wnd->rectWindow.left - wnd->rectClient.left,
                                         wnd->rectWindow.top - wnd->rectClient.top );
    WIN_ReleasePtr( wnd );
    return client_rgn;
}


/***********************************************************************
 *		BeginPaint (USER32.@)
 */
HDC WINAPI BeginPaint( HWND hwnd, PAINTSTRUCT *lps )
{
    INT dcx_flags;
    BOOL bIcon;
    HRGN hrgnUpdate;
    RECT clipRect, clientRect;
    HWND full_handle;
    WND *wndPtr;

    if (!lps) return 0;

    if (!(full_handle = WIN_IsCurrentThread( hwnd )))
    {
        if (IsWindow(hwnd))
            FIXME( "window %p belongs to other thread\n", hwnd );
        return 0;
    }
    hwnd = full_handle;

    /* send WM_NCPAINT and retrieve update region */
    hrgnUpdate = begin_ncpaint( hwnd );
    if (!hrgnUpdate && !IsWindow( hwnd )) return 0;

    HideCaret( hwnd );

    bIcon = (IsIconic(hwnd) && GetClassLongA(hwnd, GCL_HICON));

    dcx_flags = DCX_INTERSECTRGN | DCX_WINDOWPAINT | DCX_USESTYLE;
    if (bIcon) dcx_flags |= DCX_WINDOW;

    if (GetClassLongA( hwnd, GCL_STYLE ) & CS_PARENTDC)
    {
        /* Don't clip the output to the update region for CS_PARENTDC window */
        if (hrgnUpdate > (HRGN)1) DeleteObject( hrgnUpdate );
        hrgnUpdate = 0;
        dcx_flags &= ~DCX_INTERSECTRGN;
    }
    else
    {
        if (!hrgnUpdate)  /* empty region, clip everything */
        {
            hrgnUpdate = CreateRectRgn( 0, 0, 0, 0 );
        }
        else if (hrgnUpdate == (HRGN)1)  /* whole client area, don't clip */
        {
            hrgnUpdate = 0;
            dcx_flags &= ~DCX_INTERSECTRGN;
        }
    }
    lps->hdc = GetDCEx( hwnd, hrgnUpdate, dcx_flags );
    /* ReleaseDC() in EndPaint() will delete the region */

    if (!lps->hdc)
    {
        WARN("GetDCEx() failed in BeginPaint(), hwnd=%p\n", hwnd);
        DeleteObject( hrgnUpdate );
        return 0;
    }

    /* It is possible that the clip box is bigger than the window itself,
       if CS_PARENTDC flag is set. Windows never return a paint rect bigger
       than the window itself, so we need to intersect the cliprect with
       the window  */
    GetClientRect( hwnd, &clientRect );

    GetClipBox( lps->hdc, &clipRect );
    LPtoDP(lps->hdc, (LPPOINT)&clipRect, 2);      /* GetClipBox returns LP */

    IntersectRect(&lps->rcPaint, &clientRect, &clipRect);
    DPtoLP(lps->hdc, (LPPOINT)&lps->rcPaint, 2);  /* we must return LP */

    TRACE("hdc = %p box = (%ld,%ld - %ld,%ld)\n",
          lps->hdc, lps->rcPaint.left, lps->rcPaint.top, lps->rcPaint.right, lps->rcPaint.bottom );

    if (!(wndPtr = WIN_GetPtr( hwnd )) || wndPtr == WND_OTHER_PROCESS) return 0;
    lps->fErase = !(wndPtr->flags & WIN_NEEDS_ERASEBKGND);
    wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
    WIN_ReleasePtr( wndPtr );

    if (!lps->fErase)
        lps->fErase = !SendMessageA( hwnd, bIcon ? WM_ICONERASEBKGND : WM_ERASEBKGND,
                                     (WPARAM)lps->hdc, 0 );
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
