/*
 * Default window procedure
 *
 * Copyright 1993, 1996 Alexandre Julliard
 *	     1995 Alex Korobka
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>

#include "win.h"
#include "user.h"
#include "nonclient.h"
#include "winpos.h"
#include "dce.h"
#include "wine/debug.h"
#include "spy.h"
#include "windef.h"
#include "wingdi.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "wine/winuser16.h"
#include "wine/server.h"
#include "imm.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);

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
static void DEFWND_HandleWindowPosChanged( HWND hwnd, UINT flags )
{
    RECT rect;
    WND *wndPtr = WIN_GetPtr( hwnd );

    rect = wndPtr->rectClient;
    WIN_ReleasePtr( wndPtr );

    if (!(flags & SWP_NOCLIENTMOVE))
        SendMessageW( hwnd, WM_MOVE, 0, MAKELONG(rect.left, rect.top));

    if (!(flags & SWP_NOCLIENTSIZE))
    {
        WPARAM wp = SIZE_RESTORED;
        if (IsZoomed(hwnd)) wp = SIZE_MAXIMIZED;
        else if (IsIconic(hwnd)) wp = SIZE_MINIMIZED;

        SendMessageW( hwnd, WM_SIZE, wp, MAKELONG(rect.right-rect.left, rect.bottom-rect.top) );
    }
}


/***********************************************************************
 *           DEFWND_SetTextA
 *
 * Set the window text.
 */
static void DEFWND_SetTextA( HWND hwnd, LPCSTR text )
{
    int count;
    WCHAR *textW;
    WND *wndPtr;

    if (!text) text = "";
    count = MultiByteToWideChar( CP_ACP, 0, text, -1, NULL, 0 );

    if (!(wndPtr = WIN_GetPtr( hwnd ))) return;
    if ((textW = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR))))
    {
        if (wndPtr->text) HeapFree(GetProcessHeap(), 0, wndPtr->text);
        wndPtr->text = textW;
        MultiByteToWideChar( CP_ACP, 0, text, -1, textW, count );
        SERVER_START_REQ( set_window_text )
        {
            req->handle = hwnd;
            wine_server_add_data( req, textW, (count-1) * sizeof(WCHAR) );
            wine_server_call( req );
        }
        SERVER_END_REQ;
    }
    else
        ERR("Not enough memory for window text\n");
    WIN_ReleasePtr( wndPtr );

    if (USER_Driver.pSetWindowText) USER_Driver.pSetWindowText( hwnd, textW );
}

/***********************************************************************
 *           DEFWND_SetTextW
 *
 * Set the window text.
 */
static void DEFWND_SetTextW( HWND hwnd, LPCWSTR text )
{
    static const WCHAR empty_string[] = {0};
    WND *wndPtr;
    int count;

    if (!text) text = empty_string;
    count = strlenW(text) + 1;

    if (!(wndPtr = WIN_GetPtr( hwnd ))) return;
    if (wndPtr->text) HeapFree(GetProcessHeap(), 0, wndPtr->text);
    if ((wndPtr->text = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR))))
    {
        strcpyW( wndPtr->text, text );
        SERVER_START_REQ( set_window_text )
        {
            req->handle = hwnd;
            wine_server_add_data( req, wndPtr->text, (count-1) * sizeof(WCHAR) );
            wine_server_call( req );
        }
        SERVER_END_REQ;
    }
    else
        ERR("Not enough memory for window text\n");
    text = wndPtr->text;
    WIN_ReleasePtr( wndPtr );

    if (USER_Driver.pSetWindowText) USER_Driver.pSetWindowText( hwnd, text );
}

/***********************************************************************
 *           DEFWND_ControlColor
 *
 * Default colors for control painting.
 */
