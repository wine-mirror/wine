/*
 * Default window procedure
 *
 * Copyright 1993, 1996 Alexandre Julliard
 *	     1995 Alex Korobka
 */

#include <string.h>

#include "class.h"
#include "win.h"
#include "user.h"
#include "heap.h"
#include "nonclient.h"
#include "winpos.h"
#include "dce.h"
#include "debugtools.h"
#include "spy.h"
#include "tweak.h"
#include "cache.h"
#include "windef.h"
#include "wingdi.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "wine/winuser16.h"

DEFAULT_DEBUG_CHANNEL(win);

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
static void DEFWND_HandleWindowPosChanged( WND *wndPtr, UINT flags )
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
 *           DEFWND_SetTextA
 *
 * Set the window text.
 */
void DEFWND_SetTextA( WND *wndPtr, LPCSTR text )
{
    int count;

    if (!text) text = "";
    count = MultiByteToWideChar( CP_ACP, 0, text, -1, NULL, 0 );

    if (wndPtr->text) HeapFree(SystemHeap, 0, wndPtr->text);
    if ((wndPtr->text = HeapAlloc(SystemHeap, 0, count * sizeof(WCHAR))))
        MultiByteToWideChar( CP_ACP, 0, text, -1, wndPtr->text, count );
    else
        ERR("Not enough memory for window text");

    wndPtr->pDriver->pSetText(wndPtr, wndPtr->text);
}

/***********************************************************************
 *           DEFWND_SetTextW
 *
 * Set the window text.
 */
void DEFWND_SetTextW( WND *wndPtr, LPCWSTR text )
{
    static const WCHAR empty_string[] = {0};
    int count;

    if (!text) text = empty_string;
    count = strlenW(text) + 1;

    if (wndPtr->text) HeapFree(SystemHeap, 0, wndPtr->text);
    if ((wndPtr->text = HeapAlloc(SystemHeap, 0, count * sizeof(WCHAR))))
	lstrcpyW( wndPtr->text, text );
    else
        ERR("Not enough memory for window text");

    wndPtr->pDriver->pSetText(wndPtr, wndPtr->text);
}

/***********************************************************************
 *           DEFWND_ControlColor
 *
 * Default colors for control painting.
 */
HBRUSH DEFWND_ControlColor( HDC hDC, UINT16 ctlType )
{
    if( ctlType == CTLCOLOR_SCROLLBAR)
    {
	HBRUSH hb = GetSysColorBrush(COLOR_SCROLLBAR);
        if (TWEAK_WineLook == WIN31_LOOK) {
           SetTextColor( hDC, RGB(0, 0, 0) );
           SetBkColor( hDC, RGB(255, 255, 255) );
        } else {
           COLORREF bk = GetSysColor(COLOR_3DHILIGHT);
           SetTextColor( hDC, GetSysColor(COLOR_3DFACE));
           SetBkColor( hDC, bk);

           /* if COLOR_WINDOW happens to be the same as COLOR_3DHILIGHT 
            * we better use 0x55aa bitmap brush to make scrollbar's background
            * look different from the window background. 
            */
           if (bk == GetSysColor(COLOR_WINDOW)) {
               return CACHE_GetPattern55AABrush();
           }
        }
	UnrealizeObject( hb );
        return hb;
    }

    SetTextColor( hDC, GetSysColor(COLOR_WINDOWTEXT));

    if (TWEAK_WineLook > WIN31_LOOK) {
	if ((ctlType == CTLCOLOR_EDIT) || (ctlType == CTLCOLOR_LISTBOX))
	    SetBkColor( hDC, GetSysColor(COLOR_WINDOW) );
	else {
	    SetBkColor( hDC, GetSysColor(COLOR_3DFACE) );
	    return GetSysColorBrush(COLOR_3DFACE);
	}
    }
    else
	SetBkColor( hDC, GetSysColor(COLOR_WINDOW) );
    return GetSysColorBrush(COLOR_WINDOW);
}


