/*
 * Regster/Unregister servers. (for internal use)
 *
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "uuids.h"
#include "wine/unicode.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "regsvr.h"

#ifndef	NUMELEMS
#define	NUMELEMS(elem)	(sizeof(elem)/sizeof((elem)[0]))
#endif	/* NUMELEMS */

const WCHAR QUARTZ_wszREG_SZ[] =
	{'R','E','G','_','S','Z',0};
const WCHAR QUARTZ_wszInprocServer32[] =
	{'I','n','p','r','o','c','S','e','r','v','e','r','3','2',0};
const WCHAR QUARTZ_wszThreadingModel[] =
	{'T','h','r','e','a','d','i','n','g','M','o','d','e','l',0};
const WCHAR QUARTZ_wszBoth[] =
	{'B','o','t','h',0};
const WCHAR QUARTZ_wszCLSID[] =
	{'C','L','S','I','D',0};
const WCHAR QUARTZ_wszFilterData[] =
	{'F','i','l','t','e','r',' ','D','a','t','a',0};
const WCHAR QUARTZ_wszFriendlyName[] =
	{'F','r','i','e','n','d','l','y','N','a','m','e',0};
const WCHAR QUARTZ_wszInstance[] =
	{'I','n','s','t','a','n','c','e',0};
const WCHAR QUARTZ_wszMerit[] =
	{'M','e','r','i','t',0};

static
void QUARTZ_CatPathSepW( WCHAR* pBuf )
{
	int	len = strlenW(pBuf);
	pBuf[len] = '\\';
	pBuf[len+1] = 0;
}

static
void QUARTZ_GUIDtoString( WCHAR* pBuf, const GUID* pguid )
{
	/* W"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}" */
	static const WCHAR wszFmt[] =
		{'{','%','0','8','X','-','%','0','4','X','-','%','0','4','X',
		 '-','%','0','2','X','%','0','2','X','-','%','0','2','X','%',
		 '0','2','X','%','0','2','X','%','0','2','X','%','0','2','X',
		 '%','0','2','X','}',0};

	wsprintfW( pBuf, wszFmt,
		pguid->Data1, pguid->Data2, pguid->Data3,
		pguid->Data4[0], pguid->Data4[1],
		pguid->Data4[2], pguid->Data4[3],
		pguid->Data4[4], pguid->Data4[5],
		pguid->Data4[6], pguid->Data4[7] );
}

static
LONG QUARTZ_RegOpenKeyW(
	HKEY hkRoot, LPCWSTR lpszPath,
	REGSAM rsAccess, HKEY* phKey,
	BOOL fCreateKey )
{
	DWORD	dwDisp;
	WCHAR	wszREG_SZ[ NUMELEMS(QUARTZ_wszREG_SZ) ];

	memcpy(wszREG_SZ,QUARTZ_wszREG_SZ,sizeof(QUARTZ_wszREG_SZ) );

	if ( fCreateKey )
		return RegCreateKeyExW(
			hkRoot, lpszPath, 0, wszREG_SZ,
			REG_OPTION_NON_VOLATILE, rsAccess, NULL, phKey, &dwDisp );
	else
		return RegOpenKeyExW(
			hkRoot, lpszPath, 0, rsAccess, phKey );
}

static
LONG QUARTZ_RegSetValueString(
	HKEY hKey, LPCWSTR lpszName, LPCWSTR lpValue )
{
	return RegSetValueExW(
		hKey, lpszName, 0, REG_SZ,
		(const BYTE*)lpValue,
		sizeof(lpValue[0]) * (strlenW(lpValue)+1) );
}

static
LONG QUARTZ_RegSetValueDWord(
	HKEY hKey, LPCWSTR lpszName, DWORD dwValue )
{
	return RegSetValueExW(
		hKey, lpszName, 0, REG_DWORD,
		(const BYTE*)(&dwValue), sizeof(DWORD) );
}

static
LONG QUARTZ_RegSetValueBinary(
	HKEY hKey, LPCWSTR lpszName,
	const BYTE* pData, int iLenOfData )
{
	return RegSetValueExW(
		hKey, lpszName, 0, REG_BINARY, pData, iLenOfData );
}

