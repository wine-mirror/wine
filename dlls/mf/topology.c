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


#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"

#undef INITGUID
#include <guiddef.h>
#include "mfidl.h"

#include "wine/debug.h"

#include "mf_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static LONG next_node_id;
static TOPOID next_topology_id;

struct node_stream
{
    IMFMediaType *preferred_type;
    struct topology_node *connection;
    DWORD connection_stream;
};

struct node_streams
{
    struct node_stream *streams;
    size_t size;
    size_t count;
};

struct topology_node
{
    IMFTopologyNode IMFTopologyNode_iface;
    LONG refcount;
    IMFAttributes *attributes;
    MF_TOPOLOGY_TYPE node_type;
    TOPOID id;
    IUnknown *object;
    IMFMediaType *input_type; /* Only for tee nodes. */
    struct node_streams inputs;
    struct node_streams outputs;
    CRITICAL_SECTION cs;
};

struct topology
{
    IMFTopology IMFTopology_iface;
    LONG refcount;
    IMFAttributes *attributes;
    struct
    {
        struct topology_node **nodes;
        size_t size;
        size_t count;
    } nodes;
    TOPOID id;
};

struct seq_source
{
    IMFSequencerSource IMFSequencerSource_iface;
    IMFMediaSourceTopologyProvider IMFMediaSourceTopologyProvider_iface;
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

static const IMFTopologyNodeVtbl topologynodevtbl;

static struct topology_node *unsafe_impl_from_IMFTopologyNode(IMFTopologyNode *iface)
{
    if (!iface || iface->lpVtbl != &topologynodevtbl)
        return NULL;
    return impl_from_IMFTopologyNode(iface);
}

static HRESULT create_topology_node(MF_TOPOLOGY_TYPE node_type, struct topology_node **node);
static HRESULT topology_node_connect_output(struct topology_node *node, DWORD output_index,
        struct topology_node *connection, DWORD input_index);

static struct topology *unsafe_impl_from_IMFTopology(IMFTopology *iface);

static struct seq_source *impl_from_IMFSequencerSource(IMFSequencerSource *iface)
{
    return CONTAINING_RECORD(iface, struct seq_source, IMFSequencerSource_iface);
}

static struct seq_source *impl_from_IMFMediaSourceTopologyProvider(IMFMediaSourceTopologyProvider *iface)
{
    return CONTAINING_RECORD(iface, struct seq_source, IMFMediaSourceTopologyProvider_iface);
}

static HRESULT topology_node_reserve_streams(struct node_streams *streams, DWORD index)
{
    if (!mf_array_reserve((void **)&streams->streams, &streams->size, index + 1, sizeof(*streams->streams)))
        return E_OUTOFMEMORY;

    if (index >= streams->count)
    {
        memset(&streams->streams[streams->count], 0, (index - streams->count + 1) * sizeof(*streams->streams));
        streams->count = index + 1;
    }

    return S_OK;
}

static HRESULT WINAPI topology_QueryInterface(IMFTopology *iface, REFIID riid, void **out)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

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

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT topology_node_disconnect_output(struct topology_node *node, DWORD output_index)
{
    struct topology_node *connection = NULL;
    struct node_stream *stream;
    DWORD connection_stream;
    HRESULT hr = S_OK;

    EnterCriticalSection(&node->cs);

    if (output_index < node->outputs.count)
    {
        stream = &node->outputs.streams[output_index];

        if (stream->connection)
        {
            connection = stream->connection;
            connection_stream = stream->connection_stream;
            stream->connection = NULL;
            stream->connection_stream = 0;
        }
        else
            hr = MF_E_NOT_FOUND;
    }
    else
        hr = E_INVALIDARG;

    LeaveCriticalSection(&node->cs);

    if (connection)
    {
        EnterCriticalSection(&connection->cs);

        if (connection_stream < connection->inputs.count)
        {
            stream = &connection->inputs.streams[connection_stream];

            if (stream->connection)
            {
                stream->connection = NULL;
                stream->connection_stream = 0;
            }
        }

        LeaveCriticalSection(&connection->cs);

        IMFTopologyNode_Release(&connection->IMFTopologyNode_iface);
        IMFTopologyNode_Release(&node->IMFTopologyNode_iface);
    }

    return hr;
}

static void topology_node_disconnect(struct topology_node *node)
{
    struct node_stream *stream;
    size_t i;

    for (i = 0; i < node->outputs.count; ++i)
        topology_node_disconnect_output(node, i);

    for (i = 0; i < node->inputs.count; ++i)
    {
        stream = &node->inputs.streams[i];
        if (stream->connection)
            topology_node_disconnect_output(stream->connection, stream->connection_stream);
    }
}

static void topology_clear(struct topology *topology)
{
    size_t i;

    for (i = 0; i < topology->nodes.count; ++i)
    {
        topology_node_disconnect(topology->nodes.nodes[i]);
        IMFTopologyNode_Release(&topology->nodes.nodes[i]->IMFTopologyNode_iface);
    }
    free(topology->nodes.nodes);
    topology->nodes.nodes = NULL;
    topology->nodes.count = 0;
    topology->nodes.size = 0;
}

static ULONG WINAPI topology_Release(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    ULONG refcount = InterlockedDecrement(&topology->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (topology->attributes)
            IMFAttributes_Release(topology->attributes);
        topology_clear(topology);
        free(topology);
    }

    return refcount;
}

static HRESULT WINAPI topology_GetItem(IMFTopology *iface, REFGUID key, PROPVARIANT *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetItem(topology->attributes, key, value);
}

static HRESULT WINAPI topology_GetItemType(IMFTopology *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), type);

    return IMFAttributes_GetItemType(topology->attributes, key, type);
}

static HRESULT WINAPI topology_CompareItem(IMFTopology *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, result);

    return IMFAttributes_CompareItem(topology->attributes, key, value, result);
}

static HRESULT WINAPI topology_Compare(IMFTopology *iface, IMFAttributes *theirs, MF_ATTRIBUTES_MATCH_TYPE type,
        BOOL *result)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %p, %d, %p.\n", iface, theirs, type, result);

    return IMFAttributes_Compare(topology->attributes, theirs, type, result);
}

static HRESULT WINAPI topology_GetUINT32(IMFTopology *iface, REFGUID key, UINT32 *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT32(topology->attributes, key, value);
}

static HRESULT WINAPI topology_GetUINT64(IMFTopology *iface, REFGUID key, UINT64 *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT64(topology->attributes, key, value);
}

static HRESULT WINAPI topology_GetDouble(IMFTopology *iface, REFGUID key, double *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetDouble(topology->attributes, key, value);
}

