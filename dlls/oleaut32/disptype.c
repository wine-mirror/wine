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

#include "typelib.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ole);

typedef struct CTypeInfo2Impl
{
	ICOM_VFIELD(ITypeInfo2);
	UINT ref;

	INTERFACEDATA	ifd;
	LCID	lcid;
} CTypeInfo2Impl;

static OLECHAR* olestrdup(OLECHAR* psz)
{
	OLECHAR*	pret;
	DWORD	cb;

	cb = (lstrlenW(psz)+1) * sizeof(OLECHAR);
	pret = (OLECHAR*)HeapAlloc( GetProcessHeap(), 0, cb );
	if ( pret != NULL )
		memcpy( pret, psz, cb );
	return pret;
}

static HRESULT CTypeInfo2Impl_Construct(
	CTypeInfo2Impl* This,INTERFACEDATA* pifd,LCID lcid)
{
	DWORD	n,k;
	METHODDATA*	pmdst;
	const METHODDATA*	pmsrc;
	PARAMDATA*	pPdst;
	const PARAMDATA*	pPsrc;

	This->ifd.pmethdata = (METHODDATA*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(METHODDATA)*pifd->cMembers);
	if ( This->ifd.pmethdata == NULL ) return E_OUTOFMEMORY;
	This->ifd.cMembers = pifd->cMembers;
	for ( n = 0; n < pifd->cMembers; n++ )
	{
		pmdst = &This->ifd.pmethdata[n];
		pmsrc = &pifd->pmethdata[n];

		pmdst->szName = olestrdup(pmsrc->szName);
		if ( pmdst->szName == NULL ) return E_OUTOFMEMORY;
		pmdst->ppdata = NULL;
		pmdst->dispid = pmsrc->dispid;
		pmdst->iMeth = pmsrc->iMeth;
		pmdst->cc = pmsrc->cc;
		pmdst->cArgs = pmsrc->cArgs;
		pmdst->wFlags = pmsrc->wFlags;
		pmdst->vtReturn = pmsrc->vtReturn;

		if ( pmsrc->cArgs <= 0 ) continue;
		pmdst->ppdata = (PARAMDATA*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(PARAMDATA)*pmsrc->cArgs);
		if ( pmdst->ppdata == NULL ) return E_OUTOFMEMORY;

		for ( k = 0; k < pmsrc->cArgs; k++ )
		{
			pPdst = &pmdst->ppdata[k];
			pPsrc = &pmsrc->ppdata[k];
			pPdst->szName = olestrdup(pPsrc->szName);
			if ( pPdst->szName == NULL ) return E_OUTOFMEMORY;
			pPdst->vt = pPsrc->vt;
		}
	}

	This->lcid = lcid;

	return S_OK;
}

static void CTypeInfo2Impl_Destruct(CTypeInfo2Impl* This)
{
	DWORD	n,k;
	METHODDATA*	pm;
	PARAMDATA*	pP;

	if ( This->ifd.pmethdata != NULL )
	{
		for ( n = 0; n < This->ifd.cMembers; n++ )
		{
			pm = &This->ifd.pmethdata[n];
			if ( pm->szName != NULL )
				HeapFree(GetProcessHeap(),0,pm->szName);
			if ( pm->ppdata == NULL ) continue;

			for ( k = 0; k < pm->cArgs; k++ )
			{
				pP = &pm->ppdata[k];
				if ( pP->szName != NULL )
					HeapFree(GetProcessHeap(),0,pP->szName);
			}
			HeapFree(GetProcessHeap(),0,pm->ppdata);
		}
		HeapFree(GetProcessHeap(),0,This->ifd.pmethdata);
		This->ifd.pmethdata = NULL;
	}
}

/****************************************************************************/

static const METHODDATA* CTypeInfo2Impl_SearchMethodByName(
	CTypeInfo2Impl* This,const OLECHAR* pName)
{
	DWORD	n;
	METHODDATA*	pm;

	for ( n = 0; n < This->ifd.cMembers; n++ )
	{
		pm = &This->ifd.pmethdata[n];
		if ( !lstrcmpiW(pm->szName,pName) )
			return pm;
	}

	return NULL;
}

