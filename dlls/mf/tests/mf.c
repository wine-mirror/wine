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
#include "d3d9types.h"

#include "initguid.h"
#include "control.h"
#include "dmo.h"
#include "mediaobj.h"
#include "ole2.h"
#include "wmcodecdsp.h"
#include "propsys.h"
#include "propvarutil.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(MFVideoFormat_P208, 0x38303250, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(MFVideoFormat_ABGR32, 0x00000020, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(CLSID_WINEAudioConverter, 0x6a170414, 0xaad9, 0x4693, 0xb8, 0x06, 0x3a, 0x0c, 0x47, 0xc5, 0x70, 0xd6);

DEFINE_GUID(DMOVideoFormat_RGB32, D3DFMT_X8R8G8B8, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);
DEFINE_GUID(DMOVideoFormat_RGB24, D3DFMT_R8G8B8, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);
DEFINE_GUID(DMOVideoFormat_RGB565, D3DFMT_R5G6B5, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);
DEFINE_GUID(DMOVideoFormat_RGB555, D3DFMT_X1R5G5B5, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);
DEFINE_GUID(DMOVideoFormat_RGB8, D3DFMT_P8, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

#undef INITGUID
#include <guiddef.h>
#include "mfapi.h"
#include "mferror.h"
#include "mfidl.h"
#include "initguid.h"
#include "uuids.h"
#include "mmdeviceapi.h"
#include "audioclient.h"
#include "evr.h"
#include "d3d9.h"
#include "evr9.h"

#include "wine/test.h"

static HRESULT (WINAPI *pMFCreateSampleCopierMFT)(IMFTransform **copier);
static HRESULT (WINAPI *pMFGetTopoNodeCurrentType)(IMFTopologyNode *node, DWORD stream, BOOL output, IMFMediaType **type);

static BOOL has_video_processor;
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

static void check_dmo(const GUID *class_id, const WCHAR *expect_name, const GUID *expect_major_type,
        const GUID *expect_input, ULONG expect_input_count, const GUID *expect_output, ULONG expect_output_count)
{
    ULONG i, input_count = 0, output_count = 0;
    DMO_PARTIAL_MEDIATYPE output[32] = {{{0}}};
    DMO_PARTIAL_MEDIATYPE input[32] = {{{0}}};
    WCHAR name[80];
    HRESULT hr;

    winetest_push_context("%s", debugstr_w(expect_name));

    hr = DMOGetName(class_id, name);
    ok(hr == S_OK, "DMOGetName returned %#lx\n", hr);
    todo_wine_if(!wcscmp(expect_name, L"WMAudio Decoder DMO"))
    ok(!wcscmp(name, expect_name), "got name %s\n", debugstr_w(name));

    hr = DMOGetTypes(class_id, ARRAY_SIZE(input), &input_count, input,
            ARRAY_SIZE(output), &output_count, output);
    ok(hr == S_OK, "DMOGetTypes returned %#lx\n", hr);
    ok(input_count == expect_input_count, "got input_count %lu\n", input_count);
    ok(output_count == expect_output_count, "got output_count %lu\n", output_count);

    for (i = 0; i < input_count; ++i)
    {
        winetest_push_context("in %lu", i);
        ok(IsEqualGUID(&input[i].type, expect_major_type),
            "got type %s\n", debugstr_guid(&input[i].type));
        ok(IsEqualGUID(&input[i].subtype, expect_input + i),
            "got subtype %s\n", debugstr_guid(&input[i].subtype));
        winetest_pop_context();
    }

    for (i = 0; i < output_count; ++i)
    {
        winetest_push_context("out %lu", i);
        ok(IsEqualGUID(&output[i].type, expect_major_type),
            "got type %s\n", debugstr_guid(&output[i].type));
        ok(IsEqualGUID(&output[i].subtype, expect_output + i),
            "got subtype %s\n", debugstr_guid( &output[i].subtype));
        winetest_pop_context();
    }

    winetest_pop_context();
}

struct attribute_desc
{
    const GUID *key;
    const char *name;
    PROPVARIANT value;
    BOOL ratio;
    BOOL todo;
    BOOL todo_value;
};
typedef struct attribute_desc media_type_desc[32];

#define ATTR_GUID(k, g, ...)      {.key = &k, .name = #k, {.vt = VT_CLSID, .puuid = (GUID *)&g}, __VA_ARGS__ }
#define ATTR_UINT32(k, v, ...)    {.key = &k, .name = #k, {.vt = VT_UI4, .ulVal = v}, __VA_ARGS__ }
#define ATTR_BLOB(k, p, n, ...)   {.key = &k, .name = #k, {.vt = VT_VECTOR | VT_UI1, .caub = {.pElems = (void *)p, .cElems = n}}, __VA_ARGS__ }
#define ATTR_RATIO(k, n, d, ...)  {.key = &k, .name = #k, {.vt = VT_UI8, .uhVal = {.HighPart = n, .LowPart = d}}, .ratio = TRUE, __VA_ARGS__ }
#define ATTR_UINT64(k, v, ...)    {.key = &k, .name = #k, {.vt = VT_UI8, .uhVal = {.QuadPart = v}}, __VA_ARGS__ }

#define check_media_type(a, b, c) check_attributes_(__LINE__, (IMFAttributes *)a, b, c)
#define check_attributes(a, b, c) check_attributes_(__LINE__, a, b, c)
static void check_attributes_(int line, IMFAttributes *attributes, const struct attribute_desc *desc, ULONG limit)
{
    char buffer[256], *buf = buffer;
    PROPVARIANT value;
    int i, j, ret;
    HRESULT hr;

    for (i = 0; i < limit && desc[i].key; ++i)
    {
        hr = IMFAttributes_GetItem(attributes, desc[i].key, &value);
        todo_wine_if(desc[i].todo)
        ok_(__FILE__, line)(hr == S_OK, "%s missing, hr %#lx\n", debugstr_a(desc[i].name), hr);
        if (hr != S_OK) continue;

        switch (value.vt)
        {
        default: sprintf(buffer, "??"); break;
        case VT_CLSID: sprintf(buffer, "%s", debugstr_guid(value.puuid)); break;
        case VT_UI4: sprintf(buffer, "%lu", value.ulVal); break;
        case VT_UI8:
            if (desc[i].ratio)
                sprintf(buffer, "%lu:%lu", value.uhVal.HighPart, value.uhVal.LowPart);
            else
                sprintf(buffer, "%I64u", value.uhVal.QuadPart);
            break;
        case VT_VECTOR | VT_UI1:
            buf += sprintf(buf, "size %lu, data {", value.caub.cElems);
            for (j = 0; j < 16 && j < value.caub.cElems; ++j)
                buf += sprintf(buf, "0x%02x,", value.caub.pElems[j]);
            if (value.caub.cElems > 16)
                buf += sprintf(buf, "...}");
            else
                buf += sprintf(buf - (j ? 1 : 0), "}");
            break;
        }

        ret = PropVariantCompareEx(&value, &desc[i].value, 0, 0);
        todo_wine_if(desc[i].todo_value)
        ok_(__FILE__, line)(ret == 0, "%s mismatch, type %u, value %s\n",
                debugstr_a(desc[i].name), value.vt, buffer);
    }
}

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "mf_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, r.right - r.left, r.bottom - r.top, NULL, NULL, NULL, NULL);
}

static BOOL create_transform(GUID category, MFT_REGISTER_TYPE_INFO *input_type,
        MFT_REGISTER_TYPE_INFO *output_type, const WCHAR *expect_name, const GUID *expect_major_type,
        const GUID *expect_input, ULONG expect_input_count, const GUID *expect_output, ULONG expect_output_count,
        IMFTransform **transform, const GUID *expect_class_id, GUID *class_id)
{
    MFT_REGISTER_TYPE_INFO *input_types = NULL, *output_types = NULL;
    UINT32 input_count = 0, output_count = 0, count = 0, i;
    GUID *class_ids = NULL;
    WCHAR *name;
    HRESULT hr;

    hr = MFTEnum(category, 0, input_type, output_type, NULL, &class_ids, &count);
    if (FAILED(hr) || count == 0)
    {
        todo_wine
        win_skip("Failed to enumerate %s, skipping tests.\n", debugstr_w(expect_name));
        return FALSE;
    }

    ok(hr == S_OK, "MFTEnum returned %lx\n", hr);
    for (i = 0; i < count; ++i)
    {
        if (IsEqualGUID(expect_class_id, class_ids + i))
            break;
    }
    ok(i < count, "failed to find %s transform\n", debugstr_w(expect_name));
    *class_id = class_ids[i];
    CoTaskMemFree(class_ids);
    ok(IsEqualGUID(class_id, expect_class_id), "got class id %s\n", debugstr_guid(class_id));

    hr = MFTGetInfo(*class_id, &name, &input_types, &input_count, &output_types, &output_count, NULL);
    if (FAILED(hr))
    {
        todo_wine
        win_skip("Failed to get %s info, skipping tests.\n", debugstr_w(expect_name));
    }
    else
    {
        ok(hr == S_OK, "MFTEnum returned %lx\n", hr);
        ok(!wcscmp(name, expect_name), "got name %s\n", debugstr_w(name));
        ok(input_count == expect_input_count, "got input_count %u\n", input_count);
        for (i = 0; i < input_count; ++i)
        {
            ok(IsEqualGUID(&input_types[i].guidMajorType, expect_major_type),
                    "got input[%u] major %s\n", i, debugstr_guid(&input_types[i].guidMajorType));
            ok(IsEqualGUID(&input_types[i].guidSubtype, expect_input + i),
                    "got input[%u] subtype %s\n", i, debugstr_guid(&input_types[i].guidSubtype));
        }
        ok(output_count == expect_output_count, "got output_count %u\n", output_count);
        for (i = 0; i < output_count; ++i)
        {
            ok(IsEqualGUID(&output_types[i].guidMajorType, expect_major_type),
                    "got output[%u] major %s\n", i, debugstr_guid(&output_types[i].guidMajorType));
            ok(IsEqualGUID(&output_types[i].guidSubtype, expect_output + i),
                    "got output[%u] subtype %s\n", i, debugstr_guid(&output_types[i].guidSubtype));
        }
        CoTaskMemFree(output_types);
        CoTaskMemFree(input_types);
        CoTaskMemFree(name);
    }

    hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTransform, (void **)transform);
    if (FAILED(hr))
    {
        todo_wine
        win_skip("Failed to create %s instance, skipping tests.\n", debugstr_w(expect_name));
        return FALSE;
    }

    return TRUE;
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

struct test_callback
{
    IMFAsyncCallback IMFAsyncCallback_iface;
};

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
    return 2;
}

static ULONG WINAPI testcallback_Release(IMFAsyncCallback *iface)
{
    return 1;
}

static HRESULT WINAPI testcallback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    ok(flags != NULL && queue != NULL, "Unexpected arguments.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testcallback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    ok(result != NULL, "Unexpected result object.\n");

    return E_NOTIMPL;
}

static const IMFAsyncCallbackVtbl testcallbackvtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    testcallback_Invoke,
};

static void init_test_callback(struct test_callback *callback)
{
    callback->IMFAsyncCallback_iface.lpVtbl = &testcallbackvtbl;
}

