/*
 * Default window procedure
 *
 * Copyright 1993 Alexandre Julliard
 *	     1995 Alex Korobka
 */

#include <stdlib.h>
#include <stdio.h>
#include "win.h"
#include "class.h"
#include "user.h"
#include "nonclient.h"
#include "winpos.h"
#include "syscolor.h"
#include "stddebug.h"
/* #define DEBUG_MESSAGE */
#include "debug.h"
#include "spy.h"

  /* Last COLOR id */
#define COLOR_MAX   COLOR_BTNHIGHLIGHT

  /* bits in the dwKeyData */
#define KEYDATA_ALT 		0x2000
#define KEYDATA_PREVSTATE	0x4000

static short iF10Key = 0;
static short iMenuSysKey = 0;

/***********************************************************************
 *           DEFWND_SetText
 *
 * Set the window text.
 */
void DEFWND_SetText( WND *wndPtr, LPSTR text )
{
    LPSTR textPtr;

    if (!text) text = "";
    if (wndPtr->hText) USER_HEAP_FREE( wndPtr->hText );
    wndPtr->hText = USER_HEAP_ALLOC( strlen(text) + 1 );
    textPtr = (LPSTR) USER_HEAP_LIN_ADDR( wndPtr->hText );
    strcpy( textPtr, text );
    if (wndPtr->window)
        XStoreName( display, wndPtr->window, text );
}


/***********************************************************************
 *           DefWindowProc   (USER.107)
 */