static const METHODDATA* CTypeInfo2Impl_SearchMethodByDispIDAndFlags(
	CTypeInfo2Impl* This,DISPID dispid,UINT16 wFlags)
{
	DWORD	n;
	METHODDATA*	pm;

	for ( n = 0; n < This->ifd.cMembers; n++ )
	{
		pm = &This->ifd.pmethdata[n];
		if ( (pm->dispid == dispid) && (pm->wFlags & wFlags) )
			return pm;
	}

	return NULL;
}


static int CTypeInfo2Impl_SearchParamByName(
	const METHODDATA* pm,const OLECHAR* pName)
{
	PARAMDATA*	pP;
	DWORD	k;

	for ( k = 0; k < pm->cArgs; k++ )
	{
		pP = &pm->ppdata[k];
		if ( !lstrcmpiW(pP->szName,pName) )
			return (int)k;
	}

	return -1;
}


/****************************************************************************/

static HRESULT WINAPI CTypeInfo2Impl_fnQueryInterface(
	ITypeInfo2* iface,REFIID riid,void** ppvobj)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    TRACE("(%p)->(IID: %s)\n",This,debugstr_guid(riid));

    *ppvobj = NULL;
	if ( IsEqualIID(riid, &IID_IUnknown) ||
	     IsEqualIID(riid, &IID_ITypeInfo) ||
	     IsEqualIID(riid, &IID_ITypeInfo2) )
	{
		*ppvobj = (void*)iface;
		ITypeInfo2_AddRef(iface);
		return S_OK;
	}

	return E_NOINTERFACE;
}

static ULONG WINAPI CTypeInfo2Impl_fnAddRef(ITypeInfo2* iface)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

	return ++ This->ref;
}

