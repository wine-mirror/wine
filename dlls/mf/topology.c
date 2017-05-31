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
#include "mfidl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

typedef struct mftopology
{
    IMFTopology IMFTopology_iface;
    LONG ref;
} mftopology;

static inline mftopology *impl_from_IMFTopology(IMFTopology *iface)
{
    return CONTAINING_RECORD(iface, mftopology, IMFTopology_iface);
}

static HRESULT WINAPI mftopology_QueryInterface(IMFTopology *iface, REFIID riid, void **out)
{
    mftopology *This = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFTopology) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &This->IMFTopology_iface;
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

static ULONG WINAPI mftopology_AddRef(IMFTopology *iface)
{
    mftopology *This = impl_from_IMFTopology(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI mftopology_Release(IMFTopology *iface)
{
    mftopology *This = impl_from_IMFTopology(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI mftopology_GetItem(IMFTopology *iface, REFGUID key, PROPVARIANT *value)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p)\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetItemType(IMFTopology *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p)\n", This, debugstr_guid(key), type);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_CompareItem(IMFTopology *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p, %p)\n", This, debugstr_guid(key), value, result);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_Compare(IMFTopology *iface, IMFAttributes *theirs, MF_ATTRIBUTES_MATCH_TYPE type,
                BOOL *result)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%p, %d, %p)\n", This, theirs, type, result);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetUINT32(IMFTopology *iface, REFGUID key, UINT32 *value)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p)\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetUINT64(IMFTopology *iface, REFGUID key, UINT64 *value)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p)\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetDouble(IMFTopology *iface, REFGUID key, double *value)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p)\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetGUID(IMFTopology *iface, REFGUID key, GUID *value)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p)\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetStringLength(IMFTopology *iface, REFGUID key, UINT32 *length)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p)\n", This, debugstr_guid(key), length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetString(IMFTopology *iface, REFGUID key, WCHAR *value,
                UINT32 size, UINT32 *length)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p, %d, %p)\n", This, debugstr_guid(key), value, size, length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetAllocatedString(IMFTopology *iface, REFGUID key,
                                      WCHAR **value, UINT32 *length)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p, %p)\n", This, debugstr_guid(key), value, length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetBlobSize(IMFTopology *iface, REFGUID key, UINT32 *size)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p)\n", This, debugstr_guid(key), size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetBlob(IMFTopology *iface, REFGUID key, UINT8 *buf,
                UINT32 bufsize, UINT32 *blobsize)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p, %d, %p)\n", This, debugstr_guid(key), buf, bufsize, blobsize);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetAllocatedBlob(IMFTopology *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p, %p)\n", This, debugstr_guid(key), buf, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetUnknown(IMFTopology *iface, REFGUID key, REFIID riid, void **ppv)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %s, %p)\n", This, debugstr_guid(key), debugstr_guid(riid), ppv);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_SetItem(IMFTopology *iface, REFGUID key, REFPROPVARIANT Value)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p)\n", This, debugstr_guid(key), Value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_DeleteItem(IMFTopology *iface, REFGUID key)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s)\n", This, debugstr_guid(key));

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_DeleteAllItems(IMFTopology *iface)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_SetUINT32(IMFTopology *iface, REFGUID key, UINT32 value)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %d)\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_SetUINT64(IMFTopology *iface, REFGUID key, UINT64 value)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %s)\n", This, debugstr_guid(key), wine_dbgstr_longlong(value));

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_SetDouble(IMFTopology *iface, REFGUID key, double value)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %f)\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_SetGUID(IMFTopology *iface, REFGUID key, REFGUID value)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %s)\n", This, debugstr_guid(key), debugstr_guid(value));

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_SetString(IMFTopology *iface, REFGUID key, const WCHAR *value)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %s)\n", This, debugstr_guid(key), debugstr_w(value));

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_SetBlob(IMFTopology *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p, %d)\n", This, debugstr_guid(key), buf, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_SetUnknown(IMFTopology *iface, REFGUID key, IUnknown *unknown)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%s, %p)\n", This, debugstr_guid(key), unknown);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_LockStore(IMFTopology *iface)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_UnlockStore(IMFTopology *iface)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetCount(IMFTopology *iface, UINT32 *count)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%p)\n", This, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetItemByIndex(IMFTopology *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%u, %p, %p)\n", This, index, key, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_CopyAllItems(IMFTopology *iface, IMFAttributes *dest)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%p)\n", This, dest);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetTopologyID(IMFTopology *iface, TOPOID *id)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%p)\n", This, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_AddNode(IMFTopology *iface, IMFTopologyNode *node)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%p)\n", This, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_RemoveNode(IMFTopology *iface, IMFTopologyNode *node)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%p)\n", This, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetNodeCount(IMFTopology *iface, WORD *count)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%p)\n", This, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetNode(IMFTopology *iface, WORD index, IMFTopologyNode **node)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%u, %p)\n", This, index, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_Clear(IMFTopology *iface)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_CloneFrom(IMFTopology *iface, IMFTopology *topology)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%p)\n", This, topology);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetNodeByID(IMFTopology *iface, TOPOID id, IMFTopologyNode **node)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%p)\n", This, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetSourceNodeCollection(IMFTopology *iface, IMFCollection **collection)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%p)\n", This, collection);

    return E_NOTIMPL;
}

static HRESULT WINAPI mftopology_GetOutputNodeCollection(IMFTopology *iface, IMFCollection **collection)
{
    mftopology *This = impl_from_IMFTopology(iface);

    FIXME("(%p)->(%p)\n", This, collection);

    return E_NOTIMPL;
}

static const IMFTopologyVtbl mftopologyvtbl =
{
    mftopology_QueryInterface,
    mftopology_AddRef,
    mftopology_Release,
    mftopology_GetItem,
    mftopology_GetItemType,
    mftopology_CompareItem,
    mftopology_Compare,
    mftopology_GetUINT32,
    mftopology_GetUINT64,
    mftopology_GetDouble,
    mftopology_GetGUID,
    mftopology_GetStringLength,
    mftopology_GetString,
    mftopology_GetAllocatedString,
    mftopology_GetBlobSize,
    mftopology_GetBlob,
    mftopology_GetAllocatedBlob,
    mftopology_GetUnknown,
    mftopology_SetItem,
    mftopology_DeleteItem,
    mftopology_DeleteAllItems,
    mftopology_SetUINT32,
    mftopology_SetUINT64,
    mftopology_SetDouble,
    mftopology_SetGUID,
    mftopology_SetString,
    mftopology_SetBlob,
    mftopology_SetUnknown,
    mftopology_LockStore,
    mftopology_UnlockStore,
    mftopology_GetCount,
    mftopology_GetItemByIndex,
    mftopology_CopyAllItems,
    mftopology_GetTopologyID,
    mftopology_AddNode,
    mftopology_RemoveNode,
    mftopology_GetNodeCount,
    mftopology_GetNode,
    mftopology_Clear,
    mftopology_CloneFrom,
    mftopology_GetNodeByID,
    mftopology_GetSourceNodeCollection,
    mftopology_GetOutputNodeCollection,
};

/***********************************************************************
 *      MFCreateTopology (mf.@)
 */
HRESULT WINAPI MFCreateTopology(IMFTopology **topology)
{
    mftopology *object;

    if (!topology)
        return E_POINTER;

    object = HeapAlloc(GetProcessHeap(), 0, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFTopology_iface.lpVtbl = &mftopologyvtbl;
    object->ref = 1;

    *topology = &object->IMFTopology_iface;

    return S_OK;
}
