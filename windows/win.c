/*
 * Window related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "options.h"
#include "class.h"
#include "win.h"
#include "heap.h"
#include "user.h"
#include "dce.h"
#include "sysmetrics.h"
#include "cursoricon.h"
#include "heap.h"
#include "hook.h"
#include "menu.h"
#include "message.h"
#include "nonclient.h"
#include "string32.h"
#include "queue.h"
#include "winpos.h"
#include "color.h"
#include "shm_main_blk.h"
#include "dde_proc.h"
#include "callback.h"
#include "winproc.h"
#include "stddebug.h"
/* #define DEBUG_WIN  */ 
/* #define DEBUG_MENU */
#include "debug.h"

/* Desktop window */
static WND *pWndDesktop = NULL;

static HWND hwndSysModal = 0;

static WORD wDragWidth = 4;
static WORD wDragHeight= 3;

extern HCURSOR CURSORICON_IconToCursor(HICON);
extern HQUEUE  QUEUE_GetDoomedQueue();

/***********************************************************************
 *           WIN_FindWndPtr
 *
 * Return a pointer to the WND structure corresponding to a HWND.
 */
WND * WIN_FindWndPtr( HWND hwnd )
{
    WND * ptr;
    
    if (!hwnd) return NULL;
    ptr = (WND *) USER_HEAP_LIN_ADDR( hwnd );
    if (ptr->dwMagic != WND_MAGIC) return NULL;
    if (ptr->hwndSelf != hwnd)
    {
        fprintf( stderr, "Can't happen: hwnd %04x self pointer is %04x\n",
                 hwnd, ptr->hwndSelf );
        return NULL;
    }
    return ptr;
}


/***********************************************************************
 *           WIN_DumpWindow
 *
 * Dump the content of a window structure to stderr.
 */
void WIN_DumpWindow( HWND hwnd )
{
    WND *ptr;
    char className[80];
    int i;

    if (!(ptr = WIN_FindWndPtr( hwnd )))
    {
        fprintf( stderr, "%04x is not a window handle\n", hwnd );
        return;
    }

    if (!GetClassName32A( hwnd, className, sizeof(className ) ))
        strcpy( className, "#NULL#" );

    fprintf( stderr, "Window %04x (%p):\n", hwnd, ptr );
    fprintf( stderr,
             "next=%p  child=%p  parent=%p  owner=%p  class=%p '%s'\n"
             "inst=%04x  taskQ=%04x  updRgn=%04x  active=%04x hdce=%04x  idmenu=%04x\n"
             "style=%08lx  exstyle=%08lx  wndproc=%08lx  text='%s'\n"
             "client=%d,%d-%d,%d  window=%d,%d-%d,%d  iconpos=%d,%d  maxpos=%d,%d\n"
             "sysmenu=%04x  flags=%04x  props=%04x  vscroll=%04x  hscroll=%04x\n",
             ptr->next, ptr->child, ptr->parent, ptr->owner,
             ptr->class, className, ptr->hInstance, ptr->hmemTaskQ,
             ptr->hrgnUpdate, ptr->hwndLastActive, ptr->hdce, ptr->wIDmenu,
             ptr->dwStyle, ptr->dwExStyle, (DWORD)ptr->lpfnWndProc,
             ptr->text ? ptr->text : "",
             ptr->rectClient.left, ptr->rectClient.top, ptr->rectClient.right,
             ptr->rectClient.bottom, ptr->rectWindow.left, ptr->rectWindow.top,
             ptr->rectWindow.right, ptr->rectWindow.bottom, ptr->ptIconPos.x,
             ptr->ptIconPos.y, ptr->ptMaxPos.x, ptr->ptMaxPos.y, ptr->hSysMenu,
             ptr->flags, ptr->hProp, ptr->hVScroll, ptr->hHScroll );

    if (ptr->class->cbWndExtra)
    {
        fprintf( stderr, "extra bytes:" );
        for (i = 0; i < ptr->class->cbWndExtra; i++)
            fprintf( stderr, " %02x", *((BYTE*)ptr->wExtra+i) );
        fprintf( stderr, "\n" );
    }
    fprintf( stderr, "\n" );
}


/***********************************************************************
 *           WIN_WalkWindows
 *
 * Walk the windows tree and print each window on stderr.
 */
void WIN_WalkWindows( HWND hwnd, int indent )
{
    WND *ptr;
    char className[80];

    ptr = hwnd ? WIN_FindWndPtr( hwnd ) : pWndDesktop;
    if (!ptr)
    {
        fprintf( stderr, "*** Invalid window handle\n" );
        return;
    }

    if (!indent)  /* first time around */
        fprintf( stderr, "%-16.16s %-8.8s %-6.6s %-17.17s %-8.8s %s\n",
                 "hwnd", " wndPtr", "queue", "Class Name", " Style", " WndProc");

    while (ptr)
    {
        fprintf(stderr, "%*s%04x%*s", indent, "", ptr->hwndSelf, 13-indent,"");

        GlobalGetAtomName16(ptr->class->atomName,className,sizeof(className));
        
        fprintf( stderr, "%08lx %-6.4x %-17.17s %08x %04x:%04x\n",
                 (DWORD)ptr, ptr->hmemTaskQ, className,
                 (unsigned) ptr->dwStyle,
                 HIWORD(ptr->lpfnWndProc),
                 LOWORD(ptr->lpfnWndProc));
        
        if (ptr->child) WIN_WalkWindows( ptr->child->hwndSelf, indent+1 );
        ptr = ptr->next;
    }
}


/***********************************************************************
 *           WIN_GetXWindow
 *
 * Return the X window associated to a window.
 */
Window WIN_GetXWindow( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    while (wndPtr && !wndPtr->window) wndPtr = wndPtr->parent;
    return wndPtr ? wndPtr->window : 0;
}


/***********************************************************************
 *           WIN_UnlinkWindow
 *
 * Remove a window from the siblings linked list.
 */
BOOL WIN_UnlinkWindow( HWND hwnd )
{    
    WND *wndPtr, **ppWnd;

    if (!(wndPtr = WIN_FindWndPtr( hwnd )) || !wndPtr->parent) return FALSE;
    ppWnd = &wndPtr->parent->child;
    while (*ppWnd != wndPtr) ppWnd = &(*ppWnd)->next;
    *ppWnd = wndPtr->next;
    return TRUE;
}


/***********************************************************************
 *           WIN_LinkWindow
 *
 * Insert a window into the siblings linked list.
 * The window is inserted after the specified window, which can also
 * be specified as HWND_TOP or HWND_BOTTOM.
 */
BOOL WIN_LinkWindow( HWND hwnd, HWND hwndInsertAfter )
{    
    WND *wndPtr, **ppWnd;

    if (!(wndPtr = WIN_FindWndPtr( hwnd )) || !wndPtr->parent) return FALSE;

    if ((hwndInsertAfter == HWND_TOP) || (hwndInsertAfter == HWND_BOTTOM))
    {
        ppWnd = &wndPtr->parent->child;  /* Point to first sibling hwnd */
	if (hwndInsertAfter == HWND_BOTTOM)  /* Find last sibling hwnd */
	    while (*ppWnd) ppWnd = &(*ppWnd)->next;
    }
    else  /* Normal case */
    {
	WND * afterPtr = WIN_FindWndPtr( hwndInsertAfter );
	if (!afterPtr) return FALSE;
        ppWnd = &afterPtr->next;
    }
    wndPtr->next = *ppWnd;
    *ppWnd = wndPtr;
    return TRUE;
}


/***********************************************************************
 *           WIN_FindWinToRepaint
 *
 * Find a window that needs repaint.
 */
HWND WIN_FindWinToRepaint( HWND hwnd, HQUEUE hQueue )
{
    HWND hwndRet;
    WND *pWnd = pWndDesktop;

    /* Note: the desktop window never gets WM_PAINT messages */
    pWnd = hwnd ? WIN_FindWndPtr( hwnd ) : pWndDesktop->child;

    for ( ; pWnd ; pWnd = pWnd->next )
    {
        if (!(pWnd->dwStyle & WS_VISIBLE) || (pWnd->flags & WIN_NO_REDRAW))
        {
            dprintf_win( stddeb, "FindWinToRepaint: skipping window %04x\n",
                         pWnd->hwndSelf );
            continue;
        }
        if ((pWnd->hmemTaskQ == hQueue) &&
            (pWnd->hrgnUpdate || (pWnd->flags & WIN_INTERNAL_PAINT))) break;
        
        if (pWnd->child )
            if ((hwndRet = WIN_FindWinToRepaint( pWnd->child->hwndSelf, hQueue )) )
                return hwndRet;
    }
    
    if (!pWnd) return 0;
    
    hwndRet = pWnd->hwndSelf;

    /* look among siblings if we got a transparent window */
    while (pWnd && ((pWnd->dwExStyle & WS_EX_TRANSPARENT) ||
                    !(pWnd->hrgnUpdate || (pWnd->flags & WIN_INTERNAL_PAINT))))
    {
        pWnd = pWnd->next;
    }
    if (pWnd) hwndRet = pWnd->hwndSelf;
    dprintf_win(stddeb,"FindWinToRepaint: found %04x\n",hwndRet);
    return hwndRet;
}