static HRESULT WINAPI topology_GetGUID(IMFTopology *iface, REFGUID key, GUID *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetGUID(topology->attributes, key, value);
}

static HRESULT WINAPI topology_GetStringLength(IMFTopology *iface, REFGUID key, UINT32 *length)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), length);

    return IMFAttributes_GetStringLength(topology->attributes, key, length);
}

static HRESULT WINAPI topology_GetString(IMFTopology *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_guid(key), value, size, length);

    return IMFAttributes_GetString(topology->attributes, key, value, size, length);
}

static HRESULT WINAPI topology_GetAllocatedString(IMFTopology *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, length);

    return IMFAttributes_GetAllocatedString(topology->attributes, key, value, length);
}

static HRESULT WINAPI topology_GetBlobSize(IMFTopology *iface, REFGUID key, UINT32 *size)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), size);

    return IMFAttributes_GetBlobSize(topology->attributes, key, size);
}

static HRESULT WINAPI topology_GetBlob(IMFTopology *iface, REFGUID key, UINT8 *buf,
        UINT32 bufsize, UINT32 *blobsize)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_guid(key), buf, bufsize, blobsize);

    return IMFAttributes_GetBlob(topology->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI topology_GetAllocatedBlob(IMFTopology *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_GetAllocatedBlob(topology->attributes, key, buf, size);
}

static HRESULT WINAPI topology_GetUnknown(IMFTopology *iface, REFGUID key, REFIID riid, void **ppv)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(key), debugstr_guid(riid), ppv);

    return IMFAttributes_GetUnknown(topology->attributes, key, riid, ppv);
}

static HRESULT WINAPI topology_SetItem(IMFTopology *iface, REFGUID key, REFPROPVARIANT value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetItem(topology->attributes, key, value);
}

static HRESULT WINAPI topology_DeleteItem(IMFTopology *iface, REFGUID key)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s.\n", topology, debugstr_guid(key));

    return IMFAttributes_DeleteItem(topology->attributes, key);
}

static HRESULT WINAPI topology_DeleteAllItems(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_DeleteAllItems(topology->attributes);
}

static HRESULT WINAPI topology_SetUINT32(IMFTopology *iface, REFGUID key, UINT32 value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %d.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetUINT32(topology->attributes, key, value);
}

static HRESULT WINAPI topology_SetUINT64(IMFTopology *iface, REFGUID key, UINT64 value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), wine_dbgstr_longlong(value));

    return IMFAttributes_SetUINT64(topology->attributes, key, value);
}

static HRESULT WINAPI topology_SetDouble(IMFTopology *iface, REFGUID key, double value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetDouble(topology->attributes, key, value);
}

static HRESULT WINAPI topology_SetGUID(IMFTopology *iface, REFGUID key, REFGUID value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_guid(value));

    return IMFAttributes_SetGUID(topology->attributes, key, value);
}

static HRESULT WINAPI topology_SetString(IMFTopology *iface, REFGUID key, const WCHAR *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_w(value));

    return IMFAttributes_SetString(topology->attributes, key, value);
}

static HRESULT WINAPI topology_SetBlob(IMFTopology *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p, %d.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_SetBlob(topology->attributes, key, buf, size);
}

static HRESULT WINAPI topology_SetUnknown(IMFTopology *iface, REFGUID key, IUnknown *unknown)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), unknown);

    return IMFAttributes_SetUnknown(topology->attributes, key, unknown);
}

static HRESULT WINAPI topology_LockStore(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_LockStore(topology->attributes);
}

static HRESULT WINAPI topology_UnlockStore(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_UnlockStore(topology->attributes);
}

static HRESULT WINAPI topology_GetCount(IMFTopology *iface, UINT32 *count)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %p.\n", iface, count);

    return IMFAttributes_GetCount(topology->attributes, count);
}

static HRESULT WINAPI topology_GetItemByIndex(IMFTopology *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return IMFAttributes_GetItemByIndex(topology->attributes, index, key, value);
}

static HRESULT WINAPI topology_CopyAllItems(IMFTopology *iface, IMFAttributes *dest)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %p.\n", iface, dest);

    return IMFAttributes_CopyAllItems(topology->attributes, dest);
}

static HRESULT WINAPI topology_GetTopologyID(IMFTopology *iface, TOPOID *id)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %p.\n", iface, id);

    if (!id)
        return E_POINTER;

    *id = topology->id;

    return S_OK;
}

static HRESULT topology_get_node_by_id(const struct topology *topology, TOPOID id, struct topology_node **node)
{
    size_t i = 0;

    for (i = 0; i < topology->nodes.count; ++i)
    {
        if (topology->nodes.nodes[i]->id == id)
        {
            *node = topology->nodes.nodes[i];
            return S_OK;
        }
    }

    return MF_E_NOT_FOUND;
}

static HRESULT topology_add_node(struct topology *topology, struct topology_node *node)
{
    struct topology_node *match;

    if (!node)
        return E_POINTER;

    if (SUCCEEDED(topology_get_node_by_id(topology, node->id, &match)))
        return E_INVALIDARG;

    if (!mf_array_reserve((void **)&topology->nodes.nodes, &topology->nodes.size, topology->nodes.count + 1,
            sizeof(*topology->nodes.nodes)))
    {
        return E_OUTOFMEMORY;
    }

    topology->nodes.nodes[topology->nodes.count++] = node;
    IMFTopologyNode_AddRef(&node->IMFTopologyNode_iface);

    return S_OK;
}

static HRESULT WINAPI topology_AddNode(IMFTopology *iface, IMFTopologyNode *node_iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    struct topology_node *node = unsafe_impl_from_IMFTopologyNode(node_iface);

    TRACE("%p, %p.\n", iface, node_iface);

    return topology_add_node(topology, node);
}

static HRESULT WINAPI topology_RemoveNode(IMFTopology *iface, IMFTopologyNode *node)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    size_t i, count;

    TRACE("%p, %p.\n", iface, node);

    for (i = 0; i < topology->nodes.count; ++i)
    {
        if (&topology->nodes.nodes[i]->IMFTopologyNode_iface == node)
        {
            topology_node_disconnect(topology->nodes.nodes[i]);
            IMFTopologyNode_Release(&topology->nodes.nodes[i]->IMFTopologyNode_iface);
            count = topology->nodes.count - i - 1;
            if (count)
            {
                memmove(&topology->nodes.nodes[i], &topology->nodes.nodes[i + 1],
                        count * sizeof(*topology->nodes.nodes));
            }
            topology->nodes.count--;
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI topology_GetNodeCount(IMFTopology *iface, WORD *count)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    *count = topology->nodes.count;

    return S_OK;
}

static HRESULT WINAPI topology_GetNode(IMFTopology *iface, WORD index, IMFTopologyNode **node)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %u, %p.\n", iface, index, node);

    if (!node)
        return E_POINTER;

    if (index >= topology->nodes.count)
        return MF_E_INVALIDINDEX;

    *node = &topology->nodes.nodes[index]->IMFTopologyNode_iface;
    IMFTopologyNode_AddRef(*node);

    return S_OK;
}

