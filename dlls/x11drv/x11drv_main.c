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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <X11/cursorfont.h>
#include "ts_xlib.h"
#ifdef HAVE_XKB
#include <X11/XKBlib.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "winreg.h"

#include "gdi.h"
#include "user.h"
#include "win.h"
#include "x11drv.h"
#include "xvidmode.h"
#include "xrandr.h"
#include "dga2.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

static CRITICAL_SECTION X11DRV_CritSection;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &X11DRV_CritSection,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { 0, (DWORD)(__FILE__ ": X11DRV_CritSection") }
};
static CRITICAL_SECTION X11DRV_CritSection = { &critsect_debug, -1, 0, 0, 0, 0 };

Screen *screen;
Visual *visual;
unsigned int screen_width;
unsigned int screen_height;
unsigned int screen_depth;
Window root_window;
DWORD desktop_tid = 0;
int dxgrab, usedga, usexvidmode, usexrandr;
int use_xkb = 1;
int use_take_focus = 1;
int managed_mode = 1;
int client_side_with_core = 1;
int client_side_with_render = 1;
int client_side_antialias_with_core = 1;
int client_side_antialias_with_render = 1;

unsigned int X11DRV_server_startticks;

static BOOL synchronous;  /* run in synchronous mode? */
static BOOL desktop_dbl_buf;
static char *desktop_geometry;
static XVisualInfo *desktop_vi;

static x11drv_error_callback err_callback;   /* current callback for error */
static Display *err_callback_display;        /* display callback is set for */
static void *err_callback_arg;               /* error callback argument */
static int err_callback_result;              /* error callback result */
static int (*old_error_handler)( Display *, XErrorEvent * );

#define IS_OPTION_TRUE(ch) \
    ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')
#define IS_OPTION_FALSE(ch) \
    ((ch) == 'n' || (ch) == 'N' || (ch) == 'f' || (ch) == 'F' || (ch) == '0')

/***********************************************************************
 *		X11DRV_expect_error
 *
 * Setup a callback function that will be called on an X error.  The
 * callback must return non-zero if the error is the one it expected.
 * This function acquires the x11 lock; X11DRV_check_error must be
 * called in all cases to release it.
 */
void X11DRV_expect_error( Display *display, x11drv_error_callback callback, void *arg )
{
    wine_tsx11_lock();
    XSync( display, False );
    err_callback         = callback;
    err_callback_display = display;
    err_callback_arg     = arg;
    err_callback_result  = 0;
}


/***********************************************************************
 *		X11DRV_check_error
 *
 * Check if an expected X11 error occurred; return non-zero if yes.
 * Also release the x11 lock obtained in X11DRV_expect_error.
 */
int X11DRV_check_error(void)
{
    int ret;
    XSync( err_callback_display, False );
    err_callback = NULL;
    ret = err_callback_result;
    wine_tsx11_unlock();
    return ret;
}


/***********************************************************************
 *		error_handler
 */
static int error_handler( Display *display, XErrorEvent *error_evt )
{
    if (err_callback && display == err_callback_display)
    {
        if ((err_callback_result = err_callback( display, error_evt, err_callback_arg )))
        {
            TRACE( "got expected error\n" );
            return 0;
        }
    }
    if (synchronous) DebugBreak();  /* force an entry in the debugger */
    old_error_handler( display, error_evt );
    return 0;
}

/***********************************************************************
 *		wine_tsx11_lock   (X11DRV.@)
 */
void wine_tsx11_lock(void)
{
    EnterCriticalSection( &X11DRV_CritSection );
}

/***********************************************************************
 *		wine_tsx11_unlock   (X11DRV.@)
 */
void wine_tsx11_unlock(void)
{
    LeaveCriticalSection( &X11DRV_CritSection );
}

/***********************************************************************
 *		get_server_startup
 *
 * Get the server startup time
 * Won't be exact, but should be sufficient
 */
