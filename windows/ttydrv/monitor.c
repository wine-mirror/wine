/*
 * TTY monitor driver
 *
 * Copyright 1998,1999 Patrik Stridvall
 *
 */

#include "config.h"

#include "debugtools.h"
#include "heap.h"
#include "monitor.h"
#include "windef.h"
#include "ttydrv.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

/***********************************************************************
 *              TTYDRV_MONITOR_GetCursesRootWindow
 *
 * Return the Curses root window associated to the MONITOR.
 */
#ifdef HAVE_LIBCURSES
WINDOW *TTYDRV_MONITOR_GetCursesRootWindow(MONITOR *pMonitor)
{
  TTYDRV_MONITOR_DATA *pTTYMonitor =
    (TTYDRV_MONITOR_DATA *) pMonitor->pDriverData;

  return pTTYMonitor->rootWindow;
}
#endif /* defined(HAVE_LIBCURSES) */

/***********************************************************************
 *              TTYDRV_MONITOR_GetCellWidth
 */
INT TTYDRV_MONITOR_GetCellWidth(MONITOR *pMonitor)
{
  TTYDRV_MONITOR_DATA *pTTYMonitor =
    (TTYDRV_MONITOR_DATA *) pMonitor->pDriverData;

  return pTTYMonitor->cellWidth;
}

/***********************************************************************
 *              TTYDRV_MONITOR_GetCellHeight
 */
INT TTYDRV_MONITOR_GetCellHeight(MONITOR *pMonitor)
{
  TTYDRV_MONITOR_DATA *pTTYMonitor =
    (TTYDRV_MONITOR_DATA *) pMonitor->pDriverData;

  return pTTYMonitor->cellHeight;
}

/***********************************************************************
 *              TTYDRV_MONITOR_Initialize
 */
void TTYDRV_MONITOR_Initialize(MONITOR *pMonitor)
{
  TTYDRV_MONITOR_DATA *pTTYMonitor = (TTYDRV_MONITOR_DATA *) 
    HeapAlloc(SystemHeap, 0, sizeof(TTYDRV_MONITOR_DATA));
  int rows, cols;

  pMonitor->pDriverData = pTTYMonitor;

  pTTYMonitor->cellWidth = 8;
  pTTYMonitor->cellHeight = 8;

#ifdef HAVE_LIBCURSES
  pTTYMonitor->rootWindow = initscr();
  werase(pTTYMonitor->rootWindow);
  wrefresh(pTTYMonitor->rootWindow);

  getmaxyx(pTTYMonitor->rootWindow, rows, cols);
#else /* defined(HAVE_LIBCURSES) */
  rows = 60; /* FIXME: Hardcoded */
  cols = 80; /* FIXME: Hardcoded */
#endif /* defined(HAVE_LIBCURSES) */

  pTTYMonitor->width  = pTTYMonitor->cellWidth*cols;
  pTTYMonitor->height = pTTYMonitor->cellWidth*rows;
  pTTYMonitor->depth  = 1;
}

/***********************************************************************
 *              TTYDRV_MONITOR_Finalize
 */
void TTYDRV_MONITOR_Finalize(MONITOR *pMonitor)
{
  HeapFree(SystemHeap, 0, pMonitor->pDriverData);
}

/***********************************************************************
 *              TTYDRV_MONITOR_IsSingleWindow
 */
BOOL TTYDRV_MONITOR_IsSingleWindow(MONITOR *pMonitor)
{
  return TRUE;
}

/***********************************************************************
 *              TTYDRV_MONITOR_GetWidth
 *
 * Return the width of the monitor
 */
int TTYDRV_MONITOR_GetWidth(MONITOR *pMonitor)
{
  TTYDRV_MONITOR_DATA *pTTYMonitor =
    (TTYDRV_MONITOR_DATA *) pMonitor->pDriverData;

  return pTTYMonitor->width;
}

/***********************************************************************
 *              TTYDRV_MONITOR_GetHeight
 *
 * Return the height of the monitor
 */
int TTYDRV_MONITOR_GetHeight(MONITOR *pMonitor)
{
  TTYDRV_MONITOR_DATA *pTTYMonitor =
    (TTYDRV_MONITOR_DATA *) pMonitor->pDriverData;

  return pTTYMonitor->height;
}

/***********************************************************************
 *              TTYDRV_MONITOR_GetDepth
 *
 * Return the depth of the monitor
 */
int TTYDRV_MONITOR_GetDepth(MONITOR *pMonitor)
{
  TTYDRV_MONITOR_DATA *pTTYMonitor =
    (TTYDRV_MONITOR_DATA *) pMonitor->pDriverData;

  return pTTYMonitor->depth;
}

/***********************************************************************
 *              TTYDRV_MONITOR_GetScreenSaveActive
 *
 * Returns the active status of the screen saver
 */
BOOL TTYDRV_MONITOR_GetScreenSaveActive(MONITOR *pMonitor)
{
  return FALSE;
}

/***********************************************************************
 *              TTYDRV_MONITOR_SetScreenSaveActive
 *
 * Activate/Deactivate the screen saver
 */
void TTYDRV_MONITOR_SetScreenSaveActive(MONITOR *pMonitor, BOOL bActivate)
{
  FIXME("(%p, %d): stub\n", pMonitor, bActivate);
}

/***********************************************************************
 *              TTYDRV_MONITOR_GetScreenSaveTimeout
 *
 * Return the screen saver timeout
 */
int TTYDRV_MONITOR_GetScreenSaveTimeout(MONITOR *pMonitor)
{
  return 0;
}

/***********************************************************************
 *              TTYDRV_MONITOR_SetScreenSaveTimeout
 *
 * Set the screen saver timeout
 */
void TTYDRV_MONITOR_SetScreenSaveTimeout(
  MONITOR *pMonitor, int nTimeout)
{
  FIXME("(%p, %d): stub\n", pMonitor, nTimeout);
}
