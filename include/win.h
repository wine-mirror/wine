/*
 * Window definitions
 *
 * Copyright 1993 Alexandre Julliard
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

#ifndef __WINE_WIN_H
#define __WINE_WIN_H

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/windef16.h"

#include "user.h"

#define WND_MAGIC     0x444e4957  /* 'WIND' */

struct tagCLASS;
struct tagDCE;
struct tagMESSAGEQUEUE;

typedef struct tagWND
{
    HWND           hwndSelf;      /* Handle of this window */
    HWND           parent;        /* Window parent */
    HWND           owner;         /* Window owner */
    struct tagCLASS *class;       /* Window class */
    WNDPROC        winproc;       /* Window procedure */
    DWORD          dwMagic;       /* Magic number (must be WND_MAGIC) */
    DWORD          tid;           /* Owner thread id */
    HINSTANCE      hInstance;     /* Window hInstance (from CreateWindow) */
    RECT           rectClient;    /* Client area rel. to parent client area */
    RECT           rectWindow;    /* Whole window rel. to parent client area */
    LPWSTR         text;          /* Window text */
    void          *pVScroll;      /* Vertical scroll-bar info */
    void          *pHScroll;      /* Horizontal scroll-bar info */
    struct tagDCE *dce;           /* Window DCE (if CS_OWNDC or CS_CLASSDC) */
    HRGN           hrgnUpdate;    /* Update region */
    HRGN           hrgnWnd;       /* window's region */
    DWORD          dwStyle;       /* Window style (from CreateWindow) */
    DWORD          dwExStyle;     /* Extended style (from CreateWindowEx) */
    DWORD          clsStyle;      /* Class style at window creation */
    UINT           wIDmenu;       /* ID or hmenu (from CreateWindow) */
    DWORD          helpContext;   /* Help context ID */
    UINT           flags;         /* Misc. flags (see below) */
    HMENU          hSysMenu;      /* window's copy of System Menu */
    int            cbWndExtra;    /* class cbWndExtra at window creation */
    int            irefCount;     /* window's reference count*/
    DWORD          userdata;      /* User private data */
    void          *pDriverData;   /* Window driver data */
    DWORD          wExtra[1];     /* Window extra bytes */
} WND;

typedef struct
{
    RECT16	   rectNormal;
    POINT16	   ptIconPos;
    POINT16	   ptMaxPos;
    HWND           hwndIconTitle;
} INTERNALPOS, *LPINTERNALPOS;

  /* WND flags values */
#define WIN_NEEDS_BEGINPAINT   0x0001 /* WM_PAINT sent to window */
#define WIN_NEEDS_ERASEBKGND   0x0002 /* WM_ERASEBKGND must be sent to window*/
#define WIN_NEEDS_NCPAINT      0x0004 /* WM_NCPAINT must be sent to window */
#define WIN_RESTORE_MAX        0x0008 /* Maximize when restoring */
#define WIN_INTERNAL_PAINT     0x0010 /* Internal WM_PAINT message pending */
#define WIN_NEED_SIZE          0x0040 /* Internal WM_SIZE is needed */
#define WIN_NCACTIVATED        0x0080 /* last WM_NCACTIVATE was positive */
#define WIN_ISDIALOG           0x0200 /* Window is a dialog */
#define WIN_ISWIN32            0x0400 /* Understands Win32 messages */
#define WIN_NEEDS_SHOW_OWNEDPOPUP 0x0800 /* WM_SHOWWINDOW:SC_SHOW must be sent in the next ShowOwnedPopup call */
#define WIN_NEEDS_INTERNALSOP  0x1000 /* Window was hidden by WIN_InternalShowOwnedPopups */

  /* Window functions */
extern WND *WIN_GetPtr( HWND hwnd );
extern int    WIN_SuspendWndsLock( void );
extern void   WIN_RestoreWndsLock(int ipreviousLock);
extern WND*   WIN_FindWndPtr( HWND hwnd );
extern void   WIN_ReleaseWndPtr(WND *wndPtr);
extern HWND WIN_Handle32( HWND16 hwnd16 );
extern HWND WIN_IsCurrentProcess( HWND hwnd );
extern HWND WIN_IsCurrentThread( HWND hwnd );
extern void WIN_LinkWindow( HWND hwnd, HWND parent, HWND hwndInsertAfter );
extern void WIN_UnlinkWindow( HWND hwnd );
extern HWND WIN_SetOwner( HWND hwnd, HWND owner );
extern LONG WIN_SetStyle( HWND hwnd, LONG style );
extern LONG WIN_SetExStyle( HWND hwnd, LONG style );
extern void WIN_SetRectangles( HWND hwnd, const RECT *rectWindow, const RECT *rectClient );
extern BOOL WIN_GetRectangles( HWND hwnd, RECT *rectWindow, RECT *rectClient );
extern LRESULT WIN_DestroyWindow( HWND hwnd );
extern void WIN_DestroyThreadWindows( HWND hwnd );
extern BOOL WIN_CreateDesktopWindow(void);
extern BOOL WIN_IsWindowDrawable( HWND hwnd, BOOL );
extern HWND *WIN_ListParents( HWND hwnd );
extern HWND *WIN_ListChildren( HWND hwnd );
extern BOOL WIN_InternalShowOwnedPopups( HWND owner, BOOL fShow, BOOL unmanagedOnly );

inline static HWND WIN_GetFullHandle( HWND hwnd )
{
    if (!HIWORD(hwnd) && hwnd) hwnd = WIN_Handle32( LOWORD(hwnd) );
    return hwnd;
}

inline static WND *WIN_FindWndPtr16( HWND16 hwnd )
{
    /* don't bother with full conversion */
    return WIN_FindWndPtr( (HWND)(ULONG_PTR)hwnd );
}

/* to release pointers retrieved by WIN_GetPtr; do not confuse with WIN_ReleaseWndPtr!! */
inline static void WIN_ReleasePtr( WND *ptr )
{
    USER_Unlock();
}

#define WND_OTHER_PROCESS ((WND *)1)  /* returned by WIN_GetPtr on unknown window handles */

extern HBRUSH DEFWND_ControlColor( HDC hDC, UINT ctlType );  /* windows/defwnd.c */

extern BOOL FOCUS_MouseActivate( HWND hwnd );

#endif  /* __WINE_WIN_H */
