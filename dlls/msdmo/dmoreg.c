/*
 * Implements dmoreg APIs.
 *
 * Copyright (C) Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
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
 *
 *	FIXME - stub.
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "mediaobj.h"
#include "dmoreg.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msdmo);


/*

  NOTE: You may build this dll as a native dll and
        use with native non-DirectX8 environment(NT 4.0 etc).

 */

static const WCHAR wszDMOPath[] =
{'D','i','r','e','c','t','S','h','o','w','\\',
 'M','e','d','i','a','O','b','j','e','c','t','s',0};
static const WCHAR wszDMOCatPath[] =
{'D','i','r','e','c','t','S','h','o','w','\\',
 'M','e','d','i','a','O','b','j','e','c','t','s','\\',
 'C','a','t','e','g','o','r','i','e','s',0};
static const WCHAR wszInputTypes[] =
{'I','n','p','u','t','T','y','p','e','s',0};
static const WCHAR wszOutputTypes[] =
{'O','u','t','p','u','t','T','y','p','e','s',0};

static const WCHAR QUARTZ_wszREG_SZ[] = {'R','E','G','_','S','Z',0};

/*************************************************************************/


static void QUARTZ_CatPathSepW( WCHAR* pBuf )
{
	int	len = lstrlenW(pBuf);
	pBuf[len] = '\\';
	pBuf[len+1] = 0;
}

static void QUARTZ_GUIDtoString( WCHAR* pBuf, const GUID* pguid )
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
	WCHAR	wszREG_SZ[ sizeof(QUARTZ_wszREG_SZ)/sizeof(QUARTZ_wszREG_SZ[0]) ];

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
		sizeof(lpValue[0]) * (lstrlenW(lpValue)+1) );
}

static
LONG QUARTZ_RegSetValueBinary(
	HKEY hKey, LPCWSTR lpszName,
	const BYTE* pData, int iLenOfData )
{
	return RegSetValueExW(
		hKey, lpszName, 0, REG_BINARY, pData, iLenOfData );
}


/*************************************************************************/


HRESULT WINAPI DMOEnum( REFGUID rguidCat, DWORD dwFlags, DWORD dwCountOfInTypes, const DMO_PARTIAL_MEDIATYPE* pInTypes, DWORD dwCountOfOutTypes, const DMO_PARTIAL_MEDIATYPE* pOutTypes, IEnumDMO** ppEnum )
{
	FIXME( "stub!\n" );
	return E_NOTIMPL;
}

HRESULT WINAPI DMOGetName( REFCLSID rclsid, WCHAR* pwszName )
{
	FIXME( "stub!\n" );
	return E_NOTIMPL;
}


HRESULT WINAPI DMOGetTypes( REFCLSID rclsid, unsigned long ulInputTypesReq, unsigned long* pulInputTypesRet, unsigned long ulOutputTypesReq, unsigned long* pulOutputTypesRet, const DMO_PARTIAL_MEDIATYPE* pOutTypes )
{
	FIXME( "stub!\n" );
	return E_NOTIMPL;
}

HRESULT WINAPI DMOGuidToStrA( void* pv1, void* pv2 )
{
	FIXME( "(%p,%p) stub!\n", pv1, pv2 );
	return E_NOTIMPL;
}

HRESULT WINAPI DMOGuidToStrW( void* pv1, void* pv2 )
{
	FIXME( "(%p,%p) stub!\n", pv1, pv2 );
	return E_NOTIMPL;
}

HRESULT WINAPI DMORegister( LPCWSTR pwszName, REFCLSID rclsid, REFGUID rguidCat, DWORD dwFlags, DWORD dwCountOfInTypes, const DMO_PARTIAL_MEDIATYPE* pInTypes, DWORD dwCountOfOutTypes, const DMO_PARTIAL_MEDIATYPE* pOutTypes )
{
	HRESULT hr;
	HKEY	hKey;
	WCHAR	wszPath[1024];

	FIXME( "() not tested!\n" );

	hr = S_OK;

	memcpy(wszPath,wszDMOPath,sizeof(wszDMOPath));
	QUARTZ_CatPathSepW(wszPath);
	QUARTZ_GUIDtoString(&wszPath[lstrlenW(wszPath)],rclsid);
	if ( QUARTZ_RegOpenKeyW( HKEY_CLASSES_ROOT,
		wszPath, KEY_ALL_ACCESS, &hKey, TRUE ) != ERROR_SUCCESS )
		return E_FAIL;
	if ( pwszName != NULL && QUARTZ_RegSetValueString(
			hKey, NULL, pwszName ) != ERROR_SUCCESS )
		hr = E_FAIL;
	if ( dwCountOfInTypes > 0 && QUARTZ_RegSetValueBinary(
		hKey, wszInputTypes, (const BYTE*)pInTypes,
		dwCountOfInTypes * sizeof(DMO_PARTIAL_MEDIATYPE) ) != ERROR_SUCCESS )
		hr = E_FAIL;
	if ( dwCountOfOutTypes > 0 && QUARTZ_RegSetValueBinary(
		hKey, wszOutputTypes, (const BYTE*)pOutTypes,
		dwCountOfOutTypes * sizeof(DMO_PARTIAL_MEDIATYPE) ) != ERROR_SUCCESS )
		hr = E_FAIL;
	RegCloseKey( hKey );
	if ( FAILED(hr) )
		return hr;

	memcpy(wszPath,wszDMOCatPath,sizeof(wszDMOCatPath));
	QUARTZ_CatPathSepW(wszPath);
	QUARTZ_GUIDtoString(&wszPath[lstrlenW(wszPath)],rguidCat);
	QUARTZ_CatPathSepW(wszPath);
	QUARTZ_GUIDtoString(&wszPath[lstrlenW(wszPath)],rclsid);
	if ( QUARTZ_RegOpenKeyW( HKEY_CLASSES_ROOT,
		wszPath, KEY_ALL_ACCESS, &hKey, TRUE ) != ERROR_SUCCESS )
		return E_FAIL;
	RegCloseKey( hKey );
	if ( FAILED(hr) )
		return hr;

	return S_OK;
}

HRESULT WINAPI DMOStrToGuidA( void* pv1, void* pv2 )
{
	FIXME( "(%p,%p) stub!\n", pv1, pv2 );
	return E_NOTIMPL;
}

HRESULT WINAPI DMOStrToGuidW( void* pv1, void* pv2 )
{
	FIXME( "(%p,%p) stub!\n", pv1, pv2 );
	return E_NOTIMPL;
}

HRESULT WINAPI DMOUnregister( REFCLSID rclsid, REFGUID rguidCat )
{
	FIXME( "stub!\n" );

	return E_NOTIMPL;
}



