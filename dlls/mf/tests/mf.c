/*
 * Unit tests for mf.dll.
 *
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
#include <string.h>
#include <float.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"

#include "d3d9.h"
#include "mfapi.h"
#include "mferror.h"
#include "mfidl.h"
#include "mmdeviceapi.h"
#include "uuids.h"
#include "wmcodecdsp.h"

#include "mf_test.h"

#include "wine/test.h"

#include "initguid.h"
#include "evr9.h"

extern GUID DMOVideoFormat_RGB32;

HRESULT (WINAPI *pMFCreateSampleCopierMFT)(IMFTransform **copier);
HRESULT (WINAPI *pMFGetTopoNodeCurrentType)(IMFTopologyNode *node, DWORD stream, BOOL output, IMFMediaType **type);
BOOL has_video_processor;

static BOOL is_vista(void)
{
    return !pMFGetTopoNodeCurrentType;
}

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG expected_refcount, int line)
{
    ULONG refcount;
    IUnknown_AddRef(obj);
    refcount = IUnknown_Release(obj);
    ok_(__FILE__, line)(refcount == expected_refcount, "Unexpected refcount %ld, expected %ld.\n", refcount,
            expected_refcount);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

#define check_service_interface(a, b, c, d) check_service_interface_(__LINE__, a, b, c, d)
static void check_service_interface_(unsigned int line, void *iface_ptr, REFGUID service, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = MFGetService(iface, service, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "mf_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, r.right - r.left, r.bottom - r.top, NULL, NULL, NULL, NULL);
}

static IMFSample *create_sample(const BYTE *data, ULONG size)
{
    IMFMediaBuffer *media_buffer;
    IMFSample *sample;
    DWORD length;
    BYTE *buffer;
    HRESULT hr;
    ULONG ret;

    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "MFCreateSample returned %#lx\n", hr);
    hr = MFCreateMemoryBuffer(size, &media_buffer);
    ok(hr == S_OK, "MFCreateMemoryBuffer returned %#lx\n", hr);
    hr = IMFMediaBuffer_Lock(media_buffer, &buffer, NULL, &length);
    ok(hr == S_OK, "Lock returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    if (!data) memset(buffer, 0xcd, size);
    else memcpy(buffer, data, size);
    hr = IMFMediaBuffer_Unlock(media_buffer);
    ok(hr == S_OK, "Unlock returned %#lx\n", hr);
    hr = IMFMediaBuffer_SetCurrentLength(media_buffer, data ? size : 0);
    ok(hr == S_OK, "SetCurrentLength returned %#lx\n", hr);
    hr = IMFSample_AddBuffer(sample, media_buffer);
    ok(hr == S_OK, "AddBuffer returned %#lx\n", hr);
    ret = IMFMediaBuffer_Release(media_buffer);
    ok(ret == 1, "Release returned %lu\n", ret);

    return sample;
}

#define check_handler_required_attributes(a, b) check_handler_required_attributes_(__LINE__, a, b)
static void check_handler_required_attributes_(int line, IMFMediaTypeHandler *handler, const struct attribute_desc *attributes)
{
    const struct attribute_desc *attr;
    IMFMediaType *media_type;
    HRESULT hr;
    ULONG ref;

    hr = MFCreateMediaType(&media_type);
    ok_(__FILE__, line)(hr == S_OK, "MFCreateMediaType returned hr %#lx.\n", hr);
    init_media_type(media_type, attributes, -1);

    for (attr = attributes; attr && attr->key; attr++)
    {
        winetest_push_context("%s", debugstr_a(attr->name));
        hr = IMFMediaType_DeleteItem(media_type, attr->key);
        ok_(__FILE__, line)(hr == S_OK, "DeleteItem returned %#lx\n", hr);
        hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, NULL);
        todo_wine_if(attr->todo)
        ok_(__FILE__, line)(FAILED(hr) == attr->required, "IsMediaTypeSupported returned %#lx.\n", hr);
        hr = IMFMediaType_SetItem(media_type, attr->key, &attr->value);
        ok_(__FILE__, line)(hr == S_OK, "SetItem returned %#lx\n", hr);
        winetest_pop_context();
    }

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, NULL);
    ok_(__FILE__, line)(hr == S_OK, "IsMediaTypeSupported returned %#lx.\n", hr);
    ref = IMFMediaType_Release(media_type);
    ok_(__FILE__, line)(!ref, "Release returned %lu\n", ref);
}

static void create_descriptors(UINT enum_types_count, IMFMediaType **enum_types, const media_type_desc *current_desc,
        IMFPresentationDescriptor **pd, IMFStreamDescriptor **sd)
{
    HRESULT hr;

    hr = MFCreateStreamDescriptor(0, enum_types_count, enum_types, sd);
    ok(hr == S_OK, "Failed to create stream descriptor, hr %#lx.\n", hr);

    hr = MFCreatePresentationDescriptor(1, sd, pd);
    ok(hr == S_OK, "Failed to create presentation descriptor, hr %#lx.\n", hr);

    if (current_desc)
    {
        IMFMediaTypeHandler *handler;
        IMFMediaType *type;

        hr = MFCreateMediaType(&type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        init_media_type(type, *current_desc, -1);

        hr = IMFStreamDescriptor_GetMediaTypeHandler(*sd, &handler);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        IMFMediaTypeHandler_Release(handler);
        IMFMediaType_Release(type);
    }
}

static void init_source_node(IMFMediaSource *source, MF_CONNECT_METHOD method, IMFTopologyNode *node,
        IMFPresentationDescriptor *pd, IMFStreamDescriptor *sd)
{
    HRESULT hr;

    hr = IMFTopologyNode_DeleteAllItems(node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetUnknown(node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, (IUnknown *)pd);
    ok(hr == S_OK, "Failed to set node pd, hr %#lx.\n", hr);
    hr = IMFTopologyNode_SetUnknown(node, &MF_TOPONODE_STREAM_DESCRIPTOR, (IUnknown *)sd);
    ok(hr == S_OK, "Failed to set node sd, hr %#lx.\n", hr);

    if (method != -1)
    {
        hr = IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_CONNECT_METHOD, method);
        ok(hr == S_OK, "Failed to set connect method, hr %#lx.\n", hr);
    }

    if (source)
    {
        hr = IMFTopologyNode_SetUnknown(node, &MF_TOPONODE_SOURCE, (IUnknown *)source);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }
}

static void init_sink_node(IMFStreamSink *stream_sink, MF_CONNECT_METHOD method, IMFTopologyNode *node)
{
    HRESULT hr;

    hr = IMFTopologyNode_DeleteAllItems(node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetObject(node, (IUnknown *)stream_sink);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);

    if (method != -1)
    {
        hr = IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_CONNECT_METHOD, method);
        ok(hr == S_OK, "Failed to set connect method, hr %#lx.\n", hr);
    }
}

struct test_source
{
    IMFMediaSource IMFMediaSource_iface;
    LONG refcount;
    IMFPresentationDescriptor *pd;
};

static struct test_source *impl_from_IMFMediaSource(IMFMediaSource *iface)
{
    return CONTAINING_RECORD(iface, struct test_source, IMFMediaSource_iface);
}

static HRESULT WINAPI test_source_QueryInterface(IMFMediaSource *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IMFMediaSource)
            || IsEqualIID(riid, &IID_IMFMediaEventGenerator)
            || IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
    }
    else
    {
        *out = NULL;
        return E_NOINTERFACE;
    }

    IMFMediaSource_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI test_source_AddRef(IMFMediaSource *iface)
{
    struct test_source *source = impl_from_IMFMediaSource(iface);
    return InterlockedIncrement(&source->refcount);
}

static ULONG WINAPI test_source_Release(IMFMediaSource *iface)
{
    struct test_source *source = impl_from_IMFMediaSource(iface);
    ULONG refcount = InterlockedDecrement(&source->refcount);

    if (!refcount)
    {
        IMFPresentationDescriptor_Release(source->pd);
        free(source);
    }

    return refcount;
}

static HRESULT WINAPI test_source_GetEvent(IMFMediaSource *iface, DWORD flags, IMFMediaEvent **event)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_source_BeginGetEvent(IMFMediaSource *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_source_EndGetEvent(IMFMediaSource *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_source_QueueEvent(IMFMediaSource *iface, MediaEventType event_type, REFGUID ext_type,
        HRESULT hr, const PROPVARIANT *value)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_source_GetCharacteristics(IMFMediaSource *iface, DWORD *flags)
{
    *flags = 0;
    return S_OK;
}

static HRESULT WINAPI test_source_CreatePresentationDescriptor(IMFMediaSource *iface, IMFPresentationDescriptor **pd)
{
    struct test_source *source = impl_from_IMFMediaSource(iface);
    return IMFPresentationDescriptor_Clone(source->pd, pd);
}

static HRESULT WINAPI test_source_Start(IMFMediaSource *iface, IMFPresentationDescriptor *pd, const GUID *time_format,
        const PROPVARIANT *start_position)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_source_Stop(IMFMediaSource *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_source_Pause(IMFMediaSource *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_source_Shutdown(IMFMediaSource *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IMFMediaSourceVtbl test_source_vtbl =
{
    test_source_QueryInterface,
    test_source_AddRef,
    test_source_Release,
    test_source_GetEvent,
    test_source_BeginGetEvent,
    test_source_EndGetEvent,
    test_source_QueueEvent,
    test_source_GetCharacteristics,
    test_source_CreatePresentationDescriptor,
    test_source_Start,
    test_source_Stop,
    test_source_Pause,
    test_source_Shutdown,
};

static IMFMediaSource *create_test_source(IMFPresentationDescriptor *pd)
{
    struct test_source *source;

    source = calloc(1, sizeof(*source));
    source->IMFMediaSource_iface.lpVtbl = &test_source_vtbl;
    source->refcount = 1;
    IMFPresentationDescriptor_AddRef((source->pd = pd));

    return &source->IMFMediaSource_iface;
}

static HRESULT WINAPI test_unk_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
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

static ULONG WINAPI test_unk_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI test_unk_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl test_unk_vtbl =
{
    test_unk_QueryInterface,
    test_unk_AddRef,
    test_unk_Release,
};

static void test_topology(void)
{
    IMFMediaType *mediatype, *mediatype2, *mediatype3;
    IMFCollection *collection, *collection2;
    IUnknown test_unk2 = { &test_unk_vtbl };
    IUnknown test_unk = { &test_unk_vtbl };
    IMFTopologyNode *node, *node2, *node3;
    IMFTopology *topology, *topology2;
    DWORD size, io_count, index;
    MF_TOPOLOGY_TYPE node_type;
    IUnknown *object;
    WORD node_count;
    UINT32 count;
    HRESULT hr;
    TOPOID id;
    LONG ref;

    hr = MFCreateTopology(NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);
    hr = IMFTopology_GetTopologyID(topology, &id);
    ok(hr == S_OK, "Failed to get id, hr %#lx.\n", hr);
    ok(id == 1, "Unexpected id.\n");

    hr = MFCreateTopology(&topology2);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);
    hr = IMFTopology_GetTopologyID(topology2, &id);
    ok(hr == S_OK, "Failed to get id, hr %#lx.\n", hr);
    ok(id == 2, "Unexpected id.\n");

    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);
    hr = IMFTopology_GetTopologyID(topology, &id);
    ok(hr == S_OK, "Failed to get id, hr %#lx.\n", hr);
    ok(id == 3, "Unexpected id.\n");

    ref = IMFTopology_Release(topology2);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* No attributes by default. */
    for (node_type = MF_TOPOLOGY_OUTPUT_NODE; node_type < MF_TOPOLOGY_TEE_NODE; ++node_type)
    {
        hr = MFCreateTopologyNode(node_type, &node);
        ok(hr == S_OK, "Failed to create a node for type %d, hr %#lx.\n", node_type, hr);
        hr = IMFTopologyNode_GetCount(node, &count);
        ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
        ok(!count, "Unexpected attribute count %u.\n", count);
        ref = IMFTopologyNode_Release(node);
        ok(ref == 0, "Release returned %ld\n", ref);
    }

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetTopoNodeID(node, &id);
    ok(hr == S_OK, "Failed to get node id, hr %#lx.\n", hr);
    ok(((id >> 32) == GetCurrentProcessId()) && !!(id & 0xffff), "Unexpected node id %s.\n", wine_dbgstr_longlong(id));

    hr = IMFTopologyNode_SetTopoNodeID(node2, id);
    ok(hr == S_OK, "Failed to set node id, hr %#lx.\n", hr);

    hr = IMFTopology_GetNodeCount(topology, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    node_count = 1;
    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 0, "Unexpected node count %u.\n", node_count);

    /* Same id, different nodes. */
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    node_count = 0;
    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 1, "Unexpected node count %u.\n", node_count);

    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ref = IMFTopologyNode_Release(node2);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = IMFTopology_GetNodeByID(topology, id, &node2);
    ok(hr == S_OK, "Failed to get a node, hr %#lx.\n", hr);
    ok(node2 == node, "Unexpected node.\n");
    IMFTopologyNode_Release(node2);

    /* Change node id, add it again. */
    hr = IMFTopologyNode_SetTopoNodeID(node, ++id);
    ok(hr == S_OK, "Failed to set node id, hr %#lx.\n", hr);

    hr = IMFTopology_GetNodeByID(topology, id, &node2);
    ok(hr == S_OK, "Failed to get a node, hr %#lx.\n", hr);
    ok(node2 == node, "Unexpected node.\n");
    IMFTopologyNode_Release(node2);

    hr = IMFTopology_GetNodeByID(topology, id + 1, &node2);
    ok(hr == MF_E_NOT_FOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == E_INVALIDARG, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopology_GetNode(topology, 0, &node2);
    ok(hr == S_OK, "Failed to get a node, hr %#lx.\n", hr);
    ok(node2 == node, "Unexpected node.\n");
    IMFTopologyNode_Release(node2);

    hr = IMFTopology_GetNode(topology, 1, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_GetNode(topology, 1, &node2);
    ok(hr == MF_E_INVALIDINDEX, "Failed to get a node, hr %#lx.\n", hr);

    hr = IMFTopology_GetNode(topology, -2, &node2);
    ok(hr == MF_E_INVALIDINDEX, "Failed to get a node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);
    ref = IMFTopologyNode_Release(node2);
    ok(ref == 1, "Release returned %ld\n", ref);

    node_count = 0;
    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 2, "Unexpected node count %u.\n", node_count);

    /* Remove with detached node, existing id. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);
    hr = IMFTopologyNode_SetTopoNodeID(node2, id);
    ok(hr == S_OK, "Failed to set node id, hr %#lx.\n", hr);
    hr = IMFTopology_RemoveNode(topology, node2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ref = IMFTopologyNode_Release(node2);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = IMFTopology_RemoveNode(topology, node);
    ok(hr == S_OK, "Failed to remove a node, hr %#lx.\n", hr);

    node_count = 0;
    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 1, "Unexpected node count %u.\n", node_count);

    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#lx.\n", hr);

    node_count = 1;
    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 0, "Unexpected node count %u.\n", node_count);

    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetTopoNodeID(node, 123);
    ok(hr == S_OK, "Failed to set node id, hr %#lx.\n", hr);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Change id for attached node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetTopoNodeID(node, &id);
    ok(hr == S_OK, "Failed to get node id, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetTopoNodeID(node2, id);
    ok(hr == S_OK, "Failed to get node id, hr %#lx.\n", hr);

    hr = IMFTopology_GetNodeByID(topology, id, &node3);
    ok(hr == S_OK, "Failed to get a node, hr %#lx.\n", hr);
    ok(node3 == node, "Unexpected node.\n");
    IMFTopologyNode_Release(node3);

    /* Source/output collections. */
    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#lx.\n", hr);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(node2);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = IMFTopology_GetSourceNodeCollection(topology, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_GetSourceNodeCollection(topology, &collection);
    ok(hr == S_OK, "Failed to get source node collection, hr %#lx.\n", hr);
    ok(!!collection, "Unexpected object pointer.\n");

    hr = IMFTopology_GetSourceNodeCollection(topology, &collection2);
    ok(hr == S_OK, "Failed to get source node collection, hr %#lx.\n", hr);
    ok(!!collection2, "Unexpected object pointer.\n");
    ok(collection2 != collection, "Expected cloned collection.\n");

    hr = IMFCollection_GetElementCount(collection, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);
    ok(!size, "Unexpected item count.\n");

    EXPECT_REF(collection, 1);
    hr = IMFCollection_AddElement(collection, (IUnknown *)collection);
    ok(hr == S_OK, "Failed to add element, hr %#lx.\n", hr);
    EXPECT_REF(collection, 2);

    hr = IMFCollection_GetElementCount(collection, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);
    ok(size == 1, "Unexpected item count.\n");

    /* Empty collection to stop referencing itself */
    hr = IMFCollection_RemoveAllElements(collection);
    ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);

    hr = IMFCollection_GetElementCount(collection2, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);
    ok(!size, "Unexpected item count.\n");

    ref = IMFCollection_Release(collection2);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFCollection_Release(collection);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Add some nodes. */
    hr = IMFTopology_GetSourceNodeCollection(topology, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_GetOutputNodeCollection(topology, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);
    IMFTopologyNode_Release(node);

    hr = IMFTopology_GetSourceNodeCollection(topology, &collection);
    ok(hr == S_OK, "Failed to get source node collection, hr %#lx.\n", hr);
    ok(!!collection, "Unexpected object pointer.\n");
    hr = IMFCollection_GetElementCount(collection, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);
    ok(size == 1, "Unexpected item count.\n");
    ref = IMFCollection_Release(collection);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);
    IMFTopologyNode_Release(node);

    hr = IMFTopology_GetSourceNodeCollection(topology, &collection);
    ok(hr == S_OK, "Failed to get source node collection, hr %#lx.\n", hr);
    ok(!!collection, "Unexpected object pointer.\n");
    hr = IMFCollection_GetElementCount(collection, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);
    ok(size == 1, "Unexpected item count.\n");
    ref = IMFCollection_Release(collection);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);
    IMFTopologyNode_Release(node);

    hr = IMFTopology_GetSourceNodeCollection(topology, &collection);
    ok(hr == S_OK, "Failed to get source node collection, hr %#lx.\n", hr);
    ok(!!collection, "Unexpected object pointer.\n");
    hr = IMFCollection_GetElementCount(collection, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);
    ok(size == 1, "Unexpected item count.\n");
    ref = IMFCollection_Release(collection);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    /* Associated object. */
    hr = IMFTopologyNode_SetObject(node, NULL);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetObject(node, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    object = (void *)0xdeadbeef;
    hr = IMFTopologyNode_GetObject(node, &object);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(!object, "Unexpected object %p.\n", object);

    hr = IMFTopologyNode_SetObject(node, &test_unk);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetObject(node, &object);
    ok(hr == S_OK, "Failed to get object, hr %#lx.\n", hr);
    ok(object == &test_unk, "Unexpected object %p.\n", object);
    IUnknown_Release(object);

    hr = IMFTopologyNode_SetObject(node, &test_unk2);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetCount(node, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
    ok(count == 0, "Unexpected attribute count %u.\n", count);

    hr = IMFTopologyNode_SetGUID(node, &MF_TOPONODE_TRANSFORM_OBJECTID, &MF_TOPONODE_TRANSFORM_OBJECTID);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetObject(node, NULL);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);

    object = (void *)0xdeadbeef;
    hr = IMFTopologyNode_GetObject(node, &object);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(!object, "Unexpected object %p.\n", object);

    hr = IMFTopologyNode_GetCount(node, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    /* Preferred stream types. */
    hr = IMFTopologyNode_GetInputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get input count, hr %#lx.\n", hr);
    ok(io_count == 0, "Unexpected count %lu.\n", io_count);

    hr = IMFTopologyNode_GetInputPrefType(node, 0, &mediatype);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 0, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputPrefType(node, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get preferred type, hr %#lx.\n", hr);
    ok(mediatype2 == mediatype, "Unexpected mediatype instance.\n");
    IMFMediaType_Release(mediatype2);

    hr = IMFTopologyNode_SetInputPrefType(node, 0, NULL);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputPrefType(node, 0, &mediatype2);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(!mediatype2, "Unexpected mediatype instance.\n");

    hr = IMFTopologyNode_SetInputPrefType(node, 1, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 1, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get input count, hr %#lx.\n", hr);
    ok(io_count == 2, "Unexpected count %lu.\n", io_count);

    hr = IMFTopologyNode_GetOutputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get input count, hr %#lx.\n", hr);
    ok(io_count == 0, "Unexpected count %lu.\n", io_count);

    hr = IMFTopologyNode_SetOutputPrefType(node, 0, mediatype);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 1, "Release returned %ld\n", ref);

    /* Source node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 0, mediatype);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetOutputPrefType(node, 2, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetOutputPrefType(node, 0, &mediatype2);
    ok(hr == E_FAIL, "Failed to get preferred type, hr %#lx.\n", hr);
    ok(!mediatype2, "Unexpected mediatype instance.\n");

    hr = IMFTopologyNode_GetOutputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 3, "Unexpected count %lu.\n", io_count);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Tee node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 0, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputPrefType(node, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get preferred type, hr %#lx.\n", hr);
    ok(mediatype2 == mediatype, "Unexpected mediatype instance.\n");
    IMFMediaType_Release(mediatype2);

    hr = IMFTopologyNode_GetOutputPrefType(node, 0, &mediatype2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 0, "Unexpected count %lu.\n", io_count);

    hr = IMFTopologyNode_SetInputPrefType(node, 1, mediatype);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 3, mediatype);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetOutputPrefType(node, 4, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetOutputPrefType(node, 0, &mediatype2);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&mediatype2);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    /* Changing output type does not change input type. */
    hr = IMFTopologyNode_SetOutputPrefType(node, 4, mediatype2);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputPrefType(node, 0, &mediatype3);
    ok(hr == S_OK, "Failed to get preferred type, hr %#lx.\n", hr);
    ok(mediatype3 == mediatype, "Unexpected mediatype instance.\n");
    IMFMediaType_Release(mediatype3);

    IMFMediaType_Release(mediatype2);

    hr = IMFTopologyNode_GetInputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 0, "Unexpected count %lu.\n", io_count);

    hr = IMFTopologyNode_GetOutputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 5, "Unexpected count %lu.\n", io_count);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Transform node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 3, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get input count, hr %#lx.\n", hr);
    ok(io_count == 4, "Unexpected count %lu.\n", io_count);

    hr = IMFTopologyNode_SetOutputPrefType(node, 4, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 4, "Unexpected count %lu.\n", io_count);

    hr = IMFTopologyNode_GetOutputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 5, "Unexpected count %lu.\n", io_count);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);

    IMFMediaType_Release(mediatype);

    hr = IMFTopology_GetOutputNodeCollection(topology, &collection);
    ok(hr == S_OK || broken(hr == E_FAIL) /* before Win8 */, "Failed to get output node collection, hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(!!collection, "Unexpected object pointer.\n");
        hr = IMFCollection_GetElementCount(collection, &size);
        ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);
        ok(size == 1, "Unexpected item count.\n");
        ref = IMFCollection_Release(collection);
        ok(ref == 0, "Release returned %ld\n", ref);
    }

    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Connect nodes. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 1);

    hr = IMFTopologyNode_ConnectOutput(node, 0, node2, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    EXPECT_REF(node, 2);
    EXPECT_REF(node2, 2);

    IMFTopologyNode_Release(node);

    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 2);

    IMFTopologyNode_Release(node2);

    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 1);

    hr = IMFTopologyNode_GetNodeType(node2, &node_type);
    ok(hr == S_OK, "Failed to get node type, hr %#lx.\n", hr);

    IMFTopologyNode_Release(node);

    /* Connect within topology. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    EXPECT_REF(node, 2);
    EXPECT_REF(node2, 2);

    hr = IMFTopologyNode_ConnectOutput(node, 0, node2, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    EXPECT_REF(node, 3);
    EXPECT_REF(node2, 3);

    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#lx.\n", hr);

    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 1);

    /* Removing connected node breaks connection. */
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_ConnectOutput(node, 0, node2, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    hr = IMFTopology_RemoveNode(topology, node);
    ok(hr == S_OK, "Failed to remove a node, hr %#lx.\n", hr);

    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 2);

    hr = IMFTopologyNode_GetOutput(node, 0, &node3, &index);
    ok(hr == MF_E_NOT_FOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_ConnectOutput(node, 0, node2, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    hr = IMFTopology_RemoveNode(topology, node2);
    ok(hr == S_OK, "Failed to remove a node, hr %#lx.\n", hr);

    EXPECT_REF(node, 2);
    EXPECT_REF(node2, 1);

    IMFTopologyNode_Release(node);
    IMFTopologyNode_Release(node2);

    /* Cloning nodes of different types. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_CloneFrom(node, node2);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    ref = IMFTopologyNode_Release(node2);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Cloning preferred types. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetOutputPrefType(node2, 0, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    /* Vista checks for additional attributes. */
    hr = IMFTopologyNode_CloneFrom(node, node2);
    ok(hr == S_OK || broken(hr == MF_E_ATTRIBUTENOTFOUND) /* Vista */, "Failed to clone a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetOutputPrefType(node, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get preferred type, hr %#lx.\n", hr);
    ok(mediatype == mediatype2, "Unexpected media type.\n");

    IMFMediaType_Release(mediatype2);

    ref = IMFTopologyNode_Release(node2);
    ok(ref == 0, "Release returned %ld\n", ref);

    IMFMediaType_Release(mediatype);

    /* Existing preferred types are not cleared. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetOutputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 1, "Unexpected output count.\n");

    hr = IMFTopologyNode_CloneFrom(node, node2);
    ok(hr == S_OK || broken(hr == MF_E_ATTRIBUTENOTFOUND) /* Vista */, "Failed to clone a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetOutputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 1, "Unexpected output count.\n");

    hr = IMFTopologyNode_GetOutputPrefType(node, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get preferred type, hr %#lx.\n", hr);
    ok(!!mediatype2, "Unexpected media type.\n");
    IMFMediaType_Release(mediatype2);

    hr = IMFTopologyNode_CloneFrom(node2, node);
    ok(hr == S_OK || broken(hr == MF_E_ATTRIBUTENOTFOUND) /* Vista */, "Failed to clone a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetOutputCount(node2, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 1, "Unexpected output count.\n");

    ref = IMFTopologyNode_Release(node2);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Add one node, connect to another that hasn't been added. */
    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 1, "Unexpected node count.\n");

    hr = IMFTopologyNode_ConnectOutput(node, 0, node2, 0);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 1, "Unexpected node count.\n");

    /* Add same node to different topologies. */
    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#lx.\n", hr);

    hr = MFCreateTopology(&topology2);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);
    EXPECT_REF(node, 2);

    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 1, "Unexpected node count.\n");

    hr = IMFTopology_GetNodeCount(topology2, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 0, "Unexpected node count.\n");

    hr = IMFTopology_AddNode(topology2, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);
    EXPECT_REF(node, 3);

    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 1, "Unexpected node count.\n");

    hr = IMFTopology_GetNodeCount(topology2, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 1, "Unexpected node count.\n");

    ref = IMFTopology_Release(topology2);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(node2);
    ok(ref == 0, "Release returned %ld\n", ref);
}

