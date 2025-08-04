/*
 * Copyright 2022 Rémi Bernon for CodeWeavers
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
 *
 */

#ifndef __JOY_PRIVATE_H
#define __JOY_PRIVATE_H

#include <stdarg.h>
#include <stddef.h>

#include "windef.h"
#include "winbase.h"

#include "dinput.h"
#include "xinput.h"

#include "resource.h"

extern void paint_gamepad_view( HWND hwnd, XINPUT_STATE *state );

extern INT_PTR CALLBACK test_di_dialog_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
extern LRESULT CALLBACK test_di_axes_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
extern LRESULT CALLBACK test_di_povs_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
extern LRESULT CALLBACK test_di_buttons_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

extern INT_PTR CALLBACK test_xi_dialog_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
extern LRESULT CALLBACK test_xi_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

extern INT_PTR CALLBACK test_wgi_dialog_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
extern LRESULT CALLBACK test_wgi_gamepad_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

#endif /* __JOY_PRIVATE_H */
