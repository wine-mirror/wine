/*
 * Default window procedure
 *
 * Copyright 1993, 1996 Alexandre Julliard
 *	     1995 Alex Korobka
 */

#include <stdlib.h>
#include <stdio.h>
#include "win.h"
#include "user.h"
#include "heap.h"
#include "nonclient.h"
#include "winpos.h"
#include "string32.h"
#include "syscolor.h"
#include "sysmetrics.h"
#include "stddebug.h"
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
 *           DEFWND_HandleWindowPosChanged
 *
 * Handle the WM_WINDOWPOSCHANGED message.
 */
static void DEFWND_HandleWindowPosChanged( WND *wndPtr, UINT32 flags )
{
    WPARAM16 wp = SIZE_RESTORED;

    if (!(flags & SWP_NOCLIENTMOVE))
        SendMessage16( wndPtr->hwndSelf, WM_MOVE, 0,
                    MAKELONG(wndPtr->rectClient.left, wndPtr->rectClient.top));
    if (!(flags & SWP_NOCLIENTSIZE))
    {
        if (wndPtr->dwStyle & WS_MAXIMIZE) wp = SIZE_MAXIMIZED;
        else if (wndPtr->dwStyle & WS_MINIMIZE) wp = SIZE_MINIMIZED;

        SendMessage16( wndPtr->hwndSelf, WM_SIZE, wp, 
                     MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
                            wndPtr->rectClient.bottom-wndPtr->rectClient.top));
    }
}


/***********************************************************************
 *           DEFWND_SetText
 *
 * Set the window text.
 */
void DEFWND_SetText( WND *wndPtr, LPCSTR text )
{
    if (!text) text = "";
    if (wndPtr->text) HeapFree( SystemHeap, 0, wndPtr->text );
    wndPtr->text = HEAP_strdupA( SystemHeap, 0, text );
    if (wndPtr->window) XStoreName( display, wndPtr->window, wndPtr->text );
}


/***********************************************************************
 *           DEFWND_DefWinProc
 *
 * Default window procedure for messages that are the same in Win16 and Win32.
 */