static void test_session_events(IMFMediaSession *session)
{
    struct test_callback callback, callback2;
    IMFAsyncResult *result;
    IMFMediaEvent *event;
    HRESULT hr;

    init_test_callback(&callback);
    init_test_callback(&callback2);

    hr = IMFMediaSession_GetEvent(session, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_NO_EVENTS_AVAILABLE, "Unexpected hr %#lx.\n", hr);

    /* Async case. */
    hr = IMFMediaSession_BeginGetEvent(session, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSession_BeginGetEvent(session, &callback.IMFAsyncCallback_iface, (IUnknown *)session);
    ok(hr == S_OK, "Failed to Begin*, hr %#lx.\n", hr);

    /* Same callback, same state. */
    hr = IMFMediaSession_BeginGetEvent(session, &callback.IMFAsyncCallback_iface, (IUnknown *)session);
    ok(hr == MF_S_MULTIPLE_BEGIN, "Unexpected hr %#lx.\n", hr);

    /* Same callback, different state. */
    hr = IMFMediaSession_BeginGetEvent(session, &callback.IMFAsyncCallback_iface, (IUnknown *)&callback.IMFAsyncCallback_iface);
    ok(hr == MF_E_MULTIPLE_BEGIN, "Unexpected hr %#lx.\n", hr);

    /* Different callback, same state. */
    hr = IMFMediaSession_BeginGetEvent(session, &callback2.IMFAsyncCallback_iface, (IUnknown *)session);
    ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Unexpected hr %#lx.\n", hr);

    /* Different callback, different state. */
    hr = IMFMediaSession_BeginGetEvent(session, &callback2.IMFAsyncCallback_iface, (IUnknown *)&callback.IMFAsyncCallback_iface);
    ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateAsyncResult(NULL, &callback.IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);

    hr = IMFMediaSession_EndGetEvent(session, result, &event);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    /* Shutdown behavior. */
    hr = IMFMediaSession_Shutdown(session);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
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
    DWORD caps;
    HRESULT hr;

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

    /* Basic events behavior. */
    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Failed to create media session, hr %#lx.\n", hr);

    test_session_events(session);

    IMFMediaSession_Release(session);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#lx.\n", hr);
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

static HRESULT WINAPI test_grabber_callback_QueryInterface(IMFSampleGrabberSinkCallback *iface, REFIID riid,
        void **obj)
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
    return 2;
}

static ULONG WINAPI test_grabber_callback_Release(IMFSampleGrabberSinkCallback *iface)
{
    return 1;
}

static HRESULT WINAPI test_grabber_callback_OnClockStart(IMFSampleGrabberSinkCallback *iface, MFTIME systime,
        LONGLONG offset)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_grabber_callback_OnClockStop(IMFSampleGrabberSinkCallback *iface, MFTIME systime)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_grabber_callback_OnClockPause(IMFSampleGrabberSinkCallback *iface, MFTIME systime)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_grabber_callback_OnClockRestart(IMFSampleGrabberSinkCallback *iface, MFTIME systime)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_grabber_callback_OnClockSetRate(IMFSampleGrabberSinkCallback *iface, MFTIME systime, float rate)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_grabber_callback_OnSetPresentationClock(IMFSampleGrabberSinkCallback *iface,
        IMFPresentationClock *clock)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_grabber_callback_OnProcessSample(IMFSampleGrabberSinkCallback *iface, REFGUID major_type,
        DWORD sample_flags, LONGLONG sample_time, LONGLONG sample_duration, const BYTE *buffer, DWORD sample_size)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_grabber_callback_OnShutdown(IMFSampleGrabberSinkCallback *iface)
{
    return E_NOTIMPL;
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

struct test_source
{
    IMFMediaSource IMFMediaSource_iface;
    LONG refcount;
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
        free(source);

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
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_source_CreatePresentationDescriptor(IMFMediaSource *iface, IMFPresentationDescriptor **pd)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
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

static IMFMediaSource *create_test_source(void)
{
    struct test_source *source;

    source = calloc(1, sizeof(*source));
    source->IMFMediaSource_iface.lpVtbl = &test_source_vtbl;
    source->refcount = 1;

    return &source->IMFMediaSource_iface;
}

static void init_media_type(IMFMediaType *mediatype, const struct attribute_desc *desc, ULONG limit)
{
    HRESULT hr;
    ULONG i;

    hr = IMFMediaType_DeleteAllItems(mediatype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < limit && desc[i].key; ++i)
    {
        hr = IMFMediaType_SetItem(mediatype, desc[i].key, &desc[i].value);
        ok(hr == S_OK, "SetItem %s returned %#lx\n", debugstr_a(desc[i].name), hr);
    }
}

static void init_source_node(IMFMediaType *mediatype, IMFMediaSource *source, IMFTopologyNode *node)
{
    IMFPresentationDescriptor *pd;
    IMFMediaTypeHandler *handler;
    IMFStreamDescriptor *sd;
    HRESULT hr;

    hr = IMFTopologyNode_DeleteAllItems(node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateStreamDescriptor(0, 1, &mediatype, &sd);
    ok(hr == S_OK, "Failed to create stream descriptor, hr %#lx.\n", hr);

    hr = IMFStreamDescriptor_GetMediaTypeHandler(sd, &handler);
    ok(hr == S_OK, "Failed to get media type handler, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, mediatype);
    ok(hr == S_OK, "Failed to set current media type, hr %#lx.\n", hr);

    IMFMediaTypeHandler_Release(handler);

    hr = MFCreatePresentationDescriptor(1, &sd, &pd);
    ok(hr == S_OK, "Failed to create presentation descriptor, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetUnknown(node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, (IUnknown *)pd);
    ok(hr == S_OK, "Failed to set node pd, hr %#lx.\n", hr);

    IMFPresentationDescriptor_Release(pd);

    hr = IMFTopologyNode_SetUnknown(node, &MF_TOPONODE_STREAM_DESCRIPTOR, (IUnknown *)sd);
    ok(hr == S_OK, "Failed to set node sd, hr %#lx.\n", hr);

    if (source)
    {
        hr = IMFTopologyNode_SetUnknown(node, &MF_TOPONODE_SOURCE, (IUnknown *)source);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }

    IMFStreamDescriptor_Release(sd);
}

static void init_sink_node(IMFActivate *sink_activate, unsigned int method, IMFTopologyNode *node)
{
    IMFStreamSink *stream_sink;
    IMFMediaSink *sink;
    HRESULT hr;

    hr = IMFTopologyNode_DeleteAllItems(node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFActivate_ActivateObject(sink_activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Failed to activate, hr %#lx.\n", hr);

    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 0, &stream_sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFMediaSink_Release(sink);

    hr = IMFTopologyNode_SetObject(node, (IUnknown *)stream_sink);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);

    IMFStreamSink_Release(stream_sink);

    hr = IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_CONNECT_METHOD, method);
    ok(hr == S_OK, "Failed to set connect method, hr %#lx.\n", hr);
}

enum loader_test_flags
{
    LOADER_EXPECTED_DECODER = 0x1,
    LOADER_EXPECTED_CONVERTER = 0x2,
    LOADER_TODO = 0x4,
    LOADER_NEEDS_VIDEO_PROCESSOR = 0x8,
};

static void test_topology_loader(void)
{
    static const struct loader_test
    {
        media_type_desc input_type;
        media_type_desc output_type;
        MF_CONNECT_METHOD method;
        HRESULT expected_result;
        unsigned int flags;
    }
    loader_tests[] =
    {
        {
            /* PCM -> PCM, same type */
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
                ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
                ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
                ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
                ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
            },
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
                ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
                ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
                ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
                ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
            },

            MF_CONNECT_DIRECT,
            S_OK,
        },

        {
            /* PCM -> PCM, different bps. */
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
                ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
                ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
                ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
                ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
            },
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
                ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
                ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
                ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000),
                ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 48000),
                ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
                ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
            },

            MF_CONNECT_DIRECT,
            MF_E_INVALIDMEDIATYPE,
        },

        {
            /* PCM -> PCM, different bps. */
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
                ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
                ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
                ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
                ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
            },
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
                ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
                ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
                ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000),
                ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 48000),
                ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
                ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
            },

            MF_CONNECT_ALLOW_CONVERTER,
            S_OK,
            LOADER_NEEDS_VIDEO_PROCESSOR | LOADER_EXPECTED_CONVERTER | LOADER_TODO,
        },

        {
            /* MP3 -> PCM */
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
                ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_MP3),
                ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
                ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 16000),
                ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
            },
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
                ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
                ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
                ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
                ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
            },

            MF_CONNECT_DIRECT,
            MF_E_INVALIDMEDIATYPE,
        },

        {
            /* MP3 -> PCM */
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
                ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_MP3),
                ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
                ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 16000),
                ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
            },
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
                ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
                ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
                ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
                ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
            },

            MF_CONNECT_ALLOW_CONVERTER,
            MF_E_TRANSFORM_NOT_POSSIBLE_FOR_CURRENT_MEDIATYPE_COMBINATION,
            LOADER_NEEDS_VIDEO_PROCESSOR | LOADER_TODO,
        },

        {
            /* MP3 -> PCM */
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
                ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_MP3),
                ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
                ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 16000),
                ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
            },
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
                ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
                ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
                ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
                ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
            },

            MF_CONNECT_ALLOW_DECODER,
            S_OK,
            LOADER_EXPECTED_DECODER | LOADER_TODO,
        },
    };

    IMFSampleGrabberSinkCallback test_grabber_callback = { &test_grabber_callback_vtbl };
    IMFTopologyNode *src_node, *sink_node, *src_node2, *sink_node2, *mft_node;
    IMFTopology *topology, *topology2, *full_topology;
    IMFMediaType *media_type, *input_type, *output_type;
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

    /* When a decoder is involved, windows requires this attribute to be present */
    source = create_test_source();

    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_SOURCE, (IUnknown *)source);
    ok(hr == S_OK, "Failed to set node source, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = MFCreateStreamDescriptor(0, 1, &media_type, &sd);
    ok(hr == S_OK, "Failed to create stream descriptor, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_STREAM_DESCRIPTOR, (IUnknown *)sd);
    ok(hr == S_OK, "Failed to set node sd, hr %#lx.\n", hr);

    hr = MFCreatePresentationDescriptor(1, &sd, &pd);
    ok(hr == S_OK, "Failed to create presentation descriptor, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, (IUnknown *)pd);
    ok(hr == S_OK, "Failed to set node pd, hr %#lx.\n", hr);

    IMFMediaType_Release(media_type);

    hr = IMFTopology_AddNode(topology, src_node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    /* Source node only. */
    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    todo_wine_if(hr == E_INVALIDARG)
    ok(hr == MF_E_TOPO_UNSUPPORTED, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &sink_node);
    ok(hr == S_OK, "Failed to create output node, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = MFCreateSampleGrabberSinkActivate(media_type, &test_grabber_callback, &sink_activate);
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

    hr = MFCreateMediaType(&input_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&output_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(loader_tests); ++i)
    {
        const struct loader_test *test = &loader_tests[i];

        init_media_type(input_type, test->input_type, -1);
        init_media_type(output_type, test->output_type, -1);

        hr = MFCreateSampleGrabberSinkActivate(output_type, &test_grabber_callback, &sink_activate);
        ok(hr == S_OK, "Failed to create grabber sink, hr %#lx.\n", hr);

        init_source_node(input_type, source, src_node);
        init_sink_node(sink_activate, test->method, sink_node);

        hr = IMFTopology_GetCount(topology, &count);
        ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
        ok(!count, "Unexpected count %u.\n", count);

        hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
        if (test->flags & LOADER_NEEDS_VIDEO_PROCESSOR && !has_video_processor)
            ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx on test %u.\n", hr, i);
        else
        {
            todo_wine_if(test->flags & LOADER_TODO)
            ok(hr == test->expected_result, "Unexpected hr %#lx on test %u.\n", hr, i);
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
            ok(count == 1, "Unexpected count %u.\n", count);

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
            todo_wine_if(test->flags & (LOADER_EXPECTED_CONVERTER | LOADER_EXPECTED_DECODER))
            ok(node_count == count, "Unexpected node count %u.\n", node_count);

            hr = IMFTopologyNode_GetTopoNodeID(src_node, &node_id);
            ok(hr == S_OK, "Failed to get source node id, hr %#lx.\n", hr);

            hr = IMFTopology_GetNodeByID(full_topology, node_id, &src_node2);
            ok(hr == S_OK, "Failed to get source in resolved topology, hr %#lx.\n", hr);

            hr = IMFTopologyNode_GetTopoNodeID(sink_node, &node_id);
            ok(hr == S_OK, "Failed to get sink node id, hr %#lx.\n", hr);

            hr = IMFTopology_GetNodeByID(full_topology, node_id, &sink_node2);
            ok(hr == S_OK, "Failed to get sink in resolved topology, hr %#lx.\n", hr);

            if (test->flags & (LOADER_EXPECTED_DECODER | LOADER_EXPECTED_CONVERTER) && strcmp(winetest_platform, "wine"))
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

                IMFTopologyNode_Release(mft_node);
                IMFMediaType_Release(media_type);
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

        hr = IMFActivate_ShutdownObject(sink_activate);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ref = IMFActivate_Release(sink_activate);
        ok(ref == 0, "Release returned %ld\n", ref);
    }

    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopoLoader_Release(loader);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(src_node);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(sink_node);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFMediaSource_Release(source);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFPresentationDescriptor_Release(pd);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFStreamDescriptor_Release(sd);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFMediaType_Release(input_type);
    ok(ref == 0, "Release returned %ld\n", ref);
    /* FIXME: is native really leaking refs here, or are we? */
    ref = IMFMediaType_Release(output_type);
    todo_wine
    ok(ref != 0, "Release returned %ld\n", ref);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#lx.\n", hr);
}

static void test_topology_loader_evr(void)
{
    IMFTopologyNode *node, *source_node, *evr_node;
    IMFTopology *topology, *full_topology;
    IMFMediaTypeHandler *handler;
    unsigned int i, count, value;
    IMFStreamSink *stream_sink;
    IMFMediaType *media_type;
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

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)640 << 32 | 480);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    init_source_node(media_type, NULL, source_node);

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

static HRESULT WINAPI grabber_callback_QueryInterface(IMFSampleGrabberSinkCallback *iface, REFIID riid, void **obj)
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

static ULONG WINAPI grabber_callback_AddRef(IMFSampleGrabberSinkCallback *iface)
{
    return 2;
}

static ULONG WINAPI grabber_callback_Release(IMFSampleGrabberSinkCallback *iface)
{
    return 1;
}

static HRESULT WINAPI grabber_callback_OnClockStart(IMFSampleGrabberSinkCallback *iface, MFTIME time, LONGLONG offset)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI grabber_callback_OnClockStop(IMFSampleGrabberSinkCallback *iface, MFTIME time)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI grabber_callback_OnClockPause(IMFSampleGrabberSinkCallback *iface, MFTIME time)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI grabber_callback_OnClockRestart(IMFSampleGrabberSinkCallback *iface, MFTIME time)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI grabber_callback_OnClockSetRate(IMFSampleGrabberSinkCallback *iface, MFTIME time, float rate)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI grabber_callback_OnSetPresentationClock(IMFSampleGrabberSinkCallback *iface,
        IMFPresentationClock *clock)
{
    return S_OK;
}

static HRESULT WINAPI grabber_callback_OnProcessSample(IMFSampleGrabberSinkCallback *iface, REFGUID major_type,
        DWORD sample_flags, LONGLONG sample_time, LONGLONG sample_duration, const BYTE *buffer, DWORD sample_size)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI grabber_callback_OnShutdown(IMFSampleGrabberSinkCallback *iface)
{
    return S_OK;
}

static const IMFSampleGrabberSinkCallbackVtbl grabber_callback_vtbl =
{
    grabber_callback_QueryInterface,
    grabber_callback_AddRef,
    grabber_callback_Release,
    grabber_callback_OnClockStart,
    grabber_callback_OnClockStop,
    grabber_callback_OnClockPause,
    grabber_callback_OnClockRestart,
    grabber_callback_OnClockSetRate,
    grabber_callback_OnSetPresentationClock,
    grabber_callback_OnProcessSample,
    grabber_callback_OnShutdown,
};

static IMFSampleGrabberSinkCallback grabber_callback = { &grabber_callback_vtbl };

static void test_sample_grabber(void)
{
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

    hr = MFCreateSampleGrabberSinkActivate(NULL, &grabber_callback, &activate);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    EXPECT_REF(media_type, 1);
    hr = MFCreateSampleGrabberSinkActivate(media_type, &grabber_callback, &activate);
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

    hr = MFCreateSampleGrabberSinkActivate(media_type, &grabber_callback, &activate);
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
    hr = MFCreateSampleGrabberSinkActivate(media_type, &grabber_callback, &activate);
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
}

static void test_sample_grabber_is_mediatype_supported(void)
{
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

    hr = MFCreateSampleGrabberSinkActivate(media_type, &grabber_callback, &activate);
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
}

static BOOL is_supported_video_type(const GUID *guid)
{
    return IsEqualGUID(guid, &MFVideoFormat_L8)
            || IsEqualGUID(guid, &MFVideoFormat_L16)
            || IsEqualGUID(guid, &MFVideoFormat_D16)
            || IsEqualGUID(guid, &MFVideoFormat_IYUV)
            || IsEqualGUID(guid, &MFVideoFormat_YV12)
            || IsEqualGUID(guid, &MFVideoFormat_NV12)
            || IsEqualGUID(guid, &MFVideoFormat_NV21)
            || IsEqualGUID(guid, &MFVideoFormat_420O)
            || IsEqualGUID(guid, &MFVideoFormat_P010)
            || IsEqualGUID(guid, &MFVideoFormat_P016)
            || IsEqualGUID(guid, &MFVideoFormat_UYVY)
            || IsEqualGUID(guid, &MFVideoFormat_YUY2)
            || IsEqualGUID(guid, &MFVideoFormat_P208)
            || IsEqualGUID(guid, &MFVideoFormat_NV11)
            || IsEqualGUID(guid, &MFVideoFormat_AYUV)
            || IsEqualGUID(guid, &MFVideoFormat_ARGB32)
            || IsEqualGUID(guid, &MFVideoFormat_RGB32)
            || IsEqualGUID(guid, &MFVideoFormat_A2R10G10B10)
            || IsEqualGUID(guid, &MFVideoFormat_A16B16G16R16F)
            || IsEqualGUID(guid, &MFVideoFormat_RGB24)
            || IsEqualGUID(guid, &MFVideoFormat_I420)
            || IsEqualGUID(guid, &MFVideoFormat_YVYU)
            || IsEqualGUID(guid, &MFVideoFormat_RGB555)
            || IsEqualGUID(guid, &MFVideoFormat_RGB565)
            || IsEqualGUID(guid, &MFVideoFormat_RGB8)
            || IsEqualGUID(guid, &MFVideoFormat_Y216)
            || IsEqualGUID(guid, &MFVideoFormat_v410)
            || IsEqualGUID(guid, &MFVideoFormat_Y41P)
            || IsEqualGUID(guid, &MFVideoFormat_Y41T)
            || IsEqualGUID(guid, &MFVideoFormat_Y42T)
            || IsEqualGUID(guid, &MFVideoFormat_ABGR32);
}

static void test_video_processor(void)
{
    const GUID transform_inputs[22] =
    {
        MFVideoFormat_IYUV,
        MFVideoFormat_YV12,
        MFVideoFormat_NV12,
        MFVideoFormat_YUY2,
        MFVideoFormat_ARGB32,
        MFVideoFormat_RGB32,
        MFVideoFormat_NV11,
        MFVideoFormat_AYUV,
        MFVideoFormat_UYVY,
        MEDIASUBTYPE_P208,
        MFVideoFormat_RGB24,
        MFVideoFormat_RGB555,
        MFVideoFormat_RGB565,
        MFVideoFormat_RGB8,
        MFVideoFormat_I420,
        MFVideoFormat_Y216,
        MFVideoFormat_v410,
        MFVideoFormat_Y41P,
        MFVideoFormat_Y41T,
        MFVideoFormat_Y42T,
        MFVideoFormat_YVYU,
        MFVideoFormat_420O,
    };
    const GUID transform_outputs[21] =
    {
        MFVideoFormat_IYUV,
        MFVideoFormat_YV12,
        MFVideoFormat_NV12,
        MFVideoFormat_YUY2,
        MFVideoFormat_ARGB32,
        MFVideoFormat_RGB32,
        MFVideoFormat_NV11,
        MFVideoFormat_AYUV,
        MFVideoFormat_UYVY,
        MEDIASUBTYPE_P208,
        MFVideoFormat_RGB24,
        MFVideoFormat_RGB555,
        MFVideoFormat_RGB565,
        MFVideoFormat_RGB8,
        MFVideoFormat_I420,
        MFVideoFormat_Y216,
        MFVideoFormat_v410,
        MFVideoFormat_Y41P,
        MFVideoFormat_Y41T,
        MFVideoFormat_Y42T,
        MFVideoFormat_YVYU,
    };
    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Video, MFVideoFormat_NV12};
    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Video, MFVideoFormat_I420};
    DWORD input_count, output_count, input_id, output_id, flags;
    DWORD input_min, input_max, output_min, output_max, i;
    IMFAttributes *attributes, *attributes2;
    IMFMediaType *media_type, *media_type2;
    MFT_OUTPUT_DATA_BUFFER output_buffer;
    MFT_OUTPUT_STREAM_INFO output_info;
    MFT_INPUT_STREAM_INFO input_info;
    IMFSample *sample, *sample2;
    IMFTransform *transform;
    IMFMediaBuffer *buffer;
    IMFMediaEvent *event;
    unsigned int value;
    GUID class_id;
    UINT32 count;
    HRESULT hr;
    GUID guid;
    LONG ref;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    if (!create_transform(MFT_CATEGORY_VIDEO_PROCESSOR, &input_type, &output_type, L"Microsoft Video Processor MFT", &MFMediaType_Video,
            transform_inputs, ARRAY_SIZE(transform_inputs), transform_outputs, ARRAY_SIZE(transform_outputs),
            &transform, &CLSID_VideoProcessorMFT, &class_id))
        goto failed;
    has_video_processor = TRUE;

    todo_wine
    check_interface(transform, &IID_IMFVideoProcessorControl, TRUE);
    todo_wine
    check_interface(transform, &IID_IMFRealTimeClientEx, TRUE);
    check_interface(transform, &IID_IMFMediaEventGenerator, FALSE);
    check_interface(transform, &IID_IMFShutdown, FALSE);

    /* Transform global attributes. */
    hr = IMFTransform_GetAttributes(transform, &attributes);
    ok(hr == S_OK, "Failed to get attributes, hr %#lx.\n", hr);

    hr = IMFAttributes_GetCount(attributes, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!!count, "Unexpected attribute count %u.\n", count);

    value = 0;
    hr = IMFAttributes_GetUINT32(attributes, &MF_SA_D3D11_AWARE, &value);
todo_wine {
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 1, "Unexpected attribute value %u.\n", value);
}
    hr = IMFTransform_GetAttributes(transform, &attributes2);
    ok(hr == S_OK, "Failed to get attributes, hr %#lx.\n", hr);
    ok(attributes == attributes2, "Unexpected instance.\n");
    IMFAttributes_Release(attributes);
    IMFAttributes_Release(attributes2);

    hr = IMFTransform_GetStreamLimits(transform, &input_min, &input_max, &output_min, &output_max);
    ok(hr == S_OK, "Failed to get stream limits, hr %#lx.\n", hr);
    ok(input_min == input_max && input_min == 1 && output_min == output_max && output_min == 1,
            "Unexpected stream limits.\n");

    hr = IMFTransform_GetStreamCount(transform, &input_count, &output_count);
    ok(hr == S_OK, "Failed to get stream count, hr %#lx.\n", hr);
    ok(input_count == 1 && output_count == 1, "Unexpected stream count %lu, %lu.\n", input_count, output_count);

    hr = IMFTransform_GetStreamIDs(transform, 1, &input_id, 1, &output_id);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    input_id = 100;
    hr = IMFTransform_AddInputStreams(transform, 1, &input_id);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_DeleteInputStream(transform, 0);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputStatus(transform, 0, &flags);
    todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputStreamAttributes(transform, 0, &attributes);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputStatus(transform, &flags);
    todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputStreamAttributes(transform, 0, &attributes);
    ok(hr == S_OK, "Failed to get output attributes, hr %#lx.\n", hr);
    hr = IMFTransform_GetOutputStreamAttributes(transform, 0, &attributes2);
    ok(hr == S_OK, "Failed to get output attributes, hr %#lx.\n", hr);
    ok(attributes == attributes2, "Unexpected instance.\n");
    IMFAttributes_Release(attributes);
    IMFAttributes_Release(attributes2);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    todo_wine
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
    todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputCurrentType(transform, 1, &media_type);
    todo_wine
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
    todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputCurrentType(transform, 1, &media_type);
    todo_wine
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputStreamInfo(transform, 1, &input_info);
    todo_wine
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    memset(&input_info, 0xcc, sizeof(input_info));
    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
todo_wine {
    ok(hr == S_OK, "Failed to get stream info, hr %#lx.\n", hr);
    ok(input_info.dwFlags == 0, "Unexpected flag %#lx.\n", input_info.dwFlags);
    ok(input_info.cbSize == 0, "Unexpected size %lu.\n", input_info.cbSize);
    ok(input_info.cbMaxLookahead == 0, "Unexpected lookahead length %lu.\n", input_info.cbMaxLookahead);
    ok(input_info.cbAlignment == 0, "Unexpected alignment %lu.\n", input_info.cbAlignment);
}
    hr = MFCreateMediaEvent(MEUnknown, &GUID_NULL, S_OK, NULL, &event);
    ok(hr == S_OK, "Failed to create event object, hr %#lx.\n", hr);
    hr = IMFTransform_ProcessEvent(transform, 0, event);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_ProcessEvent(transform, 1, event);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ref = IMFMediaEvent_Release(event);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Configure stream types. */
    for (i = 0;;++i)
    {
        if (FAILED(hr = IMFTransform_GetInputAvailableType(transform, 0, i, &media_type)))
        {
            todo_wine
            ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);
            break;
        }

        hr = IMFTransform_GetInputAvailableType(transform, 0, i, &media_type2);
        ok(hr == S_OK, "Failed to get available type, hr %#lx.\n", hr);
        ok(media_type != media_type2, "Unexpected instance.\n");
        ref = IMFMediaType_Release(media_type2);
        ok(ref == 0, "Release returned %ld\n", ref);

        hr = IMFMediaType_GetMajorType(media_type, &guid);
        ok(hr == S_OK, "Failed to get major type, hr %#lx.\n", hr);
        ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected major type.\n");

        hr = IMFMediaType_GetCount(media_type, &count);
        ok(hr == S_OK, "Failed to get attributes count, hr %#lx.\n", hr);
        ok(count == 2, "Unexpected count %u.\n", count);

        hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
        ok(hr == S_OK, "Failed to get subtype, hr %#lx.\n", hr);
        ok(is_supported_video_type(&guid), "Unexpected media type %s.\n", wine_dbgstr_guid(&guid));

        hr = IMFTransform_SetInputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
        ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

        hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
        ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

        hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type2);
        ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

        /* FIXME: figure out if those require additional attributes or simply advertised but not supported */
        if (IsEqualGUID(&guid, &MFVideoFormat_L8) || IsEqualGUID(&guid, &MFVideoFormat_L16)
                || IsEqualGUID(&guid, &MFVideoFormat_D16) || IsEqualGUID(&guid, &MFVideoFormat_420O)
                || IsEqualGUID(&guid, &MFVideoFormat_A16B16G16R16F))
        {
            ref = IMFMediaType_Release(media_type);
            ok(ref == 0, "Release returned %ld\n", ref);
            continue;
        }

        hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, ((UINT64)16 << 32) | 16);
        ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

        hr = IMFTransform_SetInputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
        ok(hr == S_OK, "Failed to test input type %s, hr %#lx.\n", wine_dbgstr_guid(&guid), hr);

        hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
        ok(hr == S_OK, "Failed to test input type, hr %#lx.\n", hr);

        hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type2);
        ok(hr == S_OK, "Failed to get current type, hr %#lx.\n", hr);
        ok(media_type != media_type2, "Unexpected instance.\n");
        IMFMediaType_Release(media_type2);

        hr = IMFTransform_GetInputStatus(transform, 0, &flags);
        ok(hr == S_OK, "Failed to get input status, hr %#lx.\n", hr);
        ok(flags == MFT_INPUT_STATUS_ACCEPT_DATA, "Unexpected input status %#lx.\n", flags);

        hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
        ok(hr == S_OK, "Failed to get stream info, hr %#lx.\n", hr);
        ok(input_info.dwFlags == 0, "Unexpected flags %#lx.\n", input_info.dwFlags);
        ok(input_info.cbMaxLookahead == 0, "Unexpected lookahead length %lu.\n", input_info.cbMaxLookahead);
        ok(input_info.cbAlignment == 0, "Unexpected alignment %lu.\n", input_info.cbAlignment);

        IMFMediaType_Release(media_type);
    }

    /* IYUV -> RGB32 */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_IYUV);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, ((UINT64)16 << 32) | 16);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    todo_wine
    ok(hr == S_OK, "Failed to set input type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    todo_wine
    ok(hr == S_OK, "Failed to set output type, hr %#lx.\n", hr);

    memset(&output_info, 0, sizeof(output_info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    todo_wine
    ok(hr == S_OK, "Failed to get stream info, hr %#lx.\n", hr);
    ok(output_info.dwFlags == 0, "Unexpected flags %#lx.\n", output_info.dwFlags);
    todo_wine
    ok(output_info.cbSize > 0, "Unexpected size %lu.\n", output_info.cbSize);
    ok(output_info.cbAlignment == 0, "Unexpected alignment %lu.\n", output_info.cbAlignment);

    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#lx.\n", hr);

    hr = MFCreateSample(&sample2);
    ok(hr == S_OK, "Failed to create a sample, hr %#lx.\n", hr);

    memset(&output_buffer, 0, sizeof(output_buffer));
    output_buffer.pSample = sample;
    flags = 0;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output_buffer, &flags);
    todo_wine
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "Unexpected hr %#lx.\n", hr);
    ok(output_buffer.dwStatus == 0, "Unexpected buffer status, %#lx.\n", output_buffer.dwStatus);
    ok(flags == 0, "Unexpected status %#lx.\n", flags);

    hr = IMFTransform_ProcessInput(transform, 0, sample2, 0);
    todo_wine
    ok(hr == S_OK, "Failed to push a sample, hr %#lx.\n", hr);

    hr = IMFTransform_ProcessInput(transform, 0, sample2, 0);
    todo_wine
    ok(hr == MF_E_NOTACCEPTING, "Unexpected hr %#lx.\n", hr);

    memset(&output_buffer, 0, sizeof(output_buffer));
    output_buffer.pSample = sample;
    flags = 0;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output_buffer, &flags);
    todo_wine
    ok(hr == MF_E_NO_SAMPLE_TIMESTAMP, "Unexpected hr %#lx.\n", hr);
    ok(output_buffer.dwStatus == 0, "Unexpected buffer status, %#lx.\n", output_buffer.dwStatus);
    ok(flags == 0, "Unexpected status %#lx.\n", flags);

    hr = IMFSample_SetSampleTime(sample2, 0);
    ok(hr == S_OK, "Failed to set sample time, hr %#lx.\n", hr);
    memset(&output_buffer, 0, sizeof(output_buffer));
    output_buffer.pSample = sample;
    flags = 0;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output_buffer, &flags);
    todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(output_buffer.dwStatus == 0, "Unexpected buffer status, %#lx.\n", output_buffer.dwStatus);
    ok(flags == 0, "Unexpected status %#lx.\n", flags);

    hr = MFCreateMemoryBuffer(1024 * 1024, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(sample2, buffer);
    ok(hr == S_OK, "Failed to add a buffer, hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add a buffer, hr %#lx.\n", hr);

    memset(&output_buffer, 0, sizeof(output_buffer));
    output_buffer.pSample = sample;
    flags = 0;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output_buffer, &flags);
    todo_wine
    ok(hr == S_OK || broken(FAILED(hr)) /* Win8 */, "Failed to get output buffer, hr %#lx.\n", hr);
    ok(output_buffer.dwStatus == 0, "Unexpected buffer status, %#lx.\n", output_buffer.dwStatus);
    ok(flags == 0, "Unexpected status %#lx.\n", flags);

    if (SUCCEEDED(hr))
    {
        memset(&output_buffer, 0, sizeof(output_buffer));
        output_buffer.pSample = sample;
        flags = 0;
        hr = IMFTransform_ProcessOutput(transform, 0, 1, &output_buffer, &flags);
        ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "Unexpected hr %#lx.\n", hr);
        ok(output_buffer.dwStatus == 0, "Unexpected buffer status, %#lx.\n", output_buffer.dwStatus);
        ok(flags == 0, "Unexpected status %#lx.\n", flags);
    }

    ref = IMFTransform_Release(transform);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFMediaType_Release(media_type);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFSample_Release(sample2);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFSample_Release(sample);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaBuffer_Release(buffer);
    ok(ref == 0, "Release returned %ld\n", ref);

