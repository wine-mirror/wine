/*
 * Non-client area window functions
 *
 * Copyright 1994 Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "win.h"
#include "user_private.h"
#include "controls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nonclient);

#define SC_ABOUTWINE            (SC_SCREENSAVE+1)

  /* Some useful macros */
#define HAS_DLGFRAME(style,exStyle) \
    (((exStyle) & WS_EX_DLGMODALFRAME) || \
     (((style) & WS_DLGFRAME) && !((style) & WS_THICKFRAME)))

#define HAS_THICKFRAME(style,exStyle) \
    (((style) & WS_THICKFRAME) && \
     !(((style) & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME))

#define HAS_THINFRAME(style) \
    (((style) & WS_BORDER) || !((style) & (WS_CHILD | WS_POPUP)))

#define HAS_BIGFRAME(style,exStyle) \
    (((style) & (WS_THICKFRAME | WS_DLGFRAME)) || \
     ((exStyle) & WS_EX_DLGMODALFRAME))

#define HAS_STATICOUTERFRAME(style,exStyle) \
    (((exStyle) & (WS_EX_STATICEDGE|WS_EX_DLGMODALFRAME)) == \
     WS_EX_STATICEDGE)

#define HAS_MENU(hwnd,style)  ((((style) & (WS_CHILD | WS_POPUP)) != WS_CHILD) && GetMenu(hwnd))


static void adjust_window_rect( RECT *rect, DWORD style, BOOL menu, DWORD exStyle, NONCLIENTMETRICSW *ncm )
{
    int adjust = 0;

    if ((exStyle & (WS_EX_STATICEDGE|WS_EX_DLGMODALFRAME)) == WS_EX_STATICEDGE)
        adjust = 1; /* for the outer frame always present */
    else if ((exStyle & WS_EX_DLGMODALFRAME) || (style & (WS_THICKFRAME|WS_DLGFRAME)))
        adjust = 2; /* outer */

    if (style & WS_THICKFRAME)
        adjust += ncm->iBorderWidth + ncm->iPaddedBorderWidth; /* The resize border */

    if ((style & (WS_BORDER|WS_DLGFRAME)) || (exStyle & WS_EX_DLGMODALFRAME))
        adjust++; /* The other border */

    InflateRect (rect, adjust, adjust);

    if ((style & WS_CAPTION) == WS_CAPTION)
    {
        if (exStyle & WS_EX_TOOLWINDOW)
            rect->top -= ncm->iSmCaptionHeight + 1;
        else
            rect->top -= ncm->iCaptionHeight + 1;
    }
    if (menu) rect->top -= ncm->iMenuHeight + 1;

    if (exStyle & WS_EX_CLIENTEDGE)
        InflateRect(rect, GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));
}


/***********************************************************************
 *		DrawCaption (USER32.@) Draws a caption bar
 */
BOOL WINAPI DrawCaption( HWND hwnd, HDC hdc, const RECT *rect, UINT flags )
{
    return NtUserDrawCaptionTemp( hwnd, hdc, rect, 0, 0, NULL, flags & 0x103f );
}


/***********************************************************************
 *		DrawCaptionTempA (USER32.@)
 */
BOOL WINAPI DrawCaptionTempA (HWND hwnd, HDC hdc, const RECT *rect, HFONT hFont,
                              HICON hIcon, LPCSTR str, UINT uFlags)
{
    LPWSTR strW;
    INT len;
    BOOL ret = FALSE;

    if (!(uFlags & DC_TEXT) || !str)
        return NtUserDrawCaptionTemp( hwnd, hdc, rect, hFont, hIcon, NULL, uFlags );

    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    if ((strW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, str, -1, strW, len );
        ret = NtUserDrawCaptionTemp( hwnd, hdc, rect, hFont, hIcon, strW, uFlags );
        HeapFree( GetProcessHeap (), 0, strW );
    }
    return ret;
}


/***********************************************************************
 *		AdjustWindowRect (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH AdjustWindowRect( LPRECT rect, DWORD style, BOOL menu )
{
    return AdjustWindowRectEx( rect, style, menu, 0 );
}


/***********************************************************************
 *		AdjustWindowRectEx (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH AdjustWindowRectEx( LPRECT rect, DWORD style, BOOL menu, DWORD exStyle )
{
    NONCLIENTMETRICSW ncm;

    TRACE("(%s) %08lx %d %08lx\n", wine_dbgstr_rect(rect), style, menu, exStyle );

    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );

    adjust_window_rect( rect, style, menu, exStyle, &ncm );
    return TRUE;
}


/***********************************************************************
 *		AdjustWindowRectExForDpi (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH AdjustWindowRectExForDpi( LPRECT rect, DWORD style, BOOL menu,
                                                        DWORD exStyle, UINT dpi )
{
    NONCLIENTMETRICSW ncm;

    TRACE("(%s) %08lx %d %08lx %u\n", wine_dbgstr_rect(rect), style, menu, exStyle, dpi );

    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );

    adjust_window_rect( rect, style, menu, exStyle, &ncm );
    return TRUE;
}


/***********************************************************************
 *           NC_GetInsideRect
 *
 * Get the 'inside' rectangle of a window, i.e. the whole window rectangle
 * but without the borders (if any).
 */
