/*
 * Window related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wownt32.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "win.h"
#include "user.h"
#include "dce.h"
#include "controls.h"
#include "cursoricon.h"
#include "message.h"
#include "winpos.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);
WINE_DECLARE_DEBUG_CHANNEL(msg);

#define NB_USER_HANDLES  (LAST_USER_HANDLE - FIRST_USER_HANDLE + 1)

/**********************************************************************/

/* Desktop window */
static WND *pWndDesktop = NULL;

static WORD wDragWidth = 4;
static WORD wDragHeight= 3;

static void *user_handles[NB_USER_HANDLES];

/***********************************************************************
 *           create_window_handle
 *
 * Create a window handle with the server.
 */
static WND *create_window_handle( HWND parent, HWND owner, ATOM atom, INT size )
{
    BOOL res;
    user_handle_t handle = 0;
    WORD index;
    WND *win = HeapAlloc( GetProcessHeap(), 0, size );

    if (!win) return NULL;

    USER_Lock();

    SERVER_START_REQ( create_window )
    {
        req->parent = parent;
        req->owner = owner;
        req->atom = atom;
        if ((res = !wine_server_call_err( req ))) handle = reply->handle;
    }
    SERVER_END_REQ;

    if (!res)
    {
        USER_Unlock();
        HeapFree( GetProcessHeap(), 0, win );
        return NULL;
    }
    index = LOWORD(handle) - FIRST_USER_HANDLE;
    assert( index < NB_USER_HANDLES );
    user_handles[index] = win;
    win->hwndSelf = handle;
    win->dwMagic = WND_MAGIC;
    win->irefCount = 1;
    return win;
}


/***********************************************************************
 *           free_window_handle
 *
 * Free a window handle.
 */
static WND *free_window_handle( HWND hwnd )
{
    WND *ptr;
    WORD index = LOWORD(hwnd) - FIRST_USER_HANDLE;

    if (index >= NB_USER_HANDLES) return NULL;
    USER_Lock();
    if ((ptr = user_handles[index]))
    {
        SERVER_START_REQ( destroy_window )
        {
            req->handle = hwnd;
            if (!wine_server_call_err( req ))
                user_handles[index] = NULL;
            else
                ptr = NULL;
        }
        SERVER_END_REQ;
    }
    USER_Unlock();
    if (ptr) HeapFree( GetProcessHeap(), 0, ptr );
    return ptr;
}


/*******************************************************************
 *           list_window_children
 *
 * Build an array of the children of a given window. The array must be
 * freed with HeapFree. Returns NULL when no windows are found.
 */
static HWND *list_window_children( HWND hwnd, ATOM atom, DWORD tid )
{
    HWND *list;
    int size = 32;

    for (;;)
    {
        int count = 0;

        if (!(list = HeapAlloc( GetProcessHeap(), 0, size * sizeof(HWND) ))) break;

        SERVER_START_REQ( get_window_children )
        {
            req->parent = hwnd;
            req->atom = atom;
            req->tid = tid;
            wine_server_set_reply( req, list, (size-1) * sizeof(HWND) );
            if (!wine_server_call( req )) count = reply->count;
        }
        SERVER_END_REQ;
        if (count && count < size)
        {
            list[count] = 0;
            return list;
        }
        HeapFree( GetProcessHeap(), 0, list );
        if (!count) break;
        size = count + 1;  /* restart with a large enough buffer */
    }
    return NULL;
}


/*******************************************************************
 *           send_parent_notify
 */
static void send_parent_notify( HWND hwnd, UINT msg )
{
    if (!(GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD)) return;
    if (GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_NOPARENTNOTIFY) return;
    SendMessageW( GetParent(hwnd), WM_PARENTNOTIFY,
                  MAKEWPARAM( msg, GetWindowLongW( hwnd, GWL_ID )), (LPARAM)hwnd );
}


/*******************************************************************
 *		get_server_window_text
 *
 * Retrieve the window text from the server.
 */
static void get_server_window_text( HWND hwnd, LPWSTR text, INT count )
{
    size_t len = 0;

    SERVER_START_REQ( get_window_text )
    {
        req->handle = hwnd;
        wine_server_set_reply( req, text, (count - 1) * sizeof(WCHAR) );
        if (!wine_server_call_err( req )) len = wine_server_reply_size(reply);
    }
    SERVER_END_REQ;
    text[len / sizeof(WCHAR)] = 0;
}


/***********************************************************************
 *           WIN_GetPtr
 *
 * Return a pointer to the WND structure if local to the process,
 * or WND_OTHER_PROCESS if handle may be valid in other process.
 * If ret value is a valid pointer, it must be released with WIN_ReleasePtr.
 */
WND *WIN_GetPtr( HWND hwnd )
{
    WND * ptr;
    WORD index = LOWORD(hwnd) - FIRST_USER_HANDLE;

    if (index >= NB_USER_HANDLES) return NULL;

    USER_Lock();
    if ((ptr = user_handles[index]))
    {
        if (ptr->dwMagic == WND_MAGIC && (!HIWORD(hwnd) || hwnd == ptr->hwndSelf))
            return ptr;
        ptr = NULL;
    }
    else ptr = WND_OTHER_PROCESS;
    USER_Unlock();
    return ptr;
}


/***********************************************************************
 *           WIN_IsCurrentProcess
 *
 * Check whether a given window belongs to the current process (and return the full handle).
 */
HWND WIN_IsCurrentProcess( HWND hwnd )
{
    WND *ptr;
    HWND ret;

    if (!(ptr = WIN_GetPtr( hwnd )) || ptr == WND_OTHER_PROCESS) return 0;
    ret = ptr->hwndSelf;
    WIN_ReleasePtr( ptr );
    return ret;
}


/***********************************************************************
 *           WIN_IsCurrentThread
 *
 * Check whether a given window belongs to the current thread (and return the full handle).
 */
HWND WIN_IsCurrentThread( HWND hwnd )
{
    WND *ptr;
    HWND ret = 0;

    if ((ptr = WIN_GetPtr( hwnd )) && ptr != WND_OTHER_PROCESS)
    {
        if (ptr->tid == GetCurrentThreadId()) ret = ptr->hwndSelf;
        WIN_ReleasePtr( ptr );
    }
    return ret;
}


/***********************************************************************
 *           WIN_Handle32
 *
 * Convert a 16-bit window handle to a full 32-bit handle.
 */
HWND WIN_Handle32( HWND16 hwnd16 )
{
    WND *ptr;
    HWND hwnd = (HWND)(ULONG_PTR)hwnd16;

    if (hwnd16 <= 1 || hwnd16 == 0xffff) return hwnd;
    /* do sign extension for -2 and -3 */
    if (hwnd16 >= (HWND16)-3) return (HWND)(LONG_PTR)(INT16)hwnd16;

    if (!(ptr = WIN_GetPtr( hwnd ))) return hwnd;

    if (ptr != WND_OTHER_PROCESS)
    {
        hwnd = ptr->hwndSelf;
        WIN_ReleasePtr( ptr );
    }
    else  /* may belong to another process */
    {
        SERVER_START_REQ( get_window_info )
        {
            req->handle = hwnd;
            if (!wine_server_call_err( req )) hwnd = reply->full_handle;
        }
        SERVER_END_REQ;
    }
    return hwnd;
}


/***********************************************************************
 *           WIN_FindWndPtr
 *
 * Return a pointer to the WND structure corresponding to a HWND.
 */
WND * WIN_FindWndPtr( HWND hwnd )
{
    WND * ptr;

    if (!hwnd) return NULL;

    if ((ptr = WIN_GetPtr( hwnd )))
    {
        if (ptr != WND_OTHER_PROCESS)
        {
            /* increment destruction monitoring */
            ptr->irefCount++;
            return ptr;
        }
        if (IsWindow( hwnd )) /* check other processes */
        {
            ERR( "window %p belongs to other process\n", hwnd );
            /* DbgBreakPoint(); */
        }
    }
    SetLastError( ERROR_INVALID_WINDOW_HANDLE );
    return NULL;
}


/***********************************************************************
 *           WIN_ReleaseWndPtr
 *
 * Release the pointer to the WND structure.
 */
void WIN_ReleaseWndPtr(WND *wndPtr)
{
    if(!wndPtr) return;

    /* Decrement destruction monitoring value */
     wndPtr->irefCount--;
     /* Check if it's time to release the memory */
     if(wndPtr->irefCount == 0 && !wndPtr->dwMagic)
     {
         /* Release memory */
         free_window_handle( wndPtr->hwndSelf );
     }
     else if(wndPtr->irefCount < 0)
     {
         /* This else if is useful to monitor the WIN_ReleaseWndPtr function */
         ERR("forgot a Lock on %p somewhere\n",wndPtr);
     }
     /* unlock all WND structures for thread safeness */
     USER_Unlock();
}


/***********************************************************************
 *           WIN_UnlinkWindow
 *
 * Remove a window from the siblings linked list.
 */
void WIN_UnlinkWindow( HWND hwnd )
{
    WIN_LinkWindow( hwnd, 0, 0 );
}


/***********************************************************************
 *           WIN_LinkWindow
 *
 * Insert a window into the siblings linked list.
 * The window is inserted after the specified window, which can also
 * be specified as HWND_TOP or HWND_BOTTOM.
 * If parent is 0, window is unlinked from the tree.
 */
void WIN_LinkWindow( HWND hwnd, HWND parent, HWND hwndInsertAfter )
{
    WND *wndPtr = WIN_GetPtr( hwnd );

    if (!wndPtr) return;
    if (wndPtr == WND_OTHER_PROCESS)
    {
        if (IsWindow(hwnd)) ERR(" cannot link other process window %p\n", hwnd );
        return;
    }

    SERVER_START_REQ( link_window )
    {
        req->handle   = hwnd;
        req->parent   = parent;
        req->previous = hwndInsertAfter;
        if (!wine_server_call( req ))
        {
            if (reply->full_parent) wndPtr->parent = reply->full_parent;
        }

    }
    SERVER_END_REQ;
    WIN_ReleasePtr( wndPtr );
}


/***********************************************************************
 *           WIN_SetOwner
 *
 * Change the owner of a window.
 */
HWND WIN_SetOwner( HWND hwnd, HWND owner )
{
    WND *win = WIN_GetPtr( hwnd );
    HWND ret = 0;

    if (!win) return 0;
    if (win == WND_OTHER_PROCESS)
    {
        if (IsWindow(hwnd)) ERR( "cannot set owner %p on other process window %p\n", owner, hwnd );
        return 0;
    }
    SERVER_START_REQ( set_window_owner )
    {
        req->handle = hwnd;
        req->owner  = owner;
        if (!wine_server_call( req ))
        {
            win->owner = reply->full_owner;
            ret = reply->prev_owner;
        }
    }
    SERVER_END_REQ;
    WIN_ReleasePtr( win );
    return ret;
}


/***********************************************************************
 *           WIN_SetStyle
 *
 * Change the style of a window.
 */
LONG WIN_SetStyle( HWND hwnd, LONG style )
{
    BOOL ok;
    LONG ret = 0;
    WND *win = WIN_GetPtr( hwnd );

    if (!win) return 0;
    if (win == WND_OTHER_PROCESS)
    {
        if (IsWindow(hwnd))
            ERR( "cannot set style %lx on other process window %p\n", style, hwnd );
        return 0;
    }
    if (style == win->dwStyle)
    {
        WIN_ReleasePtr( win );
        return style;
    }
    SERVER_START_REQ( set_window_info )
    {
        req->handle = hwnd;
        req->flags  = SET_WIN_STYLE;
        req->style  = style;
        if ((ok = !wine_server_call( req )))
        {
            ret = reply->old_style;
            win->dwStyle = style;
        }
    }
    SERVER_END_REQ;
    WIN_ReleasePtr( win );
    if (ok && USER_Driver.pSetWindowStyle) USER_Driver.pSetWindowStyle( hwnd, ret );
    return ret;
}


/***********************************************************************
 *           WIN_SetExStyle
 *
 * Change the extended style of a window.
 */
LONG WIN_SetExStyle( HWND hwnd, LONG style )
{
    LONG ret = 0;
    WND *win = WIN_GetPtr( hwnd );

    if (!win) return 0;
    if (win == WND_OTHER_PROCESS)
    {
        if (IsWindow(hwnd))
            ERR( "cannot set exstyle %lx on other process window %p\n", style, hwnd );
        return 0;
    }
    if (style == win->dwExStyle)
    {
        WIN_ReleasePtr( win );
        return style;
    }
    SERVER_START_REQ( set_window_info )
    {
        req->handle   = hwnd;
        req->flags    = SET_WIN_EXSTYLE;
        req->ex_style = style;
        if (!wine_server_call( req ))
        {
            ret = reply->old_ex_style;
            win->dwExStyle = style;
        }
    }
    SERVER_END_REQ;
    WIN_ReleasePtr( win );
    return ret;
}


