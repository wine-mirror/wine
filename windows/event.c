/*
 * X events handling functions
 * 
 * Copyright 1993 Alexandre Julliard
 * 
 */

#include "message.h"
#include "user.h"
#include "win.h"
#include "debugtools.h"

DECLARE_DEBUG_CHANNEL(event);

/***********************************************************************
 *		EVENT_Synchronize
 *
 * Synchronize with the X server. Should not be used too often.
 */
void EVENT_Synchronize( void )
{
    int iWndsLocks = WIN_SuspendWndsLock();
    USER_Driver->pSynchronize();
    WIN_RestoreWndsLock(iWndsLocks);
}

/**********************************************************************
 *		EVENT_CheckFocus
 */
BOOL EVENT_CheckFocus(void)
{
  return USER_Driver->pCheckFocus();
}



