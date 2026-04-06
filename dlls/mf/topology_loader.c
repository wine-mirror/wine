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
#include "evr.h"
#include "d3d9.h"
#include "dxva2api.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "mf_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

enum connect_method
{
    CONNECT_DIRECT    = 1,
    CONNECT_CONVERTER = 2,
    CONNECT_DECODER   = 4,
};

static enum connect_method connect_method_from_mf(MF_CONNECT_METHOD mf_method)
{
    enum connect_method method = CONNECT_DIRECT;
    if ((mf_method & MF_CONNECT_ALLOW_CONVERTER) == MF_CONNECT_ALLOW_CONVERTER)
        method |= CONNECT_CONVERTER;
    if ((mf_method & MF_CONNECT_ALLOW_DECODER) == MF_CONNECT_ALLOW_DECODER)
        method |= CONNECT_DECODER;
    return method;
}

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
    HRESULT hr;
    TOPOID id;

    if (FAILED(hr = IMFTopologyNode_GetTopoNodeID(node, &id)))
        return hr;
    if (SUCCEEDED(hr = IMFTopology_GetNodeByID(context->output_topology, id, clone)))
        return hr;
    if (FAILED(hr = MFCreateTopologyNode(topology_node_get_type(node), clone)))
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

struct topology_branch
{
    struct
    {
        IMFTopologyNode *node;
        DWORD stream;
        IMFMediaTypeHandler *handler;
    } up, down;

    struct list entry;
};

static const char *debugstr_topology_type(MF_TOPOLOGY_TYPE type)
{
    switch (type)
    {
        case MF_TOPOLOGY_OUTPUT_NODE: return "sink";
        case MF_TOPOLOGY_SOURCESTREAM_NODE: return "source";
        case MF_TOPOLOGY_TRANSFORM_NODE: return "transform";
        case MF_TOPOLOGY_TEE_NODE: return "tee";
        default: return "unknown";
    }
}

static const char *debugstr_topology_branch(struct topology_branch *branch)
{
    return wine_dbg_sprintf("%s %p:%lu to %s %p:%lu", debugstr_topology_type(topology_node_get_type(branch->up.node)),
            branch->up.node, branch->up.stream, debugstr_topology_type(topology_node_get_type(branch->down.node)),
            branch->down.node, branch->down.stream);
}

static HRESULT topology_branch_create(IMFTopologyNode *source, DWORD source_stream,
            IMFTopologyNode *sink, DWORD sink_stream, struct topology_branch **out)
{
    IMFMediaTypeHandler *source_handler, *sink_handler;
    struct topology_branch *branch;
    HRESULT hr;

    if (FAILED(hr = topology_node_get_type_handler(source, source_stream, TRUE, &source_handler)))
        return hr;
    if (FAILED(hr = topology_node_get_type_handler(sink, sink_stream, FALSE, &sink_handler)))
    {
        IMFMediaTypeHandler_Release(source_handler);
        return hr;
    }

    if (!(branch = calloc(1, sizeof(*branch))))
    {
        IMFMediaTypeHandler_Release(sink_handler);
        IMFMediaTypeHandler_Release(source_handler);
        return E_OUTOFMEMORY;
    }

    IMFTopologyNode_AddRef((branch->up.node = source));
    branch->up.stream = source_stream;
    branch->up.handler = source_handler;
    IMFTopologyNode_AddRef((branch->down.node = sink));
    branch->down.stream = sink_stream;
    branch->down.handler = sink_handler;

    *out = branch;
    return S_OK;
}

static void topology_branch_destroy(struct topology_branch *branch)
{
    IMFTopologyNode_Release(branch->up.node);
    IMFMediaTypeHandler_Release(branch->up.handler);
    IMFTopologyNode_Release(branch->down.node);
    IMFMediaTypeHandler_Release(branch->down.handler);
    free(branch);
}

