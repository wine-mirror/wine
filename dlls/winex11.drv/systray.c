/*
 * X11 system tray management
 *
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

#include "x11drv_dll.h"
#include "commctrl.h"
#include "shellapi.h"

#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(systray);

BOOL show_systray = TRUE;

/* an individual systray icon */
struct tray_icon
{
    struct list    entry;
    HICON          image;    /* the image to render */
    HWND           owner;    /* the HWND passed in to the Shell_NotifyIcon call */
    HWND           window;   /* the adaptor window */
    BOOL           layered;  /* whether we are using a layered window */
    HWND           tooltip;  /* Icon tooltip */
    UINT           state;    /* state flags */
    UINT           id;       /* the unique id given by the app */
    UINT           callback_message;
    int            display;  /* display index, or -1 if hidden */
    WCHAR          tiptext[128];    /* tooltip text */
    WCHAR          info_text[256];  /* info balloon text */
    WCHAR          info_title[64];  /* info balloon title */
    UINT           info_flags;      /* flags for info balloon */
    UINT           info_timeout;    /* timeout for info balloon */
    HICON          info_icon;       /* info balloon icon */
    UINT           version;         /* notify icon api version */
};

static struct list icon_list = LIST_INIT( icon_list );

static const WCHAR icon_classname[] = {'_','_','w','i','n','e','x','1','1','_','t','r','a','y','_','i','c','o','n',0};
static const WCHAR tray_classname[] = {'_','_','w','i','n','e','x','1','1','_','s','t','a','n','d','a','l','o','n','e','_','t','r','a','y',0};

static BOOL show_icon( struct tray_icon *icon );
static BOOL hide_icon( struct tray_icon *icon );
static BOOL delete_icon( struct tray_icon *icon );

#define MIN_DISPLAYED 8
#define ICON_BORDER 2

#define BALLOON_CREATE_TIMER 1
#define BALLOON_SHOW_TIMER   2

#define BALLOON_CREATE_TIMEOUT   2000
#define BALLOON_SHOW_MIN_TIMEOUT 10000
#define BALLOON_SHOW_MAX_TIMEOUT 30000

static struct tray_icon *balloon_icon;
static HWND balloon_window;
static POINT balloon_pos;

/* stand-alone tray window */
static HWND standalone_tray;
static int icon_cx, icon_cy;
static unsigned int nb_displayed;

/* retrieves icon record by owner window and ID */
static struct tray_icon *get_icon(HWND owner, UINT id)
{
    struct tray_icon *this;

    LIST_FOR_EACH_ENTRY( this, &icon_list, struct tray_icon, entry )
        if ((this->id == id) && (this->owner == owner)) return this;
    return NULL;
}

static void init_common_controls(void)
{
    static BOOL initialized = FALSE;

    if (!initialized)
    {
        INITCOMMONCONTROLSEX init_tooltip;

        init_tooltip.dwSize = sizeof(INITCOMMONCONTROLSEX);
        init_tooltip.dwICC = ICC_TAB_CLASSES;

        InitCommonControlsEx(&init_tooltip);
        initialized = TRUE;
    }
}

/* create tooltip window for icon */
static void create_tooltip(struct tray_icon *icon)
{
    init_common_controls();
    icon->tooltip = CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL,
                                     WS_POPUP | TTS_ALWAYSTIP,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     icon->window, NULL, NULL, NULL);
    if (icon->tooltip)
    {
        TTTOOLINFOW ti;
        ZeroMemory(&ti, sizeof(ti));
        ti.cbSize = sizeof(TTTOOLINFOW);
        ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
        ti.hwnd = icon->window;
        ti.uId = (UINT_PTR)icon->window;
        ti.lpszText = icon->tiptext;
        SendMessageW(icon->tooltip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
    }
}

static void update_systray_balloon_position(void)
{
    RECT rect;
    POINT pos;

    if (!balloon_icon) return;
    GetWindowRect( balloon_icon->window, &rect );
    pos.x = (rect.left + rect.right) / 2;
    pos.y = (rect.top + rect.bottom) / 2;
    if (pos.x == balloon_pos.x && pos.y == balloon_pos.y) return;  /* nothing changed */
    balloon_pos = pos;
    SendMessageW( balloon_window, TTM_TRACKPOSITION, 0, MAKELONG( pos.x, pos.y ));
}