static HRESULT WINAPI topology_Clear(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p.\n", iface);

    topology_clear(topology);
    return S_OK;
}

static HRESULT WINAPI topology_CloneFrom(IMFTopology *iface, IMFTopology *src)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    struct topology *src_topology = unsafe_impl_from_IMFTopology(src);
    struct topology_node *node;
    size_t i, j;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, src);

    topology_clear(topology);

    /* Clone nodes. */
    for (i = 0; i < src_topology->nodes.count; ++i)
    {
        if (FAILED(hr = create_topology_node(src_topology->nodes.nodes[i]->node_type, &node)))
        {
            WARN("Failed to create a node, hr %#lx.\n", hr);
            break;
        }

        if (SUCCEEDED(hr = IMFTopologyNode_CloneFrom(&node->IMFTopologyNode_iface,
                &src_topology->nodes.nodes[i]->IMFTopologyNode_iface)))
        {
            topology_add_node(topology, node);
        }

        IMFTopologyNode_Release(&node->IMFTopologyNode_iface);
    }

    /* Clone connections. */
    for (i = 0; i < src_topology->nodes.count; ++i)
    {
        const struct node_streams *outputs = &src_topology->nodes.nodes[i]->outputs;

        for (j = 0; j < outputs->count; ++j)
        {
            DWORD input_index = outputs->streams[j].connection_stream;
            TOPOID id;

            if (!outputs->streams[j].connection)
                continue;

            id = outputs->streams[j].connection->id;

            /* Skip node lookup in destination topology, assuming same node order. */
            if (SUCCEEDED(hr = topology_get_node_by_id(topology, id, &node)))
                topology_node_connect_output(topology->nodes.nodes[i], j, node, input_index);
        }
    }

    /* Copy attributes and id. */
    hr = IMFTopology_CopyAllItems(src, (IMFAttributes *)&topology->IMFTopology_iface);
    if (SUCCEEDED(hr))
        topology->id = src_topology->id;

    return S_OK;
}

static HRESULT WINAPI topology_GetNodeByID(IMFTopology *iface, TOPOID id, IMFTopologyNode **ret)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    struct topology_node *node;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, ret);

    if (SUCCEEDED(hr = topology_get_node_by_id(topology, id, &node)))
    {
        *ret = &node->IMFTopologyNode_iface;
        IMFTopologyNode_AddRef(*ret);
    }
    else
        *ret = NULL;

    return hr;
}

static HRESULT topology_get_node_collection(const struct topology *topology, MF_TOPOLOGY_TYPE node_type,
        IMFCollection **collection)
{
    HRESULT hr;
    size_t i;

    if (!collection)
        return E_POINTER;

    if (FAILED(hr = MFCreateCollection(collection)))
        return hr;

    for (i = 0; i < topology->nodes.count; ++i)
    {
        if (topology->nodes.nodes[i]->node_type == node_type)
        {
            if (FAILED(hr = IMFCollection_AddElement(*collection,
                    (IUnknown *)&topology->nodes.nodes[i]->IMFTopologyNode_iface)))
            {
                IMFCollection_Release(*collection);
                *collection = NULL;
                break;
            }
        }
    }

    return hr;
}

static HRESULT WINAPI topology_GetSourceNodeCollection(IMFTopology *iface, IMFCollection **collection)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %p.\n", iface, collection);

    return topology_get_node_collection(topology, MF_TOPOLOGY_SOURCESTREAM_NODE, collection);
}

static HRESULT WINAPI topology_GetOutputNodeCollection(IMFTopology *iface, IMFCollection **collection)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %p.\n", iface, collection);

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

static struct topology *unsafe_impl_from_IMFTopology(IMFTopology *iface)
{
    if (!iface || iface->lpVtbl != &topologyvtbl)
        return NULL;
    return impl_from_IMFTopology(iface);
}

static TOPOID topology_generate_id(void)
{
    TOPOID old;

    do
    {
        old = next_topology_id;
    }
    while (InterlockedCompareExchange64((LONG64 *)&next_topology_id, old + 1, old) != old);

    return next_topology_id;
}

/***********************************************************************
 *      MFCreateTopology (mf.@)
 */
HRESULT WINAPI MFCreateTopology(IMFTopology **topology)
{
    struct topology *object;
    HRESULT hr;

    TRACE("%p.\n", topology);

    if (!topology)
        return E_POINTER;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFTopology_iface.lpVtbl = &topologyvtbl;
    object->refcount = 1;

    hr = MFCreateAttributes(&object->attributes, 0);
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

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFTopologyNode) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &node->IMFTopologyNode_iface;
        IMFTopologyNode_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *out = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI topology_node_AddRef(IMFTopologyNode *iface)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    ULONG refcount = InterlockedIncrement(&node->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI topology_node_Release(IMFTopologyNode *iface)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    ULONG refcount = InterlockedDecrement(&node->refcount);
    unsigned int i;

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (node->object)
            IUnknown_Release(node->object);
        if (node->input_type)
            IMFMediaType_Release(node->input_type);
        for (i = 0; i < node->inputs.count; ++i)
        {
            if (node->inputs.streams[i].preferred_type)
                IMFMediaType_Release(node->inputs.streams[i].preferred_type);
        }
        for (i = 0; i < node->outputs.count; ++i)
        {
            if (node->outputs.streams[i].preferred_type)
                IMFMediaType_Release(node->outputs.streams[i].preferred_type);
        }
        free(node->inputs.streams);
        free(node->outputs.streams);
        IMFAttributes_Release(node->attributes);
        DeleteCriticalSection(&node->cs);
        free(node);
    }

    return refcount;
}

static HRESULT WINAPI topology_node_GetItem(IMFTopologyNode *iface, REFGUID key, PROPVARIANT *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetItem(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_GetItemType(IMFTopologyNode *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), type);

    return IMFAttributes_GetItemType(node->attributes, key, type);
}

static HRESULT WINAPI topology_node_CompareItem(IMFTopologyNode *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, result);

    return IMFAttributes_CompareItem(node->attributes, key, value, result);
}

static HRESULT WINAPI topology_node_Compare(IMFTopologyNode *iface, IMFAttributes *theirs,
        MF_ATTRIBUTES_MATCH_TYPE type, BOOL *result)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p, %d, %p.\n", iface, theirs, type, result);

    return IMFAttributes_Compare(node->attributes, theirs, type, result);
}

