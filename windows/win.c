/*
 * Window related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include "wine/winbase16.h"
#include "options.h"
#include "class.h"
#include "win.h"
#include "heap.h"
#include "user.h"
#include "dce.h"
#include "sysmetrics.h"
#include "cursoricon.h"
#include "hook.h"
#include "menu.h"
#include "message.h"
#include "nonclient.h"
#include "queue.h"
#include "winpos.h"
#include "shm_main_blk.h"
#include "dde_proc.h"
#include "clipboard.h"
#include "winproc.h"
#include "task.h"
#include "thread.h"
#include "process.h"
#include "debug.h"
#include "winerror.h"
#include "mdi.h"
#include "local.h"
#include "desktop.h"

#ifndef X_DISPLAY_MISSING
extern DESKTOP_DRIVER X11DRV_DESKTOP_Driver;
extern WND_DRIVER X11DRV_WND_Driver;
#else /* X_DISPLAY_MISSING */
extern DESKTOP_DRIVER TTYDRV_DESKTOP_Driver;
extern WND_DRIVER TTYDRV_WND_Driver;
#endif /* X_DISPLAY_MISSING */

/* Desktop window */
static WND *pWndDesktop = NULL;

static HWND hwndSysModal = 0;

static WORD wDragWidth = 4;
static WORD wDragHeight= 3;

/* thread safeness */
static CRITICAL_SECTION WIN_CritSection;
static int ilockCounter = 0;

/***********************************************************************
 *           WIN_LockWnds
 *
 *   Locks access to all WND structures for thread safeness
 */
void WIN_LockWnds()
{
/*
    EnterCriticalSection(&WIN_CritSection);
*/
}

/***********************************************************************
 *           WIN_UnlockWnds
 *
 *  Unlocks access to all WND structures
 */
void WIN_UnlockWnds()
    {
/*
        LeaveCriticalSection(&WIN_CritSection);
*/
}
/***********************************************************************
 *           WIN_SuspendWndsLock
 *
 *   Suspend the lock on WND structures.
 *   Returns the number of locks suspended
 */
int WIN_SuspendWndsLock()
{
/*
    int isuspendedLocks = WIN_CritSection.RecursionCount;
    WIN_CritSection.RecursionCount = 0;
    LeaveCriticalSection(&WIN_CritSection);
    return isuspendedLocks;
*/
}

/***********************************************************************
 *           WIN_RestoreWndsLock
 *
 *  Restore the suspended locks on WND structures
 */
void WIN_RestoreWndslock(int ipreviousLocks)
{
/*
    EnterCriticalSection(&WIN_CritSection);
    WIN_CritSection.RecursionCount = ipreviousLocks;
*/
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
    /* Lock all WND structures for thread safeness
    WIN_LockWnds(ptr);
    and increment destruction monitoring
     ptr->irefCount++;
     */
    if (ptr->dwMagic != WND_MAGIC) goto error;
    if (ptr->hwndSelf != hwnd)
    {
        ERR( win, "Can't happen: hwnd %04x self pointer is %04x\n",
             hwnd, ptr->hwndSelf );
        goto error;
    }
    return ptr;
 error:
    /* Unlock all WND structures for thread safeness
    WIN_UnlockWnds(ptr);
     and decrement destruction monitoring value
     ptr->irefCount--;
     */
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
    /*
    WIN_LockWnds();
    initWndPtr->irefCount++;
    */
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
    /*Decrement destruction monitoring value
     wndPtr->irefCount--;
     Check if it's time to release the memory
     if(wndPtr->irefCount == 0)
     {
         Add memory releasing code here
     }
     unlock all WND structures for thread safeness
     WIN_UnlockWnds();
     */    
}

