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
#include <stddef.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"

#include "mfidl.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "mf_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct topology_loader
{
    IMFTopoLoader IMFTopoLoader_iface;
    LONG refcount;
};

static inline struct topology_loader *impl_from_IMFTopoLoader(IMFTopoLoader *iface)
{
    return CONTAINING_RECORD(iface, struct topology_loader, IMFTopoLoader_iface);
}

static HRESULT WINAPI topology_loader_QueryInterface(IMFTopoLoader *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFTopoLoader)
            || IsEqualIID(riid, &IID_IUnknown))
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
    TRACE("iface %p, refcount %lu.\n", iface, refcount);
    return refcount;
}

static ULONG WINAPI topology_loader_Release(IMFTopoLoader *iface)
{
    struct topology_loader *loader = impl_from_IMFTopoLoader(iface);
    ULONG refcount = InterlockedDecrement(&loader->refcount);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    if (!refcount)
        free(loader);

    return refcount;
}

struct topoloader_context
{
    IMFTopology *input_topology;
    IMFTopology *output_topology;
};

static HRESULT topology_loader_clone_node(struct topoloader_context *context, IMFTopologyNode *node, IMFTopologyNode **clone)
{
    MF_TOPOLOGY_TYPE node_type;
    HRESULT hr;
    TOPOID id;

    if (FAILED(hr = IMFTopologyNode_GetTopoNodeID(node, &id)))
        return hr;
    if (SUCCEEDED(hr = IMFTopology_GetNodeByID(context->output_topology, id, clone)))
        return hr;

    IMFTopologyNode_GetNodeType(node, &node_type);
    if (FAILED(hr = MFCreateTopologyNode(node_type, clone)))
        return hr;

    hr = IMFTopologyNode_CloneFrom(*clone, node);
    if (SUCCEEDED(hr))
        hr = IMFTopology_AddNode(context->output_topology, *clone);

    if (FAILED(hr))
    {
        IMFTopologyNode_Release(*clone);
        *clone = NULL;
    }
    return hr;
}

struct transform_output_type
{
    IMFMediaType *type;
    IMFTransform *transform;
    IMFActivate *activate;
};

struct connect_context
{
    struct topoloader_context *context;
    IMFTopologyNode *upstream_node;
    IMFTopologyNode *sink;
    IMFMediaTypeHandler *sink_handler;
    unsigned int output_index;
    unsigned int input_index;
    GUID converter_category;
    GUID decoder_category;
};

struct topology_branch
{
    struct
    {
        IMFTopologyNode *node;
        DWORD stream;
    } up, down;

    struct list entry;
};

static const char *debugstr_topology_branch(struct topology_branch *branch)
{
    return wine_dbg_sprintf("%p:%lu to %p:%lu", branch->up.node, branch->up.stream, branch->down.node, branch->down.stream);
}

static HRESULT topology_branch_create(IMFTopologyNode *source, DWORD source_stream,
            IMFTopologyNode *sink, DWORD sink_stream, struct topology_branch **out)
{
    struct topology_branch *branch;

    if (!(branch = calloc(1, sizeof(*branch))))
        return E_OUTOFMEMORY;

    IMFTopologyNode_AddRef((branch->up.node = source));
    branch->up.stream = source_stream;
    IMFTopologyNode_AddRef((branch->down.node = sink));
    branch->down.stream = sink_stream;

    *out = branch;
    return S_OK;
}

static void topology_branch_destroy(struct topology_branch *branch)
{
    IMFTopologyNode_Release(branch->up.node);
    IMFTopologyNode_Release(branch->down.node);
    free(branch);
}

static HRESULT topology_branch_clone_nodes(struct topoloader_context *context, struct topology_branch *branch)
{
    IMFTopologyNode *up, *down;
    HRESULT hr;

    if (FAILED(hr = topology_loader_clone_node(context, branch->up.node, &up)))
        return hr;
    if (FAILED(hr = topology_loader_clone_node(context, branch->down.node, &down)))
    {
        IMFTopologyNode_Release(up);
        return hr;
    }

    IMFTopologyNode_Release(branch->up.node);
    IMFTopologyNode_Release(branch->down.node);
    branch->up.node = up;
    branch->down.node = down;
    return hr;
}

