/*
 * Explorer desktop support
 *
 * Copyright 2006 Alexandre Julliard
 * Copyright 2013 Hans Leidekker for CodeWeavers
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

#define COBJMACROS
#define OEMRESOURCE
#include <windows.h>
#include <rpc.h>
#include <shlobj.h>
#include <shellapi.h>

#include "wine/gdi_driver.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "explorer_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(explorer);

#define DESKTOP_CLASS_ATOM ((LPCWSTR)MAKEINTATOM(32769))
#define DESKTOP_ALL_ACCESS 0x01ff

static HMODULE graphics_driver;
static BOOL using_root;

struct launcher
{
    WCHAR *path;
    HICON  icon;
    WCHAR *title;
};

static WCHAR *desktop_folder;
static WCHAR *desktop_folder_public;

static int icon_cx, icon_cy, icon_offset_cx, icon_offset_cy;
static int title_cx, title_cy, title_offset_cx, title_offset_cy;
static int desktop_width, launcher_size, launchers_per_row;

static struct launcher **launchers;
static unsigned int nb_launchers, nb_allocated;

static RECT get_icon_rect( unsigned int index )
{
    RECT rect;
    unsigned int row = index / launchers_per_row;
    unsigned int col = index % launchers_per_row;

    rect.left   = col * launcher_size + icon_offset_cx;
    rect.right  = rect.left + icon_cx;
    rect.top    = row * launcher_size + icon_offset_cy;
    rect.bottom = rect.top + icon_cy;
    return rect;
}

static RECT get_title_rect( unsigned int index )
{
    RECT rect;
    unsigned int row = index / launchers_per_row;
    unsigned int col = index % launchers_per_row;

    rect.left   = col * launcher_size + title_offset_cx;
    rect.right  = rect.left + title_cx;
    rect.top    = row * launcher_size + title_offset_cy;
    rect.bottom = rect.top + title_cy;
    return rect;
}

static const struct launcher *launcher_from_point( int x, int y )
{
    RECT icon, title;
    unsigned int index = x / launcher_size + (y / launcher_size) * launchers_per_row;

    if (index >= nb_launchers) return NULL;

    icon = get_icon_rect( index );
    title = get_title_rect( index );
    if ((x < icon.left || x > icon.right || y < icon.top || y > icon.bottom) &&
        (x < title.left || x > title.right || y < title.top || y > title.bottom)) return NULL;
    return launchers[index];
}

static void draw_launchers( HDC hdc, RECT update_rect )
{
    COLORREF color = SetTextColor( hdc, RGB(255,255,255) ); /* FIXME: depends on background color */
    int mode = SetBkMode( hdc, TRANSPARENT );
    unsigned int i;
    LOGFONTW lf;
    HFONT font;

    SystemParametersInfoW( SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0 );
    font = SelectObject( hdc, CreateFontIndirectW( &lf ) );

    for (i = 0; i < nb_launchers; i++)
    {
        RECT dummy, icon = get_icon_rect( i ), title = get_title_rect( i );

        if (IntersectRect( &dummy, &icon, &update_rect ))
            DrawIconEx( hdc, icon.left, icon.top, launchers[i]->icon, icon_cx, icon_cy,
                        0, 0, DI_DEFAULTSIZE|DI_NORMAL );

        if (IntersectRect( &dummy, &title, &update_rect ))
            DrawTextW( hdc, launchers[i]->title, -1, &title,
                       DT_CENTER|DT_WORDBREAK|DT_EDITCONTROL|DT_END_ELLIPSIS );
    }

    SelectObject( hdc, font );
    SetTextColor( hdc, color );
    SetBkMode( hdc, mode );
}

static void do_launch( const struct launcher *launcher )
{
    static const WCHAR openW[] = {'o','p','e','n',0};
    ShellExecuteW( NULL, openW, launcher->path, NULL, NULL, 0 );
}

