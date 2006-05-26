/*
 * Window procedure callbacks definitions
 *
 * Copyright 1996 Alexandre Julliard
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

#ifndef __WINE_WINPROC_H
#define __WINE_WINPROC_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"

typedef LRESULT (*winproc_callback_t)( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                       LRESULT *result, void *arg );
typedef LRESULT (*winproc_callback16_t)( HWND16 hwnd, UINT16 msg, WPARAM16 wp, LPARAM lp,
                                         LRESULT *result, void *arg );

extern WNDPROC16 WINPROC_GetProc16( WNDPROC proc, BOOL unicode );
extern WNDPROC WINPROC_AllocProc16( WNDPROC16 func );
extern WNDPROC WINPROC_GetProc( WNDPROC proc, BOOL unicode );
extern WNDPROC WINPROC_AllocProc( WNDPROC funcA, WNDPROC funcW );
extern BOOL WINPROC_IsUnicode( WNDPROC proc, BOOL def_val );

extern LRESULT WINPROC_CallProcAtoW( winproc_callback_t callback, HWND hwnd, UINT msg,
                                     WPARAM wParam, LPARAM lParam, LRESULT *result, void *arg );
extern LRESULT WINPROC_CallProc16To32A( winproc_callback_t callback, HWND16 hwnd, UINT16 msg,
                                        WPARAM16 wParam, LPARAM lParam, LRESULT *result, void *arg );
extern LRESULT WINPROC_CallProc32ATo16( winproc_callback16_t callback, HWND hwnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam, LRESULT *result, void *arg );

extern INT_PTR WINPROC_CallDlgProc16( DLGPROC16 func, HWND16 hwnd, UINT16 msg, WPARAM16 wParam, LPARAM lParam );
extern INT_PTR WINPROC_CallDlgProcA( DLGPROC func, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
extern INT_PTR WINPROC_CallDlgProcW( DLGPROC func, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

#endif  /* __WINE_WINPROC_H */
