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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_CONTROLS_H
#define __WINE_CONTROLS_H

#include "winuser.h"
#include "winproc.h"

/* Built-in class names (see _Undocumented_Windows_ p.418) */
#define POPUPMENU_CLASS_ATOM MAKEINTATOMA(32768)  /* PopupMenu */
#define DESKTOP_CLASS_ATOM   MAKEINTATOMA(32769)  /* Desktop */
#define DIALOG_CLASS_ATOMA   MAKEINTATOMA(32770)  /* Dialog */
#define DIALOG_CLASS_ATOMW   MAKEINTATOMW(32770)  /* Dialog */
#define WINSWITCH_CLASS_ATOM MAKEINTATOMA(32771)  /* WinSwitch */
#define ICONTITLE_CLASS_ATOM MAKEINTATOMA(32772)  /* IconTitle */

/* Built-in class descriptor */
struct builtin_class_descr
{
    LPCSTR  name;    /* class name */
    UINT    style;   /* class style */
    WNDPROC procA;   /* ASCII window procedure */
    WNDPROC procW;   /* Unicode window procedure */
    INT     extra;   /* window extra bytes */
    LPCSTR  cursor;  /* cursor name */
    HBRUSH  brush;   /* brush or system color */
};


/* desktop */
extern BOOL DESKTOP_SetPattern( LPCSTR pattern );

/* icon title */
extern HWND ICONTITLE_Create( HWND hwnd );

/* menu controls */
extern BOOL MENU_Init(void);
extern BOOL MENU_IsMenuActive(void);
extern HMENU MENU_GetSysMenu(HWND hWndOwner, HMENU hSysPopup);
extern UINT MENU_GetMenuBarHeight( HWND hwnd, UINT menubarWidth,
                                     INT orgX, INT orgY );
extern void MENU_TrackMouseMenuBar( HWND hwnd, INT ht, POINT pt );
extern void MENU_TrackKbdMenuBar( HWND hwnd, UINT wParam, INT vkey );
extern UINT MENU_DrawMenuBar( HDC hDC, LPRECT lprect,
                                HWND hwnd, BOOL suppress_draw );
extern UINT MENU_FindSubMenu( HMENU *hmenu, HMENU hSubTarget );

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

/* Note, that CBS_DROPDOWNLIST style is actually (CBS_SIMPLE | CBS_DROPDOWN) */
#define CB_GETTYPE( lphc )    ((lphc)->dwStyle & (CBS_DROPDOWNLIST))

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
#define DWL_WINE_DIALOGINFO (DWL_USER+sizeof(ULONG_PTR))

inline static DIALOGINFO *DIALOG_get_info( HWND hwnd )
{
    return (DIALOGINFO *)GetWindowLongW( hwnd, DWL_WINE_DIALOGINFO );
}

extern BOOL DIALOG_Init(void);
extern BOOL DIALOG_GetCharSize( HFONT hFont, SIZE * pSize );
extern void DIALOG_EnableOwner( HWND hOwner );
extern BOOL DIALOG_DisableOwner( HWND hOwner );
extern INT DIALOG_DoDialogBox( HWND hwnd, HWND owner );

#endif  /* __WINE_CONTROLS_H */
