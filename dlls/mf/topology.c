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
#include "wine/port.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "mfapi.h"
#include "mferror.h"
#include "mfidl.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static LONG next_node_id;
static TOPOID next_topology_id;

struct topology
{
    IMFTopology IMFTopology_iface;
    LONG refcount;
    IMFAttributes *attributes;
    IMFCollection *nodes;
    TOPOID id;
};

struct topology_node
{
    IMFTopologyNode IMFTopologyNode_iface;
    LONG refcount;
    IMFAttributes *attributes;
    MF_TOPOLOGY_TYPE node_type;
    TOPOID id;
};

struct topology_loader
{
    IMFTopoLoader IMFTopoLoader_iface;
    LONG refcount;
};

struct seq_source
{
    IMFSequencerSource IMFSequencerSource_iface;
    LONG refcount;
};

static inline struct topology *impl_from_IMFTopology(IMFTopology *iface)
{
    return CONTAINING_RECORD(iface, struct topology, IMFTopology_iface);
}

static struct topology_node *impl_from_IMFTopologyNode(IMFTopologyNode *iface)
{
    return CONTAINING_RECORD(iface, struct topology_node, IMFTopologyNode_iface);
}

static struct topology_loader *impl_from_IMFTopoLoader(IMFTopoLoader *iface)
{
    return CONTAINING_RECORD(iface, struct topology_loader, IMFTopoLoader_iface);
}

static struct seq_source *impl_from_IMFSequencerSource(IMFSequencerSource *iface)
{
    return CONTAINING_RECORD(iface, struct seq_source, IMFSequencerSource_iface);
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
        if (topology->attributes)
            IMFAttributes_Release(topology->attributes);
        if (topology->nodes)
            IMFCollection_Release(topology->nodes);
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
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%p)\n", iface, id);

    if (!id)
        return E_POINTER;

    *id = topology->id;

    return S_OK;
}

static HRESULT topology_get_node_by_id(const struct topology *topology, TOPOID id, IMFTopologyNode **node)
{
    IMFTopologyNode *iter;
    unsigned int i = 0;

    while (IMFCollection_GetElement(topology->nodes, i, (IUnknown **)&iter) == S_OK)
    {
        TOPOID node_id;
        HRESULT hr;

        hr = IMFTopologyNode_GetTopoNodeID(iter, &node_id);
        if (FAILED(hr))
            return hr;

        if (node_id == id)
        {
            *node = iter;
            return S_OK;
        }

        ++i;
    }

    return MF_E_NOT_FOUND;
}

static HRESULT WINAPI topology_AddNode(IMFTopology *iface, IMFTopologyNode *node)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    IMFTopologyNode *match;
    HRESULT hr;
    TOPOID id;

    TRACE("(%p)->(%p)\n", iface, node);

    if (!node)
        return E_POINTER;

    if (FAILED(hr = IMFTopologyNode_GetTopoNodeID(node, &id)))
        return hr;

    if (FAILED(topology_get_node_by_id(topology, id, &match)))
        return IMFCollection_AddElement(topology->nodes, (IUnknown *)node);

    IMFTopologyNode_Release(match);

    return E_INVALIDARG;
}

static HRESULT WINAPI topology_RemoveNode(IMFTopology *iface, IMFTopologyNode *node)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    unsigned int i = 0;
    IUnknown *element;

    TRACE("(%p)->(%p)\n", iface, node);

    while (IMFCollection_GetElement(topology->nodes, i, &element) == S_OK)
    {
        BOOL match = element == (IUnknown *)node;

        IUnknown_Release(element);

        if (match)
        {
            IMFCollection_RemoveElement(topology->nodes, i, &element);
            IUnknown_Release(element);
            return S_OK;
        }

        ++i;
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI topology_GetNodeCount(IMFTopology *iface, WORD *count)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    DWORD nodecount;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", iface, count);

    hr = IMFCollection_GetElementCount(topology->nodes, count ? &nodecount : NULL);
    if (count)
        *count = nodecount;

    return hr;
}

