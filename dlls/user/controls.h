/*
 * User controls definitions
 *
 * Copyright 2000 Alexandre Julliard
 */

#ifndef __WINE_CONTROLS_H
#define __WINE_CONTROLS_H

#include "winuser.h"
#include "winproc.h"

/* Built-in class names (see _Undocumented_Windows_ p.418) */
#define POPUPMENU_CLASS_ATOM MAKEINTATOM(32768)  /* PopupMenu */
#define DESKTOP_CLASS_ATOM   MAKEINTATOM(32769)  /* Desktop */
#define DIALOG_CLASS_ATOM    MAKEINTATOM(32770)  /* Dialog */
#define WINSWITCH_CLASS_ATOM MAKEINTATOM(32771)  /* WinSwitch */
#define ICONTITLE_CLASS_ATOM MAKEINTATOM(32772)  /* IconTitle */

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
extern void SCROLL_HandleScrollEvent( HWND hwnd, INT nBar, UINT msg, POINT pt );
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

/* Dialog info structure.
 * This structure is stored into the window extra bytes (cbWndExtra).
 * sizeof(DIALOGINFO) must be <= DLGWINDOWEXTRA (=30).
 */
#include "pshpack1.h"

typedef struct
{
    INT         msgResult;   /* 00 Last message result */
    HWINDOWPROC dlgProc;     /* 04 Dialog procedure */
    LONG        userInfo;    /* 08 User information (for DWL_USER) */

    /* implementation-dependent part */

    HWND16      hwndFocus;   /* 0c Current control with focus */
    HFONT16     hUserFont;   /* 0e Dialog font */
    HMENU16     hMenu;       /* 10 Dialog menu */
    WORD        xBaseUnit;   /* 12 Dialog units (depends on the font) */
    WORD        yBaseUnit;   /* 14 */
    INT         idResult;    /* 16 EndDialog() result / default pushbutton ID */
    UINT16      flags;       /* 1a EndDialog() called for this dialog */
    HGLOBAL16   hDialogHeap; /* 1c */
} DIALOGINFO;

#include "poppack.h"

#define DF_END  0x0001
#define DF_OWNERENABLED 0x0002

extern BOOL DIALOG_Init(void);

#endif  /* __WINE_CONTROLS_H */
