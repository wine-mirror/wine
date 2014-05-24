/*
 * IShellDispatch implementation
 *
 * Copyright 2010 Alexander Morozov for Etersoft
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winsvc.h"
#include "shlwapi.h"
#include "shlobj.h"
#include "shldisp.h"
#include "debughlp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

enum tid_t {
    NULL_tid,
    IShellDispatch5_tid,
    Folder3_tid,
    FolderItem2_tid,
    LAST_tid
};

static ITypeLib *typelib;
static const IID * const tid_ids[] =
{
    &IID_NULL,
    &IID_IShellDispatch5,
    &IID_Folder3,
    &IID_FolderItem2
};
static ITypeInfo *typeinfos[LAST_tid];

typedef struct {
    IShellDispatch5 IShellDispatch5_iface;
    LONG ref;
} ShellDispatch;

typedef struct {
    Folder3 Folder3_iface;
    LONG ref;
    VARIANT dir;
} FolderImpl;

typedef struct {
    FolderItem2 FolderItem2_iface;
    LONG ref;
    VARIANT dir;
} FolderItemImpl;

static inline ShellDispatch *impl_from_IShellDispatch5(IShellDispatch5 *iface)
{
    return CONTAINING_RECORD(iface, ShellDispatch, IShellDispatch5_iface);
}

static inline FolderImpl *impl_from_Folder(Folder3 *iface)
{
    return CONTAINING_RECORD(iface, FolderImpl, Folder3_iface);
}

static inline FolderItemImpl *impl_from_FolderItem(FolderItem2 *iface)
{
    return CONTAINING_RECORD(iface, FolderItemImpl, FolderItem2_iface);
}

static HRESULT load_typelib(void)
{
    ITypeLib *tl;
    HRESULT hr;

    hr = LoadRegTypeLib(&LIBID_Shell32, 1, 0, LOCALE_SYSTEM_DEFAULT, &tl);
    if (FAILED(hr)) {
        ERR("LoadRegTypeLib failed: %08x\n", hr);
        return hr;
    }

    if (InterlockedCompareExchangePointer((void**)&typelib, tl, NULL))
        ITypeLib_Release(tl);
    return hr;
}

void release_typelib(void)
{
    unsigned i;

    if (!typelib)
        return;

    for (i = 0; i < sizeof(typeinfos)/sizeof(*typeinfos); i++)
        if (typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);

    ITypeLib_Release(typelib);
}

static HRESULT get_typeinfo(enum tid_t tid, ITypeInfo **typeinfo)
{
    HRESULT hr;

    if (!typelib)
        hr = load_typelib();
    if (!typelib)
        return hr;

    if (!typeinfos[tid])
    {
        ITypeInfo *ti;

        hr = ITypeLib_GetTypeInfoOfGuid(typelib, tid_ids[tid], &ti);
        if (FAILED(hr))
        {
            ERR("GetTypeInfoOfGuid(%s) failed: %08x\n", debugstr_guid(tid_ids[tid]), hr);
            return hr;
        }

        if (InterlockedCompareExchangePointer((void**)(typeinfos+tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    *typeinfo = typeinfos[tid];
    return S_OK;
}

static HRESULT WINAPI FolderItemImpl_QueryInterface(FolderItem2 *iface,
        REFIID riid, LPVOID *ppv)
{
    FolderItemImpl *This = impl_from_FolderItem(iface);

    TRACE("(%p,%p,%p)\n", iface, riid, ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IDispatch, riid) ||
        IsEqualIID(&IID_FolderItem, riid) ||
        IsEqualIID(&IID_FolderItem2, riid))
        *ppv = &This->FolderItem2_iface;
    else
    {
        FIXME("not implemented for %s\n", shdebugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI FolderItemImpl_AddRef(FolderItem2 *iface)
{
    FolderItemImpl *This = impl_from_FolderItem(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p), new refcount=%i\n", iface, ref);

    return ref;
}

static ULONG WINAPI FolderItemImpl_Release(FolderItem2 *iface)
{
    FolderItemImpl *This = impl_from_FolderItem(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p), new refcount=%i\n", iface, ref);

    if (!ref)
    {
        VariantClear(&This->dir);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI FolderItemImpl_GetTypeInfoCount(FolderItem2 *iface,
        UINT *pctinfo)
{
    TRACE("(%p,%p)\n", iface, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI FolderItemImpl_GetTypeInfo(FolderItem2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HRESULT hr;

    TRACE("(%p,%u,%d,%p)\n", iface, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(FolderItem2_tid, ppTInfo);
    if (SUCCEEDED(hr))
        ITypeInfo_AddRef(*ppTInfo);
    return hr;
}

static HRESULT WINAPI FolderItemImpl_GetIDsOfNames(FolderItem2 *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid,
        DISPID *rgDispId)
{
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p,%p,%p,%u,%d,%p)\n", iface, riid, rgszNames, cNames, lcid,
            rgDispId);

    hr = get_typeinfo(FolderItem2_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_GetIDsOfNames(ti, rgszNames, cNames, rgDispId);
    return hr;
}

static HRESULT WINAPI FolderItemImpl_Invoke(FolderItem2 *iface,
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
        DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
        UINT *puArgErr)
{
    FolderItemImpl *This = impl_from_FolderItem(iface);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p,%d,%p,%d,%u,%p,%p,%p,%p)\n", iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(FolderItem2_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_Invoke(ti, This, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return hr;
}

static HRESULT WINAPI FolderItemImpl_get_Application(FolderItem2 *iface,
        IDispatch **ppid)
{
    FIXME("(%p,%p)\n", iface, ppid);

    *ppid = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_get_Parent(FolderItem2 *iface,
        IDispatch **ppid)
{
    FIXME("(%p,%p)\n", iface, ppid);

    *ppid = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_get_Name(FolderItem2 *iface, BSTR *pbs)
{
    FIXME("(%p,%p)\n", iface, pbs);

    *pbs = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_put_Name(FolderItem2 *iface, BSTR bs)
{
    FIXME("(%p,%s)\n", iface, debugstr_w(bs));

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_get_Path(FolderItem2 *iface, BSTR *pbs)
{
    FolderItemImpl *This = impl_from_FolderItem(iface);
    HRESULT ret = S_OK;
    WCHAR *pathW;
    int len;

    TRACE("(%p,%p)\n", iface, pbs);

    *pbs = NULL;
    if (V_VT(&This->dir) == VT_I4)
    {
        pathW = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
        if (!pathW) return E_OUTOFMEMORY;
        ret = SHGetFolderPathW(NULL, V_I4(&This->dir), NULL, SHGFP_TYPE_CURRENT,
                pathW);
        if (ret == S_OK)
            *pbs = SysAllocString(pathW);
        else if (ret == E_INVALIDARG)
        {
            FIXME("not implemented for %#x\n", V_I4(&This->dir));
            ret = E_NOTIMPL;
        }
        HeapFree(GetProcessHeap(), 0, pathW);
    }
    else /* VT_BSTR */
    {
        pathW = V_BSTR(&This->dir);
        len = lstrlenW(pathW);
        *pbs = SysAllocStringLen(pathW, pathW[len - 1] == '\\' ? len - 1 : len);
    }
    if (ret == S_OK && !*pbs)
        ret = E_OUTOFMEMORY;
    return ret;
}

