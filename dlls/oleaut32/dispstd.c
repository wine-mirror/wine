/*
 * Copyright 2001 Hidenori Takeshima
 *
 *	FIXME - stub
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

#include "config.h"

#include "winerror.h"
#include "winnls.h"         /* for PRIMARYLANGID */
#include "winreg.h"         /* for HKEY_LOCAL_MACHINE */
#include "winuser.h"
#include "oleauto.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ole);

typedef struct CStdDispImpl
{
	ICOM_VFIELD(IDispatch);
	struct { ICOM_VFIELD(IUnknown); } unkimpl;
	UINT ref;

	IUnknown*	punk;
	void*		pvThis;
	ITypeInfo*	pti;
} CStdDispImpl;

static HRESULT CStdDispImpl_Construct(
	CStdDispImpl* This,
	IUnknown* punkOuter, void* pvThis, ITypeInfo* pti )
{
	This->punk = punkOuter;
	This->pvThis = pvThis;
	This->pti = pti; ITypeInfo_AddRef(pti);

	return S_OK;
}

static void CStdDispImpl_Destruct(
	CStdDispImpl* This )
{
	if ( This->pti != NULL )
		ITypeInfo_Release(This->pti);
}


/****************************************************************************/


static HRESULT WINAPI In_CStdDispImpl_fnQueryInterface(
	IUnknown* iface,REFIID riid,void** ppvobj)
{
	CStdDispImpl* This = (CStdDispImpl*)(((BYTE*)iface)-offsetof(CStdDispImpl,unkimpl));

	if ( IsEqualGUID(riid,&IID_IUnknown) )
	{
		*ppvobj = (void*)iface;
		IUnknown_AddRef(iface);
		return S_OK;
	}
	if ( IsEqualGUID(riid,&IID_IDispatch) )
	{
		*ppvobj = (void*)This;
		IUnknown_AddRef((IUnknown*)This);
		return S_OK;
	}

	return E_NOINTERFACE;
}

static ULONG WINAPI In_CStdDispImpl_fnAddRef(IUnknown* iface)
{
	CStdDispImpl* This = (CStdDispImpl*)(((BYTE*)iface)-offsetof(CStdDispImpl,unkimpl));

	return ++ This->ref;
}

static ULONG WINAPI In_CStdDispImpl_fnRelease(IUnknown* iface)
{
	CStdDispImpl* This = (CStdDispImpl*)(((BYTE*)iface)-offsetof(CStdDispImpl,unkimpl));

	if ( -- This->ref > 0 ) return This->ref;

	++ This->ref;
	CStdDispImpl_Destruct(This);
	HeapFree(GetProcessHeap(),0,This);
	return 0;
}


/****************************************************************************/



static HRESULT WINAPI CStdDispImpl_fnQueryInterface(
	IDispatch* iface,REFIID riid,void** ppvobj)
{
    ICOM_THIS(CStdDispImpl,iface);

	return IUnknown_QueryInterface(This->punk,riid,ppvobj);
}

static ULONG WINAPI CStdDispImpl_fnAddRef(IDispatch* iface)
{
    ICOM_THIS(CStdDispImpl,iface);

	return IUnknown_AddRef(This->punk);
}

static ULONG WINAPI CStdDispImpl_fnRelease(IDispatch* iface)
{
    ICOM_THIS(CStdDispImpl,iface);

	return IUnknown_Release(This->punk);
}


static HRESULT WINAPI CStdDispImpl_fnGetTypeInfoCount(
	IDispatch* iface,UINT* pctinfo)
{
    ICOM_THIS(CStdDispImpl,iface);

	FIXME("(%p)\n",This);
	if ( pctinfo == NULL ) return E_POINTER;
	*pctinfo = 1;

	return S_OK;
}

static HRESULT WINAPI CStdDispImpl_fnGetTypeInfo(
	IDispatch* iface,
	UINT itiindex,LCID lcid,ITypeInfo** ppti)
{
    ICOM_THIS(CStdDispImpl,iface);

	FIXME("(%p)\n",This);

	if ( ppti != NULL ) return E_POINTER;
	*ppti = NULL;

	if ( itiindex != 0 ) return DISP_E_BADINDEX;
	/* lcid is ignored */
	ITypeInfo_AddRef(This->pti);
	*ppti = This->pti;

	return S_OK;
}

static HRESULT WINAPI CStdDispImpl_fnGetIDsOfNames(
	IDispatch* iface,
	REFIID riid,LPOLESTR* ppwszNames,UINT cNames,LCID lcid,DISPID* pdispid)
{
    ICOM_THIS(CStdDispImpl,iface);

	FIXME("(%p)\n",This);
	return DispGetIDsOfNames(This->pti,ppwszNames,cNames,pdispid);
}

static HRESULT WINAPI CStdDispImpl_fnInvoke(
	IDispatch* iface,
	DISPID dispid,REFIID riid,LCID lcid,WORD wFlags,
	DISPPARAMS* pDispParams,VARIANT* pVarResult,
	EXCEPINFO* pExcepInfo,UINT* puArgErr)
{
    ICOM_THIS(CStdDispImpl,iface);

	FIXME("(%p)\n",This);
	return DispInvoke(This->pvThis,
		This->pti,dispid,wFlags,
		pDispParams,pVarResult,
		pExcepInfo,puArgErr);
}

static ICOM_VTABLE(IUnknown) iunk =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE

	/* IUnknown */
	In_CStdDispImpl_fnQueryInterface,
	In_CStdDispImpl_fnAddRef,
	In_CStdDispImpl_fnRelease,
};

static ICOM_VTABLE(IDispatch) idisp =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE

	/* IUnknown */
	CStdDispImpl_fnQueryInterface,
	CStdDispImpl_fnAddRef,
	CStdDispImpl_fnRelease,
	/* IDispatch */
	CStdDispImpl_fnGetTypeInfoCount,
	CStdDispImpl_fnGetTypeInfo,
	CStdDispImpl_fnGetIDsOfNames,
	CStdDispImpl_fnInvoke,
};

/******************************************************************************
 *		CreateStdDispatch  (OLEAUT32.32)
 */

HRESULT WINAPI CreateStdDispatch(
	IUnknown* punkOuter,
	void* pvThis,
	ITypeInfo* pti,
	IUnknown** ppvobj )
{
	HRESULT hr;
	CStdDispImpl*	This;

	if ( punkOuter == NULL || pvThis == NULL ||
	     pti == NULL || ppvobj == NULL )
		return E_POINTER;

	This = (CStdDispImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(CStdDispImpl));
	if ( This == NULL ) return E_OUTOFMEMORY;
	ICOM_VTBL(This) = &idisp;
	ICOM_VTBL(&(This->unkimpl)) = &iunk;
	This->ref = 1;

	hr = CStdDispImpl_Construct( This, punkOuter, pvThis, pti );
	if ( FAILED(hr) )
	{
		IUnknown_Release((IUnknown*)(&This->unkimpl));
		return hr;
	}
	*ppvobj = (IUnknown*)(&This->unkimpl);

	return S_OK;
}
