/*
 * X11DRV initialization code
 *
 * Copyright 1998 Patrik Stridvall
 * Copyright 2000 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include "ts_xlib.h"

#include "winbase.h"

#include "clipboard.h"
#include "debugtools.h"
#include "gdi.h"
#include "monitor.h"
#include "options.h"
#include "user.h"
#include "win.h"
#include "x11drv.h"


static USER_DRIVER user_driver =
{
    /* event functions */
    X11DRV_EVENT_Synchronize,
    X11DRV_EVENT_CheckFocus,
    X11DRV_EVENT_UserRepaintDisable,
    /* keyboard functions */
    X11DRV_KEYBOARD_Init,
    X11DRV_KEYBOARD_VkKeyScan,
    X11DRV_KEYBOARD_MapVirtualKey,
    X11DRV_KEYBOARD_GetKeyNameText,
    X11DRV_KEYBOARD_ToAscii,
    X11DRV_KEYBOARD_GetBeepActive,
    X11DRV_KEYBOARD_SetBeepActive,
    X11DRV_KEYBOARD_Beep,
    X11DRV_KEYBOARD_GetDIState,
    X11DRV_KEYBOARD_GetDIData,
    X11DRV_KEYBOARD_GetKeyboardConfig,
    X11DRV_KEYBOARD_SetKeyboardConfig,
    /* mouse functions */
    X11DRV_MOUSE_Init,
    X11DRV_MOUSE_SetCursor,
    X11DRV_MOUSE_MoveCursor,
    X11DRV_MOUSE_EnableWarpPointer
};

static XKeyboardState keyboard_state;

Display *display;

/***********************************************************************
 *		error_handler
 */
static int error_handler(Display *display, XErrorEvent *error_evt)
{
    DebugBreak();  /* force an entry in the debugger */
    return 0;
}


/***********************************************************************
 *           X11DRV process initialisation routine
 */
static void process_attach(void)
{
    USER_Driver      = &user_driver;
    CLIPBOARD_Driver = &X11DRV_CLIPBOARD_Driver;
    MONITOR_Driver   = &X11DRV_MONITOR_Driver;
    WND_Driver       = &X11DRV_WND_Driver;

    /* We need this before calling any Xlib function */
    InitializeCriticalSection( &X11DRV_CritSection );
    MakeCriticalSectionGlobal( &X11DRV_CritSection );
  
    putenv("XKB_DISABLE="); /* Disable XKB extension if present. */

    /* Open display */
  
    if (!(display = TSXOpenDisplay( Options.display )))
    {
        MESSAGE( "%s: Can't open display: %s\n",
                 argv0, Options.display ? Options.display : "(none specified)" );
        ExitProcess(1);
    }
  
    /* tell the libX11 that we will do input method handling ourselves
     * that keep libX11 from doing anything whith dead keys, allowing Wine
     * to have total control over dead keys, that is this line allows
     * them to work in Wine, even whith a libX11 including the dead key
     * patches from Th.Quinot (http://Web.FdN.FR/~tquinot/dead-keys.en.html)
     */
    TSXOpenIM(display,NULL,NULL,NULL);

    if (Options.synchronous) XSetErrorHandler( error_handler );
    if (Options.desktopGeometry && Options.managed) Options.managed = FALSE;

    /* initialize monitor */
    X11DRV_MONITOR_Initialize( &MONITOR_PrimaryMonitor );

    /* initialize GDI */
    X11DRV_GDI_Initialize();

    /* save keyboard setup */
    TSXGetKeyboardControl(display, &keyboard_state);

    /* initialize event handling */
    X11DRV_EVENT_Init();
}


/***********************************************************************
 *           X11DRV process termination routine
 */
static void process_detach(void)
{
    /* restore keyboard setup */
    XKeyboardControl keyboard_value;
  
    keyboard_value.key_click_percent = keyboard_state.key_click_percent;
    keyboard_value.bell_percent      = keyboard_state.bell_percent;
    keyboard_value.bell_pitch        = keyboard_state.bell_pitch;
    keyboard_value.bell_duration     = keyboard_state.bell_duration;
    keyboard_value.auto_repeat_mode  = keyboard_state.global_auto_repeat;
  
    XChangeKeyboardControl(display, KBKeyClickPercent | KBBellPercent | 
                           KBBellPitch | KBBellDuration | KBAutoRepeatMode, &keyboard_value);

    /* cleanup GDI */
    X11DRV_GDI_Finalize();

#if 0  /* FIXME */

    /* cleanup monitor */
    X11DRV_MONITOR_Finalize( &MONITOR_PrimaryMonitor );

    /* close the display */
    XCloseDisplay( display );
    display = NULL;

    USER_Driver      = NULL;
    CLIPBOARD_Driver = NULL;
    MONITOR_Driver   = NULL;
    WND_Driver       = NULL;
#endif
}


/***********************************************************************
 *           X11DRV initialisation routine
 */
BOOL WINAPI X11DRV_Init( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    static int process_count;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        if (!process_count++) process_attach();
        break;
    case DLL_PROCESS_DETACH:
        if (!--process_count) process_detach();
        break;
    }
    return TRUE;
}