/***********************************************************************
 *           WIN_SetRectangles
 *
 * Set the window and client rectangles.
 */
void WIN_SetRectangles( HWND hwnd, const RECT *rectWindow, const RECT *rectClient )
{
    WND *win = WIN_GetPtr( hwnd );
    BOOL ret;

    if (!win) return;
    if (win == WND_OTHER_PROCESS)
    {
        if (IsWindow( hwnd )) ERR( "cannot set rectangles of other process window %p\n", hwnd );
        return;
    }
    SERVER_START_REQ( set_window_rectangles )
    {
        req->handle        = hwnd;
        req->window.left   = rectWindow->left;
        req->window.top    = rectWindow->top;
        req->window.right  = rectWindow->right;
        req->window.bottom = rectWindow->bottom;
        req->client.left   = rectClient->left;
        req->client.top    = rectClient->top;
        req->client.right  = rectClient->right;
        req->client.bottom = rectClient->bottom;
        ret = !wine_server_call( req );
    }
    SERVER_END_REQ;
    if (ret)
    {
        win->rectWindow = *rectWindow;
        win->rectClient = *rectClient;

        TRACE( "win %p window (%ld,%ld)-(%ld,%ld) client (%ld,%ld)-(%ld,%ld)\n", hwnd,
               rectWindow->left, rectWindow->top, rectWindow->right, rectWindow->bottom,
               rectClient->left, rectClient->top, rectClient->right, rectClient->bottom );
    }
    WIN_ReleasePtr( win );
}


/***********************************************************************
 *           WIN_GetRectangles
 *
 * Get the window and client rectangles.
 */
BOOL WIN_GetRectangles( HWND hwnd, RECT *rectWindow, RECT *rectClient )
{
    WND *win = WIN_GetPtr( hwnd );
    BOOL ret = TRUE;

    if (!win) return FALSE;
    if (win == WND_OTHER_PROCESS)
    {
        SERVER_START_REQ( get_window_rectangles )
        {
            req->handle = hwnd;
            if ((ret = !wine_server_call( req )))
            {
                if (rectWindow)
                {
                    rectWindow->left   = reply->window.left;
                    rectWindow->top    = reply->window.top;
                    rectWindow->right  = reply->window.right;
                    rectWindow->bottom = reply->window.bottom;
                }
                if (rectClient)
                {
                    rectClient->left   = reply->client.left;
                    rectClient->top    = reply->client.top;
                    rectClient->right  = reply->client.right;
                    rectClient->bottom = reply->client.bottom;
                }
            }
        }
        SERVER_END_REQ;
    }
    else
    {
        if (rectWindow) *rectWindow = win->rectWindow;
        if (rectClient) *rectClient = win->rectClient;
        WIN_ReleasePtr( win );
    }
    return ret;
}


/***********************************************************************
 *           WIN_DestroyWindow
 *
 * Destroy storage associated to a window. "Internals" p.358
 */
LRESULT WIN_DestroyWindow( HWND hwnd )
{
    WND *wndPtr;
    HWND *list;

    TRACE("%p\n", hwnd );

    if (!(hwnd = WIN_IsCurrentThread( hwnd )))
    {
        ERR( "window doesn't belong to current thread\n" );
        return 0;
    }

    /* free child windows */
    if ((list = WIN_ListChildren( hwnd )))
    {
        int i;
        for (i = 0; list[i]; i++)
        {
            if (WIN_IsCurrentThread( list[i] )) WIN_DestroyWindow( list[i] );
            else SendMessageW( list[i], WM_WINE_DESTROYWINDOW, 0, 0 );
        }
        HeapFree( GetProcessHeap(), 0, list );
    }

    /*
     * Clear the update region to make sure no WM_PAINT messages will be
     * generated for this window while processing the WM_NCDESTROY.
     */
    RedrawWindow( hwnd, NULL, 0,
                  RDW_VALIDATE | RDW_NOFRAME | RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_NOCHILDREN);

    /*
     * Send the WM_NCDESTROY to the window being destroyed.
     */
    SendMessageA( hwnd, WM_NCDESTROY, 0, 0);

    /* FIXME: do we need to fake QS_MOUSEMOVE wakebit? */

    WINPOS_CheckInternalPos( hwnd );
    if( hwnd == GetCapture()) ReleaseCapture();

    /* free resources associated with the window */

    TIMER_RemoveWindowTimers( hwnd );

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;

    if (!(wndPtr->dwStyle & WS_CHILD))
    {
        HMENU menu = (HMENU)SetWindowLongW( hwnd, GWL_ID, 0 );
        if (menu) DestroyMenu( menu );
    }
    if (wndPtr->hSysMenu)
    {
	DestroyMenu( wndPtr->hSysMenu );
	wndPtr->hSysMenu = 0;
    }
    DCE_FreeWindowDCE( hwnd );    /* Always do this to catch orphaned DCs */
    USER_Driver.pDestroyWindow( hwnd );
    WINPROC_FreeProc( wndPtr->winproc, WIN_PROC_WINDOW );
    CLASS_RemoveWindow( wndPtr->class );
    wndPtr->class = NULL;
    wndPtr->dwMagic = 0;  /* Mark it as invalid */
    WIN_ReleaseWndPtr( wndPtr );
    return 0;
}

/***********************************************************************
 *           WIN_DestroyThreadWindows
 *
 * Destroy all children of 'wnd' owned by the current thread.
 * Return TRUE if something was done.
 */
void WIN_DestroyThreadWindows( HWND hwnd )
{
    HWND *list;
    int i;

    if (!(list = WIN_ListChildren( hwnd ))) return;
    for (i = 0; list[i]; i++)
    {
        if (WIN_IsCurrentThread( list[i] ))
            DestroyWindow( list[i] );
        else
            WIN_DestroyThreadWindows( list[i] );
    }
    HeapFree( GetProcessHeap(), 0, list );
}

/***********************************************************************
 *           WIN_CreateDesktopWindow
 *
 * Create the desktop window.
 */
BOOL WIN_CreateDesktopWindow(void)
{
    struct tagCLASS *class;
    HWND hwndDesktop;
    INT wndExtra;
    DWORD clsStyle;
    WNDPROC winproc;
    DCE *dce;
    CREATESTRUCTA cs;
    RECT rect;

    TRACE("Creating desktop window\n");

    if (!WINPOS_CreateInternalPosAtom() ||
        !(class = CLASS_AddWindow( (ATOM)LOWORD(DESKTOP_CLASS_ATOM), 0, WIN_PROC_32W,
                                   &wndExtra, &winproc, &clsStyle, &dce )))
        return FALSE;

    pWndDesktop = create_window_handle( 0, 0, LOWORD(DESKTOP_CLASS_ATOM),
                                        sizeof(WND) + wndExtra - sizeof(pWndDesktop->wExtra) );
    if (!pWndDesktop) return FALSE;
    hwndDesktop = pWndDesktop->hwndSelf;

    pWndDesktop->tid               = 0;  /* nobody owns the desktop */
    pWndDesktop->parent            = 0;
    pWndDesktop->owner             = 0;
    pWndDesktop->class             = class;
    pWndDesktop->text              = NULL;
    pWndDesktop->hrgnUpdate        = 0;
    pWndDesktop->clsStyle          = clsStyle;
    pWndDesktop->dce               = NULL;
    pWndDesktop->pVScroll          = NULL;
    pWndDesktop->pHScroll          = NULL;
    pWndDesktop->helpContext       = 0;
    pWndDesktop->flags             = 0;
    pWndDesktop->hSysMenu          = 0;
    pWndDesktop->winproc           = winproc;
    pWndDesktop->cbWndExtra        = wndExtra;

    cs.lpCreateParams = NULL;
    cs.hInstance      = 0;
    cs.hMenu          = 0;
    cs.hwndParent     = 0;
    cs.x              = 0;
    cs.y              = 0;
    cs.cx             = GetSystemMetrics( SM_CXSCREEN );
    cs.cy             = GetSystemMetrics( SM_CYSCREEN );
    cs.style          = pWndDesktop->dwStyle;
    cs.dwExStyle      = pWndDesktop->dwExStyle;
    cs.lpszName       = NULL;
    cs.lpszClass      = DESKTOP_CLASS_ATOM;

    SetRect( &rect, 0, 0, cs.cx, cs.cy );
    WIN_SetRectangles( hwndDesktop, &rect, &rect );

    SERVER_START_REQ( set_window_info )
    {
        req->handle = hwndDesktop;
        req->flags  = 0;  /* don't set anything, just retrieve */
        wine_server_call( req );
        pWndDesktop->dwStyle   = reply->old_style;
        pWndDesktop->dwExStyle = reply->old_ex_style;
        pWndDesktop->hInstance = (HINSTANCE)reply->old_instance;
        pWndDesktop->userdata  = (ULONG_PTR)reply->old_user_data;
        pWndDesktop->wIDmenu   = reply->old_id;
    }
    SERVER_END_REQ;

    if (!USER_Driver.pCreateWindow( hwndDesktop, &cs, FALSE ))
    {
        WIN_ReleaseWndPtr( pWndDesktop );
        return FALSE;
    }

    pWndDesktop->flags |= WIN_NEEDS_ERASEBKGND;
    WIN_ReleaseWndPtr( pWndDesktop );
    return TRUE;
}


/***********************************************************************
 *           WIN_FixCoordinates
 *
 * Fix the coordinates - Helper for WIN_CreateWindowEx.
 * returns default show mode in sw.
 * Note: the feature presented as undocumented *is* in the MSDN since 1993.
 */
static void WIN_FixCoordinates( CREATESTRUCTA *cs, INT *sw)
{
    if (cs->x == CW_USEDEFAULT || cs->x == CW_USEDEFAULT16 ||
        cs->cx == CW_USEDEFAULT || cs->cx == CW_USEDEFAULT16)
    {
        if (cs->style & (WS_CHILD | WS_POPUP))
        {
            if (cs->x == CW_USEDEFAULT || cs->x == CW_USEDEFAULT16) cs->x = cs->y = 0;
            if (cs->cx == CW_USEDEFAULT || cs->cx == CW_USEDEFAULT16) cs->cx = cs->cy = 0;
        }
        else  /* overlapped window */
        {
            STARTUPINFOA info;

            GetStartupInfoA( &info );

            if (cs->x == CW_USEDEFAULT || cs->x == CW_USEDEFAULT16)
            {
                /* Never believe Microsoft's documentation... CreateWindowEx doc says
                 * that if an overlapped window is created with WS_VISIBLE style bit
                 * set and the x parameter is set to CW_USEDEFAULT, the system ignores
                 * the y parameter. However, disassembling NT implementation (WIN32K.SYS)
                 * reveals that
                 *
                 * 1) not only it checks for CW_USEDEFAULT but also for CW_USEDEFAULT16
                 * 2) it does not ignore the y parameter as the docs claim; instead, it
                 *    uses it as second parameter to ShowWindow() unless y is either
                 *    CW_USEDEFAULT or CW_USEDEFAULT16.
                 *
                 * The fact that we didn't do 2) caused bogus windows pop up when wine
                 * was running apps that were using this obscure feature. Example -
                 * calc.exe that comes with Win98 (only Win98, it's different from
                 * the one that comes with Win95 and NT)
                 */
                if (cs->y != CW_USEDEFAULT && cs->y != CW_USEDEFAULT16) *sw = cs->y;
                cs->x = (info.dwFlags & STARTF_USEPOSITION) ? info.dwX : 0;
                cs->y = (info.dwFlags & STARTF_USEPOSITION) ? info.dwY : 0;
            }

            if (cs->cx == CW_USEDEFAULT || cs->cx == CW_USEDEFAULT16)
            {
                if (info.dwFlags & STARTF_USESIZE)
                {
                    cs->cx = info.dwXSize;
                    cs->cy = info.dwYSize;
                }
                else  /* if no other hint from the app, pick 3/4 of the screen real estate */
                {
                    RECT r;
                    SystemParametersInfoA( SPI_GETWORKAREA, 0, &r, 0);
                    cs->cx = (((r.right - r.left) * 3) / 4) - cs->x;
                    cs->cy = (((r.bottom - r.top) * 3) / 4) - cs->y;
                }
            }
        }
    }
    else
    {
	/* neither x nor cx are default. Check the y values .
	 * In the trace we see Outlook and Outlook Express using
	 * cy set to CW_USEDEFAULT when opening the address book.
	 */
	if (cs->cy == CW_USEDEFAULT || cs->cy == CW_USEDEFAULT16) {
	    RECT r;
	    FIXME("Strange use of CW_USEDEFAULT in nHeight\n");
	    SystemParametersInfoA( SPI_GETWORKAREA, 0, &r, 0);
	    cs->cy = (((r.bottom - r.top) * 3) / 4) - cs->y;
	}
    }
}

