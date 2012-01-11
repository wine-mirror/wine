/*
 * Copyright 2012 Vincent Povirk for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wincodec.h"
#include "wincodecsdk.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct MetadataHandler {
    IWICMetadataWriter IWICMetadataWriter_iface;
    LONG ref;
    IWICPersistStream IWICPersistStream_iface;
    const MetadataHandlerVtbl *vtable;
    MetadataItem *items;
    DWORD item_count;
    CRITICAL_SECTION lock;
} MetadataHandler;

static inline MetadataHandler *impl_from_IWICMetadataWriter(IWICMetadataWriter *iface)
{
    return CONTAINING_RECORD(iface, MetadataHandler, IWICMetadataWriter_iface);
}

static inline MetadataHandler *impl_from_IWICPersistStream(IWICPersistStream *iface)
{
    return CONTAINING_RECORD(iface, MetadataHandler, IWICPersistStream_iface);
}

static void MetadataHandler_FreeItems(MetadataHandler *This)
{
    int i;

    for (i=0; i<This->item_count; i++)
    {
        PropVariantClear(&This->items[i].schema);
        PropVariantClear(&This->items[i].id);
        PropVariantClear(&This->items[i].value);
    }

    HeapFree(GetProcessHeap(), 0, This->items);
}

static HRESULT WINAPI MetadataHandler_QueryInterface(IWICMetadataWriter *iface, REFIID iid,
    void **ppv)
{
    MetadataHandler *This = impl_from_IWICMetadataWriter(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICMetadataReader, iid) ||
        (IsEqualIID(&IID_IWICMetadataWriter, iid) && This->vtable->is_writer))
    {
        *ppv = &This->IWICMetadataWriter_iface;
    }
    else if (IsEqualIID(&IID_IPersist, iid) ||
             IsEqualIID(&IID_IPersistStream, iid) ||
             IsEqualIID(&IID_IWICPersistStream, iid))
    {
        *ppv = &This->IWICPersistStream_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MetadataHandler_AddRef(IWICMetadataWriter *iface)
{
    MetadataHandler *This = impl_from_IWICMetadataWriter(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI MetadataHandler_Release(IWICMetadataWriter *iface)
{
    MetadataHandler *This = impl_from_IWICMetadataWriter(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        MetadataHandler_FreeItems(This);
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI MetadataHandler_GetMetadataFormat(IWICMetadataWriter *iface,
    GUID *pguidMetadataFormat)
{
    FIXME("(%p,%s): stub\n", iface, debugstr_guid(pguidMetadataFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_GetMetadataHandlerInfo(IWICMetadataWriter *iface,
    IWICMetadataHandlerInfo **ppIHandler)
{
    FIXME("(%p,%p): stub\n", iface, ppIHandler);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_GetCount(IWICMetadataWriter *iface,
    UINT *pcCount)
{
    FIXME("(%p,%p): stub\n", iface, pcCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_GetValueByIndex(IWICMetadataWriter *iface,
    UINT nIndex, PROPVARIANT *pvarSchema, PROPVARIANT *pvarId, PROPVARIANT *pvarValue)
{
    FIXME("(%p,%u,%p,%p,%p): stub\n", iface, nIndex, pvarSchema, pvarId, pvarValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_GetValue(IWICMetadataWriter *iface,
    const PROPVARIANT *pvarSchema, const PROPVARIANT *pvarId, PROPVARIANT *pvarValue)
{
    FIXME("(%p,%p,%p,%p): stub\n", iface, pvarSchema, pvarId, pvarValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_GetEnumerator(IWICMetadataWriter *iface,
    IWICEnumMetadataItem **ppIEnumMetadata)
{
    FIXME("(%p,%p): stub\n", iface, ppIEnumMetadata);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_SetValue(IWICMetadataWriter *iface,
    const PROPVARIANT *pvarSchema, const PROPVARIANT *pvarId, const PROPVARIANT *pvarValue)
{
    FIXME("(%p,%p,%p,%p): stub\n", iface, pvarSchema, pvarId, pvarValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_SetValueByIndex(IWICMetadataWriter *iface,
    UINT nIndex, const PROPVARIANT *pvarSchema, const PROPVARIANT *pvarId, const PROPVARIANT *pvarValue)
{
    FIXME("(%p,%u,%p,%p,%p): stub\n", iface, nIndex, pvarSchema, pvarId, pvarValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_RemoveValue(IWICMetadataWriter *iface,
    const PROPVARIANT *pvarSchema, const PROPVARIANT *pvarId)
{
    FIXME("(%p,%p,%p): stub\n", iface, pvarSchema, pvarId);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_RemoveValueByIndex(IWICMetadataWriter *iface,
    UINT nIndex)
{
    FIXME("(%p,%u): stub\n", iface, nIndex);
    return E_NOTIMPL;
}

static const IWICMetadataWriterVtbl MetadataHandler_Vtbl = {
    MetadataHandler_QueryInterface,
    MetadataHandler_AddRef,
    MetadataHandler_Release,
    MetadataHandler_GetMetadataFormat,
    MetadataHandler_GetMetadataHandlerInfo,
    MetadataHandler_GetCount,
    MetadataHandler_GetValueByIndex,
    MetadataHandler_GetValue,
    MetadataHandler_GetEnumerator,
    MetadataHandler_SetValue,
    MetadataHandler_SetValueByIndex,
    MetadataHandler_RemoveValue,
    MetadataHandler_RemoveValueByIndex
};

static HRESULT WINAPI MetadataHandler_PersistStream_QueryInterface(IWICPersistStream *iface,
    REFIID iid, void **ppv)
{
    MetadataHandler *This = impl_from_IWICPersistStream(iface);
    return IWICMetadataWriter_QueryInterface(&This->IWICMetadataWriter_iface, iid, ppv);
}

static ULONG WINAPI MetadataHandler_PersistStream_AddRef(IWICPersistStream *iface)
{
    MetadataHandler *This = impl_from_IWICPersistStream(iface);
    return IWICMetadataWriter_AddRef(&This->IWICMetadataWriter_iface);
}

static ULONG WINAPI MetadataHandler_PersistStream_Release(IWICPersistStream *iface)
{
    MetadataHandler *This = impl_from_IWICPersistStream(iface);
    return IWICMetadataWriter_Release(&This->IWICMetadataWriter_iface);
}

static HRESULT WINAPI MetadataHandler_GetClassID(IWICPersistStream *iface,
    CLSID *pClassID)
{
    FIXME("(%p,%p): stub\n", iface, pClassID);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_IsDirty(IWICPersistStream *iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_Load(IWICPersistStream *iface,
    IStream *pStm)
{
    FIXME("(%p,%p): stub\n", iface, pStm);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_Save(IWICPersistStream *iface,
    IStream *pStm, BOOL fClearDirty)
{
    FIXME("(%p,%p,%i): stub\n", iface, pStm, fClearDirty);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_GetSizeMax(IWICPersistStream *iface,
    ULARGE_INTEGER *pcbSize)
{
    FIXME("(%p,%p): stub\n", iface, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_LoadEx(IWICPersistStream *iface,
    IStream *pIStream, const GUID *pguidPreferredVendor, DWORD dwPersistOptions)
{
    MetadataHandler *This = impl_from_IWICPersistStream(iface);
    HRESULT hr;
    MetadataItem *new_items=NULL;
    DWORD item_count=0;

    TRACE("(%p,%p,%s,%x)\n", iface, pIStream, debugstr_guid(pguidPreferredVendor), dwPersistOptions);

    EnterCriticalSection(&This->lock);

    hr = This->vtable->fnLoad(pIStream, pguidPreferredVendor, dwPersistOptions,
        &new_items, &item_count);

    if (SUCCEEDED(hr))
    {
        MetadataHandler_FreeItems(This);
        This->items = new_items;
        This->item_count = item_count;
    }

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI MetadataHandler_SaveEx(IWICPersistStream *iface,
    IStream *pIStream, DWORD dwPersistOptions, BOOL fClearDirty)
{
    FIXME("(%p,%p,%x,%i): stub\n", iface, pIStream, dwPersistOptions, fClearDirty);
    return E_NOTIMPL;
}

static const IWICPersistStreamVtbl MetadataHandler_PersistStream_Vtbl = {
    MetadataHandler_PersistStream_QueryInterface,
    MetadataHandler_PersistStream_AddRef,
    MetadataHandler_PersistStream_Release,
    MetadataHandler_GetClassID,
    MetadataHandler_IsDirty,
    MetadataHandler_Load,
    MetadataHandler_Save,
    MetadataHandler_GetSizeMax,
    MetadataHandler_LoadEx,
    MetadataHandler_SaveEx
};

HRESULT MetadataReader_Create(const MetadataHandlerVtbl *vtable, IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    MetadataHandler *This;
    HRESULT hr;

    TRACE("%s\n", debugstr_guid(vtable->clsid));

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(MetadataHandler));
    if (!This) return E_OUTOFMEMORY;

    This->IWICMetadataWriter_iface.lpVtbl = &MetadataHandler_Vtbl;
    This->IWICPersistStream_iface.lpVtbl = &MetadataHandler_PersistStream_Vtbl;
    This->ref = 1;
    This->vtable = vtable;
    This->items = NULL;
    This->item_count = 0;

    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": MetadataHandler.lock");

    hr = IWICMetadataWriter_QueryInterface(&This->IWICMetadataWriter_iface, iid, ppv);

    IWICMetadataWriter_Release(&This->IWICMetadataWriter_iface);

    return hr;
}

static HRESULT LoadUnknownMetadata(IStream *input, const GUID *preferred_vendor,
    DWORD persist_options, MetadataItem **items, DWORD *item_count)
{
    HRESULT hr;
    MetadataItem *result;
    STATSTG stat;
    BYTE *data;
    ULONG bytesread;

    TRACE("\n");

    hr = IStream_Stat(input, &stat, STATFLAG_NONAME);
    if (FAILED(hr))
        return hr;

    data = HeapAlloc(GetProcessHeap(), 0, stat.cbSize.QuadPart);
    if (!data) return E_OUTOFMEMORY;

    hr = IStream_Read(input, data, stat.cbSize.QuadPart, &bytesread);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, data);
        return hr;
    }

    result = HeapAlloc(GetProcessHeap(), 0, sizeof(MetadataItem));
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, data);
        return E_OUTOFMEMORY;
    }

    PropVariantInit(&result[0].schema);
    PropVariantInit(&result[0].id);
    PropVariantInit(&result[0].value);

    result[0].value.vt = VT_BLOB;
    result[0].value.blob.cbSize = bytesread;
    result[0].value.blob.pBlobData = data;

    *items = result;
    *item_count = 1;

    return S_OK;
}

static const MetadataHandlerVtbl UnknownMetadataReader_Vtbl = {
    0,
    &CLSID_WICUnknownMetadataReader,
    LoadUnknownMetadata
};

HRESULT UnknownMetadataReader_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    return MetadataReader_Create(&UnknownMetadataReader_Vtbl, pUnkOuter, iid, ppv);
}