HBRUSH DEFWND_ControlColor( HDC hDC, UINT ctlType )
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
static void DEFWND_SetRedraw( HWND hwnd, WPARAM wParam )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    BOOL bVisible = wndPtr->dwStyle & WS_VISIBLE;

    TRACE("%04x %i\n", hwnd, (wParam!=0) );

    if( wParam )
    {
	if( !bVisible )
	{
            WIN_SetStyle( hwnd, wndPtr->dwStyle | WS_VISIBLE );
            DCE_InvalidateDCE( hwnd, &wndPtr->rectWindow );
	}
    }
    else if( bVisible )
    {
	if( wndPtr->dwStyle & WS_MINIMIZE ) wParam = RDW_VALIDATE;
	else wParam = RDW_ALLCHILDREN | RDW_VALIDATE;

        RedrawWindow( hwnd, NULL, 0, wParam );
        DCE_InvalidateDCE( hwnd, &wndPtr->rectWindow );
        WIN_SetStyle( hwnd, wndPtr->dwStyle & ~WS_VISIBLE );
    }
    WIN_ReleaseWndPtr( wndPtr );
}

/***********************************************************************
 *           DEFWND_Print
 *
 * This method handles the default behavior for the WM_PRINT message.
 */
static void DEFWND_Print( HWND hwnd, HDC hdc, ULONG uFlags)
{
  /*
   * Visibility flag.
   */
  if ( (uFlags & PRF_CHECKVISIBLE) &&
       !IsWindowVisible(hwnd) )
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
    SendMessageW(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);

  /*
   * Client area
   */
  if ( uFlags & PRF_CLIENT)
    SendMessageW(hwnd, WM_PRINTCLIENT, (WPARAM)hdc, PRF_CLIENT);
}


/*
 * helpers for calling IMM32
 *
 * WM_IME_* messages are generated only by IMM32,
 * so I assume imm32 is already LoadLibrary-ed.
 */
static HWND DEFWND_ImmGetDefaultIMEWnd( HWND hwnd )
{
    HINSTANCE hInstIMM = GetModuleHandleA( "imm32" );
    HWND (WINAPI *pFunc)(HWND);
    HWND hwndRet = 0;

    if (!hInstIMM)
    {
        ERR( "cannot get IMM32 handle\n" );
        return 0;
    }

    pFunc = (void*)GetProcAddress(hInstIMM,"ImmGetDefaultIMEWnd");
    if ( pFunc != NULL )
	hwndRet = (*pFunc)( hwnd );

    return hwndRet;
}

static BOOL DEFWND_ImmIsUIMessageA( HWND hwndIME, UINT msg, WPARAM wParam, LPARAM lParam )
{
    HINSTANCE hInstIMM = GetModuleHandleA( "imm32" );
    BOOL (WINAPI *pFunc)(HWND,UINT,WPARAM,LPARAM);
    BOOL fRet = FALSE;

    if (!hInstIMM)
    {
        ERR( "cannot get IMM32 handle\n" );
        return FALSE;
    }

    pFunc = (void*)GetProcAddress(hInstIMM,"ImmIsUIMessageA");
    if ( pFunc != NULL )
	fRet = (*pFunc)( hwndIME, msg, wParam, lParam );

    return fRet;
}

static BOOL DEFWND_ImmIsUIMessageW( HWND hwndIME, UINT msg, WPARAM wParam, LPARAM lParam )
{
    HINSTANCE hInstIMM = GetModuleHandleA( "imm32" );
    BOOL (WINAPI *pFunc)(HWND,UINT,WPARAM,LPARAM);
    BOOL fRet = FALSE;

    if (!hInstIMM)
    {
        ERR( "cannot get IMM32 handle\n" );
        return FALSE;
    }

    pFunc = (void*)GetProcAddress(hInstIMM,"ImmIsUIMessageW");
    if ( pFunc != NULL )
	fRet = (*pFunc)( hwndIME, msg, wParam, lParam );

    return fRet;
}



/***********************************************************************
 *           DEFWND_DefWinProc
 *
 * Default window procedure for messages that are the same in Win16 and Win32.
 */
