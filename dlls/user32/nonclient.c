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


static HICON NC_IconForWindow( HWND hwnd )
{
    HICON hIcon = 0;
    WND *wndPtr = WIN_GetPtr( hwnd );

    if (wndPtr && wndPtr != WND_OTHER_PROCESS && wndPtr != WND_DESKTOP)
    {
        hIcon = wndPtr->hIconSmall;
        if (!hIcon) hIcon = wndPtr->hIcon;
        WIN_ReleasePtr( wndPtr );
    }
    if (!hIcon) hIcon = (HICON) GetClassLongPtrW( hwnd, GCLP_HICONSM );
    if (!hIcon) hIcon = (HICON) GetClassLongPtrW( hwnd, GCLP_HICON );

    /* If there is no icon specified and this is not a modal dialog,
     * get the default one.
     */
    if (!hIcon && !(GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_DLGMODALFRAME))
        hIcon = LoadImageW(0, (LPCWSTR)IDI_WINLOGO, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                           GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR | LR_SHARED);
    return hIcon;
}

/* Draws the bar part(ie the big rectangle) of the caption */
static void NC_DrawCaptionBar (HDC hdc, const RECT *rect, DWORD dwStyle, 
                               BOOL active, BOOL gradient)
{
    if (gradient)
    {
        TRIVERTEX vertices[4];
        DWORD colLeft = 
            GetSysColor (active ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION);
        DWORD colRight = 
            GetSysColor (active ? COLOR_GRADIENTACTIVECAPTION 
                                : COLOR_GRADIENTINACTIVECAPTION);
        int buttonsAreaSize = GetSystemMetrics(SM_CYCAPTION) - 1;
        static GRADIENT_RECT mesh[] = {{0, 1}, {1, 2}, {2, 3}};

        vertices[0].Red   = vertices[1].Red   = GetRValue (colLeft) << 8;
        vertices[0].Green = vertices[1].Green = GetGValue (colLeft) << 8;
        vertices[0].Blue  = vertices[1].Blue  = GetBValue (colLeft) << 8;
        vertices[0].Alpha = vertices[1].Alpha = 0xff00;
        vertices[2].Red   = vertices[3].Red   = GetRValue (colRight) << 8;
        vertices[2].Green = vertices[3].Green = GetGValue (colRight) << 8;
        vertices[2].Blue  = vertices[3].Blue  = GetBValue (colRight) << 8;
        vertices[2].Alpha = vertices[3].Alpha = 0xff00;

        if ((dwStyle & WS_SYSMENU)
            && ((dwStyle & WS_MAXIMIZEBOX) || (dwStyle & WS_MINIMIZEBOX)))
            buttonsAreaSize += 2 * (GetSystemMetrics(SM_CXSIZE) + 1);

        /* area behind icon; solid filled with left color */
        vertices[0].x = rect->left;
        vertices[0].y = rect->top;
        if (dwStyle & WS_SYSMENU)
            vertices[1].x = min (rect->left + GetSystemMetrics(SM_CXSMICON), rect->right);
        else
            vertices[1].x = vertices[0].x;
        vertices[1].y = rect->bottom;

        /* area behind text; gradient */
        vertices[2].x = max (vertices[1].x, rect->right - buttonsAreaSize);
        vertices[2].y = rect->top;

        /* area behind buttons; solid filled with right color */
        vertices[3].x = rect->right;
        vertices[3].y = rect->bottom;

        GdiGradientFill (hdc, vertices, 4, mesh, 3, GRADIENT_FILL_RECT_H);
    }
    else
        FillRect (hdc, rect, GetSysColorBrush (active ?
                  COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION));
}

/***********************************************************************
 *		DrawCaption (USER32.@) Draws a caption bar
 *
 * PARAMS
 *     hwnd   [I]
 *     hdc    [I]
 *     lpRect [I]
 *     uFlags [I]
 *
 * RETURNS
 *     Success:
 *     Failure:
 */

BOOL WINAPI
DrawCaption (HWND hwnd, HDC hdc, const RECT *lpRect, UINT uFlags)
{
    return DrawCaptionTempW (hwnd, hdc, lpRect, 0, 0, NULL, uFlags & 0x103F);
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
        return DrawCaptionTempW( hwnd, hdc, rect, hFont, hIcon, NULL, uFlags );

    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    if ((strW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, str, -1, strW, len );
        ret = DrawCaptionTempW (hwnd, hdc, rect, hFont, hIcon, strW, uFlags);
        HeapFree( GetProcessHeap (), 0, strW );
    }
    return ret;
}


/***********************************************************************
 *		DrawCaptionTempW (USER32.@)
 */