static ULONG WINAPI CTypeInfo2Impl_fnRelease(ITypeInfo2* iface)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

	if ( -- This->ref > 0 ) return This->ref;

	++ This->ref;
	CTypeInfo2Impl_Destruct(This);
	HeapFree(GetProcessHeap(),0,This);
	return 0;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetTypeAttr(
	ITypeInfo2* iface,LPTYPEATTR* ppTypeAttr)
{
    ICOM_THIS(CTypeInfo2Impl,iface);
    TYPEATTR*	pta;

    FIXME("(%p)\n",This);

	if ( ppTypeAttr == NULL )
		return E_POINTER;
	pta = (TYPEATTR*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(TYPEATTR));
	if ( pta == NULL ) return E_OUTOFMEMORY;

	/* FIXME!!! */
	pta->lcid = This->lcid;
	pta->memidConstructor = MEMBERID_NIL;
	pta->memidDestructor = MEMBERID_NIL;
	pta->lpstrSchema = NULL;
	pta->cbSizeInstance = 0;
	pta->typekind = 0;
	pta->cFuncs = This->ifd.cMembers;
	pta->cVars = 0;
	pta->cImplTypes = 1;
	pta->cbSizeVft = sizeof(DWORD)*This->ifd.cMembers;
	pta->cbAlignment = sizeof(DWORD);
	pta->wTypeFlags = 0;
	pta->wMajorVerNum = 1;
	pta->wMinorVerNum = 0;
	ZeroMemory( &pta->idldescType, sizeof(pta->idldescType) );

	*ppTypeAttr = pta;
	return S_OK;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetTypeComp(
	ITypeInfo2* iface,ITypeComp** ppTComp)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetFuncDesc(
	ITypeInfo2* iface,UINT index,LPFUNCDESC* ppFuncDesc)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetVarDesc(
	ITypeInfo2* iface,UINT index,LPVARDESC* ppVarDesc)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetNames(
	ITypeInfo2* iface, MEMBERID memid,BSTR* rgBstrNames,
	UINT cMaxNames, UINT* pcNames)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetRefTypeOfImplType(
	ITypeInfo2* iface,UINT index,HREFTYPE* pRefType)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetImplTypeFlags(
	ITypeInfo2* iface,UINT index,INT* pImplTypeFlags)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetIDsOfNames(
	ITypeInfo2* iface,LPOLESTR* rgszNames,UINT cNames,MEMBERID* pMemId)
{
    ICOM_THIS(CTypeInfo2Impl,iface);
	const METHODDATA*	pm;
	DWORD	n;
	int	index;

	TRACE("(%p,%s,%u,%p)\n",This,debugstr_w(*rgszNames),cNames,pMemId);

	if ( rgszNames == NULL || pMemId == NULL ) return E_POINTER;
	if ( cNames <= 0 ) return E_INVALIDARG;
	for ( n = 0; n < cNames; n++ )
	{
		if ( rgszNames[n] == NULL ) return E_POINTER;
	}

	pm = CTypeInfo2Impl_SearchMethodByName(This,rgszNames[0]);
	if ( pm == NULL ) return DISP_E_UNKNOWNNAME;
	pMemId[0] = (MEMBERID)pm->dispid;

	for ( n = 1; n < cNames; n++ )
	{
		index = CTypeInfo2Impl_SearchParamByName(pm,rgszNames[n]);
		if ( index < 0 ) return DISP_E_UNKNOWNNAME;
		pMemId[n] = (MEMBERID)index;
	}

	return S_OK;
}

static HRESULT WINAPI CTypeInfo2Impl_fnInvoke(
	ITypeInfo2* iface,VOID* punk,MEMBERID memid,
	UINT16 wFlags,DISPPARAMS* pDispParams,VARIANT* pVarResult,
	EXCEPINFO* pExcepInfo,UINT* pArgErr)
{
    ICOM_THIS(CTypeInfo2Impl,iface);
	const METHODDATA*	pm;
	DWORD	res;
	HRESULT hr;
	int		n;
	DWORD	cargs;
	DWORD*	pargs = NULL;
	VARIANT	varError;
	VARIANT	varRet;

	FIXME("(%p,%p,%ld,%08x,%p,%p,%p,%p)\n",
		This,punk,(long)memid,(int)wFlags,
		pDispParams,pVarResult,pExcepInfo,pArgErr);

	if ( punk == NULL || pArgErr == NULL )
		return E_POINTER;

	pm = CTypeInfo2Impl_SearchMethodByDispIDAndFlags(
		This,memid,wFlags);
	if ( pm == NULL )
	{
		ERR("did not find member id %ld, flags %d!\n", (long)memid, (int)wFlags);
		return DISP_E_MEMBERNOTFOUND;
	}

	cargs = pm->cArgs + 1;
	pargs = (DWORD*)HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD)*(pm->cArgs+2) );
	if ( pargs == NULL ) { hr = E_OUTOFMEMORY; goto err_invoke; }

	V_VT(&varError) = VT_ERROR;
	pargs[0] = (DWORD)punk;
	for ( n = 1; n <= (int)pm->cArgs; n++ )
		pargs[n] = (DWORD)&varError; /* FIXME? */

	if ( (int)pDispParams->cArgs > (int)pm->cArgs )
	{
		hr = E_FAIL; /* FIXME? */
		goto err_invoke;
	}

	for ( n = 1; n <= (int)pm->cArgs; n++ )
	{
		if ( n <= (int)pDispParams->cNamedArgs )
		{
			/* FIXME - handle named args. */
			/* FIXME - check types. */

			/* pDispParams->rgdispidNamedArgs */
			/* pDispParams->cNamedArgs */
			FIXME("named args - %d\n",n);
			pargs[n] = V_UNION(&pDispParams->rgvarg[pDispParams->cArgs-n],lVal);
		}
		else
		{
			/* FIXME - check types. */
			pargs[n] = V_UNION(&pDispParams->rgvarg[pDispParams->cArgs-n],lVal);
		}
	}

	VariantInit( &varRet );
	if ( pm->vtReturn != VT_EMPTY &&
		 (!(wFlags & (DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF))) )
	{
		if ( pVarResult != NULL )
		{
			pargs[cargs] = (DWORD)pVarResult;
		}
		else
		{
			pargs[cargs] = (DWORD)(&varRet);
		}
		cargs ++;
	}

	res = _invoke(
		(*(DWORD***)punk)[pm->iMeth],
		pm->cc, cargs, pargs );
	VariantClear( &varRet );

	if ( res == (DWORD)-1 ) /* FIXME? */
	{
		hr = E_FAIL;
		goto err_invoke;
	}

	hr = S_OK;