static HRESULT topology_branch_create_indirect(struct topology_branch *branch,
        IMFTopologyNode *node, struct topology_branch **up, struct topology_branch **down)
{
    HRESULT hr;

    if (FAILED(hr = topology_branch_create(branch->up.node, branch->up.stream, node, 0, up)))
        return hr;
    if (FAILED(hr = topology_branch_create(node, 0, branch->down.node, branch->down.stream, down)))
        topology_branch_destroy(*up);

    return hr;
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

static void media_type_try_copy_attr(IMFMediaType *dst, IMFMediaType *src, const GUID *attr, HRESULT *hr)
{
    PROPVARIANT value;

    PropVariantInit(&value);
    if (SUCCEEDED(*hr) && FAILED(IMFMediaType_GetItem(dst, attr, NULL))
            && SUCCEEDED(IMFMediaType_GetItem(src, attr, &value)))
        *hr = IMFMediaType_SetItem(dst, attr, &value);
    PropVariantClear(&value);
}

/* update a media type with additional attributes reported by upstream element */
/* also present in mfreadwrite/reader.c pipeline */
static HRESULT update_media_type_from_upstream(IMFMediaType *media_type, IMFMediaType *upstream_type)
{
    HRESULT hr = S_OK;

    /* propagate common video attributes */
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_FRAME_SIZE, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_FRAME_RATE, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_DEFAULT_STRIDE, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_VIDEO_ROTATION, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_FIXED_SIZE_SAMPLES, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_PIXEL_ASPECT_RATIO, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, &hr);

    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_VIDEO_CHROMA_SITING, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_INTERLACE_MODE, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_TRANSFER_FUNCTION, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_VIDEO_PRIMARIES, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_YUV_MATRIX, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_VIDEO_LIGHTING, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_VIDEO_NOMINAL_RANGE, &hr);

    /* propagate common audio attributes */
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_NUM_CHANNELS, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_CHANNEL_MASK, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_SAMPLES_PER_BLOCK, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, &hr);

    return hr;
}

static HRESULT clone_media_type(IMFMediaType *in_type, IMFMediaType **out_type)
{
    HRESULT hr = MFCreateMediaType(out_type);
    if (FAILED(hr) || SUCCEEDED(hr = IMFMediaType_CopyAllItems(in_type, (IMFAttributes *)*out_type)))
        return hr;
    IMFMediaType_Release(*out_type);
    *out_type = NULL;
    return hr;
}

/* safe version of IMFMediaTypeHandler_GetCurrentMediaType that makes a copy of the returned type if necessary */
static HRESULT media_type_handler_get_current(IMFMediaTypeHandler *handler, IMFMediaType **type)
{
    IMFMediaType *tmp;
    HRESULT hr;

    if (FAILED(hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &tmp)))
        return hr;
    hr = clone_media_type(tmp, type);
    IMFMediaType_Release(tmp);
    return hr;
}

/* safe version of IMFMediaTypeHandler_GetMediaTypeByIndex that makes a copy of the returned type if necessary */
static HRESULT media_type_handler_get_type(IMFMediaTypeHandler *handler, UINT32 i, IMFMediaType **type)
{
    IMFMediaType *tmp;
    HRESULT hr;

    if (FAILED(hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, i, &tmp)))
        return hr;
    hr = clone_media_type(tmp, type);
    IMFMediaType_Release(tmp);
    return hr;
}

static BOOL is_better_media_type(IMFMediaType *type, IMFMediaType *best)
{
    GUID guid = {0};

    IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &guid);
    if (IsEqualGUID(&guid, &MFAudioFormat_Float) || IsEqualGUID(&guid, &MFAudioFormat_PCM))
    {
        IMFMediaType_GetGUID(best, &MF_MT_SUBTYPE, &guid);
        return !IsEqualGUID(&guid, &MFAudioFormat_Float);
    }

    return TRUE;
}

