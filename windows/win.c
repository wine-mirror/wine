/*
 * Window related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <stdlib.h>
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
#include "queue.h"
#include "winpos.h"
#include "shm_main_blk.h"
#include "dde_proc.h"
#include "clipboard.h"
#include "winproc.h"
#include "thread.h"
#include "process.h"
#include "debug.h"
#include "winerror.h"
#include "mdi.h"

extern WND_DRIVER X11DRV_WND_Driver;

/* Desktop window */
static WND *pWndDesktop = NULL;

static HWND32 hwndSysModal = 0;

static WORD wDragWidth = 4;
static WORD wDragHeight= 3;

/***********************************************************************
 *           WIN_FindWndPtr
 *
 * Return a pointer to the WND structure corresponding to a HWND.
 */
WND * WIN_FindWndPtr( HWND32 hwnd )
{
    WND * ptr;
    
    if (!hwnd || HIWORD(hwnd)) return NULL;
    ptr = (WND *) USER_HEAP_LIN_ADDR( hwnd );
    if (ptr->dwMagic != WND_MAGIC) return NULL;
    if (ptr->hwndSelf != hwnd)
    {
        ERR( win, "Can't happen: hwnd %04x self pointer is %04x\n",
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
void WIN_DumpWindow( HWND32 hwnd )
{
    WND *ptr;
    char className[80];
    int i;

    if (!(ptr = WIN_FindWndPtr( hwnd )))
    {
        WARN( win, "%04x is not a window handle\n", hwnd );
        return;
    }

    if (!GetClassName32A( hwnd, className, sizeof(className ) ))
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
             ptr->dwStyle, ptr->dwExStyle, (UINT32)ptr->winproc,
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
}


/***********************************************************************
 *           WIN_WalkWindows
 *
 * Walk the windows tree and print each window on stderr.
 */
void WIN_WalkWindows( HWND32 hwnd, int indent )
{
    WND *ptr;
    char className[80];

    ptr = hwnd ? WIN_FindWndPtr( hwnd ) : pWndDesktop;
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
                 (UINT32)ptr->dwStyle, (UINT32)ptr->winproc,
                 ptr->text?ptr->text:"<null>");
        
        if (ptr->child) WIN_WalkWindows( ptr->child->hwndSelf, indent+1 );
        ptr = ptr->next;
    }
}

/***********************************************************************
 *           WIN_UnlinkWindow
 *
 * Remove a window from the siblings linked list.
 */
BOOL32 WIN_UnlinkWindow( HWND32 hwnd )
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
BOOL32 WIN_LinkWindow( HWND32 hwnd, HWND32 hwndInsertAfter )
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
HWND32 WIN_FindWinToRepaint( HWND32 hwnd, HQUEUE16 hQueue )
{
    HWND32 hwndRet;
    WND *pWnd = pWndDesktop;

    /* Note: the desktop window never gets WM_PAINT messages 
     * The real reason why is because Windows DesktopWndProc
     * does ValidateRgn inside WM_ERASEBKGND handler.
     */

    pWnd = hwnd ? WIN_FindWndPtr( hwnd ) : pWndDesktop->child;

    for ( ; pWnd ; pWnd = pWnd->next )
    {
        if (!(pWnd->dwStyle & WS_VISIBLE))
        {
            TRACE(win, "skipping window %04x\n",
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
    TRACE(win,"found %04x\n",hwndRet);
    return hwndRet;
}


/***********************************************************************
 *           WIN_DestroyWindow
 *
 * Destroy storage associated to a window. "Internals" p.358
 */
static WND* WIN_DestroyWindow( WND* wndPtr )
{
    HWND32 hwnd = wndPtr->hwndSelf;
    WND *pWnd;

    TRACE(win, "%04x\n", wndPtr->hwndSelf );

#ifdef CONFIG_IPC
    if (main_block)
	DDE_DestroyWindow(wndPtr->hwndSelf);
#endif  /* CONFIG_IPC */
	
    /* free child windows */

    while ((pWnd = wndPtr->child))
        wndPtr->child = WIN_DestroyWindow( pWnd );

    SendMessage32A( wndPtr->hwndSelf, WM_NCDESTROY, 0, 0);

    /* FIXME: do we need to fake QS_MOUSEMOVE wakebit? */

    WINPOS_CheckInternalPos( hwnd );
    if( hwnd == GetCapture32()) ReleaseCapture();

    /* free resources associated with the window */

    TIMER_RemoveWindowTimers( wndPtr->hwndSelf );
    PROPERTY_RemoveWindowProps( wndPtr );

    wndPtr->dwMagic = 0;  /* Mark it as invalid */

    if ((wndPtr->hrgnUpdate) || (wndPtr->flags & WIN_INTERNAL_PAINT))
    {
        if (wndPtr->hrgnUpdate > 1) DeleteObject32( wndPtr->hrgnUpdate );
        QUEUE_DecPaintCount( wndPtr->hmemTaskQ );
    }

    /* toss stale messages from the queue */

    if( wndPtr->hmemTaskQ )
    {
        int           pos;
	BOOL32	      bPostQuit = FALSE;
	WPARAM32      wQuitParam = 0;
        MESSAGEQUEUE* msgQ = (MESSAGEQUEUE*) GlobalLock16(wndPtr->hmemTaskQ);

	while( (pos = QUEUE_FindMsg(msgQ, hwnd, 0, 0)) != -1 )
	{
	    if( msgQ->messages[pos].msg.message == WM_QUIT ) 
	    {
		bPostQuit = TRUE;
		wQuitParam = msgQ->messages[pos].msg.wParam;
	    }
	    QUEUE_RemoveMsg(msgQ, pos);
	}
	/* repost WM_QUIT to make sure this app exits its message loop */
	if( bPostQuit ) PostQuitMessage32(wQuitParam);
	wndPtr->hmemTaskQ = 0;
    }

    if (!(wndPtr->dwStyle & WS_CHILD))
       if (wndPtr->wIDmenu) DestroyMenu32( (HMENU32)wndPtr->wIDmenu );
    if (wndPtr->hSysMenu) DestroyMenu32( wndPtr->hSysMenu );
    wndPtr->pDriver->pDestroyWindow( wndPtr );
    DCE_FreeWindowDCE( wndPtr );    /* Always do this to catch orphaned DCs */ 
    WINPROC_FreeProc( wndPtr->winproc, WIN_PROC_WINDOW );
    wndPtr->hwndSelf = 0;
    wndPtr->class->cWindows--;
    wndPtr->class = NULL;
    pWnd = wndPtr->next;

    USER_HEAP_FREE( hwnd );
    return pWnd;
}

/***********************************************************************
 *           WIN_ResetQueueWindows
 *
 * Reset the queue of all the children of a given window.
 * Return TRUE if something was done.
 */
BOOL32 WIN_ResetQueueWindows( WND* wnd, HQUEUE16 hQueue, HQUEUE16 hNew )
{
    BOOL32 ret = FALSE;

    if (hNew)  /* Set a new queue */
    {
        for (wnd = wnd->child; (wnd); wnd = wnd->next)
        {
            if (wnd->hmemTaskQ == hQueue)
            {
                wnd->hmemTaskQ = hNew;
                ret = TRUE;
            }
            if (wnd->child)
                ret |= WIN_ResetQueueWindows( wnd, hQueue, hNew );
        }
    }
    else  /* Queue is being destroyed */
    {
        while (wnd->child)
        {
            WND *tmp = wnd->child;
            ret = FALSE;
            while (tmp)
            {
                if (tmp->hmemTaskQ == hQueue)
                {
                    DestroyWindow32( tmp->hwndSelf );
                    ret = TRUE;
                    break;
                }
                if (tmp->child && WIN_ResetQueueWindows(tmp->child,hQueue,0))
                    ret = TRUE;
                else
                    tmp = tmp->next;
            }
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
BOOL32 WIN_CreateDesktopWindow(void)
{
    CLASS *class;
    HWND32 hwndDesktop;

    TRACE(win,"Creating desktop window\n");

    if (!ICONTITLE_Init() ||
	!WINPOS_CreateInternalPosAtom() ||
	!(class = CLASS_FindClassByAtom( DESKTOP_CLASS_ATOM, 0 )))
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
    pWndDesktop->text              = NULL;
    pWndDesktop->hmemTaskQ         = 0; /* Desktop does not belong to a task */
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
    pWndDesktop->pDriver           = &X11DRV_WND_Driver;
    pWndDesktop->winproc = (WNDPROC16)class->winproc;

    /* FIXME: How do we know if it should be Unicode or not */
    if(!pWndDesktop->pDriver->pCreateDesktopWindow(pWndDesktop, class, FALSE))
      return FALSE;
    
    SendMessage32A( hwndDesktop, WM_NCCREATE, 0, 0 );
    pWndDesktop->flags |= WIN_NEEDS_ERASEBKGND;
    return TRUE;
}


/***********************************************************************
 *           WIN_CreateWindowEx
 *
 * Implementation of CreateWindowEx().
 */
static HWND32 WIN_CreateWindowEx( CREATESTRUCT32A *cs, ATOM classAtom,
                                  BOOL32 win32, BOOL32 unicode )
{
    CLASS *classPtr;
    WND *wndPtr;
    HWND16 hwnd, hwndLinkAfter;
    POINT32 maxSize, maxPos, minTrack, maxTrack;
    LRESULT (CALLBACK *localSend32)(HWND32, UINT32, WPARAM32, LPARAM);

    TRACE(win, "%s %s %08lx %08lx %d,%d %dx%d %04x %04x %08x %p\n",
          unicode ? debugres_w((LPWSTR)cs->lpszName) : debugres_a(cs->lpszName), 
          unicode ? debugres_w((LPWSTR)cs->lpszClass) : debugres_a(cs->lpszClass),
          cs->dwExStyle, cs->style, cs->x, cs->y, cs->cx, cs->cy,
          cs->hwndParent, cs->hMenu, cs->hInstance, cs->lpCreateParams );

    /* Find the parent window */

    if (cs->hwndParent)
    {
	/* Make sure parent is valid */
        if (!IsWindow32( cs->hwndParent ))
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
        GlobalGetAtomName32A( classAtom, buffer, sizeof(buffer) );
        WARN( win, "Bad class '%s'\n", buffer );
        return 0;
    }

    /* Fix the coordinates */

    if (cs->x == CW_USEDEFAULT32) 
    {
        PDB32 *pdb = PROCESS_Current();
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
    if (cs->cx == CW_USEDEFAULT32)
    {
        PDB32 *pdb = PROCESS_Current();
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

    wndPtr = (WND *) USER_HEAP_LIN_ADDR( hwnd );
    wndPtr->next  = NULL;
    wndPtr->child = NULL;

    if ((cs->style & WS_CHILD) && cs->hwndParent)
    {
        wndPtr->parent = WIN_FindWndPtr( cs->hwndParent );
        wndPtr->owner  = NULL;
    }
    else
    {
        wndPtr->parent = pWndDesktop;
        if (!cs->hwndParent || (cs->hwndParent == pWndDesktop->hwndSelf))
            wndPtr->owner = NULL;
        else
            wndPtr->owner = WIN_GetTopParentPtr(WIN_FindWndPtr(cs->hwndParent));
    }

    wndPtr->pDriver        = &X11DRV_WND_Driver;
    wndPtr->window         = 0;
    wndPtr->class          = classPtr;
    wndPtr->winproc        = classPtr->winproc;
    wndPtr->dwMagic        = WND_MAGIC;
    wndPtr->hwndSelf       = hwnd;
    wndPtr->hInstance      = cs->hInstance;
    wndPtr->text           = NULL;
    wndPtr->hmemTaskQ      = GetTaskQueue(0);
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
	CBT_CREATEWND32A cbtc;
        LRESULT ret;

	cbtc.lpcs = cs;
	cbtc.hwndInsertAfter = hwndLinkAfter;
        ret = unicode ? HOOK_CallHooks32W(WH_CBT, HCBT_CREATEWND, hwnd, (LPARAM)&cbtc)
                      : HOOK_CallHooks32A(WH_CBT, HCBT_CREATEWND, hwnd, (LPARAM)&cbtc);
        if (ret)
	{
	    TRACE(win, "CBT-hook returned 0\n");
	    USER_HEAP_FREE( hwnd );
	    return 0;
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
       return FALSE;

    /* Set the window menu */

    if ((wndPtr->dwStyle & (WS_CAPTION | WS_CHILD)) == WS_CAPTION )
    {
        if (cs->hMenu) SetMenu32(hwnd, cs->hMenu);
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
                    cs->hMenu = LoadMenu32A(cs->hInstance,PTR_SEG_TO_LIN(menuName));
                else
                    cs->hMenu = LoadMenu16(cs->hInstance,menuName);

                if (cs->hMenu) SetMenu32( hwnd, cs->hMenu );
            }
#endif
        }
    }
    else wndPtr->wIDmenu = (UINT32)cs->hMenu;

    /* Send the WM_CREATE message 
     * Perhaps we shouldn't allow width/height changes as well. 
     * See p327 in "Internals". 
     */

    maxPos.x = wndPtr->rectWindow.left; maxPos.y = wndPtr->rectWindow.top;

    localSend32 = unicode ? SendMessage32W : SendMessage32A;
    if( (*localSend32)( hwnd, WM_NCCREATE, 0, (LPARAM)cs) )
    {
        /* Insert the window in the linked list */

        WIN_LinkWindow( hwnd, hwndLinkAfter );

        WINPOS_SendNCCalcSize( hwnd, FALSE, &wndPtr->rectWindow,
                               NULL, NULL, 0, &wndPtr->rectClient );
        OffsetRect32(&wndPtr->rectWindow, maxPos.x - wndPtr->rectWindow.left,
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
                SendMessage32A( hwnd, WM_SIZE, SIZE_RESTORED,
                                MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
                                         wndPtr->rectClient.bottom-wndPtr->rectClient.top));
                SendMessage32A( hwnd, WM_MOVE, 0,
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
                swFlag = ((wndPtr->dwStyle & WS_CHILD) || GetActiveWindow32())
                    ? SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED
                    : SWP_NOZORDER | SWP_FRAMECHANGED;
                SetWindowPos32( hwnd, 0, newPos.left, newPos.top,
                                newPos.right, newPos.bottom, swFlag );
            }

	    if( wndPtr->dwStyle & WS_CHILD && !(wndPtr->dwExStyle & WS_EX_NOPARENTNOTIFY) )
	    {
		/* Notify the parent window only */

		SendMessage32A( wndPtr->parent->hwndSelf, WM_PARENTNOTIFY,
				MAKEWPARAM(WM_CREATE, wndPtr->wIDmenu), (LPARAM)hwnd );
		if( !IsWindow32(hwnd) ) return 0;
	    }

            if (cs->style & WS_VISIBLE) ShowWindow32( hwnd, SW_SHOW );

            /* Call WH_SHELL hook */

            if (!(wndPtr->dwStyle & WS_CHILD) && !wndPtr->owner)
                HOOK_CallHooks16( WH_SHELL, HSHELL_WINDOWCREATED, hwnd, 0 );

            TRACE(win, "created window %04x\n", hwnd);
            return hwnd;
        }
        WIN_UnlinkWindow( hwnd );
    }

    /* Abort window creation */

    WARN(win, "aborted by WM_xxCREATE!\n");
    WIN_DestroyWindow( wndPtr );
    return 0;
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
    CREATESTRUCT32A cs;

    /* Find the class atom */

    if (!(classAtom = GlobalFindAtom32A( className )))
    {
        fprintf( stderr, "CreateWindowEx16: bad class name " );
        if (!HIWORD(className)) fprintf( stderr, "%04x\n", LOWORD(className) );
        else fprintf( stderr, "'%s'\n", className );
        return 0;
    }

    /* Fix the coordinates */

    cs.x  = (x == CW_USEDEFAULT16) ? CW_USEDEFAULT32 : (INT32)x;
    cs.y  = (y == CW_USEDEFAULT16) ? CW_USEDEFAULT32 : (INT32)y;
    cs.cx = (width == CW_USEDEFAULT16) ? CW_USEDEFAULT32 : (INT32)width;
    cs.cy = (height == CW_USEDEFAULT16) ? CW_USEDEFAULT32 : (INT32)height;

    /* Create the window */

    cs.lpCreateParams = data;
    cs.hInstance      = (HINSTANCE32)instance;
    cs.hMenu          = (HMENU32)menu;
    cs.hwndParent     = (HWND32)parent;
    cs.style          = style;
    cs.lpszName       = windowName;
    cs.lpszClass      = className;
    cs.dwExStyle      = exStyle;
    return WIN_CreateWindowEx( &cs, classAtom, FALSE, FALSE );
}


/***********************************************************************
 *           CreateWindowEx32A   (USER32.83)
 */
HWND32 WINAPI CreateWindowEx32A( DWORD exStyle, LPCSTR className,
                                 LPCSTR windowName, DWORD style, INT32 x,
                                 INT32 y, INT32 width, INT32 height,
                                 HWND32 parent, HMENU32 menu,
                                 HINSTANCE32 instance, LPVOID data )
{
    ATOM classAtom;
    CREATESTRUCT32A cs;

    if(exStyle & WS_EX_MDICHILD)
        return MDI_CreateMDIWindow32A(className, windowName, style, x, y, width, height, parent, instance, data);
    /* Find the class atom */

    if (!(classAtom = GlobalFindAtom32A( className )))
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
HWND32 WINAPI CreateWindowEx32W( DWORD exStyle, LPCWSTR className,
                                 LPCWSTR windowName, DWORD style, INT32 x,
                                 INT32 y, INT32 width, INT32 height,
                                 HWND32 parent, HMENU32 menu,
                                 HINSTANCE32 instance, LPVOID data )
{
    ATOM classAtom;
    CREATESTRUCT32W cs;

    if(exStyle & WS_EX_MDICHILD)
        return MDI_CreateMDIWindow32W(className, windowName, style, x, y, width, height, parent, instance, data);

    /* Find the class atom */

    if (!(classAtom = GlobalFindAtom32W( className )))
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
    return WIN_CreateWindowEx( (CREATESTRUCT32A *)&cs, classAtom, TRUE, TRUE );
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

    if( CARET_GetHwnd() == pWnd->hwndSelf ) DestroyCaret32();
    if( !pWnd->window ) CLIPBOARD_GetDriver()->pResetOwner( pWnd ); 
  
    SendMessage32A( pWnd->hwndSelf, WM_DESTROY, 0, 0);

    if( IsWindow32(pWnd->hwndSelf) )
    {
	WND* pChild = pWnd->child;
	while( pChild )
	{
	    WIN_SendDestroyMsg( pChild );
	    pChild = pChild->next;
	}
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
    return DestroyWindow32(hwnd);
}


/***********************************************************************
 *           DestroyWindow32   (USER32.135)
 */
BOOL32 WINAPI DestroyWindow32( HWND32 hwnd )
{
    WND * wndPtr;

    TRACE(win, "(%04x)\n", hwnd);
    
      /* Initialization */

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (wndPtr == pWndDesktop) return FALSE; /* Can't destroy desktop */

      /* Call hooks */

    if( HOOK_CallHooks16( WH_CBT, HCBT_DESTROYWND, hwnd, 0L) )
        return FALSE;

    if (!(wndPtr->dwStyle & WS_CHILD) && !wndPtr->owner)
    {
        HOOK_CallHooks16( WH_SHELL, HSHELL_WINDOWDESTROYED, hwnd, 0L );
        /* FIXME: clean up palette - see "Internals" p.352 */
    }

    if( !QUEUE_IsExitingQueue(wndPtr->hmemTaskQ) )
	if( wndPtr->dwStyle & WS_CHILD && !(wndPtr->dwExStyle & WS_EX_NOPARENTNOTIFY) )
	{
	    /* Notify the parent window only */
	    SendMessage32A( wndPtr->parent->hwndSelf, WM_PARENTNOTIFY,
			    MAKEWPARAM(WM_DESTROY, wndPtr->wIDmenu), (LPARAM)hwnd );
	    if( !IsWindow32(hwnd) ) return TRUE;
	}

    if( wndPtr->window ) CLIPBOARD_GetDriver()->pResetOwner( wndPtr ); /* before the window is unmapped */

      /* Hide the window */

    if (wndPtr->dwStyle & WS_VISIBLE)
    {
        SetWindowPos32( hwnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW |
		        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|
		        ((QUEUE_IsExitingQueue(wndPtr->hmemTaskQ))?SWP_DEFERERASE:0) );
	if (!IsWindow32(hwnd)) return TRUE;
    }

      /* Recursively destroy owned windows */

    if( !(wndPtr->dwStyle & WS_CHILD) )
    {
      /* make sure top menu popup doesn't get destroyed */
      MENU_PatchResidentPopup( (HQUEUE16)0xFFFF, wndPtr );

      for (;;)
      {
        WND *siblingPtr = wndPtr->parent->child;  /* First sibling */
        while (siblingPtr)
        {
            if (siblingPtr->owner == wndPtr)
	    {
               if (siblingPtr->hmemTaskQ == wndPtr->hmemTaskQ)
                   break;
               else 
                   siblingPtr->owner = NULL;
	    }
            siblingPtr = siblingPtr->next;
        }
        if (siblingPtr) DestroyWindow32( siblingPtr->hwndSelf );
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
    if (!IsWindow32(hwnd)) return TRUE;

      /* Unlink now so we won't bother with the children later on */

    if( wndPtr->parent ) WIN_UnlinkWindow(hwnd);

      /* Destroy the window storage */

    WIN_DestroyWindow( wndPtr );
    return TRUE;
}


/***********************************************************************
 *           CloseWindow16   (USER.43)
 */
BOOL16 WINAPI CloseWindow16( HWND16 hwnd )
{
    return CloseWindow32( hwnd );
}

 
/***********************************************************************
 *           CloseWindow32   (USER32.56)
 */
BOOL32 WINAPI CloseWindow32( HWND32 hwnd )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr || (wndPtr->dwStyle & WS_CHILD)) return FALSE;
    ShowWindow32( hwnd, SW_MINIMIZE );
    return TRUE;
}

 
/***********************************************************************
 *           OpenIcon16   (USER.44)
 */
BOOL16 WINAPI OpenIcon16( HWND16 hwnd )
{
    return OpenIcon32( hwnd );
}


/***********************************************************************
 *           OpenIcon32   (USER32.410)
 */
BOOL32 WINAPI OpenIcon32( HWND32 hwnd )
{
    if (!IsIconic32( hwnd )) return FALSE;
    ShowWindow32( hwnd, SW_SHOWNORMAL );
    return TRUE;
}


/***********************************************************************
 *           WIN_FindWindow
 *
 * Implementation of FindWindow() and FindWindowEx().
 */
static HWND32 WIN_FindWindow( HWND32 parent, HWND32 child, ATOM className,
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
            if (!(pClass = CLASS_FindClassByAtom( className, GetExePtr(pWnd->hInstance))))
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
HWND32 WINAPI FindWindow32A( LPCSTR className, LPCSTR title )
{
    HWND32 ret = FindWindowEx32A( 0, 0, className, title );
    if (!ret) SetLastError (ERROR_CANNOT_FIND_WND_CLASS);
    return ret;
}


/***********************************************************************
 *           FindWindowEx32A   (USER32.199)
 */
HWND32 WINAPI FindWindowEx32A( HWND32 parent, HWND32 child,
                               LPCSTR className, LPCSTR title )
{
    ATOM atom = 0;

    if (className)
    {
        /* If the atom doesn't exist, then no class */
        /* with this name exists either. */
        if (!(atom = GlobalFindAtom32A( className ))) 
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
HWND32 WINAPI FindWindowEx32W( HWND32 parent, HWND32 child,
                               LPCWSTR className, LPCWSTR title )
{
    ATOM atom = 0;
    char *buffer;
    HWND32 hwnd;

    if (className)
    {
        /* If the atom doesn't exist, then no class */
        /* with this name exists either. */
        if (!(atom = GlobalFindAtom32W( className )))
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
HWND32 WINAPI FindWindow32W( LPCWSTR className, LPCWSTR title )
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
 *           GetDesktopWindow16   (USER.286)
 */
HWND16 WINAPI GetDesktopWindow16(void)
{
    return (HWND16)pWndDesktop->hwndSelf;
}


/**********************************************************************
 *           GetDesktopWindow32   (USER32.232)
 */
HWND32 WINAPI GetDesktopWindow32(void)
{
    return pWndDesktop->hwndSelf;
}


/**********************************************************************
 *           GetDesktopHwnd   (USER.278)
 *
 * Exactly the same thing as GetDesktopWindow(), but not documented.
 * Don't ask me why...
 */
HWND16 WINAPI GetDesktopHwnd(void)
{
    return (HWND16)pWndDesktop->hwndSelf;
}


/*******************************************************************
 *           EnableWindow16   (USER.34)
 */
BOOL16 WINAPI EnableWindow16( HWND16 hwnd, BOOL16 enable )
{
    return EnableWindow32( hwnd, enable );
}


/*******************************************************************
 *           EnableWindow32   (USER32.172)
 */
BOOL32 WINAPI EnableWindow32( HWND32 hwnd, BOOL32 enable )
{
    WND *wndPtr;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (enable && (wndPtr->dwStyle & WS_DISABLED))
    {
	  /* Enable window */
	wndPtr->dwStyle &= ~WS_DISABLED;
	SendMessage32A( hwnd, WM_ENABLE, TRUE, 0 );
	return TRUE;
    }
    else if (!enable && !(wndPtr->dwStyle & WS_DISABLED))
    {
	  /* Disable window */
	wndPtr->dwStyle |= WS_DISABLED;
	if ((hwnd == GetFocus32()) || IsChild32( hwnd, GetFocus32() ))
	    SetFocus32( 0 );  /* A disabled window can't have the focus */
	if ((hwnd == GetCapture32()) || IsChild32( hwnd, GetCapture32() ))
	    ReleaseCapture();  /* A disabled window can't capture the mouse */
	SendMessage32A( hwnd, WM_ENABLE, FALSE, 0 );
	return FALSE;
    }
    return ((wndPtr->dwStyle & WS_DISABLED) != 0);
}


/***********************************************************************
 *           IsWindowEnabled16   (USER.35)
 */ 
BOOL16 WINAPI IsWindowEnabled16(HWND16 hWnd)
{
    return IsWindowEnabled32(hWnd);
}


/***********************************************************************
 *           IsWindowEnabled32   (USER32.349)
 */ 
BOOL32 WINAPI IsWindowEnabled32(HWND32 hWnd)
{
    WND * wndPtr; 

    if (!(wndPtr = WIN_FindWndPtr(hWnd))) return FALSE;
    return !(wndPtr->dwStyle & WS_DISABLED);
}


/***********************************************************************
 *           IsWindowUnicode   (USER32.350)
 */
BOOL32 WINAPI IsWindowUnicode( HWND32 hwnd )
{
    WND * wndPtr; 

    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return FALSE;
    return (WINPROC_GetProcType( wndPtr->winproc ) == WIN_PROC_32W);
}


/**********************************************************************
 *	     GetWindowWord16    (USER.133)
 */
WORD WINAPI GetWindowWord16( HWND16 hwnd, INT16 offset )
{
    return GetWindowWord32( hwnd, offset );
}


/**********************************************************************
 *	     GetWindowWord32    (USER32.314)
 */
WORD WINAPI GetWindowWord32( HWND32 hwnd, INT32 offset )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(WORD) > wndPtr->class->cbWndExtra)
        {
            WARN( win, "Invalid offset %d\n", offset );
            return 0;
        }
        return *(WORD *)(((char *)wndPtr->wExtra) + offset);
    }
    switch(offset)
    {
    case GWW_ID:         
    	if (HIWORD(wndPtr->wIDmenu))
    		WARN( win,"GWW_ID: discards high bits of 0x%08x!\n",
                    wndPtr->wIDmenu);
    	return (WORD)wndPtr->wIDmenu;
    case GWW_HWNDPARENT: 
    	return wndPtr->parent ?
		wndPtr->parent->hwndSelf : (
			wndPtr->owner ?
			 wndPtr->owner->hwndSelf : 
			 0);
    case GWW_HINSTANCE:  
    	if (HIWORD(wndPtr->hInstance))
    		WARN(win,"GWW_HINSTANCE: discards high bits of 0x%08x!\n",
                    wndPtr->hInstance);
   	return (WORD)wndPtr->hInstance;
    default:
        WARN( win, "Invalid offset %d\n", offset );
        return 0;
    }
}


/**********************************************************************
 *	     WIN_GetWindowInstance
 */
HINSTANCE32 WIN_GetWindowInstance( HWND32 hwnd )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return (HINSTANCE32)0;
    return wndPtr->hInstance;
}


/**********************************************************************
 *	     SetWindowWord16    (USER.134)
 */
WORD WINAPI SetWindowWord16( HWND16 hwnd, INT16 offset, WORD newval )
{
    return SetWindowWord32( hwnd, offset, newval );
}


/**********************************************************************
 *	     SetWindowWord32    (USER32.524)
 */
WORD WINAPI SetWindowWord32( HWND32 hwnd, INT32 offset, WORD newval )
{
    WORD *ptr, retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(WORD) > wndPtr->class->cbWndExtra)
        {
            WARN( win, "Invalid offset %d\n", offset );
            return 0;
        }
        ptr = (WORD *)(((char *)wndPtr->wExtra) + offset);
    }
    else switch(offset)
    {
	case GWW_ID:        ptr = (WORD *)&wndPtr->wIDmenu; break;
	case GWW_HINSTANCE: ptr = (WORD *)&wndPtr->hInstance; break;
	case GWW_HWNDPARENT: return SetParent32( hwnd, newval );
	default:
            WARN( win, "Invalid offset %d\n", offset );
            return 0;
    }
    retval = *ptr;
    *ptr = newval;
    return retval;
}


/**********************************************************************
 *	     WIN_GetWindowLong
 *
 * Helper function for GetWindowLong().
 */
static LONG WIN_GetWindowLong( HWND32 hwnd, INT32 offset, WINDOWPROCTYPE type )
{
    LONG retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(LONG) > wndPtr->class->cbWndExtra)
        {
            WARN( win, "Invalid offset %d\n", offset );
            return 0;
        }
        retval = *(LONG *)(((char *)wndPtr->wExtra) + offset);
        /* Special case for dialog window procedure */
        if ((offset == DWL_DLGPROC) && (wndPtr->flags & WIN_ISDIALOG))
            return (LONG)WINPROC_GetProc( (HWINDOWPROC)retval, type );
        return retval;
    }
    switch(offset)
    {
        case GWL_USERDATA:   return wndPtr->userdata;
        case GWL_STYLE:      return wndPtr->dwStyle;
        case GWL_EXSTYLE:    return wndPtr->dwExStyle;
        case GWL_ID:         return (LONG)wndPtr->wIDmenu;
        case GWL_WNDPROC:    return (LONG)WINPROC_GetProc( wndPtr->winproc,
                                                           type );
        case GWL_HWNDPARENT: return wndPtr->parent ?
                                        (HWND32)wndPtr->parent->hwndSelf : 0;
        case GWL_HINSTANCE:  return wndPtr->hInstance;
        default:
            WARN( win, "Unknown offset %d\n", offset );
    }
    return 0;
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
static LONG WIN_SetWindowLong( HWND32 hwnd, INT32 offset, LONG newval,
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

            return 0;
        }
        ptr = (LONG *)(((char *)wndPtr->wExtra) + offset);
        /* Special case for dialog window procedure */
        if ((offset == DWL_DLGPROC) && (wndPtr->flags & WIN_ISDIALOG))
        {
            retval = (LONG)WINPROC_GetProc( (HWINDOWPROC)*ptr, type );
            WINPROC_SetProc( (HWINDOWPROC *)ptr, (WNDPROC16)newval, 
                             type, WIN_PROC_WINDOW );
            return retval;
        }
    }
    else switch(offset)
    {
	case GWL_ID:
		ptr = (DWORD*)&wndPtr->wIDmenu;
		break;
	case GWL_HINSTANCE:
		return SetWindowWord32( hwnd, offset, newval );
	case GWL_WNDPROC:
					retval = (LONG)WINPROC_GetProc( wndPtr->winproc, type );
		WINPROC_SetProc( &wndPtr->winproc, (WNDPROC16)newval, 
						type, WIN_PROC_WINDOW );
		return retval;
	case GWL_STYLE:
	       	style.styleOld = wndPtr->dwStyle;
		newval &= ~(WS_VISIBLE | WS_CHILD);	/* Some bits can't be changed this way */
		style.styleNew = newval | (style.styleOld & (WS_VISIBLE | WS_CHILD));

		if (wndPtr->flags & WIN_ISWIN32)
			SendMessage32A(hwnd,WM_STYLECHANGING,GWL_STYLE,(LPARAM)&style);
		wndPtr->dwStyle = style.styleNew;
		if (wndPtr->flags & WIN_ISWIN32)
			SendMessage32A(hwnd,WM_STYLECHANGED,GWL_STYLE,(LPARAM)&style);
		return style.styleOld;
		    
        case GWL_USERDATA: 
		ptr = &wndPtr->userdata; 
		break;
        case GWL_EXSTYLE:  
	        style.styleOld = wndPtr->dwExStyle;
		style.styleNew = newval;
		if (wndPtr->flags & WIN_ISWIN32)
			SendMessage32A(hwnd,WM_STYLECHANGING,GWL_EXSTYLE,(LPARAM)&style);
		wndPtr->dwExStyle = newval;
		if (wndPtr->flags & WIN_ISWIN32)
			SendMessage32A(hwnd,WM_STYLECHANGED,GWL_EXSTYLE,(LPARAM)&style);
		return style.styleOld;

	default:
            WARN( win, "Invalid offset %d\n", offset );

            /* Don't think this is right error but it should do */
            SetLastError( ERROR_OUTOFMEMORY );

            return 0;
    }
    retval = *ptr;
    *ptr = newval;
    return retval;
}


/**********************************************************************
 *	     GetWindowLong16    (USER.135)
 */
LONG WINAPI GetWindowLong16( HWND16 hwnd, INT16 offset )
{
    return WIN_GetWindowLong( (HWND32)hwnd, offset, WIN_PROC_16 );
}


/**********************************************************************
 *	     GetWindowLong32A    (USER32.305)
 */
LONG WINAPI GetWindowLong32A( HWND32 hwnd, INT32 offset )
{
    return WIN_GetWindowLong( hwnd, offset, WIN_PROC_32A );
}


/**********************************************************************
 *	     GetWindowLong32W    (USER32.306)
 */
LONG WINAPI GetWindowLong32W( HWND32 hwnd, INT32 offset )
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
LONG WINAPI SetWindowLong32A( HWND32 hwnd, INT32 offset, LONG newval )
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
 * BUGS
 *
 * GWL_STYLE does not dispatch WM_STYLE_... messages.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win32 
 *
 */
LONG WINAPI SetWindowLong32W( 
    HWND32 hwnd, /* window to alter */
    INT32 offset, /* offset, in bytes, of location to alter */
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
INT32 WINAPI GetWindowText32A( HWND32 hwnd, LPSTR lpString, INT32 nMaxCount )
{
    return (INT32)SendMessage32A( hwnd, WM_GETTEXT, nMaxCount,
                                  (LPARAM)lpString );
}

/*******************************************************************
 *	     InternalGetWindowText    (USER32.326)
 */
INT32 WINAPI InternalGetWindowText(HWND32 hwnd,LPWSTR lpString,INT32 nMaxCount )
{
    FIXME(win,"(0x%08x,%p,0x%x),stub!\n",hwnd,lpString,nMaxCount);
    return GetWindowText32W(hwnd,lpString,nMaxCount);
}


/*******************************************************************
 *	     GetWindowText32W    (USER32.312)
 */
INT32 WINAPI GetWindowText32W( HWND32 hwnd, LPWSTR lpString, INT32 nMaxCount )
{
    return (INT32)SendMessage32W( hwnd, WM_GETTEXT, nMaxCount,
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
BOOL32 WINAPI SetWindowText32A( HWND32 hwnd, LPCSTR lpString )
{
    return (BOOL32)SendMessage32A( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *	     SetWindowText32W    (USER32.523)
 */
BOOL32 WINAPI SetWindowText32W( HWND32 hwnd, LPCWSTR lpString )
{
    return (BOOL32)SendMessage32W( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
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
INT32 WINAPI GetWindowTextLength32A( HWND32 hwnd )
{
    return SendMessage32A( hwnd, WM_GETTEXTLENGTH, 0, 0 );
}

/*******************************************************************
 *         GetWindowTextLength32W   (USER32.311)
 */
INT32 WINAPI GetWindowTextLength32W( HWND32 hwnd )
{
    return SendMessage32W( hwnd, WM_GETTEXTLENGTH, 0, 0 );
}


/*******************************************************************
 *         IsWindow16   (USER.47)
 */
BOOL16 WINAPI IsWindow16( HWND16 hwnd )
{
    return IsWindow32( hwnd );
}

void WINAPI WIN16_IsWindow16( CONTEXT *context )
{
    WORD *stack = PTR_SEG_OFF_TO_LIN(SS_reg(context), SP_reg(context));
    HWND16 hwnd = (HWND16)stack[2];

    AX_reg(context) = IsWindow32( hwnd );
    ES_reg(context) = USER_HeapSel;
}


/*******************************************************************
 *         IsWindow32   (USER32.348)
 */
BOOL32 WINAPI IsWindow32( HWND32 hwnd )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    return ((wndPtr != NULL) && (wndPtr->dwMagic == WND_MAGIC));
}


/*****************************************************************
 *         GetParent16   (USER.46)
 */
HWND16 WINAPI GetParent16( HWND16 hwnd )
{
    return (HWND16)GetParent32( hwnd );
}


/*****************************************************************
 *         GetParent32   (USER32.278)
 */
HWND32 WINAPI GetParent32( HWND32 hwnd )
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    if ((!wndPtr) || (!(wndPtr->dwStyle & (WS_POPUP|WS_CHILD)))) return 0;
    wndPtr = (wndPtr->dwStyle & WS_CHILD) ? wndPtr->parent : wndPtr->owner;
    return wndPtr ? wndPtr->hwndSelf : 0;
}

/*****************************************************************
 *         WIN_GetTopParent
 *
 * Get the top-level parent for a child window.
 */
WND* WIN_GetTopParentPtr( WND* pWnd )
{
    while( pWnd && (pWnd->dwStyle & WS_CHILD)) pWnd = pWnd->parent;
    return pWnd;
}

/*****************************************************************
 *         WIN_GetTopParent
 *
 * Get the top-level parent for a child window.
 */
HWND32 WIN_GetTopParent( HWND32 hwnd )
{
    WND *wndPtr = WIN_GetTopParentPtr ( WIN_FindWndPtr( hwnd ) );
    return wndPtr ? wndPtr->hwndSelf : 0;
}


/*****************************************************************
 *         SetParent16   (USER.233)
 */
HWND16 WINAPI SetParent16( HWND16 hwndChild, HWND16 hwndNewParent )
{
    return SetParent32( hwndChild, hwndNewParent );
}


/*****************************************************************
 *         SetParent32   (USER32.495)
 */
HWND32 WINAPI SetParent32( HWND32 hwndChild, HWND32 hwndNewParent )
{
  WND *wndPtr = WIN_FindWndPtr( hwndChild );
  WND *pWndNewParent = 
    (hwndNewParent) ? WIN_FindWndPtr( hwndNewParent ) : pWndDesktop;
  WND *pWndOldParent =
    (wndPtr)?(*wndPtr->pDriver->pSetParent)(wndPtr, pWndNewParent):NULL;

  return pWndOldParent?pWndOldParent->hwndSelf:0;
}

/*******************************************************************
 *         IsChild16    (USER.48)
 */
BOOL16 WINAPI IsChild16( HWND16 parent, HWND16 child )
{
    return IsChild32(parent,child);
}


/*******************************************************************
 *         IsChild32    (USER32.339)
 */
BOOL32 WINAPI IsChild32( HWND32 parent, HWND32 child )
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
 *           IsWindowVisible16   (USER.49)
 */
BOOL16 WINAPI IsWindowVisible16( HWND16 hwnd )
{
    return IsWindowVisible32(hwnd);
}


/***********************************************************************
 *           IsWindowVisible32   (USER32.351)
 */
BOOL32 WINAPI IsWindowVisible32( HWND32 hwnd )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    while (wndPtr && (wndPtr->dwStyle & WS_CHILD))
    {
        if (!(wndPtr->dwStyle & WS_VISIBLE)) return FALSE;
        wndPtr = wndPtr->parent;
    }
    return (wndPtr && (wndPtr->dwStyle & WS_VISIBLE));
}


/***********************************************************************
 *           WIN_IsWindowDrawable
 *
 * hwnd is drawable when it is visible, all parents are not
 * minimized, and it is itself not minimized unless we are
 * trying to draw its default class icon.
 */
BOOL32 WIN_IsWindowDrawable( WND* wnd, BOOL32 icon )
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
    return GetTopWindow32(hwnd);
}


/*******************************************************************
 *         GetTopWindow32    (USER.229)
 */
HWND32 WINAPI GetTopWindow32( HWND32 hwnd )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (wndPtr && wndPtr->child) return wndPtr->child->hwndSelf;
    else return 0;
}


/*******************************************************************
 *         GetWindow16    (USER.262)
 */
HWND16 WINAPI GetWindow16( HWND16 hwnd, WORD rel )
{
    return GetWindow32( hwnd,rel );
}


/*******************************************************************
 *         GetWindow32    (USER32.302)
 */
HWND32 WINAPI GetWindow32( HWND32 hwnd, WORD rel )
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
    ShowOwnedPopups32( owner, fShow );
}


/*******************************************************************
 *         ShowOwnedPopups32  (USER32.531)
 */
BOOL32 WINAPI ShowOwnedPopups32( HWND32 owner, BOOL32 fShow )
{
    WND *pWnd = pWndDesktop->child;
    while (pWnd)
    {
        if (pWnd->owner && (pWnd->owner->hwndSelf == owner) &&
            (pWnd->dwStyle & WS_POPUP))
            ShowWindow32( pWnd->hwndSelf, fShow ? SW_SHOW : SW_HIDE );
        pWnd = pWnd->next;
    }
    return TRUE;
}


/*******************************************************************
 *         GetLastActivePopup16   (USER.287)
 */
HWND16 WINAPI GetLastActivePopup16( HWND16 hwnd )
{
    return GetLastActivePopup32( hwnd );
}

/*******************************************************************
 *         GetLastActivePopup32   (USER32.256)
 */
HWND32 WINAPI GetLastActivePopup32( HWND32 hwnd )
{
    WND *wndPtr;
    wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr == NULL) return hwnd;
    return wndPtr->hwndLastActive;
}


/*******************************************************************
 *           WIN_BuildWinArray
 *
 * Build an array of pointers to the children of a given window.
 * The array must be freed with HeapFree(SystemHeap). Return NULL
 * when no windows are found.
 */
WND **WIN_BuildWinArray( WND *wndPtr, UINT32 bwaFlags, UINT32* pTotal )
{
    WND **list, **ppWnd;
    WND *pWnd;
    UINT32 count, skipOwned, skipHidden;
    DWORD skipFlags;

    skipHidden = bwaFlags & BWA_SKIPHIDDEN;
    skipOwned = bwaFlags & BWA_SKIPOWNED;
    skipFlags = (bwaFlags & BWA_SKIPDISABLED) ? WS_DISABLED : 0;
    if( bwaFlags & BWA_SKIPICONIC ) skipFlags |= WS_MINIMIZE;

    /* First count the windows */

    if (!wndPtr) wndPtr = pWndDesktop;
    for (pWnd = wndPtr->child, count = 0; pWnd; pWnd = pWnd->next) 
    {
	if( (pWnd->dwStyle & skipFlags) || (skipOwned && pWnd->owner) ) continue;
	if( !skipHidden || pWnd->dwStyle & WS_VISIBLE ) count++;
    }

    if( count )
    {
	/* Now build the list of all windows */

	if ((list = (WND **)HeapAlloc( SystemHeap, 0, sizeof(WND *) * (count + 1))))
	{
	    for (pWnd = wndPtr->child, ppWnd = list, count = 0; pWnd; pWnd = pWnd->next)
	    {
		if( (pWnd->dwStyle & skipFlags) || (skipOwned && pWnd->owner) ) continue;
		if( !skipHidden || pWnd->dwStyle & WS_VISIBLE )
		{
		   *ppWnd++ = pWnd;
		    count++;
		}
	    }
	   *ppWnd = NULL;
	}
	else count = 0;
    } else list = NULL;

    if( pTotal ) *pTotal = count;
    return list;
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

    if (!(list = WIN_BuildWinArray( pWndDesktop, 0, NULL ))) return FALSE;

    /* Now call the callback function for every window */

    for (ppWnd = list; *ppWnd; ppWnd++)
    {
        /* Make sure that the window still exists */
        if (!IsWindow32((*ppWnd)->hwndSelf)) continue;
        if (!lpEnumFunc( (*ppWnd)->hwndSelf, lParam )) break;
    }
    HeapFree( SystemHeap, 0, list );
    return TRUE;
}


/*******************************************************************
 *           EnumWindows32   (USER32.193)
 */
BOOL32 WINAPI EnumWindows32( WNDENUMPROC32 lpEnumFunc, LPARAM lParam )
{
    return (BOOL32)EnumWindows16( (WNDENUMPROC16)lpEnumFunc, lParam );
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

    if (!(list = WIN_BuildWinArray( pWndDesktop, 0, NULL ))) return FALSE;

    /* Now call the callback function for every window */

    for (ppWnd = list; *ppWnd; ppWnd++)
    {
        /* Make sure that the window still exists */
        if (!IsWindow32((*ppWnd)->hwndSelf)) continue;
        if (QUEUE_GetQueueTask((*ppWnd)->hmemTaskQ) != hTask) continue;
        if (!func( (*ppWnd)->hwndSelf, lParam )) break;
    }
    HeapFree( SystemHeap, 0, list );
    return TRUE;
}


/**********************************************************************
 *           EnumThreadWindows   (USER32.190)
 */
BOOL32 WINAPI EnumThreadWindows( DWORD id, WNDENUMPROC32 func, LPARAM lParam )
{
    THDB	*tdb = THREAD_ID_TO_THDB(id);

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
        if (!IsWindow32((*ppWnd)->hwndSelf)) continue;
        /* Build children list first */
        childList = WIN_BuildWinArray( *ppWnd, BWA_SKIPOWNED, NULL );
        ret = func( (*ppWnd)->hwndSelf, lParam );
        if (childList)
        {
            if (ret) ret = WIN_EnumChildWindows( childList, func, lParam );
            HeapFree( SystemHeap, 0, childList );
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
    if (!(list = WIN_BuildWinArray( pParent, BWA_SKIPOWNED, NULL ))) return FALSE;
    WIN_EnumChildWindows( list, func, lParam );
    HeapFree( SystemHeap, 0, list );
    return TRUE;
}


/**********************************************************************
 *           EnumChildWindows32   (USER32.178)
 */
BOOL32 WINAPI EnumChildWindows32( HWND32 parent, WNDENUMPROC32 func,
                                  LPARAM lParam )
{
    return (BOOL32)EnumChildWindows16( (HWND16)parent, (WNDENUMPROC16)func,
                                       lParam );
}


/*******************************************************************
 *           AnyPopup16   (USER.52)
 */
BOOL16 WINAPI AnyPopup16(void)
{
    return AnyPopup32();
}


/*******************************************************************
 *           AnyPopup32   (USER32.4)
 */
BOOL32 WINAPI AnyPopup32(void)
{
    WND *wndPtr;
    for (wndPtr = pWndDesktop->child; wndPtr; wndPtr = wndPtr->next)
        if (wndPtr->owner && (wndPtr->dwStyle & WS_VISIBLE)) return TRUE;
    return FALSE;
}


/*******************************************************************
 *            FlashWindow16   (USER.105)
 */
BOOL16 WINAPI FlashWindow16( HWND16 hWnd, BOOL16 bInvert )
{
    return FlashWindow32( hWnd, bInvert );
}


/*******************************************************************
 *            FlashWindow32   (USER32.202)
 */
BOOL32 WINAPI FlashWindow32( HWND32 hWnd, BOOL32 bInvert )
{
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    TRACE(win,"%04x\n", hWnd);

    if (!wndPtr) return FALSE;

    if (wndPtr->dwStyle & WS_MINIMIZE)
    {
        if (bInvert && !(wndPtr->flags & WIN_NCACTIVATED))
        {
            HDC32 hDC = GetDC32(hWnd);
            
            if (!SendMessage16( hWnd, WM_ERASEBKGND, (WPARAM16)hDC, 0 ))
                wndPtr->flags |= WIN_NEEDS_ERASEBKGND;
            
            ReleaseDC32( hWnd, hDC );
            wndPtr->flags |= WIN_NCACTIVATED;
        }
        else
        {
            PAINT_RedrawWindow( hWnd, 0, 0, RDW_INVALIDATE | RDW_ERASE |
					  RDW_UPDATENOW | RDW_FRAME, 0 );
            wndPtr->flags &= ~WIN_NCACTIVATED;
        }
        return TRUE;
    }
    else
    {
        WPARAM16 wparam;
        if (bInvert) wparam = !(wndPtr->flags & WIN_NCACTIVATED);
        else wparam = (hWnd == GetActiveWindow32());

        SendMessage16( hWnd, WM_NCACTIVATE, wparam, (LPARAM)0 );
        return wparam;
    }
}


/*******************************************************************
 *           SetSysModalWindow16   (USER.188)
 */
HWND16 WINAPI SetSysModalWindow16( HWND16 hWnd )
{
    HWND32 hWndOldModal = hwndSysModal;
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
DWORD WINAPI GetWindowContextHelpId( HWND32 hwnd )
{
    WND *wnd = WIN_FindWndPtr( hwnd );
    if (!wnd) return 0;
    return wnd->helpContext;
}


/*******************************************************************
 *           SetWindowContextHelpId   (USER32.515)
 */
BOOL32 WINAPI SetWindowContextHelpId( HWND32 hwnd, DWORD id )
{
    WND *wnd = WIN_FindWndPtr( hwnd );
    if (!wnd) return FALSE;
    wnd->helpContext = id;
    return TRUE;
}


/*******************************************************************
 *			DRAG_QueryUpdate
 *
 * recursively find a child that contains spDragInfo->pt point 
 * and send WM_QUERYDROPOBJECT
 */
BOOL16 DRAG_QueryUpdate( HWND32 hQueryWnd, SEGPTR spDragInfo, BOOL32 bNoSend )
{
 BOOL16		wParam,bResult = 0;
 POINT32        pt;
 LPDRAGINFO	ptrDragInfo = (LPDRAGINFO) PTR_SEG_TO_LIN(spDragInfo);
 WND 	       *ptrQueryWnd = WIN_FindWndPtr(hQueryWnd),*ptrWnd;
 RECT32		tempRect;

 if( !ptrQueryWnd || !ptrDragInfo ) return 0;

 CONV_POINT16TO32( &ptrDragInfo->pt, &pt );

 GetWindowRect32(hQueryWnd,&tempRect); 

 if( !PtInRect32(&tempRect,pt) ||
     (ptrQueryWnd->dwStyle & WS_DISABLED) )
	return 0;

 if( !(ptrQueryWnd->dwStyle & WS_MINIMIZE) ) 
   {
     tempRect = ptrQueryWnd->rectClient;
     if(ptrQueryWnd->dwStyle & WS_CHILD)
        MapWindowPoints32( ptrQueryWnd->parent->hwndSelf, 0,
                           (LPPOINT32)&tempRect, 2 );

     if (PtInRect32( &tempRect, pt))
	{
	 wParam = 0;
         
	 for (ptrWnd = ptrQueryWnd->child; ptrWnd ;ptrWnd = ptrWnd->next)
             if( ptrWnd->dwStyle & WS_VISIBLE )
	     {
                 GetWindowRect32( ptrWnd->hwndSelf, &tempRect );
                 if (PtInRect32( &tempRect, pt )) break;
	     }

	 if(ptrWnd)
         {
	    TRACE(msg,"hwnd = %04x, %d %d - %d %d\n",
                        ptrWnd->hwndSelf, ptrWnd->rectWindow.left, ptrWnd->rectWindow.top,
			ptrWnd->rectWindow.right, ptrWnd->rectWindow.bottom );
            if( !(ptrWnd->dwStyle & WS_DISABLED) )
	        bResult = DRAG_QueryUpdate(ptrWnd->hwndSelf, spDragInfo, bNoSend);
         }

	 if(bResult) return bResult;
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

 return bResult;
}


/*******************************************************************
 *             DragDetect   (USER.465)
 */
BOOL16 WINAPI DragDetect16( HWND16 hWnd, POINT16 pt )
{
    POINT32 pt32;
    CONV_POINT16TO32( &pt, &pt32 );
    return DragDetect32( hWnd, pt32 );
}

/*******************************************************************
 *             DragDetect32   (USER32.151)
 */
BOOL32 WINAPI DragDetect32( HWND32 hWnd, POINT32 pt )
{
    MSG16 msg;
    RECT16  rect;

    rect.left = pt.x - wDragWidth;
    rect.right = pt.x + wDragWidth;

    rect.top = pt.y - wDragHeight;
    rect.bottom = pt.y + wDragHeight;

    SetCapture32(hWnd);

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

    if( !lpDragInfo || !spDragInfo ) return 0L;

    hBummer = LoadCursor16(0, IDC_BUMMER16);

    if( !hBummer || !wndPtr )
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

	hOldCursor = SetCursor32(hDragCursor);
    }

    lpDragInfo->hWnd   = hWnd;
    lpDragInfo->hScope = 0;
    lpDragInfo->wFlags = wObj;
    lpDragInfo->hList  = szList; /* near pointer! */
    lpDragInfo->hOfStruct = hOfStruct;
    lpDragInfo->l = 0L; 

    SetCapture32(hWnd);
    ShowCursor32( TRUE );

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
	    SetCursor32(hCurrentCursor);

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
    ShowCursor32( FALSE );

    if( hCursor )
    {
	SetCursor32( hOldCursor );
	if (hDragCursor) DestroyCursor32( hDragCursor );
    }

    if( hCurrentCursor != hBummer ) 
	msg.lParam = SendMessage16( lpDragInfo->hScope, WM_DROPOBJECT, 
				   (WPARAM16)hWnd, (LPARAM)spDragInfo );
    else
        msg.lParam = 0;
    GlobalFree16(hDragInfo);

    return (DWORD)(msg.lParam);
}
