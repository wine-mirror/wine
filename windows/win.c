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
#include "user.h"
#include "dce.h"
#include "sysmetrics.h"
#include "cursoricon.h"
#include "event.h"
#include "message.h"
#include "nonclient.h"
#include "winpos.h"
#include "color.h"
#include "shm_main_blk.h"
#include "dde_proc.h"
#include "callback.h"
#include "stddebug.h"
/* #define DEBUG_WIN  */ 
/* #define DEBUG_MENU */
#include "debug.h"

static HWND hwndDesktop  = 0;
static HWND hwndSysModal = 0;

static WORD wDragWidth = 4;
static WORD wDragHeight= 3;

extern HCURSOR CURSORICON_IconToCursor(HICON);

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
    return ptr;
}


/***********************************************************************
 *           WIN_GetXWindow
 *
 * Return the X window associated to a window.
 */
Window WIN_GetXWindow( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    while (wndPtr && !wndPtr->window)
    {
        wndPtr = WIN_FindWndPtr( wndPtr->hwndParent );
    }
    return wndPtr ? wndPtr->window : 0;
}


/***********************************************************************
 *           WIN_UnlinkWindow
 *
 * Remove a window from the siblings linked list.
 */
BOOL WIN_UnlinkWindow( HWND hwnd )
{    
    HWND * curWndPtr;
    WND *parentPtr, *wndPtr;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (!(parentPtr = WIN_FindWndPtr( wndPtr->hwndParent ))) return FALSE;

    curWndPtr = &parentPtr->hwndChild;

    while (*curWndPtr != hwnd)
    {
	WND * curPtr = WIN_FindWndPtr( *curWndPtr );
	curWndPtr = &curPtr->hwndNext;
    }
    *curWndPtr = wndPtr->hwndNext;
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
    HWND * hwndPtr = NULL;  /* pointer to hwnd to change */
    WND *wndPtr, *parentPtr;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (!(parentPtr = WIN_FindWndPtr( wndPtr->hwndParent ))) return FALSE;

    if ((hwndInsertAfter == HWND_TOP) || (hwndInsertAfter == HWND_BOTTOM))
    {
	hwndPtr = &parentPtr->hwndChild;  /* Point to first sibling hwnd */
	if (hwndInsertAfter == HWND_BOTTOM)  /* Find last sibling hwnd */
	    while (*hwndPtr)
	    {
		WND * nextPtr = WIN_FindWndPtr( *hwndPtr );
		hwndPtr = &nextPtr->hwndNext;
	    }
    }
    else  /* Normal case */
    {
	WND * afterPtr = WIN_FindWndPtr( hwndInsertAfter );
	if (afterPtr) hwndPtr = &afterPtr->hwndNext;
    }
    if (!hwndPtr) return FALSE;
    wndPtr->hwndNext = *hwndPtr;
    *hwndPtr = hwnd;
    return TRUE;
}


/***********************************************************************
 *           WIN_FindWinToRepaint
 *
 * Find a window that needs repaint.
 */
HWND WIN_FindWinToRepaint( HWND hwnd )
{
    WND * wndPtr;

      /* Note: the desktop window never gets WM_PAINT messages */
    if (!hwnd) hwnd = GetTopWindow( hwndDesktop );
    for ( ; hwnd != 0; hwnd = wndPtr->hwndNext )
    {
	if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
	dprintf_win( stddeb, "WIN_FindWinToRepaint: "NPFMT", style %08lx\n",
		     hwnd, wndPtr->dwStyle );
        if (!(wndPtr->dwStyle & WS_VISIBLE) || (wndPtr->flags & WIN_NO_REDRAW))
            continue;
        if ((wndPtr->dwStyle & WS_MINIMIZE) && (WIN_CLASS_INFO(wndPtr).hIcon))
            continue;
	if (wndPtr->hrgnUpdate || (wndPtr->flags & WIN_INTERNAL_PAINT))
	    return hwnd;
	if (wndPtr->hwndChild)
	{
	    HWND child;
	    if ((child = WIN_FindWinToRepaint( wndPtr->hwndChild )))
		return child;
	}
    }
    return 0;
}


/***********************************************************************
 *           WIN_SendParentNotify
 *
 * Send a WM_PARENTNOTIFY to all ancestors of the given window, unless
 * the window has the WS_EX_NOPARENTNOTIFY style.
 */
void WIN_SendParentNotify( HWND hwnd, WORD event, WORD idChild, LONG lValue )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    
    while (wndPtr && (wndPtr->dwStyle & WS_CHILD))
    {
        if (wndPtr->dwExStyle & WS_EX_NOPARENTNOTIFY) break;
#ifdef WINELIB32
	SendMessage( wndPtr->hwndParent, WM_PARENTNOTIFY, 
		     MAKEWPARAM(event,idChild),
		     (LPARAM)lValue );
#else
	SendMessage( wndPtr->hwndParent, WM_PARENTNOTIFY, event,
		     MAKELPARAM(LOWORD(lValue), idChild) );
#endif
        wndPtr = WIN_FindWndPtr( wndPtr->hwndParent );
    }
}


/***********************************************************************
 *           WIN_DestroyWindow
 *
 * Destroy storage associated to a window
 */
