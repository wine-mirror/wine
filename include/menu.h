/*
 * Menu definitions
 */

#ifndef __WINE_MENU_H
#define __WINE_MENU_H

extern BOOL MENU_Init(void);
extern HMENU MENU_GetDefSysMenu(void);
extern UINT MENU_GetMenuBarHeight( HWND hwnd, UINT menubarWidth,
				   int orgX, int orgY );
extern void MENU_TrackMouseMenuBar( HWND hwnd, POINT16 pt );
extern void MENU_TrackKbdMenuBar( WND*, UINT wParam, INT vkey);
extern UINT MENU_DrawMenuBar( HDC hDC, LPRECT16 lprect,
			      HWND hwnd, BOOL suppress_draw );
extern LRESULT PopupMenuWndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam );

#endif /* __WINE_MENU_H */