/***********************************************************************
 *           WIN_SendParentNotify
 *
 * Send a WM_PARENTNOTIFY to all ancestors of the given window, unless
 * the window has the WS_EX_NOPARENTNOTIFY style.
 */
void WIN_SendParentNotify( HWND hwnd, WORD event, WORD idChild, LONG lValue )
{
    LPPOINT16 lppt = (LPPOINT16)&lValue;
    WND     *wndPtr = WIN_FindWndPtr( hwnd );
    BOOL     bMouse = ((event <= WM_MOUSELAST) && (event >= WM_MOUSEFIRST));

    /* if lValue contains cursor coordinates they have to be
     * mapped to the client area of parent window */

    if (bMouse) MapWindowPoints16(0, hwnd, lppt, 1);
#ifndef WINELIB32
    else lValue = MAKELONG( LOWORD(lValue), idChild );
#endif

    while (wndPtr)
    {
        if ((wndPtr->dwExStyle & WS_EX_NOPARENTNOTIFY) ||
	   !(wndPtr->dwStyle & WS_CHILD)) break;

        if (bMouse)
        {
	    lppt->x += wndPtr->rectClient.left;
	    lppt->y += wndPtr->rectClient.top;
        }

        wndPtr = wndPtr->parent;
#ifdef WINELIB32
	SendMessage32A( wndPtr->hwndSelf, WM_PARENTNOTIFY, 
                        MAKEWPARAM( event, idChild ), lValue );
#else
	SendMessage16( wndPtr->hwndSelf, WM_PARENTNOTIFY, event, (LPARAM)lValue);
#endif
    }
}


/***********************************************************************
 *           WIN_SetWndProc
 *
 * Set the window procedure and return the old one.
 */
static WNDPROC16 WIN_SetWndProc(WND *pWnd, WNDPROC16 proc, WINDOWPROCTYPE type)
{
    WNDPROC16 oldProc = pWnd->lpfnWndProc;
    if (type == WIN_PROC_16) pWnd->lpfnWndProc = proc;
    else pWnd->lpfnWndProc = WINPROC_AllocWinProc( (WNDPROC32)proc, type );
    WINPROC_FreeWinProc( oldProc );
    return oldProc;
}


/***********************************************************************
 *           WIN_DestroyWindow
 *
 * Destroy storage associated to a window
 */
static void WIN_DestroyWindow( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );

#ifdef CONFIG_IPC
    if (main_block)
	DDE_DestroyWindow(hwnd);
#endif  /* CONFIG_IPC */
	
    if (!wndPtr) return;
    WIN_UnlinkWindow( hwnd ); /* Remove the window from the linked list */
    TIMER_RemoveWindowTimers( hwnd );
    wndPtr->dwMagic = 0;  /* Mark it as invalid */
    wndPtr->hwndSelf = 0;
    if ((wndPtr->hrgnUpdate) || (wndPtr->flags & WIN_INTERNAL_PAINT))
    {
        if (wndPtr->hrgnUpdate) DeleteObject( wndPtr->hrgnUpdate );
        QUEUE_DecPaintCount( wndPtr->hmemTaskQ );
    }
    if (!(wndPtr->dwStyle & WS_CHILD))
    {
        if (wndPtr->wIDmenu) DestroyMenu( (HMENU)wndPtr->wIDmenu );
    }
    if (wndPtr->hSysMenu) DestroyMenu( wndPtr->hSysMenu );
    if (wndPtr->window) XDestroyWindow( display, wndPtr->window );
    if (wndPtr->class->style & CS_OWNDC) DCE_FreeDCE( wndPtr->hdce );
    WIN_SetWndProc( wndPtr, (WNDPROC16)0, WIN_PROC_16 );
    wndPtr->class->cWindows--;
    USER_HEAP_FREE( hwnd );
}


/***********************************************************************
 *	     WIN_DestroyQueueWindows
 */
void WIN_DestroyQueueWindows( WND* wnd, HQUEUE hQueue )
{
    WND* next;

    while (wnd)
    {
        next = wnd->next;
        if (wnd->hmemTaskQ == hQueue) DestroyWindow( wnd->hwndSelf );
        else WIN_DestroyQueueWindows( wnd->child, hQueue );
        wnd = next;
    }
}


/***********************************************************************
 *           WIN_CreateDesktopWindow
 *
 * Create the desktop window.
 */
BOOL WIN_CreateDesktopWindow(void)
{
    CLASS *class;
    HDC hdc;
    HWND hwndDesktop;

    if (!(class = CLASS_FindClassByAtom( DESKTOP_CLASS_ATOM, 0 )))
	return FALSE;

    hwndDesktop = USER_HEAP_ALLOC( sizeof(WND)+class->cbWndExtra );
    if (!hwndDesktop) return FALSE;
    pWndDesktop = (WND *) USER_HEAP_LIN_ADDR( hwndDesktop );

    pWndDesktop->next              = NULL;
    pWndDesktop->child             = NULL;
    pWndDesktop->parent            = NULL;
    pWndDesktop->owner             = NULL;
    pWndDesktop->class             = class;
    pWndDesktop->dwMagic           = WND_MAGIC;
    pWndDesktop->hwndSelf          = hwndDesktop;
    pWndDesktop->hInstance         = 0;
    pWndDesktop->rectWindow.left   = 0;
    pWndDesktop->rectWindow.top    = 0;
    pWndDesktop->rectWindow.right  = SYSMETRICS_CXSCREEN;
    pWndDesktop->rectWindow.bottom = SYSMETRICS_CYSCREEN;
    pWndDesktop->rectClient        = pWndDesktop->rectWindow;
    pWndDesktop->rectNormal        = pWndDesktop->rectWindow;
    pWndDesktop->ptIconPos.x       = -1;
    pWndDesktop->ptIconPos.y       = -1;
    pWndDesktop->ptMaxPos.x        = -1;
    pWndDesktop->ptMaxPos.y        = -1;
    pWndDesktop->text              = NULL;
    pWndDesktop->hmemTaskQ         = 0; /* Desktop does not belong to a task */
    pWndDesktop->hrgnUpdate        = 0;
    pWndDesktop->hwndLastActive    = hwndDesktop;
    pWndDesktop->lpfnWndProc       = (WNDPROC16)0;
    pWndDesktop->dwStyle           = WS_VISIBLE | WS_CLIPCHILDREN |
                                     WS_CLIPSIBLINGS;
    pWndDesktop->dwExStyle         = 0;
    pWndDesktop->hdce              = 0;
    pWndDesktop->hVScroll          = 0;
    pWndDesktop->hHScroll          = 0;
    pWndDesktop->wIDmenu           = 0;
    pWndDesktop->flags             = 0;
    pWndDesktop->window            = rootWindow;
    pWndDesktop->hSysMenu          = 0;
    pWndDesktop->hProp             = 0;
    WIN_SetWndProc( pWndDesktop, class->lpfnWndProc,
                    WINPROC_GetWinProcType(class->lpfnWndProc) );
    EVENT_RegisterWindow( pWndDesktop->window, hwndDesktop );
    SendMessage32A( hwndDesktop, WM_NCCREATE, 0, 0 );
    if ((hdc = GetDC( hwndDesktop )) != 0)
    {
        SendMessage32A( hwndDesktop, WM_ERASEBKGND, hdc, 0 );
        ReleaseDC( hwndDesktop, hdc );
    }
    return TRUE;
}


/***********************************************************************
 *           WIN_CreateWindowEx
 *
 * Implementation of CreateWindowEx().
 */
