/*
 * TTY mouse driver
 *
 * Copyright 1998,1999 Patrik Stridvall
 */

#include "ttydrv.h"

/***********************************************************************
 *		TTYDRV_MOUSE_SetCursor
 */
void TTYDRV_MOUSE_SetCursor( struct tagCURSORICONINFO *lpCursor )
{
}

/***********************************************************************
 *		TTYDRV_MOUSE_MoveCursor
 */
void TTYDRV_MOUSE_MoveCursor(WORD wAbsX, WORD wAbsY)
{
}

/***********************************************************************
 *           TTYDRV_MOUSE_EnableWarpPointer
 */
BOOL TTYDRV_MOUSE_EnableWarpPointer(BOOL bEnable)
{
  return TRUE;
}