static void NC_GetInsideRect( HWND hwnd, enum coords_relative relative, RECT *rect,
                              DWORD style, DWORD ex_style )
{
    WIN_GetRectangles( hwnd, relative, rect, NULL );

    /* Remove frame from rectangle */
    if (HAS_THICKFRAME( style, ex_style ))
    {
        InflateRect( rect, -GetSystemMetrics(SM_CXFRAME), -GetSystemMetrics(SM_CYFRAME) );
    }
    else if (HAS_DLGFRAME( style, ex_style ))
    {
        InflateRect( rect, -GetSystemMetrics(SM_CXDLGFRAME), -GetSystemMetrics(SM_CYDLGFRAME));
    }
    else if (HAS_THINFRAME( style ))
    {
        InflateRect( rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER) );
    }

    /* We have additional border information if the window
     * is a child (but not an MDI child) */
    if ((style & WS_CHILD) && !(ex_style & WS_EX_MDICHILD))
    {
        if (ex_style & WS_EX_CLIENTEDGE)
            InflateRect (rect, -GetSystemMetrics(SM_CXEDGE), -GetSystemMetrics(SM_CYEDGE));
        if (ex_style & WS_EX_STATICEDGE)
            InflateRect (rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER));
    }
}

LRESULT NC_HandleNCMouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    POINT pt;

    TRACE("hwnd=%p wparam=%#Ix lparam=%#Ix\n", hwnd, wParam, lParam);

    if (wParam != HTHSCROLL && wParam != HTVSCROLL)
        return 0;

    WIN_GetRectangles(hwnd, COORDS_CLIENT, &rect, NULL);

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);
    ScreenToClient(hwnd, &pt);
    pt.x -= rect.left;
    pt.y -= rect.top;
    SCROLL_HandleScrollEvent(hwnd, wParam == HTHSCROLL ? SB_HORZ : SB_VERT, WM_NCMOUSEMOVE, pt);
    return 0;
}

LRESULT NC_HandleNCMouseLeave(HWND hwnd)
{
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    POINT pt = {0, 0};

    TRACE("hwnd=%p\n", hwnd);

    if (style & WS_HSCROLL)
        SCROLL_HandleScrollEvent(hwnd, SB_HORZ, WM_NCMOUSELEAVE, pt);
    if (style & WS_VSCROLL)
        SCROLL_HandleScrollEvent(hwnd, SB_VERT, WM_NCMOUSELEAVE, pt);

    return 0;
}


/***********************************************************************
 *           NC_TrackScrollBar
 *
 * Track a mouse button press on the horizontal or vertical scroll-bar.
 */
static void NC_TrackScrollBar( HWND hwnd, WPARAM wParam, POINT pt )
{
    INT scrollbar;

    if ((wParam & 0xfff0) == SC_HSCROLL)
    {
        if ((wParam & 0x0f) != HTHSCROLL) return;
	scrollbar = SB_HORZ;
    }
    else  /* SC_VSCROLL */
    {
        if ((wParam & 0x0f) != HTVSCROLL) return;
	scrollbar = SB_VERT;
    }
    SCROLL_TrackScrollBar( hwnd, scrollbar, pt );
}


/***********************************************************************
 *           NC_HandleNCLButtonDblClk
 *
 * Handle a WM_NCLBUTTONDBLCLK message. Called from DefWindowProc().
 */
LRESULT NC_HandleNCLButtonDblClk( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    /*
     * if this is an icon, send a restore since we are handling
     * a double click
     */
    if (IsIconic(hwnd))
    {
        SendMessageW( hwnd, WM_SYSCOMMAND, SC_RESTORE, lParam );
        return 0;
    }

    switch(wParam)  /* Hit test */
    {
    case HTCAPTION:
        /* stop processing if WS_MAXIMIZEBOX is missing */
        if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_MAXIMIZEBOX)
            SendMessageW( hwnd, WM_SYSCOMMAND,
                          IsZoomed(hwnd) ? SC_RESTORE : SC_MAXIMIZE, lParam );
        break;

    case HTSYSMENU:
        {
            HMENU hSysMenu = NtUserGetSystemMenu(hwnd, FALSE);
            UINT state = GetMenuState(hSysMenu, SC_CLOSE, MF_BYCOMMAND);

            /* If the close item of the sysmenu is disabled or not present do nothing */
            if ((state & (MF_DISABLED | MF_GRAYED)) || (state == 0xFFFFFFFF))
                break;

            SendMessageW( hwnd, WM_SYSCOMMAND, SC_CLOSE, lParam );
            break;
        }

    case HTHSCROLL:
        SendMessageW( hwnd, WM_SYSCOMMAND, SC_HSCROLL + HTHSCROLL, lParam );
        break;

    case HTVSCROLL:
        SendMessageW( hwnd, WM_SYSCOMMAND, SC_VSCROLL + HTVSCROLL, lParam );
        break;
    }
    return 0;
}