static HWND WIN_CreateWindowEx( DWORD exStyle, ATOM classAtom, DWORD style,
                                INT16 x, INT16 y, INT16 width, INT16 height,
                                HWND parent, HMENU menu, HINSTANCE16 instance )
{
    CLASS *classPtr;
    WND *wndPtr;
    HWND16 hwnd;
    POINT16 maxSize, maxPos, minTrack, maxTrack;

    /* Find the parent window */

    if (parent)
    {
	/* Make sure parent is valid */
        if (!IsWindow( parent ))
        {
            fprintf( stderr, "CreateWindowEx: bad parent %04x\n", parent );
	    return 0;
	}
    }
    else if (style & WS_CHILD)
    {
        fprintf( stderr, "CreateWindowEx: no parent for child window\n" );
        return 0;  /* WS_CHILD needs a parent */
    }

    /* Find the window class */

    if (!(classPtr = CLASS_FindClassByAtom( classAtom, GetExePtr(instance) )))
    {
        char buffer[256];
        GlobalGetAtomName32A( classAtom, buffer, sizeof(buffer) );
        fprintf( stderr, "CreateWindowEx: bad class '%s'\n", buffer );
        return 0;
    }

    /* Fix the coordinates */

    if (x == CW_USEDEFAULT16) x = y = 0;
    if (width == CW_USEDEFAULT16)
    {
/*        if (!(style & (WS_CHILD | WS_POPUP))) width = height = 0;
        else */
        {
            width = 600;
            height = 400;
        }
    }

    /* Create the window structure */

    if (!(hwnd = USER_HEAP_ALLOC( sizeof(*wndPtr) + classPtr->cbWndExtra
                                  - sizeof(wndPtr->wExtra) )))
    {
	dprintf_win( stddeb, "CreateWindowEx: out of memory\n" );
	return 0;
    }

    /* Fill the window structure */

    wndPtr = (WND *) USER_HEAP_LIN_ADDR( hwnd );
    wndPtr->next           = NULL;
    wndPtr->child          = NULL;
    wndPtr->parent         = (style & WS_CHILD) ? WIN_FindWndPtr( parent ) : pWndDesktop;
    wndPtr->owner          = (style & WS_CHILD) ? NULL : WIN_FindWndPtr(WIN_GetTopParent(parent));
    wndPtr->window         = 0;
    wndPtr->class          = classPtr;
    wndPtr->dwMagic        = WND_MAGIC;
    wndPtr->hwndSelf       = hwnd;
    wndPtr->hInstance      = instance;
    wndPtr->ptIconPos.x    = -1;
    wndPtr->ptIconPos.y    = -1;
    wndPtr->ptMaxPos.x     = -1;
    wndPtr->ptMaxPos.y     = -1;
    wndPtr->text           = NULL;
    wndPtr->hmemTaskQ      = GetTaskQueue(0);
    wndPtr->hrgnUpdate     = 0;
    wndPtr->hwndLastActive = hwnd;
    wndPtr->lpfnWndProc    = (WNDPROC16)0;
    wndPtr->dwStyle        = style & ~WS_VISIBLE;
    wndPtr->dwExStyle      = exStyle;
    wndPtr->wIDmenu        = 0;
    wndPtr->flags          = 0;
    wndPtr->hVScroll       = 0;
    wndPtr->hHScroll       = 0;
    wndPtr->hSysMenu       = MENU_GetDefSysMenu();
    wndPtr->hProp          = 0;

    if (classPtr->cbWndExtra) memset( wndPtr->wExtra, 0, classPtr->cbWndExtra);
    classPtr->cWindows++;

    /* Correct the window style */

    if (!(style & (WS_POPUP | WS_CHILD)))  /* Overlapped window */
    {
        wndPtr->dwStyle |= WS_CAPTION | WS_CLIPSIBLINGS;
        wndPtr->flags |= WIN_NEED_SIZE;
    }
    if (exStyle & WS_EX_DLGMODALFRAME) wndPtr->dwStyle &= ~WS_THICKFRAME;

    /* Get class or window DC if needed */

    if (classPtr->style & CS_OWNDC) wndPtr->hdce = DCE_AllocDCE(DCE_WINDOW_DC);
    else if (classPtr->style & CS_CLASSDC) wndPtr->hdce = classPtr->hdce;
    else wndPtr->hdce = 0;

    /* Set the window procedure */

    WIN_SetWndProc( wndPtr, classPtr->lpfnWndProc,
                    WINPROC_GetWinProcType(classPtr->lpfnWndProc) );

    /* Insert the window in the linked list */

    WIN_LinkWindow( hwnd, HWND_TOP );

    /* Send the WM_GETMINMAXINFO message and fix the size if needed */

    if ((style & WS_THICKFRAME) || !(style & (WS_POPUP | WS_CHILD)))
    {
        NC_GetMinMaxInfo( hwnd, &maxSize, &maxPos, &minTrack, &maxTrack );
        if (maxSize.x < width) width = maxSize.x;
        if (maxSize.y < height) height = maxSize.y;
    }
    if (width <= 0) width = 1;
    if (height <= 0) height = 1;

    wndPtr->rectWindow.left   = x;
    wndPtr->rectWindow.top    = y;
    wndPtr->rectWindow.right  = x + width;
    wndPtr->rectWindow.bottom = y + height;
    wndPtr->rectClient        = wndPtr->rectWindow;
    wndPtr->rectNormal        = wndPtr->rectWindow;

    /* Create the X window (only for top-level windows, and then only */
    /* when there's no desktop window) */

    if (!(style & WS_CHILD) && (rootWindow == DefaultRootWindow(display)))
    {
        XSetWindowAttributes win_attr;
        Atom XA_WM_DELETE_WINDOW;

	if (Options.managed && ((style & (WS_DLGFRAME | WS_THICKFRAME)) ||
            (exStyle & WS_EX_DLGMODALFRAME)))
        {
	    win_attr.event_mask = ExposureMask | KeyPressMask |
	                          KeyReleaseMask | PointerMotionMask |
	                          ButtonPressMask | ButtonReleaseMask |
	                          FocusChangeMask | StructureNotifyMask;
	    win_attr.override_redirect = FALSE;
            wndPtr->flags |= WIN_MANAGED;
	}
	else
        {
	    win_attr.event_mask = ExposureMask | KeyPressMask |
	                          KeyReleaseMask | PointerMotionMask |
	                          ButtonPressMask | ButtonReleaseMask |
	                          FocusChangeMask;
            win_attr.override_redirect = TRUE;
	}
        win_attr.colormap      = COLOR_WinColormap;
        win_attr.backing_store = Options.backingstore ? WhenMapped : NotUseful;
        win_attr.save_under    = ((classPtr->style & CS_SAVEBITS) != 0);
        win_attr.cursor        = CURSORICON_XCursor;
        wndPtr->window = XCreateWindow( display, rootWindow, x, y,
                                        width, height, 0, CopyFromParent,
                                        InputOutput, CopyFromParent,
                                        CWEventMask | CWOverrideRedirect |
                                        CWColormap | CWCursor | CWSaveUnder |
                                        CWBackingStore, &win_attr );
	XA_WM_DELETE_WINDOW = XInternAtom( display, "WM_DELETE_WINDOW",
					   False );
	XSetWMProtocols( display, wndPtr->window, &XA_WM_DELETE_WINDOW, 1 );
	if (parent)  /* Get window owner */
	{
            Window win = WIN_GetXWindow( parent );
            if (win) XSetTransientForHint( display, wndPtr->window, win );
	}
        EVENT_RegisterWindow( wndPtr->window, hwnd );
    }

    /* Set the window menu */

    if ((style & WS_CAPTION) && !(style & WS_CHILD))
    {
        if (menu) SetMenu(hwnd, menu);
        else
        {
#if 0  /* FIXME: should check if classPtr->menuNameW can be used as is */
            if (classPtr->menuNameA)
                menu = HIWORD(classPtr->menuNameA) ?
                       LoadMenu( instance, SEGPTR_GET(classPtr->menuNameA) ) :
                       LoadMenu( instance, (SEGPTR)classPtr->menuNameA );
#else
            SEGPTR menuName = (SEGPTR)GetClassLong16( hwnd, GCL_MENUNAME );
            if (menuName) menu = LoadMenu( instance, menuName );
#endif
        }
        if (menu) SetMenu( hwnd, menu );
    }
    else wndPtr->wIDmenu = (UINT)menu;

    return hwnd;
}


/***********************************************************************
 *           WIN_FinalWindowInit
 */
static HWND WIN_FinalWindowInit( WND *wndPtr, DWORD style )
{
    if (!(wndPtr->flags & WIN_NEED_SIZE))
    {
	/* send it anyway */
	SendMessage16( wndPtr->hwndSelf, WM_SIZE, SIZE_RESTORED,
		     MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
                            wndPtr->rectClient.bottom-wndPtr->rectClient.top));
        SendMessage16( wndPtr->hwndSelf, WM_MOVE, 0,
                   MAKELONG(wndPtr->rectClient.left, wndPtr->rectClient.top) );
    } 

    WIN_SendParentNotify( wndPtr->hwndSelf, WM_CREATE, wndPtr->wIDmenu,
                          (LONG)wndPtr->hwndSelf );
    if (!IsWindow(wndPtr->hwndSelf)) return 0;

    /* Show the window, maximizing or minimizing if needed */

    if (wndPtr->dwStyle & WS_MINIMIZE)
    {
        wndPtr->dwStyle &= ~WS_MAXIMIZE;
        WINPOS_FindIconPos( wndPtr->hwndSelf );
        SetWindowPos(wndPtr->hwndSelf, 0, wndPtr->ptIconPos.x,
                     wndPtr->ptIconPos.y, SYSMETRICS_CXICON, SYSMETRICS_CYICON,
                     SWP_FRAMECHANGED |
                     (style & WS_VISIBLE) ? SWP_SHOWWINDOW : 0 );
    }
    else if (wndPtr->dwStyle & WS_MAXIMIZE)
    {
        POINT16 maxSize, maxPos, minTrack, maxTrack;
        NC_GetMinMaxInfo( wndPtr->hwndSelf, &maxSize, &maxPos,
                          &minTrack, &maxTrack );
        SetWindowPos( wndPtr->hwndSelf, 0, maxPos.x, maxPos.y, maxSize.x,
                      maxSize.y, SWP_FRAMECHANGED |
                      (style & WS_VISIBLE) ? SWP_SHOWWINDOW : 0 );
    }
    else if (style & WS_VISIBLE) ShowWindow( wndPtr->hwndSelf, SW_SHOW );

    /* Call WH_SHELL hook */

    if (!(wndPtr->dwStyle & WS_CHILD) && !wndPtr->owner)
        HOOK_CallHooks( WH_SHELL, HSHELL_WINDOWCREATED, wndPtr->hwndSelf, 0 );

    return wndPtr->hwndSelf;
}


