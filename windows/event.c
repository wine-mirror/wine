/*
 * X events handling functions
 * 
 * Copyright 1993 Alexandre Julliard
 * 
 */

#include "message.h"
#include "debugtools.h"

DECLARE_DEBUG_CHANNEL(event)

/**********************************************************************/

EVENT_DRIVER *EVENT_Driver = NULL;

/***********************************************************************
 *		EVENT_Init
 *
 * Initialize input event handling
 */
BOOL EVENT_Init(void)
{
    return EVENT_Driver->pInit();
}

/***********************************************************************
 *		EVENT_Synchronize
 *
 * Synchronize with the X server. Should not be used too often.
 */
void EVENT_Synchronize( void )
{
    int iWndsLocks = WIN_SuspendWndsLock();
    EVENT_Driver->pSynchronize();
    WIN_RestoreWndsLock(iWndsLocks);
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


