/*
 * Window related functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Core.h>
#include <X11/Shell.h>

#include "class.h"
#include "win.h"
#include "user.h"
#include "dce.h"
#include "sysmetrics.h"

extern Display * display;
extern Colormap COLOR_WinColormap;

extern void EVENT_RegisterWindow( Window w, HWND hwnd );  /* event.c */

HWND firstWindow = 0;

/***********************************************************************
 *           WIN_FindWndPtr
 *
 * Return a pointer to the WND structure corresponding to a HWND.
 */
WND * WIN_FindWndPtr( HWND hwnd )
{
    WND * ptr;
    
    if (!hwnd) return NULL;
    ptr = (WND *) USER_HEAP_ADDR( hwnd );
    if (ptr->dwMagic != WND_MAGIC) return NULL;
    return ptr;
}


/***********************************************************************
 *           WIN_UnlinkWindow
 *
 * Remove a window from the siblings linked list.
 */
BOOL WIN_UnlinkWindow( HWND hwnd )
{    
    HWND * curWndPtr;
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr) return FALSE;
    if (wndPtr->hwndParent)
    {
	WND * parentPtr = WIN_FindWndPtr( wndPtr->hwndParent );
	curWndPtr = &parentPtr->hwndChild;
    }    
    else curWndPtr = &firstWindow;

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

    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;
    
    if ((hwndInsertAfter == HWND_TOP) || (hwndInsertAfter == HWND_BOTTOM))
    {
	  /* Make hwndPtr point to the first sibling hwnd */
	if (wndPtr->hwndParent)
	{
	    WND * parentPtr = WIN_FindWndPtr( wndPtr->hwndParent );
	    if (parentPtr) hwndPtr = &parentPtr->hwndChild;
	}
	else hwndPtr = &firstWindow;
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
    
    if (!hwnd) hwnd = firstWindow;
    for ( ; hwnd != 0; hwnd = wndPtr->hwndNext )
    {
	if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
	if (wndPtr->hrgnUpdate) return hwnd;
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
static void WIN_SendParentNotify( HWND hwnd, WND * wndPtr, WORD event )
{
    HWND current = wndPtr->hwndParent;

    if (wndPtr->dwExStyle & WS_EX_NOPARENTNOTIFY) return;
    while (current)
    {
	SendMessage( current, WM_PARENTNOTIFY, 
		     event, MAKELONG( hwnd, wndPtr->wIDmenu ) );
	current = GetParent( current );
    }
}


/***********************************************************************
 *           CreateWindow   (USER.41)
 */
HWND CreateWindow( LPSTR className, LPSTR windowName,
		   DWORD style, short x, short y, short width, short height,
		   HWND parent, HMENU menu, HANDLE instance, LPSTR data ) 
{
    return CreateWindowEx( 0, className, windowName, style,
			   x, y, width, height, parent, menu, instance, data );
}


/***********************************************************************
 *           CreateWindowEx   (USER.452)
 */
HWND CreateWindowEx( DWORD exStyle, LPSTR className, LPSTR windowName,
		     DWORD style, short x, short y, short width, short height,
		     HWND parent, HMENU menu, HANDLE instance, LPSTR data ) 
{
    HANDLE class, hwnd;
    CLASS *classPtr;
    WND *wndPtr, *parentPtr = NULL;
    CREATESTRUCT *createStruct;
    HANDLE hcreateStruct;
    int wmcreate;
    XSetWindowAttributes win_attr;
    Window parentWindow;
    int x_rel, y_rel;
    LPPOPUPMENU lpbar;

#ifdef DEBUG_WIN
    printf( "CreateWindowEx: %d '%s' '%s' %d,%d %dx%d %08x %x\n",
	   exStyle, className, windowName, x, y, width, height, style, parent);
#endif

    if (x == CW_USEDEFAULT) x = y = 0;
    if (width == CW_USEDEFAULT)
    {
	width = 600;
	height = 400;
    }
    if (width == 0) width = 1;
    if (height == 0) height = 1;

      /* Find the parent and class */

    if (parent) 
    {
	  /* Check if parent is valid */
	parentPtr = WIN_FindWndPtr( parent );
	if (!parent) return 0;
    }
    else if (style & WS_CHILD) return 0;  /* WS_CHILD needs a parent */
    
    if (!(class = CLASS_FindClassByName( className, &classPtr ))) {
	printf("CreateWindow BAD CLASSNAME '%s' !\n", className);
	return 0;
	}    

      /* Correct the window style */

    if (!(style & (WS_POPUP | WS_CHILD)))  /* Overlapped window */
	style |= WS_CAPTION | WS_CLIPSIBLINGS;
    if (exStyle & WS_EX_DLGMODALFRAME) style &= ~WS_THICKFRAME;

      /* Create the window structure */

    hwnd = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(WND)+classPtr->wc.cbWndExtra);
    if (!hwnd) return 0;

      /* Fill the structure */

    wndPtr = (WND *) USER_HEAP_ADDR( hwnd );
    wndPtr->hwndNext   = 0;
    wndPtr->hwndChild  = 0;
    wndPtr->dwMagic    = WND_MAGIC;
    wndPtr->hwndParent = (style & WS_CHILD) ? parent : 0;
    wndPtr->hwndOwner  = (style & WS_CHILD) ? 0 : parent;
    wndPtr->hClass     = class;
    wndPtr->hInstance  = instance;
    wndPtr->rectWindow.left   = x;
    wndPtr->rectWindow.top    = y;
    wndPtr->rectWindow.right  = x + width;
    wndPtr->rectWindow.bottom = y + height;
    wndPtr->rectClient        = wndPtr->rectWindow;
    wndPtr->rectNormal        = wndPtr->rectWindow;
    wndPtr->ptIconPos.x       = -1;
    wndPtr->ptIconPos.y       = -1;
    wndPtr->ptMaxPos.x        = -1;
    wndPtr->ptMaxPos.y        = -1;
    wndPtr->hmemTaskQ         = GetTaskQueue(0);
    wndPtr->hrgnUpdate        = 0;
    wndPtr->hwndLastActive    = 0;
    wndPtr->lpfnWndProc       = classPtr->wc.lpfnWndProc;
    wndPtr->dwStyle           = style;
    wndPtr->dwExStyle         = exStyle;
    wndPtr->hmenuSystem       = 0;
    wndPtr->wIDmenu           = menu;
    wndPtr->hText             = 0;
    wndPtr->flags             = 0;
    wndPtr->hCursor           = 0;
    wndPtr->hWndVScroll       = 0;
    wndPtr->hWndHScroll       = 0;
    wndPtr->hWndMenuBar       = 0;
    wndPtr->hWndCaption       = 0;

    if (classPtr->wc.cbWndExtra)
	memset( wndPtr->wExtra, 0, classPtr->wc.cbWndExtra );
    if (classPtr->wc.style & CS_DBLCLKS) wndPtr->flags |= WIN_DOUBLE_CLICKS;
    classPtr->cWindows++;

      /* Get class or window DC if needed */
    if (classPtr->wc.style & CS_OWNDC)
    {
	wndPtr->flags |= WIN_OWN_DC;
	wndPtr->hdce = DCE_AllocDCE( DCE_WINDOW_DC );
    }
    else if (classPtr->wc.style & CS_CLASSDC)
    {
	wndPtr->flags |= WIN_CLASS_DC;
	wndPtr->hdce = classPtr->hdce;
    }
    else wndPtr->hdce = 0;

      /* Insert the window in the linked list */

    WIN_LinkWindow( hwnd, HWND_TOP );

      /* Create the X window */

    win_attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
	                 PointerMotionMask | ButtonPressMask |
			 ButtonReleaseMask | FocusChangeMask | EnterWindowMask;
    win_attr.override_redirect = True;
    win_attr.colormap = COLOR_WinColormap;
    if (style & WS_CHILD)
    {
	parentWindow = parentPtr->window;
	x_rel = x + parentPtr->rectClient.left - parentPtr->rectWindow.left;
	y_rel = y + parentPtr->rectClient.top - parentPtr->rectWindow.top;
    }
    else
    {
	parentWindow = DefaultRootWindow( display );
	x_rel = x;
	y_rel = y;
    }
    wndPtr->window = XCreateWindow(display, parentWindow,
				   x_rel, y_rel, width, height, 0,
				   CopyFromParent, InputOutput, CopyFromParent,
				   CWEventMask | CWOverrideRedirect |
				   CWColormap, &win_attr );
    XStoreName( display, wndPtr->window, windowName );

      /* Send the WM_CREATE message */
	
    hcreateStruct = GlobalAlloc( GMEM_MOVEABLE, sizeof(CREATESTRUCT) );
    createStruct = (CREATESTRUCT *) GlobalLock( hcreateStruct );
    createStruct->lpCreateParams = data;
    createStruct->hInstance      = instance;
    createStruct->hMenu          = menu;
    createStruct->hwndParent     = parent;
    createStruct->cx             = width;
    createStruct->cy             = height;
    createStruct->x              = x;
    createStruct->y              = y;
    createStruct->style          = style;
    createStruct->lpszName       = windowName;
    createStruct->lpszClass      = className;
    createStruct->dwExStyle      = 0;

    wmcreate = SendMessage( hwnd, WM_NCCREATE, 0, (LONG)createStruct );
    if (!wmcreate) wmcreate = -1;
    else
    {
	  /* Send WM_NCCALCSIZE message */
	NCCALCSIZE_PARAMS *params;
	HANDLE hparams;
	hparams = GlobalAlloc( GMEM_MOVEABLE, sizeof(*params) );
	if (hparams)
	{
	    params = (NCCALCSIZE_PARAMS *) GlobalLock( hparams );
	    params->rgrc[0] = wndPtr->rectWindow;
	    params->lppos = NULL;
	    SendMessage( hwnd, WM_NCCALCSIZE, FALSE, (LONG)params );
	    wndPtr->rectClient = params->rgrc[0];
	    GlobalUnlock( hparams );
	    GlobalFree( hparams );
	}	
	wmcreate = SendMessage( hwnd, WM_CREATE, 0, (LONG)createStruct );
    }

    GlobalUnlock( hcreateStruct );
    GlobalFree( hcreateStruct );

    if (wmcreate == -1)
    {
	  /* Abort window creation */

	WIN_UnlinkWindow( hwnd );
	XDestroyWindow( display, wndPtr->window );
	if (wndPtr->flags & WIN_OWN_DC) DCE_FreeDCE( wndPtr->hdce );
	classPtr->cWindows--;
	USER_HEAP_FREE( hwnd );
	return 0;
    }

      /* Create scrollbars */

#if 0
    if (windowName != NULL) SetWindowText(hwnd, windowName);
    if ((style & WS_CAPTION) == WS_CAPTION) {
	wndPtr->hWndCaption = CreateWindow("CAPTION", "",
		WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
		0, -20, width, 20, hwnd, 1, instance, 0L);
	}
#endif
    if (((style & WS_CHILD) != WS_CHILD) && (wndPtr->wIDmenu != 0)) {
	lpbar = (LPPOPUPMENU) GlobalLock(wndPtr->wIDmenu);
	if (lpbar != NULL) {
	    lpbar->ownerWnd = hwnd;
	    wndPtr->hWndMenuBar = CreateWindow("POPUPMENU", "",
		WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
		0, 0, width, 20, hwnd, 2, instance, (LPSTR)lpbar);
	    }
	}
    if ((style & WS_VSCROLL) == WS_VSCROLL)
    {
	wndPtr->hWndVScroll = CreateWindow("SCROLLBAR", "",
		WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBS_VERT,
		wndPtr->rectClient.right-wndPtr->rectClient.left, 0,
                SYSMETRICS_CXVSCROLL,
		wndPtr->rectClient.bottom-wndPtr->rectClient.top,
                hwnd, 3, instance, 0L);
    }
    if ((style & WS_HSCROLL) == WS_HSCROLL)
    {
	wndPtr->hWndHScroll = CreateWindow("SCROLLBAR", "",
		WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBS_HORZ,
		0, wndPtr->rectClient.bottom-wndPtr->rectClient.top,
                wndPtr->rectClient.right-wndPtr->rectClient.left,
		SYSMETRICS_CYHSCROLL,
                hwnd, 4, instance, 0L);
    }

    EVENT_RegisterWindow( wndPtr->window, hwnd );

    WIN_SendParentNotify( hwnd, wndPtr, WM_CREATE );
    
    if (style & WS_VISIBLE) ShowWindow( hwnd, SW_SHOW );
    return hwnd;
}

/***********************************************************************
 *           DestroyWindow   (USER.53)
 */
BOOL DestroyWindow( HWND hwnd )
{
    WND * wndPtr;
    CLASS * classPtr;
    
      /* Initialisation */

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (!(classPtr = CLASS_FindClassPtr( wndPtr->hClass ))) return FALSE;
    WIN_SendParentNotify( hwnd, wndPtr, WM_DESTROY );

      /* Send destroy messages */

    SendMessage( hwnd, WM_DESTROY, 0, 0 );
    SendMessage( hwnd, WM_NCDESTROY, 0, 0 );
    
      /* Destroy all children */

    if (wndPtr->hWndVScroll) DestroyWindow(wndPtr->hWndVScroll);
    if (wndPtr->hWndHScroll) DestroyWindow(wndPtr->hWndHScroll);
    while (wndPtr->hwndChild)  /* The child removes itself from the list */
	DestroyWindow( wndPtr->hwndChild );

      /* Remove the window from the linked list */

    WIN_UnlinkWindow( hwnd );

      /* Destroy the window */

    wndPtr->dwMagic = 0;  /* Mark it as invalid */
    XDestroyWindow( display, wndPtr->window );
    if (wndPtr->flags & WIN_OWN_DC) DCE_FreeDCE( wndPtr->hdce );
    classPtr->cWindows--;
    USER_HEAP_FREE( hwnd );
    return TRUE;
}


/***********************************************************************
 *           CloseWindow   (USER.43)
 */
void CloseWindow(HWND hWnd)
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr->dwStyle & WS_CHILD) return;
    ShowWindow(hWnd, SW_MINIMIZE);
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
HWND FindWindow(LPSTR ClassMatch, LPSTR TitleMatch)
{
    return((HWND)NULL);
}
 
 
/***********************************************************************
 *           UpdateWindow   (USER.124)
 */