/***********************************************************************
 *           CreateWindow16   (USER.41)
 */
HWND16 CreateWindow16( SEGPTR className, SEGPTR windowName,
                       DWORD style, INT16 x, INT16 y, INT16 width,
                       INT16 height, HWND16 parent, HMENU16 menu,
                       HINSTANCE16 instance, SEGPTR data ) 
{
    return CreateWindowEx16( 0, className, windowName, style,
			   x, y, width, height, parent, menu, instance, data );
}


/***********************************************************************
 *           CreateWindowEx16   (USER.452)
 */
HWND16 CreateWindowEx16( DWORD exStyle, SEGPTR className, SEGPTR windowName,
                         DWORD style, INT16 x, INT16 y, INT16 width,
                         INT16 height, HWND16 parent, HMENU16 menu,
                         HINSTANCE16 instance, SEGPTR data ) 
{
    ATOM classAtom;
    HWND16 hwnd;
    WND *wndPtr;
    LRESULT wmcreate;

    dprintf_win( stddeb, "CreateWindowEx: " );
    if (HIWORD(windowName))
	dprintf_win( stddeb, "'%s' ", (char *)PTR_SEG_TO_LIN(windowName) );
    else
	dprintf_win( stddeb, "%04x ", LOWORD(windowName) );
    if (HIWORD(className))
        dprintf_win( stddeb, "'%s' ", (char *)PTR_SEG_TO_LIN(className) );
    else
        dprintf_win( stddeb, "%04x ", LOWORD(className) );

    dprintf_win(stddeb, "%08lx %08lx %d,%d %dx%d %04x %04x %04x %08lx\n",
		exStyle, style, x, y, width, height,
		parent, menu, instance, (DWORD)data);

    /* Find the class atom */

    if (!(classAtom = GlobalFindAtom16( className )))
    {
        fprintf( stderr, "CreateWindowEx16: bad class name " );
        if (!HIWORD(className)) fprintf( stderr, "%04x\n", LOWORD(className) );
        else fprintf( stderr, "'%s'\n", (char *)PTR_SEG_TO_LIN(className) );
        return 0;
    }

    hwnd = WIN_CreateWindowEx( exStyle, classAtom, style, x, y, width, height,
                               parent, menu, instance );
    if (!hwnd) return 0;
    wndPtr = (WND *) USER_HEAP_LIN_ADDR( hwnd );

      /* Send the WM_CREATE message */

#ifndef WINELIB
    if (WINPROC_GetWinProcType( wndPtr->lpfnWndProc ) == WIN_PROC_16)
    {
        /* Build the CREATESTRUCT on the 16-bit stack. */
        /* This is really ugly, but some programs (notably the */
        /* "Undocumented Windows" examples) want it that way.  */
        if (!CallWndProcNCCREATE16( wndPtr->lpfnWndProc, wndPtr->hInstance,
                  wndPtr->dwExStyle, className, windowName, wndPtr->dwStyle,
                  wndPtr->rectWindow.left, wndPtr->rectWindow.top,
                  wndPtr->rectWindow.right - wndPtr->rectWindow.left,
                  wndPtr->rectWindow.bottom - wndPtr->rectWindow.top,
                  parent, menu, wndPtr->hInstance, data, hwnd, WM_NCCREATE, 0,
                  MAKELONG( IF1632_Saved16_sp-sizeof(CREATESTRUCT16),
                            IF1632_Saved16_ss ) ))
            wmcreate = -1;
        else
        {
            WINPOS_SendNCCalcSize( hwnd, FALSE, &wndPtr->rectWindow,
                                   NULL, NULL, 0, &wndPtr->rectClient );
            wmcreate = CallWndProcNCCREATE16( wndPtr->lpfnWndProc,
                   wndPtr->hInstance, wndPtr->dwExStyle, className,
                   windowName, wndPtr->dwStyle,
                   wndPtr->rectWindow.left, wndPtr->rectWindow.top,
                   wndPtr->rectWindow.right - wndPtr->rectWindow.left,
                   wndPtr->rectWindow.bottom - wndPtr->rectWindow.top,
                   parent, menu, wndPtr->hInstance, data, hwnd, WM_CREATE, 0,
                   MAKELONG( IF1632_Saved16_sp-sizeof(CREATESTRUCT16),
                             IF1632_Saved16_ss ) );
        }
    }
    else  /* We have a 32-bit window procedure */
#endif  /* WINELIB */
    {
        CREATESTRUCT32A cs;
        cs.lpCreateParams = (LPVOID)data;
        cs.hInstance      = wndPtr->hInstance;
        cs.hMenu          = wndPtr->wIDmenu;
        cs.hwndParent     = parent;
        cs.cx             = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
        cs.cy             = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;
        cs.x              = wndPtr->rectWindow.left;
        cs.y              = wndPtr->rectWindow.top;
        cs.style          = wndPtr->dwStyle | (style & WS_VISIBLE);
        cs.lpszName       = PTR_SEG_TO_LIN(windowName);
        cs.lpszClass      = PTR_SEG_TO_LIN(className);
        cs.dwExStyle      = wndPtr->dwExStyle;

        if (!SendMessage32A( hwnd, WM_NCCREATE, 0, (LPARAM)&cs)) wmcreate = -1;
        else
        {
            WINPOS_SendNCCalcSize( hwnd, FALSE, &wndPtr->rectWindow,
                                   NULL, NULL, 0, &wndPtr->rectClient );
            wmcreate = SendMessage32A( hwnd, WM_CREATE, 0, (LPARAM)&cs );
        }
    }
    
    if (wmcreate == -1)
    {
	  /* Abort window creation */
	dprintf_win(stddeb,"CreateWindowEx: wmcreate==-1, aborting\n");
        WIN_DestroyWindow( hwnd );
	return 0;
    }

    dprintf_win(stddeb, "CreateWindowEx16: return %04x\n", hwnd);
    return WIN_FinalWindowInit( wndPtr, style );
}


/***********************************************************************
 *           CreateWindowEx32A   (USER32.82)
 */
HWND32 CreateWindowEx32A( DWORD exStyle, LPCSTR className, LPCSTR windowName,
                          DWORD style, INT32 x, INT32 y, INT32 width,
                          INT32 height, HWND32 parent, HMENU32 menu,
                          HINSTANCE32 instance, LPVOID data )
{
    ATOM classAtom;
    HWND16 hwnd;
    WND *wndPtr;
    LRESULT wmcreate;
    CREATESTRUCT32A cs;

    /* Find the class atom */

    if (!(classAtom = GlobalFindAtom32A( className )))
    {
        fprintf( stderr, "CreateWindowEx32A: bad class name " );
        if (!HIWORD(className)) fprintf( stderr, "%04x\n", LOWORD(className) );
        else fprintf( stderr, "'%s'\n", className );
        return 0;
    }

    /* Fix the coordinates */

    if (x == CW_USEDEFAULT32) x = y = (UINT32)CW_USEDEFAULT16;
    if (width == CW_USEDEFAULT32) width = height = (UINT32)CW_USEDEFAULT16;

    /* Create the window structure */

    hwnd = WIN_CreateWindowEx( exStyle, classAtom, style, x, y, width, height,
                               parent, menu, instance );
    if (!hwnd) return 0;
    wndPtr = (WND *) USER_HEAP_LIN_ADDR( hwnd );

    /* Send the WM_CREATE message */

    cs.lpCreateParams = data;
    cs.hInstance      = wndPtr->hInstance;
    cs.hMenu          = wndPtr->wIDmenu;
    cs.hwndParent     = parent;
    cs.cx             = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    cs.cy             = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;
    cs.x              = wndPtr->rectWindow.left;
    cs.y              = wndPtr->rectWindow.top;
    cs.style          = wndPtr->dwStyle | (style & WS_VISIBLE);
    cs.lpszName       = windowName;
    cs.lpszClass      = className;
    cs.dwExStyle      = wndPtr->dwExStyle;

    if (!SendMessage32A( hwnd, WM_NCCREATE, 0, (LPARAM)&cs )) wmcreate = -1;
    else
    {
        WINPOS_SendNCCalcSize( hwnd, FALSE, &wndPtr->rectWindow,
                               NULL, NULL, 0, &wndPtr->rectClient );
        wmcreate = SendMessage32A( hwnd, WM_CREATE, 0, (LPARAM)&cs );
    }

    if (wmcreate == -1)
    {
	  /* Abort window creation */
	dprintf_win(stddeb,"CreateWindowEx32A: wmcreate==-1, aborting\n");
        WIN_DestroyWindow( hwnd );
	return 0;
    }

    dprintf_win(stddeb, "CreateWindowEx32A: return %04x\n", hwnd);
    return WIN_FinalWindowInit( wndPtr, style );
}


/***********************************************************************
 *           CreateWindowEx32W   (USER32.83)
 */