static LRESULT DEFWND_DefWinProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch(msg)
    {
    case WM_NCPAINT:
        return NC_HandleNCPaint( hwnd, (HRGN)wParam );

    case WM_NCHITTEST:
        {
            POINT pt;
            pt.x = SLOWORD(lParam);
            pt.y = SHIWORD(lParam);
            return NC_HandleNCHitTest( hwnd, pt );
        }

    case WM_NCLBUTTONDOWN:
        return NC_HandleNCLButtonDown( hwnd, wParam, lParam );

    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
        return NC_HandleNCLButtonDblClk( hwnd, wParam, lParam );

    case WM_NCRBUTTONDOWN:
        /* in Windows, capture is taken when right-clicking on the caption bar */
        if (wParam==HTCAPTION)
        {
            SetCapture(hwnd);
        }
        break;

    case WM_RBUTTONUP:
        {
            POINT pt;

            if (hwnd == GetCapture())
                /* release capture if we took it on WM_NCRBUTTONDOWN */
                ReleaseCapture();

	    pt.x = SLOWORD(lParam);
	    pt.y = SHIWORD(lParam);
            ClientToScreen(hwnd, &pt);
            SendMessageW( hwnd, WM_CONTEXTMENU, (WPARAM)hwnd, MAKELPARAM(pt.x, pt.y) );
        }
        break;

    case WM_NCRBUTTONUP:
        /*
         * FIXME : we must NOT send WM_CONTEXTMENU on a WM_NCRBUTTONUP (checked
         * in Windows), but what _should_ we do? According to MSDN :
         * "If it is appropriate to do so, the system sends the WM_SYSCOMMAND
         * message to the window". When is it appropriate?
         */
        break;

    case WM_CONTEXTMENU:
        if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD)
            SendMessageW( GetParent(hwnd), msg, wParam, lParam );
        else
        {
            LONG hitcode;
            POINT pt;
            WND *wndPtr = WIN_GetPtr( hwnd );
            HMENU hMenu = wndPtr->hSysMenu;
            WIN_ReleasePtr( wndPtr );
            if (!hMenu) return 0;
            pt.x = SLOWORD(lParam);
            pt.y = SHIWORD(lParam);
            hitcode = NC_HandleNCHitTest(hwnd, pt);

            /* Track system popup if click was in the caption area. */
            if (hitcode==HTCAPTION || hitcode==HTSYSMENU)
               TrackPopupMenu(GetSystemMenu(hwnd, FALSE),
                               TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                               pt.x, pt.y, 0, hwnd, NULL);
        }
        break;

    case WM_NCACTIVATE:
        return NC_HandleNCActivate( hwnd, wParam );

    case WM_NCDESTROY:
        {
            WND *wndPtr = WIN_GetPtr( hwnd );
            if (!wndPtr) return 0;
            if (wndPtr->text) HeapFree( GetProcessHeap(), 0, wndPtr->text );
            wndPtr->text = NULL;
            if (wndPtr->pVScroll) HeapFree( GetProcessHeap(), 0, wndPtr->pVScroll );
            if (wndPtr->pHScroll) HeapFree( GetProcessHeap(), 0, wndPtr->pHScroll );
            wndPtr->pVScroll = wndPtr->pHScroll = NULL;
            WIN_ReleasePtr( wndPtr );
            return 0;
        }

    case WM_PRINT:
        DEFWND_Print(hwnd, (HDC)wParam, lParam);
        return 0;

    case WM_PAINTICON:
    case WM_PAINT:
	{
	    PAINTSTRUCT ps;
	    HDC hdc = BeginPaint( hwnd, &ps );
	    if( hdc )
	    {
              HICON hIcon;
	      if (IsIconic(hwnd) && ((hIcon = (HICON)GetClassLongW( hwnd, GCL_HICON))) )
	      {
                  RECT rc;
                  int x, y;

                  GetClientRect( hwnd, &rc );
                  x = (rc.right - rc.left - GetSystemMetrics(SM_CXICON))/2;
                  y = (rc.bottom - rc.top - GetSystemMetrics(SM_CYICON))/2;
                  TRACE("Painting class icon: vis rect=(%i,%i - %i,%i)\n",
                        ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom );
                  DrawIcon( hdc, x, y, hIcon );
	      }
	      EndPaint( hwnd, &ps );
	    }
	    return 0;
	}

    case WM_SYNCPAINT:
        RedrawWindow ( hwnd, NULL, 0, RDW_ERASENOW | RDW_ERASE | RDW_ALLCHILDREN );
        return 0;

    case WM_SETREDRAW:
        DEFWND_SetRedraw( hwnd, wParam );
        return 0;

    case WM_CLOSE:
	DestroyWindow( hwnd );
	return 0;

    case WM_MOUSEACTIVATE:
        if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD)
	{
	    LONG ret = SendMessageW( GetParent(hwnd), WM_MOUSEACTIVATE, wParam, lParam );
	    if (ret) return ret;
	}

	/* Caption clicks are handled by the NC_HandleNCLButtonDown() */
        return (LOWORD(lParam) >= HTCLIENT) ? MA_ACTIVATE : MA_NOACTIVATE;

    case WM_ACTIVATE:
	/* The default action in Windows is to set the keyboard focus to
	 * the window, if it's being activated and not minimized */
	if (LOWORD(wParam) != WA_INACTIVE) {
            if (!IsIconic(hwnd)) SetFocus(hwnd);
	}
	break;

    case WM_MOUSEWHEEL:
        if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD)
            return SendMessageW( GetParent(hwnd), WM_MOUSEWHEEL, wParam, lParam );
	break;

    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
	{
	    RECT rect;
            HDC hdc = (HDC)wParam;
            HBRUSH hbr = (HBRUSH)GetClassLongW( hwnd, GCL_HBRBACKGROUND );
            if (!hbr) return 0;

            if (GetClassLongW( hwnd, GCL_STYLE ) & CS_PARENTDC)
            {
                /* can't use GetClipBox with a parent DC or we fill the whole parent */
                GetClientRect( hwnd, &rect );
                DPtoLP( hdc, (LPPOINT)&rect, 2 );
            }
            else GetClipBox( hdc, &rect );
            FillRect( hdc, &rect, hbr );
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

    case WM_SETCURSOR:
        if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD)
	{
            /* with the exception of the border around a resizable wnd,
             * give the parent first chance to set the cursor */
            if ((LOWORD(lParam) < HTSIZEFIRST) || (LOWORD(lParam) > HTSIZELAST))
            {
                if (SendMessageW(GetParent(hwnd), WM_SETCURSOR, wParam, lParam)) return TRUE;
            }
        }
	NC_HandleSetCursor( hwnd, wParam, lParam );
        break;

    case WM_SYSCOMMAND:
        return NC_HandleSysCommand( hwnd, wParam, lParam );

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
                HWND top = GetAncestor( hwnd, GA_ROOT );
                if (!(GetClassLongW( top, GCL_STYLE ) & CS_NOCLOSE))
                    PostMessageW( top, WM_SYSCOMMAND, SC_CLOSE, 0 );
	    }
	}
	else if( wParam == VK_F10 )
	        iF10Key = 1;
	     else
	        if( wParam == VK_ESCAPE && (GetKeyState(VK_SHIFT) & 0x8000))
                    SendMessageW( hwnd, WM_SYSCOMMAND, SC_KEYMENU, VK_SPACE );
	break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
	/* Press and release F10 or ALT */
	if (((wParam == VK_MENU) && iMenuSysKey) ||
            ((wParam == VK_F10) && iF10Key))
              SendMessageW( GetAncestor( hwnd, GA_ROOT ), WM_SYSCOMMAND, SC_KEYMENU, 0L );
	iMenuSysKey = iF10Key = 0;
        break;

    case WM_SYSCHAR:
	iMenuSysKey = 0;
	if (wParam == VK_RETURN && IsIconic(hwnd))
        {
            PostMessageW( hwnd, WM_SYSCOMMAND, SC_RESTORE, 0L );
	    break;
        }
	if ((HIWORD(lParam) & KEYDATA_ALT) && wParam)
        {
	    if (wParam == VK_TAB || wParam == VK_ESCAPE) break;
            if (wParam == VK_SPACE && (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD))
                SendMessageW( GetParent(hwnd), msg, wParam, lParam );
	    else
                SendMessageW( hwnd, WM_SYSCOMMAND, SC_KEYMENU, wParam );
        }
	else /* check for Ctrl-Esc */
            if (wParam != VK_ESCAPE) MessageBeep(0);
	break;

    case WM_SHOWWINDOW:
        {
            LONG style = GetWindowLongW( hwnd, GWL_STYLE );
            if (!lParam) return 0; /* sent from ShowWindow */
            if (!(style & WS_POPUP)) return 0;
            if ((style & WS_VISIBLE) && wParam) return 0;
            if (!(style & WS_VISIBLE) && !wParam) return 0;
            if (!GetWindow( hwnd, GW_OWNER )) return 0;
            ShowWindow( hwnd, wParam ? SW_SHOWNOACTIVATE : SW_HIDE );
            break;
        }

    case WM_CANCELMODE:
        if (!(GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD)) EndMenu();
	if (GetCapture() == hwnd) ReleaseCapture();
	break;

    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
	return -1;

    case WM_DROPOBJECT:
	return DRAG_FILE;

    case WM_QUERYDROPOBJECT:
        return (GetWindowLongA( hwnd, GWL_EXSTYLE ) & WS_EX_ACCEPTFILES) != 0;

    case WM_QUERYDRAGICON:
        {
            UINT len;

            HICON hIcon = (HICON)GetClassLongW( hwnd, GCL_HICON );
            HINSTANCE instance = (HINSTANCE)GetWindowLongW( hwnd, GWL_HINSTANCE );
            if (hIcon) return (LRESULT)hIcon;
            for(len=1; len<64; len++)
                if((hIcon = LoadIconW(instance, MAKEINTRESOURCEW(len))))
                    return (LRESULT)hIcon;
            return (LRESULT)LoadIconW(0, IDI_APPLICATIONW);
        }
        break;

    case WM_ISACTIVEICON:
        {
            WND *wndPtr = WIN_GetPtr( hwnd );
            BOOL ret = (wndPtr->flags & WIN_NCACTIVATED) != 0;
            WIN_ReleasePtr( wndPtr );
            return ret;
        }

    case WM_NOTIFYFORMAT:
      if (IsWindowUnicode(hwnd)) return NFR_UNICODE;
      else return NFR_ANSI;

    case WM_QUERYOPEN:
    case WM_QUERYENDSESSION:
	return 1;

    case WM_SETICON:
        if (USER_Driver.pSetWindowIcon)
            return (LRESULT)USER_Driver.pSetWindowIcon( hwnd, (HICON)lParam, (wParam != ICON_SMALL) );
        else
	{
            HICON hOldIcon = (HICON)SetClassLongW( hwnd, (wParam != ICON_SMALL) ? GCL_HICON : GCL_HICONSM,
                                            lParam);
            SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE |
                         SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
            return (LRESULT)hOldIcon;
	}

    case WM_GETICON:
        return GetClassLongW( hwnd, (wParam != ICON_SMALL) ? GCL_HICON : GCL_HICONSM );

    case WM_HELP:
        SendMessageW( GetParent(hwnd), msg, wParam, lParam );
	break;
    }

    return 0;
}



