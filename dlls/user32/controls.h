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
#include "wine/winbase16.h"

/* Built-in class names (see _Undocumented_Windows_ p.418) */
#define POPUPMENU_CLASS_ATOM MAKEINTATOM(32768)  /* PopupMenu */
#define DESKTOP_CLASS_ATOM   MAKEINTATOM(32769)  /* Desktop */
#define DIALOG_CLASS_ATOM    MAKEINTATOM(32770)  /* Dialog */
#define WINSWITCH_CLASS_ATOM MAKEINTATOM(32771)  /* WinSwitch */
#define ICONTITLE_CLASS_ATOM MAKEINTATOM(32772)  /* IconTitle */

/* Built-in class descriptor */
struct builtin_class_descr
{
    LPCWSTR   name;    /* class name */
    UINT      style;   /* class style */
    WNDPROC   procA;   /* ASCII window procedure */
    WNDPROC   procW;   /* Unicode window procedure */
    INT       extra;   /* window extra bytes */
    ULONG_PTR cursor;  /* cursor id */
    HBRUSH    brush;   /* brush or system color */
};

extern WNDPROC EDIT_winproc_handle;

/* Class functions */
struct tagCLASS;  /* opaque structure */
struct tagWND;
extern ATOM get_int_atom_value( LPCWSTR name );
extern void CLASS_RegisterBuiltinClasses(void);
extern void CLASS_AddWindow( struct tagCLASS *class, struct tagWND *win, BOOL unicode );
extern void CLASS_FreeModuleClasses( HMODULE16 hModule );

/* defwnd proc */
extern HBRUSH DEFWND_ControlColor( HDC hDC, UINT ctlType );

/* desktop */
extern BOOL DESKTOP_SetPattern( LPCWSTR pattern );

/* icon title */
extern HWND ICONTITLE_Create( HWND hwnd );

/* menu controls */
extern HWND MENU_IsMenuActive(void);
extern UINT MENU_GetMenuBarHeight( HWND hwnd, UINT menubarWidth,
                                     INT orgX, INT orgY );
extern BOOL MENU_SetMenu(HWND, HMENU);
extern void MENU_TrackMouseMenuBar( HWND hwnd, INT ht, POINT pt );
extern void MENU_TrackKbdMenuBar( HWND hwnd, UINT wParam, WCHAR wChar );
extern UINT MENU_DrawMenuBar( HDC hDC, LPRECT lprect,
                                HWND hwnd, BOOL suppress_draw );
extern UINT MENU_FindSubMenu( HMENU *hmenu, HMENU hSubTarget );

/* nonclient area */
extern LRESULT NC_HandleNCPaint( HWND hwnd , HRGN clip);
extern LRESULT NC_HandleNCActivate( HWND hwnd, WPARAM wParam );
extern LRESULT NC_HandleNCCalcSize( HWND hwnd, RECT *winRect );
extern LRESULT NC_HandleNCHitTest( HWND hwnd, POINT pt );
extern LRESULT NC_HandleNCLButtonDown( HWND hwnd, WPARAM wParam, LPARAM lParam );
extern LRESULT NC_HandleNCLButtonDblClk( HWND hwnd, WPARAM wParam, LPARAM lParam);
extern LRESULT NC_HandleSysCommand( HWND hwnd, WPARAM wParam, LPARAM lParam );
extern LRESULT NC_HandleSetCursor( HWND hwnd, WPARAM wParam, LPARAM lParam );
extern BOOL NC_DrawSysButton( HWND hwnd, HDC hdc, BOOL down );
extern void NC_GetSysPopupPos( HWND hwnd, RECT* rect );

/* scrollbar */
extern void SCROLL_DrawScrollBar( HWND hwnd, HDC hdc, INT nBar, BOOL arrows, BOOL interior );
extern void SCROLL_TrackScrollBar( HWND hwnd, INT scrollbar, POINT pt );
extern INT SCROLL_SetNCSbState( HWND hwnd, int vMin, int vMax, int vPos,
                                int hMin, int hMax, int hPos );

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
   INT            droppedWidth;   /* last two are not used unless set */
   INT            editHeight;     /* explicitly */
} HEADCOMBO,*LPHEADCOMBO;

extern BOOL COMBO_FlipListbox( LPHEADCOMBO, BOOL, BOOL );

/* Dialog info structure */
typedef struct
{
    HWND      hwndFocus;   /* Current control with focus */
    HFONT     hUserFont;   /* Dialog font */
    HMENU     hMenu;       /* Dialog menu */
    UINT      xBaseUnit;   /* Dialog units (depends on the font) */
    UINT      yBaseUnit;
    INT       idResult;    /* EndDialog() result / default pushbutton ID */
    UINT      flags;       /* EndDialog() called for this dialog */
    HGLOBAL16 hDialogHeap;
} DIALOGINFO;

#define DF_END  0x0001
#define DF_OWNERENABLED 0x0002

/* offset of DIALOGINFO ptr in dialog extra bytes */
#define DWLP_WINE_DIALOGINFO (DWLP_USER+sizeof(ULONG_PTR))

extern DIALOGINFO *DIALOG_get_info( HWND hwnd, BOOL create );
extern void DIALOG_EnableOwner( HWND hOwner );
extern BOOL DIALOG_DisableOwner( HWND hOwner );
extern INT DIALOG_DoDialogBox( HWND hwnd, HWND owner );

#endif  /* __WINE_CONTROLS_H */
