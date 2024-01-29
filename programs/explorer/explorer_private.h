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

extern void manage_desktop( WCHAR *arg );
extern void initialize_systray( BOOL using_root, BOOL enable_shell, BOOL show_systray );
extern void initialize_appbar(void);
extern void handle_parent_notify( HWND hwnd, WPARAM wp );
extern void do_startmenu( HWND owner );
extern LRESULT menu_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

#endif  /* __WINE_EXPLORER_PRIVATE_H */
