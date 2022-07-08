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
    unsigned int marker;
    GUID key;
};

static IMFTopologyNode *topology_loader_get_node_for_marker(struct topoloader_context *context, TOPOID *id)
{
    IMFTopologyNode *node;
    unsigned short i = 0;
    unsigned int value;

    while (SUCCEEDED(IMFTopology_GetNode(context->output_topology, i++, &node)))
    {
        if (SUCCEEDED(IMFTopologyNode_GetUINT32(node, &context->key, &value)) && value == context->marker)
        {
            IMFTopologyNode_GetTopoNodeID(node, id);
            return node;
        }
        IMFTopologyNode_Release(node);
    }

    *id = 0;
    return NULL;
}

static HRESULT topology_loader_clone_node(struct topoloader_context *context, IMFTopologyNode *node,
        IMFTopologyNode **ret, unsigned int marker)
{
    IMFTopologyNode *cloned_node;
    MF_TOPOLOGY_TYPE node_type;
    HRESULT hr;

    if (ret) *ret = NULL;

    IMFTopologyNode_GetNodeType(node, &node_type);

    if (FAILED(hr = MFCreateTopologyNode(node_type, &cloned_node)))
        return hr;

    if (SUCCEEDED(hr = IMFTopologyNode_CloneFrom(cloned_node, node)))
        hr = IMFTopologyNode_SetUINT32(cloned_node, &context->key, marker);

    if (SUCCEEDED(hr))
        hr = IMFTopology_AddNode(context->output_topology, cloned_node);

    if (SUCCEEDED(hr) && ret)
    {
        *ret = cloned_node;
        IMFTopologyNode_AddRef(*ret);
    }

    IMFTopologyNode_Release(cloned_node);

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

typedef HRESULT (*p_connect_func)(struct transform_output_type *output_type, struct connect_context *context);

static void topology_loader_release_transforms(IMFActivate **activates, unsigned int count)
{
    unsigned int i;

    for (i = 0; i < count; ++i)
        IMFActivate_Release(activates[i]);
    CoTaskMemFree(activates);
}

static HRESULT topology_loader_enumerate_output_types(const GUID *category, IMFMediaType *input_type,
        p_connect_func connect_func, struct connect_context *context)
{
    MFT_REGISTER_TYPE_INFO mft_typeinfo;
    IMFActivate **activates;
    unsigned int i, count;
    HRESULT hr;

    if (FAILED(hr = IMFMediaType_GetMajorType(input_type, &mft_typeinfo.guidMajorType)))
        return hr;

    if (FAILED(hr = IMFMediaType_GetGUID(input_type, &MF_MT_SUBTYPE, &mft_typeinfo.guidSubtype)))
        return hr;

    if (FAILED(hr = MFTEnumEx(*category, MFT_ENUM_FLAG_ALL, &mft_typeinfo, NULL, &activates, &count)))
        return hr;

    hr = E_FAIL;

    for (i = 0; i < count; ++i)
    {
        IMFTransform *transform;

        if (FAILED(IMFActivate_ActivateObject(activates[i], &IID_IMFTransform, (void **)&transform)))
        {
            WARN("Failed to create a transform.\n");
            continue;
        }

        if (SUCCEEDED(hr = IMFTransform_SetInputType(transform, 0, input_type, 0)))
        {
            struct transform_output_type output_type;
            unsigned int output_count = 0;

            output_type.transform = transform;
            output_type.activate = activates[i];
            while (SUCCEEDED(IMFTransform_GetOutputAvailableType(transform, 0, output_count++, &output_type.type)))
            {
                hr = connect_func(&output_type, context);
                IMFMediaType_Release(output_type.type);
                if (SUCCEEDED(hr))
                {
                    topology_loader_release_transforms(activates, count);
                    return hr;
                }
            }
        }

        IMFActivate_ShutdownObject(activates[i]);
    }

    topology_loader_release_transforms(activates, count);

    return hr;
}

static HRESULT topology_loader_create_transform(const struct transform_output_type *output_type,
        IMFTopologyNode **node)
{
    HRESULT hr;
    GUID guid;

    if (FAILED(hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, node)))
        return hr;

    IMFTopologyNode_SetObject(*node, (IUnknown *)output_type->transform);

    if (SUCCEEDED(IMFActivate_GetGUID(output_type->activate, &MF_TRANSFORM_CATEGORY_Attribute, &guid)) &&
            (IsEqualGUID(&guid, &MFT_CATEGORY_AUDIO_DECODER) || IsEqualGUID(&guid, &MFT_CATEGORY_VIDEO_DECODER)))
    {
        IMFTopologyNode_SetUINT32(*node, &MF_TOPONODE_DECODER, 1);
    }

    if (SUCCEEDED(IMFActivate_GetGUID(output_type->activate, &MFT_TRANSFORM_CLSID_Attribute, &guid)))
        IMFTopologyNode_SetGUID(*node, &MF_TOPONODE_TRANSFORM_OBJECTID, &guid);

    return hr;
}