static LRESULT DEFWND_DefWinProc( WND *wndPtr, UINT32 msg, WPARAM32 wParam,
                                  LPARAM lParam )
{
    switch(msg)
    {
    case WM_NCPAINT:
	return NC_HandleNCPaint( wndPtr->hwndSelf, (HRGN32)wParam );

    case WM_NCHITTEST:
        return NC_HandleNCHitTest( wndPtr->hwndSelf, MAKEPOINT16(lParam) );

    case WM_NCLBUTTONDOWN:
	return NC_HandleNCLButtonDown( wndPtr->hwndSelf, wParam, lParam );

    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
	return NC_HandleNCLButtonDblClk( wndPtr, wParam, lParam );

    case WM_NCACTIVATE:
	return NC_HandleNCActivate( wndPtr, wParam );

    case WM_NCDESTROY:
	if (wndPtr->text) HeapFree( SystemHeap, 0, wndPtr->text );
	wndPtr->text = NULL;
	if (wndPtr->pVScroll) HeapFree( SystemHeap, 0, wndPtr->pVScroll );
	if (wndPtr->pHScroll) HeapFree( SystemHeap, 0, wndPtr->pHScroll );
        wndPtr->pVScroll = wndPtr->pHScroll = NULL;
	return 0;

    case WM_PAINTICON:
    case WM_PAINT:
	{
	    PAINTSTRUCT16 ps;
	    HDC16 hdc = BeginPaint16( wndPtr->hwndSelf, &ps );
	    if( hdc ) 
	    {
	      if( (wndPtr->dwStyle & WS_MINIMIZE) && wndPtr->class->hIcon )
	      {
	        int x = (wndPtr->rectWindow.right - wndPtr->rectWindow.left -
			SYSMETRICS_CXICON)/2;
	        int y = (wndPtr->rectWindow.bottom - wndPtr->rectWindow.top -
			SYSMETRICS_CYICON)/2;
		dprintf_win(stddeb,"Painting class icon: vis rect=(%i,%i - %i,%i)\n",
		ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom );
	        DrawIcon( hdc, x, y, wndPtr->class->hIcon );
	      }
	      EndPaint16( wndPtr->hwndSelf, &ps );
	    }
	    return 0;
	}

    case WM_SETREDRAW:
        if (!wParam)
        {
            ValidateRect32( wndPtr->hwndSelf, NULL );
            wndPtr->flags |= WIN_NO_REDRAW;
        }
        else wndPtr->flags &= ~WIN_NO_REDRAW;
        return 0;

    case WM_CLOSE:
	DestroyWindow( wndPtr->hwndSelf );
	return 0;

    case WM_MOUSEACTIVATE:
	if (wndPtr->dwStyle & WS_CHILD)
	{
	    LONG ret = SendMessage16( wndPtr->parent->hwndSelf,
                                      WM_MOUSEACTIVATE, wParam, lParam );
	    if (ret) return ret;
	}
	return MA_ACTIVATE;

    case WM_ACTIVATE:
	if (LOWORD(wParam) != WA_INACTIVE) SetFocus32( wndPtr->hwndSelf );
	break;

    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
	{
	    if (!wndPtr->class->hbrBackground) return 0;

	    /* FIXME: should fill icon text with hbrushActiveCaption 
		      instead of this */

	    if (wndPtr->dwStyle & WS_MINIMIZE )
	    {
		 if( wndPtr->flags & WIN_NCACTIVATED )
		 {
		   FillWindow( GetParent16(wndPtr->hwndSelf), wndPtr->hwndSelf,
                               (HDC16)wParam, sysColorObjects.hbrushActiveCaption );
		   return 1;
		 }

		 /* FIXME: should draw parent' background somehow
			     (e.g for textured desktop) ? */
	    }

	    if (wndPtr->class->hbrBackground <= (HBRUSH16)(COLOR_MAX+1))
            {
                HBRUSH16 hbrush = CreateSolidBrush( 
			 GetSysColor(((DWORD)wndPtr->class->hbrBackground)-1));
                 FillWindow( GetParent16(wndPtr->hwndSelf), wndPtr->hwndSelf,
                             (HDC16)wParam, hbrush);
                 DeleteObject (hbrush);
            }
            else FillWindow( GetParent16(wndPtr->hwndSelf), wndPtr->hwndSelf,
                             (HDC16)wParam, wndPtr->class->hbrBackground );
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
        SetBkColor( (HDC32)wParam, GetSysColor(COLOR_WINDOW) );
        SetTextColor( (HDC32)wParam, GetSysColor(COLOR_WINDOWTEXT) );
        return (LRESULT)sysColorObjects.hbrushWindow;

    case WM_CTLCOLORSCROLLBAR:
        SetBkColor( (HDC32)wParam, RGB(255, 255, 255) );
        SetTextColor( (HDC32)wParam, RGB(0, 0, 0) );
        UnrealizeObject( sysColorObjects.hbrushScrollbar );
        return (LRESULT)sysColorObjects.hbrushScrollbar;

    case WM_CTLCOLOR:
	{
	    if (HIWORD(lParam) == CTLCOLOR_SCROLLBAR)
	    {
		SetBkColor( (HDC32)wParam, RGB(255, 255, 255) );
		SetTextColor( (HDC32)wParam, RGB(0, 0, 0) );
		UnrealizeObject( sysColorObjects.hbrushScrollbar );
		return (LRESULT)sysColorObjects.hbrushScrollbar;
	    }
	    else
	    {
		SetBkColor( (HDC32)wParam, GetSysColor(COLOR_WINDOW) );
		SetTextColor( (HDC32)wParam, GetSysColor(COLOR_WINDOWTEXT) );
		return (LRESULT)sysColorObjects.hbrushWindow;
	    }
	}
	
    case WM_GETTEXTLENGTH:
        if (wndPtr->text) return (LRESULT)strlen(wndPtr->text);
        return 0;

    case WM_SETCURSOR:
	if (wndPtr->dwStyle & WS_CHILD)
	    if (SendMessage16(wndPtr->parent->hwndSelf, WM_SETCURSOR,
                            wParam, lParam))
		return TRUE;
	return NC_HandleSetCursor( wndPtr->hwndSelf, wParam, lParam );

    case WM_SYSCOMMAND:
        return NC_HandleSysCommand( wndPtr->hwndSelf, wParam,
                                    MAKEPOINT16(lParam) );

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
		     SendMessage16( wndPtr->hwndSelf, WM_SYSCOMMAND,
                                    (WPARAM16)SC_KEYMENU, (LPARAM)VK_SPACE);
	break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
	/* Press and release F10 or ALT */
	if (((wParam == VK_MENU) && iMenuSysKey) ||
            ((wParam == VK_F10) && iF10Key))
	      SendMessage16( WIN_GetTopParent(wndPtr->hwndSelf),
                             WM_SYSCOMMAND, SC_KEYMENU, 0L );
	iMenuSysKey = iF10Key = 0;
        break;

    case WM_SYSCHAR:
	iMenuSysKey = 0;
	if (wParam == VK_RETURN && (wndPtr->dwStyle & WS_MINIMIZE))
        {
	    PostMessage( wndPtr->hwndSelf, WM_SYSCOMMAND,
                         (WPARAM16)SC_RESTORE, 0L ); 
	    break;
        } 
	if ((HIWORD(lParam) & KEYDATA_ALT) && wParam)
        {
	    if (wParam == VK_TAB || wParam == VK_ESCAPE) break;
	    if (wParam == VK_SPACE && (wndPtr->dwStyle & WS_CHILD))
                SendMessage16( wndPtr->parent->hwndSelf, msg, wParam, lParam );
	    else
                SendMessage16( wndPtr->hwndSelf, WM_SYSCOMMAND,
                               (WPARAM16)SC_KEYMENU, (LPARAM)(DWORD)wParam );
        } 
	else /* check for Ctrl-Esc */
            if (wParam != VK_ESCAPE) MessageBeep(0);
	break;

    case WM_SHOWWINDOW:
        if (!lParam) return 0; /* sent from ShowWindow */
        if (!(wndPtr->dwStyle & WS_POPUP) || !wndPtr->owner) return 0;
        if ((wndPtr->dwStyle & WS_VISIBLE) && wParam) return 0;
	else if (!(wndPtr->dwStyle & WS_VISIBLE) && !wParam) return 0;
        ShowWindow( wndPtr->hwndSelf, wParam ? SW_SHOWNOACTIVATE : SW_HIDE );
	break; 

    case WM_CANCELMODE:
	/* EndMenu() should be called if in menu state but currently it's
	   impossible to detect - menu code should be updated*/
	if (GetCapture32() == wndPtr->hwndSelf) ReleaseCapture();
	break;

    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
	return -1;

    case WM_DROPOBJECT:
	return DRAG_FILE;  

    case WM_QUERYDROPOBJECT:
	if (wndPtr->dwExStyle & WS_EX_ACCEPTFILES) return 1;
	break;

    case WM_QUERYDRAGICON:
        {
            HICON16 hI = 0;
            UINT16 len = 1;
            while(len < 64)
		if( (hI = LoadIcon16(wndPtr->hInstance,MAKEINTRESOURCE(len))) )
                    return (LRESULT)hI;
        }
        break;

    case WM_QUERYOPEN:
    case WM_QUERYENDSESSION:
	return 1;
    }
    return 0;
}



/***********************************************************************
 *           DefWindowProc16   (USER.107)
 */
LRESULT DefWindowProc16( HWND16 hwnd, UINT16 msg, WPARAM16 wParam,
                         LPARAM lParam )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    LRESULT result = 0;

    if (!wndPtr) return 0;
    SPY_EnterMessage( SPY_DEFWNDPROC16, hwnd, msg, wParam, lParam );

    switch(msg)
    {
    case WM_NCCREATE:
	{
	    CREATESTRUCT16 *cs = (CREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam);
	    if (cs->lpszName)
		DEFWND_SetText( wndPtr, (LPSTR)PTR_SEG_TO_LIN(cs->lpszName) );
	    result = 1;
	}
        break;

    case WM_NCCALCSIZE:
	result = NC_HandleNCCalcSize(wndPtr, (RECT16 *)PTR_SEG_TO_LIN(lParam));
        break;

    case WM_WINDOWPOSCHANGING:
	result = WINPOS_HandleWindowPosChanging16( wndPtr,
                                       (WINDOWPOS16 *)PTR_SEG_TO_LIN(lParam) );
        break;

    case WM_WINDOWPOSCHANGED:
	{
	    WINDOWPOS16 * winPos = (WINDOWPOS16 *)PTR_SEG_TO_LIN(lParam);
            DEFWND_HandleWindowPosChanged( wndPtr, winPos->flags );
	}
        break;

    case WM_GETTEXT:
        if (wParam && wndPtr->text)
        {
            lstrcpyn32A( (LPSTR)PTR_SEG_TO_LIN(lParam), wndPtr->text, wParam );
            result = (LRESULT)strlen( (LPSTR)PTR_SEG_TO_LIN(lParam) ) + 1;
        }
        break;

    case WM_SETTEXT:
	DEFWND_SetText( wndPtr, (LPSTR)PTR_SEG_TO_LIN(lParam) );
	NC_HandleNCPaint( hwnd , (HRGN32)1 );  /* Repaint caption */
        break;

    default:
        result = DEFWND_DefWinProc( wndPtr, msg, wParam, lParam );
        break;
    }

    SPY_ExitMessage( SPY_RESULT_OK16, hwnd, msg, result );
    return result;
}


