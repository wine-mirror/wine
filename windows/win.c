/*
 * Window related functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "options.h"
#include "class.h"
#include "win.h"
#include "user.h"
#include "dce.h"
#include "sysmetrics.h"
#include "scroll.h"
#include "icon.h"

extern Colormap COLOR_WinColormap;

extern void EVENT_RegisterWindow( Window w, HWND hwnd );  /* event.c */
extern void CURSOR_SetWinCursor( HWND hwnd, HCURSOR hcursor );  /* cursor.c */
extern void WINPOS_ChangeActiveWindow( HWND hwnd, BOOL mouseMsg ); /*winpos.c*/
extern HMENU CopySysMenu(); /* menu.c */
extern LONG MDIClientWndProc(HWND hwnd, WORD message, 
			     WORD wParam, LONG lParam); /* mdi.c */


static HWND hwndDesktop = 0;
static HWND hWndSysModal = 0;

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
void WIN_SendParentNotify( HWND hwnd, WORD event, LONG lParam )
{
    HWND current = GetParent( hwnd );
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    
    if (!wndPtr || (wndPtr->dwExStyle & WS_EX_NOPARENTNOTIFY)) return;
    while (current)
    {
	SendMessage( current, WM_PARENTNOTIFY, event, lParam );
	current = GetParent( current );
    }
}


/***********************************************************************
 *           WIN_CreateDesktopWindow
 *
 * Create the desktop window.
 */
