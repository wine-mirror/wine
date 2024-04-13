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

#include <stdio.h>

#define COBJMACROS
#define OEMRESOURCE
#include <windows.h>
#include <rpc.h>
#include <shlobj.h>
#include <shellapi.h>
#include <ntuser.h>
#include "exdisp.h"

#include "wine/debug.h"
#include "explorer_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(explorer);

#define DESKTOP_CLASS_ATOM ((LPCWSTR)MAKEINTATOM(32769))
#define DESKTOP_ALL_ACCESS 0x01ff

static const WCHAR default_driver[] = {'m','a','c',',','x','1','1',0};

static BOOL using_root = TRUE;

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

static REFIID tid_ids[] =
{
    &IID_IShellWindows,
    &IID_IWebBrowser2
};

typedef enum
{
    IShellWindows_tid,
    IWebBrowser2_tid,
    LAST_tid
} tid_t;

static ITypeLib *typelib;
static ITypeInfo *typeinfos[LAST_tid];

static HRESULT load_typelib(void)
{
    HRESULT hres;
    ITypeLib *tl;

    hres = LoadRegTypeLib(&LIBID_SHDocVw, 1, 0, LOCALE_SYSTEM_DEFAULT, &tl);
    if (FAILED(hres))
    {
        ERR("LoadRegTypeLib failed: %08lx\n", hres);
        return hres;
    }

    if (InterlockedCompareExchangePointer((void**)&typelib, tl, NULL))
        ITypeLib_Release(tl);
    return hres;
}

static HRESULT get_typeinfo(tid_t tid, ITypeInfo **typeinfo)
{
    HRESULT hres;

    if (!typelib) {
        hres = load_typelib();
        if (!typelib)
            return hres;
    }

    if (!typeinfos[tid]) {
        ITypeInfo *ti;

        hres = ITypeLib_GetTypeInfoOfGuid(typelib, tid_ids[tid], &ti);
        if (FAILED(hres)) {
            ERR("GetTypeInfoOfGuid(%s) failed: %08lx\n", debugstr_guid(tid_ids[tid]), hres);
            return hres;
        }

        if (InterlockedCompareExchangePointer((void**)(typeinfos+tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    *typeinfo = typeinfos[tid];
    ITypeInfo_AddRef(*typeinfo);
    return S_OK;
}

static BOOL array_reserve(void **elements, unsigned int *capacity, unsigned int count, unsigned int size)
{
    unsigned int new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;

    return TRUE;
}

static LONG cookie_counter;

struct window
{
    LONG cookie, hwnd;
    int class;
    ITEMIDLIST *pidl;
};

struct shellwindows
{
    IShellWindows IShellWindows_iface;
    CRITICAL_SECTION cs;

    unsigned int count, max;
    struct window *windows;
};

/* This is not limited to desktop itself, every file browser window that
   explorer creates supports that. Desktop instance is special in some
   aspects, for example navigation is not possible, you can't show/hide it,
   or bring up a menu. CLSID_ShellBrowserWindow class could be used to
   create instances like that, and they should be registered with
   IShellWindows as well. */
struct shellbrowserwindow
{
    IWebBrowser2 IWebBrowser2_iface;
    IServiceProvider IServiceProvider_iface;
    IShellBrowser IShellBrowser_iface;
    IShellView *view;
};

static struct shellwindows shellwindows;
static struct shellbrowserwindow desktopshellbrowserwindow;

static inline struct shellwindows *impl_from_IShellWindows(IShellWindows *iface)
{
    return CONTAINING_RECORD(iface, struct shellwindows, IShellWindows_iface);
}

static inline struct shellbrowserwindow *impl_from_IWebBrowser2(IWebBrowser2 *iface)
{
    return CONTAINING_RECORD(iface, struct shellbrowserwindow, IWebBrowser2_iface);
}

static inline struct shellbrowserwindow *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, struct shellbrowserwindow, IServiceProvider_iface);
}

static inline struct shellbrowserwindow *impl_from_IShellBrowser(IShellBrowser *iface)
{
    return CONTAINING_RECORD(iface, struct shellbrowserwindow, IShellBrowser_iface);
}

static void shellwindows_init(void);
static void desktopshellbrowserwindow_init(void);

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
    unsigned int index;

    if (!nb_launchers) return NULL;
    index = x / launcher_size + (y / launcher_size) * launchers_per_row;
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

    font = SelectObject( hdc, font );
    DeleteObject( font );
    SetTextColor( hdc, color );
    SetBkMode( hdc, mode );
}

static WCHAR *append_path( const WCHAR *path, const WCHAR *filename, int len_filename )
{
    int len_path = lstrlenW( path );
    WCHAR *ret;

    if (len_filename == -1) len_filename = lstrlenW( filename );
    if (!(ret = malloc( (len_path + len_filename + 2) * sizeof(WCHAR) ))) return NULL;
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

    if (len == -1) len = lstrlenW( filename );
    for (p = filename + len - 1; p >= filename; p--)
    {
        if (*p == '.')
        {
            len = p - filename;
            break;
        }
    }
    if (!(ret = malloc( (len + 1) * sizeof(WCHAR) ))) return NULL;
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
        struct launcher **tmp = realloc( launchers, count * sizeof(*tmp) );
        if (!tmp) return FALSE;
        launchers = tmp;
        nb_allocated = count;
    }

    if (!(launcher = malloc( sizeof(*launcher) ))) return FALSE;
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
    free( launcher->title );
    DestroyIcon( launcher->icon );

error:
    free( launcher->path );
    free( launcher );
    return FALSE;
}

static void free_launcher( struct launcher *launcher )
{
    DestroyIcon( launcher->icon );
    free( launcher->path );
    free( launcher->title );
    free( launcher );
}

static BOOL remove_launcher( const WCHAR *folder, const WCHAR *filename, int len_filename )
{
    UINT i;
    WCHAR *path;
    BOOL ret = FALSE;

    if (!(path = append_path( folder, filename, len_filename ))) return FALSE;
    for (i = 0; i < nb_launchers; i++)
    {
        if (!wcsicmp( launchers[i]->path, path ))
        {
            free_launcher( launchers[i] );
            if (--nb_launchers)
                memmove( &launchers[i], &launchers[i + 1], sizeof(launchers[i]) * (nb_launchers - i) );
            ret = TRUE;
            break;
        }
    }
    free( path );
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
    hfont = SelectObject( hdc, hfont );
    DeleteObject( hfont );
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
            WARN( "unexpected action %lu\n", info->Action );
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
    if (!(buf0 = malloc( size ))) goto error;
    if (!(buf1 = malloc( size ))) goto error;

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
    free( buf0 );
    free( buf1 );
    if (SUCCEEDED( init )) CoUninitialize();
    return error;
}