failed:
    CoUninitialize();
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
    IMFPresentationClock *present_clock, *present_clock2;
    IMFMediaType *mediatype, *mediatype2, *mediatype3;
    IMFClockStateSink *state_sink, *state_sink2;
    IMFMediaTypeHandler *handler, *handler2;
    IMFPresentationTimeSource *time_source;
    IMFSimpleAudioVolume *simple_volume;
    IMFAudioStreamVolume *stream_volume;
    IMFMediaSink *sink, *sink2;
    IMFStreamSink *stream_sink;
    IMFAttributes *attributes;
    DWORD i, id, flags, count;
    IMFActivate *activate;
    UINT32 channel_count;
    MFCLOCK_STATE state;
    IMFClock *clock;
    IUnknown *unk;
    HRESULT hr;
    GUID guid;
    BOOL mute;
    int found;
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

    /* A number of same major/subtype entries are returned, with different degrees of finer format
       details. Some incomplete types are not accepted, check that at least one of them is considered supported. */

    for (i = 0, found = -1; i < count; ++i)
    {
        hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, i, &mediatype);
        ok(hr == S_OK, "Failed to get media type, hr %#lx.\n", hr);

        if (SUCCEEDED(IMFMediaTypeHandler_IsMediaTypeSupported(handler, mediatype, NULL)))
            found = i;
        IMFMediaType_Release(mediatype);

        if (found != -1) break;
    }
    ok(found != -1, "Haven't found a supported type.\n");

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

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, found, &mediatype2);
    ok(hr == S_OK, "Failed to get media type, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, found, &mediatype3);
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

static BOOL is_sample_copier_available_type(IMFMediaType *type)
{
    GUID major = { 0 };
    UINT32 count;
    HRESULT hr;

    hr = IMFMediaType_GetMajorType(type, &major);
    ok(hr == S_OK, "Failed to get major type, hr %#lx.\n", hr);

    hr = IMFMediaType_GetCount(type, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    return IsEqualGUID(&major, &MFMediaType_Video) || IsEqualGUID(&major, &MFMediaType_Audio);
}

static void test_sample_copier(void)
{
    IMFAttributes *attributes, *attributes2;
    DWORD in_min, in_max, out_min, out_max;
    IMFMediaType *mediatype, *mediatype2;
    MFT_OUTPUT_STREAM_INFO output_info;
    IMFSample *sample, *client_sample;
    MFT_INPUT_STREAM_INFO input_info;
    DWORD input_count, output_count;
    MFT_OUTPUT_DATA_BUFFER buffer;
    IMFMediaBuffer *media_buffer;
    IMFTransform *copier;
    DWORD flags, status;
    UINT32 value, count;
    HRESULT hr;
    LONG ref;

    if (!pMFCreateSampleCopierMFT)
    {
        win_skip("MFCreateSampleCopierMFT() is not available.\n");
        return;
    }

    hr = pMFCreateSampleCopierMFT(&copier);
    ok(hr == S_OK, "Failed to create sample copier, hr %#lx.\n", hr);

    hr = IMFTransform_GetAttributes(copier, &attributes);
    ok(hr == S_OK, "Failed to get transform attributes, hr %#lx.\n", hr);
    hr = IMFTransform_GetAttributes(copier, &attributes2);
    ok(hr == S_OK, "Failed to get transform attributes, hr %#lx.\n", hr);
    ok(attributes == attributes2, "Unexpected instance.\n");
    IMFAttributes_Release(attributes2);
    hr = IMFAttributes_GetCount(attributes, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);
    hr = IMFAttributes_GetUINT32(attributes, &MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!value, "Unexpected value %u.\n", value);
    ref = IMFAttributes_Release(attributes);
    ok(ref == 1, "Release returned %ld\n", ref);

    hr = IMFTransform_GetInputStreamAttributes(copier, 0, &attributes);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputStreamAttributes(copier, 1, &attributes);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputStreamAttributes(copier, 0, &attributes);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputStreamAttributes(copier, 1, &attributes);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputBounds(copier, 0, 0);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    /* No dynamic streams. */
    input_count = output_count = 0;
    hr = IMFTransform_GetStreamCount(copier, &input_count, &output_count);
    ok(hr == S_OK, "Failed to get stream count, hr %#lx.\n", hr);
    ok(input_count == 1 && output_count == 1, "Unexpected streams count.\n");

    hr = IMFTransform_GetStreamLimits(copier, &in_min, &in_max, &out_min, &out_max);
    ok(hr == S_OK, "Failed to get stream limits, hr %#lx.\n", hr);
    ok(in_min == in_max && in_min == 1 && out_min == out_max && out_min == 1, "Unexpected stream limits.\n");

    hr = IMFTransform_GetStreamIDs(copier, 1, &input_count, 1, &output_count);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_DeleteInputStream(copier, 0);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    /* Available types. */
    hr = IMFTransform_GetInputAvailableType(copier, 0, 0, &mediatype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(is_sample_copier_available_type(mediatype), "Unexpected type.\n");
    IMFMediaType_Release(mediatype);

    hr = IMFTransform_GetInputAvailableType(copier, 0, 1, &mediatype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(is_sample_copier_available_type(mediatype), "Unexpected type.\n");
    IMFMediaType_Release(mediatype);

    hr = IMFTransform_GetInputAvailableType(copier, 0, 2, &mediatype);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputAvailableType(copier, 1, 0, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputAvailableType(copier, 0, 0, &mediatype);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputAvailableType(copier, 1, 0, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputCurrentType(copier, 0, &mediatype);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputCurrentType(copier, 1, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputCurrentType(copier, 0, &mediatype);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputCurrentType(copier, 1, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#lx.\n", hr);

    hr = IMFTransform_ProcessInput(copier, 0, sample, 0);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(copier, 0, mediatype, 0);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, &MFVideoFormat_RGB8);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT64(mediatype, &MF_MT_FRAME_SIZE, ((UINT64)16) << 32 | 16);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputStreamInfo(copier, 0, &output_info);
    ok(hr == S_OK, "Failed to get stream info, hr %#lx.\n", hr);
    ok(!output_info.dwFlags, "Unexpected flags %#lx.\n", output_info.dwFlags);
    ok(!output_info.cbSize, "Unexpected size %lu.\n", output_info.cbSize);
    ok(!output_info.cbAlignment, "Unexpected alignment %lu.\n", output_info.cbAlignment);

    hr = IMFTransform_GetInputStreamInfo(copier, 0, &input_info);
    ok(hr == S_OK, "Failed to get stream info, hr %#lx.\n", hr);

    ok(!input_info.hnsMaxLatency, "Unexpected latency %s.\n", wine_dbgstr_longlong(input_info.hnsMaxLatency));
    ok(!input_info.dwFlags, "Unexpected flags %#lx.\n", input_info.dwFlags);
    ok(!input_info.cbSize, "Unexpected size %lu.\n", input_info.cbSize);
    ok(!input_info.cbMaxLookahead, "Unexpected lookahead size %lu.\n", input_info.cbMaxLookahead);
    ok(!input_info.cbAlignment, "Unexpected alignment %lu.\n", input_info.cbAlignment);

    hr = IMFTransform_SetOutputType(copier, 0, mediatype, 0);
    ok(hr == S_OK, "Failed to set input type, hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputStreamInfo(copier, 0, &output_info);
    ok(hr == S_OK, "Failed to get stream info, hr %#lx.\n", hr);
    ok(!output_info.dwFlags, "Unexpected flags %#lx.\n", output_info.dwFlags);
    ok(output_info.cbSize == 16 * 16, "Unexpected size %lu.\n", output_info.cbSize);
    ok(!output_info.cbAlignment, "Unexpected alignment %lu.\n", output_info.cbAlignment);

    hr = IMFTransform_GetOutputCurrentType(copier, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get current type, hr %#lx.\n", hr);
    IMFMediaType_Release(mediatype2);

    hr = IMFTransform_GetInputCurrentType(copier, 0, &mediatype2);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputStatus(copier, 0, &flags);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    /* Setting input type resets output type. */
    hr = IMFTransform_GetOutputCurrentType(copier, 0, &mediatype2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(mediatype2);

    hr = IMFTransform_SetInputType(copier, 0, mediatype, 0);
    ok(hr == S_OK, "Failed to set input type, hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputCurrentType(copier, 0, &mediatype2);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputAvailableType(copier, 0, 1, &mediatype2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(is_sample_copier_available_type(mediatype2), "Unexpected type.\n");
    IMFMediaType_Release(mediatype2);

    hr = IMFTransform_GetInputStreamInfo(copier, 0, &input_info);
    ok(hr == S_OK, "Failed to get stream info, hr %#lx.\n", hr);
    ok(!input_info.hnsMaxLatency, "Unexpected latency %s.\n", wine_dbgstr_longlong(input_info.hnsMaxLatency));
    ok(!input_info.dwFlags, "Unexpected flags %#lx.\n", input_info.dwFlags);
    ok(input_info.cbSize == 16 * 16, "Unexpected size %lu.\n", input_info.cbSize);
    ok(!input_info.cbMaxLookahead, "Unexpected lookahead size %lu.\n", input_info.cbMaxLookahead);
    ok(!input_info.cbAlignment, "Unexpected alignment %lu.\n", input_info.cbAlignment);

    hr = IMFTransform_GetOutputAvailableType(copier, 0, 0, &mediatype2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_IsEqual(mediatype2, mediatype, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(mediatype2);

    hr = IMFTransform_GetInputStatus(copier, 0, &flags);
    ok(hr == S_OK, "Failed to get input status, hr %#lx.\n", hr);
    ok(flags == MFT_INPUT_STATUS_ACCEPT_DATA, "Unexpected flags %#lx.\n", flags);

    hr = IMFTransform_GetInputCurrentType(copier, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get current type, hr %#lx.\n", hr);
    IMFMediaType_Release(mediatype2);

    hr = IMFTransform_GetOutputStatus(copier, &flags);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(copier, 0, mediatype, 0);
    ok(hr == S_OK, "Failed to set output type, hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputStatus(copier, &flags);
    ok(hr == S_OK, "Failed to get output status, hr %#lx.\n", hr);
    ok(!flags, "Unexpected flags %#lx.\n", flags);

    /* Pushing samples. */
    hr = MFCreateAlignedMemoryBuffer(output_info.cbSize, output_info.cbAlignment, &media_buffer);
    ok(hr == S_OK, "Failed to create media buffer, hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(sample, media_buffer);
    ok(hr == S_OK, "Failed to add a buffer, hr %#lx.\n", hr);
    IMFMediaBuffer_Release(media_buffer);

    EXPECT_REF(sample, 1);
    hr = IMFTransform_ProcessInput(copier, 0, sample, 0);
    ok(hr == S_OK, "Failed to process input, hr %#lx.\n", hr);
    EXPECT_REF(sample, 2);

    hr = IMFTransform_GetInputStatus(copier, 0, &flags);
    ok(hr == S_OK, "Failed to get input status, hr %#lx.\n", hr);
    ok(!flags, "Unexpected flags %#lx.\n", flags);

    hr = IMFTransform_GetOutputStatus(copier, &flags);
    ok(hr == S_OK, "Failed to get output status, hr %#lx.\n", hr);
    ok(flags == MFT_OUTPUT_STATUS_SAMPLE_READY, "Unexpected flags %#lx.\n", flags);

    hr = IMFTransform_ProcessInput(copier, 0, sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputStreamInfo(copier, 0, &output_info);
    ok(hr == S_OK, "Failed to get output info, hr %#lx.\n", hr);

    hr = MFCreateAlignedMemoryBuffer(output_info.cbSize, output_info.cbAlignment, &media_buffer);
    ok(hr == S_OK, "Failed to create media buffer, hr %#lx.\n", hr);

    hr = MFCreateSample(&client_sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(client_sample, media_buffer);
    ok(hr == S_OK, "Failed to add a buffer, hr %#lx.\n", hr);
    IMFMediaBuffer_Release(media_buffer);

    status = 0;
    memset(&buffer, 0, sizeof(buffer));
    buffer.pSample = client_sample;
    hr = IMFTransform_ProcessOutput(copier, 0, 1, &buffer, &status);
    ok(hr == S_OK, "Failed to get output, hr %#lx.\n", hr);
    EXPECT_REF(sample, 1);

    hr = IMFTransform_ProcessOutput(copier, 0, 1, &buffer, &status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "Failed to get output, hr %#lx.\n", hr);

    /* Flushing. */
    hr = IMFTransform_ProcessInput(copier, 0, sample, 0);
    ok(hr == S_OK, "Failed to process input, hr %#lx.\n", hr);
    EXPECT_REF(sample, 2);

    hr = IMFTransform_ProcessMessage(copier, MFT_MESSAGE_COMMAND_FLUSH, 0);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

    ref = IMFSample_Release(sample);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFSample_Release(client_sample);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFTransform_Release(copier);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(mediatype);
    ok(ref == 0, "Release returned %ld\n", ref);
}

struct sample_metadata
{
    unsigned int flags;
    LONGLONG duration;
    LONGLONG time;
};

static void sample_copier_process(IMFTransform *copier, IMFMediaBuffer *input_buffer,
        IMFMediaBuffer *output_buffer, const struct sample_metadata *md)
{
    static const struct sample_metadata zero_md = { 0, ~0u, ~0u };
    IMFSample *input_sample, *output_sample;
    MFT_OUTPUT_DATA_BUFFER buffer;
    DWORD flags, status;
    LONGLONG time;
    HRESULT hr;
    LONG ref;

    hr = MFCreateSample(&input_sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#lx.\n", hr);

    if (md)
    {
        hr = IMFSample_SetSampleFlags(input_sample, md->flags);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFSample_SetSampleTime(input_sample, md->time);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFSample_SetSampleDuration(input_sample, md->duration);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }

    hr = MFCreateSample(&output_sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#lx.\n", hr);

    hr = IMFSample_SetSampleFlags(output_sample, ~0u);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_SetSampleTime(output_sample, ~0u);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_SetSampleDuration(output_sample, ~0u);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(input_sample, input_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(output_sample, output_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_ProcessInput(copier, 0, input_sample, 0);
    ok(hr == S_OK, "Failed to process input, hr %#lx.\n", hr);

    status = 0;
    memset(&buffer, 0, sizeof(buffer));
    buffer.pSample = output_sample;
    hr = IMFTransform_ProcessOutput(copier, 0, 1, &buffer, &status);
    ok(hr == S_OK, "Failed to get output, hr %#lx.\n", hr);

    if (!md) md = &zero_md;

    hr = IMFSample_GetSampleFlags(output_sample, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(md->flags == flags, "Unexpected flags.\n");
    hr = IMFSample_GetSampleTime(output_sample, &time);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(md->time == time, "Unexpected time.\n");
    hr = IMFSample_GetSampleDuration(output_sample, &time);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(md->duration == time, "Unexpected duration.\n");

    ref = IMFSample_Release(input_sample);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFSample_Release(output_sample);
    ok(ref == 0, "Release returned %ld\n", ref);
}

static void test_sample_copier_output_processing(void)
{
    IMFMediaBuffer *input_buffer, *output_buffer;
    MFT_OUTPUT_STREAM_INFO output_info;
    struct sample_metadata md;
    IMFMediaType *mediatype;
    IMFTransform *copier;
    DWORD max_length;
    HRESULT hr;
    BYTE *ptr;
    LONG ref;

    if (!pMFCreateSampleCopierMFT)
        return;

    hr = pMFCreateSampleCopierMFT(&copier);
    ok(hr == S_OK, "Failed to create sample copier, hr %#lx.\n", hr);

    /* Configure for 16 x 16 of D3DFMT_X8R8G8B8. */
    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT64(mediatype, &MF_MT_FRAME_SIZE, ((UINT64)16) << 32 | 16);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(copier, 0, mediatype, 0);
    ok(hr == S_OK, "Failed to set input type, hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(copier, 0, mediatype, 0);
    ok(hr == S_OK, "Failed to set input type, hr %#lx.\n", hr);

    /* Source and destination are linear buffers, destination is twice as large. */
    hr = IMFTransform_GetOutputStreamInfo(copier, 0, &output_info);
    ok(hr == S_OK, "Failed to get output info, hr %#lx.\n", hr);

    hr = MFCreateAlignedMemoryBuffer(output_info.cbSize, output_info.cbAlignment, &output_buffer);
    ok(hr == S_OK, "Failed to create media buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(output_buffer, &ptr, &max_length, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    memset(ptr, 0xcc, max_length);
    hr = IMFMediaBuffer_Unlock(output_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateAlignedMemoryBuffer(output_info.cbSize, output_info.cbAlignment, &input_buffer);
    ok(hr == S_OK, "Failed to create media buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(input_buffer, &ptr, &max_length, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    memset(ptr, 0xaa, max_length);
    hr = IMFMediaBuffer_Unlock(input_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaBuffer_SetCurrentLength(input_buffer, 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    sample_copier_process(copier, input_buffer, output_buffer, NULL);

    hr = IMFMediaBuffer_Lock(output_buffer, &ptr, &max_length, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ptr[0] == 0xaa && ptr[4] == 0xcc, "Unexpected buffer contents.\n");

    hr = IMFMediaBuffer_Unlock(output_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    md.flags = 123;
    md.time = 10;
    md.duration = 2;
    sample_copier_process(copier, input_buffer, output_buffer, &md);

    ref = IMFMediaBuffer_Release(input_buffer);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaBuffer_Release(output_buffer);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFTransform_Release(copier);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(mediatype);
    ok(ref == 0, "Release returned %ld\n", ref);
}

static void test_MFGetTopoNodeCurrentType(void)
{
    IMFMediaType *media_type, *media_type2;
    IMFTopologyNode *node;
    HRESULT hr;
    LONG ref;

    if (!pMFGetTopoNodeCurrentType)
    {
        win_skip("MFGetTopoNodeCurrentType() is unsupported.\n");
        return;
    }

    /* Tee node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);

    hr = pMFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type2);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type2, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    /* Input type returned, if set. */
    hr = IMFTopologyNode_SetInputPrefType(node, 0, media_type2);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);

    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == media_type2, "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);

    hr = IMFTopologyNode_SetInputPrefType(node, 0, NULL);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);

    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Set second output. */
    hr = IMFTopologyNode_SetOutputPrefType(node, 1, media_type2);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);

    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetOutputPrefType(node, 1, NULL);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);

    /* Set first output. */
    hr = IMFTopologyNode_SetOutputPrefType(node, 0, media_type2);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);

    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == media_type2, "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);

    hr = IMFTopologyNode_SetOutputPrefType(node, 0, NULL);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);

    /* Set primary output. */
    hr = IMFTopologyNode_SetOutputPrefType(node, 1, media_type2);
    ok(hr == S_OK, "Failed to set media type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_PRIMARYOUTPUT, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = pMFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == media_type2, "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);

    hr = pMFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == media_type2, "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(media_type2);
    ok(ref == 0, "Release returned %ld\n", ref);
}

static void init_functions(void)
{
    HMODULE mod = GetModuleHandleA("mf.dll");

#define X(f) p##f = (void*)GetProcAddress(mod, #f)
    X(MFCreateSampleCopierMFT);
    X(MFGetTopoNodeCurrentType);
#undef X
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

#define check_sample(a, b, c) check_sample_(__LINE__, a, b, c)
static void check_sample_(int line, IMFSample *sample, const BYTE *expect_buf, HANDLE output_file)
{
    IMFMediaBuffer *media_buffer;
    DWORD length;
    BYTE *buffer;
    HRESULT hr;
    ULONG ret;

    hr = IMFSample_ConvertToContiguousBuffer(sample, &media_buffer);
    ok_(__FILE__, line)(hr == S_OK, "ConvertToContiguousBuffer returned %#lx\n", hr);
    hr = IMFMediaBuffer_Lock(media_buffer, &buffer, NULL, &length);
    ok_(__FILE__, line)(hr == S_OK, "Lock returned %#lx\n", hr);
    ok_(__FILE__, line)(!memcmp(expect_buf, buffer, length), "unexpected buffer data\n");
    if (output_file) WriteFile(output_file, buffer, length, &length, NULL);
    hr = IMFMediaBuffer_Unlock(media_buffer);
    ok_(__FILE__, line)(hr == S_OK, "Unlock returned %#lx\n", hr);
    ret = IMFMediaBuffer_Release(media_buffer);
    ok_(__FILE__, line)(ret == 1, "Release returned %lu\n", ret);
}

#define check_sample_rgb32(a, b, c) check_sample_rgb32_(__LINE__, a, b, c)
static void check_sample_rgb32_(int line, IMFSample *sample, const BYTE *expect_buf, HANDLE output_file)
{
    DWORD i, length, diff = 0, max_diff;
    IMFMediaBuffer *media_buffer;
    BYTE *buffer;
    HRESULT hr;
    ULONG ret;

    hr = IMFSample_ConvertToContiguousBuffer(sample, &media_buffer);
    ok_(__FILE__, line)(hr == S_OK, "ConvertToContiguousBuffer returned %#lx\n", hr);
    hr = IMFMediaBuffer_Lock(media_buffer, &buffer, NULL, &length);
    ok_(__FILE__, line)(hr == S_OK, "Lock returned %#lx\n", hr);

    /* check that buffer values are "close" enough, there's some pretty big
     * differences with the color converter between ffmpeg and native.
     */
    for (i = 0; i < length; i++)
    {
        if (i % 4 == 3) continue; /* ignore alpha diff */
        diff += abs((int)expect_buf[i] - (int)buffer[i]);
    }
    max_diff = length * 3 * 256;
    ok_(__FILE__, line)(diff * 100 / max_diff == 0, "unexpected buffer data\n");

    if (output_file) WriteFile(output_file, buffer, length, &length, NULL);
    hr = IMFMediaBuffer_Unlock(media_buffer);
    ok_(__FILE__, line)(hr == S_OK, "Unlock returned %#lx\n", hr);
    ret = IMFMediaBuffer_Release(media_buffer);
    ok_(__FILE__, line)(ret == 1, "Release returned %lu\n", ret);
}

#define check_sample_pcm16(a, b, c, d) check_sample_pcm16_(__LINE__, a, b, c, d)
static void check_sample_pcm16_(int line, IMFSample *sample, const BYTE *expect_buf, HANDLE output_file, BOOL todo)
{
    IMFMediaBuffer *media_buffer;
    DWORD i, length;
    BYTE *buffer;
    HRESULT hr;
    ULONG ret;

    hr = IMFSample_ConvertToContiguousBuffer(sample, &media_buffer);
    ok_(__FILE__, line)(hr == S_OK, "ConvertToContiguousBuffer returned %#lx\n", hr);
    hr = IMFMediaBuffer_Lock(media_buffer, &buffer, NULL, &length);
    ok_(__FILE__, line)(hr == S_OK, "Lock returned %#lx\n", hr);

    /* check that buffer values are close enough, there's some differences in
     * the output of audio DSP between 32bit and 64bit implementation
     */
    for (i = 0; i < length; i += 2)
    {
        DWORD expect = *(INT16 *)(expect_buf + i), value = *(INT16 *)(buffer + i);
        if (expect - value + 512 > 1024) break;
    }

    todo_wine_if(todo)
    ok_(__FILE__, line)(i == length, "unexpected buffer data\n");

    if (output_file) WriteFile(output_file, buffer, length, &length, NULL);
    hr = IMFMediaBuffer_Unlock(media_buffer);
    ok_(__FILE__, line)(hr == S_OK, "Unlock returned %#lx\n", hr);
    ret = IMFMediaBuffer_Release(media_buffer);
    ok_(__FILE__, line)(ret == 1, "Release returned %lu\n", ret);
}

static const BYTE wma_codec_data[10] = {0, 0x44, 0, 0, 0x17, 0, 0, 0, 0, 0};
static const ULONG wmaenc_block_size = 1487;
static const ULONG wmadec_block_size = 0x2000;

static void test_wma_encoder(void)
{
    const GUID transform_inputs[] =
    {
        MFAudioFormat_PCM,
        MFAudioFormat_Float,
    };
    const GUID transform_outputs[] =
    {
        MFAudioFormat_WMAudioV8,
        MFAudioFormat_WMAudioV9,
        MFAudioFormat_WMAudio_Lossless,
    };
    const GUID dmo_inputs[] =
    {
        MEDIASUBTYPE_PCM,
    };
    const GUID dmo_outputs[] =
    {
        MEDIASUBTYPE_WMAUDIO2,
        MEDIASUBTYPE_WMAUDIO3,
        MEDIASUBTYPE_WMAUDIO_LOSSLESS,
    };

    static const struct attribute_desc input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 176400),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 8),
        {0},
    };
    const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_WMAudioV8),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 4003),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, wmaenc_block_size),
        ATTR_BLOB(MF_MT_USER_DATA, wma_codec_data, sizeof(wma_codec_data)),
        {0},
    };

    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Audio, MFAudioFormat_WMAudioV8};
    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Audio, MFAudioFormat_Float};
    ULONG audio_data_len, wmaenc_data_len;
    const BYTE *audio_data, *wmaenc_data;
    MFT_OUTPUT_STREAM_INFO output_info;
    MFT_INPUT_STREAM_INFO input_info;
    MFT_OUTPUT_DATA_BUFFER output;
    WCHAR output_path[MAX_PATH];
    IMFMediaType *media_type;
    IMFTransform *transform;
    DWORD status, length;
    HANDLE output_file;
    IMFSample *sample;
    HRSRC resource;
    GUID class_id;
    ULONG i, ret;
    HRESULT hr;
    LONG ref;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    if (!create_transform(MFT_CATEGORY_AUDIO_ENCODER, &input_type, &output_type, L"WMAudio Encoder MFT", &MFMediaType_Audio,
            transform_inputs, ARRAY_SIZE(transform_inputs), transform_outputs, ARRAY_SIZE(transform_outputs),
            &transform, &CLSID_CWMAEncMediaObject, &class_id))
        goto failed;

    check_dmo(&class_id, L"WMAudio Encoder DMO", &MEDIATYPE_Audio, dmo_inputs, ARRAY_SIZE(dmo_inputs),
            dmo_outputs, ARRAY_SIZE(dmo_outputs));

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_IMediaObject, TRUE);
    check_interface(transform, &IID_IPropertyStore, TRUE);
    check_interface(transform, &IID_IPropertyBag, TRUE);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    init_media_type(media_type, input_type_desc, -1);
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    init_media_type(media_type, output_type_desc, -1);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 0, "Release returned %lu\n", ret);

    memset(&input_info, 0xcd, sizeof(input_info));
    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
    ok(hr == S_OK, "GetInputStreamInfo returned %#lx\n", hr);
    ok(input_info.hnsMaxLatency == 19969161, "got hnsMaxLatency %s\n",
            wine_dbgstr_longlong(input_info.hnsMaxLatency));
    ok(input_info.dwFlags == 0, "got dwFlags %#lx\n", input_info.dwFlags);
    ok(input_info.cbSize == 8, "got cbSize %lu\n", input_info.cbSize);
    ok(input_info.cbMaxLookahead == 0, "got cbMaxLookahead %#lx\n", input_info.cbMaxLookahead);
    ok(input_info.cbAlignment == 1, "got cbAlignment %#lx\n", input_info.cbAlignment);

    memset(&output_info, 0xcd, sizeof(output_info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == S_OK, "GetOutputStreamInfo returned %#lx\n", hr);
    ok(output_info.dwFlags == 0, "got dwFlags %#lx\n", output_info.dwFlags);
    ok(output_info.cbSize == wmaenc_block_size, "got cbSize %#lx\n", output_info.cbSize);
    ok(output_info.cbAlignment == 1, "got cbAlignment %#lx\n", output_info.cbAlignment);

    resource = FindResourceW(NULL, L"audiodata.bin", (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    audio_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    audio_data_len = SizeofResource(GetModuleHandleW(NULL), resource);
    ok(audio_data_len == 179928, "got length %lu\n", audio_data_len);

    sample = create_sample(audio_data, audio_data_len);
    hr = IMFSample_SetSampleTime(sample, 0);
    ok(hr == S_OK, "SetSampleTime returned %#lx\n", hr);
    hr = IMFSample_SetSampleDuration(sample, 10000000);
    ok(hr == S_OK, "SetSampleDuration returned %#lx\n", hr);
    hr = IMFTransform_ProcessInput(transform, 0, sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, 0);
    ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);
    hr = IMFTransform_ProcessInput(transform, 0, sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);
    ref = IMFSample_Release(sample);
    ok(ref <= 1, "Release returned %ld\n", ref);

    status = 0xdeadbeef;
    sample = create_sample(NULL, output_info.cbSize);
    memset(&output, 0, sizeof(output));
    output.pSample = sample;

    resource = FindResourceW(NULL, L"wmaencdata.bin", (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    wmaenc_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    wmaenc_data_len = SizeofResource(GetModuleHandleW(NULL), resource);
    ok(wmaenc_data_len % wmaenc_block_size == 0, "got length %lu\n", wmaenc_data_len);

    /* and generate a new one as well in a temporary directory */
    GetTempPathW(ARRAY_SIZE(output_path), output_path);
    lstrcatW(output_path, L"wmaencdata.bin");
    output_file = CreateFileW(output_path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(output_file != INVALID_HANDLE_VALUE, "CreateFileW failed, error %lu\n", GetLastError());

    i = 0;
    while (SUCCEEDED(hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status)))
    {
        winetest_push_context("%lu", i);
        ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
        ok(output.pSample == sample, "got pSample %p\n", output.pSample);
        ok(output.dwStatus == MFT_OUTPUT_DATA_BUFFER_INCOMPLETE ||
                broken(output.dwStatus == (MFT_OUTPUT_DATA_BUFFER_INCOMPLETE|7)) /* win7 */,
                "got dwStatus %#lx\n", output.dwStatus);
        ok(status == 0, "got status %#lx\n", status);
        hr = IMFSample_GetTotalLength(sample, &length);
        ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
        ok(length == wmaenc_block_size, "got length %lu\n", length);
        ok(wmaenc_data_len > i * wmaenc_block_size, "got %lu blocks\n", i);
        check_sample(sample, wmaenc_data + i * wmaenc_block_size, output_file);
        winetest_pop_context();
        i++;
    }

    trace("created %s\n", debugstr_w(output_path));
    CloseHandle(output_file);

    ret = IMFSample_Release(sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    status = 0xdeadbeef;
    sample = create_sample(NULL, output_info.cbSize);
    memset(&output, 0, sizeof(output));
    output.pSample = sample;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output.pSample == sample, "got pSample %p\n", output.pSample);
    ok(output.dwStatus == 0, "got dwStatus %#lx\n", output.dwStatus);
    ok(status == 0, "got status %#lx\n", status);
    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    ret = IMFSample_Release(sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

failed:
    CoUninitialize();
}

static void test_wma_decoder(void)
{
    const GUID transform_inputs[] =
    {
        MEDIASUBTYPE_MSAUDIO1,
        MFAudioFormat_WMAudioV8,
        MFAudioFormat_WMAudioV9,
        MFAudioFormat_WMAudio_Lossless,
    };
    const GUID transform_outputs[] =
    {
        MFAudioFormat_PCM,
        MFAudioFormat_Float,
    };
    const GUID dmo_inputs[] =
    {
        MEDIASUBTYPE_MSAUDIO1,
        MEDIASUBTYPE_WMAUDIO2,
        MEDIASUBTYPE_WMAUDIO3,
        MEDIASUBTYPE_WMAUDIO_LOSSLESS,
    };
    const GUID dmo_outputs[] =
    {
        MEDIASUBTYPE_PCM,
        MEDIASUBTYPE_IEEE_FLOAT,
    };

    static const media_type_desc expect_available_inputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_MSAUDIO1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_WMAudioV8),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_WMAudioV9),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_WMAudio_Lossless),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
    };
    static const media_type_desc expect_available_outputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
            ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
            ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
            ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 176400),
            ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 8),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
            ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
            ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
            ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 88200),
            ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        },
    };

    const struct attribute_desc input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_WMAudioV8),
        ATTR_BLOB(MF_MT_USER_DATA, wma_codec_data, sizeof(wma_codec_data)),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, wmaenc_block_size),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        {0},
    };
    static const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 88200),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
        {0},
    };

    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Audio, MFAudioFormat_WMAudioV8};
    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Audio, MFAudioFormat_Float};
    IUnknown *unknown, *tmp_unknown, outer = {&test_unk_vtbl};
    ULONG wmadec_data_len, wmaenc_data_len;
    const BYTE *wmadec_data, *wmaenc_data;
    MFT_OUTPUT_STREAM_INFO output_info;
    MFT_OUTPUT_DATA_BUFFER outputs[2];
    MFT_INPUT_STREAM_INFO input_info;
    MFT_OUTPUT_DATA_BUFFER output;
    DWORD status, flags, length;
    WCHAR output_path[MAX_PATH];
    IMediaObject *media_object;
    IPropertyBag *property_bag;
    IMFMediaType *media_type;
    IMFTransform *transform;
    LONGLONG time, duration;
    HANDLE output_file;
    IMFSample *sample;
    ULONG i, ret, ref;
    HRSRC resource;
    GUID class_id;
    UINT32 value;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    if (!create_transform(MFT_CATEGORY_AUDIO_DECODER, &input_type, &output_type, L"WMAudio Decoder MFT", &MFMediaType_Audio,
            transform_inputs, ARRAY_SIZE(transform_inputs), transform_outputs, ARRAY_SIZE(transform_outputs),
            &transform, &CLSID_CWMADecMediaObject, &class_id))
        goto failed;

    check_dmo(&class_id, L"WMAudio Decoder DMO", &MEDIATYPE_Audio, dmo_inputs, ARRAY_SIZE(dmo_inputs),
            dmo_outputs, ARRAY_SIZE(dmo_outputs));

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_IMediaObject, TRUE);
    todo_wine
    check_interface(transform, &IID_IPropertyStore, TRUE);
    check_interface(transform, &IID_IPropertyBag, TRUE);

    /* check default media types */

    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetInputStreamInfo returned %#lx\n", hr);
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetOutputStreamInfo returned %#lx\n", hr);
    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetOutputAvailableType returned %#lx\n", hr);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetInputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("in %lu", i);
        ok(hr == S_OK, "GetInputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_inputs[i], -1);
        hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
        ok(hr == MF_E_INVALIDMEDIATYPE, "SetInputType returned %#lx.\n", hr);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    todo_wine
    ok(hr == MF_E_NO_MORE_TYPES, "GetInputAvailableType returned %#lx\n", hr);
    todo_wine
    ok(i == 4, "%lu input media types\n", i);

    /* setting output media type first doesn't work */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    init_media_type(media_type, output_type_desc, -1);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "SetOutputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 0, "Release returned %lu\n", ret);

    /* check required input media type attributes */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "SetInputType returned %#lx.\n", hr);
    init_media_type(media_type, input_type_desc, 1);
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "SetInputType returned %#lx.\n", hr);
    init_media_type(media_type, input_type_desc, 2);
    for (i = 2; i < ARRAY_SIZE(input_type_desc) - 1; ++i)
    {
        hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
        ok(hr == MF_E_INVALIDMEDIATYPE, "SetInputType returned %#lx.\n", hr);
        init_media_type(media_type, input_type_desc, i + 1);
    }
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 0, "Release returned %lu\n", ret);

    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetInputStreamInfo returned %#lx\n", hr);
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetOutputStreamInfo returned %#lx\n", hr);

    /* check new output media types */

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == 2, "%lu output media types\n", i);

    /* check required output media type attributes */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "SetOutputType returned %#lx.\n", hr);
    init_media_type(media_type, output_type_desc, 1);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "SetOutputType returned %#lx.\n", hr);
    init_media_type(media_type, output_type_desc, 2);
    for (i = 2; i < ARRAY_SIZE(output_type_desc) - 1; ++i)
    {
        hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
        ok(hr == MF_E_INVALIDMEDIATYPE, "SetOutputType returned %#lx.\n", hr);
        init_media_type(media_type, output_type_desc, i + 1);
    }
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 0, "Release returned %lu\n", ret);

    memset(&input_info, 0xcd, sizeof(input_info));
    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
    ok(hr == S_OK, "GetInputStreamInfo returned %#lx\n", hr);
    ok(input_info.hnsMaxLatency == 0, "got hnsMaxLatency %s\n", wine_dbgstr_longlong(input_info.hnsMaxLatency));
    ok(input_info.dwFlags == 0, "got dwFlags %#lx\n", input_info.dwFlags);
    ok(input_info.cbSize == wmaenc_block_size, "got cbSize %lu\n", input_info.cbSize);
    ok(input_info.cbMaxLookahead == 0, "got cbMaxLookahead %#lx\n", input_info.cbMaxLookahead);
    ok(input_info.cbAlignment == 1, "got cbAlignment %#lx\n", input_info.cbAlignment);

    memset(&output_info, 0xcd, sizeof(output_info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == S_OK, "GetOutputStreamInfo returned %#lx\n", hr);
    ok(output_info.dwFlags == 0, "got dwFlags %#lx\n", output_info.dwFlags);
    todo_wine
    ok(output_info.cbSize == 0, "got cbSize %#lx\n", output_info.cbSize);
    ok(output_info.cbAlignment == 1, "got cbAlignment %#lx\n", output_info.cbAlignment);

    /* MF_MT_AUDIO_AVG_BYTES_PER_SECOND isn't required by SetInputType, but is needed for the transform to work */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    init_media_type(media_type, input_type_desc, -1);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 4003);
    ok(hr == S_OK, "SetUINT32 returned %#lx\n", hr);
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);

    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetOutputStreamInfo returned %#lx\n", hr);

    init_media_type(media_type, output_type_desc, -1);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 0, "Release returned %lu\n", ret);

    memset(&output_info, 0xcd, sizeof(output_info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == S_OK, "GetOutputStreamInfo returned %#lx\n", hr);
    ok(output_info.dwFlags == 0, "got dwFlags %#lx\n", output_info.dwFlags);
    ok(output_info.cbSize == wmadec_block_size, "got cbSize %#lx\n", output_info.cbSize);
    ok(output_info.cbAlignment == 1, "got cbAlignment %#lx\n", output_info.cbAlignment);

    resource = FindResourceW(NULL, L"wmaencdata.bin", (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    wmaenc_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    wmaenc_data_len = SizeofResource(GetModuleHandleW(NULL), resource);
    ok(wmaenc_data_len % wmaenc_block_size == 0, "got length %lu\n", wmaenc_data_len);

    sample = create_sample(wmaenc_data, wmaenc_block_size / 2);
    hr = IMFTransform_ProcessInput(transform, 0, sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
    ret = IMFSample_Release(sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    sample = create_sample(wmaenc_data, wmaenc_block_size + 1);
    hr = IMFTransform_ProcessInput(transform, 0, sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
    ret = IMFSample_Release(sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    sample = create_sample(wmaenc_data, wmaenc_block_size);
    hr = IMFTransform_ProcessInput(transform, 0, sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
    hr = IMFTransform_ProcessInput(transform, 0, sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);
    ret = IMFSample_Release(sample);
    ok(ret == 1, "Release returned %lu\n", ret);

    /* As output_info.dwFlags doesn't have MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES
     * IMFTransform_ProcessOutput needs a sample or returns MF_E_TRANSFORM_NEED_MORE_INPUT */

    status = 0xdeadbeef;
    memset(&output, 0, sizeof(output));
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output.dwStreamID == 0, "got dwStreamID %lu\n", output.dwStreamID);
    ok(!output.pSample, "got pSample %p\n", output.pSample);
    ok(output.dwStatus == MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE ||
            broken(output.dwStatus == (MFT_OUTPUT_DATA_BUFFER_INCOMPLETE|MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE)) /* Win7 */,
            "got dwStatus %#lx\n", output.dwStatus);
    ok(!output.pEvents, "got pEvents %p\n", output.pEvents);
    ok(status == 0, "got status %#lx\n", status);

    sample = create_sample(wmaenc_data, wmaenc_block_size);
    hr = IMFTransform_ProcessInput(transform, 0, sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);
    ret = IMFSample_Release(sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    status = 0xdeadbeef;
    memset(&output, 0, sizeof(output));
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(!output.pSample, "got pSample %p\n", output.pSample);
    ok(output.dwStatus == MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE ||
            broken(output.dwStatus == (MFT_OUTPUT_DATA_BUFFER_INCOMPLETE|MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE)) /* Win7 */,
            "got dwStatus %#lx\n", output.dwStatus);
    ok(status == 0, "got status %#lx\n", status);

    status = 0xdeadbeef;
    memset(&output, 0, sizeof(output));
    output_info.cbSize = wmadec_block_size;
    sample = create_sample(NULL, output_info.cbSize);
    outputs[0].pSample = sample;
    sample = create_sample(NULL, output_info.cbSize);
    outputs[1].pSample = sample;
    hr = IMFTransform_ProcessOutput(transform, 0, 2, outputs, &status);
    ok(hr == E_INVALIDARG, "ProcessOutput returned %#lx\n", hr);
    ref = IMFSample_Release(outputs[0].pSample);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFSample_Release(outputs[1].pSample);
    ok(ref == 0, "Release returned %ld\n", ref);

    resource = FindResourceW(NULL, L"wmadecdata.bin", (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    wmadec_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    wmadec_data_len = SizeofResource(GetModuleHandleW(NULL), resource);
    ok(wmadec_data_len == wmadec_block_size * 7 / 2, "got length %lu\n", wmadec_data_len);

    /* and generate a new one as well in a temporary directory */
    GetTempPathW(ARRAY_SIZE(output_path), output_path);
    lstrcatW(output_path, L"wmadecdata.bin");
    output_file = CreateFileW(output_path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(output_file != INVALID_HANDLE_VALUE, "CreateFileW failed, error %lu\n", GetLastError());

    status = 0xdeadbeef;
    output_info.cbSize = wmadec_block_size;
    sample = create_sample(NULL, output_info.cbSize);
    memset(&output, 0, sizeof(output));
    output.pSample = sample;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);

    for (i = 0; i < 4; ++i)
    {
        winetest_push_context("%lu", i);

        ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
        ok(output.pSample == sample, "got pSample %p\n", output.pSample);
        ok(output.dwStatus == MFT_OUTPUT_DATA_BUFFER_INCOMPLETE || output.dwStatus == 0 ||
                broken(output.dwStatus == (MFT_OUTPUT_DATA_BUFFER_INCOMPLETE|7) || output.dwStatus == 7) /* Win7 */,
                "got dwStatus %#lx\n", output.dwStatus);
        ok(status == 0, "got status %#lx\n", status);
        value = 0xdeadbeef;
        hr = IMFSample_GetUINT32(sample, &MFSampleExtension_CleanPoint, &value);
        ok(hr == S_OK, "GetUINT32 MFSampleExtension_CleanPoint returned %#lx\n", hr);
        ok(value == 1, "got MFSampleExtension_CleanPoint %u\n", value);
        hr = IMFSample_GetTotalLength(sample, &length);
        ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
        flags = 0xdeadbeef;
        hr = IMFSample_GetSampleFlags(sample, &flags);
        ok(hr == S_OK, "GetSampleFlags returned %#lx\n", hr);
        ok(flags == 0, "got flags %#lx\n", flags);
        time = 0xdeadbeef;
        hr = IMFSample_GetSampleTime(sample, &time);
        ok(hr == S_OK, "GetSampleTime returned %#lx\n", hr);
        ok(time == i * 928798, "got time %I64d\n", time);
        duration = 0xdeadbeef;
        hr = IMFSample_GetSampleDuration(sample, &duration);
        ok(hr == S_OK, "GetSampleDuration returned %#lx\n", hr);
        if (output.dwStatus == MFT_OUTPUT_DATA_BUFFER_INCOMPLETE ||
                broken(output.dwStatus == (MFT_OUTPUT_DATA_BUFFER_INCOMPLETE|7)))
        {
            ok(length == wmadec_block_size, "got length %lu\n", length);
            ok(duration == 928798, "got duration %I64d\n", duration);
            check_sample_pcm16(sample, wmadec_data, output_file, TRUE);
            wmadec_data += wmadec_block_size;
            wmadec_data_len -= wmadec_block_size;
        }
        else
        {
            /* FFmpeg doesn't seem to decode WMA buffers in the same way as native */
            todo_wine
            ok(length == wmadec_block_size / 2, "got length %lu\n", length);
            todo_wine
            ok(duration == 464399, "got duration %I64d\n", duration);

            if (length == wmadec_block_size / 2)
                check_sample_pcm16(sample, wmadec_data, output_file, FALSE);
            wmadec_data += length;
            wmadec_data_len -= length;
        }
        ret = IMFSample_Release(sample);
        ok(ret == 0, "Release returned %lu\n", ret);

        status = 0xdeadbeef;
        sample = create_sample(NULL, output_info.cbSize);
        memset(&output, 0, sizeof(output));
        output.pSample = sample;
        hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);

        winetest_pop_context();
    }
    todo_wine
    ok(wmadec_data_len == 0, "missing %#lx bytes\n", wmadec_data_len);

    trace("created %s\n", debugstr_w(output_path));
    CloseHandle(output_file);

    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output.pSample == sample, "got pSample %p\n", output.pSample);
    ok(output.dwStatus == 0, "got dwStatus %#lx\n", output.dwStatus);
    ok(status == 0, "got status %#lx\n", status);
    ret = IMFSample_Release(sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    status = 0xdeadbeef;
    sample = create_sample(NULL, output_info.cbSize);
    memset(&output, 0, sizeof(output));
    output.pSample = sample;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output.pSample == sample, "got pSample %p\n", output.pSample);
    ok(output.dwStatus == 0 ||
            broken(output.dwStatus == (MFT_OUTPUT_DATA_BUFFER_INCOMPLETE|7)) /* Win7 */,
            "got dwStatus %#lx\n", output.dwStatus);
    ok(status == 0, "got status %#lx\n", status);
    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    ret = IMFSample_Release(sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    sample = create_sample(wmaenc_data, wmaenc_block_size);
    hr = IMFTransform_ProcessInput(transform, 0, sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);

    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);
    ret = IMFSample_Release(sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    hr = CoCreateInstance( &CLSID_CWMADecMediaObject, &outer, CLSCTX_INPROC_SERVER, &IID_IUnknown,
                           (void **)&unknown );
    ok( hr == S_OK, "CoCreateInstance returned %#lx\n", hr );
    hr = IUnknown_QueryInterface( unknown, &IID_IMFTransform, (void **)&transform );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IUnknown_QueryInterface( unknown, &IID_IMediaObject, (void **)&media_object );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IUnknown_QueryInterface( unknown, &IID_IPropertyBag, (void **)&property_bag );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IUnknown_QueryInterface( media_object, &IID_IUnknown, (void **)&tmp_unknown );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    ok( unknown != &outer, "got outer IUnknown\n" );
    ok( transform != (void *)unknown, "got IUnknown == IMFTransform\n" );
    ok( media_object != (void *)unknown, "got IUnknown == IMediaObject\n" );
    ok( property_bag != (void *)unknown, "got IUnknown == IPropertyBag\n" );
    ok( tmp_unknown != unknown, "got inner IUnknown\n" );

    check_interface( unknown, &IID_IPersistPropertyBag, FALSE );
    check_interface( unknown, &IID_IAMFilterMiscFlags, FALSE );
    check_interface( unknown, &IID_IMediaSeeking, FALSE );
    check_interface( unknown, &IID_IMediaPosition, FALSE );
    check_interface( unknown, &IID_IReferenceClock, FALSE );
    check_interface( unknown, &IID_IBasicAudio, FALSE );

    ref = IUnknown_Release( tmp_unknown );
    ok( ref == 1, "Release returned %lu\n", ref );
    ref = IPropertyBag_Release( property_bag );
    ok( ref == 1, "Release returned %lu\n", ref );
    ref = IMediaObject_Release( media_object );
    ok( ref == 1, "Release returned %lu\n", ref );
    ref = IMFTransform_Release( transform );
    ok( ref == 1, "Release returned %lu\n", ref );
    ref = IUnknown_Release( unknown );
    ok( ref == 0, "Release returned %lu\n", ref );

failed:
    CoUninitialize();
}

#define next_h264_sample(a, b) next_h264_sample_(__LINE__, a, b)
static IMFSample *next_h264_sample_(int line, const BYTE **h264_buf, ULONG *h264_len)
{
    const BYTE *sample_data;

    ok_(__FILE__, line)(*h264_len > 4, "invalid h264 length\n");
    ok_(__FILE__, line)(*(UINT32 *)*h264_buf == 0x01000000, "invalid h264 buffer\n");
    sample_data = *h264_buf;

    (*h264_len) -= 4;
    (*h264_buf) += 4;

    while (*h264_len >= 4 && *(UINT32 *)*h264_buf != 0x01000000)
    {
        (*h264_len)--;
        (*h264_buf)++;
    }

    return create_sample(sample_data, *h264_buf - sample_data);
}

static void test_h264_decoder(void)
{
    const GUID transform_inputs[] =
    {
        MFVideoFormat_H264,
        MFVideoFormat_H264_ES,
    };
    const GUID transform_outputs[] =
    {
        MFVideoFormat_NV12,
        MFVideoFormat_YV12,
        MFVideoFormat_IYUV,
        MFVideoFormat_I420,
        MFVideoFormat_YUY2,
    };
    static const media_type_desc default_inputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264_ES),
        },
    };
    static const media_type_desc default_outputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 1920),
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YV12),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 1920),
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_IYUV),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 1920),
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 1920),
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 3840),
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
    };
    static const media_type_desc default_outputs_extra[] =
    {
        {
            ATTR_RATIO(MF_MT_FRAME_SIZE, 1920, 1080),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, 3110400),
            ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0),
        },
        {
            ATTR_RATIO(MF_MT_FRAME_SIZE, 1920, 1080),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, 3110400),
            ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0),
        },
        {
            ATTR_RATIO(MF_MT_FRAME_SIZE, 1920, 1080),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, 3110400),
            ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0),
        },
        {
            ATTR_RATIO(MF_MT_FRAME_SIZE, 1920, 1080),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, 3110400),
            ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0),
        },
        {
            ATTR_RATIO(MF_MT_FRAME_SIZE, 1920, 1080),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, 4147200),
            ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0),
        },
    };
    static const media_type_desc default_outputs_win7[] =
    {
        {
            ATTR_RATIO(MF_MT_FRAME_SIZE, 1920, 1088),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, 3133440),
        },
        {
            ATTR_RATIO(MF_MT_FRAME_SIZE, 1920, 1088),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, 3133440),
        },
        {
            ATTR_RATIO(MF_MT_FRAME_SIZE, 1920, 1088),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, 3133440),
        },
        {
            ATTR_RATIO(MF_MT_FRAME_SIZE, 1920, 1088),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, 3133440),
        },
        {
            ATTR_RATIO(MF_MT_FRAME_SIZE, 1920, 1088),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, 4177920),
        },
    };
    static const struct attribute_desc input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264),
        {0},
    };
    static const struct attribute_desc minimal_output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1920, 1080),
        {0},
    };
    static const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1920, 1080),
        ATTR_RATIO(MF_MT_FRAME_RATE, 60000, 1000),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 2, 1),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 3840),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, 3840 * 1080 * 3 / 2),
        ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0),
        {0},
    };
    static const struct attribute_desc output_type_desc_win7[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1920, 1088),
        ATTR_RATIO(MF_MT_FRAME_RATE, 60000, 1000),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 2, 1),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 3840),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, 3840 * 1088 * 3 / 2),
        ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0),
        {0},
    };
    static const struct attribute_desc new_output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_RATIO(MF_MT_FRAME_RATE, 1, 1),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 2),
        {0},
    };
    static const struct attribute_desc new_output_type_desc_win7[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_RATIO(MF_MT_FRAME_RATE, 1, 1),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 2),
        {0},
    };
    static const MFVideoArea actual_aperture = {.Area={82,84}};
    static const DWORD actual_width = 96, actual_height = 96;
    const media_type_desc actual_outputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 60000, 1000),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
            /* ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0), missing on Win7 */
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YV12),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 60000, 1000),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
            /* ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0), missing on Win7 */
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_IYUV),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 60000, 1000),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
            /* ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0), missing on Win7 */
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 60000, 1000),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
            /* ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0), missing on Win7 */
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 60000, 1000),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 2),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 2),
            /* ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0), missing on Win7 */
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
        },
    };

    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Video, MFVideoFormat_H264};
    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Video, MFVideoFormat_NV12};
    const BYTE *h264_encoded_data, *nv12_frame_data, *i420_frame_data;
    ULONG h264_encoded_data_len, nv12_frame_len, i420_frame_len;
    DWORD input_id, output_id, input_count, output_count;
    MFT_OUTPUT_STREAM_INFO output_info;
    MFT_INPUT_STREAM_INFO input_info;
    MFT_OUTPUT_DATA_BUFFER output;
    IMFMediaBuffer *media_buffer;
    DWORD status, length, count;
    WCHAR output_path[MAX_PATH];
    IMFAttributes *attributes;
    IMFMediaType *media_type;
    LONGLONG time, duration;
    IMFTransform *transform;
    BOOL is_win7 = FALSE;
    ULONG i, ret, flags;
    HANDLE output_file;
    IMFSample *sample;
    HRSRC resource;
    GUID class_id;
    UINT32 value;
    BYTE *data;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    if (!create_transform(MFT_CATEGORY_VIDEO_DECODER, &input_type, &output_type, L"Microsoft H264 Video Decoder MFT", &MFMediaType_Video,
            transform_inputs, ARRAY_SIZE(transform_inputs), transform_outputs, ARRAY_SIZE(transform_outputs),
            &transform, &CLSID_MSH264DecoderMFT, &class_id))
        goto failed;

    hr = IMFTransform_GetAttributes(transform, &attributes);
    ok(hr == S_OK, "GetAttributes returned %#lx\n", hr);
    hr = IMFAttributes_SetUINT32(attributes, &MF_LOW_LATENCY, 1);
    ok(hr == S_OK, "SetUINT32 returned %#lx\n", hr);
    ret = IMFAttributes_Release(attributes);
    todo_wine
    ok(ret == 1, "Release returned %ld\n", ret);

    /* no output type is available before an input type is set */

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetOutputAvailableType returned %#lx\n", hr);
    hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetOutputCurrentType returned %#lx\n", hr);

    /* setting output media type first doesn't work */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    init_media_type(media_type, default_outputs[0], -1);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "SetOutputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 0, "Release returned %lu\n", ret);

    /* check available input types */

    flags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_INPUT_STREAM_FIXED_SAMPLE_SIZE;
    memset(&input_info, 0xcd, sizeof(input_info));
    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
    todo_wine
    ok(hr == S_OK, "GetInputStreamInfo returned %#lx\n", hr);
    todo_wine
    ok(input_info.hnsMaxLatency == 0, "got hnsMaxLatency %s\n", wine_dbgstr_longlong(input_info.hnsMaxLatency));
    todo_wine
    ok(input_info.dwFlags == flags, "got dwFlags %#lx\n", input_info.dwFlags);
    todo_wine
    ok(input_info.cbSize == 0x1000, "got cbSize %lu\n", input_info.cbSize);
    todo_wine
    ok(input_info.cbMaxLookahead == 0, "got cbMaxLookahead %#lx\n", input_info.cbMaxLookahead);
    todo_wine
    ok(input_info.cbAlignment == 0, "got cbAlignment %#lx\n", input_info.cbAlignment);

    flags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE;
    memset(&output_info, 0xcd, sizeof(output_info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == S_OK, "GetOutputStreamInfo returned %#lx\n", hr);
    ok(output_info.dwFlags == flags, "got dwFlags %#lx\n", output_info.dwFlags);
    ok(output_info.cbSize == 1920 * 1088 * 2, "got cbSize %#lx\n", output_info.cbSize);
    ok(output_info.cbAlignment == 0, "got cbAlignment %#lx\n", output_info.cbAlignment);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetInputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("in %lu", i);
        ok(hr == S_OK, "GetInputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, default_inputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetInputAvailableType returned %#lx\n", hr);
    ok(i == 2 || broken(i == 1) /* Win7 */, "%lu input media types\n", i);

    /* check required input media type attributes */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == E_INVALIDARG, "SetInputType returned %#lx.\n", hr);
    init_media_type(media_type, input_type_desc, 1);
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    todo_wine
    ok(hr == MF_E_INVALIDMEDIATYPE, "SetInputType returned %#lx.\n", hr);
    init_media_type(media_type, input_type_desc, 2);
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 1, "Release returned %lu\n", ret);

    flags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE;
    memset(&output_info, 0xcd, sizeof(output_info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == S_OK, "GetOutputStreamInfo returned %#lx\n", hr);
    ok(output_info.dwFlags == flags, "got dwFlags %#lx\n", output_info.dwFlags);
    todo_wine
    ok(output_info.cbSize == 1920 * 1080 * 2 || broken(output_info.cbSize == 1920 * 1088 * 2) /* Win7 */,
            "got cbSize %#lx\n", output_info.cbSize);
    ok(output_info.cbAlignment == 0, "got cbAlignment %#lx\n", output_info.cbAlignment);

    /* output types can now be enumerated (though they are actually the same for all input types) */

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, default_outputs[i], -1);
        hr = IMFMediaType_GetItem(media_type, &MF_MT_VIDEO_ROTATION, NULL);
        is_win7 = broken(FAILED(hr));
        check_media_type(media_type, is_win7 ? default_outputs_win7[i] : default_outputs_extra[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == 5, "%lu output media types\n", i);

    /* check required output media type attributes */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    todo_wine
    ok(hr == E_INVALIDARG, "SetOutputType returned %#lx.\n", hr);
    init_media_type(media_type, minimal_output_type_desc, 1);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    todo_wine
    ok(hr == MF_E_INVALIDMEDIATYPE, "SetOutputType returned %#lx.\n", hr);
    init_media_type(media_type, minimal_output_type_desc, 2);
    for (i = 2; i < ARRAY_SIZE(minimal_output_type_desc) - 1; ++i)
    {
        hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
        todo_wine
        ok(hr == MF_E_ATTRIBUTENOTFOUND, "SetOutputType returned %#lx.\n", hr);
        init_media_type(media_type, minimal_output_type_desc, i + 1);
    }
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == (is_win7 ? MF_E_INVALIDMEDIATYPE : S_OK), "SetOutputType returned %#lx.\n", hr);
    init_media_type(media_type, is_win7 ? output_type_desc_win7 : output_type_desc, -1);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 1, "Release returned %lu\n", ret);

    hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
    ok(hr == S_OK, "GetOutputCurrentType returned %#lx\n", hr);
    check_media_type(media_type, is_win7 ? output_type_desc_win7 : output_type_desc, -1);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 0, "Release returned %lu\n", ret);

    /* check that the output media type we've selected don't change the enumeration */

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, default_outputs[i], -1);
        check_media_type(media_type, is_win7 ? default_outputs_win7[i] : default_outputs_extra[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == 5, "%lu output media types\n", i);

    flags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_INPUT_STREAM_FIXED_SAMPLE_SIZE;
    memset(&input_info, 0xcd, sizeof(input_info));
    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
    ok(hr == S_OK, "GetInputStreamInfo returned %#lx\n", hr);
    ok(input_info.hnsMaxLatency == 0, "got hnsMaxLatency %s\n", wine_dbgstr_longlong(input_info.hnsMaxLatency));
    ok(input_info.dwFlags == flags, "got dwFlags %#lx\n", input_info.dwFlags);
    ok(input_info.cbSize == 0x1000, "got cbSize %lu\n", input_info.cbSize);
    ok(input_info.cbMaxLookahead == 0, "got cbMaxLookahead %#lx\n", input_info.cbMaxLookahead);
    ok(input_info.cbAlignment == 0, "got cbAlignment %#lx\n", input_info.cbAlignment);

    flags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE;
    memset(&output_info, 0xcd, sizeof(output_info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == S_OK, "GetOutputStreamInfo returned %#lx\n", hr);
    ok(output_info.dwFlags == flags, "got dwFlags %#lx\n", output_info.dwFlags);
    todo_wine
    ok(output_info.cbSize == 1920 * 1080 * 2 || broken(output_info.cbSize == 1920 * 1088 * 2) /* Win7 */,
            "got cbSize %#lx\n", output_info.cbSize);
    ok(output_info.cbAlignment == 0, "got cbAlignment %#lx\n", output_info.cbAlignment);

    input_count = output_count = 0xdeadbeef;
    hr = IMFTransform_GetStreamCount(transform, &input_count, &output_count);
    todo_wine
    ok(hr == S_OK, "GetStreamCount returned %#lx\n", hr);
    todo_wine
    ok(input_count == 1, "got input_count %lu\n", input_count);
    todo_wine
    ok(output_count == 1, "got output_count %lu\n", output_count);
    hr = IMFTransform_GetStreamIDs(transform, 1, &input_id, 1, &output_id);
    ok(hr == E_NOTIMPL, "GetStreamIDs returned %#lx\n", hr);

    resource = FindResourceW(NULL, L"h264data.bin", (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    h264_encoded_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    h264_encoded_data_len = SizeofResource(GetModuleHandleW(NULL), resource);

    /* As output_info.dwFlags doesn't have MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES
     * IMFTransform_ProcessOutput needs a sample or returns an error */

    status = 0;
    memset(&output, 0, sizeof(output));
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == E_INVALIDARG || hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output.dwStreamID == 0, "got dwStreamID %lu\n", output.dwStreamID);
    ok(!output.pSample, "got pSample %p\n", output.pSample);
    ok(output.dwStatus == 0, "got dwStatus %#lx\n", output.dwStatus);
    ok(!output.pEvents, "got pEvents %p\n", output.pEvents);
    ok(status == 0, "got status %#lx\n", status);

    i = 0;
    sample = next_h264_sample(&h264_encoded_data, &h264_encoded_data_len);
    while (1)
    {
        status = 0;
        memset(&output, 0, sizeof(output));
        output.pSample = create_sample(NULL, output_info.cbSize);
        hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
        if (hr != MF_E_TRANSFORM_NEED_MORE_INPUT) break;
        ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
        ok(output.dwStreamID == 0, "got dwStreamID %lu\n", output.dwStreamID);
        ok(!!output.pSample, "got pSample %p\n", output.pSample);
        ok(output.dwStatus == 0, "got dwStatus %#lx\n", output.dwStatus);
        ok(!output.pEvents, "got pEvents %p\n", output.pEvents);
        ok(status == 0, "got status %#lx\n", status);
        hr = IMFSample_GetTotalLength(output.pSample, &length);
        ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
        ok(length == 0, "got length %lu\n", length);
        ret = IMFSample_Release(output.pSample);
        ok(ret == 0, "Release returned %lu\n", ret);

        hr = IMFTransform_ProcessInput(transform, 0, sample, 0);
        ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
        ret = IMFSample_Release(sample);
        ok(ret <= 1, "Release returned %lu\n", ret);
        sample = next_h264_sample(&h264_encoded_data, &h264_encoded_data_len);

        hr = IMFTransform_ProcessInput(transform, 0, sample, 0);
        ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
        ret = IMFSample_Release(sample);
        ok(ret <= 1, "Release returned %lu\n", ret);
        sample = next_h264_sample(&h264_encoded_data, &h264_encoded_data_len);
        i++;

        hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, 0);
        ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);
    }
    todo_wine
    ok(i == 2, "got %lu iterations\n", i);
    todo_wine
    ok(h264_encoded_data_len == 1180, "got h264_encoded_data_len %lu\n", h264_encoded_data_len);
    ok(hr == MF_E_TRANSFORM_STREAM_CHANGE, "ProcessOutput returned %#lx\n", hr);
    ok(output.dwStreamID == 0, "got dwStreamID %lu\n", output.dwStreamID);
    ok(!!output.pSample, "got pSample %p\n", output.pSample);
    ok(output.dwStatus == MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE,
            "got dwStatus %#lx\n", output.dwStatus);
    ok(!output.pEvents, "got pEvents %p\n", output.pEvents);
    ok(status == MFT_PROCESS_OUTPUT_STATUS_NEW_STREAMS,
            "got status %#lx\n", status);
    hr = IMFSample_GetTotalLength(output.pSample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    ret = IMFSample_Release(output.pSample);
    ok(ret == 0, "Release returned %lu\n", ret);

    flags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE;
    memset(&output_info, 0xcd, sizeof(output_info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == S_OK, "GetOutputStreamInfo returned %#lx\n", hr);
    ok(output_info.dwFlags == flags, "got dwFlags %#lx\n", output_info.dwFlags);
    ok(output_info.cbSize == actual_width * actual_height * 2, "got cbSize %#lx\n", output_info.cbSize);
    ok(output_info.cbAlignment == 0, "got cbAlignment %#lx\n", output_info.cbAlignment);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, actual_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == 5, "%lu output media types\n", i);

    /* current output type is still the one we selected */
    hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
    ok(hr == S_OK, "GetOutputCurrentType returned %#lx\n", hr);
    check_media_type(media_type, is_win7 ? output_type_desc_win7 : output_type_desc, -1);
    hr = IMFMediaType_GetItemType(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "GetItemType returned %#lx\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 0, "Release returned %lu\n", ret);

    /* and generate a new one as well in a temporary directory */
    GetTempPathW(ARRAY_SIZE(output_path), output_path);
    lstrcatW(output_path, L"nv12frame.bin");
    output_file = CreateFileW(output_path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(output_file != INVALID_HANDLE_VALUE, "CreateFileW failed, error %lu\n", GetLastError());

    resource = FindResourceW(NULL, L"nv12frame.bin", (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    nv12_frame_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    nv12_frame_len = SizeofResource(GetModuleHandleW(NULL), resource);
    ok(nv12_frame_len == actual_width * actual_height * 3 / 2, "got frame length %lu\n", nv12_frame_len);

    status = 0;
    memset(&output, 0, sizeof(output));
    output.pSample = create_sample(NULL, actual_width * actual_height * 2);
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
    ok(output.dwStreamID == 0, "got dwStreamID %lu\n", output.dwStreamID);
    ok(!!output.pSample, "got pSample %p\n", output.pSample);
    ok(output.dwStatus == 0, "got dwStatus %#lx\n", output.dwStatus);
    ok(!output.pEvents, "got pEvents %p\n", output.pEvents);
    ok(status == 0, "got status %#lx\n", status);

    hr = IMFSample_GetUINT32(sample, &MFSampleExtension_CleanPoint, &value);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "GetUINT32 MFSampleExtension_CleanPoint returned %#lx\n", hr);

    count = 0xdeadbeef;
    hr = IMFSample_GetBufferCount(output.pSample, &count);
    ok(hr == S_OK, "GetBufferCount returned %#lx\n", hr);
    ok(count == 1, "got count %#lx\n", count);

    flags = 0xdeadbeef;
    hr = IMFSample_GetSampleFlags(output.pSample, &flags);
    ok(hr == S_OK, "GetSampleFlags returned %#lx\n", hr);
    ok(flags == 0, "got flags %#lx\n", flags);

    time = 0xdeadbeef;
    hr = IMFSample_GetSampleTime(output.pSample, &time);
    ok(hr == S_OK, "GetSampleTime returned %#lx\n", hr);
    ok(time == 0, "got time %I64d\n", time);

    /* doesn't matter what frame rate we've selected, duration is defined by the stream */
    duration = 0xdeadbeef;
    hr = IMFSample_GetSampleDuration(output.pSample, &duration);
    ok(hr == S_OK, "GetSampleDuration returned %#lx\n", hr);
    ok(duration - 333666 <= 2, "got duration %I64d\n", duration);

    /* Win8 and before pad the data with garbage instead of original
     * buffer data, make sure it's consistent.  */
    hr = IMFSample_ConvertToContiguousBuffer(output.pSample, &media_buffer);
    ok(hr == S_OK, "ConvertToContiguousBuffer returned %#lx\n", hr);
    hr = IMFMediaBuffer_Lock(media_buffer, &data, NULL, &length);
    ok(hr == S_OK, "Lock returned %#lx\n", hr);
    ok(length == nv12_frame_len, "got length %lu\n", length);

    for (i = 0; i < actual_aperture.Area.cy; ++i)
    {
        memset(data + actual_width * i + actual_aperture.Area.cx, 0xcd, actual_width - actual_aperture.Area.cx);
        memset(data + actual_width * (actual_height + i) + actual_aperture.Area.cx, 0xcd, actual_width - actual_aperture.Area.cx);
    }
    memset(data + actual_width * actual_aperture.Area.cy, 0xcd, (actual_height - actual_aperture.Area.cy) * actual_width);
    memset(data + actual_width * (actual_height + actual_aperture.Area.cy / 2), 0xcd, (actual_height - actual_aperture.Area.cy) / 2 * actual_width);

    hr = IMFMediaBuffer_Unlock(media_buffer);
    ok(hr == S_OK, "Unlock returned %#lx\n", hr);
    IMFMediaBuffer_Release(media_buffer);

    check_sample(output.pSample, nv12_frame_data, output_file);

    ret = IMFSample_Release(output.pSample);
    ok(ret == 0, "Release returned %lu\n", ret);

    trace("created %s\n", debugstr_w(output_path));
    CloseHandle(output_file);

    /* we can change it, but only with the correct frame size */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    init_media_type(media_type, is_win7 ? output_type_desc_win7 : output_type_desc, -1);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == MF_E_INVALIDMEDIATYPE, "SetOutputType returned %#lx.\n", hr);
    init_media_type(media_type, is_win7 ? new_output_type_desc_win7 : new_output_type_desc, -1);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 1, "Release returned %lu\n", ret);

    hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
    ok(hr == S_OK, "GetOutputCurrentType returned %#lx\n", hr);
    check_media_type(media_type, is_win7 ? new_output_type_desc_win7 : new_output_type_desc, -1);
    hr = IMFMediaType_GetItemType(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "GetItemType returned %#lx\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 0, "Release returned %lu\n", ret);

    status = 0;
    memset(&output, 0, sizeof(output));
    output.pSample = create_sample(NULL, actual_width * actual_height * 2);
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    todo_wine
    ok(hr == MF_E_TRANSFORM_STREAM_CHANGE, "ProcessOutput returned %#lx\n", hr);

    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
    {
        hr = IMFTransform_ProcessInput(transform, 0, sample, 0);
        ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
        ret = IMFSample_Release(sample);
        ok(ret <= 1, "Release returned %lu\n", ret);
        sample = next_h264_sample(&h264_encoded_data, &h264_encoded_data_len);
        hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
        todo_wine
        ok(hr == MF_E_TRANSFORM_STREAM_CHANGE, "ProcessOutput returned %#lx\n", hr);
    }

    ok(output.dwStreamID == 0, "got dwStreamID %lu\n", output.dwStreamID);
    ok(!!output.pSample, "got pSample %p\n", output.pSample);
    todo_wine
    ok(output.dwStatus == MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE, "got dwStatus %#lx\n", output.dwStatus);
    ok(!output.pEvents, "got pEvents %p\n", output.pEvents);
    todo_wine
    ok(status == MFT_PROCESS_OUTPUT_STATUS_NEW_STREAMS, "got status %#lx\n", status);
    hr = IMFSample_GetTotalLength(output.pSample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    ret = IMFSample_Release(output.pSample);
    ok(ret == 0, "Release returned %lu\n", ret);

    GetTempPathW(ARRAY_SIZE(output_path), output_path);
    lstrcatW(output_path, L"i420frame.bin");
    output_file = CreateFileW(output_path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(output_file != INVALID_HANDLE_VALUE, "CreateFileW failed, error %lu\n", GetLastError());

    resource = FindResourceW(NULL, L"i420frame.bin", (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    i420_frame_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    i420_frame_len = SizeofResource(GetModuleHandleW(NULL), resource);
    ok(i420_frame_len == actual_width * actual_height * 3 / 2, "got frame length %lu\n", i420_frame_len);

    status = 0;
    memset(&output, 0, sizeof(output));
    output.pSample = create_sample(NULL, actual_width * actual_height * 2);
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    todo_wine
    ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
    ok(output.dwStreamID == 0, "got dwStreamID %lu\n", output.dwStreamID);
    ok(!!output.pSample, "got pSample %p\n", output.pSample);
    ok(output.dwStatus == 0, "got dwStatus %#lx\n", output.dwStatus);
    ok(!output.pEvents, "got pEvents %p\n", output.pEvents);
    ok(status == 0, "got status %#lx\n", status);
    if (hr != S_OK) goto skip_i420_tests;

    hr = IMFSample_GetSampleTime(output.pSample, &time);
    ok(hr == S_OK, "GetSampleTime returned %#lx\n", hr);
    ok(time - 333666 <= 2, "got time %I64d\n", time);

    duration = 0xdeadbeef;
    hr = IMFSample_GetSampleDuration(output.pSample, &duration);
    ok(hr == S_OK, "GetSampleDuration returned %#lx\n", hr);
    ok(duration - 333666 <= 2, "got duration %I64d\n", duration);

    /* Win8 and before pad the data with garbage instead of original
     * buffer data, make sure it's consistent.  */
    hr = IMFSample_ConvertToContiguousBuffer(output.pSample, &media_buffer);
    ok(hr == S_OK, "ConvertToContiguousBuffer returned %#lx\n", hr);
    hr = IMFMediaBuffer_Lock(media_buffer, &data, NULL, &length);
    ok(hr == S_OK, "Lock returned %#lx\n", hr);
    ok(length == i420_frame_len, "got length %lu\n", length);

    for (i = 0; i < actual_aperture.Area.cy; ++i)
    {
        memset(data + actual_width * i + actual_aperture.Area.cx, 0xcd, actual_width - actual_aperture.Area.cx);
        memset(data + actual_width * actual_height + actual_width / 2 * i + actual_aperture.Area.cx / 2, 0xcd,
                actual_width / 2 - actual_aperture.Area.cx / 2);
        memset(data + actual_width * actual_height + actual_width / 2 * (actual_height / 2 + i) + actual_aperture.Area.cx / 2, 0xcd,
                actual_width / 2 - actual_aperture.Area.cx / 2);
    }
    memset(data + actual_width * actual_aperture.Area.cy, 0xcd, (actual_height - actual_aperture.Area.cy) * actual_width);
    memset(data + actual_width * actual_height + actual_width / 2 * actual_aperture.Area.cy / 2, 0xcd,
            (actual_height - actual_aperture.Area.cy) / 2 * actual_width / 2);
    memset(data + actual_width * actual_height + actual_width / 2 * (actual_height / 2 + actual_aperture.Area.cy / 2), 0xcd,
            (actual_height - actual_aperture.Area.cy) / 2 * actual_width / 2);

    hr = IMFMediaBuffer_Unlock(media_buffer);
    ok(hr == S_OK, "Unlock returned %#lx\n", hr);
    IMFMediaBuffer_Release(media_buffer);

    check_sample(output.pSample, i420_frame_data, output_file);

skip_i420_tests:
    ret = IMFSample_Release(output.pSample);
    ok(ret == 0, "Release returned %lu\n", ret);

    trace("created %s\n", debugstr_w(output_path));
    CloseHandle(output_file);

    status = 0;
    memset(&output, 0, sizeof(output));
    output.pSample = create_sample(NULL, actual_width * actual_height * 2);
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output.dwStreamID == 0, "got dwStreamID %lu\n", output.dwStreamID);
    ok(!!output.pSample, "got pSample %p\n", output.pSample);
    ok(output.dwStatus == 0, "got dwStatus %#lx\n", output.dwStatus);
    ok(!output.pEvents, "got pEvents %p\n", output.pEvents);
    ok(status == 0, "got status %#lx\n", status);
    ret = IMFSample_Release(output.pSample);
    ok(ret == 0, "Release returned %lu\n", ret);

    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);
    ret = IMFSample_Release(sample);
    ok(ret == 0, "Release returned %lu\n", ret);

failed:
    CoUninitialize();
}

static void test_audio_convert(void)
{
    const GUID transform_inputs[2] =
    {
        MFAudioFormat_PCM,
        MFAudioFormat_Float,
    };
    const GUID transform_outputs[2] =
    {
        MFAudioFormat_PCM,
        MFAudioFormat_Float,
    };

    static const media_type_desc expect_available_inputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
    };
    static const media_type_desc expect_available_outputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
            ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
            ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000),
            ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 384000),
            ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 8),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        },
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
        },
    };

    static const struct attribute_desc input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 176400),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 8),
        {0},
    };
    const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 176400),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
        {0},
    };

    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Audio, MFAudioFormat_PCM};
    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Audio, MFAudioFormat_Float};
    static const ULONG audioconv_block_size = 0x4000;
    ULONG audio_data_len, audioconv_data_len;
    const BYTE *audio_data, *audioconv_data;
    MFT_OUTPUT_STREAM_INFO output_info;
    MFT_INPUT_STREAM_INFO input_info;
    MFT_OUTPUT_DATA_BUFFER output;
    WCHAR output_path[MAX_PATH];
    IMFMediaType *media_type;
    LONGLONG time, duration;
    IMFTransform *transform;
    DWORD length, status;
    HANDLE output_file;
    IMFSample *sample;
    HRSRC resource;
    GUID class_id;
    ULONG i, ret;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    if (!create_transform(MFT_CATEGORY_AUDIO_EFFECT, &input_type, &output_type, L"Resampler MFT", &MFMediaType_Audio,
            transform_inputs, ARRAY_SIZE(transform_inputs), transform_outputs, ARRAY_SIZE(transform_outputs),
            &transform, &CLSID_CResamplerMediaObject, &class_id))
        goto failed;

    check_dmo(&class_id, L"Resampler DMO", &MEDIATYPE_Audio, transform_inputs, ARRAY_SIZE(transform_inputs),
            transform_outputs, ARRAY_SIZE(transform_outputs));

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_IMediaObject, TRUE);
    check_interface(transform, &IID_IPropertyStore, TRUE);
    check_interface(transform, &IID_IPropertyBag, TRUE);
    /* check_interface(transform, &IID_IWMResamplerProps, TRUE); */

    /* check default media types */

    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetInputStreamInfo returned %#lx\n", hr);
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetOutputStreamInfo returned %#lx\n", hr);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == 4, "%lu output media types\n", i);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetInputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("in %lu", i);
        ok(hr == S_OK, "GetInputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_inputs[i], -1);
        hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
        ok(hr == MF_E_INVALIDMEDIATYPE, "SetInputType returned %#lx.\n", hr);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetInputAvailableType returned %#lx\n", hr);
    ok(i == 2, "%lu input media types\n", i);

    /* setting output media type first doesn't work */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    init_media_type(media_type, output_type_desc, -1);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "SetOutputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 0, "Release returned %lu\n", ret);

    /* check required input media type attributes */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "SetInputType returned %#lx.\n", hr);
    init_media_type(media_type, input_type_desc, 1);
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "SetInputType returned %#lx.\n", hr);
    init_media_type(media_type, input_type_desc, 2);
    for (i = 2; i < ARRAY_SIZE(input_type_desc) - 1; ++i)
    {
        hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
        ok(hr == MF_E_INVALIDMEDIATYPE, "SetInputType returned %#lx.\n", hr);
        init_media_type(media_type, input_type_desc, i + 1);
    }
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 0, "Release returned %lu\n", ret);

    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetInputStreamInfo returned %#lx\n", hr);
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetOutputStreamInfo returned %#lx\n", hr);

    /* check new output media types */

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == 4, "%lu output media types\n", i);

    /* check required output media type attributes */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "SetOutputType returned %#lx.\n", hr);
    init_media_type(media_type, output_type_desc, 1);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "SetOutputType returned %#lx.\n", hr);
    init_media_type(media_type, output_type_desc, 2);
    for (i = 2; i < ARRAY_SIZE(output_type_desc) - 1; ++i)
    {
        hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
        ok(hr == MF_E_INVALIDMEDIATYPE, "SetOutputType returned %#lx.\n", hr);
        init_media_type(media_type, output_type_desc, i + 1);
    }
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 0, "Release returned %lu\n", ret);

    memset(&input_info, 0xcd, sizeof(input_info));
    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
    ok(hr == S_OK, "GetInputStreamInfo returned %#lx\n", hr);
    ok(input_info.hnsMaxLatency == 0, "got hnsMaxLatency %s\n", wine_dbgstr_longlong(input_info.hnsMaxLatency));
    ok(input_info.dwFlags == 0, "got dwFlags %#lx\n", input_info.dwFlags);
    ok(input_info.cbSize == 8, "got cbSize %lu\n", input_info.cbSize);
    ok(input_info.cbMaxLookahead == 0, "got cbMaxLookahead %#lx\n", input_info.cbMaxLookahead);
    ok(input_info.cbAlignment == 1, "got cbAlignment %#lx\n", input_info.cbAlignment);

    memset(&output_info, 0xcd, sizeof(output_info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == S_OK, "GetOutputStreamInfo returned %#lx\n", hr);
    ok(output_info.dwFlags == 0, "got dwFlags %#lx\n", output_info.dwFlags);
    ok(output_info.cbSize == 4, "got cbSize %#lx\n", output_info.cbSize);
    ok(output_info.cbAlignment == 1, "got cbAlignment %#lx\n", output_info.cbAlignment);

    resource = FindResourceW(NULL, L"audiodata.bin", (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    audio_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    audio_data_len = SizeofResource(GetModuleHandleW(NULL), resource);
    ok(audio_data_len == 179928, "got length %lu\n", audio_data_len);

    sample = create_sample(audio_data, audio_data_len);
    hr = IMFSample_SetSampleTime(sample, 0);
    ok(hr == S_OK, "SetSampleTime returned %#lx\n", hr);
    hr = IMFSample_SetSampleDuration(sample, 10000000);
    ok(hr == S_OK, "SetSampleDuration returned %#lx\n", hr);
    hr = IMFTransform_ProcessInput(transform, 0, sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, 0);
    ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);
    hr = IMFTransform_ProcessInput(transform, 0, sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);
    ret = IMFSample_Release(sample);
    ok(ret <= 1, "Release returned %ld\n", ret);

    status = 0xdeadbeef;
    sample = create_sample(NULL, audioconv_block_size);
    memset(&output, 0, sizeof(output));
    output.pSample = sample;

    resource = FindResourceW(NULL, L"audioconvdata.bin", (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    audioconv_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    audioconv_data_len = SizeofResource(GetModuleHandleW(NULL), resource);
    ok(audioconv_data_len == 179924, "got length %lu\n", audioconv_data_len);

    /* and generate a new one as well in a temporary directory */
    GetTempPathW(ARRAY_SIZE(output_path), output_path);
    lstrcatW(output_path, L"audioconvdata.bin");
    output_file = CreateFileW(output_path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(output_file != INVALID_HANDLE_VALUE, "CreateFileW failed, error %lu\n", GetLastError());

    i = 0;
    while (SUCCEEDED(hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status)))
    {
        winetest_push_context("%lu", i);
        ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
        ok(output.pSample == sample, "got pSample %p\n", output.pSample);
        ok(output.dwStatus == MFT_OUTPUT_DATA_BUFFER_INCOMPLETE || output.dwStatus == 0 ||
                broken(output.dwStatus == (MFT_OUTPUT_DATA_BUFFER_INCOMPLETE|6) || output.dwStatus == 6) /* win7 */,
                "got dwStatus %#lx\n", output.dwStatus);
        ok(status == 0, "got status %#lx\n", status);
        if (!(output.dwStatus & MFT_OUTPUT_DATA_BUFFER_INCOMPLETE))
        {
            winetest_pop_context();
            break;
        }

        hr = IMFSample_GetSampleTime(sample, &time);
        ok(hr == S_OK, "GetSampleTime returned %#lx\n", hr);
        ok(time == i * 928798, "got time %I64d\n", time);
        hr = IMFSample_GetSampleDuration(sample, &duration);
        ok(hr == S_OK, "GetSampleDuration returned %#lx\n", hr);
        ok(duration == 928798, "got duration %I64d\n", duration);
        hr = IMFSample_GetTotalLength(sample, &length);
        ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
        ok(length == audioconv_block_size, "got length %lu\n", length);
        ok(audioconv_data_len > audioconv_block_size, "got remaining length %lu\n", audioconv_data_len);
        check_sample_pcm16(sample, audioconv_data, output_file, FALSE);
        audioconv_data_len -= audioconv_block_size;
        audioconv_data += audioconv_block_size;

        winetest_pop_context();
        i++;
    }

    hr = IMFSample_GetSampleTime(sample, &time);
    ok(hr == S_OK, "GetSampleTime returned %#lx\n", hr);
    ok(time == i * 928798, "got time %I64d\n", time);
    hr = IMFSample_GetSampleDuration(sample, &duration);
    ok(hr == S_OK, "GetSampleDuration returned %#lx\n", hr);
    todo_wine
    ok(duration == 897506, "got duration %I64d\n", duration);
    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    todo_wine
    ok(length == 15832, "got length %lu\n", length);
    ok(audioconv_data_len == 16084, "got remaining length %lu\n", audioconv_data_len);
    check_sample_pcm16(sample, audioconv_data, output_file, FALSE);
    audioconv_data_len -= length;
    audioconv_data += length;

    memset(&output, 0, sizeof(output));
    output.pSample = sample;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    todo_wine
    ok(hr == S_OK || broken(hr == MF_E_TRANSFORM_NEED_MORE_INPUT) /* win7 */, "ProcessOutput returned %#lx\n", hr);
    ok(output.pSample == sample, "got pSample %p\n", output.pSample);
    todo_wine
    ok(output.dwStatus == MFT_OUTPUT_DATA_BUFFER_INCOMPLETE || broken(output.dwStatus == 0) /* win7 */,
            "got dwStatus %#lx\n", output.dwStatus);
    ok(status == 0, "got status %#lx\n", status);

    if (hr == S_OK)
    {
        hr = IMFSample_GetSampleTime(sample, &time);
        ok(hr == S_OK, "GetSampleTime returned %#lx\n", hr);
        todo_wine
        ok(time == 10185486, "got time %I64d\n", time);
        hr = IMFSample_GetSampleDuration(sample, &duration);
        ok(hr == S_OK, "GetSampleDuration returned %#lx\n", hr);
        todo_wine
        ok(duration == 14286, "got duration %I64d\n", duration);
        hr = IMFSample_GetTotalLength(sample, &length);
        ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
        todo_wine
        ok(length == audioconv_data_len, "got length %lu\n", length);
        if (length == audioconv_data_len)
            check_sample_pcm16(sample, audioconv_data, output_file, FALSE);
    }

    trace("created %s\n", debugstr_w(output_path));
    CloseHandle(output_file);

    ret = IMFSample_Release(sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    status = 0xdeadbeef;
    sample = create_sample(NULL, audioconv_block_size);
    memset(&output, 0, sizeof(output));
    output.pSample = sample;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output.pSample == sample, "got pSample %p\n", output.pSample);
    ok(output.dwStatus == 0, "got dwStatus %#lx\n", output.dwStatus);
    ok(status == 0, "got status %#lx\n", status);
    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    ret = IMFSample_Release(sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

failed:
    CoUninitialize();
}

static void test_color_convert(void)
{
    const GUID transform_inputs[20] =
    {
        MFVideoFormat_YV12,
        MFVideoFormat_YUY2,
        MFVideoFormat_UYVY,
        MFVideoFormat_AYUV,
        MFVideoFormat_NV12,
        DMOVideoFormat_RGB32,
        DMOVideoFormat_RGB565,
        MFVideoFormat_I420,
        MFVideoFormat_IYUV,
        MFVideoFormat_YVYU,
        DMOVideoFormat_RGB24,
        DMOVideoFormat_RGB555,
        DMOVideoFormat_RGB8,
        MEDIASUBTYPE_V216,
        MEDIASUBTYPE_V410,
        MFVideoFormat_NV11,
        MFVideoFormat_Y41P,
        MFVideoFormat_Y41T,
        MFVideoFormat_Y42T,
        MFVideoFormat_YVU9,
    };
    const GUID transform_outputs[16] =
    {
        MFVideoFormat_YV12,
        MFVideoFormat_YUY2,
        MFVideoFormat_UYVY,
        MFVideoFormat_AYUV,
        MFVideoFormat_NV12,
        DMOVideoFormat_RGB32,
        DMOVideoFormat_RGB565,
        MFVideoFormat_I420,
        MFVideoFormat_IYUV,
        MFVideoFormat_YVYU,
        DMOVideoFormat_RGB24,
        DMOVideoFormat_RGB555,
        DMOVideoFormat_RGB8,
        MEDIASUBTYPE_V216,
        MEDIASUBTYPE_V410,
        MFVideoFormat_NV11,
    };
    const GUID dmo_inputs[20] =
    {
        MEDIASUBTYPE_YV12,
        MEDIASUBTYPE_YUY2,
        MEDIASUBTYPE_UYVY,
        MEDIASUBTYPE_AYUV,
        MEDIASUBTYPE_NV12,
        MEDIASUBTYPE_RGB32,
        MEDIASUBTYPE_RGB565,
        MEDIASUBTYPE_I420,
        MEDIASUBTYPE_IYUV,
        MEDIASUBTYPE_YVYU,
        MEDIASUBTYPE_RGB24,
        MEDIASUBTYPE_RGB555,
        MEDIASUBTYPE_RGB8,
        MEDIASUBTYPE_V216,
        MEDIASUBTYPE_V410,
        MEDIASUBTYPE_NV11,
        MEDIASUBTYPE_Y41P,
        MEDIASUBTYPE_Y41T,
        MEDIASUBTYPE_Y42T,
        MEDIASUBTYPE_YVU9,
    };
    const GUID dmo_outputs[16] =
    {
        MEDIASUBTYPE_YV12,
        MEDIASUBTYPE_YUY2,
        MEDIASUBTYPE_UYVY,
        MEDIASUBTYPE_AYUV,
        MEDIASUBTYPE_NV12,
        MEDIASUBTYPE_RGB32,
        MEDIASUBTYPE_RGB565,
        MEDIASUBTYPE_I420,
        MEDIASUBTYPE_IYUV,
        MEDIASUBTYPE_YVYU,
        MEDIASUBTYPE_RGB24,
        MEDIASUBTYPE_RGB555,
        MEDIASUBTYPE_RGB8,
        MEDIASUBTYPE_V216,
        MEDIASUBTYPE_V410,
        MEDIASUBTYPE_NV11,
    };

    static const media_type_desc expect_available_inputs[20] =
    {
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YV12), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_UYVY), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_AYUV), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB565), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_IYUV), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YVYU), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB555), },
        { ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_RGB8), },
        { ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_V216), },
        { ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_V410), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV11), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_Y41P), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_Y41T), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_Y42T), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YVU9), },
    };
    static const media_type_desc expect_available_outputs[16] =
    {
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YV12), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_UYVY), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_AYUV), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB565), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_IYUV), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YVYU), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB555), },
        { ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_RGB8), },
        { ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_V216), },
        { ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_V410), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV11), },
    };
    static const media_type_desc expect_available_common =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
    };

    static const MFVideoArea actual_aperture = {.Area={82,84}};
    static const DWORD actual_width = 96, actual_height = 96;
    const struct attribute_desc input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        {0},
    };
    const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        {0},
    };

    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Video, MFVideoFormat_NV12};
    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Video, MFVideoFormat_I420};
    ULONG nv12frame_data_len, rgb32_data_len;
    const BYTE *nv12frame_data, *rgb32_data;
    MFT_OUTPUT_STREAM_INFO output_info;
    MFT_INPUT_STREAM_INFO input_info;
    MFT_OUTPUT_DATA_BUFFER output;
    WCHAR output_path[MAX_PATH];
    IMFMediaType *media_type;
    LONGLONG time, duration;
    IMFTransform *transform;
    DWORD length, status;
    HANDLE output_file;
    IMFSample *sample;
    HRSRC resource;
    GUID class_id;
    ULONG i, ret;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    if (!create_transform(MFT_CATEGORY_VIDEO_EFFECT, &input_type, &output_type, L"Color Converter MFT", &MFMediaType_Video,
            transform_inputs, ARRAY_SIZE(transform_inputs), transform_outputs, ARRAY_SIZE(transform_outputs),
            &transform, &CLSID_CColorConvertDMO, &class_id))
        goto failed;

    check_dmo(&CLSID_CColorConvertDMO, L"Color Converter DMO", &MEDIATYPE_Video, dmo_inputs, ARRAY_SIZE(dmo_inputs),
            dmo_outputs, ARRAY_SIZE(dmo_outputs));

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_IMediaObject, TRUE);
    check_interface(transform, &IID_IPropertyStore, TRUE);
    todo_wine
    check_interface(transform, &IID_IMFRealTimeClient, TRUE);
    /* check_interface(transform, &IID_IWMColorConvProps, TRUE); */

    /* check default media types */

    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetInputStreamInfo returned %#lx\n", hr);
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetOutputStreamInfo returned %#lx\n", hr);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_common, -1);
        check_media_type(media_type, expect_available_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == 16, "%lu output media types\n", i);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetInputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("in %lu", i);
        ok(hr == S_OK, "GetInputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_common, -1);
        check_media_type(media_type, expect_available_inputs[i], -1);
        hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
        if (i == 12)
        {
            todo_wine
            ok(hr == MF_E_INVALIDMEDIATYPE, "SetInputType returned %#lx.\n", hr);
        }
        else
            ok(hr == E_INVALIDARG, "SetInputType returned %#lx.\n", hr);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetInputAvailableType returned %#lx\n", hr);
    ok(i == 20, "%lu input media types\n", i);

    /* check required output media type attributes */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "SetOutputType returned %#lx.\n", hr);
    init_media_type(media_type, output_type_desc, 1);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "SetOutputType returned %#lx.\n", hr);
    init_media_type(media_type, output_type_desc, 2);
    for (i = 2; i < ARRAY_SIZE(output_type_desc) - 1; ++i)
    {
        hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
        ok(hr == E_INVALIDARG, "SetOutputType returned %#lx.\n", hr);
        init_media_type(media_type, output_type_desc, i + 1);
    }
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 0, "Release returned %lu\n", ret);

    /* check required input media type attributes */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "SetInputType returned %#lx.\n", hr);
    init_media_type(media_type, input_type_desc, 1);
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "SetInputType returned %#lx.\n", hr);
    init_media_type(media_type, input_type_desc, 2);
    for (i = 2; i < ARRAY_SIZE(input_type_desc) - 1; ++i)
    {
        hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
        ok(hr == E_INVALIDARG, "SetInputType returned %#lx.\n", hr);
        init_media_type(media_type, input_type_desc, i + 1);
    }
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 0, "Release returned %lu\n", ret);

    memset(&input_info, 0xcd, sizeof(input_info));
    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
    ok(hr == S_OK, "GetInputStreamInfo returned %#lx\n", hr);
    ok(input_info.hnsMaxLatency == 0, "got hnsMaxLatency %s\n", wine_dbgstr_longlong(input_info.hnsMaxLatency));
    ok(input_info.dwFlags == 0, "got dwFlags %#lx\n", input_info.dwFlags);
    ok(input_info.cbSize == actual_width * actual_height * 3 / 2, "got cbSize %#lx\n", input_info.cbSize);
    ok(input_info.cbMaxLookahead == 0, "got cbMaxLookahead %#lx\n", input_info.cbMaxLookahead);
    ok(input_info.cbAlignment == 1, "got cbAlignment %#lx\n", input_info.cbAlignment);

    memset(&output_info, 0xcd, sizeof(output_info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == S_OK, "GetOutputStreamInfo returned %#lx\n", hr);
    ok(output_info.dwFlags == 0, "got dwFlags %#lx\n", output_info.dwFlags);
    ok(output_info.cbSize == actual_width * actual_height * 4, "got cbSize %#lx\n", output_info.cbSize);
    ok(output_info.cbAlignment == 1, "got cbAlignment %#lx\n", output_info.cbAlignment);

    resource = FindResourceW(NULL, L"nv12frame.bin", (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    nv12frame_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    nv12frame_data_len = SizeofResource(GetModuleHandleW(NULL), resource);
    ok(nv12frame_data_len == 13824, "got length %lu\n", nv12frame_data_len);

    sample = create_sample(nv12frame_data, nv12frame_data_len);
    hr = IMFSample_SetSampleTime(sample, 0);
    ok(hr == S_OK, "SetSampleTime returned %#lx\n", hr);
    hr = IMFSample_SetSampleDuration(sample, 10000000);
    ok(hr == S_OK, "SetSampleDuration returned %#lx\n", hr);
    hr = IMFTransform_ProcessInput(transform, 0, sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
    hr = IMFTransform_ProcessInput(transform, 0, sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);
    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, 0);
    ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);
    ret = IMFSample_Release(sample);
    ok(ret <= 1, "Release returned %ld\n", ret);

    resource = FindResourceW(NULL, L"rgb32frame.bin", (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    rgb32_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    rgb32_data_len = SizeofResource(GetModuleHandleW(NULL), resource);
    ok(rgb32_data_len == output_info.cbSize, "got length %lu\n", rgb32_data_len);

    /* and generate a new one as well in a temporary directory */
    GetTempPathW(ARRAY_SIZE(output_path), output_path);
    lstrcatW(output_path, L"rgb32frame.bin");
    output_file = CreateFileW(output_path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(output_file != INVALID_HANDLE_VALUE, "CreateFileW failed, error %lu\n", GetLastError());

    status = 0xdeadbeef;
    sample = create_sample(NULL, output_info.cbSize);
    memset(&output, 0, sizeof(output));
    output.pSample = sample;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
    ok(output.pSample == sample, "got pSample %p\n", output.pSample);
    ok(output.dwStatus == 0 || broken(output.dwStatus == 6) /* win7 */, "got dwStatus %#lx\n", output.dwStatus);
    ok(status == 0, "got status %#lx\n", status);

    hr = IMFSample_GetSampleTime(sample, &time);
    ok(hr == S_OK, "GetSampleTime returned %#lx\n", hr);
    ok(time == 0, "got time %I64d\n", time);
    hr = IMFSample_GetSampleDuration(sample, &duration);
    ok(hr == S_OK, "GetSampleDuration returned %#lx\n", hr);
    ok(duration == 10000000, "got duration %I64d\n", duration);
    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == output_info.cbSize, "got length %lu\n", length);
    check_sample_rgb32(sample, rgb32_data, output_file);
    rgb32_data_len -= output_info.cbSize;
    rgb32_data += output_info.cbSize;

    trace("created %s\n", debugstr_w(output_path));
    CloseHandle(output_file);

    ret = IMFSample_Release(sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    status = 0xdeadbeef;
    sample = create_sample(NULL, output_info.cbSize);
    memset(&output, 0, sizeof(output));
    output.pSample = sample;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output.pSample == sample, "got pSample %p\n", output.pSample);
    ok(output.dwStatus == 0, "got dwStatus %#lx\n", output.dwStatus);
    ok(status == 0, "got status %#lx\n", status);
    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    ret = IMFSample_Release(sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %ld\n", ret);

failed:
    CoUninitialize();
}


START_TEST(mf)
{
    init_functions();

    if (is_vista())
    {
        win_skip("Skipping tests on Vista.\n");
        return;
    }

    test_video_processor();
    test_topology();
    test_topology_tee_node();
    test_topology_loader();
    test_topology_loader_evr();
    test_MFGetService();
    test_sequencer_source();
    test_media_session();
    test_media_session_rate_control();
    test_MFShutdownObject();
    test_presentation_clock();
    test_sample_grabber();
    test_sample_grabber_is_mediatype_supported();
    test_quality_manager();
    test_sar();
    test_evr();
    test_MFCreateSimpleTypeHandler();
    test_MFGetSupportedMimeTypes();
    test_MFGetSupportedSchemes();
    test_sample_copier();
    test_sample_copier_output_processing();
    test_MFGetTopoNodeCurrentType();
    test_MFRequireProtectedEnvironment();
    test_wma_encoder();
    test_wma_decoder();
    test_h264_decoder();
    test_audio_convert();
    test_color_convert();
}