static void topology_branch_find_best_up_type(struct topology_branch *branch, IMFMediaType *down_type, IMFMediaType **type)
{
    IMFMediaType *up_type;
    DWORD flags;
    HRESULT hr;

    /* actually set the down type to enforce some transform internal parameters like frame size on the enumerated types */
    if (FAILED(hr = IMFMediaTypeHandler_SetCurrentMediaType(branch->up.handler, down_type)))
    {
        TRACE("down type %s not supported by upstream, hr %#lx\n", debugstr_media_type(down_type), hr);
        return;
    }

    for (UINT i = 0; SUCCEEDED(hr = media_type_handler_get_type(branch->up.handler, i, &up_type)); i++)
    {
        if (FAILED(hr = update_media_type_from_upstream(up_type, down_type)))
            WARN("Failed to update media type, hr %#lx\n", hr);

        if ((hr = IMFMediaType_IsEqual(up_type, down_type, &flags)) != S_OK)
            TRACE("up type %s differs from down type %s, hr %#lx flags %#lx\n", debugstr_media_type(up_type),
                    debugstr_media_type(down_type), hr, flags);
        else if (FAILED(hr = IMFMediaTypeHandler_IsMediaTypeSupported(branch->down.handler, up_type, NULL)))
            TRACE("up type %s not supported by downstream, hr %#lx\n", debugstr_media_type(up_type), hr);
        else if (*type && !is_better_media_type(up_type, *type))
            TRACE("up type %s isn't better than selected type %s\n", debugstr_media_type(up_type),
                    debugstr_media_type(*type));
        else
        {
            TRACE("selecting up type %s\n", debugstr_media_type(up_type));
            if (*type)
                IMFMediaType_Release(*type);
            *type = up_type;
            continue;
        }
        IMFMediaType_Release(up_type);
    }
}

static HRESULT topology_branch_find_best_type(struct topology_branch *branch, IMFMediaType *upstream, IMFMediaType **type)
{
    BOOL enumerate = topology_node_get_type(branch->down.node) == MF_TOPOLOGY_TRANSFORM_NODE;
    IMFMediaType *down_type;
    HRESULT hr;

    *type = NULL;

    if (SUCCEEDED(hr = media_type_handler_get_current(branch->down.handler, &down_type)))
    {
        if (FAILED(hr = update_media_type_from_upstream(down_type, upstream)))
            WARN("Failed to update media type, hr %#lx\n", hr);
        topology_branch_find_best_up_type(branch, down_type, type);
        IMFMediaType_Release(down_type);
    }
    else if (enumerate || hr == MF_E_NOT_INITIALIZED)
    {
        for (UINT i = 0; SUCCEEDED(hr = media_type_handler_get_type(branch->down.handler, i, &down_type)); i++)
        {
            if (FAILED(hr = update_media_type_from_upstream(down_type, upstream)))
                WARN("Failed to update media type, hr %#lx\n", hr);
            topology_branch_find_best_up_type(branch, down_type, type);
            IMFMediaType_Release(down_type);
        }
    }

    return *type ? S_OK : MF_E_INVALIDMEDIATYPE;
}

static HRESULT topology_branch_connect_with_type(IMFTopology *topology, struct topology_branch *branch, IMFMediaType *type)
{
    HRESULT hr;

    if (FAILED(hr = IMFMediaTypeHandler_SetCurrentMediaType(branch->up.handler, type)))
    {
        WARN("Failed to set upstream node media type, hr %#lx\n", hr);
        return hr;
    }

    if (topology_node_get_type(branch->down.node) == MF_TOPOLOGY_TRANSFORM_NODE
            && FAILED(hr = IMFMediaTypeHandler_SetCurrentMediaType(branch->down.handler, type)))
    {
        WARN("Failed to set transform node media type, hr %#lx\n", hr);
        return hr;
    }

    TRACE("Connecting branch %s with type %s.\n", debugstr_topology_branch(branch), debugstr_media_type(type));
    return IMFTopologyNode_ConnectOutput(branch->up.node, branch->up.stream, branch->down.node, branch->down.stream);
}