static HRESULT WINAPI topology_node_GetUINT32(IMFTopologyNode *iface, REFGUID key, UINT32 *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT32(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_GetUINT64(IMFTopologyNode *iface, REFGUID key, UINT64 *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT64(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_GetDouble(IMFTopologyNode *iface, REFGUID key, double *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetDouble(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_GetGUID(IMFTopologyNode *iface, REFGUID key, GUID *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetGUID(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_GetStringLength(IMFTopologyNode *iface, REFGUID key, UINT32 *length)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), length);

    return IMFAttributes_GetStringLength(node->attributes, key, length);
}

static HRESULT WINAPI topology_node_GetString(IMFTopologyNode *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_guid(key), value, size, length);

    return IMFAttributes_GetString(node->attributes, key, value, size, length);
}

static HRESULT WINAPI topology_node_GetAllocatedString(IMFTopologyNode *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, length);

    return IMFAttributes_GetAllocatedString(node->attributes, key, value, length);
}

static HRESULT WINAPI topology_node_GetBlobSize(IMFTopologyNode *iface, REFGUID key, UINT32 *size)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), size);

    return IMFAttributes_GetBlobSize(node->attributes, key, size);
}

static HRESULT WINAPI topology_node_GetBlob(IMFTopologyNode *iface, REFGUID key, UINT8 *buf,
        UINT32 bufsize, UINT32 *blobsize)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_guid(key), buf, bufsize, blobsize);

    return IMFAttributes_GetBlob(node->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI topology_node_GetAllocatedBlob(IMFTopologyNode *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_GetAllocatedBlob(node->attributes, key, buf, size);
}

static HRESULT WINAPI topology_node_GetUnknown(IMFTopologyNode *iface, REFGUID key, REFIID riid, void **ppv)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(key), debugstr_guid(riid), ppv);

    return IMFAttributes_GetUnknown(node->attributes, key, riid, ppv);
}

static HRESULT WINAPI topology_node_SetItem(IMFTopologyNode *iface, REFGUID key, REFPROPVARIANT value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetItem(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_DeleteItem(IMFTopologyNode *iface, REFGUID key)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s.\n", iface, debugstr_guid(key));

    return IMFAttributes_DeleteItem(node->attributes, key);
}

static HRESULT WINAPI topology_node_DeleteAllItems(IMFTopologyNode *iface)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_DeleteAllItems(node->attributes);
}

static HRESULT WINAPI topology_node_SetUINT32(IMFTopologyNode *iface, REFGUID key, UINT32 value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %d.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetUINT32(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_SetUINT64(IMFTopologyNode *iface, REFGUID key, UINT64 value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), wine_dbgstr_longlong(value));

    return IMFAttributes_SetUINT64(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_SetDouble(IMFTopologyNode *iface, REFGUID key, double value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetDouble(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_SetGUID(IMFTopologyNode *iface, REFGUID key, REFGUID value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_guid(value));

    return IMFAttributes_SetGUID(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_SetString(IMFTopologyNode *iface, REFGUID key, const WCHAR *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_w(value));

    return IMFAttributes_SetString(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_SetBlob(IMFTopologyNode *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p, %d.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_SetBlob(node->attributes, key, buf, size);
}

static HRESULT WINAPI topology_node_SetUnknown(IMFTopologyNode *iface, REFGUID key, IUnknown *unknown)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), unknown);

    return IMFAttributes_SetUnknown(node->attributes, key, unknown);
}

static HRESULT WINAPI topology_node_LockStore(IMFTopologyNode *iface)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_LockStore(node->attributes);
}

static HRESULT WINAPI topology_node_UnlockStore(IMFTopologyNode *iface)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_UnlockStore(node->attributes);
}

static HRESULT WINAPI topology_node_GetCount(IMFTopologyNode *iface, UINT32 *count)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, count);

    return IMFAttributes_GetCount(node->attributes, count);
}

static HRESULT WINAPI topology_node_GetItemByIndex(IMFTopologyNode *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return IMFAttributes_GetItemByIndex(node->attributes, index, key, value);
}

static HRESULT WINAPI topology_node_CopyAllItems(IMFTopologyNode *iface, IMFAttributes *dest)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, dest);

    return IMFAttributes_CopyAllItems(node->attributes, dest);
}

static HRESULT topology_node_set_object(struct topology_node *node, IUnknown *object)
{
    static const GUID *iids[3] = { &IID_IPersist, &IID_IPersistStorage, &IID_IPersistPropertyBag };
    IPersist *persist = NULL;
    BOOL has_object_id;
    GUID object_id;
    unsigned int i;
    HRESULT hr;

    has_object_id = IMFAttributes_GetGUID(node->attributes, &MF_TOPONODE_TRANSFORM_OBJECTID, &object_id) == S_OK;

    if (object && !has_object_id)
    {
        for (i = 0; i < ARRAY_SIZE(iids); ++i)
        {
            persist = NULL;
            if (SUCCEEDED(hr = IUnknown_QueryInterface(object, iids[i], (void **)&persist)))
                break;
        }

        if (persist)
        {
            if (FAILED(hr = IPersist_GetClassID(persist, &object_id)))
            {
                IPersist_Release(persist);
                persist = NULL;
            }
        }
    }

    EnterCriticalSection(&node->cs);

    if (node->object)
        IUnknown_Release(node->object);
    node->object = object;
    if (node->object)
        IUnknown_AddRef(node->object);

    if (persist)
        IMFAttributes_SetGUID(node->attributes, &MF_TOPONODE_TRANSFORM_OBJECTID, &object_id);

    LeaveCriticalSection(&node->cs);

    if (persist)
        IPersist_Release(persist);

    return S_OK;
}

static HRESULT WINAPI topology_node_SetObject(IMFTopologyNode *iface, IUnknown *object)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, object);

    return topology_node_set_object(node, object);
}

static HRESULT WINAPI topology_node_GetObject(IMFTopologyNode *iface, IUnknown **object)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, object);

    if (!object)
        return E_POINTER;

    EnterCriticalSection(&node->cs);

    *object = node->object;
    if (*object)
        IUnknown_AddRef(*object);

    LeaveCriticalSection(&node->cs);

    return *object ? S_OK : E_FAIL;
}

static HRESULT WINAPI topology_node_GetNodeType(IMFTopologyNode *iface, MF_TOPOLOGY_TYPE *node_type)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, node_type);

    *node_type = node->node_type;

    return S_OK;
}

static HRESULT WINAPI topology_node_GetTopoNodeID(IMFTopologyNode *iface, TOPOID *id)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, id);

    *id = node->id;

    return S_OK;
}