static void test_topology_tee_node(void)
{
    IMFTopologyNode *src_node, *tee_node;
    IMFMediaType *mediatype, *mediatype2;
    IMFTopology *topology;
    DWORD count;
    HRESULT hr;
    LONG ref;

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &tee_node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &src_node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(tee_node, 0, mediatype);
    ok(hr == S_OK, "Failed to set type, hr %#lx.\n", hr);

    /* Even though tee node has only one input and source has only one output,
       it's possible to connect to higher inputs/outputs. */

    /* SRC(0) -> TEE(0) */
    hr = IMFTopologyNode_ConnectOutput(src_node, 0, tee_node, 0);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputCount(tee_node, &count);
    ok(hr == S_OK, "Failed to get count, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %lu.\n", count);

    hr = IMFTopologyNode_GetInputPrefType(tee_node, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get type, hr %#lx.\n", hr);
    ok(mediatype2 == mediatype, "Unexpected type.\n");
    IMFMediaType_Release(mediatype2);

    /* SRC(0) -> TEE(1) */
    hr = IMFTopologyNode_ConnectOutput(src_node, 0, tee_node, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputCount(tee_node, &count);
    ok(hr == S_OK, "Failed to get count, hr %#lx.\n", hr);
    ok(count == 2, "Unexpected count %lu.\n", count);

    hr = IMFTopologyNode_SetInputPrefType(tee_node, 1, mediatype);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#lx.\n", hr);

    /* SRC(1) -> TEE(1) */
    hr = IMFTopologyNode_ConnectOutput(src_node, 1, tee_node, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetOutputCount(src_node, &count);
    ok(hr == S_OK, "Failed to get count, hr %#lx.\n", hr);
    ok(count == 2, "Unexpected count %lu.\n", count);

    EXPECT_REF(src_node, 2);
    EXPECT_REF(tee_node, 2);
    hr = IMFTopologyNode_DisconnectOutput(src_node, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ref = IMFTopologyNode_Release(src_node);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(tee_node);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFMediaType_Release(mediatype);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);
}

static HRESULT WINAPI test_getservice_QI(IMFGetService *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFGetService) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_getservice_AddRef(IMFGetService *iface)
{
    return 2;
}

static ULONG WINAPI test_getservice_Release(IMFGetService *iface)
{
    return 1;
}

static HRESULT WINAPI test_getservice_GetService(IMFGetService *iface, REFGUID service, REFIID riid, void **obj)
{
    *obj = (void *)0xdeadbeef;
    return 0x83eddead;
}

static const IMFGetServiceVtbl testmfgetservicevtbl =
{
    test_getservice_QI,
    test_getservice_AddRef,
    test_getservice_Release,
    test_getservice_GetService,
};

static IMFGetService test_getservice = { &testmfgetservicevtbl };

static HRESULT WINAPI testservice_QI(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IMFGetService))
        return 0x82eddead;

    return E_NOINTERFACE;
}

static HRESULT WINAPI testservice2_QI(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        return S_OK;
    }

    if (IsEqualIID(riid, &IID_IMFGetService))
    {
        *obj = &test_getservice;
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testservice_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI testservice_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl testservicevtbl =
{
    testservice_QI,
    testservice_AddRef,
    testservice_Release,
};

static const IUnknownVtbl testservice2vtbl =
{
    testservice2_QI,
    testservice_AddRef,
    testservice_Release,
};

static IUnknown testservice = { &testservicevtbl };
static IUnknown testservice2 = { &testservice2vtbl };

static void test_MFGetService(void)
{
    IUnknown *unk;
    HRESULT hr;

    hr = MFGetService(NULL, NULL, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected return value %#lx.\n", hr);

    unk = (void *)0xdeadbeef;
    hr = MFGetService(NULL, NULL, NULL, (void **)&unk);
    ok(hr == E_POINTER, "Unexpected return value %#lx.\n", hr);
    ok(unk == (void *)0xdeadbeef, "Unexpected out object.\n");

    hr = MFGetService(&testservice, NULL, NULL, NULL);
    ok(hr == 0x82eddead, "Unexpected return value %#lx.\n", hr);

    unk = (void *)0xdeadbeef;
    hr = MFGetService(&testservice, NULL, NULL, (void **)&unk);
    ok(hr == 0x82eddead, "Unexpected return value %#lx.\n", hr);
    ok(unk == (void *)0xdeadbeef, "Unexpected out object.\n");

    unk = NULL;
    hr = MFGetService(&testservice2, NULL, NULL, (void **)&unk);
    ok(hr == 0x83eddead, "Unexpected return value %#lx.\n", hr);
    ok(unk == (void *)0xdeadbeef, "Unexpected out object.\n");
}

static void test_sequencer_source(void)
{
    IMFSequencerSource *seq_source;
    HRESULT hr;
    LONG ref;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#lx.\n", hr);

    hr = MFCreateSequencerSource(NULL, &seq_source);
    ok(hr == S_OK, "Failed to create sequencer source, hr %#lx.\n", hr);

    check_interface(seq_source, &IID_IMFMediaSourceTopologyProvider, TRUE);

    ref = IMFSequencerSource_Release(seq_source);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#lx.\n", hr);
}

struct test_handler
{
    IMFMediaTypeHandler IMFMediaTypeHandler_iface;

    ULONG set_current_count;
    IMFMediaType *current_type;
    IMFMediaType *invalid_type;

    ULONG enum_count;
    ULONG media_types_count;
    IMFMediaType **media_types;
};

static struct test_handler *impl_from_IMFMediaTypeHandler(IMFMediaTypeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct test_handler, IMFMediaTypeHandler_iface);
}

static HRESULT WINAPI test_handler_QueryInterface(IMFMediaTypeHandler *iface, REFIID riid, void **obj)
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

static ULONG WINAPI test_handler_AddRef(IMFMediaTypeHandler *iface)
{
    return 2;
}

static ULONG WINAPI test_handler_Release(IMFMediaTypeHandler *iface)
{
    return 1;
}

static HRESULT WINAPI test_handler_IsMediaTypeSupported(IMFMediaTypeHandler *iface, IMFMediaType *in_type,
        IMFMediaType **out_type)
{
    struct test_handler *impl = impl_from_IMFMediaTypeHandler(iface);
    BOOL result;

    if (out_type)
        *out_type = NULL;

    if (impl->invalid_type && IMFMediaType_Compare(impl->invalid_type, (IMFAttributes *)in_type,
            MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result) == S_OK && result)
        return MF_E_INVALIDMEDIATYPE;

    if (!impl->current_type)
        return S_OK;

    if (IMFMediaType_Compare(impl->current_type, (IMFAttributes *)in_type,
            MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result) == S_OK && result)
        return S_OK;

    return MF_E_INVALIDMEDIATYPE;
}

static HRESULT WINAPI test_handler_GetMediaTypeCount(IMFMediaTypeHandler *iface, DWORD *count)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_handler_GetMediaTypeByIndex(IMFMediaTypeHandler *iface, DWORD index,
        IMFMediaType **type)
{
    struct test_handler *impl = impl_from_IMFMediaTypeHandler(iface);

    if (impl->media_types)
    {
        impl->enum_count++;

        if (index >= impl->media_types_count)
            return MF_E_NO_MORE_TYPES;

        IMFMediaType_AddRef((*type = impl->media_types[index]));
        return S_OK;
    }

    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_handler_SetCurrentMediaType(IMFMediaTypeHandler *iface, IMFMediaType *media_type)
{
    struct test_handler *impl = impl_from_IMFMediaTypeHandler(iface);

    if (impl->current_type)
        IMFMediaType_Release(impl->current_type);
    IMFMediaType_AddRef((impl->current_type = media_type));
    impl->set_current_count++;

    return S_OK;
}

static HRESULT WINAPI test_handler_GetCurrentMediaType(IMFMediaTypeHandler *iface, IMFMediaType **media_type)
{
    struct test_handler *impl = impl_from_IMFMediaTypeHandler(iface);
    HRESULT hr;

    if (!impl->current_type)
    {
        if (!impl->media_types)
            return E_FAIL;
        if (!impl->media_types_count)
            return MF_E_TRANSFORM_TYPE_NOT_SET;
        return MF_E_NOT_INITIALIZED;
    }

    if (FAILED(hr = MFCreateMediaType(media_type)))
        return hr;

    hr = IMFMediaType_CopyAllItems(impl->current_type, (IMFAttributes *)*media_type);
    if (FAILED(hr))
        IMFMediaType_Release(*media_type);

    return hr;
}

static HRESULT WINAPI test_handler_GetMajorType(IMFMediaTypeHandler *iface, GUID *type)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IMFMediaTypeHandlerVtbl test_handler_vtbl =
{
    test_handler_QueryInterface,
    test_handler_AddRef,
    test_handler_Release,
    test_handler_IsMediaTypeSupported,
    test_handler_GetMediaTypeCount,
    test_handler_GetMediaTypeByIndex,
    test_handler_SetCurrentMediaType,
    test_handler_GetCurrentMediaType,
    test_handler_GetMajorType,
};

static const struct test_handler test_handler = {.IMFMediaTypeHandler_iface.lpVtbl = &test_handler_vtbl};

struct test_media_sink
{
    IMFMediaSink IMFMediaSink_iface;
    BOOL shutdown;
};

static struct test_media_sink *impl_from_IMFMediaSink(IMFMediaSink *iface)
{
    return CONTAINING_RECORD(iface, struct test_media_sink, IMFMediaSink_iface);
}

static HRESULT WINAPI test_media_sink_QueryInterface(IMFMediaSink *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFMediaSink)
            || IsEqualIID(riid, &IID_IUnknown))
    {
        IMFMediaSink_AddRef((*obj = iface));
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_media_sink_AddRef(IMFMediaSink *iface)
{
    return 2;
}

static ULONG WINAPI test_media_sink_Release(IMFMediaSink *iface)
{
    return 1;
}

static HRESULT WINAPI test_media_sink_GetCharacteristics(IMFMediaSink *iface, DWORD *characteristics)
{
    *characteristics = 0;
    return S_OK;
}

static HRESULT WINAPI test_media_sink_AddStreamSink(IMFMediaSink *iface,
        DWORD stream_sink_id, IMFMediaType *media_type, IMFStreamSink **stream_sink)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_media_sink_RemoveStreamSink(IMFMediaSink *iface, DWORD stream_sink_id)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_media_sink_GetStreamSinkCount(IMFMediaSink *iface, DWORD *count)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_media_sink_GetStreamSinkByIndex(IMFMediaSink *iface, DWORD index, IMFStreamSink **sink)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_media_sink_GetStreamSinkById(IMFMediaSink *iface, DWORD stream_sink_id, IMFStreamSink **sink)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_media_sink_SetPresentationClock(IMFMediaSink *iface, IMFPresentationClock *clock)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_media_sink_GetPresentationClock(IMFMediaSink *iface, IMFPresentationClock **clock)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_media_sink_Shutdown(IMFMediaSink *iface)
{
    struct test_media_sink *sink = impl_from_IMFMediaSink(iface);
    ok(!sink->shutdown, "Unexpected call.\n");
    sink->shutdown = TRUE;
    return S_OK;
}

static const IMFMediaSinkVtbl test_media_sink_vtbl =
{
    test_media_sink_QueryInterface,
    test_media_sink_AddRef,
    test_media_sink_Release,
    test_media_sink_GetCharacteristics,
    test_media_sink_AddStreamSink,
    test_media_sink_RemoveStreamSink,
    test_media_sink_GetStreamSinkCount,
    test_media_sink_GetStreamSinkByIndex,
    test_media_sink_GetStreamSinkById,
    test_media_sink_SetPresentationClock,
    test_media_sink_GetPresentationClock,
    test_media_sink_Shutdown,
};

static const struct test_media_sink test_media_sink = {.IMFMediaSink_iface.lpVtbl = &test_media_sink_vtbl};

struct test_stream_sink
{
    IMFStreamSink IMFStreamSink_iface;
    IMFMediaTypeHandler *handler;
    IMFMediaSink *media_sink;
};

static struct test_stream_sink *impl_from_IMFStreamSink(IMFStreamSink *iface)
{
    return CONTAINING_RECORD(iface, struct test_stream_sink, IMFStreamSink_iface);
}

static HRESULT WINAPI test_stream_sink_QueryInterface(IMFStreamSink *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFStreamSink)
            || IsEqualIID(riid, &IID_IMFMediaEventGenerator)
            || IsEqualIID(riid, &IID_IUnknown))
    {
        IMFStreamSink_AddRef((*obj = iface));
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_stream_sink_AddRef(IMFStreamSink *iface)
{
    return 2;
}

static ULONG WINAPI test_stream_sink_Release(IMFStreamSink *iface)
{
    return 1;
}

static HRESULT WINAPI test_stream_sink_GetEvent(IMFStreamSink *iface, DWORD flags, IMFMediaEvent **event)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_BeginGetEvent(IMFStreamSink *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_EndGetEvent(IMFStreamSink *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_QueueEvent(IMFStreamSink *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_GetMediaSink(IMFStreamSink *iface, IMFMediaSink **sink)
{
    struct test_stream_sink *impl = impl_from_IMFStreamSink(iface);

    if (impl->media_sink)
    {
        IMFMediaSink_AddRef((*sink = impl->media_sink));
        return S_OK;
    }

    todo_wine
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_GetIdentifier(IMFStreamSink *iface, DWORD *id)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_GetMediaTypeHandler(IMFStreamSink *iface, IMFMediaTypeHandler **handler)
{
    struct test_stream_sink *impl = impl_from_IMFStreamSink(iface);

    if (impl->handler)
    {
        IMFMediaTypeHandler_AddRef((*handler = impl->handler));
        return S_OK;
    }

    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_ProcessSample(IMFStreamSink *iface, IMFSample *sample)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_PlaceMarker(IMFStreamSink *iface, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *marker_value, const PROPVARIANT *context)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_Flush(IMFStreamSink *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IMFStreamSinkVtbl test_stream_sink_vtbl =
{
    test_stream_sink_QueryInterface,
    test_stream_sink_AddRef,
    test_stream_sink_Release,
    test_stream_sink_GetEvent,
    test_stream_sink_BeginGetEvent,
    test_stream_sink_EndGetEvent,
    test_stream_sink_QueueEvent,
    test_stream_sink_GetMediaSink,
    test_stream_sink_GetIdentifier,
    test_stream_sink_GetMediaTypeHandler,
    test_stream_sink_ProcessSample,
    test_stream_sink_PlaceMarker,
    test_stream_sink_Flush,
};

static const struct test_stream_sink test_stream_sink = {.IMFStreamSink_iface.lpVtbl = &test_stream_sink_vtbl};

struct test_callback
{
    IMFAsyncCallback IMFAsyncCallback_iface;
    LONG refcount;

    HANDLE event;
    IMFMediaEvent *media_event;
};

static struct test_callback *impl_from_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct test_callback, IMFAsyncCallback_iface);
}

static HRESULT WINAPI testcallback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testcallback_AddRef(IMFAsyncCallback *iface)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    return InterlockedIncrement(&callback->refcount);
}

static ULONG WINAPI testcallback_Release(IMFAsyncCallback *iface)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    ULONG refcount = InterlockedDecrement(&callback->refcount);

    if (!refcount)
    {
        if (callback->media_event)
            IMFMediaEvent_Release(callback->media_event);
        CloseHandle(callback->event);
        free(callback);
    }

    return refcount;
}

static HRESULT WINAPI testcallback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    ok(flags != NULL && queue != NULL, "Unexpected arguments.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testcallback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct test_callback *callback = CONTAINING_RECORD(iface, struct test_callback, IMFAsyncCallback_iface);
    IUnknown *object;
    HRESULT hr;

    ok(result != NULL, "Unexpected result object.\n");

    if (callback->media_event)
        IMFMediaEvent_Release(callback->media_event);

    hr = IMFAsyncResult_GetObject(result, &object);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFAsyncResult_GetState(result, &object);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaEventGenerator_EndGetEvent((IMFMediaEventGenerator *)object,
            result, &callback->media_event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IUnknown_Release(object);

    SetEvent(callback->event);

    return S_OK;
}

static const IMFAsyncCallbackVtbl testcallbackvtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    testcallback_Invoke,
};

static IMFAsyncCallback *create_test_callback(void)
{
    struct test_callback *callback;

    if (!(callback = calloc(1, sizeof(*callback))))
        return NULL;

    callback->refcount = 1;
    callback->IMFAsyncCallback_iface.lpVtbl = &testcallbackvtbl;
    callback->event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!callback->event, "CreateEventW failed, error %lu\n", GetLastError());

    return &callback->IMFAsyncCallback_iface;
}

#define wait_media_event(a, b, c, d, e) wait_media_event_(__LINE__, a, b, c, d, e)
static HRESULT wait_media_event_(int line, IMFMediaSession *session, IMFAsyncCallback *callback,
        MediaEventType expect_type, DWORD timeout, PROPVARIANT *value)
{
    struct test_callback *impl = impl_from_IMFAsyncCallback(callback);
    MediaEventType type;
    HRESULT hr, status;
    DWORD ret;
    GUID guid;

    do
    {
        hr = IMFMediaSession_BeginGetEvent(session, &impl->IMFAsyncCallback_iface, (IUnknown *)session);
        ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ret = WaitForSingleObject(impl->event, timeout);
        ok_(__FILE__, line)(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", ret);
        hr = IMFMediaEvent_GetType(impl->media_event, &type);
        ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    } while (type != expect_type);

    ok_(__FILE__, line)(type == expect_type, "got type %lu\n", type);

    hr = IMFMediaEvent_GetExtendedType(impl->media_event, &guid);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)(IsEqualGUID(&guid, &GUID_NULL), "got extended type %s\n", debugstr_guid(&guid));

    hr = IMFMediaEvent_GetValue(impl->media_event, value);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEvent_GetStatus(impl->media_event, &status);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    return status;
}

static IMFMediaSource *create_media_source(const WCHAR *name, const WCHAR *mime)
{
    IMFSourceResolver *resolver;
    IMFAttributes *attributes;
    const BYTE *resource_data;
    MF_OBJECT_TYPE obj_type;
    IMFMediaSource *source;
    IMFByteStream *stream;
    ULONG resource_len;
    HRSRC resource;
    HRESULT hr;

    resource = FindResourceW(NULL, name, (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW %s failed, error %lu\n", debugstr_w(name), GetLastError());
    resource_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    resource_len = SizeofResource(GetModuleHandleW(NULL), resource);

    hr = MFCreateTempFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_DELETE_IF_EXIST, MF_FILEFLAGS_NONE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFByteStream_Write(stream, resource_data, resource_len, &resource_len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFByteStream_SetCurrentPosition(stream, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFByteStream_QueryInterface(stream, &IID_IMFAttributes, (void **)&attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetString(attributes, &MF_BYTESTREAM_CONTENT_TYPE, mime);
    ok(hr == S_OK, "Failed to set string value, hr %#lx.\n", hr);
    IMFAttributes_Release(attributes);

    hr = MFCreateSourceResolver(&resolver);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
            &obj_type, (IUnknown **)&source);
    ok(hr == S_OK || broken(hr == MF_E_UNSUPPORTED_BYTESTREAM_TYPE), "Unexpected hr %#lx.\n", hr);
    IMFSourceResolver_Release(resolver);
    IMFByteStream_Release(stream);

    if (FAILED(hr))
        return NULL;

    ok(obj_type == MF_OBJECT_MEDIASOURCE, "got %d\n", obj_type);
    return source;
}

static void test_media_session_events(void)
{
    static const media_type_desc audio_float_44100 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 4 * 44100),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 4 * 8),
    };
    static const media_type_desc audio_pcm_48000 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 2 * 48000),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 2 * 8),
    };

    struct test_stream_sink stream_sink = test_stream_sink;
    struct test_media_sink media_sink = test_media_sink;
    struct test_handler handler = test_handler;
    IMFAsyncCallback *callback, *callback2;
    IMFMediaType *input_type, *output_type;
    IMFTopologyNode *src_node, *sink_node;
    IMFPresentationDescriptor *pd;
    IMFMediaSession *session;
    IMFStreamDescriptor *sd;
    IMFAsyncResult *result;
    IMFMediaSource *source;
    IMFTopology *topology;
    IMFMediaEvent *event;
    PROPVARIANT propvar;
    HRESULT hr;
    ULONG ref;

    stream_sink.handler = &handler.IMFMediaTypeHandler_iface;
    stream_sink.media_sink = &media_sink.IMFMediaSink_iface;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#lx.\n", hr);

    callback = create_test_callback();
    callback2 = create_test_callback();

    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Failed to create media session, hr %#lx.\n", hr);

    hr = IMFMediaSession_GetEvent(session, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_NO_EVENTS_AVAILABLE, "Unexpected hr %#lx.\n", hr);

    /* Async case. */
    hr = IMFMediaSession_BeginGetEvent(session, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_BeginGetEvent(session, callback, (IUnknown *)session);
    ok(hr == S_OK, "Failed to Begin*, hr %#lx.\n", hr);
    EXPECT_REF(callback, 2);

    /* Same callback, same state. */
    hr = IMFMediaSession_BeginGetEvent(session, callback, (IUnknown *)session);
    ok(hr == MF_S_MULTIPLE_BEGIN, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(callback, 2);

    /* Same callback, different state. */
    hr = IMFMediaSession_BeginGetEvent(session, callback, (IUnknown *)callback);
    ok(hr == MF_E_MULTIPLE_BEGIN, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(callback, 2);

    /* Different callback, same state. */
    hr = IMFMediaSession_BeginGetEvent(session, callback2, (IUnknown *)session);
    ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(callback2, 1);

    /* Different callback, different state. */
    hr = IMFMediaSession_BeginGetEvent(session, callback2, (IUnknown *)callback);
    ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(callback, 2);

    hr = MFCreateAsyncResult(NULL, callback, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);

    hr = IMFMediaSession_EndGetEvent(session, result, &event);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    /* Shutdown behavior. */
    hr = IMFMediaSession_Shutdown(session);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
    IMFMediaSession_Release(session);

    /* Shutdown leaks callback */
    EXPECT_REF(callback, 2);
    EXPECT_REF(callback2, 1);

    IMFAsyncCallback_Release(callback);
    IMFAsyncCallback_Release(callback2);


    callback = create_test_callback();

    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    propvar.vt = VT_EMPTY;
    hr = IMFMediaSession_Start(session, &GUID_NULL, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionStarted, 1000, &propvar);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_EMPTY, "got vt %u\n", propvar.vt);

    hr = IMFMediaSession_Stop(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionStopped, 1000, &propvar);
    todo_wine
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_EMPTY, "got vt %u\n", propvar.vt);

    hr = IMFMediaSession_Pause(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionPaused, 1000, &propvar);
    todo_wine
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_EMPTY, "got vt %u\n", propvar.vt);

    hr = IMFMediaSession_ClearTopologies(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionTopologiesCleared, 1000, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_EMPTY, "got vt %u\n", propvar.vt);

    hr = IMFMediaSession_ClearTopologies(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionTopologiesCleared, 1000, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_EMPTY, "got vt %u\n", propvar.vt);

    hr = IMFMediaSession_Close(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionClosed, 1000, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_EMPTY, "got vt %u\n", propvar.vt);

    hr = IMFMediaSession_ClearTopologies(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionTopologiesCleared, 1000, &propvar);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_EMPTY, "got vt %u\n", propvar.vt);

    hr = IMFMediaSession_Shutdown(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* sometimes briefly leaking */
    IMFMediaSession_Release(session);


    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&input_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(input_type, audio_float_44100, -1);
    create_descriptors(1, &input_type, NULL, &pd, &sd);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &sink_node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_sink_node(&stream_sink.IMFStreamSink_iface, -1, sink_node);

    hr = MFCreateMediaType(&output_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(output_type, audio_pcm_48000, -1);
    handler.media_types_count = 1;
    handler.media_types = &output_type;

    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &src_node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_source_node(NULL, -1, src_node, pd, sd);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, sink_node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, src_node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopologyNode_ConnectOutput(src_node, 0, sink_node, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_SetTopology(session, 0, topology);
    todo_wine
    ok(hr == MF_E_TOPO_MISSING_SOURCE, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = wait_media_event(session, callback, MESessionTopologySet, 1000, &propvar);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        PropVariantClear(&propvar);
        handler.enum_count = handler.set_current_count = 0;
    }

    source = create_test_source(pd);
    init_source_node(source, -1, src_node, pd, sd);

    hr = IMFMediaSession_SetTopology(session, 0, topology);
    todo_wine
    ok(hr == MF_E_TOPO_STREAM_DESCRIPTOR_NOT_SELECTED, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = wait_media_event(session, callback, MESessionTopologySet, 1000, &propvar);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        PropVariantClear(&propvar);
        handler.enum_count = handler.set_current_count = 0;
    }

    hr = IMFMediaSession_ClearTopologies(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionTopologiesCleared, 1000, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_EMPTY, "got vt %u\n", propvar.vt);

    hr = IMFMediaSession_Shutdown(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!media_sink.shutdown, "media sink is shutdown.\n");
    media_sink.shutdown = FALSE;

    /* sometimes briefly leaking */
    IMFMediaSession_Release(session);

    if (handler.current_type)
        IMFMediaType_Release(handler.current_type);
    handler.current_type = NULL;


    /* SetTopology without a current output type */

    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFPresentationDescriptor_SelectStream(pd, 0);

    hr = IMFMediaSession_SetTopology(session, MFSESSION_SETTOPOLOGY_NORESOLUTION, topology);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionTopologySet, 1000, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_UNKNOWN, "got vt %u\n", propvar.vt);
    ok(propvar.punkVal == (IUnknown *)topology, "got punkVal %p\n", propvar.punkVal);
    PropVariantClear(&propvar);

    ok(!handler.enum_count, "got %lu GetMediaTypeByIndex\n", handler.enum_count);
    ok(!handler.set_current_count, "got %lu SetCurrentMediaType\n", handler.set_current_count);
    handler.enum_count = handler.set_current_count = 0;

    hr = IMFMediaSession_ClearTopologies(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionTopologiesCleared, 1000, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_EMPTY, "got vt %u\n", propvar.vt);

    hr = IMFMediaSession_Shutdown(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_sink.shutdown, "media sink didn't shutdown.\n");
    media_sink.shutdown = FALSE;

    /* sometimes briefly leaking */
    IMFMediaSession_Release(session);

    if (handler.current_type)
        IMFMediaType_Release(handler.current_type);
    handler.current_type = NULL;


    /* SetTopology without a current output type */

    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_SetTopology(session, 0, topology);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionTopologySet, 1000, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_UNKNOWN, "got vt %u\n", propvar.vt);
    ok(propvar.punkVal != (IUnknown *)topology, "got punkVal %p\n", propvar.punkVal);
    PropVariantClear(&propvar);

    todo_wine
    ok(!handler.enum_count, "got %lu GetMediaTypeByIndex\n", handler.enum_count);
    ok(handler.set_current_count, "got %lu SetCurrentMediaType\n", handler.set_current_count);
    handler.enum_count = handler.set_current_count = 0;

    hr = IMFMediaSession_ClearTopologies(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionTopologiesCleared, 1000, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_EMPTY, "got vt %u\n", propvar.vt);

    hr = IMFMediaSession_Shutdown(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_sink.shutdown, "media sink didn't shutdown.\n");
    media_sink.shutdown = FALSE;

    /* sometimes briefly leaking */
    IMFMediaSession_Release(session);

    if (handler.current_type)
        IMFMediaType_Release(handler.current_type);
    handler.current_type = NULL;


    /* SetTopology without a current output type, refusing input type */

    handler.invalid_type = input_type;

    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_SetTopology(session, 0, topology);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionTopologySet, 1000, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_UNKNOWN, "got vt %u\n", propvar.vt);
    ok(propvar.punkVal != (IUnknown *)topology, "got punkVal %p\n", propvar.punkVal);
    PropVariantClear(&propvar);

    ok(handler.enum_count, "got %lu GetMediaTypeByIndex\n", handler.enum_count);
    todo_wine
    ok(handler.set_current_count, "got %lu SetCurrentMediaType\n", handler.set_current_count);
    handler.enum_count = handler.set_current_count = 0;

    hr = IMFMediaSession_ClearTopologies(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionTopologiesCleared, 1000, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_EMPTY, "got vt %u\n", propvar.vt);

    hr = IMFMediaSession_Shutdown(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_sink.shutdown, "media sink didn't shutdown.\n");
    media_sink.shutdown = FALSE;

    /* sometimes briefly leaking */
    IMFMediaSession_Release(session);

    if (handler.current_type)
        IMFMediaType_Release(handler.current_type);
    handler.current_type = NULL;


    /* SetTopology without a current output type, refusing input type, requiring a converter */

    handler.media_types_count = 0;
    handler.invalid_type = input_type;

    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_SetTopology(session, 0, topology);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionTopologySet, 1000, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_UNKNOWN, "got vt %u\n", propvar.vt);
    ok(propvar.punkVal != (IUnknown *)topology, "got punkVal %p\n", propvar.punkVal);
    PropVariantClear(&propvar);

    ok(!handler.enum_count, "got %lu GetMediaTypeByIndex\n", handler.enum_count);
    ok(handler.set_current_count, "got %lu SetCurrentMediaType\n", handler.set_current_count);
    handler.enum_count = handler.set_current_count = 0;

    hr = IMFMediaSession_ClearTopologies(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionTopologiesCleared, 1000, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_EMPTY, "got vt %u\n", propvar.vt);

    hr = IMFMediaSession_Shutdown(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_sink.shutdown, "media sink didn't shutdown.\n");
    media_sink.shutdown = FALSE;

    /* sometimes briefly leaking */
    IMFMediaSession_Release(session);

    if (handler.current_type)
        IMFMediaType_Release(handler.current_type);
    handler.current_type = NULL;


    /* SetTopology with a current output type */

    handler.media_types_count = 1;
    IMFMediaType_AddRef((handler.current_type = output_type));

    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_SetTopology(session, 0, topology);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionTopologySet, 1000, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_UNKNOWN, "got vt %u\n", propvar.vt);
    ok(propvar.punkVal != (IUnknown *)topology, "got punkVal %p\n", propvar.punkVal);
    PropVariantClear(&propvar);

    ok(!handler.enum_count, "got %lu GetMediaTypeByIndex\n", handler.enum_count);
    todo_wine
    ok(handler.set_current_count, "got %lu SetCurrentMediaType\n", handler.set_current_count);
    handler.enum_count = handler.set_current_count = 0;

    hr = IMFMediaSession_ClearTopologies(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionTopologiesCleared, 1000, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_EMPTY, "got vt %u\n", propvar.vt);

    hr = IMFMediaSession_Shutdown(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_sink.shutdown, "media sink didn't shutdown.\n");
    media_sink.shutdown = FALSE;

    /* sometimes briefly leaking */
    IMFMediaSession_Release(session);

    IMFAsyncCallback_Release(callback);

    if (handler.current_type)
        IMFMediaType_Release(handler.current_type);
    handler.current_type = NULL;

    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ref = IMFTopologyNode_Release(src_node);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(sink_node);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFMediaSource_Release(source);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFPresentationDescriptor_Release(pd);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFStreamDescriptor_Release(sd);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(input_type);
    ok(ref == 0, "Release returned %ld\n", ref);


    hr = MFShutdown();
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
}

