/*
 * Copyright 2005 Jacek Caban
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

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "oaidl.h"
#include "oleauto.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

typedef struct {
    IRecordInfoVtbl *lpVtbl;
    ULONG ref;

    GUID guid;
    TYPEATTR typeattr;
    UINT lib_index;
    BSTR name;
    BSTR *field_names;
    ITypeInfo *pTypeInfo;
} IRecordInfoImpl;

static HRESULT WINAPI IRecordInfoImpl_QueryInterface(IRecordInfo *iface, REFIID riid, void **ppvObject)
{
    TRACE("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppvObject);

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IRecordInfo, riid)) {
       *ppvObject = iface;
       IRecordInfo_AddRef(iface);
       return S_OK;
    }

    FIXME("Not supported interface: %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI IRecordInfoImpl_AddRef(IRecordInfo *iface)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) -> %ld\n", This, ref);
    return ref;
}

static ULONG WINAPI IRecordInfoImpl_Release(IRecordInfo *iface)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) -> %ld\n", This, ref);
    if(!ref) {
        int i;
        for(i=0; i<This->typeattr.cVars; i++)
            HeapFree(GetProcessHeap(), 0, This->field_names[i]);
        HeapFree(GetProcessHeap(), 0, This->name);
        ITypeInfo_Release(This->pTypeInfo);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI IRecordInfoImpl_RecordInit(IRecordInfo *iface, PVOID pvNew)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    FIXME("(%p)->(%p) stub\n", This, pvNew);
    return E_NOTIMPL;
}

static HRESULT WINAPI IRecordInfoImpl_RecordClear(IRecordInfo *iface, PVOID pvExisting)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    FIXME("(%p)->(%p) stub\n", This, pvExisting);
    return E_NOTIMPL;
}

static HRESULT WINAPI IRecordInfoImpl_RecordCopy(IRecordInfo *iface, PVOID pvExisting, PVOID pvNew)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    FIXME("(%p)->(%p %p) stub\n", This, pvExisting, pvNew);
    return E_NOTIMPL;
}

static HRESULT WINAPI IRecordInfoImpl_GetGuid(IRecordInfo *iface, GUID *pguid)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    TRACE("(%p)->(%p)\n", This, pguid);

    if(!pguid)
        return E_INVALIDARG;

    memcpy(pguid, &This->guid, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI IRecordInfoImpl_GetName(IRecordInfo *iface, BSTR *pbstrName)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;

    TRACE("(%p)->(%p)\n", This, pbstrName);
    if(!pbstrName)
        return E_INVALIDARG;

    *pbstrName = SysAllocString(This->name);
    return S_OK;
}

static HRESULT WINAPI IRecordInfoImpl_GetSize(IRecordInfo *iface, ULONG *pcbSize)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    
    TRACE("(%p)->(%p)\n", This, pcbSize);

    if(!pcbSize)
        return E_INVALIDARG;
    
    *pcbSize = This->typeattr.cbSizeInstance;
    return S_OK;
}

static HRESULT WINAPI IRecordInfoImpl_GetTypeInfo(IRecordInfo *iface, ITypeInfo **ppTypeInfo)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;

    TRACE("(%p)->(%p)\n", This, ppTypeInfo);

    if(!ppTypeInfo)
        return E_INVALIDARG;

    ITypeInfo_AddRef(This->pTypeInfo);
    *ppTypeInfo = This->pTypeInfo;

    return S_OK;
}

static HRESULT WINAPI IRecordInfoImpl_GetField(IRecordInfo *iface, PVOID pvData,
                                                LPCOLESTR szFieldName, VARIANT *pvarField)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    FIXME("(%p)->(%p %s %p) stub\n", This, pvData, debugstr_w(szFieldName), pvarField);
    return E_NOTIMPL;
}

static HRESULT WINAPI IRecordInfoImpl_GetFieldNoCopy(IRecordInfo *iface, PVOID pvData, LPCOLESTR szFieldName,
                                                    VARIANT *pvarField, PVOID *ppvDataCArray)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    FIXME("(%p)->(%p %s %p %p) stub\n", This, pvData, debugstr_w(szFieldName), pvarField, ppvDataCArray);
    return E_NOTIMPL;
}

static HRESULT WINAPI IRecordInfoImpl_PutField(IRecordInfo *iface, ULONG wFlags, PVOID pvData, LPCOLESTR szFieldName,
                                               VARIANT *pvarField)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    FIXME("(%p)->(%08lx %p %s %p) stub\n", This, wFlags, pvData, debugstr_w(szFieldName), pvarField);
    return E_NOTIMPL;
}

static HRESULT WINAPI IRecordInfoImpl_PutFieldNoCopy(IRecordInfo *iface, ULONG wFlags, PVOID pvData, LPCOLESTR szFieldName,
                                                     VARIANT *pvarField)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    FIXME("(%p)->(%08lx %p %s %p) stub\n", This, wFlags, pvData, debugstr_w(szFieldName), pvarField);
    return E_NOTIMPL;
}

static HRESULT WINAPI IRecordInfoImpl_GetFieldNames(IRecordInfo *iface, ULONG *pcNames, BSTR *rgBstrNames)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    ULONG n = This->typeattr.cVars;
    int i;
    TRACE("(%p)->(%p %p)\n", This, pcNames, rgBstrNames);

    if(!pcNames)
        return E_INVALIDARG;

    if(*pcNames < n)
        n =  *pcNames;

    if(rgBstrNames) {
        for(i=0; i<n; i++)
            rgBstrNames[i] = SysAllocString(This->field_names[i]);
    }
    
    *pcNames = n;
    return S_OK;
}

static BOOL WINAPI IRecordInfoImpl_IsMatchingType(IRecordInfo *iface, IRecordInfo *pRecordInfo)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    FIXME("(%p)->(%p) stub\n", This, pRecordInfo);
    return FALSE;
}

static PVOID WINAPI IRecordInfoImpl_RecordCreate(IRecordInfo *iface)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    FIXME("(%p) stub\n", This);
    return NULL;
}

static HRESULT WINAPI IRecordInfoImpl_RecordCreateCopy(IRecordInfo *iface, PVOID pvSource, PVOID *ppvDest)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    FIXME("(%p)->(%p %p) stub\n", This, pvSource, ppvDest);
    return E_NOTIMPL;
}

static HRESULT WINAPI IRecordInfoImpl_RecordDestroy(IRecordInfo *iface, PVOID pvRecord)
{
    IRecordInfoImpl *This = (IRecordInfoImpl*)iface;
    FIXME("(%p)->(%p) stub\n", This, pvRecord);
    return E_NOTIMPL;
}

static IRecordInfoVtbl IRecordInfoImplVtbl = {
    IRecordInfoImpl_QueryInterface,
    IRecordInfoImpl_AddRef,
    IRecordInfoImpl_Release,
    IRecordInfoImpl_RecordInit,
    IRecordInfoImpl_RecordClear,
    IRecordInfoImpl_RecordCopy,
    IRecordInfoImpl_GetGuid,
    IRecordInfoImpl_GetName,
    IRecordInfoImpl_GetSize,
    IRecordInfoImpl_GetTypeInfo,
    IRecordInfoImpl_GetField,
    IRecordInfoImpl_GetFieldNoCopy,
    IRecordInfoImpl_PutField,
    IRecordInfoImpl_PutFieldNoCopy,
    IRecordInfoImpl_GetFieldNames,
    IRecordInfoImpl_IsMatchingType,
    IRecordInfoImpl_RecordCreate,
    IRecordInfoImpl_RecordCreateCopy,
    IRecordInfoImpl_RecordDestroy
};

/******************************************************************************
 *      GetRecordInfoFromGuids  [OLEAUT32.322]
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: E_INVALIDARG, if any argument is invalid.
 */
