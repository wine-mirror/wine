/*
 * SHLWAPI registry functions
 *
 * Copyright 1998 Juergen Schmied
 * Copyright 2001 Guy Albertelli
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
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/debug.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static const char *lpszContentTypeA = "Content Type";
static const WCHAR lpszContentTypeW[] = { 'C','o','n','t','e','n','t',' ','T','y','p','e','\0'};

/* internal structure of what the HUSKEY points to */
typedef struct {
    HKEY     HKCUkey;                  /* HKEY of opened HKCU key      */
    HKEY     HKLMkey;                  /* HKEY of opened HKLM key      */
    HKEY     start;                    /* HKEY of where to start       */
    WCHAR    key_string[MAX_PATH];     /* additional path from 'start' */
} Internal_HUSKEY, *LPInternal_HUSKEY;


#define REG_HKCU  TRUE
#define REG_HKLM  FALSE
/*************************************************************************
 * REG_GetHKEYFromHUSKEY
 *
 * Function:  Return the proper registry key from the HUSKEY structure
 *            also allow special predefined values.
 */
HKEY REG_GetHKEYFromHUSKEY(HUSKEY hUSKey, BOOL which)
{
        HKEY test = (HKEY) hUSKey;
        LPInternal_HUSKEY mihk = (LPInternal_HUSKEY) hUSKey;

	if ((test == HKEY_CLASSES_ROOT)        ||
	    (test == HKEY_CURRENT_CONFIG)      ||
	    (test == HKEY_CURRENT_USER)        ||
	    (test == HKEY_DYN_DATA)            ||
	    (test == HKEY_LOCAL_MACHINE)       ||
	    (test == HKEY_PERFORMANCE_DATA)    ||
/* FIXME:  need to define for Win2k, ME, XP
 *	    (test == HKEY_PERFORMANCE_TEXT)    ||
 *	    (test == HKEY_PERFORMANCE_NLSTEXT) ||
 */
	    (test == HKEY_USERS)) return test;
	if (which == REG_HKCU) return mihk->HKCUkey;
	return mihk->HKLMkey;
}


/*************************************************************************
 * SHRegOpenUSKeyA	[SHLWAPI.@]
 *
 * Opens a user-specific registry key
 */
LONG WINAPI SHRegOpenUSKeyA(
        LPCSTR Path,
        REGSAM AccessType,
        HUSKEY hRelativeUSKey,
        PHUSKEY phNewUSKey,
        BOOL fIgnoreHKCU)
{
    HKEY openHKCUkey=0;
    HKEY openHKLMkey=0;
    LONG ret2, ret1 = ~ERROR_SUCCESS;
    LPInternal_HUSKEY ihky;

    TRACE("(%s, 0x%lx, 0x%lx, %p, %s)\n", debugstr_a(Path),
	  (LONG)AccessType, (LONG)hRelativeUSKey, phNewUSKey,
	  (fIgnoreHKCU) ? "Ignoring HKCU" : "Process HKCU then HKLM");

    /* now create the internal version of HUSKEY */
    ihky = (LPInternal_HUSKEY)HeapAlloc(GetProcessHeap(), 0 ,
					sizeof(Internal_HUSKEY));
    MultiByteToWideChar(0, 0, Path, -1, ihky->key_string,
			sizeof(ihky->key_string)-1);

    if (hRelativeUSKey) {
	openHKCUkey = ((LPInternal_HUSKEY)hRelativeUSKey)->HKCUkey;
	openHKLMkey = ((LPInternal_HUSKEY)hRelativeUSKey)->HKLMkey;
    }
    else {
	openHKCUkey = HKEY_CURRENT_USER;
	openHKLMkey = HKEY_LOCAL_MACHINE;
    }

    ihky->HKCUkey = 0;
    ihky->HKLMkey = 0;
    if (!fIgnoreHKCU) {
	ret1 = RegOpenKeyExA(openHKCUkey, Path,
			     0, AccessType, &ihky->HKCUkey);
	/* if successful, then save real starting point */
	if (ret1 != ERROR_SUCCESS)
	    ihky->HKCUkey = 0;
    }
    ret2 = RegOpenKeyExA(openHKLMkey, Path,
			 0, AccessType, &ihky->HKLMkey);
    if (ret2 != ERROR_SUCCESS)
	ihky->HKLMkey = 0;

    if ((ret1 != ERROR_SUCCESS) || (ret2 != ERROR_SUCCESS))
	TRACE("one or more opens failed: HKCU=%ld HKLM=%ld\n", ret1, ret2);

    /* if all attempts have failed then bail */
    if ((ret1 != ERROR_SUCCESS) && (ret2 != ERROR_SUCCESS)) {
	HeapFree(GetProcessHeap(), 0, ihky);
	if (phNewUSKey)
	    *phNewUSKey = (HUSKEY)0;
	return ret2;
    }

    TRACE("HUSKEY=0x%08lx\n", (LONG)ihky);
    if (phNewUSKey)
	*phNewUSKey = (HUSKEY)ihky;
    return ERROR_SUCCESS;
}

/*************************************************************************
 * SHRegOpenUSKeyW	[SHLWAPI.@]
 *
 * Opens a user-specific registry key
 */
LONG WINAPI SHRegOpenUSKeyW(
        LPCWSTR Path,
        REGSAM AccessType,
        HUSKEY hRelativeUSKey,
        PHUSKEY phNewUSKey,
        BOOL fIgnoreHKCU)
{
    HKEY openHKCUkey=0;
    HKEY openHKLMkey=0;
    LONG ret2, ret1 = ~ERROR_SUCCESS;
    LPInternal_HUSKEY ihky;

    TRACE("(%s, 0x%lx, 0x%lx, %p, %s)\n", debugstr_w(Path),
	  (LONG)AccessType, (LONG)hRelativeUSKey, phNewUSKey,
	  (fIgnoreHKCU) ? "Ignoring HKCU" : "Process HKCU then HKLM");

    /* now create the internal version of HUSKEY */
    ihky = (LPInternal_HUSKEY)HeapAlloc(GetProcessHeap(), 0 ,
					sizeof(Internal_HUSKEY));
    lstrcpynW(ihky->key_string, Path, sizeof(ihky->key_string));

    if (hRelativeUSKey) {
	openHKCUkey = ((LPInternal_HUSKEY)hRelativeUSKey)->HKCUkey;
	openHKLMkey = ((LPInternal_HUSKEY)hRelativeUSKey)->HKLMkey;
    }
    else {
	openHKCUkey = HKEY_CURRENT_USER;
	openHKLMkey = HKEY_LOCAL_MACHINE;
    }

    ihky->HKCUkey = 0;
    ihky->HKLMkey = 0;
    if (!fIgnoreHKCU) {
	ret1 = RegOpenKeyExW(openHKCUkey, Path,
			    0, AccessType, &ihky->HKCUkey);
	/* if successful, then save real starting point */
	if (ret1 != ERROR_SUCCESS)
	    ihky->HKCUkey = 0;
    }
    ret2 = RegOpenKeyExW(openHKLMkey, Path,
			0, AccessType, &ihky->HKLMkey);
    if (ret2 != ERROR_SUCCESS)
	ihky->HKLMkey = 0;

    if ((ret1 != ERROR_SUCCESS) || (ret2 != ERROR_SUCCESS))
	TRACE("one or more opens failed: HKCU=%ld HKLM=%ld\n", ret1, ret2);

    /* if all attempts have failed then bail */
    if ((ret1 != ERROR_SUCCESS) && (ret2 != ERROR_SUCCESS)) {
	HeapFree(GetProcessHeap(), 0, ihky);
	if (phNewUSKey)
	    *phNewUSKey = (HUSKEY)0;
	return ret2;
    }

    TRACE("HUSKEY=0x%08lx\n", (LONG)ihky);
    if (phNewUSKey)
	*phNewUSKey = (HUSKEY)ihky;
    return ERROR_SUCCESS;
}