static void test_media_session(void)
{
    IMFRateSupport *rate_support;
    IMFAttributes *attributes;
    IMFMediaSession *session;
    MFSHUTDOWN_STATUS status;
    IMFTopology *topology;
    IMFShutdown *shutdown;
    PROPVARIANT propvar;
    IMFGetService *gs;
    IMFClock *clock;
    HRESULT hr;
    DWORD caps;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#lx.\n", hr);

    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Failed to create media session, hr %#lx.\n", hr);

    check_interface(session, &IID_IMFGetService, TRUE);
    check_interface(session, &IID_IMFRateSupport, TRUE);
    check_interface(session, &IID_IMFRateControl, TRUE);
    check_interface(session, &IID_IMFAttributes, FALSE);
    check_interface(session, &IID_IMFTopologyNodeAttributeEditor, FALSE);
    check_interface(session, &IID_IMFLocalMFTRegistration, FALSE);
    check_service_interface(session, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport, TRUE);
    check_service_interface(session, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateControl, TRUE);
    check_service_interface(session, &MF_TOPONODE_ATTRIBUTE_EDITOR_SERVICE, &IID_IMFTopologyNodeAttributeEditor, TRUE);
    check_service_interface(session, &MF_LOCAL_MFT_REGISTRATION_SERVICE, &IID_IMFLocalMFTRegistration, TRUE);

    hr = IMFMediaSession_GetClock(session, &clock);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFClock_QueryInterface(clock, &IID_IMFShutdown, (void **)&shutdown);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFShutdown_GetShutdownStatus(shutdown, &status);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_Shutdown(session);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    check_interface(session, &IID_IMFGetService, TRUE);

    hr = IMFMediaSession_QueryInterface(session, &IID_IMFGetService, (void **)&gs);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFGetService_GetService(gs, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport, (void **)&rate_support);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    IMFGetService_Release(gs);

    hr = IMFShutdown_GetShutdownStatus(shutdown, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(status == MFSHUTDOWN_COMPLETED, "Unexpected shutdown status %u.\n", status);

    IMFShutdown_Release(shutdown);

    hr = IMFMediaSession_ClearTopologies(session);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_Start(session, &GUID_NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    propvar.vt = VT_EMPTY;
    hr = IMFMediaSession_Start(session, &GUID_NULL, &propvar);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_Pause(session);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_Stop(session);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_Close(session);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_GetClock(session, &clock);
    ok(hr == MF_E_SHUTDOWN || broken(hr == E_UNEXPECTED) /* Win7 */, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_GetSessionCapabilities(session, &caps);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_GetSessionCapabilities(session, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_GetFullTopology(session, MFSESSION_GETFULLTOPOLOGY_CURRENT, 0, &topology);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_Shutdown(session);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    IMFMediaSession_Release(session);

    /* Custom topology loader, GUID is not registered. */
    hr = MFCreateAttributes(&attributes, 1);
    ok(hr == S_OK, "Failed to create attributes, hr %#lx.\n", hr);

    hr = IMFAttributes_SetGUID(attributes, &MF_SESSION_TOPOLOADER, &MF_SESSION_TOPOLOADER);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = MFCreateMediaSession(attributes, &session);
    ok(hr == S_OK, "Failed to create media session, hr %#lx.\n", hr);
    IMFMediaSession_Release(session);

    /* Disabled quality manager. */
    hr = IMFAttributes_SetGUID(attributes, &MF_SESSION_QUALITY_MANAGER, &GUID_NULL);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = MFCreateMediaSession(attributes, &session);
    ok(hr == S_OK, "Failed to create media session, hr %#lx.\n", hr);
    IMFMediaSession_Release(session);

    IMFAttributes_Release(attributes);
}

static void test_media_session_rate_control(void)
{
    IMFRateControl *rate_control, *clock_rate_control;
    IMFPresentationClock *presentation_clock;
    IMFPresentationTimeSource *time_source;
    MFCLOCK_PROPERTIES clock_props;
    IMFRateSupport *rate_support;
    IMFMediaSession *session;
    IMFClock *clock;
    HRESULT hr;
    float rate;
    BOOL thin;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#lx.\n", hr);

    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Failed to create media session, hr %#lx.\n", hr);

    hr = MFGetService((IUnknown *)session, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport, (void **)&rate_support);
    ok(hr == S_OK, "Failed to get rate support interface, hr %#lx.\n", hr);

    hr = MFGetService((IUnknown *)session, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateControl, (void **)&rate_control);
    ok(hr == S_OK, "Failed to get rate control interface, hr %#lx.\n", hr);

    hr = IMFRateControl_GetRate(rate_control, NULL, NULL);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    rate = 0.0f;
    hr = IMFRateControl_GetRate(rate_control, NULL, &rate);
    ok(hr == S_OK, "Failed to get playback rate, hr %#lx.\n", hr);
    ok(rate == 1.0f, "Unexpected rate %f.\n", rate);

    hr = IMFRateControl_GetRate(rate_control, &thin, NULL);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    thin = TRUE;
    rate = 0.0f;
    hr = IMFRateControl_GetRate(rate_control, &thin, &rate);
    ok(hr == S_OK, "Failed to get playback rate, hr %#lx.\n", hr);
    ok(!thin, "Unexpected thinning.\n");
    ok(rate == 1.0f, "Unexpected rate %f.\n", rate);

    hr = IMFMediaSession_GetClock(session, &clock);
    ok(hr == S_OK, "Failed to get clock, hr %#lx.\n", hr);

    hr = IMFClock_QueryInterface(clock, &IID_IMFPresentationClock, (void **)&presentation_clock);
    ok(hr == S_OK, "Failed to get rate control, hr %#lx.\n", hr);

    hr = IMFClock_QueryInterface(clock, &IID_IMFRateControl, (void **)&clock_rate_control);
    ok(hr == S_OK, "Failed to get rate control, hr %#lx.\n", hr);

    rate = 0.0f;
    hr = IMFRateControl_GetRate(clock_rate_control, NULL, &rate);
    ok(hr == S_OK, "Failed to get clock rate, hr %#lx.\n", hr);
    ok(rate == 1.0f, "Unexpected rate %f.\n", rate);

    hr = IMFRateControl_SetRate(clock_rate_control, FALSE, 1.5f);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#lx.\n", hr);

    hr = IMFRateControl_SetRate(rate_control, FALSE, 1.5f);
    ok(hr == S_OK, "Failed to set rate, hr %#lx.\n", hr);

    hr = IMFClock_GetProperties(clock, &clock_props);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateSystemTimeSource(&time_source);
    ok(hr == S_OK, "Failed to create time source, hr %#lx.\n", hr);

    hr = IMFPresentationClock_SetTimeSource(presentation_clock, time_source);
    ok(hr == S_OK, "Failed to set time source, hr %#lx.\n", hr);

    hr = IMFRateControl_SetRate(rate_control, FALSE, 1.5f);
    ok(hr == S_OK, "Failed to set rate, hr %#lx.\n", hr);

    rate = 0.0f;
    hr = IMFRateControl_GetRate(clock_rate_control, NULL, &rate);
    ok(hr == S_OK, "Failed to get clock rate, hr %#lx.\n", hr);
    ok(rate == 1.0f, "Unexpected rate %f.\n", rate);

    IMFPresentationTimeSource_Release(time_source);

    IMFRateControl_Release(clock_rate_control);
    IMFPresentationClock_Release(presentation_clock);
    IMFClock_Release(clock);

    IMFRateControl_Release(rate_control);
    IMFRateSupport_Release(rate_support);

    IMFMediaSession_Release(session);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#lx.\n", hr);
}

struct test_grabber_callback
{
    IMFSampleGrabberSinkCallback IMFSampleGrabberSinkCallback_iface;
    LONG refcount;

    IMFCollection *samples;
    HANDLE ready_event;
    HANDLE done_event;
};

static struct test_grabber_callback *impl_from_IMFSampleGrabberSinkCallback(IMFSampleGrabberSinkCallback *iface)
{
    return CONTAINING_RECORD(iface, struct test_grabber_callback, IMFSampleGrabberSinkCallback_iface);
}

static HRESULT WINAPI test_grabber_callback_QueryInterface(IMFSampleGrabberSinkCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFSampleGrabberSinkCallback) ||
            IsEqualIID(riid, &IID_IMFClockStateSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFSampleGrabberSinkCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_grabber_callback_AddRef(IMFSampleGrabberSinkCallback *iface)
{
    struct test_grabber_callback *grabber = impl_from_IMFSampleGrabberSinkCallback(iface);
    return InterlockedIncrement(&grabber->refcount);
}

static ULONG WINAPI test_grabber_callback_Release(IMFSampleGrabberSinkCallback *iface)
{
    struct test_grabber_callback *grabber = impl_from_IMFSampleGrabberSinkCallback(iface);
    ULONG refcount = InterlockedDecrement(&grabber->refcount);

    if (!refcount)
    {
        IMFCollection_Release(grabber->samples);
        if (grabber->ready_event)
            CloseHandle(grabber->ready_event);
        if (grabber->done_event)
            CloseHandle(grabber->done_event);
        free(grabber);
    }

    return refcount;
}

static HRESULT WINAPI test_grabber_callback_OnClockStart(IMFSampleGrabberSinkCallback *iface, MFTIME time, LONGLONG offset)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_grabber_callback_OnClockStop(IMFSampleGrabberSinkCallback *iface, MFTIME time)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_grabber_callback_OnClockPause(IMFSampleGrabberSinkCallback *iface, MFTIME time)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_grabber_callback_OnClockRestart(IMFSampleGrabberSinkCallback *iface, MFTIME time)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_grabber_callback_OnClockSetRate(IMFSampleGrabberSinkCallback *iface, MFTIME time, float rate)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_grabber_callback_OnSetPresentationClock(IMFSampleGrabberSinkCallback *iface,
        IMFPresentationClock *clock)
{
    return S_OK;
}

static HRESULT WINAPI test_grabber_callback_OnProcessSample(IMFSampleGrabberSinkCallback *iface, REFGUID major_type,
        DWORD sample_flags, LONGLONG sample_time, LONGLONG sample_duration, const BYTE *buffer, DWORD sample_size)
{
    struct test_grabber_callback *grabber = CONTAINING_RECORD(iface, struct test_grabber_callback, IMFSampleGrabberSinkCallback_iface);
    IMFSample *sample;
    HRESULT hr;
    DWORD res;

    if (!grabber->ready_event)
        return E_NOTIMPL;

    sample = create_sample(buffer, sample_size);
    hr = IMFSample_SetSampleFlags(sample, sample_flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* FIXME: sample time is inconsistent across windows versions, ignore it */
    hr = IMFSample_SetSampleTime(sample, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSample_SetSampleDuration(sample, sample_duration);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFCollection_AddElement(grabber->samples, (IUnknown *)sample);
    IMFSample_Release(sample);

    SetEvent(grabber->ready_event);
    res = WaitForSingleObject(grabber->done_event, 1000);
    ok(!res, "WaitForSingleObject returned %#lx", res);

    return S_OK;
}

static HRESULT WINAPI test_grabber_callback_OnShutdown(IMFSampleGrabberSinkCallback *iface)
{
    return S_OK;
}

static const IMFSampleGrabberSinkCallbackVtbl test_grabber_callback_vtbl =
{
    test_grabber_callback_QueryInterface,
    test_grabber_callback_AddRef,
    test_grabber_callback_Release,
    test_grabber_callback_OnClockStart,
    test_grabber_callback_OnClockStop,
    test_grabber_callback_OnClockPause,
    test_grabber_callback_OnClockRestart,
    test_grabber_callback_OnClockSetRate,
    test_grabber_callback_OnSetPresentationClock,
    test_grabber_callback_OnProcessSample,
    test_grabber_callback_OnShutdown,
};

static IMFSampleGrabberSinkCallback *create_test_grabber_callback(void)
{
    struct test_grabber_callback *grabber;
    HRESULT hr;

    if (!(grabber = calloc(1, sizeof(*grabber))))
        return NULL;

    grabber->IMFSampleGrabberSinkCallback_iface.lpVtbl = &test_grabber_callback_vtbl;
    grabber->refcount = 1;
    hr = MFCreateCollection(&grabber->samples);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    return &grabber->IMFSampleGrabberSinkCallback_iface;
}

enum loader_test_flags
{
    LOADER_EXPECTED_DECODER = 0x1,
    LOADER_EXPECTED_CONVERTER = 0x2,
    LOADER_TODO = 0x4,
    LOADER_NEEDS_VIDEO_PROCESSOR = 0x8,
    LOADER_SET_ENUMERATE_SOURCE_TYPES = 0x10,
    LOADER_NO_CURRENT_OUTPUT = 0x20,
    LOADER_SET_INVALID_INPUT = 0x40,
    LOADER_SET_MEDIA_TYPES = 0x80,
    LOADER_ADD_RESAMPLER_MFT = 0x100,
};

static void test_topology_loader(void)
{
    static const media_type_desc audio_float_44100 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 4 * 44100),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 4 * 8),
    };
    static const media_type_desc audio_pcm_44100 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
    };
    static const media_type_desc audio_pcm_48000 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 48000),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
    };
    static const media_type_desc audio_pcm_48000_resampler =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 192000),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
    };
    static const media_type_desc audio_float_48000 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 8 * 48000),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 8),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
    };
    static const media_type_desc audio_mp3_44100 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_MP3),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 16000),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
    };
    static const media_type_desc audio_pcm_44100_incomplete =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
    };
    static const media_type_desc video_i420_1280 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1280, 720),
    };
    static const media_type_desc video_color_convert_1280_rgb32 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, DMOVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1280, 720),
    };
    static const media_type_desc video_video_processor_1280_rgb32 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1280, 720),
    };
    static const media_type_desc video_video_processor_rgb32 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
    };
    static const media_type_desc video_dummy =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
    };

    const struct loader_test
    {
        const media_type_desc *input_type;
        const media_type_desc *output_type;
        const media_type_desc *current_input;
        MF_CONNECT_METHOD source_method;
        MF_CONNECT_METHOD sink_method;
        HRESULT expected_result;
        unsigned int flags;
    }
    loader_tests[] =
    {
        {
            /* PCM -> PCM, same enumerated type, no current type */
            .input_type = &audio_pcm_44100, .output_type = &audio_pcm_44100, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .expected_result = S_OK,
        },
        {
            /* PCM -> PCM, same enumerated type, incomplete current type */
            .input_type = &audio_pcm_44100, .output_type = &audio_pcm_44100, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_pcm_44100_incomplete,
            .expected_result = MF_E_INVALIDMEDIATYPE,
            .flags = LOADER_TODO,
        },
        {
            /* PCM -> PCM, same enumerated bps, different current bps */
            .input_type = &audio_pcm_48000, .output_type = &audio_pcm_48000, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_pcm_44100,
            .expected_result = MF_E_INVALIDMEDIATYPE,
        },
        {
            /* PCM -> PCM, same enumerated bps, different current bps, force enumerate */
            .input_type = &audio_pcm_48000, .output_type = &audio_pcm_48000, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_pcm_44100,
            .expected_result = S_OK,
            .flags = LOADER_SET_ENUMERATE_SOURCE_TYPES,
        },

        {
            /* PCM -> PCM, incomplete enumerated type, same current type */
            .input_type = &audio_pcm_44100_incomplete, .output_type = &audio_pcm_44100, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_pcm_44100,
            .expected_result = S_OK,
        },
        {
            /* PCM -> PCM, incomplete enumerated type, same current type, force enumerate */
            .input_type = &audio_pcm_44100_incomplete, .output_type = &audio_pcm_44100, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_pcm_44100,
            .expected_result = MF_E_NO_MORE_TYPES,
            .flags = LOADER_SET_ENUMERATE_SOURCE_TYPES | LOADER_TODO,
        },

        {
            /* PCM -> PCM, different enumerated bps, no current type */
            .input_type = &audio_pcm_44100, .output_type = &audio_pcm_48000, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .expected_result = MF_E_INVALIDMEDIATYPE,
        },
        {
            /* PCM -> PCM, different enumerated bps, same current bps */
            .input_type = &audio_pcm_44100, .output_type = &audio_pcm_48000, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_pcm_48000,
            .expected_result = S_OK,
        },
        {
            /* PCM -> PCM, different enumerated bps, same current bps, force enumerate */
            .input_type = &audio_pcm_44100, .output_type = &audio_pcm_48000, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_pcm_48000,
            .expected_result = MF_E_NO_MORE_TYPES,
            .flags = LOADER_SET_ENUMERATE_SOURCE_TYPES,
        },
        {
            /* PCM -> PCM, different enumerated bps, no current type, sink allow converter */
            .input_type = &audio_pcm_44100, .output_type = &audio_pcm_48000, .sink_method = MF_CONNECT_ALLOW_CONVERTER, .source_method = MF_CONNECT_DIRECT,
            .expected_result = S_OK,
            .flags = LOADER_EXPECTED_CONVERTER,
        },
        {
            /* PCM -> PCM, different enumerated bps, same current type, sink allow converter, force enumerate */
            .input_type = &audio_pcm_44100, .output_type = &audio_pcm_48000, .sink_method = MF_CONNECT_ALLOW_CONVERTER, .source_method = -1,
            .current_input = &audio_pcm_48000,
            .expected_result = S_OK,
            .flags = LOADER_EXPECTED_CONVERTER | LOADER_SET_ENUMERATE_SOURCE_TYPES,
        },
        {
            /* PCM -> PCM, different enumerated bps, no current type, sink allow decoder */
            .input_type = &audio_pcm_44100, .output_type = &audio_pcm_48000, .sink_method = MF_CONNECT_ALLOW_DECODER, .source_method = MF_CONNECT_DIRECT,
            .expected_result = S_OK,
            .flags = LOADER_EXPECTED_CONVERTER,
        },
        {
            /* PCM -> PCM, different enumerated bps, no current type, default methods */
            .input_type = &audio_pcm_44100, .output_type = &audio_pcm_48000, .sink_method = -1, .source_method = -1,
            .expected_result = S_OK,
            .flags = LOADER_EXPECTED_CONVERTER,
        },
        {
            /* PCM -> PCM, different enumerated bps, no current type, source allow converter */
            .input_type = &audio_pcm_44100, .output_type = &audio_pcm_48000, .sink_method = MF_CONNECT_DIRECT, .source_method = MF_CONNECT_ALLOW_CONVERTER,
            .expected_result = MF_E_INVALIDMEDIATYPE,
        },

        {
            /* Float -> PCM, refuse input type, add converter */
            .input_type = &audio_float_44100, .output_type = &audio_pcm_48000, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .expected_result = MF_E_NO_MORE_TYPES,
            .flags = LOADER_SET_INVALID_INPUT | LOADER_ADD_RESAMPLER_MFT | LOADER_EXPECTED_CONVERTER | LOADER_TODO,
        },
        {
            /* Float -> PCM, refuse input type, add converter, allow resampler output type */
            .input_type = &audio_float_44100, .output_type = &audio_pcm_48000_resampler, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .expected_result = S_OK,
            .flags = LOADER_SET_INVALID_INPUT | LOADER_ADD_RESAMPLER_MFT | LOADER_EXPECTED_CONVERTER | LOADER_TODO,
        },

        {
            /* MP3 -> PCM */
            .input_type = &audio_mp3_44100, .output_type = &audio_pcm_44100, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_mp3_44100,
            .expected_result = MF_E_INVALIDMEDIATYPE,
        },
        {
            /* MP3 -> PCM, force enumerate */
            .input_type = &audio_mp3_44100, .output_type = &audio_pcm_44100, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_mp3_44100,
            .expected_result = MF_E_NO_MORE_TYPES,
            .flags = LOADER_SET_ENUMERATE_SOURCE_TYPES,
        },
        {
            /* MP3 -> PCM */
            .input_type = &audio_mp3_44100, .output_type = &audio_pcm_44100, .sink_method = MF_CONNECT_ALLOW_CONVERTER, .source_method = -1,
            .current_input = &audio_mp3_44100,
            .expected_result = MF_E_TRANSFORM_NOT_POSSIBLE_FOR_CURRENT_MEDIATYPE_COMBINATION,
            .flags = LOADER_TODO,
        },
        {
            /* MP3 -> PCM */
            .input_type = &audio_mp3_44100, .output_type = &audio_pcm_44100, .sink_method = MF_CONNECT_ALLOW_DECODER, .source_method = -1,
            .current_input = &audio_mp3_44100,
            .expected_result = S_OK,
            .flags = LOADER_EXPECTED_DECODER | LOADER_TODO,
        },
        {
            /* MP3 -> PCM, need both decoder and converter */
            .input_type = &audio_mp3_44100, .output_type = &audio_float_48000, .sink_method = MF_CONNECT_ALLOW_DECODER, .source_method = -1,
            .current_input = &audio_mp3_44100,
            .expected_result = S_OK,
            .flags = LOADER_EXPECTED_DECODER | LOADER_EXPECTED_CONVERTER | LOADER_TODO,
        },

        {
            /* I420 -> RGB32, Color Convert media type */
            .input_type = &video_i420_1280, .output_type = &video_color_convert_1280_rgb32, .sink_method = -1, .source_method = -1,
            .expected_result = MF_E_TOPO_CODEC_NOT_FOUND,
            .flags = LOADER_NEEDS_VIDEO_PROCESSOR | LOADER_EXPECTED_CONVERTER,
        },
        {
            /* I420 -> RGB32, Video Processor media type */
            .input_type = &video_i420_1280, .output_type = &video_video_processor_1280_rgb32, .sink_method = -1, .source_method = -1,
            .expected_result = S_OK,
            .flags = LOADER_EXPECTED_CONVERTER,
        },
        {
            /* I420 -> RGB32, Video Processor media type without frame size */
            .input_type = &video_i420_1280, .output_type = &video_video_processor_rgb32, .sink_method = -1, .source_method = -1,
            .expected_result = S_OK,
            .flags = LOADER_EXPECTED_CONVERTER,
        },
        {
            /* RGB32 -> Any Video, no current output type */
            .input_type = &video_i420_1280, .output_type = &video_dummy, .sink_method = -1, .source_method = -1,
            .expected_result = S_OK,
            .flags = LOADER_NO_CURRENT_OUTPUT,
        },
        {
            /* RGB32 -> Any Video, no current output type, refuse input type */
            .input_type = &video_i420_1280, .output_type = &video_dummy, .sink_method = -1, .source_method = -1,
            .expected_result = S_OK,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_INVALID_INPUT | LOADER_EXPECTED_CONVERTER,
        },
        {
            /* RGB32 -> Any Video, no current output type, refuse input type */
            .input_type = &video_i420_1280, .output_type = &video_video_processor_rgb32, .sink_method = -1, .source_method = -1,
            .expected_result = S_OK,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_INVALID_INPUT | LOADER_SET_MEDIA_TYPES | LOADER_EXPECTED_CONVERTER,
        },
    };

    IMFTopologyNode *src_node, *sink_node, *src_node2, *sink_node2, *mft_node;
    IMFSampleGrabberSinkCallback *grabber_callback = create_test_grabber_callback();
    struct test_stream_sink stream_sink = test_stream_sink;
    IMFMediaType *media_type, *input_type, *output_type;
    IMFTopology *topology, *topology2, *full_topology;
    struct test_handler handler = test_handler;
    IMFPresentationDescriptor *pd;
    unsigned int i, count, value;
    IMFActivate *sink_activate;
    MF_TOPOLOGY_TYPE node_type;
    IMFStreamDescriptor *sd;
    IMFTransform *transform;
    IMFMediaSource *source;
    IMFTopoLoader *loader;
    IUnknown *node_object;
    WORD node_count;
    TOPOID node_id;
    DWORD index;
    HRESULT hr;
    BOOL ret;
    LONG ref;

    stream_sink.handler = &handler.IMFMediaTypeHandler_iface;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#lx.\n", hr);

    hr = MFCreateTopoLoader(NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTopoLoader(&loader);
    ok(hr == S_OK, "Failed to create topology loader, hr %#lx.\n", hr);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);

    /* Empty topology */
    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    todo_wine_if(hr == S_OK)
    ok(hr == MF_E_TOPO_UNSUPPORTED, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK) IMFTopology_Release(full_topology);

    /* Add source node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &src_node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    /* When a decoder is involved, windows requires this attribute to be present */
    create_descriptors(1, &media_type, NULL, &pd, &sd);
    IMFMediaType_Release(media_type);

    source = create_test_source(pd);

    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_SOURCE, (IUnknown *)source);
    ok(hr == S_OK, "Failed to set node source, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_STREAM_DESCRIPTOR, (IUnknown *)sd);
    ok(hr == S_OK, "Failed to set node sd, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, (IUnknown *)pd);
    ok(hr == S_OK, "Failed to set node pd, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, src_node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    /* Source node only. */
    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == MF_E_TOPO_UNSUPPORTED, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &sink_node);
    ok(hr == S_OK, "Failed to create output node, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = MFCreateSampleGrabberSinkActivate(media_type, grabber_callback, &sink_activate);
    ok(hr == S_OK, "Failed to create grabber sink, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetObject(sink_node, (IUnknown *)sink_activate);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);

    IMFMediaType_Release(media_type);

    hr = IMFTopology_AddNode(topology, sink_node);
    ok(hr == S_OK, "Failed to add sink node, hr %#lx.\n", hr);

    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    todo_wine_if(hr == MF_E_TOPO_SINK_ACTIVATES_UNSUPPORTED)
    ok(hr == MF_E_TOPO_UNSUPPORTED, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_ConnectOutput(src_node, 0, sink_node, 0);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    /* Sink was not resolved. */
    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == MF_E_TOPO_SINK_ACTIVATES_UNSUPPORTED, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetObject(sink_node, NULL);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);

    hr = IMFActivate_ShutdownObject(sink_activate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ref = IMFActivate_Release(sink_activate);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_SOURCE, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_STREAM_DESCRIPTOR, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ref = IMFMediaSource_Release(source);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFPresentationDescriptor_Release(pd);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFStreamDescriptor_Release(sd);
    ok(ref == 0, "Release returned %ld\n", ref);


    hr = MFCreateMediaType(&input_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&output_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(loader_tests); ++i)
    {
        const struct loader_test *test = &loader_tests[i];

        winetest_push_context("%u", i);

        init_media_type(input_type, *test->input_type, -1);
        init_media_type(output_type, *test->output_type, -1);

        handler.set_current_count = 0;
        if (test->flags & LOADER_NO_CURRENT_OUTPUT)
            handler.current_type = NULL;
        else
            IMFMediaType_AddRef((handler.current_type = output_type));

        if (test->flags & LOADER_SET_INVALID_INPUT)
            handler.invalid_type = input_type;
        else
            handler.invalid_type = NULL;

        handler.enum_count = 0;
        if (test->flags & LOADER_SET_MEDIA_TYPES)
        {
            handler.media_types_count = 1;
            handler.media_types = &output_type;
        }
        else
        {
            handler.media_types_count = 0;
            handler.media_types = NULL;
        }

        if (test->flags & LOADER_ADD_RESAMPLER_MFT)
        {
            hr = IMFTopology_Clear(topology);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopology_AddNode(topology, src_node);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopology_AddNode(topology, sink_node);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &mft_node);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = CoCreateInstance(&CLSID_CResamplerMediaObject, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTransform, (void **)&transform);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopologyNode_SetGUID(mft_node, &MF_TOPONODE_TRANSFORM_OBJECTID, &CLSID_CResamplerMediaObject);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopologyNode_SetObject(mft_node, (IUnknown *)transform);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            IMFTransform_Release(transform);

            hr = IMFTopology_AddNode(topology, mft_node);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopologyNode_ConnectOutput(src_node, 0, mft_node, 0);
            ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);
            hr = IMFTopologyNode_ConnectOutput(mft_node, 0, sink_node, 0);
            ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);
            IMFTopologyNode_Release(mft_node);
        }
        else
        {
            hr = IMFTopology_Clear(topology);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopology_AddNode(topology, src_node);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopology_AddNode(topology, sink_node);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopologyNode_ConnectOutput(src_node, 0, sink_node, 0);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        }

        create_descriptors(1, &input_type, test->current_input, &pd, &sd);

        source = create_test_source(pd);

        init_source_node(source, test->source_method, src_node, pd, sd);
        init_sink_node(&stream_sink.IMFStreamSink_iface, test->sink_method, sink_node);

        hr = IMFTopology_GetCount(topology, &count);
        ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
        ok(!count, "Unexpected count %u.\n", count);

        if (test->flags & LOADER_SET_ENUMERATE_SOURCE_TYPES)
            IMFTopology_SetUINT32(topology, &MF_TOPOLOGY_ENUMERATE_SOURCE_TYPES, 1);
        hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
        IMFTopology_DeleteItem(topology, &MF_TOPOLOGY_ENUMERATE_SOURCE_TYPES);

        if (test->flags & LOADER_NEEDS_VIDEO_PROCESSOR && !has_video_processor)
            ok(hr == MF_E_INVALIDMEDIATYPE || hr == MF_E_TOPO_CODEC_NOT_FOUND,
                    "Unexpected hr %#lx\n", hr);
        else
        {
            todo_wine_if(test->flags & LOADER_TODO)
            ok(hr == test->expected_result, "Unexpected hr %#lx\n", hr);
            ok(full_topology != topology, "Unexpected instance.\n");
        }

        if (test->expected_result != hr)
        {
            if (hr != S_OK) ref = 0;
            else ref = IMFTopology_Release(full_topology);
            ok(ref == 0, "Release returned %ld\n", ref);
        }
        else if (test->expected_result == S_OK)
        {
            hr = IMFTopology_GetCount(full_topology, &count);
            ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
            todo_wine
            ok(count == (test->flags & LOADER_SET_ENUMERATE_SOURCE_TYPES ? 2 : 1),
                    "Unexpected count %u.\n", count);

            value = 0xdeadbeef;
            hr = IMFTopology_GetUINT32(full_topology, &MF_TOPOLOGY_RESOLUTION_STATUS, &value);
todo_wine {
            ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
            ok(value == MF_TOPOLOGY_RESOLUTION_SUCCEEDED, "Unexpected value %#x.\n", value);
}
            count = 2;
            if (test->flags & LOADER_EXPECTED_DECODER)
                count++;
            if (test->flags & LOADER_EXPECTED_CONVERTER)
                count++;

            hr = IMFTopology_GetNodeCount(full_topology, &node_count);
            ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
            todo_wine_if(test->flags & LOADER_EXPECTED_DECODER)
            ok(node_count == count, "Unexpected node count %u.\n", node_count);

            hr = IMFTopologyNode_GetTopoNodeID(src_node, &node_id);
            ok(hr == S_OK, "Failed to get source node id, hr %#lx.\n", hr);

            hr = IMFTopology_GetNodeByID(full_topology, node_id, &src_node2);
            ok(hr == S_OK, "Failed to get source in resolved topology, hr %#lx.\n", hr);

            hr = IMFTopologyNode_GetTopoNodeID(sink_node, &node_id);
            ok(hr == S_OK, "Failed to get sink node id, hr %#lx.\n", hr);

            hr = IMFTopology_GetNodeByID(full_topology, node_id, &sink_node2);
            ok(hr == S_OK, "Failed to get sink in resolved topology, hr %#lx.\n", hr);

            if (test->flags & (LOADER_EXPECTED_DECODER | LOADER_EXPECTED_CONVERTER))
            {
                hr = IMFTopologyNode_GetOutput(src_node2, 0, &mft_node, &index);
                ok(hr == S_OK, "Failed to get transform node in resolved topology, hr %#lx.\n", hr);
                ok(!index, "Unexpected stream index %lu.\n", index);

                hr = IMFTopologyNode_GetNodeType(mft_node, &node_type);
                ok(hr == S_OK, "Failed to get transform node type in resolved topology, hr %#lx.\n", hr);
                ok(node_type == MF_TOPOLOGY_TRANSFORM_NODE, "Unexpected node type %u.\n", node_type);

                hr = IMFTopologyNode_GetObject(mft_node, &node_object);
                ok(hr == S_OK, "Failed to get object of transform node, hr %#lx.\n", hr);

                if (test->flags & LOADER_EXPECTED_DECODER)
                {
                    value = 0;
                    hr = IMFTopologyNode_GetUINT32(mft_node, &MF_TOPONODE_DECODER, &value);
                    ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
                    ok(value == 1, "Unexpected value.\n");
                }

                hr = IMFTopologyNode_GetItem(mft_node, &MF_TOPONODE_TRANSFORM_OBJECTID, NULL);
                ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);

                hr = IUnknown_QueryInterface(node_object, &IID_IMFTransform, (void **)&transform);
                ok(hr == S_OK, "Failed to get IMFTransform from transform node's object, hr %#lx.\n", hr);
                IUnknown_Release(node_object);

                hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
                ok(hr == S_OK, "Failed to get transform input type, hr %#lx.\n", hr);

                hr = IMFMediaType_Compare(input_type, (IMFAttributes *)media_type, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &ret);
                ok(hr == S_OK, "Failed to compare media types, hr %#lx.\n", hr);
                ok(ret, "Input type of first transform doesn't match source node type.\n");

                IMFTopologyNode_Release(mft_node);
                IMFMediaType_Release(media_type);
                IMFTransform_Release(transform);

                hr = IMFTopologyNode_GetInput(sink_node2, 0, &mft_node, &index);
                ok(hr == S_OK, "Failed to get transform node in resolved topology, hr %#lx.\n", hr);
                ok(!index, "Unexpected stream index %lu.\n", index);

                hr = IMFTopologyNode_GetNodeType(mft_node, &node_type);
                ok(hr == S_OK, "Failed to get transform node type in resolved topology, hr %#lx.\n", hr);
                ok(node_type == MF_TOPOLOGY_TRANSFORM_NODE, "Unexpected node type %u.\n", node_type);

                hr = IMFTopologyNode_GetItem(mft_node, &MF_TOPONODE_TRANSFORM_OBJECTID, NULL);
                ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);

                hr = IMFTopologyNode_GetObject(mft_node, &node_object);
                ok(hr == S_OK, "Failed to get object of transform node, hr %#lx.\n", hr);

                hr = IUnknown_QueryInterface(node_object, &IID_IMFTransform, (void**) &transform);
                ok(hr == S_OK, "Failed to get IMFTransform from transform node's object, hr %#lx.\n", hr);
                IUnknown_Release(node_object);

                hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
                ok(hr == S_OK, "Failed to get transform output type, hr %#lx.\n", hr);
                hr = IMFMediaType_Compare(output_type, (IMFAttributes *)media_type, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &ret);
                ok(hr == S_OK, "Failed to compare media types, hr %#lx.\n", hr);
                ok(ret, "Output type of last transform doesn't match sink node type.\n");
                IMFMediaType_Release(media_type);

                hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
                ok(hr == S_OK, "Failed to get transform input type, hr %#lx.\n", hr);
                if ((test->flags & (LOADER_EXPECTED_CONVERTER | LOADER_EXPECTED_DECODER)) != (LOADER_EXPECTED_CONVERTER | LOADER_EXPECTED_DECODER))
                {
                    hr = IMFMediaType_Compare(input_type, (IMFAttributes *)media_type, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &ret);
                    ok(hr == S_OK, "Failed to compare media types, hr %#lx.\n", hr);
                    ok(ret, "Input type of transform doesn't match source node type.\n");
                }
                IMFMediaType_Release(media_type);

                IMFTopologyNode_Release(mft_node);
                IMFTransform_Release(transform);
            }

            IMFTopologyNode_Release(src_node2);
            IMFTopologyNode_Release(sink_node2);

            hr = IMFTopology_SetUINT32(full_topology, &IID_IMFTopology, 123);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopoLoader_Load(loader, full_topology, &topology2, NULL);
            ok(hr == S_OK, "Failed to resolve topology, hr %#lx.\n", hr);
            ok(full_topology != topology2, "Unexpected instance.\n");
            hr = IMFTopology_GetUINT32(topology2, &IID_IMFTopology, &value);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            ref = IMFTopology_Release(topology2);
            ok(ref == 0, "Release returned %ld\n", ref);
            ref = IMFTopology_Release(full_topology);
            ok(ref == 0, "Release returned %ld\n", ref);
        }

        hr = IMFTopology_GetCount(topology, &count);
        ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
        ok(!count, "Unexpected count %u.\n", count);

        if (test->flags & LOADER_SET_MEDIA_TYPES)
            ok(handler.enum_count, "got %lu GetMediaTypeByIndex\n", handler.enum_count);
        else
            ok(!handler.enum_count, "got %lu GetMediaTypeByIndex\n", handler.enum_count);

        todo_wine_if((test->flags & LOADER_NO_CURRENT_OUTPUT) && !(test->flags & LOADER_SET_MEDIA_TYPES))
        ok(!handler.set_current_count, "got %lu SetCurrentMediaType\n", handler.set_current_count);

        if (handler.current_type)
            IMFMediaType_Release(handler.current_type);
        handler.current_type = NULL;

        hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_SOURCE, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_STREAM_DESCRIPTOR, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ref = IMFMediaSource_Release(source);
        ok(ref == 0, "Release returned %ld\n", ref);
        ref = IMFPresentationDescriptor_Release(pd);
        ok(ref == 0, "Release returned %ld\n", ref);
        ref = IMFStreamDescriptor_Release(sd);
        ok(ref == 0, "Release returned %ld\n", ref);

        winetest_pop_context();
    }

    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopoLoader_Release(loader);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(src_node);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(sink_node);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFMediaType_Release(input_type);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(output_type);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#lx.\n", hr);

    IMFSampleGrabberSinkCallback_Release(grabber_callback);
}

