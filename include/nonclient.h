/*
 * Window non-client functions definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_NONCLIENT_H
#define __WINE_NONCLIENT_H

#include "win.h"

extern LONG NC_HandleNCPaint( HWND32 hwnd , HRGN32 clip);
extern LONG NC_HandleNCActivate( WND *pwnd, WPARAM16 wParam );
extern LONG NC_HandleNCCalcSize( WND *pWnd, RECT32 *winRect );
extern LONG NC_HandleNCHitTest( HWND32 hwnd, POINT16 pt );
extern LONG NC_HandleNCLButtonDown( WND* pWnd, WPARAM16 wParam, LPARAM lParam );
extern LONG NC_HandleNCLButtonDblClk( WND *pWnd, WPARAM16 wParam, LPARAM lParam);
extern LONG NC_HandleSysCommand( HWND32 hwnd, WPARAM16 wParam, POINT16 pt );
extern LONG NC_HandleSetCursor( HWND32 hwnd, WPARAM16 wParam, LPARAM lParam );
extern void NC_DrawSysButton( HWND32 hwnd, HDC32 hdc, BOOL32 down );
extern void NC_DrawSysButton95( HWND32 hwnd, HDC32 hdc, BOOL32 down );
extern BOOL32 NC_GetSysPopupPos( WND* wndPtr, RECT32* rect );

#endif /* __WINE_NONCLIENT_H */