static HRESULT WINAPI FolderItemImpl_get_GetLink(FolderItem2 *iface,
        IDispatch **ppid)
{
    FIXME("(%p,%p)\n", iface, ppid);

    *ppid = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_get_GetFolder(FolderItem2 *iface,
        IDispatch **ppid)
{
    FIXME("(%p,%p)\n", iface, ppid);

    *ppid = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_get_IsLink(FolderItem2 *iface,
        VARIANT_BOOL *pb)
{
    FIXME("(%p,%p)\n", iface, pb);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_get_IsFolder(FolderItem2 *iface,
        VARIANT_BOOL *pb)
{
    FIXME("(%p,%p)\n", iface, pb);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_get_IsFileSystem(FolderItem2 *iface,
        VARIANT_BOOL *pb)
{
    FIXME("(%p,%p)\n", iface, pb);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_get_IsBrowsable(FolderItem2 *iface,
        VARIANT_BOOL *pb)
{
    FIXME("(%p,%p)\n", iface, pb);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_get_ModifyDate(FolderItem2 *iface,
        DATE *pdt)
{
    FIXME("(%p,%p)\n", iface, pdt);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_put_ModifyDate(FolderItem2 *iface, DATE dt)
{
    FIXME("(%p,%f)\n", iface, dt);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_get_Size(FolderItem2 *iface, LONG *pul)
{
    FIXME("(%p,%p)\n", iface, pul);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_get_Type(FolderItem2 *iface, BSTR *pbs)
{
    FIXME("(%p,%p)\n", iface, pbs);

    *pbs = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_Verbs(FolderItem2 *iface,
        FolderItemVerbs **ppfic)
{
    FIXME("(%p,%p)\n", iface, ppfic);

    *ppfic = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_InvokeVerb(FolderItem2 *iface,
        VARIANT vVerb)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_InvokeVerbEx(FolderItem2 *iface, VARIANT verb, VARIANT args)
{
    FIXME("(%p): stub\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderItemImpl_ExtendedProperty(FolderItem2 *iface, BSTR propname, VARIANT *ret)
{
    FIXME("(%p)->(%s %p): stub\n", iface, debugstr_w(propname), ret);

    return E_NOTIMPL;
}

static const FolderItem2Vtbl FolderItemImpl_Vtbl = {
    FolderItemImpl_QueryInterface,
    FolderItemImpl_AddRef,
    FolderItemImpl_Release,
    FolderItemImpl_GetTypeInfoCount,
    FolderItemImpl_GetTypeInfo,
    FolderItemImpl_GetIDsOfNames,
    FolderItemImpl_Invoke,
    FolderItemImpl_get_Application,
    FolderItemImpl_get_Parent,
    FolderItemImpl_get_Name,
    FolderItemImpl_put_Name,
    FolderItemImpl_get_Path,
    FolderItemImpl_get_GetLink,
    FolderItemImpl_get_GetFolder,
    FolderItemImpl_get_IsLink,
    FolderItemImpl_get_IsFolder,
    FolderItemImpl_get_IsFileSystem,
    FolderItemImpl_get_IsBrowsable,
    FolderItemImpl_get_ModifyDate,
    FolderItemImpl_put_ModifyDate,
    FolderItemImpl_get_Size,
    FolderItemImpl_get_Type,
    FolderItemImpl_Verbs,
    FolderItemImpl_InvokeVerb,
    FolderItemImpl_InvokeVerbEx,
    FolderItemImpl_ExtendedProperty
};

static HRESULT FolderItem_Constructor(VARIANT *dir, FolderItem **ppfi)
{
    FolderItemImpl *This;
    HRESULT ret;

    *ppfi = NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(FolderItemImpl));
    if (!This) return E_OUTOFMEMORY;
    This->FolderItem2_iface.lpVtbl = &FolderItemImpl_Vtbl;
    This->ref = 1;

    VariantInit(&This->dir);
    ret = VariantCopy(&This->dir, dir);
    if (FAILED(ret))
    {
        HeapFree(GetProcessHeap(), 0, This);
        return E_OUTOFMEMORY;
    }

    *ppfi = (FolderItem*)&This->FolderItem2_iface;
    return ret;
}

static HRESULT WINAPI FolderImpl_QueryInterface(Folder3 *iface, REFIID riid,
        LPVOID *ppv)
{
    FolderImpl *This = impl_from_Folder(iface);

    TRACE("(%p,%p,%p)\n", iface, riid, ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IDispatch, riid) ||
        IsEqualIID(&IID_Folder, riid) ||
        IsEqualIID(&IID_Folder2, riid) ||
        IsEqualIID(&IID_Folder3, riid))
        *ppv = &This->Folder3_iface;
    else
    {
        FIXME("not implemented for %s\n", shdebugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI FolderImpl_AddRef(Folder3 *iface)
{
    FolderImpl *This = impl_from_Folder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p), new refcount=%i\n", iface, ref);

    return ref;
}

static ULONG WINAPI FolderImpl_Release(Folder3 *iface)
{
    FolderImpl *This = impl_from_Folder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p), new refcount=%i\n", iface, ref);

    if (!ref)
    {
        VariantClear(&This->dir);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI FolderImpl_GetTypeInfoCount(Folder3 *iface, UINT *pctinfo)
{
    TRACE("(%p,%p)\n", iface, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI FolderImpl_GetTypeInfo(Folder3 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HRESULT hr;

    TRACE("(%p,%u,%d,%p)\n", iface, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(Folder3_tid, ppTInfo);
    if (SUCCEEDED(hr))
        ITypeInfo_AddRef(*ppTInfo);

    return hr;
}

static HRESULT WINAPI FolderImpl_GetIDsOfNames(Folder3 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p,%p,%p,%u,%d,%p)\n", iface, riid, rgszNames, cNames, lcid,
            rgDispId);

    hr = get_typeinfo(Folder3_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_GetIDsOfNames(ti, rgszNames, cNames, rgDispId);
    return hr;
}

static HRESULT WINAPI FolderImpl_Invoke(Folder3 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    FolderImpl *This = impl_from_Folder(iface);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p,%d,%p,%d,%u,%p,%p,%p,%p)\n", iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(Folder3_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_Invoke(ti, This, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return hr;
}

static HRESULT WINAPI FolderImpl_get_Title(Folder3 *iface, BSTR *pbs)
{
    FolderImpl *This = impl_from_Folder(iface);
    WCHAR *p;
    int len;

    TRACE("(%p,%p)\n", iface, pbs);

    *pbs = NULL;

    if (V_VT(&This->dir) == VT_I4)
    {
        FIXME("special folder constants are not supported\n");
        return E_NOTIMPL;
    }
    p = PathFindFileNameW(V_BSTR(&This->dir));
    len = lstrlenW(p);
    *pbs = SysAllocStringLen(p, p[len - 1] == '\\' ? len - 1 : len);
    return *pbs ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI FolderImpl_get_Application(Folder3 *iface,
        IDispatch **ppid)
{
    FIXME("(%p,%p)\n", iface, ppid);

    *ppid = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderImpl_get_Parent(Folder3 *iface, IDispatch **ppid)
{
    FIXME("(%p,%p)\n", iface, ppid);

    *ppid = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderImpl_get_ParentFolder(Folder3 *iface, Folder **ppsf)
{
    FIXME("(%p,%p)\n", iface, ppsf);

    *ppsf = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderImpl_Items(Folder3 *iface, FolderItems **ppid)
{
    FIXME("(%p,%p)\n", iface, ppid);

    *ppid = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderImpl_ParseName(Folder3 *iface, BSTR bName,
        FolderItem **ppid)
{
    FIXME("(%p,%s,%p)\n", iface, debugstr_w(bName), ppid);

    *ppid = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderImpl_NewFolder(Folder3 *iface, BSTR bName,
        VARIANT vOptions)
{
    FIXME("(%p,%s)\n", iface, debugstr_w(bName));

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderImpl_MoveHere(Folder3 *iface, VARIANT vItem,
        VARIANT vOptions)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderImpl_CopyHere(Folder3 *iface, VARIANT vItem,
        VARIANT vOptions)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderImpl_GetDetailsOf(Folder3 *iface, VARIANT vItem,
        int iColumn, BSTR *pbs)
{
    FIXME("(%p,%d,%p)\n", iface, iColumn, pbs);

    *pbs = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderImpl_get_Self(Folder3 *iface, FolderItem **ppfi)
{
    FolderImpl *This = impl_from_Folder(iface);

    TRACE("(%p,%p)\n", iface, ppfi);

    return FolderItem_Constructor(&This->dir, ppfi);
}

static HRESULT WINAPI FolderImpl_get_OfflineStatus(Folder3 *iface, LONG *pul)
{
    FIXME("(%p,%p)\n", iface, pul);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderImpl_Synchronize(Folder3 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderImpl_get_HaveToShowWebViewBarricade(Folder3 *iface,
        VARIANT_BOOL *pbHaveToShowWebViewBarricade)
{
    FIXME("(%p,%p)\n", iface, pbHaveToShowWebViewBarricade);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderImpl_DismissedWebViewBarricade(Folder3 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderImpl_get_ShowWebViewBarricade(Folder3 *iface,
        VARIANT_BOOL *pbShowWebViewBarricade)
{
    FIXME("(%p,%p)\n", iface, pbShowWebViewBarricade);

    return E_NOTIMPL;
}

static HRESULT WINAPI FolderImpl_put_ShowWebViewBarricade(Folder3 *iface,
        VARIANT_BOOL bShowWebViewBarricade)
{
    FIXME("(%p,%d)\n", iface, bShowWebViewBarricade);

    return E_NOTIMPL;
}

static const Folder3Vtbl FolderImpl_Vtbl = {
    FolderImpl_QueryInterface,
    FolderImpl_AddRef,
    FolderImpl_Release,
    FolderImpl_GetTypeInfoCount,
    FolderImpl_GetTypeInfo,
    FolderImpl_GetIDsOfNames,
    FolderImpl_Invoke,
    FolderImpl_get_Title,
    FolderImpl_get_Application,
    FolderImpl_get_Parent,
    FolderImpl_get_ParentFolder,
    FolderImpl_Items,
    FolderImpl_ParseName,
    FolderImpl_NewFolder,
    FolderImpl_MoveHere,
    FolderImpl_CopyHere,
    FolderImpl_GetDetailsOf,
    FolderImpl_get_Self,
    FolderImpl_get_OfflineStatus,
    FolderImpl_Synchronize,
    FolderImpl_get_HaveToShowWebViewBarricade,
    FolderImpl_DismissedWebViewBarricade,
    FolderImpl_get_ShowWebViewBarricade,
    FolderImpl_put_ShowWebViewBarricade
};

static HRESULT Folder_Constructor(VARIANT *dir, Folder **ppsdf)
{
    FolderImpl *This;
    HRESULT ret;

    *ppsdf = NULL;

    switch (V_VT(dir))
    {
        case VT_I4:
            /* FIXME: add some checks */
            break;
        case VT_BSTR:
            if (PathIsDirectoryW(V_BSTR(dir)) &&
                !PathIsRelativeW(V_BSTR(dir)) &&
                PathFileExistsW(V_BSTR(dir)))
                break;
        default:
            return S_FALSE;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(FolderImpl));
    if (!This) return E_OUTOFMEMORY;
    This->Folder3_iface.lpVtbl = &FolderImpl_Vtbl;
    This->ref = 1;

    VariantInit(&This->dir);
    ret = VariantCopy(&This->dir, dir);
    if (FAILED(ret))
    {
        HeapFree(GetProcessHeap(), 0, This);
        return E_OUTOFMEMORY;
    }

    *ppsdf = (Folder*)This;
    return ret;
}

static HRESULT WINAPI ShellDispatch_QueryInterface(IShellDispatch5 *iface,
        REFIID riid, LPVOID *ppv)
{
    ShellDispatch *This = impl_from_IShellDispatch5(iface);

    TRACE("(%p,%p,%p)\n", iface, riid, ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IDispatch, riid) ||
        IsEqualIID(&IID_IShellDispatch, riid) ||
        IsEqualIID(&IID_IShellDispatch2, riid) ||
        IsEqualIID(&IID_IShellDispatch3, riid) ||
        IsEqualIID(&IID_IShellDispatch4, riid) ||
        IsEqualIID(&IID_IShellDispatch5, riid))
        *ppv = &This->IShellDispatch5_iface;
    else
    {
        FIXME("not implemented for %s\n", shdebugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IShellDispatch5_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI ShellDispatch_AddRef(IShellDispatch5 *iface)
{
    ShellDispatch *This = impl_from_IShellDispatch5(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p), new refcount=%i\n", iface, ref);

    return ref;
}

static ULONG WINAPI ShellDispatch_Release(IShellDispatch5 *iface)
{
    ShellDispatch *This = impl_from_IShellDispatch5(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p), new refcount=%i\n", iface, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI ShellDispatch_GetTypeInfoCount(IShellDispatch5 *iface,
        UINT *pctinfo)
{
    TRACE("(%p,%p)\n", iface, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI ShellDispatch_GetTypeInfo(IShellDispatch5 *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    HRESULT hr;

    TRACE("(%p,%u,%d,%p)\n", iface, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IShellDispatch5_tid, ppTInfo);
    if (SUCCEEDED(hr))
        ITypeInfo_AddRef(*ppTInfo);
    return hr;
}

static HRESULT WINAPI ShellDispatch_GetIDsOfNames(IShellDispatch5 *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p,%p,%p,%u,%d,%p)\n", iface, riid, rgszNames, cNames, lcid,
            rgDispId);

    hr = get_typeinfo(IShellDispatch5_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_GetIDsOfNames(ti, rgszNames, cNames, rgDispId);
    return hr;
}

static HRESULT WINAPI ShellDispatch_Invoke(IShellDispatch5 *iface,
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
        DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
        UINT *puArgErr)
{
    ShellDispatch *This = impl_from_IShellDispatch5(iface);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p,%d,%p,%d,%u,%p,%p,%p,%p)\n", iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IShellDispatch5_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_Invoke(ti, This, dispIdMember, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
    return hr;
}

static HRESULT WINAPI ShellDispatch_get_Application(IShellDispatch5 *iface,
        IDispatch **ppid)
{
    FIXME("(%p,%p)\n", iface, ppid);

    *ppid = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_get_Parent(IShellDispatch5 *iface,
        IDispatch **ppid)
{
    FIXME("(%p,%p)\n", iface, ppid);

    *ppid = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_NameSpace(IShellDispatch5 *iface,
        VARIANT vDir, Folder **ppsdf)
{
    TRACE("(%p,%p)\n", iface, ppsdf);

    return Folder_Constructor(&vDir, ppsdf);
}

static HRESULT WINAPI ShellDispatch_BrowseForFolder(IShellDispatch5 *iface,
        LONG Hwnd, BSTR Title, LONG Options, VARIANT RootFolder, Folder **ppsdf)
{
    FIXME("(%p,%x,%s,%x,%p)\n", iface, Hwnd, debugstr_w(Title), Options, ppsdf);

    *ppsdf = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_Windows(IShellDispatch5 *iface,
        IDispatch **ppid)
{
    FIXME("(%p,%p)\n", iface, ppid);

    *ppid = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_Open(IShellDispatch5 *iface, VARIANT vDir)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_Explore(IShellDispatch5 *iface, VARIANT vDir)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_MinimizeAll(IShellDispatch5 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_UndoMinimizeALL(IShellDispatch5 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_FileRun(IShellDispatch5 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_CascadeWindows(IShellDispatch5 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_TileVertically(IShellDispatch5 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_TileHorizontally(IShellDispatch5 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_ShutdownWindows(IShellDispatch5 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_Suspend(IShellDispatch5 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_EjectPC(IShellDispatch5 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_SetTime(IShellDispatch5 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_TrayProperties(IShellDispatch5 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_Help(IShellDispatch5 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_FindFiles(IShellDispatch5 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_FindComputer(IShellDispatch5 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_RefreshMenu(IShellDispatch5 *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_ControlPanelItem(IShellDispatch5 *iface,
        BSTR szDir)
{
    FIXME("(%p,%s)\n", iface, debugstr_w(szDir));

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_IsRestricted(IShellDispatch5 *iface, BSTR group, BSTR restriction, LONG *value)
{
    FIXME("(%s, %s, %p): stub\n", debugstr_w(group), debugstr_w(restriction), value);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_ShellExecute(IShellDispatch5 *iface, BSTR file, VARIANT args, VARIANT dir,
        VARIANT op, VARIANT show)
{
    FIXME("(%s): stub\n", debugstr_w(file));
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_FindPrinter(IShellDispatch5 *iface, BSTR name, BSTR location, BSTR model)
{
    FIXME("(%s, %s, %s): stub\n", debugstr_w(name), debugstr_w(location), debugstr_w(model));
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_GetSystemInformation(IShellDispatch5 *iface, BSTR name, VARIANT *ret)
{
    FIXME("(%s, %p): stub\n", debugstr_w(name), ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_ServiceStart(IShellDispatch5 *iface, BSTR service, VARIANT persistent, VARIANT *ret)
{
    FIXME("(%s, %p): stub\n", debugstr_w(service), ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_ServiceStop(IShellDispatch5 *iface, BSTR service, VARIANT persistent, VARIANT *ret)
{
    FIXME("(%s, %p): stub\n", debugstr_w(service), ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_IsServiceRunning(IShellDispatch5 *iface, BSTR name, VARIANT *running)
{
    SERVICE_STATUS_PROCESS status;
    SC_HANDLE scm, service;
    DWORD dummy;

    TRACE("(%s, %p)\n", debugstr_w(name), running);

    V_VT(running) = VT_BOOL;
    V_BOOL(running) = VARIANT_FALSE;

    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm)
    {
        ERR("failed to connect to service manager\n");
        return S_OK;
    }

    service = OpenServiceW(scm, name, SERVICE_QUERY_STATUS);
    if (!service)
    {
        ERR("Failed to open service %s (%u)\n", debugstr_w(name), GetLastError());
        CloseServiceHandle(scm);
        return S_OK;
    }

    if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (BYTE *)&status,
             sizeof(SERVICE_STATUS_PROCESS), &dummy))
    {
        TRACE("failed to query service status (%u)\n", GetLastError());
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return S_OK;
    }

    if (status.dwCurrentState == SERVICE_RUNNING)
       V_BOOL(running) = VARIANT_TRUE;

    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    return S_OK;
}

static HRESULT WINAPI ShellDispatch_CanStartStopService(IShellDispatch5 *iface, BSTR service, VARIANT *ret)
{
    FIXME("(%s, %p): stub\n", debugstr_w(service), ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_ShowBrowserBar(IShellDispatch5 *iface, BSTR clsid, VARIANT show, VARIANT *ret)
{
    FIXME("(%s, %p): stub\n", debugstr_w(clsid), ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_AddToRecent(IShellDispatch5 *iface, VARIANT file, BSTR category)
{
    FIXME("(%s): stub\n", debugstr_w(category));
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_WindowsSecurity(IShellDispatch5 *iface)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_ToggleDesktop(IShellDispatch5 *iface)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_ExplorerPolicy(IShellDispatch5 *iface, BSTR policy, VARIANT *value)
{
    FIXME("(%s, %p): stub\n", debugstr_w(policy), value);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_GetSetting(IShellDispatch5 *iface, LONG setting, VARIANT_BOOL *result)
{
    FIXME("(%d %p): stub\n", setting, result);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_WindowSwitcher(IShellDispatch5 *iface)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static const IShellDispatch5Vtbl ShellDispatch5Vtbl = {
    ShellDispatch_QueryInterface,
    ShellDispatch_AddRef,
    ShellDispatch_Release,
    ShellDispatch_GetTypeInfoCount,
    ShellDispatch_GetTypeInfo,
    ShellDispatch_GetIDsOfNames,
    ShellDispatch_Invoke,
    ShellDispatch_get_Application,
    ShellDispatch_get_Parent,
    ShellDispatch_NameSpace,
    ShellDispatch_BrowseForFolder,
    ShellDispatch_Windows,
    ShellDispatch_Open,
    ShellDispatch_Explore,
    ShellDispatch_MinimizeAll,
    ShellDispatch_UndoMinimizeALL,
    ShellDispatch_FileRun,
    ShellDispatch_CascadeWindows,
    ShellDispatch_TileVertically,
    ShellDispatch_TileHorizontally,
    ShellDispatch_ShutdownWindows,
    ShellDispatch_Suspend,
    ShellDispatch_EjectPC,
    ShellDispatch_SetTime,
    ShellDispatch_TrayProperties,
    ShellDispatch_Help,
    ShellDispatch_FindFiles,
    ShellDispatch_FindComputer,
    ShellDispatch_RefreshMenu,
    ShellDispatch_ControlPanelItem,
    ShellDispatch_IsRestricted,
    ShellDispatch_ShellExecute,
    ShellDispatch_FindPrinter,
    ShellDispatch_GetSystemInformation,
    ShellDispatch_ServiceStart,
    ShellDispatch_ServiceStop,
    ShellDispatch_IsServiceRunning,
    ShellDispatch_CanStartStopService,
    ShellDispatch_ShowBrowserBar,
    ShellDispatch_AddToRecent,
    ShellDispatch_WindowsSecurity,
    ShellDispatch_ToggleDesktop,
    ShellDispatch_ExplorerPolicy,
    ShellDispatch_GetSetting,
    ShellDispatch_WindowSwitcher
};

HRESULT WINAPI IShellDispatch_Constructor(IUnknown *outer, REFIID riid, void **ppv)
{
    ShellDispatch *This;
    HRESULT ret;

    TRACE("(%p, %s)\n", outer, debugstr_guid(riid));

    *ppv = NULL;

    if (outer) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(ShellDispatch));
    if (!This) return E_OUTOFMEMORY;
    This->IShellDispatch5_iface.lpVtbl = &ShellDispatch5Vtbl;
    This->ref = 1;

    ret = IShellDispatch5_QueryInterface(&This->IShellDispatch5_iface, riid, ppv);
    IShellDispatch5_Release(&This->IShellDispatch5_iface);
    return ret;
}