/***********************************************************************
 *           DEFWND_SetRedraw
 */
static void DEFWND_SetRedraw( WND* wndPtr, WPARAM wParam )
{
    BOOL bVisible = wndPtr->dwStyle & WS_VISIBLE;

    TRACE("%04x %i\n", wndPtr->hwndSelf, (wParam!=0) );

    if( wParam )
    {
	if( !bVisible )
	{
	    wndPtr->dwStyle |= WS_VISIBLE;
	    DCE_InvalidateDCE( wndPtr, &wndPtr->rectWindow );
	}
    }
    else if( bVisible )
    {
	if( wndPtr->dwStyle & WS_MINIMIZE ) wParam = RDW_VALIDATE;
	else wParam = RDW_ALLCHILDREN | RDW_VALIDATE;

	PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, 0, wParam, 0 );
	DCE_InvalidateDCE( wndPtr, &wndPtr->rectWindow );
	wndPtr->dwStyle &= ~WS_VISIBLE;
    }
}

/***********************************************************************
 *           DEFWND_Print
 *
 * This method handles the default behavior for the WM_PRINT message.
 */
static void DEFWND_Print(
  WND*  wndPtr,
  HDC   hdc,
  ULONG uFlags)
{
  /*
   * Visibility flag.
   */
  if ( (uFlags & PRF_CHECKVISIBLE) &&
       !IsWindowVisible(wndPtr->hwndSelf) )
      return;

  /*
   * Unimplemented flags.
   */
  if ( (uFlags & PRF_CHILDREN) ||
       (uFlags & PRF_OWNED)    ||
       (uFlags & PRF_NONCLIENT) )
  {
    WARN("WM_PRINT message with unsupported flags\n");
  }

  /*
   * Background
   */
  if ( uFlags & PRF_ERASEBKGND)
    SendMessageA(wndPtr->hwndSelf, WM_ERASEBKGND, (WPARAM)hdc, 0);

  /*
   * Client area
   */
  if ( uFlags & PRF_CLIENT)
    SendMessageA(wndPtr->hwndSelf, WM_PRINTCLIENT, (WPARAM)hdc, PRF_CLIENT);
}

/***********************************************************************
 *           DEFWND_DefWinProc
 *
 * Default window procedure for messages that are the same in Win16 and Win32.
 */
