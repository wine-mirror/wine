/*
 * Window non-client functions definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_NONCLIENT_H
#define __WINE_NONCLIENT_H

#include "windef.h"

struct tagWND;

extern LONG   NC_HandleNCPaint( HWND hwnd , HRGN clip);
extern LONG   NC_HandleNCActivate( struct tagWND *pwnd, WPARAM wParam );
extern LONG   NC_HandleNCCalcSize( struct tagWND *pWnd, RECT *winRect );
extern LONG   NC_HandleNCHitTest( HWND hwnd, POINT pt );
extern LONG   NC_HandleNCLButtonDown( struct tagWND* pWnd, WPARAM wParam, LPARAM lParam );
extern LONG   NC_HandleNCLButtonDblClk( struct tagWND *pWnd, WPARAM wParam, LPARAM lParam);
extern LONG   NC_HandleSysCommand( HWND hwnd, WPARAM wParam, POINT pt );
extern LONG   NC_HandleSetCursor( HWND hwnd, WPARAM wParam, LPARAM lParam );
extern void   NC_DrawSysButton( HWND hwnd, HDC hdc, BOOL down );
extern BOOL NC_DrawSysButton95( HWND hwnd, HDC hdc, BOOL down );
extern BOOL NC_GetSysPopupPos( struct tagWND* wndPtr, RECT* rect );
extern void NC_GetInsideRect( HWND hwnd, RECT *rect );

#endif /* __WINE_NONCLIENT_H */
