#ifndef __WINE_RAS_H
#define __WINE_RAS_H

#include "windef.h"
#include "lmcons.h"

#define RAS_MaxEntryName	256
#define RAS_MaxPhoneNumber  128
#define RAS_MaxCallbackNumber RAS_MaxPhoneNumber

typedef struct tagRASCONNA {
	DWORD		dwSize;
	HRASCONN	hRasConn;
	CHAR		szEntryName[RAS_MaxEntryName+1];
} RASCONNA,*LPRASCONNA;

typedef struct tagRASENTRYNAME {
    DWORD dwSize;
    CHAR  szEntryName[ RAS_MaxEntryName + 1 ];
} RASENTRYNAME, *LPRASENTRYNAME;

typedef struct tagRASDIALPARAMS {
    DWORD dwSize;
    WCHAR szEntryName[ RAS_MaxEntryName + 1 ];
    WCHAR szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    WCHAR szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
    WCHAR szUserName[ UNLEN + 1 ];
    WCHAR szPassword[ PWLEN + 1 ];
    WCHAR szDomain[ DNLEN + 1 ];
    DWORD dwSubEntry;
    DWORD dwCallbackId;
} RASDIALPARAMS, *LPRASDIALPARAMS;


DWORD WINAPI RasEnumConnectionsA( LPRASCONNA rc, LPDWORD x, LPDWORD y);
DWORD WINAPI RasEnumEntriesA( LPSTR Reserved, LPSTR lpszPhoneBook,
        LPRASENTRYNAME lpRasEntryName, 
        LPDWORD lpcb, LPDWORD lpcEntries); 
DWORD WINAPI RasGetEntryDialParamsA( LPSTR lpszPhoneBook,
        LPRASDIALPARAMS lpRasDialParams,
        LPBOOL lpfPassword); 


#endif
