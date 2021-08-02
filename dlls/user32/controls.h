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

#include "winuser.h"

/* Built-in class names (see _Undocumented_Windows_ p.418) */
#define POPUPMENU_CLASS_ATOM MAKEINTATOM(32768)  /* PopupMenu */
#define DESKTOP_CLASS_ATOM   MAKEINTATOM(32769)  /* Desktop */
#define DIALOG_CLASS_ATOM    MAKEINTATOM(32770)  /* Dialog */
#define WINSWITCH_CLASS_ATOM MAKEINTATOM(32771)  /* WinSwitch */
#define ICONTITLE_CLASS_ATOM MAKEINTATOM(32772)  /* IconTitle */

enum builtin_winprocs
{
    /* dual A/W procs */
    WINPROC_BUTTON = 0,
    WINPROC_COMBO,
    WINPROC_DEFWND,
    WINPROC_DIALOG,
    WINPROC_EDIT,
    WINPROC_LISTBOX,
    WINPROC_MDICLIENT,
    WINPROC_SCROLLBAR,
    WINPROC_STATIC,
    WINPROC_IME,
    /* unicode-only procs */
    WINPROC_DESKTOP,
    WINPROC_ICONTITLE,
    WINPROC_MENU,
    WINPROC_MESSAGE,
    NB_BUILTIN_WINPROCS,
    NB_BUILTIN_AW_WINPROCS = WINPROC_DESKTOP
};

#define WINPROC_HANDLE (~0u >> 16)
#define BUILTIN_WINPROC(index) ((WNDPROC)(ULONG_PTR)((index) | (WINPROC_HANDLE << 16)))

/* Built-in class descriptor */
struct builtin_class_descr
{
    LPCWSTR   name;    /* class name */
    UINT      style;   /* class style */
    enum builtin_winprocs proc;
    INT       extra;   /* window extra bytes */
    ULONG_PTR cursor;  /* cursor id */
    HBRUSH    brush;   /* brush or system color */
};

extern const struct builtin_class_descr BUTTON_builtin_class DECLSPEC_HIDDEN;
extern const struct builtin_class_descr COMBO_builtin_class DECLSPEC_HIDDEN;
extern const struct builtin_class_descr COMBOLBOX_builtin_class DECLSPEC_HIDDEN;
extern const struct builtin_class_descr DIALOG_builtin_class DECLSPEC_HIDDEN;
extern const struct builtin_class_descr DESKTOP_builtin_class DECLSPEC_HIDDEN;
extern const struct builtin_class_descr EDIT_builtin_class DECLSPEC_HIDDEN;
extern const struct builtin_class_descr ICONTITLE_builtin_class DECLSPEC_HIDDEN;
extern const struct builtin_class_descr LISTBOX_builtin_class DECLSPEC_HIDDEN;
extern const struct builtin_class_descr MDICLIENT_builtin_class DECLSPEC_HIDDEN;
extern const struct builtin_class_descr MENU_builtin_class DECLSPEC_HIDDEN;
extern const struct builtin_class_descr MESSAGE_builtin_class DECLSPEC_HIDDEN;
extern const struct builtin_class_descr SCROLL_builtin_class DECLSPEC_HIDDEN;
extern const struct builtin_class_descr STATIC_builtin_class DECLSPEC_HIDDEN;
extern const struct builtin_class_descr IME_builtin_class DECLSPEC_HIDDEN;

extern LRESULT WINAPI ImeWndProcA(HWND,UINT,WPARAM,LPARAM) DECLSPEC_HIDDEN;
extern LRESULT WINAPI ImeWndProcW(HWND,UINT,WPARAM,LPARAM) DECLSPEC_HIDDEN;
extern LRESULT WINAPI DesktopWndProc(HWND,UINT,WPARAM,LPARAM) DECLSPEC_HIDDEN;
extern LRESULT WINAPI IconTitleWndProc(HWND,UINT,WPARAM,LPARAM) DECLSPEC_HIDDEN;
extern LRESULT WINAPI PopupMenuWndProc(HWND,UINT,WPARAM,LPARAM) DECLSPEC_HIDDEN;
extern LRESULT WINAPI MessageWndProc(HWND,UINT,WPARAM,LPARAM) DECLSPEC_HIDDEN;

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
    DWORD   (*wait_message)(DWORD,const HANDLE*,DWORD,DWORD,DWORD);
    HWND    (*create_window)(CREATESTRUCTW*,LPCWSTR,HINSTANCE,BOOL);
    LRESULT (*call_window_proc)(HWND,UINT,WPARAM,LPARAM,LRESULT*,void*);
    LRESULT (*call_dialog_proc)(HWND,UINT,WPARAM,LPARAM,LRESULT*,void*);
    void    (*free_icon_param)(ULONG_PTR);
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
    DWORD   (*wait_message)(DWORD,const HANDLE*,DWORD,DWORD,DWORD);
    HWND    (*create_window)(CREATESTRUCTW*,LPCWSTR,HINSTANCE,BOOL);
    HWND    (*get_win_handle)(HWND);
    WNDPROC (*alloc_winproc)(WNDPROC,BOOL);
    struct tagDIALOGINFO *(*get_dialog_info)(HWND,BOOL);
    INT     (*dialog_box_loop)(HWND,HWND);
    ULONG_PTR (*get_icon_param)(HICON);
    ULONG_PTR (*set_icon_param)(HICON,ULONG_PTR);
};

extern struct wow_handlers16 wow_handlers DECLSPEC_HIDDEN;