static HRESULT topology_branch_connect(IMFTopology *topology, enum connect_method method_mask,
        struct topology_branch *branch);
static HRESULT topology_branch_connect_indirect(IMFTopology *topology, BOOL decoder,
        struct topology_branch *branch, IMFMediaType *upstream)
{
    enum connect_method method_mask = CONNECT_DIRECT;
    MFT_REGISTER_TYPE_INFO input_info;
    IMFTransform *transform;
    IMFActivate **activates;
    IMFTopologyNode *node;
    unsigned int i, count;
    GUID category, guid;
    HRESULT hr;

    TRACE("topology %p, decoder %u, branch %s, upstream %s.\n", topology, decoder,
            debugstr_topology_branch(branch), debugstr_media_type(upstream));

    if (FAILED(hr = IMFMediaType_GetMajorType(upstream, &input_info.guidMajorType)))
        return hr;
    if (FAILED(hr = IMFMediaType_GetGUID(upstream, &MF_MT_SUBTYPE, &input_info.guidSubtype)))
        return hr;

    if (IsEqualGUID(&input_info.guidMajorType, &MFMediaType_Audio))
        category = decoder ? MFT_CATEGORY_AUDIO_DECODER : MFT_CATEGORY_AUDIO_EFFECT;
    else if (IsEqualGUID(&input_info.guidMajorType, &MFMediaType_Video))
        category = decoder ? MFT_CATEGORY_VIDEO_DECODER : MFT_CATEGORY_VIDEO_PROCESSOR;
    else
        return MF_E_INVALIDMEDIATYPE;

    if (FAILED(hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &node)))
        return hr;
    if (decoder)
    {
        IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_DECODER, 1);
        IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_MARKIN_HERE, 1);
        IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_MARKOUT_HERE, 1);
        method_mask |= CONNECT_CONVERTER;
    }

    if (FAILED(hr = MFTEnumEx(category, MFT_ENUM_FLAG_ALL, &input_info, NULL, &activates, &count)))
        return hr;

    for (i = 0; i < count; ++i)
    {
        struct topology_branch *up_branch, *down_branch;
        IMFMediaType *media_type;

        if (FAILED(IMFActivate_ActivateObject(activates[i], &IID_IMFTransform, (void **)&transform)))
            continue;
        IMFTopologyNode_SetObject(node, (IUnknown *)transform);
        IMFTopologyNode_DeleteItem(node, &MF_TOPONODE_TRANSFORM_OBJECTID);
        if (SUCCEEDED(IMFActivate_GetGUID(activates[i], &MFT_TRANSFORM_CLSID_Attribute, &guid)))
            IMFTopologyNode_SetGUID(node, &MF_TOPONODE_TRANSFORM_OBJECTID, &guid);
        IMFTransform_Release(transform);

        if (SUCCEEDED(hr = topology_branch_create_indirect(branch, node, &up_branch, &down_branch)))
        {
            if (SUCCEEDED(hr = topology_branch_connect_with_type(topology, up_branch, upstream)))
            {
                if (FAILED(hr = topology_branch_find_best_type(down_branch, upstream, &media_type)))
                    hr = topology_branch_connect(topology, method_mask, down_branch);
                else
                {
                    hr = topology_branch_connect_with_type(topology, down_branch, media_type);
                    IMFMediaType_Release(media_type);
                }
            }
            topology_branch_destroy(down_branch);
            topology_branch_destroy(up_branch);
        }

        if (SUCCEEDED(hr))
            hr = IMFTopology_AddNode(topology, node);
        if (SUCCEEDED(hr))
            break;
        IMFTopologyNode_DisconnectOutput(branch->up.node, branch->up.stream);
    }

    IMFTopologyNode_Release(node);
    for (i = 0; i < count; ++i)
        IMFActivate_Release(activates[i]);
    CoTaskMemFree(activates);

    if (!count || FAILED(hr))
        hr = decoder ? MF_E_TOPO_CODEC_NOT_FOUND
                : MF_E_TRANSFORM_NOT_POSSIBLE_FOR_CURRENT_MEDIATYPE_COMBINATION;
    TRACE("returning %#lx\n", hr);
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

