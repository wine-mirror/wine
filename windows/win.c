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
#include <X11/Xaw/Box.h>

#include "class.h"
#include "win.h"
#include "user.h"

extern Display * XT_display;
extern Screen * XT_screen;
extern Colormap COLOR_WinColormap;

static HWND firstWindow = 0;

void SCROLLBAR_CreateScrollBar(LPSTR className, LPSTR Label, HWND hwnd);
void LISTBOX_CreateListBox(LPSTR className, LPSTR Label, HWND hwnd);
void COMBOBOX_CreateComboBox(LPSTR className, LPSTR Label, HWND hwnd);

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
 *           WIN_FindWinToRepaint
 *
 * Find a window that needs repaint.
 */
HWND WIN_FindWinToRepaint( HWND hwnd )
{
    WND * wndPtr;
    
    if (!hwnd) hwnd = firstWindow;
    while (hwnd)
    {
	if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
	if (wndPtr->hrgnUpdate) return hwnd;
	if (wndPtr->hwndChild)
	{
	    HWND child;
	    if ((child = WIN_FindWinToRepaint( wndPtr->hwndChild )))
		return child;
	}
	hwnd = wndPtr->hwndNext;
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

#ifdef DEBUG_WIN
    printf( "CreateWindowEx: %s %s %d,%d %dx%d\n", className, windowName, x, y, width, height );
#endif

    if (x == CW_USEDEFAULT) x = 0;
    if (y == CW_USEDEFAULT) y = 0;
    if (width == CW_USEDEFAULT) width = 600;
    if (height == CW_USEDEFAULT) height = 400;
    if (!width) width = 1;
    if (!height) height = 1;

      /* Find the parent and class */

    if (parent) 
    {
	  /* Check if parent is valid */
	parentPtr = WIN_FindWndPtr( parent );
	if (!parent) return 0;
    }
    else if (style & WS_CHILD) return 0;  /* WS_CHILD needs a parent */

    if (!(class = CLASS_FindClassByName( className, &classPtr )))
	return 0;
    
      /* Create the window structure */

    hwnd = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(WND)+classPtr->wc.cbWndExtra);
    if (!hwnd) return 0;

      /* Fill the structure */

    wndPtr = (WND *) USER_HEAP_ADDR( hwnd );
    wndPtr->hwndNext   = 0;
    wndPtr->hwndChild  = 0;
    wndPtr->dwMagic    = WND_MAGIC;
    wndPtr->hwndParent = parent;
    wndPtr->hwndOwner  = parent;  /* What else? */
    wndPtr->hClass     = class;
    wndPtr->hInstance  = instance;
    wndPtr->rectClient.left   = x;
    wndPtr->rectClient.top    = y;
    wndPtr->rectClient.right  = x + width;
    wndPtr->rectClient.bottom = y + height;
    wndPtr->rectWindow        = wndPtr->rectClient;
    wndPtr->hrgnUpdate        = 0;
    wndPtr->hwndLastActive    = 0;
    wndPtr->lpfnWndProc       = classPtr->wc.lpfnWndProc;
    wndPtr->dwStyle           = style;
    wndPtr->dwExStyle         = exStyle;
    wndPtr->hmenuSystem       = 0;
    wndPtr->wIDmenu           = menu;
    wndPtr->hText             = 0;
    wndPtr->flags             = 0;

    if (classPtr->wc.cbWndExtra)
	memset( wndPtr->wExtra, 0, classPtr->wc.cbWndExtra );
    if (classPtr->wc.style & CS_OWNDC)
	wndPtr->hdc = CreateDC( "DISPLAY", NULL, NULL, NULL);
    else wndPtr->hdc = 0;
    classPtr->cWindows++;

      /* Insert the window in the linked list */

    if (parent)
    {
	wndPtr->hwndNext = parentPtr->hwndChild;
	parentPtr->hwndChild = hwnd;
    }
    else  /* Top-level window */
    {
	wndPtr->hwndNext = firstWindow;
	firstWindow = hwnd;
    }
    
    if (!strcasecmp(className, "SCROLLBAR"))
    {
	SCROLLBAR_CreateScrollBar(className, windowName, hwnd);
	goto WinCreated;
    }
    if (!strcasecmp(className, "LISTBOX"))
    {
	LISTBOX_CreateListBox(className, windowName, hwnd);
	goto WinCreated;
    }
    if (!strcasecmp(className, "COMBOBOX"))
    {
	COMBOBOX_CreateComboBox(className, windowName, hwnd);
	goto WinCreated;
    }
      /* Create the widgets */

    if (style & WS_CHILD)
    {
	wndPtr->shellWidget = 0;
	if (style & (WS_BORDER | WS_DLGFRAME | WS_THICKFRAME))
	{ 
	    int borderCol = 0;
	    if (COLOR_WinColormap == DefaultColormapOfScreen(XT_screen))
		borderCol = BlackPixelOfScreen(XT_screen);
	    wndPtr->winWidget = XtVaCreateManagedWidget(className,
						    compositeWidgetClass,
						    parentPtr->winWidget,
						    XtNx, x,
						    XtNy, y,
						    XtNwidth, width,
						    XtNheight, height,
						    XtNborderColor, borderCol,
						    NULL );
	}
	else
	{
	    wndPtr->winWidget = XtVaCreateManagedWidget(className,
						    compositeWidgetClass,
						    parentPtr->winWidget,
						    XtNx, x,
						    XtNy, y,
						    XtNwidth, width,
						    XtNheight, height,
						    XtNborderWidth, 0,
						    NULL );
	}
    }
    else
    {
	wndPtr->shellWidget = XtVaAppCreateShell(windowName, 
						 className,
						 topLevelShellWidgetClass,
						 XT_display,
						 XtNx, x,
						 XtNy, y,
						 XtNcolormap, COLOR_WinColormap,
						 NULL );
	wndPtr->compositeWidget = XtVaCreateManagedWidget(className,
						    formWidgetClass,
						    wndPtr->shellWidget,
						    NULL );
	if (wndPtr->wIDmenu == 0)
	{
	    wndPtr->menuBarPtr = 
		    MENU_CreateMenuBar(wndPtr->compositeWidget,
				       instance, hwnd,
				       classPtr->wc.lpszMenuName,
				       width);
	    if (wndPtr->menuBarPtr)
		    wndPtr->wIDmenu = 
			GlobalHandleFromPointer(wndPtr->menuBarPtr->firstItem);
	}
	else
	{
	    wndPtr->menuBarPtr = MENU_UseMenu(wndPtr->compositeWidget,
						  instance, hwnd,
						  wndPtr->wIDmenu, width);
	}

	if (wndPtr->menuBarPtr != NULL)
	{
	    wndPtr->winWidget = 
		    XtVaCreateManagedWidget(className,
					    compositeWidgetClass,
					    wndPtr->compositeWidget,
					    XtNwidth, width,
					    XtNheight, height,
					    XtNfromVert,
					    wndPtr->menuBarPtr->menuBarWidget,
					    XtNvertDistance, 4,
					    NULL );
	}
	else
	{
	    wndPtr->winWidget = 
		    XtVaCreateManagedWidget(className,
					compositeWidgetClass,
					wndPtr->compositeWidget,
					XtNwidth, width,
					XtNheight, height,
					NULL );
	}
    }

WinCreated:

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
    else wmcreate = SendMessage( hwnd, WM_CREATE, 0, (LONG)createStruct );

    GlobalUnlock( hcreateStruct );
    GlobalFree( hcreateStruct );
    
    if (wmcreate == -1)
    {
	  /* Abort window creation */
	if (wndPtr->shellWidget) XtDestroyWidget( wndPtr->shellWidget );
	else XtDestroyWidget( wndPtr->winWidget );
	USER_HEAP_FREE( hwnd );
	return 0;
    }
    
    EVENT_AddHandlers( wndPtr->winWidget, hwnd );

    if (style & WS_VISIBLE) ShowWindow( hwnd, SW_SHOW );
    WIN_SendParentNotify( hwnd, wndPtr, WM_CREATE );
    return hwnd;
}