extern LRESULT ButtonWndProc_common(HWND,UINT,WPARAM,LPARAM,BOOL) DECLSPEC_HIDDEN;
extern LRESULT ComboWndProc_common(HWND,UINT,WPARAM,LPARAM,BOOL) DECLSPEC_HIDDEN;
extern LRESULT EditWndProc_common(HWND,UINT,WPARAM,LPARAM,BOOL) DECLSPEC_HIDDEN;
extern LRESULT ListBoxWndProc_common(HWND,UINT,WPARAM,LPARAM,BOOL) DECLSPEC_HIDDEN;
extern LRESULT MDIClientWndProc_common(HWND,UINT,WPARAM,LPARAM,BOOL) DECLSPEC_HIDDEN;
extern LRESULT ScrollBarWndProc_common(HWND,UINT,WPARAM,LPARAM,BOOL) DECLSPEC_HIDDEN;
extern LRESULT StaticWndProc_common(HWND,UINT,WPARAM,LPARAM,BOOL) DECLSPEC_HIDDEN;

extern ULONG_PTR get_icon_param( HICON handle ) DECLSPEC_HIDDEN;
extern ULONG_PTR set_icon_param( HICON handle, ULONG_PTR param ) DECLSPEC_HIDDEN;

/* Class functions */
struct tagCLASS;  /* opaque structure */
struct tagWND;
extern ATOM get_int_atom_value( LPCWSTR name ) DECLSPEC_HIDDEN;
extern void register_builtin_classes(void) DECLSPEC_HIDDEN;
extern void register_desktop_class(void) DECLSPEC_HIDDEN;
extern WNDPROC get_class_winproc( struct tagCLASS *class ) DECLSPEC_HIDDEN;
extern struct dce *get_class_dce( struct tagCLASS *class ) DECLSPEC_HIDDEN;
extern struct dce *set_class_dce( struct tagCLASS *class, struct dce *dce ) DECLSPEC_HIDDEN;

/* defwnd proc */
extern HBRUSH DEFWND_ControlColor( HDC hDC, UINT ctlType ) DECLSPEC_HIDDEN;

/* desktop */
extern BOOL update_wallpaper( const WCHAR *wallpaper, const WCHAR *pattern ) DECLSPEC_HIDDEN;

/* menu controls */
extern HWND MENU_IsMenuActive(void) DECLSPEC_HIDDEN;
extern UINT MENU_GetMenuBarHeight( HWND hwnd, UINT menubarWidth,
                                     INT orgX, INT orgY ) DECLSPEC_HIDDEN;
extern BOOL MENU_SetMenu(HWND, HMENU) DECLSPEC_HIDDEN;
extern void MENU_TrackMouseMenuBar( HWND hwnd, INT ht, POINT pt ) DECLSPEC_HIDDEN;
extern void MENU_TrackKbdMenuBar( HWND hwnd, UINT wParam, WCHAR wChar ) DECLSPEC_HIDDEN;
extern UINT MENU_DrawMenuBar( HDC hDC, LPRECT lprect, HWND hwnd ) DECLSPEC_HIDDEN;
extern void MENU_EndMenu(HWND) DECLSPEC_HIDDEN;

/* nonclient area */
extern LRESULT NC_HandleNCPaint( HWND hwnd , HRGN clip) DECLSPEC_HIDDEN;
extern LRESULT NC_HandleNCActivate( HWND hwnd, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;
extern LRESULT NC_HandleNCCalcSize( HWND hwnd, WPARAM wParam, RECT *winRect ) DECLSPEC_HIDDEN;
extern LRESULT NC_HandleNCHitTest( HWND hwnd, POINT pt ) DECLSPEC_HIDDEN;
extern LRESULT NC_HandleNCLButtonDown( HWND hwnd, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;
extern LRESULT NC_HandleNCMouseMove( HWND hwnd, POINT pt ) DECLSPEC_HIDDEN;
extern LRESULT NC_HandleNCMouseLeave( HWND hwnd ) DECLSPEC_HIDDEN;
extern LRESULT NC_HandleNCRButtonDown( HWND hwnd, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;
extern LRESULT NC_HandleNCLButtonDblClk( HWND hwnd, WPARAM wParam, LPARAM lParam) DECLSPEC_HIDDEN;
extern LRESULT NC_HandleSysCommand( HWND hwnd, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;
extern LRESULT NC_HandleSetCursor( HWND hwnd, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;
extern BOOL NC_DrawSysButton( HWND hwnd, HDC hdc, BOOL down ) DECLSPEC_HIDDEN;
extern void NC_GetSysPopupPos( HWND hwnd, RECT* rect ) DECLSPEC_HIDDEN;

/* scrollbar */

extern void SCROLL_DrawNCScrollBar( HWND hwnd, HDC hdc, BOOL draw_horizontal, BOOL draw_vertical ) DECLSPEC_HIDDEN;
extern void SCROLL_DrawScrollBar( HWND hwnd, HDC hdc, INT nBar, enum SCROLL_HITTEST hit_test,
                                  const struct SCROLL_TRACKING_INFO *tracking_info, BOOL arrows,
                                  BOOL interior ) DECLSPEC_HIDDEN;
extern void SCROLL_HandleScrollEvent( HWND hwnd, INT nBar, UINT msg, POINT pt ) DECLSPEC_HIDDEN;
extern void SCROLL_TrackScrollBar( HWND hwnd, INT scrollbar, POINT pt ) DECLSPEC_HIDDEN;

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

extern BOOL COMBO_FlipListbox( LPHEADCOMBO, BOOL, BOOL ) DECLSPEC_HIDDEN;

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

extern DIALOGINFO *DIALOG_get_info( HWND hwnd, BOOL create ) DECLSPEC_HIDDEN;
extern INT DIALOG_DoDialogBox( HWND hwnd, HWND owner ) DECLSPEC_HIDDEN;

HRGN set_control_clipping( HDC hdc, const RECT *rect ) DECLSPEC_HIDDEN;

#endif  /* __WINE_CONTROLS_H */
