/*
 * X11 main driver
 *
 * Copyright 1998 Patrik Stridvall
 *
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include <X11/Xlocale.h>
#include "ts_xlib.h"
#include "ts_xresource.h"
#include "ts_xutil.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "clipboard.h"
#include "debugtools.h"
#include "desktop.h"
#include "keyboard.h"
#include "main.h"
#include "message.h"
#include "monitor.h"
#include "mouse.h"
#include "options.h"
#include "win.h"
#include "windef.h"
#include "x11drv.h"
#include "xmalloc.h"
#include "version.h"

/**********************************************************************/

static void X11DRV_USER_SaveSetup(void);
static void X11DRV_USER_RestoreSetup(void);

/**********************************************************************/

static XKeyboardState X11DRV_XKeyboardState;
Display *display;

/***********************************************************************
 *              X11DRV_GetXScreen
 *
 * Return the X screen associated to the current desktop.
 */
Screen *X11DRV_GetXScreen()
{
  return X11DRV_MONITOR_GetXScreen(&MONITOR_PrimaryMonitor);
}

/***********************************************************************
 *              X11DRV_GetXRootWindow
 *
 * Return the X display associated to the current desktop.
 */
Window X11DRV_GetXRootWindow()
{
  return X11DRV_MONITOR_GetXRootWindow(&MONITOR_PrimaryMonitor);
}

/***********************************************************************
 *		X11DRV_USER_ErrorHandler
 */
static int X11DRV_USER_ErrorHandler(Display *display, XErrorEvent *error_evt)
{
    DebugBreak();  /* force an entry in the debugger */
    return 0;
}

/***********************************************************************
 *              X11DRV_USER_Initialize
 */
BOOL X11DRV_USER_Initialize(void)
{
  CLIPBOARD_Driver = &X11DRV_CLIPBOARD_Driver;
  DESKTOP_Driver = &X11DRV_DESKTOP_Driver;
  EVENT_Driver = &X11DRV_EVENT_Driver;
  KEYBOARD_Driver = &X11DRV_KEYBOARD_Driver;
  MONITOR_Driver = &X11DRV_MONITOR_Driver;
  MOUSE_Driver = &X11DRV_MOUSE_Driver;
  WND_Driver = &X11DRV_WND_Driver;

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

  if (Options.synchronous) XSetErrorHandler( X11DRV_USER_ErrorHandler );
  if (Options.desktopGeometry && Options.managed) Options.managed = FALSE;

  X11DRV_USER_SaveSetup();

  return TRUE;
}

/***********************************************************************
 *              X11DRV_USER_Finalize
 */
void X11DRV_USER_Finalize(void)
{
  X11DRV_USER_RestoreSetup();
}

/**************************************************************************
 *		X11DRV_USER_BeginDebugging
 */
void X11DRV_USER_BeginDebugging(void)
{
  TSXUngrabServer(display);
  TSXFlush(display);
}

/**************************************************************************
 *		X11DRV_USER_EndDebugging
 */
void X11DRV_USER_EndDebugging(void)
{
}

/***********************************************************************
 *           X11DRV_USER_SaveSetup
 */
static void X11DRV_USER_SaveSetup()
{
  TSXGetKeyboardControl(display, &X11DRV_XKeyboardState);
}

/***********************************************************************
 *           X11DRV_USER_RestoreSetup
 */
static void X11DRV_USER_RestoreSetup()
{
  XKeyboardControl keyboard_value;
  
  keyboard_value.key_click_percent	= X11DRV_XKeyboardState.key_click_percent;
  keyboard_value.bell_percent 	= X11DRV_XKeyboardState.bell_percent;
  keyboard_value.bell_pitch		= X11DRV_XKeyboardState.bell_pitch;
  keyboard_value.bell_duration	= X11DRV_XKeyboardState.bell_duration;
  keyboard_value.auto_repeat_mode	= X11DRV_XKeyboardState.global_auto_repeat;
  
  XChangeKeyboardControl(display, KBKeyClickPercent | KBBellPercent | 
			 KBBellPitch | KBBellDuration | KBAutoRepeatMode, &keyboard_value);
}

#endif /* X_DISPLAY_MISSING */
