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
 *		TTYDRV_EVENT_AddIO
 */
void TTYDRV_EVENT_AddIO(int fd, unsigned flag)
{
}

/***********************************************************************
 *		TTYDRV_EVENT_DeleteIO
 */
void TTYDRV_EVENT_DeleteIO(int fd, unsigned flag)
{
}

/***********************************************************************
 *		TTYDRV_EVENT_WaitNetEvent
 */
BOOL TTYDRV_EVENT_WaitNetEvent(BOOL sleep, BOOL peek)
{
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_EVENT_Synchronize
 */
void TTYDRV_EVENT_Synchronize(void)
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
 *		TTYDRV_EVENT_Pending
 */
BOOL TTYDRV_EVENT_Pending(void)
{
  return FALSE;
}

/***********************************************************************
 *		TTYDRV_EVENT_IsUserIdle
 */
BOOL16 TTYDRV_EVENT_IsUserIdle(void)
{
  return TRUE;
}

/**********************************************************************
 *		TTYDRV_EVENT_WakeUp
 */
void TTYDRV_EVENT_WakeUp(void)
{
}
