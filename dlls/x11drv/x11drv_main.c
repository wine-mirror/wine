/*
 * X11DRV initialization code
 *
 * Copyright 1998 Patrik Stridvall
 * Copyright 2000 Alexandre Julliard
 */

#include "config.h"

#ifdef NO_REENTRANT_X11
/* Get pointers to the static errno and h_errno variables used by Xlib. This
   must be done before including <errno.h> makes the variables invisible.  */
extern int errno;
static int *perrno = &errno;
extern int h_errno;
static int *ph_errno = &h_errno;
#endif  /* NO_REENTRANT_X11 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <X11/cursorfont.h>
#include "ts_xlib.h"
#include "ts_xutil.h"
#include "ts_shape.h"

#include "winbase.h"
#include "wine/winbase16.h"
#include "winreg.h"

#include "debugtools.h"
#include "gdi.h"
#include "file.h"
#include "options.h"
#include "user.h"
#include "win.h"
#include "wine_gl.h"
#include "x11drv.h"
#include "xvidmode.h"
#include "dga2.h"

DEFAULT_DEBUG_CHANNEL(x11drv);

static void (*old_tsx11_lock)(void);
static void (*old_tsx11_unlock)(void);

static CRITICAL_SECTION X11DRV_CritSection = CRITICAL_SECTION_INIT("X11DRV_CritSection");

Screen *screen;
Visual *visual;
unsigned int screen_width;
unsigned int screen_height;
unsigned int screen_depth;
Window root_window;
int dxgrab, usedga, usexvidmode;

unsigned int X11DRV_server_startticks;

static BOOL synchronous;  /* run in synchronous mode? */
static char *desktop_geometry;
static XVisualInfo *desktop_vi;

#define IS_OPTION_TRUE(ch) \
    ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')
#define IS_OPTION_FALSE(ch) \
    ((ch) == 'n' || (ch) == 'N' || (ch) == 'f' || (ch) == 'F' || (ch) == '0')

#ifdef NO_REENTRANT_X11
static int* (*old_errno_location)(void);
static int* (*old_h_errno_location)(void);

/***********************************************************************
 *           x11_errno_location
 *
 * Get the per-thread errno location.
 */
static int *x11_errno_location(void)
{
    /* Use static libc errno while running in Xlib. */
    if (X11DRV_CritSection.OwningThread == GetCurrentThreadId()) return perrno;
    return old_errno_location();
}

/***********************************************************************
 *           x11_h_errno_location
 *
 * Get the per-thread h_errno location.
 */
static int *x11_h_errno_location(void)
{
    /* Use static libc h_errno while running in Xlib. */
    if (X11DRV_CritSection.OwningThread == GetCurrentThreadId()) return ph_errno;
    return old_h_errno_location();
}
#endif /* NO_REENTRANT_X11 */

/***********************************************************************
 *		error_handler
 */
static int error_handler(Display *display, XErrorEvent *error_evt)
{
    DebugBreak();  /* force an entry in the debugger */
    return 0;
}

/***********************************************************************
 *		lock_tsx11
 */
static void lock_tsx11(void)
{
    RtlEnterCriticalSection( &X11DRV_CritSection );
}

/***********************************************************************
 *		unlock_tsx11
 */
static void unlock_tsx11(void)
{
    RtlLeaveCriticalSection( &X11DRV_CritSection );
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

    if (GetModuleFileName16( GetCurrentTask(), buffer, MAX_PATH ) ||
        GetModuleFileNameA( 0, buffer, MAX_PATH ))
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

    /* check --managed option in wine config file if it was not set on command line */

    if (!Options.managed)
    {
        if (!get_config_key( hkey, appkey, "Managed", buffer, sizeof(buffer) ))
            Options.managed = IS_OPTION_TRUE( buffer[0] );
    }

    if (!get_config_key( hkey, appkey, "Desktop", buffer, sizeof(buffer) ))
    {
        /* Imperfect validation:  If Desktop=N, then we don't turn on
        ** the --desktop option.  We should really validate for a correct
        ** sizing entry */
        if (!IS_OPTION_FALSE(buffer[0])) desktop_geometry = strdup(buffer);
    }

    if (!get_config_key( hkey, appkey, "DXGrab", buffer, sizeof(buffer) ))
        dxgrab = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UseDGA", buffer, sizeof(buffer) ))
        usedga = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UseXVidMode", buffer, sizeof(buffer) ))
        usexvidmode = IS_OPTION_TRUE( buffer[0] );

    screen_depth = 0;
    if (!get_config_key( hkey, appkey, "ScreenDepth", buffer, sizeof(buffer) ))
        screen_depth = atoi(buffer);

    if (!get_config_key( hkey, appkey, "Synchronous", buffer, sizeof(buffer) ))
        synchronous = IS_OPTION_TRUE( buffer[0] );

    if (appkey) RegCloseKey( appkey );
    RegCloseKey( hkey );
}


/***********************************************************************
 *		setup_opengl_visual
 *
 * Setup the default visual used for OpenGL and Direct3D, and the desktop
 * window (if it exists).  If OpenGL isn't available, the visual is simply 
 * set to the default visual for the display
 */