static void WIN_DestroyWindow( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    CLASS *classPtr = CLASS_FindClassPtr( wndPtr->hClass );

#ifdef CONFIG_IPC
    if (main_block)
	DDE_DestroyWindow(hwnd);
#endif  /* CONFIG_IPC */
	
    if (!wndPtr || !classPtr) return;
    WIN_UnlinkWindow( hwnd ); /* Remove the window from the linked list */
    wndPtr->dwMagic = 0;  /* Mark it as invalid */
    if ((wndPtr->hrgnUpdate) || (wndPtr->flags & WIN_INTERNAL_PAINT))
    {
        if (wndPtr->hrgnUpdate) DeleteObject( wndPtr->hrgnUpdate );
        MSG_DecPaintCount( wndPtr->hmemTaskQ );
    }
    if (!(wndPtr->dwStyle & WS_CHILD))
    {
        if (wndPtr->wIDmenu) DestroyMenu( (HMENU)wndPtr->wIDmenu );
    }
    if (wndPtr->hSysMenu) DestroyMenu( wndPtr->hSysMenu );
    if (wndPtr->window) XDestroyWindow( display, wndPtr->window );
    if (classPtr->wc.style & CS_OWNDC) DCE_FreeDCE( wndPtr->hdce );
    classPtr->cWindows--;
    USER_HEAP_FREE( hwnd );
}


/***********************************************************************
 *           WIN_CreateDesktopWindow
 *
 * Create the desktop window.
 */
BOOL WIN_CreateDesktopWindow(void)
{
    WND *wndPtr;
    HCLASS hclass;
    CLASS *classPtr;
    HDC hdc;

    if (!(hclass = CLASS_FindClassByName( DESKTOP_CLASS_ATOM, 0, &classPtr )))
	return FALSE;

    hwndDesktop = USER_HEAP_ALLOC( sizeof(WND)+classPtr->wc.cbWndExtra );
    if (!hwndDesktop) return FALSE;
    wndPtr = (WND *) USER_HEAP_LIN_ADDR( hwndDesktop );

    wndPtr->hwndNext          = 0;
    wndPtr->hwndChild         = 0;
    wndPtr->dwMagic           = WND_MAGIC;
    wndPtr->hwndParent        = 0;
    wndPtr->hwndOwner         = 0;
    wndPtr->hClass            = hclass;
    wndPtr->hInstance         = 0;
    wndPtr->rectWindow.left   = 0;
    wndPtr->rectWindow.top    = 0;
    wndPtr->rectWindow.right  = SYSMETRICS_CXSCREEN;
    wndPtr->rectWindow.bottom = SYSMETRICS_CYSCREEN;
    wndPtr->rectClient        = wndPtr->rectWindow;
    wndPtr->rectNormal        = wndPtr->rectWindow;
    wndPtr->ptIconPos.x       = -1;
    wndPtr->ptIconPos.y       = -1;
    wndPtr->ptMaxPos.x        = -1;
    wndPtr->ptMaxPos.y        = -1;
    wndPtr->hmemTaskQ         = 0;  /* Desktop does not belong to a task */
    wndPtr->hrgnUpdate        = 0;
    wndPtr->hwndLastActive    = hwndDesktop;
    wndPtr->lpfnWndProc       = classPtr->wc.lpfnWndProc;
    wndPtr->dwStyle           = WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    wndPtr->dwExStyle         = 0;
    wndPtr->hdce              = 0;
    wndPtr->hVScroll          = 0;
    wndPtr->hHScroll          = 0;
    wndPtr->wIDmenu           = 0;
    wndPtr->hText             = 0;
    wndPtr->flags             = 0;
    wndPtr->window            = rootWindow;
    wndPtr->hSysMenu          = 0;
    wndPtr->hProp	      = 0;
    EVENT_RegisterWindow( wndPtr->window, hwndDesktop );
    SendMessage( hwndDesktop, WM_NCCREATE, 0, 0 );
    if ((hdc = GetDC( hwndDesktop )) != 0)
    {
        SendMessage( hwndDesktop, WM_ERASEBKGND, (WPARAM)hdc, 0 );
        ReleaseDC( hwndDesktop, hdc );
    }
    return TRUE;
}


/***********************************************************************
 *           CreateWindow   (USER.41)
 */
HWND CreateWindow( SEGPTR className, SEGPTR windowName,
		   DWORD style, INT x, INT y, INT width, INT height,
		   HWND parent, HMENU menu, HANDLE instance, SEGPTR data ) 
{
    return CreateWindowEx( 0, className, windowName, style,
			   x, y, width, height, parent, menu, instance, data );
}


/***********************************************************************
 *           CreateWindowEx   (USER.452)
 */