/***********************************************************************
 *           dump_window_styles
 */
static void dump_window_styles( DWORD style, DWORD exstyle )
{
    TRACE( "style:" );
    if(style & WS_POPUP) TRACE(" WS_POPUP");
    if(style & WS_CHILD) TRACE(" WS_CHILD");
    if(style & WS_MINIMIZE) TRACE(" WS_MINIMIZE");
    if(style & WS_VISIBLE) TRACE(" WS_VISIBLE");
    if(style & WS_DISABLED) TRACE(" WS_DISABLED");
    if(style & WS_CLIPSIBLINGS) TRACE(" WS_CLIPSIBLINGS");
    if(style & WS_CLIPCHILDREN) TRACE(" WS_CLIPCHILDREN");
    if(style & WS_MAXIMIZE) TRACE(" WS_MAXIMIZE");
    if((style & WS_CAPTION) == WS_CAPTION) TRACE(" WS_CAPTION");
    else
    {
        if(style & WS_BORDER) TRACE(" WS_BORDER");
        if(style & WS_DLGFRAME) TRACE(" WS_DLGFRAME");
    }
    if(style & WS_VSCROLL) TRACE(" WS_VSCROLL");
    if(style & WS_HSCROLL) TRACE(" WS_HSCROLL");
    if(style & WS_SYSMENU) TRACE(" WS_SYSMENU");
    if(style & WS_THICKFRAME) TRACE(" WS_THICKFRAME");
    if(style & WS_GROUP) TRACE(" WS_GROUP");
    if(style & WS_TABSTOP) TRACE(" WS_TABSTOP");
    if(style & WS_MINIMIZEBOX) TRACE(" WS_MINIMIZEBOX");
    if(style & WS_MAXIMIZEBOX) TRACE(" WS_MAXIMIZEBOX");

    /* FIXME: Add dumping of BS_/ES_/SBS_/LBS_/CBS_/DS_/etc. styles */
#define DUMPED_STYLES \
    (WS_POPUP | \
     WS_CHILD | \
     WS_MINIMIZE | \
     WS_VISIBLE | \
     WS_DISABLED | \
     WS_CLIPSIBLINGS | \
     WS_CLIPCHILDREN | \
     WS_MAXIMIZE | \
     WS_BORDER | \
     WS_DLGFRAME | \
     WS_VSCROLL | \
     WS_HSCROLL | \
     WS_SYSMENU | \
     WS_THICKFRAME | \
     WS_GROUP | \
     WS_TABSTOP | \
     WS_MINIMIZEBOX | \
     WS_MAXIMIZEBOX)

    if(style & ~DUMPED_STYLES) TRACE(" %08lx", style & ~DUMPED_STYLES);
    TRACE("\n");
#undef DUMPED_STYLES

    TRACE( "exstyle:" );
    if(exstyle & WS_EX_DLGMODALFRAME) TRACE(" WS_EX_DLGMODALFRAME");
    if(exstyle & WS_EX_DRAGDETECT) TRACE(" WS_EX_DRAGDETECT");
    if(exstyle & WS_EX_NOPARENTNOTIFY) TRACE(" WS_EX_NOPARENTNOTIFY");
    if(exstyle & WS_EX_TOPMOST) TRACE(" WS_EX_TOPMOST");
    if(exstyle & WS_EX_ACCEPTFILES) TRACE(" WS_EX_ACCEPTFILES");
    if(exstyle & WS_EX_TRANSPARENT) TRACE(" WS_EX_TRANSPARENT");
    if(exstyle & WS_EX_MDICHILD) TRACE(" WS_EX_MDICHILD");
    if(exstyle & WS_EX_TOOLWINDOW) TRACE(" WS_EX_TOOLWINDOW");
    if(exstyle & WS_EX_WINDOWEDGE) TRACE(" WS_EX_WINDOWEDGE");
    if(exstyle & WS_EX_CLIENTEDGE) TRACE(" WS_EX_CLIENTEDGE");
    if(exstyle & WS_EX_CONTEXTHELP) TRACE(" WS_EX_CONTEXTHELP");
    if(exstyle & WS_EX_RIGHT) TRACE(" WS_EX_RIGHT");
    if(exstyle & WS_EX_RTLREADING) TRACE(" WS_EX_RTLREADING");
    if(exstyle & WS_EX_LEFTSCROLLBAR) TRACE(" WS_EX_LEFTSCROLLBAR");
    if(exstyle & WS_EX_CONTROLPARENT) TRACE(" WS_EX_CONTROLPARENT");
    if(exstyle & WS_EX_STATICEDGE) TRACE(" WS_EX_STATICEDGE");
    if(exstyle & WS_EX_APPWINDOW) TRACE(" WS_EX_APPWINDOW");
    if(exstyle & WS_EX_LAYERED) TRACE(" WS_EX_LAYERED");

#define DUMPED_EX_STYLES \
    (WS_EX_DLGMODALFRAME | \
     WS_EX_DRAGDETECT | \
     WS_EX_NOPARENTNOTIFY | \
     WS_EX_TOPMOST | \
     WS_EX_ACCEPTFILES | \
     WS_EX_TRANSPARENT | \
     WS_EX_MDICHILD | \
     WS_EX_TOOLWINDOW | \
     WS_EX_WINDOWEDGE | \
     WS_EX_CLIENTEDGE | \
     WS_EX_CONTEXTHELP | \
     WS_EX_RIGHT | \
     WS_EX_RTLREADING | \
     WS_EX_LEFTSCROLLBAR | \
     WS_EX_CONTROLPARENT | \
     WS_EX_STATICEDGE | \
     WS_EX_APPWINDOW | \
     WS_EX_LAYERED)

    if(exstyle & ~DUMPED_EX_STYLES) TRACE(" %08lx", exstyle & ~DUMPED_EX_STYLES);
    TRACE("\n");
#undef DUMPED_EX_STYLES
}


/***********************************************************************
 *           WIN_CreateWindowEx
 *
 * Implementation of CreateWindowEx().
 */
static HWND WIN_CreateWindowEx( CREATESTRUCTA *cs, ATOM classAtom,
				WINDOWPROCTYPE type )
{
    INT sw = SW_SHOW;
    struct tagCLASS *classPtr;
    WND *wndPtr;
    HWND hwnd, parent, owner;
    INT wndExtra;
    DWORD clsStyle;
    WNDPROC winproc;
    DCE *dce;
    BOOL unicode = (type == WIN_PROC_32W);

    TRACE("%s %s ex=%08lx style=%08lx %d,%d %dx%d parent=%p menu=%p inst=%p params=%p\n",
          (type == WIN_PROC_32W) ? debugstr_w((LPWSTR)cs->lpszName) : debugstr_a(cs->lpszName),
          (type == WIN_PROC_32W) ? debugstr_w((LPWSTR)cs->lpszClass) : debugstr_a(cs->lpszClass),
          cs->dwExStyle, cs->style, cs->x, cs->y, cs->cx, cs->cy,
          cs->hwndParent, cs->hMenu, cs->hInstance, cs->lpCreateParams );

    if(TRACE_ON(win)) dump_window_styles( cs->style, cs->dwExStyle );

    TRACE("winproc type is %d (%s)\n", type, (type == WIN_PROC_16) ? "WIN_PROC_16" :
	    ((type == WIN_PROC_32A) ? "WIN_PROC_32A" : "WIN_PROC_32W") );

    /* Find the parent window */

    parent = GetDesktopWindow();
    owner = 0;

    if (cs->hwndParent == HWND_MESSAGE)
    {
      /* native ole32.OleInitialize uses HWND_MESSAGE to create the
       * message window (style: WS_POPUP|WS_DISABLED)
       */
      FIXME("Parent is HWND_MESSAGE\n");
    }
    else if (cs->hwndParent)
    {
	/* Make sure parent is valid */
        if (!IsWindow( cs->hwndParent ))
        {
            WARN("Bad parent %p\n", cs->hwndParent );
	    return 0;
	}
        if ((cs->style & (WS_CHILD|WS_POPUP)) == WS_CHILD)
            parent = WIN_GetFullHandle(cs->hwndParent);
        else
            owner = GetAncestor( cs->hwndParent, GA_ROOT );
    }
    else if ((cs->style & (WS_CHILD|WS_POPUP)) == WS_CHILD)
    {
        WARN("No parent for child window\n" );
        return 0;  /* WS_CHILD needs a parent, but WS_POPUP doesn't */
    }

    /* Find the window class */
    if (!(classPtr = CLASS_AddWindow( classAtom, cs->hInstance, type,
                                      &wndExtra, &winproc, &clsStyle, &dce )))
    {
        WARN("Bad class '%s'\n", cs->lpszClass );
        return 0;
    }

    WIN_FixCoordinates(cs, &sw); /* fix default coordinates */

    /* Correct the window style - stage 1
     *
     * These are patches that appear to affect both the style loaded into the
     * WIN structure and passed in the CreateStruct to the WM_CREATE etc.
     *
     * WS_EX_WINDOWEDGE appears to be enforced based on the other styles, so
     * why does the user get to set it?
     */

    /* This has been tested for WS_CHILD | WS_VISIBLE.  It has not been
     * tested for WS_POPUP
     */
    if ((cs->dwExStyle & WS_EX_DLGMODALFRAME) ||
        ((!(cs->dwExStyle & WS_EX_STATICEDGE)) &&
          (cs->style & (WS_DLGFRAME | WS_THICKFRAME))))
        cs->dwExStyle |= WS_EX_WINDOWEDGE;
    else
        cs->dwExStyle &= ~WS_EX_WINDOWEDGE;

    /* Create the window structure */

    if (!(wndPtr = create_window_handle( parent, owner, classAtom,
                                         sizeof(*wndPtr) + wndExtra - sizeof(wndPtr->wExtra) )))
    {
	TRACE("out of memory\n" );
	return 0;
    }
    hwnd = wndPtr->hwndSelf;

    /* Fill the window structure */

    wndPtr->tid            = GetCurrentThreadId();
    wndPtr->owner          = owner;
    wndPtr->parent         = parent;
    wndPtr->class          = classPtr;
    wndPtr->winproc        = winproc;
    wndPtr->hInstance      = cs->hInstance;
    wndPtr->text           = NULL;
    wndPtr->hrgnUpdate     = 0;
    wndPtr->hrgnWnd        = 0;
    wndPtr->dwStyle        = cs->style & ~WS_VISIBLE;
    wndPtr->dwExStyle      = cs->dwExStyle;
    wndPtr->clsStyle       = clsStyle;
    wndPtr->wIDmenu        = 0;
    wndPtr->helpContext    = 0;
    wndPtr->flags          = (type == WIN_PROC_16) ? 0 : WIN_ISWIN32;
    wndPtr->pVScroll       = NULL;
    wndPtr->pHScroll       = NULL;
    wndPtr->userdata       = 0;
    wndPtr->hSysMenu       = (wndPtr->dwStyle & WS_SYSMENU) ? MENU_GetSysMenu( hwnd, 0 ) : 0;
    wndPtr->cbWndExtra     = wndExtra;

    if (wndExtra) memset( wndPtr->wExtra, 0, wndExtra);

    /* Correct the window style - stage 2 */

    if (!(cs->style & WS_CHILD))
    {
	wndPtr->dwStyle |= WS_CLIPSIBLINGS;
	if (!(cs->style & WS_POPUP))
	{
            wndPtr->dwStyle |= WS_CAPTION;
            wndPtr->flags |= WIN_NEED_SIZE;
	}
    }
    SERVER_START_REQ( set_window_info )
    {
        req->handle    = hwnd;
        req->flags     = SET_WIN_STYLE | SET_WIN_EXSTYLE | SET_WIN_INSTANCE;
        req->style     = wndPtr->dwStyle;
        req->ex_style  = wndPtr->dwExStyle;
        req->instance  = (void *)wndPtr->hInstance;
        wine_server_call( req );
    }
    SERVER_END_REQ;

    /* Get class or window DC if needed */

    if (clsStyle & CS_OWNDC) wndPtr->dce = DCE_AllocDCE(hwnd,DCE_WINDOW_DC);
    else if (clsStyle & CS_CLASSDC) wndPtr->dce = dce;
    else wndPtr->dce = NULL;

    /* Set the window menu */

    if (((wndPtr->dwStyle & (WS_CAPTION|WS_CHILD)) == WS_CAPTION) ||
        (wndPtr->dwExStyle & WS_EX_APPWINDOW))
    {
        if (cs->hMenu) SetMenu(hwnd, cs->hMenu);
        else
        {
            LPCSTR menuName = (LPCSTR)GetClassLongA( hwnd, GCL_MENUNAME );
            if (menuName)
            {
                if (HIWORD(cs->hInstance))
                    cs->hMenu = LoadMenuA(cs->hInstance,menuName);
                else
                    cs->hMenu = HMENU_32(LoadMenu16(HINSTANCE_16(cs->hInstance),menuName));

                if (cs->hMenu) SetMenu( hwnd, cs->hMenu );
            }
        }
    }
    else SetWindowLongW( hwnd, GWL_ID, (UINT)cs->hMenu );
    WIN_ReleaseWndPtr( wndPtr );

    if (!USER_Driver.pCreateWindow( hwnd, cs, unicode))
    {
        WIN_DestroyWindow( hwnd );
        return 0;
    }

    /* Notify the parent window only */

    send_parent_notify( hwnd, WM_CREATE );
    if (!IsWindow( hwnd )) return 0;

    if (cs->style & WS_VISIBLE)
    {
        /* in case WS_VISIBLE got set in the meantime */
        if (!(wndPtr = WIN_GetPtr( hwnd ))) return 0;
        WIN_SetStyle( hwnd, wndPtr->dwStyle & ~WS_VISIBLE );
        WIN_ReleasePtr( wndPtr );
        ShowWindow( hwnd, sw );
    }

    /* Call WH_SHELL hook */

    if (!(GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD) && !GetWindow( hwnd, GW_OWNER ))
        HOOK_CallHooks( WH_SHELL, HSHELL_WINDOWCREATED, (WPARAM)hwnd, 0, TRUE );

    TRACE("created window %p\n", hwnd);
    return hwnd;
}