/***********************************************************************
 *		DefWindowProc (USER.107)
 */
LRESULT WINAPI DefWindowProc16( HWND16 hwnd16, UINT16 msg, WPARAM16 wParam,
                                LPARAM lParam )
{
    LRESULT result = 0;
    HWND hwnd = WIN_Handle32( hwnd16 );

    if (!WIN_IsCurrentProcess( hwnd ))
    {
        if (!IsWindow( hwnd )) return 0;
        ERR( "called for other process window %x\n", hwnd );
        return 0;
    }
    SPY_EnterMessage( SPY_DEFWNDPROC16, hwnd, msg, wParam, lParam );

    switch(msg)
    {
    case WM_NCCREATE:
	{
	    CREATESTRUCT16 *cs = MapSL(lParam);
	    /* check for string, as static icons, bitmaps (SS_ICON, SS_BITMAP)
	     * may have child window IDs instead of window name */
	    if (HIWORD(cs->lpszName))
                DEFWND_SetTextA( hwnd, MapSL(cs->lpszName) );
	    result = 1;
	}
        break;

    case WM_NCCALCSIZE:
        {
            RECT rect32;
            CONV_RECT16TO32( MapSL(lParam), &rect32 );
            result = NC_HandleNCCalcSize( hwnd, &rect32 );
            CONV_RECT32TO16( &rect32, MapSL(lParam) );
        }
        break;

    case WM_WINDOWPOSCHANGING:
        result = WINPOS_HandleWindowPosChanging16( hwnd, MapSL(lParam) );
        break;

    case WM_WINDOWPOSCHANGED:
	{
	    WINDOWPOS16 * winPos = MapSL(lParam);
            DEFWND_HandleWindowPosChanged( hwnd, winPos->flags );
	}
        break;

    case WM_GETTEXT:
    case WM_SETTEXT:
        result = DefWindowProcA( hwnd, msg, wParam, (LPARAM)MapSL(lParam) );
        break;

    default:
        result = DefWindowProcA( hwnd, msg, wParam, lParam );
        break;
    }

    SPY_ExitMessage( SPY_RESULT_DEFWND16, hwnd, msg, result, wParam, lParam );
    return result;
}


