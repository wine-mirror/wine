/*
 * Window non-client functions definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_NONCLIENT_H
#define __WINE_NONCLIENT_H

#include "win.h"

extern void NC_GetMinMaxInfo( WND *pWnd, POINT16 *maxSize, POINT16 *maxPos,
                              POINT16 *minTrack, POINT16 *maxTrack );
extern void NC_DoNCPaint( HWND32 hwnd, HRGN32 clip, BOOL32 suppress_menupaint);
extern LONG NC_HandleNCPaint( HWND32 hwnd , HRGN32 clip);
extern LONG NC_HandleNCActivate( WND *pwnd, WPARAM16 wParam );
extern LONG NC_HandleNCCalcSize( WND *pWnd, RECT16 *winRect );
extern LONG NC_HandleNCHitTest( HWND32 hwnd, POINT16 pt );
extern LONG NC_HandleNCLButtonDown( HWND32 hwnd, WPARAM16 wParam,
                                    LPARAM lParam );
extern LONG NC_HandleNCLButtonDblClk( WND *pWnd, WPARAM16 wParam, LPARAM lParam);
extern LONG NC_HandleSysCommand( HWND32 hwnd, WPARAM16 wParam, POINT16 pt );
extern LONG NC_HandleSetCursor( HWND32 hwnd, WPARAM16 wParam, LPARAM lParam );

#endif /* __WINE_NONCLIENT_H */