/***********************************************************************
 *		CreateWindow (USER.41)
 */
HWND16 WINAPI CreateWindow16( LPCSTR className, LPCSTR windowName,
                              DWORD style, INT16 x, INT16 y, INT16 width,
                              INT16 height, HWND16 parent, HMENU16 menu,
                              HINSTANCE16 instance, LPVOID data )
{
    return CreateWindowEx16( 0, className, windowName, style,
			   x, y, width, height, parent, menu, instance, data );
}


/***********************************************************************
 *		CreateWindowEx (USER.452)
 */
HWND16 WINAPI CreateWindowEx16( DWORD exStyle, LPCSTR className,
                                LPCSTR windowName, DWORD style, INT16 x,
                                INT16 y, INT16 width, INT16 height,
                                HWND16 parent, HMENU16 menu,
                                HINSTANCE16 instance, LPVOID data )
{
    ATOM classAtom;
    CREATESTRUCTA cs;
    char buffer[256];

    /* Find the class atom */

    if (HIWORD(className))
    {
        if (!(classAtom = GlobalFindAtomA( className )))
        {
            ERR( "bad class name %s\n", debugstr_a(className) );
            return 0;
        }
    }
    else
    {
        classAtom = LOWORD(className);
        if (!GlobalGetAtomNameA( classAtom, buffer, sizeof(buffer) ))
        {
            ERR( "bad atom %x\n", classAtom);
            return 0;
        }
        className = buffer;
    }

    /* Fix the coordinates */

    cs.x  = (x == CW_USEDEFAULT16) ? CW_USEDEFAULT : (INT)x;
    cs.y  = (y == CW_USEDEFAULT16) ? CW_USEDEFAULT : (INT)y;
    cs.cx = (width == CW_USEDEFAULT16) ? CW_USEDEFAULT : (INT)width;
    cs.cy = (height == CW_USEDEFAULT16) ? CW_USEDEFAULT : (INT)height;

    /* Create the window */

    cs.lpCreateParams = data;
    cs.hInstance      = HINSTANCE_32(instance);
    cs.hMenu          = HMENU_32(menu);
    cs.hwndParent     = WIN_Handle32( parent );
    cs.style          = style;
    cs.lpszName       = windowName;
    cs.lpszClass      = className;
    cs.dwExStyle      = exStyle;

    return HWND_16( WIN_CreateWindowEx( &cs, classAtom, WIN_PROC_16 ));
}


/***********************************************************************
 *		CreateWindowExA (USER32.@)
 */
HWND WINAPI CreateWindowExA( DWORD exStyle, LPCSTR className,
                                 LPCSTR windowName, DWORD style, INT x,
                                 INT y, INT width, INT height,
                                 HWND parent, HMENU menu,
                                 HINSTANCE instance, LPVOID data )
{
    ATOM classAtom;
    CREATESTRUCTA cs;
    char buffer[256];

    if(!instance)
        instance=GetModuleHandleA(NULL);

    if(exStyle & WS_EX_MDICHILD)
        return CreateMDIWindowA(className, windowName, style, x, y, width, height, parent, instance, (LPARAM)data);

    /* Find the class atom */

    if (HIWORD(className))
    {
        if (!(classAtom = GlobalFindAtomA( className )))
        {
            ERR( "bad class name %s\n", debugstr_a(className) );
            return 0;
        }
    }
    else
    {
        classAtom = LOWORD(className);
        if (!GlobalGetAtomNameA( classAtom, buffer, sizeof(buffer) ))
        {
            ERR( "bad atom %x\n", classAtom);
            return 0;
        }
        className = buffer;
    }

    /* Create the window */

    cs.lpCreateParams = data;
    cs.hInstance      = instance;
    cs.hMenu          = menu;
    cs.hwndParent     = parent;
    cs.x              = x;
    cs.y              = y;
    cs.cx             = width;
    cs.cy             = height;
    cs.style          = style;
    cs.lpszName       = windowName;
    cs.lpszClass      = className;
    cs.dwExStyle      = exStyle;

    return WIN_CreateWindowEx( &cs, classAtom, WIN_PROC_32A );
}


/***********************************************************************
 *		CreateWindowExW (USER32.@)
 */
HWND WINAPI CreateWindowExW( DWORD exStyle, LPCWSTR className,
                                 LPCWSTR windowName, DWORD style, INT x,
                                 INT y, INT width, INT height,
                                 HWND parent, HMENU menu,
                                 HINSTANCE instance, LPVOID data )
{
    ATOM classAtom;
    CREATESTRUCTW cs;
    WCHAR buffer[256];

    if(!instance)
        instance=GetModuleHandleW(NULL);

    if(exStyle & WS_EX_MDICHILD)
        return CreateMDIWindowW(className, windowName, style, x, y, width, height, parent, instance, (LPARAM)data);

    /* Find the class atom */

    if (HIWORD(className))
    {
        if (!(classAtom = GlobalFindAtomW( className )))
        {
            ERR( "bad class name %s\n", debugstr_w(className) );
            return 0;
        }
    }
    else
    {
        classAtom = LOWORD(className);
        if (!GlobalGetAtomNameW( classAtom, buffer, sizeof(buffer)/sizeof(WCHAR) ))
        {
            ERR( "bad atom %x\n", classAtom);
            return 0;
        }
        className = buffer;
    }

    /* Create the window */

    cs.lpCreateParams = data;
    cs.hInstance      = instance;
    cs.hMenu          = menu;
    cs.hwndParent     = parent;
    cs.x              = x;
    cs.y              = y;
    cs.cx             = width;
    cs.cy             = height;
    cs.style          = style;
    cs.lpszName       = windowName;
    cs.lpszClass      = className;
    cs.dwExStyle      = exStyle;

    /* Note: we rely on the fact that CREATESTRUCTA and */
    /* CREATESTRUCTW have the same layout. */
    return WIN_CreateWindowEx( (CREATESTRUCTA *)&cs, classAtom, WIN_PROC_32W );
}


/***********************************************************************
 *           WIN_SendDestroyMsg
 */
static void WIN_SendDestroyMsg( HWND hwnd )
{
    GUITHREADINFO info;

    if (GetGUIThreadInfo( GetCurrentThreadId(), &info ))
    {
        if (hwnd == info.hwndCaret) DestroyCaret();
    }
    if (USER_Driver.pResetSelectionOwner)
        USER_Driver.pResetSelectionOwner( hwnd, TRUE );

    /*
     * Send the WM_DESTROY to the window.
     */
    SendMessageA( hwnd, WM_DESTROY, 0, 0);

    /*
     * This WM_DESTROY message can trigger re-entrant calls to DestroyWindow
     * make sure that the window still exists when we come back.
     */
    if (IsWindow(hwnd))
    {
        HWND* pWndArray;
        int i;

        if (!(pWndArray = WIN_ListChildren( hwnd ))) return;

        /* start from the end (FIXME: is this needed?) */
        for (i = 0; pWndArray[i]; i++) ;

        while (--i >= 0)
        {
            if (IsWindow( pWndArray[i] )) WIN_SendDestroyMsg( pWndArray[i] );
        }
        HeapFree( GetProcessHeap(), 0, pWndArray );
    }
    else
      WARN("\tdestroyed itself while in WM_DESTROY!\n");
}


/***********************************************************************
 *		DestroyWindow (USER32.@)
 */
BOOL WINAPI DestroyWindow( HWND hwnd )
{
    BOOL is_child;
    HWND h;

    if (!(hwnd = WIN_IsCurrentThread( hwnd )) || (hwnd == GetDesktopWindow()))
    {
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }

    TRACE("(%p)\n", hwnd);

    /* Look whether the focus is within the tree of windows we will
     * be destroying.
     */
    h = GetFocus();
    if (h == hwnd || IsChild( hwnd, h ))
    {
        HWND parent = GetAncestor( hwnd, GA_PARENT );
        if (parent == GetDesktopWindow()) parent = 0;
        SetFocus( parent );
    }

      /* Call hooks */

    if (HOOK_CallHooks( WH_CBT, HCBT_DESTROYWND, (WPARAM)hwnd, 0, TRUE )) return FALSE;

    is_child = (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD) != 0;

    if (is_child)
    {
        if (!USER_IsExitingThread( GetCurrentThreadId() ))
            send_parent_notify( hwnd, WM_DESTROY );
    }
    else if (!GetWindow( hwnd, GW_OWNER ))
    {
        HOOK_CallHooks( WH_SHELL, HSHELL_WINDOWDESTROYED, (WPARAM)hwnd, 0L, TRUE );
        /* FIXME: clean up palette - see "Internals" p.352 */
    }

    if (!IsWindow(hwnd)) return TRUE;

    if (USER_Driver.pResetSelectionOwner)
        USER_Driver.pResetSelectionOwner( hwnd, FALSE ); /* before the window is unmapped */

      /* Hide the window */

    if (!ShowWindow( hwnd, SW_HIDE ))
    {
        if (hwnd == GetActiveWindow()) WINPOS_ActivateOtherWindow( hwnd );
    }
    if (!IsWindow(hwnd)) return TRUE;

      /* Recursively destroy owned windows */

    if (!is_child)
    {
        for (;;)
        {
            int i, got_one = 0;
            HWND *list = WIN_ListChildren( GetDesktopWindow() );
            if (list)
            {
                for (i = 0; list[i]; i++)
                {
                    if (GetWindow( list[i], GW_OWNER ) != hwnd) continue;
                    if (WIN_IsCurrentThread( list[i] ))
                    {
                        DestroyWindow( list[i] );
                        got_one = 1;
                        continue;
                    }
                    WIN_SetOwner( list[i], 0 );
                }
                HeapFree( GetProcessHeap(), 0, list );
            }
            if (!got_one) break;
        }
    }

      /* Send destroy messages */

    WIN_SendDestroyMsg( hwnd );
    if (!IsWindow( hwnd )) return TRUE;

    if (GetClipboardOwner() == hwnd)
        CLIPBOARD_ReleaseOwner();

      /* Unlink now so we won't bother with the children later on */

    WIN_UnlinkWindow( hwnd );

      /* Destroy the window storage */

    WIN_DestroyWindow( hwnd );
    return TRUE;
}


/***********************************************************************
 *		CloseWindow (USER32.@)
 */
BOOL WINAPI CloseWindow( HWND hwnd )
{
    if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD) return FALSE;
    ShowWindow( hwnd, SW_MINIMIZE );
    return TRUE;
}


/***********************************************************************
 *		OpenIcon (USER32.@)
 */
BOOL WINAPI OpenIcon( HWND hwnd )
{
    if (!IsIconic( hwnd )) return FALSE;
    ShowWindow( hwnd, SW_SHOWNORMAL );
    return TRUE;
}


/***********************************************************************
 *           WIN_FindWindow
 *
 * Implementation of FindWindow() and FindWindowEx().
 */
static HWND WIN_FindWindow( HWND parent, HWND child, ATOM className, LPCWSTR title )
{
    HWND *list = NULL;
    HWND retvalue = 0;
    int i = 0, len = 0;
    WCHAR *buffer = NULL;

    if (!parent) parent = GetDesktopWindow();
    if (title)
    {
        len = strlenW(title) + 1;  /* one extra char to check for chars beyond the end */
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) return 0;
    }

    if (!(list = list_window_children( parent, className, 0 ))) goto done;

    if (child)
    {
        child = WIN_GetFullHandle( child );
        while (list[i] && list[i] != child) i++;
        if (!list[i]) goto done;
        i++;  /* start from next window */
    }

    if (title)
    {
        while (list[i])
        {
            if (GetWindowTextW( list[i], buffer, len ) && !strcmpiW( buffer, title )) break;
            i++;
        }
    }
    retvalue = list[i];

 done:
    if (list) HeapFree( GetProcessHeap(), 0, list );
    if (buffer) HeapFree( GetProcessHeap(), 0, buffer );
    return retvalue;
}



