/*
 * Window non-client functions definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_NONCLIENT_H
#define __WINE_NONCLIENT_H

#include "windef.h"

extern LONG NC_HandleNCPaint( HWND hwnd , HRGN clip);
extern LONG NC_HandleNCActivate( HWND hwnd, WPARAM wParam );
extern LONG NC_HandleNCCalcSize( HWND hwnd, RECT *winRect );
extern LONG NC_HandleNCHitTest( HWND hwnd, POINT pt );
extern LONG NC_HandleNCLButtonDown( HWND hwnd, WPARAM wParam, LPARAM lParam );
extern LONG NC_HandleNCLButtonDblClk( HWND hwnd, WPARAM wParam, LPARAM lParam);
extern LONG NC_HandleSysCommand( HWND hwnd, WPARAM wParam, POINT pt );
extern LONG NC_HandleSetCursor( HWND hwnd, WPARAM wParam, LPARAM lParam );
extern void NC_DrawSysButton( HWND hwnd, HDC hdc, BOOL down );
extern BOOL NC_DrawSysButton95( HWND hwnd, HDC hdc, BOOL down );
extern void NC_GetSysPopupPos( HWND hwnd, RECT* rect );
extern void NC_GetInsideRect( HWND hwnd, RECT *rect );

#endif /* __WINE_NONCLIENT_H */