static HRESULT WINAPI topology_node_SetTopoNodeID(IMFTopologyNode *iface, TOPOID id)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(id));

    node->id = id;

    return S_OK;
}

static HRESULT WINAPI topology_node_GetInputCount(IMFTopologyNode *iface, DWORD *count)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, count);

    *count = node->inputs.count;

    return S_OK;
}

static HRESULT WINAPI topology_node_GetOutputCount(IMFTopologyNode *iface, DWORD *count)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, count);

    *count = node->outputs.count;

    return S_OK;
}

static void topology_node_set_stream_type(struct node_stream *stream, IMFMediaType *mediatype)
{
    if (stream->preferred_type)
        IMFMediaType_Release(stream->preferred_type);
    stream->preferred_type = mediatype;
    if (stream->preferred_type)
        IMFMediaType_AddRef(stream->preferred_type);
}

static HRESULT topology_node_connect_output(struct topology_node *node, DWORD output_index,
        struct topology_node *connection, DWORD input_index)
{
    struct node_stream *stream;
    HRESULT hr;

    if (node->node_type == MF_TOPOLOGY_OUTPUT_NODE || connection->node_type == MF_TOPOLOGY_SOURCESTREAM_NODE)
         return E_FAIL;

    EnterCriticalSection(&node->cs);
    EnterCriticalSection(&connection->cs);

    topology_node_disconnect_output(node, output_index);
    if (input_index < connection->inputs.count)
    {
        stream = &connection->inputs.streams[input_index];
        if (stream->connection)
            topology_node_disconnect_output(stream->connection, stream->connection_stream);
    }

    hr = topology_node_reserve_streams(&node->outputs, output_index);
    if (SUCCEEDED(hr))
    {
        size_t old_count = connection->inputs.count;
        hr = topology_node_reserve_streams(&connection->inputs, input_index);
        if (SUCCEEDED(hr) && !old_count && connection->input_type)
        {
            topology_node_set_stream_type(connection->inputs.streams, connection->input_type);
            IMFMediaType_Release(connection->input_type);
            connection->input_type = NULL;
        }
    }

    if (SUCCEEDED(hr))
    {
        node->outputs.streams[output_index].connection = connection;
        IMFTopologyNode_AddRef(&node->outputs.streams[output_index].connection->IMFTopologyNode_iface);
        node->outputs.streams[output_index].connection_stream = input_index;
        connection->inputs.streams[input_index].connection = node;
        IMFTopologyNode_AddRef(&connection->inputs.streams[input_index].connection->IMFTopologyNode_iface);
        connection->inputs.streams[input_index].connection_stream = output_index;
    }

    LeaveCriticalSection(&connection->cs);
    LeaveCriticalSection(&node->cs);

    return hr;
}

static HRESULT WINAPI topology_node_ConnectOutput(IMFTopologyNode *iface, DWORD output_index,
        IMFTopologyNode *peer, DWORD input_index)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    struct topology_node *connection = unsafe_impl_from_IMFTopologyNode(peer);

    TRACE("%p, %lu, %p, %lu.\n", iface, output_index, peer, input_index);

    if (!connection)
    {
        WARN("External node implementations are not supported.\n");
        return E_UNEXPECTED;
    }

    return topology_node_connect_output(node, output_index, connection, input_index);
}

static HRESULT WINAPI topology_node_DisconnectOutput(IMFTopologyNode *iface, DWORD output_index)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %lu.\n", iface, output_index);

    return topology_node_disconnect_output(node, output_index);
}

static HRESULT WINAPI topology_node_GetInput(IMFTopologyNode *iface, DWORD input_index, IMFTopologyNode **ret,
        DWORD *output_index)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %p, %p.\n", iface, input_index, ret, output_index);

    EnterCriticalSection(&node->cs);

    if (input_index < node->inputs.count)
    {
        const struct node_stream *stream = &node->inputs.streams[input_index];

        if (stream->connection)
        {
            *ret = &stream->connection->IMFTopologyNode_iface;
            IMFTopologyNode_AddRef(*ret);
            *output_index = stream->connection_stream;
        }
        else
            hr = MF_E_NOT_FOUND;
    }
    else
        hr = E_INVALIDARG;

    LeaveCriticalSection(&node->cs);

    return hr;
}

static HRESULT WINAPI topology_node_GetOutput(IMFTopologyNode *iface, DWORD output_index, IMFTopologyNode **ret,
        DWORD *input_index)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %p, %p.\n", iface, output_index, ret, input_index);

    EnterCriticalSection(&node->cs);

    if (output_index < node->outputs.count)
    {
        const struct node_stream *stream = &node->outputs.streams[output_index];

        if (stream->connection)
        {
            *ret = &stream->connection->IMFTopologyNode_iface;
            IMFTopologyNode_AddRef(*ret);
            *input_index = stream->connection_stream;
        }
        else
            hr = MF_E_NOT_FOUND;
    }
    else
        hr = E_INVALIDARG;

    LeaveCriticalSection(&node->cs);

    return hr;
}

static HRESULT WINAPI topology_node_SetOutputPrefType(IMFTopologyNode *iface, DWORD index, IMFMediaType *mediatype)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %p.\n", iface, index, mediatype);

    EnterCriticalSection(&node->cs);

    if (node->node_type != MF_TOPOLOGY_OUTPUT_NODE)
    {
        if (SUCCEEDED(hr = topology_node_reserve_streams(&node->outputs, index)))
            topology_node_set_stream_type(&node->outputs.streams[index], mediatype);
    }
    else
        hr = E_NOTIMPL;

    LeaveCriticalSection(&node->cs);

    return hr;
}

static HRESULT topology_node_get_pref_type(struct node_streams *streams, unsigned int index, IMFMediaType **mediatype)
{
    *mediatype = streams->streams[index].preferred_type;
    if (*mediatype)
    {
        IMFMediaType_AddRef(*mediatype);
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI topology_node_GetOutputPrefType(IMFTopologyNode *iface, DWORD index, IMFMediaType **mediatype)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %p.\n", iface, index, mediatype);

    EnterCriticalSection(&node->cs);

    if (index < node->outputs.count)
        hr = topology_node_get_pref_type(&node->outputs, index, mediatype);
    else
        hr = E_INVALIDARG;

    LeaveCriticalSection(&node->cs);

    return hr;
}

static HRESULT WINAPI topology_node_SetInputPrefType(IMFTopologyNode *iface, DWORD index, IMFMediaType *mediatype)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %p.\n", iface, index, mediatype);

    EnterCriticalSection(&node->cs);

    switch (node->node_type)
    {
        case MF_TOPOLOGY_TEE_NODE:
            if (index)
            {
                hr = MF_E_INVALIDTYPE;
                break;
            }
            if (node->inputs.count)
                topology_node_set_stream_type(&node->inputs.streams[index], mediatype);
            else
            {
                if (node->input_type)
                    IMFMediaType_Release(node->input_type);
                node->input_type = mediatype;
                if (node->input_type)
                    IMFMediaType_AddRef(node->input_type);
            }
            break;
        case MF_TOPOLOGY_SOURCESTREAM_NODE:
            hr = E_NOTIMPL;
            break;
        default:
            if (SUCCEEDED(hr = topology_node_reserve_streams(&node->inputs, index)))
                topology_node_set_stream_type(&node->inputs.streams[index], mediatype);
    }

    LeaveCriticalSection(&node->cs);

    return hr;
}

