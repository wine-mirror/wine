/*
 * Window definitions
 *
 * Copyright 1993 Alexandre Julliard
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

#ifndef __WINE_WIN_H
#define __WINE_WIN_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>

#include "user_private.h"
#include "wine/server_protocol.h"

struct tagCLASS;
struct tagDIALOGINFO;

  /* Window functions */
extern HWND get_hwnd_message_parent(void) DECLSPEC_HIDDEN;
extern BOOL is_desktop_window( HWND hwnd ) DECLSPEC_HIDDEN;
extern WND *WIN_GetPtr( HWND hwnd ) DECLSPEC_HIDDEN;
extern void WIN_ReleasePtr( WND *ptr ) DECLSPEC_HIDDEN;
extern HWND WIN_GetFullHandle( HWND hwnd ) DECLSPEC_HIDDEN;
extern HWND WIN_IsCurrentProcess( HWND hwnd ) DECLSPEC_HIDDEN;
extern HWND WIN_IsCurrentThread( HWND hwnd ) DECLSPEC_HIDDEN;
extern ULONG WIN_SetStyle( HWND hwnd, ULONG set_bits, ULONG clear_bits ) DECLSPEC_HIDDEN;
extern BOOL WIN_GetRectangles( HWND hwnd, enum coords_relative relative, RECT *rectWindow, RECT *rectClient ) DECLSPEC_HIDDEN;
extern HWND WIN_CreateWindowEx( CREATESTRUCTW *cs, LPCWSTR className, HINSTANCE module, BOOL unicode ) DECLSPEC_HIDDEN;
extern HWND *WIN_ListChildren( HWND hwnd ) DECLSPEC_HIDDEN;
extern void MDI_CalcDefaultChildPos( HWND hwndClient, INT total, LPPOINT lpPos, INT delta, UINT *id ) DECLSPEC_HIDDEN;
extern HDESK open_winstation_desktop( HWINSTA hwinsta, LPCWSTR name, DWORD flags, BOOL inherit, ACCESS_MASK access ) DECLSPEC_HIDDEN;

extern void WINPOS_ActivateOtherWindow( HWND hwnd ) DECLSPEC_HIDDEN;

extern UINT get_win_monitor_dpi( HWND hwnd ) DECLSPEC_HIDDEN;
extern UINT get_thread_dpi(void) DECLSPEC_HIDDEN;
extern POINT map_dpi_point( POINT pt, UINT dpi_from, UINT dpi_to ) DECLSPEC_HIDDEN;
extern POINT point_win_to_phys_dpi( HWND hwnd, POINT pt ) DECLSPEC_HIDDEN;
extern RECT map_dpi_rect( RECT rect, UINT dpi_from, UINT dpi_to ) DECLSPEC_HIDDEN;
extern RECT rect_win_to_thread_dpi( HWND hwnd, RECT rect ) DECLSPEC_HIDDEN;

static inline void mirror_rect( const RECT *window_rect, RECT *rect )
{
    int width = window_rect->right - window_rect->left;
    int tmp = rect->left;
    rect->left = width - rect->right;
    rect->right = width - tmp;
}

#endif  /* __WINE_WIN_H */
