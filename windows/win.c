/*
 * Window related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wine/unicode.h"
#include "win.h"
#include "heap.h"
#include "user.h"
#include "dce.h"
#include "controls.h"
#include "cursoricon.h"
#include "hook.h"
#include "message.h"
#include "queue.h"
#include "task.h"
#include "winpos.h"
#include "winerror.h"
#include "stackframe.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win);
DECLARE_DEBUG_CHANNEL(msg);

/**********************************************************************/

/* Desktop window */
static WND *pWndDesktop = NULL;

static HWND hwndSysModal = 0;

static WORD wDragWidth = 4;
static WORD wDragHeight= 3;

/* thread safeness */
extern SYSLEVEL USER_SysLevel;  /* FIXME */

/***********************************************************************
 *           WIN_SuspendWndsLock
 *
 *   Suspend the lock on WND structures.
 *   Returns the number of locks suspended
 */
int WIN_SuspendWndsLock( void )
{
    int isuspendedLocks = _ConfirmSysLevel( &USER_SysLevel );
    int count = isuspendedLocks;

    while ( count-- > 0 )
        _LeaveSysLevel( &USER_SysLevel );

    return isuspendedLocks;
}

/***********************************************************************
 *           WIN_RestoreWndsLock
 *
 *  Restore the suspended locks on WND structures
 */
void WIN_RestoreWndsLock( int ipreviousLocks )
{
    while ( ipreviousLocks-- > 0 )
        _EnterSysLevel( &USER_SysLevel );
}

/***********************************************************************
 *           WIN_FindWndPtr
 *
 * Return a pointer to the WND structure corresponding to a HWND.
 */
WND * WIN_FindWndPtr( HWND hwnd )
{
    WND * ptr;
    
    if (!hwnd || HIWORD(hwnd)) goto error2;
    ptr = (WND *) USER_HEAP_LIN_ADDR( hwnd );
    /* Lock all WND structures for thread safeness*/
    USER_Lock();
    /*and increment destruction monitoring*/
     ptr->irefCount++;

    if (ptr->dwMagic != WND_MAGIC) goto error;
    if (ptr->hwndSelf != hwnd)
    {
        ERR("Can't happen: hwnd %04x self pointer is %04x\n",hwnd, ptr->hwndSelf );
        goto error;
    }
    /* returns a locked pointer */
    return ptr;
 error:
    /* Unlock all WND structures for thread safeness*/
    USER_Unlock();
    /* and decrement destruction monitoring value */
     ptr->irefCount--;

error2:
    if ( hwnd!=0 )
      SetLastError( ERROR_INVALID_WINDOW_HANDLE );
    return NULL;
}

/***********************************************************************
 *           WIN_LockWndPtr
 *
 * Use in case the wnd ptr is not initialized with WIN_FindWndPtr
 * but by initWndPtr;
 * Returns the locked initialisation pointer
 */
WND *WIN_LockWndPtr(WND *initWndPtr)
{
    if(!initWndPtr) return 0;

    /* Lock all WND structures for thread safeness*/
    USER_Lock();
    /*and increment destruction monitoring*/
    initWndPtr->irefCount++;
    
    return initWndPtr;

}
        
/***********************************************************************
 *           WIN_ReleaseWndPtr
 *
 * Release the pointer to the WND structure.
 */
void WIN_ReleaseWndPtr(WND *wndPtr)
{
    if(!wndPtr) return;

    /*Decrement destruction monitoring value*/
     wndPtr->irefCount--;
     /* Check if it's time to release the memory*/
     if(wndPtr->irefCount == 0 && !wndPtr->dwMagic)
     {
         /* Release memory */
         USER_HEAP_FREE( wndPtr->hwndSelf);
         wndPtr->hwndSelf = 0;
     }
     else if(wndPtr->irefCount < 0)
     {
         /* This else if is useful to monitor the WIN_ReleaseWndPtr function */
         ERR("forgot a Lock on %p somewhere\n",wndPtr);
     }
     /*unlock all WND structures for thread safeness*/
     USER_Unlock();
}

/***********************************************************************
 *           WIN_UpdateWndPtr
 *
 * Updates the value of oldPtr to newPtr.
 */
void WIN_UpdateWndPtr(WND **oldPtr, WND *newPtr)
{
    WND *tmpWnd = NULL;
    
    tmpWnd = WIN_LockWndPtr(newPtr);
    WIN_ReleaseWndPtr(*oldPtr);
    *oldPtr = tmpWnd;

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
        WARN("%04x is not a window handle\n", hwnd );
        return;
    }

    if (!GetClassNameA( hwnd, className, sizeof(className ) ))
        strcpy( className, "#NULL#" );

    TRACE("Window %04x (%p):\n", hwnd, ptr );
    DPRINTF( "next=%p  child=%p  parent=%p  owner=%p  class=%p '%s'\n"
             "inst=%04x  taskQ=%04x  updRgn=%04x  active=%04x dce=%p  idmenu=%08x\n"
             "style=%08lx  exstyle=%08lx  wndproc=%08x  text='%s'\n"
             "client=%d,%d-%d,%d  window=%d,%d-%d,%d"
             "sysmenu=%04x  flags=%04x  props=%p  vscroll=%p  hscroll=%p\n",
             ptr->next, ptr->child, ptr->parent, ptr->owner,
             ptr->class, className, ptr->hInstance, ptr->hmemTaskQ,
             ptr->hrgnUpdate, ptr->hwndLastActive, ptr->dce, ptr->wIDmenu,
             ptr->dwStyle, ptr->dwExStyle, (UINT)ptr->winproc,
             ptr->text ? debugstr_w(ptr->text) : "",
             ptr->rectClient.left, ptr->rectClient.top, ptr->rectClient.right,
             ptr->rectClient.bottom, ptr->rectWindow.left, ptr->rectWindow.top,
             ptr->rectWindow.right, ptr->rectWindow.bottom, ptr->hSysMenu,
             ptr->flags, ptr->pProp, ptr->pVScroll, ptr->pHScroll );

    if (ptr->cbWndExtra)
    {
        DPRINTF( "extra bytes:" );
        for (i = 0; i < ptr->cbWndExtra; i++)
            DPRINTF( " %02x", *((BYTE*)ptr->wExtra+i) );
        DPRINTF( "\n" );
    }
    DPRINTF( "\n" );
    WIN_ReleaseWndPtr(ptr);
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

    ptr = hwnd ? WIN_FindWndPtr( hwnd ) : WIN_GetDesktop();

    if (!ptr)
    {
        WARN("Invalid window handle %04x\n", hwnd );
        return;
    }

    if (!indent)  /* first time around */
       DPRINTF( "%-16.16s %-8.8s %-6.6s %-17.17s %-8.8s %s\n",
                 "hwnd", " wndPtr", "queue", "Class Name", " Style", " WndProc"
                 " Text");

    while (ptr)
    {
        DPRINTF( "%*s%04x%*s", indent, "", ptr->hwndSelf, 13-indent,"");

        GetClassNameA( ptr->hwndSelf, className, sizeof(className) );
        DPRINTF( "%08lx %-6.4x %-17.17s %08x %08x %.14s\n",
                 (DWORD)ptr, ptr->hmemTaskQ, className,
                 (UINT)ptr->dwStyle, (UINT)ptr->winproc,
                 ptr->text ? debugstr_w(ptr->text) : "<null>");
        
        if (ptr->child) WIN_WalkWindows( ptr->child->hwndSelf, indent+1 );
        WIN_UpdateWndPtr(&ptr,ptr->next);
    }
}

/***********************************************************************
 *           WIN_UnlinkWindow
 *
 * Remove a window from the siblings linked list.
 */
BOOL WIN_UnlinkWindow( HWND hwnd )
{    
    WND *wndPtr, **ppWnd;
    BOOL ret = FALSE;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    else if(!wndPtr->parent)
    {
        WIN_ReleaseWndPtr(wndPtr);
        return FALSE;
    }

    ppWnd = &wndPtr->parent->child;
    while (*ppWnd && *ppWnd != wndPtr) ppWnd = &(*ppWnd)->next;
    if (*ppWnd)
    {
        *ppWnd = wndPtr->next;
        ret = TRUE;
    }
    WIN_ReleaseWndPtr(wndPtr);
    return ret;
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

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    else if(!wndPtr->parent)
    {
        WIN_ReleaseWndPtr(wndPtr);
        return FALSE;
    }
    if ((hwndInsertAfter == HWND_TOP) || (hwndInsertAfter == HWND_BOTTOM))
    {
        ppWnd = &wndPtr->parent->child;  /* Point to first sibling hwnd */
	if (hwndInsertAfter == HWND_BOTTOM)  /* Find last sibling hwnd */
	    while (*ppWnd) ppWnd = &(*ppWnd)->next;
    }
    else  /* Normal case */
    {
	WND * afterPtr = WIN_FindWndPtr( hwndInsertAfter );
            if (!afterPtr)
            {
                WIN_ReleaseWndPtr(wndPtr);
                return FALSE;
            }
        ppWnd = &afterPtr->next;
        WIN_ReleaseWndPtr(afterPtr);
    }
    wndPtr->next = *ppWnd;
    *ppWnd = wndPtr;
    WIN_ReleaseWndPtr(wndPtr);
    return TRUE;
}


/***********************************************************************
 *           WIN_FindWinToRepaint
 *
 * Find a window that needs repaint.
 */
