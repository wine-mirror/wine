/*
 * Window non-client functions definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_NONCLIENT_H
#define __WINE_NONCLIENT_H

#include "windows.h"

extern void NC_GetInsideRect( HWND hwnd, RECT *rect );
extern void NC_GetMinMaxInfo( HWND hwnd, POINT *maxSize, POINT *maxPos,
                              POINT *minTrack, POINT *maxTrack );
extern void NC_DoNCPaint( HWND hwnd, BOOL active, BOOL suppress_menupaint );
extern LONG NC_HandleNCPaint( HWND hwnd );
extern LONG NC_HandleNCActivate( HWND hwnd, WPARAM wParam );
extern LONG NC_HandleNCCalcSize( HWND hwnd, NCCALCSIZE_PARAMS *params );
extern LONG NC_HandleNCHitTest( HWND hwnd, POINT pt );
extern LONG NC_HandleNCLButtonDown( HWND hwnd, WPARAM wParam, LPARAM lParam );
extern LONG NC_HandleNCLButtonDblClk( HWND hwnd, WPARAM wParam, LPARAM lParam );
extern LONG NC_HandleSysCommand( HWND hwnd, WPARAM wParam, POINT pt );
extern LONG NC_HandleSetCursor( HWND hwnd, WPARAM wParam, LPARAM lParam );

#endif /* __WINE_NONCLIENT_H */