static HRESULT topology_node_list_branches(IMFTopologyNode *node, struct list *branches)
{
    struct topology_branch *branch;
    DWORD i, count, down_stream;
    IMFTopologyNode *down_node;
    HRESULT hr;

    hr = IMFTopologyNode_GetOutputCount(node, &count);
    for (i = 0; SUCCEEDED(hr) && i < count; ++i)
    {
        if (FAILED(IMFTopologyNode_GetOutput(node, i, &down_node, &down_stream)))
            continue;

        if (SUCCEEDED(hr = topology_branch_create(node, i, down_node, down_stream, &branch)))
            list_add_tail(branches, &branch->entry);

        IMFTopologyNode_Release(down_node);
    }

    return hr;
}

static HRESULT topology_branch_fill_media_type(IMFMediaType *up_type, IMFMediaType *down_type)
{
    HRESULT hr = S_OK;
    PROPVARIANT value;
    UINT32 count;
    GUID key;

    if (FAILED(hr = IMFMediaType_GetCount(up_type, &count)))
        return hr;

    while (count--)
    {
        PropVariantInit(&value);
        hr = IMFMediaType_GetItemByIndex(up_type, count, &key, &value);
        if (SUCCEEDED(hr) && FAILED(IMFMediaType_GetItem(down_type, &key, NULL)))
            hr = IMFMediaType_SetItem(down_type, &key, &value);
        PropVariantClear(&value);
        if (FAILED(hr))
            return hr;
    }

    return hr;
}

static HRESULT topology_branch_connect(IMFTopology *topology, MF_CONNECT_METHOD method_mask,
        struct topology_branch *branch, BOOL enumerate_source_types);
static HRESULT topology_branch_connect_down(IMFTopology *topology, MF_CONNECT_METHOD method_mask,
        struct topology_branch *branch, IMFMediaType *up_type);
static HRESULT topology_branch_connect_indirect(IMFTopology *topology, MF_CONNECT_METHOD method_mask,
        struct topology_branch *branch, IMFMediaType *up_type, IMFMediaType *down_type)
{
    BOOL decoder = (method_mask & MF_CONNECT_ALLOW_DECODER) == MF_CONNECT_ALLOW_DECODER;
    MFT_REGISTER_TYPE_INFO input_info, output_info;
    IMFTransform *transform;
    IMFActivate **activates;
    IMFTopologyNode *node;
    unsigned int i, count;
    GUID category, guid;
    HRESULT hr;

    TRACE("topology %p, method_mask %#x, branch %s, up_type %p, down_type %p.\n",
            topology, method_mask, debugstr_topology_branch(branch), up_type, down_type);

    if (FAILED(hr = IMFMediaType_GetMajorType(up_type, &input_info.guidMajorType)))
        return hr;
    if (FAILED(hr = IMFMediaType_GetGUID(up_type, &MF_MT_SUBTYPE, &input_info.guidSubtype)))
        return hr;
    if (!down_type)
        output_info = input_info;
    else
    {
        if (FAILED(hr = IMFMediaType_GetMajorType(down_type, &output_info.guidMajorType)))
            return hr;
        if (FAILED(hr = IMFMediaType_GetGUID(down_type, &MF_MT_SUBTYPE, &output_info.guidSubtype)))
            return hr;
    }

    if (IsEqualGUID(&input_info.guidMajorType, &MFMediaType_Audio))
        category = decoder ? MFT_CATEGORY_AUDIO_DECODER : MFT_CATEGORY_AUDIO_EFFECT;
    else if (IsEqualGUID(&input_info.guidMajorType, &MFMediaType_Video))
        category = decoder ? MFT_CATEGORY_VIDEO_DECODER : MFT_CATEGORY_VIDEO_PROCESSOR;
    else
        return MF_E_INVALIDMEDIATYPE;

