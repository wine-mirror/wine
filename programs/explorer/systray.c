/*
 * Copyright (C) 2004 Mike Hearn, for CodeWeavers
 * Copyright (C) 2005 Robert Shearman
 * Copyright (C) 2008 Alexandre Julliard
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

#include <assert.h>

#define NONAMELESSUNION
#define _WIN32_IE 0x500
#include <windows.h>
#include <commctrl.h>

#include <wine/debug.h>
#include <wine/list.h>

#include "explorer_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(systray);

struct notify_data  /* platform-independent format for NOTIFYICONDATA */
{
    LONG  hWnd;
    UINT  uID;
    UINT  uFlags;
    UINT  uCallbackMessage;
    WCHAR szTip[128];
    DWORD dwState;
    DWORD dwStateMask;
    WCHAR szInfo[256];
    union {
        UINT uTimeout;
        UINT uVersion;
    } u;
    WCHAR szInfoTitle[64];
    DWORD dwInfoFlags;
    GUID  guidItem;
    /* data for the icon bitmap */
    UINT width;
    UINT height;
    UINT planes;
    UINT bpp;
};

static int (CDECL *wine_notify_icon)(DWORD,NOTIFYICONDATAW *);

/* an individual systray icon, unpacked from the NOTIFYICONDATA and always in unicode */
struct icon
{
    struct list    entry;
    HICON          image;    /* the image to render */
    HWND           owner;    /* the HWND passed in to the Shell_NotifyIcon call */
    HWND           tooltip;  /* Icon tooltip */
    UINT           id;       /* the unique id given by the app */
    UINT           callback_message;
    int            display;  /* index in display list, or -1 if hidden */
    WCHAR          tiptext[128]; /* Tooltip text. If empty => tooltip disabled */
};

static struct list icon_list = LIST_INIT( icon_list );
static HWND tray_window;

static unsigned int alloc_displayed;
static unsigned int nb_displayed;
static struct icon **displayed;  /* array of currently displayed icons */

static BOOL hide_systray;
static int icon_cx, icon_cy, tray_width;

#define MIN_DISPLAYED 8
#define ICON_BORDER  2

/* Retrieves icon record by owner window and ID */
static struct icon *get_icon(HWND owner, UINT id)
{
    struct icon *this;

    /* search for the icon */
    LIST_FOR_EACH_ENTRY( this, &icon_list, struct icon, entry )
        if ((this->id == id) && (this->owner == owner)) return this;

    return NULL;
}

static RECT get_icon_rect( struct icon *icon )
{
    RECT rect;

    rect.right = tray_width - icon_cx * icon->display;
    rect.left = rect.right - icon_cx;
    rect.top = 0;
    rect.bottom = icon_cy;
    return rect;
}

/* Creates tooltip window for icon. */
static void create_tooltip(struct icon *icon)
{
    TTTOOLINFOW ti;
    static BOOL tooltips_initialized = FALSE;

    /* Register tooltip classes if this is the first icon */
    if (!tooltips_initialized)
    {
        INITCOMMONCONTROLSEX init_tooltip;

        init_tooltip.dwSize = sizeof(INITCOMMONCONTROLSEX);
        init_tooltip.dwICC = ICC_TAB_CLASSES;

        InitCommonControlsEx(&init_tooltip);
        tooltips_initialized = TRUE;
    }

    icon->tooltip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL,
                                   WS_POPUP | TTS_ALWAYSTIP,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   tray_window, NULL, NULL, NULL);

    ZeroMemory(&ti, sizeof(ti));
    ti.cbSize = sizeof(TTTOOLINFOW);
    ti.hwnd = tray_window;
    ti.lpszText = icon->tiptext;
    if (icon->display != -1) ti.rect = get_icon_rect( icon );
    SendMessageW(icon->tooltip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
}