HWND WIN_FindWinToRepaint( HWND hwnd )
{
    HWND hwndRet;
    WND *pWnd;

    /* Note: the desktop window never gets WM_PAINT messages 
     * The real reason why is because Windows DesktopWndProc
     * does ValidateRgn inside WM_ERASEBKGND handler.
     */
    if (hwnd == GetDesktopWindow()) hwnd = 0;

    pWnd = hwnd ? WIN_FindWndPtr(hwnd) : WIN_LockWndPtr(pWndDesktop->child);

    for ( ; pWnd ; WIN_UpdateWndPtr(&pWnd,pWnd->next))
    {
        if (!(pWnd->dwStyle & WS_VISIBLE))
        {
            TRACE("skipping window %04x\n",
                         pWnd->hwndSelf );
        }
        else if ((pWnd->hrgnUpdate || (pWnd->flags & WIN_INTERNAL_PAINT)) &&
                 GetWindowThreadProcessId( pWnd->hwndSelf, NULL ) == GetCurrentThreadId())
            break;
        
        else if (pWnd->child )
            if ((hwndRet = WIN_FindWinToRepaint( pWnd->child->hwndSelf )) )
            {
                WIN_ReleaseWndPtr(pWnd);
                return hwndRet;
    }
    
    }
    
    if(!pWnd)
    {
        return 0;
    }
    
    hwndRet = pWnd->hwndSelf;

    /* look among siblings if we got a transparent window */
    while (pWnd && ((pWnd->dwExStyle & WS_EX_TRANSPARENT) ||
                    !(pWnd->hrgnUpdate || (pWnd->flags & WIN_INTERNAL_PAINT))))
    {
        WIN_UpdateWndPtr(&pWnd,pWnd->next);
    }
    if (pWnd)
    {
        hwndRet = pWnd->hwndSelf;
    WIN_ReleaseWndPtr(pWnd);
    }
    TRACE("found %04x\n",hwndRet);
    return hwndRet;
}


/***********************************************************************
 *           WIN_DestroyWindow
 *
 * Destroy storage associated to a window. "Internals" p.358
 * returns a locked wndPtr->next
 */
static WND* WIN_DestroyWindow( WND* wndPtr )
{
    HWND hwnd = wndPtr->hwndSelf;
    WND *pWnd;

    TRACE("%04x\n", wndPtr->hwndSelf );

    /* free child windows */
    WIN_LockWndPtr(wndPtr->child);
    while ((pWnd = wndPtr->child))
    {
        wndPtr->child = WIN_DestroyWindow( pWnd );
        WIN_ReleaseWndPtr(pWnd);
    }

    /*
     * Clear the update region to make sure no WM_PAINT messages will be
     * generated for this window while processing the WM_NCDESTROY.
     */
    RedrawWindow( wndPtr->hwndSelf, NULL, 0,
                  RDW_VALIDATE | RDW_NOFRAME | RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_NOCHILDREN);

    /*
     * Send the WM_NCDESTROY to the window being destroyed.
     */
    SendMessageA( wndPtr->hwndSelf, WM_NCDESTROY, 0, 0);

    /* FIXME: do we need to fake QS_MOUSEMOVE wakebit? */

    WINPOS_CheckInternalPos( hwnd );
    if( hwnd == GetCapture()) ReleaseCapture();

    /* free resources associated with the window */

    TIMER_RemoveWindowTimers( wndPtr->hwndSelf );
    PROPERTY_RemoveWindowProps( wndPtr );

    /* toss stale messages from the queue */

    QUEUE_CleanupWindow( hwnd );
    wndPtr->hmemTaskQ = 0;

    if (!(wndPtr->dwStyle & WS_CHILD))
       if (wndPtr->wIDmenu)
       {
	   DestroyMenu( wndPtr->wIDmenu );
	   wndPtr->wIDmenu = 0;
       }
    if (wndPtr->hSysMenu)
    {
	DestroyMenu( wndPtr->hSysMenu );
	wndPtr->hSysMenu = 0;
    }
    USER_Driver.pDestroyWindow( wndPtr->hwndSelf );
    DCE_FreeWindowDCE( wndPtr );    /* Always do this to catch orphaned DCs */ 
    WINPROC_FreeProc( wndPtr->winproc, WIN_PROC_WINDOW );
    CLASS_RemoveWindow( wndPtr->class );
    wndPtr->class = NULL;
    wndPtr->dwMagic = 0;  /* Mark it as invalid */

    WIN_UpdateWndPtr(&pWnd,wndPtr->next);

    return pWnd;
}

/***********************************************************************
 *           WIN_DestroyThreadWindows
 *
 * Destroy all children of 'wnd' owned by the current thread.
 * Return TRUE if something was done.
 */