HRESULT QUARTZ_CreateCLSIDPath(
        WCHAR* pwszBuf, DWORD dwBufLen,
        const CLSID* pclsid,
        LPCWSTR lpszPathFromCLSID )
{
	int avail;

	strcpyW( pwszBuf, QUARTZ_wszCLSID );
	QUARTZ_CatPathSepW( pwszBuf+5 );
	QUARTZ_GUIDtoString( pwszBuf+6, pclsid );
	if ( lpszPathFromCLSID != NULL )
	{
		avail = (int)dwBufLen - strlenW(pwszBuf) - 8;
		if ( avail <= strlenW(lpszPathFromCLSID) )
			return E_FAIL;
		QUARTZ_CatPathSepW( pwszBuf );
		strcatW( pwszBuf, lpszPathFromCLSID );
	}

	return NOERROR;
}

HRESULT QUARTZ_OpenCLSIDKey(
	HKEY* phKey,	/* [OUT] hKey */
	REGSAM rsAccess,	/* [IN] access */
	BOOL fCreate,	/* TRUE = RegCreateKey, FALSE = RegOpenKey */
	const CLSID* pclsid,	/* CLSID */
	LPCWSTR lpszPathFromCLSID )	/* related path from CLSID */
{
	WCHAR	szKey[ 1024 ];
	HRESULT	hr;
	LONG	lr;

	hr = QUARTZ_CreateCLSIDPath(
		szKey, NUMELEMS(szKey),
		pclsid, lpszPathFromCLSID );
	if ( FAILED(hr) )
		return hr;

	lr = QUARTZ_RegOpenKeyW(
		HKEY_CLASSES_ROOT, szKey, rsAccess, phKey, fCreate );
	if ( lr != ERROR_SUCCESS )
		return E_FAIL;

	return S_OK;
}



HRESULT QUARTZ_RegisterAMovieDLLServer(
	const CLSID* pclsid,	/* [IN] CLSID */
	LPCWSTR lpFriendlyName,	/* [IN] Friendly name */
	LPCWSTR lpNameOfDLL,	/* [IN] name of the registered DLL */
	BOOL fRegister )	/* [IN] TRUE = register, FALSE = unregister */
{
	HRESULT	hr;
	HKEY	hKey;

	if ( fRegister )
	{
		hr = QUARTZ_OpenCLSIDKey(
			&hKey, KEY_ALL_ACCESS, TRUE,
			pclsid, NULL );
		if ( FAILED(hr) )
			return hr;

			if ( lpFriendlyName != NULL && QUARTZ_RegSetValueString(
				hKey, NULL, lpFriendlyName ) != ERROR_SUCCESS )
				hr = E_FAIL;

		RegCloseKey( hKey );
		if ( FAILED(hr) )
			return hr;

		hr = QUARTZ_OpenCLSIDKey(
			&hKey, KEY_ALL_ACCESS, TRUE,
			pclsid, QUARTZ_wszInprocServer32 );
		if ( FAILED(hr) )
			return hr;

			if ( QUARTZ_RegSetValueString(
				hKey, NULL, lpNameOfDLL ) != ERROR_SUCCESS )
				hr = E_FAIL;
			if ( QUARTZ_RegSetValueString(
				hKey, QUARTZ_wszThreadingModel,
				QUARTZ_wszBoth ) != ERROR_SUCCESS )
				hr = E_FAIL;

		RegCloseKey( hKey );
		if ( FAILED(hr) )
			return hr;
	}
	else
	{
		hr = QUARTZ_OpenCLSIDKey(
			&hKey, KEY_ALL_ACCESS, FALSE,
			pclsid, NULL );
		if ( FAILED(hr) )
			return NOERROR;

			RegDeleteValueW( hKey, NULL );
			RegDeleteValueW( hKey, QUARTZ_wszThreadingModel );

		RegCloseKey( hKey );
		if ( FAILED(hr) )
			return hr;

		/* I think key should be deleted only if no subkey exists. */
		FIXME( "unregister %s - key should be removed!\n",
				debugstr_guid(pclsid) );
	}

	return NOERROR;
}