static void test_topology_loader_evr(void)
{
    static const media_type_desc media_type_desc =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 640, 480),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE),
        {0},
    };
    IMFTopologyNode *node, *source_node, *evr_node;
    IMFTopology *topology, *full_topology;
    IMFPresentationDescriptor *pd;
    IMFMediaTypeHandler *handler;
    unsigned int i, count, value;
    IMFStreamSink *stream_sink;
    IMFMediaType *media_type;
    IMFStreamDescriptor *sd;
    IMFActivate *activate;
    IMFTopoLoader *loader;
    IMFMediaSink *sink;
    WORD node_count;
    UINT64 value64;
    HWND window;
    HRESULT hr;
    LONG ref;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    hr = MFCreateTopoLoader(&loader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Source node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &source_node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);
    init_media_type(media_type, media_type_desc, -1);

    create_descriptors(1, &media_type, &media_type_desc, &pd, &sd);
    init_source_node(NULL, -1, source_node, pd, sd);
    IMFPresentationDescriptor_Release(pd);
    IMFStreamDescriptor_Release(sd);

    /* EVR sink node. */
    window = create_window();

    hr = MFCreateVideoRendererActivate(window, &activate);
    ok(hr == S_OK, "Failed to create activate object, hr %#lx.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetStreamSinkById(sink, 0, &stream_sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &evr_node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetObject(evr_node, (IUnknown *)stream_sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamSink_GetMediaTypeHandler(stream_sink, &handler);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaTypeHandler_Release(handler);

    IMFStreamSink_Release(stream_sink);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, source_node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, evr_node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopologyNode_ConnectOutput(source_node, 0, evr_node, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetUINT32(evr_node, &MF_TOPONODE_CONNECT_METHOD, MF_CONNECT_DIRECT);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetCount(evr_node, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_GetNodeCount(full_topology, &node_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_count == 3, "Unexpected node count %u.\n", node_count);

    for (i = 0; i < node_count; ++i)
    {
        MF_TOPOLOGY_TYPE node_type;

        hr = IMFTopology_GetNode(full_topology, i, &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFTopologyNode_GetNodeType(node, &node_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        if (node_type == MF_TOPOLOGY_OUTPUT_NODE)
        {
            value = 1;
            hr = IMFTopologyNode_GetUINT32(node, &MF_TOPONODE_STREAMID, &value);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(!value, "Unexpected stream id %u.\n", value);
        }
        else if (node_type == MF_TOPOLOGY_SOURCESTREAM_NODE)
        {
            value64 = 1;
            hr = IMFTopologyNode_GetUINT64(node, &MF_TOPONODE_MEDIASTART, &value64);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(!value64, "Unexpected value.\n");
        }

        IMFTopologyNode_Release(node);
    }

    ref = IMFTopology_Release(full_topology);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopoLoader_Release(loader);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(source_node);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(evr_node);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
    ref = IMFActivate_Release(activate);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaSink_Release(sink);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFMediaType_Release(media_type);
    ok(ref == 0, "Release returned %ld\n", ref);

    DestroyWindow(window);

    CoUninitialize();
}

static HRESULT WINAPI testshutdown_QueryInterface(IMFShutdown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFShutdown) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFShutdown_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testshutdown_AddRef(IMFShutdown *iface)
{
    return 2;
}

static ULONG WINAPI testshutdown_Release(IMFShutdown *iface)
{
    return 1;
}

static HRESULT WINAPI testshutdown_Shutdown(IMFShutdown *iface)
{
    return 0xdead;
}

static HRESULT WINAPI testshutdown_GetShutdownStatus(IMFShutdown *iface, MFSHUTDOWN_STATUS *status)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IMFShutdownVtbl testshutdownvtbl =
{
    testshutdown_QueryInterface,
    testshutdown_AddRef,
    testshutdown_Release,
    testshutdown_Shutdown,
    testshutdown_GetShutdownStatus,
};

static void test_MFShutdownObject(void)
{
    IMFShutdown testshutdown = { &testshutdownvtbl };
    IUnknown testshutdown2 = { &testservicevtbl };
    HRESULT hr;

    hr = MFShutdownObject(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFShutdownObject((IUnknown *)&testshutdown);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = MFShutdownObject(&testshutdown2);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
}

enum clock_action
{
    CLOCK_START,
    CLOCK_STOP,
    CLOCK_PAUSE,
};

static HRESULT WINAPI test_clock_sink_QueryInterface(IMFClockStateSink *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFClockStateSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFClockStateSink_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_clock_sink_AddRef(IMFClockStateSink *iface)
{
    return 2;
}

static ULONG WINAPI test_clock_sink_Release(IMFClockStateSink *iface)
{
    return 1;
}

static HRESULT WINAPI test_clock_sink_OnClockStart(IMFClockStateSink *iface, MFTIME system_time, LONGLONG offset)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_clock_sink_OnClockStop(IMFClockStateSink *iface, MFTIME system_time)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_clock_sink_OnClockPause(IMFClockStateSink *iface, MFTIME system_time)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_clock_sink_OnClockRestart(IMFClockStateSink *iface, MFTIME system_time)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_clock_sink_OnClockSetRate(IMFClockStateSink *iface, MFTIME system_time, float rate)
{
    return E_NOTIMPL;
}

static const IMFClockStateSinkVtbl test_clock_sink_vtbl =
{
    test_clock_sink_QueryInterface,
    test_clock_sink_AddRef,
    test_clock_sink_Release,
    test_clock_sink_OnClockStart,
    test_clock_sink_OnClockStop,
    test_clock_sink_OnClockPause,
    test_clock_sink_OnClockRestart,
    test_clock_sink_OnClockSetRate,
};

static void test_presentation_clock(void)
{
    static const struct clock_state_test
    {
        enum clock_action action;
        MFCLOCK_STATE clock_state;
        MFCLOCK_STATE source_state;
        HRESULT hr;
    }
    clock_state_change[] =
    {
        { CLOCK_STOP, MFCLOCK_STATE_STOPPED, MFCLOCK_STATE_INVALID },
        { CLOCK_PAUSE, MFCLOCK_STATE_STOPPED, MFCLOCK_STATE_INVALID, MF_E_INVALIDREQUEST },
        { CLOCK_STOP, MFCLOCK_STATE_STOPPED, MFCLOCK_STATE_INVALID, MF_E_CLOCK_STATE_ALREADY_SET },
        { CLOCK_START, MFCLOCK_STATE_RUNNING, MFCLOCK_STATE_RUNNING },
        { CLOCK_START, MFCLOCK_STATE_RUNNING, MFCLOCK_STATE_RUNNING },
        { CLOCK_PAUSE, MFCLOCK_STATE_PAUSED, MFCLOCK_STATE_PAUSED },
        { CLOCK_PAUSE, MFCLOCK_STATE_PAUSED, MFCLOCK_STATE_PAUSED, MF_E_CLOCK_STATE_ALREADY_SET },
        { CLOCK_STOP, MFCLOCK_STATE_STOPPED, MFCLOCK_STATE_STOPPED },
        { CLOCK_START, MFCLOCK_STATE_RUNNING, MFCLOCK_STATE_RUNNING },
        { CLOCK_STOP, MFCLOCK_STATE_STOPPED, MFCLOCK_STATE_STOPPED },
        { CLOCK_STOP, MFCLOCK_STATE_STOPPED, MFCLOCK_STATE_STOPPED, MF_E_CLOCK_STATE_ALREADY_SET },
        { CLOCK_PAUSE, MFCLOCK_STATE_STOPPED, MFCLOCK_STATE_STOPPED, MF_E_INVALIDREQUEST },
        { CLOCK_START, MFCLOCK_STATE_RUNNING, MFCLOCK_STATE_RUNNING },
        { CLOCK_PAUSE, MFCLOCK_STATE_PAUSED, MFCLOCK_STATE_PAUSED },
        { CLOCK_START, MFCLOCK_STATE_RUNNING, MFCLOCK_STATE_RUNNING },
    };
    IMFClockStateSink test_sink = { &test_clock_sink_vtbl };
    IMFPresentationTimeSource *time_source;
    MFCLOCK_PROPERTIES props, props2;
    IMFRateControl *rate_control;
    IMFPresentationClock *clock;
    MFSHUTDOWN_STATUS status;
    IMFShutdown *shutdown;
    MFTIME systime, time;
    LONGLONG clock_time;
    MFCLOCK_STATE state;
    unsigned int i;
    DWORD value;
    float rate;
    HRESULT hr;
    BOOL thin;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    hr = MFCreatePresentationClock(&clock);
    ok(hr == S_OK, "Failed to create presentation clock, hr %#lx.\n", hr);

    check_interface(clock, &IID_IMFTimer, TRUE);
    check_interface(clock, &IID_IMFRateControl, TRUE);
    check_interface(clock, &IID_IMFPresentationClock, TRUE);
    check_interface(clock, &IID_IMFShutdown, TRUE);
    check_interface(clock, &IID_IMFClock, TRUE);

    hr = IMFPresentationClock_QueryInterface(clock, &IID_IMFRateControl, (void **)&rate_control);
    ok(hr == S_OK, "Failed to get rate control interface, hr %#lx.\n", hr);

    hr = IMFPresentationClock_GetTimeSource(clock, &time_source);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationClock_GetTimeSource(clock, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationClock_GetClockCharacteristics(clock, &value);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationClock_GetClockCharacteristics(clock, NULL);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationClock_GetTime(clock, &time);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationClock_GetTime(clock, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    value = 1;
    hr = IMFPresentationClock_GetContinuityKey(clock, &value);
    ok(hr == S_OK, "Failed to get continuity key, hr %#lx.\n", hr);
    ok(value == 0, "Unexpected value %lu.\n", value);

    hr = IMFPresentationClock_GetProperties(clock, &props);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationClock_GetState(clock, 0, &state);
    ok(hr == S_OK, "Failed to get state, hr %#lx.\n", hr);
    ok(state == MFCLOCK_STATE_INVALID, "Unexpected state %d.\n", state);

    hr = IMFPresentationClock_GetCorrelatedTime(clock, 0, &clock_time, &systime);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationClock_GetCorrelatedTime(clock, 0, NULL, &systime);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationClock_GetCorrelatedTime(clock, 0, &time, NULL);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#lx.\n", hr);

    /* Sinks. */
    hr = IMFPresentationClock_AddClockStateSink(clock, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationClock_AddClockStateSink(clock, &test_sink);
    ok(hr == S_OK, "Failed to add a sink, hr %#lx.\n", hr);

    hr = IMFPresentationClock_AddClockStateSink(clock, &test_sink);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationClock_RemoveClockStateSink(clock, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationClock_RemoveClockStateSink(clock, &test_sink);
    ok(hr == S_OK, "Failed to remove sink, hr %#lx.\n", hr);

    hr = IMFPresentationClock_RemoveClockStateSink(clock, &test_sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* State change commands, time source is not set yet. */
    hr = IMFPresentationClock_Start(clock, 0);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationClock_Pause(clock);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationClock_Stop(clock);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#lx.\n", hr);

    hr = IMFRateControl_SetRate(rate_control, FALSE, 0.0f);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#lx.\n", hr);

    /* Set default time source. */
    hr = MFCreateSystemTimeSource(&time_source);
    ok(hr == S_OK, "Failed to create time source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetClockCharacteristics(time_source, &value);
    ok(hr == S_OK, "Failed to get time source flags, hr %#lx.\n", hr);
    ok(value == (MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ | MFCLOCK_CHARACTERISTICS_FLAG_IS_SYSTEM_CLOCK),
            "Unexpected clock flags %#lx.\n", value);

    hr = IMFPresentationClock_SetTimeSource(clock, time_source);
    ok(hr == S_OK, "Failed to set time source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetProperties(time_source, &props2);
    ok(hr == S_OK, "Failed to get time source properties, hr %#lx.\n", hr);

    hr = IMFPresentationClock_GetClockCharacteristics(clock, &value);
    ok(hr == S_OK, "Failed to get clock flags, hr %#lx.\n", hr);
    ok(value == (MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ | MFCLOCK_CHARACTERISTICS_FLAG_IS_SYSTEM_CLOCK),
            "Unexpected clock flags %#lx.\n", value);

    hr = IMFPresentationClock_GetProperties(clock, &props);
    ok(hr == S_OK, "Failed to get clock properties, hr %#lx.\n", hr);
    ok(!memcmp(&props, &props2, sizeof(props)), "Unexpected clock properties.\n");

    /* Changing rate at initial state. */
    hr = IMFPresentationClock_GetState(clock, 0, &state);
    ok(hr == S_OK, "Failed to get clock state, hr %#lx.\n", hr);
    ok(state == MFCLOCK_STATE_INVALID, "Unexpected state %d.\n", state);

    hr = IMFRateControl_SetRate(rate_control, FALSE, 0.0f);
    ok(hr == S_OK, "Failed to set clock rate, hr %#lx.\n", hr);
    hr = IMFRateControl_GetRate(rate_control, &thin, &rate);
    ok(hr == S_OK, "Failed to get clock rate, hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected rate.\n");
    hr = IMFRateControl_SetRate(rate_control, FALSE, 1.0f);
    ok(hr == S_OK, "Failed to set clock rate, hr %#lx.\n", hr);

    /* State changes. */
    for (i = 0; i < ARRAY_SIZE(clock_state_change); ++i)
    {
        switch (clock_state_change[i].action)
        {
            case CLOCK_STOP:
                hr = IMFPresentationClock_Stop(clock);
                break;
            case CLOCK_PAUSE:
                hr = IMFPresentationClock_Pause(clock);
                break;
            case CLOCK_START:
                hr = IMFPresentationClock_Start(clock, 0);
                break;
            default:
                ;
        }
        ok(hr == clock_state_change[i].hr, "%u: unexpected hr %#lx.\n", i, hr);

        hr = IMFPresentationTimeSource_GetState(time_source, 0, &state);
        ok(hr == S_OK, "%u: failed to get state, hr %#lx.\n", i, hr);
        ok(state == clock_state_change[i].source_state, "%u: unexpected state %d.\n", i, state);

        hr = IMFPresentationClock_GetState(clock, 0, &state);
        ok(hr == S_OK, "%u: failed to get state, hr %#lx.\n", i, hr);
        ok(state == clock_state_change[i].clock_state, "%u: unexpected state %d.\n", i, state);
    }

    /* Clock time stamps. */
    hr = IMFPresentationClock_Start(clock, 10);
    ok(hr == S_OK, "Failed to start presentation clock, hr %#lx.\n", hr);

    hr = IMFPresentationClock_Pause(clock);
    ok(hr == S_OK, "Failed to pause presentation clock, hr %#lx.\n", hr);

    hr = IMFPresentationClock_GetTime(clock, &time);
    ok(hr == S_OK, "Failed to get clock time, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &clock_time, &systime);
    ok(hr == S_OK, "Failed to get time source time, hr %#lx.\n", hr);
    ok(time == clock_time, "Unexpected clock time.\n");

    hr = IMFPresentationClock_GetCorrelatedTime(clock, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get clock time, hr %#lx.\n", hr);
    ok(time == clock_time, "Unexpected clock time.\n");

    IMFPresentationTimeSource_Release(time_source);

    hr = IMFRateControl_GetRate(rate_control, NULL, &rate);
    ok(hr == S_OK, "Failed to get clock rate, hr %#lx.\n", hr);

    hr = IMFRateControl_GetRate(rate_control, &thin, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFRateControl_GetRate(rate_control, &thin, &rate);
    ok(hr == S_OK, "Failed to get clock rate, hr %#lx.\n", hr);
    ok(rate == 1.0f, "Unexpected rate.\n");
    ok(!thin, "Unexpected thinning.\n");

    hr = IMFPresentationClock_GetState(clock, 0, &state);
    ok(hr == S_OK, "Failed to get clock state, hr %#lx.\n", hr);
    ok(state == MFCLOCK_STATE_PAUSED, "Unexpected state %d.\n", state);

    hr = IMFPresentationClock_Start(clock, 0);
    ok(hr == S_OK, "Failed to stop, hr %#lx.\n", hr);

    hr = IMFRateControl_SetRate(rate_control, FALSE, 0.0f);
    ok(hr == S_OK, "Failed to set clock rate, hr %#lx.\n", hr);
    hr = IMFRateControl_GetRate(rate_control, &thin, &rate);
    ok(hr == S_OK, "Failed to get clock rate, hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected rate.\n");
    hr = IMFRateControl_SetRate(rate_control, FALSE, 1.0f);
    ok(hr == S_OK, "Failed to set clock rate, hr %#lx.\n", hr);
    hr = IMFRateControl_SetRate(rate_control, FALSE, 0.0f);
    ok(hr == S_OK, "Failed to set clock rate, hr %#lx.\n", hr);
    hr = IMFRateControl_SetRate(rate_control, FALSE, 0.5f);
    ok(hr == S_OK, "Failed to set clock rate, hr %#lx.\n", hr);
    hr = IMFRateControl_SetRate(rate_control, TRUE, -1.0f);
    ok(hr == MF_E_THINNING_UNSUPPORTED, "Unexpected hr %#lx.\n", hr);
    hr = IMFRateControl_SetRate(rate_control, TRUE, 0.0f);
    ok(hr == MF_E_THINNING_UNSUPPORTED, "Unexpected hr %#lx.\n", hr);
    hr = IMFRateControl_SetRate(rate_control, TRUE, 1.0f);
    ok(hr == MF_E_THINNING_UNSUPPORTED, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationClock_GetState(clock, 0, &state);
    ok(hr == S_OK, "Failed to get clock state, hr %#lx.\n", hr);
    ok(state == MFCLOCK_STATE_RUNNING, "Unexpected state %d.\n", state);

    hr = IMFRateControl_GetRate(rate_control, &thin, &rate);
    ok(hr == S_OK, "Failed to get clock rate, hr %#lx.\n", hr);
    ok(rate == 0.5f, "Unexpected rate.\n");
    ok(!thin, "Unexpected thinning.\n");

    IMFRateControl_Release(rate_control);

    hr = IMFPresentationClock_QueryInterface(clock, &IID_IMFShutdown, (void **)&shutdown);
    ok(hr == S_OK, "Failed to get shutdown interface, hr %#lx.\n", hr);

    /* Shutdown behavior. */
    hr = IMFShutdown_GetShutdownStatus(shutdown, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFShutdown_GetShutdownStatus(shutdown, &status);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFShutdown_Shutdown(shutdown);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    time_source = NULL;
    hr = IMFPresentationClock_GetTimeSource(clock, &time_source);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!time_source, "Unexpected instance %p.\n", time_source);
    IMFPresentationTimeSource_Release(time_source);

    hr = IMFPresentationClock_GetTime(clock, &time);
    ok(hr == S_OK, "Failed to get time, hr %#lx.\n", hr);

    hr = IMFShutdown_GetShutdownStatus(shutdown, &status);
    ok(hr == S_OK, "Failed to get status, hr %#lx.\n", hr);
    ok(status == MFSHUTDOWN_COMPLETED, "Unexpected status.\n");

    hr = IMFPresentationClock_Start(clock, 0);
    ok(hr == S_OK, "Failed to start the clock, hr %#lx.\n", hr);

    hr = IMFShutdown_GetShutdownStatus(shutdown, &status);
    ok(hr == S_OK, "Failed to get status, hr %#lx.\n", hr);
    ok(status == MFSHUTDOWN_COMPLETED, "Unexpected status.\n");

    hr = IMFShutdown_Shutdown(shutdown);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFShutdown_Release(shutdown);

    IMFPresentationClock_Release(clock);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
}

static void test_sample_grabber(void)
{
    IMFSampleGrabberSinkCallback *grabber_callback = create_test_grabber_callback();
    IMFMediaType *media_type, *media_type2, *media_type3;
    IMFMediaTypeHandler *handler, *handler2;
    IMFPresentationTimeSource *time_source;
    IMFPresentationClock *clock, *clock2;
    IMFStreamSink *stream, *stream2;
    IMFRateSupport *rate_support;
    IMFMediaEventGenerator *eg;
    IMFMediaSink *sink, *sink2;
    DWORD flags, count, id;
    IMFActivate *activate;
    IMFMediaEvent *event;
    UINT32 attr_count;
    float rate;
    HRESULT hr;
    GUID guid;
    LONG ref;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = MFCreateSampleGrabberSinkActivate(NULL, NULL, &activate);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateSampleGrabberSinkActivate(NULL, grabber_callback, &activate);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    EXPECT_REF(media_type, 1);
    hr = MFCreateSampleGrabberSinkActivate(media_type, grabber_callback, &activate);
    ok(hr == S_OK, "Failed to create grabber activate, hr %#lx.\n", hr);
    EXPECT_REF(media_type, 2);

    hr = IMFActivate_GetCount(activate, &attr_count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
    ok(!attr_count, "Unexpected count %u.\n", attr_count);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Failed to activate object, hr %#lx.\n", hr);

    check_interface(sink, &IID_IMFClockStateSink, TRUE);
    check_interface(sink, &IID_IMFMediaEventGenerator, TRUE);
    check_interface(sink, &IID_IMFGetService, TRUE);
    check_interface(sink, &IID_IMFRateSupport, TRUE);
    check_service_interface(sink, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport, TRUE);

    if (SUCCEEDED(MFGetService((IUnknown *)sink, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport, (void **)&rate_support)))
    {
        hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_FORWARD, FALSE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(rate == FLT_MAX, "Unexpected rate %f.\n", rate);

        hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_FORWARD, TRUE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(rate == FLT_MAX, "Unexpected rate %f.\n", rate);

        hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_REVERSE, FALSE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(rate == -FLT_MAX, "Unexpected rate %f.\n", rate);

        hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_REVERSE, TRUE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(rate == -FLT_MAX, "Unexpected rate %f.\n", rate);

        hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_FORWARD, FALSE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

        hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_FORWARD, TRUE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

        hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_REVERSE, FALSE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

        hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_REVERSE, TRUE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

        hr = IMFRateSupport_IsRateSupported(rate_support, TRUE, 1.0f, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(rate == 1.0f, "Unexpected rate %f.\n", rate);

        IMFRateSupport_Release(rate_support);
    }

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Failed to get sink flags, hr %#lx.\n", hr);
    ok(flags & MEDIASINK_FIXED_STREAMS, "Unexpected flags %#lx.\n", flags);

    hr = IMFMediaSink_GetStreamSinkCount(sink, &count);
    ok(hr == S_OK, "Failed to get stream count, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected stream count %lu.\n", count);

    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 0, &stream);
    ok(hr == S_OK, "Failed to get sink stream, hr %#lx.\n", hr);

    check_interface(stream, &IID_IMFMediaEventGenerator, TRUE);
    check_interface(stream, &IID_IMFMediaTypeHandler, TRUE);

    hr = IMFStreamSink_GetIdentifier(stream, &id);
    ok(hr == S_OK, "Failed to get stream id, hr %#lx.\n", hr);
    ok(id == 0, "Unexpected id %#lx.\n", id);

    hr = IMFStreamSink_GetMediaSink(stream, &sink2);
    ok(hr == S_OK, "Failed to get media sink, hr %lx.\n", hr);
    ok(sink2 == sink, "Unexpected sink.\n");
    IMFMediaSink_Release(sink2);

    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 1, &stream2);
    ok(hr == MF_E_INVALIDINDEX, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetStreamSinkById(sink, 1, &stream2);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_AddStreamSink(sink, 1, NULL, &stream2);
    ok(hr == MF_E_STREAMSINKS_FIXED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_RemoveStreamSink(sink, 0);
    ok(hr == MF_E_STREAMSINKS_FIXED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_RemoveStreamSink(sink, 1);
    ok(hr == MF_E_STREAMSINKS_FIXED, "Unexpected hr %#lx.\n", hr);

    check_interface(sink, &IID_IMFClockStateSink, TRUE);

    /* Event generator. */
    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFMediaEventGenerator, (void **)&eg);
    ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

    hr = IMFMediaEventGenerator_GetEvent(eg, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_NO_EVENTS_AVAILABLE, "Unexpected hr %#lx.\n", hr);

    check_interface(sink, &IID_IMFPresentationTimeSource, FALSE);

    hr = IMFStreamSink_QueryInterface(stream, &IID_IMFMediaTypeHandler, (void **)&handler2);
    ok(hr == S_OK, "Failed to get handler interface, hr %#lx.\n", hr);

    hr = IMFStreamSink_GetMediaTypeHandler(stream, &handler);
    ok(hr == S_OK, "Failed to get type handler, hr %#lx.\n", hr);
    hr = IMFMediaTypeHandler_GetMediaTypeCount(handler, &count);
    ok(hr == S_OK, "Failed to get media type count, hr %#lx.\n", hr);
    ok(count == 0, "Unexpected count %lu.\n", count);
    ok(handler == handler2, "Unexpected handler.\n");

    IMFMediaTypeHandler_Release(handler);
    IMFMediaTypeHandler_Release(handler2);

    /* Set clock. */
    hr = MFCreatePresentationClock(&clock);
    ok(hr == S_OK, "Failed to create clock object, hr %#lx.\n", hr);

    hr = IMFMediaSink_GetPresentationClock(sink, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetPresentationClock(sink, &clock2);
    ok(hr == MF_E_NO_CLOCK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_SetPresentationClock(sink, NULL);
    ok(hr == S_OK, "Failed to set presentation clock, hr %#lx.\n", hr);

    hr = IMFMediaSink_SetPresentationClock(sink, clock);
    ok(hr == S_OK, "Failed to set presentation clock, hr %#lx.\n", hr);

    hr = MFCreateSystemTimeSource(&time_source);
    ok(hr == S_OK, "Failed to create time source, hr %#lx.\n", hr);

    hr = IMFPresentationClock_SetTimeSource(clock, time_source);
    ok(hr == S_OK, "Failed to set time source, hr %#lx.\n", hr);
    IMFPresentationTimeSource_Release(time_source);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Failed to get sink flags, hr %#lx.\n", hr);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Failed to get sink flags, hr %#lx.\n", hr);

    hr = IMFStreamSink_GetMediaTypeHandler(stream, &handler);
    ok(hr == S_OK, "Failed to get type handler, hr %#lx.\n", hr);

    /* On Win8+ this initialization happens automatically. */
    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeCount(handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeCount(handler, &count);
    ok(hr == S_OK, "Failed to get media type count, hr %#lx.\n", hr);
    ok(count == 0, "Unexpected count %lu.\n", count);

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == S_OK, "Failed to get major type, hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Audio), "Unexpected major type %s.\n", wine_dbgstr_guid(&guid));

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type2);
    ok(hr == S_OK, "Failed to get current type, hr %#lx.\n", hr);
    ok(media_type2 == media_type, "Unexpected media type.\n");
    IMFMediaType_Release(media_type2);

    hr = MFCreateMediaType(&media_type2);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type2, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type2, &MF_MT_SUBTYPE, &MFAudioFormat_Float);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type2, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT32(media_type2, &MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type2);
    ok(hr == S_OK, "Failed to get current type, hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type);
    ok(hr == S_OK, "Failed to get current type, hr %#lx.\n", hr);
    ok(media_type2 == media_type, "Unexpected media type.\n");
    IMFMediaType_Release(media_type);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, 0, &media_type);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type2, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, NULL);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, &media_type3);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    media_type3 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, &media_type3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type3 == (void *)0xdeadbeef, "Unexpected media type %p.\n", media_type3);

    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_SAMPLE_SIZE, 1024);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    media_type3 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, &media_type3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type3 == (void *)0xdeadbeef, "Unexpected media type %p.\n", media_type3);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEventGenerator_GetEvent(eg, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_NO_EVENTS_AVAILABLE, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamSink_GetEvent(stream, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_NO_EVENTS_AVAILABLE, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(clock, 3);
    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    ref = IMFPresentationClock_Release(clock);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = IMFMediaEventGenerator_GetEvent(eg, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_AddStreamSink(sink, 1, NULL, &stream2);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetStreamSinkCount(sink, &count);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 0, &stream2);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamSink_GetEvent(stream, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamSink_GetMediaSink(stream, &sink2);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#lx.\n", hr);

    id = 1;
    hr = IMFStreamSink_GetIdentifier(stream, &id);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#lx.\n", hr);
    ok(id == 1, "Unexpected id %lu.\n", id);

    media_type3 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, &media_type3);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#lx.\n", hr);
    ok(media_type3 == (void *)0xdeadbeef, "Unexpected media type %p.\n", media_type3);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, NULL, NULL);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, NULL);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeCount(handler, &count);
    ok(hr == S_OK, "Failed to get type count, hr %#lx.\n", hr);

    ref = IMFMediaType_Release(media_type2);
    todo_wine
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(media_type);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, 0, &media_type);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#lx.\n", hr);

    IMFMediaTypeHandler_Release(handler);

    handler = (void *)0xdeadbeef;
    hr = IMFStreamSink_GetMediaTypeHandler(stream, &handler);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#lx.\n", hr);
    ok(handler == (void *)0xdeadbeef, "Unexpected pointer.\n");

    hr = IMFStreamSink_GetMediaTypeHandler(stream, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    IMFMediaEventGenerator_Release(eg);
    IMFStreamSink_Release(stream);

    ref = IMFActivate_Release(activate);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaSink_Release(sink);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Rateless mode with MF_SAMPLEGRABBERSINK_IGNORE_CLOCK. */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = MFCreateSampleGrabberSinkActivate(media_type, grabber_callback, &activate);
    ok(hr == S_OK, "Failed to create grabber activate, hr %#lx.\n", hr);

    hr = IMFActivate_SetUINT32(activate, &MF_SAMPLEGRABBERSINK_IGNORE_CLOCK, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Failed to activate object, hr %#lx.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Failed to get sink flags, hr %#lx.\n", hr);
    ok(flags & MEDIASINK_RATELESS, "Unexpected flags %#lx.\n", flags);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    /* required for the sink to be fully released */
    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    ref = IMFActivate_Release(activate);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaSink_Release(sink);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Detaching */
    hr = MFCreateSampleGrabberSinkActivate(media_type, grabber_callback, &activate);
    ok(hr == S_OK, "Failed to create grabber activate, hr %#lx.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Failed to activate object, hr %#lx.\n", hr);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink2);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFActivate_GetCount(activate, &attr_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFActivate_DetachObject(activate);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    ref = IMFActivate_Release(activate);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* required for the sink to be fully released */
    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    ref = IMFMediaSink_Release(sink);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFMediaType_Release(media_type);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    IMFSampleGrabberSinkCallback_Release(grabber_callback);
}

static void test_sample_grabber_is_mediatype_supported(void)
{
    IMFSampleGrabberSinkCallback *grabber_callback = create_test_grabber_callback();
    IMFMediaType *media_type, *media_type2, *media_type3;
    IMFMediaTypeHandler *handler;
    IMFActivate *activate;
    IMFStreamSink *stream;
    IMFMediaSink *sink;
    HRESULT hr;
    GUID guid;
    LONG ref;

    /* IsMediaTypeSupported checks are done against the creation type, and check format data */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = MFCreateSampleGrabberSinkActivate(media_type, grabber_callback, &activate);
    ok(hr == S_OK, "Failed to create grabber activate, hr %#lx.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Failed to activate object, hr %#lx.\n", hr);

    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 0, &stream);
    ok(hr == S_OK, "Failed to get sink stream, hr %#lx.\n", hr);
    hr = IMFStreamSink_GetMediaTypeHandler(stream, &handler);
    ok(hr == S_OK, "Failed to get type handler, hr %#lx.\n", hr);
    IMFStreamSink_Release(stream);

    /* On Win8+ this initialization happens automatically. */
    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type2);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type2, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type2, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type2, &MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type2, NULL);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Failed to set media type, hr %#lx.\n", hr);

    /* Make it match grabber type sample rate. */
    hr = IMFMediaType_SetUINT32(media_type2, &MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type2, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type2);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);
    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type3);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);
    ok(media_type3 == media_type2, "Unexpected media type instance.\n");
    IMFMediaType_Release(media_type3);

    /* Change original type. */
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type2, NULL);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT32(media_type2, &MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type2, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Audio), "Unexpected major type.\n");

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Audio), "Unexpected major type.\n");

    IMFMediaTypeHandler_Release(handler);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ref = IMFActivate_Release(activate);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* required for the sink to be fully released */
    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    ref = IMFMediaSink_Release(sink);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFMediaType_Release(media_type2);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(media_type);
    ok(ref == 0, "Release returned %ld\n", ref);

    IMFSampleGrabberSinkCallback_Release(grabber_callback);
}

static void test_sample_grabber_orientation(GUID subtype)
{
    media_type_desc video_rgb32_desc =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, subtype),
    };

    struct test_grabber_callback *grabber_callback;
    IMFTopologyNode *src_node, *sink_node;
    IMFPresentationDescriptor *pd;
    IMFAsyncCallback *callback;
    IMFActivate *sink_activate;
    IMFMediaType *output_type;
    IMFMediaSession *session;
    IMFStreamDescriptor *sd;
    IMFMediaSource *source;
    IMFTopology *topology;
    PROPVARIANT propvar;
    BOOL selected;
    HRESULT hr;
    DWORD res;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    if (!(source = create_media_source(L"test.mp4", L"video/mp4")))
    {
        win_skip("MP4 media source is not supported, skipping tests.\n");
        return;
    }

    callback = create_test_callback();
    grabber_callback = impl_from_IMFSampleGrabberSinkCallback(create_test_grabber_callback());

    grabber_callback->ready_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!grabber_callback->ready_event, "CreateEventW failed, error %lu\n", GetLastError());
    grabber_callback->done_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!grabber_callback->done_event, "CreateEventW failed, error %lu\n", GetLastError());

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#lx.\n", hr);

    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &sink_node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &src_node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, sink_node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, src_node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopologyNode_ConnectOutput(src_node, 0, sink_node, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSource_CreatePresentationDescriptor(source, &pd);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, 0, &selected, &sd);
    ok(selected, "got selected %u.\n", !!selected);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_source_node(source, -1, src_node, pd, sd);
    IMFTopologyNode_Release(src_node);
    IMFPresentationDescriptor_Release(pd);
    IMFStreamDescriptor_Release(sd);

    hr = MFCreateMediaType(&output_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(output_type, video_rgb32_desc, -1);
    hr = MFCreateSampleGrabberSinkActivate(output_type, &grabber_callback->IMFSampleGrabberSinkCallback_iface, &sink_activate);
    ok(hr == S_OK, "Failed to create grabber sink, hr %#lx.\n", hr);
    IMFMediaType_Release(output_type);

    hr = IMFTopologyNode_SetObject(sink_node, (IUnknown *)sink_activate);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);
    hr = IMFTopologyNode_SetUINT32(sink_node, &MF_TOPONODE_CONNECT_METHOD, MF_CONNECT_ALLOW_DECODER);
    ok(hr == S_OK, "Failed to set connect method, hr %#lx.\n", hr);
    IMFTopologyNode_Release(sink_node);

    hr = IMFTopology_SetUINT32(topology, &MF_TOPOLOGY_ENUMERATE_SOURCE_TYPES, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_SetTopology(session, 0, topology);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFTopology_Release(topology);

    propvar.vt = VT_EMPTY;
    hr = IMFMediaSession_Start(session, &GUID_NULL, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    res = WaitForSingleObject(grabber_callback->ready_event, 5000);
    ok(!res, "WaitForSingleObject returned %#lx\n", res);
    CloseHandle(grabber_callback->ready_event);
    grabber_callback->ready_event = NULL;

    if (IsEqualGUID(&subtype, &MFVideoFormat_RGB32))
    {
        const struct buffer_desc buffer_desc_rgb32 =
        {
            .length = 64 * 64 * 4, .compare = compare_rgb32, .dump = dump_rgb32, .rect = {.right = 64, .bottom = 64},
        };
        const struct sample_desc sample_desc_rgb32 =
        {
            .sample_duration = 333667, .buffer_count = 1, .buffers = &buffer_desc_rgb32,
        };
        check_mf_sample_collection(grabber_callback->samples, &sample_desc_rgb32, L"rgb32frame-grabber.bmp");
    }
    else if (IsEqualGUID(&subtype, &MFVideoFormat_NV12))
    {
        const struct buffer_desc buffer_desc_nv12 =
        {
            .length = 64 * 64 * 3 / 2, .compare = compare_nv12, .dump = dump_nv12, .rect = {.right = 64, .bottom = 64},
        };
        const struct sample_desc sample_desc_nv12 =
        {
            .sample_duration = 333667, .buffer_count = 1, .buffers = &buffer_desc_nv12,
        };
        check_mf_sample_collection(grabber_callback->samples, &sample_desc_nv12, L"nv12frame-grabber.bmp");
    }

    SetEvent(grabber_callback->done_event);

    hr = IMFMediaSession_ClearTopologies(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaSession_Close(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(session, callback, MESessionClosed, 1000, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_EMPTY, "got vt %u\n", propvar.vt);

    hr = IMFMediaSession_Shutdown(session);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaSource_Shutdown(source);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFActivate_ShutdownObject(sink_activate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFActivate_Release(sink_activate);
    IMFMediaSession_Release(session);
    IMFMediaSource_Release(source);

    hr = MFShutdown();
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFSampleGrabberSinkCallback_Release(&grabber_callback->IMFSampleGrabberSinkCallback_iface);
}

static void test_quality_manager(void)
{
    IMFPresentationClock *clock;
    IMFQualityManager *manager;
    IMFTopology *topology;
    HRESULT hr;
    LONG ref;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#lx.\n", hr);

    hr = MFCreatePresentationClock(&clock);
    ok(hr == S_OK, "Failed to create presentation clock, hr %#lx.\n", hr);

    hr = MFCreateStandardQualityManager(&manager);
    ok(hr == S_OK, "Failed to create quality manager, hr %#lx.\n", hr);

    check_interface(manager, &IID_IMFQualityManager, TRUE);
    check_interface(manager, &IID_IMFClockStateSink, TRUE);

    hr = IMFQualityManager_NotifyPresentationClock(manager, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFQualityManager_NotifyTopology(manager, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Set clock, then shutdown. */
    EXPECT_REF(clock, 1);
    EXPECT_REF(manager, 1);
    hr = IMFQualityManager_NotifyPresentationClock(manager, clock);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(clock, 2);
    EXPECT_REF(manager, 2);

    hr = IMFQualityManager_Shutdown(manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(clock, 1);

    hr = IMFQualityManager_NotifyPresentationClock(manager, clock);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFQualityManager_NotifyTopology(manager, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFQualityManager_NotifyPresentationClock(manager, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFQualityManager_Shutdown(manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ref = IMFQualityManager_Release(manager);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFCreateStandardQualityManager(&manager);
    ok(hr == S_OK, "Failed to create quality manager, hr %#lx.\n", hr);

    EXPECT_REF(clock, 1);
    EXPECT_REF(manager, 1);
    hr = IMFQualityManager_NotifyPresentationClock(manager, clock);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(manager, 2);
    EXPECT_REF(clock, 2);
    hr = IMFQualityManager_Shutdown(manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ref = IMFQualityManager_Release(manager);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFPresentationClock_Release(clock);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Set topology. */
    hr = MFCreateStandardQualityManager(&manager);
    ok(hr == S_OK, "Failed to create quality manager, hr %#lx.\n", hr);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(topology, 1);
    hr = IMFQualityManager_NotifyTopology(manager, topology);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(topology, 2);

    hr = IMFQualityManager_NotifyTopology(manager, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(topology, 1);

    hr = IMFQualityManager_NotifyTopology(manager, topology);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(topology, 2);
    hr = IMFQualityManager_Shutdown(manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(topology, 1);

    hr = IMFQualityManager_NotifyTopology(manager, topology);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    ref = IMFQualityManager_Release(manager);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFCreateStandardQualityManager(&manager);
    ok(hr == S_OK, "Failed to create quality manager, hr %#lx.\n", hr);

    EXPECT_REF(topology, 1);
    hr = IMFQualityManager_NotifyTopology(manager, topology);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(topology, 2);

    ref = IMFQualityManager_Release(manager);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#lx.\n", hr);
}

static void check_sar_rate_support(IMFMediaSink *sink)
{
    IMFRateSupport *rate_support;
    IMFMediaTypeHandler *handler;
    IMFStreamSink *stream_sink;
    IMFMediaType *media_type;
    HRESULT hr;
    float rate;

    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFRateSupport, (void **)&rate_support);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) return;

    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 0, &stream_sink);
    if (hr == MF_E_SHUTDOWN)
    {
        hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_FORWARD, FALSE, NULL);
        ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_FORWARD, FALSE, &rate);
        ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_FORWARD, FALSE, &rate);
        ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

        IMFRateSupport_Release(rate_support);
        return;
    }
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamSink_GetMediaTypeHandler(stream_sink, &handler);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFStreamSink_Release(stream_sink);

    hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_FORWARD, FALSE, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_FORWARD, FALSE, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type);
    if (SUCCEEDED(hr))
    {
        hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_FORWARD, FALSE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_FORWARD, TRUE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_REVERSE, FALSE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_REVERSE, TRUE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_FORWARD, FALSE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_FORWARD, TRUE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_REVERSE, FALSE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_REVERSE, TRUE, &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        IMFMediaType_Release(media_type);
    }
    else
    {
        hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_FORWARD, FALSE, &rate);
        ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_FORWARD, TRUE, &rate);
        ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_REVERSE, FALSE, &rate);
        ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_REVERSE, TRUE, &rate);
        ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_FORWARD, FALSE, &rate);
        ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_FORWARD, TRUE, &rate);
        ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_REVERSE, FALSE, &rate);
        ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_REVERSE, TRUE, &rate);
        ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);
    }

    IMFMediaTypeHandler_Release(handler);
    IMFRateSupport_Release(rate_support);
}

