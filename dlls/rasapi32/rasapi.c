/*
 * RASAPI32
 * 
 * Copyright 1998 Marcus Meissner
 */

#include "wintypes.h"
#include "ras.h"
#include "debug.h"

/**************************************************************************
 *                 RasEnumConnections32A			[RASAPI32.544]
 */
DWORD WINAPI RasEnumConnectionsA( LPRASCONNA rca, LPDWORD x, LPDWORD y) {
	/* Remote Access Service stuff is done by underlying OS anyway */
	FIXME(ras,"(%p,%p,%p),stub!\n",rca,x,y);
	return 0;
}