/* Synchronize tooltip text with tooltip window */
static void update_tooltip_text(struct icon *icon)
{
    TTTOOLINFOW ti;

    ZeroMemory(&ti, sizeof(ti));
    ti.cbSize = sizeof(TTTOOLINFOW);
    ti.hwnd = tray_window;
    ti.lpszText = icon->tiptext;

    SendMessageW(icon->tooltip, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
}

/* synchronize tooltip position with tooltip window */
static void update_tooltip_position( struct icon *icon )
{
    TTTOOLINFOW ti;

    ZeroMemory(&ti, sizeof(ti));
    ti.cbSize = sizeof(TTTOOLINFOW);
    ti.hwnd = tray_window;
    if (icon->display != -1) ti.rect = get_icon_rect( icon );
    SendMessageW( icon->tooltip, TTM_NEWTOOLRECTW, 0, (LPARAM)&ti );
}

/* find the icon located at a certain point in the tray window */
static struct icon *icon_from_point( int x, int y )
{
    if (y < 0 || y >= icon_cy) return NULL;
    x = tray_width - x;
    if (x < 0 || x >= icon_cx * nb_displayed) return NULL;
    return displayed[x / icon_cx];
}

/* invalidate the portion of the tray window that contains the specified icons */
static void invalidate_icons( unsigned int start, unsigned int end )
{
    RECT rect;

    rect.left = tray_width - (end + 1) * icon_cx;
    rect.top  = 0;
    rect.right = tray_width - start * icon_cx;
    rect.bottom = icon_cy;
    InvalidateRect( tray_window, &rect, TRUE );
}

/* make an icon visible */
static BOOL show_icon(struct icon *icon)
{
    WINE_TRACE("id=0x%x, hwnd=%p\n", icon->id, icon->owner);

    if (icon->display != -1) return TRUE;  /* already displayed */

    if (nb_displayed >= alloc_displayed)
    {
        unsigned int new_count = max( alloc_displayed * 2, 32 );
        struct icon **ptr;
        if (displayed) ptr = HeapReAlloc( GetProcessHeap(), 0, displayed, new_count * sizeof(*ptr) );
        else ptr = HeapAlloc( GetProcessHeap(), 0, new_count * sizeof(*ptr) );
        if (!ptr) return FALSE;
        displayed = ptr;
        alloc_displayed = new_count;
    }

    icon->display = nb_displayed;
    displayed[nb_displayed++] = icon;
    update_tooltip_position( icon );
    invalidate_icons( nb_displayed-1, nb_displayed-1 );

    if (nb_displayed == 1 && !hide_systray) ShowWindow( tray_window, SW_SHOWNA );

    create_tooltip(icon);
    return TRUE;
}

/* make an icon invisible */
static BOOL hide_icon(struct icon *icon)
{
    unsigned int i;

    WINE_TRACE("id=0x%x, hwnd=%p\n", icon->id, icon->owner);

    if (icon->display == -1) return TRUE;  /* already hidden */

    assert( nb_displayed );
    for (i = icon->display; i < nb_displayed - 1; i++)
    {
        displayed[i] = displayed[i + 1];
        displayed[i]->display = i;
        update_tooltip_position( displayed[i] );
    }
    nb_displayed--;
    invalidate_icons( icon->display, nb_displayed );
    icon->display = -1;

    if (!nb_displayed) ShowWindow( tray_window, SW_HIDE );

    update_tooltip_position( icon );
    return TRUE;
}

/* Modifies an existing icon record */
static BOOL modify_icon( struct icon *icon, NOTIFYICONDATAW *nid )
{
    WINE_TRACE("id=0x%x, hwnd=%p\n", nid->uID, nid->hWnd);

    /* demarshal the request from the NID */
    if (!icon)
    {
        WINE_WARN("Invalid icon ID (0x%x) for HWND %p\n", nid->uID, nid->hWnd);
        return FALSE;
    }

    if ((nid->uFlags & NIF_STATE) && (nid->dwStateMask & NIS_HIDDEN))
    {
        if (nid->dwState & NIS_HIDDEN) hide_icon( icon );
        else show_icon( icon );
    }

    if (nid->uFlags & NIF_ICON)
    {
        if (icon->image) DestroyIcon(icon->image);
        icon->image = CopyIcon(nid->hIcon);
        if (icon->display != -1) invalidate_icons( icon->display, icon->display );
    }

    if (nid->uFlags & NIF_MESSAGE)
    {
        icon->callback_message = nid->uCallbackMessage;
    }
    if (nid->uFlags & NIF_TIP)
    {
        lstrcpynW(icon->tiptext, nid->szTip, sizeof(icon->tiptext)/sizeof(WCHAR));
        if (icon->display != -1) update_tooltip_text(icon);
    }
    if (nid->uFlags & NIF_INFO && nid->cbSize >= NOTIFYICONDATAA_V2_SIZE)
    {
        WINE_FIXME("balloon tip title %s, message %s\n", wine_dbgstr_w(nid->szInfoTitle), wine_dbgstr_w(nid->szInfo));
    }
    return TRUE;
}

/* Adds a new icon record to the list */
static BOOL add_icon(NOTIFYICONDATAW *nid)
{
    struct icon  *icon;

    WINE_TRACE("id=0x%x, hwnd=%p\n", nid->uID, nid->hWnd);

    if ((icon = get_icon(nid->hWnd, nid->uID)))
    {
        WINE_WARN("duplicate tray icon add, buggy app?\n");
        return FALSE;
    }

    if (!(icon = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*icon))))
    {
        WINE_ERR("out of memory\n");
        return FALSE;
    }

    ZeroMemory(icon, sizeof(struct icon));
    icon->id     = nid->uID;
    icon->owner  = nid->hWnd;
    icon->display = -1;

    if (list_empty( &icon_list )) SetTimer( tray_window, 1, 2000, NULL );
    list_add_tail(&icon_list, &icon->entry);

    modify_icon( icon, nid );
    /* show icon, unless hidden state was explicitly specified */
    if (!((nid->uFlags & NIF_STATE) && (nid->dwStateMask & NIS_HIDDEN))) show_icon( icon );
    return TRUE;
}

