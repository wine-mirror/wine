#ifndef __WINE_NONCLIENT_H
#define __WINE_NONCLIENT_H

extern LONG NC_HandleNCPaint( HWND hwnd, HRGN hrgn );
extern LONG NC_HandleNCActivate( HWND hwnd, WORD wParam );
extern LONG NC_HandleNCCalcSize( HWND hwnd, NCCALCSIZE_PARAMS *params );
extern LONG NC_HandleNCHitTest( HWND hwnd, POINT pt );
extern LONG NC_HandleNCLButtonDown( HWND hwnd, WORD wParam, LONG lParam );
extern LONG NC_HandleNCLButtonDblClk( HWND hwnd, WORD wParam, LONG lParam );
extern LONG NC_HandleSysCommand( HWND hwnd, WORD wParam, POINT pt );
extern LONG NC_HandleSetCursor( HWND hwnd, WORD wParam, LONG lParam );

#endif /* __WINE_NONCLIENT_H */
