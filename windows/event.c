/*
 * X events handling functions
 * 
 * Copyright 1993 Alexandre Julliard
 * 
 */

#include "message.h"

extern EVENT_DRIVER X11DRV_EVENT_Driver;

/***********************************************************************
 *		EVENT_GetDriver
 */
EVENT_DRIVER *EVENT_GetDriver(void)
{
  return &X11DRV_EVENT_Driver;
}

/***********************************************************************
 *		EVENT_Init
 *
 * Initialize network IO.
 */
BOOL32 EVENT_Init(void)
{
  return EVENT_GetDriver()->pInit();
}

/***********************************************************************
 *		EVENT_AddIO 
 */
void EVENT_AddIO(int fd, unsigned io_type)
{
  EVENT_GetDriver()->pAddIO(fd, io_type);
}

/***********************************************************************
 *		EVENT_DeleteIO 
 */
void EVENT_DeleteIO(int fd, unsigned io_type)
{
  EVENT_GetDriver()->pDeleteIO(fd, io_type);
}

/***********************************************************************
 * 		EVENT_WaitNetEvent
 *
 * Wait for a network event, optionally sleeping until one arrives.
 * Return TRUE if an event is pending, FALSE on timeout or error
 * (for instance lost connection with the server).
 */
BOOL32 EVENT_WaitNetEvent(BOOL32 sleep, BOOL32 peek)
{
  return EVENT_GetDriver()->pWaitNetEvent(sleep, peek);
}

/***********************************************************************
 *		EVENT_Synchronize
 *
 * Synchronize with the X server. Should not be used too often.
 */
void EVENT_Synchronize(void)
{
  EVENT_GetDriver()->pSynchronize();
}

/**********************************************************************
 *		EVENT_CheckFocus
 */
BOOL32 EVENT_CheckFocus(void)
{
  return EVENT_GetDriver()->pCheckFocus();
}

/***********************************************************************
 *		EVENT_QueryPointer
 */
BOOL32 EVENT_QueryPointer(DWORD *posX, DWORD *posY, DWORD *state)
{
  return EVENT_GetDriver()->pQueryPointer(posX, posY, state);
}


/***********************************************************************
 *		EVENT_DummyMotionNotify
 *
 * Generate a dummy MotionNotify event. Used to force a WM_SETCURSOR message.
 */
void EVENT_DummyMotionNotify(void)
{
  EVENT_GetDriver()->pDummyMotionNotify();
}

/**********************************************************************
 *		X11DRV_EVENT_Pending
 */
BOOL32 EVENT_Pending()
{
  return EVENT_GetDriver()->pPending();
}

/***********************************************************************
 *		IsUserIdle	(USER.333)
 *
 * Check if we have pending X events.
 */
BOOL16 WINAPI IsUserIdle(void)
{
  return EVENT_GetDriver()->pIsUserIdle();
}
