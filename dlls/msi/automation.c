/*
 * Implementation of OLE Automation for Microsoft Installer (msi.dll)
 *
 * Copyright 2007 Misha Koshelev
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#include "msidefs.h"
#include "msipriv.h"
#include "activscp.h"
#include "oleauto.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "msiserver.h"
#include "msiserver_dispids.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/*
 * AutomationObject - "base" class for all automation objects. For each interface, we implement Invoke function
 *                    called from AutomationObject::Invoke, and pass this function to create_automation_object.
 */

typedef interface AutomationObject AutomationObject;

interface AutomationObject {
    /*
     * VTables - We provide IDispatch, IProvideClassInfo, IProvideClassInfo2, IProvideMultipleClassInfo
     */
    const IDispatchVtbl *lpVtbl;
    const IProvideMultipleClassInfoVtbl *lpvtblIProvideMultipleClassInfo;

    /* Object reference count */
    LONG ref;

    /* Clsid for this class and it's appropriate ITypeInfo object */
    LPCLSID clsid;
    ITypeInfo *iTypeInfo;

    /* The MSI handle of the current object */
    MSIHANDLE msiHandle;

    /* A function that is called from AutomationObject::Invoke, specific to this type of object. */
    HRESULT (STDMETHODCALLTYPE *funcInvoke)(
        AutomationObject* This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr);
};

/* VTables */
static const struct IDispatchVtbl AutomationObject_Vtbl;
static const struct IProvideMultipleClassInfoVtbl AutomationObject_IProvideMultipleClassInfo_Vtbl;

/* Load type info so we don't have to process GetIDsOfNames */
HRESULT load_type_info(IDispatch *iface, ITypeInfo **pptinfo, REFIID clsid, LCID lcid)
{
    HRESULT hr;
    LPTYPELIB pLib = NULL;
    LPTYPEINFO pInfo = NULL;
    static const WCHAR szMsiServer[] = {'m','s','i','s','e','r','v','e','r','.','t','l','b'};

    TRACE("(%p)->(%s,%d)\n", iface, debugstr_guid(clsid), lcid);

    /* Load registered type library */
    hr = LoadRegTypeLib(&LIBID_WindowsInstaller, 1, 0, lcid, &pLib);
    if (FAILED(hr)) {
	hr = LoadTypeLib(szMsiServer, &pLib);
	if (FAILED(hr)) {
	    ERR("Could not load msiserver.tlb\n");
	    return hr;
	}
    }

    /* Get type information for object */
    hr = ITypeLib_GetTypeInfoOfGuid(pLib, clsid, &pInfo);
    ITypeLib_Release(pLib);
    if (FAILED(hr)) {
	ERR("Could not load ITypeInfo for %s\n", debugstr_guid(clsid));
	return hr;
    }
    *pptinfo = pInfo;
    return S_OK;
}

/* Create the automation object, placing the result in the pointer ppObj. The automation object is created
 * with the appropriate clsid and invocation function. */
HRESULT create_automation_object(MSIHANDLE msiHandle, IUnknown *pUnkOuter, LPVOID *ppObj, REFIID clsid,
	    HRESULT (STDMETHODCALLTYPE *funcInvoke)(AutomationObject*,DISPID,REFIID,LCID,WORD,DISPPARAMS*,
						    VARIANT*,EXCEPINFO*,UINT*))
{
    AutomationObject *object;
    HRESULT hr;

    TRACE("(%ld,%p,%p,%s,%p)\n", (unsigned long)msiHandle, pUnkOuter, ppObj, debugstr_guid(clsid), funcInvoke);

    if( pUnkOuter )
	return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(AutomationObject));

    /* Set all the VTable references */
    object->lpVtbl = &AutomationObject_Vtbl;
    object->lpvtblIProvideMultipleClassInfo = &AutomationObject_IProvideMultipleClassInfo_Vtbl;
    object->ref = 1;

    /* Store data that was passed */
    object->msiHandle = msiHandle;
    object->clsid = (LPCLSID)clsid;
    object->funcInvoke = funcInvoke;

    /* Load our TypeInfo so we don't have to process GetIDsOfNames */
    object->iTypeInfo = NULL;
    hr = load_type_info((IDispatch *)object, &object->iTypeInfo, clsid, 0x0);
    if (FAILED(hr)) {
	HeapFree(GetProcessHeap(), 0, object);
	return hr;
    }

    *ppObj = object;

    return S_OK;
}