static HRESULT connect_to_sink(struct transform_output_type *output_type, struct connect_context *context)
{
    IMFTopologyNode *node;
    HRESULT hr;

    if (FAILED(IMFMediaTypeHandler_IsMediaTypeSupported(context->sink_handler, output_type->type, NULL)))
        return MF_E_TRANSFORM_NOT_POSSIBLE_FOR_CURRENT_MEDIATYPE_COMBINATION;

    if (FAILED(hr = topology_loader_create_transform(output_type, &node)))
        return hr;

    IMFTopology_AddNode(context->context->output_topology, node);
    IMFTopologyNode_ConnectOutput(context->upstream_node, 0, node, 0);
    IMFTopologyNode_ConnectOutput(node, 0, context->sink, 0);

    IMFTopologyNode_Release(node);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(context->sink_handler, output_type->type);
    if (SUCCEEDED(hr))
        hr = IMFTransform_SetOutputType(output_type->transform, 0, output_type->type, 0);

    return S_OK;
}

static HRESULT connect_to_converter(struct transform_output_type *output_type, struct connect_context *context)
{
    struct connect_context sink_ctx;
    IMFTopologyNode *node;
    HRESULT hr;

    if (SUCCEEDED(connect_to_sink(output_type, context)))
        return S_OK;

    if (FAILED(hr = topology_loader_create_transform(output_type, &node)))
        return hr;

    sink_ctx = *context;
    sink_ctx.upstream_node = node;

    if (SUCCEEDED(hr = topology_loader_enumerate_output_types(&context->converter_category, output_type->type,
            connect_to_sink, &sink_ctx)))
    {
        hr = IMFTopology_AddNode(context->context->output_topology, node);
    }
    IMFTopologyNode_Release(node);

    if (SUCCEEDED(hr))
    {
        IMFTopology_AddNode(context->context->output_topology, node);
        IMFTopologyNode_ConnectOutput(context->upstream_node, 0, node, 0);

        hr = IMFTransform_SetOutputType(output_type->transform, 0, output_type->type, 0);
    }

    return hr;
}

typedef HRESULT (*p_topology_loader_connect_func)(struct topoloader_context *context, IMFTopologyNode *upstream_node,
        unsigned int output_index, IMFTopologyNode *downstream_node, unsigned int input_index);

static HRESULT topology_loader_get_mft_categories(IMFMediaTypeHandler *handler, GUID *decode_cat, GUID *convert_cat)
{
    GUID major;
    HRESULT hr;

    if (FAILED(hr = IMFMediaTypeHandler_GetMajorType(handler, &major)))
        return hr;

    if (IsEqualGUID(&major, &MFMediaType_Audio))
    {
        *decode_cat = MFT_CATEGORY_AUDIO_DECODER;
        *convert_cat = MFT_CATEGORY_AUDIO_EFFECT;
    }
    else if (IsEqualGUID(&major, &MFMediaType_Video))
    {
        *decode_cat = MFT_CATEGORY_VIDEO_DECODER;
        *convert_cat = MFT_CATEGORY_VIDEO_EFFECT;
    }
    else
    {
        WARN("Unexpected major type %s.\n", debugstr_guid(&major));
        return MF_E_INVALIDTYPE;
    }

    return S_OK;
}

