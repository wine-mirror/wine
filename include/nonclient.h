/*
 * Window non-client functions definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_NONCLIENT_H
#define __WINE_NONCLIENT_H

#include "win.h"

extern LONG   NC_HandleNCPaint( HWND hwnd , HRGN clip);
extern LONG   NC_HandleNCActivate( WND *pwnd, WPARAM16 wParam );
extern LONG   NC_HandleNCCalcSize( WND *pWnd, RECT *winRect );
extern LONG   NC_HandleNCHitTest( HWND hwnd, POINT16 pt );
extern LONG   NC_HandleNCLButtonDown( WND* pWnd, WPARAM16 wParam, LPARAM lParam );
extern LONG   NC_HandleNCLButtonDblClk( WND *pWnd, WPARAM16 wParam, LPARAM lParam);
extern LONG   NC_HandleSysCommand( HWND hwnd, WPARAM16 wParam, POINT16 pt );
extern LONG   NC_HandleSetCursor( HWND hwnd, WPARAM16 wParam, LPARAM lParam );
extern void   NC_DrawSysButton( HWND hwnd, HDC hdc, BOOL down );
extern BOOL NC_DrawSysButton95( HWND hwnd, HDC hdc, BOOL down );
extern BOOL NC_GetSysPopupPos( WND* wndPtr, RECT* rect );

#endif /* __WINE_NONCLIENT_H */