static void get_server_startup(void)
{
    struct timeval t;
    gettimeofday( &t, NULL );
    X11DRV_server_startticks = ((t.tv_sec * 1000) + (t.tv_usec / 1000)) - GetTickCount();
}


/***********************************************************************
 *		get_config_key
 *
 * Get a config key from either the app-specific or the default config
 */
inline static DWORD get_config_key( HKEY defkey, HKEY appkey, const char *name,
                                    char *buffer, DWORD size )
{
    if (appkey && !RegQueryValueExA( appkey, name, 0, NULL, buffer, &size )) return 0;
    return RegQueryValueExA( defkey, name, 0, NULL, buffer, &size );
}


/***********************************************************************
 *		setup_options
 *
 * Setup the x11drv options.
 */
static void setup_options(void)
{
    char buffer[MAX_PATH+16];
    HKEY hkey, appkey = 0;
    DWORD count;

    if (RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\x11drv", 0, NULL,
                         REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ))
    {
        ERR("Cannot create config registry key\n" );
        ExitProcess(1);
    }

    /* open the app-specific key */

    if (GetModuleFileNameA( 0, buffer, MAX_PATH ))
    {
        HKEY tmpkey;
        char *p, *appname = buffer;
        if ((p = strrchr( appname, '/' ))) appname = p + 1;
        if ((p = strrchr( appname, '\\' ))) appname = p + 1;
        strcat( appname, "\\x11drv" );
        if (!RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\AppDefaults", &tmpkey ))
        {
            if (RegOpenKeyA( tmpkey, appname, &appkey )) appkey = 0;
            RegCloseKey( tmpkey );
        }
    }

    /* get the display name */

    strcpy( buffer, "DISPLAY=" );
    count = sizeof(buffer) - 8;
    if (!RegQueryValueExA( hkey, "display", 0, NULL, buffer + 8, &count ))
    {
        const char *display_name = getenv( "DISPLAY" );
        if (display_name && strcmp( buffer, display_name ))
            MESSAGE( "x11drv: Warning: $DISPLAY variable ignored, using '%s' specified in config file\n",
                     buffer + 8 );
        putenv( strdup(buffer) );
    }

    if (!get_config_key( hkey, appkey, "Desktop", buffer, sizeof(buffer) ))
    {
        /* Imperfect validation:  If Desktop=N, then we don't turn on
        ** the --desktop option.  We should really validate for a correct
        ** sizing entry */
        if (!IS_OPTION_FALSE(buffer[0])) desktop_geometry = strdup(buffer);
    }

    if (!get_config_key( hkey, appkey, "Managed", buffer, sizeof(buffer) ))
        managed_mode = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "DXGrab", buffer, sizeof(buffer) ))
        dxgrab = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UseDGA", buffer, sizeof(buffer) ))
        usedga = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UseXVidMode", buffer, sizeof(buffer) ))
        usexvidmode = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UseXRandR", buffer, sizeof(buffer) ))
        usexrandr = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UseTakeFocus", buffer, sizeof(buffer) ))
        use_take_focus = IS_OPTION_TRUE( buffer[0] );

    screen_depth = 0;
    if (!get_config_key( hkey, appkey, "ScreenDepth", buffer, sizeof(buffer) ))
        screen_depth = atoi(buffer);

    if (!get_config_key( hkey, appkey, "Synchronous", buffer, sizeof(buffer) ))
        synchronous = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "ClientSideWithCore", buffer, sizeof(buffer) ))
        client_side_with_core = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "ClientSideWithRender", buffer, sizeof(buffer) ))
        client_side_with_render = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "ClientSideAntiAliasWithCore", buffer, sizeof(buffer) ))
        client_side_antialias_with_core = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "ClientSideAntiAliasWithRender", buffer, sizeof(buffer) ))
        client_side_antialias_with_render = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "DesktopDoubleBuffered", buffer, sizeof(buffer) ))
        desktop_dbl_buf = IS_OPTION_TRUE( buffer[0] );

    if (appkey) RegCloseKey( appkey );
    RegCloseKey( hkey );
}