HWND CreateWindowEx( DWORD exStyle, SEGPTR className, SEGPTR windowName,
		     DWORD style, INT x, INT y, INT width, INT height,
		     HWND parent, HMENU menu, HANDLE instance, SEGPTR data ) 
{
    HANDLE class, hwnd;
    CLASS *classPtr;
    WND *wndPtr;
    POINT maxSize, maxPos, minTrack, maxTrack;
    CREATESTRUCT createStruct;
    int wmcreate;
    XSetWindowAttributes win_attr;

    /* FIXME: windowName and className should be SEGPTRs */

    dprintf_win( stddeb, "CreateWindowEx: " );
    if (HIWORD(windowName))
	dprintf_win( stddeb, "'%s' ", (char *)PTR_SEG_TO_LIN(windowName) );
    else
	dprintf_win( stddeb, "%04x ", LOWORD(windowName) );
    if (HIWORD(className))
        dprintf_win( stddeb, "'%s' ", (char *)PTR_SEG_TO_LIN(className) );
    else
        dprintf_win( stddeb, "%04x ", LOWORD(className) );

    dprintf_win(stddeb, "%08lx %08lx %d,%d %dx%d "NPFMT" "NPFMT" "NPFMT" %08lx\n",
		exStyle, style, x, y, width, height,
		parent, menu, instance, (DWORD)data);

    if (x == CW_USEDEFAULT) x = y = 0;
    if (width == CW_USEDEFAULT)
    {
	width = 600;
	height = 400;
    }

      /* Find the parent and class */

    if (parent)
    {
	/* Make sure parent is valid */
        if (!IsWindow( parent )) {
	    dprintf_win(stddeb,"CreateWindowEx: Parent "NPFMT" is not a window\n", parent);
	    return 0;
	}
    }
    else 
    {
	if (style & WS_CHILD) {
	    dprintf_win(stddeb,"CreateWindowEx: no parent\n");
	    return 0;  /* WS_CHILD needs a parent */
	}
    }

    if (!(class = CLASS_FindClassByName( className, GetExePtr(instance),
                                         &classPtr )))
    {
        fprintf(stderr,"CreateWindow BAD CLASSNAME " );
        if (HIWORD(className)) fprintf( stderr, "'%s'\n", 
                                        (char *)PTR_SEG_TO_LIN(className) );
        else fprintf( stderr, "%04x\n", LOWORD(className) );
        return 0;
    }

      /* Correct the window style */

    if (!(style & (WS_POPUP | WS_CHILD)))  /* Overlapped window */
	style |= WS_CAPTION | WS_CLIPSIBLINGS;
    if (exStyle & WS_EX_DLGMODALFRAME) style &= ~WS_THICKFRAME;

      /* Create the window structure */

    hwnd = USER_HEAP_ALLOC( sizeof(WND)+classPtr->wc.cbWndExtra );
    if (!hwnd) {
	dprintf_win(stddeb,"CreateWindowEx: Out of memory\n");
	return 0;
    }

      /* Fill the structure */

    wndPtr = (WND *) USER_HEAP_LIN_ADDR( hwnd );
    wndPtr->hwndNext       = 0;
    wndPtr->hwndChild      = 0;
    wndPtr->window         = 0;
    wndPtr->dwMagic        = WND_MAGIC;
    wndPtr->hwndParent     = (style & WS_CHILD) ? parent : hwndDesktop;
    wndPtr->hwndOwner      = (style & WS_CHILD) ? 0 : WIN_GetTopParent(parent);
    wndPtr->hClass         = class;
    wndPtr->hInstance      = instance;
    wndPtr->ptIconPos.x    = -1;
    wndPtr->ptIconPos.y    = -1;
    wndPtr->ptMaxPos.x     = -1;
    wndPtr->ptMaxPos.y     = -1;
    wndPtr->hmemTaskQ      = GetTaskQueue(0);
    wndPtr->hrgnUpdate     = 0;
    wndPtr->hwndLastActive = hwnd;
    wndPtr->lpfnWndProc    = classPtr->wc.lpfnWndProc;
    wndPtr->dwStyle        = style & ~WS_VISIBLE;
    wndPtr->dwExStyle      = exStyle;
    wndPtr->wIDmenu        = 0;
    wndPtr->hText          = 0;
    wndPtr->flags          = 0;
    wndPtr->hVScroll       = 0;
    wndPtr->hHScroll       = 0;
    wndPtr->hSysMenu       = 0;
    wndPtr->hProp          = 0;

    if (classPtr->wc.cbWndExtra)
	memset( wndPtr->wExtra, 0, classPtr->wc.cbWndExtra );
    classPtr->cWindows++;

      /* Get class or window DC if needed */

    if (classPtr->wc.style & CS_OWNDC)
        wndPtr->hdce = DCE_AllocDCE( DCE_WINDOW_DC );
    else if (classPtr->wc.style & CS_CLASSDC)
        wndPtr->hdce = classPtr->hdce;
    else
        wndPtr->hdce = 0;

      /* Insert the window in the linked list */

    WIN_LinkWindow( hwnd, HWND_TOP );

      /* Send the WM_GETMINMAXINFO message and fix the size if needed */

    NC_GetMinMaxInfo( hwnd, &maxSize, &maxPos, &minTrack, &maxTrack );

    if (maxSize.x < width) width = maxSize.x;
    if (maxSize.y < height) height = maxSize.y;
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
        win_attr.save_under    = ((classPtr->wc.style & CS_SAVEBITS) != 0);
        win_attr.cursor        = CURSORICON_XCursor;
        wndPtr->window = XCreateWindow( display, rootWindow, x, y,
                                        width, height, 0, CopyFromParent,
                                        InputOutput, CopyFromParent,
                                        CWEventMask | CWOverrideRedirect |
                                        CWColormap | CWCursor | CWSaveUnder |
                                        CWBackingStore, &win_attr );
        XStoreName( display, wndPtr->window, PTR_SEG_TO_LIN(windowName) );
        EVENT_RegisterWindow( wndPtr->window, hwnd );
    }
    
    if ((style & WS_CAPTION) && !(style & WS_CHILD))
    {
        if (menu) SetMenu(hwnd, menu);
        else if (classPtr->wc.lpszMenuName)
            SetMenu( hwnd, LoadMenu( instance, classPtr->wc.lpszMenuName ) );
    }
    else wndPtr->wIDmenu = (UINT)menu;

    GetSystemMenu( hwnd, TRUE );  /* Create a copy of the system menu */

      /* Send the WM_CREATE message */

    createStruct.lpCreateParams = (LPSTR)data;
    createStruct.hInstance      = instance;
    createStruct.hMenu          = menu;
    createStruct.hwndParent     = parent;
    createStruct.cx             = width;
    createStruct.cy             = height;
    createStruct.x              = x;
    createStruct.y              = y;
    createStruct.style          = style;
    createStruct.lpszName       = windowName;
    createStruct.lpszClass      = className;
    createStruct.dwExStyle      = 0;

    wmcreate = SendMessage( hwnd, WM_NCCREATE, 0, (LPARAM)MAKE_SEGPTR(&createStruct) );
    if (!wmcreate)
    {
	dprintf_win(stddeb,"CreateWindowEx: WM_NCCREATE return 0\n");
	wmcreate = -1;
    }
    else
    {
	WINPOS_SendNCCalcSize( hwnd, FALSE, &wndPtr->rectWindow,
			       NULL, NULL, NULL, &wndPtr->rectClient );
	wmcreate = SendMessage(hwnd, WM_CREATE, 0, (LPARAM)MAKE_SEGPTR(&createStruct));
    }

    if (wmcreate == -1)
    {
	  /* Abort window creation */
	dprintf_win(stddeb,"CreateWindowEx: wmcreate==-1, aborting\n");
        WIN_DestroyWindow( hwnd );
	return 0;
    }

    WIN_SendParentNotify( hwnd, WM_CREATE, wndPtr->wIDmenu, (LONG)hwnd );

    /* Show the window, maximizing or minimizing if needed */

    if (wndPtr->dwStyle & WS_MINIMIZE)
    {
        wndPtr->dwStyle &= ~WS_MAXIMIZE;
        WINPOS_FindIconPos( hwnd );
        SetWindowPos( hwnd, 0, wndPtr->ptIconPos.x, wndPtr->ptIconPos.y,
                      SYSMETRICS_CXICON, SYSMETRICS_CYICON,
                      SWP_FRAMECHANGED |
                      (style & WS_VISIBLE) ? SWP_SHOWWINDOW : 0 );
    }
    else if (wndPtr->dwStyle & WS_MAXIMIZE)
    {
        SetWindowPos( hwnd, 0, maxPos.x, maxPos.y, maxSize.x, maxSize.y,
                      SWP_FRAMECHANGED |
                      (style & WS_VISIBLE) ? SWP_SHOWWINDOW : 0 );
    }
    else if (style & WS_VISIBLE) ShowWindow( hwnd, SW_SHOW );

    dprintf_win(stddeb, "CreateWindowEx: return "NPFMT" \n", hwnd);
    return hwnd;
}