static void add_folder( const WCHAR *folder )
{
    static const WCHAR lnkW[] = L"\\*.lnk";
    int len = lstrlenW( folder ) + lstrlenW( lnkW );
    WIN32_FIND_DATAW data;
    HANDLE handle;
    WCHAR *glob;

    if (!(glob = malloc( (len + 1) * sizeof(WCHAR) ))) return;
    lstrcpyW( glob, folder );
    lstrcatW( glob, lnkW );

    if ((handle = FindFirstFileW( glob, &data )) != INVALID_HANDLE_VALUE)
    {
        do { add_launcher( folder, data.cFileName, -1 ); } while (FindNextFileW( handle, &data ));
        FindClose( handle );
    }
    free( glob );
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
    if (!launchers_per_row) launchers_per_row = 1;

    hr = SHGetKnownFolderPath( &FOLDERID_Desktop, KF_FLAG_CREATE, NULL, &desktop_folder );
    if (FAILED( hr ))
    {
        ERR( "Could not get user desktop folder\n" );
        return;
    }
    hr = SHGetKnownFolderPath( &FOLDERID_PublicDesktop, KF_FLAG_CREATE, NULL, &desktop_folder_public );
    if (FAILED( hr ))
    {
        ERR( "Could not get public desktop folder\n" );
        CoTaskMemFree( desktop_folder );
        return;
    }
    if ((launchers = malloc( 2 * sizeof(launchers[0]) )))
    {
        nb_allocated = 2;

        init = CoInitialize( NULL );
        add_folder( desktop_folder );
        add_folder( desktop_folder_public );
        if (SUCCEEDED( init )) CoUninitialize();

        CreateThread( NULL, 0, watch_desktop_folders, hwnd, 0, NULL );
    }
}

/**************************************************************************
 *		wait_clipboard_mutex
 *
 * Make sure that there's only one clipboard thread per window station.
 */
static BOOL wait_clipboard_mutex(void)
{
    static const WCHAR prefix[] = L"__wine_clipboard_";
    WCHAR buffer[MAX_PATH + ARRAY_SIZE( prefix )];
    HANDLE mutex;

    memcpy( buffer, prefix, sizeof(prefix) );
    if (!GetUserObjectInformationW( GetProcessWindowStation(), UOI_NAME,
                                    buffer + ARRAY_SIZE( prefix ) - 1,
                                    sizeof(buffer) - sizeof(prefix), NULL ))
    {
        ERR( "failed to get winstation name\n" );
        return FALSE;
    }
    mutex = CreateMutexW( NULL, TRUE, buffer );
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        TRACE( "waiting for mutex %s\n", debugstr_w( buffer ));
        WaitForSingleObject( mutex, INFINITE );
    }
    return TRUE;
}


/**************************************************************************
 *		clipboard_wndproc
 *
 * Window procedure for the clipboard manager.
 */
static LRESULT CALLBACK clipboard_wndproc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    switch (msg)
    {
    case WM_NCCREATE:
    case WM_CLIPBOARDUPDATE:
    case WM_RENDERFORMAT:
    case WM_TIMER:
    case WM_DESTROYCLIPBOARD:
    case WM_USER:
        return NtUserMessageCall( hwnd, msg, wp, lp, 0, NtUserClipboardWindowProc, FALSE );
    }

    return DefWindowProcW( hwnd, msg, wp, lp );
}


/**************************************************************************
 *		clipboard_thread
 *
 * Thread running inside the desktop process to manage the clipboard
 */
static DWORD WINAPI clipboard_thread( void *arg )
{
    static const WCHAR clipboard_classname[] = L"__wine_clipboard_manager";
    WNDCLASSW class;
    ATOM atom;
    MSG msg;

    if (!wait_clipboard_mutex()) return 0;

    memset( &class, 0, sizeof(class) );
    class.lpfnWndProc   = clipboard_wndproc;
    class.lpszClassName = clipboard_classname;

    if (!(atom = RegisterClassW( &class )) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        ERR( "could not register clipboard window class err %lu\n", GetLastError() );
        return 0;
    }
    if (!CreateWindowW( clipboard_classname, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, NULL ))
    {
        TRACE( "failed to create clipboard window err %lu\n", GetLastError() );
        UnregisterClassW( MAKEINTRESOURCEW(atom), NULL );
        return 0;
    }

    while (GetMessageW( &msg, 0, 0, 0 )) DispatchMessageW( &msg );
    return 0;
}

static WNDPROC desktop_orig_wndproc;

