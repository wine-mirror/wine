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

#endif /* X_DISPLAY_MISSING */