static HRESULT topology_branch_connect_direct(IMFTopology *topology, struct topology_branch *branch, IMFMediaType *up_type)
{
    IMFMediaType *down_type, *type;
    DWORD flags;
    HRESULT hr;

    TRACE("topology %p, branch %s, up_type %s.\n", topology, debugstr_topology_branch(branch),
            debugstr_media_type(up_type));

    if (SUCCEEDED(hr = IMFMediaTypeHandler_GetCurrentMediaType(branch->down.handler, &down_type)))
    {
        if (IMFMediaType_IsEqual(up_type, down_type, &flags) == S_OK)
            hr = topology_branch_connect_with_type(topology, branch, up_type);
        else
        {
            TRACE("current type %s differs from up type %s, hr %#lx\n", debugstr_media_type(down_type),
                    debugstr_media_type(up_type), hr);
            hr = MF_E_INVALIDMEDIATYPE;
        }
        IMFMediaType_Release(down_type);
        TRACE("returning %#lx\n", hr);
        return hr;
    }

    IMFMediaType_AddRef((type = up_type));

    if (hr == MF_E_NOT_INITIALIZED)
    {
        for (UINT i = 0; SUCCEEDED(hr = IMFMediaTypeHandler_GetMediaTypeByIndex(branch->down.handler, i, &down_type)); i++)
        {
            if ((hr = IMFMediaType_IsEqual(up_type, down_type, &flags)) != S_OK)
                TRACE("down type %s differs from up type %s, hr %#lx flags %#lx\n", debugstr_media_type(down_type),
                        debugstr_media_type(up_type), hr, flags);
            else if (FAILED(hr = IMFMediaTypeHandler_IsMediaTypeSupported(branch->down.handler, down_type, NULL)))
                TRACE("down type %s not supported by downstream, hr %#lx\n", debugstr_media_type(down_type), hr);
            else if (FAILED(hr = IMFMediaTypeHandler_IsMediaTypeSupported(branch->up.handler, down_type, NULL)))
                TRACE("down type %s not supported by upstream, hr %#lx\n", debugstr_media_type(down_type), hr);
            else
            {
                TRACE("selecting down type %s\n", debugstr_media_type(down_type));
                IMFMediaType_Release(type);
                type = down_type;
                continue;
            }
            IMFMediaType_Release(down_type);
        }
    }

    if (FAILED(hr = IMFMediaTypeHandler_IsMediaTypeSupported(branch->down.handler, type, NULL)))
        TRACE("type %s not supported by downstream, hr %#lx\n", debugstr_media_type(type), hr);
    else
        hr = topology_branch_connect_with_type(topology, branch, type);

    IMFMediaType_Release(type);
    TRACE("returning %#lx\n", hr);
    return hr;
}

static HRESULT topology_branch_connect_down(IMFTopology *topology, enum connect_method method,
        struct topology_branch *branch, IMFMediaType *up_type)
{
    HRESULT hr = MF_E_INVALIDMEDIATYPE;

    TRACE("topology %p, branch %s, up_type %s.\n", topology, debugstr_topology_branch(branch), debugstr_media_type(up_type));

    if (FAILED(hr) && (method & CONNECT_DIRECT))
        hr = topology_branch_connect_direct(topology, branch, up_type);
    if (FAILED(hr) && (method & CONNECT_CONVERTER))
        hr = topology_branch_connect_indirect(topology, FALSE, branch, up_type);
    if (FAILED(hr) && (method & CONNECT_DECODER))
        hr = topology_branch_connect_indirect(topology, TRUE, branch, up_type);

