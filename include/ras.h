#ifndef __WINE_RAS_H
#define __WINE_RAS_H

#include "wintypes.h"

#define RAS_MaxEntryName	256

typedef struct tagRASCONNA {
	DWORD		dwSize;
	HRASCONN	hRasConn;
	CHAR		szEntryName[RAS_MaxEntryName+1];
} RASCONNA,*LPRASCONNA;

DWORD WINAPI RasEnumConnectionsA( LPRASCONNA rc, LPDWORD x, LPDWORD y);
#endif