HRESULT WINAPI GetRecordInfoFromGuids(REFGUID rGuidTypeLib, ULONG uVerMajor,
                        ULONG uVerMinor, LCID lcid, REFGUID rGuidTypeInfo, IRecordInfo** ppRecInfo)
{
    ITypeInfo *pTypeInfo;
    ITypeLib *pTypeLib;
    HRESULT hres;
    
    TRACE("(%p,%ld,%ld,%ld,%p,%p)\n", rGuidTypeLib, uVerMajor, uVerMinor,
            lcid, rGuidTypeInfo, ppRecInfo);

    hres = LoadRegTypeLib(rGuidTypeLib, uVerMajor, uVerMinor, lcid, &pTypeLib);
    if(FAILED(hres)) {
        WARN("LoadRegTypeLib failed!\n");
        return hres;
    }

    hres = ITypeLib_GetTypeInfoOfGuid(pTypeLib, rGuidTypeInfo, &pTypeInfo);
    ITypeLib_Release(pTypeLib);
    if(FAILED(hres)) {
        WARN("GetTypeInfoOfGuid failed!\n");
        return hres;
    }

    hres = GetRecordInfoFromTypeInfo(pTypeInfo, ppRecInfo);
    ITypeInfo_Release(pTypeInfo);
    return hres;
}

