/*
 * Window definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_WIN_H
#define __WINE_WIN_H

#include "queue.h"
#include "class.h"

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
#define RDW_EX_USEHRGN		0x0001
#define RDW_EX_DELETEHRGN	0x0002
#define RDW_EX_XYWINDOW		0x0004
#define RDW_EX_TOPFRAME		0x0010

struct tagCLASS;
struct tagDCE;
struct tagDC;
struct _WND_DRIVER;

typedef struct tagWND
{
    struct tagWND *next;          /* Next sibling */
    struct tagWND *child;         /* First child */
    struct tagWND *parent;        /* Window parent (from CreateWindow) */
    struct tagWND *owner;         /* Window owner */
    struct tagCLASS *class;       /* Window class */
    HWINDOWPROC    winproc;       /* Window procedure */
    DWORD          dwMagic;       /* Magic number (must be WND_MAGIC) */
    HWND         hwndSelf;      /* Handle of this window */
    HINSTANCE    hInstance;     /* Window hInstance (from CreateWindow) */
    RECT         rectClient;    /* Client area rel. to parent client area */
    RECT         rectWindow;    /* Whole window rel. to parent client area */
    LPSTR          text;          /* Window text */
    void          *pVScroll;      /* Vertical scroll-bar info */
    void          *pHScroll;      /* Horizontal scroll-bar info */
    void          *pProp;         /* Pointer to properties list */
    struct tagDCE *dce;           /* Window DCE (if CS_OWNDC or CS_CLASSDC) */
    HGLOBAL16      hmemTaskQ;     /* Task queue global memory handle */
    HRGN16         hrgnUpdate;    /* Update region */
    HWND         hwndLastActive;/* Last active popup hwnd */
    DWORD          dwStyle;       /* Window style (from CreateWindow) */
    DWORD          dwExStyle;     /* Extended style (from CreateWindowEx) */
    UINT         wIDmenu;       /* ID or hmenu (from CreateWindow) */
    DWORD          helpContext;   /* Help context ID */
    WORD           flags;         /* Misc. flags (see below) */
    HMENU16        hSysMenu;      /* window's copy of System Menu */
    int            irefCount;     /* window's reference count*/
    DWORD          userdata;      /* User private data */
    struct _WND_DRIVER *pDriver;  /* Window driver */
    void          *pDriverData;   /* Window driver data */
    DWORD          wExtra[1];     /* Window extra bytes */
} WND;

/* Host attributes */

#define HAK_BITGRAVITY     1
#define HAK_ACCEPTFOCUS	   2

/* Bit Gravity */

#define BGForget           0
#define BGNorthWest        1
#define BGNorth            2
#define BGNorthEast        3
#define BGWest             4
#define BGCenter           5
#define BGEast             6
#define BGSouthWest        7
#define BGSouth            8
#define BGSouthEast        9
#define BGStatic           10