BOOL WIN_DestroyThreadWindows( HWND hwnd )
{
    BOOL ret = FALSE;
    WND *wnd = WIN_FindWndPtr( hwnd );

    if (!wnd) return FALSE;
    while (wnd->child)
    {
        WND *tmp = WIN_LockWndPtr(wnd->child);
        ret = FALSE;
        while (tmp)
        {
            if (GetWindowThreadProcessId( tmp->hwndSelf, NULL ) == GetCurrentThreadId())
            {
                DestroyWindow( tmp->hwndSelf );
                ret = TRUE;
                break;
            }
            if (tmp->child && WIN_DestroyThreadWindows( tmp->hwndSelf ))
                ret = TRUE;
            else
                WIN_UpdateWndPtr(&tmp,tmp->next);
        }
        WIN_ReleaseWndPtr(tmp);
        if (!ret) break;
    }
    WIN_ReleaseWndPtr( wnd );
    return ret;
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

    TRACE("Creating desktop window\n");


    if (!WINPOS_CreateInternalPosAtom() ||
        !(class = CLASS_AddWindow( (ATOM)LOWORD(DESKTOP_CLASS_ATOM), 0, WIN_PROC_32W,
                                   &wndExtra, &winproc, &clsStyle, &dce )))
        return FALSE;

    hwndDesktop = USER_HEAP_ALLOC( sizeof(WND) + wndExtra );
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
    pWndDesktop->rectWindow.right  = GetSystemMetrics(SM_CXSCREEN);
    pWndDesktop->rectWindow.bottom = GetSystemMetrics(SM_CYSCREEN);
    pWndDesktop->rectClient        = pWndDesktop->rectWindow;
    pWndDesktop->text              = NULL;
    pWndDesktop->hmemTaskQ         = 0;
    pWndDesktop->hrgnUpdate        = 0;
    pWndDesktop->hwndLastActive    = hwndDesktop;
    pWndDesktop->dwStyle           = WS_VISIBLE | WS_CLIPCHILDREN |
                                     WS_CLIPSIBLINGS;
    pWndDesktop->dwExStyle         = 0;
    pWndDesktop->clsStyle          = clsStyle;
    pWndDesktop->dce               = NULL;
    pWndDesktop->pVScroll          = NULL;
    pWndDesktop->pHScroll          = NULL;
    pWndDesktop->pProp             = NULL;
    pWndDesktop->wIDmenu           = 0;
    pWndDesktop->helpContext       = 0;
    pWndDesktop->flags             = 0;
    pWndDesktop->hSysMenu          = 0;
    pWndDesktop->userdata          = 0;
    pWndDesktop->winproc           = winproc;
    pWndDesktop->cbWndExtra        = wndExtra;
    pWndDesktop->irefCount         = 0;

    cs.lpCreateParams = NULL;
    cs.hInstance      = 0;
    cs.hMenu          = 0;
    cs.hwndParent     = 0;
    cs.x              = 0;
    cs.y              = 0;
    cs.cx             = pWndDesktop->rectWindow.right;
    cs.cy             = pWndDesktop->rectWindow.bottom;
    cs.style          = pWndDesktop->dwStyle;
    cs.dwExStyle      = pWndDesktop->dwExStyle;
    cs.lpszName       = NULL;
    cs.lpszClass      = DESKTOP_CLASS_ATOM;

    if (!USER_Driver.pCreateWindow( hwndDesktop, &cs, FALSE )) return FALSE;

    pWndDesktop->flags |= WIN_NEEDS_ERASEBKGND;
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
    HWND hwnd, hwndLinkAfter;
    POINT maxSize, maxPos, minTrack, maxTrack;
    INT wndExtra;
    DWORD clsStyle;
    WNDPROC winproc;
    DCE *dce;
    BOOL unicode = (type == WIN_PROC_32W);

    TRACE("%s %s %08lx %08lx %d,%d %dx%d %04x %04x %08x %p\n",
          (type == WIN_PROC_32W) ? debugres_w((LPWSTR)cs->lpszName) : debugres_a(cs->lpszName), 
          (type == WIN_PROC_32W) ? debugres_w((LPWSTR)cs->lpszClass) : debugres_a(cs->lpszClass),
          cs->dwExStyle, cs->style, cs->x, cs->y, cs->cx, cs->cy,
          cs->hwndParent, cs->hMenu, cs->hInstance, cs->lpCreateParams );

    TRACE("winproc type is %d (%s)\n", type, (type == WIN_PROC_16) ? "WIN_PROC_16" :
	    ((type == WIN_PROC_32A) ? "WIN_PROC_32A" : "WIN_PROC_32W") );

    /* Find the parent window */

    if (cs->hwndParent)
    {
	/* Make sure parent is valid */
        if (!IsWindow( cs->hwndParent ))
        {
            WARN("Bad parent %04x\n", cs->hwndParent );
	    return 0;
	}
    } else if ((cs->style & WS_CHILD) && !(cs->style & WS_POPUP)) {
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

    if (!(hwnd = USER_HEAP_ALLOC( sizeof(*wndPtr) + wndExtra - sizeof(wndPtr->wExtra) )))
    {
	TRACE("out of memory\n" );
	return 0;
    }

    /* Fill the window structure */

    wndPtr = WIN_LockWndPtr((WND *) USER_HEAP_LIN_ADDR( hwnd ));
    wndPtr->next  = NULL;
    wndPtr->child = NULL;

    if ((cs->style & WS_CHILD) && cs->hwndParent)
    {
        wndPtr->parent = WIN_FindWndPtr( cs->hwndParent );
        wndPtr->owner  = NULL;
        WIN_ReleaseWndPtr(wndPtr->parent);
    }
    else
    {
        wndPtr->parent = pWndDesktop;
        if (!cs->hwndParent || (cs->hwndParent == pWndDesktop->hwndSelf))
            wndPtr->owner = NULL;
        else
        {
            WND *tmpWnd = WIN_FindWndPtr(cs->hwndParent);
            wndPtr->owner = WIN_GetTopParentPtr(tmpWnd);
            WIN_ReleaseWndPtr(wndPtr->owner);
            WIN_ReleaseWndPtr(tmpWnd);
	}
    }
    

    wndPtr->class          = classPtr;
    wndPtr->winproc        = winproc;
    wndPtr->dwMagic        = WND_MAGIC;
    wndPtr->hwndSelf       = hwnd;
    wndPtr->hInstance      = cs->hInstance;
    wndPtr->text           = NULL;
    wndPtr->hmemTaskQ      = InitThreadInput16( 0, 0 );
    wndPtr->hrgnUpdate     = 0;
    wndPtr->hrgnWnd        = 0;
    wndPtr->hwndLastActive = hwnd;
    wndPtr->dwStyle        = cs->style & ~WS_VISIBLE;
    wndPtr->dwExStyle      = cs->dwExStyle;
    wndPtr->clsStyle       = clsStyle;
    wndPtr->wIDmenu        = 0;
    wndPtr->helpContext    = 0;
    wndPtr->flags          = (type == WIN_PROC_16) ? 0 : WIN_ISWIN32;
    wndPtr->pVScroll       = NULL;
    wndPtr->pHScroll       = NULL;
    wndPtr->pProp          = NULL;
    wndPtr->userdata       = 0;
    wndPtr->hSysMenu       = (wndPtr->dwStyle & WS_SYSMENU)
			     ? MENU_GetSysMenu( hwnd, 0 ) : 0;
    wndPtr->cbWndExtra     = wndExtra;
    wndPtr->irefCount      = 1;

    if (wndExtra) memset( wndPtr->wExtra, 0, wndExtra);

    /* Call the WH_CBT hook */

    hwndLinkAfter = ((cs->style & (WS_CHILD|WS_MAXIMIZE)) == WS_CHILD)
 ? HWND_BOTTOM : HWND_TOP;

    if (HOOK_IsHooked( WH_CBT ))
    {
	CBT_CREATEWNDA cbtc;
        LRESULT ret;

	cbtc.lpcs = cs;
	cbtc.hwndInsertAfter = hwndLinkAfter;
        ret = (type == WIN_PROC_32W) ? HOOK_CallHooksW(WH_CBT, HCBT_CREATEWND, hwnd, (LPARAM)&cbtc)
                      : HOOK_CallHooksA(WH_CBT, HCBT_CREATEWND, hwnd, (LPARAM)&cbtc);
        if (ret)
	{
	    TRACE("CBT-hook returned 0\n");
	    USER_HEAP_FREE( hwnd );
            CLASS_RemoveWindow( classPtr );
            hwnd =  0;
            goto end;
	}
    }

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

    /* Get class or window DC if needed */

    if (clsStyle & CS_OWNDC) wndPtr->dce = DCE_AllocDCE(hwnd,DCE_WINDOW_DC);
    else if (clsStyle & CS_CLASSDC) wndPtr->dce = dce;
    else wndPtr->dce = NULL;

    /* Initialize the dimensions before sending WM_GETMINMAXINFO */

    wndPtr->rectWindow.left   = cs->x;
    wndPtr->rectWindow.top    = cs->y;
    wndPtr->rectWindow.right  = cs->x + cs->cx;
    wndPtr->rectWindow.bottom = cs->y + cs->cy;
    wndPtr->rectClient        = wndPtr->rectWindow;

    /* Send the WM_GETMINMAXINFO message and fix the size if needed */

    if ((cs->style & WS_THICKFRAME) || !(cs->style & (WS_POPUP | WS_CHILD)))
    {
        WINPOS_GetMinMaxInfo( hwnd, &maxSize, &maxPos, &minTrack, &maxTrack);
        if (maxSize.x < cs->cx) cs->cx = maxSize.x;
        if (maxSize.y < cs->cy) cs->cy = maxSize.y;
        if (cs->cx < minTrack.x ) cs->cx = minTrack.x;
        if (cs->cy < minTrack.y ) cs->cy = minTrack.y;
    }

    if (cs->cx < 0) cs->cx = 0;
    if (cs->cy < 0) cs->cy = 0;

    wndPtr->rectWindow.left   = cs->x;
    wndPtr->rectWindow.top    = cs->y;
    wndPtr->rectWindow.right  = cs->x + cs->cx;
    wndPtr->rectWindow.bottom = cs->y + cs->cy;
    wndPtr->rectClient        = wndPtr->rectWindow;

    /* Set the window menu */

    if ((wndPtr->dwStyle & (WS_CAPTION | WS_CHILD)) == WS_CAPTION )
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
                    cs->hMenu = LoadMenu16(cs->hInstance,menuName);

                if (cs->hMenu) SetMenu( hwnd, cs->hMenu );
            }
        }
    }
    else wndPtr->wIDmenu = (UINT)cs->hMenu;

    if (!USER_Driver.pCreateWindow( wndPtr->hwndSelf, cs, unicode))
    {
        WARN("aborted by WM_xxCREATE!\n");
        WIN_ReleaseWndPtr(WIN_DestroyWindow( wndPtr ));
        CLASS_RemoveWindow( classPtr );
        WIN_ReleaseWndPtr(wndPtr);
        return 0;
    }

    if( (wndPtr->dwStyle & WS_CHILD) && !(wndPtr->dwExStyle & WS_EX_NOPARENTNOTIFY) )
    {
        /* Notify the parent window only */

        SendMessageA( wndPtr->parent->hwndSelf, WM_PARENTNOTIFY,
                      MAKEWPARAM(WM_CREATE, wndPtr->wIDmenu), (LPARAM)hwnd );
        if( !IsWindow(hwnd) )
        {
            hwnd = 0;
            goto end;
        }
    }

    if (cs->style & WS_VISIBLE)
    {
        /* in case WS_VISIBLE got set in the meantime */
        wndPtr->dwStyle &= ~WS_VISIBLE;
        ShowWindow( hwnd, sw );
    }

    /* Call WH_SHELL hook */

    if (!(wndPtr->dwStyle & WS_CHILD) && !wndPtr->owner)
        HOOK_CallHooksA( WH_SHELL, HSHELL_WINDOWCREATED, hwnd, 0 );

    TRACE("created window %04x\n", hwnd);
 end:
    WIN_ReleaseWndPtr(wndPtr);
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
            ERR( "bad class name %s\n", debugres_a(className) );
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
    cs.hInstance      = (HINSTANCE)instance;
    cs.hMenu          = (HMENU)menu;
    cs.hwndParent     = (HWND)parent;
    cs.style          = style;
    cs.lpszName       = windowName;
    cs.lpszClass      = className;
    cs.dwExStyle      = exStyle;

    return WIN_CreateWindowEx( &cs, classAtom, WIN_PROC_16 );
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
            ERR( "bad class name %s\n", debugres_a(className) );
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
        instance=GetModuleHandleA(NULL);

    if(exStyle & WS_EX_MDICHILD)
        return CreateMDIWindowW(className, windowName, style, x, y, width, height, parent, instance, (LPARAM)data);

    /* Find the class atom */

    if (HIWORD(className))
    {
        if (!(classAtom = GlobalFindAtomW( className )))
        {
            ERR( "bad class name %s\n", debugres_w(className) );
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
static void WIN_SendDestroyMsg( WND* pWnd )
{
    if( CARET_GetHwnd() == pWnd->hwndSelf ) DestroyCaret();
    if (USER_Driver.pResetSelectionOwner)
        USER_Driver.pResetSelectionOwner( pWnd->hwndSelf, TRUE );

    /*
     * Send the WM_DESTROY to the window.
     */
    SendMessageA( pWnd->hwndSelf, WM_DESTROY, 0, 0);

    /*
     * This WM_DESTROY message can trigger re-entrant calls to DestroyWindow
     * make sure that the window still exists when we come back.
     */
    if (IsWindow(pWnd->hwndSelf))
    {
      HWND* pWndArray = NULL;
      WND*  pChild    = NULL;
      int   nKidCount = 0;

      /*
       * Now, if the window has kids, we have to send WM_DESTROY messages 
       * recursively to it's kids. It seems that those calls can also
       * trigger re-entrant calls to DestroyWindow for the kids so we must 
       * protect against corruption of the list of siblings. We first build
       * a list of HWNDs representing all the kids.
       */
      pChild = WIN_LockWndPtr(pWnd->child);
      while( pChild )
      {
	nKidCount++;
	WIN_UpdateWndPtr(&pChild,pChild->next);
      }

      /*
       * If there are no kids, we're done.
       */
      if (nKidCount==0)
	return;

      pWndArray = HeapAlloc(GetProcessHeap(), 0, nKidCount*sizeof(HWND));

      /*
       * Sanity check
       */
      if (pWndArray==NULL)
	return;

      /*
       * Now, enumerate all the kids in a list, since we wait to make the SendMessage
       * call, our linked list of siblings should be safe.
       */
      nKidCount = 0;
      pChild = WIN_LockWndPtr(pWnd->child);
      while( pChild )
      {
	pWndArray[nKidCount] = pChild->hwndSelf;
	nKidCount++;
	WIN_UpdateWndPtr(&pChild,pChild->next);
      }

      /*
       * Now that we have a list, go through that list again and send the destroy
       * message to those windows. We are using the HWND to retrieve the
       * WND pointer so we are effectively checking that all the kid windows are
       * still valid before sending the message.
       */
      while (nKidCount>0)
      {
	pChild = WIN_FindWndPtr(pWndArray[--nKidCount]);

	if (pChild!=NULL)
	{
	  WIN_SendDestroyMsg( pChild );
	  WIN_ReleaseWndPtr(pChild);	  
	}
      }

      /*
       * Cleanup
       */
      HeapFree(GetProcessHeap(), 0, pWndArray);
    }
    else
      WARN("\tdestroyed itself while in WM_DESTROY!\n");
}


/***********************************************************************
 *		DestroyWindow (USER.53)
 */
BOOL16 WINAPI DestroyWindow16( HWND16 hwnd )
{
    return DestroyWindow(hwnd);
}


/***********************************************************************
 *		DestroyWindow (USER32.@)
 */
BOOL WINAPI DestroyWindow( HWND hwnd )
{
    WND * wndPtr;
    BOOL retvalue;
    HWND h;
    BOOL bFocusSet = FALSE;

    TRACE("(%04x)\n", hwnd);

    /* Initialization */

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (wndPtr == pWndDesktop)
    {
        retvalue = FALSE; /* Can't destroy desktop */
	goto end;
    }

    /* Look whether the focus is within the tree of windows we will
     * be destroying.
     */
    h = GetFocus16();
    while (h && (GetWindowLongA(h,GWL_STYLE) & WS_CHILD))
    {
	if (h == hwnd)
	{
	    SetFocus(GetParent(h));
	    bFocusSet = TRUE;
	    break;
	}
	h = GetParent(h);
    }
    /* If the focus is on the window we will destroy and it has no parent,
     * set the focus to 0.
     */
    if (! bFocusSet && (h == hwnd))
    {                   
	if (!(GetWindowLongA(h,GWL_STYLE) & WS_CHILD))
	    SetFocus(0);
    }

      /* Call hooks */

    if( HOOK_CallHooksA( WH_CBT, HCBT_DESTROYWND, hwnd, 0L) )
    {
        retvalue = FALSE;
        goto end;
    }

    if (!(wndPtr->dwStyle & WS_CHILD) && !wndPtr->owner)
    {
        HOOK_CallHooksA( WH_SHELL, HSHELL_WINDOWDESTROYED, hwnd, 0L );
        /* FIXME: clean up palette - see "Internals" p.352 */
    }

    if( !QUEUE_IsExitingQueue(wndPtr->hmemTaskQ) )
	if( wndPtr->dwStyle & WS_CHILD && !(wndPtr->dwExStyle & WS_EX_NOPARENTNOTIFY) )
	{
	    /* Notify the parent window only */
	    SendMessageA( wndPtr->parent->hwndSelf, WM_PARENTNOTIFY,
			    MAKEWPARAM(WM_DESTROY, wndPtr->wIDmenu), (LPARAM)hwnd );
            if( !IsWindow(hwnd) )
            {
                retvalue = TRUE;
                goto end;
            }
	}

    if (USER_Driver.pResetSelectionOwner)
        USER_Driver.pResetSelectionOwner( hwnd, FALSE ); /* before the window is unmapped */

      /* Hide the window */

    if (wndPtr->dwStyle & WS_VISIBLE)
    {
        ShowWindow( hwnd, SW_HIDE );
        if (!IsWindow(hwnd))
        {
            retvalue = TRUE;
            goto end;
        }
    }

      /* Recursively destroy owned windows */

    if( !(wndPtr->dwStyle & WS_CHILD) )
    {
      for (;;)
      {
        WND *siblingPtr = WIN_LockWndPtr(wndPtr->parent->child);  /* First sibling */
        while (siblingPtr)
        {
            if (siblingPtr->owner == wndPtr)
	    {
               if (siblingPtr->hmemTaskQ == wndPtr->hmemTaskQ)
                   break;
               else 
                   siblingPtr->owner = NULL;
	    }
            WIN_UpdateWndPtr(&siblingPtr,siblingPtr->next);
        }
        if (siblingPtr)
        {
            DestroyWindow( siblingPtr->hwndSelf );
            WIN_ReleaseWndPtr(siblingPtr);
        }
        else break;
      }

      WINPOS_ActivateOtherWindow(wndPtr->hwndSelf);

      if( wndPtr->owner &&
	  wndPtr->owner->hwndLastActive == wndPtr->hwndSelf )
	  wndPtr->owner->hwndLastActive = wndPtr->owner->hwndSelf;
    }

      /* Send destroy messages */

    WIN_SendDestroyMsg( wndPtr );
    if (!IsWindow(hwnd))
    {
        retvalue = TRUE;
        goto end;
    }

      /* Unlink now so we won't bother with the children later on */

    if( wndPtr->parent ) WIN_UnlinkWindow(hwnd);

      /* Destroy the window storage */

    WIN_ReleaseWndPtr(WIN_DestroyWindow( wndPtr ));
    retvalue = TRUE;
end:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/***********************************************************************
 *		CloseWindow (USER.43)
 */
BOOL16 WINAPI CloseWindow16( HWND16 hwnd )
{
    return CloseWindow( hwnd );
}

 
/***********************************************************************
 *		CloseWindow (USER32.@)
 */
BOOL WINAPI CloseWindow( HWND hwnd )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    BOOL retvalue;
    
    if (!wndPtr || (wndPtr->dwStyle & WS_CHILD))
    {
        retvalue = FALSE;
        goto end;
    }
    ShowWindow( hwnd, SW_MINIMIZE );
    retvalue = TRUE;
end:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;

}

 
/***********************************************************************
 *		OpenIcon (USER.44)
 */
BOOL16 WINAPI OpenIcon16( HWND16 hwnd )
{
    return OpenIcon( hwnd );
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
static HWND WIN_FindWindow( HWND parent, HWND child, ATOM className,
                              LPCWSTR title )
{
    WND *pWnd;
    HWND retvalue;

    if (child)
    {
        if (!(pWnd = WIN_FindWndPtr( child ))) return 0;
        if (parent)
        {
            if (!pWnd->parent || (pWnd->parent->hwndSelf != parent))
            {
                retvalue = 0;
                goto end;
            }
        }
        else if (pWnd->parent != pWndDesktop)
        {
            retvalue = 0;
            goto end;
        }
        WIN_UpdateWndPtr(&pWnd,pWnd->next);
    }
    else
    {
        if (!(pWnd = parent ? WIN_FindWndPtr(parent) : WIN_LockWndPtr(pWndDesktop)))
        {
            retvalue = 0;
            goto end;
        }
        WIN_UpdateWndPtr(&pWnd,pWnd->child);
    }
    if (!pWnd)
    {
        retvalue = 0;
        goto end;
    }

    for ( ; pWnd ; WIN_UpdateWndPtr(&pWnd,pWnd->next))
    {
        if (className && (GetClassWord(pWnd->hwndSelf, GCW_ATOM) != className))
            continue;  /* Not the right class */

        /* Now check the title */

        if (!title)
        {
            retvalue = pWnd->hwndSelf;
            goto end;
        }
        if (pWnd->text && !strcmpW( pWnd->text, title ))
        {
            retvalue = pWnd->hwndSelf;
            goto end;
        }
    }
    retvalue = 0;
    /* In this case we need to check whether other processes
       own a window with the given paramters on the Desktop,
       but we don't, so let's at least warn about it */
    FIXME("Returning 0 without checking other processes\n");
end:
    WIN_ReleaseWndPtr(pWnd);
    return retvalue;
}



/***********************************************************************
 *		FindWindow (USER.50)
 */
HWND16 WINAPI FindWindow16( LPCSTR className, LPCSTR title )
{
    return FindWindowA( className, title );
}


/***********************************************************************
 *		FindWindowEx (USER.427)
 */
HWND16 WINAPI FindWindowEx16( HWND16 parent, HWND16 child, LPCSTR className, LPCSTR title )
{
    return FindWindowExA( parent, child, className, title );
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

    buffer = HEAP_strdupAtoW( GetProcessHeap(), 0, title );
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
 *           WIN_GetDesktop
 * returns a locked pointer
 */
WND *WIN_GetDesktop(void)
{
    return WIN_LockWndPtr(pWndDesktop);
}
/**********************************************************************
 *           WIN_ReleaseDesktop
 * unlock the desktop pointer
 */
void WIN_ReleaseDesktop(void)
{
    WIN_ReleaseWndPtr(pWndDesktop);
}


/**********************************************************************
 *		GetDesktopWindow (USER.286)
 */
HWND16 WINAPI GetDesktopWindow16(void)
{
    return (HWND16)pWndDesktop->hwndSelf;
}


/**********************************************************************
 *		GetDesktopWindow (USER32.@)
 */
HWND WINAPI GetDesktopWindow(void)
{
    if (pWndDesktop) return pWndDesktop->hwndSelf;
    ERR( "You need the -desktop option when running with native USER\n" );
    ExitProcess(1);
    return 0;
}


/**********************************************************************
 *		GetDesktopHwnd (USER.278)
 *
 * Exactly the same thing as GetDesktopWindow(), but not documented.
 * Don't ask me why...
 */
HWND16 WINAPI GetDesktopHwnd16(void)
{
    return (HWND16)pWndDesktop->hwndSelf;
}


/*******************************************************************
 *		EnableWindow (USER.34)
 */
BOOL16 WINAPI EnableWindow16( HWND16 hwnd, BOOL16 enable )
{
    return EnableWindow( hwnd, enable );
}


/*******************************************************************
 *		EnableWindow (USER32.@)
 */
BOOL WINAPI EnableWindow( HWND hwnd, BOOL enable )
{
    WND *wndPtr;
    BOOL retvalue;

    TRACE("( %x, %d )\n", hwnd, enable);

    if (USER_Driver.pEnableWindow)
        return USER_Driver.pEnableWindow( hwnd, enable );

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;

    retvalue = ((wndPtr->dwStyle & WS_DISABLED) != 0);

    if (enable && (wndPtr->dwStyle & WS_DISABLED))
    {
        wndPtr->dwStyle &= ~WS_DISABLED; /* Enable window */
        SendMessageA( hwnd, WM_ENABLE, TRUE, 0 );
    }
    else if (!enable && !(wndPtr->dwStyle & WS_DISABLED))
    {
        SendMessageA( wndPtr->hwndSelf, WM_CANCELMODE, 0, 0);

        wndPtr->dwStyle |= WS_DISABLED; /* Disable window */

        if (hwnd == GetFocus())
            SetFocus( 0 );  /* A disabled window can't have the focus */

        if (hwnd == GetCapture())
            ReleaseCapture();  /* A disabled window can't capture the mouse */

        SendMessageA( hwnd, WM_ENABLE, FALSE, 0 );
    }
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/***********************************************************************
 *		IsWindowEnabled (USER.35)
 */ 
BOOL16 WINAPI IsWindowEnabled16(HWND16 hWnd)
{
    return IsWindowEnabled(hWnd);
}


/***********************************************************************
 *		IsWindowEnabled (USER32.@)
 */ 
BOOL WINAPI IsWindowEnabled(HWND hWnd)
{
    WND * wndPtr; 
    BOOL retvalue;

    if (!(wndPtr = WIN_FindWndPtr(hWnd))) return FALSE;
    retvalue = !(wndPtr->dwStyle & WS_DISABLED);
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;

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
 *		GetWindowWord (USER.133)
 */
WORD WINAPI GetWindowWord16( HWND16 hwnd, INT16 offset )
{
    return GetWindowWord( hwnd, offset );
}


/**********************************************************************
 *		GetWindowWord (USER32.@)
 */
WORD WINAPI GetWindowWord( HWND hwnd, INT offset )
{
    WORD retvalue;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(WORD) > wndPtr->cbWndExtra)
        {
            WARN("Invalid offset %d\n", offset );
            retvalue = 0;
            goto end;
        }
        retvalue = *(WORD *)(((char *)wndPtr->wExtra) + offset);
        goto end;
    }
    switch(offset)
    {
    case GWW_ID:         
    	if (HIWORD(wndPtr->wIDmenu))
    		WARN("GWW_ID: discards high bits of 0x%08x!\n",
                    wndPtr->wIDmenu);
        retvalue = (WORD)wndPtr->wIDmenu;
        goto end;
    case GWW_HWNDPARENT: 
    	retvalue =  GetParent(hwnd);
        goto end;
    case GWW_HINSTANCE:  
    	if (HIWORD(wndPtr->hInstance))
    		WARN("GWW_HINSTANCE: discards high bits of 0x%08x!\n",
                    wndPtr->hInstance);
        retvalue = (WORD)wndPtr->hInstance;
        goto end;
    default:
        WARN("Invalid offset %d\n", offset );
        retvalue = 0;
        goto end;
    }
end:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}

/**********************************************************************
 *		SetWindowWord (USER.134)
 */
WORD WINAPI SetWindowWord16( HWND16 hwnd, INT16 offset, WORD newval )
{
    return SetWindowWord( hwnd, offset, newval );
}


/**********************************************************************
 *		SetWindowWord (USER32.@)
 */
WORD WINAPI SetWindowWord( HWND hwnd, INT offset, WORD newval )
{
    WORD *ptr, retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(WORD) > wndPtr->cbWndExtra)
        {
            WARN("Invalid offset %d\n", offset );
            retval = 0;
            goto end;
        }
        ptr = (WORD *)(((char *)wndPtr->wExtra) + offset);
    }
    else switch(offset)
    {
	case GWW_ID:        ptr = (WORD *)&wndPtr->wIDmenu; break;
	case GWW_HINSTANCE: ptr = (WORD *)&wndPtr->hInstance; break;
        case GWW_HWNDPARENT: retval = SetParent( hwnd, newval );
                             goto end;
	default:
            WARN("Invalid offset %d\n", offset );
            retval = 0;
            goto end;
    }
    retval = *ptr;
    *ptr = newval;
end:
    WIN_ReleaseWndPtr(wndPtr);
    return retval;
}


/**********************************************************************
 *	     WIN_GetWindowLong
 *
 * Helper function for GetWindowLong().
 */
static LONG WIN_GetWindowLong( HWND hwnd, INT offset, WINDOWPROCTYPE type )
{
    LONG retvalue;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(LONG) > wndPtr->cbWndExtra)
        {
            WARN("Invalid offset %d\n", offset );
            retvalue = 0;
            goto end;
        }
        retvalue = *(LONG *)(((char *)wndPtr->wExtra) + offset);
        /* Special case for dialog window procedure */
        if ((offset == DWL_DLGPROC) && (wndPtr->flags & WIN_ISDIALOG))
        {
            retvalue = (LONG)WINPROC_GetProc( (HWINDOWPROC)retvalue, type );
            goto end;
    }
        goto end;
    }
    switch(offset)
    {
        case GWL_USERDATA:   retvalue = wndPtr->userdata;
                             goto end;
        case GWL_STYLE:      retvalue = wndPtr->dwStyle;
                             goto end;
        case GWL_EXSTYLE:    retvalue = wndPtr->dwExStyle;
                             goto end;
        case GWL_ID:         retvalue = (LONG)wndPtr->wIDmenu;
                             goto end;
        case GWL_WNDPROC:    retvalue = (LONG)WINPROC_GetProc( wndPtr->winproc,
                                                           type );
                             goto end;
        case GWL_HWNDPARENT: retvalue = GetParent(hwnd);
                             goto end;
        case GWL_HINSTANCE:  retvalue = wndPtr->hInstance;
                             goto end;
        default:
            WARN("Unknown offset %d\n", offset );
    }
    retvalue = 0;
end:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/**********************************************************************
 *	     WIN_SetWindowLong
 *
 * Helper function for SetWindowLong().
 *
 * 0 is the failure code. However, in the case of failure SetLastError
 * must be set to distinguish between a 0 return value and a failure.
 *
 * FIXME: The error values for SetLastError may not be right. Can
 *        someone check with the real thing?
 */
static LONG WIN_SetWindowLong( HWND hwnd, INT offset, LONG newval,
                               WINDOWPROCTYPE type )
{
    LONG *ptr, retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    STYLESTRUCT style;
    
    TRACE("%x=%p %x %lx %x\n",hwnd, wndPtr, offset, newval, type);

    if (!wndPtr)
    {
       /* Is this the right error? */
       SetLastError( ERROR_INVALID_WINDOW_HANDLE );
       return 0;
    }

    if (offset >= 0)
    {
        if (offset + sizeof(LONG) > wndPtr->cbWndExtra)
        {
            WARN("Invalid offset %d\n", offset );

            /* Is this the right error? */
            SetLastError( ERROR_OUTOFMEMORY );

            retval = 0;
            goto end;
        }
        ptr = (LONG *)(((char *)wndPtr->wExtra) + offset);
        /* Special case for dialog window procedure */
        if ((offset == DWL_DLGPROC) && (wndPtr->flags & WIN_ISDIALOG))
        {
            retval = (LONG)WINPROC_GetProc( (HWINDOWPROC)*ptr, type );
            WINPROC_SetProc( (HWINDOWPROC *)ptr, (WNDPROC16)newval, 
                             type, WIN_PROC_WINDOW );
            goto end;
        }
    }
    else switch(offset)
    {
	case GWL_ID:
		ptr = (DWORD*)&wndPtr->wIDmenu;
		break;
	case GWL_HINSTANCE:
                retval = SetWindowWord( hwnd, offset, newval );
                goto end;
	case GWL_WNDPROC:
		retval = (LONG)WINPROC_GetProc( wndPtr->winproc, type );
		WINPROC_SetProc( &wndPtr->winproc, (WNDPROC16)newval, 
						type, WIN_PROC_WINDOW );
		goto end;
	case GWL_STYLE:
	       	style.styleOld = wndPtr->dwStyle;
		style.styleNew = newval;
                SendMessageA(hwnd,WM_STYLECHANGING,GWL_STYLE,(LPARAM)&style);
		wndPtr->dwStyle = style.styleNew;
                SendMessageA(hwnd,WM_STYLECHANGED,GWL_STYLE,(LPARAM)&style);
                retval = style.styleOld;
                goto end;
		    
        case GWL_USERDATA: 
		ptr = &wndPtr->userdata; 
		break;
        case GWL_EXSTYLE:  
	        style.styleOld = wndPtr->dwExStyle;
		style.styleNew = newval;
                SendMessageA(hwnd,WM_STYLECHANGING,GWL_EXSTYLE,(LPARAM)&style);
		wndPtr->dwExStyle = newval;
                SendMessageA(hwnd,WM_STYLECHANGED,GWL_EXSTYLE,(LPARAM)&style);
                retval = style.styleOld;
                goto end;

	default:
            WARN("Invalid offset %d\n", offset );

            /* Don't think this is right error but it should do */
            SetLastError( ERROR_OUTOFMEMORY );

            retval = 0;
            goto end;
    }
    retval = *ptr;
    *ptr = newval;
end:
    WIN_ReleaseWndPtr(wndPtr);
    return retval;
}


/**********************************************************************
 *		GetWindowLong (USER.135)
 */
LONG WINAPI GetWindowLong16( HWND16 hwnd, INT16 offset )
{
    return WIN_GetWindowLong( (HWND)hwnd, offset, WIN_PROC_16 );
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
    return WIN_SetWindowLong( hwnd, offset, newval, WIN_PROC_16 );
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
 * Do not use GWL_STYLE to change the window's WS_DISABLE style;
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
 *
 * BUGS
 *
 * GWL_STYLE does not dispatch WM_STYLE... messages.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win32 
 *
 */
LONG WINAPI SetWindowLongW( 
    HWND hwnd,  /* [in] window to alter */
    INT offset, /* [in] offset, in bytes, of location to alter */
    LONG newval /* [in] new value of location */
) {
    return WIN_SetWindowLong( hwnd, offset, newval, WIN_PROC_32W );
}


/*******************************************************************
 *		GetWindowText (USER.36)
 */
INT16 WINAPI GetWindowText16( HWND16 hwnd, SEGPTR lpString, INT16 nMaxCount )
{
    return (INT16)SendMessage16(hwnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString);
}


/*******************************************************************
 *		GetWindowTextA (USER32.@)
 */
INT WINAPI GetWindowTextA( HWND hwnd, LPSTR lpString, INT nMaxCount )
{
    return (INT)SendMessageA( hwnd, WM_GETTEXT, nMaxCount,
                                  (LPARAM)lpString );
}

/*******************************************************************
 *		InternalGetWindowText (USER32.@)
 */
INT WINAPI InternalGetWindowText(HWND hwnd,LPWSTR lpString,INT nMaxCount )
{
    WND *win = WIN_FindWndPtr( hwnd );
    if (!win) return 0;
    if (win->text) lstrcpynW( lpString, win->text, nMaxCount );
    else lpString[0] = 0;
    WIN_ReleaseWndPtr( win );
    return strlenW(lpString);
}


/*******************************************************************
 *		GetWindowTextW (USER32.@)
 */
INT WINAPI GetWindowTextW( HWND hwnd, LPWSTR lpString, INT nMaxCount )
{
    return (INT)SendMessageW( hwnd, WM_GETTEXT, nMaxCount,
                                  (LPARAM)lpString );
}


/*******************************************************************
 *		SetWindowText (USER.37)
 */
BOOL16 WINAPI SetWindowText16( HWND16 hwnd, SEGPTR lpString )
{
    return (BOOL16)SendMessage16( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *		SetWindowText  (USER32.@)
 *		SetWindowTextA (USER32.@)
 */
BOOL WINAPI SetWindowTextA( HWND hwnd, LPCSTR lpString )
{
    return (BOOL)SendMessageA( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *		SetWindowTextW (USER32.@)
 */
BOOL WINAPI SetWindowTextW( HWND hwnd, LPCWSTR lpString )
{
    return (BOOL)SendMessageW( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *		GetWindowTextLength (USER.38)
 */
INT16 WINAPI GetWindowTextLength16( HWND16 hwnd )
{
    return (INT16)GetWindowTextLengthA( hwnd );
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
 *		IsWindow (USER.47)
 */
BOOL16 WINAPI IsWindow16( HWND16 hwnd )
{
    CURRENT_STACK16->es = USER_HeapSel;
    return IsWindow( hwnd );
}


/*******************************************************************
 *		IsWindow (USER32.@)
 */
BOOL WINAPI IsWindow( HWND hwnd )
{
    WND * wndPtr;
    BOOL retvalue;
    
    if(!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    retvalue = (wndPtr->dwMagic == WND_MAGIC);
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
    
}


/*****************************************************************
 *		GetParent (USER.46)
 */
HWND16 WINAPI GetParent16( HWND16 hwnd )
{
    return (HWND16)GetParent( hwnd );
}


/*****************************************************************
 *		GetParent (USER32.@)
 */
HWND WINAPI GetParent( HWND hwnd )
{
    WND *wndPtr;
    HWND retvalue = 0;
    
    if(!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
    if ((!(wndPtr->dwStyle & (WS_POPUP|WS_CHILD))))
	goto end;

    WIN_UpdateWndPtr(&wndPtr,((wndPtr->dwStyle & WS_CHILD) ? wndPtr->parent : wndPtr->owner));
    if (wndPtr)
        retvalue = wndPtr->hwndSelf;

end:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
    
}

/*****************************************************************
 *         WIN_GetTopParent
 *
 * Get the top-level parent for a child window.
 * returns a locked pointer
 */
WND* WIN_GetTopParentPtr( WND* pWnd )
{
    WND *tmpWnd = WIN_LockWndPtr(pWnd);
    
    while( tmpWnd && (tmpWnd->dwStyle & WS_CHILD))
    {
        WIN_UpdateWndPtr(&tmpWnd,tmpWnd->parent);
    }
    return tmpWnd;
}

/*****************************************************************
 *         WIN_GetTopParent
 *
 * Get the top-level parent for a child window.
 */
HWND WIN_GetTopParent( HWND hwnd )
{
    HWND retvalue;
    WND *tmpPtr = WIN_FindWndPtr(hwnd);
    WND *wndPtr = WIN_GetTopParentPtr (tmpPtr );
    
    retvalue = wndPtr ? wndPtr->hwndSelf : 0;
    WIN_ReleaseWndPtr(tmpPtr);
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/*****************************************************************
 *		SetParent (USER.233)
 */
HWND16 WINAPI SetParent16( HWND16 hwndChild, HWND16 hwndNewParent )
{
    return SetParent( hwndChild, hwndNewParent );
}


/*****************************************************************
 *		SetParent (USER32.@)
 */
HWND WINAPI SetParent( HWND hwnd, HWND parent )
{
    WND *wndPtr;
    WND *pWndParent;
    DWORD dwStyle;
    HWND retvalue;

    if (hwnd == GetDesktopWindow()) /* sanity check */
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    if (USER_Driver.pSetParent)
        return USER_Driver.pSetParent( hwnd, parent );

    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;

    dwStyle = wndPtr->dwStyle;

    if (!parent) parent = GetDesktopWindow();

    if (!(pWndParent = WIN_FindWndPtr(parent)))
    {
        WIN_ReleaseWndPtr( wndPtr );
        return 0;
    }

    /* Windows hides the window first, then shows it again
     * including the WM_SHOWWINDOW messages and all */
    if (dwStyle & WS_VISIBLE) ShowWindow( hwnd, SW_HIDE );

    retvalue = wndPtr->parent->hwndSelf;  /* old parent */
    if (pWndParent != wndPtr->parent)
    {
        WIN_UnlinkWindow(wndPtr->hwndSelf);
        wndPtr->parent = pWndParent;

        if (parent != GetDesktopWindow()) /* a child window */
        {
            if( !( wndPtr->dwStyle & WS_CHILD ) )
            {
                if( wndPtr->wIDmenu != 0)
                {
                    DestroyMenu( (HMENU) wndPtr->wIDmenu );
                    wndPtr->wIDmenu = 0;
                }
            }
        }
        WIN_LinkWindow(wndPtr->hwndSelf, HWND_TOP);
    }
    WIN_ReleaseWndPtr( pWndParent );
    WIN_ReleaseWndPtr( wndPtr );

    /* SetParent additionally needs to make hwnd the topmost window
       in the x-order and send the expected WM_WINDOWPOSCHANGING and
       WM_WINDOWPOSCHANGED notification messages. 
    */
    SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                  SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|
                  ((dwStyle & WS_VISIBLE)?SWP_SHOWWINDOW:0));
    /* FIXME: a WM_MOVE is also generated (in the DefWindowProc handler
     * for WM_WINDOWPOSCHANGED) in Windows, should probably remove SWP_NOMOVE */
    return retvalue;
}


/*******************************************************************
 *		IsChild (USER.48)
 */
BOOL16 WINAPI IsChild16( HWND16 parent, HWND16 child )
{
    return IsChild(parent,child);
}


/*******************************************************************
 *		IsChild (USER32.@)
 */
BOOL WINAPI IsChild( HWND parent, HWND child )
{
    WND * wndPtr = WIN_FindWndPtr( child );
    while (wndPtr && wndPtr->parent)
    {
        WIN_UpdateWndPtr(&wndPtr,wndPtr->parent);
        if (wndPtr->hwndSelf == GetDesktopWindow()) break;
        if (wndPtr->hwndSelf == parent)
        {
            WIN_ReleaseWndPtr(wndPtr);
            return TRUE;
        }
    }
    WIN_ReleaseWndPtr(wndPtr);
    return FALSE;
}


/***********************************************************************
 *		IsWindowVisible (USER.49)
 */
BOOL16 WINAPI IsWindowVisible16( HWND16 hwnd )
{
    return IsWindowVisible(hwnd);
}


/***********************************************************************
 *		IsWindowVisible (USER32.@)
 */
BOOL WINAPI IsWindowVisible( HWND hwnd )
{
    BOOL retval;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    while (wndPtr && wndPtr->parent)
    {
        if (!(wndPtr->dwStyle & WS_VISIBLE))
    {
            WIN_ReleaseWndPtr(wndPtr);
            return FALSE;
        }
        WIN_UpdateWndPtr(&wndPtr,wndPtr->parent);
    }
    retval = (wndPtr && (wndPtr->dwStyle & WS_VISIBLE));
    WIN_ReleaseWndPtr(wndPtr);
    return retval;
}


/***********************************************************************
 *           WIN_IsWindowDrawable
 *
 * hwnd is drawable when it is visible, all parents are not
 * minimized, and it is itself not minimized unless we are
 * trying to draw its default class icon.
 */
BOOL WIN_IsWindowDrawable( WND* wnd, BOOL icon )
{
  if (!(wnd->dwStyle & WS_VISIBLE)) return FALSE;
  if ((wnd->dwStyle & WS_MINIMIZE) &&
       icon && GetClassLongA( wnd->hwndSelf, GCL_HICON ))  return FALSE;
  for(wnd = wnd->parent; wnd; wnd = wnd->parent)
    if( wnd->dwStyle & WS_MINIMIZE ||
      !(wnd->dwStyle & WS_VISIBLE) ) break;
  return (wnd == NULL);
}


/*******************************************************************
 *		GetTopWindow (USER.229)
 */
HWND16 WINAPI GetTopWindow16( HWND16 hwnd )
{
    return GetTopWindow(hwnd);
}


/*******************************************************************
 *		GetTopWindow (USER32.@)
 */
HWND WINAPI GetTopWindow( HWND hwnd )
{
    HWND retval = 0;
    WND * wndPtr = (hwnd) ?
	           WIN_FindWndPtr( hwnd ) : WIN_GetDesktop();

    if (wndPtr && wndPtr->child)
        retval = wndPtr->child->hwndSelf;

    WIN_ReleaseWndPtr(wndPtr);
    return retval;
}


/*******************************************************************
 *		GetWindow (USER.262)
 */
HWND16 WINAPI GetWindow16( HWND16 hwnd, WORD rel )
{
    return GetWindow( hwnd,rel );
}


/*******************************************************************
 *		GetWindow (USER32.@)
 */
HWND WINAPI GetWindow( HWND hwnd, WORD rel )
{
    HWND retval;
    
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    switch(rel)
    {
    case GW_HWNDFIRST:
	retval = wndPtr->parent ? wndPtr->parent->child->hwndSelf : 0;
        goto end;
	
    case GW_HWNDLAST:
        if (!wndPtr->parent)
        {
            retval = 0;  /* Desktop window */
            goto end;
        }
        while (wndPtr->next)
        {
            WIN_UpdateWndPtr(&wndPtr,wndPtr->next);
        }
        retval = wndPtr->hwndSelf;
        goto end;
	
    case GW_HWNDNEXT:
	retval = wndPtr->next ? wndPtr->next->hwndSelf : 0;
        goto end;
	
    case GW_HWNDPREV:
        if (!wndPtr->parent)
        {
            retval = 0;  /* Desktop window */
            goto end;
        }
        WIN_UpdateWndPtr(&wndPtr,wndPtr->parent->child); /* First sibling */
        if (wndPtr->hwndSelf == hwnd)
        {
            retval = 0;  /* First in list */
            goto end;
        }
        while (wndPtr->next)
        {
            if (wndPtr->next->hwndSelf == hwnd)
            {
                retval = wndPtr->hwndSelf;
                goto end;
            }
            WIN_UpdateWndPtr(&wndPtr,wndPtr->next);
        }
        retval = 0;
        goto end;
	
    case GW_OWNER:
        retval = wndPtr->owner ? wndPtr->owner->hwndSelf : 0;
        goto end;

    case GW_CHILD:
        retval = wndPtr->child ? wndPtr->child->hwndSelf : 0;
        goto end;
    }
    retval = 0;
end:
    WIN_ReleaseWndPtr(wndPtr);
    return retval;
}


/*******************************************************************
 *		GetNextWindow (USER.230)
 */
HWND16 WINAPI GetNextWindow16( HWND16 hwnd, WORD flag )
{
    if ((flag != GW_HWNDNEXT) && (flag != GW_HWNDPREV)) return 0;
    return GetWindow16( hwnd, flag );
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
    HWND *win_array = WIN_BuildWinArray( GetDesktopWindow() );

    if (!win_array) return TRUE;

    /*
     * Show windows Lowest first, Highest last to preserve Z-Order
     */
    while (win_array[count]) count++;
    while (--count >= 0)
    {
        WND *pWnd = WIN_FindWndPtr( win_array[count] );
        if (!pWnd) continue;

        if (pWnd->owner && (pWnd->owner->hwndSelf == owner) && (pWnd->dwStyle & WS_POPUP))
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
    WIN_ReleaseWinArray( win_array );

    return TRUE;
}

/*******************************************************************
 *		ShowOwnedPopups (USER.265)
 */
void WINAPI ShowOwnedPopups16( HWND16 owner, BOOL16 fShow )
{
    ShowOwnedPopups( owner, fShow );
}


/*******************************************************************
 *		ShowOwnedPopups (USER32.@)
 */
BOOL WINAPI ShowOwnedPopups( HWND owner, BOOL fShow )
{
    int count = 0;

    HWND *win_array = WIN_BuildWinArray(GetDesktopWindow());

    if (!win_array) return TRUE;

    while (win_array[count]) count++;
    while (--count >= 0)
    {
        WND *pWnd = WIN_FindWndPtr( win_array[count] );
        if (!pWnd) continue;

        if (pWnd->owner && (pWnd->owner->hwndSelf == owner) && (pWnd->dwStyle & WS_POPUP))
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
    WIN_ReleaseWinArray( win_array );
    return TRUE;
}


/*******************************************************************
 *		GetLastActivePopup (USER.287)
 */
HWND16 WINAPI GetLastActivePopup16( HWND16 hwnd )
{
    return GetLastActivePopup( hwnd );
}

/*******************************************************************
 *		GetLastActivePopup (USER32.@)
 */
HWND WINAPI GetLastActivePopup( HWND hwnd )
{
    HWND retval;
    WND *wndPtr =WIN_FindWndPtr(hwnd);
    if (!wndPtr) return hwnd;
    retval = wndPtr->hwndLastActive;
    WIN_ReleaseWndPtr(wndPtr);
    if ((retval != hwnd) && (!IsWindow(retval)))
       retval = hwnd;
    return retval;
}


/*******************************************************************
 *           WIN_BuildWinArray
 *
 * Build an array of pointers to the children of a given window.
 * The array must be freed with WIN_ReleaseWinArray. Return NULL
 * when no windows are found.
 */
HWND *WIN_BuildWinArray( HWND hwnd )
{
    /* Future: this function will lock all windows associated with this array */
    WND *pWnd, *wndPtr = WIN_FindWndPtr( hwnd );
    HWND *list, *phwnd;
    UINT count = 0;

    /* First count the windows */

    if (!wndPtr) return NULL;

    pWnd = WIN_LockWndPtr(wndPtr->child);
    while (pWnd)
    {
        count++;
        WIN_UpdateWndPtr(&pWnd,pWnd->next);
    }

    if( count )
    {
	/* Now build the list of all windows */

	if ((list = HeapAlloc( GetProcessHeap(), 0, sizeof(HWND) * (count + 1))))
	{
	    for (pWnd = WIN_LockWndPtr(wndPtr->child), phwnd = list, count = 0; pWnd; WIN_UpdateWndPtr(&pWnd,pWnd->next))
	    {
                *phwnd++ = pWnd->hwndSelf;
                count++;
	    }
            WIN_ReleaseWndPtr(pWnd);
            *phwnd = 0;
	}
	else count = 0;
    } else list = NULL;

    WIN_ReleaseWndPtr( wndPtr );
    return list;
}

/*******************************************************************
 *           WIN_ReleaseWinArray
 */
void WIN_ReleaseWinArray(HWND *wndArray)
{
    if (wndArray) HeapFree( GetProcessHeap(), 0, wndArray );
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

    if (!(list = WIN_BuildWinArray( GetDesktopWindow() ))) return FALSE;

    /* Now call the callback function for every window */

    iWndsLocks = WIN_SuspendWndsLock();
    for (i = 0; list[i]; i++)
    {
        /* Make sure that the window still exists */
        if (!IsWindow( list[i] )) continue;
        if (!(ret = lpEnumFunc( list[i], lParam ))) break;
    }
    WIN_RestoreWndsLock(iWndsLocks);
    WIN_ReleaseWinArray(list);
    return ret;
}


/**********************************************************************
 *		EnumTaskWindows16   (USER.225)
 */
BOOL16 WINAPI EnumTaskWindows16( HTASK16 hTask, WNDENUMPROC16 func,
                                 LPARAM lParam )
{
    TDB *tdb = TASK_GetPtr( hTask );
    if (!tdb) return FALSE;
    return EnumThreadWindows( (DWORD)tdb->teb->tid, (WNDENUMPROC)func, lParam );
}


/**********************************************************************
 *		EnumThreadWindows (USER32.@)
 */
BOOL WINAPI EnumThreadWindows( DWORD id, WNDENUMPROC func, LPARAM lParam )
{
    HWND *list;
    int i, iWndsLocks;

    if (!(list = WIN_BuildWinArray( GetDesktopWindow() ))) return FALSE;

    /* Now call the callback function for every window */

    iWndsLocks = WIN_SuspendWndsLock();
    for (i = 0; list[i]; i++)
    {
        if (GetWindowThreadProcessId( list[i], NULL ) != id) continue;
        if (!func( list[i], lParam )) break;
    }
    WIN_RestoreWndsLock(iWndsLocks);
    WIN_ReleaseWinArray(list);
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
        childList = WIN_BuildWinArray( *list );

        ret = func( *list, lParam );

        if (childList)
        {
            if (ret) ret = WIN_EnumChildWindows( childList, func, lParam );
            WIN_ReleaseWinArray(childList);
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

    if (!(list = WIN_BuildWinArray( parent ))) return FALSE;
    iWndsLocks = WIN_SuspendWndsLock();
    WIN_EnumChildWindows( list, func, lParam );
    WIN_RestoreWndsLock(iWndsLocks);
    WIN_ReleaseWinArray(list);
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
    WND *wndPtr = WIN_LockWndPtr(pWndDesktop->child);
    BOOL retvalue;
    
    while (wndPtr)
    {
        if (wndPtr->owner && (wndPtr->dwStyle & WS_VISIBLE))
        {
            retvalue = TRUE;
            goto end;
	}
        WIN_UpdateWndPtr(&wndPtr,wndPtr->next);
    }
    retvalue = FALSE;
end:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/*******************************************************************
 *		FlashWindow (USER.105)
 */
BOOL16 WINAPI FlashWindow16( HWND16 hWnd, BOOL16 bInvert )
{
    return FlashWindow( hWnd, bInvert );
}


/*******************************************************************
 *		FlashWindow (USER32.@)
 */
BOOL WINAPI FlashWindow( HWND hWnd, BOOL bInvert )
{
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    TRACE("%04x\n", hWnd);

    if (!wndPtr) return FALSE;

    if (wndPtr->dwStyle & WS_MINIMIZE)
    {
        if (bInvert && !(wndPtr->flags & WIN_NCACTIVATED))
        {
            HDC hDC = GetDC(hWnd);
            
            if (!SendMessageW( hWnd, WM_ERASEBKGND, (WPARAM16)hDC, 0 ))
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
        else wparam = (hWnd == GetActiveWindow());

        SendMessageW( hWnd, WM_NCACTIVATE, wparam, (LPARAM)0 );
        WIN_ReleaseWndPtr(wndPtr);
        return wparam;
    }
}


/*******************************************************************
 *		SetSysModalWindow (USER.188)
 */
HWND16 WINAPI SetSysModalWindow16( HWND16 hWnd )
{
    HWND hWndOldModal = hwndSysModal;
    hwndSysModal = hWnd;
    FIXME("EMPTY STUB !! SetSysModalWindow(%04x) !\n", hWnd);
    return hWndOldModal;
}


/*******************************************************************
 *		GetSysModalWindow (USER.189)
 */
HWND16 WINAPI GetSysModalWindow16(void)
{
    return hwndSysModal;
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
 *			DRAG_QueryUpdate
 *
 * recursively find a child that contains spDragInfo->pt point 
 * and send WM_QUERYDROPOBJECT
 */
BOOL16 DRAG_QueryUpdate( HWND hQueryWnd, SEGPTR spDragInfo, BOOL bNoSend )
{
    BOOL16 wParam, bResult = 0;
    POINT pt;
    LPDRAGINFO16 ptrDragInfo = MapSL(spDragInfo);
    RECT tempRect;

    if (!ptrDragInfo) return FALSE;

    CONV_POINT16TO32( &ptrDragInfo->pt, &pt );

    GetWindowRect(hQueryWnd,&tempRect);

    if( !PtInRect(&tempRect,pt) || !IsWindowEnabled(hQueryWnd)) return FALSE;

    if (!IsIconic( hQueryWnd ))
    {
        WND *ptrWnd, *ptrQueryWnd = WIN_FindWndPtr(hQueryWnd);

        tempRect = ptrQueryWnd->rectClient;
        if(ptrQueryWnd->parent)
            MapWindowPoints( ptrQueryWnd->parent->hwndSelf, 0,
                             (LPPOINT)&tempRect, 2 );

        if (PtInRect( &tempRect, pt))
        {
            wParam = 0;

            for (ptrWnd = WIN_LockWndPtr(ptrQueryWnd->child); ptrWnd ;WIN_UpdateWndPtr(&ptrWnd,ptrWnd->next))
            {
                if( ptrWnd->dwStyle & WS_VISIBLE )
                {
                    GetWindowRect( ptrWnd->hwndSelf, &tempRect );
                    if (PtInRect( &tempRect, pt )) break;
                }
            }

            if(ptrWnd)
            {
                TRACE_(msg)("hwnd = %04x, %d %d - %d %d\n",
                            ptrWnd->hwndSelf, ptrWnd->rectWindow.left, ptrWnd->rectWindow.top,
                            ptrWnd->rectWindow.right, ptrWnd->rectWindow.bottom );
                if( !(ptrWnd->dwStyle & WS_DISABLED) )
                    bResult = DRAG_QueryUpdate(ptrWnd->hwndSelf, spDragInfo, bNoSend);

                WIN_ReleaseWndPtr(ptrWnd);
            }

            if(bResult)
            {
                WIN_ReleaseWndPtr(ptrQueryWnd);
                return bResult;
            }
        }
        else wParam = 1;
        WIN_ReleaseWndPtr(ptrQueryWnd);
    }
    else wParam = 1;

    ScreenToClient16(hQueryWnd,&ptrDragInfo->pt);

    ptrDragInfo->hScope = hQueryWnd;

    if (bNoSend) bResult = (GetWindowLongA( hQueryWnd, GWL_EXSTYLE ) & WS_EX_ACCEPTFILES) != 0;
    else bResult = SendMessage16( hQueryWnd, WM_QUERYDROPOBJECT, (WPARAM16)wParam, spDragInfo );

    if( !bResult ) CONV_POINT32TO16( &pt, &ptrDragInfo->pt );

    return bResult;
}


/*******************************************************************
 *		DragDetect (USER.465)
 */
BOOL16 WINAPI DragDetect16( HWND16 hWnd, POINT16 pt )
{
    POINT pt32;
    CONV_POINT16TO32( &pt, &pt32 );
    return DragDetect( hWnd, pt32 );
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
 *		DragObject (USER.464)
 */
DWORD WINAPI DragObject16( HWND16 hwndScope, HWND16 hWnd, UINT16 wObj,
                           HANDLE16 hOfStruct, WORD szList, HCURSOR16 hCursor )
{
    MSG	msg;
    LPDRAGINFO16 lpDragInfo;
    SEGPTR	spDragInfo;
    HCURSOR16 	hDragCursor=0, hOldCursor=0, hBummer=0;
    HGLOBAL16	hDragInfo  = GlobalAlloc16( GMEM_SHARE | GMEM_ZEROINIT, 2*sizeof(DRAGINFO16));
    HCURSOR16	hCurrentCursor = 0;
    HWND16	hCurrentWnd = 0;

    lpDragInfo = (LPDRAGINFO16) GlobalLock16(hDragInfo);
    spDragInfo = K32WOWGlobalLock16(hDragInfo);

    if( !lpDragInfo || !spDragInfo ) return 0L;

    if (!(hBummer = LoadCursorA(0, MAKEINTRESOURCEA(OCR_NO))))
    {
        GlobalFree16(hDragInfo);
        return 0L;
    }

    if(hCursor)
    {
	if( !(hDragCursor = CURSORICON_IconToCursor(hCursor, FALSE)) )
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
    ShowCursor( TRUE );

    do
    {
        GetMessageW( &msg, 0, WM_MOUSEFIRST, WM_MOUSELAST );

       *(lpDragInfo+1) = *lpDragInfo;

	lpDragInfo->pt.x = msg.pt.x;
	lpDragInfo->pt.y = msg.pt.y;

	/* update DRAGINFO struct */
	TRACE_(msg)("lpDI->hScope = %04x\n",lpDragInfo->hScope);

	if( DRAG_QueryUpdate(hwndScope, spDragInfo, FALSE) > 0 )
	    hCurrentCursor = hCursor;
	else
        {
            hCurrentCursor = hBummer;
            lpDragInfo->hScope = 0;
	}
	if( hCurrentCursor )
	    SetCursor(hCurrentCursor);

	/* send WM_DRAGLOOP */
	SendMessage16( hWnd, WM_DRAGLOOP, (WPARAM16)(hCurrentCursor != hBummer), 
	                                  (LPARAM) spDragInfo );
	/* send WM_DRAGSELECT or WM_DRAGMOVE */
	if( hCurrentWnd != lpDragInfo->hScope )
	{
	    if( hCurrentWnd )
	        SendMessage16( hCurrentWnd, WM_DRAGSELECT, 0, 
		       (LPARAM)MAKELONG(LOWORD(spDragInfo)+sizeof(DRAGINFO16),
				        HIWORD(spDragInfo)) );
	    hCurrentWnd = lpDragInfo->hScope;
	    if( hCurrentWnd )
                SendMessage16( hCurrentWnd, WM_DRAGSELECT, 1, (LPARAM)spDragInfo); 
	}
	else
	    if( hCurrentWnd )
	        SendMessage16( hCurrentWnd, WM_DRAGMOVE, 0, (LPARAM)spDragInfo);

    } while( msg.message != WM_LBUTTONUP && msg.message != WM_NCLBUTTONUP );

    ReleaseCapture();
    ShowCursor( FALSE );

    if( hCursor )
    {
	SetCursor( hOldCursor );
	if (hDragCursor) DestroyCursor( hDragCursor );
    }

    if( hCurrentCursor != hBummer ) 
	msg.lParam = SendMessage16( lpDragInfo->hScope, WM_DROPOBJECT, 
				   (WPARAM16)hWnd, (LPARAM)spDragInfo );
    else
        msg.lParam = 0;
    GlobalFree16(hDragInfo);

    return (DWORD)(msg.lParam);
}


/******************************************************************************
 *		GetWindowModuleFileNameA (USER32.@)
 */
UINT WINAPI GetWindowModuleFileNameA( HWND hwnd, LPSTR lpszFileName, UINT cchFileNameMax)
{
    FIXME("GetWindowModuleFileNameA(hwnd 0x%x, lpszFileName %p, cchFileNameMax %u) stub!\n",
          hwnd, lpszFileName, cchFileNameMax);
    return 0;
}

/******************************************************************************
 *		GetWindowModuleFileNameW (USER32.@)
 */
UINT WINAPI GetWindowModuleFileNameW( HWND hwnd, LPSTR lpszFileName, UINT cchFileNameMax)
{
    FIXME("GetWindowModuleFileNameW(hwnd 0x%x, lpszFileName %p, cchFileNameMax %u) stub!\n",
          hwnd, lpszFileName, cchFileNameMax);
    return 0;
}
