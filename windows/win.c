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
#include "heap.h"

extern Display * XT_display;


static HWND firstWindow = 0;

void BUTTON_CreateButton(LPSTR className, LPSTR buttonLabel, HWND hwnd);


/***********************************************************************
 *           WIN_FindWndPtr
 *
 * Return a pointer to the WND structure corresponding to a HWND.
 * The caller must GlobalUnlock the pointer.
 */
WND * WIN_FindWndPtr( HWND hwnd )
{
    WND * ptr;
    
    if (!hwnd) return NULL;
    ptr = (WND *) GlobalLock( hwnd );
    if (ptr->dwMagic != WND_MAGIC)
    {
	GlobalUnlock( hwnd );
	return NULL;
    }
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
 *           CreateWindow   (USER.41)
 */
HWND CreateWindow( LPSTR className, LPSTR windowName,
		   DWORD style, short x, short y, short width, short height,
		   HWND parent, HMENU menu, HANDLE instance, LPSTR data ) 
{
    HANDLE class, hwnd;
    CLASS *classPtr;
    WND *wndPtr, *parentPtr = NULL;
    CREATESTRUCT *createStruct;
    HANDLE hcreateStruct;
    int wmcreate;
    Widget parentWidget = 0;

#ifdef DEBUG_WIN
    printf( "CreateWindow: %s %s %d,%d %dx%d\n", className, windowName, x, y, width, height );
#endif

    if (x == CW_USEDEFAULT) x = 0;
    if (y == CW_USEDEFAULT) y = 0;
    if (width == CW_USEDEFAULT) width = 600;
    if (height == CW_USEDEFAULT) height = 400;

      /* Find the parent and class */

    if (parent) 
    {
	  /* Check if parent is valid */
	parentPtr = WIN_FindWndPtr( parent );
	if (!parent) return 0;
    }
    else if (style & WS_CHILD) return 0;  /* WS_CHILD needs a parent */

    if (!(class = CLASS_FindClassByName( className, &classPtr )))
    {
	GlobalUnlock( parent );
	return 0;
    }
    
      /* Create the window structure */

    hwnd = GlobalAlloc( GMEM_MOVEABLE, sizeof(WND)+classPtr->wc.cbWndExtra );
    if (!hwnd)
    {
	GlobalUnlock( parent );
	return 0;
    }

      /* Fill the structure */

    wndPtr = (WND *) GlobalLock( hwnd );
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
    wndPtr->hDCE              = 0;
    wndPtr->hmenuSystem       = 0;
    wndPtr->wIDmenu           = menu;
    wndPtr->flags             = 0;
    if (classPtr->wc.cbWndExtra)
	memset( wndPtr->wExtra, 0, classPtr->wc.cbWndExtra );
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
    
      /* Create the widgets */

    if (!strcasecmp(className, "BUTTON"))
    {
	BUTTON_CreateButton(className, windowName, hwnd);
    }
    else
    {
	if (style & WS_CHILD)
	{
	    wndPtr->shellWidget = 0;
	    wndPtr->winWidget = XtVaCreateManagedWidget(className,
						    coreWidgetClass,
						    parentPtr->winWidget,
						    XtNx, x,
						    XtNy, y,
						    XtNwidth, width,
						    XtNheight, height,
						    NULL );
	}
	else
	{
	    wndPtr->shellWidget = XtVaAppCreateShell(className, 
						 windowName,
						 topLevelShellWidgetClass,
						 XT_display,
						 XtNx, x,
						 XtNy, y,
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
    }

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

    wmcreate = CallWindowProc( wndPtr->lpfnWndProc, hwnd, 
			       WM_CREATE, 0, (LONG) createStruct );
    GlobalUnlock( hcreateStruct );
    GlobalFree( hcreateStruct );
    
    if (wmcreate == -1)
    {
	  /* Abort window creation */
	if (wndPtr->shellWidget) XtDestroyWidget( wndPtr->shellWidget );
	else XtDestroyWidget( wndPtr->winWidget );
	GlobalUnlock( parent );
	GlobalUnlock( hwnd );
	GlobalFree( hwnd );
	return 0;
    }
    
    EVENT_AddHandlers( wndPtr->winWidget, hwnd );

    if (style & WS_VISIBLE) ShowWindow( hwnd, SW_SHOW );
    
    GlobalUnlock( parent );
    GlobalUnlock( hwnd );
    return hwnd;
}

/***********************************************************************
 *           DestroyWindow   (USER.53)
 */
BOOL DestroyWindow( HWND hwnd )
{
    WND *wndPtr, *parentPtr;
    CLASS * classPtr;
    
    wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    if (wndPtr->hwndParent)
    {
	parentPtr = WIN_FindWndPtr( wndPtr->hwndParent );
	printf( "INTERNAL ERROR: DestroyWindow: Invalid window parent\n" );
	return FALSE;
    }
    
    classPtr = CLASS_FindClassPtr( wndPtr->hClass );
    if (!classPtr)
    {
	printf( "INTERNAL ERROR: DestroyWindow: Invalid window class\n" );
	return FALSE;
    }
    
    SendMessage( hwnd, WM_DESTROY, 0, 0 );
    
      /* Destroy all children */

    /* ........... */

      /* Remove the window from the linked list */
    
    /* ........... */

      /* Destroy the window */

    if (wndPtr->shellWidget) XtDestroyWidget( wndPtr->shellWidget );
    else XtDestroyWidget( wndPtr->winWidget );
    classPtr->cWindows--;
    if (wndPtr->hwndParent) GlobalUnlock( wndPtr->hwndParent );
    GlobalUnlock( hwnd );
    GlobalFree( hwnd );
    return TRUE;
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
	GlobalUnlock( hwnd );
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
	GlobalUnlock( hwnd );
	XtVaGetValues(wndPtr->winWidget, 
		      XtNwidth, &width,
		      XtNheight, &height,
		      NULL );
	SendMessage( hwnd, WM_SIZE, SIZE_RESTORED, 
		     (width & 0xffff) | (height << 16) );
	SendMessage( hwnd, WM_SHOWWINDOW, TRUE, 0 );
    }
    return TRUE;
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
    WND *wndPtr;
    HMENU hmenu;
    
    wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr == NULL)
	return 0;

    hmenu = wndPtr->wIDmenu;
    
    GlobalUnlock(hwnd);
    return hmenu;
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

    if (wndPtr->dwStyle & WS_CHILD)
    {
	GlobalUnlock(hwnd);
	return FALSE;
    }

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
	
	GlobalUnlock(hwnd);
	return FALSE;
    }

    GlobalUnlock(hwnd);
    return TRUE;
}