/***********************************************************************
 *           DestroyWindow   (USER.53)
 */
BOOL DestroyWindow( HWND hwnd )
{
    WND * wndPtr;
    HWND * curWndPtr;
    CLASS * classPtr;
    
      /* Initialisation */

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (!(classPtr = CLASS_FindClassPtr( wndPtr->hClass ))) return FALSE;
    WIN_SendParentNotify( hwnd, wndPtr, WM_DESTROY );

      /* Send destroy messages */

    SendMessage( hwnd, WM_DESTROY, 0, 0 );
    SendMessage( hwnd, WM_NCDESTROY, 0, 0 );
    
      /* Destroy all children */

    while (wndPtr->hwndChild)  /* The child removes itself from the list */
	DestroyWindow( wndPtr->hwndChild );

      /* Remove the window from the linked list */

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

      /* Destroy the window */

    if (wndPtr->shellWidget) XtDestroyWidget( wndPtr->shellWidget );
    else XtDestroyWidget( wndPtr->winWidget );
    if (wndPtr->hdc) DeleteDC( wndPtr->hdc );
    classPtr->cWindows--;
    USER_HEAP_FREE( hwnd );
    return TRUE;
}


/***********************************************************************
 *           GetWindowRect   (USER.32)
 */
void GetWindowRect( HWND hwnd, LPRECT rect ) 
{
    int x, y, width, height;  
    WND * wndPtr = WIN_FindWndPtr( hwnd ); 

    if (wndPtr) 
    {
	XtVaGetValues(wndPtr->winWidget,
		      XtNx, &x, XtNy, &y,
		      XtNwidth, &width,
		      XtNheight, &height,
		      NULL );
	rect->left  = x & 0xffff;
	rect->top = y & 0xffff;
	rect->right  = width & 0xffff;
	rect->bottom = height & 0xffff;
    }
}