/***********************************************************************
 *           DestroyWindow   (USER.53)
 */
BOOL DestroyWindow( HWND hwnd )
{
    WND * wndPtr;
    CLASS * classPtr;

    dprintf_win(stddeb, "DestroyWindow ("NPFMT")\n", hwnd);
    
      /* Initialisation */

    if (hwnd == hwndDesktop) return FALSE;  /* Can't destroy desktop */
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (!(classPtr = CLASS_FindClassPtr( wndPtr->hClass ))) return FALSE;

      /* Hide the window */

    if (wndPtr->dwStyle & WS_VISIBLE)
	SetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOACTIVATE |
		      SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE );
    if ((hwnd == GetCapture()) || IsChild( hwnd, GetCapture() ))
	ReleaseCapture();
    WIN_SendParentNotify( hwnd, WM_DESTROY, wndPtr->wIDmenu, (LONG)hwnd );

      /* Recursively destroy owned windows */

    for (;;)
    {
        HWND hwndSibling = GetWindow( hwnd, GW_HWNDFIRST );
        while (hwndSibling)
        {
            WND *siblingPtr = WIN_FindWndPtr( hwndSibling );
            if (siblingPtr->hwndOwner == hwnd) break;
            hwndSibling = siblingPtr->hwndNext;
        }
        if (hwndSibling) DestroyWindow( hwndSibling );
        else break;
    }

      /* Send destroy messages and destroy children */

    SendMessage( hwnd, WM_DESTROY, 0, 0 );
    while (wndPtr->hwndChild)  /* The child removes itself from the list */
	DestroyWindow( wndPtr->hwndChild );
    SendMessage( hwnd, WM_NCDESTROY, 0, 0 );

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
 *           FindWindow   (USER.50)
 */