/* Macros to get pointer to AutomationObject from the other VTables. */
static inline AutomationObject *obj_from_IProvideMultipleClassInfo( IProvideMultipleClassInfo *iface )
{
    return (AutomationObject *)((char*)iface - FIELD_OFFSET(AutomationObject, lpvtblIProvideMultipleClassInfo));
}

/*
 * AutomationObject methods
 */

/*** IUnknown methods ***/
static HRESULT WINAPI AutomationObject_QueryInterface(IDispatch* iface, REFIID riid, void** ppvObject)
{
    AutomationObject *This = (AutomationObject *)iface;

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

    if (ppvObject == NULL)
      return E_INVALIDARG;

    *ppvObject = 0;

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDispatch) || IsEqualGUID(riid, This->clsid))
        *ppvObject = This;
    else if (IsEqualGUID(riid, &IID_IProvideClassInfo) ||
             IsEqualGUID(riid, &IID_IProvideClassInfo2) ||
             IsEqualGUID(riid, &IID_IProvideMultipleClassInfo))
        *ppvObject = &This->lpvtblIProvideMultipleClassInfo;
    else
    {
	TRACE("() : asking for unsupported interface %s\n",debugstr_guid(riid));
	return E_NOINTERFACE;
    }

    /*
     * Query Interface always increases the reference count by one when it is
     * successful
     */
    IClassFactory_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI AutomationObject_AddRef(IDispatch* iface)
{
    AutomationObject *This = (AutomationObject *)iface;

    TRACE("(%p/%p)\n", iface, This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI AutomationObject_Release(IDispatch* iface)
{
    AutomationObject *This = (AutomationObject *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)\n", iface, This);

    if (!ref)
    {
	MsiCloseHandle(This->msiHandle);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** IDispatch methods ***/
static HRESULT WINAPI AutomationObject_GetTypeInfoCount(
        IDispatch* iface,
        UINT* pctinfo)
{
    AutomationObject *This = (AutomationObject *)iface;

    TRACE("(%p/%p)->(%p)\n", iface, This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI AutomationObject_GetTypeInfo(
        IDispatch* iface,
        UINT iTInfo,
        LCID lcid,
        ITypeInfo** ppTInfo)
{
    AutomationObject *This = (AutomationObject *)iface;
    TRACE("(%p/%p)->(%d,%d,%p)\n", iface, This, iTInfo, lcid, ppTInfo);

    ITypeInfo_AddRef(This->iTypeInfo);
    *ppTInfo = This->iTypeInfo;
    return S_OK;
}

static HRESULT WINAPI AutomationObject_GetIDsOfNames(
        IDispatch* iface,
        REFIID riid,
        LPOLESTR* rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID* rgDispId)
{
    AutomationObject *This = (AutomationObject *)iface;
    TRACE("(%p/%p)->(%p,%p,%d,%d,%p)\n", iface, This, riid, rgszNames, cNames, lcid, rgDispId);

    if (!IsEqualGUID(riid, &IID_NULL)) return E_INVALIDARG;
    return ITypeInfo_GetIDsOfNames(This->iTypeInfo, rgszNames, cNames, rgDispId);
}

/* Maximum number of allowed function parameters+1 */
#define MAX_FUNC_PARAMS 20

/* Some error checking is done here to simplify individual object function invocation */
static HRESULT WINAPI AutomationObject_Invoke(
        IDispatch* iface,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
{
    AutomationObject *This = (AutomationObject *)iface;
    HRESULT hr;
    unsigned int uArgErr;
    VARIANT varResultDummy;
    BSTR bstrName = NULL;

    TRACE("(%p/%p)->(%d,%p,%d,%d,%p,%p,%p,%p)\n", iface, This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    if (!IsEqualIID(riid, &IID_NULL))
    {
	ERR("riid was %s instead of IID_NULL\n", debugstr_guid(riid));
	return DISP_E_UNKNOWNNAME;
    }

    if (wFlags & DISPATCH_PROPERTYGET && !pVarResult)
    {
	ERR("NULL pVarResult not allowed when DISPATCH_PROPERTYGET specified\n");
	return DISP_E_PARAMNOTOPTIONAL;
    }

    /* This simplifies our individual object invocation functions */
    if (puArgErr == NULL) puArgErr = &uArgErr;
    if (pVarResult == NULL) pVarResult = &varResultDummy;

    /* Assume return type is void unless determined otherwise */
    VariantInit(pVarResult);

    /* If we are tracing, we want to see the name of the member we are invoking */
    if (TRACE_ON(msi))
    {
	ITypeInfo_GetDocumentation(This->iTypeInfo, dispIdMember, &bstrName, NULL, NULL, NULL);
	TRACE("Method %d, %s\n", dispIdMember, debugstr_w(bstrName));
    }

    hr = This->funcInvoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr);

    if (hr == DISP_E_MEMBERNOTFOUND) {
	if (bstrName == NULL) ITypeInfo_GetDocumentation(This->iTypeInfo, dispIdMember, &bstrName, NULL, NULL, NULL);
	FIXME("Method %d, %s wflags %d not implemented, clsid %s\n", dispIdMember, debugstr_w(bstrName), wFlags, debugstr_guid(This->clsid));
    }
    else if (pExcepInfo &&
             (hr == DISP_E_PARAMNOTFOUND ||
              hr == DISP_E_EXCEPTION)) {
        static const WCHAR szComma[] = { ',',0 };
        static WCHAR szExceptionSource[] = {'M','s','i',' ','A','P','I',' ','E','r','r','o','r',0};
        WCHAR szExceptionDescription[MAX_PATH];
        BSTR bstrParamNames[MAX_FUNC_PARAMS];
        unsigned namesNo, i;
        BOOL bFirst = TRUE;

        if (FAILED(ITypeInfo_GetNames(This->iTypeInfo, dispIdMember, bstrParamNames,
                                      MAX_FUNC_PARAMS, &namesNo)))
        {
            TRACE("Failed to retrieve names for dispIdMember %d\n", dispIdMember);
        }
        else
        {
            memset(szExceptionDescription, 0, sizeof(szExceptionDescription));
            for (i=0; i<namesNo; i++)
            {
                if (bFirst) bFirst = FALSE;
                else {
                    lstrcpyW(&szExceptionDescription[lstrlenW(szExceptionDescription)], szComma);
                }
                lstrcpyW(&szExceptionDescription[lstrlenW(szExceptionDescription)], bstrParamNames[i]);
                SysFreeString(bstrParamNames[i]);
            }

            memset(pExcepInfo, 0, sizeof(EXCEPINFO));
            pExcepInfo->wCode = 1000;
            pExcepInfo->bstrSource = szExceptionSource;
            pExcepInfo->bstrDescription = szExceptionDescription;
            hr = DISP_E_EXCEPTION;
        }
    }

    /* Make sure we free the return variant if it is our dummy variant */
    if (pVarResult == &varResultDummy) VariantClear(pVarResult);

    /* Free function name if we retrieved it */
    if (bstrName) SysFreeString(bstrName);

    TRACE("Returning 0x%08x, %s\n", hr, SUCCEEDED(hr) ? "ok" : "not ok");

    return hr;
}

static const struct IDispatchVtbl AutomationObject_Vtbl =
{
    AutomationObject_QueryInterface,
    AutomationObject_AddRef,
    AutomationObject_Release,
    AutomationObject_GetTypeInfoCount,
    AutomationObject_GetTypeInfo,
    AutomationObject_GetIDsOfNames,
    AutomationObject_Invoke
};

/*
 * IProvideMultipleClassInfo methods
 */

static HRESULT WINAPI AutomationObject_IProvideMultipleClassInfo_QueryInterface(
  IProvideMultipleClassInfo* iface,
  REFIID     riid,
  VOID**     ppvoid)
{
    AutomationObject *This = obj_from_IProvideMultipleClassInfo(iface);
    return AutomationObject_QueryInterface((IDispatch *)This, riid, ppvoid);
}

static ULONG WINAPI AutomationObject_IProvideMultipleClassInfo_AddRef(IProvideMultipleClassInfo* iface)
{
    AutomationObject *This = obj_from_IProvideMultipleClassInfo(iface);
    return AutomationObject_AddRef((IDispatch *)This);
}

static ULONG WINAPI AutomationObject_IProvideMultipleClassInfo_Release(IProvideMultipleClassInfo* iface)
{
    AutomationObject *This = obj_from_IProvideMultipleClassInfo(iface);
    return AutomationObject_Release((IDispatch *)This);
}

static HRESULT WINAPI AutomationObject_IProvideMultipleClassInfo_GetClassInfo(IProvideMultipleClassInfo* iface, ITypeInfo** ppTI)
{
    AutomationObject *This = obj_from_IProvideMultipleClassInfo(iface);
    TRACE("(%p/%p)->(%p)\n", iface, This, ppTI);
    return load_type_info((IDispatch *)This, ppTI, This->clsid, 0);
}

static HRESULT WINAPI AutomationObject_IProvideMultipleClassInfo_GetGUID(IProvideMultipleClassInfo* iface, DWORD dwGuidKind, GUID* pGUID)
{
    AutomationObject *This = obj_from_IProvideMultipleClassInfo(iface);
    TRACE("(%p/%p)->(%d,%s)\n", iface, This, dwGuidKind, debugstr_guid(pGUID));

    if (dwGuidKind != GUIDKIND_DEFAULT_SOURCE_DISP_IID)
	return E_INVALIDARG;
    else {
	*pGUID = *This->clsid;
	return S_OK;
    }
}

static HRESULT WINAPI AutomationObject_GetMultiTypeInfoCount(IProvideMultipleClassInfo* iface, ULONG* pcti)
{
    AutomationObject *This = obj_from_IProvideMultipleClassInfo(iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, pcti);
    *pcti = 1;
    return S_OK;
}

static HRESULT WINAPI AutomationObject_GetInfoOfIndex(IProvideMultipleClassInfo* iface,
        ULONG iti,
        DWORD dwFlags,
        ITypeInfo** pptiCoClass,
        DWORD* pdwTIFlags,
        ULONG* pcdispidReserved,
        IID* piidPrimary,
        IID* piidSource)
{
    AutomationObject *This = obj_from_IProvideMultipleClassInfo(iface);

    TRACE("(%p/%p)->(%d,%d,%p,%p,%p,%p,%p)\n", iface, This, iti, dwFlags, pptiCoClass, pdwTIFlags, pcdispidReserved, piidPrimary, piidSource);

    if (iti != 0)
	return E_INVALIDARG;

    if (dwFlags & MULTICLASSINFO_GETTYPEINFO)
        load_type_info((IDispatch *)This, pptiCoClass, This->clsid, 0);

    if (dwFlags & MULTICLASSINFO_GETNUMRESERVEDDISPIDS)
    {
	*pdwTIFlags = 0;
	*pcdispidReserved = 0;
    }

    if (dwFlags & MULTICLASSINFO_GETIIDPRIMARY){
	*piidPrimary = *This->clsid;
    }

    if (dwFlags & MULTICLASSINFO_GETIIDSOURCE){
        *piidSource = *This->clsid;
    }

    return S_OK;
}

static const IProvideMultipleClassInfoVtbl AutomationObject_IProvideMultipleClassInfo_Vtbl =
{
    AutomationObject_IProvideMultipleClassInfo_QueryInterface,
    AutomationObject_IProvideMultipleClassInfo_AddRef,
    AutomationObject_IProvideMultipleClassInfo_Release,
    AutomationObject_IProvideMultipleClassInfo_GetClassInfo,
    AutomationObject_IProvideMultipleClassInfo_GetGUID,
    AutomationObject_GetMultiTypeInfoCount,
    AutomationObject_GetInfoOfIndex
};

/*
 * Individual Object Invocation Functions
 */

static HRESULT WINAPI RecordImpl_Invoke(
        AutomationObject* This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
{
    WCHAR *szString;
    DWORD dwLen;
    UINT ret;
    VARIANTARG varg0, varg1;
    HRESULT hr;

    VariantInit(&varg0);
    VariantInit(&varg1);

    switch (dispIdMember)
    {
	case DISPID_RECORD_STRINGDATA:
	    if (wFlags & DISPATCH_PROPERTYGET) {
                hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                V_VT(pVarResult) = VT_BSTR;
                V_BSTR(pVarResult) = NULL;
		ret = MsiRecordGetStringW(This->msiHandle, V_I4(&varg0), NULL, &dwLen);
                if (ret == ERROR_SUCCESS)
                {
                    szString = msi_alloc((++dwLen)*sizeof(WCHAR));
                    if (szString)
                    {
                        if ((ret = MsiRecordGetStringW(This->msiHandle, V_I4(&varg0), szString, &dwLen)) == ERROR_SUCCESS)
                            V_BSTR(pVarResult) = SysAllocString(szString);
                        msi_free(szString);
                    }
                }
                if (ret != ERROR_SUCCESS)
		    ERR("MsiRecordGetString returned %d\n", ret);
	    } else if (wFlags & DISPATCH_PROPERTYPUT) {
                hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                hr = DispGetParam(pDispParams, DISPID_PROPERTYPUT, VT_BSTR, &varg1, puArgErr);
                if (FAILED(hr)) return hr;
		if ((ret = MsiRecordSetStringW(This->msiHandle, V_I4(&varg0), V_BSTR(&varg1))) != ERROR_SUCCESS)
                {
                    ERR("MsiRecordSetString returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
	    }
	    break;

         default:
            return DISP_E_MEMBERNOTFOUND;
    }

    return S_OK;
}

static HRESULT WINAPI ViewImpl_Invoke(
        AutomationObject* This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
{
    MSIHANDLE msiHandle;
    IDispatch *pDispatch = NULL;
    UINT ret;
    VARIANTARG varg0, varg1;
    HRESULT hr;

    VariantInit(&varg0);
    VariantInit(&varg1);

    switch (dispIdMember)
    {
	case DISPID_VIEW_EXECUTE:
	    if (wFlags & DISPATCH_METHOD)
	    {
                hr = DispGetParam(pDispParams, 0, VT_DISPATCH, &varg0, puArgErr);
                if (SUCCEEDED(hr) && V_DISPATCH(&varg0) != NULL)
                    MsiViewExecute(This->msiHandle, ((AutomationObject *)V_DISPATCH(&varg0))->msiHandle);
                else
                    MsiViewExecute(This->msiHandle, 0);
	    }
	    break;

	case DISPID_VIEW_FETCH:
	    if (wFlags & DISPATCH_METHOD)
	    {
                V_VT(pVarResult) = VT_DISPATCH;
                if ((ret = MsiViewFetch(This->msiHandle, &msiHandle)) == ERROR_SUCCESS)
                {
                    if (SUCCEEDED(create_automation_object(msiHandle, NULL, (LPVOID*)&pDispatch, &DIID_Record, RecordImpl_Invoke)))
                    {
                        IDispatch_AddRef(pDispatch);
                        V_DISPATCH(pVarResult) = pDispatch;
                    }
                }
		else if (ret == ERROR_NO_MORE_ITEMS)
                    V_DISPATCH(pVarResult) = NULL;
                else
                {
                    ERR("MsiViewFetch returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
	    }
	    break;

	case DISPID_VIEW_CLOSE:
	    if (wFlags & DISPATCH_METHOD)
	    {
		MsiViewClose(This->msiHandle);
	    }
	    break;

         default:
            return DISP_E_MEMBERNOTFOUND;
    }

    return S_OK;
}

static HRESULT WINAPI DatabaseImpl_Invoke(
        AutomationObject* This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
{
    MSIHANDLE msiHandle;
    IDispatch *pDispatch = NULL;
    UINT ret;
    VARIANTARG varg0, varg1;
    HRESULT hr;

    VariantInit(&varg0);
    VariantInit(&varg1);

    switch (dispIdMember)
    {
	case DISPID_DATABASE_OPENVIEW:
	    if (wFlags & DISPATCH_METHOD)
	    {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                V_VT(pVarResult) = VT_DISPATCH;
                if ((ret = MsiDatabaseOpenViewW(This->msiHandle, V_BSTR(&varg0), &msiHandle)) == ERROR_SUCCESS)
                {
                    if (SUCCEEDED(create_automation_object(msiHandle, NULL, (LPVOID*)&pDispatch, &DIID_View, ViewImpl_Invoke)))
                    {
                        IDispatch_AddRef(pDispatch);
                        V_DISPATCH(pVarResult) = pDispatch;
                    }
                }
		else
                {
                    ERR("MsiDatabaseOpenView returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
	    }
	    break;

         default:
            return DISP_E_MEMBERNOTFOUND;
    }

    return S_OK;
}

static HRESULT WINAPI SessionImpl_Invoke(
        AutomationObject* This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
{
    WCHAR *szString;
    DWORD dwLen;
    IDispatch *pDispatch = NULL;
    MSIHANDLE msiHandle;
    LANGID langId;
    UINT ret;
    INSTALLSTATE iInstalled, iAction;
    VARIANTARG varg0, varg1;
    HRESULT hr;

    VariantInit(&varg0);
    VariantInit(&varg1);

    switch (dispIdMember)
    {
	case DISPID_SESSION_PROPERTY:
	    if (wFlags & DISPATCH_PROPERTYGET) {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                V_VT(pVarResult) = VT_BSTR;
                V_BSTR(pVarResult) = NULL;
		ret = MsiGetPropertyW(This->msiHandle, V_BSTR(&varg0), NULL, &dwLen);
                if (ret == ERROR_SUCCESS)
                {
                    szString = msi_alloc((++dwLen)*sizeof(WCHAR));
                    if (szString)
                    {
                        if ((ret = MsiGetPropertyW(This->msiHandle, V_BSTR(&varg0), szString, &dwLen)) == ERROR_SUCCESS)
                            V_BSTR(pVarResult) = SysAllocString(szString);
                        msi_free(szString);
                    }
                }
                if (ret != ERROR_SUCCESS)
		    ERR("MsiGetProperty returned %d\n", ret);
	    } else if (wFlags & DISPATCH_PROPERTYPUT) {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                hr = DispGetParam(pDispParams, DISPID_PROPERTYPUT, VT_BSTR, &varg1, puArgErr);
                if (FAILED(hr)) {
                    VariantClear(&varg0);
                    return hr;
                }
		if ((ret = MsiSetPropertyW(This->msiHandle, V_BSTR(&varg0), V_BSTR(&varg1))) != ERROR_SUCCESS)
                {
                    ERR("MsiSetProperty returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
	    }
	    break;

	case DISPID_SESSION_LANGUAGE:
	    if (wFlags & DISPATCH_PROPERTYGET) {
		langId = MsiGetLanguage(This->msiHandle);
                V_VT(pVarResult) = VT_I4;
                V_I4(pVarResult) = langId;
	    }
	    break;

	case DISPID_SESSION_MODE:
	    if (wFlags & DISPATCH_PROPERTYGET) {
                hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                V_VT(pVarResult) = VT_BOOL;
		V_BOOL(pVarResult) = MsiGetMode(This->msiHandle, V_I4(&varg0));
	    } else if (wFlags & DISPATCH_PROPERTYPUT) {
                hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                hr = DispGetParam(pDispParams, DISPID_PROPERTYPUT, VT_BOOL, &varg1, puArgErr);
                if (FAILED(hr)) return hr;
		if ((ret = MsiSetMode(This->msiHandle, V_I4(&varg0), V_BOOL(&varg1))) != ERROR_SUCCESS)
                {
                    ERR("MsiSetMode returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
	    }
	    break;

	case DISPID_SESSION_DATABASE:
	    if (wFlags & DISPATCH_PROPERTYGET) {
                V_VT(pVarResult) = VT_DISPATCH;
		if ((msiHandle = MsiGetActiveDatabase(This->msiHandle)))
                {
                    if (SUCCEEDED(create_automation_object(msiHandle, NULL, (LPVOID*)&pDispatch, &DIID_Database, DatabaseImpl_Invoke)))
                    {
                        IDispatch_AddRef(pDispatch);
                        V_DISPATCH(pVarResult) = pDispatch;
                    }
                }
		else
                {
                    ERR("MsiGetActiveDatabase failed\n");
                    return DISP_E_EXCEPTION;
                }
	    }
	    break;

        case DISPID_SESSION_DOACTION:
            if (wFlags & DISPATCH_METHOD) {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                ret = MsiDoActionW(This->msiHandle, V_BSTR(&varg0));
                V_VT(pVarResult) = VT_I4;
                switch (ret)
                {
                    case ERROR_FUNCTION_NOT_CALLED:
                        V_I4(pVarResult) = msiDoActionStatusNoAction;
                        break;
                    case ERROR_SUCCESS:
                        V_I4(pVarResult) = msiDoActionStatusSuccess;
                        break;
                    case ERROR_INSTALL_USEREXIT:
                        V_I4(pVarResult) = msiDoActionStatusUserExit;
                        break;
                    case ERROR_INSTALL_FAILURE:
                        V_I4(pVarResult) = msiDoActionStatusFailure;
                        break;
                    case ERROR_INSTALL_SUSPEND:
                        V_I4(pVarResult) = msiDoActionStatusSuspend;
                        break;
                    case ERROR_MORE_DATA:
                        V_I4(pVarResult) = msiDoActionStatusFinished;
                        break;
                    case ERROR_INVALID_HANDLE_STATE:
                        V_I4(pVarResult) = msiDoActionStatusWrongState;
                        break;
                    case ERROR_INVALID_DATA:
                        V_I4(pVarResult) = msiDoActionStatusBadActionData;
                        break;
                    default:
                        FIXME("MsiDoAction returned unhandled value %d\n", ret);
                        return DISP_E_EXCEPTION;
                }
            }
            break;

        case DISPID_SESSION_SETINSTALLLEVEL:
            hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
            if (FAILED(hr)) return hr;
            if ((ret = MsiSetInstallLevel(This->msiHandle, V_I4(&varg0))) != ERROR_SUCCESS)
            {
                ERR("MsiSetInstallLevel returned %d\n", ret);
                return DISP_E_EXCEPTION;
            }
            break;

	case DISPID_SESSION_FEATURECURRENTSTATE:
	    if (wFlags & DISPATCH_PROPERTYGET) {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                V_VT(pVarResult) = VT_I4;
		if ((ret = MsiGetFeatureStateW(This->msiHandle, V_BSTR(&varg0), &iInstalled, &iAction)) == ERROR_SUCCESS)
		    V_I4(pVarResult) = iInstalled;
		else
		{
		    ERR("MsiGetFeatureState returned %d\n", ret);
                    V_I4(pVarResult) = msiInstallStateUnknown;
		}
	    }
	    break;

	case DISPID_SESSION_FEATUREREQUESTSTATE:
	    if (wFlags & DISPATCH_PROPERTYGET) {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                V_VT(pVarResult) = VT_I4;
		if ((ret = MsiGetFeatureStateW(This->msiHandle, V_BSTR(&varg0), &iInstalled, &iAction)) == ERROR_SUCCESS)
		    V_I4(pVarResult) = iAction;
		else
		{
		    ERR("MsiGetFeatureState returned %d\n", ret);
                    V_I4(pVarResult) = msiInstallStateUnknown;
		}
	    } else if (wFlags & DISPATCH_PROPERTYPUT) {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                hr = DispGetParam(pDispParams, DISPID_PROPERTYPUT, VT_I4, &varg1, puArgErr);
                if (FAILED(hr)) {
                    VariantClear(&varg0);
                    return hr;
                }
		if ((ret = MsiSetFeatureStateW(This->msiHandle, V_BSTR(&varg0), V_I4(&varg1))) != ERROR_SUCCESS)
                {
                    ERR("MsiSetFeatureState returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
	    }
	    break;

         default:
            return DISP_E_MEMBERNOTFOUND;
    }

    return S_OK;
}

static HRESULT WINAPI InstallerImpl_Invoke(
        AutomationObject* This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
{
    MSIHANDLE msiHandle;
    IDispatch *pDispatch = NULL;
    UINT ret;
    VARIANTARG varg0, varg1;
    HRESULT hr;

    VariantInit(&varg0);
    VariantInit(&varg1);

    switch (dispIdMember)
    {
	case DISPID_INSTALLER_OPENPACKAGE:
	    if (wFlags & DISPATCH_METHOD)
	    {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                hr = DispGetParam(pDispParams, 1, VT_I4, &varg1, puArgErr);
                if (FAILED(hr)) return hr;
                V_VT(pVarResult) = VT_DISPATCH;
		if ((ret = MsiOpenPackageExW(V_BSTR(&varg0), V_I4(&varg1), &msiHandle)) == ERROR_SUCCESS)
                {
                    if (SUCCEEDED(create_automation_object(msiHandle, NULL, (LPVOID*)&pDispatch, &DIID_Session, SessionImpl_Invoke)))
                    {
                        IDispatch_AddRef(pDispatch);
                        V_DISPATCH(pVarResult) = pDispatch;
                    }
                }
		else
                {
                    ERR("MsiOpenPackageEx returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
	    }
	    break;

         default:
            return DISP_E_MEMBERNOTFOUND;
    }

    return S_OK;
}

/* Wrapper around create_automation_object to create an installer object. */
HRESULT create_msiserver(IUnknown *pOuter, LPVOID *ppObj)
{
    return create_automation_object(0, pOuter, ppObj, &DIID_Installer, InstallerImpl_Invoke);
}
