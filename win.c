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

extern Widget XT_topLevelWidget;


static HWND firstWindow = 0;


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
 *           CreateWindow   (USER.41)
 */
HWND CreateWindow( LPSTR className, LPSTR windowName,
		   DWORD style, int x, int y, int width, int height,
		   HWND parent, HMENU menu, HANDLE instance, LPSTR data ) 
{
    HANDLE class, hwnd;
    CLASS *classPtr;
    WND *wndPtr, *parentPtr = NULL;
    CREATESTRUCT createStruct;
    Widget parentWidget = 0;

    printf( "CreateWindow: %s\n", windowName );

    if (x == CW_USEDEFAULT) x = 0;
    if (y == CW_USEDEFAULT) y = 0;
    if (width == CW_USEDEFAULT) width = 600;
    if (height == CW_USEDEFAULT) height = 400;

      /* Find the parent and class */

    if (parent) 
    {
	  /* Check if parent is valid */
	parentPtr = WIN_FindWndPtr( parent );
	if (!parentPtr) return 0;
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
	GlobalUnlock( class );
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
    
      /* Fill the CREATESTRUCT */

    createStruct.lpCreateParams = data;
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

      /* Create the widgets */

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
						 XtDisplay(XT_topLevelWidget),
						 XtNx, x,
						 XtNy, y,
						 NULL );
    	wndPtr->winWidget = XtVaCreateManagedWidget(className,
						    compositeWidgetClass,
						    wndPtr->shellWidget,
						    XtNwidth, width,
						    XtNheight, height,
						    NULL );
    }

      /* Send the WM_CREATE message */

    if (CallWindowProc( wndPtr->lpfnWndProc, hwnd, 
		        WM_CREATE, 0, (LONG) &createStruct ) == -1)
    {
	  /* Abort window creation */
	if (wndPtr->shellWidget) XtDestroyWidget( wndPtr->shellWidget );
	else XtDestroyWidget( wndPtr->winWidget );
	GlobalUnlock( parent );
	GlobalUnlock( class );
	GlobalUnlock( hwnd );
	GlobalFree( hwnd );
	return 0;
    }
    
    EVENT_AddHandlers( wndPtr->winWidget, hwnd );
    
    GlobalUnlock( parent );
    GlobalUnlock( class );
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
    GlobalUnlock( wndPtr->hClass );
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
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    rect->left = rect->top = rect->right = rect->bottom = 0;
    if (wndPtr) 
    {
	XtVaGetValues(wndPtr->winWidget,
		      XtNwidth, &rect->right,
		      XtNheight, &rect->bottom,
		      NULL );
	GlobalUnlock( hwnd );
    }
}


/***********************************************************************
 *           ShowWindow   (USER.42)
 */
BOOL ShowWindow( HWND hwnd, int cmd ) 
{    
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (wndPtr) 
    {
	if (wndPtr->shellWidget) XtRealizeWidget( wndPtr->shellWidget );
	GlobalUnlock( hwnd );
    }
    return TRUE;
}


/***********************************************************************
 *           UpdateWindow   (USER.124)
 */
void UpdateWindow( HWND hwnd )
{
    SendMessage( hwnd, WM_PAINT, 0, 0 );
}