#ifdef HAVE_OPENGL
static void setup_opengl_visual( Display *display )
{
    int err_base, evt_base;

    /* In order to support OpenGL or D3D, we require a double-buffered 
     * visual */
    if (glXQueryExtension(display, &err_base, &evt_base) == True) {
	  int dblBuf[]={GLX_RGBA,GLX_DEPTH_SIZE,16,GLX_DOUBLEBUFFER,None};
	
	  ENTER_GL();
	  desktop_vi = glXChooseVisual(display, DefaultScreen(display), dblBuf);
	  LEAVE_GL();
    }

    if (desktop_vi != NULL) {
      visual       = desktop_vi->visual;
      screen       = ScreenOfDisplay(display, desktop_vi->screen);
      screen_depth = desktop_vi->depth;
    }
}
#endif /* HAVE_OPENGL */    

/***********************************************************************
 *           X11DRV process initialisation routine
 */
static void process_attach(void)
{
    Display *display;

    get_server_startup();
    setup_options();

    /* setup TSX11 locking */
#ifdef NO_REENTRANT_X11
    old_errno_location = InterlockedExchangePointer( &wine_errno_location,
                                                     x11_errno_location );
    old_h_errno_location = InterlockedExchangePointer( &wine_h_errno_location,
                                                       x11_h_errno_location );
#endif /* NO_REENTRANT_X11 */
    old_tsx11_lock    = wine_tsx11_lock;
    old_tsx11_unlock  = wine_tsx11_unlock;
    wine_tsx11_lock   = lock_tsx11;
    wine_tsx11_unlock = unlock_tsx11;

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

    /* If OpenGL is available, change the default visual, etc as necessary */
#ifdef HAVE_OPENGL
    setup_opengl_visual( display );
#endif /* HAVE_OPENGL */

    /* tell the libX11 that we will do input method handling ourselves
     * that keep libX11 from doing anything whith dead keys, allowing Wine
     * to have total control over dead keys, that is this line allows
     * them to work in Wine, even whith a libX11 including the dead key
     * patches from Th.Quinot (http://Web.FdN.FR/~tquinot/dead-keys.en.html)
     */
    TSXOpenIM( display, NULL, NULL, NULL);

    if (synchronous)
    {
        XSetErrorHandler( error_handler );
        XSynchronize( display, True );
    }

    screen_width  = WidthOfScreen( screen );
    screen_height = HeightOfScreen( screen );

    if (desktop_geometry)
    {
        Options.managed = FALSE;
        root_window = X11DRV_create_desktop( desktop_vi, desktop_geometry );
    }

    /* initialize GDI */
    if(!X11DRV_GDI_Initialize( display ))
    {
        ERR( "Couldn't Initialize GDI.\n" );
        ExitProcess(1);
    }

#ifdef HAVE_LIBXXF86VM
    /* initialize XVidMode */
    X11DRV_XF86VM_Init();
#endif
#ifdef HAVE_LIBXXF86DGA2
    /* initialize DGA2 */
    X11DRV_XF86DGA2_Init();
#endif
#ifdef HAVE_OPENGL
    /* initialize GLX */
    /*X11DRV_GLX_Init();*/
#endif

    /* load display.dll */
    LoadLibrary16( "display" );
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
        wine_tsx11_unlock();
        HeapFree( GetProcessHeap(), 0, data );
    }
}


/***********************************************************************
 *           X11DRV process termination routine
 */
static void process_detach(void)
{
#ifdef HAVE_OPENGL
    /* cleanup GLX */
    /*X11DRV_GLX_Cleanup();*/
#endif
#ifdef HAVE_LIBXXF86DGA2
    /* cleanup DGA2 */
    X11DRV_XF86DGA2_Cleanup();
#endif
#ifdef HAVE_LIBXXF86VM
    /* cleanup XVidMode */
    X11DRV_XF86VM_Cleanup();
#endif

    /* FIXME: should detach all threads */
    thread_detach();

    /* cleanup GDI */
    X11DRV_GDI_Finalize();

    /* restore TSX11 locking */
    wine_tsx11_lock = old_tsx11_lock;
    wine_tsx11_unlock = old_tsx11_unlock;
#ifdef NO_REENTRANT_X11
    wine_errno_location = old_errno_location;
    wine_h_errno_location = old_h_errno_location;
#endif /* NO_REENTRANT_X11 */
    RtlDeleteCriticalSection( &X11DRV_CritSection );
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
    if (synchronous) XSynchronize( data->display, True );
    wine_tsx11_unlock();
    data->display_fd = FILE_DupUnixHandle( ConnectionNumber(data->display),
                                           GENERIC_READ | SYNCHRONIZE );
    data->process_event_count = 0;
    NtCurrentTeb()->driver_data = data;
    return data;
}


/***********************************************************************
 *           X11DRV initialisation routine
 */
BOOL WINAPI X11DRV_Init( HINSTANCE hinst, DWORD reason, LPVOID reserved )
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
    if(bActivate)
        TSXActivateScreenSaver(gdi_display);
    else
        TSXResetScreenSaver(gdi_display);
}

/***********************************************************************
 *              GetScreenSaveTimeout (X11DRV.@)
 *
 * Return the screen saver timeout
 */
int X11DRV_GetScreenSaveTimeout(void)
{
    int timeout, temp;
    TSXGetScreenSaver(gdi_display, &timeout, &temp, &temp, &temp);
    return timeout;
}

/***********************************************************************
 *              SetScreenSaveTimeout (X11DRV.@)
 *
 * Set the screen saver timeout
 */
void X11DRV_SetScreenSaveTimeout(int nTimeout)
{
    /* timeout is a 16bit entity (CARD16) in the protocol, so it should
     * not get over 32767 or it will get negative. */
    if (nTimeout>32767) nTimeout = 32767;
    TSXSetScreenSaver(gdi_display, nTimeout, 60, DefaultBlanking, DefaultExposures);
}
