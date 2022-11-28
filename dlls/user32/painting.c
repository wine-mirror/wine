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

#include "user_private.h"


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


/*************************************************************************
 *		ScrollWindow (USER32.@)
 *
 */
BOOL WINAPI ScrollWindow( HWND hwnd, INT dx, INT dy,
                          const RECT *rect, const RECT *clipRect )
{
    UINT flags = SW_INVALIDATE | SW_ERASE | (rect ? 0 : SW_SCROLLCHILDREN) | SW_NODCCACHE;
    return NtUserScrollWindowEx( hwnd, dx, dy, rect, clipRect, 0, NULL, flags );
}
