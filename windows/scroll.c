/*
 * Scroll windows and DCs
 *
 * Copyright  David W. Metcalfe, 1993
 *	      Alex Korobka       1995,1996
 *
 *
 */

#include <stdlib.h>

#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "winuser.h"
#include "win.h"
#include "gdi.h"
#include "dce.h"
#include "region.h"
#include "user.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(scroll);

/*************************************************************************
 *		ScrollWindow (USER.61)
 */
void WINAPI ScrollWindow16(HWND16 hwnd, INT16 dx, INT16 dy, const RECT16 *rect,
                           const RECT16 *clipRect )
{
    RECT rect32, clipRect32;

    if (rect) CONV_RECT16TO32( rect, &rect32 );
    if (clipRect) CONV_RECT16TO32( clipRect, &clipRect32 );
    ScrollWindow( hwnd, dx, dy, rect ? &rect32 : NULL,
                    clipRect ? &clipRect32 : NULL );
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
 *		ScrollDC (USER.221)
 */
BOOL16 WINAPI ScrollDC16( HDC16 hdc, INT16 dx, INT16 dy, const RECT16 *rect,
                          const RECT16 *cliprc, HRGN16 hrgnUpdate,
                          LPRECT16 rcUpdate )
{
    RECT rect32, clipRect32, rcUpdate32;
    BOOL16 ret;

    if (rect) CONV_RECT16TO32( rect, &rect32 );
    if (cliprc) CONV_RECT16TO32( cliprc, &clipRect32 );
    ret = ScrollDC( hdc, dx, dy, rect ? &rect32 : NULL,
                      cliprc ? &clipRect32 : NULL, hrgnUpdate, &rcUpdate32 );
    if (rcUpdate) CONV_RECT32TO16( &rcUpdate32, rcUpdate );
    return ret;
}


/*************************************************************************
 *		ScrollDC (USER32.@)
 * 
 *   Only the hrgnUpdate is return in device coordinate.
 *   rcUpdate must be returned in logical coordinate to comply with win API.
 *
 */
BOOL WINAPI ScrollDC( HDC hdc, INT dx, INT dy, const RECT *rc,
                          const RECT *prLClip, HRGN hrgnUpdate,
                          LPRECT rcUpdate )
{
    RECT rect, rClip, rSrc;
    POINT src, dest;
    DC *dc = DC_GetDCUpdate( hdc );

    TRACE("%04x %d,%d hrgnUpdate=%04x rcUpdate = %p cliprc = (%d,%d-%d,%d), rc=(%d,%d-%d,%d)\n",
                   (HDC16)hdc, dx, dy, hrgnUpdate, rcUpdate, 
		   prLClip ? prLClip->left : 0, prLClip ? prLClip->top : 0, prLClip ? prLClip->right : 0, prLClip ? prLClip->bottom : 0,
		   rc ? rc->left : 0, rc ? rc->top : 0, rc ? rc->right : 0, rc ? rc->bottom : 0 );

    if ( !dc || !hdc ) return FALSE;

/*
    TRACE(scroll,"\t[wndOrgX=%i, wndExtX=%i, vportOrgX=%i, vportExtX=%i]\n",
          dc->wndOrgX, dc->wndExtX, dc->vportOrgX, dc->vportExtX );
    TRACE(scroll,"\t[wndOrgY=%i, wndExtY=%i, vportOrgY=%i, vportExtY=%i]\n",
	  dc->wndOrgY, dc->wndExtY, dc->vportOrgY, dc->vportExtY );
*/

    /* compute device clipping region (in device coordinates) */

    if ( rc )
	rect = *rc;
    else /* maybe we should just return FALSE? */
	GetClipBox( hdc, &rect );

    LPtoDP( hdc, (LPPOINT)&rect, 2 );

    if (prLClip)
    {
        rClip = *prLClip;
        LPtoDP( hdc, (LPPOINT)&rClip, 2 );
	IntersectRect( &rClip, &rect, &rClip );
    }
    else
        rClip = rect;

    dx = XLPTODP ( dc, rect.left + dx ) - XLPTODP ( dc, rect.left );
    dy = YLPTODP ( dc, rect.top + dy ) - YLPTODP ( dc, rect.top );

    rSrc = rClip;
    OffsetRect( &rSrc, -dx, -dy );
    IntersectRect( &rSrc, &rSrc, &rect );

    if(dc->hVisRgn)
    {
        if (!IsRectEmpty(&rSrc))
        {
            dest.x = (src.x = rSrc.left) + dx;
            dest.y = (src.y = rSrc.top) + dy;

            /* copy bits */
    
            DPtoLP( hdc, (LPPOINT)&rSrc, 2 );
            DPtoLP( hdc, &src, 1 );
            DPtoLP( hdc, &dest, 1 );

            if (!BitBlt( hdc, dest.x, dest.y,
                           rSrc.right - rSrc.left, rSrc.bottom - rSrc.top,
                           hdc, src.x, src.y, SRCCOPY))
            {
                GDI_ReleaseObj( hdc );
                return FALSE;
            }
        }

        /* compute update areas */

        if (hrgnUpdate || rcUpdate)
        {
            HRGN hrgn =
              (hrgnUpdate) ? hrgnUpdate : CreateRectRgn( 0,0,0,0 );
            HRGN hrgn2;

            hrgn2 = CreateRectRgnIndirect( &rect );
            OffsetRgn( hrgn2, dc->DCOrgX, dc->DCOrgY );
            CombineRgn( hrgn2, hrgn2, dc->hVisRgn, RGN_AND );
            OffsetRgn( hrgn2, -dc->DCOrgX, -dc->DCOrgY );
            SetRectRgn( hrgn, rClip.left, rClip.top,
                          rClip.right, rClip.bottom );
            CombineRgn( hrgn, hrgn, hrgn2, RGN_AND );
            OffsetRgn( hrgn2, dx, dy );
            CombineRgn( hrgn, hrgn, hrgn2, RGN_DIFF );

            if( rcUpdate )
	    {
		GetRgnBox( hrgn, rcUpdate );

		/* Put the rcUpdate in logical coordinate */
		DPtoLP( hdc, (LPPOINT)rcUpdate, 2 );
	    }
            if (!hrgnUpdate) DeleteObject( hrgn );
            DeleteObject( hrgn2 );

        }
    }
    else
    {
       if (hrgnUpdate) SetRectRgn(hrgnUpdate, 0, 0, 0, 0);
       if (rcUpdate) SetRectEmpty(rcUpdate);
    }

    GDI_ReleaseObj( hdc );
    return TRUE;
}


/*************************************************************************
 *		ScrollWindowEx (USER.319)
 */
INT16 WINAPI ScrollWindowEx16( HWND16 hwnd, INT16 dx, INT16 dy,
                               const RECT16 *rect, const RECT16 *clipRect,
                               HRGN16 hrgnUpdate, LPRECT16 rcUpdate,
                               UINT16 flags )
{
    RECT rect32, clipRect32, rcUpdate32;
    BOOL16 ret;

    if (rect) CONV_RECT16TO32( rect, &rect32 );
    if (clipRect) CONV_RECT16TO32( clipRect, &clipRect32 );
    ret = ScrollWindowEx( hwnd, dx, dy, rect ? &rect32 : NULL,
                            clipRect ? &clipRect32 : NULL, hrgnUpdate,
                            (rcUpdate) ? &rcUpdate32 : NULL, flags );
    if (rcUpdate) CONV_RECT32TO16( &rcUpdate32, rcUpdate );
    return ret;
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
    if (USER_Driver.pScrollWindowEx)
        return USER_Driver.pScrollWindowEx( hwnd, dx, dy, rect, clipRect,
                                            hrgnUpdate, rcUpdate, flags );
    return ERROR;
}