void UpdateWindow( HWND hwnd )
{
    if (GetUpdateRect( hwnd, NULL, FALSE )) 
    {
	if (IsWindowVisible( hwnd )) SendMessage( hwnd, WM_PAINT, 0, 0 );
    }
}

/**********************************************************************
 *	     GetMenu	    (USER.157)
 */
HMENU GetMenu( HWND hwnd ) 
{ 
    WND * wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr == NULL)
	return 0;
    return wndPtr->wIDmenu;
}

/**********************************************************************
 *           SetMenu        (USER.158)
 */
BOOL SetMenu(HWND hwnd, HMENU hmenu)
{
    return FALSE;
}


/**********************************************************************
 *           GetDesktopWindow        (USER.286)
 */
HWND GetDesktopWindow()
{
    return 0;
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
	case GWW_HWNDPARENT: return wndPtr->hwndParent;
	case GWW_HINSTANCE:  return wndPtr->hInstance;
    }
    return 0;
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
	case GWW_ID: ptr = &wndPtr->wIDmenu;
	case GWW_HINSTANCE: ptr = &wndPtr->hInstance;
	default: return 0;
    }
    retval = *ptr;
    *ptr = newval;
    return retval;
}



/*******************************************************************
 *    WIN_SetSensitive
 * 
 *      sets hWnd and all children to the same sensitivity
 * 
 *      sets hWnd sensitive and then calls SetSensitive on hWnd's child
 *      and all of hWnd's child's Next windows 
 */