/***********************************************************************
 *           X11DRV process initialisation routine
 */
static void process_attach(void)
{
    Display *display;

    get_server_startup();
    setup_options();

    /* Open display */

    if (!(display = TSXOpenDisplay( NULL )))
    {
        MESSAGE( "x11drv: Can't open display: %s\n", XDisplayName(NULL) );
        ExitProcess(1);
    }
    fcntl( ConnectionNumber(display), F_SETFD, 1 ); /* set close on exec flag */
    screen = DefaultScreenOfDisplay( display );
    visual = DefaultVisual( display, DefaultScreen(display) );
    root_window = DefaultRootWindow( display );
    old_error_handler = XSetErrorHandler( error_handler );

    /* Initialize screen depth */

    if (screen_depth)  /* depth specified */
    {
        int depth_count, i;
        int *depth_list = TSXListDepths(display, DefaultScreen(display), &depth_count);
        for (i = 0; i < depth_count; i++)
            if (depth_list[i] == screen_depth) break;
        TSXFree( depth_list );
        if (i >= depth_count)
	{
            MESSAGE( "x11drv: Depth %d not supported on this screen.\n", screen_depth );
            ExitProcess(1);
	}
    }
    else screen_depth = DefaultDepthOfScreen( screen );

    /* check for Xkb extension */
#ifdef HAVE_XKB
    if (use_xkb)
    {
        int xkb_opcode, xkb_event, xkb_error;
        int xkb_major = XkbMajorVersion, xkb_minor = XkbMinorVersion;

        use_xkb = XkbQueryExtension(display, &xkb_opcode, &xkb_event, &xkb_error,
                                    &xkb_major, &xkb_minor);
        if (use_xkb) /* we have XKB, approximate Windows behaviour */
            XkbSetDetectableAutoRepeat(display, True, NULL);
    }
#endif

    /* Initialize OpenGL */
    X11DRV_OpenGL_Init(display);

    /* If OpenGL is available, change the default visual, etc as necessary */
    if (desktop_dbl_buf && (desktop_vi = X11DRV_setup_opengl_visual( display )))
    {
        visual       = desktop_vi->visual;
        screen       = ScreenOfDisplay(display, desktop_vi->screen);
        screen_depth = desktop_vi->depth;
    }

    if (synchronous) XSynchronize( display, True );

    screen_width  = WidthOfScreen( screen );
    screen_height = HeightOfScreen( screen );

    X11DRV_Settings_Init();

    if (desktop_geometry)
        root_window = X11DRV_create_desktop( desktop_vi, desktop_geometry );

    /* initialize GDI */
    if(!X11DRV_GDI_Initialize( display ))
    {
        ERR( "Couldn't Initialize GDI.\n" );
        ExitProcess(1);
    }

    /* Initialize clipboard */
    if (!X11DRV_InitClipboard( display ))
    {
        ERR( "Couldn't Initialize clipboard.\n" );
        ExitProcess(1);
    }

#ifdef HAVE_LIBXXF86VM
    /* initialize XVidMode */
    X11DRV_XF86VM_Init();
#endif
#ifdef HAVE_LIBXRANDR
    /* initialize XRandR */
    X11DRV_XRandR_Init();
#endif
#ifdef HAVE_LIBXXF86DGA2
    /* initialize DGA2 */
    X11DRV_XF86DGA2_Init();
#endif
}


/***********************************************************************
 *           X11DRV thread termination routine
 */
static void thread_detach(void)
{
    struct x11drv_thread_data *data = NtCurrentTeb()->driver_data;

    if (data)
    {
        CloseHandle( data->display_fd );
        wine_tsx11_lock();
        XCloseDisplay( data->display );
        /* if (data->xim) XCloseIM( data->xim ); */ /* crashes Xlib */
        wine_tsx11_unlock();
        HeapFree( GetProcessHeap(), 0, data );
    }
}