/***********************************************************************
 *		DefWindowProcA (USER32.@)
 *
 */
LRESULT WINAPI DefWindowProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    LRESULT result = 0;
    HWND full_handle;

    if (!(full_handle = WIN_IsCurrentProcess( hwnd )))
    {
        if (!IsWindow( hwnd )) return 0;
        ERR( "called for other process window %x\n", hwnd );
        return 0;
    }
    hwnd = full_handle;

    SPY_EnterMessage( SPY_DEFWNDPROC, hwnd, msg, wParam, lParam );

    switch(msg)
    {
    case WM_NCCREATE:
	{
	    CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
	    /* check for string, as static icons, bitmaps (SS_ICON, SS_BITMAP)
	     * may have child window IDs instead of window name */
	    if (HIWORD(cs->lpszName))
                DEFWND_SetTextA( hwnd, cs->lpszName );
	    result = 1;
	}
        break;

    case WM_NCCALCSIZE:
        result = NC_HandleNCCalcSize( hwnd, (RECT *)lParam );
        break;

    case WM_WINDOWPOSCHANGING:
        result = WINPOS_HandleWindowPosChanging( hwnd, (WINDOWPOS *)lParam );
        break;

    case WM_WINDOWPOSCHANGED:
	{
	    WINDOWPOS * winPos = (WINDOWPOS *)lParam;
            DEFWND_HandleWindowPosChanged( hwnd, winPos->flags );
	}
        break;

    case WM_GETTEXTLENGTH:
        {
            WND *wndPtr = WIN_GetPtr( hwnd );
            if (wndPtr && wndPtr->text)
                result = WideCharToMultiByte( CP_ACP, 0, wndPtr->text, strlenW(wndPtr->text),
                                              NULL, 0, NULL, NULL );
            WIN_ReleasePtr( wndPtr );
        }
        break;

    case WM_GETTEXT:
        if (wParam)
        {
            LPSTR dest = (LPSTR)lParam;
            WND *wndPtr = WIN_GetPtr( hwnd );

            if (!wndPtr) break;
            if (wndPtr->text)
            {
                if (!WideCharToMultiByte( CP_ACP, 0, wndPtr->text, -1,
                                          dest, wParam, NULL, NULL )) dest[wParam-1] = 0;
                result = strlen( dest );
            }
            else dest[0] = '\0';
            WIN_ReleasePtr( wndPtr );
        }
        break;

    case WM_SETTEXT:
        DEFWND_SetTextA( hwnd, (LPCSTR)lParam );
        if( (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CAPTION) == WS_CAPTION )
	    NC_HandleNCPaint( hwnd , (HRGN)1 );  /* Repaint caption */
	result = 1; /* success. FIXME: check text length */
        break;

    /* for far east users (IMM32) - <hidenori@a2.ctktv.ne.jp> */
    case WM_IME_CHAR:
	{
	    CHAR    chChar1 = (CHAR)( (wParam>>8) & 0xff );
	    CHAR    chChar2 = (CHAR)( wParam & 0xff );

	    SendMessageA( hwnd, WM_CHAR, (WPARAM)chChar1, lParam );
	    if ( IsDBCSLeadByte( chChar1 ) )
		SendMessageA( hwnd, WM_CHAR, (WPARAM)chChar2, lParam );
	}
	break;
    case WM_IME_KEYDOWN:
	result = SendMessageA( hwnd, WM_KEYDOWN, wParam, lParam );
	break;
    case WM_IME_KEYUP:
	result = SendMessageA( hwnd, WM_KEYUP, wParam, lParam );
	break;

    case WM_IME_STARTCOMPOSITION:
    case WM_IME_COMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_SELECT:
	{
	    HWND hwndIME;

	    hwndIME = DEFWND_ImmGetDefaultIMEWnd( hwnd );
	    if (hwndIME)
		result = SendMessageA( hwndIME, msg, wParam, lParam );
	}
	break;
    case WM_IME_SETCONTEXT:
	{
	    HWND hwndIME;

	    hwndIME = DEFWND_ImmGetDefaultIMEWnd( hwnd );
	    if (hwndIME)
		result = DEFWND_ImmIsUIMessageA( hwndIME, msg, wParam, lParam );
	}
	break;

    default:
        result = DEFWND_DefWinProc( hwnd, msg, wParam, lParam );
        break;
    }

    SPY_ExitMessage( SPY_RESULT_DEFWND, hwnd, msg, result, wParam, lParam );
    return result;
}