    if (FAILED(hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &node)))
        return hr;
    if (!decoder)
        method_mask = MF_CONNECT_DIRECT;
    else
    {
        IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_DECODER, 1);
        method_mask = MF_CONNECT_ALLOW_CONVERTER;
    }

    if (FAILED(hr = MFTEnumEx(category, MFT_ENUM_FLAG_ALL, &input_info, decoder ? NULL : &output_info, &activates, &count)))
        return hr;

    for (i = 0, hr = MF_E_TRANSFORM_NOT_POSSIBLE_FOR_CURRENT_MEDIATYPE_COMBINATION; i < count; ++i)
    {
        struct topology_branch down_branch = {.up.node = node, .down = branch->down};
        struct topology_branch up_branch = {.up = branch->up, .down.node = node};
        MF_CONNECT_METHOD method = method_mask;
        IMFMediaType *media_type;

        if (FAILED(IMFActivate_ActivateObject(activates[i], &IID_IMFTransform, (void **)&transform)))
            continue;

        IMFTopologyNode_SetObject(node, (IUnknown *)transform);
        IMFTopologyNode_DeleteItem(node, &MF_TOPONODE_TRANSFORM_OBJECTID);
        if (SUCCEEDED(IMFActivate_GetGUID(activates[i], &MFT_TRANSFORM_CLSID_Attribute, &guid)))
            IMFTopologyNode_SetGUID(node, &MF_TOPONODE_TRANSFORM_OBJECTID, &guid);

        hr = topology_branch_connect_down(topology, MF_CONNECT_DIRECT, &up_branch, up_type);
        if (down_type)
        {
            if (SUCCEEDED(topology_branch_fill_media_type(up_type, down_type))
                    && SUCCEEDED(IMFTransform_SetOutputType(transform, 0, down_type, 0)))
                method = MF_CONNECT_DIRECT;
        }
        IMFTransform_Release(transform);

        if (SUCCEEDED(hr) && method != MF_CONNECT_DIRECT
                && SUCCEEDED(IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type)))
        {
            if (SUCCEEDED(topology_branch_fill_media_type(up_type, media_type)))
                IMFTransform_SetOutputType(transform, 0, media_type, 0);
            IMFMediaType_Release(media_type);
        }

        if (SUCCEEDED(hr))
            hr = topology_branch_connect(topology, method, &down_branch, !down_type);
        if (SUCCEEDED(hr))
            hr = IMFTopology_AddNode(topology, node);
        if (SUCCEEDED(hr))
            break;
    }

    IMFTopologyNode_Release(node);
    for (i = 0; i < count; ++i)
        IMFActivate_Release(activates[i]);
    CoTaskMemFree(activates);

    if (!count)
        return MF_E_TOPO_CODEC_NOT_FOUND;
    return hr;
}

static HRESULT get_first_supported_media_type(IMFMediaTypeHandler *handler, IMFMediaType **type)
{
    IMFMediaType *media_type;
    HRESULT hr;
    DWORD i;

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, type);
    if (hr != MF_E_NOT_INITIALIZED)
        return hr;

    for (i = 0; SUCCEEDED(hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, i, &media_type)); i++)
    {
        if (SUCCEEDED(hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, NULL)))
        {
            *type = media_type;
            return hr;
        }

        IMFMediaType_Release(media_type);
    }

    return hr;
}

HRESULT topology_node_init_media_type(IMFTopologyNode *node, DWORD stream, BOOL output, IMFMediaType **type)
{
    IMFMediaTypeHandler *handler;
    HRESULT hr;

    if (SUCCEEDED(hr = topology_node_get_type_handler(node, stream, output, &handler)))
    {
        if (SUCCEEDED(hr = get_first_supported_media_type(handler, type)))
            hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, *type);
        IMFMediaTypeHandler_Release(handler);
    }

    return hr;
}

static HRESULT topology_branch_connect_down(IMFTopology *topology, MF_CONNECT_METHOD method_mask,
        struct topology_branch *branch, IMFMediaType *up_type)
{
    IMFMediaTypeHandler *down_handler;
    IMFMediaType *down_type = NULL;
    MF_TOPOLOGY_TYPE type;
    UINT32 method;
    DWORD flags;
    HRESULT hr;

