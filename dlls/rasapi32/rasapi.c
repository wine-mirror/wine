/*
 * RASAPI32
 * 
 * Copyright 1998 Marcus Meissner
 */

#include "windef.h"
#include "ras.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ras)

/**************************************************************************
 *                 RasEnumConnections32A			[RASAPI32.544]
 */
DWORD WINAPI RasEnumConnectionsA( LPRASCONNA rca, LPDWORD x, LPDWORD y) {
	/* Remote Access Service stuff is done by underlying OS anyway */
	FIXME("(%p,%p,%p),stub!\n",rca,x,y);
	return 0;
}
