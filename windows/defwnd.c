/*
 * Default window procedure
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#ifndef USE_XLIB
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#endif

#include "windows.h"
#include "win.h"
#include "class.h"
#include "user.h"

extern Display * XT_display;

/***********************************************************************
 *           DefWindowProc   (USER.107)
 */
LONG DefWindowProc( HWND hwnd, WORD msg, WORD wParam, LONG lParam )
{
    CLASS * classPtr;
    LPSTR textPtr;
    int len;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    
#ifdef DEBUG_MESSAGE
    printf( "DefWindowProc: %d %d %d %08x\n", hwnd, msg, wParam, lParam );
#endif

    switch(msg)
    {
    case WM_NCCREATE:
	{
	    CREATESTRUCT * createStruct = (CREATESTRUCT *)lParam;
	    if (createStruct->lpszName)
	    {
		  /* Allocate space for window text */
		wndPtr->hText = USER_HEAP_ALLOC(GMEM_MOVEABLE, 
					strlen(createStruct->lpszName) + 2);
		textPtr = (LPSTR)USER_HEAP_ADDR(wndPtr->hText);
		strcpy(textPtr, createStruct->lpszName);
		*(textPtr + strlen(createStruct->lpszName) + 1) = '\0';
		                         /* for use by edit control */
	    }
	    return 1;
	}

    case WM_NCCALCSIZE:
	{
#ifdef USE_XLIB
	    NCCALCSIZE_PARAMS *params = (NCCALCSIZE_PARAMS *)lParam;
	    if (wndPtr->dwStyle & WS_CHILD)
	    {
		if (wndPtr->dwStyle & WS_BORDER)
		{
		    params->rgrc[0].left   += 1; /* SM_CXBORDER */
		    params->rgrc[0].top    += 1; /* SM_CYBORDER */
		    params->rgrc[0].right  -= 1; /* SM_CXBORDER */
		    params->rgrc[0].bottom -= 1; /* SM_CYBORDER */
		}
	    }
	    else
	    {
		params->rgrc[0].left   += 4; /* SM_CXFRAME */
		params->rgrc[0].top    += 30; /* SM_CYFRAME+SM_CYCAPTION */
		params->rgrc[0].right  -= 4; /* SM_CXFRAME */
		params->rgrc[0].bottom -= 4; /* SM_CYFRAME */
		if (wndPtr->dwStyle & WS_VSCROLL)
		{
		    params->rgrc[0].right -= 16; /* SM_CXVSCROLL */
		}
		if (wndPtr->dwStyle & WS_HSCROLL)
		{
		    params->rgrc[0].bottom += 16;  /* SM_CYHSCROLL */
		}
	    }
#endif
	    return 0;
	}
	
    case WM_CREATE:
	return 0;

    case WM_NCDESTROY:
	{
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

    case WM_WINDOWPOSCHANGED:
	{
	    WINDOWPOS * winPos = (WINDOWPOS *)lParam;
	    if (!(winPos->flags & SWP_NOMOVE))
		SendMessage( hwnd, WM_MOVE, 0,
		             MAKELONG( wndPtr->rectClient.left,
				       wndPtr->rectClient.top ));
	    if (!(winPos->flags & SWP_NOSIZE))
		SendMessage( hwnd, WM_SIZE, SIZE_RESTORED,
		   MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
			    wndPtr->rectClient.bottom-wndPtr->rectClient.top));
	    return 0;
	}

    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
	{
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
	    if (wndPtr->hText)
	    {
		textPtr = (LPSTR)USER_HEAP_ADDR(wndPtr->hText);
		return (DWORD)strlen(textPtr);
	    }
	    return (0L);
	}

    case WM_SETTEXT:
	{
	    if (wndPtr->hText)
		USER_HEAP_FREE(wndPtr->hText);

	    wndPtr->hText = USER_HEAP_ALLOC(GMEM_MOVEABLE, 
					    strlen((LPSTR)lParam) + 1);
	    textPtr = (LPSTR)USER_HEAP_ADDR(wndPtr->hText);
	    strcpy(textPtr, (LPSTR)lParam);
#ifdef USE_XLIB
	    XStoreName( XT_display, wndPtr->window, textPtr );
#else
	    if (wndPtr->shellWidget)
		XtVaSetValues( wndPtr->shellWidget, XtNtitle, textPtr, NULL );
#endif
	    return (0L);
	}
    case WM_SETCURSOR:
	if (wndPtr->hCursor != (HCURSOR)NULL)
	    SetCursor(wndPtr->hCursor);
	return 0L;
    }
    return 0;
}
