/*
 * X11 desktop driver
 *
 * Copyright 1998 Patrik Stridvall
 *
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include "debugtools.h"
#include "desktop.h"
#include "monitor.h"
#include "options.h"
#include "win.h"
#include "windef.h"
#include "x11drv.h"

/***********************************************************************
 *              X11DRV_DESKTOP_GetXScreen
 *
 * Return the X screen associated to the desktop.
 */
Screen *X11DRV_DESKTOP_GetXScreen(DESKTOP *pDesktop)
{
  return X11DRV_MONITOR_GetXScreen(pDesktop->pPrimaryMonitor);
}

/***********************************************************************
 *              X11DRV_DESKTOP_GetXRootWindow
 *
 * Return the X root window associated to the desktop.
 */
Window X11DRV_DESKTOP_GetXRootWindow(DESKTOP *pDesktop)
{
  return X11DRV_MONITOR_GetXRootWindow(pDesktop->pPrimaryMonitor);
}

#endif /* X_DISPLAY_MISSING */