static void balloon_create_timer( struct tray_icon *icon )
{
    TTTOOLINFOW ti;

    init_common_controls();
    balloon_window = CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL,
                                      WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_BALLOON | TTS_CLOSE,
                                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                      icon->window, NULL, NULL, NULL);

    memset( &ti, 0, sizeof(ti) );
    ti.cbSize = sizeof(TTTOOLINFOW);
    ti.hwnd = icon->window;
    ti.uId = (UINT_PTR)icon->window;
    ti.uFlags = TTF_TRACK | TTF_IDISHWND;
    ti.lpszText = icon->info_text;
    SendMessageW( balloon_window, TTM_ADDTOOLW, 0, (LPARAM)&ti );
    if ((icon->info_flags & NIIF_ICONMASK) == NIIF_USER)
        SendMessageW( balloon_window, TTM_SETTITLEW, (WPARAM)icon->info_icon, (LPARAM)icon->info_title );
    else
        SendMessageW( balloon_window, TTM_SETTITLEW, icon->info_flags, (LPARAM)icon->info_title );
    balloon_icon = icon;
    balloon_pos.x = balloon_pos.y = MAXLONG;
    update_systray_balloon_position();
    SendMessageW( balloon_window, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti );
    KillTimer( icon->window, BALLOON_CREATE_TIMER );
    SetTimer( icon->window, BALLOON_SHOW_TIMER, icon->info_timeout, NULL );
}

static BOOL show_balloon( struct tray_icon *icon )
{
    if (standalone_tray && !show_systray) return FALSE;  /* no systray window */
    if (!icon->window) return FALSE;  /* not displayed */
    if (!icon->info_text[0]) return FALSE;  /* no balloon */
    balloon_icon = icon;
    SetTimer( icon->window, BALLOON_CREATE_TIMER, BALLOON_CREATE_TIMEOUT, NULL );
    return TRUE;
}

static void hide_balloon(void)
{
    if (!balloon_icon) return;
    if (balloon_window)
    {
        KillTimer( balloon_icon->window, BALLOON_SHOW_TIMER );
        DestroyWindow( balloon_window );
        balloon_window = 0;
    }
    else KillTimer( balloon_icon->window, BALLOON_CREATE_TIMER );
    balloon_icon = NULL;
}

static void show_next_balloon(void)
{
    struct tray_icon *icon;

    LIST_FOR_EACH_ENTRY( icon, &icon_list, struct tray_icon, entry )
        if (show_balloon( icon )) break;
}

static void update_balloon( struct tray_icon *icon )
{
    if (balloon_icon == icon)
    {
        hide_balloon();
        show_balloon( icon );
    }
    else if (!balloon_icon)
    {
        if (!show_balloon( icon )) return;
    }
    if (!balloon_icon) show_next_balloon();
}

static void balloon_timer(void)
{
    if (balloon_icon) balloon_icon->info_text[0] = 0;  /* clear text now that balloon has been shown */
    hide_balloon();
    show_next_balloon();
}