static LRESULT DEFWND_DefWinProc( WND *wndPtr, UINT msg, WPARAM wParam,
                                  LPARAM lParam )
{
    switch(msg)
    {
    case WM_NCPAINT:
	return NC_HandleNCPaint( wndPtr->hwndSelf, (HRGN)wParam );

    case WM_NCHITTEST:
        return NC_HandleNCHitTest( wndPtr->hwndSelf, MAKEPOINT16(lParam) );

    case WM_NCLBUTTONDOWN:
	return NC_HandleNCLButtonDown( wndPtr, wParam, lParam );

    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
	return NC_HandleNCLButtonDblClk( wndPtr, wParam, lParam );

    case WM_RBUTTONUP:
    case WM_NCRBUTTONUP:
        if ((wndPtr->flags & WIN_ISWIN32) || (TWEAK_WineLook > WIN31_LOOK))
        {
	    ClientToScreen16(wndPtr->hwndSelf, (LPPOINT16)&lParam);
            SendMessageA( wndPtr->hwndSelf, WM_CONTEXTMENU,
			    wndPtr->hwndSelf, lParam);
        }
        break;

    case WM_CONTEXTMENU:
	if( wndPtr->dwStyle & WS_CHILD )
	    SendMessageA( wndPtr->parent->hwndSelf, msg, wParam, lParam );
	else if (wndPtr->hSysMenu)
        {
            LONG hitcode;
            POINT16 pt = MAKEPOINT16(lParam);

            ScreenToClient16(wndPtr->hwndSelf, &pt);
            hitcode = NC_HandleNCHitTest(wndPtr->hwndSelf, pt);

            /* Track system popup if click was in the caption area. */
            if (hitcode==HTCAPTION || hitcode==HTSYSMENU)
                TrackPopupMenu(GetSystemMenu(wndPtr->hwndSelf, FALSE),
                               TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                               pt.x, pt.y, 0, wndPtr->hwndSelf, NULL);
        }
	break;

    case WM_NCACTIVATE:
	return NC_HandleNCActivate( wndPtr, wParam );

    case WM_NCDESTROY:
	if (wndPtr->text) HeapFree( SystemHeap, 0, wndPtr->text );
	wndPtr->text = NULL;
	if (wndPtr->pVScroll) HeapFree( SystemHeap, 0, wndPtr->pVScroll );
	if (wndPtr->pHScroll) HeapFree( SystemHeap, 0, wndPtr->pHScroll );
        wndPtr->pVScroll = wndPtr->pHScroll = NULL;
	return 0;

    case WM_PRINT:
        DEFWND_Print(wndPtr, (HDC)wParam, lParam);
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
			GetSystemMetrics(SM_CXICON))/2;
	        int y = (wndPtr->rectWindow.bottom - wndPtr->rectWindow.top -
			GetSystemMetrics(SM_CYICON))/2;
		TRACE("Painting class icon: vis rect=(%i,%i - %i,%i)\n",
		ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom );
	        DrawIcon( hdc, x, y, wndPtr->class->hIcon );
	      }
	      EndPaint16( wndPtr->hwndSelf, &ps );
	    }
	    return 0;
	}

    case WM_SYNCPAINT:
	if (wndPtr->hrgnUpdate) 
	{
	   RedrawWindow ( wndPtr->hwndSelf, 0, wndPtr->hrgnUpdate,
                    RDW_ERASENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN );
        }
        return 0;

    case WM_SETREDRAW:
	DEFWND_SetRedraw( wndPtr, wParam );
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

	/* Caption clicks are handled by the NC_HandleNCLButtonDown() */ 
        return (LOWORD(lParam) >= HTCLIENT) ? MA_ACTIVATE : MA_NOACTIVATE;

    case WM_ACTIVATE:
	/* The default action in Windows is to set the keyboard focus to
	 * the window, if it's being activated and not minimized */
	if (LOWORD(wParam) != WA_INACTIVE) {
		if (!(wndPtr->dwStyle & WS_MINIMIZE))
			SetFocus(wndPtr->hwndSelf);
	}
	break;

     case WM_MOUSEWHEEL:
	if (wndPtr->dwStyle & WS_CHILD)
	{
	    return SendMessageA( wndPtr->parent->hwndSelf,
				 WM_MOUSEWHEEL, wParam, lParam );
	}
	break;

    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
	{
	    RECT rect;

	    if (!wndPtr->class->hbrBackground) return 0;

	    /*  Since WM_ERASEBKGND may receive either a window dc or a    */ 
	    /*  client dc, the area to be erased has to be retrieved from  */
	    /*  the device context.      				   */
	    GetClipBox( (HDC)wParam, &rect );

            /* Always call the Win32 variant of FillRect even on Win16,
             * since despite the fact that Win16, as well as Win32,
             * supports special background brushes for a window class,
             * the Win16 variant of FillRect does not.
             */
            FillRect( (HDC) wParam, &rect, wndPtr->class->hbrBackground);
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
    case WM_CTLCOLORSCROLLBAR:
	return (LRESULT)DEFWND_ControlColor( (HDC)wParam, msg - WM_CTLCOLORMSGBOX );

    case WM_CTLCOLOR:
	return (LRESULT)DEFWND_ControlColor( (HDC)wParam, HIWORD(lParam) );
	
    case WM_GETTEXTLENGTH:
        if (wndPtr->text) return (LRESULT)strlenW(wndPtr->text);
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

	    if( wParam == VK_F4 )	/* try to close the window */
	    {
		HWND hWnd = WIN_GetTopParent( wndPtr->hwndSelf );
		wndPtr = WIN_FindWndPtr( hWnd );
		if( wndPtr && !(wndPtr->class->style & CS_NOCLOSE) )
		    PostMessage16( hWnd, WM_SYSCOMMAND, SC_CLOSE, 0 );
                WIN_ReleaseWndPtr(wndPtr);
	    }
	} 
	else if( wParam == VK_F10 )
	        iF10Key = 1;
	     else
	        if( wParam == VK_ESCAPE && (GetKeyState(VK_SHIFT) & 0x8000))
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
	    PostMessage16( wndPtr->hwndSelf, WM_SYSCOMMAND,
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
	if (wndPtr->parent == WIN_GetDesktop()) EndMenu();
	if (GetCapture() == wndPtr->hwndSelf) ReleaseCapture();
        WIN_ReleaseDesktop();
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
            HICON16 hIcon=0;
            UINT16 len;

            if( (hIcon=wndPtr->class->hCursor) ) return (LRESULT)hIcon;
            for(len=1; len<64; len++)
                if((hIcon=LoadIcon16(wndPtr->hInstance,MAKEINTRESOURCE16(len))))
                    return (LRESULT)hIcon;
            return (LRESULT)LoadIcon16(0,IDI_APPLICATION16);
        }
        break;

    case WM_ISACTIVEICON:
	return ((wndPtr->flags & WIN_NCACTIVATED) != 0);

    case WM_NOTIFYFORMAT:
      if (IsWindowUnicode(wndPtr->hwndSelf)) return NFR_UNICODE;
      else return NFR_ANSI;
        
    case WM_QUERYOPEN:
    case WM_QUERYENDSESSION:
	return 1;

    case WM_SETICON:
	{
		int index = (wParam != ICON_SMALL) ? GCL_HICON : GCL_HICONSM;
		HICON16 hOldIcon = GetClassLongA(wndPtr->hwndSelf, index); 
		SetClassLongA(wndPtr->hwndSelf, index, lParam);

		SetWindowPos(wndPtr->hwndSelf, 0, 0, 0, 0, 0, SWP_FRAMECHANGED
			 | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE
			 | SWP_NOZORDER);

		if( wndPtr->flags & WIN_NATIVE )
		    wndPtr->pDriver->pSetHostAttr(wndPtr, HAK_ICONS, 0);

		return hOldIcon;
	}

    case WM_GETICON:
	{
		int index = (wParam != ICON_SMALL) ? GCL_HICON : GCL_HICONSM;
		return GetClassLongA(wndPtr->hwndSelf, index); 
	}

    case WM_HELP:
	SendMessageA( wndPtr->parent->hwndSelf, msg, wParam, lParam );
	break;
    }

    return 0;
}