/* window procedure for the desktop window */
static LRESULT WINAPI desktop_wnd_proc( HWND hwnd, UINT message, WPARAM wp, LPARAM lp )
{
    TRACE( "got msg %04x wp %Ix lp %Ix\n", message, wp, lp );

    switch(message)
    {
    case WM_SYSCOMMAND:
        switch(wp & 0xfff0)
        {
        case SC_CLOSE:
            ExitWindows( 0, 0 );
            return 0;
        }
        break;

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

    case WM_PARENTNOTIFY:
        handle_parent_notify( (HWND)lp, wp );
        return 0;

    case WM_LBUTTONDBLCLK:
        if (!using_root)
        {
            const struct launcher *launcher = launcher_from_point( (short)LOWORD(lp), (short)HIWORD(lp) );
            if (launcher) ShellExecuteW( NULL, L"open", launcher->path, NULL, NULL, 0 );
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
    }

    return desktop_orig_wndproc( hwnd, message, wp, lp );
}

/* parse the desktop size specification */
static BOOL parse_size( const WCHAR *size, unsigned int *width, unsigned int *height )
{
    WCHAR *end;

    *width = wcstoul( size, &end, 10 );
    if (end == size) return FALSE;
    if (*end != 'x') return FALSE;
    size = end + 1;
    *height = wcstoul( size, &end, 10 );
    return !*end;
}

/* retrieve the desktop name to use if not specified on the command line */
static const WCHAR *get_default_desktop_name(void)
{
    static WCHAR buffer[MAX_PATH];
    DWORD size = sizeof(buffer);
    HDESK desk = GetThreadDesktop( GetCurrentThreadId() );
    WCHAR *ret = NULL;
    HKEY hkey;

    if (desk && GetUserObjectInformationW( desk, UOI_NAME, buffer, ARRAY_SIZE( buffer ), NULL ))
    {
        if (wcsicmp( buffer, L"Default" )) return buffer;
    }

    /* @@ Wine registry key: HKCU\Software\Wine\Explorer */
    if (!RegOpenKeyW( HKEY_CURRENT_USER, L"Software\\Wine\\Explorer", &hkey ))
    {
        if (!RegQueryValueExW( hkey, L"Desktop", 0, NULL, (LPBYTE)buffer, &size ) && *buffer) ret = buffer;
        RegCloseKey( hkey );
    }
    return ret;
}

/* retrieve the default desktop size from the registry */
static BOOL get_default_desktop_size( const WCHAR *name, unsigned int *width, unsigned int *height )
{
    HKEY hkey;
    WCHAR buffer[64];
    DWORD size = sizeof(buffer);
    BOOL found = FALSE;

    *width = 800;
    *height = 600;

    /* @@ Wine registry key: HKCU\Software\Wine\Explorer\Desktops */
    if (!RegOpenKeyW( HKEY_CURRENT_USER, L"Software\\Wine\\Explorer\\Desktops", &hkey ))
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

static BOOL get_default_enable_shell( const WCHAR *name )
{
    HKEY hkey;
    BOOL found = FALSE;
    BOOL result;
    DWORD size = sizeof(result);

    /* @@ Wine registry key: HKCU\Software\Wine\Explorer\Desktops */
    if (!RegOpenKeyW( HKEY_CURRENT_USER, L"Software\\Wine\\Explorer\\Desktops", &hkey ))
    {
        if (!RegGetValueW( hkey, name, L"EnableShell", RRF_RT_REG_DWORD, NULL, &result, &size ))
            found = TRUE;
        RegCloseKey( hkey );
    }
    /* Default off, except for the magic desktop name "shell" */
    if (!found) result = (lstrcmpiW( name, L"shell" ) == 0);
    return result;
}

static BOOL get_default_show_systray( const WCHAR *name )
{
    HKEY hkey;
    BOOL found = FALSE;
    BOOL result;
    DWORD size = sizeof(result);

    /* @@ Wine registry key: HKCU\Software\Wine\Explorer\Desktops */
    if (name && !RegOpenKeyW( HKEY_CURRENT_USER, L"Software\\Wine\\Explorer\\Desktops", &hkey ))
    {
        if (!RegGetValueW( hkey, name, L"ShowSystray", RRF_RT_REG_DWORD, NULL, &result, &size ))
            found = TRUE;
        RegCloseKey( hkey );
    }

    /* Try again with a global Explorer setting */
    /* @@ Wine registry key: HKCU\Software\Wine\Explorer */
    if (!found && !RegOpenKeyW( HKEY_CURRENT_USER, L"Software\\Wine\\Explorer", &hkey ))
    {
        if (!RegGetValueW( hkey, NULL, L"ShowSystray", RRF_RT_REG_DWORD, NULL, &result, &size ))
            found = TRUE;
        RegCloseKey( hkey );
    }

    /* Default on */
    if (!found) result = TRUE;
    return result;
}

static void load_graphics_driver( const WCHAR *driver, GUID *guid )
{
    static const WCHAR device_keyW[] = L"System\\CurrentControlSet\\Control\\Video\\{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\0000";

    WCHAR buffer[MAX_PATH], libname[32], *name, *next;
    WCHAR key[ARRAY_SIZE( device_keyW ) + 39];
    BOOL null_driver = FALSE;
    HMODULE module = 0;
    HKEY hkey;
    char error[80];

    if (!driver)
    {
        lstrcpyW( buffer, default_driver );

        /* @@ Wine registry key: HKCU\Software\Wine\Drivers */
        if (!RegOpenKeyW( HKEY_CURRENT_USER, L"Software\\Wine\\Drivers", &hkey ))
        {
            DWORD count = sizeof(buffer);
            RegQueryValueExW( hkey, L"Graphics", 0, NULL, (LPBYTE)buffer, &count );
            RegCloseKey( hkey );
        }
    }
    else lstrcpynW( buffer, driver, ARRAY_SIZE( buffer ));

    name = buffer;
    while (name)
    {
        next = wcschr( name, ',' );
        if (next) *next++ = 0;

        if (!wcscmp( name, L"null" ))
        {
            memset( guid, 0, sizeof(*guid) );
            TRACE( "display %s using null driver\n", debugstr_guid(guid) );
            wcscpy( libname, L"null" );
            null_driver = TRUE;
            break;
        }

        swprintf( libname, ARRAY_SIZE( libname ), L"wine%s.drv", name );
        if ((module = LoadLibraryW( libname )) != 0) break;
        switch (GetLastError())
        {
        case ERROR_MOD_NOT_FOUND:
            strcpy( error, "The graphics driver is missing. Check your build!" );
            break;
        case ERROR_DLL_INIT_FAILED:
            strcpy( error, "Make sure that your X server is running and that $DISPLAY is set correctly." );
            break;
        default:
            sprintf( error, "Unknown error (%lu).", GetLastError() );
            break;
        }
        name = next;
    }

    TRACE( "display %s driver %s\n", debugstr_guid(guid), debugstr_w(libname) );

    swprintf( key, ARRAY_SIZE(key), device_keyW, guid->Data1, guid->Data2, guid->Data3,
              guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
              guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );

    if (!RegCreateKeyExW( HKEY_LOCAL_MACHINE, key, 0, NULL,
                          REG_OPTION_VOLATILE, KEY_SET_VALUE, NULL, &hkey, NULL  ))
    {
        if (module || null_driver)
            RegSetValueExW( hkey, L"GraphicsDriver", 0, REG_SZ,
                            (BYTE *)libname, (lstrlenW(libname) + 1) * sizeof(WCHAR) );
        else
            RegSetValueExA( hkey, "DriverError", 0, REG_SZ, (BYTE *)error, strlen(error) + 1 );
        RegCloseKey( hkey );
    }
}

static const char *debugstr_devmodew( const DEVMODEW *devmode )
{
    char position[32] = {0};

    if (devmode->dmFields & DM_POSITION)
    {
        snprintf( position, sizeof(position), " at (%d,%d)",
                 (int)devmode->dmPosition.x, (int)devmode->dmPosition.y );
    }

    return wine_dbg_sprintf( "%ux%u %ubits %uHz rotated %u degrees%s",
                             (unsigned int)devmode->dmPelsWidth,
                             (unsigned int)devmode->dmPelsHeight,
                             (unsigned int)devmode->dmBitsPerPel,
                             (unsigned int)devmode->dmDisplayFrequency,
                             (unsigned int)devmode->dmDisplayOrientation * 90,
                             position );
}

static void initialize_display_settings( unsigned int width, unsigned int height )
{
    DISPLAY_DEVICEW device = {.cb = sizeof(DISPLAY_DEVICEW)};
    DWORD i = 0, flags = CDS_GLOBAL | CDS_UPDATEREGISTRY;

    /* Store current display mode in the registry */
    while (EnumDisplayDevicesW( NULL, i++, &device, 0 ))
    {
        DEVMODEW devmode = {.dmSize = sizeof(DEVMODEW)};

        if (!EnumDisplaySettingsExW( device.DeviceName, ENUM_CURRENT_SETTINGS, &devmode, 0))
        {
            ERR( "Failed to query current display settings for %s.\n", debugstr_w( device.DeviceName ) );
            continue;
        }

        TRACE( "Device %s current display mode %s.\n", debugstr_w( device.DeviceName ), debugstr_devmodew( &devmode ) );

        if (ChangeDisplaySettingsExW( device.DeviceName, &devmode, 0, flags | CDS_NORESET, 0 ))
            ERR( "Failed to initialize registry display settings for %s.\n", debugstr_w( device.DeviceName ) );
    }

    if (!using_root)
    {
        DEVMODEW devmode =
        {
            .dmSize = sizeof(DEVMODEW),
            .dmFields = DM_PELSWIDTH | DM_PELSHEIGHT,
            .dmPelsWidth = width,
            .dmPelsHeight = height,
        };

        /* in virtual desktop mode, set the primary display settings to match desktop size */
        if (ChangeDisplaySettingsExW( NULL, &devmode, 0, flags, NULL ))
            ERR( "Failed to set primary display settings.\n" );
    }
}

static void set_desktop_window_title( HWND hwnd, const WCHAR *name )
{
    static const WCHAR desktop_nameW[] = L"Wine desktop";
    static const WCHAR desktop_name_separatorW[] = L" - ";
    WCHAR *window_titleW = NULL;
    int window_title_len;

    if (!name[0])
    {
        SetWindowTextW( hwnd, desktop_nameW );
        return;
    }

    window_title_len = lstrlenW(name) * sizeof(WCHAR)
                     + sizeof(desktop_name_separatorW)
                     + sizeof(desktop_nameW);
    window_titleW = malloc( window_title_len );
    if (!window_titleW)
    {
        SetWindowTextW( hwnd, desktop_nameW );
        return;
    }

    lstrcpyW( window_titleW, name );
    lstrcatW( window_titleW, desktop_name_separatorW );
    lstrcatW( window_titleW, desktop_nameW );

    SetWindowTextW( hwnd, window_titleW );
    free( window_titleW );
}

static inline BOOL is_whitespace(WCHAR c)
{
    return c == ' ' || c == '\t';
}

/* main desktop management function */
void manage_desktop( WCHAR *arg )
{
    HDESK desktop = 0;
    GUID guid;
    MSG msg;
    HWND hwnd;
    unsigned int width, height;
    WCHAR *cmdline = NULL, *driver = NULL;
    WCHAR *p = arg;
    const WCHAR *name = NULL;
    BOOL enable_shell = FALSE, show_systray = TRUE;
    void (WINAPI *pShellDDEInit)( BOOL ) = NULL;
    HMODULE shell32;
    HANDLE thread;
    DWORD id;

    /* get the rest of the command line (if any) */
    while (*p && !is_whitespace(*p)) p++;
    if (*p)
    {
        *p++ = 0;
        while (*p && is_whitespace(*p)) p++;
        if (*p) cmdline = p;
    }

    /* parse the desktop option */
    /* the option is of the form /desktop=name[,widthxheight[,driver]] */
    if ((arg[0] == '=' || arg[0] == ',') && arg[1] && arg[1] != ',')
    {
        arg++;
        name = arg;
        if ((p = wcschr( arg, ',' )))
        {
            *p++ = 0;
            if ((driver = wcschr( p, ',' ))) *driver++ = 0;
        }
        if (!p || !parse_size( p, &width, &height ))
            get_default_desktop_size( name, &width, &height );
    }
    else if ((name = get_default_desktop_name()))
    {
        if (!get_default_desktop_size( name, &width, &height )) width = height = 0;
    }

    if (name) enable_shell = get_default_enable_shell( name );
    show_systray = get_default_show_systray( name );

    UuidCreate( &guid );
    TRACE( "display guid %s\n", debugstr_guid(&guid) );
    load_graphics_driver( driver, &guid );

    if (name && width && height)
    {
        DEVMODEW devmode = {.dmPelsWidth = width, .dmPelsHeight = height};
        /* magic: desktop "root" means use the root window */
        if ((using_root = !wcsicmp( name, L"root" ))) desktop = CreateDesktopW( name, NULL, NULL, DF_WINE_ROOT_DESKTOP, DESKTOP_ALL_ACCESS, NULL );
        else desktop = CreateDesktopW( name, NULL, &devmode, DF_WINE_VIRTUAL_DESKTOP, DESKTOP_ALL_ACCESS, NULL );
        if (!desktop)
        {
            ERR( "failed to create desktop %s error %ld\n", debugstr_w(name), GetLastError() );
            ExitProcess( 1 );
        }
        SetThreadDesktop( desktop );
    }

    /* create the desktop window */
    hwnd = CreateWindowExW( 0, DESKTOP_CLASS_ATOM, NULL,
                            WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 0, 0, 0, 0, 0, &guid );

    if (hwnd)
    {
        /* create the HWND_MESSAGE parent */
        CreateWindowExW( 0, L"Message", NULL, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                         0, 0, 100, 100, 0, 0, 0, NULL );

        desktop_orig_wndproc = (WNDPROC)SetWindowLongPtrW( hwnd, GWLP_WNDPROC,
            (LONG_PTR)desktop_wnd_proc );
        SendMessageW( hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIconW( 0, MAKEINTRESOURCEW(OIC_WINLOGO)));
        if (name) set_desktop_window_title( hwnd, name );
        SetWindowPos( hwnd, 0, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
                      GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN),
                      SWP_SHOWWINDOW );
        thread = CreateThread( NULL, 0, clipboard_thread, NULL, 0, &id );
        if (thread) CloseHandle( thread );
        SystemParametersInfoW( SPI_SETDESKWALLPAPER, 0, NULL, FALSE );
        ClipCursor( NULL );
        initialize_display_settings( width, height );
        initialize_appbar();

        if (using_root) enable_shell = FALSE;

        initialize_systray( using_root, enable_shell, show_systray );
        if (!using_root) initialize_launchers( hwnd );

        if ((shell32 = LoadLibraryW( L"shell32.dll" )) &&
            (pShellDDEInit = (void *)GetProcAddress( shell32, (LPCSTR)188)))
        {
            pShellDDEInit( TRUE );
        }
    }

    /* if we have a command line, execute it */
    if (cmdline)
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;

        memset( &si, 0, sizeof(si) );
        si.cb = sizeof(si);
        TRACE( "starting %s\n", debugstr_w(cmdline) );
        if (CreateProcessW( NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ))
        {
            CloseHandle( pi.hThread );
            CloseHandle( pi.hProcess );
        }
    }

    desktopshellbrowserwindow_init();
    shellwindows_init();

    /* run the desktop message loop */
    if (hwnd)
    {
        TRACE( "desktop message loop starting on hwnd %p\n", hwnd );
        while (GetMessageW( &msg, 0, 0, 0 )) DispatchMessageW( &msg );
        TRACE( "desktop message loop exiting for hwnd %p\n", hwnd );
    }

    if (pShellDDEInit) pShellDDEInit( FALSE );

    ExitProcess( 0 );
}

