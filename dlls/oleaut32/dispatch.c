/**
 * Dispatch API functions
 *
 * Copyright 2000  Francois Jacques, Macadamian Technologies Inc.
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
 * TODO: Type coercion is implemented in variant.c but not called yet.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "windef.h"
#include "objbase.h"
#include "oleauto.h"
#include "winerror.h"
#include "winreg.h"         /* for HKEY_LOCAL_MACHINE */
#include "winnls.h"         /* for PRIMARYLANGID */

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);
WINE_DECLARE_DEBUG_CHANNEL(typelib);

static IDispatch * WINAPI StdDispatch_Construct(IUnknown * punkOuter, void * pvThis, ITypeInfo * pTypeInfo);

/******************************************************************************
 *		DispInvoke (OLEAUT32.30)
 *
 *
 * Calls method of an object through its IDispatch interface.
 *
 * NOTES
 * 		- Defer	method invocation to ITypeInfo::Invoke()
 *
 * RETURNS
 *
 * 		S_OK on success.
 */
HRESULT WINAPI DispInvoke(
	VOID       *_this,        /* [in] object instance */
	ITypeInfo  *ptinfo,       /* [in] object's type info */
	DISPID      dispidMember, /* [in] member id */
	USHORT      wFlags,       /* [in] kind of method call */
	DISPPARAMS *pparams,      /* [in] array of arguments */
	VARIANT    *pvarResult,   /* [out] result of method call */
	EXCEPINFO  *pexcepinfo,   /* [out] information about exception */
	UINT       *puArgErr)     /* [out] index of bad argument(if any) */
{
    HRESULT hr = E_FAIL;

    /**
     * TODO:
     * For each param, call DispGetParam to perform type coercion
     */
    FIXME("Coercion of arguments not implemented\n");

    ITypeInfo_Invoke(ptinfo, _this, dispidMember, wFlags,
                    pparams, pvarResult, pexcepinfo, puArgErr);

    return (hr);
}


/******************************************************************************
 *		DispGetIDsOfNames (OLEAUT32.29)
 *
 * Convert a set of names to dispids, based on information
 * contained in object's type library.
 *
 * NOTES
 * 		- Defers to ITypeInfo::GetIDsOfNames()
 *
 * RETURNS
 *
 * 		S_OK on success.
 */
HRESULT WINAPI DispGetIDsOfNames(
	ITypeInfo  *ptinfo,    /* [in] */
	OLECHAR   **rgszNames, /* [in] */
	UINT        cNames,    /* [in] */
	DISPID     *rgdispid)  /* [out] */
{
    HRESULT hr = E_FAIL;

    ITypeInfo_GetIDsOfNames(ptinfo, rgszNames, cNames, rgdispid);
    return (hr);
}

/******************************************************************************
 *		DispGetParam (OLEAUT32.28)
 *
 * Retrive a parameter from a DISPPARAMS structures and coerce it to
 * specified variant type
 *
 * NOTES
 * 		Coercion is done using system (0) locale.
 *
 * RETURNS
 *
 * 		S_OK on success.
 */
HRESULT WINAPI DispGetParam(
	DISPPARAMS *pdispparams, /* [in] */
	UINT        position,    /* [in] */
	VARTYPE     vtTarg,      /* [in] */
	VARIANT    *pvarResult,  /* [out] */
	UINT       *puArgErr)    /* [out] */
{
    /* position is counted backwards */
    UINT pos;
    HRESULT hr;

    TRACE("position=%d, cArgs=%d, cNamedArgs=%d\n",
          position, pdispparams->cArgs, pdispparams->cNamedArgs);
    if (position < pdispparams->cArgs) {
      /* positional arg? */
      pos = pdispparams->cArgs - position - 1;
    } else {
      /* FIXME: is this how to handle named args? */
      for (pos=0; pos<pdispparams->cNamedArgs; pos++)
        if (pdispparams->rgdispidNamedArgs[pos] == position) break;

      if (pos==pdispparams->cNamedArgs)
        return DISP_E_PARAMNOTFOUND;
    }
    hr = VariantChangeType(pvarResult,
                           &pdispparams->rgvarg[pos],
                           0, vtTarg);
    if (hr == DISP_E_TYPEMISMATCH) *puArgErr = pos;
    return hr;
}

/******************************************************************************
 * CreateStdDispatch [OLEAUT32.32]
 */
HRESULT WINAPI CreateStdDispatch(
        IUnknown* punkOuter,
        void* pvThis,
	ITypeInfo* ptinfo,
	IUnknown** ppunkStdDisp)
{
    TRACE("(%p, %p, %p, %p)\n", punkOuter, pvThis, ptinfo, ppunkStdDisp);

    *ppunkStdDisp = (LPUNKNOWN)StdDispatch_Construct(punkOuter, pvThis, ptinfo);
    if (!*ppunkStdDisp)
        return E_OUTOFMEMORY;
    return S_OK;
}

