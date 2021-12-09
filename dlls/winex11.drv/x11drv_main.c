/*
 * X11DRV initialization code
 *
 * Copyright 1998 Patrik Stridvall
 * Copyright 2000 Alexandre Julliard
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

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#ifdef HAVE_XKB
#include <X11/XKBlib.h>
#endif
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
#include <X11/extensions/Xrender.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"

#include "x11drv.h"
#include "xcomposite.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);
WINE_DECLARE_DEBUG_CHANNEL(synchronous);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

XVisualInfo default_visual = { 0 };
XVisualInfo argb_visual = { 0 };
Colormap default_colormap = None;
XPixmapFormatValues **pixmap_formats;
unsigned int screen_bpp;
Window root_window;
BOOL usexvidmode = TRUE;
BOOL usexrandr = TRUE;
BOOL usexcomposite = TRUE;
BOOL use_xkb = TRUE;
BOOL use_take_focus = TRUE;
BOOL use_primary_selection = FALSE;
BOOL use_system_cursors = TRUE;
BOOL show_systray = TRUE;
BOOL grab_pointer = TRUE;
BOOL grab_fullscreen = FALSE;
BOOL managed_mode = TRUE;
BOOL decorated_mode = TRUE;
BOOL private_color_map = FALSE;
int primary_monitor = 0;
BOOL client_side_graphics = TRUE;
BOOL client_side_with_render = TRUE;
BOOL shape_layered_windows = TRUE;
int copy_default_colors = 128;
int alloc_system_colors = 256;
DWORD thread_data_tls_index = TLS_OUT_OF_INDEXES;
int xrender_error_base = 0;
HMODULE x11drv_module = 0;
char *process_name = NULL;

static x11drv_error_callback err_callback;   /* current callback for error */
static Display *err_callback_display;        /* display callback is set for */
static void *err_callback_arg;               /* error callback argument */
static int err_callback_result;              /* error callback result */
static unsigned long err_serial;             /* serial number of first request */
static int (*old_error_handler)( Display *, XErrorEvent * );
static BOOL use_xim = TRUE;
static char input_style[20];

static CRITICAL_SECTION x11drv_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &x11drv_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": x11drv_section") }
};
static CRITICAL_SECTION x11drv_section = { &critsect_debug, -1, 0, 0, 0, 0 };

static CRITICAL_SECTION x11drv_error_section;
static CRITICAL_SECTION_DEBUG x11drv_error_section_debug =
{
    0, 0, &x11drv_error_section,
    { &x11drv_error_section_debug.ProcessLocksList, &x11drv_error_section_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": x11drv_error_section") }
};
static CRITICAL_SECTION x11drv_error_section = { &x11drv_error_section_debug, -1, 0, 0, 0, 0 };

struct d3dkmt_vidpn_source
{
    D3DKMT_VIDPNSOURCEOWNER_TYPE type;      /* VidPN source owner type */
    D3DDDI_VIDEO_PRESENT_SOURCE_ID id;      /* VidPN present source id */
    D3DKMT_HANDLE device;                   /* Kernel mode device context */
    struct list entry;                      /* List entry */
};

static struct list d3dkmt_vidpn_sources = LIST_INIT( d3dkmt_vidpn_sources );   /* VidPN source information list */

#define IS_OPTION_TRUE(ch) \
    ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')
#define IS_OPTION_FALSE(ch) \
    ((ch) == 'n' || (ch) == 'N' || (ch) == 'f' || (ch) == 'F' || (ch) == '0')

Atom X11DRV_Atoms[NB_XATOMS - FIRST_XATOM];

