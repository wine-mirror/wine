/*
 * TAPI32 line services
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "winbase.h"
#include "windef.h"
#include "winreg.h"
#include "winerror.h"
#include "tapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(tapi);

/***********************************************************************
 *		lineAccept (TAPI32.@)
 */
DWORD WINAPI lineAccept(HCALL hCall, LPCSTR lpsUserUserInfo, DWORD dwSize)
{
    FIXME("(%p, %s, %ld): stub.\n", hCall, lpsUserUserInfo, dwSize);
    return 1;
}

/***********************************************************************
 *		lineAddProvider (TAPI32.@)
 */
DWORD WINAPI lineAddProvider(LPCSTR lpszProviderName, HWND hwndOwner, LPDWORD lpdwPermanentProviderID)
{
    FIXME("(%s, %p, %p): stub.\n", lpszProviderName, hwndOwner, lpdwPermanentProviderID);
    return 1;
}

/***********************************************************************
 *		lineAddToConference (TAPI32.@)
 */
DWORD WINAPI lineAddToConference(HCALL hConfCall, HCALL hConsultCall)
{
    FIXME("(%p, %p): stub.\n", hConfCall, hConsultCall);
    return 1;
}

/***********************************************************************
 *		lineAnswer (TAPI32.@)
 */
DWORD WINAPI lineAnswer(HCALL hCall, LPCSTR lpsUserUserInfo, DWORD dwSize)
{
    FIXME("(%p, %s, %ld): stub.\n", hCall, lpsUserUserInfo, dwSize);
    return 1;
}

/***********************************************************************
 *		lineBlindTransfer (TAPI32.@)
 */
DWORD WINAPI lineBlindTransfer(HCALL hCall, LPCSTR lpszDestAddress, DWORD dwCountryCode)
{
    FIXME("(%p, %s, %08lx): stub.\n", hCall, lpszDestAddress, dwCountryCode);
    return 1;
}

/***********************************************************************
 *		lineClose (TAPI32.@)
 */
DWORD WINAPI lineClose(HLINE hLine)
{
    FIXME("(%p): stub.\n", hLine);
    return 0;
}

/***********************************************************************
 *		lineCompleteCall (TAPI32.@)
 */
DWORD WINAPI lineCompleteCall(HCALL hCall, LPDWORD lpdwCompletionID, DWORD dwCompletionMode, DWORD dwMessageID)
{
    FIXME("(%p, %p, %08lx, %08lx): stub.\n", hCall, lpdwCompletionID, dwCompletionMode, dwMessageID);
    return 1;
}

/***********************************************************************
 *		lineCompleteTransfer (TAPI32.@)
 */
DWORD WINAPI lineCompleteTransfer(HCALL hCall, HCALL hConsultCall, LPHCALL lphConfCall, DWORD dwTransferMode)
{
    FIXME("(%p, %p, %p, %08lx): stub.\n", hCall, hConsultCall, lphConfCall, dwTransferMode);
    return 1;
}

/***********************************************************************
 *		lineConfigDialog (TAPI32.@)
 */
DWORD WINAPI lineConfigDialog(DWORD dwDeviceID, HWND hwndOwner, LPCSTR lpszDeviceClass)
{
    FIXME("(%08lx, %p, %s): stub.\n", dwDeviceID, hwndOwner, lpszDeviceClass);
    return 0;
}

/***********************************************************************
 *		lineConfigDialogEdit (TAPI32.@)
 */
DWORD WINAPI lineConfigDialogEdit(DWORD dwDeviceID, HWND hwndOwner, LPCSTR lpszDeviceClass, LPVOID const lpDeviceConfigIn, DWORD dwSize, LPVARSTRING lpDeviceConfigOut)
{
    FIXME("stub.\n");
    return 0;
}

/***********************************************************************
 *		lineConfigProvider (TAPI32.@)
 */
DWORD WINAPI lineConfigProvider(HWND hwndOwner, DWORD dwPermanentProviderID)
{
    FIXME("(%p, %08lx): stub.\n", hwndOwner, dwPermanentProviderID);
    return 0;
}

/***********************************************************************
 *		lineDeallocateCall (TAPI32.@)
 */
DWORD WINAPI lineDeallocateCall(HCALL hCall)
{
    FIXME("(%p): stub.\n", hCall);
    return 0;
}

/***********************************************************************
 *		lineDevSpecific (TAPI32.@)
 */
DWORD WINAPI lineDevSpecific(HLINE hLine, DWORD dwAddressId, HCALL hCall, LPVOID lpParams, DWORD dwSize)
{
    FIXME("(%p, %08lx, %p, %p, %ld): stub.\n", hLine, dwAddressId, hCall, lpParams, dwSize);
    return 1;
}

