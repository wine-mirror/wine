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

#include "user_private.h"
#include "controls.h"
#include "wine/debug.h"


#define SC_ABOUTWINE            (SC_SCREENSAVE+1)

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
 *           NC_HandleSysCommand
 *
 * Handle a WM_SYSCOMMAND message. Called from DefWindowProc().
 */
LRESULT NC_HandleSysCommand( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    if (!NtUserMessageCall( hwnd, WM_SYSCOMMAND, wParam, lParam, 0, NtUserDefWindowProc, FALSE ))
        return 0;

    switch (wParam & 0xfff0)
    {
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
                const char * (CDECL *p_wine_get_version)(void);
                char app[256];

                p_wine_get_version = (void *)GetProcAddress( GetModuleHandleW(L"ntdll.dll"), "wine_get_version" );
                aboutproc = (void *)GetProcAddress( hmodule, "ShellAboutA" );
                if (p_wine_get_version && aboutproc)
                {
                    snprintf( app, ARRAY_SIZE(app), "Wine %s", p_wine_get_version() );
                    aboutproc( hwnd, app, NULL, 0 );
                }
                FreeLibrary( hmodule );
            }
        }
        break;
    }
    return 0;
}

static void user_draw_mdi_button( HDC hdc, enum NONCLIENT_BUTTON_TYPE type, RECT rect, BOOL down, BOOL grayed )
{
    UINT flags;

    switch (type)
    {
    case MENU_CLOSE_BUTTON:
        flags = DFCS_CAPTIONCLOSE;
        break;
    case MENU_MIN_BUTTON:
        flags = DFCS_CAPTIONMIN;
        break;
    case MENU_MAX_BUTTON:
        flags = DFCS_CAPTIONMAX;
        break;
    case MENU_RESTORE_BUTTON:
        flags = DFCS_CAPTIONRESTORE;
        break;
    case MENU_HELP_BUTTON:
        flags = DFCS_CAPTIONHELP;
        break;
    default:
        return;
    }

    if (down)
        flags |= DFCS_PUSHED;
    if (grayed)
        flags |= DFCS_INACTIVE;

    DrawFrameControl( hdc, &rect, DFC_CAPTION, flags );
}

void WINAPI USER_NonClientButtonDraw( HWND hwnd, HDC hdc, enum NONCLIENT_BUTTON_TYPE type,
                                      RECT rect, BOOL down, BOOL grayed )
{
    switch (type)
    {
    case MENU_CLOSE_BUTTON:
    case MENU_MIN_BUTTON:
    case MENU_MAX_BUTTON:
    case MENU_RESTORE_BUTTON:
    case MENU_HELP_BUTTON:
        user_draw_mdi_button( hdc, type, rect, down, grayed );
        return;
    }
}