static const char * const atom_names[NB_XATOMS - FIRST_XATOM] =
{
    "CLIPBOARD",
    "COMPOUND_TEXT",
    "EDID",
    "INCR",
    "MANAGER",
    "MULTIPLE",
    "SELECTION_DATA",
    "TARGETS",
    "TEXT",
    "TIMESTAMP",
    "UTF8_STRING",
    "RAW_ASCENT",
    "RAW_DESCENT",
    "RAW_CAP_HEIGHT",
    "Rel X",
    "Rel Y",
    "WM_PROTOCOLS",
    "WM_DELETE_WINDOW",
    "WM_STATE",
    "WM_TAKE_FOCUS",
    "DndProtocol",
    "DndSelection",
    "_ICC_PROFILE",
    "_MOTIF_WM_HINTS",
    "_NET_STARTUP_INFO_BEGIN",
    "_NET_STARTUP_INFO",
    "_NET_SUPPORTED",
    "_NET_SYSTEM_TRAY_OPCODE",
    "_NET_SYSTEM_TRAY_S0",
    "_NET_SYSTEM_TRAY_VISUAL",
    "_NET_WM_ICON",
    "_NET_WM_MOVERESIZE",
    "_NET_WM_NAME",
    "_NET_WM_PID",
    "_NET_WM_PING",
    "_NET_WM_STATE",
    "_NET_WM_STATE_ABOVE",
    "_NET_WM_STATE_DEMANDS_ATTENTION",
    "_NET_WM_STATE_FULLSCREEN",
    "_NET_WM_STATE_MAXIMIZED_HORZ",
    "_NET_WM_STATE_MAXIMIZED_VERT",
    "_NET_WM_STATE_SKIP_PAGER",
    "_NET_WM_STATE_SKIP_TASKBAR",
    "_NET_WM_USER_TIME",
    "_NET_WM_USER_TIME_WINDOW",
    "_NET_WM_WINDOW_OPACITY",
    "_NET_WM_WINDOW_TYPE",
    "_NET_WM_WINDOW_TYPE_DIALOG",
    "_NET_WM_WINDOW_TYPE_NORMAL",
    "_NET_WM_WINDOW_TYPE_UTILITY",
    "_NET_WORKAREA",
    "_GTK_WORKAREAS_D0",
    "_XEMBED",
    "_XEMBED_INFO",
    "XdndAware",
    "XdndEnter",
    "XdndPosition",
    "XdndStatus",
    "XdndLeave",
    "XdndFinished",
    "XdndDrop",
    "XdndActionCopy",
    "XdndActionMove",
    "XdndActionLink",
    "XdndActionAsk",
    "XdndActionPrivate",
    "XdndSelection",
    "XdndTypeList",
    "HTML Format",
    "WCF_DIF",
    "WCF_ENHMETAFILE",
    "WCF_HDROP",
    "WCF_PENDATA",
    "WCF_RIFF",
    "WCF_SYLK",
    "WCF_TIFF",
    "WCF_WAVE",
    "image/bmp",
    "image/gif",
    "image/jpeg",
    "image/png",
    "text/html",
    "text/plain",
    "text/rtf",
    "text/richtext",
    "text/uri-list"
};

/***********************************************************************
 *		ignore_error
 *
 * Check if the X error is one we can ignore.
 */
static inline BOOL ignore_error( Display *display, XErrorEvent *event )
{
    if ((event->request_code == X_SetInputFocus ||
         event->request_code == X_ChangeWindowAttributes ||
         event->request_code == X_SendEvent) &&
        (event->error_code == BadMatch ||
         event->error_code == BadWindow)) return TRUE;

    /* the clipboard display interacts with external windows, ignore all errors */
    if (display == clipboard_display) return TRUE;

    /* ignore a number of errors on gdi display caused by creating/destroying windows */
    if (display == gdi_display)
    {
        if (event->error_code == BadDrawable ||
            event->error_code == BadGC ||
            event->error_code == BadWindow)
            return TRUE;
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
        if (xrender_error_base)  /* check for XRender errors */
        {
            if (event->error_code == xrender_error_base + BadPicture) return TRUE;
        }
#endif
    }
    return FALSE;
}


/***********************************************************************
 *		X11DRV_expect_error
 *
 * Setup a callback function that will be called on an X error.  The
 * callback must return non-zero if the error is the one it expected.
 */
void X11DRV_expect_error( Display *display, x11drv_error_callback callback, void *arg )
{
    EnterCriticalSection( &x11drv_error_section );
    err_callback         = callback;
    err_callback_display = display;
    err_callback_arg     = arg;
    err_callback_result  = 0;
    err_serial           = NextRequest(display);
}


/***********************************************************************
 *		X11DRV_check_error
 *
 * Check if an expected X11 error occurred; return non-zero if yes.
 * The caller is responsible for calling XSync first if necessary.
 */
int X11DRV_check_error(void)
{
    int res = err_callback_result;
    err_callback = NULL;
    LeaveCriticalSection( &x11drv_error_section );
    return res;
}


