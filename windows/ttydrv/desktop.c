/*
 * TTY desktop driver
 *
 * Copyright 1998,1999 Patrik Stridvall
 *
 */

#include "config.h"

#include "debugtools.h"
#include "desktop.h"
#include "monitor.h"
#include "ttydrv.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

/***********************************************************************
 *              TTYDRV_DESKTOP_GetCursesRootWindow
 *
 * Return the Curses root window associated to the desktop.
 */
#ifdef HAVE_LIBCURSES
WINDOW *TTYDRV_DESKTOP_GetCursesRootWindow(DESKTOP *pDesktop)
{
  return TTYDRV_MONITOR_GetCursesRootWindow(pDesktop->pPrimaryMonitor);
}
#endif /* defined(HAVE_LIBCURSES) */

/***********************************************************************
 *              TTYDRV_DESKTOP_Initialize
 */
void TTYDRV_DESKTOP_Initialize(DESKTOP *pDesktop)
{
  TRACE("(%p): stub\n", pDesktop);

  pDesktop->pPrimaryMonitor = &MONITOR_PrimaryMonitor;
}

/***********************************************************************
 *              TTYDRV_DESKTOP_Finalize
 */
void TTYDRV_DESKTOP_Finalize(DESKTOP *pDesktop)
{
  TRACE("(%p): stub\n", pDesktop);
}

/***********************************************************************
 *              TTYDRV_DESKTOP_GetScreenWidth
 *
 * Return the width of the screen associated to the desktop.
 */
int TTYDRV_DESKTOP_GetScreenWidth(DESKTOP *pDesktop)
{
  return MONITOR_GetWidth(pDesktop->pPrimaryMonitor);
}

/***********************************************************************
 *              TTYDRV_DESKTOP_GetScreenHeight
 *
 * Return the width of the screen associated to the desktop.
 */
int TTYDRV_DESKTOP_GetScreenHeight(DESKTOP *pDesktop)
{
  return MONITOR_GetHeight(pDesktop->pPrimaryMonitor);
}

/***********************************************************************
 *              TTYDRV_DESKTOP_GetScreenDepth
 *
 * Return the depth of the screen associated to the desktop.
 */
int TTYDRV_DESKTOP_GetScreenDepth(DESKTOP *pDesktop)
{
  return MONITOR_GetDepth(pDesktop->pPrimaryMonitor);
}
