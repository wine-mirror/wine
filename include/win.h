/*
 * Window definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef WIN_H
#define WIN_H

#include <X11/Xlib.h>

#include "ldt.h"
#include "class.h"

#define WND_MAGIC     0x444e4957  /* 'WIND' */

  /* Built-in class names (see _Undocumented_Windows_ p.418) */
#define POPUPMENU_CLASS_NAME "#32768"  /* PopupMenu */
#define DESKTOP_CLASS_NAME   "#32769"  /* Desktop */
#define DIALOG_CLASS_NAME    "#32770"  /* Dialog */
#define WINSWITCH_CLASS_NAME "#32771"  /* WinSwitch */
#define ICONTITLE_CLASS_NAME "#32772"  /* IconTitle */

#define POPUPMENU_CLASS_ATOM MAKEINTATOM(32768)  /* PopupMenu */
#define DESKTOP_CLASS_ATOM   MAKEINTATOM(32769)  /* Desktop */
#define DIALOG_CLASS_ATOM    MAKEINTATOM(32770)  /* Dialog */
#define WINSWITCH_CLASS_ATOM MAKEINTATOM(32771)  /* WinSwitch */
#define ICONTITLE_CLASS_ATOM MAKEINTATOM(32772)  /* IconTitle */

typedef struct tagWND
{
    struct tagWND *next;         /* Next sibling */
    struct tagWND *child;        /* First child */
    struct tagWND *parent;       /* Window parent (from CreateWindow) */
    struct tagWND *owner;        /* Window owner */
    DWORD        dwMagic;        /* Magic number (must be WND_MAGIC) */
    HWND         hwndSelf;       /* Handle of this window */
    HCLASS       hClass;         /* Window class */
    HANDLE       hInstance;      /* Window hInstance (from CreateWindow) */
    RECT         rectClient;     /* Client area rel. to parent client area */
    RECT         rectWindow;     /* Whole window rel. to parent client area */
    RECT         rectNormal;     /* Window rect. when in normal state */
    POINT        ptIconPos;      /* Icon position */
    POINT        ptMaxPos;       /* Maximized window position */
    HGLOBAL      hmemTaskQ;      /* Task queue global memory handle */
    HRGN         hrgnUpdate;     /* Update region */
    HWND         hwndLastActive; /* Last active popup hwnd */
    WNDPROC      lpfnWndProc;    /* Window procedure */
    DWORD        dwStyle;        /* Window style (from CreateWindow) */
    DWORD        dwExStyle;      /* Extended style (from CreateWindowEx) */
    HANDLE       hdce;           /* Window DCE (if CS_OWNDC or CS_CLASSDC) */
    HANDLE       hVScroll;       /* Vertical scroll-bar info */
    HANDLE       hHScroll;       /* Horizontal scroll-bar info */
    UINT         wIDmenu;        /* ID or hmenu (from CreateWindow) */
    HANDLE       hText;          /* Handle of window text */
    WORD         flags;          /* Misc. flags (see below) */
    Window       window;         /* X window (only for top-level windows) */
    HMENU        hSysMenu;	 /* window's copy of System Menu */
    HANDLE       hProp;          /* Handle of Properties List */
    WORD         wExtra[1];      /* Window extra bytes */
} WND;

  /* WND flags values */
#define WIN_NEEDS_BEGINPAINT   0x0001 /* WM_PAINT sent to window */
#define WIN_NEEDS_ERASEBKGND   0x0002 /* WM_ERASEBKGND must be sent to window*/
#define WIN_NEEDS_NCPAINT      0x0004 /* WM_NCPAINT must be sent to window */
#define WIN_RESTORE_MAX        0x0008 /* Maximize when restoring */
#define WIN_INTERNAL_PAINT     0x0010 /* Internal WM_PAINT message pending */
#define WIN_NO_REDRAW          0x0020 /* WM_SETREDRAW called for this window */
#define WIN_GOT_SIZEMSG        0x0040 /* WM_SIZE has been sent to the window */
#define WIN_NCACTIVATED        0x0080 /* last WM_NCACTIVATE was positive */
#define WIN_MANAGED            0x0100 /* Window managed by the X wm */

#define WIN_CLASS_INFO(wndPtr)   (CLASS_FindClassPtr((wndPtr)->hClass)->wc)
#define WIN_CLASS_STYLE(wndPtr)  (WIN_CLASS_INFO(wndPtr).style)

  /* Window functions */
extern WND *WIN_FindWndPtr( HWND hwnd );
extern WND *WIN_GetDesktop(void);
extern void WIN_DumpWindow( HWND hwnd );
extern void WIN_WalkWindows( HWND hwnd, int indent );
extern Window WIN_GetXWindow( HWND hwnd );
extern BOOL WIN_UnlinkWindow( HWND hwnd );
extern BOOL WIN_LinkWindow( HWND hwnd, HWND hwndInsertAfter );
extern HWND WIN_FindWinToRepaint( HWND hwnd, HQUEUE hQueue );
extern void WIN_SendParentNotify( HWND hwnd, WORD event,
                                  WORD idChild, LONG lValue );
extern BOOL WIN_CreateDesktopWindow(void);
extern HWND WIN_GetTopParent( HWND hwnd );
extern HINSTANCE WIN_GetWindowInstance( HWND hwnd );

extern Display * display;
extern Screen * screen;
extern Window rootWindow;

#endif  /* WIN_H */