/***********************************************************************
 *		error_handler
 */
static int error_handler( Display *display, XErrorEvent *error_evt )
{
    if (err_callback && display == err_callback_display &&
        (long)(error_evt->serial - err_serial) >= 0)
    {
        if ((err_callback_result = err_callback( display, error_evt, err_callback_arg )))
        {
            TRACE( "got expected error %d req %d\n",
                   error_evt->error_code, error_evt->request_code );
            return 0;
        }
    }
    if (ignore_error( display, error_evt ))
    {
        TRACE( "got ignored error %d req %d\n",
               error_evt->error_code, error_evt->request_code );
        return 0;
    }
    if (TRACE_ON(synchronous))
    {
        ERR( "X protocol error: serial=%ld, request_code=%d - breaking into debugger\n",
             error_evt->serial, error_evt->request_code );
        DebugBreak();  /* force an entry in the debugger */
    }
    old_error_handler( display, error_evt );
    return 0;
}

/***********************************************************************
 *		init_pixmap_formats
 */
static void init_pixmap_formats( Display *display )
{
    int i, count, max = 32;
    XPixmapFormatValues *formats = XListPixmapFormats( display, &count );

    for (i = 0; i < count; i++)
    {
        TRACE( "depth %u, bpp %u, pad %u\n",
               formats[i].depth, formats[i].bits_per_pixel, formats[i].scanline_pad );
        if (formats[i].depth > max) max = formats[i].depth;
    }
    pixmap_formats = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pixmap_formats) * (max + 1) );
    for (i = 0; i < count; i++) pixmap_formats[formats[i].depth] = &formats[i];
}


/***********************************************************************
 *		get_config_key
 *
 * Get a config key from either the app-specific or the default config
 */
static inline DWORD get_config_key( HKEY defkey, HKEY appkey, const char *name,
                                    char *buffer, DWORD size )
{
    if (appkey && !RegQueryValueExA( appkey, name, 0, NULL, (LPBYTE)buffer, &size )) return 0;
    if (defkey && !RegQueryValueExA( defkey, name, 0, NULL, (LPBYTE)buffer, &size )) return 0;
    return ERROR_FILE_NOT_FOUND;
}


/***********************************************************************
 *		setup_options
 *
 * Setup the x11drv options.
 */
static void setup_options(void)
{
    static const WCHAR x11driverW[] = {'\\','X','1','1',' ','D','r','i','v','e','r',0};
    char buffer[64];
    WCHAR bufferW[MAX_PATH+16];
    HKEY hkey, appkey = 0;
    DWORD len;

    /* @@ Wine registry key: HKCU\Software\Wine\X11 Driver */
    if (RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\X11 Driver", &hkey )) hkey = 0;

    /* open the app-specific key */

    len = (GetModuleFileNameW( 0, bufferW, MAX_PATH ));
    if (len && len < MAX_PATH)
    {
        HKEY tmpkey;
        WCHAR *p, *appname = bufferW;
        if ((p = strrchrW( appname, '/' ))) appname = p + 1;
        if ((p = strrchrW( appname, '\\' ))) appname = p + 1;
        CharLowerW(appname);
        len = WideCharToMultiByte( CP_UNIXCP, 0, appname, -1, NULL, 0, NULL, NULL );
        if ((process_name = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_UNIXCP, 0, appname, -1, process_name, len, NULL, NULL );
        strcatW( appname, x11driverW );
        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\X11 Driver */
        if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\AppDefaults", &tmpkey ))
        {
            if (RegOpenKeyW( tmpkey, appname, &appkey )) appkey = 0;
            RegCloseKey( tmpkey );
        }
    }

    if (!get_config_key( hkey, appkey, "Managed", buffer, sizeof(buffer) ))
        managed_mode = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "Decorated", buffer, sizeof(buffer) ))
        decorated_mode = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UseXVidMode", buffer, sizeof(buffer) ))
        usexvidmode = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UseXRandR", buffer, sizeof(buffer) ))
        usexrandr = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UseTakeFocus", buffer, sizeof(buffer) ))
        use_take_focus = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UsePrimarySelection", buffer, sizeof(buffer) ))
        use_primary_selection = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UseSystemCursors", buffer, sizeof(buffer) ))
        use_system_cursors = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "ShowSystray", buffer, sizeof(buffer) ))
        show_systray = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "GrabPointer", buffer, sizeof(buffer) ))
        grab_pointer = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "GrabFullscreen", buffer, sizeof(buffer) ))
        grab_fullscreen = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "ScreenDepth", buffer, sizeof(buffer) ))
        default_visual.depth = atoi(buffer);

    if (!get_config_key( hkey, appkey, "ClientSideGraphics", buffer, sizeof(buffer) ))
        client_side_graphics = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "ClientSideWithRender", buffer, sizeof(buffer) ))
        client_side_with_render = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UseXIM", buffer, sizeof(buffer) ))
        use_xim = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "ShapeLayeredWindows", buffer, sizeof(buffer) ))
        shape_layered_windows = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "PrivateColorMap", buffer, sizeof(buffer) ))
        private_color_map = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "PrimaryMonitor", buffer, sizeof(buffer) ))
        primary_monitor = atoi( buffer );

    if (!get_config_key( hkey, appkey, "CopyDefaultColors", buffer, sizeof(buffer) ))
        copy_default_colors = atoi(buffer);

    if (!get_config_key( hkey, appkey, "AllocSystemColors", buffer, sizeof(buffer) ))
        alloc_system_colors = atoi(buffer);

    get_config_key( hkey, appkey, "InputStyle", input_style, sizeof(input_style) );

    if (appkey) RegCloseKey( appkey );
    if (hkey) RegCloseKey( hkey );
}