/***********************************************************************
 *           GetClientRect   (USER.33)
 */
void GetClientRect( HWND hwnd, LPRECT rect ) 
{
    int width, height;
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    rect->left = rect->top = rect->right = rect->bottom = 0;
    if (wndPtr) 
    {
	XtVaGetValues(wndPtr->winWidget,
		      XtNwidth, &width,
		      XtNheight, &height,
		      NULL );
	rect->right  = width & 0xffff;
	rect->bottom = height & 0xffff;
    }
}


/***********************************************************************
 *           ShowWindow   (USER.42)
 */
BOOL ShowWindow( HWND hwnd, int cmd ) 
{    
    int width, height;
    
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (wndPtr) 
    {
	if (wndPtr->shellWidget) XtRealizeWidget( wndPtr->shellWidget );
	XtVaGetValues(wndPtr->winWidget, 
		      XtNwidth, &width,
		      XtNheight, &height,
		      NULL );
	SendMessage( hwnd, WM_SIZE, SIZE_RESTORED, 
		     (width & 0xffff) | (height << 16) );
	SendMessage( hwnd, WM_SHOWWINDOW, TRUE, 0 );
/*
	printf("ShowWindow(%X, %X); !\n", hwnd, cmd);
*/
	switch(cmd)
	    {
	    case SW_HIDE:
		XtSetMappedWhenManaged(wndPtr->winWidget, FALSE);
		break;
	    case SW_SHOWNA:
	    case SW_SHOWMINNOACTIVE:
	    case SW_SHOWNOACTIVATE:
	    case SW_MINIMIZE:
	    case SW_MAXIMIZE:
	    case SW_SHOWMAXIMIZED:
	    case SW_SHOWMINIMIZED:
	    case SW_SHOW:
	    case SW_NORMAL:
	    case SW_SHOWNORMAL:
		XtSetMappedWhenManaged(wndPtr->winWidget, TRUE);
		break;
	    default:
		break;
	    }
    }
    return TRUE;
}


/***********************************************************************
 *           MoveWindow   (USER.56)
 */