/***********************************************************************
 *           WIN_ReleaseWndPtr
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
        WARN( win, "%04x is not a window handle\n", hwnd );
        return;
    }

    if (!GetClassNameA( hwnd, className, sizeof(className ) ))
        strcpy( className, "#NULL#" );

    TRACE( win, "Window %04x (%p):\n", hwnd, ptr );
    DUMP(    "next=%p  child=%p  parent=%p  owner=%p  class=%p '%s'\n"
             "inst=%04x  taskQ=%04x  updRgn=%04x  active=%04x dce=%p  idmenu=%08x\n"
             "style=%08lx  exstyle=%08lx  wndproc=%08x  text='%s'\n"
             "client=%d,%d-%d,%d  window=%d,%d-%d,%d"
             "sysmenu=%04x  flags=%04x  props=%p  vscroll=%p  hscroll=%p\n",
             ptr->next, ptr->child, ptr->parent, ptr->owner,
             ptr->class, className, ptr->hInstance, ptr->hmemTaskQ,
             ptr->hrgnUpdate, ptr->hwndLastActive, ptr->dce, ptr->wIDmenu,
             ptr->dwStyle, ptr->dwExStyle, (UINT)ptr->winproc,
             ptr->text ? ptr->text : "",
             ptr->rectClient.left, ptr->rectClient.top, ptr->rectClient.right,
             ptr->rectClient.bottom, ptr->rectWindow.left, ptr->rectWindow.top,
             ptr->rectWindow.right, ptr->rectWindow.bottom, ptr->hSysMenu,
             ptr->flags, ptr->pProp, ptr->pVScroll, ptr->pHScroll );

    if (ptr->class->cbWndExtra)
    {
        DUMP( "extra bytes:" );
        for (i = 0; i < ptr->class->cbWndExtra; i++)
            DUMP( " %02x", *((BYTE*)ptr->wExtra+i) );
        DUMP( "\n" );
    }
    DUMP( "\n" );
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
        WARN( win, "Invalid window handle %04x\n", hwnd );
        return;
    }

    if (!indent)  /* first time around */
       DUMP( "%-16.16s %-8.8s %-6.6s %-17.17s %-8.8s %s\n",
                 "hwnd", " wndPtr", "queue", "Class Name", " Style", " WndProc"
                 " Text");

    while (ptr)
    {
        DUMP( "%*s%04x%*s", indent, "", ptr->hwndSelf, 13-indent,"");

        GlobalGetAtomName16(ptr->class->atomName,className,sizeof(className));
        
        DUMP( "%08lx %-6.4x %-17.17s %08x %08x %.14s\n",
                 (DWORD)ptr, ptr->hmemTaskQ, className,
                 (UINT)ptr->dwStyle, (UINT)ptr->winproc,
                 ptr->text?ptr->text:"<null>");
        
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

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    else if(!wndPtr->parent)
    {
        WIN_ReleaseWndPtr(wndPtr);
        return FALSE;
    }
    ppWnd = &wndPtr->parent->child;
    while (*ppWnd != wndPtr) ppWnd = &(*ppWnd)->next;
    *ppWnd = wndPtr->next;
    WIN_ReleaseWndPtr(wndPtr);
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
HWND WIN_FindWinToRepaint( HWND hwnd, HQUEUE16 hQueue )
{
    HWND hwndRet;
    WND *pWnd;

    /* Note: the desktop window never gets WM_PAINT messages 
     * The real reason why is because Windows DesktopWndProc
     * does ValidateRgn inside WM_ERASEBKGND handler.
     */

    pWnd = hwnd ? WIN_FindWndPtr(hwnd) : WIN_LockWndPtr(pWndDesktop->child);

    for ( ; pWnd ; WIN_UpdateWndPtr(&pWnd,pWnd->next))
    {
        if (!(pWnd->dwStyle & WS_VISIBLE))
        {
            TRACE(win, "skipping window %04x\n",
                         pWnd->hwndSelf );
        }
        else if ((pWnd->hmemTaskQ == hQueue) &&
            (pWnd->hrgnUpdate || (pWnd->flags & WIN_INTERNAL_PAINT))) break;
        
        else if (pWnd->child )
            if ((hwndRet = WIN_FindWinToRepaint( pWnd->child->hwndSelf, hQueue )) )
                goto end;
    }
    
    if(!pWnd)
    {
        hwndRet = 0;
        goto end;
    }
    
    hwndRet = pWnd->hwndSelf;

    /* look among siblings if we got a transparent window */
    while (pWnd && ((pWnd->dwExStyle & WS_EX_TRANSPARENT) ||
                    !(pWnd->hrgnUpdate || (pWnd->flags & WIN_INTERNAL_PAINT))))
    {
        WIN_UpdateWndPtr(&pWnd,pWnd->next);
    }
    if (pWnd) hwndRet = pWnd->hwndSelf;
    TRACE(win,"found %04x\n",hwndRet);
  end:
    WIN_ReleaseWndPtr(pWnd);
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

    TRACE(win, "%04x\n", wndPtr->hwndSelf );

#ifdef CONFIG_IPC
    if (main_block)
	DDE_DestroyWindow(wndPtr->hwndSelf);
#endif  /* CONFIG_IPC */
	
    /* free child windows */
    WIN_LockWndPtr(wndPtr->child);
    while ((pWnd = wndPtr->child))
    {
        wndPtr->child = WIN_DestroyWindow( pWnd );
        WIN_ReleaseWndPtr(pWnd);
    }

    SendMessageA( wndPtr->hwndSelf, WM_NCDESTROY, 0, 0);

    /* FIXME: do we need to fake QS_MOUSEMOVE wakebit? */

    WINPOS_CheckInternalPos( wndPtr );
    if( hwnd == GetCapture()) ReleaseCapture();

    /* free resources associated with the window */

    TIMER_RemoveWindowTimers( wndPtr->hwndSelf );
    PROPERTY_RemoveWindowProps( wndPtr );

    wndPtr->dwMagic = 0;  /* Mark it as invalid */

    if ((wndPtr->hrgnUpdate) || (wndPtr->flags & WIN_INTERNAL_PAINT))
    {
        if (wndPtr->hrgnUpdate > 1) DeleteObject( wndPtr->hrgnUpdate );
        QUEUE_DecPaintCount( wndPtr->hmemTaskQ );
    }

    /* toss stale messages from the queue */

    if( wndPtr->hmemTaskQ )
    {
	BOOL	      bPostQuit = FALSE;
	WPARAM      wQuitParam = 0;
        MESSAGEQUEUE* msgQ = (MESSAGEQUEUE*) QUEUE_Lock(wndPtr->hmemTaskQ);
        QMSG *qmsg;

	while( (qmsg = QUEUE_FindMsg(msgQ, hwnd, 0, 0)) != 0 )
	{
	    if( qmsg->msg.message == WM_QUIT )
	    {
		bPostQuit = TRUE;
		wQuitParam = qmsg->msg.wParam;
	    }
	    QUEUE_RemoveMsg(msgQ, qmsg);
	}

        QUEUE_Unlock(msgQ);
        
	/* repost WM_QUIT to make sure this app exits its message loop */
	if( bPostQuit ) PostQuitMessage(wQuitParam);
	wndPtr->hmemTaskQ = 0;
    }

    if (!(wndPtr->dwStyle & WS_CHILD))
       if (wndPtr->wIDmenu) DestroyMenu( (HMENU)wndPtr->wIDmenu );
    if (wndPtr->hSysMenu) DestroyMenu( wndPtr->hSysMenu );
    wndPtr->pDriver->pDestroyWindow( wndPtr );
    DCE_FreeWindowDCE( wndPtr );    /* Always do this to catch orphaned DCs */ 
    WINPROC_FreeProc( wndPtr->winproc, WIN_PROC_WINDOW );
    wndPtr->hwndSelf = 0;
    wndPtr->class->cWindows--;
    wndPtr->class = NULL;

    WIN_UpdateWndPtr(&pWnd,wndPtr->next);

    wndPtr->pDriver->pFinalize(wndPtr);
    USER_HEAP_FREE( hwnd );
    return pWnd;
}

/***********************************************************************
 *           WIN_ResetQueueWindows
 *
 * Reset the queue of all the children of a given window.
 * Return TRUE if something was done.
 */
BOOL WIN_ResetQueueWindows( WND* wnd, HQUEUE16 hQueue, HQUEUE16 hNew )
{
    BOOL ret = FALSE;

    if (hNew)  /* Set a new queue */
    {
        for (wnd = WIN_LockWndPtr(wnd->child); (wnd);WIN_UpdateWndPtr(&wnd,wnd->next))
        {
            if (wnd->hmemTaskQ == hQueue)
            {
                wnd->hmemTaskQ = hNew;
                ret = TRUE;
            }
            if (wnd->child)
            {
                ret |= WIN_ResetQueueWindows( wnd, hQueue, hNew );
        }
    }
    }
    else  /* Queue is being destroyed */
    {
        while (wnd->child)
        {
            WND *tmp = WIN_LockWndPtr(wnd->child);
            WND *tmp2;
            ret = FALSE;
            while (tmp)
            {
                if (tmp->hmemTaskQ == hQueue)
                {
                    DestroyWindow( tmp->hwndSelf );
                    ret = TRUE;
                    break;
                }
                tmp2 = WIN_LockWndPtr(tmp->child);
                if (tmp2 && WIN_ResetQueueWindows(tmp2,hQueue,0))
                    ret = TRUE;
                else
                {
                    WIN_UpdateWndPtr(&tmp,tmp->next);
                }
                WIN_ReleaseWndPtr(tmp2);
            }
            WIN_ReleaseWndPtr(tmp);
            if (!ret) break;
        }
    }
    return ret;
}

/***********************************************************************
 *           WIN_CreateDesktopWindow
 *
 * Create the desktop window.
 */
BOOL WIN_CreateDesktopWindow(void)
{
    CLASS *class;
    HWND hwndDesktop;
    DESKTOP *pDesktop;

    TRACE(win,"Creating desktop window\n");

    if (!ICONTITLE_Init() ||
	!WINPOS_CreateInternalPosAtom() ||
	!(class = CLASS_FindClassByAtom( DESKTOP_CLASS_ATOM, 0 )))
	return FALSE;

    hwndDesktop = USER_HEAP_ALLOC( sizeof(WND)+class->cbWndExtra );
    if (!hwndDesktop) return FALSE;
    pWndDesktop = (WND *) USER_HEAP_LIN_ADDR( hwndDesktop );

    pDesktop = (DESKTOP *) pWndDesktop->wExtra;
#ifndef X_DISPLAY_MISSING
    pDesktop->pDriver = &X11DRV_DESKTOP_Driver;
    pWndDesktop->pDriver = &X11DRV_WND_Driver;
#else /* X_DISPLAY_MISSING */
    pDesktop->pDriver = &TTYDRV_DESKTOP_Driver;
    pWndDesktop->pDriver = &TTYDRV_WND_Driver;
#endif /* X_DISPLAY_MISSING */

    pDesktop->pDriver->pInitialize(pDesktop);
    pWndDesktop->pDriver->pInitialize(pWndDesktop);

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
    pWndDesktop->text              = NULL;
    pWndDesktop->hmemTaskQ         = GetFastQueue16();
    pWndDesktop->hrgnUpdate        = 0;
    pWndDesktop->hwndLastActive    = hwndDesktop;
    pWndDesktop->dwStyle           = WS_VISIBLE | WS_CLIPCHILDREN |
                                     WS_CLIPSIBLINGS;
    pWndDesktop->dwExStyle         = 0;
    pWndDesktop->dce               = NULL;
    pWndDesktop->pVScroll          = NULL;
    pWndDesktop->pHScroll          = NULL;
    pWndDesktop->pProp             = NULL;
    pWndDesktop->wIDmenu           = 0;
    pWndDesktop->helpContext       = 0;
    pWndDesktop->flags             = 0;
    pWndDesktop->hSysMenu          = 0;
    pWndDesktop->userdata          = 0;
    pWndDesktop->winproc = (WNDPROC16)class->winproc;
    pWndDesktop->irefCount         = 0;

    /* FIXME: How do we know if it should be Unicode or not */
    if(!pWndDesktop->pDriver->pCreateDesktopWindow(pWndDesktop, class, FALSE))
      return FALSE;
    
    SendMessageA( hwndDesktop, WM_NCCREATE, 0, 0 );

    /* Initialisation of the critical section for thread safeness
     InitializeCriticalSection(&WIN_CritSection);
     MakeCriticalSectionGlobal(&WIN_CritSection);
     */
    pWndDesktop->flags |= WIN_NEEDS_ERASEBKGND;
    return TRUE;
}


/***********************************************************************
 *           WIN_CreateWindowEx
 *
 * Implementation of CreateWindowEx().
 */
static HWND WIN_CreateWindowEx( CREATESTRUCTA *cs, ATOM classAtom,
                                  BOOL win32, BOOL unicode )
{
    CLASS *classPtr;
    WND *wndPtr;
    HWND retvalue;
    HWND16 hwnd, hwndLinkAfter;
    POINT maxSize, maxPos, minTrack, maxTrack;
    LRESULT (CALLBACK *localSend32)(HWND, UINT, WPARAM, LPARAM);

    TRACE(win, "%s %s %08lx %08lx %d,%d %dx%d %04x %04x %08x %p\n",
          unicode ? debugres_w((LPWSTR)cs->lpszName) : debugres_a(cs->lpszName), 
          unicode ? debugres_w((LPWSTR)cs->lpszClass) : debugres_a(cs->lpszClass),
          cs->dwExStyle, cs->style, cs->x, cs->y, cs->cx, cs->cy,
          cs->hwndParent, cs->hMenu, cs->hInstance, cs->lpCreateParams );

    /* Find the parent window */

    if (cs->hwndParent)
    {
	/* Make sure parent is valid */
        if (!IsWindow( cs->hwndParent ))
        {
            WARN( win, "Bad parent %04x\n", cs->hwndParent );
	    return 0;
	}
    } else if ((cs->style & WS_CHILD) && !(cs->style & WS_POPUP)) {
        WARN( win, "No parent for child window\n" );
        return 0;  /* WS_CHILD needs a parent, but WS_POPUP doesn't */
    }

    /* Find the window class */
    if (!(classPtr = CLASS_FindClassByAtom( classAtom, win32?cs->hInstance:GetExePtr(cs->hInstance) )))
    {
        char buffer[256];
        GlobalGetAtomNameA( classAtom, buffer, sizeof(buffer) );
        WARN( win, "Bad class '%s'\n", buffer );
        return 0;
    }

    /* Fix the coordinates */

    if (cs->x == CW_USEDEFAULT) 
    {
        PDB *pdb = PROCESS_Current();
        if (   !(cs->style & (WS_CHILD | WS_POPUP))
            &&  (pdb->env_db->startup_info->dwFlags & STARTF_USEPOSITION) )
        {
            cs->x = pdb->env_db->startup_info->dwX;
            cs->y = pdb->env_db->startup_info->dwY;
        }
        else
        {
            cs->x = 0;
            cs->y = 0;
        }
    }
    if (cs->cx == CW_USEDEFAULT)
    {
        PDB *pdb = PROCESS_Current();
        if (   !(cs->style & (WS_CHILD | WS_POPUP))
            &&  (pdb->env_db->startup_info->dwFlags & STARTF_USESIZE) )
        {
            cs->cx = pdb->env_db->startup_info->dwXSize;
            cs->cy = pdb->env_db->startup_info->dwYSize;
        }
        else
        {
            cs->cx = 600; /* FIXME */
            cs->cy = 400;
        }
    }

    /* Create the window structure */

    if (!(hwnd = USER_HEAP_ALLOC( sizeof(*wndPtr) + classPtr->cbWndExtra
                                  - sizeof(wndPtr->wExtra) )))
    {
	TRACE(win, "out of memory\n" );
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
            wndPtr->owner = WIN_GetTopParentPtr(WIN_FindWndPtr(cs->hwndParent));
            WIN_ReleaseWndPtr(wndPtr->owner);
    }
    }

    wndPtr->pDriver = wndPtr->parent->pDriver;
    wndPtr->pDriver->pInitialize(wndPtr);

    wndPtr->class          = classPtr;
    wndPtr->winproc        = classPtr->winproc;
    wndPtr->dwMagic        = WND_MAGIC;
    wndPtr->hwndSelf       = hwnd;
    wndPtr->hInstance      = cs->hInstance;
    wndPtr->text           = NULL;
    wndPtr->hmemTaskQ      = GetFastQueue16();
    wndPtr->hrgnUpdate     = 0;
    wndPtr->hwndLastActive = hwnd;
    wndPtr->dwStyle        = cs->style & ~WS_VISIBLE;
    wndPtr->dwExStyle      = cs->dwExStyle;
    wndPtr->wIDmenu        = 0;
    wndPtr->helpContext    = 0;
    wndPtr->flags          = win32 ? WIN_ISWIN32 : 0;
    wndPtr->pVScroll       = NULL;
    wndPtr->pHScroll       = NULL;
    wndPtr->pProp          = NULL;
    wndPtr->userdata       = 0;
    wndPtr->hSysMenu       = (wndPtr->dwStyle & WS_SYSMENU)
			     ? MENU_GetSysMenu( hwnd, 0 ) : 0;

    if (classPtr->cbWndExtra) memset( wndPtr->wExtra, 0, classPtr->cbWndExtra);

    /* Call the WH_CBT hook */

    hwndLinkAfter = ((cs->style & (WS_CHILD|WS_MAXIMIZE)) == WS_CHILD)
 ? HWND_BOTTOM : HWND_TOP;

    if (HOOK_IsHooked( WH_CBT ))
    {
	CBT_CREATEWNDA cbtc;
        LRESULT ret;

	cbtc.lpcs = cs;
	cbtc.hwndInsertAfter = hwndLinkAfter;
        ret = unicode ? HOOK_CallHooksW(WH_CBT, HCBT_CREATEWND, hwnd, (LPARAM)&cbtc)
                      : HOOK_CallHooksA(WH_CBT, HCBT_CREATEWND, hwnd, (LPARAM)&cbtc);
        if (ret)
	{
	    TRACE(win, "CBT-hook returned 0\n");
	    wndPtr->pDriver->pFinalize(wndPtr);
	    USER_HEAP_FREE( hwnd );
            retvalue =  0;
            goto end;
	}
    }

    /* Increment class window counter */

    classPtr->cWindows++;

    /* Correct the window style */

    if (!(cs->style & (WS_POPUP | WS_CHILD)))  /* Overlapped window */
    {
        wndPtr->dwStyle |= WS_CAPTION | WS_CLIPSIBLINGS;
        wndPtr->flags |= WIN_NEED_SIZE;
    }
    if (cs->dwExStyle & WS_EX_DLGMODALFRAME) wndPtr->dwStyle &= ~WS_THICKFRAME;

    /* Get class or window DC if needed */

    if (classPtr->style & CS_OWNDC) wndPtr->dce = DCE_AllocDCE(hwnd,DCE_WINDOW_DC);
    else if (classPtr->style & CS_CLASSDC) wndPtr->dce = classPtr->dce;
    else wndPtr->dce = NULL;

    /* Send the WM_GETMINMAXINFO message and fix the size if needed */

    if ((cs->style & WS_THICKFRAME) || !(cs->style & (WS_POPUP | WS_CHILD)))
    {
        WINPOS_GetMinMaxInfo( wndPtr, &maxSize, &maxPos, &minTrack, &maxTrack);
        if (maxSize.x < cs->cx) cs->cx = maxSize.x;
        if (maxSize.y < cs->cy) cs->cy = maxSize.y;
        if (cs->cx < minTrack.x ) cs->cx = minTrack.x;
        if (cs->cy < minTrack.y ) cs->cy = minTrack.y;
    }

    if(cs->style & WS_CHILD)
    {
        if(cs->cx < 0) cs->cx = 0;
        if(cs->cy < 0) cs->cy = 0;
    }
    else
    {
        if (cs->cx <= 0) cs->cx = 1;
        if (cs->cy <= 0) cs->cy = 1;
    }

    wndPtr->rectWindow.left   = cs->x;
    wndPtr->rectWindow.top    = cs->y;
    wndPtr->rectWindow.right  = cs->x + cs->cx;
    wndPtr->rectWindow.bottom = cs->y + cs->cy;
    wndPtr->rectClient        = wndPtr->rectWindow;

    if(!wndPtr->pDriver->pCreateWindow(wndPtr, classPtr, cs, unicode))
    {
        retvalue = FALSE;
        goto end;
    }

    /* Set the window menu */

    if ((wndPtr->dwStyle & (WS_CAPTION | WS_CHILD)) == WS_CAPTION )
    {
        if (cs->hMenu) SetMenu(hwnd, cs->hMenu);
        else
        {
#if 0  /* FIXME: should check if classPtr->menuNameW can be used as is */
            if (classPtr->menuNameA)
                cs->hMenu = HIWORD(classPtr->menuNameA) ?
                       LoadMenu(cs->hInstance,SEGPTR_GET(classPtr->menuNameA)):
                       LoadMenu(cs->hInstance,(SEGPTR)classPtr->menuNameA);
#else
	    SEGPTR menuName = (SEGPTR)GetClassLong16( hwnd, GCL_MENUNAME );
            if (menuName)
            {
                if (HIWORD(cs->hInstance))
                    cs->hMenu = LoadMenuA(cs->hInstance,PTR_SEG_TO_LIN(menuName));
                else
                    cs->hMenu = LoadMenu16(cs->hInstance,menuName);

                if (cs->hMenu) SetMenu( hwnd, cs->hMenu );
            }
#endif
        }
    }
    else wndPtr->wIDmenu = (UINT)cs->hMenu;

    /* Send the WM_CREATE message 
     * Perhaps we shouldn't allow width/height changes as well. 
     * See p327 in "Internals". 
     */

    maxPos.x = wndPtr->rectWindow.left; maxPos.y = wndPtr->rectWindow.top;

    localSend32 = unicode ? SendMessageW : SendMessageA;
    if( (*localSend32)( hwnd, WM_NCCREATE, 0, (LPARAM)cs) )
    {
        /* Insert the window in the linked list */

        WIN_LinkWindow( hwnd, hwndLinkAfter );

        WINPOS_SendNCCalcSize( hwnd, FALSE, &wndPtr->rectWindow,
                               NULL, NULL, 0, &wndPtr->rectClient );
        OffsetRect(&wndPtr->rectWindow, maxPos.x - wndPtr->rectWindow.left,
                                          maxPos.y - wndPtr->rectWindow.top);
        if( ((*localSend32)( hwnd, WM_CREATE, 0, (LPARAM)cs )) != -1 )
        {
            /* Send the size messages */

            if (!(wndPtr->flags & WIN_NEED_SIZE))
            {
                /* send it anyway */
	        if (((wndPtr->rectClient.right-wndPtr->rectClient.left) <0)
		    ||((wndPtr->rectClient.bottom-wndPtr->rectClient.top)<0))
		  WARN(win,"sending bogus WM_SIZE message 0x%08lx\n",
			MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
				 wndPtr->rectClient.bottom-wndPtr->rectClient.top));
                SendMessageA( hwnd, WM_SIZE, SIZE_RESTORED,
                                MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
                                         wndPtr->rectClient.bottom-wndPtr->rectClient.top));
                SendMessageA( hwnd, WM_MOVE, 0,
                                MAKELONG( wndPtr->rectClient.left,
                                          wndPtr->rectClient.top ) );
            }

            /* Show the window, maximizing or minimizing if needed */

            if (wndPtr->dwStyle & (WS_MINIMIZE | WS_MAXIMIZE))
            {
		RECT16 newPos;
		UINT16 swFlag = (wndPtr->dwStyle & WS_MINIMIZE) ? SW_MINIMIZE : SW_MAXIMIZE;
                wndPtr->dwStyle &= ~(WS_MAXIMIZE | WS_MINIMIZE);
		WINPOS_MinMaximize( wndPtr, swFlag, &newPos );
                swFlag = ((wndPtr->dwStyle & WS_CHILD) || GetActiveWindow())
                    ? SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED
                    : SWP_NOZORDER | SWP_FRAMECHANGED;
                SetWindowPos( hwnd, 0, newPos.left, newPos.top,
                                newPos.right, newPos.bottom, swFlag );
            }

	    if( wndPtr->dwStyle & WS_CHILD && !(wndPtr->dwExStyle & WS_EX_NOPARENTNOTIFY) )
	    {
		/* Notify the parent window only */

		SendMessageA( wndPtr->parent->hwndSelf, WM_PARENTNOTIFY,
				MAKEWPARAM(WM_CREATE, wndPtr->wIDmenu), (LPARAM)hwnd );
                if( !IsWindow(hwnd) )
                {
                    retvalue = 0;
                    goto end;
	    }
	    }

            if (cs->style & WS_VISIBLE) ShowWindow( hwnd, SW_SHOW );

            /* Call WH_SHELL hook */

            if (!(wndPtr->dwStyle & WS_CHILD) && !wndPtr->owner)
                HOOK_CallHooks16( WH_SHELL, HSHELL_WINDOWCREATED, hwnd, 0 );

            TRACE(win, "created window %04x\n", hwnd);
            retvalue = hwnd;
            goto end;
        }
        WIN_UnlinkWindow( hwnd );
    }

    /* Abort window creation */

    WARN(win, "aborted by WM_xxCREATE!\n");
    WIN_DestroyWindow( wndPtr );
    retvalue = 0;
end:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/***********************************************************************
 *           CreateWindow16   (USER.41)
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
 *           CreateWindowEx16   (USER.452)
 */
HWND16 WINAPI CreateWindowEx16( DWORD exStyle, LPCSTR className,
                                LPCSTR windowName, DWORD style, INT16 x,
                                INT16 y, INT16 width, INT16 height,
                                HWND16 parent, HMENU16 menu,
                                HINSTANCE16 instance, LPVOID data ) 
{
    ATOM classAtom;
    CREATESTRUCTA cs;

    /* Find the class atom */

    if (!(classAtom = GlobalFindAtomA( className )))
    {
        fprintf( stderr, "CreateWindowEx16: bad class name " );
        if (!HIWORD(className)) fprintf( stderr, "%04x\n", LOWORD(className) );
        else fprintf( stderr, "'%s'\n", className );
        return 0;
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
    return WIN_CreateWindowEx( &cs, classAtom, FALSE, FALSE );
}


/***********************************************************************
 *           CreateWindowEx32A   (USER32.83)
 */
HWND WINAPI CreateWindowExA( DWORD exStyle, LPCSTR className,
                                 LPCSTR windowName, DWORD style, INT x,
                                 INT y, INT width, INT height,
                                 HWND parent, HMENU menu,
                                 HINSTANCE instance, LPVOID data )
{
    ATOM classAtom;
    CREATESTRUCTA cs;

    if(exStyle & WS_EX_MDICHILD)
        return MDI_CreateMDIWindowA(className, windowName, style, x, y, width, height, parent, instance, (LPARAM)data);
    /* Find the class atom */

    if (!(classAtom = GlobalFindAtomA( className )))
    {
        fprintf( stderr, "CreateWindowEx32A: bad class name " );
        if (!HIWORD(className)) fprintf( stderr, "%04x\n", LOWORD(className) );
        else fprintf( stderr, "'%s'\n", className );
        return 0;
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
    return WIN_CreateWindowEx( &cs, classAtom, TRUE, FALSE );
}


/***********************************************************************
 *           CreateWindowEx32W   (USER32.84)
 */
HWND WINAPI CreateWindowExW( DWORD exStyle, LPCWSTR className,
                                 LPCWSTR windowName, DWORD style, INT x,
                                 INT y, INT width, INT height,
                                 HWND parent, HMENU menu,
                                 HINSTANCE instance, LPVOID data )
{
    ATOM classAtom;
    CREATESTRUCTW cs;

    if(exStyle & WS_EX_MDICHILD)
        return MDI_CreateMDIWindowW(className, windowName, style, x, y, width, height, parent, instance, (LPARAM)data);

    /* Find the class atom */

    if (!(classAtom = GlobalFindAtomW( className )))
    {
    	if (HIWORD(className))
        {
            LPSTR cn = HEAP_strdupWtoA( GetProcessHeap(), 0, className );
            WARN( win, "Bad class name '%s'\n",cn);
            HeapFree( GetProcessHeap(), 0, cn );
	}
        else
            WARN( win, "Bad class name %p\n", className );
        return 0;
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
    /* Note: we rely on the fact that CREATESTRUCT32A and */
    /* CREATESTRUCT32W have the same layout. */
    return WIN_CreateWindowEx( (CREATESTRUCTA *)&cs, classAtom, TRUE, TRUE );
}


/***********************************************************************
 *           WIN_CheckFocus
 */
static void WIN_CheckFocus( WND* pWnd )
{
    if( GetFocus16() == pWnd->hwndSelf )
	SetFocus16( (pWnd->dwStyle & WS_CHILD) ? pWnd->parent->hwndSelf : 0 ); 
}

/***********************************************************************
 *           WIN_SendDestroyMsg
 */
static void WIN_SendDestroyMsg( WND* pWnd )
{
    WIN_CheckFocus(pWnd);

    if( CARET_GetHwnd() == pWnd->hwndSelf ) DestroyCaret();
    CLIPBOARD_GetDriver()->pResetOwner( pWnd, TRUE ); 

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
      WIN_CheckFocus(pWnd);	
    }
    else
      WARN(win, "\tdestroyed itself while in WM_DESTROY!\n");
}


/***********************************************************************
 *           DestroyWindow16   (USER.53)
 */
BOOL16 WINAPI DestroyWindow16( HWND16 hwnd )
{
    return DestroyWindow(hwnd);
}


/***********************************************************************
 *           DestroyWindow32   (USER32.135)
 */
BOOL WINAPI DestroyWindow( HWND hwnd )
{
    WND * wndPtr;
    BOOL retvalue;

    TRACE(win, "(%04x)\n", hwnd);
    
      /* Initialization */

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (wndPtr == pWndDesktop)
    {
        WIN_ReleaseWndPtr(wndPtr);
        return FALSE; /* Can't destroy desktop */
    }

      /* Call hooks */

    if( HOOK_CallHooks16( WH_CBT, HCBT_DESTROYWND, hwnd, 0L) )
    {
        retvalue = FALSE;
        goto end;
    }

    if (!(wndPtr->dwStyle & WS_CHILD) && !wndPtr->owner)
    {
        HOOK_CallHooks16( WH_SHELL, HSHELL_WINDOWDESTROYED, hwnd, 0L );
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

    CLIPBOARD_GetDriver()->pResetOwner( wndPtr, FALSE ); /* before the window is unmapped */

      /* Hide the window */

    if (wndPtr->dwStyle & WS_VISIBLE)
    {
        SetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW |
		        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|
		        ((QUEUE_IsExitingQueue(wndPtr->hmemTaskQ))?SWP_DEFERERASE:0) );
        if (!IsWindow(hwnd))
        {
            retvalue = TRUE;
            goto end;
        }
    }

      /* Recursively destroy owned windows */

    if( !(wndPtr->dwStyle & WS_CHILD) )
    {
      /* make sure top menu popup doesn't get destroyed */
      MENU_PatchResidentPopup( (HQUEUE16)0xFFFF, wndPtr );

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
        if (siblingPtr) DestroyWindow( siblingPtr->hwndSelf );
        else break;
      }

      if( !Options.managed || EVENT_CheckFocus() )
          WINPOS_ActivateOtherWindow(wndPtr);

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

    WIN_DestroyWindow( wndPtr );
    retvalue = TRUE;
end:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/***********************************************************************
 *           CloseWindow16   (USER.43)
 */
BOOL16 WINAPI CloseWindow16( HWND16 hwnd )
{
    return CloseWindow( hwnd );
}

 
/***********************************************************************
 *           CloseWindow32   (USER32.56)
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
 *           OpenIcon16   (USER.44)
 */
BOOL16 WINAPI OpenIcon16( HWND16 hwnd )
{
    return OpenIcon( hwnd );
}


/***********************************************************************
 *           OpenIcon32   (USER32.410)
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
                              LPCSTR title )
{
    WND *pWnd;
    HWND retvalue;
    CLASS *pClass = NULL;

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

    /* For a child window, all siblings will have the same hInstance, */
    /* so we can look for the class once and for all. */

    if (className && (pWnd->dwStyle & WS_CHILD))
    {
        if (!(pClass = CLASS_FindClassByAtom( className, pWnd->hInstance )))
        {
            retvalue = 0;
            goto end;
        }
    }


    for ( ; pWnd ; WIN_UpdateWndPtr(&pWnd,pWnd->next))
    {
        if (className && !(pWnd->dwStyle & WS_CHILD))
        {
            if (!(pClass = CLASS_FindClassByAtom( className, pWnd->hInstance)))
                continue;  /* Skip this window */
        }

        if (pClass && (pWnd->class != pClass))
            continue;  /* Not the right class */

        /* Now check the title */

        if (!title)
        {
            retvalue = pWnd->hwndSelf;
            goto end;
        }
        if (pWnd->text && !strcmp( pWnd->text, title ))
        {
            retvalue = pWnd->hwndSelf;
            goto end;
        }
    }
    retvalue = 0;
end:
    WIN_ReleaseWndPtr(pWnd);
    return retvalue;
}



/***********************************************************************
 *           FindWindow16   (USER.50)
 */
HWND16 WINAPI FindWindow16( SEGPTR className, LPCSTR title )
{
    return FindWindowEx16( 0, 0, className, title );
}


/***********************************************************************
 *           FindWindowEx16   (USER.427)
 */
HWND16 WINAPI FindWindowEx16( HWND16 parent, HWND16 child,
                              SEGPTR className, LPCSTR title )
{
    ATOM atom = 0;

    TRACE(win, "%04x %04x '%s' '%s'\n", parent,
		child, HIWORD(className)?(char *)PTR_SEG_TO_LIN(className):"",
		title ? title : "");

    if (className)
    {
        /* If the atom doesn't exist, then no class */
        /* with this name exists either. */
        if (!(atom = GlobalFindAtom16( className ))) return 0;
    }
    return WIN_FindWindow( parent, child, atom, title );
}


/***********************************************************************
 *           FindWindow32A   (USER32.198)
 */
HWND WINAPI FindWindowA( LPCSTR className, LPCSTR title )
{
    HWND ret = FindWindowExA( 0, 0, className, title );
    if (!ret) SetLastError (ERROR_CANNOT_FIND_WND_CLASS);
    return ret;
}


/***********************************************************************
 *           FindWindowEx32A   (USER32.199)
 */
HWND WINAPI FindWindowExA( HWND parent, HWND child,
                               LPCSTR className, LPCSTR title )
{
    ATOM atom = 0;

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
    return WIN_FindWindow( parent, child, atom, title );
}


/***********************************************************************
 *           FindWindowEx32W   (USER32.200)
 */
HWND WINAPI FindWindowExW( HWND parent, HWND child,
                               LPCWSTR className, LPCWSTR title )
{
    ATOM atom = 0;
    char *buffer;
    HWND hwnd;

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
    buffer = HEAP_strdupWtoA( GetProcessHeap(), 0, title );
    hwnd = WIN_FindWindow( parent, child, atom, buffer );
    HeapFree( GetProcessHeap(), 0, buffer );
    return hwnd;
}


/***********************************************************************
 *           FindWindow32W   (USER32.201)
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
 *           GetDesktopWindow16   (USER.286)
 */
HWND16 WINAPI GetDesktopWindow16(void)
{
    return (HWND16)pWndDesktop->hwndSelf;
}


/**********************************************************************
 *           GetDesktopWindow32   (USER32.232)
 */
HWND WINAPI GetDesktopWindow(void)
{
    return pWndDesktop->hwndSelf;
}


/**********************************************************************
 *           GetDesktopHwnd   (USER.278)
 *
 * Exactly the same thing as GetDesktopWindow(), but not documented.
 * Don't ask me why...
 */
HWND16 WINAPI GetDesktopHwnd16(void)
{
    return (HWND16)pWndDesktop->hwndSelf;
}


/*******************************************************************
 *           EnableWindow16   (USER.34)
 */
BOOL16 WINAPI EnableWindow16( HWND16 hwnd, BOOL16 enable )
{
    return EnableWindow( hwnd, enable );
}


/*******************************************************************
 *           EnableWindow32   (USER32.172)
 */
BOOL WINAPI EnableWindow( HWND hwnd, BOOL enable )
{
    WND *wndPtr;
    BOOL retvalue;

    TRACE(win,"EnableWindow32: ( %x, %d )\n", hwnd, enable);
   
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (enable && (wndPtr->dwStyle & WS_DISABLED))
    {
	  /* Enable window */
	wndPtr->dwStyle &= ~WS_DISABLED;
	SendMessageA( hwnd, WM_ENABLE, TRUE, 0 );
        retvalue = TRUE;
        goto end;
    }
    else if (!enable && !(wndPtr->dwStyle & WS_DISABLED))
    {
	  /* Disable window */
	wndPtr->dwStyle |= WS_DISABLED;
	if ((hwnd == GetFocus()) || IsChild( hwnd, GetFocus() ))
        {
	    SetFocus( 0 );  /* A disabled window can't have the focus */
        }
	if ((hwnd == GetCapture()) || IsChild( hwnd, GetCapture() ))
        {
	    ReleaseCapture();  /* A disabled window can't capture the mouse */
        }
	SendMessageA( hwnd, WM_ENABLE, FALSE, 0 );
        retvalue = FALSE;
        goto end;
    }
    retvalue = ((wndPtr->dwStyle & WS_DISABLED) != 0);
end:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/***********************************************************************
 *           IsWindowEnabled16   (USER.35)
 */ 
BOOL16 WINAPI IsWindowEnabled16(HWND16 hWnd)
{
    return IsWindowEnabled(hWnd);
}


/***********************************************************************
 *           IsWindowEnabled32   (USER32.349)
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
 *           IsWindowUnicode   (USER32.350)
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
 *	     GetWindowWord16    (USER.133)
 */
WORD WINAPI GetWindowWord16( HWND16 hwnd, INT16 offset )
{
    return GetWindowWord( hwnd, offset );
}


/**********************************************************************
 *	     GetWindowWord32    (USER32.314)
 */
WORD WINAPI GetWindowWord( HWND hwnd, INT offset )
{
    WORD retvalue;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(WORD) > wndPtr->class->cbWndExtra)
        {
            WARN( win, "Invalid offset %d\n", offset );
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
    		WARN( win,"GWW_ID: discards high bits of 0x%08x!\n",
                    wndPtr->wIDmenu);
        retvalue = (WORD)wndPtr->wIDmenu;
        goto end;
    case GWW_HWNDPARENT: 
    	retvalue =  GetParent(hwnd);
        goto end;
    case GWW_HINSTANCE:  
    	if (HIWORD(wndPtr->hInstance))
    		WARN(win,"GWW_HINSTANCE: discards high bits of 0x%08x!\n",
                    wndPtr->hInstance);
        retvalue = (WORD)wndPtr->hInstance;
        goto end;
    default:
        WARN( win, "Invalid offset %d\n", offset );
        retvalue = 0;
        goto end;
    }
end:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/**********************************************************************
 *	     WIN_GetWindowInstance
 */
HINSTANCE WIN_GetWindowInstance( HWND hwnd )
{
    
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return (HINSTANCE)0;
    WIN_ReleaseWndPtr(wndPtr);
    return wndPtr->hInstance;
    
}


/**********************************************************************
 *	     SetWindowWord16    (USER.134)
 */
WORD WINAPI SetWindowWord16( HWND16 hwnd, INT16 offset, WORD newval )
{
    return SetWindowWord( hwnd, offset, newval );
}


/**********************************************************************
 *	     SetWindowWord32    (USER32.524)
 */
WORD WINAPI SetWindowWord( HWND hwnd, INT offset, WORD newval )
{
    WORD *ptr, retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(WORD) > wndPtr->class->cbWndExtra)
        {
            WARN( win, "Invalid offset %d\n", offset );
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
            WARN( win, "Invalid offset %d\n", offset );
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
        if (offset + sizeof(LONG) > wndPtr->class->cbWndExtra)
        {
            WARN( win, "Invalid offset %d\n", offset );
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
            WARN( win, "Unknown offset %d\n", offset );
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
    
    TRACE(win,"%x=%p %x %lx %x\n",hwnd, wndPtr, offset, newval, type);

    if (!wndPtr)
    {
       /* Is this the right error? */
       SetLastError( ERROR_INVALID_WINDOW_HANDLE );
       return 0;
    }

    if (offset >= 0)
    {
        if (offset + sizeof(LONG) > wndPtr->class->cbWndExtra)
        {
            WARN( win, "Invalid offset %d\n", offset );

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
		goto end;;
	case GWL_STYLE:
	       	style.styleOld = wndPtr->dwStyle;
		newval &= ~(WS_VISIBLE | WS_CHILD);	/* Some bits can't be changed this way */
		style.styleNew = newval | (style.styleOld & (WS_VISIBLE | WS_CHILD));

		if (wndPtr->flags & WIN_ISWIN32)
			SendMessageA(hwnd,WM_STYLECHANGING,GWL_STYLE,(LPARAM)&style);
		wndPtr->dwStyle = style.styleNew;
		if (wndPtr->flags & WIN_ISWIN32)
			SendMessageA(hwnd,WM_STYLECHANGED,GWL_STYLE,(LPARAM)&style);
                retval = style.styleOld;
                goto end;
		    
        case GWL_USERDATA: 
		ptr = &wndPtr->userdata; 
		break;
        case GWL_EXSTYLE:  
	        style.styleOld = wndPtr->dwExStyle;
		style.styleNew = newval;
		if (wndPtr->flags & WIN_ISWIN32)
			SendMessageA(hwnd,WM_STYLECHANGING,GWL_EXSTYLE,(LPARAM)&style);
		wndPtr->dwExStyle = newval;
		if (wndPtr->flags & WIN_ISWIN32)
			SendMessageA(hwnd,WM_STYLECHANGED,GWL_EXSTYLE,(LPARAM)&style);
                retval = style.styleOld;
                goto end;

	default:
            WARN( win, "Invalid offset %d\n", offset );

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
 *	     GetWindowLong16    (USER.135)
 */
LONG WINAPI GetWindowLong16( HWND16 hwnd, INT16 offset )
{
    return WIN_GetWindowLong( (HWND)hwnd, offset, WIN_PROC_16 );
}


/**********************************************************************
 *	     GetWindowLong32A    (USER32.305)
 */
LONG WINAPI GetWindowLongA( HWND hwnd, INT offset )
{
    return WIN_GetWindowLong( hwnd, offset, WIN_PROC_32A );
}


/**********************************************************************
 *	     GetWindowLong32W    (USER32.306)
 */
LONG WINAPI GetWindowLongW( HWND hwnd, INT offset )
{
    return WIN_GetWindowLong( hwnd, offset, WIN_PROC_32W );
}


/**********************************************************************
 *	     SetWindowLong16    (USER.136)
 */
LONG WINAPI SetWindowLong16( HWND16 hwnd, INT16 offset, LONG newval )
{
    return WIN_SetWindowLong( hwnd, offset, newval, WIN_PROC_16 );
}


/**********************************************************************
 *	     SetWindowLong32A    (USER32.517)
 */
LONG WINAPI SetWindowLongA( HWND hwnd, INT offset, LONG newval )
{
    return WIN_SetWindowLong( hwnd, offset, newval, WIN_PROC_32A );
}


/**********************************************************************
 *	     SetWindowLong32W    (USER32.518) Set window attribute
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
    HWND hwnd, /* window to alter */
    INT offset, /* offset, in bytes, of location to alter */
    LONG newval  /* new value of location */
) {
    return WIN_SetWindowLong( hwnd, offset, newval, WIN_PROC_32W );
}


/*******************************************************************
 *	     GetWindowText16    (USER.36)
 */
INT16 WINAPI GetWindowText16( HWND16 hwnd, SEGPTR lpString, INT16 nMaxCount )
{
    return (INT16)SendMessage16(hwnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString);
}


/*******************************************************************
 *	     GetWindowText32A    (USER32.309)
 */
INT WINAPI GetWindowTextA( HWND hwnd, LPSTR lpString, INT nMaxCount )
{
    return (INT)SendMessageA( hwnd, WM_GETTEXT, nMaxCount,
                                  (LPARAM)lpString );
}

/*******************************************************************
 *	     InternalGetWindowText    (USER32.326)
 */
INT WINAPI InternalGetWindowText(HWND hwnd,LPWSTR lpString,INT nMaxCount )
{
    FIXME(win,"(0x%08x,%p,0x%x),stub!\n",hwnd,lpString,nMaxCount);
    return GetWindowTextW(hwnd,lpString,nMaxCount);
}


/*******************************************************************
 *	     GetWindowText32W    (USER32.312)
 */
INT WINAPI GetWindowTextW( HWND hwnd, LPWSTR lpString, INT nMaxCount )
{
    return (INT)SendMessageW( hwnd, WM_GETTEXT, nMaxCount,
                                  (LPARAM)lpString );
}


/*******************************************************************
 *	     SetWindowText16    (USER.37)
 */
BOOL16 WINAPI SetWindowText16( HWND16 hwnd, SEGPTR lpString )
{
    return (BOOL16)SendMessage16( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *	     SetWindowText32A    (USER32.521)
 */
BOOL WINAPI SetWindowTextA( HWND hwnd, LPCSTR lpString )
{
    return (BOOL)SendMessageA( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *	     SetWindowText32W    (USER32.523)
 */
BOOL WINAPI SetWindowTextW( HWND hwnd, LPCWSTR lpString )
{
    return (BOOL)SendMessageW( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *         GetWindowTextLength16    (USER.38)
 */
INT16 WINAPI GetWindowTextLength16( HWND16 hwnd )
{
    return (INT16)SendMessage16( hwnd, WM_GETTEXTLENGTH, 0, 0 );
}


/*******************************************************************
 *         GetWindowTextLength32A   (USER32.310)
 */
INT WINAPI GetWindowTextLengthA( HWND hwnd )
{
    return SendMessageA( hwnd, WM_GETTEXTLENGTH, 0, 0 );
}

/*******************************************************************
 *         GetWindowTextLength32W   (USER32.311)
 */
INT WINAPI GetWindowTextLengthW( HWND hwnd )
{
    return SendMessageW( hwnd, WM_GETTEXTLENGTH, 0, 0 );
}


/*******************************************************************
 *         IsWindow16   (USER.47)
 */
BOOL16 WINAPI IsWindow16( HWND16 hwnd )
{
    return IsWindow( hwnd );
}

void WINAPI WIN16_IsWindow16( CONTEXT *context )
{
    WORD *stack = PTR_SEG_OFF_TO_LIN(SS_reg(context), SP_reg(context));
    HWND16 hwnd = (HWND16)stack[2];

    AX_reg(context) = IsWindow( hwnd );
    ES_reg(context) = USER_HeapSel;
}


/*******************************************************************
 *         IsWindow32   (USER32.348)
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
 *         GetParent16   (USER.46)
 */
HWND16 WINAPI GetParent16( HWND16 hwnd )
{
    return (HWND16)GetParent( hwnd );
}


/*****************************************************************
 *         GetParent32   (USER32.278)
 */
HWND WINAPI GetParent( HWND hwnd )
{
    WND *wndPtr;
    HWND retvalue;
    
    if(!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
    if ((!(wndPtr->dwStyle & (WS_POPUP|WS_CHILD))))
    {
        WIN_ReleaseWndPtr(wndPtr);
        return 0;
    }
    WIN_UpdateWndPtr(&wndPtr,((wndPtr->dwStyle & WS_CHILD) ? wndPtr->parent : wndPtr->owner));
    retvalue = wndPtr ? wndPtr->hwndSelf : 0;

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
    while( pWnd && (pWnd->dwStyle & WS_CHILD))
    {
        WIN_UpdateWndPtr(&pWnd,pWnd->parent);
    }
    return pWnd;
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
 *         SetParent16   (USER.233)
 */
HWND16 WINAPI SetParent16( HWND16 hwndChild, HWND16 hwndNewParent )
{
    return SetParent( hwndChild, hwndNewParent );
}


/*****************************************************************
 *         SetParent32   (USER32.495)
 */
HWND WINAPI SetParent( HWND hwndChild, HWND hwndNewParent )
{
  WND *wndPtr;
  DWORD dwStyle;
  WND *pWndNewParent;
  WND *pWndOldParent;
  HWND retvalue;


  if(!(wndPtr = WIN_FindWndPtr(hwndChild))) return 0;

  dwStyle = wndPtr->dwStyle;

  pWndNewParent = hwndNewParent ? WIN_FindWndPtr(hwndNewParent)
                                : WIN_LockWndPtr(pWndDesktop);

  /* Windows hides the window first, then shows it again
   * including the WM_SHOWWINDOW messages and all */
  if (dwStyle & WS_VISIBLE)
      ShowWindow( hwndChild, SW_HIDE );

  pWndOldParent = WIN_LockWndPtr((*wndPtr->pDriver->pSetParent)(wndPtr, pWndNewParent));

  /* SetParent32 additionally needs to make hwndChild the topmost window
     in the x-order and send the expected WM_WINDOWPOSCHANGING and
     WM_WINDOWPOSCHANGED notification messages. 
  */
  SetWindowPos( hwndChild, HWND_TOPMOST, 0, 0, 0, 0,
      SWP_NOMOVE|SWP_NOSIZE|((dwStyle & WS_VISIBLE)?SWP_SHOWWINDOW:0));
  /* FIXME: a WM_MOVE is also generated (in the DefWindowProc handler
   * for WM_WINDOWPOSCHANGED) in Windows, should probably remove SWP_NOMOVE */

  retvalue = pWndOldParent?pWndOldParent->hwndSelf:0;

  WIN_ReleaseWndPtr(pWndOldParent);
  WIN_ReleaseWndPtr(pWndNewParent);
  WIN_ReleaseWndPtr(wndPtr);

  return retvalue;
  
}

/*******************************************************************
 *         IsChild16    (USER.48)
 */
BOOL16 WINAPI IsChild16( HWND16 parent, HWND16 child )
{
    return IsChild(parent,child);
}


/*******************************************************************
 *         IsChild32    (USER32.339)
 */
BOOL WINAPI IsChild( HWND parent, HWND child )
{
    WND * wndPtr = WIN_FindWndPtr( child );
    while (wndPtr && (wndPtr->dwStyle & WS_CHILD))
    {
        WIN_UpdateWndPtr(&wndPtr,wndPtr->parent);
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
 *           IsWindowVisible16   (USER.49)
 */
BOOL16 WINAPI IsWindowVisible16( HWND16 hwnd )
{
    return IsWindowVisible(hwnd);
}


/***********************************************************************
 *           IsWindowVisible32   (USER32.351)
 */
BOOL WINAPI IsWindowVisible( HWND hwnd )
{
    BOOL retval;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    while (wndPtr && (wndPtr->dwStyle & WS_CHILD))
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
  if( (wnd->dwStyle & WS_MINIMIZE &&
       icon && wnd->class->hIcon) ||
     !(wnd->dwStyle & WS_VISIBLE) ) return FALSE;
  for(wnd = wnd->parent; wnd; wnd = wnd->parent)
    if( wnd->dwStyle & WS_MINIMIZE ||
      !(wnd->dwStyle & WS_VISIBLE) ) break;
  return (wnd == NULL);
}


/*******************************************************************
 *         GetTopWindow16    (USER.229)
 */
HWND16 WINAPI GetTopWindow16( HWND16 hwnd )
{
    return GetTopWindow(hwnd);
}


/*******************************************************************
 *         GetTopWindow32    (USER.229)
 */
HWND WINAPI GetTopWindow( HWND hwnd )
{
    HWND retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (wndPtr && wndPtr->child)
    {
        retval = wndPtr->child->hwndSelf;
    }
    else retval = 0;
    WIN_ReleaseWndPtr(wndPtr);
    return retval;
}


/*******************************************************************
 *         GetWindow16    (USER.262)
 */
HWND16 WINAPI GetWindow16( HWND16 hwnd, WORD rel )
{
    return GetWindow( hwnd,rel );
}


/*******************************************************************
 *         GetWindow32    (USER32.302)
 */
HWND WINAPI GetWindow( HWND hwnd, WORD rel )
{
    HWND retval;
    
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    switch(rel)
    {
    case GW_HWNDFIRST:
        if (wndPtr->parent) retval = wndPtr->parent->child->hwndSelf;
        else retval = 0;
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
        if (!wndPtr->next) retval = 0;
        else retval = wndPtr->next->hwndSelf;
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
 *         GetNextWindow16    (USER.230)
 */
HWND16 WINAPI GetNextWindow16( HWND16 hwnd, WORD flag )
{
    if ((flag != GW_HWNDNEXT) && (flag != GW_HWNDPREV)) return 0;
    return GetWindow16( hwnd, flag );
}

/*******************************************************************
 *         ShowOwnedPopups16  (USER.265)
 */
void WINAPI ShowOwnedPopups16( HWND16 owner, BOOL16 fShow )
{
    ShowOwnedPopups( owner, fShow );
}


/*******************************************************************
 *         ShowOwnedPopups32  (USER32.531)
 */
BOOL WINAPI ShowOwnedPopups( HWND owner, BOOL fShow )
{
    WND *pWnd;
    pWnd = WIN_LockWndPtr(pWndDesktop->child);
    while (pWnd)
    {
        if (pWnd->owner && (pWnd->owner->hwndSelf == owner) &&
            (pWnd->dwStyle & WS_POPUP))
            ShowWindow( pWnd->hwndSelf, fShow ? SW_SHOW : SW_HIDE );
        WIN_UpdateWndPtr(&pWnd,pWnd->next);
    }
    return TRUE;
}


/*******************************************************************
 *         GetLastActivePopup16   (USER.287)
 */
HWND16 WINAPI GetLastActivePopup16( HWND16 hwnd )
{
    return GetLastActivePopup( hwnd );
}

/*******************************************************************
 *         GetLastActivePopup32   (USER32.256)
 */
HWND WINAPI GetLastActivePopup( HWND hwnd )
{
    WND *wndPtr;
    HWND retval;
    wndPtr = WIN_FindWndPtr(hwnd);
    if (!wndPtr) return hwnd;
    retval = wndPtr->hwndLastActive;
    WIN_ReleaseWndPtr(wndPtr);
    return retval;
}


/*******************************************************************
 *           WIN_BuildWinArray
 *
 * Build an array of pointers to the children of a given window.
 * The array must be freed with HeapFree(SystemHeap). Return NULL
 * when no windows are found.
 */
WND **WIN_BuildWinArray( WND *wndPtr, UINT bwaFlags, UINT* pTotal )
{
    /* Future : this function will lock all windows associated with this array */
    
    WND **list, **ppWnd;
    WND *pWnd;
    UINT count = 0, skipOwned, skipHidden;
    DWORD skipFlags;

    skipHidden = bwaFlags & BWA_SKIPHIDDEN;
    skipOwned = bwaFlags & BWA_SKIPOWNED;
    skipFlags = (bwaFlags & BWA_SKIPDISABLED) ? WS_DISABLED : 0;
    if( bwaFlags & BWA_SKIPICONIC ) skipFlags |= WS_MINIMIZE;

    /* First count the windows */

    if (!wndPtr)
        wndPtr = WIN_GetDesktop();

    pWnd = WIN_LockWndPtr(wndPtr->child);
    while (pWnd)
    {
	if( (pWnd->dwStyle & skipFlags) || (skipOwned && pWnd->owner) ) continue;
        if( !skipHidden || pWnd->dwStyle & WS_VISIBLE )
        {
            count++;
            WIN_UpdateWndPtr(&pWnd,pWnd->next);
    }

    }

    if( count )
    {
	/* Now build the list of all windows */

	if ((list = (WND **)HeapAlloc( SystemHeap, 0, sizeof(WND *) * (count + 1))))
	{
	    for (pWnd = WIN_LockWndPtr(wndPtr->child), ppWnd = list, count = 0; pWnd; WIN_UpdateWndPtr(&pWnd,pWnd->next))
	    {
		if( (pWnd->dwStyle & skipFlags) || (skipOwned && pWnd->owner) );
		else if( !skipHidden || pWnd->dwStyle & WS_VISIBLE )
		{
		   *ppWnd++ = pWnd;
		    count++;
		}
	    }
            WIN_ReleaseWndPtr(pWnd);
	   *ppWnd = NULL;
	}
	else count = 0;
    } else list = NULL;

    if( pTotal ) *pTotal = count;
    return list;
}
/*******************************************************************
 *           WIN_ReleaseWinArray
 */
void WIN_ReleaseWinArray(WND **wndArray)
{
    /* Future : this function will also unlock all windows associated with wndArray */
     HeapFree( SystemHeap, 0, wndArray );

}

/*******************************************************************
 *           EnumWindows16   (USER.54)
 */
BOOL16 WINAPI EnumWindows16( WNDENUMPROC16 lpEnumFunc, LPARAM lParam )
{
    WND **list, **ppWnd;

    /* We have to build a list of all windows first, to avoid */
    /* unpleasant side-effects, for instance if the callback  */
    /* function changes the Z-order of the windows.           */

    if (!(list = WIN_BuildWinArray(WIN_GetDesktop(), 0, NULL )))
    {
        WIN_ReleaseDesktop();
        return FALSE;
    }

    /* Now call the callback function for every window */

    for (ppWnd = list; *ppWnd; ppWnd++)
    {
        /* Make sure that the window still exists */
        if (!IsWindow((*ppWnd)->hwndSelf)) continue;
        if (!lpEnumFunc( (*ppWnd)->hwndSelf, lParam )) break;
    }
    WIN_ReleaseWinArray(list);
    WIN_ReleaseDesktop();
    return TRUE;
}


/*******************************************************************
 *           EnumWindows32   (USER32.193)
 */
BOOL WINAPI EnumWindows( WNDENUMPROC lpEnumFunc, LPARAM lParam )
{
    return (BOOL)EnumWindows16( (WNDENUMPROC16)lpEnumFunc, lParam );
}


/**********************************************************************
 *           EnumTaskWindows16   (USER.225)
 */
BOOL16 WINAPI EnumTaskWindows16( HTASK16 hTask, WNDENUMPROC16 func,
                                 LPARAM lParam )
{
    WND **list, **ppWnd;

    /* This function is the same as EnumWindows(),    */
    /* except for an added check on the window's task. */

    if (!(list = WIN_BuildWinArray( WIN_GetDesktop(), 0, NULL )))
    {
        WIN_ReleaseDesktop();
        return FALSE;
    }

    /* Now call the callback function for every window */

    for (ppWnd = list; *ppWnd; ppWnd++)
    {
        /* Make sure that the window still exists */
        if (!IsWindow((*ppWnd)->hwndSelf)) continue;
        if (QUEUE_GetQueueTask((*ppWnd)->hmemTaskQ) != hTask) continue;
        if (!func( (*ppWnd)->hwndSelf, lParam )) break;
    }
    WIN_ReleaseWinArray(list);
    WIN_ReleaseDesktop();
    return TRUE;
}


/**********************************************************************
 *           EnumThreadWindows   (USER32.190)
 */
BOOL WINAPI EnumThreadWindows( DWORD id, WNDENUMPROC func, LPARAM lParam )
{
    THDB *tdb = THREAD_IdToTHDB(id);

    return (BOOL16)EnumTaskWindows16(tdb->teb.htask16, (WNDENUMPROC16)func, lParam);
}


/**********************************************************************
 *           WIN_EnumChildWindows
 *
 * Helper function for EnumChildWindows().
 */
static BOOL16 WIN_EnumChildWindows( WND **ppWnd, WNDENUMPROC16 func,
                                    LPARAM lParam )
{
    WND **childList;
    BOOL16 ret = FALSE;

    for ( ; *ppWnd; ppWnd++)
    {
        /* Make sure that the window still exists */
        if (!IsWindow((*ppWnd)->hwndSelf)) continue;
        /* Build children list first */
        childList = WIN_BuildWinArray( *ppWnd, BWA_SKIPOWNED, NULL );
        ret = func( (*ppWnd)->hwndSelf, lParam );
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
 *           EnumChildWindows16   (USER.55)
 */
BOOL16 WINAPI EnumChildWindows16( HWND16 parent, WNDENUMPROC16 func,
                                  LPARAM lParam )
{
    WND **list, *pParent;

    if (!(pParent = WIN_FindWndPtr( parent ))) return FALSE;
    if (!(list = WIN_BuildWinArray( pParent, BWA_SKIPOWNED, NULL )))
    {
        WIN_ReleaseWndPtr(pParent);
        return FALSE;
    }
    WIN_EnumChildWindows( list, func, lParam );
    WIN_ReleaseWinArray(list);
    WIN_ReleaseWndPtr(pParent);
    return TRUE;
}


/**********************************************************************
 *           EnumChildWindows32   (USER32.178)
 */
BOOL WINAPI EnumChildWindows( HWND parent, WNDENUMPROC func,
                                  LPARAM lParam )
{
    return (BOOL)EnumChildWindows16( (HWND16)parent, (WNDENUMPROC16)func,
                                       lParam );
}


/*******************************************************************
 *           AnyPopup16   (USER.52)
 */
BOOL16 WINAPI AnyPopup16(void)
{
    return AnyPopup();
}


/*******************************************************************
 *           AnyPopup32   (USER32.4)
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
 *            FlashWindow16   (USER.105)
 */
BOOL16 WINAPI FlashWindow16( HWND16 hWnd, BOOL16 bInvert )
{
    return FlashWindow( hWnd, bInvert );
}


/*******************************************************************
 *            FlashWindow32   (USER32.202)
 */
BOOL WINAPI FlashWindow( HWND hWnd, BOOL bInvert )
{
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    TRACE(win,"%04x\n", hWnd);

    if (!wndPtr) return FALSE;

    if (wndPtr->dwStyle & WS_MINIMIZE)
    {
        if (bInvert && !(wndPtr->flags & WIN_NCACTIVATED))
        {
            HDC hDC = GetDC(hWnd);
            
            if (!SendMessage16( hWnd, WM_ERASEBKGND, (WPARAM16)hDC, 0 ))
                wndPtr->flags |= WIN_NEEDS_ERASEBKGND;
            
            ReleaseDC( hWnd, hDC );
            wndPtr->flags |= WIN_NCACTIVATED;
        }
        else
        {
            PAINT_RedrawWindow( hWnd, 0, 0, RDW_INVALIDATE | RDW_ERASE |
					  RDW_UPDATENOW | RDW_FRAME, 0 );
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

        SendMessage16( hWnd, WM_NCACTIVATE, wparam, (LPARAM)0 );
        WIN_ReleaseWndPtr(wndPtr);
        return wparam;
    }
}


/*******************************************************************
 *           SetSysModalWindow16   (USER.188)
 */
HWND16 WINAPI SetSysModalWindow16( HWND16 hWnd )
{
    HWND hWndOldModal = hwndSysModal;
    hwndSysModal = hWnd;
    FIXME(win, "EMPTY STUB !! SetSysModalWindow(%04x) !\n", hWnd);
    return hWndOldModal;
}


/*******************************************************************
 *           GetSysModalWindow16   (USER.52)
 */
HWND16 WINAPI GetSysModalWindow16(void)
{
    return hwndSysModal;
}


/*******************************************************************
 *           GetWindowContextHelpId   (USER32.303)
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
 *           SetWindowContextHelpId   (USER32.515)
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
 BOOL16		wParam,bResult = 0;
 POINT        pt;
 LPDRAGINFO	ptrDragInfo = (LPDRAGINFO) PTR_SEG_TO_LIN(spDragInfo);
 WND 	       *ptrQueryWnd = WIN_FindWndPtr(hQueryWnd),*ptrWnd;
 RECT		tempRect;

 if( !ptrQueryWnd || !ptrDragInfo )
 {
     WIN_ReleaseWndPtr(ptrQueryWnd);
     return 0;
 }

 CONV_POINT16TO32( &ptrDragInfo->pt, &pt );

 GetWindowRect(hQueryWnd,&tempRect); 

 if( !PtInRect(&tempRect,pt) ||
     (ptrQueryWnd->dwStyle & WS_DISABLED) )
 {
     WIN_ReleaseWndPtr(ptrQueryWnd);
	return 0;
 }

 if( !(ptrQueryWnd->dwStyle & WS_MINIMIZE) ) 
   {
     tempRect = ptrQueryWnd->rectClient;
     if(ptrQueryWnd->dwStyle & WS_CHILD)
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
	    TRACE(msg,"hwnd = %04x, %d %d - %d %d\n",
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
   }
 else wParam = 1;

 ScreenToClient16(hQueryWnd,&ptrDragInfo->pt);

 ptrDragInfo->hScope = hQueryWnd;

 bResult = ( bNoSend ) 
	   ? ptrQueryWnd->dwExStyle & WS_EX_ACCEPTFILES
	   : SendMessage16( hQueryWnd ,WM_QUERYDROPOBJECT ,
                          (WPARAM16)wParam ,(LPARAM) spDragInfo );
 if( !bResult ) 
     CONV_POINT32TO16( &pt, &ptrDragInfo->pt );

 WIN_ReleaseWndPtr(ptrQueryWnd);
 return bResult;
}


/*******************************************************************
 *             DragDetect   (USER.465)
 */
BOOL16 WINAPI DragDetect16( HWND16 hWnd, POINT16 pt )
{
    POINT pt32;
    CONV_POINT16TO32( &pt, &pt32 );
    return DragDetect( hWnd, pt32 );
}

/*******************************************************************
 *             DragDetect32   (USER32.151)
 */
BOOL WINAPI DragDetect( HWND hWnd, POINT pt )
{
    MSG16 msg;
    RECT16  rect;

    rect.left = pt.x - wDragWidth;
    rect.right = pt.x + wDragWidth;

    rect.top = pt.y - wDragHeight;
    rect.bottom = pt.y + wDragHeight;

    SetCapture(hWnd);

    while(1)
    {
	while(PeekMessage16(&msg ,0 ,WM_MOUSEFIRST ,WM_MOUSELAST ,PM_REMOVE))
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
 *             DragObject16   (USER.464)
 */
DWORD WINAPI DragObject16( HWND16 hwndScope, HWND16 hWnd, UINT16 wObj,
                           HANDLE16 hOfStruct, WORD szList, HCURSOR16 hCursor )
{
    MSG16	msg;
    LPDRAGINFO	lpDragInfo;
    SEGPTR	spDragInfo;
    HCURSOR16 	hDragCursor=0, hOldCursor=0, hBummer=0;
    HGLOBAL16	hDragInfo  = GlobalAlloc16( GMEM_SHARE | GMEM_ZEROINIT, 2*sizeof(DRAGINFO));
    WND        *wndPtr = WIN_FindWndPtr(hWnd);
    HCURSOR16	hCurrentCursor = 0;
    HWND16	hCurrentWnd = 0;

    lpDragInfo = (LPDRAGINFO) GlobalLock16(hDragInfo);
    spDragInfo = (SEGPTR) WIN16_GlobalLock16(hDragInfo);

    if( !lpDragInfo || !spDragInfo )
    {
        WIN_ReleaseWndPtr(wndPtr);
        return 0L;
    }

    hBummer = LoadCursor16(0, IDC_BUMMER16);

    if( !hBummer || !wndPtr )
    {
        GlobalFree16(hDragInfo);
        WIN_ReleaseWndPtr(wndPtr);
        return 0L;
    }

    if(hCursor)
    {
	if( !(hDragCursor = CURSORICON_IconToCursor(hCursor, FALSE)) )
	{
	    GlobalFree16(hDragInfo);
            WIN_ReleaseWndPtr(wndPtr);
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
	do{  WaitMessage(); }
	while( !PeekMessage16(&msg,0,WM_MOUSEFIRST,WM_MOUSELAST,PM_REMOVE) );

       *(lpDragInfo+1) = *lpDragInfo;

	lpDragInfo->pt = msg.pt;

	/* update DRAGINFO struct */
	TRACE(msg,"lpDI->hScope = %04x\n",lpDragInfo->hScope);

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
		       (LPARAM)MAKELONG(LOWORD(spDragInfo)+sizeof(DRAGINFO),
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
    WIN_ReleaseWndPtr(wndPtr);

    return (DWORD)(msg.lParam);
}