static WCHAR *append_path( const WCHAR *path, const WCHAR *filename, int len_filename )
{
    int len_path = strlenW( path );
    WCHAR *ret;

    if (len_filename == -1) len_filename = strlenW( filename );
    if (!(ret = HeapAlloc( GetProcessHeap(), 0, (len_path + len_filename + 2) * sizeof(WCHAR) )))
        return NULL;
    memcpy( ret, path, len_path * sizeof(WCHAR) );
    ret[len_path] = '\\';
    memcpy( ret + len_path + 1, filename, len_filename * sizeof(WCHAR) );
    ret[len_path + 1 + len_filename] = 0;
    return ret;
}

static IShellLinkW *load_shelllink( const WCHAR *path )
{
    HRESULT hr;
    IShellLinkW *link;
    IPersistFile *file;

    hr = CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW,
                           (void **)&link );
    if (FAILED( hr )) return NULL;

    hr = IShellLinkW_QueryInterface( link, &IID_IPersistFile, (void **)&file );
    if (FAILED( hr ))
    {
        IShellLinkW_Release( link );
        return NULL;
    }
    hr = IPersistFile_Load( file, path, 0 );
    IPersistFile_Release( file );
    if (FAILED( hr ))
    {
        IShellLinkW_Release( link );
        return NULL;
    }
    return link;
}

static HICON extract_icon( IShellLinkW *link )
{
    WCHAR tmp_path[MAX_PATH], icon_path[MAX_PATH], target_path[MAX_PATH];
    HICON icon = NULL;
    int index;

    tmp_path[0] = 0;
    IShellLinkW_GetIconLocation( link, tmp_path, MAX_PATH, &index );
    ExpandEnvironmentStringsW( tmp_path, icon_path, MAX_PATH );

    if (icon_path[0]) ExtractIconExW( icon_path, index, &icon, NULL, 1 );
    if (!icon)
    {
        tmp_path[0] = 0;
        IShellLinkW_GetPath( link, tmp_path, MAX_PATH, NULL, SLGP_RAWPATH );
        ExpandEnvironmentStringsW( tmp_path, target_path, MAX_PATH );
        ExtractIconExW( target_path, index, &icon, NULL, 1 );
    }
    return icon;
}