void MoveWindow(HWND hWnd, short x, short y, short w, short h, BOOL bRepaint)
{    
    WND * wndPtr = WIN_FindWndPtr( hWnd );
    if (wndPtr) 
    {
	wndPtr->rectClient.left   = x;
	wndPtr->rectClient.top    = y;
	wndPtr->rectClient.right  = x + w;
	wndPtr->rectClient.bottom = y + h;
	XtVaSetValues(wndPtr->winWidget, XtNx, x, XtNy, y,
		      XtNwidth, w, XtNheight, h, NULL );
	SendMessage(hWnd, WM_MOVE, 0, MAKELONG(x, y)); 
	printf("MoveWindow(%X, %d, %d, %d, %d, %d); !\n", 
			hWnd, x, y, w, h, bRepaint);
	if (bRepaint) {
	    InvalidateRect(hWnd, NULL, TRUE);
	    UpdateWindow(hWnd);
	    }
    }
}


/***********************************************************************
 *           UpdateWindow   (USER.124)
 */
void UpdateWindow( HWND hwnd )
{
    if (GetUpdateRect( hwnd, NULL, FALSE )) 
	SendMessage( hwnd, WM_PAINT, 0, 0 );
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
    WND *wndPtr;
    
    wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr == NULL)
	return FALSE;

    if (wndPtr->dwStyle & WS_CHILD) return FALSE;

    if (wndPtr->menuBarPtr != NULL)
    {
	XtVaSetValues(wndPtr->winWidget, XtNfromVert, NULL, NULL);
	MENU_CollapseMenu(wndPtr->menuBarPtr);
    }
    
    wndPtr->menuBarPtr = MENU_UseMenu(wndPtr->compositeWidget, 
				      wndPtr->hInstance, hwnd, hmenu, 
				      wndPtr->rectClient.right -
				      wndPtr->rectClient.left);

    if (wndPtr->menuBarPtr != NULL)
    {
	XtVaSetValues(wndPtr->winWidget, 
		      XtNfromVert, wndPtr->menuBarPtr->menuBarWidget, 
		      XtNvertDistance, 4,
		      NULL);
    }
    else
    {
	if (wndPtr->wIDmenu != 0)
	{
	    wndPtr->menuBarPtr = MENU_UseMenu(wndPtr->compositeWidget, 
					      wndPtr->hInstance, hwnd, 
					      wndPtr->wIDmenu, 
					      wndPtr->rectClient.right -
					      wndPtr->rectClient.left);
	}
	return FALSE;
    }

    return TRUE;
}


/***********************************************************************
 *           SetWindowPos   (USER.232)
 */
void SetWindowPos(HWND hWnd, HWND hWndInsertAfter, short x, short y, short w, short h, WORD wFlag)
{    
    WND * wndPtr = WIN_FindWndPtr( hWnd );
    if (wndPtr) 
    {
	if ((wFlag & SWP_NOMOVE) == 0) {
	    wndPtr->rectClient.left   = x;
	    wndPtr->rectClient.top    = y;
	    XtVaSetValues(wndPtr->winWidget, XtNx, x, XtNy, y, NULL );
	    }
	if ((wFlag & SWP_NOSIZE) == 0) {
	    wndPtr->rectClient.right  = x + w;
	    wndPtr->rectClient.bottom = y + h;
	    XtVaSetValues(wndPtr->winWidget, XtNwidth, w, XtNheight, h, NULL );
	    }
	if ((wFlag & SWP_NOREDRAW) == 0) {
	    InvalidateRect(hWnd, NULL, TRUE);
	    UpdateWindow(hWnd);
	    }
	if ((wFlag & SWP_HIDEWINDOW) == SWP_HIDEWINDOW) 
	    ShowWindow(hWnd, SW_HIDE);
	if ((wFlag & SWP_SHOWWINDOW) == SWP_SHOWWINDOW) 
	    ShowWindow(hWnd, SW_SHOW);
/*
	if ((wFlag & SWP_NOACTIVATE) == 0) 
	    SetActiveWindow(hWnd);
*/
	printf("SetWindowPos(%X, %X, %d, %d, %d, %d, %X); !\n", 
		hWnd, hWndInsertAfter, x, y, w, h, wFlag);
    }
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


/*****************************************************************
 *         GetParent              (USER.46)
 */
HWND GetParent(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    return wndPtr->hwndParent;
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
