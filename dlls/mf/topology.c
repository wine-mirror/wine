/*
 * Copyright 2017 Nikolay Sivov
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
#include "initguid.h"
#include "mfapi.h"
#include "mfidl.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct topology
{
    IMFTopology IMFTopology_iface;
    LONG refcount;
    IMFAttributes *attributes;
};

static inline struct topology *impl_from_IMFTopology(IMFTopology *iface)
{
    return CONTAINING_RECORD(iface, struct topology, IMFTopology_iface);
}

static HRESULT WINAPI topology_QueryInterface(IMFTopology *iface, REFIID riid, void **out)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s %p)\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFTopology) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &topology->IMFTopology_iface;
    }
    else
    {
        FIXME("(%s, %p)\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI topology_AddRef(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    ULONG refcount = InterlockedIncrement(&topology->refcount);

    TRACE("(%p) refcount=%u\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI topology_Release(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    ULONG refcount = InterlockedDecrement(&topology->refcount);

    TRACE("(%p) refcount=%u\n", iface, refcount);

    if (!refcount)
    {
        IMFAttributes_Release(topology->attributes);
        heap_free(topology);
    }

    return refcount;
}

static HRESULT WINAPI topology_GetItem(IMFTopology *iface, REFGUID key, PROPVARIANT *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetItem(topology->attributes, key, value);
}

static HRESULT WINAPI topology_GetItemType(IMFTopology *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), type);

    return IMFAttributes_GetItemType(topology->attributes, key, type);
}

static HRESULT WINAPI topology_CompareItem(IMFTopology *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p, %p)\n", iface, debugstr_guid(key), value, result);

    return IMFAttributes_CompareItem(topology->attributes, key, value, result);
}

static HRESULT WINAPI topology_Compare(IMFTopology *iface, IMFAttributes *theirs, MF_ATTRIBUTES_MATCH_TYPE type,
        BOOL *result)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%p, %d, %p)\n", iface, theirs, type, result);

    return IMFAttributes_Compare(topology->attributes, theirs, type, result);
}

static HRESULT WINAPI topology_GetUINT32(IMFTopology *iface, REFGUID key, UINT32 *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT32(topology->attributes, key, value);
}

static HRESULT WINAPI topology_GetUINT64(IMFTopology *iface, REFGUID key, UINT64 *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT64(topology->attributes, key, value);
}

static HRESULT WINAPI topology_GetDouble(IMFTopology *iface, REFGUID key, double *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetDouble(topology->attributes, key, value);
}

static HRESULT WINAPI topology_GetGUID(IMFTopology *iface, REFGUID key, GUID *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetGUID(topology->attributes, key, value);
}

static HRESULT WINAPI topology_GetStringLength(IMFTopology *iface, REFGUID key, UINT32 *length)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), length);

    return IMFAttributes_GetStringLength(topology->attributes, key, length);
}

static HRESULT WINAPI topology_GetString(IMFTopology *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p, %d, %p)\n", iface, debugstr_guid(key), value, size, length);

    return IMFAttributes_GetString(topology->attributes, key, value, size, length);
}

static HRESULT WINAPI topology_GetAllocatedString(IMFTopology *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p, %p)\n", iface, debugstr_guid(key), value, length);

    return IMFAttributes_GetAllocatedString(topology->attributes, key, value, length);
}

static HRESULT WINAPI topology_GetBlobSize(IMFTopology *iface, REFGUID key, UINT32 *size)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), size);

    return IMFAttributes_GetBlobSize(topology->attributes, key, size);
}

static HRESULT WINAPI topology_GetBlob(IMFTopology *iface, REFGUID key, UINT8 *buf,
        UINT32 bufsize, UINT32 *blobsize)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p, %d, %p)\n", iface, debugstr_guid(key), buf, bufsize, blobsize);

    return IMFAttributes_GetBlob(topology->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI topology_GetAllocatedBlob(IMFTopology *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p, %p)\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_GetAllocatedBlob(topology->attributes, key, buf, size);
}

static HRESULT WINAPI topology_GetUnknown(IMFTopology *iface, REFGUID key, REFIID riid, void **ppv)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %s, %p)\n", iface, debugstr_guid(key), debugstr_guid(riid), ppv);

    return IMFAttributes_GetUnknown(topology->attributes, key, riid, ppv);
}

static HRESULT WINAPI topology_SetItem(IMFTopology *iface, REFGUID key, REFPROPVARIANT value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetItem(topology->attributes, key, value);
}

static HRESULT WINAPI topology_DeleteItem(IMFTopology *iface, REFGUID key)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s)\n", topology, debugstr_guid(key));

    return IMFAttributes_DeleteItem(topology->attributes, key);
}

static HRESULT WINAPI topology_DeleteAllItems(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)\n", iface);

    return IMFAttributes_DeleteAllItems(topology->attributes);
}

static HRESULT WINAPI topology_SetUINT32(IMFTopology *iface, REFGUID key, UINT32 value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %d)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetUINT32(topology->attributes, key, value);
}

static HRESULT WINAPI topology_SetUINT64(IMFTopology *iface, REFGUID key, UINT64 value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %s)\n", iface, debugstr_guid(key), wine_dbgstr_longlong(value));

    return IMFAttributes_SetUINT64(topology->attributes, key, value);
}

static HRESULT WINAPI topology_SetDouble(IMFTopology *iface, REFGUID key, double value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %f)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetDouble(topology->attributes, key, value);
}

static HRESULT WINAPI topology_SetGUID(IMFTopology *iface, REFGUID key, REFGUID value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %s)\n", iface, debugstr_guid(key), debugstr_guid(value));

    return IMFAttributes_SetGUID(topology->attributes, key, value);
}

static HRESULT WINAPI topology_SetString(IMFTopology *iface, REFGUID key, const WCHAR *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %s)\n", iface, debugstr_guid(key), debugstr_w(value));

    return IMFAttributes_SetString(topology->attributes, key, value);
}

static HRESULT WINAPI topology_SetBlob(IMFTopology *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p, %d)\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_SetBlob(topology->attributes, key, buf, size);
}

static HRESULT WINAPI topology_SetUnknown(IMFTopology *iface, REFGUID key, IUnknown *unknown)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), unknown);

    return IMFAttributes_SetUnknown(topology->attributes, key, unknown);
}

static HRESULT WINAPI topology_LockStore(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)\n", iface);

    return IMFAttributes_LockStore(topology->attributes);
}

static HRESULT WINAPI topology_UnlockStore(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)\n", iface);

    return IMFAttributes_UnlockStore(topology->attributes);
}

static HRESULT WINAPI topology_GetCount(IMFTopology *iface, UINT32 *count)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%p)\n", iface, count);

    return IMFAttributes_GetCount(topology->attributes, count);
}

static HRESULT WINAPI topology_GetItemByIndex(IMFTopology *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%u, %p, %p)\n", iface, index, key, value);

    return IMFAttributes_GetItemByIndex(topology->attributes, index, key, value);
}

static HRESULT WINAPI topology_CopyAllItems(IMFTopology *iface, IMFAttributes *dest)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%p)\n", iface, dest);

    return IMFAttributes_CopyAllItems(topology->attributes, dest);
}

static HRESULT WINAPI topology_GetTopologyID(IMFTopology *iface, TOPOID *id)
{
    FIXME("(%p)->(%p)\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_AddNode(IMFTopology *iface, IMFTopologyNode *node)
{
    FIXME("(%p)->(%p)\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_RemoveNode(IMFTopology *iface, IMFTopologyNode *node)
{
    FIXME("(%p)->(%p)\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_GetNodeCount(IMFTopology *iface, WORD *count)
{
    FIXME("(%p)->(%p)\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_GetNode(IMFTopology *iface, WORD index, IMFTopologyNode **node)
{
    FIXME("(%p)->(%u, %p)\n", iface, index, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_Clear(IMFTopology *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_CloneFrom(IMFTopology *iface, IMFTopology *src_topology)
{
    FIXME("(%p)->(%p)\n", iface, src_topology);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_GetNodeByID(IMFTopology *iface, TOPOID id, IMFTopologyNode **node)
{
    FIXME("(%p)->(%p)\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_GetSourceNodeCollection(IMFTopology *iface, IMFCollection **collection)
{
    FIXME("(%p)->(%p)\n", iface, collection);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_GetOutputNodeCollection(IMFTopology *iface, IMFCollection **collection)
{
    FIXME("(%p)->(%p)\n", iface, collection);

    return E_NOTIMPL;
}

static const IMFTopologyVtbl topologyvtbl =
{
    topology_QueryInterface,
    topology_AddRef,
    topology_Release,
    topology_GetItem,
    topology_GetItemType,
    topology_CompareItem,
    topology_Compare,
    topology_GetUINT32,
    topology_GetUINT64,
    topology_GetDouble,
    topology_GetGUID,
    topology_GetStringLength,
    topology_GetString,
    topology_GetAllocatedString,
    topology_GetBlobSize,
    topology_GetBlob,
    topology_GetAllocatedBlob,
    topology_GetUnknown,
    topology_SetItem,
    topology_DeleteItem,
    topology_DeleteAllItems,
    topology_SetUINT32,
    topology_SetUINT64,
    topology_SetDouble,
    topology_SetGUID,
    topology_SetString,
    topology_SetBlob,
    topology_SetUnknown,
    topology_LockStore,
    topology_UnlockStore,
    topology_GetCount,
    topology_GetItemByIndex,
    topology_CopyAllItems,
    topology_GetTopologyID,
    topology_AddNode,
    topology_RemoveNode,
    topology_GetNodeCount,
    topology_GetNode,
    topology_Clear,
    topology_CloneFrom,
    topology_GetNodeByID,
    topology_GetSourceNodeCollection,
    topology_GetOutputNodeCollection,
};

/***********************************************************************
 *      MFCreateTopology (mf.@)
 */
HRESULT WINAPI MFCreateTopology(IMFTopology **topology)
{
    struct topology *object;
    HRESULT hr;

    TRACE("(%p)\n", topology);

    if (!topology)
        return E_POINTER;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFTopology_iface.lpVtbl = &topologyvtbl;
    object->refcount = 1;
    hr = MFCreateAttributes(&object->attributes, 0);
    if (FAILED(hr))
    {
        heap_free(object);
        return hr;
    }

    *topology = &object->IMFTopology_iface;

    return S_OK;
}