static HRESULT WINAPI topology_GetNode(IMFTopology *iface, WORD index, IMFTopologyNode **node)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%u, %p)\n", iface, index, node);

    if (!node)
        return E_POINTER;

    return SUCCEEDED(IMFCollection_GetElement(topology->nodes, index, (IUnknown **)node)) ?
            S_OK : MF_E_INVALIDINDEX;
}

static HRESULT WINAPI topology_Clear(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)\n", iface);

    return IMFCollection_RemoveAllElements(topology->nodes);
}

static HRESULT WINAPI topology_CloneFrom(IMFTopology *iface, IMFTopology *src_topology)
{
    FIXME("(%p)->(%p)\n", iface, src_topology);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_GetNodeByID(IMFTopology *iface, TOPOID id, IMFTopologyNode **node)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%p)\n", iface, node);

    return topology_get_node_by_id(topology, id, node);
}

static HRESULT topology_add_node_of_type(const struct topology *topology, IMFTopologyNode *node,
        MF_TOPOLOGY_TYPE filter, IMFCollection *collection)
{
    MF_TOPOLOGY_TYPE node_type;
    HRESULT hr;

    if (FAILED(hr = IMFTopologyNode_GetNodeType(node, &node_type)))
        return hr;

    if (node_type != filter)
        return S_OK;

    return IMFCollection_AddElement(collection, (IUnknown *)node);
}

static HRESULT topology_get_node_collection(const struct topology *topology, MF_TOPOLOGY_TYPE node_type,
        IMFCollection **collection)
{
    IMFTopologyNode *node;
    unsigned int i = 0;
    HRESULT hr;

    if (!collection)
        return E_POINTER;

    if (FAILED(hr = MFCreateCollection(collection)))
        return hr;

    while (IMFCollection_GetElement(topology->nodes, i++, (IUnknown **)&node) == S_OK)
    {
        hr = topology_add_node_of_type(topology, node, node_type, *collection);
        IMFTopologyNode_Release(node);
        if (FAILED(hr))
        {
            IMFCollection_Release(*collection);
            *collection = NULL;
            break;
        }
    }

    return hr;
}

static HRESULT WINAPI topology_GetSourceNodeCollection(IMFTopology *iface, IMFCollection **collection)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%p)\n", iface, collection);

    return topology_get_node_collection(topology, MF_TOPOLOGY_SOURCESTREAM_NODE, collection);
}

static HRESULT WINAPI topology_GetOutputNodeCollection(IMFTopology *iface, IMFCollection **collection)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("(%p)->(%p)\n", iface, collection);

    return topology_get_node_collection(topology, MF_TOPOLOGY_OUTPUT_NODE, collection);
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

static TOPOID topology_generate_id(void)
{
    TOPOID old;

    do
    {
        old = next_topology_id;
    }
    while (interlocked_cmpxchg64((LONG64 *)&next_topology_id, old + 1, old) != old);

    return next_topology_id;
}

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
    if (SUCCEEDED(hr))
        hr = MFCreateCollection(&object->nodes);

    if (FAILED(hr))
    {
        IMFTopology_Release(&object->IMFTopology_iface);
        return hr;
    }

    object->id = topology_generate_id();

    *topology = &object->IMFTopology_iface;

    return S_OK;
}

static HRESULT WINAPI topology_node_QueryInterface(IMFTopologyNode *iface, REFIID riid, void **out)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s %p)\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFTopologyNode) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &node->IMFTopologyNode_iface;
        IMFTopologyNode_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI topology_node_AddRef(IMFTopologyNode *iface)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    ULONG refcount = InterlockedIncrement(&node->refcount);

    TRACE("(%p) refcount=%u\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI topology_node_Release(IMFTopologyNode *iface)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    ULONG refcount = InterlockedDecrement(&node->refcount);

    TRACE("(%p) refcount=%u\n", iface, refcount);

    if (!refcount)
    {
        IMFAttributes_Release(node->attributes);
        heap_free(node);
    }

    return refcount;
}

static HRESULT WINAPI topology_node_GetItem(IMFTopologyNode *iface, REFGUID key, PROPVARIANT *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetItem(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_GetItemType(IMFTopologyNode *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), type);

    return IMFAttributes_GetItemType(node->attributes, key, type);
}