/***********************************************************************
 *		FindWindowA (USER32.@)
 */
HWND WINAPI FindWindowA( LPCSTR className, LPCSTR title )
{
    HWND ret = FindWindowExA( 0, 0, className, title );
    if (!ret) SetLastError (ERROR_CANNOT_FIND_WND_CLASS);
    return ret;
}


/***********************************************************************
 *		FindWindowExA (USER32.@)
 */
HWND WINAPI FindWindowExA( HWND parent, HWND child,
                               LPCSTR className, LPCSTR title )
{
    ATOM atom = 0;
    LPWSTR buffer;
    HWND hwnd;
    INT len;

    if (className)
    {
        /* If the atom doesn't exist, then no class */
        /* with this name exists either. */
        if (!(atom = GlobalFindAtomA( className )))
        {
            SetLastError (ERROR_CANNOT_FIND_WND_CLASS);
            return 0;
        }
    }
    if (!title) return WIN_FindWindow( parent, child, atom, NULL );

    len = MultiByteToWideChar( CP_ACP, 0, title, -1, NULL, 0 );
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return 0;
    MultiByteToWideChar( CP_ACP, 0, title, -1, buffer, len );
    hwnd = WIN_FindWindow( parent, child, atom, buffer );
    HeapFree( GetProcessHeap(), 0, buffer );
    return hwnd;
}


/***********************************************************************
 *		FindWindowExW (USER32.@)
 */
HWND WINAPI FindWindowExW( HWND parent, HWND child,
                               LPCWSTR className, LPCWSTR title )
{
    ATOM atom = 0;

    if (className)
    {
        /* If the atom doesn't exist, then no class */
        /* with this name exists either. */
        if (!(atom = GlobalFindAtomW( className )))
        {
            SetLastError (ERROR_CANNOT_FIND_WND_CLASS);
            return 0;
        }
    }
    return WIN_FindWindow( parent, child, atom, title );
}


/***********************************************************************
 *		FindWindowW (USER32.@)
 */
HWND WINAPI FindWindowW( LPCWSTR className, LPCWSTR title )
{
    return FindWindowExW( 0, 0, className, title );
}


/**********************************************************************
 *		GetDesktopWindow (USER32.@)
 */
HWND WINAPI GetDesktopWindow(void)
{
    if (pWndDesktop) return pWndDesktop->hwndSelf;
    ERR( "Wine init error: either you're trying to use an invalid native USER.EXE config, or some graphics/GUI libraries or DLLs didn't initialize properly. Aborting.\n" );
    ExitProcess(1);
    return 0;
}


/*******************************************************************
 *		EnableWindow (USER32.@)
 */
BOOL WINAPI EnableWindow( HWND hwnd, BOOL enable )
{
    WND *wndPtr;
    BOOL retvalue;
    LONG style;
    HWND full_handle;

    if (is_broadcast(hwnd))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!(full_handle = WIN_IsCurrentThread( hwnd )))
        return SendMessageW( hwnd, WM_WINE_ENABLEWINDOW, enable, 0 );

    hwnd = full_handle;

    TRACE("( %p, %d )\n", hwnd, enable);

    if (!(wndPtr = WIN_GetPtr( hwnd ))) return FALSE;
    style = wndPtr->dwStyle;
    retvalue = ((style & WS_DISABLED) != 0);
    WIN_ReleasePtr( wndPtr );

    if (enable && retvalue)
    {
        WIN_SetStyle( hwnd, style & ~WS_DISABLED );
        SendMessageA( hwnd, WM_ENABLE, TRUE, 0 );
    }
    else if (!enable && !retvalue)
    {
        HWND focus_wnd, capture_wnd;

        SendMessageA( hwnd, WM_CANCELMODE, 0, 0);

        WIN_SetStyle( hwnd, style | WS_DISABLED );

        focus_wnd = GetFocus();
        if (hwnd == focus_wnd || IsChild(hwnd, focus_wnd))
            SetFocus( 0 );  /* A disabled window can't have the focus */

        capture_wnd = GetCapture();
        if (hwnd == capture_wnd || IsChild(hwnd, capture_wnd))
            ReleaseCapture();  /* A disabled window can't capture the mouse */

        SendMessageA( hwnd, WM_ENABLE, FALSE, 0 );
    }
    return retvalue;
}


/***********************************************************************
 *		IsWindowEnabled (USER32.@)
 */
BOOL WINAPI IsWindowEnabled(HWND hWnd)
{
    return !(GetWindowLongW( hWnd, GWL_STYLE ) & WS_DISABLED);
}


/***********************************************************************
 *		IsWindowUnicode (USER32.@)
 */