    TRACE("topology %p, method_mask %#x, branch %s, up_type %p.\n",
            topology, method_mask, debugstr_topology_branch(branch), up_type);

    if (FAILED(IMFTopologyNode_GetUINT32(branch->down.node, &MF_TOPONODE_CONNECT_METHOD, &method)))
        method = MF_CONNECT_ALLOW_DECODER;

    if (FAILED(hr = topology_node_get_type_handler(branch->down.node, branch->down.stream, FALSE, &down_handler)))
        return hr;

    if (SUCCEEDED(hr = get_first_supported_media_type(down_handler, &down_type))
            && IMFMediaType_IsEqual(up_type, down_type, &flags) == S_OK)
    {
        TRACE("Connecting branch %s with current type %p.\n", debugstr_topology_branch(branch), up_type);
        hr = IMFTopologyNode_ConnectOutput(branch->up.node, branch->up.stream, branch->down.node, branch->down.stream);
        goto done;
    }

    if (SUCCEEDED(hr = IMFMediaTypeHandler_IsMediaTypeSupported(down_handler, up_type, NULL)))
    {
        TRACE("Connected branch %s with upstream type %p.\n", debugstr_topology_branch(branch), up_type);

        if (SUCCEEDED(IMFTopologyNode_GetNodeType(branch->down.node, &type)) && type == MF_TOPOLOGY_TRANSFORM_NODE
                && FAILED(hr = IMFMediaTypeHandler_SetCurrentMediaType(down_handler, up_type)))
            WARN("Failed to set transform node media type, hr %#lx\n", hr);

        hr = IMFTopologyNode_ConnectOutput(branch->up.node, branch->up.stream, branch->down.node, branch->down.stream);

        goto done;
    }

    if (FAILED(hr) && (method & method_mask & MF_CONNECT_ALLOW_CONVERTER) == MF_CONNECT_ALLOW_CONVERTER)
        hr = topology_branch_connect_indirect(topology, MF_CONNECT_ALLOW_CONVERTER,
                branch, up_type, down_type);

    if (FAILED(hr) && (method & method_mask & MF_CONNECT_ALLOW_DECODER) == MF_CONNECT_ALLOW_DECODER)
        hr = topology_branch_connect_indirect(topology, MF_CONNECT_ALLOW_DECODER,
                branch, up_type, down_type);

done:
    if (down_type)
        IMFMediaType_Release(down_type);
    IMFMediaTypeHandler_Release(down_handler);

    return hr;
}

static HRESULT topology_branch_foreach_up_types(IMFTopology *topology, MF_CONNECT_METHOD method_mask,
        struct topology_branch *branch)
{
    IMFMediaTypeHandler *handler;
    IMFMediaType *type;
    DWORD index = 0;
    HRESULT hr;

    if (FAILED(hr = topology_node_get_type_handler(branch->up.node, branch->up.stream, TRUE, &handler)))
        return hr;

    while (SUCCEEDED(hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, index++, &type)))
    {
        hr = topology_branch_connect_down(topology, method_mask, branch, type);
        if (SUCCEEDED(hr))
            hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, type);
        IMFMediaType_Release(type);
        if (SUCCEEDED(hr))
            break;
    }

    IMFMediaTypeHandler_Release(handler);
    return hr;
}

static HRESULT topology_branch_connect(IMFTopology *topology, MF_CONNECT_METHOD method_mask,
        struct topology_branch *branch, BOOL enumerate_source_types)
{
    UINT32 method;
    HRESULT hr;

    TRACE("topology %p, method_mask %#x, branch %s.\n", topology, method_mask, debugstr_topology_branch(branch));

    if (FAILED(IMFTopologyNode_GetUINT32(branch->up.node, &MF_TOPONODE_CONNECT_METHOD, &method)))
        method = MF_CONNECT_DIRECT;

