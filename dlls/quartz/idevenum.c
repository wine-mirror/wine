/*
 * Implementation of ICreateDevEnum for CLSID_SystemDeviceEnum.
 *
 * FIXME - stub.
 *
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "objidl.h"
#include "oleidl.h"
#include "ocidl.h"
#include "strmif.h"
#include "control.h"
#include "uuids.h"
#include "wine/unicode.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "devenum.h"
#include "regsvr.h"
#include "enumunk.h"
#include "complist.h"
#include "devmon.h"


static HRESULT WINAPI
ICreateDevEnum_fnQueryInterface(ICreateDevEnum* iface,REFIID riid,void** ppobj)
{
	CSysDevEnum_THIS(iface,createdevenum);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
ICreateDevEnum_fnAddRef(ICreateDevEnum* iface)
{
	CSysDevEnum_THIS(iface,createdevenum);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
ICreateDevEnum_fnRelease(ICreateDevEnum* iface)
{
	CSysDevEnum_THIS(iface,createdevenum);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
ICreateDevEnum_fnCreateClassEnumerator(ICreateDevEnum* iface,REFCLSID rclsidDeviceClass,IEnumMoniker** ppobj, DWORD dwFlags)
{
	CSysDevEnum_THIS(iface,createdevenum);
	HRESULT	hr;
	HKEY	hKey;
	QUARTZ_CompList*	pMonList;
	IMoniker*	pMon;
	DWORD	dwIndex;
	LONG	lr;
	WCHAR	wszPath[ 1024 ];
	DWORD	dwLen;
	DWORD	dwNameMax;
	DWORD	cbName;
	FILETIME	ftLastWrite;

	TRACE("(%p)->(%s,%p,%08lx)\n",This,
		debugstr_guid(rclsidDeviceClass),ppobj,dwFlags);
	if ( dwFlags != 0 )
	{
		FIXME("unknown flags %08lx\n",dwFlags);
		return E_NOTIMPL;
	}

	if ( ppobj == NULL )
		return E_POINTER;
	*ppobj = NULL;

	hr = QUARTZ_CreateCLSIDPath(
		wszPath, sizeof(wszPath)/sizeof(wszPath[0]),
		rclsidDeviceClass, QUARTZ_wszInstance );
	if ( FAILED(hr) )
		return hr;

	if ( RegOpenKeyExW( HKEY_CLASSES_ROOT, wszPath,
		0, KEY_READ, &hKey ) != ERROR_SUCCESS )
		return E_FAIL;

	dwLen = strlenW(wszPath);
	wszPath[dwLen++] = '\\'; wszPath[dwLen] = 0;
	dwNameMax = sizeof(wszPath)/sizeof(wszPath[0]) - dwLen - 8;

	pMonList = QUARTZ_CompList_Alloc();
	if ( pMonList == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto err;
	}

	/* enumerate all subkeys. */
	dwIndex = 0;
	while ( 1 )
	{
		cbName = dwNameMax;
		lr = RegEnumKeyExW(
			hKey, dwIndex, &wszPath[dwLen], &cbName,
			NULL, NULL, NULL, &ftLastWrite );
		if ( lr == ERROR_NO_MORE_ITEMS )
			break;
		if ( lr != ERROR_SUCCESS )
		{
			hr = E_FAIL;
			goto err;
		}

		hr = QUARTZ_CreateDeviceMoniker(
			HKEY_CLASSES_ROOT, wszPath, &pMon );
		if ( FAILED(hr) )
			goto err;

		hr = QUARTZ_CompList_AddComp(
			pMonList, (IUnknown*)pMon, NULL, 0 );
		IMoniker_Release( pMon );

		if ( FAILED(hr) )
			goto err;

		dwIndex ++;
	}

	/* create an enumerator. */
	hr = QUARTZ_CreateEnumUnknown(
		&IID_IEnumMoniker, (void**)ppobj, pMonList );
	if ( FAILED(hr) )
		goto err;

	hr = S_OK;
err:
	if ( pMonList != NULL )
		QUARTZ_CompList_Free( pMonList );
	RegCloseKey( hKey );

	return hr;
}

static ICOM_VTABLE(ICreateDevEnum) icreatedevenum =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	ICreateDevEnum_fnQueryInterface,
	ICreateDevEnum_fnAddRef,
	ICreateDevEnum_fnRelease,
	/* ICreateDevEnum fields */
	ICreateDevEnum_fnCreateClassEnumerator,
};

HRESULT CSysDevEnum_InitICreateDevEnum( CSysDevEnum* psde )
{
	TRACE("(%p)\n",psde);
	ICOM_VTBL(&psde->createdevenum) = &icreatedevenum;

	return NOERROR;
}

void CSysDevEnum_UninitICreateDevEnum( CSysDevEnum* psde )
{
	TRACE("(%p)\n",psde);
}
