/*
 * User controls definitions
 *
 * Copyright 2000 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_CONTROLS_H
#define __WINE_CONTROLS_H

#include "ntuser.h"

extern LRESULT WINAPI ImeWndProcA(HWND,UINT,WPARAM,LPARAM);
extern LRESULT WINAPI ImeWndProcW(HWND,UINT,WPARAM,LPARAM);
extern LRESULT WINAPI DesktopWndProcA(HWND,UINT,WPARAM,LPARAM);
extern LRESULT WINAPI DesktopWndProcW(HWND,UINT,WPARAM,LPARAM);
extern LRESULT WINAPI DialogWndProcA(HWND,UINT,WPARAM,LPARAM);
extern LRESULT WINAPI DialogWndProcW(HWND,UINT,WPARAM,LPARAM);
extern LRESULT WINAPI IconTitleWndProcA(HWND,UINT,WPARAM,LPARAM);
extern LRESULT WINAPI IconTitleWndProcW(HWND,UINT,WPARAM,LPARAM);
extern LRESULT WINAPI PopupMenuWndProcA(HWND,UINT,WPARAM,LPARAM);
extern LRESULT WINAPI PopupMenuWndProcW(HWND,UINT,WPARAM,LPARAM);
extern LRESULT WINAPI MessageWndProc(HWND,UINT,WPARAM,LPARAM);

/* Wow handlers */

/* the structures must match the corresponding ones in user.exe */
struct wow_handlers16
{
    LRESULT (*button_proc)(HWND,UINT,WPARAM,LPARAM,BOOL);
    LRESULT (*combo_proc)(HWND,UINT,WPARAM,LPARAM,BOOL);
    LRESULT (*edit_proc)(HWND,UINT,WPARAM,LPARAM,BOOL);
    LRESULT (*listbox_proc)(HWND,UINT,WPARAM,LPARAM,BOOL);
    LRESULT (*mdiclient_proc)(HWND,UINT,WPARAM,LPARAM,BOOL);
    LRESULT (*scrollbar_proc)(HWND,UINT,WPARAM,LPARAM,BOOL);
    LRESULT (*static_proc)(HWND,UINT,WPARAM,LPARAM,BOOL);
    HWND    (*create_window)(CREATESTRUCTW*,LPCWSTR,HINSTANCE,BOOL);
    LRESULT (*call_window_proc)(HWND,UINT,WPARAM,LPARAM,LRESULT*,void*);
    LRESULT (*call_dialog_proc)(HWND,UINT,WPARAM,LPARAM,LRESULT*,void*);
};

struct wow_handlers32
{
    LRESULT (*button_proc)(HWND,UINT,WPARAM,LPARAM,BOOL);
    LRESULT (*combo_proc)(HWND,UINT,WPARAM,LPARAM,BOOL);
    LRESULT (*edit_proc)(HWND,UINT,WPARAM,LPARAM,BOOL);
    LRESULT (*listbox_proc)(HWND,UINT,WPARAM,LPARAM,BOOL);
    LRESULT (*mdiclient_proc)(HWND,UINT,WPARAM,LPARAM,BOOL);
    LRESULT (*scrollbar_proc)(HWND,UINT,WPARAM,LPARAM,BOOL);
    LRESULT (*static_proc)(HWND,UINT,WPARAM,LPARAM,BOOL);
    HWND    (*create_window)(CREATESTRUCTW*,LPCWSTR,HINSTANCE,BOOL);
    HWND    (*get_win_handle)(HWND);
    WNDPROC (*alloc_winproc)(WNDPROC,BOOL);
    struct tagDIALOGINFO *(*get_dialog_info)(HWND,BOOL);
    INT     (*dialog_box_loop)(HWND,HWND);
};

extern struct wow_handlers16 wow_handlers;

extern LRESULT ButtonWndProc_common(HWND,UINT,WPARAM,LPARAM,BOOL);
extern LRESULT ComboWndProc_common(HWND,UINT,WPARAM,LPARAM,BOOL);
extern LRESULT EditWndProc_common(HWND,UINT,WPARAM,LPARAM,BOOL);
extern LRESULT ListBoxWndProc_common(HWND,UINT,WPARAM,LPARAM,BOOL);
extern LRESULT MDIClientWndProc_common(HWND,UINT,WPARAM,LPARAM,BOOL);
extern LRESULT ScrollBarWndProc_common(HWND,UINT,WPARAM,LPARAM,BOOL);
extern LRESULT StaticWndProc_common(HWND,UINT,WPARAM,LPARAM,BOOL);

/* desktop */
extern BOOL update_wallpaper( const WCHAR *wallpaper, const WCHAR *pattern );

/* nonclient area */
extern LRESULT NC_HandleSysCommand( HWND hwnd, WPARAM wParam, LPARAM lParam );

/* scrollbar */

extern void SCROLL_DrawScrollBar( HWND hwnd, HDC hdc, INT nBar, enum SCROLL_HITTEST hit_test,
                                  const struct SCROLL_TRACKING_INFO *tracking_info, BOOL arrows,
                                  BOOL interior );
extern void SCROLL_HandleScrollEvent( HWND hwnd, INT nBar, UINT msg, POINT pt );
extern void SCROLL_TrackScrollBar( HWND hwnd, INT scrollbar, POINT pt );

/* combo box */

#define ID_CB_LISTBOX           1000
#define ID_CB_EDIT              1001

/* internal flags */
#define CBF_DROPPED             0x0001
#define CBF_BUTTONDOWN          0x0002
#define CBF_NOROLLUP            0x0004
#define CBF_MEASUREITEM         0x0008
#define CBF_FOCUSED             0x0010
#define CBF_CAPTURE             0x0020
#define CBF_EDIT                0x0040
#define CBF_NORESIZE            0x0080
#define CBF_NOTIFY              0x0100
#define CBF_NOREDRAW            0x0200
#define CBF_SELCHANGE           0x0400
#define CBF_NOEDITNOTIFY        0x1000
#define CBF_NOLBSELECT          0x2000  /* do not change current selection */
#define CBF_BEENFOCUSED         0x4000  /* has it ever had focus           */
#define CBF_EUI                 0x8000

/* combo state struct */
typedef struct
{
   HWND           self;
   HWND           owner;
   UINT           dwStyle;
   HWND           hWndEdit;
   HWND           hWndLBox;
   UINT           wState;
   HFONT          hFont;
   RECT           textRect;
   RECT           buttonRect;
   RECT           droppedRect;
   INT            droppedIndex;
   INT            fixedOwnerDrawHeight;
   INT            droppedWidth;   /* not used unless set explicitly */
   INT            item_height;
} HEADCOMBO,*LPHEADCOMBO;

extern BOOL COMBO_FlipListbox( LPHEADCOMBO, BOOL, BOOL );

/* Dialog info structure (note: shared with user.exe) */
typedef struct tagDIALOGINFO
{
    HWND      hwndFocus;   /* Current control with focus */
    HFONT     hUserFont;   /* Dialog font */
    HMENU     hMenu;       /* Dialog menu */
    UINT      xBaseUnit;   /* Dialog units (depends on the font) */
    UINT      yBaseUnit;
    INT       idResult;    /* EndDialog() result / default pushbutton ID */
    UINT      flags;       /* EndDialog() called for this dialog */
} DIALOGINFO;

#define DF_END  0x0001

extern DIALOGINFO *DIALOG_get_info( HWND hwnd, BOOL create );
extern INT DIALOG_DoDialogBox( HWND hwnd, HWND owner );

HRGN set_control_clipping( HDC hdc, const RECT *rect );

#endif  /* __WINE_CONTROLS_H */