BOOL WINAPI IsWindowUnicode( HWND hwnd )
{
    WND * wndPtr;
    BOOL retvalue;

    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return FALSE;
    retvalue = (WINPROC_GetProcType( wndPtr->winproc ) == WIN_PROC_32W);
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/**********************************************************************
 *		GetWindowWord (USER32.@)
 */
WORD WINAPI GetWindowWord( HWND hwnd, INT offset )
{
    if (offset >= 0)
    {
        WORD retvalue = 0;
        WND *wndPtr = WIN_GetPtr( hwnd );
        if (!wndPtr)
        {
            SetLastError( ERROR_INVALID_WINDOW_HANDLE );
            return 0;
        }
        if (wndPtr == WND_OTHER_PROCESS)
        {
            if (IsWindow( hwnd ))
                FIXME( "(%d) not supported yet on other process window %p\n", offset, hwnd );
            SetLastError( ERROR_INVALID_WINDOW_HANDLE );
            return 0;
        }
        if (offset > (int)(wndPtr->cbWndExtra - sizeof(WORD)))
        {
            WARN("Invalid offset %d\n", offset );
            SetLastError( ERROR_INVALID_INDEX );
        }
        else retvalue = *(WORD *)(((char *)wndPtr->wExtra) + offset);
        WIN_ReleasePtr( wndPtr );
        return retvalue;
    }

    switch(offset)
    {
    case GWL_HWNDPARENT:
        return GetWindowLongW( hwnd, offset );
    case GWL_ID:
    case GWL_HINSTANCE:
        {
            LONG ret = GetWindowLongW( hwnd, offset );
            if (HIWORD(ret))
                WARN("%d: discards high bits of 0x%08lx!\n", offset, ret );
            return LOWORD(ret);
        }
    default:
        WARN("Invalid offset %d\n", offset );
        return 0;
    }
}


/**********************************************************************
 *		SetWindowWord (USER32.@)
 */
WORD WINAPI SetWindowWord( HWND hwnd, INT offset, WORD newval )
{
    WORD *ptr, retval;
    WND * wndPtr;

    switch(offset)
    {
    case GWL_ID:
    case GWL_HINSTANCE:
    case GWL_HWNDPARENT:
        return SetWindowLongW( hwnd, offset, (UINT)newval );
    default:
        if (offset < 0)
        {
            WARN("Invalid offset %d\n", offset );
            SetLastError( ERROR_INVALID_INDEX );
            return 0;
        }
    }

    wndPtr = WIN_GetPtr( hwnd );
    if (wndPtr == WND_OTHER_PROCESS)
    {
        if (IsWindow(hwnd))
            FIXME( "set %d <- %x not supported yet on other process window %p\n",
                   offset, newval, hwnd );
        wndPtr = NULL;
    }
    if (!wndPtr)
    {
       SetLastError( ERROR_INVALID_WINDOW_HANDLE );
       return 0;
    }

    if (offset > (int)(wndPtr->cbWndExtra - sizeof(WORD)))
    {
        WARN("Invalid offset %d\n", offset );
        WIN_ReleasePtr(wndPtr);
        SetLastError( ERROR_INVALID_INDEX );
        return 0;
    }
    ptr = (WORD *)(((char *)wndPtr->wExtra) + offset);
    retval = *ptr;
    *ptr = newval;
    WIN_ReleasePtr(wndPtr);
    return retval;
}


/**********************************************************************
 *	     WIN_GetWindowLong
 *
 * Helper function for GetWindowLong().
 */
static LONG WIN_GetWindowLong( HWND hwnd, INT offset, WINDOWPROCTYPE type )
{
    LONG retvalue = 0;
    WND *wndPtr;

    if (offset == GWL_HWNDPARENT)
    {
        HWND parent = GetAncestor( hwnd, GA_PARENT );
        if (parent == GetDesktopWindow()) parent = GetWindow( hwnd, GW_OWNER );
        return (LONG)parent;
    }

    if (!(wndPtr = WIN_GetPtr( hwnd )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    if (wndPtr == WND_OTHER_PROCESS)
    {
        if (offset >= 0)
        {
            if (IsWindow(hwnd))
                FIXME( "(%d) not supported on other process window %p\n", offset, hwnd );
            SetLastError( ERROR_INVALID_WINDOW_HANDLE );
            return 0;
        }
        if (offset == GWL_WNDPROC)
        {
            SetLastError( ERROR_ACCESS_DENIED );
            return 0;
        }
        SERVER_START_REQ( set_window_info )
        {
            req->handle = hwnd;
            req->flags  = 0;  /* don't set anything, just retrieve */
            if (!wine_server_call_err( req ))
            {
                switch(offset)
                {
                case GWL_STYLE:     retvalue = reply->old_style; break;
                case GWL_EXSTYLE:   retvalue = reply->old_ex_style; break;
                case GWL_ID:        retvalue = reply->old_id; break;
                case GWL_HINSTANCE: retvalue = (ULONG_PTR)reply->old_instance; break;
                case GWL_USERDATA:  retvalue = (ULONG_PTR)reply->old_user_data; break;
                default:
                    SetLastError( ERROR_INVALID_INDEX );
                    break;
                }
            }
        }
        SERVER_END_REQ;
        return retvalue;
    }

    /* now we have a valid wndPtr */

    if (offset >= 0)
    {
        if (offset > (int)(wndPtr->cbWndExtra - sizeof(LONG)))
        {
          /*
            * Some programs try to access last element from 16 bit
            * code using illegal offset value. Hopefully this is
            * what those programs really expect.
            */
           if (type == WIN_PROC_16 &&
               wndPtr->cbWndExtra >= 4 &&
               offset == wndPtr->cbWndExtra - sizeof(WORD))
           {
               INT offset2 = wndPtr->cbWndExtra - sizeof(LONG);

               ERR( "- replaced invalid offset %d with %d\n",
                    offset, offset2 );

                retvalue = *(LONG *)(((char *)wndPtr->wExtra) + offset2);
                WIN_ReleasePtr( wndPtr );
                return retvalue;
            }
            WARN("Invalid offset %d\n", offset );
            WIN_ReleasePtr( wndPtr );
            SetLastError( ERROR_INVALID_INDEX );
            return 0;
        }
        retvalue = *(LONG *)(((char *)wndPtr->wExtra) + offset);
        /* Special case for dialog window procedure */
        if ((offset == DWL_DLGPROC) && (wndPtr->flags & WIN_ISDIALOG))
            retvalue = (LONG)WINPROC_GetProc( (WNDPROC)retvalue, type );
        WIN_ReleasePtr( wndPtr );
        return retvalue;
    }

    switch(offset)
    {
    case GWL_USERDATA:   retvalue = wndPtr->userdata; break;
    case GWL_STYLE:      retvalue = wndPtr->dwStyle; break;
    case GWL_EXSTYLE:    retvalue = wndPtr->dwExStyle; break;
    case GWL_ID:         retvalue = (LONG)wndPtr->wIDmenu; break;
    case GWL_WNDPROC:    retvalue = (LONG)WINPROC_GetProc( wndPtr->winproc, type ); break;
    case GWL_HINSTANCE:  retvalue = (LONG)wndPtr->hInstance; break;
    default:
        WARN("Unknown offset %d\n", offset );
        SetLastError( ERROR_INVALID_INDEX );
        break;
    }
    WIN_ReleasePtr(wndPtr);
    return retvalue;
}


/**********************************************************************
 *	     WIN_SetWindowLong
 *
 * Helper function for SetWindowLong().
 *
 * 0 is the failure code. However, in the case of failure SetLastError
 * must be set to distinguish between a 0 return value and a failure.
 */
static LONG WIN_SetWindowLong( HWND hwnd, INT offset, LONG newval,
                               WINDOWPROCTYPE type )
{
    LONG retval = 0;
    WND *wndPtr;

    TRACE( "%p %d %lx %x\n", hwnd, offset, newval, type );

    if (is_broadcast(hwnd))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!WIN_IsCurrentProcess( hwnd ))
    {
        if (offset == GWL_WNDPROC)
        {
            SetLastError( ERROR_ACCESS_DENIED );
            return 0;
        }
        return SendMessageW( hwnd, WM_WINE_SETWINDOWLONG, offset, newval );
    }

    wndPtr = WIN_GetPtr( hwnd );
    if (wndPtr->hwndSelf == GetDesktopWindow())
    {
        /* can't change anything on the desktop window */
        WIN_ReleasePtr( wndPtr );
        SetLastError( ERROR_ACCESS_DENIED );
        return 0;
    }

    if (offset >= 0)
    {
        LONG *ptr = (LONG *)(((char *)wndPtr->wExtra) + offset);
        if (offset > (int)(wndPtr->cbWndExtra - sizeof(LONG)))
        {
            WARN("Invalid offset %d\n", offset );
            WIN_ReleasePtr( wndPtr );
            SetLastError( ERROR_INVALID_INDEX );
            return 0;
        }
        /* Special case for dialog window procedure */
        if ((offset == DWL_DLGPROC) && (wndPtr->flags & WIN_ISDIALOG))
        {
            retval = (LONG)WINPROC_GetProc( (WNDPROC)*ptr, type );
            WINPROC_SetProc( (WNDPROC *)ptr, (WNDPROC)newval, type, WIN_PROC_WINDOW );
            WIN_ReleasePtr( wndPtr );
            return retval;
        }
        retval = *ptr;
        *ptr = newval;
        WIN_ReleasePtr( wndPtr );
    }
    else
    {
        STYLESTRUCT style;
        BOOL ok;

        /* first some special cases */
        switch( offset )
        {
        case GWL_STYLE:
        case GWL_EXSTYLE:
            style.styleOld =
                offset == GWL_STYLE ? wndPtr->dwStyle : wndPtr->dwExStyle;
            style.styleNew = newval;
            WIN_ReleasePtr( wndPtr );
            SendMessageW( hwnd, WM_STYLECHANGING, offset, (LPARAM)&style );
            if (!(wndPtr = WIN_GetPtr( hwnd )) || wndPtr == WND_OTHER_PROCESS) return 0;
            newval = style.styleNew;
            break;
        case GWL_HWNDPARENT:
            if (wndPtr->parent == GetDesktopWindow())
            {
                WIN_ReleasePtr( wndPtr );
                return (LONG)WIN_SetOwner( hwnd, (HWND)newval );
            }
            else
            {
                WIN_ReleasePtr( wndPtr );
                return (LONG)SetParent( hwnd, (HWND)newval );
            }
        case GWL_WNDPROC:
            retval = (LONG)WINPROC_GetProc( wndPtr->winproc, type );
            WINPROC_SetProc( &wndPtr->winproc, (WNDPROC)newval, type, WIN_PROC_WINDOW );
            WIN_ReleasePtr( wndPtr );
            return retval;
        case GWL_ID:
        case GWL_HINSTANCE:
        case GWL_USERDATA:
            break;
        default:
            WIN_ReleasePtr( wndPtr );
            WARN("Invalid offset %d\n", offset );
            SetLastError( ERROR_INVALID_INDEX );
            return 0;
        }

        SERVER_START_REQ( set_window_info )
        {
            req->handle = hwnd;
            switch(offset)
            {
            case GWL_STYLE:
                req->flags = SET_WIN_STYLE;
                req->style = newval;
                break;
            case GWL_EXSTYLE:
                req->flags = SET_WIN_EXSTYLE;
                req->ex_style = newval;
                break;
            case GWL_ID:
                req->flags = SET_WIN_ID;
                req->id = newval;
                break;
            case GWL_HINSTANCE:
                req->flags = SET_WIN_INSTANCE;
                req->instance = (void *)newval;
                break;
            case GWL_USERDATA:
                req->flags = SET_WIN_USERDATA;
                req->user_data = (void *)newval;
                break;
            }
            if ((ok = !wine_server_call_err( req )))
            {
                switch(offset)
                {
                case GWL_STYLE:
                    wndPtr->dwStyle = newval;
                    retval = reply->old_style;
                    break;
                case GWL_EXSTYLE:
                    wndPtr->dwExStyle = newval;
                    retval = reply->old_ex_style;
                    break;
                case GWL_ID:
                    wndPtr->wIDmenu = newval;
                    retval = reply->old_id;
                    break;
                case GWL_HINSTANCE:
                    wndPtr->hInstance = (HINSTANCE)newval;
                    retval = (ULONG_PTR)reply->old_instance;
                    break;
                case GWL_USERDATA:
                    wndPtr->userdata = newval;
                    retval = (ULONG_PTR)reply->old_user_data;
                    break;
                }
            }
        }
        SERVER_END_REQ;
        WIN_ReleasePtr( wndPtr );

        if (!ok) return 0;

        if (offset == GWL_STYLE && USER_Driver.pSetWindowStyle)
            USER_Driver.pSetWindowStyle( hwnd, retval );

        if (offset == GWL_STYLE || offset == GWL_EXSTYLE)
            SendMessageW( hwnd, WM_STYLECHANGED, offset, (LPARAM)&style );

    }
    return retval;
}


/**********************************************************************
 *		GetWindowLong (USER.135)
 */
LONG WINAPI GetWindowLong16( HWND16 hwnd, INT16 offset )
{
    return WIN_GetWindowLong( WIN_Handle32(hwnd), offset, WIN_PROC_16 );
}


/**********************************************************************
 *		GetWindowLongA (USER32.@)
 */
LONG WINAPI GetWindowLongA( HWND hwnd, INT offset )
{
    return WIN_GetWindowLong( hwnd, offset, WIN_PROC_32A );
}


/**********************************************************************
 *		GetWindowLongW (USER32.@)
 */
LONG WINAPI GetWindowLongW( HWND hwnd, INT offset )
{
    return WIN_GetWindowLong( hwnd, offset, WIN_PROC_32W );
}


/**********************************************************************
 *		SetWindowLong (USER.136)
 */
LONG WINAPI SetWindowLong16( HWND16 hwnd, INT16 offset, LONG newval )
{
    return WIN_SetWindowLong( WIN_Handle32(hwnd), offset, newval, WIN_PROC_16 );
}


/**********************************************************************
 *		SetWindowLongA (USER32.@)
 */
LONG WINAPI SetWindowLongA( HWND hwnd, INT offset, LONG newval )
{
    return WIN_SetWindowLong( hwnd, offset, newval, WIN_PROC_32A );
}


/**********************************************************************
 *		SetWindowLongW (USER32.@) Set window attribute
 *
 * SetWindowLong() alters one of a window's attributes or sets a 32-bit (long)
 * value in a window's extra memory.
 *
 * The _hwnd_ parameter specifies the window.  is the handle to a
 * window that has extra memory. The _newval_ parameter contains the
 * new attribute or extra memory value.  If positive, the _offset_
 * parameter is the byte-addressed location in the window's extra
 * memory to set.  If negative, _offset_ specifies the window
 * attribute to set, and should be one of the following values:
 *
 * GWL_EXSTYLE      The window's extended window style
 *
 * GWL_STYLE        The window's window style.
 *
 * GWL_WNDPROC      Pointer to the window's window procedure.
 *
 * GWL_HINSTANCE    The window's pplication instance handle.
 *
 * GWL_ID           The window's identifier.
 *
 * GWL_USERDATA     The window's user-specified data.
 *
 * If the window is a dialog box, the _offset_ parameter can be one of
 * the following values:
 *
 * DWL_DLGPROC      The address of the window's dialog box procedure.
 *
 * DWL_MSGRESULT    The return value of a message
 *                  that the dialog box procedure processed.
 *
 * DWL_USER         Application specific information.
 *
 * RETURNS
 *
 * If successful, returns the previous value located at _offset_. Otherwise,
 * returns 0.
 *
 * NOTES
 *
 * Extra memory for a window class is specified by a nonzero cbWndExtra
 * parameter of the WNDCLASS structure passed to RegisterClass() at the
 * time of class creation.
 *
 * Using GWL_WNDPROC to set a new window procedure effectively creates
 * a window subclass. Use CallWindowProc() in the new windows procedure
 * to pass messages to the superclass's window procedure.
 *
 * The user data is reserved for use by the application which created
 * the window.
 *
 * Do not use GWL_STYLE to change the window's WS_DISABLED style;
 * instead, call the EnableWindow() function to change the window's
 * disabled state.
 *
 * Do not use GWL_HWNDPARENT to reset the window's parent, use
 * SetParent() instead.
 *
 * Win95:
 * When offset is GWL_STYLE and the calling app's ver is 4.0,
 * it sends WM_STYLECHANGING before changing the settings
 * and WM_STYLECHANGED afterwards.
 * App ver 4.0 can't use SetWindowLong to change WS_EX_TOPMOST.
 */
LONG WINAPI SetWindowLongW(
    HWND hwnd,  /* [in] window to alter */
    INT offset, /* [in] offset, in bytes, of location to alter */
    LONG newval /* [in] new value of location */
) {
    return WIN_SetWindowLong( hwnd, offset, newval, WIN_PROC_32W );
}


/*******************************************************************
 *		GetWindowTextA (USER32.@)
 */
INT WINAPI GetWindowTextA( HWND hwnd, LPSTR lpString, INT nMaxCount )
{
    WCHAR *buffer;

    if (WIN_IsCurrentProcess( hwnd ))
        return (INT)SendMessageA( hwnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString );

    /* when window belongs to other process, don't send a message */
    if (nMaxCount <= 0) return 0;
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, nMaxCount * sizeof(WCHAR) ))) return 0;
    get_server_window_text( hwnd, buffer, nMaxCount );
    if (!WideCharToMultiByte( CP_ACP, 0, buffer, -1, lpString, nMaxCount, NULL, NULL ))
        lpString[nMaxCount-1] = 0;
    HeapFree( GetProcessHeap(), 0, buffer );
    return strlen(lpString);
}


/*******************************************************************
 *		InternalGetWindowText (USER32.@)
 */
INT WINAPI InternalGetWindowText(HWND hwnd,LPWSTR lpString,INT nMaxCount )
{
    WND *win;

    if (nMaxCount <= 0) return 0;
    if (!(win = WIN_GetPtr( hwnd ))) return 0;
    if (win != WND_OTHER_PROCESS)
    {
        if (win->text) lstrcpynW( lpString, win->text, nMaxCount );
        else lpString[0] = 0;
        WIN_ReleasePtr( win );
    }
    else
    {
        get_server_window_text( hwnd, lpString, nMaxCount );
    }
    return strlenW(lpString);
}


/*******************************************************************
 *		GetWindowTextW (USER32.@)
 */
INT WINAPI GetWindowTextW( HWND hwnd, LPWSTR lpString, INT nMaxCount )
{
    if (WIN_IsCurrentProcess( hwnd ))
        return (INT)SendMessageW( hwnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString );

    /* when window belongs to other process, don't send a message */
    if (nMaxCount <= 0) return 0;
    get_server_window_text( hwnd, lpString, nMaxCount );
    return strlenW(lpString);
}


/*******************************************************************
 *		SetWindowText  (USER32.@)
 *		SetWindowTextA (USER32.@)
 */