HWND FindWindow( SEGPTR ClassMatch, LPSTR TitleMatch )
{
    HCLASS hclass;
    CLASS *classPtr;
    HWND hwnd;

    if (ClassMatch)
    {
	hclass = CLASS_FindClassByName( ClassMatch, (HINSTANCE)0xffff,
                                        &classPtr );
	if (!hclass) return 0;
    }
    else hclass = 0;

    hwnd = GetTopWindow( hwndDesktop );
    while(hwnd)
    {
	WND *wndPtr = WIN_FindWndPtr( hwnd );
	if (!hclass || (wndPtr->hClass == hclass))
	{
	      /* Found matching class */
	    if (!TitleMatch) return hwnd;
	    if (wndPtr->hText)
	    {
		char *textPtr = (char *) USER_HEAP_LIN_ADDR( wndPtr->hText );
		if (!strcmp( textPtr, TitleMatch )) return hwnd;
	    }
	}
	hwnd = wndPtr->hwndNext;
    }
    return 0;
}
 
 
/**********************************************************************
 *           GetDesktopWindow        (USER.286)
 *	     GetDeskTopHwnd          (USER.278)
 */
HWND GetDesktopWindow(void)
{
    return hwndDesktop;
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
	SendMessage( hwnd, WM_ENABLE, TRUE, 0 );
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
	SendMessage( hwnd, WM_ENABLE, FALSE, 0 );
	return FALSE;
    }
    return ((wndPtr->dwStyle & WS_DISABLED) != 0);
}


/***********************************************************************
 *           IsWindowEnabled   (USER.35)
 */ 
BOOL IsWindowEnabled(HWND hWnd)
{
    WND * wndPtr; 

    if (!(wndPtr = WIN_FindWndPtr(hWnd))) return FALSE;
    return !(wndPtr->dwStyle & WS_DISABLED);
}


/**********************************************************************
 *	     GetWindowWord    (USER.133)
 */
WORD GetWindowWord( HWND hwnd, short offset )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0) return *(WORD *)(((char *)wndPtr->wExtra) + offset);
    switch(offset)
    {
	case GWW_ID:         return wndPtr->wIDmenu;
#ifdef WINELIB32
        case GWW_HWNDPARENT:
        case GWW_HINSTANCE: 
            fprintf(stderr,"GetWindowWord called with offset %d.\n",offset);
            return 0;
#else
	case GWW_HWNDPARENT: return (WORD)wndPtr->hwndParent;
	case GWW_HINSTANCE:  return (WORD)wndPtr->hInstance;
#endif
    }
    return 0;
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
 *	     SetWindowWord    (USER.134)
 */
WORD SetWindowWord( HWND hwnd, short offset, WORD newval )
{
    WORD *ptr, retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0) ptr = (WORD *)(((char *)wndPtr->wExtra) + offset);
    else switch(offset)
    {
#ifdef WINELIB32
	case GWW_ID:
	case GWW_HINSTANCE:
            fprintf(stderr,"SetWindowWord called with offset %d.\n",offset);
            return 0;
#else
	case GWW_ID:        ptr = &wndPtr->wIDmenu;   break;
	case GWW_HINSTANCE: ptr = (WORD*)&wndPtr->hInstance; break;
#endif
	default: return 0;
    }
    retval = *ptr;
    *ptr = newval;
    return retval;
}


/**********************************************************************
 *	     GetWindowLong    (USER.135)
 */
LONG GetWindowLong( HWND hwnd, short offset )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0) return *(LONG *)(((char *)wndPtr->wExtra) + offset);
    switch(offset)
    {
	case GWL_STYLE:   return wndPtr->dwStyle;
        case GWL_EXSTYLE: return wndPtr->dwExStyle;
	case GWL_WNDPROC: return (LONG)wndPtr->lpfnWndProc;
#ifdef WINELIB32
	case GWW_HWNDPARENT: return (LONG)wndPtr->hwndParent;
	case GWW_HINSTANCE:  return (LONG)wndPtr->hInstance;
#endif
    }
    return 0;
}


/**********************************************************************
 *	     SetWindowLong    (USER.136)
 */
LONG SetWindowLong( HWND hwnd, short offset, LONG newval )
{
    LONG *ptr, retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    if (offset >= 0) ptr = (LONG *)(((char *)wndPtr->wExtra) + offset);
    else switch(offset)
    {
	case GWL_STYLE:   ptr = &wndPtr->dwStyle; break;
        case GWL_EXSTYLE: ptr = &wndPtr->dwExStyle; break;
	case GWL_WNDPROC: ptr = (LONG *)&wndPtr->lpfnWndProc; break;
	default: return 0;
    }
    retval = *ptr;
    *ptr = newval;
    return retval;
}


/*******************************************************************
 *         GetWindowText          (USER.36)
 */
int WIN16_GetWindowText( HWND hwnd, SEGPTR lpString, int nMaxCount )
{
    return (int)SendMessage(hwnd, WM_GETTEXT, (WORD)nMaxCount, 
			                      (DWORD)lpString);
}

