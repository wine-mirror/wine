/*
 * Explorer desktop support
 *
 * Copyright 2006 Alexandre Julliard
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

#include <stdio.h>

#define OEMRESOURCE

#include <windows.h>
#include <wine/debug.h>
#include "explorer_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(explorer);

#define DESKTOP_CLASS_ATOM ((LPCWSTR)MAKEINTATOM(32769))
#define DESKTOP_ALL_ACCESS 0x01ff

extern HANDLE __wine_make_process_system(void);

static BOOL using_root;

/* window procedure for the desktop window */
static LRESULT WINAPI desktop_wnd_proc( HWND hwnd, UINT message, WPARAM wp, LPARAM lp )
{
    WINE_TRACE( "got msg %x wp %lx lp %lx\n", message, wp, lp );

    switch(message)
    {
    case WM_SYSCOMMAND:
        if ((wp & 0xfff0) == SC_CLOSE) ExitWindows( 0, 0 );
        return 0;

    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    case WM_SETCURSOR:
        return (LRESULT)SetCursor( LoadCursorA( 0, (LPSTR)IDC_ARROW ) );

    case WM_NCHITTEST:
        return HTCLIENT;

    case WM_ERASEBKGND:
        if (!using_root) PaintDesktop( (HDC)wp );
        return TRUE;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint( hwnd, &ps );
            if (!using_root && ps.fErase) PaintDesktop( ps.hdc );
            EndPaint( hwnd, &ps );
        }
        return 0;

    default:
        return DefWindowProcW( hwnd, message, wp, lp );
    }
}

/* create the desktop and the associated X11 window, and make it the current desktop */
static unsigned long create_desktop( const char *name, unsigned int width, unsigned int height )
{
    HMODULE x11drv = GetModuleHandleA( "winex11.drv" );
    HDESK desktop;
    unsigned long xwin = 0;
    unsigned long (*create_desktop_func)(unsigned int, unsigned int);

    desktop = CreateDesktopA( name, NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL );
    if (!desktop)
    {
        WINE_ERR( "failed to create desktop %s error %d\n", wine_dbgstr_a(name), GetLastError() );
        ExitProcess( 1 );
    }
    /* magic: desktop "root" means use the X11 root window */
    if (x11drv && strcasecmp( name, "root" ))
    {
        create_desktop_func = (void *)GetProcAddress( x11drv, "wine_create_desktop" );
        if (create_desktop_func) xwin = create_desktop_func( width, height );
    }
    SetThreadDesktop( desktop );
    return xwin;
}

/* retrieve the default desktop size from the X11 driver config */
/* FIXME: this is for backwards compatibility, should probably be changed */
static BOOL get_default_desktop_size( unsigned int *width, unsigned int *height )
{
    HKEY hkey;
    char buffer[64];
    DWORD size = sizeof(buffer);
    BOOL ret = FALSE;

    /* @@ Wine registry key: HKCU\Software\Wine\X11 Driver */
    if (RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\X11 Driver", &hkey )) return FALSE;
    if (!RegQueryValueExA( hkey, "Desktop", 0, NULL, (LPBYTE)buffer, &size ))
        ret = (sscanf( buffer, "%ux%u", width, height ) == 2);
    RegCloseKey( hkey );
    return ret;
}

/* main desktop management function */
void manage_desktop( char *arg )
{
    MSG msg;
    HWND hwnd;
    unsigned long xwin = 0;
    unsigned int width, height;
    char *cmdline = NULL;
    char *p = arg;
    static const WCHAR desktop_nameW[] = {'W','i','n','e',' ','d','e','s','k','t','o','p',0};

    /* get the rest of the command line (if any) */
    while (*p && !isspace(*p)) p++;
    if (*p)
    {
        *p++ = 0;
        while (*p && isspace(*p)) p++;
        if (*p) cmdline = p;
    }

    /* parse the desktop option */
    /* the option is of the form /desktop=name[,widthxheight] */
    if (*arg == '=' || *arg == ',')
    {
        arg++;
        if ((p = strchr( arg, ',' ))) *p++ = 0;
        if (!p || sscanf( p, "%ux%u", &width, &height ) != 2)
        {
            width = 800;
            height = 600;
        }
        xwin = create_desktop( arg, width, height );
    }
    else if (get_default_desktop_size( &width, &height ))
    {
        xwin = create_desktop( "Default", width, height );
    }

    if (!xwin)  /* using the root window */
    {
        using_root = TRUE;
        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);
    }

    /* create the desktop window */
    hwnd = CreateWindowExW( 0, DESKTOP_CLASS_ATOM, NULL,
                            WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                            0, 0, width, height, 0, 0, 0, NULL );
    if (hwnd == GetDesktopWindow())
    {
        SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)desktop_wnd_proc );
        SendMessageW( hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIconW( 0, MAKEINTRESOURCEW(OIC_WINLOGO)));
        SetWindowTextW( hwnd, desktop_nameW );
        SystemParametersInfoA( SPI_SETDESKPATTERN, -1, NULL, FALSE );
        SetDeskWallPaper( (LPSTR)-1 );
        initialize_diskarbitration();
        initialize_hal();
        initialize_systray();
    }
    else
    {
        DestroyWindow( hwnd );  /* someone beat us to it */
        hwnd = 0;
    }

    /* if we have a command line, execute it */
    if (cmdline)
    {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;

        memset( &si, 0, sizeof(si) );
        si.cb = sizeof(si);
        WINE_TRACE( "starting %s\n", wine_dbgstr_a(cmdline) );
        if (CreateProcessA( NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ))
        {
            CloseHandle( pi.hThread );
            CloseHandle( pi.hProcess );
        }
    }

    /* run the desktop message loop */
    if (hwnd)
    {
        /* we don't use the system process event, the server
         * posts a WM_CLOSE when the last desktop user is gone */
        CloseHandle( __wine_make_process_system() );

        WINE_TRACE( "desktop message loop starting on hwnd %p\n", hwnd );
        while (GetMessageW( &msg, 0, 0, 0 )) DispatchMessageW( &msg );
        WINE_TRACE( "desktop message loop exiting for hwnd %p\n", hwnd );
    }

    ExitProcess( 0 );
}