/***********************************************************************
 *		lineDevSpecificFeature (TAPI32.@)
 */
DWORD WINAPI lineDevSpecificFeature(HLINE hLine, DWORD dwFeature, LPVOID lpParams, DWORD dwSize)
{
    FIXME("(%p, %08lx, %p, %ld): stub.\n", hLine, dwFeature, lpParams, dwSize);
    return 1;
}

/***********************************************************************
 *		lineDial (TAPI32.@)
 */
DWORD WINAPI lineDial(HCALL hCall, LPCSTR lpszDestAddress, DWORD dwCountryCode)
{
    FIXME("(%p, %s, %08lx): stub.\n", hCall, lpszDestAddress, dwCountryCode);
    return 1;
}

/***********************************************************************
 *		lineDrop (TAPI32.@)
 */
DWORD WINAPI lineDrop(HCALL hCall, LPCSTR lpsUserUserInfo, DWORD dwSize)
{
    FIXME("(%p, %s, %08lx): stub.\n", hCall, lpsUserUserInfo, dwSize);
    return 1;
}

/***********************************************************************
 *		lineForward (TAPI32.@)
 */
DWORD WINAPI lineForward(HLINE hLine, DWORD bAllAddress, DWORD dwAdressID, LPLINEFORWARDLIST lpForwardList, DWORD dwNumRingsNoAnswer, LPHCALL lphConsultCall, LPLINECALLPARAMS lpCallParams)
{
    FIXME("stub.\n");
    return 1;
}

/***********************************************************************
 *		lineGatherDigits (TAPI32.@)
 */
DWORD WINAPI lineGatherDigits(HCALL hCall, DWORD dwDigitModes, LPSTR lpsDigits, DWORD dwNumDigits, LPCSTR lpszTerminationDigits, DWORD dwFirstDigitTimeout, DWORD dwInterDigitTimeout)
{
    FIXME("stub.\n");
    return 0;
}

/***********************************************************************
 *		lineGenerateDigits (TAPI32.@)
 */
DWORD WINAPI lineGenerateDigits(HCALL hCall, DWORD dwDigitModes, LPCSTR lpszDigits, DWORD dwDuration)
{
    FIXME("(%p, %08lx, %s, %ld): stub.\n", hCall, dwDigitModes, lpszDigits, dwDuration);
    return 0;
}

/***********************************************************************
 *		lineGenerateTone (TAPI32.@)
 */
DWORD WINAPI lineGenerateTone(HCALL hCall, DWORD dwToneMode, DWORD dwDuration, DWORD dwNumTones, LPLINEGENERATETONE lpTones)
{
    FIXME("(%p, %08lx, %ld, %ld, %p): stub.\n", hCall, dwToneMode, dwDuration, dwNumTones, lpTones);
    return 0;
}

/***********************************************************************
 *		lineGetAddressCaps (TAPI32.@)
 */
DWORD WINAPI lineGetAddressCaps(HLINEAPP hLineApp, DWORD dwDeviceID, DWORD dwAddressID, DWORD dwAPIVersion, DWORD dwExtVersion, LPLINEADDRESSCAPS lpAddressCaps)
{
    FIXME("(%p, %08lx, %08lx, %08lx, %08lx, %p): stub.\n", hLineApp, dwDeviceID, dwAddressID, dwAPIVersion, dwExtVersion, lpAddressCaps);
    return 0;
}

/***********************************************************************
 *		lineGetAddressID (TAPI32.@)
 */
DWORD WINAPI lineGetAddressID(HLINE hLine, LPDWORD lpdwAddressID, DWORD dwAddressMode, LPCSTR lpsAddress, DWORD dwSize)
{
    FIXME("%p, %p, %08lx, %s, %ld): stub.\n", hLine, lpdwAddressID, dwAddressMode, lpsAddress, dwSize);
    return 0;
}

/***********************************************************************
 *		lineGetAddressStatus (TAPI32.@)
 */
DWORD WINAPI lineGetAddressStatus(HLINE hLine, DWORD dwAddressID, LPLINEADDRESSSTATUS lpAddressStatus)
{
    FIXME("(%p, %08lx, %p): stub.\n", hLine, dwAddressID, lpAddressStatus);
    return 0;
}

/***********************************************************************
 *		lineGetAppPriority (TAPI32.@)
 */
DWORD WINAPI lineGetAppPriority(LPCSTR lpszAppFilename, DWORD dwMediaMode, LPLINEEXTENSIONID const lpExtensionID, DWORD dwRequestMode, LPVARSTRING lpExtensionName, LPDWORD lpdwPriority)
{
    FIXME("(%s, %08lx, %p, %08lx, %p, %p): stub.\n", lpszAppFilename, dwMediaMode, lpExtensionID, dwRequestMode, lpExtensionName, lpdwPriority);
    return 0;
}

