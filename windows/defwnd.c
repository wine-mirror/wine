/*
 * Default window procedure
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include "windows.h"
#include "win.h"
#include "class.h"
#include "user.h"


/***********************************************************************
 *           DefWindowProc   (USER.107)
 */
LONG DefWindowProc( HWND hwnd, WORD msg, WORD wParam, LONG lParam )
{
    WND * wndPtr;
    CLASS * classPtr;
    LPSTR textPtr;
    int len;
    
#ifdef DEBUG_MESSAGE
    printf( "DefWindowProc: %d %d %d %08x\n", hwnd, msg, wParam, lParam );
#endif

    switch(msg)
    {
    case WM_NCCREATE:
	{
	    CREATESTRUCT * createStruct = (CREATESTRUCT *)lParam;
	    wndPtr = WIN_FindWndPtr(hwnd);
	    if (createStruct->lpszName)
	    {
		  /* Allocate space for window text */
		wndPtr->hText = USER_HEAP_ALLOC(GMEM_MOVEABLE, 
					strlen(createStruct->lpszName) + 1);
		textPtr = (LPSTR)USER_HEAP_ADDR(wndPtr->hText);
		strcpy(textPtr, createStruct->lpszName);
	    }
	    return 1;
	}
	
    case WM_CREATE:
	return 0;

    case WM_NCDESTROY:
	{
	    wndPtr = WIN_FindWndPtr(hwnd);
	    if (wndPtr->hText) USER_HEAP_FREE(wndPtr->hText);
	    wndPtr->hText = 0;
	    return 0;
	}
	
    case WM_PAINT:
	{
	    PAINTSTRUCT paintstruct;
	    BeginPaint( hwnd, &paintstruct );
	    EndPaint( hwnd, &paintstruct );
	    return 0;
	}

    case WM_CLOSE:
	DestroyWindow( hwnd );
	return 0;

    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
	{
	    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 1;
	    if (!(classPtr = CLASS_FindClassPtr( wndPtr->hClass ))) return 1;
	    if (!classPtr->wc.hbrBackground) return 1;
	    FillWindow( GetParent(hwnd), hwnd, (HDC)wParam,
		        classPtr->wc.hbrBackground );
	    return 0;
	}

    case WM_GETDLGCODE:
	return 0;

    case WM_CTLCOLOR:
	{
	    if (HIWORD(lParam) == CTLCOLOR_SCROLLBAR)
	    {
		SetBkColor( (HDC)wParam, RGB(255, 255, 255) );
		SetTextColor( (HDC)wParam, RGB(0, 0, 0) );
/*	        hbr = sysClrObjects.hbrScrollbar;
		UnrealizeObject(hbr); */
		return GetStockObject(LTGRAY_BRUSH);
	    }
	    else
	    {
		SetBkColor( (HDC)wParam, GetSysColor(COLOR_WINDOW) );
		SetTextColor( (HDC)wParam, GetSysColor(COLOR_WINDOWTEXT) );
/*	        hbr = sysClrObjects.hbrWindow; */
		return GetStockObject(WHITE_BRUSH);
	    }
	}
	
    case WM_GETTEXT:
	{
	    if (wParam)
	    {
		wndPtr = WIN_FindWndPtr(hwnd);
		if (wndPtr->hText)
		{
		    textPtr = (LPSTR)USER_HEAP_ADDR(wndPtr->hText);
		    if ((int)wParam > (len = strlen(textPtr)))
		    {
			strcpy((char *)lParam, textPtr);
			return (DWORD)len;
		    }
		}
	        lParam = (DWORD)NULL;
	    }
	    return (0L);
	}

    case WM_GETTEXTLENGTH:
	{
	    wndPtr = WIN_FindWndPtr(hwnd);
	    if (wndPtr->hText)
	    {
		textPtr = (LPSTR)USER_HEAP_ADDR(wndPtr->hText);
		return (DWORD)strlen(textPtr);
	    }
	    return (0L);
	}

    case WM_SETTEXT:
	{
	    wndPtr = WIN_FindWndPtr(hwnd);
	    if (wndPtr->hText)
		USER_HEAP_FREE(wndPtr->hText);

	    wndPtr->hText = USER_HEAP_ALLOC(GMEM_MOVEABLE, 
					    strlen((LPSTR)lParam) + 1);
	    textPtr = (LPSTR)USER_HEAP_ADDR(wndPtr->hText);
	    strcpy(textPtr, (LPSTR)lParam);
	    if (wndPtr->shellWidget)
		XtVaSetValues( wndPtr->shellWidget, XtNtitle, textPtr, NULL );
	    return (0L);
	}
    }
    return 0;
}