/******************************************************************************
 * CreateDispTypeInfo [OLEAUT32.31]
 */
HRESULT WINAPI CreateDispTypeInfo(
	INTERFACEDATA *pidata,
	LCID lcid,
	ITypeInfo **pptinfo)
{
	FIXME("(%p,%ld,%p),stub\n",pidata,lcid,pptinfo);
	return 0;
}

typedef struct
{
    ICOM_VFIELD(IDispatch);
    IUnknown * outerUnknown;
    void * pvThis;
    ITypeInfo * pTypeInfo;
    ULONG ref;
} StdDispatch;

static HRESULT WINAPI StdDispatch_QueryInterface(
  LPDISPATCH iface,
  REFIID riid,
  void** ppvObject)
{
    ICOM_THIS(StdDispatch, iface);
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppvObject);

    if (This->outerUnknown)
        return IUnknown_QueryInterface(This->outerUnknown, riid, ppvObject);

    if (IsEqualIID(riid, &IID_IDispatch) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObject = (LPVOID)This;
	IUnknown_AddRef((LPUNKNOWN)*ppvObject);
	return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI StdDispatch_AddRef(LPDISPATCH iface)
{
    ICOM_THIS(StdDispatch, iface);
    TRACE("()\n");
    This->ref++;
    if (This->outerUnknown)
        return IUnknown_AddRef(This->outerUnknown);
    else
        return This->ref;
}

static ULONG WINAPI StdDispatch_Release(LPDISPATCH iface)
{
    ICOM_THIS(StdDispatch, iface);
    ULONG ret;
    TRACE("(%p)->()\n", This);

    This->ref--;

    if (This->outerUnknown)
        ret = IUnknown_Release(This->outerUnknown);
    else
        ret = This->ref;

    if (This->ref <= 0)
        CoTaskMemFree(This);

    return ret;
}

static HRESULT WINAPI StdDispatch_GetTypeInfoCount(LPDISPATCH iface, UINT * pctinfo)
{
    TRACE("(%p)\n", pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI StdDispatch_GetTypeInfo(LPDISPATCH iface, UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{
    ICOM_THIS(StdDispatch, iface);
    TRACE("(%d, %lx, %p)\n", iTInfo, lcid, ppTInfo);

    if (iTInfo != 0)
        return DISP_E_BADINDEX;
    *ppTInfo = This->pTypeInfo;
    return S_OK;
}

static HRESULT WINAPI StdDispatch_GetIDsOfNames(LPDISPATCH iface, REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId)
{
    ICOM_THIS(StdDispatch, iface);
    TRACE("(%s, %p, %d, 0x%lx, %p)\n", debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    if (!IsEqualGUID(riid, &IID_NULL))
    {
        FIXME(" expected riid == IID_NULL\n");
        return E_INVALIDARG;
    }
    return DispGetIDsOfNames(This->pTypeInfo, rgszNames, cNames, rgDispId);
}

static HRESULT WINAPI StdDispatch_Invoke(LPDISPATCH iface, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pDispParams, VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr)
{
    ICOM_THIS(StdDispatch, iface);
    TRACE("(%ld, %s, 0x%lx, 0x%x, %p, %p, %p, %p)\n", dispIdMember, debugstr_guid(riid), lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    if (!IsEqualGUID(riid, &IID_NULL))
    {
        FIXME(" expected riid == IID_NULL\n");
        return E_INVALIDARG;
    }
    return DispInvoke(This->pvThis, This->pTypeInfo, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static ICOM_VTABLE(IDispatch) StdDispatch_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  StdDispatch_QueryInterface,
  StdDispatch_AddRef,
  StdDispatch_Release,
  StdDispatch_GetTypeInfoCount,
  StdDispatch_GetTypeInfo,
  StdDispatch_GetIDsOfNames,
  StdDispatch_Invoke
};

static IDispatch * WINAPI StdDispatch_Construct(
  IUnknown * punkOuter,
  void * pvThis,
  ITypeInfo * pTypeInfo)
{
    StdDispatch * pStdDispatch;

    pStdDispatch = CoTaskMemAlloc(sizeof(StdDispatch));
    if (!pStdDispatch)
        return (IDispatch *)pStdDispatch;

    pStdDispatch->lpVtbl = &StdDispatch_VTable;
    pStdDispatch->outerUnknown = punkOuter;
    pStdDispatch->pvThis = pvThis;
    pStdDispatch->pTypeInfo = pTypeInfo;
    pStdDispatch->ref = 1;

    return (IDispatch *)pStdDispatch;
}