static HRESULT WINAPI topology_node_CompareItem(IMFTopologyNode *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p, %p)\n", iface, debugstr_guid(key), value, result);

    return IMFAttributes_CompareItem(node->attributes, key, value, result);
}

static HRESULT WINAPI topology_node_Compare(IMFTopologyNode *iface, IMFAttributes *theirs,
        MF_ATTRIBUTES_MATCH_TYPE type, BOOL *result)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%p, %d, %p)\n", iface, theirs, type, result);

    return IMFAttributes_Compare(node->attributes, theirs, type, result);
}

static HRESULT WINAPI topology_node_GetUINT32(IMFTopologyNode *iface, REFGUID key, UINT32 *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT32(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_GetUINT64(IMFTopologyNode *iface, REFGUID key, UINT64 *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT64(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_GetDouble(IMFTopologyNode *iface, REFGUID key, double *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetDouble(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_GetGUID(IMFTopologyNode *iface, REFGUID key, GUID *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetGUID(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_GetStringLength(IMFTopologyNode *iface, REFGUID key, UINT32 *length)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), length);

    return IMFAttributes_GetStringLength(node->attributes, key, length);
}

static HRESULT WINAPI topology_node_GetString(IMFTopologyNode *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p, %d, %p)\n", iface, debugstr_guid(key), value, size, length);

    return IMFAttributes_GetString(node->attributes, key, value, size, length);
}

static HRESULT WINAPI topology_node_GetAllocatedString(IMFTopologyNode *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p, %p)\n", iface, debugstr_guid(key), value, length);

    return IMFAttributes_GetAllocatedString(node->attributes, key, value, length);
}

static HRESULT WINAPI topology_node_GetBlobSize(IMFTopologyNode *iface, REFGUID key, UINT32 *size)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), size);

    return IMFAttributes_GetBlobSize(node->attributes, key, size);
}

static HRESULT WINAPI topology_node_GetBlob(IMFTopologyNode *iface, REFGUID key, UINT8 *buf,
        UINT32 bufsize, UINT32 *blobsize)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p, %d, %p)\n", iface, debugstr_guid(key), buf, bufsize, blobsize);

    return IMFAttributes_GetBlob(node->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI topology_node_GetAllocatedBlob(IMFTopologyNode *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p, %p)\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_GetAllocatedBlob(node->attributes, key, buf, size);
}

static HRESULT WINAPI topology_node_GetUnknown(IMFTopologyNode *iface, REFGUID key, REFIID riid, void **ppv)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %s, %p)\n", iface, debugstr_guid(key), debugstr_guid(riid), ppv);

    return IMFAttributes_GetUnknown(node->attributes, key, riid, ppv);
}

static HRESULT WINAPI topology_node_SetItem(IMFTopologyNode *iface, REFGUID key, REFPROPVARIANT value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetItem(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_DeleteItem(IMFTopologyNode *iface, REFGUID key)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s)\n", iface, debugstr_guid(key));

    return IMFAttributes_DeleteItem(node->attributes, key);
}

static HRESULT WINAPI topology_node_DeleteAllItems(IMFTopologyNode *iface)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)\n", iface);

    return IMFAttributes_DeleteAllItems(node->attributes);
}

static HRESULT WINAPI topology_node_SetUINT32(IMFTopologyNode *iface, REFGUID key, UINT32 value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %d)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetUINT32(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_SetUINT64(IMFTopologyNode *iface, REFGUID key, UINT64 value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %s)\n", iface, debugstr_guid(key), wine_dbgstr_longlong(value));

    return IMFAttributes_SetUINT64(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_SetDouble(IMFTopologyNode *iface, REFGUID key, double value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %f)\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetDouble(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_SetGUID(IMFTopologyNode *iface, REFGUID key, REFGUID value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %s)\n", iface, debugstr_guid(key), debugstr_guid(value));

    return IMFAttributes_SetGUID(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_SetString(IMFTopologyNode *iface, REFGUID key, const WCHAR *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %s)\n", iface, debugstr_guid(key), debugstr_w(value));

    return IMFAttributes_SetString(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_SetBlob(IMFTopologyNode *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p, %d)\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_SetBlob(node->attributes, key, buf, size);
}

static HRESULT WINAPI topology_node_SetUnknown(IMFTopologyNode *iface, REFGUID key, IUnknown *unknown)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), unknown);

    return IMFAttributes_SetUnknown(node->attributes, key, unknown);
}

static HRESULT WINAPI topology_node_LockStore(IMFTopologyNode *iface)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)\n", iface);

    return IMFAttributes_LockStore(node->attributes);
}