int GetWindowText( HWND hwnd, LPSTR lpString, int nMaxCount )
{
    int len;
    HANDLE handle;

      /* We have to allocate a buffer on the USER heap */
      /* to be able to pass its address to 16-bit code */
    if (!(handle = USER_HEAP_ALLOC( nMaxCount ))) return 0;
    len = (int)SendMessage( hwnd, WM_GETTEXT, (WPARAM)nMaxCount, 
                            (LPARAM)USER_HEAP_SEG_ADDR(handle) );
    strncpy( lpString, USER_HEAP_LIN_ADDR(handle), nMaxCount );
    USER_HEAP_FREE( handle );
    return len;
}


/*******************************************************************
 *         SetWindowText          (USER.37)
 */
void WIN16_SetWindowText( HWND hwnd, SEGPTR lpString )
{
    SendMessage( hwnd, WM_SETTEXT, 0, (DWORD)lpString );
}

void SetWindowText( HWND hwnd, LPCSTR lpString )
{
    HANDLE handle;

      /* We have to allocate a buffer on the USER heap */
      /* to be able to pass its address to 16-bit code */
    if (!(handle = USER_HEAP_ALLOC( strlen(lpString)+1 ))) return;
    strcpy( USER_HEAP_LIN_ADDR(handle), lpString );
    SendMessage( hwnd, WM_SETTEXT, 0, (LPARAM)USER_HEAP_SEG_ADDR(handle) );
    USER_HEAP_FREE( handle );
}


/*******************************************************************
 *         GetWindowTextLength    (USER.38)
 */
int GetWindowTextLength(HWND hwnd)
{
    return (int)SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0 );
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
    return (wndPtr->dwStyle & WS_CHILD) ?
            wndPtr->hwndParent : wndPtr->hwndOwner;
}


/*****************************************************************
 *         WIN_GetTopParent
 *
 * Get the top-level parent for a child window.
 */
HWND WIN_GetTopParent( HWND hwnd )
{
    while (hwnd)
    {
        WND *wndPtr = WIN_FindWndPtr( hwnd );
        if (wndPtr->dwStyle & WS_CHILD) hwnd = wndPtr->hwndParent;
        else break;
    }
    return hwnd;
}


/*****************************************************************
 *         SetParent              (USER.233)
 */
HWND SetParent(HWND hwndChild, HWND hwndNewParent)
{
    HWND temp;

    WND *wndPtr = WIN_FindWndPtr(hwndChild);
    if (!wndPtr || !(wndPtr->dwStyle & WS_CHILD)) return 0;

    temp = wndPtr->hwndParent;

    WIN_UnlinkWindow(hwndChild);
    if (hwndNewParent)
      wndPtr->hwndParent = hwndNewParent;
    else
      wndPtr->hwndParent = GetDesktopWindow();
    WIN_LinkWindow(hwndChild, HWND_BOTTOM);
    
    if (IsWindowVisible(hwndChild)) UpdateWindow(hwndChild);
    
    return temp;
}



/*******************************************************************
 *         IsChild    (USER.48)
 */
BOOL IsChild( HWND parent, HWND child )
{
    WND * wndPtr = WIN_FindWndPtr( child );
    while (wndPtr && (wndPtr->dwStyle & WS_CHILD))
    {
	if (wndPtr->hwndParent == parent) return TRUE;
        wndPtr = WIN_FindWndPtr( wndPtr->hwndParent );
    }
    return FALSE;
}


/***********************************************************************
 *           IsWindowVisible   (USER.49)
 */
BOOL IsWindowVisible( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    while (wndPtr && (wndPtr->dwStyle & WS_CHILD))
    {
        if (!(wndPtr->dwStyle & WS_VISIBLE)) return FALSE;
        wndPtr = WIN_FindWndPtr( wndPtr->hwndParent );
    }
    return (wndPtr && (wndPtr->dwStyle & WS_VISIBLE));
}

 
 
/*******************************************************************
 *         GetTopWindow    (USER.229)
 */
