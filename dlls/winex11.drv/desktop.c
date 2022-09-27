/*
 * X11DRV desktop window handling
 *
 * Copyright 2001 Alexandre Julliard
 * Copyright 2020 Zhiyi Zhang for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include <X11/cursorfont.h>
#include <X11/Xlib.h>

#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include "x11drv.h"

/* avoid conflict with field names in included win32 headers */
#undef Status
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

static unsigned int max_width;
static unsigned int max_height;
static unsigned int desktop_width;
static unsigned int desktop_height;

static struct screen_size {
    unsigned int width;
    unsigned int height;
} screen_sizes[] = {
    /* 4:3 */
    { 320,  240},
    { 400,  300},
    { 512,  384},
    { 640,  480},
    { 768,  576},
    { 800,  600},
    {1024,  768},
    {1152,  864},
    {1280,  960},
    {1400, 1050},
    {1600, 1200},
    {2048, 1536},
    /* 5:4 */
    {1280, 1024},
    {2560, 2048},
    /* 16:9 */
    {1280,  720},
    {1366,  768},
    {1600,  900},
    {1920, 1080},
    {2560, 1440},
    {3840, 2160},
    /* 16:10 */
    { 320,  200},
    { 640,  400},
    {1280,  800},
    {1440,  900},
    {1680, 1050},
    {1920, 1200},
    {2560, 1600}
};

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1

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

/* retrieve the default desktop size from the registry */
static BOOL get_default_desktop_size( unsigned int *width, unsigned int *height )
{
    static const WCHAR defaultW[] = {'D','e','f','a','u','l','t',0};
    WCHAR buffer[4096];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    DWORD size;
    HKEY hkey;

    /* @@ Wine registry key: HKCU\Software\Wine\Explorer\Desktops */
    if (!(hkey = open_hkcu_key( "Software\\Wine\\Explorer\\Desktops" ))) return FALSE;

    size = query_reg_value( hkey, defaultW, value, sizeof(buffer) );
    NtClose( hkey );
    if (!size || value->Type != REG_SZ) return FALSE;

    if (!parse_size( (const WCHAR *)value->Data, width, height )) return FALSE;
    return TRUE;
}

/* Return TRUE if Wine is currently in virtual desktop mode */
BOOL is_virtual_desktop(void)
{
    return root_window != DefaultRootWindow( gdi_display );
}

/* Virtual desktop display settings handler */
static BOOL X11DRV_desktop_get_id( const WCHAR *device_name, ULONG_PTR *id )
{
    WCHAR primary_adapter[CCHDEVICENAME];

    if (!get_primary_adapter( primary_adapter ) || wcsicmp( primary_adapter, device_name ))
        return FALSE;

    *id = 0;
    return TRUE;
}

static void add_desktop_mode( DEVMODEW *mode, DWORD depth, DWORD width, DWORD height )
{
    mode->dmSize = sizeof(*mode);
    mode->dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                     DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY;
    mode->u1.s2.dmDisplayOrientation = DMDO_DEFAULT;
    mode->dmBitsPerPel = depth;
    mode->dmPelsWidth = width;
    mode->dmPelsHeight = height;
    mode->u2.dmDisplayFlags = 0;
    mode->dmDisplayFrequency = 60;
}