static HRESULT WINAPI topology_node_GetInputPrefType(IMFTopologyNode *iface, DWORD index, IMFMediaType **mediatype)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %p.\n", iface, index, mediatype);

    EnterCriticalSection(&node->cs);

    if (index < node->inputs.count)
    {
        hr = topology_node_get_pref_type(&node->inputs, index, mediatype);
    }
    else if (node->node_type == MF_TOPOLOGY_TEE_NODE && node->input_type)
    {
        *mediatype = node->input_type;
        IMFMediaType_AddRef(*mediatype);
    }
    else
        hr = E_INVALIDARG;

    LeaveCriticalSection(&node->cs);

    return hr;
}

static HRESULT WINAPI topology_node_CloneFrom(IMFTopologyNode *iface, IMFTopologyNode *src_node)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    MF_TOPOLOGY_TYPE node_type;
    IMFMediaType *mediatype;
    IUnknown *object;
    DWORD count, i;
    TOPOID topoid;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, src_node);

    if (FAILED(hr = IMFTopologyNode_GetNodeType(src_node, &node_type)))
        return hr;

    if (node->node_type != node_type)
        return MF_E_INVALIDREQUEST;

    if (FAILED(hr = IMFTopologyNode_GetTopoNodeID(src_node, &topoid)))
        return hr;

    object = NULL;
    IMFTopologyNode_GetObject(src_node, &object);

    EnterCriticalSection(&node->cs);

    hr = IMFTopologyNode_CopyAllItems(src_node, node->attributes);

    if (SUCCEEDED(hr))
        hr = topology_node_set_object(node, object);

    if (SUCCEEDED(hr))
        node->id = topoid;

    if (SUCCEEDED(IMFTopologyNode_GetInputCount(src_node, &count)))
    {
        for (i = 0; i < count; ++i)
        {
            if (SUCCEEDED(IMFTopologyNode_GetInputPrefType(src_node, i, &mediatype)))
            {
                IMFTopologyNode_SetInputPrefType(iface, i, mediatype);
                IMFMediaType_Release(mediatype);
            }
        }
    }

    if (SUCCEEDED(IMFTopologyNode_GetOutputCount(src_node, &count)))
    {
        for (i = 0; i < count; ++i)
        {
            if (SUCCEEDED(IMFTopologyNode_GetOutputPrefType(src_node, i, &mediatype)))
            {
                IMFTopologyNode_SetOutputPrefType(iface, i, mediatype);
                IMFMediaType_Release(mediatype);
            }
        }
    }

    LeaveCriticalSection(&node->cs);

    if (object)
        IUnknown_Release(object);

    return hr;
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

static HRESULT create_topology_node(MF_TOPOLOGY_TYPE node_type, struct topology_node **node)
{
    HRESULT hr;

    if (!(*node = calloc(1, sizeof(**node))))
        return E_OUTOFMEMORY;

    (*node)->IMFTopologyNode_iface.lpVtbl = &topologynodevtbl;
    (*node)->refcount = 1;
    (*node)->node_type = node_type;
    hr = MFCreateAttributes(&(*node)->attributes, 0);
    if (FAILED(hr))
    {
        free(*node);
        return hr;
    }
    (*node)->id = ((TOPOID)GetCurrentProcessId() << 32) | InterlockedIncrement(&next_node_id);
    InitializeCriticalSection(&(*node)->cs);

    return S_OK;
}

HRESULT topology_node_get_object(IMFTopologyNode *node, REFIID riid, void **obj)
{
    IUnknown *unk;
    HRESULT hr;

    *obj = NULL;

    if (SUCCEEDED(hr = IMFTopologyNode_GetObject(node, &unk)))
    {
        hr = IUnknown_QueryInterface(unk, riid, obj);
        IUnknown_Release(unk);
    }

    return hr;
}

/***********************************************************************
 *      MFCreateTopologyNode (mf.@)
 */
HRESULT WINAPI MFCreateTopologyNode(MF_TOPOLOGY_TYPE node_type, IMFTopologyNode **node)
{
    struct topology_node *object;
    HRESULT hr;

    TRACE("%d, %p.\n", node_type, node);

    if (!node)
        return E_POINTER;

    hr = create_topology_node(node_type, &object);
    if (SUCCEEDED(hr))
        *node = &object->IMFTopologyNode_iface;

    return hr;
}

/* private helper for node types without an actual IMFMediaTypeHandler */
struct type_handler
{
    IMFMediaTypeHandler IMFMediaTypeHandler_iface;
    LONG refcount;

    IMFTopologyNode *node;
    DWORD stream;
    BOOL output;

    IMFTransform *transform;
};

static struct type_handler *impl_from_IMFMediaTypeHandler(IMFMediaTypeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct type_handler, IMFMediaTypeHandler_iface);
}