HRESULT QUARTZ_RegisterCategory(
	const CLSID* pguidFilterCategory,	/* [IN] Category */
	LPCWSTR lpFriendlyName,	/* [IN] friendly name */
	DWORD dwMerit,	/* [IN] merit */
	BOOL fRegister )	/* [IN] TRUE = register, FALSE = unregister */
{
	HRESULT	hr;
	HKEY	hKey;
	WCHAR	szFilterPath[ 256 ];
	WCHAR	szCLSID[ 256 ];

	QUARTZ_GUIDtoString( szCLSID, pguidFilterCategory );
	strcpyW( szFilterPath, QUARTZ_wszInstance );
	QUARTZ_CatPathSepW( szFilterPath );
	strcatW( szFilterPath, szCLSID );

	if ( fRegister )
	{
		hr = QUARTZ_OpenCLSIDKey(
			&hKey, KEY_ALL_ACCESS, TRUE,
			&CLSID_ActiveMovieCategories, szFilterPath );
		if ( FAILED(hr) )
			return hr;

			if ( QUARTZ_RegSetValueString(
				hKey, QUARTZ_wszCLSID, szCLSID ) != ERROR_SUCCESS )
				hr = E_FAIL;
			if ( lpFriendlyName != NULL && QUARTZ_RegSetValueString(
				hKey, QUARTZ_wszFriendlyName,
				lpFriendlyName ) != ERROR_SUCCESS )
				hr = E_FAIL;
			if ( dwMerit != 0 &&
				 QUARTZ_RegSetValueDWord(
				 	hKey, QUARTZ_wszMerit, dwMerit ) != ERROR_SUCCESS )
				hr = E_FAIL;

		RegCloseKey( hKey );
		if ( FAILED(hr) )
			return hr;
	}
	else
	{
		hr = QUARTZ_OpenCLSIDKey(
			&hKey, KEY_ALL_ACCESS, FALSE,
			&CLSID_ActiveMovieCategories, szFilterPath );
		if ( FAILED(hr) )
			return NOERROR;

			RegDeleteValueW( hKey, QUARTZ_wszCLSID );
			RegDeleteValueW( hKey, QUARTZ_wszFriendlyName );
			RegDeleteValueW( hKey, QUARTZ_wszMerit );

		RegCloseKey( hKey );
		if ( FAILED(hr) )
			return hr;

		/* I think key should be deleted only if no subkey exists. */
		FIXME( "unregister category %s - key should be removed!\n",
			debugstr_guid(pguidFilterCategory) );
	}

	return NOERROR;
}


HRESULT QUARTZ_RegisterAMovieFilter(
	const CLSID* pguidFilterCategory,	/* [IN] Category */
	const CLSID* pclsid,	/* [IN] CLSID of this filter */
	const BYTE* pbFilterData,	/* [IN] filter data(no spec) */
	DWORD cbFilterData,	/* [IN] size of the filter data */
	LPCWSTR lpFriendlyName,	/* [IN] friendly name */
	LPCWSTR lpInstance,	/* [IN] instance */
	BOOL fRegister )	/* [IN] TRUE = register, FALSE = unregister */
{
	HRESULT	hr;
	HKEY	hKey;
	WCHAR	szFilterPath[ 256 ];
	WCHAR	szCLSID[ 256 ];

	QUARTZ_GUIDtoString( szCLSID, pclsid );
	strcpyW( szFilterPath, QUARTZ_wszInstance );
	QUARTZ_CatPathSepW( szFilterPath );
	strcatW( szFilterPath, ( lpInstance != NULL ) ? lpInstance : szCLSID );

	if ( fRegister )
	{
		hr = QUARTZ_OpenCLSIDKey(
			&hKey, KEY_ALL_ACCESS, TRUE,
			pguidFilterCategory, szFilterPath );
		if ( FAILED(hr) )
			return hr;

			if ( QUARTZ_RegSetValueString(
				hKey, QUARTZ_wszCLSID, szCLSID ) != ERROR_SUCCESS )
				hr = E_FAIL;
			if ( pbFilterData != NULL && cbFilterData > 0 &&
				 QUARTZ_RegSetValueBinary(
				 	hKey, QUARTZ_wszFilterData,
				 	pbFilterData, cbFilterData ) != ERROR_SUCCESS )
				hr = E_FAIL;
			if ( lpFriendlyName != NULL && QUARTZ_RegSetValueString(
				hKey, QUARTZ_wszFriendlyName,
				lpFriendlyName ) != ERROR_SUCCESS )
				hr = E_FAIL;

		RegCloseKey( hKey );
		if ( FAILED(hr) )
			return hr;
	}
	else
	{
		hr = QUARTZ_OpenCLSIDKey(
			&hKey, KEY_ALL_ACCESS, FALSE,
			pguidFilterCategory, szFilterPath );
		if ( FAILED(hr) )
			return NOERROR;

			RegDeleteValueW( hKey, QUARTZ_wszCLSID );
			RegDeleteValueW( hKey, QUARTZ_wszFilterData );
			RegDeleteValueW( hKey, QUARTZ_wszFriendlyName );

		RegCloseKey( hKey );
		if ( FAILED(hr) )
			return hr;

		/* I think key should be deleted only if no subkey exists. */
		FIXME( "unregister category %s filter %s - key should be removed!\n",
			debugstr_guid(pguidFilterCategory),
			debugstr_guid(pclsid) );
	}

	return NOERROR;
}