/* synchronize tooltip text with tooltip window */
static void update_tooltip_text(struct tray_icon *icon)
{
    TTTOOLINFOW ti;

    ZeroMemory(&ti, sizeof(ti));
    ti.cbSize = sizeof(TTTOOLINFOW);
    ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
    ti.hwnd = icon->window;
    ti.uId = (UINT_PTR)icon->window;
    ti.lpszText = icon->tiptext;

    SendMessageW(icon->tooltip, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
}

/* get the size of the stand-alone tray window */
static SIZE get_window_size(void)
{
    SIZE size;
    RECT rect;

    rect.left = 0;
    rect.top = 0;
    rect.right = icon_cx * max( nb_displayed, MIN_DISPLAYED );
    rect.bottom = icon_cy;
    AdjustWindowRect( &rect, WS_CAPTION, FALSE );
    size.cx = rect.right - rect.left;
    size.cy = rect.bottom - rect.top;
    return size;
}

/* get the position of an icon in the stand-alone tray */
static POINT get_icon_pos( struct tray_icon *icon )
{
    POINT pos;

    pos.x = icon_cx * icon->display;
    pos.y = 0;
    return pos;
}

/* window procedure for the standalone tray window */
static LRESULT WINAPI standalone_tray_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch (msg)
    {
    case WM_MOVE:
        update_systray_balloon_position();
        break;
    case WM_CLOSE:
        ShowWindow( hwnd, SW_HIDE );
        hide_balloon();
        show_systray = FALSE;
        return 0;
    case WM_DESTROY:
        standalone_tray = 0;
        break;
    }
    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

/* add an icon to the standalone tray window */
static void add_to_standalone_tray( struct tray_icon *icon )
{
    SIZE size;
    POINT pos;

    if (!standalone_tray)
    {
        static const WCHAR winname[] = {'W','i','n','e',' ','S','y','s','t','e','m',' ','T','r','a','y',0};

        size = get_window_size();
        standalone_tray = CreateWindowExW( 0, tray_classname, winname, WS_CAPTION | WS_SYSMENU,
                                           CW_USEDEFAULT, CW_USEDEFAULT, size.cx, size.cy, 0, 0, 0, 0 );
        if (!standalone_tray) return;
    }

    icon->display = nb_displayed;
    pos = get_icon_pos( icon );
    CreateWindowW( icon_classname, NULL, WS_CHILD | WS_VISIBLE,
                   pos.x, pos.y, icon_cx, icon_cy, standalone_tray, NULL, NULL, icon );
    if (!icon->window)
    {
        icon->display = -1;
        return;
    }

    nb_displayed++;
    size = get_window_size();
    SetWindowPos( standalone_tray, 0, 0, 0, size.cx, size.cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER );
    if (nb_displayed == 1 && show_systray) ShowWindow( standalone_tray, SW_SHOWNA );
    TRACE( "added %u now %d icons\n", icon->id, nb_displayed );
}

/* remove an icon from the stand-alone tray */
static void remove_from_standalone_tray( struct tray_icon *icon )
{
    struct tray_icon *ptr;
    POINT pos;

    if (icon->display == -1) return;

    LIST_FOR_EACH_ENTRY( ptr, &icon_list, struct tray_icon, entry )
    {
        if (ptr == icon) continue;
        if (ptr->display < icon->display) continue;
        ptr->display--;
        pos = get_icon_pos( ptr );
        SetWindowPos( ptr->window, 0, pos.x, pos.y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER );
    }
    icon->display = -1;
    if (!--nb_displayed) ShowWindow( standalone_tray, SW_HIDE );
    TRACE( "removed %u now %d icons\n", icon->id, nb_displayed );
}

static void repaint_tray_icon( struct tray_icon *icon )
{
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    int width = GetSystemMetrics( SM_CXSMICON );
    int height = GetSystemMetrics( SM_CYSMICON );
    BITMAPINFO *info;
    HBITMAP dib, mask;
    HDC hdc;
    RECT rc;
    SIZE size;
    POINT pos;
    int i, x, y;
    void *color_bits, *mask_bits;
    DWORD *ptr;
    BOOL has_alpha = FALSE;

    GetWindowRect( icon->window, &rc );
    size.cx = rc.right - rc.left;
    size.cy = rc.bottom - rc.top;
    pos.x = (size.cx - width) / 2;
    pos.y = (size.cy - height) / 2;

    info = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, FIELD_OFFSET( BITMAPINFO, bmiColors[2] ));
    if (!info) return;
    info->bmiHeader.biSize = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth = size.cx;
    info->bmiHeader.biHeight = size.cy;
    info->bmiHeader.biBitCount = 32;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biCompression = BI_RGB;

    hdc = CreateCompatibleDC( 0 );
    if (!(dib = CreateDIBSection( 0, info, DIB_RGB_COLORS, &color_bits, NULL, 0 ))) goto done;
    SelectObject( hdc, dib );
    DrawIconEx( hdc, pos.x, pos.y, icon->image, width, height, 0, 0, DI_DEFAULTSIZE | DI_NORMAL );

    /* check if the icon was drawn with an alpha channel */
    for (i = 0, ptr = color_bits; i < size.cx * size.cy; i++)
        if ((has_alpha = (ptr[i] & 0xff000000) != 0)) break;

    if (!has_alpha)
    {
        unsigned int width_bytes = (size.cx + 31) / 32 * 4;

        info->bmiHeader.biBitCount = 1;
        info->bmiColors[0].rgbRed      = 0;
        info->bmiColors[0].rgbGreen    = 0;
        info->bmiColors[0].rgbBlue     = 0;
        info->bmiColors[0].rgbReserved = 0;
        info->bmiColors[1].rgbRed      = 0xff;
        info->bmiColors[1].rgbGreen    = 0xff;
        info->bmiColors[1].rgbBlue     = 0xff;
        info->bmiColors[1].rgbReserved = 0;

        if (!(mask = CreateDIBSection( 0, info, DIB_RGB_COLORS, &mask_bits, NULL, 0 ))) goto done;
        memset( mask_bits, 0xff, width_bytes * size.cy );
        SelectObject( hdc, mask );
        DrawIconEx( hdc, pos.x, pos.y, icon->image, width, height, 0, 0, DI_DEFAULTSIZE | DI_MASK );

        for (y = 0, ptr = color_bits; y < size.cy; y++)
            for (x = 0; x < size.cx; x++, ptr++)
                if (!((((BYTE *)mask_bits)[y * width_bytes + x / 8] << (x % 8)) & 0x80))
                    *ptr |= 0xff000000;

        SelectObject( hdc, dib );
        DeleteObject( mask );
    }

    UpdateLayeredWindow( icon->window, 0, NULL, NULL, hdc, NULL, 0, &blend, ULW_ALPHA );
done:
    HeapFree (GetProcessHeap(), 0, info);
    if (hdc) DeleteDC( hdc );
    if (dib) DeleteObject( dib );
}