static BOOL WIN_SetSensitive(HWND hWnd, BOOL fEnable)
{
    WND *wndPtr;
    HWND hwnd;

    printf("in SetSenitive\n");

    if (!hWnd) return 0;
    if (!(wndPtr = WIN_FindWndPtr(hWnd))) return 0;

	
    if (fEnable) {
        wndPtr->dwStyle &= ~WS_DISABLED;
     } else {
        wndPtr->dwStyle |= WS_DISABLED;
     }
  
    hwnd=wndPtr->hwndChild;
    while (hwnd) {					/* mk next child sens */
        WIN_SetSensitive(hwnd, fEnable); 
        if ( !(wndPtr=WIN_FindWndPtr(hwnd)) ) return 0;
	hwnd=wndPtr->hwndNext;
    }
    return 1;

}

/*******************************************************************
 *           EnableWindow   (USER.34)
 * 
 *
 */

BOOL EnableWindow(HWND hWnd, BOOL fEnable)
{
    WND *wndPtr;
    int eprev;
   
    if (hWnd == 0) return 0;
    
    wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == 0) return 0;

    eprev = ! (wndPtr->dwStyle & WS_DISABLED);

    if (fEnable != eprev) {                        /* change req */
        printf("changing window\n");
        WIN_SetSensitive(hWnd, fEnable); 
        SendMessage(hWnd, WM_ENABLE, (WORD)fEnable, 0);  
    }
    return !eprev;
}

