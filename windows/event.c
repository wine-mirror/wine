/*
 * X events handling functions
 * 
 * Copyright 1993 Alexandre Julliard
 * 
 */

#include "message.h"

/**********************************************************************/

EVENT_DRIVER *EVENT_Driver = NULL;

/***********************************************************************
 *		EVENT_Init
 *
 * Initialize network IO.
 */
BOOL EVENT_Init(void)
{
  return EVENT_Driver->pInit();
}

/***********************************************************************
 *		EVENT_AddIO 
 */
void EVENT_AddIO(int fd, unsigned io_type)
{
  EVENT_Driver->pAddIO(fd, io_type);
}

/***********************************************************************
 *		EVENT_DeleteIO 
 */
void EVENT_DeleteIO(int fd, unsigned io_type)
{
  EVENT_Driver->pDeleteIO(fd, io_type);
}

/***********************************************************************
 * 		EVENT_WaitNetEvent
 *
 * Wait for a network event, optionally sleeping until one arrives.
 * Return TRUE if an event is pending, FALSE on timeout or error
 * (for instance lost connection with the server).
 */
BOOL EVENT_WaitNetEvent(BOOL sleep, BOOL peek)
{
  return EVENT_Driver->pWaitNetEvent(sleep, peek);
}

/***********************************************************************
 *		EVENT_Synchronize
 *
 * Synchronize with the X server. Should not be used too often.
 */
void EVENT_Synchronize(void)
{
  EVENT_Driver->pSynchronize();
}

/**********************************************************************
 *		EVENT_CheckFocus
 */
BOOL EVENT_CheckFocus(void)
{
  return EVENT_Driver->pCheckFocus();
}

/***********************************************************************
 *		EVENT_QueryPointer
 */
BOOL EVENT_QueryPointer(DWORD *posX, DWORD *posY, DWORD *state)
{
  return EVENT_Driver->pQueryPointer(posX, posY, state);
}


/***********************************************************************
 *		EVENT_DummyMotionNotify
 *
 * Generate a dummy MotionNotify event. Used to force a WM_SETCURSOR message.
 */
void EVENT_DummyMotionNotify(void)
{
  EVENT_Driver->pDummyMotionNotify();
}

/**********************************************************************
 *		EVENT_Pending
 */
BOOL EVENT_Pending()
{
  return EVENT_Driver->pPending();
}

/***********************************************************************
 *		IsUserIdle	(USER.333)
 *
 * Check if we have pending X events.
 */
BOOL16 WINAPI IsUserIdle16(void)
{
  return EVENT_Driver->pIsUserIdle();
}

/***********************************************************************
 *		EVENT_WakeUp
 *
 * Wake up the scheduler (EVENT_WaitNetEvent). Use by 32 bit thread
 * when thew want signaled an event to a 16 bit task. This function
 * will become obsolete when an Asynchronous thread will be implemented
 */
void EVENT_WakeUp(void)
{
  EVENT_Driver->pWakeUp();
}