static BOOL notify_owner( struct tray_icon *icon, UINT msg, LPARAM lparam )
{
    WPARAM wp = icon->id;
    LPARAM lp = msg;

    if (icon->version >= NOTIFYICON_VERSION_4)
    {
        POINT pt = { (short)LOWORD(lparam), (short)HIWORD(lparam) };

        ClientToScreen( icon->window, &pt );
        wp = MAKEWPARAM( pt.x, pt.y );
        lp = MAKELPARAM( msg, icon->id );
    }

    TRACE( "relaying 0x%x\n", msg );
    if (!SendNotifyMessageW( icon->owner, icon->callback_message, wp, lp ) &&
        (GetLastError() == ERROR_INVALID_WINDOW_HANDLE))
    {
        WARN( "application window was destroyed, removing icon %u\n", icon->id );
        delete_icon( icon );
        return FALSE;
    }
    return TRUE;
}

/* window procedure for the individual tray icon window */
static LRESULT WINAPI tray_icon_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    struct tray_icon *icon = NULL;

    TRACE("hwnd=%p, msg=0x%x\n", hwnd, msg);

    /* set the icon data for the window from the data passed into CreateWindow */
    if (msg == WM_NCCREATE)
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LPARAM)((const CREATESTRUCTW *)lparam)->lpCreateParams);

    icon = (struct tray_icon *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_CREATE:
        icon->window = hwnd;
        create_tooltip( icon );
        break;

    case WM_SIZE:
        if (icon->window && icon->layered) repaint_tray_icon( icon );
        break;

    case WM_PAINT:
        if (!icon->layered)
        {
            PAINTSTRUCT ps;
            RECT rc;
            HDC hdc;
            int cx = GetSystemMetrics( SM_CXSMICON );
            int cy = GetSystemMetrics( SM_CYSMICON );

            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rc);
            TRACE("painting rect %s\n", wine_dbgstr_rect(&rc));
            DrawIconEx( hdc, (rc.left + rc.right - cx) / 2, (rc.top + rc.bottom - cy) / 2,
                        icon->image, cx, cy, 0, 0, DI_DEFAULTSIZE|DI_NORMAL );
            EndPaint(hwnd, &ps);
            return 0;
        }
        break;

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
        notify_owner( icon, msg, lparam );
        break;

    case WM_LBUTTONUP:
        if (!notify_owner( icon, msg, lparam )) break;
        if (icon->version > 0) notify_owner( icon, NIN_SELECT, lparam );
        break;

    case WM_RBUTTONUP:
        if (!notify_owner( icon, msg, lparam )) break;
        if (icon->version > 0) notify_owner( icon, WM_CONTEXTMENU, lparam );
        break;

    case WM_WINDOWPOSCHANGED:
        update_systray_balloon_position();
        break;

    case WM_TIMER:
        switch (wparam)
        {
        case BALLOON_CREATE_TIMER:
            balloon_create_timer( icon );
            break;
        case BALLOON_SHOW_TIMER:
            balloon_timer();
            break;
        }
        return 0;

    case WM_CLOSE:
        if (icon->display == -1)
        {
            TRACE( "icon %u no longer embedded\n", icon->id );
            hide_icon( icon );
            add_to_standalone_tray( icon );
        }
        return 0;
    }
    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static BOOL init_systray(void)
{
    static BOOL init_done;
    WNDCLASSEXW class;

    if (init_done) return TRUE;
    if (!X11DRV_CALL( systray_init, NULL ))
    {
        init_done = TRUE;
        return FALSE;
    }

    icon_cx = GetSystemMetrics( SM_CXSMICON ) + 2 * ICON_BORDER;
    icon_cy = GetSystemMetrics( SM_CYSMICON ) + 2 * ICON_BORDER;

    memset( &class, 0, sizeof(class) );
    class.cbSize        = sizeof(class);
    class.lpfnWndProc   = tray_icon_wndproc;
    class.hIcon         = LoadIconW(0, (LPCWSTR)IDI_WINLOGO);
    class.hCursor       = LoadCursorW( 0, (LPCWSTR)IDC_ARROW );
    class.lpszClassName = icon_classname;
    class.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;

    if (!RegisterClassExW( &class ) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        ERR( "Could not register icon tray window class\n" );
        return FALSE;
    }

    class.lpfnWndProc   = standalone_tray_wndproc;
    class.hbrBackground = (HBRUSH)COLOR_WINDOW;
    class.lpszClassName = tray_classname;
    class.style         = CS_DBLCLKS;

    if (!RegisterClassExW( &class ) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        ERR( "Could not register standalone tray window class\n" );
        return FALSE;
    }

    init_done = TRUE;
    return TRUE;
}

