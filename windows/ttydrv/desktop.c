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