static HRESULT WINAPI topology_node_UnlockStore(IMFTopologyNode *iface)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)\n", iface);

    return IMFAttributes_UnlockStore(node->attributes);
}

static HRESULT WINAPI topology_node_GetCount(IMFTopologyNode *iface, UINT32 *count)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%p)\n", iface, count);

    return IMFAttributes_GetCount(node->attributes, count);
}

static HRESULT WINAPI topology_node_GetItemByIndex(IMFTopologyNode *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%u, %p, %p)\n", iface, index, key, value);

    return IMFAttributes_GetItemByIndex(node->attributes, index, key, value);
}

static HRESULT WINAPI topology_node_CopyAllItems(IMFTopologyNode *iface, IMFAttributes *dest)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%p)\n", iface, dest);

    return IMFAttributes_CopyAllItems(node->attributes, dest);
}

static HRESULT WINAPI topology_node_SetObject(IMFTopologyNode *iface, IUnknown *object)
{
    FIXME("(%p)->(%p)\n", iface, object);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_node_GetObject(IMFTopologyNode *iface, IUnknown **object)
{
    FIXME("(%p)->(%p)\n", iface, object);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_node_GetNodeType(IMFTopologyNode *iface, MF_TOPOLOGY_TYPE *node_type)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%p)\n", iface, node_type);

    *node_type = node->node_type;

    return S_OK;
}

static HRESULT WINAPI topology_node_GetTopoNodeID(IMFTopologyNode *iface, TOPOID *id)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%p)\n", iface, id);

    *id = node->id;

    return S_OK;
}

static HRESULT WINAPI topology_node_SetTopoNodeID(IMFTopologyNode *iface, TOPOID id)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("(%p)->(%s)\n", iface, wine_dbgstr_longlong(id));

    node->id = id;

    return S_OK;
}