/***********************************************************************
 *           IsWindowEnabled   (USER.35)
 */
 
BOOL IsWindowEnabled(HWND hWnd)
{
    WND * wndPtr; 

    if (hWnd == 0) return 0;
    wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == 0) return 0;

    return !(wndPtr->dwStyle & WS_DISABLED);
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
	case GWL_STYLE:   ptr = &wndPtr->dwStyle;
        case GWL_EXSTYLE: ptr = &wndPtr->dwExStyle;
	case GWL_WNDPROC: ptr = (LONG *)(&wndPtr->lpfnWndProc);
	default: return 0;
    }
    retval = *ptr;
    *ptr = newval;
    return retval;
}


/*******************************************************************
 *         GetWindowText          (USER.36)
 */
int GetWindowText(HWND hwnd, LPSTR lpString, int nMaxCount)
{
    return (int)SendMessage(hwnd, WM_GETTEXT, (WORD)nMaxCount, 
			                      (DWORD)lpString);
}

/*******************************************************************
 *         SetWindowText          (USER.37)
 */
void SetWindowText(HWND hwnd, LPSTR lpString)
{
    SendMessage(hwnd, WM_SETTEXT, (WORD)NULL, (DWORD)lpString);
}

/*******************************************************************
 *         GetWindowTextLength    (USER.38)
 */
int GetWindowTextLength(HWND hwnd)
{
    return (int)SendMessage(hwnd, WM_GETTEXTLENGTH, (WORD)NULL, 
			                            (DWORD)NULL);
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
    return wndPtr->hwndParent;
}


/*******************************************************************
 *         IsChild    (USER.48)
 */
BOOL IsChild( HWND parent, HWND child )
{
    HWND curChild;
    WND * parentPtr;
    WND * childPtr;

    if (!(parentPtr = WIN_FindWndPtr( parent ))) return FALSE;
    curChild = parentPtr->hwndChild;

    while (curChild)
    {
	if (curChild == child) return TRUE;
	if (IsChild( curChild, child )) return TRUE;
	if (!(childPtr = WIN_FindWndPtr( curChild ))) return FALSE;
	curChild = childPtr->hwndNext;
    }    
    return FALSE;
}


/***********************************************************************
 *           IsWindowVisible   (USER.49)
 */
BOOL IsWindowVisible(HWND hWnd)
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == 0) return(FALSE);
    else return ((wndPtr->dwStyle & WS_VISIBLE) != 0);
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
	else return firstWindow;
	
    case GW_HWNDLAST:
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
	    else hwndPrev = firstWindow;
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