/***********************************************************************
 *           DefWindowProc32A   (USER32.125)
 */
LRESULT DefWindowProc32A( HWND32 hwnd, UINT32 msg, WPARAM32 wParam,
                          LPARAM lParam )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    LRESULT result = 0;

    if (!wndPtr) return 0;
    SPY_EnterMessage( SPY_DEFWNDPROC32, hwnd, msg, wParam, lParam );

    switch(msg)
    {
    case WM_NCCREATE:
	{
	    CREATESTRUCT32A *cs = (CREATESTRUCT32A *)lParam;
	    if (cs->lpszName) DEFWND_SetText( wndPtr, cs->lpszName );
	    result = 1;
	}
        break;

    case WM_NCCALCSIZE:
        {
            RECT16 rect16;
            CONV_RECT32TO16( (RECT32 *)lParam, &rect16 );
            result = NC_HandleNCCalcSize( wndPtr, &rect16 );
            CONV_RECT16TO32( &rect16, (RECT32 *)lParam );
        }
        break;

    case WM_WINDOWPOSCHANGING:
	result = WINPOS_HandleWindowPosChanging32( wndPtr,
                                                   (WINDOWPOS32 *)lParam );
        break;

    case WM_WINDOWPOSCHANGED:
	{
	    WINDOWPOS32 * winPos = (WINDOWPOS32 *)lParam;
            DEFWND_HandleWindowPosChanged( wndPtr, winPos->flags );
	}
        break;

    case WM_GETTEXT:
        if (wParam && wndPtr->text)
        {
            lstrcpyn32A( (LPSTR)lParam, wndPtr->text, wParam );
            result = (LRESULT)strlen( (LPSTR)lParam ) + 1;
        }
        break;

    case WM_SETTEXT:
	DEFWND_SetText( wndPtr, (LPSTR)lParam );
	NC_HandleNCPaint( hwnd , (HRGN32)1 );  /* Repaint caption */
        break;

    default:
        result = DEFWND_DefWinProc( wndPtr, msg, wParam, lParam );
        break;
    }

    SPY_ExitMessage( SPY_RESULT_OK32, hwnd, msg, result );
    return result;
}