#ifdef SONAME_LIBXCOMPOSITE

#define MAKE_FUNCPTR(f) typeof(f) * p##f;
MAKE_FUNCPTR(XCompositeQueryExtension)
MAKE_FUNCPTR(XCompositeQueryVersion)
MAKE_FUNCPTR(XCompositeVersion)
MAKE_FUNCPTR(XCompositeRedirectWindow)
MAKE_FUNCPTR(XCompositeRedirectSubwindows)
MAKE_FUNCPTR(XCompositeUnredirectWindow)
MAKE_FUNCPTR(XCompositeUnredirectSubwindows)
MAKE_FUNCPTR(XCompositeCreateRegionFromBorderClip)
MAKE_FUNCPTR(XCompositeNameWindowPixmap)
#undef MAKE_FUNCPTR

static int xcomp_event_base;
static int xcomp_error_base;

static void X11DRV_XComposite_Init(void)
{
    void *xcomposite_handle = dlopen(SONAME_LIBXCOMPOSITE, RTLD_NOW);
    if (!xcomposite_handle)
    {
        TRACE("Unable to open %s, XComposite disabled\n", SONAME_LIBXCOMPOSITE);
        usexcomposite = FALSE;
        return;
    }

#define LOAD_FUNCPTR(f) \
    if((p##f = dlsym(xcomposite_handle, #f)) == NULL) goto sym_not_found
    LOAD_FUNCPTR(XCompositeQueryExtension);
    LOAD_FUNCPTR(XCompositeQueryVersion);
    LOAD_FUNCPTR(XCompositeVersion);
    LOAD_FUNCPTR(XCompositeRedirectWindow);
    LOAD_FUNCPTR(XCompositeRedirectSubwindows);
    LOAD_FUNCPTR(XCompositeUnredirectWindow);
    LOAD_FUNCPTR(XCompositeUnredirectSubwindows);
    LOAD_FUNCPTR(XCompositeCreateRegionFromBorderClip);
    LOAD_FUNCPTR(XCompositeNameWindowPixmap);
#undef LOAD_FUNCPTR

    if(!pXCompositeQueryExtension(gdi_display, &xcomp_event_base,
                                  &xcomp_error_base)) {
        TRACE("XComposite extension could not be queried; disabled\n");
        dlclose(xcomposite_handle);
        xcomposite_handle = NULL;
        usexcomposite = FALSE;
        return;
    }
    TRACE("XComposite is up and running error_base = %d\n", xcomp_error_base);
    return;

sym_not_found:
    TRACE("Unable to load function pointers from %s, XComposite disabled\n", SONAME_LIBXCOMPOSITE);
    dlclose(xcomposite_handle);
    xcomposite_handle = NULL;
    usexcomposite = FALSE;
}
#endif /* defined(SONAME_LIBXCOMPOSITE) */

static void init_visuals( Display *display, int screen )
{
    int count;
    XVisualInfo *info;

    argb_visual.screen     = screen;
    argb_visual.class      = TrueColor;
    argb_visual.depth      = 32;
    argb_visual.red_mask   = 0xff0000;
    argb_visual.green_mask = 0x00ff00;
    argb_visual.blue_mask  = 0x0000ff;

    if ((info = XGetVisualInfo( display, VisualScreenMask | VisualDepthMask | VisualClassMask |
                                VisualRedMaskMask | VisualGreenMaskMask | VisualBlueMaskMask,
                                &argb_visual, &count )))
    {
        argb_visual = *info;
        XFree( info );
    }

    default_visual.screen = screen;
    if (default_visual.depth)  /* depth specified */
    {
        if (default_visual.depth == 32 && argb_visual.visual)
        {
            default_visual = argb_visual;
        }
        else if ((info = XGetVisualInfo( display, VisualScreenMask | VisualDepthMask, &default_visual, &count )))
        {
            default_visual = *info;
            XFree( info );
        }
        else WARN( "no visual found for depth %d\n", default_visual.depth );
    }

    if (!default_visual.visual)
    {
        default_visual.depth         = DefaultDepth( display, screen );
        default_visual.visual        = DefaultVisual( display, screen );
        default_visual.visualid      = default_visual.visual->visualid;
        default_visual.class         = default_visual.visual->class;
        default_visual.red_mask      = default_visual.visual->red_mask;
        default_visual.green_mask    = default_visual.visual->green_mask;
        default_visual.blue_mask     = default_visual.visual->blue_mask;
        default_visual.colormap_size = default_visual.visual->map_entries;
        default_visual.bits_per_rgb  = default_visual.visual->bits_per_rgb;
    }
    default_colormap = XCreateColormap( display, root_window, default_visual.visual, AllocNone );

    TRACE( "default visual %lx class %u argb %lx\n",
           default_visual.visualid, default_visual.class, argb_visual.visualid );
}

/***********************************************************************
 *           X11DRV process initialisation routine
 */
static BOOL process_attach(void)
{
    Display *display;
    void *libx11 = dlopen( SONAME_LIBX11, RTLD_NOW|RTLD_GLOBAL );

    if (!libx11)
    {
        ERR( "failed to load %s: %s\n", SONAME_LIBX11, dlerror() );
        return FALSE;
    }
    pXGetEventData = dlsym( libx11, "XGetEventData" );
    pXFreeEventData = dlsym( libx11, "XFreeEventData" );
#ifdef SONAME_LIBXEXT
    dlopen( SONAME_LIBXEXT, RTLD_NOW|RTLD_GLOBAL );
#endif

    setup_options();

    if ((thread_data_tls_index = TlsAlloc()) == TLS_OUT_OF_INDEXES) return FALSE;

    /* Open display */

    if (!XInitThreads()) ERR( "XInitThreads failed, trouble ahead\n" );
    if (!(display = XOpenDisplay( NULL ))) return FALSE;

    fcntl( ConnectionNumber(display), F_SETFD, 1 ); /* set close on exec flag */
    root_window = DefaultRootWindow( display );
    gdi_display = display;
    old_error_handler = XSetErrorHandler( error_handler );

    init_pixmap_formats( display );
    init_visuals( display, DefaultScreen( display ));
    screen_bpp = pixmap_formats[default_visual.depth]->bits_per_pixel;

    XInternAtoms( display, (char **)atom_names, NB_XATOMS - FIRST_XATOM, False, X11DRV_Atoms );

    winContext = XUniqueContext();
    win_data_context = XUniqueContext();
    cursor_context = XUniqueContext();

    if (TRACE_ON(synchronous)) XSynchronize( display, True );

    xinerama_init( DisplayWidth( display, default_visual.screen ),
                   DisplayHeight( display, default_visual.screen ));
    X11DRV_Settings_Init();

    /* initialize XVidMode */
    X11DRV_XF86VM_Init();
    /* initialize XRandR */
    X11DRV_XRandR_Init();
#ifdef SONAME_LIBXCOMPOSITE
    X11DRV_XComposite_Init();
#endif
    X11DRV_XInput2_Init();

#ifdef HAVE_XKB
    if (use_xkb) use_xkb = XkbUseExtension( gdi_display, NULL, NULL );
#endif
    X11DRV_InitKeyboard( gdi_display );
    if (use_xim) use_xim = X11DRV_InitXIM( input_style );

    init_user_driver();
    X11DRV_DisplayDevices_Init(FALSE);
    return TRUE;
}


/***********************************************************************
 *           ThreadDetach (X11DRV.@)
 */
void CDECL X11DRV_ThreadDetach(void)
{
    struct x11drv_thread_data *data = TlsGetValue( thread_data_tls_index );

    if (data)
    {
        vulkan_thread_detach();
        if (data->xim) XCloseIM( data->xim );
        if (data->font_set) XFreeFontSet( data->display, data->font_set );
        XCloseDisplay( data->display );
        HeapFree( GetProcessHeap(), 0, data );
        /* clear data in case we get re-entered from user32 before the thread is truly dead */
        TlsSetValue( thread_data_tls_index, NULL );
    }
}


/* store the display fd into the message queue */
static void set_queue_display_fd( Display *display )
{
    HANDLE handle;
    int ret;

    if (wine_server_fd_to_handle( ConnectionNumber(display), GENERIC_READ | SYNCHRONIZE, 0, &handle ))
    {
        MESSAGE( "x11drv: Can't allocate handle for display fd\n" );
        ExitProcess(1);
    }
    SERVER_START_REQ( set_queue_fd )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (ret)
    {
        MESSAGE( "x11drv: Can't store handle for display fd\n" );
        ExitProcess(1);
    }
    CloseHandle( handle );
}


/***********************************************************************
 *           X11DRV thread initialisation routine
 */
struct x11drv_thread_data *x11drv_init_thread_data(void)
{
    struct x11drv_thread_data *data = x11drv_thread_data();

    if (data) return data;

    if (!(data = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data) )))
    {
        ERR( "could not create data\n" );
        ExitProcess(1);
    }
    if (!(data->display = XOpenDisplay(NULL)))
    {
        ERR_(winediag)( "x11drv: Can't open display: %s. Please ensure that your X server is running and that $DISPLAY is set correctly.\n", XDisplayName(NULL));
        ExitProcess(1);
    }

    fcntl( ConnectionNumber(data->display), F_SETFD, 1 ); /* set close on exec flag */

#ifdef HAVE_XKB
    if (use_xkb && XkbUseExtension( data->display, NULL, NULL ))
        XkbSetDetectableAutoRepeat( data->display, True, NULL );
#endif

    if (TRACE_ON(synchronous)) XSynchronize( data->display, True );

    set_queue_display_fd( data->display );
    TlsSetValue( thread_data_tls_index, data );

    if (use_xim) X11DRV_SetupXIM();

    return data;
}


