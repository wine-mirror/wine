/*
 * Window definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_WIN_H
#define __WINE_WIN_H

#include "config.h"

#ifndef X_DISPLAY_MISSING
#include <X11/Xlib.h>
#endif /* !defined(X_DISPLAY_MISSING) */

#include "ldt.h"
#include "class.h"
#include "winproc.h"

#define WND_MAGIC     0x444e4957  /* 'WIND' */

  /* Built-in class names (see _Undocumented_Windows_ p.418) */
#define POPUPMENU_CLASS_NAME "#32768"  /* PopupMenu */
#define DESKTOP_CLASS_NAME   "#32769"  /* Desktop */
#define DIALOG_CLASS_NAME    "#32770"  /* Dialog */
#define WINSWITCH_CLASS_NAME "#32771"  /* WinSwitch */
#define ICONTITLE_CLASS_NAME "#32772"  /* IconTitle */

#define POPUPMENU_CLASS_ATOM MAKEINTATOM(32768)  /* PopupMenu */
#define DESKTOP_CLASS_ATOM   ((ATOM)32769)       /* Desktop */
#define DIALOG_CLASS_ATOM    MAKEINTATOM(32770)  /* Dialog */
#define WINSWITCH_CLASS_ATOM MAKEINTATOM(32771)  /* WinSwitch */
#define ICONTITLE_CLASS_ATOM MAKEINTATOM(32772)  /* IconTitle */

/* Built-in 32-bit classes */
typedef enum
{
    BIC32_BUTTON,
    BIC32_EDIT,
    BIC32_LISTBOX,
    BIC32_COMBO,
    BIC32_COMBOLB,
    BIC32_POPUPMENU,
    BIC32_STATIC,
    BIC32_SCROLL,
    BIC32_MDICLIENT,
    BIC32_DESKTOP,
    BIC32_DIALOG,
    BIC32_ICONTITLE,
    BIC32_NB_CLASSES
} BUILTIN_CLASS32;

  /* PAINT_RedrawWindow() control flags */
#define RDW_C_USEHRGN		0x0001
#define RDW_C_DELETEHRGN	0x0002

struct tagDCE;
struct tagDC;
struct _WND_DRIVER;

typedef struct tagWND
{
    struct tagWND *next;          /* Next sibling */
    struct tagWND *child;         /* First child */
    struct tagWND *parent;        /* Window parent (from CreateWindow) */
    struct tagWND *owner;         /* Window owner */
    CLASS         *class;         /* Window class */
    HWINDOWPROC    winproc;       /* Window procedure */
    DWORD          dwMagic;       /* Magic number (must be WND_MAGIC) */
    HWND32         hwndSelf;      /* Handle of this window */
    HINSTANCE32    hInstance;     /* Window hInstance (from CreateWindow) */
    RECT32         rectClient;    /* Client area rel. to parent client area */
    RECT32         rectWindow;    /* Whole window rel. to parent client area */
    LPSTR          text;          /* Window text */
    void          *pVScroll;      /* Vertical scroll-bar info */
    void          *pHScroll;      /* Horizontal scroll-bar info */
    void          *pProp;         /* Pointer to properties list */
    struct tagDCE *dce;           /* Window DCE (if CS_OWNDC or CS_CLASSDC) */
    HGLOBAL16      hmemTaskQ;     /* Task queue global memory handle */
    HRGN16         hrgnUpdate;    /* Update region */
    HWND32         hwndLastActive;/* Last active popup hwnd */
    DWORD          dwStyle;       /* Window style (from CreateWindow) */
    DWORD          dwExStyle;     /* Extended style (from CreateWindowEx) */
    UINT32         wIDmenu;       /* ID or hmenu (from CreateWindow) */
    DWORD          helpContext;   /* Help context ID */
    WORD           flags;         /* Misc. flags (see below) */
#if 0
    Window         window;        /* X window (only for top-level windows) */
#endif
    HMENU16        hSysMenu;      /* window's copy of System Menu */
    DWORD          userdata;      /* User private data */
    struct _WND_DRIVER *pDriver;  /* Window driver */
    void          *pDriverData;   /* Window driver data */
    DWORD          wExtra[1];     /* Window extra bytes */
} WND;

