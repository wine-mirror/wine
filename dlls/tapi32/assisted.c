/*
 * TAPI32 Assisted Telephony
 *
 * Copyright 1999  Andreas Mohr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "winbase.h"
#include "windef.h"
#include "tapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(tapi);

/***********************************************************************
 *		tapiGetLocationInfo (TAPI32.@)
 */
DWORD WINAPI tapiGetLocationInfo(LPSTR lpszCountryCode, LPSTR lpszCityCode)
{
    char temp[30];

    FIXME("(%p, %p): file sections ???\n", lpszCountryCode, lpszCityCode);
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
