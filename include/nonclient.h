/*
 * Window non-client functions definitions
 *
 * Copyright 1995 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_NONCLIENT_H
#define __WINE_NONCLIENT_H

#include <windef.h>

extern LONG NC_HandleNCPaint( HWND hwnd , HRGN clip);
extern LONG NC_HandleNCActivate( HWND hwnd, WPARAM wParam );
extern LONG NC_HandleNCCalcSize( HWND hwnd, RECT *winRect );
extern LONG NC_HandleNCHitTest( HWND hwnd, POINT pt );
extern LONG NC_HandleNCLButtonDown( HWND hwnd, WPARAM wParam, LPARAM lParam );
extern LONG NC_HandleNCLButtonDblClk( HWND hwnd, WPARAM wParam, LPARAM lParam);
extern LONG NC_HandleSysCommand( HWND hwnd, WPARAM wParam, LPARAM lParam );
extern LONG NC_HandleSetCursor( HWND hwnd, WPARAM wParam, LPARAM lParam );
extern void NC_DrawSysButton( HWND hwnd, HDC hdc, BOOL down );
extern BOOL NC_DrawSysButton95( HWND hwnd, HDC hdc, BOOL down );
extern void NC_GetSysPopupPos( HWND hwnd, RECT* rect );
extern void NC_GetInsideRect( HWND hwnd, RECT *rect );

#endif /* __WINE_NONCLIENT_H */