/*************************************************************************
 * SHRegCloseUSKey	[SHLWAPI.@]
 *
 * Closes a user-specific registry key
 */
LONG WINAPI SHRegCloseUSKey(
        HUSKEY hUSKey)
{
    LPInternal_HUSKEY mihk = (LPInternal_HUSKEY)hUSKey;
    LONG ret = ERROR_SUCCESS;

    if (mihk->HKCUkey)
	ret = RegCloseKey(mihk->HKCUkey);
    if (mihk->HKLMkey)
	ret = RegCloseKey(mihk->HKLMkey);
    HeapFree(GetProcessHeap(), 0, mihk);
    return ret;
}

/*************************************************************************
 *      SHRegQueryUSValueA	[SHLWAPI.@]
 */
LONG WINAPI SHRegQueryUSValueA(
	HUSKEY hUSKey,             /* [in]  */
	LPCSTR pszValue,
	LPDWORD pdwType,
	LPVOID pvData,
	LPDWORD pcbData,
	BOOL fIgnoreHKCU,
	LPVOID pvDefaultData,
	DWORD dwDefaultDataSize)
{
	LONG ret = ~ERROR_SUCCESS;
	LONG i, maxmove;
	HKEY dokey;
	CHAR *src, *dst;

	/* if user wants HKCU, and it exists, then try it */
	if (!fIgnoreHKCU && (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	    ret = RegQueryValueExA(dokey,
				   pszValue, 0, pdwType, pvData, pcbData);
	    TRACE("HKCU RegQueryValue returned %08lx\n", ret);
	}

	/* if HKCU did not work and HKLM exists, then try it */
	if ((ret != ERROR_SUCCESS) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	    ret = RegQueryValueExA(dokey,
				   pszValue, 0, pdwType, pvData, pcbData);
	    TRACE("HKLM RegQueryValue returned %08lx\n", ret);
	}

	/* if neither worked, and default data exists, then use it */
	if (ret != ERROR_SUCCESS) {
	    if (pvDefaultData && (dwDefaultDataSize != 0)) {
		maxmove = (dwDefaultDataSize >= *pcbData) ? *pcbData : dwDefaultDataSize;
		src = (CHAR*)pvDefaultData;
		dst = (CHAR*)pvData;
		for(i=0; i<maxmove; i++) *dst++ = *src++;
		*pcbData = maxmove;
		TRACE("setting default data\n");
		ret = ERROR_SUCCESS;
	    }
	}
	return ret;
}


/*************************************************************************
 *      SHRegQueryUSValueW	[SHLWAPI.@]
 */
LONG WINAPI SHRegQueryUSValueW(
	HUSKEY hUSKey,             /* [in]  */
	LPCWSTR pszValue,
	LPDWORD pdwType,
	LPVOID pvData,
	LPDWORD pcbData,
	BOOL fIgnoreHKCU,
	LPVOID pvDefaultData,
	DWORD dwDefaultDataSize)
{
	LONG ret = ~ERROR_SUCCESS;
	LONG i, maxmove;
	HKEY dokey;
	CHAR *src, *dst;

	/* if user wants HKCU, and it exists, then try it */
	if (!fIgnoreHKCU && (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	    ret = RegQueryValueExW(dokey,
				   pszValue, 0, pdwType, pvData, pcbData);
	    TRACE("HKCU RegQueryValue returned %08lx\n", ret);
	}

	/* if HKCU did not work and HKLM exists, then try it */
	if ((ret != ERROR_SUCCESS) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	    ret = RegQueryValueExW(dokey,
				   pszValue, 0, pdwType, pvData, pcbData);
	    TRACE("HKLM RegQueryValue returned %08lx\n", ret);
	}

	/* if neither worked, and default data exists, then use it */
	if (ret != ERROR_SUCCESS) {
	    if (pvDefaultData && (dwDefaultDataSize != 0)) {
		maxmove = (dwDefaultDataSize >= *pcbData) ? *pcbData : dwDefaultDataSize;
		src = (CHAR*)pvDefaultData;
		dst = (CHAR*)pvData;
		for(i=0; i<maxmove; i++) *dst++ = *src++;
		*pcbData = maxmove;
		TRACE("setting default data\n");
		ret = ERROR_SUCCESS;
	    }
	}
	return ret;
}

/*************************************************************************
 * SHRegGetUSValueA	[SHLWAPI.@]
 *
 * Gets a user-specific registry value
 *   Will open the key, query the value, and close the key
 */
LONG WINAPI SHRegGetUSValueA(
	LPCSTR   pSubKey,
	LPCSTR   pValue,
	LPDWORD  pwType,
	LPVOID   pvData,
	LPDWORD  pcbData,
	BOOL     flagIgnoreHKCU,
	LPVOID   pDefaultData,
	DWORD    wDefaultDataSize)
{
	HUSKEY myhuskey;
	LONG ret;

	if (!pvData || !pcbData) return ERROR_INVALID_FUNCTION; /* FIXME:wrong*/
	TRACE("key '%s', value '%s', datalen %ld,  %s\n",
	      debugstr_a(pSubKey), debugstr_a(pValue), *pcbData,
	      (flagIgnoreHKCU) ? "Ignoring HKCU" : "Tries HKCU then HKLM");

	ret = SHRegOpenUSKeyA(pSubKey, 0x1, 0, &myhuskey, flagIgnoreHKCU);
	if (ret == ERROR_SUCCESS) {
	    ret = SHRegQueryUSValueA(myhuskey, pValue, pwType, pvData,
				     pcbData, flagIgnoreHKCU, pDefaultData,
				     wDefaultDataSize);
	    SHRegCloseUSKey(myhuskey);
	}
	return ret;
}

/*************************************************************************
 * SHRegGetUSValueW	[SHLWAPI.@]
 *
 * Gets a user-specific registry value
 *   Will open the key, query the value, and close the key
 */
LONG WINAPI SHRegGetUSValueW(
	LPCWSTR  pSubKey,
	LPCWSTR  pValue,
	LPDWORD  pwType,
	LPVOID   pvData,
	LPDWORD  pcbData,
	BOOL     flagIgnoreHKCU,
	LPVOID   pDefaultData,
	DWORD    wDefaultDataSize)
{
	HUSKEY myhuskey;
	LONG ret;

	if (!pvData || !pcbData) return ERROR_INVALID_FUNCTION; /* FIXME:wrong*/
	TRACE("key '%s', value '%s', datalen %ld,  %s\n",
	      debugstr_w(pSubKey), debugstr_w(pValue), *pcbData,
	      (flagIgnoreHKCU) ? "Ignoring HKCU" : "Tries HKCU then HKLM");

	ret = SHRegOpenUSKeyW(pSubKey, 0x1, 0, &myhuskey, flagIgnoreHKCU);
	if (ret == ERROR_SUCCESS) {
	    ret = SHRegQueryUSValueW(myhuskey, pValue, pwType, pvData,
				     pcbData, flagIgnoreHKCU, pDefaultData,
				     wDefaultDataSize);
	    SHRegCloseUSKey(myhuskey);
	}
	return ret;
}

/*************************************************************************
 * SHRegGetBoolUSValueA   [SHLWAPI.@]
 */
BOOL WINAPI SHRegGetBoolUSValueA(
	LPCSTR pszSubKey,
	LPCSTR pszValue,
	BOOL fIgnoreHKCU,
	BOOL fDefault)
{
	LONG retvalue;
	DWORD type, datalen, work;
	BOOL ret = fDefault;
	CHAR data[10];

	TRACE("key '%s', value '%s', %s\n",
	      debugstr_a(pszSubKey), debugstr_a(pszValue),
	      (fIgnoreHKCU) ? "Ignoring HKCU" : "Tries HKCU then HKLM");

	datalen = sizeof(data)-1;
	if (!(retvalue = SHRegGetUSValueA( pszSubKey, pszValue, &type,
					   data, &datalen,
					   fIgnoreHKCU, 0, 0))) {
	    /* process returned data via type into bool */
	    switch (type) {
	    case REG_SZ:
		data[9] = '\0';     /* set end of string */
		if (lstrcmpiA(data, "YES") == 0) ret = TRUE;
		if (lstrcmpiA(data, "TRUE") == 0) ret = TRUE;
		if (lstrcmpiA(data, "NO") == 0) ret = FALSE;
		if (lstrcmpiA(data, "FALSE") == 0) ret = FALSE;
		break;
	    case REG_DWORD:
		work = *(LPDWORD)data;
		ret = (work != 0);
		break;
	    case REG_BINARY:
		if (datalen == 1) {
		    ret = (data[0] != '\0');
		    break;
		}
	    default:
		FIXME("Unsupported registry data type %ld\n", type);
		ret = FALSE;
	    }
	    TRACE("got value (type=%ld), returing <%s>\n", type,
		  (ret) ? "TRUE" : "FALSE");
	}
	else {
	    ret = fDefault;
	    TRACE("returning default data <%s>\n",
		  (ret) ? "TRUE" : "FALSE");
	}
	return ret;
}

/*************************************************************************
 * SHRegGetBoolUSValueW	  [SHLWAPI.@]
 */
BOOL WINAPI SHRegGetBoolUSValueW(
	LPCWSTR pszSubKey,
	LPCWSTR pszValue,
	BOOL fIgnoreHKCU,
	BOOL fDefault)
{
	static const WCHAR wYES[]=  {'Y','E','S','\0'};
	static const WCHAR wTRUE[]= {'T','R','U','E','\0'};
	static const WCHAR wNO[]=   {'N','O','\0'};
	static const WCHAR wFALSE[]={'F','A','L','S','E','\0'};
	LONG retvalue;
	DWORD type, datalen, work;
	BOOL ret = fDefault;
	WCHAR data[10];

	TRACE("key '%s', value '%s', %s\n",
	      debugstr_w(pszSubKey), debugstr_w(pszValue),
	      (fIgnoreHKCU) ? "Ignoring HKCU" : "Tries HKCU then HKLM");

	datalen = (sizeof(data)-1) * sizeof(WCHAR);
	if (!(retvalue = SHRegGetUSValueW( pszSubKey, pszValue, &type,
					   data, &datalen,
					   fIgnoreHKCU, 0, 0))) {
	    /* process returned data via type into bool */
	    switch (type) {
	    case REG_SZ:
		data[9] = L'\0';     /* set end of string */
		if (lstrcmpiW(data, wYES)==0 || lstrcmpiW(data, wTRUE)==0)
		    ret = TRUE;
		else if (lstrcmpiW(data, wNO)==0 || lstrcmpiW(data, wFALSE)==0)
		    ret = FALSE;
		break;
	    case REG_DWORD:
		work = *(LPDWORD)data;
		ret = (work != 0);
		break;
	    case REG_BINARY:
		if (datalen == 1) {
		    ret = (data[0] != L'\0');
		    break;
		}
	    default:
		FIXME("Unsupported registry data type %ld\n", type);
		ret = FALSE;
	    }
	    TRACE("got value (type=%ld), returing <%s>\n", type,
		  (ret) ? "TRUE" : "FALSE");
	}
	else {
	    ret = fDefault;
	    TRACE("returning default data <%s>\n",
		  (ret) ? "TRUE" : "FALSE");
	}
	return ret;
}

/*************************************************************************
 *      SHRegQueryInfoUSKeyA	[SHLWAPI.@]
 */
LONG WINAPI SHRegQueryInfoUSKeyA(
	HUSKEY hUSKey,             /* [in]  */
	LPDWORD pcSubKeys,
	LPDWORD pcchMaxSubKeyLen,
	LPDWORD pcValues,
	LPDWORD pcchMaxValueNameLen,
	SHREGENUM_FLAGS enumRegFlags)
{
	HKEY dokey;
	LONG ret;

	TRACE("(0x%lx,%p,%p,%p,%p,%d)\n",
	      (LONG)hUSKey,pcSubKeys,pcchMaxSubKeyLen,pcValues,
	      pcchMaxValueNameLen,enumRegFlags);

	/* if user wants HKCU, and it exists, then try it */
	if (((enumRegFlags == SHREGENUM_HKCU) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	    ret = RegQueryInfoKeyA(dokey, 0, 0, 0,
				   pcSubKeys, pcchMaxSubKeyLen, 0,
				   pcValues, pcchMaxValueNameLen, 0, 0, 0);
	    if ((ret == ERROR_SUCCESS) ||
		(enumRegFlags == SHREGENUM_HKCU))
		return ret;
	}
	if (((enumRegFlags == SHREGENUM_HKLM) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	    return RegQueryInfoKeyA(dokey, 0, 0, 0,
				    pcSubKeys, pcchMaxSubKeyLen, 0,
				    pcValues, pcchMaxValueNameLen, 0, 0, 0);
	}
	return ERROR_INVALID_FUNCTION;
}

/*************************************************************************
 *      SHRegQueryInfoUSKeyW	[SHLWAPI.@]
 */
LONG WINAPI SHRegQueryInfoUSKeyW(
	HUSKEY hUSKey,             /* [in]  */
	LPDWORD pcSubKeys,
	LPDWORD pcchMaxSubKeyLen,
	LPDWORD pcValues,
	LPDWORD pcchMaxValueNameLen,
	SHREGENUM_FLAGS enumRegFlags)
{
	HKEY dokey;
	LONG ret;

	TRACE("(0x%lx,%p,%p,%p,%p,%d)\n",
	      (LONG)hUSKey,pcSubKeys,pcchMaxSubKeyLen,pcValues,
	      pcchMaxValueNameLen,enumRegFlags);

	/* if user wants HKCU, and it exists, then try it */
	if (((enumRegFlags == SHREGENUM_HKCU) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	    ret = RegQueryInfoKeyW(dokey, 0, 0, 0,
				   pcSubKeys, pcchMaxSubKeyLen, 0,
				   pcValues, pcchMaxValueNameLen, 0, 0, 0);
	    if ((ret == ERROR_SUCCESS) ||
		(enumRegFlags == SHREGENUM_HKCU))
		return ret;
	}
	if (((enumRegFlags == SHREGENUM_HKLM) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	    return RegQueryInfoKeyW(dokey, 0, 0, 0,
				    pcSubKeys, pcchMaxSubKeyLen, 0,
				    pcValues, pcchMaxValueNameLen, 0, 0, 0);
	}
	return ERROR_INVALID_FUNCTION;
}

/*************************************************************************
 *      SHRegEnumUSKeyA   	[SHLWAPI.@]
 */
LONG WINAPI SHRegEnumUSKeyA(
	HUSKEY hUSKey,                 /* [in]  */
	DWORD dwIndex,                 /* [in]  */
	LPSTR pszName,                 /* [out] */
	LPDWORD pcchValueNameLen,      /* [in/out] */
	SHREGENUM_FLAGS enumRegFlags)  /* [in]  */
{
	HKEY dokey;

	TRACE("(0x%lx,%ld,%p,%p(%ld),%d)\n",
	      (LONG)hUSKey, dwIndex, pszName, pcchValueNameLen,
	      *pcchValueNameLen, enumRegFlags);

	if (((enumRegFlags == SHREGENUM_HKCU) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	    return RegEnumKeyExA(dokey, dwIndex, pszName, pcchValueNameLen,
				0, 0, 0, 0);
	}

	if (((enumRegFlags == SHREGENUM_HKLM) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	    return RegEnumKeyExA(dokey, dwIndex, pszName, pcchValueNameLen,
				0, 0, 0, 0);
	}
	FIXME("no support for SHREGNUM_BOTH\n");
	return ERROR_INVALID_FUNCTION;
}

/*************************************************************************
 *      SHRegEnumUSKeyW   	[SHLWAPI.@]
 */
LONG WINAPI SHRegEnumUSKeyW(
	HUSKEY hUSKey,                 /* [in]  */
	DWORD dwIndex,                 /* [in]  */
	LPWSTR pszName,                /* [out] */
	LPDWORD pcchValueNameLen,      /* [in/out] */
	SHREGENUM_FLAGS enumRegFlags)  /* [in]  */
{
	HKEY dokey;

	TRACE("(0x%lx,%ld,%p,%p(%ld),%d)\n",
	      (LONG)hUSKey, dwIndex, pszName, pcchValueNameLen,
	      *pcchValueNameLen, enumRegFlags);

	if (((enumRegFlags == SHREGENUM_HKCU) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	    return RegEnumKeyExW(dokey, dwIndex, pszName, pcchValueNameLen,
				0, 0, 0, 0);
	}

	if (((enumRegFlags == SHREGENUM_HKLM) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	    return RegEnumKeyExW(dokey, dwIndex, pszName, pcchValueNameLen,
				0, 0, 0, 0);
	}
	FIXME("no support for SHREGNUM_BOTH\n");
	return ERROR_INVALID_FUNCTION;
}

/*************************************************************************
 *      SHRegWriteUSValueA   	[SHLWAPI.@]
 */
LONG  WINAPI SHRegWriteUSValueA(HUSKEY hUSKey, LPCSTR pszValue, DWORD dwType,
				LPVOID pvData, DWORD cbData, DWORD dwFlags)
{
    HKEY dokey;

    TRACE("(0x%lx,%s,%ld,%p,%ld,%ld)\n",
	  (LONG)hUSKey, debugstr_a(pszValue), dwType, pvData, cbData, dwFlags);

    if ((dwFlags & SHREGSET_FORCE_HKCU) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	RegSetValueExA(dokey, pszValue, 0, dwType, pvData, cbData);
    }

    if ((dwFlags & SHREGSET_FORCE_HKLM) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	RegSetValueExA(dokey, pszValue, 0, dwType, pvData, cbData);
    }

    if (dwFlags & (SHREGSET_FORCE_HKCU | SHREGSET_FORCE_HKLM))
	return ERROR_SUCCESS;

    FIXME("SHREGSET_HKCU or SHREGSET_HKLM not supported\n");
    return ERROR_SUCCESS;
}

/*************************************************************************
 *      SHRegWriteUSValueW   	[SHLWAPI.@]
 */
LONG  WINAPI SHRegWriteUSValueW(HUSKEY hUSKey, LPCWSTR pszValue, DWORD dwType,
				LPVOID pvData, DWORD cbData, DWORD dwFlags)
{
    HKEY dokey;

    TRACE("(0x%lx,%s,%ld,%p,%ld,%ld)\n",
	  (LONG)hUSKey, debugstr_w(pszValue), dwType, pvData, cbData, dwFlags);

    if ((dwFlags & SHREGSET_FORCE_HKCU) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	RegSetValueExW(dokey, pszValue, 0, dwType, pvData, cbData);
    }

    if ((dwFlags & SHREGSET_FORCE_HKLM) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	RegSetValueExW(dokey, pszValue, 0, dwType, pvData, cbData);
    }

    if (dwFlags & (SHREGSET_FORCE_HKCU | SHREGSET_FORCE_HKLM))
	return ERROR_SUCCESS;

    FIXME("SHREGSET_HKCU or SHREGSET_HKLM not supported\n");
    return ERROR_SUCCESS;
}

/*************************************************************************
 * SHRegGetPathA   [SHLWAPI.@]
 *
 * Get a path from the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key containing path to get
 *   lpszValue  [I] Name of value containing path to get
 *   lpszPath   [O] Buffer for returned path
 *   dwFlags    [I] Reserved
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. lpszPath contains the path.
 *   Failure: An error code from RegOpenKeyExA or SHQueryValueExA.
 */
DWORD WINAPI SHRegGetPathA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue,
                           LPSTR lpszPath, DWORD dwFlags)
{
  DWORD dwSize = MAX_PATH;

  TRACE("(hkey=0x%08x,%s,%s,%p,%ld)\n", hKey, debugstr_a(lpszSubKey),
        debugstr_a(lpszValue), lpszPath, dwFlags);

  return SHGetValueA(hKey, lpszSubKey, lpszValue, 0, lpszPath, &dwSize);
}

/*************************************************************************
 * SHRegGetPathW   [SHLWAPI.@]
 *
 * See SHRegGetPathA.
 */
DWORD WINAPI SHRegGetPathW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue,
                           LPWSTR lpszPath, DWORD dwFlags)
{
  DWORD dwSize = MAX_PATH;

  TRACE("(hkey=0x%08x,%s,%s,%p,%ld)\n", hKey, debugstr_w(lpszSubKey),
        debugstr_w(lpszValue), lpszPath, dwFlags);

  return SHGetValueW(hKey, lpszSubKey, lpszValue, 0, lpszPath, &dwSize);
}


/*************************************************************************
 * SHRegSetPathA   [SHLWAPI.@]
 *
 * Write a path to the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key containing path to set
 *   lpszValue  [I] Name of value containing path to set
 *   lpszPath   [O] Path to write
 *   dwFlags    [I] Reserved
 *
 * RETURNS
 *   Success: ERROR_SUCCESS.
 *   Failure: An error code from SHSetValueA.
 */
DWORD WINAPI SHRegSetPathA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue,
                           LPCSTR lpszPath, DWORD dwFlags)
{
  char szBuff[MAX_PATH];

  FIXME("(hkey=0x%08x,%s,%s,%p,%ld) - semi-stub\n",hKey, debugstr_a(lpszSubKey),
        debugstr_a(lpszValue), lpszPath, dwFlags);

  lstrcpyA(szBuff, lpszPath);

  /* FIXME: PathUnExpandEnvStringsA(szBuff); */

  return SHSetValueA(hKey,lpszSubKey, lpszValue, REG_SZ, szBuff,
                     lstrlenA(szBuff));
}

/*************************************************************************
 * SHRegSetPathW   [SHLWAPI.@]
 *
 * See SHRegSetPathA.
 */
DWORD WINAPI SHRegSetPathW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue,
                           LPCWSTR lpszPath, DWORD dwFlags)
{
  WCHAR szBuff[MAX_PATH];

  FIXME("(hkey=0x%08x,%s,%s,%p,%ld) - semi-stub\n",hKey, debugstr_w(lpszSubKey),
        debugstr_w(lpszValue), lpszPath, dwFlags);

  lstrcpyW(szBuff, lpszPath);

  /* FIXME: PathUnExpandEnvStringsW(szBuff); */

  return SHSetValueW(hKey,lpszSubKey, lpszValue, REG_SZ, szBuff,
                     lstrlenW(szBuff));
}

/*************************************************************************
 * SHGetValueA   [SHLWAPI.@]
 *
 * Get a value from the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key containing value to get
 *   lpszValue  [I] Name of value to get
 *   pwType     [O] Pointer to the values type
 *   pvData     [O] Pointer to the values data
 *   pcbData    [O] Pointer to the values size
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. Output parameters contain the details read.
 *   Failure: An error code from RegOpenKeyExA or SHQueryValueExA.
 */
DWORD WINAPI SHGetValueA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pcbData)
{
  DWORD dwRet = 0;
  HKEY hSubKey = 0;

  TRACE("(hkey=0x%08x,%s,%s,%p,%p,%p)\n", hKey, debugstr_a(lpszSubKey),
        debugstr_a(lpszValue), pwType, pvData, pcbData);

  /*   lpszSubKey can be 0. In this case the value is taken from the
   *   current key.
   */
  if(lpszSubKey)
    dwRet = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_QUERY_VALUE, &hSubKey);

  if (! dwRet)
  {
    /* SHQueryValueEx expands Environment strings */
    dwRet = SHQueryValueExA(hSubKey ? hSubKey : hKey, lpszValue, 0, pwType, pvData, pcbData);
    if (hSubKey) RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHGetValueW   [SHLWAPI.@]
 *
 * See SHGetValueA.
 */
DWORD WINAPI SHGetValueW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pcbData)
{
  DWORD dwRet = 0;
  HKEY hSubKey = 0;

  TRACE("(hkey=0x%08x,%s,%s,%p,%p,%p)\n", hKey, debugstr_w(lpszSubKey),
        debugstr_w(lpszValue), pwType, pvData, pcbData);

  if(lpszSubKey)
    dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_QUERY_VALUE, &hSubKey);

  if (! dwRet)
  {
    dwRet = SHQueryValueExW(hSubKey ? hSubKey : hKey, lpszValue, 0, pwType, pvData, pcbData);
    if (hSubKey) RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHSetValueA   [SHLWAPI.@]
 *
 * Set a value in the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key under hKey
 *   lpszValue  [I] Name of value to set
 *   dwType     [I] Type of the value
 *   pvData     [I] Data of the value
 *   cbData     [I] Size of the value
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. The value is set with the data given.
 *   Failure: An error code from RegCreateKeyExA or RegSetValueExA
 *
 * NOTES
 *   If the sub key does not exist, it is created before the value is set. If
 *   The sub key is NULL or an empty string, then the value is added directly
 *   to hKey instead.
 */
DWORD WINAPI SHSetValueA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue,
                         DWORD dwType, LPCVOID pvData, DWORD cbData)
{
  DWORD dwRet = ERROR_SUCCESS, dwDummy;
  HKEY  hSubKey;
  LPSTR szEmpty = "";

  TRACE("(hkey=0x%08x,%s,%s,%ld,%p,%ld)\n", hKey, debugstr_a(lpszSubKey),
          debugstr_a(lpszValue), dwType, pvData, cbData);

  if (lpszSubKey && *lpszSubKey)
    dwRet = RegCreateKeyExA(hKey, lpszSubKey, 0, szEmpty,
                            0, KEY_SET_VALUE, NULL, &hSubKey, &dwDummy);
  else
    hSubKey = hKey;
  if (!dwRet)
  {
    dwRet = RegSetValueExA(hSubKey, lpszValue, 0, dwType, pvData, cbData);
    if (hSubKey != hKey)
      RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHSetValueW   [SHLWAPI.@]
 *
 * See SHSetValueA.
 */
DWORD WINAPI SHSetValueW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue,
                         DWORD dwType, LPCVOID pvData, DWORD cbData)
{
  DWORD dwRet = ERROR_SUCCESS, dwDummy;
  HKEY  hSubKey;
  WCHAR szEmpty[] = { '\0' };

  TRACE("(hkey=0x%08x,%s,%s,%ld,%p,%ld)\n", hKey, debugstr_w(lpszSubKey),
        debugstr_w(lpszValue), dwType, pvData, cbData);

  if (lpszSubKey && *lpszSubKey)
    dwRet = RegCreateKeyExW(hKey, lpszSubKey, 0, szEmpty,
                            0, KEY_SET_VALUE, NULL, &hSubKey, &dwDummy);
  else
    hSubKey = hKey;
  if (!dwRet)
  {
    dwRet = RegSetValueExW(hSubKey, lpszValue, 0, dwType, pvData, cbData);
    if (hSubKey != hKey)
      RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHQueryInfoKeyA   [SHLWAPI.@]
 *
 * Get information about a registry key. See RegQueryInfoKeyA.
 */
LONG WINAPI SHQueryInfoKeyA(HKEY hKey, LPDWORD pwSubKeys, LPDWORD pwSubKeyMax,
                            LPDWORD pwValues, LPDWORD pwValueMax)
{
  TRACE("(hkey=0x%08x,%p,%p,%p,%p)\n", hKey, pwSubKeys, pwSubKeyMax,
        pwValues, pwValueMax);
  return RegQueryInfoKeyA(hKey, NULL, NULL, NULL, pwSubKeys, pwSubKeyMax,
                          NULL, pwValues, pwValueMax, NULL, NULL, NULL);
}

/*************************************************************************
 * SHQueryInfoKeyW   [SHLWAPI.@]
 *
 * See SHQueryInfoKeyA
 */
LONG WINAPI SHQueryInfoKeyW(HKEY hKey, LPDWORD pwSubKeys, LPDWORD pwSubKeyMax,
                            LPDWORD pwValues, LPDWORD pwValueMax)
{
  TRACE("(hkey=0x%08x,%p,%p,%p,%p)\n", hKey, pwSubKeys, pwSubKeyMax,
        pwValues, pwValueMax);
  return RegQueryInfoKeyW(hKey, NULL, NULL, NULL, pwSubKeys, pwSubKeyMax,
                          NULL, pwValues, pwValueMax, NULL, NULL, NULL);
}

/*
  DWORD dwRet, dwType, dwDataLen;

  FIXME("(hkey=0x%08x,%s,%p,%p,%p,%p=%ld)\n", hKey, debugstr_a(lpszValue),
        lpReserved, pwType, pvData, pcbData, pcbData ? *pcbData : 0);

  if (pcbData) dwDataLen = *pcbData;

  dwRet = RegQueryValueExA(hKey, lpszValue, lpReserved, &dwType, pvData, &dwDataLen);
  if (!dwRet)
  {
    if (dwType == REG_EXPAND_SZ)
    {
      LPSTR szExpand;
      LPBYTE pData = pvData;

      if (!pData)
      {
        if (!pcbData || !(pData = (LPBYTE) LocalAlloc(GMEM_ZEROINIT, *pcbData)))
          return ERROR_OUTOFMEMORY;

        if ((dwRet = RegQueryValueExA (hKey, lpszValue, lpReserved, &dwType, pData, &dwDataLen)))
          return dwRet;
      }

      if (!pcbData && pData != pvData)
      {
        WARN("Invalid pcbData would crash under Win32!\n");
        return ERROR_OUTOFMEMORY;
      }

      szExpand = (LPBYTE) LocalAlloc(GMEM_ZEROINIT, *pcbData);
      if (!szExpand)
      {
        if ( pData != pvData ) LocalFree((HLOCAL)pData);
        return ERROR_OUTOFMEMORY;
      }
      if ((ExpandEnvironmentStringsA(pvData, szExpand, *pcbData) > 0))
      {
        dwDataLen = strlen(szExpand) + 1;
        strncpy(pvData, szExpand, *pcbData);
      }
      else
      {
        if ( pData != pvData ) LocalFree((HLOCAL)pData);
        LocalFree((HLOCAL)szExpand);
        return GetLastError();
      }
      if (pData != pvData) LocalFree((HLOCAL)pData);
      LocalFree((HLOCAL)szExpand);
      dwType = REG_SZ;
    }
    if (dwType == REG_SZ && pvData && pcbData && dwDataLen >= *pcbData)
    {
      ((LPBYTE) pvData)[*pcbData] = '\0';
    }
  }
  if ( pwType ) *pwType = dwType;
  if ( pcbData ) *pcbData = dwDataLen;
  return dwRet;
*/

/*************************************************************************
 * SHQueryValueExA   [SHLWAPI.@]
 *
 * Get a value from the registry, expanding environment variable strings.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszValue  [I] Name of value to query
 *   lpReserved [O] Reserved for future use; must be NULL
 *   pwType     [O] Optional pointer updated with the values type
 *   pvData     [O] Optional pointer updated with the values data
 *   pcbData    [O] Optional pointer updated with the values size
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. Any non-NULL output parameters are updated with
 *            information about the value.
 *   Failure: ERROR_OUTOFMEMORY if memory allocation fails, or the type of the
 *            data is REG_EXPAND_SZ and pcbData is NULL. Otherwise an error
 *            code from RegQueryValueExA or ExpandEnvironmentStringsA.
 *
 * NOTES
 *   Either pwType, pvData or pcbData may be NULL if the caller doesn't want
 *   the type, data or size information for the value.
 *
 *   If the type of the data is REG_EXPAND_SZ, it is expanded to REG_SZ. The
 *   value returned will be truncated if it is of type REG_SZ and bigger than
 *   the buffer given to store it.
 */
DWORD WINAPI SHQueryValueExA( HKEY hKey, LPCSTR lpszValue,
                              LPDWORD lpReserved, LPDWORD pwType,
                              LPVOID pvData, LPDWORD pcbData)
{
  DWORD dwRet, dwType, dwUnExpDataLen = 0, dwExpDataLen;

  TRACE("(hkey=0x%08x,%s,%p,%p,%p,%p=%ld)\n", hKey, debugstr_a(lpszValue),
        lpReserved, pwType, pvData, pcbData, pcbData ? *pcbData : 0);

  if (pcbData) dwUnExpDataLen = *pcbData;

  dwRet = RegQueryValueExA(hKey, lpszValue, lpReserved, &dwType, pvData, &dwUnExpDataLen);

  if (pcbData && (dwType == REG_EXPAND_SZ))
  {
    DWORD nBytesToAlloc;

    /* Expand type REG_EXPAND_SZ into REG_SZ */
    LPSTR szData;

    /* If the caller didn't supply a buffer or the buffer is to small we have
     * to allocate our own
     */
    if ((!pvData) || (dwRet == ERROR_MORE_DATA) )
    {
      char cNull = '\0';
      nBytesToAlloc = (!pvData || (dwRet == ERROR_MORE_DATA)) ? dwUnExpDataLen : *pcbData;

      szData = (LPSTR) LocalAlloc(GMEM_ZEROINIT, nBytesToAlloc);
      RegQueryValueExA (hKey, lpszValue, lpReserved, NULL, (LPBYTE)szData, &nBytesToAlloc);
      dwExpDataLen = ExpandEnvironmentStringsA(szData, &cNull, 1);
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree((HLOCAL) szData);
    }
    else
    {
      nBytesToAlloc = lstrlenA(pvData) * sizeof (CHAR);
      szData = (LPSTR) LocalAlloc(GMEM_ZEROINIT, nBytesToAlloc + 1);
      lstrcpyA(szData, pvData);
      dwExpDataLen = ExpandEnvironmentStringsA(szData, pvData, *pcbData / sizeof(CHAR));
      if (dwExpDataLen > *pcbData) dwRet = ERROR_MORE_DATA;
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree((HLOCAL) szData);
    }
  }

  /* Update the type and data size if the caller wanted them */
  if ( dwType == REG_EXPAND_SZ ) dwType = REG_SZ;
  if ( pwType ) *pwType = dwType;
  if ( pcbData ) *pcbData = dwUnExpDataLen;
  return dwRet;
}


/*************************************************************************
 * SHQueryValueExW   [SHLWAPI.@]
 *
 * See SHQueryValueExA.
 */
DWORD WINAPI SHQueryValueExW(HKEY hKey, LPCWSTR lpszValue,
                             LPDWORD lpReserved, LPDWORD pwType,
                             LPVOID pvData, LPDWORD pcbData)
{
  DWORD dwRet, dwType, dwUnExpDataLen = 0, dwExpDataLen;

  TRACE("(hkey=0x%08x,%s,%p,%p,%p,%p=%ld)\n", hKey, debugstr_w(lpszValue),
        lpReserved, pwType, pvData, pcbData, pcbData ? *pcbData : 0);

  if (pcbData) dwUnExpDataLen = *pcbData;

  dwRet = RegQueryValueExW(hKey, lpszValue, lpReserved, &dwType, pvData, &dwUnExpDataLen);

  if (pcbData && (dwType == REG_EXPAND_SZ))
  {
    DWORD nBytesToAlloc;

    /* Expand type REG_EXPAND_SZ into REG_SZ */
    LPWSTR szData;

    /* If the caller didn't supply a buffer or the buffer is to small we have
     * to allocate our own
     */
    if ((!pvData) || (dwRet == ERROR_MORE_DATA) )
    {
      WCHAR cNull = '\0';
      nBytesToAlloc = (!pvData || (dwRet == ERROR_MORE_DATA)) ? dwUnExpDataLen : *pcbData;

      szData = (LPWSTR) LocalAlloc(GMEM_ZEROINIT, nBytesToAlloc);
      RegQueryValueExW (hKey, lpszValue, lpReserved, NULL, (LPBYTE)szData, &nBytesToAlloc);
      dwExpDataLen = ExpandEnvironmentStringsW(szData, &cNull, 1);
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree((HLOCAL) szData);
    }
    else
    {
      nBytesToAlloc = lstrlenW(pvData) * sizeof(WCHAR);
      szData = (LPWSTR) LocalAlloc(GMEM_ZEROINIT, nBytesToAlloc + 1);
      lstrcpyW(szData, pvData);
      dwExpDataLen = ExpandEnvironmentStringsW(szData, pvData, *pcbData/sizeof(WCHAR) );
      if (dwExpDataLen > *pcbData) dwRet = ERROR_MORE_DATA;
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree((HLOCAL) szData);
    }
  }

  /* Update the type and data size if the caller wanted them */
  if ( dwType == REG_EXPAND_SZ ) dwType = REG_SZ;
  if ( pwType ) *pwType = dwType;
  if ( pcbData ) *pcbData = dwUnExpDataLen;
  return dwRet;
}

/*************************************************************************
 * SHDeleteKeyA   [SHLWAPI.@]
 *
 * Delete a registry key and any sub keys/values present
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key to delete
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. The key is deleted.
 *   Failure: An error code from RegOpenKeyExA, RegQueryInfoKeyA,
 *          RegEnumKeyExA or RegDeleteKeyA.
 */
DWORD WINAPI SHDeleteKeyA(HKEY hKey, LPCSTR lpszSubKey)
{
  DWORD dwRet, dwKeyCount = 0, dwMaxSubkeyLen = 0, dwSize, i;
  CHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
  HKEY hSubKey = 0;

  TRACE("(hkey=0x%08x,%s)\n", hKey, debugstr_a(lpszSubKey));

  dwRet = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
  if(!dwRet)
  {
    /* Find how many subkeys there are */
    dwRet = RegQueryInfoKeyA(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                             &dwMaxSubkeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if(!dwRet)
    {
      dwMaxSubkeyLen++;
      if (dwMaxSubkeyLen > sizeof(szNameBuf))
        /* Name too big: alloc a buffer for it */
        lpszName = HeapAlloc(GetProcessHeap(), 0, dwMaxSubkeyLen*sizeof(CHAR));

      if(!lpszName)
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
      else
      {
        /* Recursively delete all the subkeys */
        for(i = 0; i < dwKeyCount && !dwRet; i++)
        {
          dwSize = dwMaxSubkeyLen;
          dwRet = RegEnumKeyExA(hSubKey, i, lpszName, &dwSize, NULL, NULL, NULL, NULL);
          if(!dwRet)
            dwRet = SHDeleteKeyA(hSubKey, lpszName);
        }
        if (lpszName != szNameBuf)
          HeapFree(GetProcessHeap(), 0, lpszName); /* Free buffer if allocated */
      }
    }

    RegCloseKey(hSubKey);
    if(!dwRet)
      dwRet = RegDeleteKeyA(hKey, lpszSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteKeyW   [SHLWAPI.@]
 *
 * See SHDeleteKeyA.
 */
DWORD WINAPI SHDeleteKeyW(HKEY hKey, LPCWSTR lpszSubKey)
{
  DWORD dwRet, dwKeyCount = 0, dwMaxSubkeyLen = 0, dwSize, i;
  WCHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
  HKEY hSubKey = 0;

  TRACE("(hkey=0x%08x,%s)\n", hKey, debugstr_w(lpszSubKey));

  dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
  if(!dwRet)
  {
    /* Find how many subkeys there are */
    dwRet = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                             &dwMaxSubkeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if(!dwRet)
    {
      dwMaxSubkeyLen++;
      if (dwMaxSubkeyLen > sizeof(szNameBuf)/sizeof(WCHAR))
        /* Name too big: alloc a buffer for it */
        lpszName = HeapAlloc(GetProcessHeap(), 0, dwMaxSubkeyLen*sizeof(WCHAR));

      if(!lpszName)
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
      else
      {
        /* Recursively delete all the subkeys */
        for(i = 0; i < dwKeyCount && !dwRet; i++)
        {
          dwSize = dwMaxSubkeyLen;
          dwRet = RegEnumKeyExW(hSubKey, i, lpszName, &dwSize, NULL, NULL, NULL, NULL);
          if(!dwRet)
            dwRet = SHDeleteKeyW(hSubKey, lpszName);
        }

        if (lpszName != szNameBuf)
          HeapFree(GetProcessHeap(), 0, lpszName); /* Free buffer if allocated */
      }
    }

    RegCloseKey(hSubKey);
    if(!dwRet)
      dwRet = RegDeleteKeyW(hKey, lpszSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteEmptyKeyA   [SHLWAPI.@]
 *
 * Delete a registry key with no sub keys.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key to delete
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. The key is deleted.
 *   Failure: If the key is not empty, returns ERROR_KEY_HAS_CHILDREN. Otherwise
 *          returns an error code from RegOpenKeyExA, RegQueryInfoKeyA or
 *          RegDeleteKeyA.
 */
DWORD WINAPI SHDeleteEmptyKeyA(HKEY hKey, LPCSTR lpszSubKey)
{
  DWORD dwRet, dwKeyCount = 0;
  HKEY hSubKey = 0;

  TRACE("(hkey=0x%08x,%s)\n", hKey, debugstr_a(lpszSubKey));

  dwRet = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
  if(!dwRet)
  {
    dwRet = RegQueryInfoKeyA(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    RegCloseKey(hSubKey);
    if(!dwRet)
    {
      if (!dwKeyCount)
        dwRet = RegDeleteKeyA(hKey, lpszSubKey);
      else
        dwRet = ERROR_KEY_HAS_CHILDREN;
    }
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteEmptyKeyW   [SHLWAPI.@]
 *
 * See SHDeleteEmptyKeyA.
 */
DWORD WINAPI SHDeleteEmptyKeyW(HKEY hKey, LPCWSTR lpszSubKey)
{
  DWORD dwRet, dwKeyCount = 0;
  HKEY hSubKey = 0;

  TRACE("(hkey=0x%08x, %s)\n", hKey, debugstr_w(lpszSubKey));

  dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
  if(!dwRet)
  {
    dwRet = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    RegCloseKey(hSubKey);
    if(!dwRet)
    {
      if (!dwKeyCount)
        dwRet = RegDeleteKeyW(hKey, lpszSubKey);
      else
        dwRet = ERROR_KEY_HAS_CHILDREN;
    }
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteOrphanKeyA   [SHLWAPI.@]
 *
 * Delete a registry key with no sub keys or values.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key to possibly delete
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. The key has been deleted if it was an orphan.
 *   Failure: An error from RegOpenKeyExA, RegQueryValueExA, or RegDeleteKeyA.
 */
DWORD WINAPI SHDeleteOrphanKeyA(HKEY hKey, LPCSTR lpszSubKey)
{
  HKEY hSubKey;
  DWORD dwKeyCount = 0, dwValueCount = 0, dwRet;

  TRACE("(hkey=0x%08x,%s)\n", hKey, debugstr_a(lpszSubKey));

  dwRet = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);

  if(!dwRet)
  {
    /* Get subkey and value count */
    dwRet = RegQueryInfoKeyA(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                             NULL, NULL, &dwValueCount, NULL, NULL, NULL, NULL);

    if(!dwRet && !dwKeyCount && !dwValueCount)
    {
      dwRet = RegDeleteKeyA(hKey, lpszSubKey);
    }
    RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteOrphanKeyW   [SHLWAPI.@]
 *
 * See SHDeleteOrphanKeyA.
 */
DWORD WINAPI SHDeleteOrphanKeyW(HKEY hKey, LPCWSTR lpszSubKey)
{
  HKEY hSubKey;
  DWORD dwKeyCount = 0, dwValueCount = 0, dwRet;

  TRACE("(hkey=0x%08x,%s)\n", hKey, debugstr_w(lpszSubKey));

  dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);

  if(!dwRet)
  {
    /* Get subkey and value count */
    dwRet = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                             NULL, NULL, &dwValueCount, NULL, NULL, NULL, NULL);

    if(!dwRet && !dwKeyCount && !dwValueCount)
    {
      dwRet = RegDeleteKeyW(hKey, lpszSubKey);
    }
    RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteValueA   [SHLWAPI.@]
 *
 * Delete a value from the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key containing value to delete
 *   lpszValue  [I] Name of value to delete
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. The value is deleted.
 *   Failure: An error code from RegOpenKeyExA or RegDeleteValueA.
 */
DWORD WINAPI SHDeleteValueA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue)
{
  DWORD dwRet;
  HKEY hSubKey;

  TRACE("(hkey=0x%08x,%s,%s)\n", hKey, debugstr_a(lpszSubKey), debugstr_a(lpszValue));

  dwRet = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_SET_VALUE, &hSubKey);
  if (!dwRet)
  {
    dwRet = RegDeleteValueA(hSubKey, lpszValue);
    RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteValueW   [SHLWAPI.@]
 *
 * See SHDeleteValueA.
 */
DWORD WINAPI SHDeleteValueW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue)
{
  DWORD dwRet;
  HKEY hSubKey;

  TRACE("(hkey=0x%08x,%s,%s)\n", hKey, debugstr_w(lpszSubKey), debugstr_w(lpszValue));

  dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_SET_VALUE, &hSubKey);
  if (!dwRet)
  {
    dwRet = RegDeleteValueW(hSubKey, lpszValue);
    RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHEnumKeyExA   [SHLWAPI.@]
 *
 * Enumerate sub keys in a registry key.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   dwIndex    [I] Index of key to enumerate
 *   lpszSubKey [O] Pointer updated with the subkey name
 *   pwLen      [O] Pointer updated with the subkey length
 *
 * RETURN
 *   Success: ERROR_SUCCESS. lpszSubKey and pwLen are updated.
 *   Failure: An error code from RegEnumKeyExA.
 */
LONG WINAPI SHEnumKeyExA(HKEY hKey, DWORD dwIndex, LPSTR lpszSubKey,
                         LPDWORD pwLen)
{
  TRACE("(hkey=0x%08x,%ld,%s,%p)\n", hKey, dwIndex, debugstr_a(lpszSubKey), pwLen);

  return RegEnumKeyExA(hKey, dwIndex, lpszSubKey, pwLen, NULL, NULL, NULL, NULL);
}

/*************************************************************************
 * SHEnumKeyExW   [SHLWAPI.@]
 *
 * See SHEnumKeyExA.
 */
LONG WINAPI SHEnumKeyExW(HKEY hKey, DWORD dwIndex, LPWSTR lpszSubKey,
                         LPDWORD pwLen)
{
  TRACE("(hkey=0x%08x,%ld,%s,%p)\n", hKey, dwIndex, debugstr_w(lpszSubKey), pwLen);

  return RegEnumKeyExW(hKey, dwIndex, lpszSubKey, pwLen, NULL, NULL, NULL, NULL);
}

/*************************************************************************
 * SHEnumValueA   [SHLWAPI.@]
 *
 * Enumerate values in a registry key.
 *
 * PARAMS
 *   hKey      [I] Handle to registry key
 *   dwIndex   [I] Index of key to enumerate
 *   lpszValue [O] Pointer updated with the values name
 *   pwLen     [O] Pointer updated with the values length
 *   pwType    [O] Pointer updated with the values type
 *   pvData    [O] Pointer updated with the values data
 *   pcbData   [O] Pointer updated with the values size
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. Output parameters are updated.
 *   Failure: An error code from RegEnumValueExA.
 */
LONG WINAPI SHEnumValueA(HKEY hKey, DWORD dwIndex, LPSTR lpszValue,
                         LPDWORD pwLen, LPDWORD pwType,
                         LPVOID pvData, LPDWORD pcbData)
{
  TRACE("(hkey=0x%08x,%ld,%s,%p,%p,%p,%p)\n", hKey, dwIndex,
        debugstr_a(lpszValue), pwLen, pwType, pvData, pcbData);

 return RegEnumValueA(hKey, dwIndex, lpszValue, pwLen, NULL,
                      pwType, pvData, pcbData);
}

/*************************************************************************
 * SHEnumValueW   [SHLWAPI.@]
 *
 * See SHEnumValueA.
 */
LONG WINAPI SHEnumValueW(HKEY hKey, DWORD dwIndex, LPWSTR lpszValue,
                         LPDWORD pwLen, LPDWORD pwType,
                         LPVOID pvData, LPDWORD pcbData)
{
  TRACE("(hkey=0x%08x,%ld,%s,%p,%p,%p,%p)\n", hKey, dwIndex,
        debugstr_w(lpszValue), pwLen, pwType, pvData, pcbData);

  return RegEnumValueW(hKey, dwIndex, lpszValue, pwLen, NULL,
                       pwType, pvData, pcbData);
}

/*************************************************************************
 * @   [SHLWAPI.205]
 *
 * Wrapper for SHGetValueA in case machine is in safe mode.
 */
DWORD WINAPI SHLWAPI_205(HKEY hkey, LPCSTR pSubKey, LPCSTR pValue,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pbData)
{
  if (GetSystemMetrics(SM_CLEANBOOT))
    return ERROR_INVALID_FUNCTION;
  return SHGetValueA(hkey, pSubKey, pValue, pwType, pvData, pbData);
}

/*************************************************************************
 * @   [SHLWAPI.206]
 *
 * Unicode version of SHLWAPI_205.
 */
DWORD WINAPI SHLWAPI_206(HKEY hkey, LPCWSTR pSubKey, LPCWSTR pValue,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pbData)
{
  if (GetSystemMetrics(SM_CLEANBOOT))
    return ERROR_INVALID_FUNCTION;
  return SHGetValueW(hkey, pSubKey, pValue, pwType, pvData, pbData);
}

/*************************************************************************
 * @   [SHLWAPI.320]
 *
 * Set a content type in the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key under hKey
 *   lpszValue  [I] Value to set
 *
 * RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL WINAPI SHLWAPI_320(LPCSTR lpszSubKey, LPCSTR lpszValue)
{
  DWORD dwRet;

  if (!lpszValue)
  {
    WARN("Invalid lpszValue would crash under Win32!\n");
    return FALSE;
  }

  dwRet = SHSetValueA(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeA,
                      REG_SZ, lpszValue, strlen(lpszValue));
  return dwRet ? FALSE : TRUE;
}

/*************************************************************************
 * @   [SHLWAPI.321]
 *
 * Unicode version of SHLWAPI_320.
 */
BOOL WINAPI SHLWAPI_321(LPCWSTR lpszSubKey, LPCWSTR lpszValue)
{
  DWORD dwRet;

  if (!lpszValue)
  {
    WARN("Invalid lpszValue would crash under Win32!\n");
    return FALSE;
  }

  dwRet = SHSetValueW(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeW,
                      REG_SZ, lpszValue, strlenW(lpszValue));
  return dwRet ? FALSE : TRUE;
}

/*************************************************************************
 * @   [SHLWAPI.322]
 *
 * Delete a content type from the registry.
 *
 * PARAMS
 *   lpszSubKey [I] Name of sub key
 *
 * RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL WINAPI SHLWAPI_322(LPCSTR lpszSubKey)
{
  HRESULT ret = SHDeleteValueA(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeA);
  return ret ? FALSE : TRUE;
}

/*************************************************************************
 * @   [SHLWAPI.323]
 *
 * Unicode version of SHLWAPI_322.
 */
BOOL WINAPI SHLWAPI_323(LPCWSTR lpszSubKey)
{
  HRESULT ret = SHDeleteValueW(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeW);
  return ret ? FALSE : TRUE;
}


/*************************************************************************
 * SHRegDuplicateHKey   [SHLWAPI.@]
 */
HKEY WINAPI SHRegDuplicateHKey(HKEY hKey)
{
    HKEY newKey = 0;

    RegOpenKeyExA(hKey, 0, 0, MAXIMUM_ALLOWED, &newKey);
    TRACE("new key is %08x\n", newKey);
    return newKey;
}