static void test_sar(void)
{
    static const struct attribute_desc input_type_desc_48000[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float, .required = TRUE, .todo = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 2 * (32 / 8), .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 2 * (32 / 8) * 48000, .required = TRUE),
        {0},
    };
    static const struct attribute_desc input_type_desc_44100[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float, .required = TRUE, .todo = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 2 * (32 / 8), .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 2 * (32 / 8) * 44100, .required = TRUE),
        {0},
    };

    IMFPresentationClock *present_clock, *present_clock2;
    IMFMediaType *mediatype, *mediatype2, *mediatype3;
    IMFClockStateSink *state_sink, *state_sink2;
    IMFMediaTypeHandler *handler, *handler2;
    IMFPresentationTimeSource *time_source;
    IMFSimpleAudioVolume *simple_volume;
    IMFAudioStreamVolume *stream_volume;
    IMFMediaSink *sink, *sink2;
    IMFStreamSink *stream_sink;
    UINT32 channel_count, rate;
    IMFAttributes *attributes;
    DWORD id, flags, count;
    IMFActivate *activate;
    MFCLOCK_STATE state;
    IMFClock *clock;
    IUnknown *unk;
    HRESULT hr;
    GUID guid;
    BOOL mute;
    LONG ref;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    hr = MFCreateAudioRenderer(NULL, &sink);
    if (hr == MF_E_NO_AUDIO_PLAYBACK_DEVICE)
    {
        skip("No audio playback device available.\n");
        CoUninitialize();
        return;
    }
    ok(hr == S_OK, "Failed to create renderer, hr %#lx.\n", hr);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#lx.\n", hr);

    hr = MFCreatePresentationClock(&present_clock);
    ok(hr == S_OK, "Failed to create presentation clock, hr %#lx.\n", hr);

    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFPresentationTimeSource, (void **)&time_source);
    todo_wine
    ok(hr == S_OK, "Failed to get time source interface, hr %#lx.\n", hr);