static HRESULT topology_loader_connect(IMFMediaTypeHandler *sink_handler, unsigned int sink_method,
        struct connect_context *sink_ctx, struct connect_context *convert_ctx, IMFMediaType *media_type)
{
    HRESULT hr;

    if (SUCCEEDED(hr = IMFMediaTypeHandler_IsMediaTypeSupported(sink_handler, media_type, NULL))
            && SUCCEEDED(hr = IMFMediaTypeHandler_SetCurrentMediaType(sink_handler, media_type)))
    {
        hr = IMFTopologyNode_ConnectOutput(sink_ctx->upstream_node, sink_ctx->output_index, sink_ctx->sink, sink_ctx->input_index);
    }

    if (FAILED(hr) && sink_method & MF_CONNECT_ALLOW_CONVERTER)
    {
        hr = topology_loader_enumerate_output_types(&convert_ctx->converter_category, media_type, connect_to_sink, sink_ctx);
    }

    if (FAILED(hr) && sink_method & MF_CONNECT_ALLOW_DECODER)
    {
        hr = topology_loader_enumerate_output_types(&convert_ctx->decoder_category, media_type,
                connect_to_converter, convert_ctx);
    }

    return hr;
}

static HRESULT topology_loader_foreach_source_type(IMFMediaTypeHandler *sink_handler, IMFMediaTypeHandler *source_handler,
        unsigned int sink_method, struct connect_context *sink_ctx, struct connect_context *convert_ctx)
{
    unsigned int index = 0;
    IMFMediaType *media_type;
    HRESULT hr;

    while (SUCCEEDED(hr = IMFMediaTypeHandler_GetMediaTypeByIndex(source_handler, index++, &media_type)))
    {
        hr = topology_loader_connect(sink_handler, sink_method, sink_ctx, convert_ctx, media_type);
        if (SUCCEEDED(hr)) break;
        IMFMediaType_Release(media_type);
        media_type = NULL;
    }

    if (media_type)
    {
        hr = IMFMediaTypeHandler_SetCurrentMediaType(source_handler, media_type);
        IMFMediaType_Release(media_type);
    }

    return hr;
}