/* dock systray windows again with the new owner */
NTSTATUS WINAPI x11drv_systray_change_owner( void *arg, ULONG size )
{
    struct systray_change_owner_params *params = arg;
    struct systray_dock_params dock_params;
    struct tray_icon *icon;

    LIST_FOR_EACH_ENTRY( icon, &icon_list, struct tray_icon, entry )
    {
        if (icon->display == -1) continue;
        hide_icon( icon );

        dock_params.event_handle = params->event_handle;
        dock_params.icon = icon;
        dock_params.cx = icon_cx;
        dock_params.cy = icon_cy;
        dock_params.layered = &icon->layered;
        X11DRV_CALL( systray_dock, &dock_params );
    }

    return 0;
}

/* hide a tray icon */
static BOOL hide_icon( struct tray_icon *icon )
{
    TRACE( "id=0x%x, hwnd=%p\n", icon->id, icon->owner );

    if (!icon->window) return TRUE;  /* already hidden */

    X11DRV_CALL( systray_hide, &icon->window );
    DestroyWindow(icon->window);
    DestroyWindow(icon->tooltip);
    icon->window = 0;
    icon->layered = FALSE;
    icon->tooltip = 0;
    remove_from_standalone_tray( icon );
    update_balloon( icon );
    return TRUE;
}

/* make the icon visible */
static BOOL show_icon( struct tray_icon *icon )
{
    struct systray_dock_params params;

    if (icon->window) return TRUE;  /* already shown */

    TRACE( "id=0x%x, hwnd=%p\n", icon->id, icon->owner );

    params.event_handle = 0;
    params.icon = icon;
    params.cx = icon_cx;
    params.cy = icon_cy;
    params.layered = &icon->layered;

    if (X11DRV_CALL( systray_dock, &params ))
        add_to_standalone_tray( icon );

    update_balloon( icon );
    return TRUE;
}