BOOL WIN_CreateDesktopWindow()
{
    WND *wndPtr;
    HCLASS hclass;
    CLASS *classPtr;

    if (!(hclass = CLASS_FindClassByName( DESKTOP_CLASS_NAME, &classPtr )))
	return FALSE;

    hwndDesktop = USER_HEAP_ALLOC( GMEM_MOVEABLE,
				   sizeof(WND)+classPtr->wc.cbWndExtra );
    if (!hwndDesktop) return FALSE;
    wndPtr = (WND *) USER_HEAP_ADDR( hwndDesktop );

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
    wndPtr->hwndLastActive    = 0;
    wndPtr->lpfnWndProc       = classPtr->wc.lpfnWndProc;
    wndPtr->dwStyle           = WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    wndPtr->dwExStyle         = 0;
    wndPtr->hdce              = 0;
    wndPtr->VScroll           = NULL;
    wndPtr->HScroll           = NULL;
    wndPtr->scroll_flags      = 0;
    wndPtr->wIDmenu           = 0;
    wndPtr->hText             = 0;
    wndPtr->flags             = 0;
    wndPtr->window            = rootWindow;
    wndPtr->hSysMenu          = 0;
    wndPtr->hProp	          = 0;
    wndPtr->hTask	          = 0;

      /* Send dummy WM_NCCREATE message */
    SendMessage( hwndDesktop, WM_NCCREATE, 0, 0 );
    EVENT_RegisterWindow( wndPtr->window, hwndDesktop );
    RedrawWindow( hwndDesktop, NULL, 0,
		  RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW );
    return TRUE;
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
    XSetWindowAttributes win_attr, icon_attr;
    int iconWidth, iconHeight;

#ifdef DEBUG_WIN
    printf( "CreateWindowEx: %04X '%s' '%s' %04X %d,%d %dx%d %04X %04X %04X %08X\n",
				exStyle, className, windowName, style, x, y, width, height, 
				parent, menu, instance, data);
#endif
	/* 'soundrec.exe' has negative position ! 
	Why ? For now, here a patch : */
	if (x < 0) x = 0;
	if (y < 0) y = 0;
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
	if (!(parentPtr = WIN_FindWndPtr( parent ))) return 0;
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
	wndPtr->window 	   = 0;
    wndPtr->dwMagic    = WND_MAGIC;
    wndPtr->hwndParent = (style & WS_CHILD) ? parent : hwndDesktop;
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
    wndPtr->hwndPrevActive    = 0;
    wndPtr->hwndLastActive    = 0;
    wndPtr->lpfnWndProc       = classPtr->wc.lpfnWndProc;
    wndPtr->dwStyle           = style;
    wndPtr->dwExStyle         = exStyle;
	wndPtr->wIDmenu   		  = 0;
    wndPtr->hText             = 0;
    wndPtr->flags             = 0;
    wndPtr->VScroll           = NULL;
    wndPtr->HScroll           = NULL;
    wndPtr->scroll_flags      = 0;
    wndPtr->hSysMenu          = 0;
    wndPtr->hProp	          = 0;
    wndPtr->hTask	          = 0;

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
			 ButtonReleaseMask | EnterWindowMask;
    win_attr.override_redirect = (rootWindow == DefaultRootWindow(display));
    win_attr.colormap          = COLOR_WinColormap;
    if (!(style & WS_CHILD))
    {
	parentPtr = WIN_FindWndPtr( hwndDesktop );
	  /* Only select focus events on top-level override-redirect windows */
	if (win_attr.override_redirect) win_attr.event_mask |= FocusChangeMask;
    }
    if (Options.nobackingstore)
       win_attr.backing_store = NotUseful;
    else
       win_attr.backing_store = Always;

    win_attr.save_under = ((classPtr->wc.style & CS_SAVEBITS) != 0);

    wndPtr->window = XCreateWindow( display, parentPtr->window,
		   x + parentPtr->rectClient.left - parentPtr->rectWindow.left,
		   y + parentPtr->rectClient.top - parentPtr->rectWindow.top,
		   width, height, 0,
		   CopyFromParent, InputOutput, CopyFromParent,
		   CWEventMask | CWOverrideRedirect | CWColormap |
		   CWSaveUnder | CWBackingStore, &win_attr );
    XStoreName( display, wndPtr->window, windowName );


    /* create icon window */

    icon_attr.override_redirect = rootWindow==DefaultRootWindow(display);
    icon_attr.background_pixel = WhitePixelOfScreen(screen);
    icon_attr.event_mask = ExposureMask | KeyPressMask |
                            ButtonPressMask | ButtonReleaseMask;

    wndPtr->hIcon = classPtr->wc.hIcon;
    if (wndPtr->hIcon != (HICON)NULL) {
      ICONALLOC   *lpico;
      lpico = (ICONALLOC *)GlobalLock(wndPtr->hIcon);
      printf("icon is %d x %d\n", 
              (int)lpico->descriptor.Width,
              (int)lpico->descriptor.Height);
      iconWidth = (int)lpico->descriptor.Width;
      iconHeight = (int)lpico->descriptor.Height;
    } else {
      printf("icon was NULL\n");
      iconWidth = 64;
      iconHeight = 64;
    }

    wndPtr->icon = XCreateWindow(display, parentPtr->window,
                    10, 10, 100, iconHeight+20, 
                    0, CopyFromParent,
                    InputOutput, CopyFromParent,
                    CWBorderPixel | CWEventMask | CWOverrideRedirect, 
                    &icon_attr);
   
    if (style & WS_MINIMIZE) 
    {
      style &= ~WS_MINIMIZE;
    }
 


#ifdef DEBUG_MENU
    printf("CreateWindowEx // menu=%04X instance=%04X classmenu=%08X !\n", 
    	menu, instance, classPtr->wc.lpszMenuName); 
#endif
	if ((style & WS_CAPTION) && (style & WS_CHILD) == 0) {
		if (menu != 0)
			SetMenu(hwnd, menu);
		else {
			if (classPtr->wc.lpszMenuName != NULL)
				SetMenu(hwnd, LoadMenu(instance, classPtr->wc.lpszMenuName));
			}
		}
	else
		wndPtr->wIDmenu   = menu;

      /* Send the WM_CREATE message */

    hcreateStruct = USER_HEAP_ALLOC( GMEM_MOVEABLE, sizeof(CREATESTRUCT) );
    createStruct = (CREATESTRUCT *) USER_HEAP_ADDR( hcreateStruct );
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
	hparams = USER_HEAP_ALLOC( GMEM_MOVEABLE, sizeof(*params) );
	if (hparams)
	{
	    params = (NCCALCSIZE_PARAMS *) USER_HEAP_ADDR( hparams );
	    params->rgrc[0] = wndPtr->rectWindow;
	    params->lppos = NULL;
	    SendMessage( hwnd, WM_NCCALCSIZE, FALSE, (LONG)params );
	    wndPtr->rectClient = params->rgrc[0];
	    USER_HEAP_FREE( hparams );
	}	
	wmcreate = SendMessage( hwnd, WM_CREATE, 0, (LONG)createStruct );
    }

    USER_HEAP_FREE( hcreateStruct );

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

      /* Create a copy of SysMenu */
    if (style & WS_SYSMENU) wndPtr->hSysMenu = CopySysMenu();

	/* Register window in current task windows list */
	AddWindowToTask(GetCurrentTask(), hwnd);

      /* Set window cursor */
    if (classPtr->wc.hCursor) CURSOR_SetWinCursor( hwnd, classPtr->wc.hCursor);
    else CURSOR_SetWinCursor( hwnd, LoadCursor( 0, IDC_ARROW ));

    EVENT_RegisterWindow( wndPtr->window, hwnd );
    EVENT_RegisterWindow( wndPtr->icon, hwnd );

    WIN_SendParentNotify( hwnd, WM_CREATE, MAKELONG( hwnd, wndPtr->wIDmenu ) );
    
    if (style & WS_VISIBLE) ShowWindow( hwnd, SW_SHOW );
#ifdef DEBUG_WIN
    printf( "CreateWindowEx: return %04X \n", hwnd);