    TRACE("returning %#lx\n", hr);
    return hr;
}

static HRESULT topology_branch_foreach_up_types(IMFTopology *topology, enum connect_method method_mask,
        struct topology_branch *branch)
{
    HRESULT hr = MF_E_INVALIDMEDIATYPE;
    UINT32 enumerate = TRUE;
    IMFMediaType *up_type;

    TRACE("topology %p, method_mask %#x, branch %s.\n", topology, method_mask, debugstr_topology_branch(branch));

    switch (topology_node_get_type(branch->up.node))
    {
        case MF_TOPOLOGY_SOURCESTREAM_NODE:
            if (FAILED(IMFTopology_GetUINT32(topology, &MF_TOPOLOGY_ENUMERATE_SOURCE_TYPES, &enumerate)))
                enumerate = FALSE;
            break;
        default:
            if (SUCCEEDED(IMFMediaTypeHandler_GetCurrentMediaType(branch->up.handler, &up_type)))
            {
                IMFMediaType_Release(up_type);
                enumerate = FALSE;
            }
            break;
    }

    if (enumerate)
    {
        for (UINT i = 0; SUCCEEDED(hr = IMFMediaTypeHandler_GetMediaTypeByIndex(branch->up.handler, i, &up_type)); i++)
        {
            hr = topology_branch_connect_down(topology, method_mask, branch, up_type);
            IMFMediaType_Release(up_type);
            if (SUCCEEDED(hr))
                return hr;
        }
        return hr;
    }

    if (SUCCEEDED(hr = IMFMediaTypeHandler_GetCurrentMediaType(branch->up.handler, &up_type))
            || SUCCEEDED(hr = IMFMediaTypeHandler_GetMediaTypeByIndex(branch->up.handler, 0, &up_type)))
    {
        hr = topology_branch_connect_down(topology, method_mask, branch, up_type);
        IMFMediaType_Release(up_type);
        return hr;
    }

    TRACE("returning %#lx\n", hr);
    return hr;
}

static HRESULT topology_branch_connect(IMFTopology *topology, enum connect_method method_mask, struct topology_branch *branch)
{
    HRESULT hr = MF_E_INVALIDMEDIATYPE;
    UINT32 up_method, down_method;

    TRACE("topology %p, method_mask %#x, branch %s.\n", topology, method_mask, debugstr_topology_branch(branch));

    if (FAILED(IMFTopologyNode_GetUINT32(branch->down.node, &MF_TOPONODE_CONNECT_METHOD, &down_method)))
        down_method = MF_CONNECT_ALLOW_DECODER;
    down_method = connect_method_from_mf(down_method) & method_mask;

    if (topology_node_get_type(branch->up.node) != MF_TOPOLOGY_SOURCESTREAM_NODE
            || FAILED(IMFTopologyNode_GetUINT32(branch->up.node, &MF_TOPONODE_CONNECT_METHOD, &up_method)))
        up_method = MF_CONNECT_DIRECT;

    if (up_method & MF_CONNECT_RESOLVE_INDEPENDENT_OUTPUTTYPES)
        hr = topology_branch_foreach_up_types(topology, down_method, branch);
    else
    {
        if (FAILED(hr) && (down_method & CONNECT_DIRECT))
            hr = topology_branch_foreach_up_types(topology, CONNECT_DIRECT, branch);
        if (FAILED(hr) && (down_method & CONNECT_CONVERTER))
            hr = topology_branch_foreach_up_types(topology, CONNECT_CONVERTER, branch);
        if (FAILED(hr) && (down_method & CONNECT_DECODER))
            hr = topology_branch_foreach_up_types(topology, CONNECT_DECODER, branch);
    }

    TRACE("returning %#lx\n", hr);
    return hr;
}

