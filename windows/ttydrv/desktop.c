/*
 * TTY desktop driver
 *
 * Copyright 1998,1999 Patrik Stridvall
 *
 */

#include "desktop.h"
#include "monitor.h"
#include "ttydrv.h"

/***********************************************************************
 *              TTYDRV_DESKTOP_Initialize
 */
void TTYDRV_DESKTOP_Initialize(DESKTOP *pDesktop)
{
}

/***********************************************************************
 *              TTYDRV_DESKTOP_Finalize
 */
void TTYDRV_DESKTOP_Finalize(DESKTOP *pDesktop)
{
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