#endif
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

    if (hwnd == hwndDesktop) return FALSE;  /* Can't destroy desktop */
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (!(classPtr = CLASS_FindClassPtr( wndPtr->hClass ))) return FALSE;

      /* Hide the window */

    if (wndPtr->dwStyle & WS_VISIBLE)
	SetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOACTIVATE |
		      SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE );
    if ((hwnd == GetCapture()) || IsChild( hwnd, GetCapture() ))
	ReleaseCapture();
    WIN_SendParentNotify( hwnd, WM_DESTROY, MAKELONG(hwnd, wndPtr->wIDmenu) );

      /* Send destroy messages and destroy children */

    SendMessage( hwnd, WM_DESTROY, 0, 0 );
    while (wndPtr->hwndChild)  /* The child removes itself from the list */
	DestroyWindow( wndPtr->hwndChild );
    SendMessage( hwnd, WM_NCDESTROY, 0, 0 );

      /* Remove the window from current task windows list */
	RemoveWindowFromTask(GetCurrentTask(), hwnd);

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
    HCLASS hclass;
    CLASS *classPtr;
    HWND hwnd;

    if (ClassMatch)
    {
	hclass = CLASS_FindClassByName( ClassMatch, &classPtr );
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
		char *textPtr = (char *) USER_HEAP_ADDR( wndPtr->hText );
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
	  break;
        case GWL_EXSTYLE: ptr = &wndPtr->dwExStyle;
	  break;
	case GWL_WNDPROC: ptr = (LONG *)(&wndPtr->lpfnWndProc);
	  break;
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
    if (!wndPtr || !(wndPtr->dwStyle & WS_CHILD)) return 0;
    return wndPtr->hwndParent;
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
 *    EnumWindows             (USER.54)
 * 
 *  o gets the desktop window and iterates over all the windows
 *    which are direct decendents of the desktop * by iterating over
 *    the desktop's child window and all the child windows next
 *    pointers
 *
 *  o call wndenumprc for every child window the desktop has
 *    (parameters to Callback16 passed backwards so they are
 *    put in in pascal calling order)
 *
 *  o if wndenumprc returns 0 exit
 * 
 */
BOOL EnumWindows(FARPROC wndenumprc, LPARAM lParam)
{
    HWND hwnd = GetTopWindow( GetDesktopWindow() );
    WND *wndPtr;
    int result;

#ifdef DEBUG_ENUM
    printf("EnumWindows\n");
#endif 

    while (hwnd) {
      char *ptr;

        if ( !(wndPtr=WIN_FindWndPtr(hwnd)) ) {
              return 0;
      }
#ifdef DEBUG_ENUM
      if (XFetchName(display, wndPtr->window, &ptr) && ptr)
              printf("found a window (%s)\n", ptr);
      else 
              printf("found nameless parent window\n");
#endif
#ifdef WINELIB
      (*wndenumprc)(hwnd, lParam);
#else
      result = CallBack16(wndenumprc, 2, lParam, (int) hwnd);
#endif
      if ( ! result )  {
              return 0;
      }
      hwnd=wndPtr->hwndNext;
    }
    return 1; /* for now */
}

/*******************************************************************
 *    WIN_EnumChildWin
 *
 *   o hwnd is the first child to use, loop until all next windows
 *     are processed
 * 
 *   o call wdnenumprc with parameters in inverse order (pascal)
 *
 *   o call ourselves with the next child window
 * 
 */
static BOOL WIN_EnumChildWin(HWND hwnd, FARPROC wndenumprc, LPARAM lParam)
{
    WND *wndPtr;
    int result;


    while (hwnd) {
      char *ptr;
      if ( !(wndPtr=WIN_FindWndPtr(hwnd)) ) {
            return 0;
        }
#ifdef DEBUG_ENUM
      if (XFetchName(display, wndPtr->window, &ptr) && ptr)
              printf("EnumChild: found a child window (%s)\n", ptr);
      else 
              printf("EnumChild: nameless child\n");
      
        if (!(wndPtr->dwStyle & WS_CHILD)) {
           printf("this is not a child window!  What is it doing here?\n");
           return 0;
      }
#endif
#ifdef WINELIB
        if (!(*wndenumprc, 2, lParam, (int) hwnd)) {
#else
        if (!CallBack16(wndenumprc, 2, lParam, (int) hwnd)) {
#endif
                return 0;
      }
      if (!WIN_EnumChildWin(wndPtr->hwndChild, wndenumprc, lParam)) {
          return 0;
      }
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
BOOL EnumChildWindows(HWND hwnd, FARPROC wndenumprc, LPARAM lParam)
{
    WND *wndPtr;

#ifdef DEBUG_ENUM
    printf("EnumChildWindows\n");
#endif

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
	printf("EMPTY STUB !! AnyPopup !\n");
	return FALSE;
}

/*******************************************************************
 *			FlashWindow		[USER.105]
 */
BOOL FlashWindow(HWND hWnd, BOOL bInvert)
{
	printf("EMPTY STUB !! FlashWindow !\n");
	return FALSE;
}


/*******************************************************************
 *			SetSysModalWindow		[USER.188]
 */
HWND SetSysModalWindow(HWND hWnd)
{
	HWND hWndOldModal = hWndSysModal;
	hWndSysModal = hWnd;
	printf("EMPTY STUB !! SetSysModalWindow(%04X) !\n", hWnd);
	return hWndOldModal;
}


/*******************************************************************
 *			GetSysModalWindow		[USER.189]
 */
HWND GetSysModalWindow(void)
{
	return hWndSysModal;
}