err_invoke:
	HeapFree( GetProcessHeap(), 0, pargs );

	return hr;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetDocumentation(
	ITypeInfo2* iface,
	MEMBERID memid, BSTR* pBstrName, BSTR* pBstrDocString,
	DWORD* pdwHelpContext, BSTR* pBstrHelpFile)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetDllEntry(
	ITypeInfo2* iface,MEMBERID memid,
	INVOKEKIND invKind,BSTR* pBstrDllName,BSTR* pBstrName,
	WORD* pwOrdinal)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetRefTypeInfo(
	ITypeInfo2* iface,HREFTYPE hRefType,ITypeInfo** ppTInfo)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnAddressOfMember(
	ITypeInfo2 *iface,
	MEMBERID memid,INVOKEKIND invKind,PVOID* ppv)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnCreateInstance(
	ITypeInfo2* iface,
	IUnknown* punk,REFIID riid,VOID** ppvObj)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetMops(
	ITypeInfo2* iface,MEMBERID memid,BSTR* pBstrMops)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetContainingTypeLib(
	ITypeInfo2* iface,ITypeLib** ppTLib,UINT* pIndex)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnReleaseTypeAttr(
	ITypeInfo2* iface,TYPEATTR* pTypeAttr)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p)\n",This);

	if ( pTypeAttr != NULL )
		HeapFree(GetProcessHeap(),0,pTypeAttr);

	return S_OK;
}

static HRESULT WINAPI CTypeInfo2Impl_fnReleaseFuncDesc(
	ITypeInfo2* iface,FUNCDESC* pFuncDesc)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnReleaseVarDesc(
	ITypeInfo2* iface,VARDESC* pVarDesc)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetTypeKind(
	ITypeInfo2* iface,TYPEKIND* pTypeKind)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetTypeFlags(
	ITypeInfo2* iface,UINT* pTypeFlags)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetFuncIndexOfMemId(
	ITypeInfo2* iface,
    MEMBERID memid,INVOKEKIND invKind,UINT* pFuncIndex)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetVarIndexOfMemId(
	ITypeInfo2* iface,
    MEMBERID memid,UINT* pVarIndex)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetCustData(
	ITypeInfo2* iface,REFGUID guid,VARIANT* pVarVal)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetFuncCustData(
	ITypeInfo2* iface,UINT index,REFGUID guid,VARIANT* pVarVal)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetParamCustData(
	ITypeInfo2* iface,UINT indexFunc,UINT indexParam,
	REFGUID guid,VARIANT* pVarVal)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetVarCustData(
	ITypeInfo2* iface,UINT index,REFGUID guid,VARIANT* pVarVal)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetImplTypeCustData(
	ITypeInfo2 * iface,UINT index,REFGUID guid,VARIANT* pVarVal)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetDocumentation2(
	ITypeInfo2* iface,MEMBERID memid,LCID lcid,
	BSTR* pbstrHelpString,DWORD* pdwHelpStringContext,BSTR* pbstrHelpStringDll)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetAllCustData(
	ITypeInfo2* iface,CUSTDATA* pCustData)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetAllFuncCustData(
	ITypeInfo2* iface,UINT index,CUSTDATA* pCustData)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetAllParamCustData(
	ITypeInfo2* iface,UINT indexFunc,UINT indexParam,CUSTDATA* pCustData)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetAllVarCustData(
	ITypeInfo2* iface,UINT index,CUSTDATA* pCustData)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI CTypeInfo2Impl_fnGetAllImplTypeCustData(
	ITypeInfo2* iface,UINT index,CUSTDATA* pCustData)
{
    ICOM_THIS(CTypeInfo2Impl,iface);

    FIXME("(%p) stub!\n",This);

	return E_NOTIMPL;
}