static BOOL X11DRV_desktop_get_modes( ULONG_PTR id, DWORD flags, DEVMODEW **new_modes, UINT *mode_count )
{
    UINT depth_idx, size_idx, mode_idx = 0;
    UINT screen_width, screen_height;
    DEVMODEW *modes;

    if (!get_default_desktop_size( &screen_width, &screen_height ))
    {
        screen_width = max_width;
        screen_height = max_height;
    }

    /* Allocate memory for modes in different color depths */
    if (!(modes = calloc( (ARRAY_SIZE(screen_sizes) + 2) * DEPTH_COUNT, sizeof(*modes))) )
    {
        RtlSetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    for (depth_idx = 0; depth_idx < DEPTH_COUNT; ++depth_idx)
    {
        for (size_idx = 0; size_idx < ARRAY_SIZE(screen_sizes); ++size_idx)
        {
            if (screen_sizes[size_idx].width > max_width ||
                screen_sizes[size_idx].height > max_height)
                continue;

            if (screen_sizes[size_idx].width == max_width &&
                screen_sizes[size_idx].height == max_height)
                continue;

            if (screen_sizes[size_idx].width == screen_width &&
                screen_sizes[size_idx].height == screen_height)
                continue;

            add_desktop_mode( &modes[mode_idx++], depths[depth_idx], screen_sizes[size_idx].width,
                              screen_sizes[size_idx].height );
        }

        add_desktop_mode( &modes[mode_idx++], depths[depth_idx], screen_width, screen_height );
        if (max_width != screen_width || max_height != screen_height)
            add_desktop_mode( &modes[mode_idx++], depths[depth_idx], max_width, max_height );
    }

    *new_modes = modes;
    *mode_count = mode_idx;
    return TRUE;
}

static void X11DRV_desktop_free_modes( DEVMODEW *modes )
{
    free( modes );
}

static BOOL X11DRV_desktop_get_current_mode( ULONG_PTR id, DEVMODEW *mode )
{
    RECT primary_rect = NtUserGetPrimaryMonitorRect();

    mode->dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                     DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY | DM_POSITION;
    mode->u1.s2.dmDisplayOrientation = DMDO_DEFAULT;
    mode->dmBitsPerPel = screen_bpp;
    mode->dmPelsWidth = primary_rect.right - primary_rect.left;
    mode->dmPelsHeight = primary_rect.bottom - primary_rect.top;
    mode->u2.dmDisplayFlags = 0;
    mode->dmDisplayFrequency = 60;
    mode->u1.s2.dmPosition.x = 0;
    mode->u1.s2.dmPosition.y = 0;
    return TRUE;
}

static LONG X11DRV_desktop_set_current_mode( ULONG_PTR id, const DEVMODEW *mode )
{
    if (mode->dmFields & DM_BITSPERPEL && mode->dmBitsPerPel != screen_bpp)
        WARN("Cannot change screen color depth from %dbits to %dbits!\n", screen_bpp, mode->dmBitsPerPel);

    desktop_width = mode->dmPelsWidth;
    desktop_height = mode->dmPelsHeight;
    return DISP_CHANGE_SUCCESSFUL;
}

static void query_desktop_work_area( RECT *rc_work )
{
    static const WCHAR trayW[] = {'S','h','e','l','l','_','T','r','a','y','W','n','d'};
    UNICODE_STRING str = { sizeof(trayW), sizeof(trayW), (WCHAR *)trayW };
    RECT rect;
    HWND hwnd = NtUserFindWindowEx( 0, 0, &str, NULL, 0 );

    if (!hwnd || !NtUserIsWindowVisible( hwnd )) return;
    if (!NtUserGetWindowRect( hwnd, &rect )) return;
    if (rect.top) rc_work->bottom = rect.top;
    else rc_work->top = rect.bottom;
    TRACE( "found tray %p %s work area %s\n", hwnd, wine_dbgstr_rect( &rect ), wine_dbgstr_rect( rc_work ) );
}

static BOOL X11DRV_desktop_get_gpus( struct gdi_gpu **new_gpus, int *count )
{
    static const WCHAR wine_adapterW[] = {'W','i','n','e',' ','A','d','a','p','t','e','r',0};
    struct gdi_gpu *gpu;

    gpu = calloc( 1, sizeof(*gpu) );
    if (!gpu) return FALSE;

    if (!get_host_primary_gpu( gpu ))
    {
        WARN( "Failed to get host primary gpu.\n" );
        lstrcpyW( gpu->name, wine_adapterW );
    }

    *new_gpus = gpu;
    *count = 1;
    return TRUE;
}

static void X11DRV_desktop_free_gpus( struct gdi_gpu *gpus )
{
    free( gpus );
}

/* TODO: Support multi-head virtual desktop */
static BOOL X11DRV_desktop_get_adapters( ULONG_PTR gpu_id, struct gdi_adapter **new_adapters, int *count )
{
    struct gdi_adapter *adapter;

    adapter = calloc( 1, sizeof(*adapter) );
    if (!adapter) return FALSE;

    adapter->state_flags = DISPLAY_DEVICE_PRIMARY_DEVICE;
    if (desktop_width && desktop_height)
        adapter->state_flags |= DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;

    *new_adapters = adapter;
    *count = 1;
    return TRUE;
}

static void X11DRV_desktop_free_adapters( struct gdi_adapter *adapters )
{
    free( adapters );
}

static BOOL X11DRV_desktop_get_monitors( ULONG_PTR adapter_id, struct gdi_monitor **new_monitors, int *count )
{
    static const WCHAR generic_nonpnp_monitorW[] = {
        'G','e','n','e','r','i','c',' ',
        'N','o','n','-','P','n','P',' ','M','o','n','i','t','o','r',0};
    struct gdi_monitor *monitor;

    monitor = calloc( 1, sizeof(*monitor) );
    if (!monitor) return FALSE;

    lstrcpyW( monitor->name, generic_nonpnp_monitorW );
    SetRect( &monitor->rc_monitor, 0, 0, desktop_width, desktop_height );
    SetRect( &monitor->rc_work, 0, 0, desktop_width, desktop_height );
    query_desktop_work_area( &monitor->rc_work );
    monitor->state_flags = DISPLAY_DEVICE_ATTACHED;
    monitor->edid_len = 0;
    monitor->edid = NULL;
    if (desktop_width && desktop_height)
        monitor->state_flags |= DISPLAY_DEVICE_ACTIVE;

    *new_monitors = monitor;
    *count = 1;
    return TRUE;
}

static void X11DRV_desktop_free_monitors( struct gdi_monitor *monitors, int count )
{
    free( monitors );
}

/***********************************************************************
 *		X11DRV_init_desktop
 *
 * Setup the desktop when not using the root window.
 */
void X11DRV_init_desktop( Window win, unsigned int width, unsigned int height )
{
    RECT primary_rect = get_host_primary_monitor_rect();
    struct x11drv_settings_handler settings_handler;

    root_window = win;
    managed_mode = FALSE;  /* no managed windows in desktop mode */
    desktop_width = width;
    desktop_height = height;
    max_width = primary_rect.right;
    max_height = primary_rect.bottom;

    /* Initialize virtual desktop display settings handler */
    settings_handler.name = "Virtual Desktop";
    settings_handler.priority = 1000;
    settings_handler.get_id = X11DRV_desktop_get_id;
    settings_handler.get_modes = X11DRV_desktop_get_modes;
    settings_handler.free_modes = X11DRV_desktop_free_modes;
    settings_handler.get_current_mode = X11DRV_desktop_get_current_mode;
    settings_handler.set_current_mode = X11DRV_desktop_set_current_mode;
    X11DRV_Settings_SetHandler( &settings_handler );

    /* Initialize virtual desktop mode display device handler */
    desktop_handler.name = "Virtual Desktop";
    desktop_handler.get_gpus = X11DRV_desktop_get_gpus;
    desktop_handler.get_adapters = X11DRV_desktop_get_adapters;
    desktop_handler.get_monitors = X11DRV_desktop_get_monitors;
    desktop_handler.free_gpus = X11DRV_desktop_free_gpus;
    desktop_handler.free_adapters = X11DRV_desktop_free_adapters;
    desktop_handler.free_monitors = X11DRV_desktop_free_monitors;
    desktop_handler.register_event_handlers = NULL;
    TRACE("Display device functions are now handled by: Virtual Desktop\n");
    X11DRV_DisplayDevices_Init( TRUE );
}


/***********************************************************************
 *           x11drv_create_desktop
 *
 * Create the X11 desktop window for the desktop mode.
 */
NTSTATUS x11drv_create_desktop( void *arg )
{
    static const WCHAR rootW[] = {'r','o','o','t',0};
    const struct create_desktop_params *params = arg;
    XSetWindowAttributes win_attr;
    Window win;
    Display *display = thread_init_display();
    WCHAR name[MAX_PATH];

    if (!NtUserGetObjectInformation( NtUserGetThreadDesktop( GetCurrentThreadId() ),
                                     UOI_NAME, name, sizeof(name), NULL ))
        name[0] = 0;

    TRACE( "%s %ux%u\n", debugstr_w(name), params->width, params->height );

    /* magic: desktop "root" means use the root window */
    if (!wcsicmp( name, rootW )) return FALSE;

    /* Create window */
    win_attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | EnterWindowMask |
                          PointerMotionMask | ButtonPressMask | ButtonReleaseMask | FocusChangeMask;
    win_attr.cursor = XCreateFontCursor( display, XC_top_left_arrow );

    if (default_visual.visual != DefaultVisual( display, DefaultScreen(display) ))
        win_attr.colormap = XCreateColormap( display, DefaultRootWindow(display),
                                             default_visual.visual, AllocNone );
    else
        win_attr.colormap = None;

    win = XCreateWindow( display, DefaultRootWindow(display),
                         0, 0, params->width, params->height, 0, default_visual.depth, InputOutput,
                         default_visual.visual, CWEventMask | CWCursor | CWColormap, &win_attr );
    if (!win) return FALSE;
    if (!create_desktop_win_data( win )) return FALSE;

    X11DRV_init_desktop( win, params->width, params->height );
    if (is_desktop_fullscreen())
    {
        TRACE("setting desktop to fullscreen\n");
        XChangeProperty( display, win, x11drv_atom(_NET_WM_STATE), XA_ATOM, 32,
            PropModeReplace, (unsigned char*)&x11drv_atom(_NET_WM_STATE_FULLSCREEN),
            1);
    }
    XFlush( display );
    return TRUE;
}

BOOL is_desktop_fullscreen(void)
{
    RECT primary_rect = NtUserGetPrimaryMonitorRect();
    return (primary_rect.right - primary_rect.left == max_width &&
            primary_rect.bottom - primary_rect.top == max_height);
}

static void update_desktop_fullscreen( unsigned int width, unsigned int height)
{
    Display *display = thread_display();
    XEvent xev;

    if (!display || !is_virtual_desktop()) return;

    xev.xclient.type = ClientMessage;
    xev.xclient.window = root_window;
    xev.xclient.message_type = x11drv_atom(_NET_WM_STATE);
    xev.xclient.serial = 0;
    xev.xclient.display = display;
    xev.xclient.send_event = True;
    xev.xclient.format = 32;
    if (width == max_width && height == max_height)
        xev.xclient.data.l[0] = _NET_WM_STATE_ADD;
    else
        xev.xclient.data.l[0] = _NET_WM_STATE_REMOVE;
    xev.xclient.data.l[1] = x11drv_atom(_NET_WM_STATE_FULLSCREEN);
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 1;

    TRACE("action=%li\n", xev.xclient.data.l[0]);

    XSendEvent( display, DefaultRootWindow(display), False,
                SubstructureRedirectMask | SubstructureNotifyMask, &xev );

    xev.xclient.data.l[1] = x11drv_atom(_NET_WM_STATE_MAXIMIZED_VERT);
    xev.xclient.data.l[2] = x11drv_atom(_NET_WM_STATE_MAXIMIZED_HORZ);
    XSendEvent( display, DefaultRootWindow(display), False,
                SubstructureRedirectMask | SubstructureNotifyMask, &xev );
}

/***********************************************************************
 *		X11DRV_resize_desktop
 */
void X11DRV_resize_desktop(void)
{
    static RECT old_virtual_rect;

    RECT primary_rect, virtual_rect;
    HWND hwnd = NtUserGetDesktopWindow();
    INT width, height;

    virtual_rect = NtUserGetVirtualScreenRect();
    primary_rect = NtUserGetPrimaryMonitorRect();
    width = primary_rect.right;
    height = primary_rect.bottom;

    TRACE( "desktop %p change to (%dx%d)\n", hwnd, width, height );
    update_desktop_fullscreen( width, height );
    NtUserSetWindowPos( hwnd, 0, virtual_rect.left, virtual_rect.top,
                        virtual_rect.right - virtual_rect.left, virtual_rect.bottom - virtual_rect.top,
                        SWP_NOZORDER | SWP_NOACTIVATE | SWP_DEFERERASE );
    ungrab_clipping_window();
    send_message_timeout( HWND_BROADCAST, WM_X11DRV_DESKTOP_RESIZED, old_virtual_rect.left,
                          old_virtual_rect.top, SMTO_ABORTIFHUNG, 2000, FALSE );

    /* forward clip_fullscreen_window request to the foreground window */
    send_notify_message( NtUserGetForegroundWindow(), WM_X11DRV_CLIP_CURSOR_REQUEST, TRUE, TRUE );

    old_virtual_rect = virtual_rect;
}