/***********************************************************************
 *           DefWindowProc32W   (USER32.126)
 */
LRESULT DefWindowProc32W( HWND32 hwnd, UINT32 msg, WPARAM32 wParam,
                          LPARAM lParam )
{
    LRESULT result;

    switch(msg)
    {
    case WM_NCCREATE:
	{
	    CREATESTRUCT32W *cs = (CREATESTRUCT32W *)lParam;
	    if (cs->lpszName)
            {
                WND *wndPtr = WIN_FindWndPtr( hwnd );
                LPSTR str = STRING32_DupUniToAnsi( cs->lpszName );
                DEFWND_SetText( wndPtr, str );
                free( str );
            }
	    result = 1;
	}
        break;

    case WM_GETTEXT:
        {
            LPSTR str = malloc( wParam );
            result = DefWindowProc32A( hwnd, msg, wParam, (LPARAM)str );
            STRING32_AnsiToUni( (LPWSTR)lParam, str );
            free( str );
        }
        break;

    case WM_SETTEXT:
        {
            LPSTR str = STRING32_DupUniToAnsi( (LPWSTR)lParam );
            result = DefWindowProc32A( hwnd, msg, wParam, (LPARAM)str );
            free( str );
        }
        break;

    default:
        result = DefWindowProc32A( hwnd, msg, wParam, lParam );
        break;
    }
    return result;
}