/******************************************************************************
 *      GetRecordInfoFromTypeInfo [OLEAUT32.332]
 */
HRESULT WINAPI GetRecordInfoFromTypeInfo(ITypeInfo* pTI, IRecordInfo** ppRecInfo) {
    HRESULT hres;
    TYPEATTR *typeattr;
    IRecordInfoImpl *ret;
    ITypeInfo *pTypeInfo;
    int i;
    GUID guid;

    TRACE("(%p %p)\n", pTI, ppRecInfo);

    if(!pTI || !ppRecInfo)
        return E_INVALIDARG;
    
    hres = ITypeInfo_GetTypeAttr(pTI, &typeattr);
    if(FAILED(hres) || !typeattr) {
        WARN("GetTypeAttr failed: %08lx\n", hres);
        return hres;
    }

    if(typeattr->typekind == TKIND_ALIAS) {
        hres = ITypeInfo_GetRefTypeInfo(pTI, typeattr->tdescAlias.u.hreftype, &pTypeInfo);
        memcpy(&guid, &typeattr->guid, sizeof(GUID));
        ITypeInfo_ReleaseTypeAttr(pTI, typeattr);
        if(FAILED(hres)) {
            WARN("GetRefTypeInfo failed: %08lx\n", hres);
            return hres;
        }
        ITypeInfo_GetTypeAttr(pTypeInfo, &typeattr);
    }else  {
        pTypeInfo = pTI;
        ITypeInfo_AddRef(pTypeInfo);
        memcpy(&guid, &typeattr->guid, sizeof(GUID));
    }

    if(typeattr->typekind != TKIND_RECORD) {
        WARN("typekind != TKIND_RECORD\n");
        ITypeInfo_ReleaseTypeAttr(pTypeInfo, typeattr);
        ITypeInfo_Release(pTypeInfo);
        return E_INVALIDARG;
    }

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(*ret));
    ret->lpVtbl = &IRecordInfoImplVtbl;
    ret->ref = 1;
    ret->pTypeInfo = pTypeInfo;
    memcpy(&ret->guid, &guid, sizeof(GUID));
    memcpy(&ret->typeattr, typeattr, sizeof(TYPEATTR));
    ITypeInfo_ReleaseTypeAttr(pTypeInfo, typeattr);

    /* NOTE: Windows implementation calls ITypeInfo::GetCantainingTypeLib and
     *       ITypeLib::GetLibAttr, but we currently don't need this.
     */

    hres = ITypeInfo_GetDocumentation(pTypeInfo, MEMBERID_NIL, &ret->name, NULL, NULL, NULL);
    if(FAILED(hres)) {
        WARN("ITypeInfo::GetDocumentation failed\n");
        ret->name = NULL;
    }

    ret->field_names = HeapAlloc(GetProcessHeap(), 0, ret->typeattr.cVars*sizeof(BSTR));
    for(i = 0; i<ret->typeattr.cVars; i++) {
        VARDESC *vardesc;
        hres = ITypeInfo_GetVarDesc(pTypeInfo, i, &vardesc);
        if(FAILED(hres)) {
            WARN("GetVarDesc failed\n");
            continue;
        }
        hres = ITypeInfo_GetDocumentation(pTypeInfo, vardesc->memid, ret->field_names+i, NULL, NULL, NULL);
        if(FAILED(hres))
            WARN("GetDocumentation failed: %08lx\n", hres);
        ITypeInfo_ReleaseVarDesc(pTypeInfo, vardesc);
    }
        
    *ppRecInfo = (IRecordInfo*)ret;

    return S_OK;
}