/* Deletes tray icon window and icon record */
static BOOL delete_icon(struct icon *icon)
{
    hide_icon(icon);
    list_remove(&icon->entry);
    DestroyIcon(icon->image);
    HeapFree(GetProcessHeap(), 0, icon);
    if (list_empty( &icon_list )) KillTimer( tray_window, 1 );
    return TRUE;
}

/* cleanup icons belonging to windows that have been destroyed */
static void cleanup_destroyed_windows(void)
{
    struct icon *icon, *next;

    LIST_FOR_EACH_ENTRY_SAFE( icon, next, &icon_list, struct icon, entry )
        if (!IsWindow( icon->owner )) delete_icon( icon );
}

static BOOL handle_incoming(HWND hwndSource, COPYDATASTRUCT *cds)
{
    struct icon *icon = NULL;
    const struct notify_data *data;
    NOTIFYICONDATAW nid;
    int ret = FALSE;

    if (cds->cbData < sizeof(*data)) return FALSE;
    data = cds->lpData;

    nid.cbSize           = sizeof(nid);
    nid.hWnd             = LongToHandle( data->hWnd );
    nid.uID              = data->uID;
    nid.uFlags           = data->uFlags;
    nid.uCallbackMessage = data->uCallbackMessage;
    nid.hIcon            = 0;
    nid.dwState          = data->dwState;
    nid.dwStateMask      = data->dwStateMask;
    nid.u.uTimeout       = data->u.uTimeout;
    nid.dwInfoFlags      = data->dwInfoFlags;
    nid.guidItem         = data->guidItem;
    lstrcpyW( nid.szTip, data->szTip );
    lstrcpyW( nid.szInfo, data->szInfo );
    lstrcpyW( nid.szInfoTitle, data->szInfoTitle );
    nid.hBalloonIcon     = 0;

    /* FIXME: if statement only needed because we don't support interprocess
     * icon handles */
    if ((nid.uFlags & NIF_ICON) && cds->cbData > sizeof(*data))
    {
        LONG cbMaskBits;
        LONG cbColourBits;
        const char *buffer = (const char *)(data + 1);

        cbMaskBits = (data->width * data->height + 15) / 16 * 2;
        cbColourBits = (data->planes * data->width * data->height * data->bpp + 15) / 16 * 2;

        if (cds->cbData < sizeof(*data) + cbMaskBits + cbColourBits)
        {
            WINE_ERR("buffer underflow\n");
            return FALSE;
        }
        nid.hIcon = CreateIcon(NULL, data->width, data->height, data->planes, data->bpp,
                               buffer, buffer + cbMaskBits);
    }

    /* try forward to x11drv first */
    if (cds->dwData == NIM_ADD || !(icon = get_icon( nid.hWnd, nid.uID )))
    {
        if (wine_notify_icon && ((ret = wine_notify_icon( cds->dwData, &nid )) != -1))
        {
            if (nid.hIcon) DestroyIcon( nid.hIcon );
            return ret;
        }
        ret = FALSE;
    }

    switch (cds->dwData)
    {
    case NIM_ADD:
        ret = add_icon(&nid);
        break;
    case NIM_DELETE:
        if (icon) ret = delete_icon( icon );
        break;
    case NIM_MODIFY:
        if (icon) ret = modify_icon( icon, &nid );
        break;
    default:
        WINE_FIXME("unhandled tray message: %ld\n", cds->dwData);
        break;
    }

    if (nid.hIcon) DestroyIcon( nid.hIcon );
    return ret;
}