/***********************************************************************
 *           X11DRV initialisation routine
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    BOOL ret = TRUE;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        x11drv_module = hinst;
        ret = process_attach();
        break;
    }
    return ret;
}


/***********************************************************************
 *              SystemParametersInfo (X11DRV.@)
 */
BOOL CDECL X11DRV_SystemParametersInfo( UINT action, UINT int_param, void *ptr_param, UINT flags )
{
    switch (action)
    {
    case SPI_GETSCREENSAVEACTIVE:
        if (ptr_param)
        {
            int timeout, temp;
            XGetScreenSaver(gdi_display, &timeout, &temp, &temp, &temp);
            *(BOOL *)ptr_param = timeout != 0;
            return TRUE;
        }
        break;
    case SPI_SETSCREENSAVEACTIVE:
        {
            int timeout, interval, prefer_blanking, allow_exposures;
            static int last_timeout = 15 * 60;

            XLockDisplay( gdi_display );
            XGetScreenSaver(gdi_display, &timeout, &interval, &prefer_blanking,
                            &allow_exposures);
            if (timeout) last_timeout = timeout;

            timeout = int_param ? last_timeout : 0;
            XSetScreenSaver(gdi_display, timeout, interval, prefer_blanking,
                            allow_exposures);
            XUnlockDisplay( gdi_display );
        }
        break;
    }
    return FALSE;  /* let user32 handle it */
}