typedef struct _WND_DRIVER
{
    void   (*pInitialize)(WND *);
    void   (*pFinalize)(WND *);
    BOOL32 (*pCreateDesktopWindow)(WND *, CLASS *, BOOL32);
    BOOL32 (*pCreateWindow)(WND *, CLASS *, CREATESTRUCT32A *, BOOL32);
    BOOL32 (*pDestroyWindow)(WND *);
    WND*   (*pSetParent)(WND *, WND *);
    void   (*pForceWindowRaise)(WND *);
    void   (*pSetWindowPos)(WND *, const WINDOWPOS32 *, BOOL32);
    void   (*pSetText)(WND *, LPCSTR);
    void   (*pSetFocus)(WND *);
    void   (*pPreSizeMove)(WND *);
    void   (*pPostSizeMove)(WND *);
    void   (*pScrollWindow)(WND *, struct tagDC *, INT32, INT32, const RECT32 *, BOOL32);
    void   (*pSetDrawable)(WND *, struct tagDC *, WORD, BOOL32);
    BOOL32 (*pIsSelfClipping)(WND *);
} WND_DRIVER;

typedef struct
{
    RECT16	   rectNormal;
    POINT16	   ptIconPos;
    POINT16	   ptMaxPos;
    HWND16	   hwndIconTitle;
} INTERNALPOS, *LPINTERNALPOS;

  /* WND flags values */
#define WIN_NEEDS_BEGINPAINT   0x0001 /* WM_PAINT sent to window */
#define WIN_NEEDS_ERASEBKGND   0x0002 /* WM_ERASEBKGND must be sent to window*/
#define WIN_NEEDS_NCPAINT      0x0004 /* WM_NCPAINT must be sent to window */
#define WIN_RESTORE_MAX        0x0008 /* Maximize when restoring */
#define WIN_INTERNAL_PAINT     0x0010 /* Internal WM_PAINT message pending */
/* Used to have WIN_NO_REDRAW  0x0020 here */
#define WIN_NEED_SIZE          0x0040 /* Internal WM_SIZE is needed */
#define WIN_NCACTIVATED        0x0080 /* last WM_NCACTIVATE was positive */
#define WIN_MANAGED            0x0100 /* Window managed by the X wm */
#define WIN_ISDIALOG           0x0200 /* Window is a dialog */
#define WIN_ISWIN32            0x0400 /* Understands Win32 messages */
#define WIN_SAVEUNDER_OVERRIDE 0x0800

  /* BuildWinArray() flags */
#define BWA_SKIPDISABLED	0x0001
#define BWA_SKIPHIDDEN		0x0002
#define BWA_SKIPOWNED		0x0004
#define BWA_SKIPICONIC		0x0008

  /* Window functions */
extern WND*   WIN_FindWndPtr( HWND32 hwnd );
extern WND*   WIN_GetDesktop(void);
extern void   WIN_DumpWindow( HWND32 hwnd );
extern void   WIN_WalkWindows( HWND32 hwnd, int indent );
extern BOOL32 WIN_UnlinkWindow( HWND32 hwnd );
extern BOOL32 WIN_LinkWindow( HWND32 hwnd, HWND32 hwndInsertAfter );
extern HWND32 WIN_FindWinToRepaint( HWND32 hwnd, HQUEUE16 hQueue );
extern BOOL32 WIN_ResetQueueWindows( WND* wnd, HQUEUE16 hQueue, HQUEUE16 hNew);
extern BOOL32 WIN_CreateDesktopWindow(void);
extern HWND32 WIN_GetTopParent( HWND32 hwnd );
extern WND*   WIN_GetTopParentPtr( WND* pWnd );
extern BOOL32 WIN_IsWindowDrawable(WND*, BOOL32 );
extern HINSTANCE32 WIN_GetWindowInstance( HWND32 hwnd );
extern WND**  WIN_BuildWinArray( WND *wndPtr, UINT32 bwa, UINT32* pnum );

extern HWND32 CARET_GetHwnd(void);
extern void CARET_GetRect(LPRECT32 lprc);  /* windows/caret.c */

extern BOOL16 DRAG_QueryUpdate( HWND32, SEGPTR, BOOL32 );
extern void DEFWND_SetText( WND *wndPtr, LPCSTR text );
extern HBRUSH32 DEFWND_ControlColor( HDC32 hDC, UINT16 ctlType );     /* windows/defwnd.c */

extern void PROPERTY_RemoveWindowProps( WND *pWnd );  		      /* windows/property.c */

extern BOOL32 PAINT_RedrawWindow( HWND32 hwnd, const RECT32 *rectUpdate,
                                  HRGN32 hrgnUpdate, UINT32 flags,
                                  UINT32 control );		      /* windows/painting.c */

/* controls/widgets.c */
extern BOOL32 WIDGETS_Init( void );
extern BOOL32 WIDGETS_IsControl32( WND* pWnd, BUILTIN_CLASS32 cls );  

/* controls/icontitle.c */
extern HWND32 ICONTITLE_Create( WND* );
extern BOOL32 ICONTITLE_Init( void );

/* windows/focus.c */
extern void FOCUS_SwitchFocus( HWND32 , HWND32 );

extern Display * display;
extern Screen * screen;
extern Window rootWindow;

#endif  /* __WINE_WIN_H */
