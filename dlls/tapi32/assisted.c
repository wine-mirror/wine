/*
 * TAPI32 Assisted Telephony
 *
 * Copyright 1999  Andreas Mohr
 */

#include "winbase.h"
#include "windef.h"
#include "tapi.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(tapi);

/***********************************************************************
 *		tapiGetLocationInfo (TAPI32.@)
 */
DWORD WINAPI tapiGetLocationInfo(LPSTR lpszCountryCode, LPSTR lpszCityCode)
{
    char temp[30];
    
    FIXME("(%s, %s): file sections ???\n", lpszCountryCode, lpszCityCode);
    if (!(GetPrivateProfileStringA("Locations", "CurrentLocation", "", temp, 30, "telephon.ini")))
        return TAPIERR_REQUESTFAILED;
    if (!(GetPrivateProfileStringA("Locations", "FIXME_ENTRY", "", lpszCityCode, 8, "telephon.ini"))) 
        return TAPIERR_REQUESTFAILED;
    return 0;
}

/***********************************************************************
 *		tapiRequestMakeCall (TAPI32.@)
 */
DWORD WINAPI tapiRequestMakeCall(LPCSTR lpszDestAddress, LPCSTR lpszAppName,
                                 LPCSTR lpszCalledParty, LPCSTR lpszComment)
{
    FIXME("(%s, %s, %s, %s): stub.\n", lpszDestAddress, lpszAppName, lpszCalledParty, lpszComment);
    return 0;
}
