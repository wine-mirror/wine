/*
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
#include "wine/obj_base.h"
#include "objidl.h"
#include "oleidl.h"
#include "ocidl.h"
#include "oleauto.h"
#include "strmif.h"
#include "uuids.h"
#include "wine/unicode.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "devmon.h"
#include "regsvr.h"


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

	dwLen = sizeof(WCHAR)*(strlenW(lpKeyPath)+1);
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
	LONG	lr;
	DWORD	dwSize;
	DWORD	dwValueType;

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
		TRACE( "RegQueryValueExW failed.\n" );
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
			TRACE( "type of VARIANT is not BSTR\n" );
			return E_FAIL;
		}

		pVar->n1.n2.n3.bstrVal = SysAllocStringByteLen(
			NULL, dwSize );
		if ( pVar->n1.n2.n3.bstrVal == NULL )
		{
			TRACE( "out of memory.\n" );
			return E_OUTOFMEMORY;
		}
		lr = RegQueryValueExW(
			This->m_hKey, lpszPropName, NULL,
			&dwValueType,
			(BYTE*)pVar->n1.n2.n3.bstrVal, &dwSize );
		if ( lr != ERROR_SUCCESS )
		{
			TRACE( "failed to query value\n" );
			SysFreeString(pVar->n1.n2.n3.bstrVal);
			return E_FAIL;
		}
		TRACE( "value is BSTR; %s\n", debugstr_w(pVar->n1.n2.n3.bstrVal) );
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

	FIXME("(%p)->(%s,%p) stub!\n",This,
		debugstr_w(lpszPropName),pVar);

	if ( lpszPropName == NULL || pVar == NULL )
		return E_POINTER;

	return E_NOTIMPL;
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
	ICOM_VTBL(&prpb->propbag) = &ipropbag;

	if ( RegOpenKeyExW(
		hkRoot, lpKeyPath, 0,
		KEY_ALL_ACCESS, &prpb->m_hKey ) != ERROR_SUCCESS )
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



