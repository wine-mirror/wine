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

#include "config.h"
#include "wine/port.h"
#include <stdio.h>
#include "wine/unicode.h"

#define OEMRESOURCE

#include <windows.h>
#include <wine/debug.h>
#include "explorer_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(explorer);

#define DESKTOP_CLASS_ATOM ((LPCWSTR)MAKEINTATOM(32769))
#define DESKTOP_ALL_ACCESS 0x01ff

static BOOL using_root;

/* screen saver handler */
static BOOL start_screensaver( void )
{
    if (using_root)
    {
        const char *argv[3] = { "xdg-screensaver", "activate", NULL };
        int pid = spawnvp( _P_NOWAIT, argv[0], argv );
        if (pid > 0)
        {
            WINE_TRACE( "started process %d\n", pid );
            return TRUE;
        }
    }
    return FALSE;
}

/* window procedure for the desktop window */
static LRESULT WINAPI desktop_wnd_proc( HWND hwnd, UINT message, WPARAM wp, LPARAM lp )
{
    WINE_TRACE( "got msg %04x wp %lx lp %lx\n", message, wp, lp );

    switch(message)
    {
    case WM_SYSCOMMAND:
        switch(wp & 0xfff0)
        {
        case SC_CLOSE:
            ExitWindows( 0, 0 );
            break;
        case SC_SCREENSAVE:
            return start_screensaver();
        }
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
static unsigned long create_desktop( const WCHAR *name, unsigned int width, unsigned int height )
{
    static const WCHAR rootW[] = {'r','o','o','t',0};
    HMODULE x11drv = GetModuleHandleA( "winex11.drv" );
    HDESK desktop;
    unsigned long xwin = 0;
    unsigned long (CDECL *create_desktop_func)(unsigned int, unsigned int);

    desktop = CreateDesktopW( name, NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL );
    if (!desktop)
    {
        WINE_ERR( "failed to create desktop %s error %d\n", wine_dbgstr_w(name), GetLastError() );
        ExitProcess( 1 );
    }
    /* magic: desktop "root" means use the X11 root window */
    if (x11drv && strcmpiW( name, rootW ))
    {
        create_desktop_func = (void *)GetProcAddress( x11drv, "wine_create_desktop" );
        if (create_desktop_func) xwin = create_desktop_func( width, height );
    }
    SetThreadDesktop( desktop );
    return xwin;
}

/* parse the desktop size specification */
static BOOL parse_size( const WCHAR *size, unsigned int *width, unsigned int *height )
{
    WCHAR *end;

    *width = strtoulW( size, &end, 10 );
    if (end == size) return FALSE;
    if (*end != 'x') return FALSE;
    size = end + 1;
    *height = strtoulW( size, &end, 10 );
    return !*end;
}

/* retrieve the desktop name to use if not specified on the command line */
static const WCHAR *get_default_desktop_name(void)
{
    static const WCHAR desktopW[] = {'D','e','s','k','t','o','p',0};
    static const WCHAR defaultW[] = {'D','e','f','a','u','l','t',0};
    static const WCHAR explorer_keyW[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
                                          'E','x','p','l','o','r','e','r',0};
    static WCHAR buffer[MAX_PATH];
    DWORD size = sizeof(buffer);
    HDESK desk = GetThreadDesktop( GetCurrentThreadId() );
    WCHAR *ret = NULL;
    HKEY hkey;

    if (desk && GetUserObjectInformationW( desk, UOI_NAME, buffer, sizeof(buffer)/sizeof(WCHAR), NULL ))
    {
        if (strcmpiW( buffer, defaultW )) return buffer;
    }

    /* @@ Wine registry key: HKCU\Software\Wine\Explorer */
    if (!RegOpenKeyW( HKEY_CURRENT_USER, explorer_keyW, &hkey ))
    {
        if (!RegQueryValueExW( hkey, desktopW, 0, NULL, (LPBYTE)buffer, &size )) ret = buffer;
        RegCloseKey( hkey );
    }
    return ret;
}

/* retrieve the default desktop size from the registry */
static BOOL get_default_desktop_size( const WCHAR *name, unsigned int *width, unsigned int *height )
{
    static const WCHAR desktop_keyW[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
                                         'E','x','p','l','o','r','e','r','\\',
                                         'D','e','s','k','t','o','p','s',0};
    HKEY hkey;
    WCHAR buffer[64];
    DWORD size = sizeof(buffer);
    BOOL found = FALSE;

    *width = 800;
    *height = 600;

    /* @@ Wine registry key: HKCU\Software\Wine\Explorer\Desktops */
    if (!RegOpenKeyW( HKEY_CURRENT_USER, desktop_keyW, &hkey ))
    {
        if (!RegQueryValueExW( hkey, name, 0, NULL, (LPBYTE)buffer, &size ))
        {
            found = TRUE;
            if (!parse_size( buffer, width, height )) *width = *height = 0;
        }
        RegCloseKey( hkey );
    }
    return found;
}

static void initialize_display_settings( HWND desktop )
{
    static const WCHAR display_device_guid_propW[] = {
        '_','_','w','i','n','e','_','d','i','s','p','l','a','y','_',
        'd','e','v','i','c','e','_','g','u','i','d',0 };
    GUID guid;
    RPC_CSTR guid_str;
    ATOM guid_atom;
    DEVMODEW dmW;

    UuidCreate( &guid );
    UuidToStringA( &guid, &guid_str );
    WINE_TRACE( "display guid %s\n", guid_str );

    guid_atom = GlobalAddAtomA( (LPCSTR)guid_str );
    SetPropW( desktop, display_device_guid_propW, ULongToHandle(guid_atom) );

    RpcStringFreeA( &guid_str );

    /* Store current display mode in the registry */
    if (EnumDisplaySettingsExW( NULL, ENUM_CURRENT_SETTINGS, &dmW, 0 ))
    {
        WINE_TRACE( "Current display mode %ux%u %u bpp %u Hz\n", dmW.dmPelsWidth,
                    dmW.dmPelsHeight, dmW.dmBitsPerPel, dmW.dmDisplayFrequency );
        ChangeDisplaySettingsExW( NULL, &dmW, 0,
                                  CDS_GLOBAL | CDS_NORESET | CDS_UPDATEREGISTRY,
                                  NULL );
    }
}

static void set_desktop_window_title( HWND hwnd, const WCHAR *name )
{
    static const WCHAR desktop_nameW[] = {'W','i','n','e',' ','d','e','s','k','t','o','p',0};
    static const WCHAR desktop_name_separatorW[] = {' ', '-', ' ', 0};
    WCHAR *window_titleW = NULL;
    int window_title_len;

    if (!name[0])
    {
        SetWindowTextW( hwnd, desktop_nameW );
        return;
    }

    window_title_len = strlenW(name) * sizeof(WCHAR)
                     + sizeof(desktop_name_separatorW)
                     + sizeof(desktop_nameW);
    window_titleW = HeapAlloc( GetProcessHeap(), 0, window_title_len );
    if (!window_titleW)
    {
        SetWindowTextW( hwnd, desktop_nameW );
        return;
    }

    strcpyW( window_titleW, name );
    strcatW( window_titleW, desktop_name_separatorW );
    strcatW( window_titleW, desktop_nameW );

    SetWindowTextW( hwnd, window_titleW );
    HeapFree( GetProcessHeap(), 0, window_titleW );
}

/* main desktop management function */
void manage_desktop( WCHAR *arg )
{
    static const WCHAR defaultW[] = {'D','e','f','a','u','l','t',0};
    static const WCHAR messageW[] = {'M','e','s','s','a','g','e',0};
    MSG msg;
    HWND hwnd, msg_hwnd;
    unsigned long xwin = 0;
    unsigned int width, height;
    WCHAR *cmdline = NULL;
    WCHAR *p = arg;
    const WCHAR *name = NULL;

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
        name = arg;
        if ((p = strchrW( arg, ',' ))) *p++ = 0;
        if (!p || !parse_size( p, &width, &height ))
            get_default_desktop_size( name, &width, &height );
    }
    else if ((name = get_default_desktop_name()))
    {
        if (!get_default_desktop_size( name, &width, &height )) width = height = 0;
    }
    else  /* check for the X11 driver key for backwards compatibility (to be removed) */
    {
        static const WCHAR desktopW[] = {'D','e','s','k','t','o','p',0};
        static const WCHAR x11_keyW[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
                                         'X','1','1',' ','D','r','i','v','e','r',0};
        HKEY hkey;
        WCHAR buffer[64];
        DWORD size = sizeof(buffer);

        width = height = 0;
        /* @@ Wine registry key: HKCU\Software\Wine\X11 Driver */
        if (!RegOpenKeyW( HKEY_CURRENT_USER, x11_keyW, &hkey ))
        {
            if (!RegQueryValueExW( hkey, desktopW, 0, NULL, (LPBYTE)buffer, &size ))
            {
                name = defaultW;
                if (!parse_size( buffer, &width, &height )) width = height = 0;
            }
            RegCloseKey( hkey );
        }
    }

    if (name && width && height) xwin = create_desktop( name, width, height );

    if (!xwin) using_root = TRUE; /* using the root window */

    /* create the desktop window */
    hwnd = CreateWindowExW( 0, DESKTOP_CLASS_ATOM, NULL,
                            WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                            GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
                            GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN),
                            0, 0, 0, NULL );

    /* create the HWND_MESSAGE parent */
    msg_hwnd = CreateWindowExW( 0, messageW, NULL, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                0, 0, 100, 100, 0, 0, 0, NULL );

    if (hwnd == GetDesktopWindow())
    {
        HMODULE shell32;
        void (WINAPI *pShellDDEInit)( BOOL );

        SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)desktop_wnd_proc );
        SendMessageW( hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIconW( 0, MAKEINTRESOURCEW(OIC_WINLOGO)));
        if (name) set_desktop_window_title( hwnd, name );
        SystemParametersInfoA( SPI_SETDESKPATTERN, -1, NULL, FALSE );
        SetDeskWallPaper( (LPSTR)-1 );
        initialize_display_settings( hwnd );
        initialize_appbar();
        initialize_systray( using_root );

        if ((shell32 = LoadLibraryA( "shell32.dll" )) &&
            (pShellDDEInit = (void *)GetProcAddress( shell32, (LPCSTR)188)))
        {
            pShellDDEInit( TRUE );
        }
    }
    else
    {
        DestroyWindow( hwnd );  /* someone beat us to it */
        hwnd = 0;
    }

    if (GetAncestor( msg_hwnd, GA_PARENT )) DestroyWindow( msg_hwnd );  /* someone beat us to it */

    /* if we have a command line, execute it */
    if (cmdline)
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;

        memset( &si, 0, sizeof(si) );
        si.cb = sizeof(si);
        WINE_TRACE( "starting %s\n", wine_dbgstr_w(cmdline) );
        if (CreateProcessW( NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ))
        {
            CloseHandle( pi.hThread );
            CloseHandle( pi.hProcess );
        }
    }

    /* run the desktop message loop */
    if (hwnd)
    {
        WINE_TRACE( "desktop message loop starting on hwnd %p\n", hwnd );
        while (GetMessageW( &msg, 0, 0, 0 )) DispatchMessageW( &msg );
        WINE_TRACE( "desktop message loop exiting for hwnd %p\n", hwnd );
    }

    ExitProcess( 0 );
}