    if (enumerate_source_types)
    {
        if (method & MF_CONNECT_RESOLVE_INDEPENDENT_OUTPUTTYPES)
            hr = topology_branch_foreach_up_types(topology, method_mask & MF_CONNECT_ALLOW_DECODER, branch);
        else
        {
            hr = topology_branch_foreach_up_types(topology, method_mask & MF_CONNECT_DIRECT, branch);
            if (FAILED(hr))
                hr = topology_branch_foreach_up_types(topology, method_mask & MF_CONNECT_ALLOW_CONVERTER, branch);
            if (FAILED(hr))
                hr = topology_branch_foreach_up_types(topology, method_mask & MF_CONNECT_ALLOW_DECODER, branch);
        }
    }
    else
    {
        IMFMediaTypeHandler *up_handler;
        IMFMediaType *up_type;

        if (FAILED(hr = topology_node_get_type_handler(branch->up.node, branch->up.stream, TRUE, &up_handler)))
            return hr;
        if (SUCCEEDED(hr = IMFMediaTypeHandler_GetCurrentMediaType(up_handler, &up_type))
                || SUCCEEDED(hr = IMFMediaTypeHandler_GetMediaTypeByIndex(up_handler, 0, &up_type)))
        {
            hr = topology_branch_connect_down(topology, method_mask, branch, up_type);
            IMFMediaType_Release(up_type);
        }
        IMFMediaTypeHandler_Release(up_handler);
    }

    TRACE("returning %#lx\n", hr);
    return hr;
}

static HRESULT topology_loader_resolve_branches(struct topoloader_context *context, struct list *branches,
        BOOL enumerate_source_types)
{
    struct list new_branches = LIST_INIT(new_branches);
    struct topology_branch *branch, *next;
    MF_TOPOLOGY_TYPE node_type;
    HRESULT hr = S_OK;

    LIST_FOR_EACH_ENTRY_SAFE(branch, next, branches, struct topology_branch, entry)
    {
        list_remove(&branch->entry);

        if (FAILED(hr = topology_node_list_branches(branch->down.node, &new_branches)))
            WARN("Failed to list branches from branch %s\n", debugstr_topology_branch(branch));
        else if (FAILED(hr = IMFTopologyNode_GetNodeType(branch->up.node, &node_type)))
            WARN("Failed to get source node type for branch %s\n", debugstr_topology_branch(branch));
        else if (FAILED(hr = topology_branch_clone_nodes(context, branch)))
            WARN("Failed to clone nodes for branch %s\n", debugstr_topology_branch(branch));
        else
        {
            hr = topology_branch_connect(context->output_topology, MF_CONNECT_ALLOW_DECODER, branch, enumerate_source_types);
            if (hr == MF_E_INVALIDMEDIATYPE && !enumerate_source_types && node_type == MF_TOPOLOGY_TRANSFORM_NODE)
                hr = topology_branch_connect(context->output_topology, MF_CONNECT_ALLOW_DECODER, branch, TRUE);
        }

        topology_branch_destroy(branch);
        if (FAILED(hr))
            break;
    }

    list_move_tail(branches, &new_branches);
    return hr;
}

static BOOL topology_loader_is_node_d3d_aware(IMFTopologyNode *node)
{
    IMFAttributes *attributes;
    unsigned int d3d_aware = 0;
    IMFTransform *transform;

    if (FAILED(topology_node_get_object(node, &IID_IMFAttributes, (void **)&attributes)))
        return FALSE;

    IMFAttributes_GetUINT32(attributes, &MF_SA_D3D_AWARE, &d3d_aware);
    IMFAttributes_Release(attributes);

    if (!d3d_aware && SUCCEEDED(topology_node_get_object(node, &IID_IMFTransform, (void **)&transform)))
    {
        d3d_aware = mf_is_sample_copier_transform(transform);
        IMFTransform_Release(transform);
    }

    return !!d3d_aware;
}