LRESULT DefWindowProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    CLASS * classPtr;
    LPSTR textPtr;
    int len;
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    SPY_EnterMessage( SPY_DEFWNDPROC, hwnd, msg, wParam, lParam );

    switch(msg)
    {
    case WM_NCCREATE:
	{
	    CREATESTRUCT *createStruct = (CREATESTRUCT*)PTR_SEG_TO_LIN(lParam);
	    if (createStruct->lpszName)
		DEFWND_SetText( wndPtr,
                               (LPSTR)PTR_SEG_TO_LIN(createStruct->lpszName) );
	    return 1;
	}

    case WM_NCCALCSIZE:
	return NC_HandleNCCalcSize( hwnd,
                                 (NCCALCSIZE_PARAMS *)PTR_SEG_TO_LIN(lParam) );

    case WM_PAINTICON: 
    case WM_NCPAINT:
	return NC_HandleNCPaint( hwnd, (HRGN)wParam );

    case WM_NCHITTEST:
        {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            return NC_HandleNCHitTest( hwnd, pt );
        }

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
	    LONG ret = SendMessage( wndPtr->parent->hwndSelf, WM_MOUSEACTIVATE,
				    wParam, lParam );
	    if (ret) return ret;
	}
	return MA_ACTIVATE;

    case WM_ACTIVATE:
      /* LOWORD() needed for WINELIB32 implementation.  Should be fine. */
	if (LOWORD(wParam)!=WA_INACTIVE) SetFocus( hwnd );
	break;

    case WM_WINDOWPOSCHANGING:
	return WINPOS_HandleWindowPosChanging( (WINDOWPOS *)PTR_SEG_TO_LIN(lParam) );

    case WM_WINDOWPOSCHANGED:
	{
	    WINDOWPOS * winPos = (WINDOWPOS *)PTR_SEG_TO_LIN(lParam);
	    WPARAM	wp     = SIZE_RESTORED;

	    if (!(winPos->flags & SWP_NOCLIENTMOVE))
		SendMessage( hwnd, WM_MOVE, 0,
		             MAKELONG( wndPtr->rectClient.left,
				       wndPtr->rectClient.top ));
	    if (!(winPos->flags & SWP_NOCLIENTSIZE))
		 {
	       	   if( wndPtr->dwStyle & WS_MAXIMIZE ) wp = SIZE_MAXIMIZED;
		   else if(wndPtr->dwStyle & WS_MINIMIZE ) wp = SIZE_MINIMIZED;

		   SendMessage( hwnd, WM_SIZE, wp, 
			        MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
			                 wndPtr->rectClient.bottom-wndPtr->rectClient.top));

                 }
	    return 0;
	}

    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
	{
	    if (!(classPtr = CLASS_FindClassPtr( wndPtr->hClass ))) return 0;
	    if (!classPtr->wc.hbrBackground) return 0;
            if (classPtr->wc.hbrBackground <= (HBRUSH)(COLOR_MAX+1))
            {
                 HBRUSH hbrush;
                 hbrush = CreateSolidBrush(
                     GetSysColor(((DWORD)classPtr->wc.hbrBackground)-1));
                 FillWindow( GetParent(hwnd), hwnd, (HDC)wParam, hbrush);
                 DeleteObject (hbrush);
            }
            else
	         FillWindow( GetParent(hwnd), hwnd, (HDC)wParam,
		        classPtr->wc.hbrBackground );
	    return 1;
	}

    case WM_GETDLGCODE:
	return 0;

    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        SetBkColor( (HDC)wParam, GetSysColor(COLOR_WINDOW) );
        SetTextColor( (HDC)wParam, GetSysColor(COLOR_WINDOWTEXT) );
        return (LONG)sysColorObjects.hbrushWindow;

    case WM_CTLCOLORSCROLLBAR:
        SetBkColor( (HDC)wParam, RGB(255, 255, 255) );
        SetTextColor( (HDC)wParam, RGB(0, 0, 0) );
        UnrealizeObject( sysColorObjects.hbrushScrollbar );
        return (LONG)sysColorObjects.hbrushScrollbar;

    case WM_CTLCOLOR:
	{
	    if (HIWORD(lParam) == CTLCOLOR_SCROLLBAR)
	    {
		SetBkColor( (HDC)wParam, RGB(255, 255, 255) );
		SetTextColor( (HDC)wParam, RGB(0, 0, 0) );
		UnrealizeObject( sysColorObjects.hbrushScrollbar );
		return (LONG)sysColorObjects.hbrushScrollbar;
	    }
	    else
	    {
		SetBkColor( (HDC)wParam, GetSysColor(COLOR_WINDOW) );
		SetTextColor( (HDC)wParam, GetSysColor(COLOR_WINDOWTEXT) );
		return (LONG)sysColorObjects.hbrushWindow;
	    }
	}
	
    case WM_GETTEXT:
	{
	    if (wParam)
	    {
		if (wndPtr->hText)
		{
		    textPtr = (LPSTR)USER_HEAP_LIN_ADDR(wndPtr->hText);
		    if ((int)wParam > (len = strlen(textPtr)))
		    {
			strcpy((char *)PTR_SEG_TO_LIN(lParam), textPtr);
			return (DWORD)len;
		    }
		}
	    }
	    return 0;
	}

    case WM_GETTEXTLENGTH:
	{
	    if (wndPtr->hText)
	    {
		textPtr = (LPSTR)USER_HEAP_LIN_ADDR(wndPtr->hText);
		return (DWORD)strlen(textPtr);
	    }
	    return 0;
	}

    case WM_SETTEXT:
	DEFWND_SetText( wndPtr, (LPSTR)PTR_SEG_TO_LIN(lParam) );
	NC_HandleNCPaint( hwnd , (HRGN)1 );  /* Repaint caption */
	return 0;

    case WM_SETCURSOR:
	if (wndPtr->dwStyle & WS_CHILD)
	    if (SendMessage(wndPtr->parent->hwndSelf, WM_SETCURSOR,
                            wParam, lParam))
		return TRUE;
	return NC_HandleSetCursor( hwnd, wParam, lParam );

    case WM_SYSCOMMAND:
	{
          POINT pt = { LOWORD(lParam), HIWORD(lParam) };
          return NC_HandleSysCommand( hwnd, wParam, pt );
	}
    case WM_KEYDOWN:

	if(wParam == VK_F10) iF10Key = VK_F10;
	break;

    case WM_SYSKEYDOWN:

	if( HIWORD(lParam) & KEYDATA_ALT )
	  {
	    /* if( HIWORD(lParam) & ~KEYDATA_PREVSTATE ) */
	      if( wParam == VK_MENU && !iMenuSysKey )
		iMenuSysKey = 1;
	      else
		iMenuSysKey = 0;
	    
	    iF10Key = 0;

	  } 
	else if( wParam == VK_F10 )
	         iF10Key = 1;
	     else
	         if( wParam == VK_ESCAPE && GetKeyState(VK_SHIFT) < 0 )
		     SendMessage( hwnd, WM_SYSCOMMAND, (WPARAM)SC_KEYMENU, 
 						       (LPARAM)VK_SPACE);
	break;

    case WM_KEYUP:
    case WM_SYSKEYUP:

	/* Press and release F10 or ALT */

	if( ( wParam == VK_MENU && iMenuSysKey ) 
	      || ( wParam == VK_F10 && iF10Key ) )

	      SendMessage( WIN_GetTopParent(hwnd), WM_SYSCOMMAND,
			   SC_KEYMENU, 0L );

	iMenuSysKey = iF10Key = 0;
        break;

    case WM_SYSCHAR:

	iMenuSysKey = 0;

	if( wParam == VK_RETURN && (wndPtr->dwStyle & WS_MINIMIZE) )
	  {
	    PostMessage(hwnd, WM_SYSCOMMAND, (WPARAM)SC_RESTORE, 0L ); 
	    break;
	  }  

	if( (HIWORD(lParam) & KEYDATA_ALT) && wParam )
	  {
	    if( wParam == VK_TAB || wParam == VK_ESCAPE )
	      break;

	    if( wParam == VK_SPACE && (wndPtr->dwStyle & WS_CHILD) )
	      SendMessage( wndPtr->parent->hwndSelf, msg, wParam, lParam );
	    else
	      SendMessage(hwnd, WM_SYSCOMMAND, (WPARAM)SC_KEYMENU, (LPARAM)(DWORD)wParam );
	  } 
	else
	  /* check for Ctrl-Esc */
	  if( wParam != VK_ESCAPE )   
	      MessageBeep(0);
	  
	break;

    case WM_SHOWWINDOW:
	if( !lParam ) return 0; /* sent from ShowWindow */

	if( !(wndPtr->dwStyle & WS_POPUP) || !wndPtr->owner ) 
	      return 0;

	if( wndPtr->dwStyle & WS_VISIBLE )
	    { if( wParam ) return 0; }
	else
	      if(!wParam ) return 0;
  
	ShowWindow(hwnd,(wParam)? SW_SHOWNOACTIVATE: SW_HIDE);
	break; 

    case WM_CANCELMODE:

	/* EndMenu() should be called if in menu state but currently it's
	   impossible to detect - menu code should be updated*/

	if( GetCapture() == hwnd )
	    ReleaseCapture();

	break;

    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
	return -1;

    case WM_DROPOBJECT:
	return DRAG_FILE;  

    case WM_QUERYDROPOBJECT:
	if(wndPtr->dwExStyle & WS_EX_ACCEPTFILES)
	   return 1;
	break;

    case WM_QUERYDRAGICON:
	{
	 HICON hI = 0;

	 len = 1;
	 while(len < 64)
		if( (hI = LoadIcon(wndPtr->hInstance,MAKEINTRESOURCE(len))) )
		     return (LRESULT)hI;
	}
        break;

    case WM_QUERYOPEN:
    case WM_QUERYENDSESSION:
	return 1;

    }
    return 0;
}