/* Modifies an existing icon record */
static BOOL modify_icon( struct tray_icon *icon, NOTIFYICONDATAW *nid )
{
    TRACE( "id=0x%x hwnd=%p flags=%x\n", nid->uID, nid->hWnd, nid->uFlags );

    if (nid->uFlags & NIF_STATE)
    {
        icon->state = (icon->state & ~nid->dwStateMask) | (nid->dwState & nid->dwStateMask);
    }

    if (nid->uFlags & NIF_ICON)
    {
        if (icon->image) DestroyIcon(icon->image);
        icon->image = CopyIcon(nid->hIcon);
        if (icon->window)
        {
            if (icon->display != -1) InvalidateRect( icon->window, NULL, TRUE );
            else if (icon->layered) repaint_tray_icon( icon );
            else X11DRV_CALL( systray_clear, &icon->window );
        }
    }

    if (nid->uFlags & NIF_MESSAGE)
    {
        icon->callback_message = nid->uCallbackMessage;
    }
    if (nid->uFlags & NIF_TIP)
    {
        lstrcpynW(icon->tiptext, nid->szTip, ARRAY_SIZE( icon->tiptext ));
        if (icon->tooltip) update_tooltip_text(icon);
    }
    if (nid->uFlags & NIF_INFO && nid->cbSize >= NOTIFYICONDATAA_V2_SIZE)
    {
        lstrcpynW( icon->info_text, nid->szInfo, ARRAY_SIZE( icon->info_text ));
        lstrcpynW( icon->info_title, nid->szInfoTitle, ARRAY_SIZE( icon->info_title ));
        icon->info_flags = nid->dwInfoFlags;
        icon->info_timeout = max(min(nid->uTimeout, BALLOON_SHOW_MAX_TIMEOUT), BALLOON_SHOW_MIN_TIMEOUT);
        icon->info_icon = nid->hBalloonIcon;
        update_balloon( icon );
    }
    if (icon->state & NIS_HIDDEN) hide_icon( icon );
    else show_icon( icon );
    return TRUE;
}

/* Adds a new icon record to the list */
static BOOL add_icon(NOTIFYICONDATAW *nid)
{
    struct tray_icon  *icon;

    TRACE("id=0x%x, hwnd=%p\n", nid->uID, nid->hWnd);

    if ((icon = get_icon(nid->hWnd, nid->uID)))
    {
        WARN("duplicate tray icon add, buggy app?\n");
        return FALSE;
    }

    if (!(icon = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*icon))))
    {
        ERR("out of memory\n");
        return FALSE;
    }

    ZeroMemory(icon, sizeof(struct tray_icon));
    icon->id     = nid->uID;
    icon->owner  = nid->hWnd;
    icon->display = -1;

    list_add_tail(&icon_list, &icon->entry);

    return modify_icon( icon, nid );
}

/* delete tray icon window and icon structure */
static BOOL delete_icon( struct tray_icon *icon )
{
    hide_icon( icon );
    list_remove( &icon->entry );
    DestroyIcon( icon->image );
    HeapFree( GetProcessHeap(), 0, icon );
    return TRUE;
}

/* cleanup all icons for a given window */
static void cleanup_icons( HWND owner )
{
    struct tray_icon *this, *next;

    LIST_FOR_EACH_ENTRY_SAFE( this, next, &icon_list, struct tray_icon, entry )
        if (this->owner == owner) delete_icon( this );
}


/***********************************************************************
 *              wine_notify_icon   (X11DRV.@)
 *
 * Driver-side implementation of Shell_NotifyIcon.
 */
int CDECL wine_notify_icon( DWORD msg, NOTIFYICONDATAW *data )
{
    BOOL ret = FALSE;
    struct tray_icon *icon;

    switch (msg)
    {
    case NIM_ADD:
        if (!init_systray()) return -1;  /* fall back to default handling */
        ret = add_icon( data );
        break;
    case NIM_DELETE:
        if ((icon = get_icon( data->hWnd, data->uID ))) ret = delete_icon( icon );
        break;
    case NIM_MODIFY:
        if ((icon = get_icon( data->hWnd, data->uID ))) ret = modify_icon( icon, data );
        break;
    case NIM_SETVERSION:
        if ((icon = get_icon( data->hWnd, data->uID )))
        {
            icon->version = data->uVersion;
            ret = TRUE;
        }
        break;
    case 0xdead:  /* Wine extension: owner window has died */
        cleanup_icons( data->hWnd );
        break;
    default:
        FIXME( "unhandled tray message: %lu\n", msg );
        break;
    }
    return ret;
}


/* window procedure for foreign windows */
LRESULT WINAPI foreign_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch(msg)
    {
    case WM_WINDOWPOSCHANGED:
        update_systray_balloon_position();
        break;
    case WM_PARENTNOTIFY:
        if (LOWORD(wparam) == WM_DESTROY)
        {
            TRACE( "%p: got parent notify destroy for win %Ix\n", hwnd, lparam );
            PostMessageW( hwnd, WM_CLOSE, 0, 0 );  /* so that we come back here once the child is gone */
        }
        return 0;
    case WM_CLOSE:
        if (GetWindow( hwnd, GW_CHILD )) return 0;  /* refuse to die if we still have children */
        break;
    }
    return DefWindowProcW( hwnd, msg, wparam, lparam );
}
