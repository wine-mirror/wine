/*
 * URL Moniker
 *
 * FIXME - stub
 *
 * Copyright (C) 2002 Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
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
 */

#include "config.h"

#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "ole2.h"
#include "urlmon.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(comimpl);

#include "comimpl.h"


/*
 * NOTE:
 *	URL Moniker supports the following protocols(at least):
 *
 * CLSID_HttpProtocol
 * CLSID_FtpProtocol
 * CLSID_GopherProtocol
 * CLSID_HttpSProtocol
 * CLSID_FileProtocol
 *
 */

typedef struct CURLMonikerImpl
{
	COMIMPL_IUnkImpl vfunk;
	struct { ICOM_VFIELD(IMoniker); } moniker;
	struct { ICOM_VFIELD(IROTData); } rotd;

	/* IMoniker stuffs */
} CURLMonikerImpl;

#define CURLMonikerImpl_THIS(iface,member) CURLMonikerImpl* This = ((CURLMonikerImpl*)(((char*)iface)-offsetof(CURLMonikerImpl,member)))


static HRESULT WINAPI
IMoniker_fnQueryInterface(IMoniker* iface,REFIID riid,void** ppobj)
{
	CURLMonikerImpl_THIS(iface,moniker);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->vfunk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IMoniker_fnAddRef(IMoniker* iface)
{
	CURLMonikerImpl_THIS(iface,moniker);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->vfunk.punkControl);
}

static ULONG WINAPI
IMoniker_fnRelease(IMoniker* iface)
{
	CURLMonikerImpl_THIS(iface,moniker);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->vfunk.punkControl);
}

static HRESULT WINAPI IMoniker_fnGetClassID(IMoniker* iface, CLSID *pClassID)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub\n",This);

	return E_NOTIMPL;
#if 0
	TRACE("(%p)->()\n",This);

	if ( pClassID == NULL )
		return E_POINTER;
	memcpy( pClassID, &CLSID_StdURLMoniker, sizeof(CLSID) );

	return NOERROR;
#endif
}

static HRESULT WINAPI IMoniker_fnIsDirty(IMoniker* iface)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnLoad(IMoniker* iface, IStream* pStm)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnSave(IMoniker* iface, IStream* pStm, BOOL fClearDirty)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnGetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnBindToObject(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->(%p,%p,%s,%p)\n",This,
		pbc,pmkToLeft,debugstr_guid(riid),ppvResult);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnBindToStorage(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->(%p,%p,%s,%p)\n",This,
		pbc,pmkToLeft,debugstr_guid(riid),ppvResult);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnReduce(IMoniker* iface,IBindCtx* pbc, DWORD dwReduceHowFar,IMoniker** ppmkToLeft, IMoniker** ppmkReduced)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnComposeWith(IMoniker* iface,IMoniker* pmkRight,BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnEnum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnIsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnHash(IMoniker* iface,DWORD* pdwHash)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnIsRunning(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, IMoniker* pmkNewlyRunning)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnGetTimeOfLastChange(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft, FILETIME* pCompositeTime)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnInverse(IMoniker* iface,IMoniker** ppmk)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnCommonPrefixWith(IMoniker* iface,IMoniker* pmkOther, IMoniker** ppmkPrefix)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnRelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnGetDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR *ppszDisplayName)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnParseDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IMoniker_fnIsSystemMoniker(IMoniker* iface,DWORD* pdwMksys)
{
	CURLMonikerImpl_THIS(iface,moniker);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
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




static HRESULT WINAPI
IROTData_fnQueryInterface(IROTData* iface,REFIID riid,void** ppobj)
{
	CURLMonikerImpl_THIS(iface,rotd);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->vfunk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IROTData_fnAddRef(IROTData* iface)
{
	CURLMonikerImpl_THIS(iface,rotd);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->vfunk.punkControl);
}

static ULONG WINAPI
IROTData_fnRelease(IROTData* iface)
{
	CURLMonikerImpl_THIS(iface,rotd);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->vfunk.punkControl);
}

static HRESULT WINAPI IROTData_fnGetComparisonData(IROTData* iface,BYTE* pbData,ULONG cbMax,ULONG* pcbData)
{
	CURLMonikerImpl_THIS(iface,rotd);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}



static ICOM_VTABLE(IROTData) irotdata =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IROTData_fnQueryInterface,
	IROTData_fnAddRef,
	IROTData_fnRelease,
	/* IROTData fields */
	IROTData_fnGetComparisonData,
};



static COMIMPL_IFEntry IFEntries[] =
{
	{ &IID_IPersist, offsetof(CURLMonikerImpl,moniker)-offsetof(CURLMonikerImpl,vfunk) },
	{ &IID_IPersistStream, offsetof(CURLMonikerImpl,moniker)-offsetof(CURLMonikerImpl,vfunk) },
	{ &IID_IMoniker, offsetof(CURLMonikerImpl,moniker)-offsetof(CURLMonikerImpl,vfunk) },
	{ &IID_IROTData, offsetof(CURLMonikerImpl,rotd)-offsetof(CURLMonikerImpl,vfunk) },
};

static void CURLMonikerImpl_Destructor(IUnknown* iface)
{
	CURLMonikerImpl_THIS(iface,vfunk);

	TRACE("(%p)\n",This);
}

static HRESULT CURLMonikerImpl_AllocObj(
	void** ppobj,
	IMoniker* pmonContext,
	LPCWSTR lpwszURL )
{
	CURLMonikerImpl* This;

	This = (CURLMonikerImpl*)COMIMPL_AllocObj( sizeof(CURLMonikerImpl) );
	if ( This == NULL ) return E_OUTOFMEMORY;
	COMIMPL_IUnkInit( &This->vfunk, NULL );
	This->vfunk.pEntries = IFEntries;
	This->vfunk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);
	This->vfunk.pOnFinalRelease = CURLMonikerImpl_Destructor;

	ICOM_VTBL(&This->moniker) = &imoniker;
	ICOM_VTBL(&This->rotd) = &irotdata;

	*ppobj = (void*)(&This->vfunk);

	return S_OK;
}


/***********************************************************************
 *
 *	CreateURLMoniker (URLMON.@)
 *
 *	S_OK           success
 *	E_OUTOFMEMORY  out of memory
 *	MK_E_SYNTAX    not a valid url
 *
 */

HRESULT WINAPI CreateURLMoniker(
	IMoniker* pmonContext,
	LPCWSTR lpwszURL,
	IMoniker** ppmon )
{
	HRESULT hr;
	IUnknown* punk = NULL;

	FIXME("(%p,%s,%p)\n",pmonContext,debugstr_w(lpwszURL),ppmon);

	if ( ppmon == NULL )
		return E_POINTER;
	*ppmon = NULL;

	hr = CURLMonikerImpl_AllocObj( (void**)&punk, pmonContext, lpwszURL );
	if ( FAILED(hr) )
		return hr;

	hr = IUnknown_QueryInterface( punk, &IID_IMoniker, (void**)ppmon );
	IUnknown_Release( punk );

	return hr;
}