static HRESULT WINAPI type_handler_QueryInterface(IMFMediaTypeHandler *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFMediaTypeHandler)
            || IsEqualIID(riid, &IID_IUnknown))
    {
        IMFMediaTypeHandler_AddRef((*obj = iface));
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI type_handler_AddRef(IMFMediaTypeHandler *iface)
{
    struct type_handler *handler = impl_from_IMFMediaTypeHandler(iface);
    ULONG refcount = InterlockedIncrement(&handler->refcount);
    return refcount;
}

static ULONG WINAPI type_handler_Release(IMFMediaTypeHandler *iface)
{
    struct type_handler *handler = impl_from_IMFMediaTypeHandler(iface);
    ULONG refcount = InterlockedDecrement(&handler->refcount);

    if (!refcount)
    {
        if (handler->transform)
            IMFTransform_Release(handler->transform);
        IMFTopologyNode_Release(handler->node);
        free(handler);
    }

    return refcount;
}

static HRESULT WINAPI type_handler_IsMediaTypeSupported(IMFMediaTypeHandler *iface, IMFMediaType *in_type,
        IMFMediaType **out_type)
{
    struct type_handler *handler = impl_from_IMFMediaTypeHandler(iface);
    IMFMediaType *type;
    DWORD flags;
    HRESULT hr;

    if (out_type)
        *out_type = NULL;

    if (handler->transform)
    {
        if (handler->output)
            return IMFTransform_SetOutputType(handler->transform, handler->stream, in_type, MFT_SET_TYPE_TEST_ONLY);
        else
            return IMFTransform_SetInputType(handler->transform, handler->stream, in_type, MFT_SET_TYPE_TEST_ONLY);
    }

    if (FAILED(hr = IMFMediaTypeHandler_GetCurrentMediaType(iface, &type)))
        return hr;

    hr = IMFMediaType_IsEqual(type, in_type, &flags);
    IMFMediaType_Release(type);
    return hr;
}

static HRESULT WINAPI type_handler_GetMediaTypeCount(IMFMediaTypeHandler *iface, DWORD *count)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI type_handler_GetMediaTypeByIndex(IMFMediaTypeHandler *iface, DWORD index,
        IMFMediaType **type)
{
    struct type_handler *handler = impl_from_IMFMediaTypeHandler(iface);

    if (handler->transform)
    {
        if (handler->output)
            return IMFTransform_GetOutputAvailableType(handler->transform, handler->stream, index, type);
        else
            return IMFTransform_GetInputAvailableType(handler->transform, handler->stream, index, type);
    }

    if (index)
        return MF_E_NO_MORE_TYPES;

    return IMFMediaTypeHandler_GetCurrentMediaType(iface, type);
}

static HRESULT WINAPI type_handler_SetCurrentMediaType(IMFMediaTypeHandler *iface, IMFMediaType *type)
{
    struct type_handler *handler = impl_from_IMFMediaTypeHandler(iface);

    if (handler->transform)
    {
        if (handler->output)
            return IMFTransform_SetOutputType(handler->transform, handler->stream, type, 0);
        else
            return IMFTransform_SetInputType(handler->transform, handler->stream, type, 0);
    }

    return IMFTopologyNode_SetInputPrefType(handler->node, handler->stream, type);
}

static HRESULT WINAPI type_handler_GetCurrentMediaType(IMFMediaTypeHandler *iface, IMFMediaType **type)
{
    struct type_handler *handler = impl_from_IMFMediaTypeHandler(iface);
    UINT32 output;

    if (handler->transform)
    {
        if (handler->output)
            return IMFTransform_GetOutputCurrentType(handler->transform, handler->stream, type);
        else
            return IMFTransform_GetInputCurrentType(handler->transform, handler->stream, type);
    }

    if (SUCCEEDED(IMFTopologyNode_GetInputPrefType(handler->node, 0, type)))
        return S_OK;

    if (FAILED(IMFTopologyNode_GetUINT32(handler->node, &MF_TOPONODE_PRIMARYOUTPUT, &output)))
        output = 0;

    return IMFTopologyNode_GetOutputPrefType(handler->node, output, type);
}

static HRESULT WINAPI type_handler_GetMajorType(IMFMediaTypeHandler *iface, GUID *type)
{
    IMFMediaType *media_type;
    HRESULT hr;

    if (FAILED(hr = IMFMediaTypeHandler_GetCurrentMediaType(iface, &media_type)))
        return hr;

    hr = IMFMediaType_GetMajorType(media_type, type);
    IMFMediaType_Release(media_type);
    return hr;
}

static const IMFMediaTypeHandlerVtbl type_handler_vtbl =
{
    type_handler_QueryInterface,
    type_handler_AddRef,
    type_handler_Release,
    type_handler_IsMediaTypeSupported,
    type_handler_GetMediaTypeCount,
    type_handler_GetMediaTypeByIndex,
    type_handler_SetCurrentMediaType,
    type_handler_GetCurrentMediaType,
    type_handler_GetMajorType,
};

static HRESULT type_handler_create(IMFTopologyNode *node, DWORD stream, BOOL output, IMFTransform *transform, IMFMediaTypeHandler **out)
{
    struct type_handler *handler;

    if (!(handler = calloc(1, sizeof(*handler)))) return E_OUTOFMEMORY;
    handler->IMFMediaTypeHandler_iface.lpVtbl = &type_handler_vtbl;
    handler->refcount = 1;
    handler->stream = stream;
    handler->output = output;
    IMFTopologyNode_AddRef((handler->node = node));
    if (transform)
        IMFTransform_AddRef((handler->transform = transform));

    *out = &handler->IMFMediaTypeHandler_iface;
    return S_OK;
}

HRESULT topology_node_get_type_handler(IMFTopologyNode *node, DWORD stream,
        BOOL output, IMFMediaTypeHandler **handler)
{
    MF_TOPOLOGY_TYPE node_type;
    IMFStreamSink *stream_sink;
    IMFStreamDescriptor *sd;
    IMFTransform *transform;
    HRESULT hr;

    if (FAILED(hr = IMFTopologyNode_GetNodeType(node, &node_type)))
        return hr;

    switch (node_type)
    {
        case MF_TOPOLOGY_OUTPUT_NODE:
            if (output || stream)
                return MF_E_INVALIDSTREAMNUMBER;

            if (SUCCEEDED(hr = topology_node_get_object(node, &IID_IMFStreamSink, (void **)&stream_sink)))
            {
                hr = IMFStreamSink_GetMediaTypeHandler(stream_sink, handler);
                IMFStreamSink_Release(stream_sink);
            }
            break;
        case MF_TOPOLOGY_SOURCESTREAM_NODE:
            if (!output || stream)
                return MF_E_INVALIDSTREAMNUMBER;

            if (SUCCEEDED(hr = IMFTopologyNode_GetUnknown(node, &MF_TOPONODE_STREAM_DESCRIPTOR,
                    &IID_IMFStreamDescriptor, (void **)&sd)))
            {
                hr = IMFStreamDescriptor_GetMediaTypeHandler(sd, handler);
                IMFStreamDescriptor_Release(sd);
            }
            break;
        case MF_TOPOLOGY_TRANSFORM_NODE:
            if (SUCCEEDED(hr = topology_node_get_object(node, &IID_IMFTransform, (void **)&transform)))
            {
                hr = type_handler_create(node, stream, output, transform, handler);
                IMFTransform_Release(transform);
            }
            break;
        case MF_TOPOLOGY_TEE_NODE:
            hr = type_handler_create(node, stream, output, NULL, handler);
            break;
        default:
            WARN("Unexpected node type %u.\n", node_type);
            return MF_E_UNEXPECTED;
    }

    return hr;
}

/***********************************************************************
 *      MFGetTopoNodeCurrentType (mf.@)
 */
HRESULT WINAPI MFGetTopoNodeCurrentType(IMFTopologyNode *node, DWORD stream, BOOL output, IMFMediaType **type)
{
    IMFMediaTypeHandler *handler;
    HRESULT hr;

    TRACE("%p, %lu, %d, %p.\n", node, stream, output, type);

    if (FAILED(hr = topology_node_get_type_handler(node, stream, output, &handler)))
        return hr;

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, type);
    IMFMediaTypeHandler_Release(handler);
    return hr;
}