/***********************************************************************
 *		lineGetCallInfo (TAPI32.@)
 */
DWORD WINAPI lineGetCallInfo(HCALL hCall, LPLINECALLINFO lpCallInfo)
{
    FIXME("(%p, %p): stub.\n", hCall, lpCallInfo);
    return 0;
}

/***********************************************************************
 *		lineGetCallStatus (TAPI32.@)
 */
DWORD WINAPI lineGetCallStatus(HCALL hCall, LPLINECALLSTATUS lpCallStatus)
{
    FIXME("(%p, %p): stub.\n", hCall, lpCallStatus);
    return 0;
}

/***********************************************************************
 *		lineGetConfRelatedCalls (TAPI32.@)
 */
DWORD WINAPI lineGetConfRelatedCalls(HCALL hCall, LPLINECALLLIST lpCallList)
{
    FIXME("(%p, %p): stub.\n", hCall, lpCallList);
    return 0;
}

typedef struct tagTAPI_CountryInfo
{
    DWORD  dwCountryID;
    DWORD  dwCountryCode;
    LPSTR  lpCountryName;
    LPSTR  lpSameAreaRule;
    LPSTR  lpLongDistanceRule;
    LPSTR  lpInternationalRule;
} TAPI_CountryInfo;

/***********************************************************************
 *		lineGetCountry (TAPI32.@)
 */