static HRESULT topology_loader_resolve_branches(struct topoloader_context *context, struct list *branches)
{
    enum connect_method method_mask = connect_method_from_mf(MF_CONNECT_ALLOW_DECODER);
    struct topology_branch *branch, *next;
    HRESULT hr = S_OK;

    LIST_FOR_EACH_ENTRY_SAFE(branch, next, branches, struct topology_branch, entry)
    {
        list_remove(&branch->entry);

        if (FAILED(hr = topology_branch_clone_nodes(context, branch)))
            WARN("Failed to clone nodes for branch %s\n", debugstr_topology_branch(branch));
        else
            hr = topology_branch_connect(context->output_topology, method_mask, branch);

        topology_branch_destroy(branch);
        if (FAILED(hr))
            break;
    }

    return hr;
}

static BOOL topology_node_get_object_attributes(IMFTopologyNode *node, IMFAttributes **attributes)
{
    IMFTransform *transform;
    HRESULT hr;

    if (SUCCEEDED(topology_node_get_object(node, &IID_IMFTransform, (void **)&transform)))
    {
        hr = IMFTransform_GetAttributes(transform, attributes);
        IMFTransform_Release(transform);
        return hr;
    }

    return topology_node_get_object(node, &IID_IMFAttributes, (void **)&attributes);
}

BOOL topology_node_is_d3d_aware(IMFTopologyNode *node)
{
    UINT32 d3d_aware, d3d11_aware;
    IMFAttributes *attributes;

    if (FAILED(topology_node_get_object_attributes(node, &attributes)))
        return FALSE;
    if (FAILED(IMFAttributes_GetUINT32(attributes, &MF_SA_D3D_AWARE, &d3d_aware)))
        d3d_aware = FALSE;
    if (FAILED(IMFAttributes_GetUINT32(attributes, &MF_SA_D3D11_AWARE, &d3d11_aware)))
        d3d11_aware = FALSE;
    IMFAttributes_Release(attributes);

    return d3d_aware || d3d11_aware;
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

HRESULT topology_node_set_device_manager(IMFTopologyNode *node, IUnknown *device_manager)
{
    IMFTransform *transform;
    HRESULT hr;

    if (SUCCEEDED(hr = topology_node_get_object(node, &IID_IMFTransform, (void **)&transform)))
    {
        hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_SET_D3D_MANAGER, (LONG_PTR)device_manager);
        IMFTransform_Release(transform);
    }

    if (SUCCEEDED(hr))
    {
        IMFTopologyNode *upstream;
        DWORD i, count, output;

        hr = IMFTopologyNode_GetInputCount(node, &count);

        for (i = 0; SUCCEEDED(hr) && i < count; i++)
        {
            if (FAILED(IMFTopologyNode_GetInput(node, 0, &upstream, &output)))
                continue;

            if (topology_node_is_d3d_aware(upstream))
                topology_node_set_device_manager(upstream, device_manager);

            IMFTopologyNode_Release(upstream);
        }
    }

    return hr;
}

HRESULT stream_sink_get_device_manager(IMFStreamSink *stream_sink, IUnknown **device_manager)
{
    HRESULT hr;

    if (SUCCEEDED(hr = MFGetService((IUnknown *)stream_sink, &MR_VIDEO_ACCELERATION_SERVICE,
            &IID_IMFDXGIDeviceManager, (void **)device_manager)))
        return hr;
    if (SUCCEEDED(hr = MFGetService((IUnknown *)stream_sink, &MR_VIDEO_ACCELERATION_SERVICE,
            &IID_IDirect3DDeviceManager9, (void **)device_manager)))
        return hr;

    return hr;
}