/* IShellWindows implementation */
static HRESULT WINAPI shellwindows_QueryInterface(IShellWindows *iface, REFIID riid, void **ppvObject)
{
    struct shellwindows *This = impl_from_IShellWindows(iface);

    TRACE("%s %p\n", debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IShellWindows) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *ppvObject = &This->IShellWindows_iface;
    }
    else
    {
        WARN("Unsupported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
    }

    if (*ppvObject)
    {
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI shellwindows_AddRef(IShellWindows *iface)
{
    return 2;
}

static ULONG WINAPI shellwindows_Release(IShellWindows *iface)
{
    return 1;
}

static HRESULT WINAPI shellwindows_GetTypeInfoCount(IShellWindows *iface, UINT *pctinfo)
{
    TRACE("%p\n", pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI shellwindows_GetTypeInfo(IShellWindows *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    TRACE("%d %ld %p\n", iTInfo, lcid, ppTInfo);
    return get_typeinfo(IShellWindows_tid, ppTInfo);
}

static HRESULT WINAPI shellwindows_GetIDsOfNames(IShellWindows *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid,
        DISPID *rgDispId)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%s %p %d %ld %p\n", debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if (!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IShellWindows_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI shellwindows_Invoke(IShellWindows *iface,
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
        DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%ld %s %ld %08x %p %p %p %p\n", dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IShellWindows_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI shellwindows_get_Count(IShellWindows *iface, LONG *count)
{
    FIXME("%p\n", count);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellwindows_Item(IShellWindows *iface, VARIANT index,
    IDispatch **folder)
{
    FIXME("%s %p\n", debugstr_variant(&index), folder);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellwindows__NewEnum(IShellWindows *iface, IUnknown **ppunk)
{
    FIXME("%p\n", ppunk);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellwindows_Register(IShellWindows *iface,
        IDispatch *disp, LONG hwnd, int class, LONG *cookie)
{
    struct shellwindows *sw = impl_from_IShellWindows(iface);
    struct window *window;

    TRACE("iface %p, disp %p, hwnd %#lx, class %u, cookie %p.\n", iface, disp, hwnd, class, cookie);

    if (!hwnd)
        return E_POINTER;

    if (disp)
        FIXME("Ignoring IDispatch %p.\n", disp);

    EnterCriticalSection(&sw->cs);

    if (!array_reserve((void **)&sw->windows, &sw->max, sw->count + 1, sizeof(*sw->windows)))
    {
        LeaveCriticalSection(&sw->cs);
        return E_OUTOFMEMORY;
    }

    window = &sw->windows[sw->count++];
    window->hwnd = hwnd;
    window->class = class;
    *cookie = window->cookie = ++cookie_counter;
    window->pidl = NULL;

    LeaveCriticalSection(&sw->cs);
    return S_OK;
}

static HRESULT WINAPI shellwindows_RegisterPending(IShellWindows *iface,
    LONG threadid, VARIANT *loc, VARIANT *root, int class, LONG *cookie)
{
    FIXME("0x%lx %s %s 0x%x %p\n", threadid, debugstr_variant(loc), debugstr_variant(root),
            class, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellwindows_Revoke(IShellWindows *iface, LONG cookie)
{
    struct shellwindows *sw = impl_from_IShellWindows(iface);
    unsigned int i;

    TRACE("iface %p, cookie %lu.\n", iface, cookie);

    EnterCriticalSection(&sw->cs);

    for (i = 0; i < sw->count; ++i)
    {
        if (sw->windows[i].cookie == cookie)
        {
            --sw->count;
            memmove(&sw->windows[i], &sw->windows[i + 1], (sw->count - i) * sizeof(*sw->windows));
            LeaveCriticalSection(&sw->cs);
            return S_OK;
        }
    }

    LeaveCriticalSection(&sw->cs);
    return S_FALSE;
}

static HRESULT WINAPI shellwindows_OnNavigate(IShellWindows *iface, LONG cookie, VARIANT *location)
{
    struct shellwindows *sw = impl_from_IShellWindows(iface);
    unsigned int i;

    TRACE("iface %p, cookie %lu, location %s.\n", iface, cookie, debugstr_variant(location));

    if (V_VT(location) != (VT_ARRAY | VT_UI1))
    {
        FIXME("Unexpected variant type %s.\n", debugstr_vt(V_VT(location)));
        return E_NOTIMPL;
    }

    EnterCriticalSection(&sw->cs);

    for (i = 0; i < sw->count; ++i)
    {
        if (sw->windows[i].cookie == cookie)
        {
            size_t len = V_ARRAY(location)->rgsabound[0].cElements;
            if (!(sw->windows[i].pidl = realloc(sw->windows[i].pidl, len)))
            {
                LeaveCriticalSection(&sw->cs);
                return E_OUTOFMEMORY;
            }
            memcpy(sw->windows[i].pidl, V_ARRAY(location)->pvData, len);

            LeaveCriticalSection(&sw->cs);
            return S_OK;
        }
    }

    LeaveCriticalSection(&sw->cs);
    return E_INVALIDARG;
}

static HRESULT WINAPI shellwindows_OnActivated(IShellWindows *iface, LONG cookie, VARIANT_BOOL active)
{
    FIXME("0x%lx 0x%x\n", cookie, active);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellwindows_FindWindowSW(IShellWindows *iface, VARIANT *location,
        VARIANT *root, int class, LONG *hwnd, int options, IDispatch **disp)
{
    struct shellwindows *sw = impl_from_IShellWindows(iface);
    unsigned int i;

    TRACE("iface %p, location %p, root %p, class %#x, hwnd %p, options %#x, disp %p.\n",
            iface, location, root, class, hwnd, options, disp);

    if (class == SWC_DESKTOP)
    {
        *hwnd = (LONG)(LONG_PTR)GetDesktopWindow();
        if (options & SWFO_NEEDDISPATCH)
        {
            *disp = (IDispatch *)&desktopshellbrowserwindow.IWebBrowser2_iface;
            IDispatch_AddRef(*disp);
        }
        return S_OK;
    }

    if (options)
        FIXME("Ignoring options %#x.\n", options);

    if (V_VT(location) != (VT_ARRAY | VT_UI1))
    {
        FIXME("Unexpected variant type %s.\n", debugstr_vt(V_VT(location)));
        return E_NOTIMPL;
    }

    EnterCriticalSection(&sw->cs);

    for (i = 0; i < sw->count; ++i)
    {
        if (sw->windows[i].class == class && ILIsEqual(V_ARRAY(location)->pvData, sw->windows[i].pidl))
        {
            *hwnd = sw->windows[i].hwnd;
            LeaveCriticalSection(&sw->cs);
            return S_OK;
        }
    }

    LeaveCriticalSection(&sw->cs);
    return S_FALSE;
}

static HRESULT WINAPI shellwindows_OnCreated(IShellWindows *iface, LONG cookie, IUnknown *punk)
{
    FIXME("0x%lx %p\n", cookie, punk);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellwindows_ProcessAttachDetach(IShellWindows *iface, VARIANT_BOOL attach)
{
    FIXME("0x%x\n", attach);
    return E_NOTIMPL;
}

static const IShellWindowsVtbl shellwindowsvtbl =
{
    shellwindows_QueryInterface,
    shellwindows_AddRef,
    shellwindows_Release,
    shellwindows_GetTypeInfoCount,
    shellwindows_GetTypeInfo,
    shellwindows_GetIDsOfNames,
    shellwindows_Invoke,
    shellwindows_get_Count,
    shellwindows_Item,
    shellwindows__NewEnum,
    shellwindows_Register,
    shellwindows_RegisterPending,
    shellwindows_Revoke,
    shellwindows_OnNavigate,
    shellwindows_OnActivated,
    shellwindows_FindWindowSW,
    shellwindows_OnCreated,
    shellwindows_ProcessAttachDetach
};

struct shellwindows_classfactory
{
    IClassFactory IClassFactory_iface;
    DWORD classreg;
};

static inline struct shellwindows_classfactory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct shellwindows_classfactory, IClassFactory_iface);
}

static HRESULT WINAPI swclassfactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppvObject)
{
    struct shellwindows_classfactory *This = impl_from_IClassFactory(iface);

    TRACE("%s %p\n", debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObject = &This->IClassFactory_iface;
    }
    else
    {
        WARN("Unsupported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
    }

    if (*ppvObject)
    {
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI swclassfactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI swclassfactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI swclassfactory_CreateInstance(IClassFactory *iface,
    IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
    TRACE("%p %s %p\n", pUnkOuter, debugstr_guid(riid), ppvObject);
    return IShellWindows_QueryInterface(&shellwindows.IShellWindows_iface, riid, ppvObject);
}

static HRESULT WINAPI swclassfactory_LockServer(IClassFactory *iface, BOOL lock)
{
    TRACE("%u\n", lock);
    return E_NOTIMPL;
}

static const IClassFactoryVtbl swclassfactoryvtbl =
{
    swclassfactory_QueryInterface,
    swclassfactory_AddRef,
    swclassfactory_Release,
    swclassfactory_CreateInstance,
    swclassfactory_LockServer
};

static struct shellwindows_classfactory shellwindows_classfactory = { { &swclassfactoryvtbl } };

static HRESULT WINAPI webbrowser_QueryInterface(IWebBrowser2 *iface, REFIID riid, void **ppv)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);

    *ppv = NULL;

    if (IsEqualGUID(&IID_IWebBrowser2, riid) ||
        IsEqualGUID(&IID_IWebBrowserApp, riid) ||
        IsEqualGUID(&IID_IWebBrowser, riid) ||
        IsEqualGUID(&IID_IDispatch, riid) ||
        IsEqualGUID(&IID_IUnknown, riid))
    {
        *ppv = &This->IWebBrowser2_iface;
    }
    else if (IsEqualGUID(&IID_IServiceProvider, riid))
    {
        *ppv = &This->IServiceProvider_iface;
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p) interface not supported\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI webbrowser_AddRef(IWebBrowser2 *iface)
{
    return 2;
}

static ULONG WINAPI webbrowser_Release(IWebBrowser2 *iface)
{
    return 1;
}

/* IDispatch methods */
static HRESULT WINAPI webbrowser_GetTypeInfoCount(IWebBrowser2 *iface, UINT *pctinfo)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI webbrowser_GetTypeInfo(IWebBrowser2 *iface, UINT iTInfo, LCID lcid,
                                     LPTYPEINFO *ppTInfo)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    TRACE("(%p)->(%d %ld %p)\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IWebBrowser2_tid, ppTInfo);
}

static HRESULT WINAPI webbrowser_GetIDsOfNames(IWebBrowser2 *iface, REFIID riid,
                                       LPOLESTR *rgszNames, UINT cNames,
                                       LCID lcid, DISPID *rgDispId)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %d %ld %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IWebBrowser2_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI webbrowser_Invoke(IWebBrowser2 *iface, DISPID dispIdMember,
                                REFIID riid, LCID lcid, WORD wFlags,
                                DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%ld %s %ld %08x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IWebBrowser2_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &This->IWebBrowser2_iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

/* IWebBrowser methods */
static HRESULT WINAPI webbrowser_GoBack(IWebBrowser2 *iface)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_GoForward(IWebBrowser2 *iface)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_GoHome(IWebBrowser2 *iface)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_GoSearch(IWebBrowser2 *iface)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_Navigate(IWebBrowser2 *iface, BSTR szUrl,
                                  VARIANT *Flags, VARIANT *TargetFrameName,
                                  VARIANT *PostData, VARIANT *Headers)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%s %s %s %s %s): stub\n", This, debugstr_w(szUrl), debugstr_variant(Flags),
          debugstr_variant(TargetFrameName), debugstr_variant(PostData),
          debugstr_variant(Headers));
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_Refresh(IWebBrowser2 *iface)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_Refresh2(IWebBrowser2 *iface, VARIANT *Level)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_variant(Level));
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_Stop(IWebBrowser2 *iface)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_Application(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, ppDisp);

    *ppDisp = (IDispatch*)iface;
    IDispatch_AddRef(*ppDisp);

    return S_OK;
}

static HRESULT WINAPI webbrowser_get_Parent(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_Container(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_Document(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_TopLevelContainer(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_Type(IWebBrowser2 *iface, BSTR *Type)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Type);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_Left(IWebBrowser2 *iface, LONG *pl)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_Left(IWebBrowser2 *iface, LONG Left)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%ld)\n", This, Left);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_Top(IWebBrowser2 *iface, LONG *pl)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_Top(IWebBrowser2 *iface, LONG Top)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%ld)\n", This, Top);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_Width(IWebBrowser2 *iface, LONG *pl)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_Width(IWebBrowser2 *iface, LONG Width)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%ld)\n", This, Width);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_Height(IWebBrowser2 *iface, LONG *pl)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_Height(IWebBrowser2 *iface, LONG Height)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%ld)\n", This, Height);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_LocationName(IWebBrowser2 *iface, BSTR *LocationName)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, LocationName);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_LocationURL(IWebBrowser2 *iface, BSTR *LocationURL)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, LocationURL);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_Busy(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_Quit(IWebBrowser2 *iface)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_ClientToWindow(IWebBrowser2 *iface, int *pcx, int *pcy)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p %p)\n", This, pcx, pcy);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_PutProperty(IWebBrowser2 *iface, BSTR szProperty, VARIANT vtValue)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(szProperty), debugstr_variant(&vtValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_GetProperty(IWebBrowser2 *iface, BSTR szProperty, VARIANT *pvtValue)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(szProperty), debugstr_variant(pvtValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_Name(IWebBrowser2 *iface, BSTR *Name)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Name);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_HWND(IWebBrowser2 *iface, SHANDLE_PTR *pHWND)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pHWND);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_FullName(IWebBrowser2 *iface, BSTR *FullName)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, FullName);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_Path(IWebBrowser2 *iface, BSTR *Path)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Path);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_Visible(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_Visible(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_StatusBar(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_StatusBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_StatusText(IWebBrowser2 *iface, BSTR *StatusText)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, StatusText);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_StatusText(IWebBrowser2 *iface, BSTR StatusText)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(StatusText));
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_ToolBar(IWebBrowser2 *iface, int *Value)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_ToolBar(IWebBrowser2 *iface, int Value)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_MenuBar(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_MenuBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_FullScreen(IWebBrowser2 *iface, VARIANT_BOOL *pbFullScreen)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pbFullScreen);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_FullScreen(IWebBrowser2 *iface, VARIANT_BOOL bFullScreen)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, bFullScreen);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_Navigate2(IWebBrowser2 *iface, VARIANT *URL, VARIANT *Flags,
        VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%s %s %s %s %s)\n", This, debugstr_variant(URL), debugstr_variant(Flags),
          debugstr_variant(TargetFrameName), debugstr_variant(PostData), debugstr_variant(Headers));
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_QueryStatusWB(IWebBrowser2 *iface, OLECMDID cmdID, OLECMDF *pcmdf)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%d %p)\n", This, cmdID, pcmdf);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_ExecWB(IWebBrowser2 *iface, OLECMDID cmdID,
        OLECMDEXECOPT cmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%d %d %s %p)\n", This, cmdID, cmdexecopt, debugstr_variant(pvaIn), pvaOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_ShowBrowserBar(IWebBrowser2 *iface, VARIANT *pvaClsid,
        VARIANT *pvarShow, VARIANT *pvarSize)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%s %s %s)\n", This, debugstr_variant(pvaClsid), debugstr_variant(pvarShow),
          debugstr_variant(pvarSize));
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_ReadyState(IWebBrowser2 *iface, READYSTATE *lpReadyState)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, lpReadyState);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_Offline(IWebBrowser2 *iface, VARIANT_BOOL *pbOffline)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pbOffline);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_Offline(IWebBrowser2 *iface, VARIANT_BOOL bOffline)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, bOffline);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_Silent(IWebBrowser2 *iface, VARIANT_BOOL *pbSilent)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pbSilent);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_Silent(IWebBrowser2 *iface, VARIANT_BOOL bSilent)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, bSilent);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_RegisterAsBrowser(IWebBrowser2 *iface,
        VARIANT_BOOL *pbRegister)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pbRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_RegisterAsBrowser(IWebBrowser2 *iface,
        VARIANT_BOOL bRegister)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, bRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_RegisterAsDropTarget(IWebBrowser2 *iface,
        VARIANT_BOOL *pbRegister)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pbRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_RegisterAsDropTarget(IWebBrowser2 *iface,
        VARIANT_BOOL bRegister)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, bRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_TheaterMode(IWebBrowser2 *iface, VARIANT_BOOL *pbRegister)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pbRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_TheaterMode(IWebBrowser2 *iface, VARIANT_BOOL bRegister)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    TRACE("(%p)->(%x)\n", This, bRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_AddressBar(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_AddressBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_get_Resizable(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI webbrowser_put_Resizable(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    struct shellbrowserwindow *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

static const IWebBrowser2Vtbl webbrowser2vtbl =
{
    webbrowser_QueryInterface,
    webbrowser_AddRef,
    webbrowser_Release,
    webbrowser_GetTypeInfoCount,
    webbrowser_GetTypeInfo,
    webbrowser_GetIDsOfNames,
    webbrowser_Invoke,
    webbrowser_GoBack,
    webbrowser_GoForward,
    webbrowser_GoHome,
    webbrowser_GoSearch,
    webbrowser_Navigate,
    webbrowser_Refresh,
    webbrowser_Refresh2,
    webbrowser_Stop,
    webbrowser_get_Application,
    webbrowser_get_Parent,
    webbrowser_get_Container,
    webbrowser_get_Document,
    webbrowser_get_TopLevelContainer,
    webbrowser_get_Type,
    webbrowser_get_Left,
    webbrowser_put_Left,
    webbrowser_get_Top,
    webbrowser_put_Top,
    webbrowser_get_Width,
    webbrowser_put_Width,
    webbrowser_get_Height,
    webbrowser_put_Height,
    webbrowser_get_LocationName,
    webbrowser_get_LocationURL,
    webbrowser_get_Busy,
    webbrowser_Quit,
    webbrowser_ClientToWindow,
    webbrowser_PutProperty,
    webbrowser_GetProperty,
    webbrowser_get_Name,
    webbrowser_get_HWND,
    webbrowser_get_FullName,
    webbrowser_get_Path,
    webbrowser_get_Visible,
    webbrowser_put_Visible,
    webbrowser_get_StatusBar,
    webbrowser_put_StatusBar,
    webbrowser_get_StatusText,
    webbrowser_put_StatusText,
    webbrowser_get_ToolBar,
    webbrowser_put_ToolBar,
    webbrowser_get_MenuBar,
    webbrowser_put_MenuBar,
    webbrowser_get_FullScreen,
    webbrowser_put_FullScreen,
    webbrowser_Navigate2,
    webbrowser_QueryStatusWB,
    webbrowser_ExecWB,
    webbrowser_ShowBrowserBar,
    webbrowser_get_ReadyState,
    webbrowser_get_Offline,
    webbrowser_put_Offline,
    webbrowser_get_Silent,
    webbrowser_put_Silent,
    webbrowser_get_RegisterAsBrowser,
    webbrowser_put_RegisterAsBrowser,
    webbrowser_get_RegisterAsDropTarget,
    webbrowser_put_RegisterAsDropTarget,
    webbrowser_get_TheaterMode,
    webbrowser_put_TheaterMode,
    webbrowser_get_AddressBar,
    webbrowser_put_AddressBar,
    webbrowser_get_Resizable,
    webbrowser_put_Resizable
};

static HRESULT WINAPI serviceprovider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    struct shellbrowserwindow *This = impl_from_IServiceProvider(iface);
    return IWebBrowser2_QueryInterface(&This->IWebBrowser2_iface, riid, ppv);
}

static ULONG WINAPI serviceprovider_AddRef(IServiceProvider *iface)
{
    struct shellbrowserwindow *This = impl_from_IServiceProvider(iface);
    return IWebBrowser2_AddRef(&This->IWebBrowser2_iface);
}

static ULONG WINAPI serviceprovider_Release(IServiceProvider *iface)
{
    struct shellbrowserwindow *This = impl_from_IServiceProvider(iface);
    return IWebBrowser2_Release(&This->IWebBrowser2_iface);
}

static HRESULT WINAPI serviceprovider_QueryService(IServiceProvider *iface, REFGUID service,
    REFIID riid, void **ppv)
{
    struct shellbrowserwindow *This = impl_from_IServiceProvider(iface);

    TRACE("%s %s %p\n", debugstr_guid(service), debugstr_guid(riid), ppv);

    if (IsEqualGUID(service, &SID_STopLevelBrowser))
        return IShellBrowser_QueryInterface(&This->IShellBrowser_iface, riid, ppv);

    WARN("unknown service id %s\n", debugstr_guid(service));
    return E_NOTIMPL;
}

static const IServiceProviderVtbl serviceprovidervtbl =
{
    serviceprovider_QueryInterface,
    serviceprovider_AddRef,
    serviceprovider_Release,
    serviceprovider_QueryService
};

/* IShellBrowser */
static HRESULT WINAPI shellbrowser_QueryInterface(IShellBrowser *iface, REFIID riid, void **ppv)
{
    TRACE("%s %p\n", debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualGUID(&IID_IShellBrowser, riid) ||
        IsEqualGUID(&IID_IOleWindow, riid) ||
        IsEqualGUID(&IID_IUnknown, riid))
    {
        *ppv = iface;
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI shellbrowser_AddRef(IShellBrowser *iface)
{
    struct shellbrowserwindow *This = impl_from_IShellBrowser(iface);
    return IWebBrowser2_AddRef(&This->IWebBrowser2_iface);
}

static ULONG WINAPI shellbrowser_Release(IShellBrowser *iface)
{
    struct shellbrowserwindow *This = impl_from_IShellBrowser(iface);
    return IWebBrowser2_Release(&This->IWebBrowser2_iface);
}

static HRESULT WINAPI shellbrowser_GetWindow(IShellBrowser *iface, HWND *phwnd)
{
    FIXME("%p\n", phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_ContextSensitiveHelp(IShellBrowser *iface, BOOL mode)
{
    FIXME("%d\n", mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_InsertMenusSB(IShellBrowser *iface, HMENU hmenuShared,
    OLEMENUGROUPWIDTHS *menuwidths)
{
    FIXME("%p %p\n", hmenuShared, menuwidths);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_SetMenuSB(IShellBrowser *iface, HMENU hmenuShared,
    HOLEMENU holemenuReserved, HWND hwndActiveObject)
{
    FIXME("%p %p %p\n", hmenuShared, holemenuReserved, hwndActiveObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_RemoveMenusSB(IShellBrowser *iface, HMENU hmenuShared)
{
    FIXME("%p\n", hmenuShared);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_SetStatusTextSB(IShellBrowser *iface, LPCOLESTR text)
{
    FIXME("%s\n", debugstr_w(text));
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_EnableModelessSB(IShellBrowser *iface, BOOL enable)
{
    FIXME("%d\n", enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_TranslateAcceleratorSB(IShellBrowser *iface, MSG *pmsg, WORD wID)
{
    FIXME("%p 0x%x\n", pmsg, wID);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_BrowseObject(IShellBrowser *iface, LPCITEMIDLIST pidl, UINT flags)
{
    FIXME("%p %x\n", pidl, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_GetViewStateStream(IShellBrowser *iface, DWORD mode, IStream **stream)
{
    FIXME("0x%lx %p\n", mode, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_GetControlWindow(IShellBrowser *iface, UINT id, HWND *phwnd)
{
    FIXME("%d %p\n", id, phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_SendControlMsg(IShellBrowser *iface, UINT id, UINT uMsg,
    WPARAM wParam, LPARAM lParam, LRESULT *pret)
{
    FIXME("%d %d %Ix %Ix %p\n", id, uMsg, wParam, lParam, pret);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_QueryActiveShellView(IShellBrowser *iface, IShellView **view)
{
    TRACE("%p\n", view);

    *view = desktopshellbrowserwindow.view;
    IShellView_AddRef(*view);
    return S_OK;
}

static HRESULT WINAPI shellbrowser_OnViewWindowActive(IShellBrowser *iface, IShellView *view)
{
    FIXME("%p\n", view);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_SetToolbarItems(IShellBrowser *iface, LPTBBUTTONSB buttons,
    UINT count, UINT flags)
{
    FIXME("%p %d 0x%x\n", buttons, count, flags);
    return E_NOTIMPL;
}

static const IShellBrowserVtbl shellbrowservtbl = {
    shellbrowser_QueryInterface,
    shellbrowser_AddRef,
    shellbrowser_Release,
    shellbrowser_GetWindow,
    shellbrowser_ContextSensitiveHelp,
    shellbrowser_InsertMenusSB,
    shellbrowser_SetMenuSB,
    shellbrowser_RemoveMenusSB,
    shellbrowser_SetStatusTextSB,
    shellbrowser_EnableModelessSB,
    shellbrowser_TranslateAcceleratorSB,
    shellbrowser_BrowseObject,
    shellbrowser_GetViewStateStream,
    shellbrowser_GetControlWindow,
    shellbrowser_SendControlMsg,
    shellbrowser_QueryActiveShellView,
    shellbrowser_OnViewWindowActive,
    shellbrowser_SetToolbarItems
};

static void desktopshellbrowserwindow_init(void)
{
    IShellFolder *folder;

    desktopshellbrowserwindow.IWebBrowser2_iface.lpVtbl = &webbrowser2vtbl;
    desktopshellbrowserwindow.IServiceProvider_iface.lpVtbl = &serviceprovidervtbl;
    desktopshellbrowserwindow.IShellBrowser_iface.lpVtbl = &shellbrowservtbl;

    if (FAILED(SHGetDesktopFolder(&folder)))
        return;

    IShellFolder_CreateViewObject(folder, NULL, &IID_IShellView, (void**)&desktopshellbrowserwindow.view);
}

static void shellwindows_init(void)
{
    HRESULT hr;

    CoInitialize(NULL);

    shellwindows.IShellWindows_iface.lpVtbl = &shellwindowsvtbl;
    InitializeCriticalSection(&shellwindows.cs);
    shellwindows.cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": shellwindows.cs");

    hr = CoRegisterClassObject(&CLSID_ShellWindows,
        (IUnknown*)&shellwindows_classfactory.IClassFactory_iface,
        CLSCTX_LOCAL_SERVER,
        REGCLS_MULTIPLEUSE,
        &shellwindows_classfactory.classreg);

    if (FAILED(hr))
        WARN("Failed to register ShellWindows object: %08lx\n", hr);
}
