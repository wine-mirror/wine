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

/***********************************************************************
 *              X11DRV_DESKTOP_Initialize
 */
void X11DRV_DESKTOP_Initialize(DESKTOP *pDesktop)
{
  pDesktop->pPrimaryMonitor = &MONITOR_PrimaryMonitor;
}

/***********************************************************************
 *              X11DRV_DESKTOP_Finalize
 */
void X11DRV_DESKTOP_Finalize(DESKTOP *pDesktop)
{
}

/***********************************************************************
 *              X11DRV_DESKTOP_GetScreenWidth
 *
 * Return the width of the screen associated to the desktop.
 */
int X11DRV_DESKTOP_GetScreenWidth(DESKTOP *pDesktop)
{
  return MONITOR_GetWidth(pDesktop->pPrimaryMonitor);
}

/***********************************************************************
 *              X11DRV_DESKTOP_GetScreenHeight
 *
 * Return the width of the screen associated to the desktop.
 */
int X11DRV_DESKTOP_GetScreenHeight(DESKTOP *pDesktop)
{
  return MONITOR_GetHeight(pDesktop->pPrimaryMonitor);
}

/***********************************************************************
 *              X11DRV_DESKTOP_GetScreenDepth
 *
 * Return the depth of the screen associated to the desktop.
 */
int X11DRV_DESKTOP_GetScreenDepth(DESKTOP *pDesktop)
{
  return MONITOR_GetDepth(pDesktop->pPrimaryMonitor);
}

#endif /* X_DISPLAY_MISSING */
