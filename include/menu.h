/*
 * Menu definitions
 */

#ifndef __WINE_MENU_H
#define __WINE_MENU_H

extern BOOL MENU_Init(void);
extern HMENU16 MENU_GetDefSysMenu(void);
extern void MENU_InitSysMenuPopup(HMENU16 hmenu, DWORD style, DWORD clsStyle);
extern UINT MENU_GetMenuBarHeight( HWND hwnd, UINT menubarWidth,
				   int orgX, int orgY );
extern void MENU_TrackMouseMenuBar( WND* , INT16 ht, POINT16 pt );
extern void MENU_TrackKbdMenuBar( WND*, UINT16 wParam, INT16 vkey);
extern UINT MENU_DrawMenuBar( HDC32 hDC, LPRECT16 lprect,
			      HWND hwnd, BOOL suppress_draw );
extern LRESULT PopupMenuWndProc( HWND hwnd, UINT message,
                                 WPARAM16 wParam, LPARAM lParam );

#endif /* __WINE_MENU_H */