/* Right now this should be used for output nodes only. */
static HRESULT topology_loader_connect_d3d_aware_sink(struct topoloader_context *context,
        IMFTopologyNode *node, MFTOPOLOGY_DXVA_MODE dxva_mode)
{
    IMFTopologyNode *upstream_node;
    IMFTransform *copier = NULL;
    IMFStreamSink *stream_sink;
    IUnknown *device_manager;
    DWORD upstream_output;
    HRESULT hr;

    if (FAILED(hr = topology_node_get_object(node, &IID_IMFStreamSink, (void **)&stream_sink)))
        return hr;

    if (SUCCEEDED(hr = stream_sink_get_device_manager(stream_sink, &device_manager)))
    {
        if (SUCCEEDED(IMFTopologyNode_GetInput(node, 0, &upstream_node, &upstream_output)))
        {
            BOOL needs_copier = dxva_mode == MFTOPOLOGY_DXVA_DEFAULT;
            IMFTransform *transform;

            if (needs_copier && SUCCEEDED(topology_node_get_object(upstream_node, &IID_IMFTransform, (void **)&transform)))
            {
                MFT_OUTPUT_STREAM_INFO info = {0};

                if (FAILED(IMFTransform_GetOutputStreamInfo(transform, upstream_output, &info))
                        || !(info.dwFlags & (MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES | MFT_OUTPUT_STREAM_PROVIDES_SAMPLES)))
                    needs_copier = FALSE;

                IMFTransform_Release(transform);
            }

            if (needs_copier && SUCCEEDED(hr = topology_loader_create_copier(upstream_node, upstream_output, node, 0, &copier)))
            {
                hr = topology_loader_connect_copier(context, upstream_node, upstream_output, node, 0, copier);
                IMFTransform_Release(copier);
            }

            if (dxva_mode == MFTOPOLOGY_DXVA_FULL && topology_node_is_d3d_aware(upstream_node))
                topology_node_set_device_manager(upstream_node, device_manager);

            IMFTopologyNode_Release(upstream_node);
        }

        IUnknown_Release(device_manager);
    }

    IMFStreamSink_Release(stream_sink);

    return hr;
}

static void topology_loader_resolve_complete(struct topoloader_context *context)
{
    MFTOPOLOGY_DXVA_MODE dxva_mode;
    IMFTopologyNode *node;
    WORD i, node_count;
    HRESULT hr;

    IMFTopology_GetNodeCount(context->output_topology, &node_count);

    if (FAILED(IMFTopology_GetUINT32(context->input_topology, &MF_TOPOLOGY_DXVA_MODE, (UINT32 *)&dxva_mode)))
        dxva_mode = 0;

    for (i = 0; i < node_count; ++i)
    {
        if (SUCCEEDED(IMFTopology_GetNode(context->output_topology, i, &node)))
        {
            switch (topology_node_get_type(node))
            {
                case MF_TOPOLOGY_OUTPUT_NODE:
                    /* Set MF_TOPONODE_STREAMID for all outputs. */
                    if (FAILED(IMFTopologyNode_GetItem(node, &MF_TOPONODE_STREAMID, NULL)))
                        IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_STREAMID, 0);

                    if (FAILED(hr = topology_loader_connect_d3d_aware_sink(context, node, dxva_mode)))
                        WARN("Failed to connect D3D-aware input, hr %#lx.\n", hr);
                    break;
                case MF_TOPOLOGY_SOURCESTREAM_NODE:
                    /* Set MF_TOPONODE_MEDIASTART for all sources. */
                    if (FAILED(IMFTopologyNode_GetItem(node, &MF_TOPONODE_MEDIASTART, NULL)))
                        IMFTopologyNode_SetUINT64(node, &MF_TOPONODE_MEDIASTART, 0);
                    break;
                default:
                    ;
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
    IMFTopology *output_topology;
    IMFTopologyNode *node;
    unsigned short i = 0;
    IMFStreamSink *sink;
    IUnknown *object;
    TOPOID topoid;
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
        switch (topology_node_get_type(node))
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

    IMFTopology_GetTopologyID(input_topology, &topoid);
    if (FAILED(hr = create_topology(topoid, &output_topology)))
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

    while (SUCCEEDED(hr) && !list_empty(&branches))
        hr = topology_loader_resolve_branches(&context, &branches);

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
