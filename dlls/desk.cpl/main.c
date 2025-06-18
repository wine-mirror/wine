/*
 * Copyright 2024 Rémi Bernon for CodeWeavers
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
 *
 */

#include "desk_private.h"

#include <commctrl.h>
#include <cpl.h>
#include "ole2.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(deskcpl);

static HMODULE module;

struct device_entry
{
    struct list entry;
    DISPLAY_DEVICEW adapter;
    DEVMODEW current; /* device mode to be applied */
    DEVMODEW pending; /* pending mode when moving */

    RECT draw_rect;
    float draw_scale;
    BOOL mouse_over;
    POINT move_point;
};
static struct list devices = LIST_INIT( devices );
static struct device_entry *selected;
static BOOL dragging;

static void clear_devices( HWND hwnd )
{
    struct device_entry *entry, *next;

    selected = NULL;
    dragging = FALSE;
    SendDlgItemMessageW( hwnd, IDC_DISPLAY_SETTINGS_LIST, CB_RESETCONTENT, 0, 0 );

    LIST_FOR_EACH_ENTRY_SAFE( entry, next, &devices, struct device_entry, entry )
    {
        list_remove( &entry->entry );
        free( entry );
    }
}

static void refresh_device_list( HWND hwnd )
{
    DISPLAY_DEVICEW adapter = {.cb = sizeof(adapter)};
    struct device_entry *entry;
    UINT i;

    clear_devices( hwnd );

    for (i = 0; EnumDisplayDevicesW( NULL, i, &adapter, 0 ); ++i)
    {
        /* FIXME: Implement detached adapters */
        if (!(adapter.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) continue;
        if (!(entry = calloc( 1, sizeof(*entry) ))) return;
        entry->adapter = adapter;
        entry->current.dmSize = sizeof(entry->current);
        EnumDisplaySettingsW( adapter.DeviceName, ENUM_CURRENT_SETTINGS, &entry->current );
        entry->pending = entry->current;
        list_add_tail( &devices, &entry->entry );
    }
}

static BOOL is_same_devmode( const DEVMODEW *a, const DEVMODEW *b )
{
    return a->dmDisplayOrientation == b->dmDisplayOrientation &&
           a->dmBitsPerPel == b->dmBitsPerPel &&
           a->dmPelsWidth == b->dmPelsWidth &&
           a->dmPelsHeight == b->dmPelsHeight &&
           a->dmDisplayFrequency == b->dmDisplayFrequency;
}

static void update_display_settings_list( HWND hwnd )
{
    DEVMODEW mode = {.dmSize = sizeof(mode)};
    UINT i, sel = -1;

    SendDlgItemMessageW( hwnd, IDC_DISPLAY_SETTINGS_LIST, CB_RESETCONTENT, 0, 0 );
    if (!selected) return;

    for (i = 0; EnumDisplaySettingsExW( selected->adapter.DeviceName, i, &mode, 0 ); ++i)
    {
        WCHAR buffer[1024];
        swprintf( buffer, ARRAY_SIZE(buffer), L"%ux%u (%uHz %ubpp)",
                  mode.dmPelsWidth, mode.dmPelsHeight, mode.dmDisplayFrequency, mode.dmBitsPerPel );
        SendDlgItemMessageW( hwnd, IDC_DISPLAY_SETTINGS_LIST, CB_ADDSTRING, 0, (LPARAM)buffer );
        if (is_same_devmode( &mode, &selected->current )) sel = i;
    }

    if (sel != -1) SendDlgItemMessageW( hwnd, IDC_DISPLAY_SETTINGS_LIST, CB_SETCURSEL, sel, 0 );
}

static RECT rect_from_devmode( const DEVMODEW *mode )
{
    RECT rect = {0};
    if (mode->dmFields & DM_POSITION) SetRect( &rect, mode->dmPosition.x, mode->dmPosition.y, mode->dmPosition.x, mode->dmPosition.y );
    if (mode->dmFields & DM_PELSWIDTH) rect.right += mode->dmPelsWidth;
    if (mode->dmFields & DM_PELSHEIGHT) rect.bottom += mode->dmPelsHeight;
    return rect;
}

static void device_entry_move_rect( struct device_entry *device, DEVMODEW mode )
{
    RECT new_rect, old_rect, tmp;
    struct device_entry *entry;

    old_rect = rect_from_devmode( &device->pending );
    new_rect = rect_from_devmode( &mode );

    /* adjust the position to avoid any overlapping */
    LIST_FOR_EACH_ENTRY( entry, &devices, struct device_entry, entry )
    {
        RECT other = rect_from_devmode( &entry->current );
        if (entry == device) continue;
        if (!IntersectRect( &tmp, &other, &new_rect )) continue;
        if (old_rect.left >= other.right) mode.dmPosition.x = other.right;
        if (old_rect.right <= other.left) mode.dmPosition.x = other.left - mode.dmPelsWidth;
        if (old_rect.top >= other.bottom) mode.dmPosition.y = other.bottom;
        if (old_rect.bottom <= other.top) mode.dmPosition.y = other.top - mode.dmPelsHeight;
        new_rect = rect_from_devmode( &mode );
    }

    /* if our adjustments caused more intersection, keep the original position */
    LIST_FOR_EACH_ENTRY( entry, &devices, struct device_entry, entry )
    {
        RECT other = rect_from_devmode( &entry->current );
        if (entry == device) continue;
        if (!IntersectRect( &tmp, &other, &new_rect )) continue;
        mode = device->pending;
    }

    device->pending = mode;
}

static void device_entry_snap_rect( struct device_entry *device )
{
    POINT offset = {LONG_MAX, LONG_MAX}, nearest = {LONG_MAX, LONG_MAX};
    DEVMODEW mode = device->pending;
    struct device_entry *entry;
    RECT new_rect, tmp;

    new_rect = rect_from_devmode( &mode );

    /* snap the position to the nearest rectangles */
    LIST_FOR_EACH_ENTRY( entry, &devices, struct device_entry, entry )
    {
        RECT other = rect_from_devmode( &entry->current );
        POINT diff = {LONG_MAX, LONG_MAX};

        if (entry == device) continue;

        if (new_rect.left >= other.right) diff.x = other.right - new_rect.left;
        if (new_rect.right <= other.left) diff.x = other.left - new_rect.right;
        if (new_rect.top >= other.bottom) diff.y = other.bottom - new_rect.top;
        if (new_rect.bottom <= other.top) diff.y = other.top - new_rect.bottom;
        if (abs( nearest.x ) > abs( diff.x ) && abs( nearest.y ) > abs( diff.y )) nearest = diff;

        SetRect( &tmp, LONG_MIN, new_rect.top, LONG_MAX, new_rect.bottom );
        if (IntersectRect( &tmp, &tmp, &other ))
        {
            diff.y = LONG_MAX;
            if (abs( offset.x ) > abs( diff.x ) && abs( offset.y ) > abs( diff.x )) offset = diff;
        }

        SetRect( &tmp, new_rect.left, LONG_MIN, new_rect.right, LONG_MAX );
        if (IntersectRect( &tmp, &tmp, &other ))
        {
            diff.x = LONG_MAX;
            if (abs( offset.y ) > abs( diff.y ) && abs( offset.x ) > abs( diff.y )) offset = diff;
        }
    }

    if (offset.x == LONG_MAX && offset.y == LONG_MAX) offset = nearest;
    if (offset.x != LONG_MAX) mode.dmPosition.x += offset.x;
    if (offset.y != LONG_MAX) mode.dmPosition.y += offset.y;

    /* if our adjustments caused more intersection, keep the original position */
    LIST_FOR_EACH_ENTRY( entry, &devices, struct device_entry, entry )
    {
        RECT other = rect_from_devmode( &entry->current );
        if (entry == device) continue;
        if (!IntersectRect( &tmp, &other, &new_rect )) continue;
        mode = device->pending;
    }

    device->current = mode;
    device->pending = mode;
}

static void handle_display_settings_change( HWND hwnd )
{
    DEVMODEW mode = {.dmSize = sizeof(mode)};
    struct device_entry *entry;
    int i;

    if (!selected) return;

    i = SendDlgItemMessageW( hwnd, IDC_DISPLAY_SETTINGS_LIST, CB_GETCURSEL, 0, 0 );
    if (i < 0) return;

    if (EnumDisplaySettingsExW( selected->adapter.DeviceName, i, &mode, 0 ))
    {
        mode.dmPosition = selected->current.dmPosition;
        mode.dmFields |= DM_POSITION;
        device_entry_move_rect( selected, mode );
        device_entry_snap_rect( selected );
    }

    LIST_FOR_EACH_ENTRY( entry, &devices, struct device_entry, entry )
        device_entry_snap_rect( entry );

    InvalidateRect( GetDlgItem( hwnd, IDC_VIRTUAL_DESKTOP ), NULL, TRUE );
}

static void handle_display_settings_apply(void)
{
    struct device_entry *entry;

    LIST_FOR_EACH_ENTRY( entry, &devices, struct device_entry, entry )
        ChangeDisplaySettingsExW( entry->adapter.DeviceName, &entry->current, 0, CDS_UPDATEREGISTRY | CDS_NORESET, NULL );
    ChangeDisplaySettingsExW( NULL, NULL, 0, 0, NULL );
}

static void handle_emulate_modeset_change( HWND hwnd )
{
    const WCHAR *value = L"N";
    HKEY hkey;

    /* Registry key can be found in HKCU\Software\Wine\X11 Driver */
    if (!RegCreateKeyExW( HKEY_CURRENT_USER, L"Software\\Wine\\X11 Driver", 0, NULL, 0,
                          KEY_SET_VALUE, NULL, &hkey, NULL ))
    {
        if (IsDlgButtonChecked( hwnd, IDC_EMULATE_MODESET ) == BST_CHECKED) value = L"Y";
        RegSetValueExW( hkey, L"EmulateModeset", 0, REG_SZ, (BYTE *)value, (wcslen( value ) + 1) * sizeof(WCHAR) );
        RegCloseKey( hkey );
    }
}

static BOOL get_option( const WCHAR *option, BOOL default_value )
{
    BOOL ret = default_value;
    WCHAR buffer[MAX_PATH];
    DWORD size = sizeof(buffer);

#define IS_OPTION_TRUE(ch) \
    ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')

    /* Registry key can be found in HKCU\Software\Wine\X11 Driver */
    if (!RegGetValueW( HKEY_CURRENT_USER, L"Software\\Wine\\X11 Driver", option, RRF_RT_REG_SZ, NULL,
                       (BYTE *)buffer, &size ))
        ret = IS_OPTION_TRUE(buffer[0]);

#undef IS_OPTION_TRUE
    return ret;
}

static RECT map_virtual_client_rect( RECT rect, RECT client_rect, RECT virtual_rect, float scale )
{
    OffsetRect( &rect, -(virtual_rect.left + virtual_rect.right) / 2, -(virtual_rect.top + virtual_rect.bottom) / 2 );
    rect.left *= scale;
    rect.right *= scale;
    rect.top *= scale;
    rect.bottom *= scale;
    OffsetRect( &rect, (client_rect.left + client_rect.right) / 2, (client_rect.top + client_rect.bottom) / 2 );
    return rect;
}

static void draw_monitor_rect( HDC hdc, struct device_entry *entry, RECT rect )
{
    HFONT font = SelectObject( hdc, GetStockObject( ANSI_VAR_FONT ) );

    SelectObject( hdc, GetStockObject( DC_BRUSH ) );
    SetDCBrushColor( hdc, GetSysColor( entry == selected ? COLOR_HIGHLIGHT : COLOR_WINDOW ) );
    SetDCPenColor( hdc, GetSysColor( entry->mouse_over ? COLOR_HIGHLIGHT : COLOR_WINDOWFRAME ) );
    Rectangle( hdc, rect.left, rect.top, rect.right, rect.bottom );

    DrawTextW( hdc, entry->adapter.DeviceName, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP );
    SelectObject( hdc, font );
}

static LRESULT CALLBACK desktop_view_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    HWND dialog = (HWND)GetWindowLongPtrW( hwnd, GWLP_USERDATA );

    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    if (msg == WM_PAINT)
    {
        RECT rect, client_rect, virtual_rect = {0};
        struct device_entry *entry;
        PAINTSTRUCT paint;
        float scale;
        HDC hdc;

        GetClientRect( hwnd, &client_rect );

        hdc = BeginPaint( hwnd, &paint );
        FillRect( hdc, &client_rect, (HBRUSH)(COLOR_WINDOW + 1) );
        SetDCPenColor( hdc, GetSysColor( COLOR_WINDOWFRAME ) );
        SelectObject( hdc, GetStockObject( GRAY_BRUSH ) );
        SelectObject( hdc, GetStockObject( DC_PEN ) );
        RoundRect( hdc, client_rect.left, client_rect.top, client_rect.right, client_rect.bottom, 5, 5 );

        LIST_FOR_EACH_ENTRY( entry, &devices, struct device_entry, entry )
        {
            rect = rect_from_devmode( &entry->pending );
            UnionRect( &virtual_rect, &virtual_rect, &rect );
        }
        scale = min( (client_rect.right - client_rect.left) / (float)(virtual_rect.right - virtual_rect.left),
                     (client_rect.bottom - client_rect.top) / (float)(virtual_rect.bottom - virtual_rect.top) );
        scale *= 0.95;

        rect = map_virtual_client_rect( virtual_rect, client_rect, virtual_rect, scale );
        SelectObject( hdc, GetStockObject( LTGRAY_BRUSH ) );
        Rectangle( hdc, rect.left, rect.top, rect.right, rect.bottom );
        scale *= 0.95;

        LIST_FOR_EACH_ENTRY( entry, &devices, struct device_entry, entry )
        {
            rect = rect_from_devmode( &entry->pending );
            rect = map_virtual_client_rect( rect, client_rect, virtual_rect, scale );
            draw_monitor_rect( hdc, entry, rect );
            entry->draw_rect = rect;
            entry->draw_scale = scale;
        }

        EndPaint( hwnd, &paint );
        return 0;
    }

    if (msg == WM_MOUSEMOVE)
    {
        POINT pt = {(short)LOWORD(lparam), (short)HIWORD(lparam)};
        struct device_entry *entry;
        BOOL changed = FALSE;

        LIST_FOR_EACH_ENTRY( entry, &devices, struct device_entry, entry )
        {
            POINT rel = {pt.x - entry->move_point.x, pt.y - entry->move_point.y};
            BOOL mouse_over = PtInRect( &entry->draw_rect, pt );

            if (entry->mouse_over != mouse_over) changed = TRUE;
            entry->mouse_over = mouse_over;

            if (!dragging && entry == selected && (wparam & MK_LBUTTON) &&
                max( abs( rel.x ), abs( rel.y ) ) >= 5)
                dragging = TRUE;

            if (entry == selected && dragging)
            {
                DEVMODEW mode = entry->current;
                mode.dmPosition.x += rel.x / entry->draw_scale;
                mode.dmPosition.y += rel.y / entry->draw_scale;
                device_entry_move_rect( entry, mode );
                SetCapture( hwnd );
                changed = TRUE;
            }
        }

        if (changed) InvalidateRect( hwnd, NULL, TRUE );
        return 0;
    }

    if (msg == WM_LBUTTONDOWN)
    {
        POINT pt = {(short)LOWORD(lparam), (short)HIWORD(lparam)};
        struct device_entry *entry;
        BOOL changed = FALSE;

        selected = NULL;
        dragging = FALSE;

        LIST_FOR_EACH_ENTRY( entry, &devices, struct device_entry, entry )
        {
            BOOL mouse_over = PtInRect( &entry->draw_rect, pt );
            if ((entry == selected) != mouse_over) changed = TRUE;
            if (mouse_over) selected = entry;
            entry->move_point = pt;
        }

        update_display_settings_list( dialog );
        if (changed) InvalidateRect( hwnd, NULL, TRUE );
        return 0;
    }

    if (msg == WM_LBUTTONUP)
    {
        struct device_entry *entry;
        SetCapture( 0 );

        if (selected) device_entry_snap_rect( selected );
        LIST_FOR_EACH_ENTRY( entry, &devices, struct device_entry, entry )
            device_entry_snap_rect( entry );

        dragging = FALSE;
        InvalidateRect( hwnd, NULL, TRUE );
        return 0;
    }

    if (msg == WM_RBUTTONDOWN)
    {
        SetCapture( 0 );
        refresh_device_list( hwnd );
        InvalidateRect( hwnd, NULL, TRUE );
        return 0;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void create_desktop_view( HWND hwnd )
{
    HWND parent, view;
    LONG margin;
    RECT rect;

    parent = GetDlgItem( hwnd, IDC_VIRTUAL_DESKTOP );
    GetClientRect( parent, &rect );
    rect.top += 6;

    margin = (rect.bottom - rect.top) * 5 / 100;
    InflateRect( &rect, -margin, -margin );

    view = CreateWindowW( L"DeskCplDesktop", NULL, WS_CHILD, rect.left, rect.top, rect.right - rect.left,
                          rect.bottom - rect.top, parent, NULL, NULL, module );
    SetWindowLongPtrW( view, GWLP_USERDATA, (UINT_PTR)hwnd );
    ShowWindow( view, SW_SHOW );
}

static INT_PTR CALLBACK desktop_dialog_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    switch (msg)
    {
    case WM_INITDIALOG:
        refresh_device_list( hwnd );
        create_desktop_view( hwnd );
        SendMessageW( GetDlgItem( hwnd, IDC_EMULATE_MODESET ), BM_SETCHECK,
                      get_option( L"EmulateModeset", FALSE ), 0 );
        return TRUE;

    case WM_COMMAND:
        switch (wparam)
        {
        case MAKEWPARAM( IDC_DISPLAY_SETTINGS_LIST, CBN_SELCHANGE ):
            handle_display_settings_change( hwnd );
            break;
        case IDC_EMULATE_MODESET:
            handle_emulate_modeset_change( hwnd );
            break;
        case IDC_DISPLAY_SETTINGS_APPLY:
            handle_display_settings_apply();
            break;
        case IDC_DISPLAY_SETTINGS_RESET:
            refresh_device_list( hwnd );
            InvalidateRect( hwnd, NULL, TRUE );
            break;
        }
        return TRUE;

    case WM_NOTIFY:
        return TRUE;
    }

    return FALSE;
}

static int CALLBACK property_sheet_callback( HWND hwnd, UINT msg, LPARAM lparam )
{
    TRACE( "hwnd %p, msg %#x, lparam %#Ix\n", hwnd, msg, lparam );
    return 0;
}

static void create_property_sheets( HWND parent )
{
    INITCOMMONCONTROLSEX init =
    {
        .dwSize = sizeof(INITCOMMONCONTROLSEX),
        .dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES,
    };
    PROPSHEETPAGEW pages[] =
    {
        {
            .dwSize = sizeof(PROPSHEETPAGEW),
            .hInstance = module,
            .pszTemplate = MAKEINTRESOURCEW( IDD_DESKTOP ),
            .pfnDlgProc = desktop_dialog_proc,
        },
    };
    PROPSHEETHEADERW header =
    {
        .dwSize = sizeof(PROPSHEETHEADERW),
        .dwFlags = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK,
        .hwndParent = parent,
        .hInstance = module,
        .pszCaption = MAKEINTRESOURCEW( IDS_CPL_NAME ),
        .nPages = ARRAY_SIZE(pages),
        .ppsp = pages,
        .pfnCallback = property_sheet_callback,
    };
    ACTCTXW context_desc =
    {
        .cbSize = sizeof(ACTCTXW),
        .hModule = module,
        .lpResourceName = MAKEINTRESOURCEW( 124 ),
        .dwFlags = ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID,
    };
    ULONG_PTR cookie;
    HANDLE context;
    BOOL activated;

    OleInitialize( NULL );

    context = CreateActCtxW( &context_desc );
    if (context == INVALID_HANDLE_VALUE) activated = FALSE;
    else activated = ActivateActCtx( context, &cookie );

    InitCommonControlsEx( &init );
    PropertySheetW( &header );

    if (activated) DeactivateActCtx( 0, cookie );
    ReleaseActCtx( context );
    OleUninitialize();
}

static void register_window_class(void)
{
    WNDCLASSW desktop_class =
    {
        .hInstance = module,
        .lpfnWndProc = desktop_view_proc,
        .lpszClassName = L"DeskCplDesktop",
    };
    RegisterClassW( &desktop_class );
}

static void unregister_window_class(void)
{
    UnregisterClassW( L"DeskCplDesktop", module );
}

/*********************************************************************
 * CPlApplet (desk.cpl.@)
 */
LONG CALLBACK CPlApplet( HWND hwnd, UINT command, LPARAM param1, LPARAM param2 )
{
    TRACE( "hwnd %p, command %u, param1 %#Ix, param2 %#Ix\n", hwnd, command, param1, param2 );

    switch (command)
    {
    case CPL_INIT:
        register_window_class();
        return TRUE;

    case CPL_GETCOUNT:
        return 1;

    case CPL_INQUIRE:
    {
        CPLINFO *info = (CPLINFO *)param2;
        info->idIcon = ICO_MAIN;
        info->idName = IDS_CPL_NAME;
        info->idInfo = IDS_CPL_INFO;
        info->lData = 0;
        return TRUE;
    }

    case CPL_DBLCLK:
        create_property_sheets( hwnd );
        break;

    case CPL_STOP:
        unregister_window_class();
        break;
    }

    return FALSE;
}

/*********************************************************************
 *  DllMain
 */
BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, LPVOID reserved )
{
    TRACE( "instance %p, reason %ld, reserved %p\n", instance, reason, reserved );

    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls( instance );
        module = instance;
    }

    return TRUE;
}