static HRESULT topology_loader_connect_source_to_sink(struct topoloader_context *context, IMFTopologyNode *source,
        unsigned int output_index, IMFTopologyNode *sink, unsigned int input_index)
{
    IMFMediaTypeHandler *source_handler = NULL, *sink_handler = NULL;
    struct connect_context convert_ctx, sink_ctx;
    MF_CONNECT_METHOD source_method, sink_method;
    unsigned int enumerate_source_types = 0;
    IMFMediaType *media_type;
    HRESULT hr;

    TRACE("attempting to connect %p:%u to %p:%u\n", source, output_index, sink, input_index);

    if (FAILED(hr = topology_node_get_type_handler(source, output_index, TRUE, &source_handler)))
        goto done;
    if (FAILED(hr = topology_node_get_type_handler(sink, input_index, FALSE, &sink_handler)))
        goto done;

    if (FAILED(IMFTopologyNode_GetUINT32(source, &MF_TOPONODE_CONNECT_METHOD, &source_method)))
        source_method = MF_CONNECT_DIRECT;
    if (FAILED(IMFTopologyNode_GetUINT32(sink, &MF_TOPONODE_CONNECT_METHOD, &sink_method)))
        sink_method = MF_CONNECT_ALLOW_DECODER;

    sink_ctx.context = context;
    sink_ctx.upstream_node = source;
    sink_ctx.sink = sink;
    sink_ctx.sink_handler = sink_handler;
    sink_ctx.output_index = output_index;
    sink_ctx.input_index = input_index;

    convert_ctx = sink_ctx;
    if (FAILED(hr = topology_loader_get_mft_categories(source_handler, &convert_ctx.decoder_category,
            &convert_ctx.converter_category)))
        goto done;

    IMFTopology_GetUINT32(context->output_topology, &MF_TOPOLOGY_ENUMERATE_SOURCE_TYPES, &enumerate_source_types);

    if (enumerate_source_types)
    {
        if (source_method & MF_CONNECT_RESOLVE_INDEPENDENT_OUTPUTTYPES)
        {
            hr = topology_loader_foreach_source_type(sink_handler, source_handler, MF_CONNECT_ALLOW_DECODER,
                    &sink_ctx, &convert_ctx);
        }
        else
        {
            hr = topology_loader_foreach_source_type(sink_handler, source_handler, MF_CONNECT_DIRECT,
                    &sink_ctx, &convert_ctx);
            if (FAILED(hr))
                hr = topology_loader_foreach_source_type(sink_handler, source_handler, MF_CONNECT_ALLOW_CONVERTER,
                        &sink_ctx, &convert_ctx);
            if (FAILED(hr))
                hr = topology_loader_foreach_source_type(sink_handler, source_handler, MF_CONNECT_ALLOW_DECODER,
                        &sink_ctx, &convert_ctx);
        }
    }
    else
    {
        if (SUCCEEDED(hr = IMFMediaTypeHandler_GetCurrentMediaType(source_handler, &media_type)))
        {
            hr = topology_loader_connect(sink_handler, sink_method, &sink_ctx, &convert_ctx, media_type);
            IMFMediaType_Release(media_type);
        }
    }

done:
    if (source_handler)
        IMFMediaTypeHandler_Release(source_handler);
    if (sink_handler)
        IMFMediaTypeHandler_Release(sink_handler);

    return hr;
}

static HRESULT topology_loader_resolve_branch(struct topoloader_context *context, IMFTopologyNode *upstream_node,
        unsigned int output_index, IMFTopologyNode *downstream_node, unsigned input_index)
{
    static const p_topology_loader_connect_func connectors[MF_TOPOLOGY_TEE_NODE+1][MF_TOPOLOGY_TEE_NODE+1] =
    {
          /* OUTPUT */ { NULL },
    /* SOURCESTREAM */ { topology_loader_connect_source_to_sink, NULL, NULL, NULL },
       /* TRANSFORM */ { NULL },
             /* TEE */ { NULL },
    };
    MF_TOPOLOGY_TYPE u_type, d_type;
    IMFTopologyNode *node;
    HRESULT hr;
    TOPOID id;

    /* Downstream node might have already been cloned. */
    IMFTopologyNode_GetTopoNodeID(downstream_node, &id);
    if (FAILED(IMFTopology_GetNodeByID(context->output_topology, id, &node)))
        topology_loader_clone_node(context, downstream_node, &node, context->marker + 1);

    IMFTopologyNode_GetNodeType(upstream_node, &u_type);
    IMFTopologyNode_GetNodeType(downstream_node, &d_type);

    if (!connectors[u_type][d_type])
    {
        WARN("Unsupported branch kind %d -> %d.\n", u_type, d_type);
        IMFTopologyNode_Release(node);
        return E_FAIL;
    }

    hr = connectors[u_type][d_type](context, upstream_node, output_index, node, input_index);
    IMFTopologyNode_Release(node);
    return hr;
}

