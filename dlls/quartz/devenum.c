/*
 * Implementation of CLSID_SystemDeviceEnum.
 * Implements IMoniker for CLSID_CDeviceMoniker.
 * Implements IPropertyBag. (internal)
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
#include "objidl.h"
#include "oleidl.h"
#include "ocidl.h"
#include "oleauto.h"
#include "strmif.h"
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "devenum.h"
#include "enumunk.h"
#include "complist.h"
#include "regsvr.h"

#ifndef NUMELEMS
#define NUMELEMS(elem)	(sizeof(elem)/sizeof(elem[0]))
#endif	/* NUMELEMS */

/***************************************************************************
 *
 *	new/delete for CLSID_SystemDeviceEnum
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry CSysDevEnum_IFEntries[] =
{
  { &IID_ICreateDevEnum, offsetof(CSysDevEnum,createdevenum)-offsetof(CSysDevEnum,unk) },
};


static void QUARTZ_DestroySystemDeviceEnum(IUnknown* punk)
{
	CSysDevEnum_THIS(punk,unk);

	CSysDevEnum_UninitICreateDevEnum( This );
}

HRESULT QUARTZ_CreateSystemDeviceEnum(IUnknown* punkOuter,void** ppobj)
{
	CSysDevEnum*	psde;
	HRESULT	hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	psde = (CSysDevEnum*)QUARTZ_AllocObj( sizeof(CSysDevEnum) );
	if ( psde == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &psde->unk, punkOuter );

	hr = CSysDevEnum_InitICreateDevEnum( psde );
	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj( psde );
		return hr;
	}

	psde->unk.pEntries = CSysDevEnum_IFEntries;
	psde->unk.dwEntries = sizeof(CSysDevEnum_IFEntries)/sizeof(CSysDevEnum_IFEntries[0]);
	psde->unk.pOnFinalRelease = QUARTZ_DestroySystemDeviceEnum;

	*ppobj = (void*)(&psde->unk);

	return S_OK;
}


/***************************************************************************
 *
 *	CSysDevEnum::ICreateDevEnum
 *
 */


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
		wszPath, sizeof(wszPath)/sizeof(wszPath[0]) - 16,
		rclsidDeviceClass, QUARTZ_wszInstance );
	if ( FAILED(hr) )
		return hr;

	lr = RegOpenKeyExW( HKEY_CLASSES_ROOT, wszPath,
		0, KEY_READ, &hKey );
	if ( lr != ERROR_SUCCESS )
	{
		TRACE("cannot open %s\n",debugstr_w(wszPath));
		if ( lr == ERROR_FILE_NOT_FOUND ||
			 lr == ERROR_PATH_NOT_FOUND )
			return S_FALSE;
		return E_FAIL;
	}

	dwLen = lstrlenW(wszPath);
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
			TRACE("RegEnumKeyEx returns %08lx\n",lr);
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


/***************************************************************************
 *
 *	CDeviceMoniker::IMoniker
 *
 */