static HRESULT WINAPI topology_node_GetInputCount(IMFTopologyNode *iface, DWORD *count)
{
    FIXME("(%p)->(%p)\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_node_GetOutputCount(IMFTopologyNode *iface, DWORD *count)
{
    FIXME("(%p)->(%p)\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_node_ConnectOutput(IMFTopologyNode *iface, DWORD output_index,
        IMFTopologyNode *node, DWORD input_index)
{
    FIXME("(%p)->(%u, %p, %u)\n", iface, output_index, node, input_index);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_node_DisconnectOutput(IMFTopologyNode *iface, DWORD index)
{
    FIXME("(%p)->(%u)\n", iface, index);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_node_GetInput(IMFTopologyNode *iface, DWORD input_index, IMFTopologyNode **node,
        DWORD *output_index)
{
    FIXME("(%p)->(%u, %p, %p)\n", iface, input_index, node, output_index);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_node_GetOutput(IMFTopologyNode *iface, DWORD output_index, IMFTopologyNode **node,
        DWORD *input_index)
{
    FIXME("(%p)->(%u, %p, %p)\n", iface, output_index, node, input_index);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_node_SetOutputPrefType(IMFTopologyNode *iface, DWORD index, IMFMediaType *type)
{
    FIXME("(%p)->(%u, %p)\n", iface, index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_node_GetOutputPrefType(IMFTopologyNode *iface, DWORD output_index, IMFMediaType **type)
{
    FIXME("(%p)->(%u, %p)\n", iface, output_index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_node_SetInputPrefType(IMFTopologyNode *iface, DWORD index, IMFMediaType *type)
{
    FIXME("(%p)->(%u, %p)\n", iface, index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_node_GetInputPrefType(IMFTopologyNode *iface, DWORD index, IMFMediaType **type)
{
    FIXME("(%p)->(%u, %p)\n", iface, index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI topology_node_CloneFrom(IMFTopologyNode *iface, IMFTopologyNode *node)
{
    FIXME("(%p)->(%p)\n", iface, node);

    return E_NOTIMPL;
}

static const IMFTopologyNodeVtbl topologynodevtbl =
{
    topology_node_QueryInterface,
    topology_node_AddRef,
    topology_node_Release,
    topology_node_GetItem,
    topology_node_GetItemType,
    topology_node_CompareItem,
    topology_node_Compare,
    topology_node_GetUINT32,
    topology_node_GetUINT64,
    topology_node_GetDouble,
    topology_node_GetGUID,
    topology_node_GetStringLength,
    topology_node_GetString,
    topology_node_GetAllocatedString,
    topology_node_GetBlobSize,
    topology_node_GetBlob,
    topology_node_GetAllocatedBlob,
    topology_node_GetUnknown,
    topology_node_SetItem,
    topology_node_DeleteItem,
    topology_node_DeleteAllItems,
    topology_node_SetUINT32,
    topology_node_SetUINT64,
    topology_node_SetDouble,
    topology_node_SetGUID,
    topology_node_SetString,
    topology_node_SetBlob,
    topology_node_SetUnknown,
    topology_node_LockStore,
    topology_node_UnlockStore,
    topology_node_GetCount,
    topology_node_GetItemByIndex,
    topology_node_CopyAllItems,
    topology_node_SetObject,
    topology_node_GetObject,
    topology_node_GetNodeType,
    topology_node_GetTopoNodeID,
    topology_node_SetTopoNodeID,
    topology_node_GetInputCount,
    topology_node_GetOutputCount,
    topology_node_ConnectOutput,
    topology_node_DisconnectOutput,
    topology_node_GetInput,
    topology_node_GetOutput,
    topology_node_SetOutputPrefType,
    topology_node_GetOutputPrefType,
    topology_node_SetInputPrefType,
    topology_node_GetInputPrefType,
    topology_node_CloneFrom,
};

/***********************************************************************
 *      MFCreateTopologyNode (mf.@)
 */
HRESULT WINAPI MFCreateTopologyNode(MF_TOPOLOGY_TYPE node_type, IMFTopologyNode **node)
{
    struct topology_node *object;
    HRESULT hr;

    TRACE("(%p)\n", node);

    if (!node)
        return E_POINTER;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFTopologyNode_iface.lpVtbl = &topologynodevtbl;
    object->refcount = 1;
    object->node_type = node_type;
    hr = MFCreateAttributes(&object->attributes, 0);
    if (FAILED(hr))
    {
        heap_free(object);
        return hr;
    }
    object->id = ((TOPOID)GetCurrentProcessId() << 32) | InterlockedIncrement(&next_node_id);

    *node = &object->IMFTopologyNode_iface;

    return S_OK;
}

static HRESULT WINAPI topology_loader_QueryInterface(IMFTopoLoader *iface, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFTopoLoader) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFTopoLoader_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI topology_loader_AddRef(IMFTopoLoader *iface)
{
    struct topology_loader *loader = impl_from_IMFTopoLoader(iface);
    ULONG refcount = InterlockedIncrement(&loader->refcount);

    TRACE("(%p) refcount=%u\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI topology_loader_Release(IMFTopoLoader *iface)
{
    struct topology_loader *loader = impl_from_IMFTopoLoader(iface);
    ULONG refcount = InterlockedDecrement(&loader->refcount);

    TRACE("(%p) refcount=%u\n", iface, refcount);

    if (!refcount)
    {
        heap_free(loader);
    }

    return refcount;
}

static HRESULT WINAPI topology_loader_Load(IMFTopoLoader *iface, IMFTopology *input_topology,
        IMFTopology **output_topology, IMFTopology *current_topology)
{
    FIXME("%p, %p, %p, %p.\n", iface, input_topology, output_topology, current_topology);

    return E_NOTIMPL;
}

static const IMFTopoLoaderVtbl topologyloadervtbl =
{
    topology_loader_QueryInterface,
    topology_loader_AddRef,
    topology_loader_Release,
    topology_loader_Load,
};

/***********************************************************************
 *      MFCreateTopoLoader (mf.@)
 */
HRESULT WINAPI MFCreateTopoLoader(IMFTopoLoader **loader)
{
    struct topology_loader *object;

    TRACE("%p.\n", loader);

    if (!loader)
        return E_POINTER;

    object = heap_alloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFTopoLoader_iface.lpVtbl = &topologyloadervtbl;
    object->refcount = 1;

    *loader = &object->IMFTopoLoader_iface;

    return S_OK;
}

static HRESULT WINAPI seq_source_QueryInterface(IMFSequencerSource *iface, REFIID riid, void **out)
{
    struct seq_source *seq_source = impl_from_IMFSequencerSource(iface);

    TRACE("(%p)->(%s %p)\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFSequencerSource) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &seq_source->IMFSequencerSource_iface;
        IMFSequencerSource_AddRef(iface);
        return S_OK;
    }

    WARN("Unimplemented %s.\n", debugstr_guid(riid));
    *out = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI seq_source_AddRef(IMFSequencerSource *iface)
{
    struct seq_source *seq_source = impl_from_IMFSequencerSource(iface);
    ULONG refcount = InterlockedIncrement(&seq_source->refcount);

    TRACE("(%p) refcount=%u\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI seq_source_Release(IMFSequencerSource *iface)
{
    struct seq_source *seq_source = impl_from_IMFSequencerSource(iface);
    ULONG refcount = InterlockedDecrement(&seq_source->refcount);

    TRACE("(%p) refcount=%u\n", iface, refcount);

    if (!refcount)
    {
        heap_free(seq_source);
    }

    return refcount;
}

static HRESULT WINAPI seq_source_AppendTopology(IMFSequencerSource *iface, IMFTopology *topology,
        DWORD flags, MFSequencerElementId *id)
{
    FIXME("%p, %p, %x, %p\n", iface, topology, flags, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI seq_source_DeleteTopology(IMFSequencerSource *iface, MFSequencerElementId id)
{
    FIXME("%p, %#x\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI seq_source_GetPresentationContext(IMFSequencerSource *iface,
        IMFPresentationDescriptor *descriptor, MFSequencerElementId *id, IMFTopology **topology)
{
    FIXME("%p, %p, %p, %p\n", iface, descriptor, id, topology);

    return E_NOTIMPL;
}

static HRESULT WINAPI seq_source_UpdateTopology(IMFSequencerSource *iface, MFSequencerElementId id,
        IMFTopology *topology)
{
    FIXME("%p, %#x, %p\n", iface, id, topology);

    return E_NOTIMPL;
}

static HRESULT WINAPI seq_source_UpdateTopologyFlags(IMFSequencerSource *iface, MFSequencerElementId id, DWORD flags)
{
    FIXME("%p, %#x, %#x\n", iface, id, flags);

    return E_NOTIMPL;
}

static const IMFSequencerSourceVtbl seqsourcevtbl =
{
    seq_source_QueryInterface,
    seq_source_AddRef,
    seq_source_Release,
    seq_source_AppendTopology,
    seq_source_DeleteTopology,
    seq_source_GetPresentationContext,
    seq_source_UpdateTopology,
    seq_source_UpdateTopologyFlags,
};

/***********************************************************************
 *      MFCreateSequencerSource (mf.@)
 */
HRESULT WINAPI MFCreateSequencerSource(IUnknown *reserved, IMFSequencerSource **seq_source)
{
    struct seq_source *object;

    TRACE("(%p, %p)\n", reserved, seq_source);

    if (!seq_source)
        return E_POINTER;

    object = heap_alloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFSequencerSource_iface.lpVtbl = &seqsourcevtbl;
    object->refcount = 1;

    *seq_source = &object->IMFSequencerSource_iface;

    return S_OK;
}