static HRESULT topology_loader_resolve_nodes(struct topoloader_context *context, unsigned int *layer_size)
{
    IMFTopologyNode *downstream_node, *node, *orig_node;
    MF_TOPOLOGY_TYPE node_type;
    unsigned int size = 0;
    DWORD input_index;
    HRESULT hr = S_OK;
    TOPOID id;

    while ((node = topology_loader_get_node_for_marker(context, &id)))
    {
        ++size;

        IMFTopologyNode_GetNodeType(node, &node_type);
        switch (node_type)
        {
            case MF_TOPOLOGY_SOURCESTREAM_NODE:
                if (SUCCEEDED(hr = IMFTopology_GetNodeByID(context->input_topology, id, &orig_node)))
                {
                    hr = IMFTopologyNode_GetOutput(orig_node, 0, &downstream_node, &input_index);
                    IMFTopologyNode_Release(orig_node);
                }

                if (FAILED(hr))
                {
                    IMFTopology_RemoveNode(context->output_topology, node);
                    IMFTopologyNode_Release(node);
                    continue;
                }

                hr = topology_loader_resolve_branch(context, node, 0, downstream_node, input_index);
                IMFTopologyNode_Release(downstream_node);
                break;
            case MF_TOPOLOGY_TRANSFORM_NODE:
            case MF_TOPOLOGY_TEE_NODE:
                FIXME("Unsupported node type %d.\n", node_type);
                break;
            default:
                WARN("Unexpected node type %d.\n", node_type);
        }

        IMFTopologyNode_DeleteItem(node, &context->key);
        IMFTopologyNode_Release(node);

        if (FAILED(hr))
            break;
    }

    *layer_size = size;

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
    IMFMediaType *input_type = NULL, *output_type = NULL;
    IMFTransform *transform;
    HRESULT hr;

    if (FAILED(hr = MFCreateSampleCopierMFT(&transform)))
        return hr;

    if (FAILED(hr = MFGetTopoNodeCurrentType(upstream_node, upstream_output, TRUE, &input_type)))
        WARN("Failed to get upstream media type hr %#lx.\n", hr);

    if (SUCCEEDED(hr) && FAILED(hr = MFGetTopoNodeCurrentType(downstream_node, downstream_input, FALSE, &output_type)))
        WARN("Failed to get downstream media type hr %#lx.\n", hr);

    if (SUCCEEDED(hr) && FAILED(hr = IMFTransform_SetInputType(transform, 0, input_type, 0)))
        WARN("Input type wasn't accepted, hr %#lx.\n", hr);

    if (SUCCEEDED(hr) && FAILED(hr = IMFTransform_SetOutputType(transform, 0, output_type, 0)))
        WARN("Output type wasn't accepted, hr %#lx.\n", hr);

    if (SUCCEEDED(hr))
    {
        *copier = transform;
        IMFTransform_AddRef(*copier);
    }

    if (input_type)
        IMFMediaType_Release(input_type);
    if (output_type)
        IMFMediaType_Release(output_type);

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
    struct topoloader_context context = { 0 };
    IMFTopology *output_topology;
    MF_TOPOLOGY_TYPE node_type;
    unsigned int layer_size;
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
    memset(&context.key, 0xff, sizeof(context.key));

    /* Clone source nodes, use initial marker value. */
    i = 0;
    while (SUCCEEDED(IMFTopology_GetNode(input_topology, i++, &node)))
    {
        IMFTopologyNode_GetNodeType(node, &node_type);

        if (node_type == MF_TOPOLOGY_SOURCESTREAM_NODE)
        {
            if (FAILED(hr = topology_loader_clone_node(&context, node, NULL, 0)))
                WARN("Failed to clone source node, hr %#lx.\n", hr);
        }

        IMFTopologyNode_Release(node);
    }

    for (context.marker = 0;; ++context.marker)
    {
        if (FAILED(hr = topology_loader_resolve_nodes(&context, &layer_size)))
        {
            WARN("Failed to resolve for marker %u, hr %#lx.\n", context.marker, hr);
            break;
        }

        /* Reached last marker value. */
        if (!layer_size)
            break;
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
