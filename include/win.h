/*
 * Window definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef WIN_H
#define WIN_H

#include <X11/Xlib.h>

#include "windows.h"
#include "menu.h"

#define WND_MAGIC     0x444e4957  /* 'WIND' */


typedef struct tagWND
{
    HWND         hwndNext;       /* Next sibling */
    HWND         hwndChild;      /* First child */
    DWORD        dwMagic;        /* Magic number (must be WND_MAGIC) */
    HWND         hwndParent;     /* Window parent (from CreateWindow) */
    HWND         hwndOwner;      /* Window owner */
    HCLASS       hClass;         /* Window class */
    HANDLE       hInstance;      /* Window hInstance (from CreateWindow) */
    RECT         rectClient;     /* Client area rel. to parent client area */
    RECT         rectWindow;     /* Whole window rel. to parent client area */
    RECT         rectNormal;     /* Window rect. when in normal state */
    POINT        ptIconPos;      /* Icon position */
    POINT        ptMaxPos;       /* Maximized window position */
    HANDLE       hmemTaskQ;      /* Task queue global memory handle */
    HRGN         hrgnUpdate;     /* Update region */
    HWND         hwndLastActive; /* Last active popup hwnd */
    FARPROC      lpfnWndProc;    /* Window procedure */
    DWORD        dwStyle;        /* Window style (from CreateWindow) */
    DWORD        dwExStyle;      /* Extended style (from CreateWindowEx) */
    HANDLE       hdce;           /* Window DCE (if CS_OWNDC or CS_CLASSDC) */
    HMENU        hmenuSystem;    /* System menu */
    HCURSOR      hCursor;    	 /* Window Current Cursor */
    void	 *VScroll;	 /* Vertical ScrollBar Struct Pointer */
    void	 *HScroll;	 /* Horizontal ScrollBar Struct Pointer */
    WORD         wIDmenu;        /* ID or hmenu (from CreateWindow) */
    HANDLE       hText;          /* Handle of window text */
    WORD         flags;          /* Misc. flags */
    Window       window;         /* X window */
    LPMENUBAR	 menuBarPtr;	 /* Menu bar */
    HMENU	 hSysMenu;	 /* window's copy of System Menu */
    HWND	 hWndMenuBar;	 /* Menu bar */
    WORD         wExtra[1];      /* Window extra bytes */
} WND;

  /* WND flags values */
#define WIN_ERASE_UPDATERGN     0x01  /* Update region needs erasing */
#define WIN_NEEDS_BEGINPAINT    0x02  /* WM_PAINT sent to window */
#define WIN_GOT_SIZEMSG         0x04  /* WM_SIZE has been sent to the window */
#define WIN_OWN_DC              0x08  /* Win class has style CS_OWNDC */
#define WIN_CLASS_DC            0x10  /* Win class has style CS_CLASSDC */
#define WIN_DOUBLE_CLICKS       0x20  /* Win class has style CS_DBLCLKS */
#define WIN_RESTORE_MAX         0x40  /* Maximize when restoring */

  /* First top-level window */
extern HWND firstWindow;

  /* Window functions */
WND *WIN_FindWndPtr( HWND hwnd );
BOOL WIN_UnlinkWindow( HWND hwnd );
BOOL WIN_LinkWindow( HWND hwnd, HWND hwndInsertAfter );
HWND WIN_FindWinToRepaint( HWND hwnd );


#endif  /* WIN_H */