static HRESULT WINAPI seq_source_QueryInterface(IMFSequencerSource *iface, REFIID riid, void **out)
{
    struct seq_source *seq_source = impl_from_IMFSequencerSource(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    *out = NULL;

    if (IsEqualIID(riid, &IID_IMFSequencerSource) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &seq_source->IMFSequencerSource_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaSourceTopologyProvider))
    {
        *out = &seq_source->IMFMediaSourceTopologyProvider_iface;
    }
    else
    {
        WARN("Unimplemented %s.\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    if (*out)
        IUnknown_AddRef((IUnknown *)*out);

    return S_OK;
}

static ULONG WINAPI seq_source_AddRef(IMFSequencerSource *iface)
{
    struct seq_source *seq_source = impl_from_IMFSequencerSource(iface);
    ULONG refcount = InterlockedIncrement(&seq_source->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI seq_source_Release(IMFSequencerSource *iface)
{
    struct seq_source *seq_source = impl_from_IMFSequencerSource(iface);
    ULONG refcount = InterlockedDecrement(&seq_source->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
        free(seq_source);

    return refcount;
}

static HRESULT WINAPI seq_source_AppendTopology(IMFSequencerSource *iface, IMFTopology *topology,
        DWORD flags, MFSequencerElementId *id)
{
    FIXME("%p, %p, %lx, %p.\n", iface, topology, flags, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI seq_source_DeleteTopology(IMFSequencerSource *iface, MFSequencerElementId id)
{
    FIXME("%p, %#lx.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI seq_source_GetPresentationContext(IMFSequencerSource *iface,
        IMFPresentationDescriptor *descriptor, MFSequencerElementId *id, IMFTopology **topology)
{
    FIXME("%p, %p, %p, %p.\n", iface, descriptor, id, topology);

    return E_NOTIMPL;
}

static HRESULT WINAPI seq_source_UpdateTopology(IMFSequencerSource *iface, MFSequencerElementId id,
        IMFTopology *topology)
{
    FIXME("%p, %#lx, %p.\n", iface, id, topology);

    return E_NOTIMPL;
}

static HRESULT WINAPI seq_source_UpdateTopologyFlags(IMFSequencerSource *iface, MFSequencerElementId id, DWORD flags)
{
    FIXME("%p, %#lx, %#lx.\n", iface, id, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI seq_source_topology_provider_QueryInterface(IMFMediaSourceTopologyProvider *iface, REFIID riid,
        void **obj)
{
    struct seq_source *seq_source = impl_from_IMFMediaSourceTopologyProvider(iface);
    return IMFSequencerSource_QueryInterface(&seq_source->IMFSequencerSource_iface, riid, obj);
}

static ULONG WINAPI seq_source_topology_provider_AddRef(IMFMediaSourceTopologyProvider *iface)
{
    struct seq_source *seq_source = impl_from_IMFMediaSourceTopologyProvider(iface);
    return IMFSequencerSource_AddRef(&seq_source->IMFSequencerSource_iface);
}

static ULONG WINAPI seq_source_topology_provider_Release(IMFMediaSourceTopologyProvider *iface)
{
    struct seq_source *seq_source = impl_from_IMFMediaSourceTopologyProvider(iface);
    return IMFSequencerSource_Release(&seq_source->IMFSequencerSource_iface);
}

static HRESULT WINAPI seq_source_topology_provider_GetMediaSourceTopology(IMFMediaSourceTopologyProvider *iface,
        IMFPresentationDescriptor *pd, IMFTopology **topology)
{
    FIXME("%p, %p, %p.\n", iface, pd, topology);

    return E_NOTIMPL;
}

static const IMFMediaSourceTopologyProviderVtbl seq_source_topology_provider_vtbl =
{
    seq_source_topology_provider_QueryInterface,
    seq_source_topology_provider_AddRef,
    seq_source_topology_provider_Release,
    seq_source_topology_provider_GetMediaSourceTopology,
};

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

    TRACE("%p, %p.\n", reserved, seq_source);

    if (!seq_source)
        return E_POINTER;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFSequencerSource_iface.lpVtbl = &seqsourcevtbl;
    object->IMFMediaSourceTopologyProvider_iface.lpVtbl = &seq_source_topology_provider_vtbl;
    object->refcount = 1;

    *seq_source = &object->IMFSequencerSource_iface;

    return S_OK;
}

struct segment_offset
{
    IUnknown IUnknown_iface;
    LONG refcount;
    MFSequencerElementId id;
    MFTIME timeoffset;
};

static inline struct segment_offset *impl_offset_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct segment_offset, IUnknown_iface);
}

static HRESULT WINAPI segment_offset_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI segment_offset_AddRef(IUnknown *iface)
{
    struct segment_offset *offset = impl_offset_from_IUnknown(iface);
    return InterlockedIncrement(&offset->refcount);
}

static ULONG WINAPI segment_offset_Release(IUnknown *iface)
{
    struct segment_offset *offset = impl_offset_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&offset->refcount);

    if (!refcount)
        free(offset);

    return refcount;
}

static const IUnknownVtbl segment_offset_vtbl =
{
    segment_offset_QueryInterface,
    segment_offset_AddRef,
    segment_offset_Release,
};

/***********************************************************************
 *      MFCreateSequencerSegmentOffset (mf.@)
 */
HRESULT WINAPI MFCreateSequencerSegmentOffset(MFSequencerElementId id, MFTIME timeoffset, PROPVARIANT *ret)
{
    struct segment_offset *offset;

    TRACE("%#lx, %s, %p.\n", id, debugstr_time(timeoffset), ret);

    if (!ret)
        return E_POINTER;

    if (!(offset = calloc(1, sizeof(*offset))))
        return E_OUTOFMEMORY;

    offset->IUnknown_iface.lpVtbl = &segment_offset_vtbl;
    offset->refcount = 1;
    offset->id = id;
    offset->timeoffset = timeoffset;

    V_VT(ret) = VT_UNKNOWN;
    V_UNKNOWN(ret) = &offset->IUnknown_iface;

    return S_OK;
}
