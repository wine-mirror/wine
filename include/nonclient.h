/*
 * Window non-client functions definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_NONCLIENT_H
#define __WINE_NONCLIENT_H

#include "win.h"

extern void NC_GetMinMaxInfo( HWND hwnd, POINT16 *maxSize, POINT16 *maxPos,
                              POINT16 *minTrack, POINT16 *maxTrack );
extern void NC_DoNCPaint( HWND hwnd, HRGN clip, BOOL suppress_menupaint );
extern LONG NC_HandleNCPaint( HWND hwnd , HRGN clip);
extern LONG NC_HandleNCActivate( WND *pwnd, WPARAM wParam );
extern LONG NC_HandleNCCalcSize( WND *pWnd, RECT16 *winRect );
extern LONG NC_HandleNCHitTest( HWND hwnd, POINT16 pt );
extern LONG NC_HandleNCLButtonDown( HWND hwnd, WPARAM wParam, LPARAM lParam );
extern LONG NC_HandleNCLButtonDblClk( WND *pWnd, WPARAM wParam, LPARAM lParam);
extern LONG NC_HandleSysCommand( HWND hwnd, WPARAM wParam, POINT16 pt );
extern LONG NC_HandleSetCursor( HWND hwnd, WPARAM wParam, LPARAM lParam );

#endif /* __WINE_NONCLIENT_H */