if (SUCCEEDED(hr))
{
    hr = IMFPresentationTimeSource_QueryInterface(time_source, &IID_IMFClockStateSink, (void **)&state_sink2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFPresentationTimeSource_QueryInterface(time_source, &IID_IMFClockStateSink, (void **)&state_sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(state_sink == state_sink2, "Unexpected clock sink.\n");
    IMFClockStateSink_Release(state_sink2);
    IMFClockStateSink_Release(state_sink);

    hr = IMFPresentationTimeSource_GetUnderlyingClock(time_source, &clock);
    ok(hr == MF_E_NO_CLOCK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetClockCharacteristics(time_source, &flags);
    ok(hr == S_OK, "Failed to get flags, hr %#lx.\n", hr);
    ok(flags == MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ, "Unexpected flags %#lx.\n", flags);

    hr = IMFPresentationTimeSource_GetState(time_source, 0, &state);
    ok(hr == S_OK, "Failed to get clock state, hr %#lx.\n", hr);
    ok(state == MFCLOCK_STATE_INVALID, "Unexpected state %d.\n", state);

    hr = IMFPresentationTimeSource_QueryInterface(time_source, &IID_IMFClockStateSink, (void **)&state_sink);
    ok(hr == S_OK, "Failed to get state sink, hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockStart(state_sink, 0, 0);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    IMFClockStateSink_Release(state_sink);

    IMFPresentationTimeSource_Release(time_source);
}
    hr = IMFMediaSink_AddStreamSink(sink, 123, NULL, &stream_sink);
    ok(hr == MF_E_STREAMSINKS_FIXED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_RemoveStreamSink(sink, 0);
    ok(hr == MF_E_STREAMSINKS_FIXED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetStreamSinkCount(sink, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetStreamSinkCount(sink, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %lu.\n", count);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(flags == (MEDIASINK_FIXED_STREAMS | MEDIASINK_CAN_PREROLL), "Unexpected flags %#lx.\n", flags);

    check_interface(sink, &IID_IMFMediaSinkPreroll, TRUE);
    check_interface(sink, &IID_IMFMediaEventGenerator, TRUE);
    check_interface(sink, &IID_IMFClockStateSink, TRUE);
    check_interface(sink, &IID_IMFGetService, TRUE);
    todo_wine check_interface(sink, &IID_IMFPresentationTimeSource, TRUE);
    todo_wine check_service_interface(sink, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport, TRUE);
    check_service_interface(sink, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateControl, FALSE);
    check_service_interface(sink, &MR_POLICY_VOLUME_SERVICE, &IID_IMFSimpleAudioVolume, TRUE);
    check_service_interface(sink, &MR_STREAM_VOLUME_SERVICE, &IID_IMFAudioStreamVolume, TRUE);

    /* Clock */
    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFClockStateSink, (void **)&state_sink);
    ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockStart(state_sink, 0, 0);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockPause(state_sink, 0);
    ok(hr == MF_E_INVALID_STATE_TRANSITION, "Unexpected hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockStop(state_sink, 0);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockRestart(state_sink, 0);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    IMFClockStateSink_Release(state_sink);

    hr = IMFMediaSink_SetPresentationClock(sink, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_SetPresentationClock(sink, present_clock);
    todo_wine
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateSystemTimeSource(&time_source);
    ok(hr == S_OK, "Failed to create time source, hr %#lx.\n", hr);

    hr = IMFPresentationClock_SetTimeSource(present_clock, time_source);
    ok(hr == S_OK, "Failed to set time source, hr %#lx.\n", hr);
    IMFPresentationTimeSource_Release(time_source);

    hr = IMFMediaSink_SetPresentationClock(sink, present_clock);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetPresentationClock(sink, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetPresentationClock(sink, &present_clock2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(present_clock == present_clock2, "Unexpected instance.\n");
    IMFPresentationClock_Release(present_clock2);

    /* Stream */
    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 0, &stream_sink);
    ok(hr == S_OK, "Failed to get a stream, hr %#lx.\n", hr);

    check_interface(stream_sink, &IID_IMFMediaEventGenerator, TRUE);
    check_interface(stream_sink, &IID_IMFMediaTypeHandler, TRUE);
    todo_wine check_interface(stream_sink, &IID_IMFGetService, TRUE);

    hr = IMFStreamSink_GetIdentifier(stream_sink, &id);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!id, "Unexpected id.\n");

    hr = IMFStreamSink_GetMediaSink(stream_sink, &sink2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(sink == sink2, "Unexpected object.\n");
    IMFMediaSink_Release(sink2);

    hr = IMFStreamSink_GetMediaTypeHandler(stream_sink, &handler);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamSink_QueryInterface(stream_sink, &IID_IMFMediaTypeHandler, (void **)&handler2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(handler2 == handler, "Unexpected instance.\n");
    IMFMediaTypeHandler_Release(handler2);

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == S_OK, "Failed to get major type, hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Audio), "Unexpected type %s.\n", wine_dbgstr_guid(&guid));

    count = 0;
    hr = IMFMediaTypeHandler_GetMediaTypeCount(handler, &count);
    ok(hr == S_OK, "Failed to get type count, hr %#lx.\n", hr);
    ok(!!count, "Unexpected type count %lu.\n", count);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, count, &mediatype);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &mediatype);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, 0, &mediatype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == 48000 || rate == 44100, "got rate %u.\n", rate);
    IMFMediaType_Release(mediatype);

    check_handler_required_attributes(handler, rate == 44100 ? input_type_desc_44100 : input_type_desc_48000);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &mediatype);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    /* Actual return value is MF_E_ATRIBUTENOTFOUND triggered by missing MF_MT_MAJOR_TYPE */
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, mediatype, NULL);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, mediatype, NULL);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, mediatype, NULL);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, mediatype);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, count - 1, &mediatype2);
    ok(hr == S_OK, "Failed to get media type, hr %#lx.\n", hr);
    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, count - 1, &mediatype3);
    ok(hr == S_OK, "Failed to get media type, hr %#lx.\n", hr);
    ok(mediatype2 == mediatype3, "Unexpected instance.\n");
    IMFMediaType_Release(mediatype3);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, mediatype2, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFMediaType_Release(mediatype);

    check_sar_rate_support(sink);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, mediatype2);
    ok(hr == S_OK, "Failed to set current type, hr %#lx.\n", hr);

    check_sar_rate_support(sink);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &mediatype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mediatype == mediatype2, "Unexpected instance.\n");
    IMFMediaType_Release(mediatype);

    IMFMediaType_Release(mediatype2);

    /* Reset back to uninitialized state. */
    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    IMFMediaTypeHandler_Release(handler);

    /* State change with initialized stream. */
    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFClockStateSink, (void **)&state_sink);
    ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockStart(state_sink, 0, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockStart(state_sink, 0, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockPause(state_sink, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockStop(state_sink, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockStop(state_sink, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockPause(state_sink, 0);
    ok(hr == MF_E_INVALID_STATE_TRANSITION, "Unexpected hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockRestart(state_sink, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockRestart(state_sink, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockStop(state_sink, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFClockStateSink_Release(state_sink);

    IMFStreamSink_Release(stream_sink);

    /* Volume control */
    hr = MFGetService((IUnknown *)sink, &MR_POLICY_VOLUME_SERVICE, &IID_IMFSimpleAudioVolume, (void **)&simple_volume);
    ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

    hr = IMFSimpleAudioVolume_GetMute(simple_volume, &mute);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFSimpleAudioVolume_Release(simple_volume);

    hr = MFGetService((IUnknown *)sink, &MR_STREAM_VOLUME_SERVICE, &IID_IMFAudioStreamVolume, (void **)&stream_volume);
    ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

    hr = IMFAudioStreamVolume_GetChannelCount(stream_volume, &channel_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFAudioStreamVolume_GetChannelCount(stream_volume, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    IMFAudioStreamVolume_Release(stream_volume);

    hr = MFGetService((IUnknown *)sink, &MR_AUDIO_POLICY_SERVICE, &IID_IMFAudioPolicy, (void **)&unk);
    ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);
    IUnknown_Release(unk);

    /* Shutdown */
    EXPECT_REF(present_clock, 2);
    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
    EXPECT_REF(present_clock, 1);

    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_AddStreamSink(sink, 123, NULL, &stream_sink);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_RemoveStreamSink(sink, 0);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetStreamSinkCount(sink, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetStreamSinkCount(sink, &count);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_SetPresentationClock(sink, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_SetPresentationClock(sink, present_clock);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetPresentationClock(sink, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetPresentationClock(sink, &present_clock2);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    check_sar_rate_support(sink);

    ref = IMFMediaSink_Release(sink);
    todo_wine
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Activation */
    hr = MFCreateAudioRendererActivate(&activate);
    ok(hr == S_OK, "Failed to create activation object, hr %#lx.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Failed to activate, hr %#lx.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink2);
    ok(hr == S_OK, "Failed to activate, hr %#lx.\n", hr);
    ok(sink == sink2, "Unexpected instance.\n");
    IMFMediaSink_Release(sink2);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Failed to get sink flags, hr %#lx.\n", hr);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink2);
    ok(hr == S_OK, "Failed to activate, hr %#lx.\n", hr);
    todo_wine
    ok(sink == sink2, "Unexpected instance.\n");

    hr = IMFMediaSink_GetCharacteristics(sink2, &flags);
    todo_wine
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    IMFMediaSink_Release(sink2);

    hr = IMFActivate_DetachObject(activate);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ref = IMFActivate_Release(activate);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaSink_Release(sink);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFPresentationClock_Release(present_clock);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#lx.\n", hr);

    /* SAR attributes */
    hr = MFCreateAttributes(&attributes, 0);
    ok(hr == S_OK, "Failed to create attributes, hr %#lx.\n", hr);

    /* Specify role. */
    hr = IMFAttributes_SetUINT32(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ROLE, eMultimedia);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = MFCreateAudioRenderer(attributes, &sink);
    ok(hr == S_OK, "Failed to create a sink, hr %#lx.\n", hr);

    /* required for the sink to be fully released */
    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ref = IMFMediaSink_Release(sink);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Invalid endpoint. */
    hr = IMFAttributes_SetString(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ID, L"endpoint");
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = MFCreateAudioRenderer(attributes, &sink);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFAttributes_DeleteItem(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ROLE);
    ok(hr == S_OK, "Failed to remove attribute, hr %#lx.\n", hr);

    hr = MFCreateAudioRenderer(attributes, &sink);
    ok(hr == MF_E_NO_AUDIO_PLAYBACK_DEVICE, "Failed to create a sink, hr %#lx.\n", hr);

    ref = IMFAttributes_Release(attributes);
    ok(ref == 0, "Release returned %ld\n", ref);

    CoUninitialize();
}

static void test_evr(void)
{
    static const float supported_rates[] =
    {
        0.0f, 1.0f, -20.0f, 20.0f, 1000.0f, -1000.0f,
    };
    IMFVideoSampleAllocatorCallback *allocator_callback;
    IMFStreamSink *stream_sink, *stream_sink2;
    IMFVideoDisplayControl *display_control;
    IMFMediaType *media_type, *media_type2;
    IMFPresentationTimeSource *time_source;
    IMFVideoSampleAllocator *allocator;
    IMFMediaTypeHandler *type_handler;
    IMFVideoRenderer *video_renderer;
    IMFPresentationClock *clock;
    IMFMediaSink *sink, *sink2;
    IMFAttributes *attributes;
    UINT32 attr_count, value;
    IMFActivate *activate;
    HWND window, window2;
    IMFRateSupport *rs;
    DWORD flags, count;
    LONG sample_count;
    IMFSample *sample;
    unsigned int i;
    UINT64 window3;
    float rate;
    HRESULT hr;
    GUID guid;
    LONG ref;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#lx.\n", hr);

    hr = MFCreateVideoRenderer(&IID_IMFVideoRenderer, (void **)&video_renderer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoRenderer_InitializeRenderer(video_renderer, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* required for the video renderer to be fully released */
    hr = IMFVideoRenderer_QueryInterface(video_renderer, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaSink_Release(sink);

    ref = IMFVideoRenderer_Release(video_renderer);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFCreateVideoRendererActivate(NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* Window */
    window = create_window();
    hr = MFCreateVideoRendererActivate(window, &activate);
    ok(hr == S_OK, "Failed to create activate object, hr %#lx.\n", hr);

    hr = IMFActivate_GetUINT64(activate, &MF_ACTIVATE_VIDEO_WINDOW, &window3);
    ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
    ok(UlongToHandle(window3) == window, "Unexpected value.\n");

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(sink, &IID_IMFMediaSinkPreroll, TRUE);
    check_interface(sink, &IID_IMFVideoRenderer, TRUE);
    check_interface(sink, &IID_IMFMediaEventGenerator, TRUE);
    check_interface(sink, &IID_IMFClockStateSink, TRUE);
    check_interface(sink, &IID_IMFGetService, TRUE);
    check_interface(sink, &IID_IMFQualityAdvise, TRUE);
    check_interface(sink, &IID_IMFRateSupport, TRUE);
    check_interface(sink, &IID_IMFRateControl, FALSE);
    check_service_interface(sink, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoProcessor, TRUE);
    check_service_interface(sink, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoMixerBitmap, TRUE);
    check_service_interface(sink, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoMixerControl, TRUE);
    check_service_interface(sink, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoMixerControl2, TRUE);
    check_service_interface(sink, &MR_VIDEO_RENDER_SERVICE, &IID_IMFVideoDisplayControl, TRUE);
    check_service_interface(sink, &MR_VIDEO_RENDER_SERVICE, &IID_IMFVideoPositionMapper, TRUE);
    check_service_interface(sink, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IMFVideoSampleAllocator, FALSE);
    check_service_interface(sink, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IDirect3DDeviceManager9, TRUE);
    check_service_interface(sink, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport, TRUE);

    hr = MFGetService((IUnknown *)sink, &MR_VIDEO_RENDER_SERVICE, &IID_IMFVideoDisplayControl,
            (void **)&display_control);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    window2 = NULL;
    hr = IMFVideoDisplayControl_GetVideoWindow(display_control, &window2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(window2 == window, "Unexpected window %p.\n", window2);

    IMFVideoDisplayControl_Release(display_control);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ref = IMFActivate_Release(activate);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaSink_Release(sink);
    ok(ref == 0, "Release returned %ld\n", ref);
    DestroyWindow(window);

    hr = MFCreateVideoRendererActivate(NULL, &activate);
    ok(hr == S_OK, "Failed to create activate object, hr %#lx.\n", hr);

    hr = IMFActivate_GetCount(activate, &attr_count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
    ok(attr_count == 1, "Unexpected count %u.\n", attr_count);

    hr = IMFActivate_GetUINT64(activate, &MF_ACTIVATE_VIDEO_WINDOW, &window3);
    ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
    ok(!window3, "Unexpected value.\n");

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Failed to activate, hr %#lx.\n", hr);

    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFAttributes, (void **)&attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_interface(attributes, &IID_IMFMediaSink, TRUE);

    hr = IMFAttributes_GetCount(attributes, &attr_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!attr_count, "Unexpected count %u.\n", attr_count);
    /* Rendering preferences are not immediately propagated to the presenter. */
    hr = IMFAttributes_SetUINT32(attributes, &EVRConfig_ForceBob, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFGetService((IUnknown *)sink, &MR_VIDEO_RENDER_SERVICE, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoDisplayControl_GetRenderingPrefs(display_control, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!flags, "Unexpected flags %#lx.\n", flags);
    IMFVideoDisplayControl_Release(display_control);
    IMFAttributes_Release(attributes);

    /* Primary stream type handler. */
    hr = IMFMediaSink_GetStreamSinkById(sink, 0, &stream_sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamSink_QueryInterface(stream_sink, &IID_IMFAttributes, (void **)&attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_GetCount(attributes, &attr_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(attr_count == 2, "Unexpected count %u.\n", attr_count);
    value = 0;
    hr = IMFAttributes_GetUINT32(attributes, &MF_SA_REQUIRED_SAMPLE_COUNT, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 1, "Unexpected attribute value %u.\n", value);
    value = 0;
    hr = IMFAttributes_GetUINT32(attributes, &MF_SA_D3D_AWARE, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 1, "Unexpected attribute value %u.\n", value);

    check_interface(attributes, &IID_IMFStreamSink, TRUE);
    IMFAttributes_Release(attributes);

    hr = IMFStreamSink_GetMediaTypeHandler(stream_sink, &type_handler);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected type %s.\n", wine_dbgstr_guid(&guid));

    /* Supported types are not advertised. */
    hr = IMFMediaTypeHandler_GetMediaTypeCount(type_handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    count = 1;
    hr = IMFMediaTypeHandler_GetMediaTypeCount(type_handler, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %lu.\n", count);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(type_handler, 0, NULL);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(type_handler, 0, &media_type);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(type_handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(type_handler, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)640 << 32 | 480);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type, &media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    media_type2 = (void *)0x1;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected media type %p.\n", media_type2);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(type_handler, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_interface(media_type2, &IID_IMFVideoMediaType, TRUE);
    IMFMediaType_Release(media_type2);

    IMFMediaType_Release(media_type);

    IMFMediaTypeHandler_Release(type_handler);

    /* Stream uses an allocator. */
    check_service_interface(stream_sink, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IMFVideoSampleAllocator, TRUE);
    check_service_interface(stream_sink, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IDirect3DDeviceManager9, TRUE);
todo_wine {
    check_service_interface(stream_sink, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoProcessor, TRUE);
    check_service_interface(stream_sink, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoMixerBitmap, TRUE);
    check_service_interface(stream_sink, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoMixerControl, TRUE);
    check_service_interface(stream_sink, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoMixerControl2, TRUE);
    check_service_interface(stream_sink, &MR_VIDEO_RENDER_SERVICE, &IID_IMFVideoDisplayControl, TRUE);
    check_service_interface(stream_sink, &MR_VIDEO_RENDER_SERVICE, &IID_IMFVideoPositionMapper, TRUE);
    check_service_interface(stream_sink, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport, TRUE);
}
    hr = MFGetService((IUnknown *)stream_sink, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IMFVideoSampleAllocator,
            (void **)&allocator);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_QueryInterface(allocator, &IID_IMFVideoSampleAllocatorCallback, (void **)&allocator_callback);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    sample_count = 0;
    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_callback, &sample_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!sample_count, "Unexpected sample count %ld.\n", sample_count);

    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    IMFVideoSampleAllocatorCallback_Release(allocator_callback);
    IMFVideoSampleAllocator_Release(allocator);
    IMFStreamSink_Release(stream_sink);

    /* Same test for a substream. */
    hr = IMFMediaSink_AddStreamSink(sink, 1, NULL, &stream_sink2);
    ok(hr == S_OK || broken(hr == E_INVALIDARG), "Unexpected hr %#lx.\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = MFGetService((IUnknown *)stream_sink2, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IMFVideoSampleAllocator,
                (void **)&allocator);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IMFVideoSampleAllocator_Release(allocator);

        hr = IMFMediaSink_RemoveStreamSink(sink, 1);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        ref = IMFStreamSink_Release(stream_sink2);
        ok(ref == 0, "Release returned %ld\n", ref);
    }

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(flags == (MEDIASINK_CAN_PREROLL | MEDIASINK_CLOCK_REQUIRED), "Unexpected flags %#lx.\n", flags);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    /* Activate again. */
    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink2);
    ok(hr == S_OK, "Failed to activate, hr %#lx.\n", hr);
    todo_wine
    ok(sink == sink2, "Unexpected instance.\n");
    IMFMediaSink_Release(sink2);

    hr = IMFActivate_DetachObject(activate);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink2);
    ok(hr == S_OK, "Failed to activate, hr %#lx.\n", hr);
    todo_wine
    ok(sink == sink2, "Unexpected instance.\n");
    IMFMediaSink_Release(sink2);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    ref = IMFActivate_Release(activate);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaSink_Release(sink);
    todo_wine
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Set clock. */
    window = create_window();

    hr = MFCreateVideoRendererActivate(window, &activate);
    ok(hr == S_OK, "Failed to create activate object, hr %#lx.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ref = IMFActivate_Release(activate);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFCreateSystemTimeSource(&time_source);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreatePresentationClock(&clock);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationClock_SetTimeSource(clock, time_source);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFPresentationTimeSource_Release(time_source);

    hr = IMFMediaSink_SetPresentationClock(sink, clock);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFRateSupport, (void **)&rs);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    rate = 1.0f;
    hr = IMFRateSupport_GetSlowestRate(rs, MFRATE_FORWARD, FALSE, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

    rate = 1.0f;
    hr = IMFRateSupport_GetSlowestRate(rs, MFRATE_REVERSE, FALSE, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

    rate = 1.0f;
    hr = IMFRateSupport_GetSlowestRate(rs, MFRATE_FORWARD, TRUE, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

    rate = 1.0f;
    hr = IMFRateSupport_GetSlowestRate(rs, MFRATE_REVERSE, TRUE, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

    hr = IMFRateSupport_GetFastestRate(rs, MFRATE_FORWARD, FALSE, &rate);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFRateSupport_GetFastestRate(rs, MFRATE_REVERSE, FALSE, &rate);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFRateSupport_GetFastestRate(rs, MFRATE_FORWARD, TRUE, &rate);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFRateSupport_GetFastestRate(rs, MFRATE_REVERSE, TRUE, &rate);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFRateSupport_GetFastestRate(rs, MFRATE_REVERSE, TRUE, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(supported_rates); ++i)
    {
        rate = supported_rates[i] + 1.0f;
        hr = IMFRateSupport_IsRateSupported(rs, TRUE, supported_rates[i], &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(rate == supported_rates[i], "Unexpected rate %f.\n", rate);

        rate = supported_rates[i] + 1.0f;
        hr = IMFRateSupport_IsRateSupported(rs, FALSE, supported_rates[i], &rate);
        ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);
        ok(rate == supported_rates[i], "Unexpected rate %f.\n", rate);

        hr = IMFRateSupport_IsRateSupported(rs, TRUE, supported_rates[i], NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_IsRateSupported(rs, FALSE, supported_rates[i], NULL);
        ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);
    }

    /* Configuring stream type make rate support work. */
    hr = IMFMediaSink_GetStreamSinkById(sink, 0, &stream_sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFStreamSink_GetMediaTypeHandler(stream_sink, &type_handler);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)64 << 32 | 64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, media_type);
    ok(hr == S_OK, "Failed to set current type, hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);
    IMFMediaTypeHandler_Release(type_handler);
    IMFStreamSink_Release(stream_sink);

    rate = 1.0f;
    hr = IMFRateSupport_GetSlowestRate(rs, MFRATE_FORWARD, TRUE, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

    rate = 1.0f;
    hr = IMFRateSupport_GetSlowestRate(rs, MFRATE_REVERSE, TRUE, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

    rate = 0.0f;
    hr = IMFRateSupport_GetFastestRate(rs, MFRATE_FORWARD, TRUE, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == FLT_MAX, "Unexpected rate %f.\n", rate);

    rate = 0.0f;
    hr = IMFRateSupport_GetFastestRate(rs, MFRATE_REVERSE, TRUE, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == -FLT_MAX, "Unexpected rate %f.\n", rate);

    hr = IMFRateSupport_GetFastestRate(rs, MFRATE_REVERSE, TRUE, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFRateSupport_GetSlowestRate(rs, MFRATE_REVERSE, TRUE, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(supported_rates); ++i)
    {
        rate = supported_rates[i] + 1.0f;
        hr = IMFRateSupport_IsRateSupported(rs, TRUE, supported_rates[i], &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(rate == supported_rates[i], "Unexpected rate %f.\n", rate);

        rate = supported_rates[i] + 1.0f;
        hr = IMFRateSupport_IsRateSupported(rs, FALSE, supported_rates[i], &rate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(rate == supported_rates[i], "Unexpected rate %f.\n", rate);

        hr = IMFRateSupport_IsRateSupported(rs, TRUE, supported_rates[i], NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFRateSupport_IsRateSupported(rs, FALSE, supported_rates[i], NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }

    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetStreamSinkCount(sink, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetStreamSinkCount(sink, &count);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFRateSupport_GetSlowestRate(rs, MFRATE_FORWARD, FALSE, &rate);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFRateSupport_GetFastestRate(rs, MFRATE_FORWARD, FALSE, &rate);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFRateSupport_GetSlowestRate(rs, MFRATE_FORWARD, FALSE, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFRateSupport_GetFastestRate(rs, MFRATE_FORWARD, FALSE, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFRateSupport_IsRateSupported(rs, TRUE, 1.0f, &rate);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    ref = IMFRateSupport_Release(rs);
    ok(ref == 1, "Release returned %ld\n", ref);
    ref = IMFMediaSink_Release(sink);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFPresentationClock_Release(clock);
    ok(ref == 0, "Release returned %ld\n", ref);

    DestroyWindow(window);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#lx.\n", hr);
}

static void test_MFCreateSimpleTypeHandler(void)
{
    IMFMediaType *media_type, *media_type2, *media_type3;
    IMFMediaTypeHandler *handler;
    DWORD count;
    HRESULT hr;
    GUID guid;
    LONG ref;

    hr = MFCreateSimpleTypeHandler(&handler);
    ok(hr == S_OK, "Failed to create object, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeCount(handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, NULL, NULL);
    ok(hr == MF_E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    count = 0;
    hr = IMFMediaTypeHandler_GetMediaTypeCount(handler, &count);
    ok(hr == S_OK, "Failed to get type count, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %lu.\n", count);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    media_type = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!media_type, "Unexpected pointer.\n");

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, NULL);
    ok(hr == MF_E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type);
    ok(hr == S_OK, "Failed to set current type, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, 0, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type2 == media_type, "Unexpected type.\n");
    IMFMediaType_Release(media_type2);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, &media_type2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, 1, &media_type2);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == media_type2, "Unexpected pointer.\n");
    IMFMediaType_Release(media_type2);

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == S_OK, "Failed to get major type, hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected major type.\n");

    hr = MFCreateMediaType(&media_type3);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type3, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    /* Different major types. */
    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type3, &media_type2);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type3, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    /* Handler missing subtype. */
    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_SUBTYPE, &MFVideoFormat_RGB8);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type3, &media_type2);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    /* Different subtypes. */
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB24);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type3, &media_type2);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    /* Same major/subtype. */
    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_SUBTYPE, &MFVideoFormat_RGB24);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type3, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    /* Set one more attribute. */
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)4 << 32 | 4);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type3, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    ref = IMFMediaType_Release(media_type3);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, NULL);
    ok(hr == S_OK, "Failed to set current type, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    ref = IMFMediaTypeHandler_Release(handler);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(media_type);
    ok(ref == 0, "Release returned %ld\n", ref);
}

static void test_MFGetSupportedMimeTypes(void)
{
    PROPVARIANT value;
    HRESULT hr;

    hr = MFGetSupportedMimeTypes(NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    value.vt = VT_EMPTY;
    hr = MFGetSupportedMimeTypes(&value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == (VT_VECTOR | VT_LPWSTR), "Unexpected value type %#x.\n", value.vt);

    PropVariantClear(&value);
}

static void test_MFGetSupportedSchemes(void)
{
    PROPVARIANT value;
    HRESULT hr;

    hr = MFGetSupportedSchemes(NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    value.vt = VT_EMPTY;
    hr = MFGetSupportedSchemes(&value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == (VT_VECTOR | VT_LPWSTR), "Unexpected value type %#x.\n", value.vt);

    PropVariantClear(&value);
}

static void test_MFGetTopoNodeCurrentType(void)
{
    static const struct attribute_desc media_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1920, 1080),
        {0},
    };
    IMFMediaType *media_type, *input_types[2], *output_types[2];
    IMFStreamDescriptor *input_descriptor, *output_descriptor;
    struct test_stream_sink stream_sink = test_stream_sink;
    IMFMediaTypeHandler *input_handler, *output_handler;
    IMFTransform *transform;
    IMFTopologyNode *node;
    DWORD flags;
    HRESULT hr;
    LONG ref;

    if (!pMFGetTopoNodeCurrentType)
    {
        win_skip("MFGetTopoNodeCurrentType() is unsupported.\n");
        return;
    }

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&input_types[0]);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);
    init_media_type(input_types[0], media_type_desc, -1);
    hr = MFCreateMediaType(&input_types[1]);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);
    init_media_type(input_types[1], media_type_desc, -1);
    hr = MFCreateMediaType(&output_types[0]);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);
    init_media_type(output_types[0], media_type_desc, -1);
    hr = MFCreateMediaType(&output_types[1]);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);
    init_media_type(output_types[1], media_type_desc, -1);

    hr = MFCreateStreamDescriptor(0, 2, input_types, &input_descriptor);
    ok(hr == S_OK, "Failed to create IMFStreamDescriptor hr %#lx.\n", hr);
    hr = IMFStreamDescriptor_GetMediaTypeHandler(input_descriptor, &input_handler);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateStreamDescriptor(0, 2, output_types, &output_descriptor);
    ok(hr == S_OK, "Failed to create IMFStreamDescriptor hr %#lx.\n", hr);
    hr = IMFStreamDescriptor_GetMediaTypeHandler(output_descriptor, &output_handler);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_CColorConvertDMO, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);


    /* Tee node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Set second output. */
    hr = IMFTopologyNode_SetOutputPrefType(node, 1, output_types[1]);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 1, TRUE, &media_type);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 1, FALSE, &media_type);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    /* Set first output. */
    hr = IMFTopologyNode_SetOutputPrefType(node, 0, output_types[0]);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == output_types[0], "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);
    hr = pMFGetTopoNodeCurrentType(node, 1, TRUE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == output_types[0], "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);
    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == output_types[0], "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);

    /* Set primary output. */
    hr = IMFTopologyNode_SetOutputPrefType(node, 1, output_types[1]);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);
    hr = IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_PRIMARYOUTPUT, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == output_types[1], "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);
    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == output_types[1], "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);
    hr = pMFGetTopoNodeCurrentType(node, 1, FALSE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == output_types[1], "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);

    /* Input type returned, if set. */
    hr = IMFTopologyNode_SetInputPrefType(node, 0, input_types[0]);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == input_types[0], "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);
    hr = pMFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == input_types[0], "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);

    hr = IMFTopologyNode_SetInputPrefType(node, 0, NULL);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);
    hr = IMFTopologyNode_SetOutputPrefType(node, 0, NULL);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);
    hr = IMFTopologyNode_SetOutputPrefType(node, 1, NULL);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);
    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);


    /* Source node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 1, TRUE, &media_type);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetUnknown(node, &MF_TOPONODE_STREAM_DESCRIPTOR, (IUnknown *)input_descriptor);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(input_handler, output_types[0]);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == output_types[0], "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);


    /* Output node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 1, FALSE, &media_type);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    stream_sink.handler = output_handler;
    hr = IMFTopologyNode_SetObject(node, (IUnknown *)&stream_sink.IMFStreamSink_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(output_handler, input_types[0]);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == input_types[0], "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);


    /* Transform node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 1, TRUE, &media_type);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 1, FALSE, &media_type);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetObject(node, (IUnknown *)transform);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 1, TRUE, &media_type);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 1, FALSE, &media_type);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, input_types[0], 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_IsEqual(media_type, input_types[0], &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    hr = IMFTransform_SetOutputType(transform, 0, output_types[0], 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = pMFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_IsEqual(media_type, output_types[0], &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);


    ref = IMFTransform_Release(transform);
    ok(ref == 0, "Release returned %ld\n", ref);

    IMFMediaTypeHandler_Release(input_handler);
    IMFMediaTypeHandler_Release(output_handler);
    ref = IMFStreamDescriptor_Release(input_descriptor);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFStreamDescriptor_Release(output_descriptor);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFMediaType_Release(input_types[0]);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(input_types[1]);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(output_types[0]);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(output_types[1]);
    ok(ref == 0, "Release returned %ld\n", ref);

    CoUninitialize();
}

void init_functions(void)
{
    HMODULE mod = GetModuleHandleA("mf.dll");
    IMFTransform *transform;
    HRESULT hr;

#define X(f) p##f = (void*)GetProcAddress(mod, #f)
    X(MFCreateSampleCopierMFT);
    X(MFGetTopoNodeCurrentType);
#undef X

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_VideoProcessorMFT, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTransform, (void **)&transform);
    if (hr == S_OK)
    {
        has_video_processor = TRUE;
        IMFTransform_Release(transform);
    }

    CoUninitialize();
}

static void test_MFRequireProtectedEnvironment(void)
{
    IMFPresentationDescriptor *pd;
    IMFMediaType *mediatype;
    IMFStreamDescriptor *sd;
    HRESULT hr;
    LONG ref;

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateStreamDescriptor(0, 1, &mediatype, &sd);
    ok(hr == S_OK, "Failed to create stream descriptor, hr %#lx.\n", hr);

    hr = MFCreatePresentationDescriptor(1, &sd, &pd);
    ok(hr == S_OK, "Failed to create presentation descriptor, hr %#lx.\n", hr);

    hr = IMFPresentationDescriptor_SelectStream(pd, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFRequireProtectedEnvironment(pd);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamDescriptor_SetUINT32(sd, &MF_SD_PROTECTED, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFRequireProtectedEnvironment(pd);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationDescriptor_DeselectStream(pd, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFRequireProtectedEnvironment(pd);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ref = IMFPresentationDescriptor_Release(pd);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFStreamDescriptor_Release(sd);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(mediatype);
    ok(ref == 0, "Release returned %ld\n", ref);
}

START_TEST(mf)
{
    init_functions();

    if (is_vista())
    {
        win_skip("Skipping tests on Vista.\n");
        return;
    }

    test_topology();
    test_topology_tee_node();
    test_topology_loader();
    test_topology_loader_evr();
    test_MFGetService();
    test_sequencer_source();
    test_media_session();
    test_media_session_events();
    test_media_session_rate_control();
    test_MFShutdownObject();
    test_presentation_clock();
    test_sample_grabber();
    test_sample_grabber_is_mediatype_supported();
    test_sample_grabber_orientation(MFVideoFormat_RGB32);
    test_sample_grabber_orientation(MFVideoFormat_NV12);
    test_quality_manager();
    test_sar();
    test_evr();
    test_MFCreateSimpleTypeHandler();
    test_MFGetSupportedMimeTypes();
    test_MFGetSupportedSchemes();
    test_MFGetTopoNodeCurrentType();
    test_MFRequireProtectedEnvironment();
}