HWND32 CreateWindowEx32W( DWORD exStyle, LPCWSTR className, LPCWSTR windowName,
                          DWORD style, INT32 x, INT32 y, INT32 width,
                          INT32 height, HWND32 parent, HMENU32 menu,
                          HINSTANCE32 instance, LPVOID data )
{
    ATOM classAtom;
    HWND16 hwnd;
    WND *wndPtr;
    LRESULT wmcreate;
    CREATESTRUCT32W cs;

    /* Find the class atom */

    if (!(classAtom = GlobalFindAtom32W( className )))
    {
        fprintf( stderr, "CreateWindowEx32W: bad class name %p\n", className );
        return 0;
    }

    /* Fix the coordinates */

    if (x == CW_USEDEFAULT32) x = y = (UINT32)CW_USEDEFAULT16;
    if (width == CW_USEDEFAULT32) width = height = (UINT32)CW_USEDEFAULT16;

    /* Create the window structure */

    hwnd = WIN_CreateWindowEx( exStyle, classAtom, style, x, y, width, height,
                               parent, menu, instance );
    if (!hwnd) return 0;
    wndPtr = (WND *) USER_HEAP_LIN_ADDR( hwnd );

    /* Send the WM_CREATE message */

    cs.lpCreateParams = data;
    cs.hInstance      = wndPtr->hInstance;
    cs.hMenu          = wndPtr->wIDmenu;
    cs.hwndParent     = parent;
    cs.cx             = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    cs.cy             = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;
    cs.x              = wndPtr->rectWindow.left;
    cs.y              = wndPtr->rectWindow.top;
    cs.style          = wndPtr->dwStyle | (style & WS_VISIBLE);
    cs.lpszName       = windowName;
    cs.lpszClass      = className;
    cs.dwExStyle      = wndPtr->dwExStyle;

    if (!SendMessage32W( hwnd, WM_NCCREATE, 0, (LPARAM)&cs )) wmcreate = -1;
    else
    {
        WINPOS_SendNCCalcSize( hwnd, FALSE, &wndPtr->rectWindow,
                               NULL, NULL, 0, &wndPtr->rectClient );
        wmcreate = SendMessage32W( hwnd, WM_CREATE, 0, (LPARAM)&cs );
    }

    if (wmcreate == -1)
    {
	  /* Abort window creation */
	dprintf_win(stddeb,"CreateWindowEx32W: wmcreate==-1, aborting\n");
        WIN_DestroyWindow( hwnd );
	return 0;
    }

    dprintf_win(stddeb, "CreateWindowEx32W: return %04x\n", hwnd);
    return WIN_FinalWindowInit( wndPtr, style );
}


/***********************************************************************
 *           DestroyWindow   (USER.53)
 */
BOOL DestroyWindow( HWND hwnd )
{
    WND * wndPtr;

    dprintf_win(stddeb, "DestroyWindow(%04x)\n", hwnd);
    
      /* Initialization */

    if (hwnd == pWndDesktop->hwndSelf) return FALSE; /* Can't destroy desktop*/
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;

      /* Top-level window */

    if (!(wndPtr->dwStyle & WS_CHILD) && !wndPtr->owner)
    {
        HOOK_CallHooks( WH_SHELL, HSHELL_WINDOWDESTROYED, hwnd, 0L );
        /* FIXME: clean up palette - see "Internals" p.352 */
    }

      /* Hide the window */

    if (wndPtr->dwStyle & WS_VISIBLE)
	SetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOACTIVATE |
		      SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE );
    if ((hwnd == GetCapture()) || IsChild( hwnd, GetCapture() ))
	ReleaseCapture();
   if (!QUEUE_GetDoomedQueue())
       WIN_SendParentNotify( hwnd, WM_DESTROY, wndPtr->wIDmenu, (LONG)hwnd );

      /* Recursively destroy owned windows */

    for (;;)
    {
        WND *siblingPtr = wndPtr->parent->child;  /* First sibling */
        while (siblingPtr)
        {
            if (siblingPtr->owner == wndPtr) break;
            siblingPtr = siblingPtr->next;
        }
        if (siblingPtr) DestroyWindow( siblingPtr->hwndSelf );
        else break;
    }

      /* Send destroy messages and destroy children */

    SendMessage16( hwnd, WM_DESTROY, 0, 0 );
    while (wndPtr->child)  /* The child removes itself from the list */
	DestroyWindow( wndPtr->child->hwndSelf );
    SendMessage16( hwnd, WM_NCDESTROY, 0, 0 );

      /* Destroy the window */

    WIN_DestroyWindow( hwnd );
    return TRUE;
}


/***********************************************************************
 *           CloseWindow   (USER.43)
 */
BOOL CloseWindow(HWND hWnd)
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr->dwStyle & WS_CHILD) return TRUE;
    ShowWindow(hWnd, SW_MINIMIZE);
    return TRUE;
}

 
/***********************************************************************
 *           OpenIcon   (USER.44)
 */
BOOL OpenIcon(HWND hWnd)
{
    if (!IsIconic(hWnd)) return FALSE;
    ShowWindow(hWnd, SW_SHOWNORMAL);
    return(TRUE);
}


/***********************************************************************
 *           WIN_FindWindow
 *
 * Implementation of FindWindow() and FindWindowEx().
 */
static HWND WIN_FindWindow( HWND parent, HWND child, ATOM className,
                            LPCSTR title )
{
    WND *pWnd;
    CLASS *pClass = NULL;

    if (child)
    {
        if (!(pWnd = WIN_FindWndPtr( child ))) return 0;
        if (parent)
        {
            if (!pWnd->parent || (pWnd->parent->hwndSelf != parent)) return 0;
        }
        else if (pWnd->parent != pWndDesktop) return 0;
        pWnd = pWnd->next;
    }
    else
    {
        if (!(pWnd = parent ? WIN_FindWndPtr(parent) : pWndDesktop)) return 0;
        pWnd = pWnd->child;
    }
    if (!pWnd) return 0;

    /* For a child window, all siblings will have the same hInstance, */
    /* so we can look for the class once and for all. */

    if (className && (pWnd->dwStyle & WS_CHILD))
    {
        if (!(pClass = CLASS_FindClassByAtom( className, pWnd->hInstance )))
            return 0;
    }

    for ( ; pWnd; pWnd = pWnd->next)
    {
        if (className && !(pWnd->dwStyle & WS_CHILD))
        {
            if (!(pClass = CLASS_FindClassByAtom( className, pWnd->hInstance)))
                continue;  /* Skip this window */
        }
        if (pClass && (pWnd->class != pClass))
            continue;  /* Not the right class */

        /* Now check the title */

        if (!title) return pWnd->hwndSelf;
        if (pWnd->text && !strcmp( pWnd->text, title )) return pWnd->hwndSelf;
    }
    return 0;
}



/***********************************************************************
 *           FindWindow16   (USER.50)
 */
HWND FindWindow16( SEGPTR className, LPCSTR title )
{
    return FindWindowEx16( 0, 0, className, title );
}


/***********************************************************************
 *           FindWindowEx16   (USER.427)
 */
HWND FindWindowEx16( HWND parent, HWND child, SEGPTR className, LPCSTR title )
{
    ATOM atom;

    atom = className ? GlobalFindAtom16( className ) : 0;
    return WIN_FindWindow( parent, child, atom, title );
}


/***********************************************************************
 *           FindWindow32A   (USER32.197)
 */
HWND FindWindow32A( LPCSTR className, LPCSTR title )
{
    return FindWindowEx32A( 0, 0, className, title );
}


/***********************************************************************
 *           FindWindowEx32A   (USER32.198)
 */
HWND FindWindowEx32A( HWND parent, HWND child, LPCSTR className, LPCSTR title )
{
    ATOM atom;

    atom = className ? GlobalFindAtom32A( className ) : 0;
    return WIN_FindWindow( 0, 0, atom, title );
}


/***********************************************************************
 *           FindWindowEx32W   (USER32.199)
 */
HWND FindWindowEx32W(HWND parent, HWND child, LPCWSTR className, LPCWSTR title)
{
    ATOM atom;
    char *buffer;
    HWND hwnd;

    atom = className ? GlobalFindAtom32W( className ) : 0;
    buffer = STRING32_DupUniToAnsi( title );
    hwnd = WIN_FindWindow( 0, 0, atom, buffer );
    free( buffer );
    return hwnd;
}


/***********************************************************************
 *           FindWindow32W   (USER32.200)
 */
HWND FindWindow32W( LPCWSTR className, LPCWSTR title )
{
    return FindWindowEx32W( 0, 0, className, title );
}


/**********************************************************************
 *           WIN_GetDesktop
 */
WND *WIN_GetDesktop(void)
{
    return pWndDesktop;
}


/**********************************************************************
 *           GetDesktopWindow   (USER.286)
 */
HWND GetDesktopWindow(void)
{
    return pWndDesktop->hwndSelf;
}


/**********************************************************************
 *           GetDesktopHwnd   (USER.278)
 *
 * Exactly the same thing as GetDesktopWindow(), but not documented.
 * Don't ask me why...
 */
HWND GetDesktopHwnd(void)
{
    return pWndDesktop->hwndSelf;
}


/*******************************************************************
 *           EnableWindow   (USER.34)
 */