static HRESULT topology_loader_create_copier(IMFTopologyNode *upstream_node, DWORD upstream_output,
        IMFTopologyNode *downstream_node, unsigned int downstream_input, IMFTransform **copier)
{
    IMFMediaType *up_type = NULL;
    IMFTransform *transform;
    HRESULT hr;

    if (FAILED(hr = MFCreateSampleCopierMFT(&transform)))
        return hr;

    if (FAILED(hr = MFGetTopoNodeCurrentType(upstream_node, upstream_output, TRUE, &up_type)))
        WARN("Failed to get upstream media type hr %#lx.\n", hr);

    if (SUCCEEDED(hr) && FAILED(hr = IMFTransform_SetInputType(transform, 0, up_type, 0)))
        WARN("Input type wasn't accepted, hr %#lx.\n", hr);

    /* We assume, that up_type is set to a value compatible with the down node by the branch resolver. */
    if (SUCCEEDED(hr) && FAILED(hr = IMFTransform_SetOutputType(transform, 0, up_type, 0)))
        WARN("Output type wasn't accepted, hr %#lx.\n", hr);

    if (SUCCEEDED(hr))
    {
        *copier = transform;
        IMFTransform_AddRef(*copier);
    }

    if (up_type)
        IMFMediaType_Release(up_type);

    IMFTransform_Release(transform);

    return hr;
}

static HRESULT topology_loader_connect_copier(struct topoloader_context *context, IMFTopologyNode *upstream_node,
        DWORD upstream_output, IMFTopologyNode *downstream_node, DWORD downstream_input, IMFTransform *copier)
{
    IMFTopologyNode *copier_node;
    HRESULT hr;

    if (FAILED(hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &copier_node)))
        return hr;

    IMFTopologyNode_SetObject(copier_node, (IUnknown *)copier);
    IMFTopology_AddNode(context->output_topology, copier_node);
    IMFTopologyNode_ConnectOutput(upstream_node, upstream_output, copier_node, 0);
    IMFTopologyNode_ConnectOutput(copier_node, 0, downstream_node, downstream_input);

    IMFTopologyNode_Release(copier_node);

    return S_OK;
}

/* Right now this should be used for output nodes only. */
static HRESULT topology_loader_connect_d3d_aware_input(struct topoloader_context *context,
        IMFTopologyNode *node)
{
    IMFTopologyNode *upstream_node;
    IMFTransform *copier = NULL;
    IMFStreamSink *stream_sink;
    DWORD upstream_output;
    HRESULT hr;

    if (FAILED(hr = topology_node_get_object(node, &IID_IMFStreamSink, (void **)&stream_sink)))
        return hr;

    if (topology_loader_is_node_d3d_aware(node))
    {
        if (SUCCEEDED(IMFTopologyNode_GetInput(node, 0, &upstream_node, &upstream_output)))
        {
            if (!topology_loader_is_node_d3d_aware(upstream_node))
            {
                if (SUCCEEDED(hr = topology_loader_create_copier(upstream_node, upstream_output, node, 0, &copier)))
                {
                    hr = topology_loader_connect_copier(context, upstream_node, upstream_output, node, 0, copier);
                    IMFTransform_Release(copier);
                }
            }
            IMFTopologyNode_Release(upstream_node);
        }
    }

    IMFStreamSink_Release(stream_sink);

    return hr;
}

static void topology_loader_resolve_complete(struct topoloader_context *context)
{
    MF_TOPOLOGY_TYPE node_type;
    IMFTopologyNode *node;
    WORD i, node_count;
    HRESULT hr;

    IMFTopology_GetNodeCount(context->output_topology, &node_count);

    for (i = 0; i < node_count; ++i)
    {
        if (SUCCEEDED(IMFTopology_GetNode(context->output_topology, i, &node)))
        {
            IMFTopologyNode_GetNodeType(node, &node_type);

            if (node_type == MF_TOPOLOGY_OUTPUT_NODE)
            {
                /* Set MF_TOPONODE_STREAMID for all outputs. */
                if (FAILED(IMFTopologyNode_GetItem(node, &MF_TOPONODE_STREAMID, NULL)))
                    IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_STREAMID, 0);

                if (FAILED(hr = topology_loader_connect_d3d_aware_input(context, node)))
                    WARN("Failed to connect D3D-aware input, hr %#lx.\n", hr);
            }
            else if (node_type == MF_TOPOLOGY_SOURCESTREAM_NODE)
            {
                /* Set MF_TOPONODE_MEDIASTART for all sources. */
                if (FAILED(IMFTopologyNode_GetItem(node, &MF_TOPONODE_MEDIASTART, NULL)))
                    IMFTopologyNode_SetUINT64(node, &MF_TOPONODE_MEDIASTART, 0);
            }

            IMFTopologyNode_Release(node);
        }
    }
}