HWND GetTopWindow( HWND hwnd )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (wndPtr) return wndPtr->hwndChild;
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
	if (wndPtr->hwndParent)
	{
	    WND * parentPtr = WIN_FindWndPtr( wndPtr->hwndParent );
	    return parentPtr->hwndChild;
	}
	else return 0;
	
    case GW_HWNDLAST:
	if (!wndPtr->hwndParent) return 0;  /* Desktop window */
	while (wndPtr->hwndNext)
	{
	    hwnd = wndPtr->hwndNext;
	    wndPtr = WIN_FindWndPtr( hwnd );
	}
	return hwnd;
	
    case GW_HWNDNEXT:
	return wndPtr->hwndNext;
	
    case GW_HWNDPREV:	
	{
	    HWND hwndPrev;
	    
	    if (wndPtr->hwndParent)
	    {
		WND * parentPtr = WIN_FindWndPtr( wndPtr->hwndParent );
		hwndPrev = parentPtr->hwndChild;
	    }
	    else return 0;  /* Desktop window */
	    if (hwndPrev == hwnd) return 0;
	    while (hwndPrev)
	    {
		wndPtr = WIN_FindWndPtr( hwndPrev );
		if (wndPtr->hwndNext == hwnd) break;
		hwndPrev = wndPtr->hwndNext;
	    }
	    return hwndPrev;
	}
	
    case GW_OWNER:
	return wndPtr->hwndOwner;

    case GW_CHILD:
	return wndPtr->hwndChild;
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
    HWND hwnd = GetWindow( hwndDesktop, GW_CHILD );
    while (hwnd)
    {
        WND *wnd = WIN_FindWndPtr(hwnd);
        if (wnd->hwndOwner == owner && (wnd->dwStyle & WS_POPUP))
            ShowWindow( hwnd, fShow ? SW_SHOW : SW_HIDE );
        hwnd = wnd->hwndNext;
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
    HWND hwnd;
    WND *wndPtr;
    HWND *list, *pWnd;
    int count;

    /* We have to build a list of all windows first, to avoid */
    /* unpleasant side-effects, for instance if the callback  */
    /* function changes the Z-order of the windows.           */

      /* First count the windows */

    count = 0;
    for (hwnd = GetTopWindow(hwndDesktop); hwnd != 0; hwnd = wndPtr->hwndNext)
    {
        if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
        count++;
    }
    if (!count) return TRUE;

      /* Now build the list of all windows */

    if (!(list = (HWND *)malloc( sizeof(HWND) * count ))) return FALSE;
    for (hwnd = GetTopWindow(hwndDesktop), pWnd = list; hwnd != 0; hwnd = wndPtr->hwndNext)
    {
        wndPtr = WIN_FindWndPtr( hwnd );
        *pWnd++ = hwnd;
    }

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
    HWND hwnd;
    WND *wndPtr;
    HWND *list, *pWnd;
    HANDLE hQueue = GetTaskQueue( hTask );
    int count;

    /* This function is the same as EnumWindows(),    */
    /* except for an added check on the window queue. */

      /* First count the windows */

    count = 0;
    for (hwnd = GetTopWindow(hwndDesktop); hwnd != 0; hwnd = wndPtr->hwndNext)
    {
        if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
        if (wndPtr->hmemTaskQ == hQueue) count++;
    }
    if (!count) return TRUE;

      /* Now build the list of all windows */

    if (!(list = (HWND *)malloc( sizeof(HWND) * count ))) return FALSE;
    for (hwnd = GetTopWindow(hwndDesktop), pWnd = list; hwnd != 0; hwnd = wndPtr->hwndNext)
    {
        wndPtr = WIN_FindWndPtr( hwnd );
        if (wndPtr->hmemTaskQ == hQueue) *pWnd++ = hwnd;
    }

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
static BOOL WIN_EnumChildWin(HWND hwnd, FARPROC wndenumprc, LPARAM lParam)
{
    WND *wndPtr;

    while (hwnd)
    {
        if (!(wndPtr=WIN_FindWndPtr(hwnd))) return 0;
        if (!CallEnumWindowsProc( wndenumprc, hwnd, lParam )) return 0;
        if (!WIN_EnumChildWin(wndPtr->hwndChild, wndenumprc, lParam)) return 0;
        hwnd=wndPtr->hwndNext;
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
    hwnd = wndPtr->hwndChild;
    return WIN_EnumChildWin(hwnd, wndenumprc, lParam);         
}


/*******************************************************************
 *			AnyPopup		[USER.52]
 */
BOOL AnyPopup()
{
 WND   *wndPtr = WIN_FindWndPtr(hwndDesktop);
 HWND   hwnd = wndPtr->hwndChild;
 
 for( ; hwnd ; hwnd = wndPtr->hwndNext )
  {
	wndPtr = WIN_FindWndPtr(hwnd);
	if(wndPtr->hwndOwner)
	   if(wndPtr->dwStyle & WS_VISIBLE)
		return TRUE;
  }
	return FALSE;
}

/*******************************************************************
 *			FlashWindow		[USER.105]
 */
BOOL FlashWindow(HWND hWnd, BOOL bInvert)
{
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    dprintf_win(stddeb,"FlashWindow: "NPFMT"\n", hWnd);

    if (!wndPtr) return FALSE;

    if (wndPtr->dwStyle & WS_MINIMIZE)
    {
        if (bInvert && !(wndPtr->flags & WIN_NCACTIVATED))
        {
            HDC hDC = GetDC(hWnd);
            
            if (!SendMessage( hWnd, WM_ERASEBKGND, (WPARAM)hDC, (LPARAM)0 ))
                wndPtr->flags |= WIN_NEEDS_ERASEBKGND;
            
            ReleaseDC( hWnd, hDC );
            wndPtr->flags |= WIN_NCACTIVATED;
        }
        else
        {
            RedrawWindow( hWnd, 0, 0, RDW_INVALIDATE | RDW_ERASE |
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

        SendMessage( hWnd, WM_NCACTIVATE, wparam, (LPARAM)0 );
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
    dprintf_win(stdnimp,"EMPTY STUB !! SetSysModalWindow("NPFMT") !\n", hWnd);
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
 HWND		hWnd;
 BOOL		wParam,bResult = 0;
 POINT		pt;
 LPDRAGINFO	ptrDragInfo = (LPDRAGINFO) PTR_SEG_TO_LIN(spDragInfo);
 WND 	       *ptrQueryWnd = WIN_FindWndPtr(hQueryWnd),*ptrWnd;
 RECT		tempRect;	/* this sucks */

 if( !ptrQueryWnd || !ptrDragInfo ) return 0;

 pt 		= ptrDragInfo->pt;

 GetWindowRect(hQueryWnd,&tempRect); 

 if( !PtInRect(&tempRect,pt) ||
     (ptrQueryWnd->dwStyle & WS_DISABLED) )
	return 0;

 if( !(ptrQueryWnd->dwStyle & WS_MINIMIZE) ) 
   {
     tempRect = ptrQueryWnd->rectClient;
     if(ptrQueryWnd->dwStyle & WS_CHILD)
        MapWindowPoints(ptrQueryWnd->hwndParent,0,(LPPOINT)&tempRect,2);

     if( PtInRect(&tempRect,pt) )
	{
	 wParam = 0;
	 ptrWnd = WIN_FindWndPtr(hWnd = ptrQueryWnd->hwndChild);

	 for( ;ptrWnd ;ptrWnd = WIN_FindWndPtr(hWnd = ptrWnd->hwndNext) )
	   if( ptrWnd->dwStyle & WS_VISIBLE )
	     {
	      GetWindowRect(hWnd,&tempRect);

	      if( PtInRect(&tempRect,pt) ) 
		  break;
	     }

	 if(ptrWnd)
	    dprintf_msg(stddeb,"DragQueryUpdate: hwnd = "NPFMT", %i %i - %i %i\n",hWnd,
			(int)ptrWnd->rectWindow.left,(int)ptrWnd->rectWindow.top,
			(int)ptrWnd->rectWindow.right,(int)ptrWnd->rectWindow.bottom);	 
	 else
	    dprintf_msg(stddeb,"DragQueryUpdate: hwnd = "NPFMT"\n",hWnd);

	 if(ptrWnd)
	   if( !(ptrWnd->dwStyle & WS_DISABLED) )
	        bResult = DRAG_QueryUpdate(hWnd, spDragInfo);

	 if(bResult) return bResult;
	}
     else wParam = 1;
   }
 else wParam = 1;

 ScreenToClient(hQueryWnd,&ptrDragInfo->pt);

 ptrDragInfo->hScope = hQueryWnd;

 bResult = SendMessage( hQueryWnd ,WM_QUERYDROPOBJECT ,
                       (WPARAM)wParam ,(LPARAM) spDragInfo );
 if( !bResult ) 
      ptrDragInfo->pt = pt;

 return bResult;
}

/*******************************************************************
 *                      DragDetect ( USER.465 )
 *
 */
BOOL DragDetect(HWND hWnd, POINT pt)
{
  MSG   msg;
  RECT  rect;

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
		  POINT pt = { LOWORD(msg.lParam), HIWORD(msg.lParam) };
                  if( !PtInRect( &rect, pt ) )
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
 HANDLE		hDragInfo  = GlobalAlloc( GMEM_SHARE | GMEM_ZEROINIT, 2*sizeof(DRAGINFO));
 WND           *wndPtr = WIN_FindWndPtr(hWnd);
 DWORD		dwRet = 0;
 short	 	dragDone = 0;
 HCURSOR	hCurrentCursor = 0;
 HWND		hCurrentWnd = 0;
 WORD	        btemp;

 lpDragInfo = (LPDRAGINFO) GlobalLock(hDragInfo);
 spDragInfo = (SEGPTR) WIN16_GlobalLock(hDragInfo);

 if( !lpDragInfo || !spDragInfo ) return 0L;

 hBummer = LoadCursor(0,IDC_BUMMER);

 if( !hBummer || !wndPtr )
   {
        GlobalFree(hDragInfo);
        return 0L;
   }

 if(hCursor)
   {
	if( !(hDragCursor = CURSORICON_IconToCursor(hCursor)) )
	  {
	   GlobalFree(hDragInfo);
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
    dprintf_msg(stddeb,"drag: lpDI->hScope = "NPFMT"\n",lpDragInfo->hScope);

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
    SendMessage( hWnd, WM_DRAGLOOP, (WPARAM)(hCurrentCursor != hBummer) , 
	                            (LPARAM) spDragInfo );
    /* send WM_DRAGSELECT or WM_DRAGMOVE */
    if( hCurrentWnd != lpDragInfo->hScope )
	{
	 if( hCurrentWnd )
	   SendMessage( hCurrentWnd, WM_DRAGSELECT, 0, 
		       (LPARAM)MAKELONG(LOWORD(spDragInfo)+sizeof(DRAGINFO),
				        HIWORD(spDragInfo)) );
	 hCurrentWnd = lpDragInfo->hScope;
	 if( hCurrentWnd )
           SendMessage( hCurrentWnd, WM_DRAGSELECT, 1, (LPARAM)spDragInfo); 
	}
    else
	if( hCurrentWnd )
	   SendMessage( hCurrentWnd, WM_DRAGMOVE, 0, (LPARAM)spDragInfo);


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
	dwRet = SendMessage( lpDragInfo->hScope, WM_DROPOBJECT, 
			     (WPARAM)hWnd, (LPARAM)spDragInfo );
 GlobalFree(hDragInfo);

 return dwRet;
}