typedef struct _WND_DRIVER
{
    void   (*pInitialize)(WND *);
    void   (*pFinalize)(WND *);
    BOOL (*pCreateDesktopWindow)(WND *, struct tagCLASS *, BOOL);
    BOOL (*pCreateWindow)(WND *, struct tagCLASS *, CREATESTRUCTA *, BOOL);
    BOOL (*pDestroyWindow)(WND *);
    WND*   (*pSetParent)(WND *, WND *);
    void   (*pForceWindowRaise)(WND *);
    void   (*pSetWindowPos)(WND *, const WINDOWPOS *, BOOL);
    void   (*pSetText)(WND *, LPCSTR);
    void   (*pSetFocus)(WND *);
    void   (*pPreSizeMove)(WND *);
    void   (*pPostSizeMove)(WND *);
    void   (*pSurfaceCopy)(WND *, struct tagDC *, INT, INT, const RECT *, BOOL);
    void   (*pSetDrawable)(WND *, struct tagDC *, WORD, BOOL);
    BOOL   (*pSetHostAttr)(WND *, INT haKey, INT value);
    BOOL (*pIsSelfClipping)(WND *);
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
#define WIN_NATIVE	       0x0020 /* Directly mapped to the window provided by the driver */
#define WIN_NEED_SIZE          0x0040 /* Internal WM_SIZE is needed */
#define WIN_NCACTIVATED        0x0080 /* last WM_NCACTIVATE was positive */
#define WIN_MANAGED            0x0100 /* Window managed by the window system */
#define WIN_ISDIALOG           0x0200 /* Window is a dialog */
#define WIN_ISWIN32            0x0400 /* Understands Win32 messages */

  /* BuildWinArray() flags */
#define BWA_SKIPDISABLED	0x0001
#define BWA_SKIPHIDDEN		0x0002
#define BWA_SKIPOWNED		0x0004
#define BWA_SKIPICONIC		0x0008

  /* Window functions */
extern void   WIN_LockWnds();
extern void   WIN_UnlockWnds();
extern int    WIN_SuspendWndsLock();
extern void   WIN_RestoreWndsLock(int ipreviousLock);
extern WND*   WIN_FindWndPtr( HWND hwnd );
extern WND*   WIN_LockWndPtr(WND *wndPtr);
extern void   WIN_ReleaseWndPtr(WND *wndPtr);
extern void   WIN_UpdateWndPtr(WND **oldPtr,WND *newPtr);
extern WND*   WIN_GetDesktop(void);
extern void   WIN_ReleaseDesktop(void);
extern void   WIN_DumpWindow( HWND hwnd );
extern void   WIN_WalkWindows( HWND hwnd, int indent );
extern BOOL WIN_UnlinkWindow( HWND hwnd );
extern BOOL WIN_LinkWindow( HWND hwnd, HWND hwndInsertAfter );
extern HWND WIN_FindWinToRepaint( HWND hwnd, HQUEUE16 hQueue );
extern BOOL WIN_ResetQueueWindows( WND* wnd, HQUEUE16 hQueue, HQUEUE16 hNew);
extern BOOL WIN_CreateDesktopWindow(void);
extern HWND WIN_GetTopParent( HWND hwnd );
extern WND*   WIN_GetTopParentPtr( WND* pWnd );
extern BOOL WIN_IsWindowDrawable(WND*, BOOL );
extern HINSTANCE WIN_GetWindowInstance( HWND hwnd );
extern WND**  WIN_BuildWinArray( WND *wndPtr, UINT bwa, UINT* pnum );
extern void   WIN_ReleaseWinArray(WND **wndArray);

extern HWND CARET_GetHwnd(void);
extern void CARET_GetRect(LPRECT lprc);  /* windows/caret.c */

extern BOOL16 DRAG_QueryUpdate( HWND, SEGPTR, BOOL );
extern void DEFWND_SetText( WND *wndPtr, LPCSTR text );
extern HBRUSH DEFWND_ControlColor( HDC hDC, UINT16 ctlType );     /* windows/defwnd.c */

extern void PROPERTY_RemoveWindowProps( WND *pWnd );  		      /* windows/property.c */

extern BOOL PAINT_RedrawWindow( HWND hwnd, const RECT *rectUpdate,
                                  HRGN hrgnUpdate, UINT flags,
                                  UINT control );		      /* windows/painting.c */
extern HRGN WIN_UpdateNCRgn(WND* wnd, BOOL bUpdate, BOOL bForce);     /* windows/painting.c */


/* controls/widgets.c */
extern BOOL WIDGETS_Init( void );
extern BOOL WIDGETS_IsControl( WND* pWnd, BUILTIN_CLASS32 cls );  

/* controls/icontitle.c */
extern HWND ICONTITLE_Create( WND* );
extern BOOL ICONTITLE_Init( void );

/* windows/focus.c */
extern void FOCUS_SwitchFocus( MESSAGEQUEUE *pMsgQ, HWND , HWND );

#endif  /* __WINE_WIN_H */
