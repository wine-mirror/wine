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
extern Colormap COLOR_WinColormap;

static HWND firstWindow = 0;

void BUTTON_CreateButton(LPSTR className, LPSTR buttonLabel, HWND hwnd);


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
    LPSTR textPtr;

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
    wndPtr->dwExStyle         = 0;
    wndPtr->hmenuSystem       = 0;
    wndPtr->wIDmenu           = menu;
    wndPtr->flags             = 0;

    if (classPtr->wc.cbWndExtra)
	memset( wndPtr->wExtra, 0, classPtr->wc.cbWndExtra );
    if (classPtr->wc.style & CS_OWNDC)
	wndPtr->hdc = CreateDC( "DISPLAY", NULL, NULL, NULL);
    else wndPtr->hdc = 0;
    classPtr->cWindows++;

      /* Create buffer for window text */
    wndPtr->hText = USER_HEAP_ALLOC(GMEM_MOVEABLE, strlen(windowName) + 1);
    textPtr = (LPSTR)USER_HEAP_ADDR(wndPtr->hText);
    strcpy(textPtr, windowName);

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

    if (style & WS_CHILD)
    {
	wndPtr->shellWidget = 0;
	if (style & (WS_BORDER | WS_DLGFRAME | WS_THICKFRAME))
	{ 
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
	    wndPtr->winWidget = XtVaCreateManagedWidget(className,
						    coreWidgetClass,
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
	wndPtr->shellWidget = XtVaAppCreateShell(className, 
						 windowName,
						 topLevelShellWidgetClass,
						 XT_display,
						 XtNx, x,
						 XtNy, y,
#ifdef USE_PRIVATE_MAP
						 XtNcolormap, COLOR_WinColormap,
#endif
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
	USER_HEAP_FREE( hwnd );
	return 0;
    }
    
    EVENT_AddHandlers( wndPtr->winWidget, hwnd );

    if (style & WS_VISIBLE) ShowWindow( hwnd, SW_SHOW );
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
    if (wndPtr->hdc) DeleteDC( wndPtr->hdc );
    classPtr->cWindows--;
    USER_HEAP_FREE(wndPtr->hText);
    USER_HEAP_FREE( hwnd );
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
	case GWL_WNDPROC: return wndPtr->lpfnWndProc;
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
	case GWL_WNDPROC: ptr = &wndPtr->lpfnWndProc;
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
    HWND hwndParent = wndPtr->hwndParent;
    GlobalUnlock(hwnd);
    return hwndParent;
}

/****************************************************************
 *         GetDlgCtrlID           (USER.277)
 */

int GetDlgCtrlID(HWND hwnd)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    int ctrlID = wndPtr->wIDmenu;
    GlobalUnlock(hwnd);
    return ctrlID;
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
 *         GetWindowTextLength    (USER.38)
 */

int GetWindowTextLength(HWND hwnd)
{
    return (int)SendMessage(hwnd, WM_GETTEXTLENGTH, (WORD)NULL, 
			                            (DWORD)NULL);
}


