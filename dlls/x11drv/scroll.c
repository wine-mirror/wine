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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(scroll);


/*************************************************************************
 *		ScrollDC   (X11DRV.@)
 */
BOOL X11DRV_ScrollDC( HDC hdc, INT dx, INT dy, const RECT *lprcScroll,
                      const RECT *lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate )

{
    RECT rSrc, rClipped_src, rClip, rDst, offset;
    INT code = X11DRV_START_EXPOSURES;

    if (hrgnUpdate || lprcUpdate)
        ExtEscape( hdc, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, 0, NULL );

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
        HRGN hrgn = hrgnUpdate, hrgn2, hrgn3 = 0;

        code = X11DRV_END_EXPOSURES;
        ExtEscape( hdc, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, sizeof(hrgn3), (LPSTR)&hrgn3 );

        if (hrgn) SetRectRgn( hrgn, rClipped_src.left, rClipped_src.top, rClipped_src.right, rClipped_src.bottom );
        else hrgn = CreateRectRgn( rClipped_src.left, rClipped_src.top, rClipped_src.right, rClipped_src.bottom );

        hrgn2 = CreateRectRgnIndirect( &rSrc );
        OffsetRgn(hrgn2, offset.right - offset.left,  offset.bottom - offset.top );
        CombineRgn(hrgn, hrgn, hrgn2, RGN_OR);

        SetRectRgn( hrgn2, rDst.left, rDst.top, rDst.right, rDst.bottom );
        CombineRgn( hrgn, hrgn, hrgn2, RGN_DIFF );

        SetRectRgn( hrgn2, rClip.left, rClip.top, rClip.right, rClip.bottom );
        CombineRgn( hrgn, hrgn, hrgn2, RGN_AND );

        if (hrgn3)
        {
            CombineRgn( hrgn, hrgn, hrgn3, RGN_OR );
            DeleteObject( hrgn3 );
        }

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
