/*
 * Menu definitions
 */

#ifndef __WINE_MENU_H
#define __WINE_MENU_H

#include "windef.h"

struct tagWND;

LRESULT WINAPI PopupMenuWndProc( HWND hwnd, UINT message, WPARAM wParam,
                                 LPARAM lParam );

extern BOOL MENU_Init(void);
extern BOOL MENU_IsMenuActive(void);
extern HMENU MENU_GetSysMenu(HWND hWndOwner, HMENU hSysPopup);
extern UINT MENU_GetMenuBarHeight( HWND hwnd, UINT menubarWidth,
                                     INT orgX, INT orgY );
extern BOOL MENU_PatchResidentPopup( HQUEUE16, struct tagWND* );
extern void MENU_TrackMouseMenuBar( struct tagWND *wnd, INT ht, POINT pt );
extern void MENU_TrackKbdMenuBar( struct tagWND *wnd, UINT wParam, INT vkey );
extern UINT MENU_DrawMenuBar( HDC hDC, LPRECT lprect,
                                HWND hwnd, BOOL suppress_draw );
extern UINT MENU_FindSubMenu( HMENU *hmenu, HMENU hSubTarget );

#endif /* __WINE_MENU_H */
