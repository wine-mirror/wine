/*
 * TTY monitor driver
 *
 * Copyright 1998,1999 Patrik Stridvall
 *
 */

#include "heap.h"
#include "monitor.h"
#include "windef.h"
#include "ttydrv.h"

/***********************************************************************
 *              TTYDRV_MONITOR_Initialize
 */
void TTYDRV_MONITOR_Initialize(MONITOR *pMonitor)
{
  TTYDRV_MONITOR_DATA *pTTYMonitor = (TTYDRV_MONITOR_DATA *) 
    HeapAlloc(SystemHeap, 0, sizeof(TTYDRV_MONITOR_DATA));

  pMonitor->pDriverData = pTTYMonitor;

  pTTYMonitor->width  = 640; /* FIXME: Screen width in pixels */
  pTTYMonitor->height = 480; /* FIXME: Screen height in pixels */
  pTTYMonitor->depth  = 1;   /* FIXME: Screen depth */
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
}