/***********************************************************************
 *           NC_HandleSysCommand
 *
 * Handle a WM_SYSCOMMAND message. Called from DefWindowProc().
 */
LRESULT NC_HandleSysCommand( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    TRACE("hwnd %p WM_SYSCOMMAND %Ix %Ix\n", hwnd, wParam, lParam );

    if (!NtUserMessageCall( hwnd, WM_SYSCOMMAND, wParam, lParam, 0, NtUserDefWindowProc, FALSE ))
        return 0;

    switch (wParam & 0xfff0)
    {
    case SC_CLOSE:
        return SendMessageW( hwnd, WM_CLOSE, 0, 0 );

    case SC_VSCROLL:
    case SC_HSCROLL:
        {
            POINT pt;
            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);
            NC_TrackScrollBar( hwnd, wParam, pt );
        }
        break;

    case SC_TASKLIST:
        WinExec( "taskman.exe", SW_SHOWNORMAL );
        break;

    case SC_SCREENSAVE:
        if (wParam == SC_ABOUTWINE)
        {
            HMODULE hmodule = LoadLibraryA( "shell32.dll" );
            if (hmodule)
            {
                BOOL (WINAPI *aboutproc)(HWND, LPCSTR, LPCSTR, HICON);
                extern const char * CDECL wine_get_version(void);
                char app[256];

                sprintf( app, "Wine %s", wine_get_version() );
                aboutproc = (void *)GetProcAddress( hmodule, "ShellAboutA" );
                if (aboutproc) aboutproc( hwnd, app, NULL, 0 );
                FreeLibrary( hmodule );
            }
        }
        break;

    case SC_HOTKEY:
    case SC_ARRANGE:
    case SC_NEXTWINDOW:
    case SC_PREVWINDOW:
        FIXME("unimplemented WM_SYSCOMMAND %04Ix!\n", wParam);
        break;
    }
    return 0;
}

/***********************************************************************
 *		GetTitleBarInfo (USER32.@)
 * TODO: Handle STATE_SYSTEM_PRESSED
 */
BOOL WINAPI GetTitleBarInfo(HWND hwnd, PTITLEBARINFO tbi) {
    DWORD dwStyle;
    DWORD dwExStyle;

    TRACE("(%p %p)\n", hwnd, tbi);

    if(!tbi) {
        SetLastError(ERROR_NOACCESS);
        return FALSE;
    }

    if(tbi->cbSize != sizeof(TITLEBARINFO)) {
        TRACE("Invalid TITLEBARINFO size: %ld\n", tbi->cbSize);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
    dwExStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    NC_GetInsideRect(hwnd, COORDS_SCREEN, &tbi->rcTitleBar, dwStyle, dwExStyle);

    tbi->rcTitleBar.bottom = tbi->rcTitleBar.top;
    if(dwExStyle & WS_EX_TOOLWINDOW)
        tbi->rcTitleBar.bottom += GetSystemMetrics(SM_CYSMCAPTION);
    else {
        tbi->rcTitleBar.bottom += GetSystemMetrics(SM_CYCAPTION);
        tbi->rcTitleBar.left += GetSystemMetrics(SM_CXSIZE);
    }

    ZeroMemory(tbi->rgstate, sizeof(tbi->rgstate));
    /* Does the title bar always have STATE_SYSTEM_FOCUSABLE?
     * Under XP it seems to
     */
    tbi->rgstate[0] = STATE_SYSTEM_FOCUSABLE;
    if(dwStyle & WS_CAPTION) {
        tbi->rgstate[1] = STATE_SYSTEM_INVISIBLE;
        if(dwStyle & WS_SYSMENU) {
            if(!(dwStyle & (WS_MINIMIZEBOX|WS_MAXIMIZEBOX))) {
                tbi->rgstate[2] = STATE_SYSTEM_INVISIBLE;
                tbi->rgstate[3] = STATE_SYSTEM_INVISIBLE;
            }
            else {
                if(!(dwStyle & WS_MINIMIZEBOX))
                    tbi->rgstate[2] = STATE_SYSTEM_UNAVAILABLE;
                if(!(dwStyle & WS_MAXIMIZEBOX))
                    tbi->rgstate[3] = STATE_SYSTEM_UNAVAILABLE;
            }
            if(!(dwExStyle & WS_EX_CONTEXTHELP))
                tbi->rgstate[4] = STATE_SYSTEM_INVISIBLE;
            if(GetClassLongW(hwnd, GCL_STYLE) & CS_NOCLOSE)
                tbi->rgstate[5] = STATE_SYSTEM_UNAVAILABLE;
        }
        else {
            tbi->rgstate[2] = STATE_SYSTEM_INVISIBLE;
            tbi->rgstate[3] = STATE_SYSTEM_INVISIBLE;
            tbi->rgstate[4] = STATE_SYSTEM_INVISIBLE;
            tbi->rgstate[5] = STATE_SYSTEM_INVISIBLE;
        }
    }
    else
        tbi->rgstate[0] |= STATE_SYSTEM_INVISIBLE;
    return TRUE;
}