DWORD WINAPI lineGetCountry(DWORD dwCountryID, DWORD dwAPIVersion, LPLINECOUNTRYLIST lpLineCountryList)
{
    DWORD dwAvailSize, dwOffset, i, num_countries, max_subkey_len;
    LPLINECOUNTRYENTRY lpLCE;
    HKEY hkey;
    char *subkey_name;

    if(!lpLineCountryList) {
	TRACE("(%08lx, %08lx, %p): stub. Returning LINEERR_INVALPOINTER\n",
	      dwCountryID, dwAPIVersion, lpLineCountryList);
        return LINEERR_INVALPOINTER;
    }

    TRACE("(%08lx, %08lx, %p(%ld)): stub.\n",
	  dwCountryID, dwAPIVersion, lpLineCountryList,
	  lpLineCountryList->dwTotalSize);

    if(RegOpenKeyA(HKEY_LOCAL_MACHINE,
		   "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Country List",
		   &hkey) != ERROR_SUCCESS)
        return LINEERR_INIFILECORRUPT;


    dwAvailSize = lpLineCountryList->dwTotalSize;
    dwOffset = sizeof (LINECOUNTRYLIST);

    if(dwAvailSize<dwOffset)
        return LINEERR_STRUCTURETOOSMALL;

    memset(lpLineCountryList, 0, dwAvailSize);

    lpLineCountryList->dwTotalSize         = dwAvailSize;
    lpLineCountryList->dwUsedSize          = dwOffset;
    lpLineCountryList->dwNumCountries      = 0;
    lpLineCountryList->dwCountryListSize   = 0;
    lpLineCountryList->dwCountryListOffset = dwOffset;

    lpLCE = (LPLINECOUNTRYENTRY)(&lpLineCountryList[1]);

    if(RegQueryInfoKeyA(hkey, NULL, NULL, NULL, &num_countries, &max_subkey_len,
			NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
        RegCloseKey(hkey);
        return LINEERR_STRUCTURETOOSMALL;
    }

    if(dwCountryID)
        dwOffset = sizeof (LINECOUNTRYENTRY);
    else
        dwOffset += num_countries * sizeof (LINECOUNTRYENTRY);

    max_subkey_len++;
    subkey_name = HeapAlloc(GetProcessHeap(), 0, max_subkey_len);
    for(i = 0; i < num_countries; i++)
    {
        DWORD len, size, size_int, size_long, size_name, size_same;
	HKEY hsubkey;

	if(RegEnumKeyA(hkey, i, subkey_name, max_subkey_len) !=
	   ERROR_SUCCESS)
	    continue;

        if(dwCountryID && (atoi(subkey_name) != dwCountryID))
            continue;

	if(RegOpenKeyA(hkey, subkey_name, &hsubkey) != ERROR_SUCCESS)
	    continue;

	RegQueryValueExA(hsubkey, "InternationalRule", NULL, NULL,
			 NULL, &size_int);
        len  = size_int;

	RegQueryValueExA(hsubkey, "LongDistanceRule", NULL, NULL,
			 NULL, &size_long);
        len += size_long;

	RegQueryValueExA(hsubkey, "Name", NULL, NULL,
			 NULL, &size_name);
        len += size_name;

	RegQueryValueExA(hsubkey, "SameAreaRule", NULL, NULL,
			 NULL, &size_same);
        len += size_same;

        if(dwAvailSize < (dwOffset+len))
        {
            dwOffset += len;
	    RegCloseKey(hsubkey);
	    if(dwCountryID)
		break;
            continue;
        }

        lpLineCountryList->dwNumCountries++;
        lpLineCountryList->dwCountryListSize += sizeof (LINECOUNTRYENTRY);
        lpLineCountryList->dwUsedSize += len + sizeof (LINECOUNTRYENTRY);

	if(dwCountryID)
	    i = 0;

        lpLCE[i].dwCountryID = atoi(subkey_name);
	size = sizeof(DWORD);
	RegQueryValueExA(hsubkey, "CountryCode", NULL, NULL,
			 (BYTE*)&lpLCE[i].dwCountryCode, &size);

	lpLCE[i].dwNextCountryID = 0;
        
	if(i > 0)
            lpLCE[i-1].dwNextCountryID = lpLCE[i].dwCountryID;

        /* add country name */
        lpLCE[i].dwCountryNameSize = size_name;
        lpLCE[i].dwCountryNameOffset = dwOffset;
	RegQueryValueExA(hsubkey, "Name", NULL, NULL,
			 ((LPSTR)lpLineCountryList)+dwOffset,
			 &size_name);
        dwOffset += size_name;

        /* add Same Area Rule */
        lpLCE[i].dwSameAreaRuleSize = size_same;
        lpLCE[i].dwSameAreaRuleOffset = dwOffset;
	RegQueryValueExA(hsubkey, "SameAreaRule", NULL, NULL,
			 ((LPSTR)lpLineCountryList)+dwOffset,
			 &size_same);
        dwOffset += size_same;

        /* add Long Distance Rule */
        lpLCE[i].dwLongDistanceRuleSize = size_long;
        lpLCE[i].dwLongDistanceRuleOffset = dwOffset;
	RegQueryValueExA(hsubkey, "LongDistanceRule", NULL, NULL,
			 ((LPSTR)lpLineCountryList)+dwOffset,
			 &size_long);
        dwOffset += size_long;

        /* add Long Distance Rule */
        lpLCE[i].dwInternationalRuleSize = size_int;
        lpLCE[i].dwInternationalRuleOffset = dwOffset;
	RegQueryValueExA(hsubkey, "InternationalRule", NULL, NULL,
			 ((LPSTR)lpLineCountryList)+dwOffset,
			 &size_int);
        dwOffset += size_int;
	RegCloseKey(hsubkey);

        TRACE("Added country %s at %p\n", (LPSTR)lpLineCountryList + lpLCE[i].dwCountryNameOffset,
	      &lpLCE[i]);

	if(dwCountryID) break;
    }

    lpLineCountryList->dwNeededSize = dwOffset;

    TRACE("%ld available %ld required\n", dwAvailSize, dwOffset);

    HeapFree(GetProcessHeap(), 0, subkey_name);
    RegCloseKey(hkey);

    return 0;
}

/***********************************************************************
 *		lineGetDevCaps (TAPI32.@)
 */
DWORD WINAPI lineGetDevCaps(HLINEAPP hLineApp, DWORD dwDeviceID, DWORD dwAPIVersion, DWORD dwExtVersion, LPLINEDEVCAPS lpLineDevCaps)
{
    FIXME("(%p, %08lx, %08lx, %08lx, %p): stub.\n", hLineApp, dwDeviceID, dwAPIVersion, dwExtVersion, lpLineDevCaps);
    return 0;
}

/***********************************************************************
 *		lineGetDevConfig (TAPI32.@)
 */
DWORD WINAPI lineGetDevConfig(DWORD dwDeviceID, LPVARSTRING lpDeviceConfig, LPCSTR lpszDeviceClass)
{
    FIXME("(%08lx, %p, %s): stub.\n", dwDeviceID, lpDeviceConfig, lpszDeviceClass);
    return 0;
}

/***********************************************************************
 *		lineGetID (TAPI32.@)
 */
DWORD WINAPI lineGetID(HLINE hLine, DWORD dwAddressID, HCALL hCall, DWORD dwSelect, LPVARSTRING lpDeviceID, LPCSTR lpszDeviceClass)
{
    FIXME("(%p, %08lx, %p, %08lx, %p, %s): stub.\n", hLine, dwAddressID, hCall, dwSelect, lpDeviceID, lpszDeviceClass);
    return 0;
}

/***********************************************************************
 *		lineGetIcon (TAPI32.@)
 */
DWORD WINAPI lineGetIcon(DWORD dwDeviceID, LPCSTR lpszDeviceClass, HICON *lphIcon)
{
    FIXME("(%08lx, %s, %p): stub.\n", dwDeviceID, lpszDeviceClass, lphIcon);
    return 0;
}

/***********************************************************************
 *		lineGetLineDevStatus (TAPI32.@)
 */
DWORD WINAPI lineGetLineDevStatus(HLINE hLine, LPLINEDEVSTATUS lpLineDevStatus)
{
    FIXME("(%p, %p): stub.\n", hLine, lpLineDevStatus);
    return 0;
}

/***********************************************************************
 *		lineGetNewCalls (TAPI32.@)
 */
DWORD WINAPI lineGetNewCalls(HLINE hLine, DWORD dwAddressID, DWORD dwSelect, LPLINECALLLIST lpCallList)
{
    FIXME("(%p, %08lx, %08lx, %p): stub.\n", hLine, dwAddressID, dwSelect, lpCallList);
    return 0;
}

/***********************************************************************
 *		lineGetNumRings (TAPI32.@)
 */
DWORD WINAPI lineGetNumRings(HLINE hLine, DWORD dwAddressID, LPDWORD lpdwNumRings)
{
    FIXME("(%p, %08lx, %p): stub.\n", hLine, dwAddressID, lpdwNumRings);
    return 0;
}

/***********************************************************************
 *		lineGetProviderList (TAPI32.@)
 */
DWORD WINAPI lineGetProviderList(DWORD dwAPIVersion, LPLINEPROVIDERLIST lpProviderList)
{
    FIXME("(%08lx, %p): stub.\n", dwAPIVersion, lpProviderList);
    return 0;
}

/***********************************************************************
 *		lineGetRequest (TAPI32.@)
 */
DWORD WINAPI lineGetRequest(HLINEAPP hLineApp, DWORD dwRequestMode, LPVOID lpRequestBuffer)
{
    FIXME("%p, %08lx, %p): stub.\n", hLineApp, dwRequestMode, lpRequestBuffer);
    return 0;
}

/***********************************************************************
 *		lineGetStatusMessages (TAPI32.@)
 */
DWORD WINAPI lineGetStatusMessages(HLINE hLine, LPDWORD lpdwLineStatus, LPDWORD lpdwAddressStates)
{
    FIXME("(%p, %p, %p): stub.\n", hLine, lpdwLineStatus, lpdwAddressStates);
    return 0;
}

/***********************************************************************
 *		lineGetTranslateCaps (TAPI32.@)
 */
DWORD WINAPI lineGetTranslateCaps(HLINEAPP hLineApp, DWORD dwAPIVersion, LPLINETRANSLATECAPS lpTranslateCaps)
{
    FIXME("(%p, %08lx, %p): stub.\n", hLineApp, dwAPIVersion, lpTranslateCaps);
    if(lpTranslateCaps->dwTotalSize >= sizeof(DWORD))
        memset(&lpTranslateCaps->dwNeededSize, 0, lpTranslateCaps->dwTotalSize - sizeof(DWORD));
    return 0;
}

/***********************************************************************
 *		lineHandoff (TAPI32.@)
 */
DWORD WINAPI lineHandoff(HCALL hCall, LPCSTR lpszFileName, DWORD dwMediaMode)
{
    FIXME("(%p, %s, %08lx): stub.\n", hCall, lpszFileName, dwMediaMode);
    return 0;
}

/***********************************************************************
 *		lineHold (TAPI32.@)
 */
DWORD WINAPI lineHold(HCALL hCall)
{
    FIXME("(%p): stub.\n", hCall);
    return 1;
}

/***********************************************************************
 *		lineInitialize (TAPI32.@)
 */
DWORD WINAPI lineInitialize(
  LPHLINEAPP lphLineApp,
  HINSTANCE hInstance,
  LINECALLBACK lpfnCallback,
  LPCSTR lpszAppName,
  LPDWORD lpdwNumDevs)
{
    FIXME("(%p, %p, %p, %s, %p): stub.\n", lphLineApp, hInstance,
	  lpfnCallback, debugstr_a(lpszAppName), lpdwNumDevs);
    return 0;
}

/***********************************************************************
 *		lineMakeCall (TAPI32.@)
 */
DWORD WINAPI lineMakeCall(HLINE hLine, LPHCALL lphCall, LPCSTR lpszDestAddress, DWORD dwCountryCode, LPLINECALLPARAMS lpCallParams)
{
    FIXME("(%p, %p, %s, %08lx, %p): stub.\n", hLine, lphCall, lpszDestAddress, dwCountryCode, lpCallParams);
    return 1;
}

/***********************************************************************
 *		lineMonitorDigits (TAPI32.@)
 */
DWORD WINAPI lineMonitorDigits(HCALL hCall, DWORD dwDigitModes)
{
    FIXME("(%p, %08lx): stub.\n", hCall, dwDigitModes);
    return 0;
}

/***********************************************************************
 *		lineMonitorMedia (TAPI32.@)
 */
DWORD WINAPI lineMonitorMedia(HCALL hCall, DWORD dwMediaModes)
{
    FIXME("(%p, %08lx): stub.\n", hCall, dwMediaModes);
    return 0;
}

/***********************************************************************
 *		lineMonitorTones (TAPI32.@)
 */
DWORD WINAPI lineMonitorTones(HCALL hCall, LPLINEMONITORTONE lpToneList, DWORD dwNumEntries)
{
    FIXME("(%p, %p, %08lx): stub.\n", hCall, lpToneList, dwNumEntries);
    return 0;
}

/***********************************************************************
 *		lineNegotiateAPIVersion (TAPI32.@)
 */
DWORD WINAPI lineNegotiateAPIVersion(
  HLINEAPP hLineApp,
  DWORD dwDeviceID,
  DWORD dwAPILowVersion,
  DWORD dwAPIHighVersion,
  LPDWORD lpdwAPIVersion,
  LPLINEEXTENSIONID lpExtensionID
)
{
    FIXME("(%p, %ld, %ld, %ld, %p, %p): stub.\n", hLineApp, dwDeviceID,
	  dwAPILowVersion, dwAPIHighVersion, lpdwAPIVersion, lpExtensionID);
    *lpdwAPIVersion = dwAPIHighVersion;
    return 0;
}

/***********************************************************************
 *		lineNegotiateExtVersion (TAPI32.@)
 */
DWORD WINAPI lineNegotiateExtVersion(HLINEAPP hLineApp, DWORD dwDeviceID, DWORD dwAPIVersion, DWORD dwExtLowVersion, DWORD dwExtHighVersion, LPDWORD lpdwExtVersion)
{
    FIXME("stub.\n");
    return 0;
}

/***********************************************************************
 *		lineOpen (TAPI32.@)
 */
DWORD WINAPI lineOpen(HLINEAPP hLineApp, DWORD dwDeviceID, LPHLINE lphLine, DWORD dwAPIVersion, DWORD dwExtVersion, DWORD dwCallbackInstance, DWORD dwPrivileges, DWORD dwMediaModes, LPLINECALLPARAMS lpCallParams)
{
    FIXME("stub.\n");
    return 0;
}

/***********************************************************************
 *		linePark (TAPI32.@)
 */
DWORD WINAPI linePark(HCALL hCall, DWORD dwParkMode, LPCSTR lpszDirAddress, LPVARSTRING lpNonDirAddress)
{
    FIXME("(%p, %08lx, %s, %p): stub.\n", hCall, dwParkMode, lpszDirAddress, lpNonDirAddress);
    return 1;
}

/***********************************************************************
 *		linePickup (TAPI32.@)
 */
DWORD WINAPI linePickup(HLINE hLine, DWORD dwAddressID, LPHCALL lphCall, LPCSTR lpszDestAddress, LPCSTR lpszGroupID)
{
    FIXME("(%p, %08lx, %p, %s, %s): stub.\n", hLine, dwAddressID, lphCall, lpszDestAddress, lpszGroupID);
    return 1;
}

/***********************************************************************
 *		linePrepareAddToConference (TAPI32.@)
 */
DWORD WINAPI linePrepareAddToConference(HCALL hConfCall, LPHCALL lphConsultCall, LPLINECALLPARAMS lpCallParams)
{
    FIXME("(%p, %p, %p): stub.\n", hConfCall, lphConsultCall, lpCallParams);
    return 1;
}

/***********************************************************************
 *		lineRedirect (TAPI32.@)
 */
DWORD WINAPI lineRedirect(
  HCALL hCall,
  LPCSTR lpszDestAddress,
  DWORD  dwCountryCode) {

  FIXME(": stub.\n");
  return 1;
}

/***********************************************************************
 *		lineRegisterRequestRecipient (TAPI32.@)
 */
DWORD WINAPI lineRegisterRequestRecipient(HLINEAPP hLineApp, DWORD dwRegistrationInstance, DWORD dwRequestMode, DWORD dwEnable)
{
    FIXME("(%p, %08lx, %08lx, %08lx): stub.\n", hLineApp, dwRegistrationInstance, dwRequestMode, dwEnable);
    return 1;
}

/***********************************************************************
 *		lineReleaseUserUserInfo (TAPI32.@)
 */
DWORD WINAPI lineReleaseUserUserInfo(HCALL hCall)
{
    FIXME("(%p): stub.\n", hCall);
    return 1;
}

/***********************************************************************
 *		lineRemoveFromConference (TAPI32.@)
 */
DWORD WINAPI lineRemoveFromConference(HCALL hCall)
{
    FIXME("(%p): stub.\n", hCall);
    return 1;
}

/***********************************************************************
 *		lineRemoveProvider (TAPI32.@)
 */
DWORD WINAPI lineRemoveProvider(DWORD dwPermanentProviderID, HWND hwndOwner)
{
    FIXME("(%08lx, %p): stub.\n", dwPermanentProviderID, hwndOwner);
    return 1;
}

/***********************************************************************
 *		lineSecureCall (TAPI32.@)
 */
DWORD WINAPI lineSecureCall(HCALL hCall)
{
    FIXME("(%p): stub.\n", hCall);
    return 1;
}

/***********************************************************************
 *		lineSendUserUserInfo (TAPI32.@)
 */
DWORD WINAPI lineSendUserUserInfo(HCALL hCall, LPCSTR lpsUserUserInfo, DWORD dwSize)
{
    FIXME("(%p, %s, %08lx): stub.\n", hCall, lpsUserUserInfo, dwSize);
    return 1;
}

/***********************************************************************
 *		lineSetAppPriority (TAPI32.@)
 */
DWORD WINAPI lineSetAppPriority(LPCSTR lpszAppFilename, DWORD dwMediaMode, LPLINEEXTENSIONID const lpExtensionID, DWORD dwRequestMode, LPCSTR lpszExtensionName, DWORD dwPriority)
{
    FIXME("(%s, %08lx, %p, %08lx, %s, %08lx): stub.\n", lpszAppFilename, dwMediaMode, lpExtensionID, dwRequestMode, lpszExtensionName, dwPriority);
    return 0;
}

/***********************************************************************
 *		lineSetAppSpecific (TAPI32.@)
 */
DWORD WINAPI lineSetAppSpecific(HCALL hCall, DWORD dwAppSpecific)
{
    FIXME("(%p, %08lx): stub.\n", hCall, dwAppSpecific);
    return 0;
}

/***********************************************************************
 *		lineSetCallParams (TAPI32.@)
 */
DWORD WINAPI lineSetCallParams(HCALL hCall, DWORD dwBearerMode, DWORD dwMinRate, DWORD dwMaxRate, LPLINEDIALPARAMS lpDialParams)
{
    FIXME("(%p, %08lx, %08lx, %08lx, %p): stub.\n", hCall, dwBearerMode, dwMinRate, dwMaxRate, lpDialParams);
    return 1;
}

/***********************************************************************
 *		lineSetCallPrivilege (TAPI32.@)
 */
DWORD WINAPI lineSetCallPrivilege(HCALL hCall, DWORD dwCallPrivilege)
{
    FIXME("(%p, %08lx): stub.\n", hCall, dwCallPrivilege);
    return 0;
}

/***********************************************************************
 *		lineSetCurrentLocation (TAPI32.@)
 */
DWORD WINAPI lineSetCurrentLocation(HLINEAPP hLineApp, DWORD dwLocation)
{
    FIXME("(%p, %08lx): stub.\n", hLineApp, dwLocation);
    return 0;
}

/***********************************************************************
 *		lineSetDevConfig (TAPI32.@)
 */
DWORD WINAPI lineSetDevConfig(DWORD dwDeviceID, LPVOID lpDeviceConfig, DWORD dwSize, LPCSTR lpszDeviceClass)
{
    FIXME("(%08lx, %p, %08lx, %s): stub.\n", dwDeviceID, lpDeviceConfig, dwSize, lpszDeviceClass);
    return 0;
}

/***********************************************************************
 *		lineSetMediaControl (TAPI32.@)
 */
DWORD WINAPI lineSetMediaControl(
HLINE hLine,
DWORD dwAddressID,
HCALL hCall,
DWORD dwSelect,
LPLINEMEDIACONTROLDIGIT const lpDigitList,
DWORD dwDigitNumEntries,
LPLINEMEDIACONTROLMEDIA const lpMediaList,
DWORD dwMediaNumEntries,
LPLINEMEDIACONTROLTONE const lpToneList,
DWORD dwToneNumEntries,
LPLINEMEDIACONTROLCALLSTATE const lpCallStateList,
DWORD dwCallStateNumEntries)
{
    FIXME(": stub.\n");
    return 0;
}

/***********************************************************************
 *		lineSetMediaMode (TAPI32.@)
 */
DWORD WINAPI lineSetMediaMode(HCALL hCall, DWORD dwMediaModes)
{
    FIXME("(%p, %08lx): stub.\n", hCall, dwMediaModes);
    return 0;
}

/***********************************************************************
 *		lineSetNumRings (TAPI32.@)
 */
DWORD WINAPI lineSetNumRings(HLINE hLine, DWORD dwAddressID, DWORD dwNumRings)
{
    FIXME("(%p, %08lx, %08lx): stub.\n", hLine, dwAddressID, dwNumRings);
    return 0;
}

/***********************************************************************
 *		lineSetStatusMessages (TAPI32.@)
 */
DWORD WINAPI lineSetStatusMessages(HLINE hLine, DWORD dwLineStates, DWORD dwAddressStates)
{
    FIXME("(%p, %08lx, %08lx): stub.\n", hLine, dwLineStates, dwAddressStates);
    return 0;
}

/***********************************************************************
 *		lineSetTerminal (TAPI32.@)
 */
DWORD WINAPI lineSetTerminal(HLINE hLine, DWORD dwAddressID, HCALL hCall, DWORD dwSelect, DWORD dwTerminalModes, DWORD dwTerminalID, DWORD bEnable)
{
    FIXME("(%p, %08lx, %p, %08lx, %08lx, %08lx, %08lx): stub.\n", hLine, dwAddressID, hCall, dwSelect, dwTerminalModes, dwTerminalID, bEnable);
    return 1;
}

/***********************************************************************
 *		lineSetTollList (TAPI32.@)
 */
DWORD WINAPI lineSetTollList(HLINEAPP hLineApp, DWORD dwDeviceID, LPCSTR lpszAddressIn, DWORD dwTollListOption)
{
    FIXME("(%p, %08lx, %s, %08lx): stub.\n", hLineApp, dwDeviceID, lpszAddressIn, dwTollListOption);
    return 0;
}

/***********************************************************************
 *		lineSetupConference (TAPI32.@)
 */
DWORD WINAPI lineSetupConference(HCALL hCall, HLINE hLine, LPHCALL lphConfCall, LPHCALL lphConsultCall, DWORD dwNumParties, LPLINECALLPARAMS lpCallParams)
{
    FIXME("(%p, %p, %p, %p, %08lx, %p): stub.\n", hCall, hLine, lphConfCall, lphConsultCall, dwNumParties, lpCallParams);
    return 1;
}

/***********************************************************************
 *		lineSetupTransfer (TAPI32.@)
 */
DWORD WINAPI lineSetupTransfer(HCALL hCall, LPHCALL lphConsultCall, LPLINECALLPARAMS lpCallParams)
{
    FIXME("(%p, %p, %p): stub.\n", hCall, lphConsultCall, lpCallParams);
    return 1;
}

/***********************************************************************
 *		lineShutdown (TAPI32.@)
 */
DWORD WINAPI lineShutdown(HLINEAPP hLineApp)
{
    FIXME("(%p): stub.\n", hLineApp);
    return 0;
}

/***********************************************************************
 *		lineSwapHold (TAPI32.@)
 */
DWORD WINAPI lineSwapHold(HCALL hActiveCall, HCALL hHeldCall)
{
    FIXME("(active: %p, held: %p): stub.\n", hActiveCall, hHeldCall);
    return 1;
}

/***********************************************************************
 *		lineTranslateAddress (TAPI32.@)
 */
DWORD WINAPI lineTranslateAddress(HLINEAPP hLineApp, DWORD dwDeviceID, DWORD dwAPIVersion, LPCSTR lpszAddressIn, DWORD dwCard, DWORD dwTranslateOptions, LPLINETRANSLATEOUTPUT lpTranslateOutput)
{
    FIXME("(%p, %08lx, %08lx, %s, %08lx, %08lx, %p): stub.\n", hLineApp, dwDeviceID, dwAPIVersion, lpszAddressIn, dwCard, dwTranslateOptions, lpTranslateOutput);
    return 0;
}

/***********************************************************************
 *		lineTranslateDialog (TAPI32.@)
 */
DWORD WINAPI lineTranslateDialog(HLINEAPP hLineApp, DWORD dwDeviceID, DWORD dwAPIVersion, HWND hwndOwner, LPCSTR lpszAddressIn)
{
    FIXME("(%p, %08lx, %08lx, %p, %s): stub.\n", hLineApp, dwDeviceID, dwAPIVersion, hwndOwner, lpszAddressIn);
    return 0;
}

/***********************************************************************
 *		lineUncompleteCall (TAPI32.@)
 */
DWORD WINAPI lineUncompleteCall(HLINE hLine, DWORD dwCompletionID)
{
    FIXME("(%p, %08lx): stub.\n", hLine, dwCompletionID);
    return 1;
}

/***********************************************************************
 *		lineUnhold (TAPI32.@)
 */
DWORD WINAPI lineUnhold(HCALL hCall)
{
    FIXME("(%p): stub.\n", hCall);
    return 1;
}

/***********************************************************************
 *		lineUnpark (TAPI32.@)
 */
DWORD WINAPI lineUnpark(HLINE hLine, DWORD dwAddressID, LPHCALL lphCall, LPCSTR lpszDestAddress)
{
    FIXME("(%p, %08lx, %p, %s): stub.\n", hLine, dwAddressID, lphCall, lpszDestAddress);
    return 1;
}