static ICOM_VTABLE(ITypeInfo2) itypeinfo2 =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE

	/* IUnknown */
    CTypeInfo2Impl_fnQueryInterface,
    CTypeInfo2Impl_fnAddRef,
    CTypeInfo2Impl_fnRelease,
	/* ITypeInfo */
    CTypeInfo2Impl_fnGetTypeAttr,
    CTypeInfo2Impl_fnGetTypeComp,
    CTypeInfo2Impl_fnGetFuncDesc,
    CTypeInfo2Impl_fnGetVarDesc,
    CTypeInfo2Impl_fnGetNames,
    CTypeInfo2Impl_fnGetRefTypeOfImplType,
    CTypeInfo2Impl_fnGetImplTypeFlags,
    CTypeInfo2Impl_fnGetIDsOfNames,
    CTypeInfo2Impl_fnInvoke,
    CTypeInfo2Impl_fnGetDocumentation,
    CTypeInfo2Impl_fnGetDllEntry,
    CTypeInfo2Impl_fnGetRefTypeInfo,
    CTypeInfo2Impl_fnAddressOfMember,
    CTypeInfo2Impl_fnCreateInstance,
    CTypeInfo2Impl_fnGetMops,
    CTypeInfo2Impl_fnGetContainingTypeLib,
    CTypeInfo2Impl_fnReleaseTypeAttr,
    CTypeInfo2Impl_fnReleaseFuncDesc,
    CTypeInfo2Impl_fnReleaseVarDesc,
	/* ITypeInfo2 */
    CTypeInfo2Impl_fnGetTypeKind,
    CTypeInfo2Impl_fnGetTypeFlags,
    CTypeInfo2Impl_fnGetFuncIndexOfMemId,
    CTypeInfo2Impl_fnGetVarIndexOfMemId,
    CTypeInfo2Impl_fnGetCustData,
    CTypeInfo2Impl_fnGetFuncCustData,
    CTypeInfo2Impl_fnGetParamCustData,
    CTypeInfo2Impl_fnGetVarCustData,
    CTypeInfo2Impl_fnGetImplTypeCustData,
    CTypeInfo2Impl_fnGetDocumentation2,
    CTypeInfo2Impl_fnGetAllCustData,
    CTypeInfo2Impl_fnGetAllFuncCustData,
    CTypeInfo2Impl_fnGetAllParamCustData,
    CTypeInfo2Impl_fnGetAllVarCustData,
    CTypeInfo2Impl_fnGetAllImplTypeCustData,
};


/******************************************************************************
 *		CreateDispTypeInfo  (OLEAUT32.31)
 */
HRESULT WINAPI CreateDispTypeInfo(
	INTERFACEDATA* pifd,
	LCID lcid,
	ITypeInfo** ppinfo )
{
	HRESULT hr;
	CTypeInfo2Impl*	This;

	if ( ppinfo == NULL )
		return E_POINTER;

	This = (CTypeInfo2Impl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(CTypeInfo2Impl));
	if ( This == NULL ) return E_OUTOFMEMORY;
	ICOM_VTBL(This) = &itypeinfo2;
	This->ref = 1;

	hr = CTypeInfo2Impl_Construct(This,pifd,lcid);
	if ( FAILED(hr) )
	{
		IUnknown_Release((IUnknown*)This);
		return hr;
	}
	*ppinfo = (ITypeInfo*)This;

	return S_OK;
}
