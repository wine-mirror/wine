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
#include "user.h"
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
 *		ScrollWindow (USER32.@)
 *
 */
BOOL WINAPI ScrollWindow( HWND hwnd, INT dx, INT dy,
                              const RECT *rect, const RECT *clipRect )
{
    return
        (ERROR != ScrollWindowEx( hwnd, dx, dy, rect, clipRect, 0, NULL,
                                    (rect ? 0 : SW_SCROLLCHILDREN) |
                                    SW_INVALIDATE ));
}

/*************************************************************************
 *		ScrollDC (USER32.@)
 *
 * dx, dy, lprcScroll and lprcClip are all in logical coordinates (msdn is wrong)
 * hrgnUpdate is returned in device coordinates with rcUpdate in logical coordinates.
 */
BOOL WINAPI ScrollDC( HDC hdc, INT dx, INT dy, const RECT *lprcScroll,
                      const RECT *lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate )

{
    RECT rSrc, rClipped_src, rClip, rDst, offset;

    TRACE( "%p %d,%d hrgnUpdate=%p lprcUpdate = %p\n", hdc, dx, dy, hrgnUpdate, lprcUpdate );
    if (lprcClip) TRACE( "lprcClip = %s\n", wine_dbgstr_rect(lprcClip));
    if (lprcScroll) TRACE( "lprcScroll = %s\n", wine_dbgstr_rect(lprcScroll));

    /* compute device clipping region (in device coordinates) */

    if (lprcScroll) rSrc = *lprcScroll;
    else GetClipBox( hdc, &rSrc );
    LPtoDP(hdc, (LPPOINT)&rSrc, 2);

    if (lprcClip) rClip = *lprcClip;
    else GetClipBox( hdc, &rClip );
    LPtoDP(hdc, (LPPOINT)&rClip, 2);

    IntersectRect( &rClipped_src, &rSrc, &rClip );
    TRACE("rSrc %s rClip %s clipped rSrc %s\n", wine_dbgstr_rect(&rSrc),
          wine_dbgstr_rect(&rClip), wine_dbgstr_rect(&rClipped_src));

    rDst = rClipped_src;
    SetRect(&offset, 0, 0, dx, dy);
    LPtoDP(hdc, (LPPOINT)&offset, 2);
    OffsetRect( &rDst, offset.right - offset.left,  offset.bottom - offset.top );
    TRACE("rDst before clipping %s\n", wine_dbgstr_rect(&rDst));
    IntersectRect( &rDst, &rDst, &rClip );
    TRACE("rDst after clipping %s\n", wine_dbgstr_rect(&rDst));

    if (!IsRectEmpty(&rDst))
    {
        /* copy bits */
        RECT rDst_lp = rDst, rSrc_lp = rDst;

        OffsetRect( &rSrc_lp, offset.left - offset.right, offset.top - offset.bottom );
        DPtoLP(hdc, (LPPOINT)&rDst_lp, 2);
        DPtoLP(hdc, (LPPOINT)&rSrc_lp, 2);

        if (!BitBlt( hdc, rDst_lp.left, rDst_lp.top,
                     rDst_lp.right - rDst_lp.left, rDst_lp.bottom - rDst_lp.top,
                     hdc, rSrc_lp.left, rSrc_lp.top, SRCCOPY))
            return FALSE;
    }

    /* compute update areas.  This is the clipped source or'ed with the unclipped source translated minus the
     clipped src translated (rDst) all clipped to rClip */

    if (hrgnUpdate || lprcUpdate)
    {
        HRGN hrgn = hrgnUpdate, hrgn2;

        if (hrgn) SetRectRgn( hrgn, rClipped_src.left, rClipped_src.top, rClipped_src.right, rClipped_src.bottom );
        else hrgn = CreateRectRgn( rClipped_src.left, rClipped_src.top, rClipped_src.right, rClipped_src.bottom );

        hrgn2 = CreateRectRgnIndirect( &rSrc );
        OffsetRgn(hrgn2, offset.right - offset.left,  offset.bottom - offset.top );
        CombineRgn(hrgn, hrgn, hrgn2, RGN_OR);

        SetRectRgn( hrgn2, rDst.left, rDst.top, rDst.right, rDst.bottom );
        CombineRgn( hrgn, hrgn, hrgn2, RGN_DIFF );

        SetRectRgn( hrgn2, rClip.left, rClip.top, rClip.right, rClip.bottom );
        CombineRgn( hrgn, hrgn, hrgn2, RGN_AND );

        if( lprcUpdate )
        {
            GetRgnBox( hrgn, lprcUpdate );

            /* Put the lprcUpdate in logical coordinate */
            DPtoLP( hdc, (LPPOINT)lprcUpdate, 2 );
            TRACE("returning lprcUpdate %s\n", wine_dbgstr_rect(lprcUpdate));
        }
        if (!hrgnUpdate) DeleteObject( hrgn );
        DeleteObject( hrgn2 );
    }
    return TRUE;
}


/*************************************************************************
 *		ScrollWindowEx (USER32.@)
 *
 * NOTE: Use this function instead of ScrollWindow32
 */
INT WINAPI ScrollWindowEx( HWND hwnd, INT dx, INT dy,
                               const RECT *rect, const RECT *clipRect,
                               HRGN hrgnUpdate, LPRECT rcUpdate,
                               UINT flags )
{
    RECT rc, cliprc;
    INT result;
    
    if (!WIN_IsWindowDrawable( hwnd, TRUE )) return ERROR;
    hwnd = WIN_GetFullHandle( hwnd );

    GetClientRect(hwnd, &rc);
    if (rect) IntersectRect(&rc, &rc, rect);

    if (clipRect) IntersectRect(&cliprc,&rc,clipRect);
    else cliprc = rc;

    if (!IsRectEmpty(&cliprc) && (dx || dy))
    {
        RECT caretrc = rc;
        HWND hwndCaret = fix_caret(hwnd, &caretrc, flags);

	if (USER_Driver.pScrollWindowEx)
            result = USER_Driver.pScrollWindowEx( hwnd, dx, dy, rect, clipRect,
                                                  hrgnUpdate, rcUpdate, flags );
	else
	    result = ERROR; /* FIXME: we should have a fallback implementation */
	
        if( hwndCaret )
        {
            SetCaretPos( caretrc.left + dx, caretrc.top + dy );
            ShowCaret(hwndCaret);
        }
    }
    else 
	result = NULLREGION;
    
    return result;
}