BOOL EnableWindow( HWND hwnd, BOOL enable )
{
    WND *wndPtr;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (enable && (wndPtr->dwStyle & WS_DISABLED))
    {
	  /* Enable window */
	wndPtr->dwStyle &= ~WS_DISABLED;
	SendMessage16( hwnd, WM_ENABLE, TRUE, 0 );
	return TRUE;
    }
    else if (!enable && !(wndPtr->dwStyle & WS_DISABLED))
    {
	  /* Disable window */
	wndPtr->dwStyle |= WS_DISABLED;
	if ((hwnd == GetFocus()) || IsChild( hwnd, GetFocus() ))
	    SetFocus( 0 );  /* A disabled window can't have the focus */
	if ((hwnd == GetCapture()) || IsChild( hwnd, GetCapture() ))
	    ReleaseCapture();  /* A disabled window can't capture the mouse */
	SendMessage16( hwnd, WM_ENABLE, FALSE, 0 );
	return FALSE;
    }
    return ((wndPtr->dwStyle & WS_DISABLED) != 0);
}


/***********************************************************************
 *           IsWindowEnabled   (USER.35) (USER32.348)
 */ 
BOOL IsWindowEnabled(HWND hWnd)
{
    WND * wndPtr; 

    if (!(wndPtr = WIN_FindWndPtr(hWnd))) return FALSE;
    return !(wndPtr->dwStyle & WS_DISABLED);
}


/***********************************************************************
 *           IsWindowUnicode   (USER32.349)
 */
BOOL IsWindowUnicode( HWND hwnd )
{
    WND * wndPtr; 

    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return FALSE;
    return (WINPROC_GetWinProcType( wndPtr->lpfnWndProc ) == WIN_PROC_32W);
}


/**********************************************************************
 *	     GetWindowWord    (USER.133) (USER32.313)
 */
WORD GetWindowWord( HWND32 hwnd, INT32 offset )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0) return *(WORD *)(((char *)wndPtr->wExtra) + offset);
    switch(offset)
    {
    case GWW_ID:         return wndPtr->wIDmenu;
    case GWW_HWNDPARENT: return wndPtr->parent ? wndPtr->parent->hwndSelf : 0;
    case GWW_HINSTANCE:  return (WORD)wndPtr->hInstance;
    default:
        fprintf( stderr, "GetWindowWord: invalid offset %d\n", offset );
        return 0;
    }
}


/**********************************************************************
 *	     WIN_GetWindowInstance
 */
HINSTANCE WIN_GetWindowInstance(HWND hwnd)
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return (HINSTANCE)0;
    return wndPtr->hInstance;
}


/**********************************************************************
 *	     SetWindowWord    (USER.134) (USER32.523)
 */
WORD SetWindowWord( HWND32 hwnd, INT32 offset, WORD newval )
{
    WORD *ptr, retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0) ptr = (WORD *)(((char *)wndPtr->wExtra) + offset);
    else switch(offset)
    {
	case GWW_ID:        ptr = (WORD *)&wndPtr->wIDmenu; break;
	case GWW_HINSTANCE: ptr = (WORD *)&wndPtr->hInstance; break;
	default:
            fprintf( stderr, "SetWindowWord: invalid offset %d\n", offset );
            return 0;
    }
    retval = *ptr;
    *ptr = newval;
    return retval;
}


/**********************************************************************
 *	     GetWindowLong    (USER.135)
 */
LONG GetWindowLong( HWND32 hwnd, INT32 offset )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0) return *(LONG *)(((char *)wndPtr->wExtra) + offset);
    switch(offset)
    {
        case GWL_USERDATA:   return 0;
        case GWL_STYLE:      return wndPtr->dwStyle;
        case GWL_EXSTYLE:    return wndPtr->dwExStyle;
        case GWL_ID:         return wndPtr->wIDmenu;
        case GWL_WNDPROC:    return (LONG)wndPtr->lpfnWndProc;
        case GWL_HWNDPARENT: return wndPtr->parent ?
                                        (HWND32)wndPtr->parent->hwndSelf : 0;
        case GWL_HINSTANCE:  return (HINSTANCE32)wndPtr->hInstance;
        default:
            fprintf( stderr, "GetWindowLong: unknown offset %d\n", offset );
    }
    return 0;
}


/**********************************************************************
 *	     SetWindowLong16    (USER.136)
 */
LONG SetWindowLong16( HWND16 hwnd, INT16 offset, LONG newval )
{
    WND *wndPtr;
    switch(offset)
    {
    case GWL_WNDPROC:
        wndPtr = WIN_FindWndPtr( hwnd );
        return (LONG)WIN_SetWndProc( wndPtr, (WNDPROC16)newval, WIN_PROC_16 );
    default:
        return SetWindowLong32A( hwnd, offset, newval );
    }
}


/**********************************************************************
 *	     SetWindowLong32A    (USER32.516)
 */
LONG SetWindowLong32A( HWND32 hwnd, INT32 offset, LONG newval )
{
    LONG *ptr, retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0) ptr = (LONG *)(((char *)wndPtr->wExtra) + offset);
    else switch(offset)
    {
        case GWL_ID:
        case GWL_HINSTANCE:
            return SetWindowWord( hwnd, offset, (WORD)newval );
	case GWL_WNDPROC:
            return (LONG)WIN_SetWndProc( wndPtr, (WNDPROC16)newval,
                                         WIN_PROC_32A );
        case GWL_USERDATA: return 0;
	case GWL_STYLE:   ptr = &wndPtr->dwStyle; break;
        case GWL_EXSTYLE: ptr = &wndPtr->dwExStyle; break;
	default:
            fprintf( stderr, "SetWindowLong: invalid offset %d\n", offset );
            return 0;
    }
    retval = *ptr;
    *ptr = newval;
    return retval;
}


/**********************************************************************
 *	     SetWindowLong32W    (USER32.517)
 */
LONG SetWindowLong32W( HWND32 hwnd, INT32 offset, LONG newval )
{
    WND *wndPtr;
    switch(offset)
    {
    case GWL_WNDPROC:
        wndPtr = WIN_FindWndPtr( hwnd );
        return (LONG)WIN_SetWndProc( wndPtr, (WNDPROC16)newval, WIN_PROC_32W );
    default:
        return SetWindowLong32A( hwnd, offset, newval );
    }
}


/*******************************************************************
 *	     GetWindowText16    (USER.36)
 */
INT16 GetWindowText16( HWND16 hwnd, SEGPTR lpString, INT16 nMaxCount )
{
    return (INT16)SendMessage16(hwnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString);
}


/*******************************************************************
 *	     GetWindowText32A    (USER32.308)
 */
INT32 GetWindowText32A( HWND32 hwnd, LPSTR lpString, INT32 nMaxCount )
{
    return (INT32)SendMessage32A( hwnd, WM_GETTEXT, nMaxCount,
                                  (LPARAM)lpString );
}


/*******************************************************************
 *	     GetWindowText32W    (USER32.311)
 */
INT32 GetWindowText32W( HWND32 hwnd, LPWSTR lpString, INT32 nMaxCount )
{
    return (INT32)SendMessage32W( hwnd, WM_GETTEXT, nMaxCount,
                                  (LPARAM)lpString );
}


/*******************************************************************
 *	     SetWindowText16    (USER.37)
 */
