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

#if 0
#pragma makedep unix
#endif

#include "ntgdi_private.h"
#include "ntuser_private.h"

/**********************************************************************
 *           NtUserWindowFromDC (win32u.@)
 */
HWND WINAPI NtUserWindowFromDC( HDC hdc )
{
    struct dce *dce;
    HWND hwnd = 0;

    user_lock();
    dce = (struct dce *)GetDCHook( hdc, NULL );
    if (dce) hwnd = dce->hwnd;
    user_unlock();
    return hwnd;
}