/**********************************************************************
 *           X11DRV_D3DKMTSetVidPnSourceOwner
 */
NTSTATUS CDECL X11DRV_D3DKMTSetVidPnSourceOwner( const D3DKMT_SETVIDPNSOURCEOWNER *desc )
{
    struct d3dkmt_vidpn_source *source, *source2;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL found;
    UINT i;

    TRACE("(%p)\n", desc);

    EnterCriticalSection( &x11drv_section );

    /* Check parameters */
    for (i = 0; i < desc->VidPnSourceCount; ++i)
    {
        LIST_FOR_EACH_ENTRY( source, &d3dkmt_vidpn_sources, struct d3dkmt_vidpn_source, entry )
        {
            if (source->id == desc->pVidPnSourceId[i])
            {
                /* Same device */
                if (source->device == desc->hDevice)
                {
                    if ((source->type == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE
                         && (desc->pType[i] == D3DKMT_VIDPNSOURCEOWNER_SHARED
                             || desc->pType[i] == D3DKMT_VIDPNSOURCEOWNER_EMULATED))
                        || (source->type == D3DKMT_VIDPNSOURCEOWNER_EMULATED
                            && desc->pType[i] == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE))
                    {
                        status = STATUS_INVALID_PARAMETER;
                        goto done;
                    }
                }
                /* Different devices */
                else
                {
                    if ((source->type == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE
                         || source->type == D3DKMT_VIDPNSOURCEOWNER_EMULATED)
                        && (desc->pType[i] == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE
                            || desc->pType[i] == D3DKMT_VIDPNSOURCEOWNER_EMULATED))
                    {
                        status = STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE;
                        goto done;
                    }
                }
            }
        }

        /* On Windows, it seems that all video present sources are owned by DMM clients, so any attempt to set
         * D3DKMT_VIDPNSOURCEOWNER_SHARED come back STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE */
        if (desc->pType[i] == D3DKMT_VIDPNSOURCEOWNER_SHARED)
        {
            status = STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE;
            goto done;
        }

        /* FIXME: D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVEGDI unsupported */
        if (desc->pType[i] == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVEGDI || desc->pType[i] > D3DKMT_VIDPNSOURCEOWNER_EMULATED)
        {
            status = STATUS_INVALID_PARAMETER;
            goto done;
        }
    }

    /* Remove owner */
    if (!desc->VidPnSourceCount && !desc->pType && !desc->pVidPnSourceId)
    {
        LIST_FOR_EACH_ENTRY_SAFE( source, source2, &d3dkmt_vidpn_sources, struct d3dkmt_vidpn_source, entry )
        {
            if (source->device == desc->hDevice)
            {
                list_remove( &source->entry );
                heap_free( source );
            }
        }
        goto done;
    }

    /* Add owner */
    for (i = 0; i < desc->VidPnSourceCount; ++i)
    {
        found = FALSE;
        LIST_FOR_EACH_ENTRY( source, &d3dkmt_vidpn_sources, struct d3dkmt_vidpn_source, entry )
        {
            if (source->device == desc->hDevice && source->id == desc->pVidPnSourceId[i])
            {
                found = TRUE;
                break;
            }
        }

        if (found)
            source->type = desc->pType[i];
        else
        {
            source = heap_alloc( sizeof( *source ) );
            if (!source)
            {
                status = STATUS_NO_MEMORY;
                goto done;
            }

            source->id = desc->pVidPnSourceId[i];
            source->type = desc->pType[i];
            source->device = desc->hDevice;
            list_add_tail( &d3dkmt_vidpn_sources, &source->entry );
        }
    }

done:
    LeaveCriticalSection( &x11drv_section );
    return status;
}

/**********************************************************************
 *           X11DRV_D3DKMTCheckVidPnExclusiveOwnership
 */
NTSTATUS CDECL X11DRV_D3DKMTCheckVidPnExclusiveOwnership( const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *desc )
{
    struct d3dkmt_vidpn_source *source;

    TRACE("(%p)\n", desc);

    if (!desc || !desc->hAdapter)
        return STATUS_INVALID_PARAMETER;

    EnterCriticalSection( &x11drv_section );
    LIST_FOR_EACH_ENTRY( source, &d3dkmt_vidpn_sources, struct d3dkmt_vidpn_source, entry )
    {
        if (source->id == desc->VidPnSourceId && source->type == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE)
        {
            LeaveCriticalSection( &x11drv_section );
            return STATUS_GRAPHICS_PRESENT_OCCLUDED;
        }
    }
    LeaveCriticalSection( &x11drv_section );
    return STATUS_SUCCESS;
}