static void do_hide_systray(void)
{
    SetWindowPos( tray_window, 0,
                  GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN),
                  GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN),
                  0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
}

static LRESULT WINAPI tray_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_COPYDATA:
        return handle_incoming((HWND)wparam, (COPYDATASTRUCT *)lparam);

    case WM_DISPLAYCHANGE:
        if (hide_systray) do_hide_systray();
        else
        {
            tray_width = GetSystemMetrics( SM_CXSCREEN );
            SetWindowPos( tray_window, 0, 0, GetSystemMetrics( SM_CYSCREEN ) - icon_cy,
                          tray_width, icon_cy, SWP_NOZORDER | SWP_NOACTIVATE );
        }
        break;

    case WM_TIMER:
        cleanup_destroyed_windows();
        break;

    case WM_PAINT:
        {
            unsigned int i;
            PAINTSTRUCT ps;
            HDC hdc;

            hdc = BeginPaint( hwnd, &ps );
            for (i = 0; i < nb_displayed; i++)
            {
                RECT dummy, rect = get_icon_rect( displayed[i] );
                if (IntersectRect( &dummy, &rect, &ps.rcPaint ))
                    DrawIconEx( hdc, rect.left + ICON_BORDER, rect.top + ICON_BORDER, displayed[i]->image,
                                icon_cx - 2*ICON_BORDER, icon_cy - 2*ICON_BORDER,
                            0, 0, DI_DEFAULTSIZE|DI_NORMAL);
            }
            EndPaint( hwnd, &ps );
            break;
        }

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
        {
            MSG message;
            struct icon *icon = icon_from_point( (short)LOWORD(lparam), (short)HIWORD(lparam) );
            if (!icon) break;

            /* notify the owner hwnd of the message */
            WINE_TRACE("relaying 0x%x\n", msg);

            message.hwnd = hwnd;
            message.message = msg;
            message.wParam = wparam;
            message.lParam = lparam;
            SendMessageW( icon->tooltip, TTM_RELAYEVENT, 0, (LPARAM)&message );

            if (!PostMessageW( icon->owner, icon->callback_message, (WPARAM) icon->id, (LPARAM) msg ) &&
                GetLastError() == ERROR_INVALID_WINDOW_HANDLE)
            {
                WINE_WARN("application window was destroyed without removing "
                          "notification icon, removing automatically\n");
                delete_icon( icon );
            }
            break;
        }

    case WM_CLOSE:
        /* don't destroy the tray window, just hide it */
        ShowWindow( hwnd, SW_HIDE );
        return 0;

    default:
        return DefWindowProcW( hwnd, msg, wparam, lparam );
    }
    return 0;
}

/* this function creates the listener window */
void initialize_systray( BOOL using_root )
{
    HMODULE x11drv;
    WNDCLASSEXW class;
    static const WCHAR classname[] = {'S','h','e','l','l','_','T','r','a','y','W','n','d',0};

    if ((x11drv = GetModuleHandleA( "winex11.drv" )))
        wine_notify_icon = (void *)GetProcAddress( x11drv, "wine_notify_icon" );

    icon_cx = GetSystemMetrics( SM_CXSMICON ) + 2*ICON_BORDER;
    icon_cy = GetSystemMetrics( SM_CYSMICON ) + 2*ICON_BORDER;
    hide_systray = using_root;

    /* register the systray listener window class */
    ZeroMemory(&class, sizeof(class));
    class.cbSize        = sizeof(class);
    class.style         = CS_DBLCLKS | CS_HREDRAW;
    class.lpfnWndProc   = tray_wndproc;
    class.hInstance     = NULL;
    class.hIcon         = LoadIconW(0, (LPCWSTR)IDI_WINLOGO);
    class.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
    class.hbrBackground = (HBRUSH) COLOR_WINDOW;
    class.lpszClassName = (WCHAR *) &classname;

    if (!RegisterClassExW(&class))
    {
        WINE_ERR("Could not register SysTray window class\n");
        return;
    }

    tray_width = GetSystemMetrics( SM_CXSCREEN );
    tray_window = CreateWindowExW( WS_EX_NOACTIVATE, classname, NULL, WS_POPUP,
                                   0, GetSystemMetrics( SM_CYSCREEN ) - icon_cy,
                                   tray_width, icon_cy, 0, 0, 0, 0 );
    if (!tray_window)
    {
        WINE_ERR("Could not create tray window\n");
        return;
    }

    if (hide_systray) do_hide_systray();
}
