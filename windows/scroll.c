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
    if (USER_Driver.pScrollDC)
        return USER_Driver.pScrollDC( hdc, dx, dy, rc, prLClip, hrgnUpdate, rcUpdate );
    return FALSE;
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
