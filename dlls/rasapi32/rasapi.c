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
 *                 RasEnumConnectionsA			[RASAPI32.544]
 */
DWORD WINAPI RasEnumConnectionsA( LPRASCONNA rca, LPDWORD x, LPDWORD y) {
	/* Remote Access Service stuff is done by underlying OS anyway */
	FIXME("(%p,%p,%p),stub!\n",rca,x,y);
	return 0;
}

/**************************************************************************
 *                 RasEnumEntriesA		        	[RASAPI32.546]
 */
DWORD WINAPI RasEnumEntriesA( LPSTR Reserved, LPSTR lpszPhoneBook,
        LPRASENTRYNAME lpRasEntryName, 
        LPDWORD lpcb, LPDWORD lpcEntries) 
{
	FIXME("(%p,%s,%p,%p,%p),stub!\n",Reserved,debugstr_a(lpszPhoneBook),
            lpRasEntryName,lpcb,lpcEntries);
        *lpcEntries = 0;
	return 0;
}

/**************************************************************************
 *                 RasGetEntryDialParamsA			[RASAPI32.550]
 */
DWORD WINAPI RasGetEntryDialParamsA( LPSTR lpszPhoneBook,
        LPRASDIALPARAMS lpRasDialParams,
        LPBOOL lpfPassword) 
{
	FIXME("(%s,%p,%p),stub!\n",debugstr_a(lpszPhoneBook),
            lpRasDialParams,lpfPassword);
	return 0;
}