/***********************************************************************
 *           X11DRV process termination routine
 */
static void process_detach(void)
{
#ifdef HAVE_LIBXXF86DGA2
    /* cleanup DGA2 */
    X11DRV_XF86DGA2_Cleanup();
#endif
#ifdef HAVE_LIBXXF86VM
    /* cleanup XVidMode */
    X11DRV_XF86VM_Cleanup();
#endif
    if(using_client_side_fonts)
        X11DRV_XRender_Finalize();

    /* FIXME: should detach all threads */
    thread_detach();

    /* cleanup GDI */
    X11DRV_GDI_Finalize();

    DeleteCriticalSection( &X11DRV_CritSection );
}


/***********************************************************************
 *           X11DRV thread initialisation routine
 */
struct x11drv_thread_data *x11drv_init_thread_data(void)
{
    struct x11drv_thread_data *data;

    if (!(data = HeapAlloc( GetProcessHeap(), 0, sizeof(*data) )))
    {
        ERR( "could not create data\n" );
        ExitProcess(1);
    }
    wine_tsx11_lock();
    if (!(data->display = XOpenDisplay(NULL)))
    {
        wine_tsx11_unlock();
        MESSAGE( "x11drv: Can't open display: %s\n", XDisplayName(NULL) );
        ExitProcess(1);
    }
    fcntl( ConnectionNumber(data->display), F_SETFD, 1 ); /* set close on exec flag */

    if ((data->xim = XOpenIM( data->display, NULL, NULL, NULL )))
    {
        TRACE("X display of IM = %p\n", XDisplayOfIM(data->xim));
        TRACE("Using %s locale of Input Method\n", XLocaleOfIM(data->xim));
    }
    else
        WARN("Can't open input method\n");

#ifdef HAVE_XKB
    if (use_xkb) XkbSetDetectableAutoRepeat( data->display, True, NULL );
#endif

    if (synchronous) XSynchronize( data->display, True );
    wine_tsx11_unlock();
    if (wine_server_fd_to_handle( ConnectionNumber(data->display), GENERIC_READ | SYNCHRONIZE,
                                  FALSE, &data->display_fd ))
    {
        MESSAGE( "x11drv: Can't allocate handle for display fd\n" );
        ExitProcess(1);
    }
    data->process_event_count = 0;
    data->cursor = None;
    data->cursor_window = None;
    data->last_focus = 0;
    NtCurrentTeb()->driver_data = data;
    if (desktop_tid) AttachThreadInput( GetCurrentThreadId(), desktop_tid, TRUE );
    return data;
}


/***********************************************************************
 *           X11DRV initialisation routine
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        process_attach();
        break;
    case DLL_THREAD_DETACH:
        thread_detach();
        break;
    case DLL_PROCESS_DETACH:
        process_detach();
        break;
    }
    return TRUE;
}

/***********************************************************************
 *              GetScreenSaveActive (X11DRV.@)
 *
 * Returns the active status of the screen saver
 */
BOOL X11DRV_GetScreenSaveActive(void)
{
    int timeout, temp;
    TSXGetScreenSaver(gdi_display, &timeout, &temp, &temp, &temp);
    return timeout != 0;
}

/***********************************************************************
 *              SetScreenSaveActive (X11DRV.@)
 *
 * Activate/Deactivate the screen saver
 */
void X11DRV_SetScreenSaveActive(BOOL bActivate)
{
    int timeout, interval, prefer_blanking, allow_exposures;
    static int last_timeout = 15 * 60;

    TSXGetScreenSaver(gdi_display, &timeout, &interval, &prefer_blanking,
                      &allow_exposures);
    if (timeout) last_timeout = timeout;

    timeout = bActivate ? last_timeout : 0;
    TSXSetScreenSaver(gdi_display, timeout, interval, prefer_blanking,
                      allow_exposures);
}
