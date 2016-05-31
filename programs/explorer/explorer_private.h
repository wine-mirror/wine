/*
 * Explorer private definitions
 *
 * Copyright 2006 Alexandre Julliard
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

#ifndef __WINE_EXPLORER_PRIVATE_H
#define __WINE_EXPLORER_PRIVATE_H

extern void manage_desktop( WCHAR *arg ) DECLSPEC_HIDDEN;
extern void initialize_systray( HMODULE graphics_driver, BOOL using_root, BOOL enable_shell ) DECLSPEC_HIDDEN;
extern void initialize_appbar(void) DECLSPEC_HIDDEN;
extern void handle_parent_notify( HWND hwnd, WPARAM wp ) DECLSPEC_HIDDEN;
extern void do_startmenu( HWND owner ) DECLSPEC_HIDDEN;
extern LRESULT menu_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) DECLSPEC_HIDDEN;

#endif  /* __WINE_EXPLORER_PRIVATE_H */