void SetWindowText16( HWND16 hwnd, SEGPTR lpString )
{
    SendMessage16( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *	     SetWindowText32A    (USER32.)
 */
void SetWindowText32A( HWND32 hwnd, LPCSTR lpString )
{
    SendMessage32A( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *	     SetWindowText32W    (USER32.)
 */
void SetWindowText32W( HWND32 hwnd, LPCWSTR lpString )
{
    SendMessage32W( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *         GetWindowTextLength    (USER.38)
 */
int GetWindowTextLength(HWND hwnd)
{
    return (int)SendMessage16(hwnd, WM_GETTEXTLENGTH, 0, 0 );
}


/*******************************************************************
 *         IsWindow    (USER.47)
 */
BOOL IsWindow( HWND hwnd )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    return ((wndPtr != NULL) && (wndPtr->dwMagic == WND_MAGIC));
}


/*****************************************************************
 *         GetParent              (USER.46)
 */
HWND GetParent(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    if (!wndPtr) return 0;
    wndPtr = (wndPtr->dwStyle & WS_CHILD) ? wndPtr->parent : wndPtr->owner;
    return wndPtr ? wndPtr->hwndSelf : 0;
}


/*****************************************************************
 *         WIN_GetTopParent
 *
 * Get the top-level parent for a child window.
 */
HWND WIN_GetTopParent( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    while (wndPtr && (wndPtr->dwStyle & WS_CHILD)) wndPtr = wndPtr->parent;
    return wndPtr ? wndPtr->hwndSelf : 0;
}


/*****************************************************************
 *         SetParent              (USER.233)
 */
HWND SetParent(HWND hwndChild, HWND hwndNewParent)
{
    HWND oldParent;

    WND *wndPtr = WIN_FindWndPtr(hwndChild);
    WND *pWndParent = WIN_FindWndPtr( hwndNewParent );
    if (!wndPtr || !pWndParent || !(wndPtr->dwStyle & WS_CHILD)) return 0;

    oldParent = wndPtr->parent->hwndSelf;

    WIN_UnlinkWindow(hwndChild);
    if (hwndNewParent) wndPtr->parent = pWndParent;
    WIN_LinkWindow(hwndChild, HWND_BOTTOM);
    
    if (IsWindowVisible(hwndChild)) UpdateWindow(hwndChild);
    
    return oldParent;
}



/*******************************************************************
 *         IsChild    (USER.48)
 */
BOOL IsChild( HWND parent, HWND child )
{
    WND * wndPtr = WIN_FindWndPtr( child );
    while (wndPtr && (wndPtr->dwStyle & WS_CHILD))
    {
        wndPtr = wndPtr->parent;
	if (wndPtr->hwndSelf == parent) return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           IsWindowVisible   (USER.49) (USER32.350)
 */
BOOL IsWindowVisible( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    while (wndPtr && (wndPtr->dwStyle & WS_CHILD))
    {
        if (!(wndPtr->dwStyle & WS_VISIBLE)) return FALSE;
        wndPtr = wndPtr->parent;
    }
    return (wndPtr && (wndPtr->dwStyle & WS_VISIBLE));
}

 
 
/*******************************************************************
 *         GetTopWindow    (USER.229)
 */
HWND GetTopWindow( HWND hwnd )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (wndPtr && wndPtr->child) return wndPtr->child->hwndSelf;
    else return 0;
}


/*******************************************************************
 *         GetWindow    (USER.262)
 */
HWND GetWindow( HWND hwnd, WORD rel )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    switch(rel)
    {
    case GW_HWNDFIRST:
        if (wndPtr->parent) return wndPtr->parent->child->hwndSelf;
	else return 0;
	
    case GW_HWNDLAST:
	if (!wndPtr->parent) return 0;  /* Desktop window */
	while (wndPtr->next) wndPtr = wndPtr->next;
        return wndPtr->hwndSelf;
	
    case GW_HWNDNEXT:
        if (!wndPtr->next) return 0;
	return wndPtr->next->hwndSelf;
	
    case GW_HWNDPREV:
        if (!wndPtr->parent) return 0;  /* Desktop window */
        wndPtr = wndPtr->parent->child;  /* First sibling */
        if (wndPtr->hwndSelf == hwnd) return 0;  /* First in list */
        while (wndPtr->next)
        {
            if (wndPtr->next->hwndSelf == hwnd) return wndPtr->hwndSelf;
            wndPtr = wndPtr->next;
        }
        return 0;
	
    case GW_OWNER:
	return wndPtr->owner ? wndPtr->owner->hwndSelf : 0;

    case GW_CHILD:
	return wndPtr->child ? wndPtr->child->hwndSelf : 0;
    }
    return 0;
}


/*******************************************************************
 *         GetNextWindow    (USER.230)
 */
HWND GetNextWindow( HWND hwnd, WORD flag )
{
    if ((flag != GW_HWNDNEXT) && (flag != GW_HWNDPREV)) return 0;
    return GetWindow( hwnd, flag );
}

/*******************************************************************
 *         ShowOwnedPopups  (USER.265)
 */
void ShowOwnedPopups( HWND owner, BOOL fShow )
{
    WND *pWnd = pWndDesktop->child;
    while (pWnd)
    {
        if (pWnd->owner && (pWnd->owner->hwndSelf == owner) &&
            (pWnd->dwStyle & WS_POPUP))
            ShowWindow( pWnd->hwndSelf, fShow ? SW_SHOW : SW_HIDE );
        pWnd = pWnd->next;
    }
}


/*******************************************************************
 *         GetLastActivePopup    (USER.287)
 */
HWND GetLastActivePopup(HWND hwnd)
{
    WND *wndPtr;
    wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr == NULL) return hwnd;
    return wndPtr->hwndLastActive;
}


/*******************************************************************
 *           EnumWindows   (USER.54)
 */
BOOL EnumWindows( WNDENUMPROC lpEnumFunc, LPARAM lParam )
{
    WND *wndPtr;
    HWND *list, *pWnd;
    int count;

    /* We have to build a list of all windows first, to avoid */
    /* unpleasant side-effects, for instance if the callback  */
    /* function changes the Z-order of the windows.           */

      /* First count the windows */

    count = 0;
    for (wndPtr = pWndDesktop->child; wndPtr; wndPtr = wndPtr->next) count++;
    if (!count) return TRUE;

      /* Now build the list of all windows */

    if (!(pWnd = list = (HWND *)malloc( sizeof(HWND) * count ))) return FALSE;
    for (wndPtr = pWndDesktop->child; wndPtr; wndPtr = wndPtr->next)
        *pWnd++ = wndPtr->hwndSelf;

      /* Now call the callback function for every window */

    for (pWnd = list; count > 0; count--, pWnd++)
    {
          /* Make sure that window still exists */
        if (!IsWindow(*pWnd)) continue;
        if (!CallEnumWindowsProc( lpEnumFunc, *pWnd, lParam )) break;
    }
    free( list );
    return TRUE;
}


/**********************************************************************
 *           EnumTaskWindows   (USER.225)
 */
BOOL EnumTaskWindows( HTASK hTask, WNDENUMPROC lpEnumFunc, LPARAM lParam )
{
    WND *wndPtr;
    HWND *list, *pWnd;
    HANDLE hQueue = GetTaskQueue( hTask );
    int count;

    /* This function is the same as EnumWindows(),    */
    /* except for an added check on the window queue. */

      /* First count the windows */

    count = 0;
    for (wndPtr = pWndDesktop->child; wndPtr; wndPtr = wndPtr->next)
        if (wndPtr->hmemTaskQ == hQueue) count++;
    if (!count) return TRUE;

      /* Now build the list of all windows */

    if (!(pWnd = list = (HWND *)malloc( sizeof(HWND) * count ))) return FALSE;
    for (wndPtr = pWndDesktop->child; wndPtr; wndPtr = wndPtr->next)
        if (wndPtr->hmemTaskQ == hQueue) *pWnd++ = wndPtr->hwndSelf;

      /* Now call the callback function for every window */

    for (pWnd = list; count > 0; count--, pWnd++)
    {
          /* Make sure that window still exists */
        if (!IsWindow(*pWnd)) continue;
        if (!CallEnumTaskWndProc( lpEnumFunc, *pWnd, lParam )) break;
    }
    free( list );
    return TRUE;
}


/*******************************************************************
 *    WIN_EnumChildWin
 *
 *   o hwnd is the first child to use, loop until all next windows
 *     are processed
 * 
 *   o call wdnenumprc
 *
 *   o call ourselves with the next child window
 * 
 */
static BOOL WIN_EnumChildWin( WND *wndPtr, FARPROC wndenumprc, LPARAM lParam )
{
    WND *pWndNext, *pWndChild;
    while (wndPtr)
    {
        pWndNext = wndPtr->next;    /* storing hwnd is a way to avoid.. */
        pWndChild = wndPtr->child;  /* ..side effects after wndenumprc  */
        if (!CallEnumWindowsProc( wndenumprc, wndPtr->hwndSelf, lParam ))
            return 0;
        if (pWndChild && IsWindow(pWndChild->hwndSelf))
            if (!WIN_EnumChildWin(pWndChild, wndenumprc, lParam)) return 0;
        wndPtr = pWndNext;
    } 
    return 1;
}


/*******************************************************************
 *    EnumChildWindows        (USER.55)
 *
 *   o gets the first child of hwnd
 *
 *   o calls WIN_EnumChildWin to do a recursive decent of child windows
 */
BOOL EnumChildWindows(HWND hwnd, WNDENUMPROC wndenumprc, LPARAM lParam)
{
    WND *wndPtr;

    dprintf_enum(stddeb,"EnumChildWindows\n");

    if (hwnd == 0) return 0;
    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
    return WIN_EnumChildWin(wndPtr->child, wndenumprc, lParam);
}


/*******************************************************************
 *           AnyPopup   (USER.52)
 */
BOOL AnyPopup(void)
{
    WND *wndPtr;
    for (wndPtr = pWndDesktop->child; wndPtr; wndPtr = wndPtr->next)
        if (wndPtr->owner && (wndPtr->dwStyle & WS_VISIBLE)) return TRUE;
    return FALSE;
}

/*******************************************************************
 *			FlashWindow		[USER.105]
 */
BOOL FlashWindow(HWND hWnd, BOOL bInvert)
{
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    dprintf_win(stddeb,"FlashWindow: %04x\n", hWnd);

    if (!wndPtr) return FALSE;

    if (wndPtr->dwStyle & WS_MINIMIZE)
    {
        if (bInvert && !(wndPtr->flags & WIN_NCACTIVATED))
        {
            HDC hDC = GetDC(hWnd);
            
            if (!SendMessage16( hWnd, WM_ERASEBKGND, (WPARAM)hDC, (LPARAM)0 ))
                wndPtr->flags |= WIN_NEEDS_ERASEBKGND;
            
            ReleaseDC( hWnd, hDC );
            wndPtr->flags |= WIN_NCACTIVATED;
        }
        else
        {
            RedrawWindow32( hWnd, 0, 0, RDW_INVALIDATE | RDW_ERASE |
                            RDW_UPDATENOW | RDW_FRAME );
            wndPtr->flags &= ~WIN_NCACTIVATED;
        }
        return TRUE;
    }
    else
    {
        WPARAM wparam;
        if (bInvert) wparam = !(wndPtr->flags & WIN_NCACTIVATED);
        else wparam = (hWnd == GetActiveWindow());

        SendMessage16( hWnd, WM_NCACTIVATE, wparam, (LPARAM)0 );
        return wparam;
    }
}


/*******************************************************************
 *			SetSysModalWindow		[USER.188]
 */
HWND SetSysModalWindow(HWND hWnd)
{
    HWND hWndOldModal = hwndSysModal;
    hwndSysModal = hWnd;
    dprintf_win(stdnimp,"EMPTY STUB !! SetSysModalWindow(%04x) !\n", hWnd);
    return hWndOldModal;
}


/*******************************************************************
 *			GetSysModalWindow		[USER.189]
 */
HWND GetSysModalWindow(void)
{
    return hwndSysModal;
}

/*******************************************************************
 *			DRAG_QueryUpdate
 *
 * recursively find a child that contains spDragInfo->pt point 
 * and send WM_QUERYDROPOBJECT
 */
BOOL DRAG_QueryUpdate( HWND hQueryWnd, SEGPTR spDragInfo )
{
 BOOL		wParam,bResult = 0;
 POINT16        pt;
 LPDRAGINFO	ptrDragInfo = (LPDRAGINFO) PTR_SEG_TO_LIN(spDragInfo);
 WND 	       *ptrQueryWnd = WIN_FindWndPtr(hQueryWnd),*ptrWnd;
 RECT16		tempRect;	/* this sucks */

 if( !ptrQueryWnd || !ptrDragInfo ) return 0;

 pt 		= ptrDragInfo->pt;

 GetWindowRect16(hQueryWnd,&tempRect); 

 if( !PtInRect16(&tempRect,pt) ||
     (ptrQueryWnd->dwStyle & WS_DISABLED) )
	return 0;

 if( !(ptrQueryWnd->dwStyle & WS_MINIMIZE) ) 
   {
     tempRect = ptrQueryWnd->rectClient;
     if(ptrQueryWnd->dwStyle & WS_CHILD)
        MapWindowPoints16(ptrQueryWnd->parent->hwndSelf,0,(LPPOINT16)&tempRect,2);

     if( PtInRect16(&tempRect,pt) )
	{
	 wParam = 0;
         
	 for (ptrWnd = ptrQueryWnd->child; ptrWnd ;ptrWnd = ptrWnd->next)
             if( ptrWnd->dwStyle & WS_VISIBLE )
	     {
                 GetWindowRect16(ptrWnd->hwndSelf,&tempRect);

                 if( PtInRect16(&tempRect,pt) ) 
                     break;
	     }

	 if(ptrWnd)
         {
	    dprintf_msg(stddeb,"DragQueryUpdate: hwnd = %04x, %d %d - %d %d\n",
                        ptrWnd->hwndSelf, ptrWnd->rectWindow.left, ptrWnd->rectWindow.top,
			ptrWnd->rectWindow.right, ptrWnd->rectWindow.bottom );
            if( !(ptrWnd->dwStyle & WS_DISABLED) )
	        bResult = DRAG_QueryUpdate(ptrWnd->hwndSelf, spDragInfo);
         }

	 if(bResult) return bResult;
	}
     else wParam = 1;
   }
 else wParam = 1;

 ScreenToClient16(hQueryWnd,&ptrDragInfo->pt);

 ptrDragInfo->hScope = hQueryWnd;

 bResult = SendMessage16( hQueryWnd ,WM_QUERYDROPOBJECT ,
                       (WPARAM)wParam ,(LPARAM) spDragInfo );
 if( !bResult ) 
      ptrDragInfo->pt = pt;

 return bResult;
}

/*******************************************************************
 *                      DragDetect ( USER.465 )
 *
 */
BOOL16 DragDetect(HWND16 hWnd, POINT16 pt)
{
  MSG   msg;
  RECT16  rect;

  rect.left = pt.x - wDragWidth;
  rect.right = pt.x + wDragWidth;

  rect.top = pt.y - wDragHeight;
  rect.bottom = pt.y + wDragHeight;

  SetCapture(hWnd);

  while(1)
   {
        while(PeekMessage(&msg ,0 ,WM_MOUSEFIRST ,WM_MOUSELAST ,PM_REMOVE))
         {
           if( msg.message == WM_LBUTTONUP )
                {
                  ReleaseCapture();
                  return 0;
                }
           if( msg.message == WM_MOUSEMOVE )
		{
                  if( !PtInRect16( &rect, MAKEPOINT16(msg.lParam) ) )
                    {
                      ReleaseCapture();
                      return 1;
                    }
		}
         }
        WaitMessage();
   }

  return 0;
}

/******************************************************************************
 *                              DragObject ( USER.464 )
 *
 */
DWORD DragObject(HWND hwndScope, HWND hWnd, WORD wObj, HANDLE hOfStruct,
                WORD szList , HCURSOR hCursor)
{
 MSG	 	msg;
 LPDRAGINFO	lpDragInfo;
 SEGPTR		spDragInfo;
 HCURSOR 	hDragCursor=0, hOldCursor=0, hBummer=0;
 HANDLE		hDragInfo  = GlobalAlloc16( GMEM_SHARE | GMEM_ZEROINIT, 2*sizeof(DRAGINFO));
 WND           *wndPtr = WIN_FindWndPtr(hWnd);
 DWORD		dwRet = 0;
 short	 	dragDone = 0;
 HCURSOR	hCurrentCursor = 0;
 HWND		hCurrentWnd = 0;
 WORD	        btemp;

 lpDragInfo = (LPDRAGINFO) GlobalLock16(hDragInfo);
 spDragInfo = (SEGPTR) WIN16_GlobalLock16(hDragInfo);

 if( !lpDragInfo || !spDragInfo ) return 0L;

 hBummer = LoadCursor(0,IDC_BUMMER);

 if( !hBummer || !wndPtr )
   {
        GlobalFree16(hDragInfo);
        return 0L;
   }

 if(hCursor)
   {
	if( !(hDragCursor = CURSORICON_IconToCursor(hCursor)) )
	  {
	   GlobalFree16(hDragInfo);
	   return 0L;
	  }

	if( hDragCursor == hCursor ) hDragCursor = 0;
	else hCursor = hDragCursor;

	hOldCursor = SetCursor(hDragCursor);
   }

 lpDragInfo->hWnd   = hWnd;
 lpDragInfo->hScope = 0;
 lpDragInfo->wFlags = wObj;
 lpDragInfo->hList  = szList; /* near pointer! */
 lpDragInfo->hOfStruct = hOfStruct;
 lpDragInfo->l = 0L; 

 SetCapture(hWnd);
 ShowCursor(1);

 while( !dragDone )
  {
    WaitMessage();

    if( !PeekMessage(&msg,0,WM_MOUSEFIRST,WM_MOUSELAST,PM_REMOVE) )
	 continue;

   *(lpDragInfo+1) = *lpDragInfo;

    lpDragInfo->pt = msg.pt;

    /* update DRAGINFO struct */
    dprintf_msg(stddeb,"drag: lpDI->hScope = %04x\n",lpDragInfo->hScope);

    if( (btemp = (WORD)DRAG_QueryUpdate(hwndScope, spDragInfo)) > 0 )
	 hCurrentCursor = hCursor;
    else
        {
         hCurrentCursor = hBummer;
         lpDragInfo->hScope = 0;
	}
    if( hCurrentCursor )
        SetCursor(hCurrentCursor);

    dprintf_msg(stddeb,"drag: got %04x\n",btemp);

    /* send WM_DRAGLOOP */
    SendMessage16( hWnd, WM_DRAGLOOP, (WPARAM)(hCurrentCursor != hBummer) , 
	                            (LPARAM) spDragInfo );
    /* send WM_DRAGSELECT or WM_DRAGMOVE */
    if( hCurrentWnd != lpDragInfo->hScope )
	{
	 if( hCurrentWnd )
	   SendMessage16( hCurrentWnd, WM_DRAGSELECT, 0, 
		       (LPARAM)MAKELONG(LOWORD(spDragInfo)+sizeof(DRAGINFO),
				        HIWORD(spDragInfo)) );
	 hCurrentWnd = lpDragInfo->hScope;
	 if( hCurrentWnd )
           SendMessage16( hCurrentWnd, WM_DRAGSELECT, 1, (LPARAM)spDragInfo); 
	}
    else
	if( hCurrentWnd )
	   SendMessage16( hCurrentWnd, WM_DRAGMOVE, 0, (LPARAM)spDragInfo);


    /* check if we're done */
    if( msg.message == WM_LBUTTONUP || msg.message == WM_NCLBUTTONUP )
	dragDone = TRUE;
  }

 ReleaseCapture();
 ShowCursor(0);

 if( hCursor )
   {
     SetCursor(hOldCursor);
     if( hDragCursor )
	 DestroyCursor(hDragCursor);
   }

 if( hCurrentCursor != hBummer ) 
	dwRet = SendMessage16( lpDragInfo->hScope, WM_DROPOBJECT, 
			     (WPARAM)hWnd, (LPARAM)spDragInfo );
 GlobalFree16(hDragInfo);

 return dwRet;
}