static HRESULT WINAPI topology_loader_Load(IMFTopoLoader *iface, IMFTopology *input_topology,
        IMFTopology **ret_topology, IMFTopology *current_topology)
{
    struct list branches = LIST_INIT(branches);
    struct topoloader_context context = { 0 };
    struct topology_branch *branch, *next;
    UINT32 enumerate_source_types;
    IMFTopology *output_topology;
    MF_TOPOLOGY_TYPE node_type;
    IMFTopologyNode *node;
    unsigned short i = 0;
    IMFStreamSink *sink;
    IUnknown *object;
    HRESULT hr = E_FAIL;

    FIXME("iface %p, input_topology %p, ret_topology %p, current_topology %p stub!\n",
          iface, input_topology, ret_topology, current_topology);

    if (current_topology)
        FIXME("Current topology instance is ignored.\n");

    /* Basic sanity checks for input topology:

       - source nodes must have stream descriptor set;
       - sink nodes must be resolved to stream sink objects;
    */
    while (SUCCEEDED(IMFTopology_GetNode(input_topology, i++, &node)))
    {
        IMFTopologyNode_GetNodeType(node, &node_type);

        switch (node_type)
        {
            case MF_TOPOLOGY_OUTPUT_NODE:
                if (SUCCEEDED(hr = IMFTopologyNode_GetObject(node, &object)))
                {
                    /* Sinks must be bound beforehand. */
                    if (FAILED(IUnknown_QueryInterface(object, &IID_IMFStreamSink, (void **)&sink)))
                        hr = MF_E_TOPO_SINK_ACTIVATES_UNSUPPORTED;
                    else if (sink)
                        IMFStreamSink_Release(sink);
                    IUnknown_Release(object);
                }
                break;
            case MF_TOPOLOGY_SOURCESTREAM_NODE:
                hr = IMFTopologyNode_GetItem(node, &MF_TOPONODE_STREAM_DESCRIPTOR, NULL);
                break;
            default:
                ;
        }

        IMFTopologyNode_Release(node);
        if (FAILED(hr))
            return hr;
    }

    if (FAILED(hr = MFCreateTopology(&output_topology)))
        return hr;

    IMFTopology_CopyAllItems(input_topology, (IMFAttributes *)output_topology);

    context.input_topology = input_topology;
    context.output_topology = output_topology;

    for (i = 0; SUCCEEDED(IMFTopology_GetNode(input_topology, i, &node)); i++)
    {
        hr = topology_node_list_branches(node, &branches);
        IMFTopologyNode_Release(node);
        if (FAILED(hr))
            break;
    }
    if (SUCCEEDED(hr) && list_empty(&branches))
        hr = MF_E_TOPO_UNSUPPORTED;

    if (FAILED(IMFTopology_GetUINT32(input_topology, &MF_TOPOLOGY_ENUMERATE_SOURCE_TYPES,
            &enumerate_source_types)))
        enumerate_source_types = 0;

    while (SUCCEEDED(hr) && !list_empty(&branches))
        hr = topology_loader_resolve_branches(&context, &branches, enumerate_source_types);

    LIST_FOR_EACH_ENTRY_SAFE(branch, next, &branches, struct topology_branch, entry)
    {
        WARN("Failed to resolve branch %s\n", debugstr_topology_branch(branch));
        list_remove(&branch->entry);
        topology_branch_destroy(branch);
    }

    if (FAILED(hr))
        IMFTopology_Release(output_topology);
    else
    {
        topology_loader_resolve_complete(&context);
        *ret_topology = output_topology;
    }

    return hr;
}

static const IMFTopoLoaderVtbl topology_loader_vtbl =
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

    TRACE("loader %p.\n", loader);

    if (!loader)
        return E_POINTER;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFTopoLoader_iface.lpVtbl = &topology_loader_vtbl;
    object->refcount = 1;

    *loader = &object->IMFTopoLoader_iface;

    return S_OK;
}