static WCHAR *build_title( const WCHAR *filename, int len )
{
    const WCHAR *p;
    WCHAR *ret;

    if (len == -1) len = strlenW( filename );
    for (p = filename + len - 1; p >= filename; p--)
    {
        if (*p == '.')
        {
            len = p - filename;
            break;
        }
    }
    if (!(ret = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) return NULL;
    memcpy( ret, filename, len * sizeof(WCHAR) );
    ret[len] = 0;
    return ret;
}

static BOOL add_launcher( const WCHAR *folder, const WCHAR *filename, int len_filename )
{
    struct launcher *launcher;
    IShellLinkW *link;

    if (nb_launchers == nb_allocated)
    {
        unsigned int count = nb_allocated * 2;
        struct launcher **tmp = HeapReAlloc( GetProcessHeap(), 0, launchers, count * sizeof(*tmp) );
        if (!tmp) return FALSE;
        launchers = tmp;
        nb_allocated = count;
    }

    if (!(launcher = HeapAlloc( GetProcessHeap(), 0, sizeof(*launcher) ))) return FALSE;
    if (!(launcher->path = append_path( folder, filename, len_filename ))) goto error;
    if (!(link = load_shelllink( launcher->path ))) goto error;

    launcher->icon = extract_icon( link );
    launcher->title = build_title( filename, len_filename );
    IShellLinkW_Release( link );
    if (launcher->icon && launcher->title)
    {
        launchers[nb_launchers++] = launcher;
        return TRUE;
    }
    HeapFree( GetProcessHeap(), 0, launcher->title );
    DestroyIcon( launcher->icon );

error:
    HeapFree( GetProcessHeap(), 0, launcher->path );
    HeapFree( GetProcessHeap(), 0, launcher );
    return FALSE;
}

static void free_launcher( struct launcher *launcher )
{
    DestroyIcon( launcher->icon );
    HeapFree( GetProcessHeap(), 0, launcher->path );
    HeapFree( GetProcessHeap(), 0, launcher->title );
    HeapFree( GetProcessHeap(), 0, launcher );
}

static BOOL remove_launcher( const WCHAR *folder, const WCHAR *filename, int len_filename )
{
    UINT i;
    WCHAR *path;
    BOOL ret = FALSE;

    if (!(path = append_path( folder, filename, len_filename ))) return FALSE;
    for (i = 0; i < nb_launchers; i++)
    {
        if (!strcmpiW( launchers[i]->path, path ))
        {
            free_launcher( launchers[i] );
            if (--nb_launchers)
                memmove( &launchers[i], &launchers[i + 1], sizeof(launchers[i]) * (nb_launchers - i) );
            ret = TRUE;
            break;
        }
    }
    HeapFree( GetProcessHeap(), 0, path );
    return ret;
}

static BOOL get_icon_text_metrics( HWND hwnd, TEXTMETRICW *tm )
{
    BOOL ret;
    HDC hdc;
    LOGFONTW lf;
    HFONT hfont;

    hdc = GetDC( hwnd );
    SystemParametersInfoW( SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0 );
    hfont = SelectObject( hdc, CreateFontIndirectW( &lf ) );
    ret = GetTextMetricsW( hdc, tm );
    SelectObject( hdc, hfont );
    ReleaseDC( hwnd, hdc );
    return ret;
}

static BOOL process_changes( const WCHAR *folder, char *buf )
{
    FILE_NOTIFY_INFORMATION *info = (FILE_NOTIFY_INFORMATION *)buf;
    BOOL ret = FALSE;

    for (;;)
    {
        switch (info->Action)
        {
        case FILE_ACTION_ADDED:
        case FILE_ACTION_RENAMED_NEW_NAME:
            if (add_launcher( folder, info->FileName, info->FileNameLength / sizeof(WCHAR) ))
                ret = TRUE;
            break;

        case FILE_ACTION_REMOVED:
        case FILE_ACTION_RENAMED_OLD_NAME:
            if (remove_launcher( folder, info->FileName, info->FileNameLength / sizeof(WCHAR) ))
                ret = TRUE;
            break;

        default:
            WARN( "unexpected action %u\n", info->Action );
            break;
        }
        if (!info->NextEntryOffset) break;
        info = (FILE_NOTIFY_INFORMATION *)((char *)info + info->NextEntryOffset);
    }
    return ret;
}

static DWORD CALLBACK watch_desktop_folders( LPVOID param )
{
    HWND hwnd = param;
    HRESULT init = CoInitialize( NULL );
    HANDLE dir0, dir1, events[2];
    OVERLAPPED ovl0, ovl1;
    char *buf0 = NULL, *buf1 = NULL;
    DWORD count, size = 4096, error = ERROR_OUTOFMEMORY;
    BOOL ret, redraw;

    dir0 = CreateFileW( desktop_folder, FILE_LIST_DIRECTORY|SYNCHRONIZE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED, NULL );
    if (dir0 == INVALID_HANDLE_VALUE) return GetLastError();
    dir1 = CreateFileW( desktop_folder_public, FILE_LIST_DIRECTORY|SYNCHRONIZE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED, NULL );
    if (dir1 == INVALID_HANDLE_VALUE)
    {
        CloseHandle( dir0 );
        return GetLastError();
    }
    if (!(ovl0.hEvent = events[0] = CreateEventW( NULL, FALSE, FALSE, NULL ))) goto error;
    if (!(ovl1.hEvent = events[1] = CreateEventW( NULL, FALSE, FALSE, NULL ))) goto error;
    if (!(buf0 = HeapAlloc( GetProcessHeap(), 0, size ))) goto error;
    if (!(buf1 = HeapAlloc( GetProcessHeap(), 0, size ))) goto error;

    for (;;)
    {
        ret = ReadDirectoryChangesW( dir0, buf0, size, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME, NULL, &ovl0, NULL );
        if (!ret)
        {
            error = GetLastError();
            goto error;
        }
        ret = ReadDirectoryChangesW( dir1, buf1, size, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME, NULL, &ovl1, NULL );
        if (!ret)
        {
            error = GetLastError();
            goto error;
        }

        redraw = FALSE;
        switch ((error = WaitForMultipleObjects( 2, events, FALSE, INFINITE )))
        {
        case WAIT_OBJECT_0:
            if (!GetOverlappedResult( dir0, &ovl0, &count, FALSE ) || !count) break;
            if (process_changes( desktop_folder, buf0 )) redraw = TRUE;
            break;

        case WAIT_OBJECT_0 + 1:
            if (!GetOverlappedResult( dir1, &ovl1, &count, FALSE ) || !count) break;
            if (process_changes( desktop_folder_public, buf1 )) redraw = TRUE;
            break;

        default:
            goto error;
        }
        if (redraw) InvalidateRect( hwnd, NULL, TRUE );
    }

error:
    CloseHandle( dir0 );
    CloseHandle( dir1 );
    CloseHandle( events[0] );
    CloseHandle( events[1] );
    HeapFree( GetProcessHeap(), 0, buf0 );
    HeapFree( GetProcessHeap(), 0, buf1 );
    if (SUCCEEDED( init )) CoUninitialize();
    return error;
}

static void add_folder( const WCHAR *folder )
{
    static const WCHAR lnkW[] = {'\\','*','.','l','n','k',0};
    int len = strlenW( folder ) + strlenW( lnkW );
    WIN32_FIND_DATAW data;
    HANDLE handle;
    WCHAR *glob;

    if (!(glob = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) return;
    strcpyW( glob, folder );
    strcatW( glob, lnkW );

    if ((handle = FindFirstFileW( glob, &data )) != INVALID_HANDLE_VALUE)
    {
        do { add_launcher( folder, data.cFileName, -1 ); } while (FindNextFileW( handle, &data ));
        FindClose( handle );
    }
    HeapFree( GetProcessHeap(), 0, glob );
}

#define BORDER_SIZE  4
#define PADDING_SIZE 4
#define TITLE_CHARS  14

static void initialize_launchers( HWND hwnd )
{
    HRESULT hr, init;
    TEXTMETRICW tm;
    int icon_size;

    if (!(get_icon_text_metrics( hwnd, &tm ))) return;

    icon_cx = GetSystemMetrics( SM_CXICON );
    icon_cy = GetSystemMetrics( SM_CYICON );
    icon_size = max( icon_cx, icon_cy );
    title_cy = tm.tmHeight * 2;
    title_cx = max( tm.tmAveCharWidth * TITLE_CHARS, icon_size + PADDING_SIZE + title_cy );
    launcher_size = BORDER_SIZE + title_cx + BORDER_SIZE;
    icon_offset_cx = (launcher_size - icon_cx) / 2;
    icon_offset_cy = BORDER_SIZE + (icon_size - icon_cy) / 2;
    title_offset_cx = BORDER_SIZE;
    title_offset_cy = BORDER_SIZE + icon_size + PADDING_SIZE;
    desktop_width = GetSystemMetrics( SM_CXSCREEN );
    launchers_per_row = desktop_width / launcher_size;

    hr = SHGetKnownFolderPath( &FOLDERID_Desktop, KF_FLAG_CREATE, NULL, &desktop_folder );
    if (FAILED( hr ))
    {
        WINE_ERR("Could not get user desktop folder\n");
        return;
    }
    hr = SHGetKnownFolderPath( &FOLDERID_PublicDesktop, KF_FLAG_CREATE, NULL, &desktop_folder_public );
    if (FAILED( hr ))
    {
        WINE_ERR("Could not get public desktop folder\n");
        CoTaskMemFree( desktop_folder );
        return;
    }
    if ((launchers = HeapAlloc( GetProcessHeap(), 0, 2 * sizeof(launchers[0]) )))
    {
        nb_allocated = 2;

        init = CoInitialize( NULL );
        add_folder( desktop_folder );
        add_folder( desktop_folder_public );
        if (SUCCEEDED( init )) CoUninitialize();

        CreateThread( NULL, 0, watch_desktop_folders, hwnd, 0, NULL );
    }
}

/* screen saver handler */
static BOOL start_screensaver( void )
{
    if (using_root)
    {
        const char *argv[3] = { "xdg-screensaver", "activate", NULL };
        int pid = _spawnvp( _P_DETACH, argv[0], argv );
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

    case WM_SETTINGCHANGE:
        if (wp == SPI_SETDESKWALLPAPER)
            SystemParametersInfoW( SPI_SETDESKWALLPAPER, 0, NULL, FALSE );
        return 0;

    case WM_LBUTTONDBLCLK:
        {
            const struct launcher *launcher = launcher_from_point( (short)LOWORD(lp), (short)HIWORD(lp) );
            if (launcher) do_launch( launcher );
        }
        return 0;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint( hwnd, &ps );
            if (!using_root)
            {
                if (ps.fErase) PaintDesktop( ps.hdc );
                draw_launchers( ps.hdc, ps.rcPaint );
            }
            EndPaint( hwnd, &ps );
        }
        return 0;

    default:
        return DefWindowProcW( hwnd, message, wp, lp );
    }
}

/* create the desktop and the associated driver window, and make it the current desktop */
static BOOL create_desktop( const WCHAR *name, unsigned int width, unsigned int height )
{
    static const WCHAR rootW[] = {'r','o','o','t',0};
    BOOL ret = FALSE;
    BOOL (CDECL *create_desktop_func)(unsigned int, unsigned int);

    /* magic: desktop "root" means use the root window */
    if (graphics_driver && strcmpiW( name, rootW ))
    {
        create_desktop_func = (void *)GetProcAddress( graphics_driver, "wine_create_desktop" );
        if (create_desktop_func) ret = create_desktop_func( width, height );
    }
    return ret;
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
    static const WCHAR messageW[] = {'M','e','s','s','a','g','e',0};
    HDESK desktop = 0;
    MSG msg;
    HDC hdc;
    HWND hwnd, msg_hwnd;
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

    if (name && width && height)
    {
        if (!(desktop = CreateDesktopW( name, NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL )))
        {
            WINE_ERR( "failed to create desktop %s error %d\n", wine_dbgstr_w(name), GetLastError() );
            ExitProcess( 1 );
        }
        SetThreadDesktop( desktop );
    }

    /* create the desktop window */
    hwnd = CreateWindowExW( 0, DESKTOP_CLASS_ATOM, NULL,
                            WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 0, 0, 0, 0, 0, NULL );

    /* create the HWND_MESSAGE parent */
    msg_hwnd = CreateWindowExW( 0, messageW, NULL, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                0, 0, 100, 100, 0, 0, 0, NULL );

    if (hwnd == GetDesktopWindow())
    {
        HMODULE shell32;
        void (WINAPI *pShellDDEInit)( BOOL );

        if (desktop)
        {
            hdc = GetDC( hwnd );
            graphics_driver = __wine_get_driver_module( hdc );
            using_root = !create_desktop( name, width, height );
            ReleaseDC( hwnd, hdc );
        }
        SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)desktop_wnd_proc );
        SendMessageW( hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIconW( 0, MAKEINTRESOURCEW(OIC_WINLOGO)));
        if (name) set_desktop_window_title( hwnd, name );
        SetWindowPos( hwnd, 0, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
                      GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN),
                      SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW );
        SystemParametersInfoW( SPI_SETDESKWALLPAPER, 0, NULL, FALSE );
        ClipCursor( NULL );
        initialize_display_settings( hwnd );
        initialize_appbar();
        initialize_systray( graphics_driver, using_root );
        if (!using_root) initialize_launchers( hwnd );

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