/***********************************************************************
 *		DefWindowProcW (USER32.@) Calls default window message handler
 *
 * Calls default window procedure for messages not processed
 *  by application.
 *
 *  RETURNS
 *     Return value is dependent upon the message.
*/
LRESULT WINAPI DefWindowProcW(
    HWND hwnd,      /* [in] window procedure receiving message */
    UINT msg,       /* [in] message identifier */
    WPARAM wParam,  /* [in] first message parameter */
    LPARAM lParam )   /* [in] second message parameter */
{
    LRESULT result = 0;
    HWND full_handle;

    if (!(full_handle = WIN_IsCurrentProcess( hwnd )))
    {
        if (!IsWindow( hwnd )) return 0;
        ERR( "called for other process window %x\n", hwnd );
        return 0;
    }
    hwnd = full_handle;
    SPY_EnterMessage( SPY_DEFWNDPROC, hwnd, msg, wParam, lParam );

    switch(msg)
    {
    case WM_NCCREATE:
	{
	    CREATESTRUCTW *cs = (CREATESTRUCTW *)lParam;
	    /* check for string, as static icons, bitmaps (SS_ICON, SS_BITMAP)
	     * may have child window IDs instead of window name */
	    if (HIWORD(cs->lpszName))
	        DEFWND_SetTextW( hwnd, cs->lpszName );
	    result = 1;
	}
        break;

    case WM_NCCALCSIZE:
        result = NC_HandleNCCalcSize( hwnd, (RECT *)lParam );
        break;

    case WM_WINDOWPOSCHANGING:
        result = WINPOS_HandleWindowPosChanging( hwnd, (WINDOWPOS *)lParam );
        break;

    case WM_WINDOWPOSCHANGED:
	{
	    WINDOWPOS * winPos = (WINDOWPOS *)lParam;
            DEFWND_HandleWindowPosChanged( hwnd, winPos->flags );
	}
        break;

    case WM_GETTEXTLENGTH:
        {
            WND *wndPtr = WIN_GetPtr( hwnd );
            if (wndPtr && wndPtr->text) result = (LRESULT)strlenW(wndPtr->text);
            WIN_ReleasePtr( wndPtr );
        }
        break;

    case WM_GETTEXT:
        if (wParam)
        {
            LPWSTR dest = (LPWSTR)lParam;
            WND *wndPtr = WIN_GetPtr( hwnd );

            if (!wndPtr) break;
            if (wndPtr->text)
            {
                lstrcpynW( dest, wndPtr->text, wParam );
                result = strlenW( dest );
            }
            else dest[0] = '\0';
            WIN_ReleasePtr( wndPtr );
        }
        break;

    case WM_SETTEXT:
        DEFWND_SetTextW( hwnd, (LPCWSTR)lParam );
        if( (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CAPTION) == WS_CAPTION )
	    NC_HandleNCPaint( hwnd , (HRGN)1 );  /* Repaint caption */
	result = 1; /* success. FIXME: check text length */
        break;

    /* for far east users (IMM32) - <hidenori@a2.ctktv.ne.jp> */
    case WM_IME_CHAR:
	SendMessageW( hwnd, WM_CHAR, wParam, lParam );
	break;
    case WM_IME_SETCONTEXT:
	{
	    HWND hwndIME;

	    hwndIME = DEFWND_ImmGetDefaultIMEWnd( hwnd );
	    if (hwndIME)
		result = DEFWND_ImmIsUIMessageW( hwndIME, msg, wParam, lParam );
	}
	break;

    default:
        result = DEFWND_DefWinProc( hwnd, msg, wParam, lParam );
        break;
    }
    SPY_ExitMessage( SPY_RESULT_DEFWND, hwnd, msg, result, wParam, lParam );
    return result;
}