/***********************************************************************
 *           DefWindowProc16   (USER.107)
 */
LRESULT WINAPI DefWindowProc16( HWND16 hwnd, UINT16 msg, WPARAM16 wParam,
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
		DEFWND_SetTextA( wndPtr, (LPCSTR)PTR_SEG_TO_LIN(cs->lpszName) );
	    result = 1;
	}
        break;

    case WM_NCCALCSIZE:
        {
            RECT rect32;
            CONV_RECT16TO32( (RECT16 *)PTR_SEG_TO_LIN(lParam), &rect32 );
            result = NC_HandleNCCalcSize( wndPtr, &rect32 );
            CONV_RECT32TO16( &rect32, (RECT16 *)PTR_SEG_TO_LIN(lParam) );
        }
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
            lstrcpynWtoA( (LPSTR)PTR_SEG_TO_LIN(lParam), wndPtr->text, wParam );
            result = (LRESULT)strlen( (LPSTR)PTR_SEG_TO_LIN(lParam) );
        }
        break;

    case WM_SETTEXT:
	DEFWND_SetTextA( wndPtr, (LPCSTR)PTR_SEG_TO_LIN(lParam) );
	if( wndPtr->dwStyle & WS_CAPTION ) NC_HandleNCPaint( hwnd , (HRGN)1 );
        break;

    default:
        result = DEFWND_DefWinProc( wndPtr, msg, wParam, lParam );
        break;
    }

    WIN_ReleaseWndPtr(wndPtr);
    SPY_ExitMessage( SPY_RESULT_DEFWND16, hwnd, msg, result );
    return result;
}