static HRESULT WINAPI
IMoniker_fnQueryInterface(IMoniker* iface,REFIID riid,void** ppobj)
{
	CDeviceMoniker_THIS(iface,moniker);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IMoniker_fnAddRef(IMoniker* iface)
{
	CDeviceMoniker_THIS(iface,moniker);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IMoniker_fnRelease(IMoniker* iface)
{
	CDeviceMoniker_THIS(iface,moniker);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI IMoniker_fnGetClassID(IMoniker* iface, CLSID *pClassID)
{
	CDeviceMoniker_THIS(iface,moniker);

	TRACE("(%p)->()\n",This);

	if ( pClassID == NULL )
		return E_POINTER;
	memcpy( pClassID, &CLSID_CDeviceMoniker, sizeof(CLSID) );

	return NOERROR;
}

static HRESULT WINAPI IMoniker_fnIsDirty(IMoniker* iface)
{
	CDeviceMoniker_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnLoad(IMoniker* iface, IStream* pStm)
{
	CDeviceMoniker_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnSave(IMoniker* iface, IStream* pStm, BOOL fClearDirty)
{
	CDeviceMoniker_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnGetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize)
{
	CDeviceMoniker_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnBindToObject(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult)
{
	CDeviceMoniker_THIS(iface,moniker);
	HRESULT	hr;
	IPropertyBag*	pPropBag;
	IPersistPropertyBag*	pPersistPropBag;
	VARIANT	vClsid;
	CLSID	clsid;

	TRACE("(%p)->(%p,%p,%s,%p)\n",This,
		pbc,pmkToLeft,debugstr_guid(riid),ppvResult);
	if ( pbc != NULL )
	{
		FIXME("IBindCtx* pbc != NULL not implemented.\n");
		return E_FAIL;
	}
	if ( pmkToLeft != NULL )
	{
		FIXME("IMoniker* pmkToLeft != NULL not implemented.\n");
		return E_FAIL;
	}
	if ( ppvResult == NULL )
		return E_POINTER;

	hr = QUARTZ_CreateRegPropertyBag(
			This->m_hkRoot, This->m_pwszPath, &pPropBag );
	if ( FAILED(hr) )
		return hr;

	vClsid.n1.n2.vt = VT_BSTR;
	hr = IPropertyBag_Read(
			pPropBag, QUARTZ_wszCLSID, &vClsid, NULL );
	IPropertyBag_Release( pPropBag );
	if ( FAILED(hr) )
		return hr;

	hr = CLSIDFromString( vClsid.n1.n2.n3.bstrVal, &clsid );
	SysFreeString(vClsid.n1.n2.n3.bstrVal);
	if ( FAILED(hr) )
		return hr;

	hr = CoCreateInstance(
		&clsid, NULL, CLSCTX_INPROC_SERVER, riid, ppvResult );
	if ( FAILED(hr) )
		return hr;

	hr = IUnknown_QueryInterface((IUnknown*)*ppvResult,&IID_IPersistPropertyBag,(void**)&pPersistPropBag);
	if ( hr == E_NOINTERFACE )
	{
		hr = S_OK;
	}
	else
	if ( SUCCEEDED(hr) )
	{
		hr = QUARTZ_CreateRegPropertyBag(
				This->m_hkRoot, This->m_pwszPath, &pPropBag );
		if ( SUCCEEDED(hr) )
		{
			hr = IPersistPropertyBag_Load(pPersistPropBag,pPropBag,NULL);
			IPropertyBag_Release( pPropBag );
		}
		IPersistPropertyBag_Release(pPersistPropBag);
	}

	if ( FAILED(hr) )
	{
		IUnknown_Release((IUnknown*)*ppvResult);
		*ppvResult = NULL;
	}

	TRACE( "hr = %08lx\n", hr );

	return hr;
}

static HRESULT WINAPI IMoniker_fnBindToStorage(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult)
{
	CDeviceMoniker_THIS(iface,moniker);
	HRESULT	hr;

	TRACE("(%p)->(%p,%p,%s,%p)\n",This,
		pbc,pmkToLeft,debugstr_guid(riid),ppvResult);
	if ( pbc != NULL )
	{
		FIXME("IBindCtx* pbc != NULL not implemented.\n");
		return E_FAIL;
	}
	if ( pmkToLeft != NULL )
	{
		FIXME("IMoniker* pmkToLeft != NULL not implemented.\n");
		return E_FAIL;
	}
	if ( ppvResult == NULL )
		return E_POINTER;

	hr = E_NOINTERFACE;
	if ( IsEqualGUID(riid,&IID_IUnknown) ||
		 IsEqualGUID(riid,&IID_IPropertyBag) )
	{
		hr = QUARTZ_CreateRegPropertyBag(
			This->m_hkRoot, This->m_pwszPath,
			(IPropertyBag**)ppvResult );
	}

	return hr;
}

static HRESULT WINAPI IMoniker_fnReduce(IMoniker* iface,IBindCtx* pbc, DWORD dwReduceHowFar,IMoniker** ppmkToLeft, IMoniker** ppmkReduced)
{
	CDeviceMoniker_THIS(iface,moniker);

	TRACE("(%p)->()\n",This);

	if ( ppmkReduced == NULL )
		return E_POINTER;

	*ppmkReduced = iface; IMoniker_AddRef(iface);

	return MK_S_REDUCED_TO_SELF;
}

static HRESULT WINAPI IMoniker_fnComposeWith(IMoniker* iface,IMoniker* pmkRight,BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite)
{
	CDeviceMoniker_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnEnum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
	CDeviceMoniker_THIS(iface,moniker);

	TRACE("(%p)->()\n",This);

	if ( ppenumMoniker == NULL )
		return E_POINTER;

	*ppenumMoniker = NULL;
	return NOERROR;
}

static HRESULT WINAPI IMoniker_fnIsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker)
{
	CDeviceMoniker_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnHash(IMoniker* iface,DWORD* pdwHash)
{
	CDeviceMoniker_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnIsRunning(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, IMoniker* pmkNewlyRunning)
{
	CDeviceMoniker_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnGetTimeOfLastChange(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft, FILETIME* pCompositeTime)
{
	CDeviceMoniker_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnInverse(IMoniker* iface,IMoniker** ppmk)
{
	CDeviceMoniker_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnCommonPrefixWith(IMoniker* iface,IMoniker* pmkOther, IMoniker** ppmkPrefix)
{
	CDeviceMoniker_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnRelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
	CDeviceMoniker_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnGetDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR *ppszDisplayName)
{
	CDeviceMoniker_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnParseDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut)
{
	CDeviceMoniker_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnIsSystemMoniker(IMoniker* iface,DWORD* pdwMksys)
{
	CDeviceMoniker_THIS(iface,moniker);

	TRACE("(%p)->()\n",This);
	if ( pdwMksys == NULL )
		return E_POINTER;

	*pdwMksys = MKSYS_NONE;
	return S_FALSE;
}


static ICOM_VTABLE(IMoniker) imoniker =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IMoniker_fnQueryInterface,
	IMoniker_fnAddRef,
	IMoniker_fnRelease,
	/* IPersist fields */
	IMoniker_fnGetClassID,
	/* IPersistStream fields */
	IMoniker_fnIsDirty,
	IMoniker_fnLoad,
	IMoniker_fnSave,
	IMoniker_fnGetSizeMax,
	/* IMoniker fields */
	IMoniker_fnBindToObject,
	IMoniker_fnBindToStorage,
	IMoniker_fnReduce,
	IMoniker_fnComposeWith,
	IMoniker_fnEnum,
	IMoniker_fnIsEqual,
	IMoniker_fnHash,
	IMoniker_fnIsRunning,
	IMoniker_fnGetTimeOfLastChange,
	IMoniker_fnInverse,
	IMoniker_fnCommonPrefixWith,
	IMoniker_fnRelativePathTo,
	IMoniker_fnGetDisplayName,
	IMoniker_fnParseDisplayName,
	IMoniker_fnIsSystemMoniker,
};


static HRESULT CDeviceMoniker_InitIMoniker(
	CDeviceMoniker* pdm, HKEY hkRoot, LPCWSTR lpKeyPath )
{
	DWORD	dwLen;

	ICOM_VTBL(&pdm->moniker) = &imoniker;
	pdm->m_hkRoot = hkRoot;
	pdm->m_pwszPath = NULL;

	dwLen = sizeof(WCHAR)*(lstrlenW(lpKeyPath)+1);
	pdm->m_pwszPath = (WCHAR*)QUARTZ_AllocMem( dwLen );
	if ( pdm->m_pwszPath == NULL )
		return E_OUTOFMEMORY;
	memcpy( pdm->m_pwszPath, lpKeyPath, dwLen );

	return NOERROR;
}

static void CDeviceMoniker_UninitIMoniker(
	CDeviceMoniker* pdm )
{
	if ( pdm->m_pwszPath != NULL )
		QUARTZ_FreeMem( pdm->m_pwszPath );
}

/***************************************************************************
 *
 *	new/delete for CDeviceMoniker
 *
 */

static void QUARTZ_DestroyDeviceMoniker(IUnknown* punk)
{
	CDeviceMoniker_THIS(punk,unk);

	CDeviceMoniker_UninitIMoniker( This );
}

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry CDeviceMoniker_IFEntries[] =
{
  { &IID_IPersist, offsetof(CDeviceMoniker,moniker)-offsetof(CDeviceMoniker,unk) },
  { &IID_IPersistStream, offsetof(CDeviceMoniker,moniker)-offsetof(CDeviceMoniker,unk) },
  { &IID_IMoniker, offsetof(CDeviceMoniker,moniker)-offsetof(CDeviceMoniker,unk) },
};

HRESULT QUARTZ_CreateDeviceMoniker(
	HKEY hkRoot, LPCWSTR lpKeyPath,
	IMoniker** ppMoniker )
{
	CDeviceMoniker*	pdm;
	HRESULT	hr;

	TRACE("(%08x,%s,%p)\n",hkRoot,debugstr_w(lpKeyPath),ppMoniker );

	pdm = (CDeviceMoniker*)QUARTZ_AllocObj( sizeof(CDeviceMoniker) );
	if ( pdm == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &pdm->unk, NULL );
	hr = CDeviceMoniker_InitIMoniker( pdm, hkRoot, lpKeyPath );
	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj( pdm );
		return hr;
	}

	pdm->unk.pEntries = CDeviceMoniker_IFEntries;
	pdm->unk.dwEntries = sizeof(CDeviceMoniker_IFEntries)/sizeof(CDeviceMoniker_IFEntries[0]);
	pdm->unk.pOnFinalRelease = &QUARTZ_DestroyDeviceMoniker;

	*ppMoniker = (IMoniker*)(&pdm->moniker);

	return S_OK;
}


/***************************************************************************
 *
 *	CRegPropertyBag::IPropertyBag
 *
 */

static HRESULT WINAPI
IPropertyBag_fnQueryInterface(IPropertyBag* iface,REFIID riid,void** ppobj)
{
	CRegPropertyBag_THIS(iface,propbag);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IPropertyBag_fnAddRef(IPropertyBag* iface)
{
	CRegPropertyBag_THIS(iface,propbag);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IPropertyBag_fnRelease(IPropertyBag* iface)
{
	CRegPropertyBag_THIS(iface,propbag);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IPropertyBag_fnRead(IPropertyBag* iface,LPCOLESTR lpszPropName,VARIANT* pVar,IErrorLog* pLog)
{
	CRegPropertyBag_THIS(iface,propbag);
	HRESULT hr;
	LONG	lr;
	DWORD	dwSize;
	DWORD	dwValueType;
	DWORD	dwDWordValue;
	SAFEARRAYBOUND	sab;
	SAFEARRAY*	pArray;

	TRACE("(%p)->(%s,%p,%p)\n",This,
		debugstr_w(lpszPropName),pVar,pLog);

	if ( lpszPropName == NULL || pVar == NULL )
		return E_POINTER;

	dwSize = 0;
	lr = RegQueryValueExW(
		This->m_hKey, lpszPropName, NULL,
		&dwValueType, NULL, &dwSize );
	if ( lr != ERROR_SUCCESS )
	{
		WARN( "RegQueryValueExW failed.\n" );
		return E_INVALIDARG;
	}

	switch ( dwValueType )
	{
	case REG_SZ:
		TRACE( "REG_SZ / length = %lu\n", dwSize );
		if ( pVar->n1.n2.vt == VT_EMPTY )
			pVar->n1.n2.vt = VT_BSTR;
		if ( pVar->n1.n2.vt != VT_BSTR )
		{
			FIXME( "type of VARIANT is not BSTR.\n" );
			return E_FAIL;
		}

		pVar->n1.n2.n3.bstrVal = SysAllocStringByteLen(
			NULL, dwSize );
		if ( pVar->n1.n2.n3.bstrVal == NULL )
		{
			WARN( "out of memory.\n" );
			return E_OUTOFMEMORY;
		}
		lr = RegQueryValueExW(
			This->m_hKey, lpszPropName, NULL,
			&dwValueType,
			(BYTE*)pVar->n1.n2.n3.bstrVal, &dwSize );
		if ( lr != ERROR_SUCCESS )
		{
			WARN( "failed to query value\n" );
			SysFreeString(pVar->n1.n2.n3.bstrVal);
			return E_FAIL;
		}
		TRACE( "value is BSTR; %s\n", debugstr_w(pVar->n1.n2.n3.bstrVal) );
		break;
	case REG_BINARY:
		TRACE( "REG_BINARY / length = %lu\n", dwSize );
		if ( pVar->n1.n2.vt == VT_EMPTY )
			pVar->n1.n2.vt = VT_ARRAY|VT_UI1;
		if ( pVar->n1.n2.vt != (VT_ARRAY|VT_UI1) )
		{
			FIXME( "type of VARIANT is not VT_ARRAY|VT_UI1.\n" );
			return E_FAIL;
		}
		sab.lLbound = 0;
		sab.cElements = dwSize;
		pArray = SafeArrayCreate( VT_UI1, 1, &sab );
		if ( pArray == NULL )
			return E_OUTOFMEMORY;
		hr = SafeArrayLock( pArray );
		if ( FAILED(hr) )
		{
			WARN( "safe array can't be locked\n" );
			SafeArrayDestroy( pArray );
			return hr;
		}
		lr = RegQueryValueExW(
			This->m_hKey, lpszPropName, NULL,
			&dwValueType,
			(BYTE*)pArray->pvData, &dwSize );
		SafeArrayUnlock( pArray );
		if ( lr != ERROR_SUCCESS )
		{
			WARN( "failed to query value\n" );
			SafeArrayDestroy( pArray );
			return E_FAIL;
		}
		pVar->n1.n2.n3.parray = pArray;
		TRACE( "value is SAFEARRAY - array of BYTE; \n" );
		break;
	case REG_DWORD:
		TRACE( "REG_DWORD / length = %lu\n", dwSize );
		if ( dwSize != sizeof(DWORD) )
		{
			WARN( "The length of REG_DWORD value is not sizeof(DWORD).\n" );
			return E_FAIL;
		}
		if ( pVar->n1.n2.vt == VT_EMPTY )
			pVar->n1.n2.vt = VT_I4;
		if ( pVar->n1.n2.vt != VT_I4 )
		{
			FIXME( "type of VARIANT is not VT_I4.\n" );
			return E_FAIL;
		}
		lr = RegQueryValueExW(
			This->m_hKey, lpszPropName, NULL,
			&dwValueType,
			(BYTE*)(&dwDWordValue), &dwSize );
		if ( lr != ERROR_SUCCESS )
		{
			WARN( "failed to query value\n" );
			return E_FAIL;
		}
		pVar->n1.n2.n3.lVal = dwDWordValue;
		TRACE( "value is DWORD; %08lx\n", dwDWordValue );
		break;
	default:
		FIXME("(%p)->(%s,%p,%p) - unsupported value type.\n",This,
			debugstr_w(lpszPropName),pVar,pLog);
		return E_FAIL;
	}

	TRACE( "returned successfully.\n" );
	return NOERROR;
}

static HRESULT WINAPI
IPropertyBag_fnWrite(IPropertyBag* iface,LPCOLESTR lpszPropName,VARIANT* pVar)
{
	CRegPropertyBag_THIS(iface,propbag);
	HRESULT hr;
	LONG	lr;
	DWORD	dwDWordValue;
	SAFEARRAY*	pArray;

	TRACE("(%p)->(%s,%p)\n",This,
		debugstr_w(lpszPropName),pVar);

	if ( lpszPropName == NULL || pVar == NULL )
		return E_POINTER;

	switch ( pVar->n1.n2.vt )
	{
	case VT_I4: /* REG_DWORD */
		dwDWordValue = pVar->n1.n2.n3.lVal;
		lr = RegSetValueExW(
			This->m_hKey, lpszPropName, 0,
			REG_DWORD,
			(const BYTE*)(&dwDWordValue), sizeof(DWORD) );
		if ( lr != ERROR_SUCCESS )
		{
			WARN( "failed to set value\n" );
			return E_FAIL;
		}
		break;
	case VT_BSTR: /* REG_SZ */
		lr = RegSetValueExW(
			This->m_hKey, lpszPropName, 0,
			REG_SZ,
			(const BYTE*)(pVar->n1.n2.n3.bstrVal),
			SysStringByteLen( pVar->n1.n2.n3.bstrVal ) );
		if ( lr != ERROR_SUCCESS )
		{
			WARN( "failed to set value\n" );
			return E_FAIL;
		}
		break;
	case (VT_ARRAY|VT_UI1): /* REG_BINARY */
		pArray = pVar->n1.n2.n3.parray;
		if ( pArray->cDims != 1 || pArray->cbElements != 1 ||
			 pArray->rgsabound[0].lLbound != 0 )
		{
			WARN( "invalid array\n" );
			return E_INVALIDARG;
		}
		hr = SafeArrayLock( pArray );
		if ( FAILED(hr) )
		{
			WARN( "safe array can't be locked\n" );
			return hr;
		}
		lr = RegSetValueExW(
			This->m_hKey, lpszPropName, 0,
			REG_BINARY,
			(const BYTE*)pArray->pvData,
			pArray->rgsabound[0].cElements );
		SafeArrayUnlock( pArray );
		if ( lr != ERROR_SUCCESS )
		{
			WARN( "failed to set value\n" );
			return E_FAIL;
		}
		break;
	default:
		FIXME("(%p)->(%s,%p) invalid/unsupported VARIANT type %04x\n",This,
			debugstr_w(lpszPropName),pVar,pVar->n1.n2.vt);
		return E_INVALIDARG;
	}

	TRACE( "returned successfully.\n" );
	return NOERROR;
}




static ICOM_VTABLE(IPropertyBag) ipropbag =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IPropertyBag_fnQueryInterface,
	IPropertyBag_fnAddRef,
	IPropertyBag_fnRelease,
	/* IPropertyBag fields */
	IPropertyBag_fnRead,
	IPropertyBag_fnWrite,
};

static HRESULT CRegPropertyBag_InitIPropertyBag(
	CRegPropertyBag* prpb, HKEY hkRoot, LPCWSTR lpKeyPath )
{
	WCHAR	wszREG_SZ[ NUMELEMS(QUARTZ_wszREG_SZ) ];
	DWORD	dwDisp;

	ICOM_VTBL(&prpb->propbag) = &ipropbag;

	memcpy(wszREG_SZ,QUARTZ_wszREG_SZ,sizeof(QUARTZ_wszREG_SZ) );

	if ( RegCreateKeyExW(
			hkRoot, lpKeyPath, 0, wszREG_SZ,
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
			&prpb->m_hKey, &dwDisp ) != ERROR_SUCCESS )
		return E_FAIL;

	return NOERROR;
}

static void CRegPropertyBag_UninitIPropertyBag(
	CRegPropertyBag* prpb )
{
	RegCloseKey( prpb->m_hKey );
}


/***************************************************************************
 *
 *	new/delete for CRegPropertyBag
 *
 */

static void QUARTZ_DestroyRegPropertyBag(IUnknown* punk)
{
	CRegPropertyBag_THIS(punk,unk);

	CRegPropertyBag_UninitIPropertyBag(This);
}


/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry CRegPropertyBag_IFEntries[] =
{
  { &IID_IPropertyBag, offsetof(CRegPropertyBag,propbag)-offsetof(CRegPropertyBag,unk) },
};

HRESULT QUARTZ_CreateRegPropertyBag(
	HKEY hkRoot, LPCWSTR lpKeyPath,
	IPropertyBag** ppPropBag )
{
	CRegPropertyBag*	prpb;
	HRESULT	hr;

	TRACE("(%08x,%s,%p)\n",hkRoot,debugstr_w(lpKeyPath),ppPropBag );

	prpb = (CRegPropertyBag*)QUARTZ_AllocObj( sizeof(CRegPropertyBag) );
	if ( prpb == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &prpb->unk, NULL );
	hr = CRegPropertyBag_InitIPropertyBag( prpb, hkRoot, lpKeyPath );
	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj( prpb );
		return hr;
	}

	prpb->unk.pEntries = CRegPropertyBag_IFEntries;
	prpb->unk.dwEntries = sizeof(CRegPropertyBag_IFEntries)/sizeof(CRegPropertyBag_IFEntries[0]);
	prpb->unk.pOnFinalRelease = &QUARTZ_DestroyRegPropertyBag;

	*ppPropBag = (IPropertyBag*)(&prpb->propbag);

	return S_OK;
}

/***************************************************************************
 *
 *	Helper for registering filters.
 *
 */

HRESULT QUARTZ_GetFilterRegPath(
	WCHAR** ppwszPath,	/* [OUT] path from HKEY_CLASSES_ROOT */
	const CLSID* pguidFilterCategory,	/* [IN] Category */
	const CLSID* pclsid,	/* [IN] CLSID of this filter */
	LPCWSTR lpInstance )	/* [IN] instance */
{
	HRESULT	hr;
	WCHAR	szKey[ 1024 ];
	WCHAR	szFilterPath[ 512 ];
	WCHAR	szCLSID[ 256 ];
	int	buflen;

	TRACE("(%p,%s,%s,%s)\n",ppwszPath,debugstr_guid(pguidFilterCategory),debugstr_guid(pclsid),debugstr_w(lpInstance) );

	*ppwszPath = NULL;

	QUARTZ_GUIDtoString( szCLSID, pclsid );
	lstrcpyW( szFilterPath, QUARTZ_wszInstance );
	QUARTZ_CatPathSepW( szFilterPath );
	if ( lpInstance != NULL )
	{
		if ( lstrlenW(lpInstance) >= 256 )
			return E_INVALIDARG;
		lstrcatW( szFilterPath, lpInstance );
	}
	else
	{
		lstrcatW( szFilterPath, szCLSID );
	}

	hr = QUARTZ_CreateCLSIDPath(
		szKey, NUMELEMS(szKey),
		pguidFilterCategory, szFilterPath );
	if ( FAILED(hr) )
		return hr;

	buflen = sizeof(WCHAR)*(lstrlenW(szKey)+1);
	*ppwszPath = QUARTZ_AllocMem( buflen );
	if ( *ppwszPath == NULL )
		return E_OUTOFMEMORY;
	memcpy( *ppwszPath, szKey, buflen );
	return S_OK;
}

HRESULT QUARTZ_RegisterFilterToMoniker(
	IMoniker* pMoniker,	/* [IN] Moniker */
	const CLSID* pclsid,	/* [IN] CLSID of this filter */
	LPCWSTR lpFriendlyName,	/* [IN] friendly name */
	const BYTE* pbFilterData,	/* [IN] filter data */
	DWORD cbFilterData )	/* [IN] size of the filter data */
{
	IPropertyBag*	pPropBag = NULL;
	WCHAR	wszClsid[128];
	VARIANT var;
	HRESULT hr;
	SAFEARRAYBOUND	sab;
	SAFEARRAY*	pArray = NULL;

	TRACE("(%p,%s,%s,%p,%08lu)\n",pMoniker,debugstr_guid(pclsid),debugstr_w(lpFriendlyName),pbFilterData,cbFilterData);

	hr = IMoniker_BindToStorage(pMoniker,NULL,NULL,&IID_IPropertyBag,(void**)&pPropBag);
	if ( FAILED(hr) )
		goto err;
	QUARTZ_GUIDtoString( wszClsid, pclsid );
	var.n1.n2.vt = VT_BSTR;
	var.n1.n2.n3.bstrVal = SysAllocString(wszClsid);
	if ( var.n1.n2.n3.bstrVal == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto err;
	}
	hr = IPropertyBag_Write(pPropBag,QUARTZ_wszCLSID,&var);
	SysFreeString(var.n1.n2.n3.bstrVal);
	if ( FAILED(hr) )
		goto err;

	var.n1.n2.vt = VT_BSTR;
	var.n1.n2.n3.bstrVal = SysAllocString(lpFriendlyName);
	if ( var.n1.n2.n3.bstrVal == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto err;
	}
	hr = IPropertyBag_Write(pPropBag,QUARTZ_wszFriendlyName,&var);
	SysFreeString(var.n1.n2.n3.bstrVal);
	if ( FAILED(hr) )
		goto err;

	if ( pbFilterData != NULL )
	{
		var.n1.n2.vt = VT_ARRAY|VT_UI1;
		sab.lLbound = 0;
		sab.cElements = cbFilterData;
		pArray = SafeArrayCreate( VT_UI1, 1, &sab );
		if ( pArray == NULL )
		{
			hr = E_OUTOFMEMORY;
			goto err;
		}
		hr = SafeArrayLock( pArray );
		if ( FAILED(hr) )
			goto err;
		var.n1.n2.n3.parray = pArray;
		memcpy( pArray->pvData, pbFilterData, cbFilterData );
		hr = IPropertyBag_Write(pPropBag,QUARTZ_wszFilterData,&var);
		SafeArrayUnlock( pArray );
		if ( FAILED(hr) )
			goto err;
	}

	hr = NOERROR;
err:
	if ( pArray != NULL )
		SafeArrayDestroy( pArray );
	if ( pPropBag != NULL )
		IPropertyBag_Release(pPropBag);

	return hr;
}

HRESULT QUARTZ_RegDeleteKey( HKEY hkRoot, LPCWSTR lpKeyPath )
{
	LONG	lr;
	HRESULT hr;
	HKEY	hKey;
	DWORD	dwIndex;
	DWORD	cbName;
	FILETIME	ftLastWrite;
	DWORD	dwType;
	WCHAR	wszPath[ 512 ];

	TRACE("(%08x,%s)\n",hkRoot,debugstr_w(lpKeyPath));

	lr = RegOpenKeyExW( hkRoot, lpKeyPath, 0, KEY_ALL_ACCESS, &hKey );
	if ( lr == ERROR_SUCCESS )
	{
		dwIndex = 0;
		while ( 1 )
		{
			cbName = NUMELEMS(wszPath);
			lr = RegEnumKeyExW(
				hKey, dwIndex, wszPath, &cbName,
				NULL, NULL, NULL, &ftLastWrite );
			if ( lr != ERROR_SUCCESS )
				break;
			hr = QUARTZ_RegDeleteKey( hKey, wszPath );
			if ( FAILED(hr) )
				return hr;
			if ( hr != S_OK )
				dwIndex ++;
		}
		while ( 1 )
		{
			cbName = NUMELEMS(wszPath);
			lr = RegEnumValueW(
				hKey, 0, wszPath, &cbName, 0,
				&dwType, NULL, 0 );
			if ( lr != ERROR_SUCCESS )
				break;
			lr = RegDeleteValueW( hKey, wszPath );
			if ( lr != ERROR_SUCCESS )
			{
				WARN("RegDeleteValueW - %08lx\n",lr);
				return E_FAIL;
			}
		}
	}
	RegCloseKey( hKey );

	lr = RegDeleteKeyW( hkRoot, lpKeyPath );
	WARN("RegDeleteKeyW - %08lx\n",lr);
	if ( lr != ERROR_SUCCESS && lr != ERROR_FILE_NOT_FOUND )
		return S_FALSE;
	return S_OK;
}

static
HRESULT QUARTZ_GetPropertyFromMoniker(
	IMoniker* pMoniker,	/* [IN] Moniker */
	LPCOLESTR lpszPropName,	/* [IN] Property */
	VARIANT* pVar )	/* [OUT] */
{
	IPropertyBag*	pPropBag = NULL;
	HRESULT hr;

	TRACE("(%p,%s,%p)\n",pMoniker,debugstr_w(lpszPropName),pVar);

	hr = IMoniker_BindToStorage(pMoniker,NULL,NULL,&IID_IPropertyBag,(void**)&pPropBag);
	if ( FAILED(hr) )
		return hr;
	hr = IPropertyBag_Read(pPropBag,lpszPropName,pVar,NULL);
	IPropertyBag_Release(pPropBag);

	return hr;
}

HRESULT QUARTZ_GetCLSIDFromMoniker(
	IMoniker* pMoniker,	/* [IN] Moniker */
	CLSID* pclsid )	/* [OUT] */
{
	VARIANT	var;
	HRESULT hr;

	var.n1.n2.vt = VT_BSTR;
	hr = QUARTZ_GetPropertyFromMoniker(
		pMoniker, QUARTZ_wszCLSID, &var );
	if ( hr == S_OK )
	{
		hr = CLSIDFromString(var.n1.n2.n3.bstrVal,pclsid);
		SysFreeString(var.n1.n2.n3.bstrVal);
	}

	return hr;
}

HRESULT QUARTZ_GetMeritFromMoniker(
	IMoniker* pMoniker,	/* [IN] Moniker */
	DWORD* pdwMerit )	/* [OUT] */
{
	VARIANT	var;
	HRESULT hr;

	var.n1.n2.vt = VT_I4;
	hr = QUARTZ_GetPropertyFromMoniker(
		pMoniker, QUARTZ_wszMerit, &var );
	if ( hr == S_OK )
		*pdwMerit = var.n1.n2.n3.lVal;

	return hr;
}

HRESULT QUARTZ_GetFilterDataFromMoniker(
	IMoniker* pMoniker,	/* [IN] Moniker */
	BYTE** ppbFilterData,	/* [OUT] */
	DWORD* pcbFilterData )	/* [OUT] */
{
	VARIANT	var;
	HRESULT hr;
	SAFEARRAY*	pArray;

	var.n1.n2.vt = VT_ARRAY|VT_UI1;
	hr = QUARTZ_GetPropertyFromMoniker(
		pMoniker, QUARTZ_wszFilterData, &var );
	if ( hr == S_OK )
	{
		pArray = var.n1.n2.n3.parray;
		hr = SafeArrayLock( pArray );
		if ( SUCCEEDED(hr) )
		{
			*pcbFilterData = pArray->rgsabound[0].cElements - pArray->rgsabound[0].lLbound;
			*ppbFilterData = (BYTE*)QUARTZ_AllocMem( *pcbFilterData );
			if ( *ppbFilterData == NULL )
				hr = E_OUTOFMEMORY;
			else
				memcpy( *ppbFilterData, pArray->pvData, *pcbFilterData );

			SafeArrayUnlock( pArray );
		}
		SafeArrayDestroy( pArray );
	}

	return hr;
}

