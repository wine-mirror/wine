/*
 * Menu definitions
 */

#ifndef __WINE_MENU_H
#define __WINE_MENU_H

extern BOOL32 MENU_Init(void);
extern HMENU32 MENU_GetSysMenu(HWND32 hWndOwner, HMENU32 hSysPopup);
extern UINT32 MENU_GetMenuBarHeight( HWND32 hwnd, UINT32 menubarWidth,
                                     INT32 orgX, INT32 orgY );
extern void MENU_TrackMouseMenuBar( WND *wnd, INT32 ht, POINT32 pt );
extern void MENU_TrackKbdMenuBar( WND *wnd, UINT32 wParam, INT32 vkey );
extern UINT32 MENU_DrawMenuBar( HDC32 hDC, LPRECT32 lprect,
                                HWND32 hwnd, BOOL32 suppress_draw );

#endif /* __WINE_MENU_H */
