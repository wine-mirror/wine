#ifndef __WINE_RAS_H
#define __WINE_RAS_H

#include "wintypes.h"

#define RAS_MaxEntryName	256

typedef struct tagRASCONN32A {
	DWORD		dwSize;
	HRASCONN32	hRasConn;
	CHAR		szEntryName[RAS_MaxEntryName+1];
} RASCONN32A,*LPRASCONN32A;

DWORD WINAPI RasEnumConnections32A( LPRASCONN32A rc, LPDWORD x, LPDWORD y);
#endif
