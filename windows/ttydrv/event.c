/*
 * TTY event driver
 *
 * Copyright 1998-1999 Patrik Stridvall
 */

#include "ttydrv.h"

/***********************************************************************
 *		TTYDRV_EVENT_Init
 */
BOOL TTYDRV_EVENT_Init(void)
{
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_EVENT_Synchronize
 */
void TTYDRV_EVENT_Synchronize( BOOL bProcessEvents )
{
}

/***********************************************************************
 *		TTYDRV_EVENT_CheckFocus
 */
BOOL TTYDRV_EVENT_CheckFocus(void)
{
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_EVENT_QueryPointer
 */
BOOL TTYDRV_EVENT_QueryPointer(DWORD *posX, DWORD *posY, DWORD *state)
{
  if(posX)
    *posX = 0;

  if(posY)
    *posY = 0;

  if(state)
    *state = 0;

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_EVENT_DummyMotionNotify
 */
void TTYDRV_EVENT_DummyMotionNotify(void)
{
}

/***********************************************************************
 *		TTYDRV_EVENT_UserRepaintDisable
 */
void TTYDRV_EVENT_UserRepaintDisable( BOOL bDisable )
{
}

