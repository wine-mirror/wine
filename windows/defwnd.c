/*
 * Default window procedure
 *
 * Copyright 1993 Alexandre Julliard

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";
*/

#include <stdlib.h>
#include <stdio.h>
#include "windows.h"
#include "win.h"
#include "class.h"
#include "user.h"
#include "nonclient.h"
#include "winpos.h"
#include "syscolor.h"
#include "stddebug.h"
/* #define DEBUG_MESSAGE */
#include "debug.h"

  /* Last COLOR id */
#define COLOR_MAX   COLOR_BTNHIGHLIGHT

/***********************************************************************
 *           DEFWND_SetText
 *
 * Set the window text.
 */
void DEFWND_SetText( HWND hwnd, LPSTR text )
{
    LPSTR textPtr;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (wndPtr->hText) USER_HEAP_FREE( wndPtr->hText );
    wndPtr->hText = USER_HEAP_ALLOC( LMEM_MOVEABLE, strlen(text) + 1 );
    textPtr = (LPSTR) USER_HEAP_ADDR( wndPtr->hText );
    strcpy( textPtr, text );
}

#include <assert.h>

/***********************************************************************
 *           DefWindowProc   (USER.107)
 */
LONG DefWindowProc( HWND hwnd, WORD msg, WORD wParam, LONG lParam )
{
	MEASUREITEMSTRUCT *measure;
	CLASS * classPtr;
	LPSTR textPtr;
	int len;
	WND * wndPtr = WIN_FindWndPtr( hwnd );
    
    dprintf_message(stddeb, "DefWindowProc: %d %d %d %08lx\n", 
		    hwnd, msg, wParam, lParam );

    switch(msg)
    {
    case WM_NCCREATE:
	{
	    CREATESTRUCT * createStruct = (CREATESTRUCT *)lParam;
	    if (createStruct->lpszName)
		DEFWND_SetText( hwnd, createStruct->lpszName );
	    return 1;
	}

    case WM_NCCALCSIZE:
	return NC_HandleNCCalcSize( hwnd, (NCCALCSIZE_PARAMS *)lParam );

    case WM_PAINTICON: 
    case WM_NCPAINT:
	return NC_HandleNCPaint( hwnd, (HRGN)wParam );

    case WM_NCHITTEST:
	return NC_HandleNCHitTest( hwnd, MAKEPOINT(lParam) );

    case WM_NCLBUTTONDOWN:
	return NC_HandleNCLButtonDown( hwnd, wParam, lParam );

    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
	return NC_HandleNCLButtonDblClk( hwnd, wParam, lParam );

    case WM_NCACTIVATE:
	return NC_HandleNCActivate( hwnd, wParam );

    case WM_NCDESTROY:
	if (wndPtr->hText) USER_HEAP_FREE(wndPtr->hText);
	if (wndPtr->hVScroll) USER_HEAP_FREE(wndPtr->hVScroll);
	if (wndPtr->hHScroll) USER_HEAP_FREE(wndPtr->hHScroll);
	wndPtr->hText = wndPtr->hVScroll = wndPtr->hHScroll = 0;
	return 0;
	
    case WM_PAINT:
	{
	    PAINTSTRUCT paintstruct;
	    BeginPaint( hwnd, &paintstruct );
	    EndPaint( hwnd, &paintstruct );
	    return 0;
	}

    case WM_SETREDRAW:
        if (!wParam)
        {
            ValidateRect( hwnd, NULL );
            wndPtr->flags |= WIN_NO_REDRAW;
        }
        else wndPtr->flags &= ~WIN_NO_REDRAW;
        return 0;

    case WM_CLOSE:
	DestroyWindow( hwnd );
	return 0;

    case WM_MOUSEACTIVATE:
	if (wndPtr->dwStyle & WS_CHILD)
	{
	    LONG ret = SendMessage( wndPtr->hwndParent, WM_MOUSEACTIVATE,
				    wParam, lParam );
	    if (ret) return ret;
	}
	return MA_ACTIVATE;

    case WM_ACTIVATE:
	if (wParam) SetFocus( hwnd );
	break;

    case WM_WINDOWPOSCHANGING:
	return WINPOS_HandleWindowPosChanging( (WINDOWPOS *)lParam );

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
            if (classPtr->wc.hbrBackground <= COLOR_MAX+1)
            {
                 HBRUSH hbrush;
                 hbrush = CreateSolidBrush(
                     GetSysColor(classPtr->wc.hbrBackground-1));
                 FillWindow( GetParent(hwnd), hwnd, (HDC)wParam, hbrush);
                 DeleteObject (hbrush);
            }
            else
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
		UnrealizeObject( sysColorObjects.hbrushScrollbar );
		return sysColorObjects.hbrushScrollbar;
	    }
	    else
	    {
		SetBkColor( (HDC)wParam, GetSysColor(COLOR_WINDOW) );
		SetTextColor( (HDC)wParam, GetSysColor(COLOR_WINDOWTEXT) );
		return sysColorObjects.hbrushWindow;
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
	DEFWND_SetText( hwnd, (LPSTR)lParam );
	NC_HandleNCPaint( hwnd, (HRGN)1 );  /* Repaint caption */
	return 0;

    case WM_SETCURSOR:
	if (wndPtr->dwStyle & WS_CHILD)
	    if (SendMessage(wndPtr->hwndParent, WM_SETCURSOR, wParam, lParam))
		return TRUE;
	return NC_HandleSetCursor( hwnd, wParam, lParam );

    case WM_SYSCOMMAND:
	return NC_HandleSysCommand( hwnd, wParam, MAKEPOINT(lParam) );

    case WM_SYSKEYDOWN:
	if (wParam == VK_MENU)
	{   /* Send to WS_OVERLAPPED parent. TODO: Handle MDI */
	    HWND top;
            for(top=hwnd;GetParent(top)!=0;top=GetParent(top));
	    SendMessage( top, WM_SYSCOMMAND, SC_KEYMENU, 0L );
	}
	break;

    case WM_SYSKEYUP:
		break;    	
    case WM_MEASUREITEM:
		measure = (MEASUREITEMSTRUCT *)lParam;
		switch(measure->CtlType) {
			case ODT_BUTTON:
				break;
			case ODT_COMBOBOX:
				measure->itemHeight = 10;
/*				printf("defwndproc WM_MEASUREITEM // ODT_COMBOBOX !\n");*/
				break;
			case ODT_LISTBOX:
				measure->itemHeight = 10;
/*				printf("defwndproc WM_MEASUREITEM // ODT_LISTBOX !\n");*/
				break;
			case ODT_MENU:
				break;
			default:
				break;
			}
    	break;    	
    }
    return 0;
}