BOOL WINAPI DrawCaptionTempW (HWND hwnd, HDC hdc, const RECT *rect, HFONT hFont,
                              HICON hIcon, LPCWSTR str, UINT uFlags)
{
    RECT   rc = *rect;

    TRACE("(%p,%p,%p,%p,%p,%s,%08x)\n",
          hwnd, hdc, rect, hFont, hIcon, debugstr_w(str), uFlags);

    /* drawing background */
    if (uFlags & DC_INBUTTON) {
        FillRect (hdc, &rc, GetSysColorBrush (COLOR_3DFACE));

        if (uFlags & DC_ACTIVE) {
            HBRUSH hbr = SelectObject (hdc, SYSCOLOR_Get55AABrush());
            PatBlt (hdc, rc.left, rc.top,
                      rc.right-rc.left, rc.bottom-rc.top, 0xFA0089);
            SelectObject (hdc, hbr);
        }
    }
    else {
        DWORD style = GetWindowLongW (hwnd, GWL_STYLE);
        NC_DrawCaptionBar (hdc, &rc, style, uFlags & DC_ACTIVE, uFlags & DC_GRADIENT);
    }


    /* drawing icon */
    if ((uFlags & DC_ICON) && !(uFlags & DC_SMALLCAP)) {
        POINT pt;

        pt.x = rc.left + 2;
        pt.y = (rc.bottom + rc.top - GetSystemMetrics(SM_CYSMICON)) / 2;

        if (!hIcon) hIcon = NC_IconForWindow(hwnd);
        NtUserDrawIconEx( hdc, pt.x, pt.y, hIcon, GetSystemMetrics(SM_CXSMICON),
                          GetSystemMetrics(SM_CYSMICON), 0, 0, DI_NORMAL );
        rc.left = pt.x + GetSystemMetrics( SM_CXSMICON );
    }

    /* drawing text */
    if (uFlags & DC_TEXT) {
        HFONT hOldFont;
        WCHAR text[128];

        if (uFlags & DC_INBUTTON)
            SetTextColor (hdc, GetSysColor (COLOR_BTNTEXT));
        else if (uFlags & DC_ACTIVE)
            SetTextColor (hdc, GetSysColor (COLOR_CAPTIONTEXT));
        else
            SetTextColor (hdc, GetSysColor (COLOR_INACTIVECAPTIONTEXT));

        SetBkMode (hdc, TRANSPARENT);

        if (hFont)
            hOldFont = SelectObject (hdc, hFont);
        else {
            NONCLIENTMETRICSW nclm;
            HFONT hNewFont;
            nclm.cbSize = sizeof(NONCLIENTMETRICSW);
            SystemParametersInfoW (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
            hNewFont = CreateFontIndirectW ((uFlags & DC_SMALLCAP) ?
                &nclm.lfSmCaptionFont : &nclm.lfCaptionFont);
            hOldFont = SelectObject (hdc, hNewFont);
        }

        if (!str)
        {
            if (!GetWindowTextW( hwnd, text, ARRAY_SIZE( text ))) text[0] = 0;
            str = text;
        }
        rc.left += 2;
        DrawTextW( hdc, str, -1, &rc, ((uFlags & 0x4000) ? DT_CENTER : DT_LEFT) |
                   DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS );

        if (hFont)
            SelectObject (hdc, hOldFont);
        else
            DeleteObject (SelectObject (hdc, hOldFont));
    }

    /* drawing focus ??? */
    if (uFlags & 0x2000)
        FIXME("undocumented flag (0x2000)!\n");

    return FALSE;
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
 *           NC_HandleSetCursor
 *
 * Handle a WM_SETCURSOR message. Called from DefWindowProc().
 */
LRESULT NC_HandleSetCursor( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    hwnd = WIN_GetFullHandle( (HWND)wParam );

    switch((short)LOWORD(lParam))
    {
    case HTERROR:
        {
            WORD msg = HIWORD( lParam );
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_MBUTTONDOWN) ||
                (msg == WM_RBUTTONDOWN) || (msg == WM_XBUTTONDOWN))
                MessageBeep(0);
        }
        break;

    case HTCLIENT:
        {
            HCURSOR hCursor = (HCURSOR)GetClassLongPtrW(hwnd, GCLP_HCURSOR);
            if(hCursor) {
                NtUserSetCursor(hCursor);
                return TRUE;
            }
            return FALSE;
        }

    case HTLEFT:
    case HTRIGHT:
        return (LRESULT)NtUserSetCursor( LoadCursorA( 0, (LPSTR)IDC_SIZEWE ) );

    case HTTOP:
    case HTBOTTOM:
        return (LRESULT)NtUserSetCursor( LoadCursorA( 0, (LPSTR)IDC_SIZENS ) );

    case HTTOPLEFT:
    case HTBOTTOMRIGHT:
        return (LRESULT)NtUserSetCursor( LoadCursorA( 0, (LPSTR)IDC_SIZENWSE ) );

    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
        return (LRESULT)NtUserSetCursor( LoadCursorA( 0, (LPSTR)IDC_SIZENESW ) );
    }

    /* Default cursor: arrow */
    return (LRESULT)NtUserSetCursor( LoadCursorA( 0, (LPSTR)IDC_ARROW ) );
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
