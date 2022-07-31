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

BOOL WINAPI NtUserEndPaint( HWND hwnd, const PAINTSTRUCT *ps )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserEndPaint( hwnd, ps );
}

BOOL WINAPI NtUserDrawCaptionTemp( HWND hwnd, HDC hdc, const RECT *rect, HFONT font,
                                   HICON icon, const WCHAR *str, UINT flags )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserDrawCaptionTemp( hwnd, hdc, rect, font, icon, str, flags );
}

DWORD WINAPI NtUserDrawMenuBarTemp( HWND hwnd, HDC hdc, RECT *rect, HMENU handle, HFONT font )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserDrawMenuBarTemp( hwnd, hdc, rect, handle, font );
}

INT WINAPI NtUserExcludeUpdateRgn( HDC hdc, HWND hwnd )
{
    if (!unix_funcs) return ERROR;
    return unix_funcs->pNtUserExcludeUpdateRgn( hdc, hwnd );
}

INT WINAPI NtUserReleaseDC( HWND hwnd, HDC hdc )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserReleaseDC( hwnd, hdc );
}

BOOL WINAPI NtUserScrollDC( HDC hdc, INT dx, INT dy, const RECT *scroll, const RECT *clip,
                            HRGN ret_update_rgn, RECT *update_rect )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserScrollDC( hdc, dx, dy, scroll, clip, ret_update_rgn, update_rect );
}

HPALETTE WINAPI NtUserSelectPalette( HDC hdc, HPALETTE hpal, WORD bkg )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSelectPalette( hdc, hpal, bkg );
}

BOOL WINAPI NtUserUpdateLayeredWindow( HWND hwnd, HDC hdc_dst, const POINT *pts_dst, const SIZE *size,
                                       HDC hdc_src, const POINT *pts_src, COLORREF key,
                                       const BLENDFUNCTION *blend, DWORD flags, const RECT *dirty )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserUpdateLayeredWindow( hwnd, hdc_dst, pts_dst, size, hdc_src, pts_src,
                                                   key, blend, flags, dirty );
}

INT WINAPI SetDIBits( HDC hdc, HBITMAP hbitmap, UINT startscan,
                      UINT lines, const void *bits, const BITMAPINFO *info,
                      UINT coloruse )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pSetDIBits( hdc, hbitmap, startscan, lines, bits, info, coloruse );
}

BOOL CDECL __wine_get_icm_profile( HDC hdc, BOOL allow_default, DWORD *size, WCHAR *filename )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->get_icm_profile( hdc, allow_default, size, filename );
}

BOOL CDECL __wine_get_brush_bitmap_info( HBRUSH handle, BITMAPINFO *info, void *bits, UINT *usage )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->get_brush_bitmap_info( handle, info, bits, usage );
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
