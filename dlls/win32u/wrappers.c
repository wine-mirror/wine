/*
 * Unix call wrappers
 *
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "win32u_private.h"

static const struct unix_funcs *unix_funcs;

BOOL CDECL __wine_get_icm_profile( HDC hdc, BOOL allow_default, DWORD *size, WCHAR *filename )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->get_icm_profile( hdc, allow_default, size, filename );
}

BOOL CDECL __wine_get_file_outline_text_metric( const WCHAR *path, OUTLINETEXTMETRICW *otm )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->get_file_outline_text_metric( path, otm );
}

BOOL CDECL __wine_send_input( HWND hwnd, const INPUT *input, const RAWINPUT *rawinput )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->wine_send_input( hwnd, input, rawinput );
}

extern void wrappers_init( unixlib_handle_t handle )
{
    const void *args;
    if (!__wine_unix_call( handle, 1, &args )) unix_funcs = args;
}