/***********************************************************************
 *  DefWindowProcA [USER32.126] 
 *
 */
LRESULT WINAPI DefWindowProcA( HWND hwnd, UINT msg, WPARAM wParam,
                                 LPARAM lParam )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    LRESULT result = 0;

    if (!wndPtr) return 0;
    SPY_EnterMessage( SPY_DEFWNDPROC, hwnd, msg, wParam, lParam );

    switch(msg)
    {
    case WM_NCCREATE:
	{
	    CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
	    if (cs->lpszName) DEFWND_SetTextA( wndPtr, cs->lpszName );
	    result = 1;
	}
        break;

    case WM_NCCALCSIZE:
        result = NC_HandleNCCalcSize( wndPtr, (RECT *)lParam );
        break;

    case WM_WINDOWPOSCHANGING:
	result = WINPOS_HandleWindowPosChanging( wndPtr,
                                                   (WINDOWPOS *)lParam );
        break;

    case WM_WINDOWPOSCHANGED:
	{
	    WINDOWPOS * winPos = (WINDOWPOS *)lParam;
            DEFWND_HandleWindowPosChanged( wndPtr, winPos->flags );
	}
        break;

    case WM_GETTEXT:
        if (wParam && wndPtr->text)
        {
            lstrcpynWtoA( (LPSTR)lParam, wndPtr->text, wParam );
            result = (LRESULT)strlen( (LPSTR)lParam );
        }
        break;

    case WM_SETTEXT:
	DEFWND_SetTextA( wndPtr, (LPCSTR)lParam );
	NC_HandleNCPaint( hwnd , (HRGN)1 );  /* Repaint caption */
        break;

    default:
        result = DEFWND_DefWinProc( wndPtr, msg, wParam, lParam );
        break;
    }

    WIN_ReleaseWndPtr(wndPtr);
    SPY_ExitMessage( SPY_RESULT_DEFWND, hwnd, msg, result );
    return result;
}


/***********************************************************************
 * DefWindowProcW [USER32.127] Calls default window message handler
 * 
 * Calls default window procedure for messages not processed 
 *  by application.
 *
 *  RETURNS
 *     Return value is dependent upon the message.
*/
LRESULT WINAPI DefWindowProcW( 
    HWND hwnd,      /* [in] window procedure recieving message */
    UINT msg,       /* [in] message identifier */
    WPARAM wParam,  /* [in] first message parameter */
    LPARAM lParam )   /* [in] second message parameter */
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    LRESULT result = 0;

    if (!wndPtr) return 0;
    switch(msg)
    {
    case WM_NCCREATE:
	{
	    CREATESTRUCTW *cs = (CREATESTRUCTW *)lParam;
	    if (cs->lpszName) DEFWND_SetTextW( wndPtr, cs->lpszName );
	    result = 1;
	}
        break;

    case WM_GETTEXT:
        if (wParam && wndPtr->text)
        {
            lstrcpynW( (LPWSTR)lParam, wndPtr->text, wParam );
            result = strlenW( (LPWSTR)lParam );
        }
        break;

    case WM_SETTEXT:
        DEFWND_SetTextW( wndPtr, (LPCWSTR)lParam );
	NC_HandleNCPaint( hwnd , (HRGN)1 );  /* Repaint caption */
        break;

    default:
        result = DefWindowProcA( hwnd, msg, wParam, lParam );
        break;
    }
    WIN_ReleaseWndPtr(wndPtr);
    return result;
}