BOOL WINAPI SetWindowTextA( HWND hwnd, LPCSTR lpString )
{
    if (is_broadcast(hwnd))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!WIN_IsCurrentProcess( hwnd ))
    {
        FIXME( "cannot set text %s of other process window %p\n", debugstr_a(lpString), hwnd );
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }
    return (BOOL)SendMessageA( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *		SetWindowTextW (USER32.@)
 */
BOOL WINAPI SetWindowTextW( HWND hwnd, LPCWSTR lpString )
{
    if (is_broadcast(hwnd))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!WIN_IsCurrentProcess( hwnd ))
    {
        FIXME( "cannot set text %s of other process window %p\n", debugstr_w(lpString), hwnd );
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }
    return (BOOL)SendMessageW( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *		GetWindowTextLengthA (USER32.@)
 */
INT WINAPI GetWindowTextLengthA( HWND hwnd )
{
    return SendMessageA( hwnd, WM_GETTEXTLENGTH, 0, 0 );
}

/*******************************************************************
 *		GetWindowTextLengthW (USER32.@)
 */
INT WINAPI GetWindowTextLengthW( HWND hwnd )
{
    return SendMessageW( hwnd, WM_GETTEXTLENGTH, 0, 0 );
}


/*******************************************************************
 *		IsWindow (USER32.@)
 */
BOOL WINAPI IsWindow( HWND hwnd )
{
    WND *ptr;
    BOOL ret;

    if (!(ptr = WIN_GetPtr( hwnd ))) return FALSE;

    if (ptr != WND_OTHER_PROCESS)
    {
        WIN_ReleasePtr( ptr );
        return TRUE;
    }

    /* check other processes */
    SERVER_START_REQ( get_window_info )
    {
        req->handle = hwnd;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *		GetWindowThreadProcessId (USER32.@)
 */
DWORD WINAPI GetWindowThreadProcessId( HWND hwnd, LPDWORD process )
{
    WND *ptr;
    DWORD tid = 0;

    if (!(ptr = WIN_GetPtr( hwnd )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE);
        return 0;
    }

    if (ptr != WND_OTHER_PROCESS)
    {
        /* got a valid window */
        tid = ptr->tid;
        if (process) *process = GetCurrentProcessId();
        WIN_ReleasePtr( ptr );
        return tid;
    }

    /* check other processes */
    SERVER_START_REQ( get_window_info )
    {
        req->handle = hwnd;
        if (!wine_server_call_err( req ))
        {
            tid = (DWORD)reply->tid;
            if (process) *process = (DWORD)reply->pid;
        }
    }
    SERVER_END_REQ;
    return tid;
}


/*****************************************************************
 *		GetParent (USER32.@)
 */
HWND WINAPI GetParent( HWND hwnd )
{
    WND *wndPtr;
    HWND retvalue = 0;

    if (!(wndPtr = WIN_GetPtr( hwnd )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (wndPtr == WND_OTHER_PROCESS)
    {
        LONG style = GetWindowLongW( hwnd, GWL_STYLE );
        if (style & (WS_POPUP | WS_CHILD))
        {
            SERVER_START_REQ( get_window_tree )
            {
                req->handle = hwnd;
                if (!wine_server_call_err( req ))
                {
                    if (style & WS_POPUP) retvalue = reply->owner;
                    else if (style & WS_CHILD) retvalue = reply->parent;
                }
            }
            SERVER_END_REQ;
        }
    }
    else
    {
        if (wndPtr->dwStyle & WS_POPUP) retvalue = wndPtr->owner;
        else if (wndPtr->dwStyle & WS_CHILD) retvalue = wndPtr->parent;
        WIN_ReleasePtr( wndPtr );
    }
    return retvalue;
}


/*****************************************************************
 *		GetAncestor (USER32.@)
 */
HWND WINAPI GetAncestor( HWND hwnd, UINT type )
{
    WND *win;
    HWND *list, ret = 0;

    switch(type)
    {
    case GA_PARENT:
        if (!(win = WIN_GetPtr( hwnd )))
        {
            SetLastError( ERROR_INVALID_WINDOW_HANDLE );
            return 0;
        }
        if (win != WND_OTHER_PROCESS)
        {
            ret = win->parent;
            WIN_ReleasePtr( win );
        }
        else /* need to query the server */
        {
            SERVER_START_REQ( get_window_tree )
            {
                req->handle = hwnd;
                if (!wine_server_call_err( req )) ret = reply->parent;
            }
            SERVER_END_REQ;
        }
        break;

    case GA_ROOT:
        if (!(list = WIN_ListParents( hwnd ))) return 0;

        if (!list[0] || !list[1]) ret = WIN_GetFullHandle( hwnd );  /* top-level window */
        else
        {
            int count = 2;
            while (list[count]) count++;
            ret = list[count - 2];  /* get the one before the desktop */
        }
        HeapFree( GetProcessHeap(), 0, list );
        break;

    case GA_ROOTOWNER:
        if ((ret = WIN_GetFullHandle( hwnd )) == GetDesktopWindow()) return 0;
        for (;;)
        {
            HWND parent = GetParent( ret );
            if (!parent) break;
            ret = parent;
        }
        break;
    }
    return ret;
}


/*****************************************************************
 *		SetParent (USER32.@)
 */
HWND WINAPI SetParent( HWND hwnd, HWND parent )
{
    WND *wndPtr;
    HWND retvalue, full_handle;
    BOOL was_visible;

    if (is_broadcast(hwnd) || is_broadcast(parent))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!parent) parent = GetDesktopWindow();
    else parent = WIN_GetFullHandle( parent );

    if (!IsWindow( parent ))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    if (!(full_handle = WIN_IsCurrentThread( hwnd )))
        return (HWND)SendMessageW( hwnd, WM_WINE_SETPARENT, (WPARAM)parent, 0 );

    hwnd = full_handle;

    if (USER_Driver.pSetParent)
        return USER_Driver.pSetParent( hwnd, parent );

    /* Windows hides the window first, then shows it again
     * including the WM_SHOWWINDOW messages and all */
    was_visible = ShowWindow( hwnd, SW_HIDE );

    if (!IsWindow( parent )) return 0;
    if (!(wndPtr = WIN_GetPtr(hwnd)) || wndPtr == WND_OTHER_PROCESS) return 0;

    retvalue = wndPtr->parent;  /* old parent */
    if (parent != retvalue)
    {
        WIN_LinkWindow( hwnd, parent, HWND_TOP );

        if (parent != GetDesktopWindow()) /* a child window */
        {
            if (!(wndPtr->dwStyle & WS_CHILD))
            {
                HMENU menu = (HMENU)SetWindowLongW( hwnd, GWL_ID, 0 );
                if (menu) DestroyMenu( menu );
            }
        }
    }
    WIN_ReleasePtr( wndPtr );

    /* SetParent additionally needs to make hwnd the topmost window
       in the x-order and send the expected WM_WINDOWPOSCHANGING and
       WM_WINDOWPOSCHANGED notification messages.
    */
    SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                  SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | (was_visible ? SWP_SHOWWINDOW : 0) );
    /* FIXME: a WM_MOVE is also generated (in the DefWindowProc handler
     * for WM_WINDOWPOSCHANGED) in Windows, should probably remove SWP_NOMOVE */
    return retvalue;
}


/*******************************************************************
 *		IsChild (USER32.@)
 */
BOOL WINAPI IsChild( HWND parent, HWND child )
{
    HWND *list = WIN_ListParents( child );
    int i;
    BOOL ret;

    if (!list) return FALSE;
    parent = WIN_GetFullHandle( parent );
    for (i = 0; list[i]; i++) if (list[i] == parent) break;
    ret = (list[i] != 0);
    HeapFree( GetProcessHeap(), 0, list );
    return ret;
}


/***********************************************************************
 *		IsWindowVisible (USER32.@)
 */
BOOL WINAPI IsWindowVisible( HWND hwnd )
{
    HWND *list;
    BOOL retval;
    int i;

    if (!(GetWindowLongW( hwnd, GWL_STYLE ) & WS_VISIBLE)) return FALSE;
    if (!(list = WIN_ListParents( hwnd ))) return TRUE;
    for (i = 0; list[i]; i++)
        if (!(GetWindowLongW( list[i], GWL_STYLE ) & WS_VISIBLE)) break;
    retval = !list[i];
    HeapFree( GetProcessHeap(), 0, list );
    return retval;
}


/***********************************************************************
 *           WIN_IsWindowDrawable
 *
 * hwnd is drawable when it is visible, all parents are not
 * minimized, and it is itself not minimized unless we are
 * trying to draw its default class icon.
 */
BOOL WIN_IsWindowDrawable( HWND hwnd, BOOL icon )
{
    HWND *list;
    BOOL retval;
    int i;
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );

    if (!(style & WS_VISIBLE)) return FALSE;
    if ((style & WS_MINIMIZE) && icon && GetClassLongA( hwnd, GCL_HICON ))  return FALSE;

    if (!(list = WIN_ListParents( hwnd ))) return TRUE;
    for (i = 0; list[i]; i++)
        if ((GetWindowLongW( list[i], GWL_STYLE ) & (WS_VISIBLE|WS_MINIMIZE)) != WS_VISIBLE)
            break;
    retval = !list[i];
    HeapFree( GetProcessHeap(), 0, list );
    return retval;
}


/*******************************************************************
 *		GetTopWindow (USER32.@)
 */
HWND WINAPI GetTopWindow( HWND hwnd )
{
    if (!hwnd) hwnd = GetDesktopWindow();
    return GetWindow( hwnd, GW_CHILD );
}


/*******************************************************************
 *		GetWindow (USER32.@)
 */
HWND WINAPI GetWindow( HWND hwnd, UINT rel )
{
    HWND retval = 0;

    if (rel == GW_OWNER)  /* this one may be available locally */
    {
        WND *wndPtr = WIN_GetPtr( hwnd );
        if (!wndPtr)
        {
            SetLastError( ERROR_INVALID_HANDLE );
            return 0;
        }
        if (wndPtr != WND_OTHER_PROCESS)
        {
            retval = wndPtr->owner;
            WIN_ReleasePtr( wndPtr );
            return retval;
        }
        /* else fall through to server call */
    }

    SERVER_START_REQ( get_window_tree )
    {
        req->handle = hwnd;
        if (!wine_server_call_err( req ))
        {
            switch(rel)
            {
            case GW_HWNDFIRST:
                retval = reply->first_sibling;
                break;
            case GW_HWNDLAST:
                retval = reply->last_sibling;
                break;
            case GW_HWNDNEXT:
                retval = reply->next_sibling;
                break;
            case GW_HWNDPREV:
                retval = reply->prev_sibling;
                break;
            case GW_OWNER:
                retval = reply->owner;
                break;
            case GW_CHILD:
                retval = reply->first_child;
                break;
            }
        }
    }
    SERVER_END_REQ;
    return retval;
}


/***********************************************************************
 *           WIN_InternalShowOwnedPopups
 *
 * Internal version of ShowOwnedPopups; Wine functions should use this
 * to avoid interfering with application calls to ShowOwnedPopups
 * and to make sure the application can't prevent showing/hiding.
 *
 * Set unmanagedOnly to TRUE to show/hide unmanaged windows only.
 *
 */

BOOL WIN_InternalShowOwnedPopups( HWND owner, BOOL fShow, BOOL unmanagedOnly )
{
    int count = 0;
    WND *pWnd;
    HWND *win_array = WIN_ListChildren( GetDesktopWindow() );

    if (!win_array) return TRUE;

    /*
     * Show windows Lowest first, Highest last to preserve Z-Order
     */
    while (win_array[count]) count++;
    while (--count >= 0)
    {
        if (GetWindow( win_array[count], GW_OWNER ) != owner) continue;
        if (!(pWnd = WIN_FindWndPtr( win_array[count] ))) continue;

        if (pWnd->dwStyle & WS_POPUP)
        {
            if (fShow)
            {
                /* check in window was flagged for showing in previous WIN_InternalShowOwnedPopups call */
                if (pWnd->flags & WIN_NEEDS_INTERNALSOP)
                {
                    /*
                     * Call ShowWindow directly because an application can intercept WM_SHOWWINDOW messages
                     */
                    ShowWindow(pWnd->hwndSelf,SW_SHOW);
                    pWnd->flags &= ~WIN_NEEDS_INTERNALSOP; /* remove the flag */
                }
            }
            else
            {
                if ( IsWindowVisible(pWnd->hwndSelf) &&                   /* hide only if window is visible */
                     !( pWnd->flags & WIN_NEEDS_INTERNALSOP ) &&          /* don't hide if previous call already did it */
                     !( unmanagedOnly && (pWnd->dwExStyle & WS_EX_MANAGED) ) ) /* don't hide managed windows if unmanagedOnly is TRUE */
                {
                    /*
                     * Call ShowWindow directly because an application can intercept WM_SHOWWINDOW messages
                     */
                    ShowWindow(pWnd->hwndSelf,SW_HIDE);
                    /* flag the window for showing on next WIN_InternalShowOwnedPopups call */
                    pWnd->flags |= WIN_NEEDS_INTERNALSOP;
                }
            }
        }
        WIN_ReleaseWndPtr( pWnd );
    }
    HeapFree( GetProcessHeap(), 0, win_array );

    return TRUE;
}

/*******************************************************************
 *		ShowOwnedPopups (USER32.@)
 */
BOOL WINAPI ShowOwnedPopups( HWND owner, BOOL fShow )
{
    int count = 0;
    WND *pWnd;
    HWND *win_array = WIN_ListChildren( GetDesktopWindow() );

    if (!win_array) return TRUE;

    while (win_array[count]) count++;
    while (--count >= 0)
    {
        if (GetWindow( win_array[count], GW_OWNER ) != owner) continue;
        if (!(pWnd = WIN_FindWndPtr( win_array[count] ))) continue;

        if (pWnd->dwStyle & WS_POPUP)
        {
            if (fShow)
            {
                if (pWnd->flags & WIN_NEEDS_SHOW_OWNEDPOPUP)
                {
                    /* In Windows, ShowOwnedPopups(TRUE) generates
                     * WM_SHOWWINDOW messages with SW_PARENTOPENING,
                     * regardless of the state of the owner
                     */
                    SendMessageA(pWnd->hwndSelf, WM_SHOWWINDOW, SW_SHOW, SW_PARENTOPENING);
                    pWnd->flags &= ~WIN_NEEDS_SHOW_OWNEDPOPUP;
                }
            }
            else
            {
                if (IsWindowVisible(pWnd->hwndSelf))
                {
                    /* In Windows, ShowOwnedPopups(FALSE) generates
                     * WM_SHOWWINDOW messages with SW_PARENTCLOSING,
                     * regardless of the state of the owner
                     */
                    SendMessageA(pWnd->hwndSelf, WM_SHOWWINDOW, SW_HIDE, SW_PARENTCLOSING);
                    pWnd->flags |= WIN_NEEDS_SHOW_OWNEDPOPUP;
                }
            }
        }
        WIN_ReleaseWndPtr( pWnd );
    }
    HeapFree( GetProcessHeap(), 0, win_array );
    return TRUE;
}


/*******************************************************************
 *		GetLastActivePopup (USER32.@)
 */
HWND WINAPI GetLastActivePopup( HWND hwnd )
{
    HWND retval = hwnd;

    SERVER_START_REQ( get_window_info )
    {
        req->handle = hwnd;
        if (!wine_server_call_err( req )) retval = reply->last_active;
    }
    SERVER_END_REQ;
    return retval;
}


/*******************************************************************
 *           WIN_ListParents
 *
 * Build an array of all parents of a given window, starting with
 * the immediate parent. The array must be freed with HeapFree.
 * Returns NULL if window is a top-level window.
 */
HWND *WIN_ListParents( HWND hwnd )
{
    WND *win;
    HWND current, *list;
    int pos = 0, size = 16, count = 0;

    if (!(list = HeapAlloc( GetProcessHeap(), 0, size * sizeof(HWND) ))) return NULL;

    current = hwnd;
    for (;;)
    {
        if (!(win = WIN_GetPtr( current ))) goto empty;
        if (win == WND_OTHER_PROCESS) break;  /* need to do it the hard way */
        list[pos] = win->parent;
        WIN_ReleasePtr( win );
        if (!(current = list[pos]))
        {
            if (!pos) goto empty;
            return list;
        }
        if (++pos == size - 1)
        {
            /* need to grow the list */
            HWND *new_list = HeapReAlloc( GetProcessHeap(), 0, list, (size+16) * sizeof(HWND) );
            if (!new_list) goto empty;
            list = new_list;
            size += 16;
        }
    }

    /* at least one parent belongs to another process, have to query the server */

    for (;;)
    {
        count = 0;
        SERVER_START_REQ( get_window_parents )
        {
            req->handle = hwnd;
            wine_server_set_reply( req, list, (size-1) * sizeof(HWND) );
            if (!wine_server_call( req )) count = reply->count;
        }
        SERVER_END_REQ;
        if (!count) goto empty;
        if (size > count)
        {
            list[count] = 0;
            return list;
        }
        HeapFree( GetProcessHeap(), 0, list );
        size = count + 1;
        if (!(list = HeapAlloc( GetProcessHeap(), 0, size * sizeof(HWND) ))) return NULL;
    }

 empty:
    HeapFree( GetProcessHeap(), 0, list );
    return NULL;
}


/*******************************************************************
 *           WIN_ListChildren
 *
 * Build an array of the children of a given window. The array must be
 * freed with HeapFree. Returns NULL when no windows are found.
 */
HWND *WIN_ListChildren( HWND hwnd )
{
    return list_window_children( hwnd, 0, 0 );
}


/*******************************************************************
 *		EnumWindows (USER32.@)
 */
BOOL WINAPI EnumWindows( WNDENUMPROC lpEnumFunc, LPARAM lParam )
{
    HWND *list;
    BOOL ret = TRUE;
    int i, iWndsLocks;

    /* We have to build a list of all windows first, to avoid */
    /* unpleasant side-effects, for instance if the callback */
    /* function changes the Z-order of the windows.          */

    if (!(list = WIN_ListChildren( GetDesktopWindow() ))) return TRUE;

    /* Now call the callback function for every window */

    iWndsLocks = WIN_SuspendWndsLock();
    for (i = 0; list[i]; i++)
    {
        /* Make sure that the window still exists */
        if (!IsWindow( list[i] )) continue;
        if (!(ret = lpEnumFunc( list[i], lParam ))) break;
    }
    WIN_RestoreWndsLock(iWndsLocks);
    HeapFree( GetProcessHeap(), 0, list );
    return ret;
}


/**********************************************************************
 *		EnumThreadWindows (USER32.@)
 */
BOOL WINAPI EnumThreadWindows( DWORD id, WNDENUMPROC func, LPARAM lParam )
{
    HWND *list;
    int i, iWndsLocks;

    if (!(list = list_window_children( GetDesktopWindow(), 0, id ))) return TRUE;

    /* Now call the callback function for every window */

    iWndsLocks = WIN_SuspendWndsLock();
    for (i = 0; list[i]; i++)
        if (!func( list[i], lParam )) break;
    WIN_RestoreWndsLock(iWndsLocks);
    HeapFree( GetProcessHeap(), 0, list );
    return TRUE;
}


/**********************************************************************
 *           WIN_EnumChildWindows
 *
 * Helper function for EnumChildWindows().
 */
static BOOL WIN_EnumChildWindows( HWND *list, WNDENUMPROC func, LPARAM lParam )
{
    HWND *childList;
    BOOL ret = FALSE;

    for ( ; *list; list++)
    {
        /* Make sure that the window still exists */
        if (!IsWindow( *list )) continue;
        /* skip owned windows */
        if (GetWindow( *list, GW_OWNER )) continue;
        /* Build children list first */
        childList = WIN_ListChildren( *list );

        ret = func( *list, lParam );

        if (childList)
        {
            if (ret) ret = WIN_EnumChildWindows( childList, func, lParam );
            HeapFree( GetProcessHeap(), 0, childList );
        }
        if (!ret) return FALSE;
    }
    return TRUE;
}


/**********************************************************************
 *		EnumChildWindows (USER32.@)
 */
BOOL WINAPI EnumChildWindows( HWND parent, WNDENUMPROC func, LPARAM lParam )
{
    HWND *list;
    int iWndsLocks;

    if (!(list = WIN_ListChildren( parent ))) return FALSE;
    iWndsLocks = WIN_SuspendWndsLock();
    WIN_EnumChildWindows( list, func, lParam );
    WIN_RestoreWndsLock(iWndsLocks);
    HeapFree( GetProcessHeap(), 0, list );
    return TRUE;
}


/*******************************************************************
 *		AnyPopup (USER.52)
 */
BOOL16 WINAPI AnyPopup16(void)
{
    return AnyPopup();
}


/*******************************************************************
 *		AnyPopup (USER32.@)
 */
BOOL WINAPI AnyPopup(void)
{
    int i;
    BOOL retvalue;
    HWND *list = WIN_ListChildren( GetDesktopWindow() );

    if (!list) return FALSE;
    for (i = 0; list[i]; i++)
    {
        if (IsWindowVisible( list[i] ) && GetWindow( list[i], GW_OWNER )) break;
    }
    retvalue = (list[i] != 0);
    HeapFree( GetProcessHeap(), 0, list );
    return retvalue;
}


/*******************************************************************
 *		FlashWindow (USER32.@)
 */
BOOL WINAPI FlashWindow( HWND hWnd, BOOL bInvert )
{
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    TRACE("%p\n", hWnd);

    if (!wndPtr) return FALSE;
    hWnd = wndPtr->hwndSelf;  /* make it a full handle */

    if (wndPtr->dwStyle & WS_MINIMIZE)
    {
        if (bInvert && !(wndPtr->flags & WIN_NCACTIVATED))
        {
            HDC hDC = GetDC(hWnd);

            if (!SendMessageW( hWnd, WM_ERASEBKGND, (WPARAM)hDC, 0 ))
                wndPtr->flags |= WIN_NEEDS_ERASEBKGND;

            ReleaseDC( hWnd, hDC );
            wndPtr->flags |= WIN_NCACTIVATED;
        }
        else
        {
            RedrawWindow( hWnd, 0, 0, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_FRAME );
            wndPtr->flags &= ~WIN_NCACTIVATED;
        }
        WIN_ReleaseWndPtr(wndPtr);
        return TRUE;
    }
    else
    {
        WPARAM16 wparam;
        if (bInvert) wparam = !(wndPtr->flags & WIN_NCACTIVATED);
        else wparam = (hWnd == GetForegroundWindow());

        WIN_ReleaseWndPtr(wndPtr);
        SendMessageW( hWnd, WM_NCACTIVATE, wparam, (LPARAM)0 );
        return wparam;
    }
}

/*******************************************************************
 *		FlashWindowEx (USER32.@)
 */
BOOL WINAPI FlashWindowEx( PFLASHWINFO pfwi )
{
    FIXME("%p\n", pfwi);
    return TRUE;
}

/*******************************************************************
 *		GetWindowContextHelpId (USER32.@)
 */
DWORD WINAPI GetWindowContextHelpId( HWND hwnd )
{
    DWORD retval;
    WND *wnd = WIN_FindWndPtr( hwnd );
    if (!wnd) return 0;
    retval = wnd->helpContext;
    WIN_ReleaseWndPtr(wnd);
    return retval;
}


/*******************************************************************
 *		SetWindowContextHelpId (USER32.@)
 */
BOOL WINAPI SetWindowContextHelpId( HWND hwnd, DWORD id )
{
    WND *wnd = WIN_FindWndPtr( hwnd );
    if (!wnd) return FALSE;
    wnd->helpContext = id;
    WIN_ReleaseWndPtr(wnd);
    return TRUE;
}


/*******************************************************************
 *		DragDetect (USER32.@)
 */
BOOL WINAPI DragDetect( HWND hWnd, POINT pt )
{
    MSG msg;
    RECT rect;

    rect.left = pt.x - wDragWidth;
    rect.right = pt.x + wDragWidth;

    rect.top = pt.y - wDragHeight;
    rect.bottom = pt.y + wDragHeight;

    SetCapture(hWnd);

    while(1)
    {
	while(PeekMessageA(&msg ,0 ,WM_MOUSEFIRST ,WM_MOUSELAST ,PM_REMOVE))
        {
            if( msg.message == WM_LBUTTONUP )
	    {
		ReleaseCapture();
		return 0;
            }
            if( msg.message == WM_MOUSEMOVE )
	    {
                POINT tmp;
                tmp.x = LOWORD(msg.lParam);
                tmp.y = HIWORD(msg.lParam);
		if( !PtInRect( &rect, tmp ))
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
 *		GetWindowModuleFileNameA (USER32.@)
 */
UINT WINAPI GetWindowModuleFileNameA( HWND hwnd, LPSTR lpszFileName, UINT cchFileNameMax)
{
    FIXME("GetWindowModuleFileNameA(hwnd %p, lpszFileName %p, cchFileNameMax %u) stub!\n",
          hwnd, lpszFileName, cchFileNameMax);
    return 0;
}

/******************************************************************************
 *		GetWindowModuleFileNameW (USER32.@)
 */
UINT WINAPI GetWindowModuleFileNameW( HWND hwnd, LPSTR lpszFileName, UINT cchFileNameMax)
{
    FIXME("GetWindowModuleFileNameW(hwnd %p, lpszFileName %p, cchFileNameMax %u) stub!\n",
          hwnd, lpszFileName, cchFileNameMax);
    return 0;
}

/******************************************************************************
 *              GetWindowInfo (USER32.@)
 * hwnd: in
 * pwi:  out.
 * MS Documentation mentions that pwi->cbSize must be set to SIZEOF(WINDOWINFO)
 *    this may be because this structure changed over time. If this is the
 *    the case, then please: FIXME.
 *    Using the structure described in MSDN for 98/ME/NT(4.0 SP3)/2000/XP.
 */
BOOL WINAPI GetWindowInfo( HWND hwnd, PWINDOWINFO pwi)
{
    if (!pwi) return FALSE;
    if (pwi->cbSize != sizeof(WINDOWINFO))
    {
        FIXME("windowinfo->cbSize != sizeof(WINDOWINFO). Please report\n");
        return FALSE;
    }
    if (!IsWindow(hwnd)) return FALSE;

    GetWindowRect(hwnd, &pwi->rcWindow);
    GetClientRect(hwnd, &pwi->rcClient);
    /* translate to screen coordinates */
    MapWindowPoints(hwnd, 0, (LPPOINT)&pwi->rcClient, 2);

    pwi->dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
    pwi->dwExStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    pwi->dwWindowStatus = ((GetActiveWindow() == hwnd) ? WS_ACTIVECAPTION : 0);

    pwi->cxWindowBorders = pwi->rcClient.left - pwi->rcWindow.left;
    pwi->cyWindowBorders = pwi->rcWindow.bottom - pwi->rcClient.bottom;

    pwi->atomWindowType = GetClassLongW( hwnd, GCW_ATOM );
    pwi->wCreatorVersion = 0x0400;

    return TRUE;
}
