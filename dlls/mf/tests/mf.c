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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"

#include "initguid.h"
#include "ole2.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(MFVideoFormat_P208, 0x38303250, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(MFVideoFormat_ABGR32, 0x00000020, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

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

#include "wine/test.h"

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG expected_refcount, int line)
{
    ULONG refcount;
    IUnknown_AddRef(obj);
    refcount = IUnknown_Release(obj);
    ok_(__FILE__, line)(refcount == expected_refcount, "Unexpected refcount %d, expected %d.\n", refcount,
            expected_refcount);
}

static WCHAR *load_resource(const WCHAR *name)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    lstrcatW(pathW, name);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0,
                       NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %d\n",
       wine_dbgstr_w(pathW), GetLastError());

    res = FindResourceW(NULL, name, (LPCWSTR)RT_RCDATA);
    ok(res != 0, "couldn't find resource\n");
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    WriteFile(file, ptr, SizeofResource(GetModuleHandleA(NULL), res),
               &written, NULL);
    ok(written == SizeofResource(GetModuleHandleA(NULL), res),
       "couldn't write resource\n" );
    CloseHandle(file);

    return pathW;
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
    MF_TOPOLOGY_TYPE node_type;
    UINT32 count, index;
    IUnknown *object;
    WORD node_count;
    DWORD size;
    HRESULT hr;
    TOPOID id;

    hr = MFCreateTopology(NULL);
    ok(hr == E_POINTER, "got %#x\n", hr);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#x.\n", hr);
    hr = IMFTopology_GetTopologyID(topology, &id);
    ok(hr == S_OK, "Failed to get id, hr %#x.\n", hr);
    ok(id == 1, "Unexpected id.\n");

    hr = MFCreateTopology(&topology2);
    ok(hr == S_OK, "Failed to create topology, hr %#x.\n", hr);
    hr = IMFTopology_GetTopologyID(topology2, &id);
    ok(hr == S_OK, "Failed to get id, hr %#x.\n", hr);
    ok(id == 2, "Unexpected id.\n");

    IMFTopology_Release(topology);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#x.\n", hr);
    hr = IMFTopology_GetTopologyID(topology, &id);
    ok(hr == S_OK, "Failed to get id, hr %#x.\n", hr);
    ok(id == 3, "Unexpected id.\n");

    IMFTopology_Release(topology2);

    /* No attributes by default. */
    for (node_type = MF_TOPOLOGY_OUTPUT_NODE; node_type < MF_TOPOLOGY_TEE_NODE; ++node_type)
    {
        hr = MFCreateTopologyNode(node_type, &node);
        ok(hr == S_OK, "Failed to create a node for type %d, hr %#x.\n", node_type, hr);
        hr = IMFTopologyNode_GetCount(node, &count);
        ok(hr == S_OK, "Failed to get attribute count, hr %#x.\n", hr);
        ok(!count, "Unexpected attribute count %u.\n", count);
        IMFTopologyNode_Release(node);
    }

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetTopoNodeID(node, &id);
    ok(hr == S_OK, "Failed to get node id, hr %#x.\n", hr);
    ok(((id >> 32) == GetCurrentProcessId()) && !!(id & 0xffff), "Unexpected node id %s.\n", wine_dbgstr_longlong(id));

    hr = IMFTopologyNode_SetTopoNodeID(node2, id);
    ok(hr == S_OK, "Failed to set node id, hr %#x.\n", hr);

    hr = IMFTopology_GetNodeCount(topology, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFTopology_AddNode(topology, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    node_count = 1;
    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(node_count == 0, "Unexpected node count %u.\n", node_count);

    /* Same id, different nodes. */
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);

    node_count = 0;
    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(node_count == 1, "Unexpected node count %u.\n", node_count);

    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);
    IMFTopologyNode_Release(node2);

    hr = IMFTopology_GetNodeByID(topology, id, &node2);
    ok(hr == S_OK, "Failed to get a node, hr %#x.\n", hr);
    ok(node2 == node, "Unexpected node.\n");
    IMFTopologyNode_Release(node2);

    /* Change node id, add it again. */
    hr = IMFTopologyNode_SetTopoNodeID(node, ++id);
    ok(hr == S_OK, "Failed to set node id, hr %#x.\n", hr);

    hr = IMFTopology_GetNodeByID(topology, id, &node2);
    ok(hr == S_OK, "Failed to get a node, hr %#x.\n", hr);
    ok(node2 == node, "Unexpected node.\n");
    IMFTopologyNode_Release(node2);

    hr = IMFTopology_GetNodeByID(topology, id + 1, &node2);
    ok(hr == MF_E_NOT_FOUND, "Unexpected hr %#x.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == E_INVALIDARG, "Failed to add a node, hr %#x.\n", hr);

    hr = IMFTopology_GetNode(topology, 0, &node2);
    ok(hr == S_OK, "Failed to get a node, hr %#x.\n", hr);
    ok(node2 == node, "Unexpected node.\n");
    IMFTopologyNode_Release(node2);

    hr = IMFTopology_GetNode(topology, 1, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFTopology_GetNode(topology, 1, &node2);
    ok(hr == MF_E_INVALIDINDEX, "Failed to get a node, hr %#x.\n", hr);

    hr = IMFTopology_GetNode(topology, -2, &node2);
    ok(hr == MF_E_INVALIDINDEX, "Failed to get a node, hr %#x.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);
    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);
    IMFTopologyNode_Release(node2);

    node_count = 0;
    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(node_count == 2, "Unexpected node count %u.\n", node_count);

    /* Remove with detached node, existing id. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);
    hr = IMFTopologyNode_SetTopoNodeID(node2, id);
    ok(hr == S_OK, "Failed to set node id, hr %#x.\n", hr);
    hr = IMFTopology_RemoveNode(topology, node2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);
    IMFTopologyNode_Release(node2);

    hr = IMFTopology_RemoveNode(topology, node);
    ok(hr == S_OK, "Failed to remove a node, hr %#x.\n", hr);

    node_count = 0;
    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(node_count == 1, "Unexpected node count %u.\n", node_count);

    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#x.\n", hr);

    node_count = 1;
    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(node_count == 0, "Unexpected node count %u.\n", node_count);

    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetTopoNodeID(node, 123);
    ok(hr == S_OK, "Failed to set node id, hr %#x.\n", hr);

    IMFTopologyNode_Release(node);

    /* Change id for attached node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);

    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetTopoNodeID(node, &id);
    ok(hr == S_OK, "Failed to get node id, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetTopoNodeID(node2, id);
    ok(hr == S_OK, "Failed to get node id, hr %#x.\n", hr);

    hr = IMFTopology_GetNodeByID(topology, id, &node3);
    ok(hr == S_OK, "Failed to get a node, hr %#x.\n", hr);
    ok(node3 == node, "Unexpected node.\n");
    IMFTopologyNode_Release(node3);

    IMFTopologyNode_Release(node);
    IMFTopologyNode_Release(node2);

    /* Source/output collections. */
    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#x.\n", hr);

    hr = IMFTopology_GetSourceNodeCollection(topology, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFTopology_GetSourceNodeCollection(topology, &collection);
    ok(hr == S_OK, "Failed to get source node collection, hr %#x.\n", hr);
    ok(!!collection, "Unexpected object pointer.\n");

    hr = IMFTopology_GetSourceNodeCollection(topology, &collection2);
    ok(hr == S_OK, "Failed to get source node collection, hr %#x.\n", hr);
    ok(!!collection2, "Unexpected object pointer.\n");
    ok(collection2 != collection, "Expected cloned collection.\n");

    hr = IMFCollection_GetElementCount(collection, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#x.\n", hr);
    ok(!size, "Unexpected item count.\n");

    hr = IMFCollection_AddElement(collection, (IUnknown *)collection);
    ok(hr == S_OK, "Failed to add element, hr %#x.\n", hr);

    hr = IMFCollection_GetElementCount(collection, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#x.\n", hr);
    ok(size == 1, "Unexpected item count.\n");

    hr = IMFCollection_GetElementCount(collection2, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#x.\n", hr);
    ok(!size, "Unexpected item count.\n");

    IMFCollection_Release(collection2);
    IMFCollection_Release(collection);

    /* Add some nodes. */
    hr = IMFTopology_GetSourceNodeCollection(topology, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFTopology_GetOutputNodeCollection(topology, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#x.\n", hr);
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);
    IMFTopologyNode_Release(node);

    hr = IMFTopology_GetSourceNodeCollection(topology, &collection);
    ok(hr == S_OK, "Failed to get source node collection, hr %#x.\n", hr);
    ok(!!collection, "Unexpected object pointer.\n");
    hr = IMFCollection_GetElementCount(collection, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#x.\n", hr);
    ok(size == 1, "Unexpected item count.\n");
    IMFCollection_Release(collection);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#x.\n", hr);
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);
    IMFTopologyNode_Release(node);

    hr = IMFTopology_GetSourceNodeCollection(topology, &collection);
    ok(hr == S_OK, "Failed to get source node collection, hr %#x.\n", hr);
    ok(!!collection, "Unexpected object pointer.\n");
    hr = IMFCollection_GetElementCount(collection, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#x.\n", hr);
    ok(size == 1, "Unexpected item count.\n");
    IMFCollection_Release(collection);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#x.\n", hr);
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);
    IMFTopologyNode_Release(node);

    hr = IMFTopology_GetSourceNodeCollection(topology, &collection);
    ok(hr == S_OK, "Failed to get source node collection, hr %#x.\n", hr);
    ok(!!collection, "Unexpected object pointer.\n");
    hr = IMFCollection_GetElementCount(collection, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#x.\n", hr);
    ok(size == 1, "Unexpected item count.\n");
    IMFCollection_Release(collection);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#x.\n", hr);
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);

    /* Associated object. */
    hr = IMFTopologyNode_SetObject(node, NULL);
    ok(hr == S_OK, "Failed to set object, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetObject(node, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    object = (void *)0xdeadbeef;
    hr = IMFTopologyNode_GetObject(node, &object);
    ok(hr == E_FAIL, "Unexpected hr %#x.\n", hr);
    ok(!object, "Unexpected object %p.\n", object);

    hr = IMFTopologyNode_SetObject(node, &test_unk);
    ok(hr == S_OK, "Failed to set object, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetObject(node, &object);
    ok(hr == S_OK, "Failed to get object, hr %#x.\n", hr);
    ok(object == &test_unk, "Unexpected object %p.\n", object);
    IUnknown_Release(object);

    hr = IMFTopologyNode_SetObject(node, &test_unk2);
    ok(hr == S_OK, "Failed to set object, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetCount(node, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected attribute count %u.\n", count);

    hr = IMFTopologyNode_SetGUID(node, &MF_TOPONODE_TRANSFORM_OBJECTID, &MF_TOPONODE_TRANSFORM_OBJECTID);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetObject(node, NULL);
    ok(hr == S_OK, "Failed to set object, hr %#x.\n", hr);

    object = (void *)0xdeadbeef;
    hr = IMFTopologyNode_GetObject(node, &object);
    ok(hr == E_FAIL, "Unexpected hr %#x.\n", hr);
    ok(!object, "Unexpected object %p.\n", object);

    hr = IMFTopologyNode_GetCount(node, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    /* Preferred stream types. */
    hr = IMFTopologyNode_GetInputCount(node, &count);
    ok(hr == S_OK, "Failed to get input count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected count %u.\n", count);

    hr = IMFTopologyNode_GetInputPrefType(node, 0, &mediatype);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 0, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetInputPrefType(node, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get preferred type, hr %#x.\n", hr);
    ok(mediatype2 == mediatype, "Unexpected mediatype instance.\n");
    IMFMediaType_Release(mediatype2);

    hr = IMFTopologyNode_SetInputPrefType(node, 0, NULL);
    ok(hr == S_OK, "Failed to set preferred type, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetInputPrefType(node, 0, &mediatype2);
    ok(hr == E_FAIL, "Unexpected hr %#x.\n", hr);
    ok(!mediatype2, "Unexpected mediatype instance.\n");

    hr = IMFTopologyNode_SetInputPrefType(node, 1, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 1, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetInputCount(node, &count);
    ok(hr == S_OK, "Failed to get input count, hr %#x.\n", hr);
    ok(count == 2, "Unexpected count %u.\n", count);

    hr = IMFTopologyNode_GetOutputCount(node, &count);
    ok(hr == S_OK, "Failed to get input count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected count %u.\n", count);

    hr = IMFTopologyNode_SetOutputPrefType(node, 0, mediatype);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    IMFTopologyNode_Release(node);

    /* Source node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 0, mediatype);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFTopologyNode_SetOutputPrefType(node, 2, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetOutputPrefType(node, 0, &mediatype2);
    ok(hr == E_FAIL, "Failed to get preferred type, hr %#x.\n", hr);
    ok(!mediatype2, "Unexpected mediatype instance.\n");

    hr = IMFTopologyNode_GetOutputCount(node, &count);
    ok(hr == S_OK, "Failed to get output count, hr %#x.\n", hr);
    ok(count == 3, "Unexpected count %u.\n", count);

    IMFTopologyNode_Release(node);

    /* Tee node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 0, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetInputPrefType(node, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get preferred type, hr %#x.\n", hr);
    ok(mediatype2 == mediatype, "Unexpected mediatype instance.\n");
    IMFMediaType_Release(mediatype2);

    hr = IMFTopologyNode_GetOutputPrefType(node, 0, &mediatype2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFTopologyNode_GetInputCount(node, &count);
    ok(hr == S_OK, "Failed to get output count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected count %u.\n", count);

    hr = IMFTopologyNode_SetInputPrefType(node, 1, mediatype);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#x.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 3, mediatype);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#x.\n", hr);

    hr = IMFTopologyNode_SetOutputPrefType(node, 4, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetOutputPrefType(node, 0, &mediatype2);
    ok(hr == E_FAIL, "Unexpected hr %#x.\n", hr);

    hr = MFCreateMediaType(&mediatype2);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    /* Changing output type does not change input type. */
    hr = IMFTopologyNode_SetOutputPrefType(node, 4, mediatype2);
    ok(hr == S_OK, "Failed to set preferred type, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetInputPrefType(node, 0, &mediatype3);
    ok(hr == S_OK, "Failed to get preferred type, hr %#x.\n", hr);
    ok(mediatype3 == mediatype, "Unexpected mediatype instance.\n");
    IMFMediaType_Release(mediatype3);

    IMFMediaType_Release(mediatype2);

    hr = IMFTopologyNode_GetInputCount(node, &count);
    ok(hr == S_OK, "Failed to get output count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected count %u.\n", count);

    hr = IMFTopologyNode_GetOutputCount(node, &count);
    ok(hr == S_OK, "Failed to get output count, hr %#x.\n", hr);
    ok(count == 5, "Unexpected count %u.\n", count);

    IMFTopologyNode_Release(node);

    /* Transform node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 3, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetInputCount(node, &count);
    ok(hr == S_OK, "Failed to get input count, hr %#x.\n", hr);
    ok(count == 4, "Unexpected count %u.\n", count);

    hr = IMFTopologyNode_SetOutputPrefType(node, 4, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetInputCount(node, &count);
    ok(hr == S_OK, "Failed to get output count, hr %#x.\n", hr);
    ok(count == 4, "Unexpected count %u.\n", count);

    hr = IMFTopologyNode_GetOutputCount(node, &count);
    ok(hr == S_OK, "Failed to get output count, hr %#x.\n", hr);
    ok(count == 5, "Unexpected count %u.\n", count);

    IMFTopologyNode_Release(node);

    IMFMediaType_Release(mediatype);

    hr = IMFTopology_GetOutputNodeCollection(topology, &collection);
    ok(hr == S_OK || broken(hr == E_FAIL) /* before Win8 */, "Failed to get output node collection, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(!!collection, "Unexpected object pointer.\n");
        hr = IMFCollection_GetElementCount(collection, &size);
        ok(hr == S_OK, "Failed to get item count, hr %#x.\n", hr);
        ok(size == 1, "Unexpected item count.\n");
        IMFCollection_Release(collection);
    }

    IMFTopology_Release(topology);

    /* Connect nodes. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 1);

    hr = IMFTopologyNode_ConnectOutput(node, 0, node2, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#x.\n", hr);

    EXPECT_REF(node, 2);
    EXPECT_REF(node2, 2);

    IMFTopologyNode_Release(node);

    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 2);

    IMFTopologyNode_Release(node2);

    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 1);

    hr = IMFTopologyNode_GetNodeType(node2, &node_type);
    ok(hr == S_OK, "Failed to get node type, hr %#x.\n", hr);

    IMFTopologyNode_Release(node);

    /* Connect within topology. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#x.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);

    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);

    EXPECT_REF(node, 2);
    EXPECT_REF(node2, 2);

    hr = IMFTopologyNode_ConnectOutput(node, 0, node2, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#x.\n", hr);

    EXPECT_REF(node, 3);
    EXPECT_REF(node2, 3);

    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#x.\n", hr);

    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 1);

    /* Removing connected node breaks connection. */
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);

    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);

    hr = IMFTopologyNode_ConnectOutput(node, 0, node2, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#x.\n", hr);

    hr = IMFTopology_RemoveNode(topology, node);
    ok(hr == S_OK, "Failed to remove a node, hr %#x.\n", hr);

    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 2);

    hr = IMFTopologyNode_GetOutput(node, 0, &node3, &index);
    ok(hr == MF_E_NOT_FOUND, "Unexpected hr %#x.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);

    hr = IMFTopologyNode_ConnectOutput(node, 0, node2, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#x.\n", hr);

    hr = IMFTopology_RemoveNode(topology, node2);
    ok(hr == S_OK, "Failed to remove a node, hr %#x.\n", hr);

    EXPECT_REF(node, 2);
    EXPECT_REF(node2, 1);

    IMFTopologyNode_Release(node);
    IMFTopologyNode_Release(node2);

    /* Cloning nodes of different types. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = IMFTopologyNode_CloneFrom(node, node2);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#x.\n", hr);

    IMFTopologyNode_Release(node2);

    /* Cloning preferred types. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetOutputPrefType(node2, 0, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#x.\n", hr);

    /* Vista checks for additional attributes. */
    hr = IMFTopologyNode_CloneFrom(node, node2);
    ok(hr == S_OK || broken(hr == MF_E_ATTRIBUTENOTFOUND) /* Vista */, "Failed to clone a node, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetOutputPrefType(node, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get preferred type, hr %#x.\n", hr);
    ok(mediatype == mediatype2, "Unexpected media type.\n");

    IMFMediaType_Release(mediatype2);
    IMFMediaType_Release(mediatype);

    IMFTopologyNode_Release(node2);

    /* Existing preferred types are not cleared. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetOutputCount(node, &count);
    ok(hr == S_OK, "Failed to get output count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected output count.\n");

    hr = IMFTopologyNode_CloneFrom(node, node2);
    ok(hr == S_OK || broken(hr == MF_E_ATTRIBUTENOTFOUND) /* Vista */, "Failed to clone a node, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetOutputCount(node, &count);
    ok(hr == S_OK, "Failed to get output count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected output count.\n");

    hr = IMFTopologyNode_GetOutputPrefType(node, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get preferred type, hr %#x.\n", hr);
    ok(!!mediatype2, "Unexpected media type.\n");
    IMFMediaType_Release(mediatype2);

    hr = IMFTopologyNode_CloneFrom(node2, node);
    ok(hr == S_OK || broken(hr == MF_E_ATTRIBUTENOTFOUND) /* Vista */, "Failed to clone a node, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetOutputCount(node2, &count);
    ok(hr == S_OK, "Failed to get output count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected output count.\n");

    IMFTopologyNode_Release(node2);
    IMFTopologyNode_Release(node);

    /* Add one node, connect to another that hasn't been added. */
    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#x.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);

    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(node_count == 1, "Unexpected node count.\n");

    hr = IMFTopologyNode_ConnectOutput(node, 0, node2, 0);
    ok(hr == S_OK, "Failed to connect nodes, hr %#x.\n", hr);

    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(node_count == 1, "Unexpected node count.\n");

    IMFTopologyNode_Release(node);
    IMFTopologyNode_Release(node2);

    /* Add same node to different topologies. */
    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#x.\n", hr);

    hr = MFCreateTopology(&topology2);
    ok(hr == S_OK, "Failed to create topology, hr %#x.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);
    EXPECT_REF(node, 2);

    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(node_count == 1, "Unexpected node count.\n");

    hr = IMFTopology_GetNodeCount(topology2, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(node_count == 0, "Unexpected node count.\n");

    hr = IMFTopology_AddNode(topology2, node);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);
    EXPECT_REF(node, 3);

    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(node_count == 1, "Unexpected node count.\n");

    hr = IMFTopology_GetNodeCount(topology2, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(node_count == 1, "Unexpected node count.\n");

    IMFTopology_Release(topology2);
    IMFTopology_Release(topology);
}

static void test_topology_tee_node(void)
{
    IMFTopologyNode *src_node, *tee_node;
    IMFMediaType *mediatype, *mediatype2;
    IMFTopology *topology;
    unsigned int count;
    HRESULT hr;

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#x.\n", hr);

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &tee_node);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &src_node);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(tee_node, 0, mediatype);
    ok(hr == S_OK, "Failed to set type, hr %#x.\n", hr);

    /* Even though tee node has only one input and source has only one output,
       it's possible to connect to higher inputs/outputs. */

    /* SRC(0) -> TEE(0) */
    hr = IMFTopologyNode_ConnectOutput(src_node, 0, tee_node, 0);
    ok(hr == S_OK, "Failed to connect nodes, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetInputCount(tee_node, &count);
    ok(hr == S_OK, "Failed to get count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    hr = IMFTopologyNode_GetInputPrefType(tee_node, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get type, hr %#x.\n", hr);
    ok(mediatype2 == mediatype, "Unexpected type.\n");
    IMFMediaType_Release(mediatype2);

    /* SRC(0) -> TEE(1) */
    hr = IMFTopologyNode_ConnectOutput(src_node, 0, tee_node, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetInputCount(tee_node, &count);
    ok(hr == S_OK, "Failed to get count, hr %#x.\n", hr);
    ok(count == 2, "Unexpected count %u.\n", count);

    hr = IMFTopologyNode_SetInputPrefType(tee_node, 1, mediatype);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#x.\n", hr);

    /* SRC(1) -> TEE(1) */
    hr = IMFTopologyNode_ConnectOutput(src_node, 1, tee_node, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#x.\n", hr);

    hr = IMFTopologyNode_GetOutputCount(src_node, &count);
    ok(hr == S_OK, "Failed to get count, hr %#x.\n", hr);
    ok(count == 2, "Unexpected count %u.\n", count);

    IMFMediaType_Release(mediatype);
    IMFTopologyNode_Release(src_node);
    IMFTopologyNode_Release(tee_node);
    IMFTopology_Release(topology);
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
    ok(hr == E_POINTER, "Unexpected return value %#x.\n", hr);

    unk = (void *)0xdeadbeef;
    hr = MFGetService(NULL, NULL, NULL, (void **)&unk);
    ok(hr == E_POINTER, "Unexpected return value %#x.\n", hr);
    ok(unk == (void *)0xdeadbeef, "Unexpected out object.\n");

    hr = MFGetService(&testservice, NULL, NULL, NULL);
    ok(hr == 0x82eddead, "Unexpected return value %#x.\n", hr);

    unk = (void *)0xdeadbeef;
    hr = MFGetService(&testservice, NULL, NULL, (void **)&unk);
    ok(hr == 0x82eddead, "Unexpected return value %#x.\n", hr);
    ok(unk == (void *)0xdeadbeef, "Unexpected out object.\n");

    unk = NULL;
    hr = MFGetService(&testservice2, NULL, NULL, (void **)&unk);
    ok(hr == 0x83eddead, "Unexpected return value %#x.\n", hr);
    ok(unk == (void *)0xdeadbeef, "Unexpected out object.\n");
}

static void test_sequencer_source(void)
{
    IMFMediaSourceTopologyProvider *provider;
    IMFSequencerSource *seq_source;
    HRESULT hr;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#x.\n", hr);

    hr = MFCreateSequencerSource(NULL, &seq_source);
    ok(hr == S_OK, "Failed to create sequencer source, hr %#x.\n", hr);

    hr = IMFSequencerSource_QueryInterface(seq_source, &IID_IMFMediaSourceTopologyProvider, (void **)&provider);
    ok(hr == S_OK, "Failed to get provider interface, hr %#x.\n", hr);
    IMFMediaSourceTopologyProvider_Release(provider);

    IMFSequencerSource_Release(seq_source);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#x.\n", hr);
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
    ok(hr == MF_E_NO_EVENTS_AVAILABLE, "Unexpected hr %#x.\n", hr);

    /* Async case. */
    hr = IMFMediaSession_BeginGetEvent(session, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_BeginGetEvent(session, &callback.IMFAsyncCallback_iface, (IUnknown *)session);
    ok(hr == S_OK, "Failed to Begin*, hr %#x.\n", hr);

    /* Same callback, same state. */
    hr = IMFMediaSession_BeginGetEvent(session, &callback.IMFAsyncCallback_iface, (IUnknown *)session);
    ok(hr == MF_S_MULTIPLE_BEGIN, "Unexpected hr %#x.\n", hr);

    /* Same callback, different state. */
    hr = IMFMediaSession_BeginGetEvent(session, &callback.IMFAsyncCallback_iface, (IUnknown *)&callback);
    ok(hr == MF_E_MULTIPLE_BEGIN, "Unexpected hr %#x.\n", hr);

    /* Different callback, same state. */
    hr = IMFMediaSession_BeginGetEvent(session, &callback2.IMFAsyncCallback_iface, (IUnknown *)session);
    ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Unexpected hr %#x.\n", hr);

    /* Different callback, different state. */
    hr = IMFMediaSession_BeginGetEvent(session, &callback2.IMFAsyncCallback_iface, (IUnknown *)&callback.IMFAsyncCallback_iface);
    ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Unexpected hr %#x.\n", hr);

    hr = MFCreateAsyncResult(NULL, &callback.IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#x.\n", hr);

    hr = IMFMediaSession_EndGetEvent(session, result, &event);
    ok(hr == E_FAIL, "Unexpected hr %#x.\n", hr);

    /* Shutdown behavior. */
    hr = IMFMediaSession_Shutdown(session);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
}

static void test_media_session(void)
{
    IMFRateControl *rate_control, *rate_control2;
    IMFLocalMFTRegistration *local_reg;
    MFCLOCK_PROPERTIES clock_props;
    IMFRateSupport *rate_support;
    IMFAttributes *attributes;
    IMFMediaSession *session;
    IMFTopology *topology;
    PROPVARIANT propvar;
    IMFGetService *gs;
    IMFClock *clock;
    IUnknown *unk;
    HRESULT hr;
    float rate;
    DWORD caps;
    BOOL thin;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#x.\n", hr);

    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Failed to create media session, hr %#x.\n", hr);

    hr = IMFMediaSession_QueryInterface(session, &IID_IMFAttributes, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_QueryInterface(session, &IID_IMFGetService, (void **)&gs);
    ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);

    hr = IMFGetService_GetService(gs, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport, (void **)&rate_support);
    ok(hr == S_OK, "Failed to get rate support interface, hr %#x.\n", hr);

    hr = IMFGetService_GetService(gs, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateControl, (void **)&rate_control);
    ok(hr == S_OK, "Failed to get rate control interface, hr %#x.\n", hr);

    hr = IMFGetService_GetService(gs, &MF_LOCAL_MFT_REGISTRATION_SERVICE, &IID_IMFLocalMFTRegistration,
            (void **)&local_reg);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Vista */, "Failed to get registration service, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
        IMFLocalMFTRegistration_Release(local_reg);

    hr = IMFRateSupport_QueryInterface(rate_support, &IID_IMFMediaSession, (void **)&unk);
    ok(hr == S_OK, "Failed to get session interface, hr %#x.\n", hr);
    ok(unk == (IUnknown *)session, "Unexpected pointer.\n");
    IUnknown_Release(unk);

    hr = IMFRateControl_GetRate(rate_control, NULL, NULL);
    ok(FAILED(hr), "Unexpected hr %#x.\n", hr);

    rate = 0.0f;
    hr = IMFRateControl_GetRate(rate_control, NULL, &rate);
    ok(hr == S_OK, "Failed to get playback rate, hr %#x.\n", hr);
    ok(rate == 1.0f, "Unexpected rate %f.\n", rate);

    hr = IMFRateControl_GetRate(rate_control, &thin, NULL);
    ok(FAILED(hr), "Unexpected hr %#x.\n", hr);

    thin = TRUE;
    rate = 0.0f;
    hr = IMFRateControl_GetRate(rate_control, &thin, &rate);
    ok(hr == S_OK, "Failed to get playback rate, hr %#x.\n", hr);
    ok(!thin, "Unexpected thinning.\n");
    ok(rate == 1.0f, "Unexpected rate %f.\n", rate);

    hr = IMFMediaSession_GetClock(session, &clock);
    ok(hr == S_OK, "Failed to get clock, hr %#x.\n", hr);

    hr = IMFClock_QueryInterface(clock, &IID_IMFRateControl, (void **)&rate_control2);
    ok(hr == S_OK, "Failed to get rate control, hr %#x.\n", hr);

    rate = 0.0f;
    hr = IMFRateControl_GetRate(rate_control2, NULL, &rate);
    ok(hr == S_OK, "Failed to get clock rate, hr %#x.\n", hr);
    ok(rate == 1.0f, "Unexpected rate %f.\n", rate);

    hr = IMFRateControl_SetRate(rate_control, FALSE, 1.5f);
todo_wine
    ok(hr == S_OK, "Failed to set rate, hr %#x.\n", hr);

    IMFRateControl_Release(rate_control2);

    hr = IMFClock_GetProperties(clock, &clock_props);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);
    IMFClock_Release(clock);

    IMFRateControl_Release(rate_control);
    IMFRateSupport_Release(rate_support);

    IMFGetService_Release(gs);

    IMFMediaSession_Release(session);

    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Failed to create media session, hr %#x.\n", hr);

    hr = IMFMediaSession_Shutdown(session);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    hr = IMFMediaSession_ClearTopologies(session);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_Start(session, &GUID_NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    propvar.vt = VT_EMPTY;
    hr = IMFMediaSession_Start(session, &GUID_NULL, &propvar);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_Pause(session);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_Stop(session);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_Close(session);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_GetClock(session, &clock);
    ok(hr == MF_E_SHUTDOWN || broken(hr == E_UNEXPECTED) /* Win7 */, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_GetSessionCapabilities(session, &caps);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_GetSessionCapabilities(session, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_GetFullTopology(session, MFSESSION_GETFULLTOPOLOGY_CURRENT, 0, &topology);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_Shutdown(session);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    IMFMediaSession_Release(session);

    /* Custom topology loader, GUID is not registered. */
    hr = MFCreateAttributes(&attributes, 1);
    ok(hr == S_OK, "Failed to create attributes, hr %#x.\n", hr);

    hr = IMFAttributes_SetGUID(attributes, &MF_SESSION_TOPOLOADER, &MF_SESSION_TOPOLOADER);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = MFCreateMediaSession(attributes, &session);
    ok(hr == S_OK, "Failed to create media session, hr %#x.\n", hr);
    IMFMediaSession_Release(session);

    /* Disabled quality manager. */
    hr = IMFAttributes_SetGUID(attributes, &MF_SESSION_QUALITY_MANAGER, &GUID_NULL);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = MFCreateMediaSession(attributes, &session);
    ok(hr == S_OK, "Failed to create media session, hr %#x.\n", hr);
    IMFMediaSession_Release(session);

    IMFAttributes_Release(attributes);

    /* Basic events behavior. */
    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Failed to create media session, hr %#x.\n", hr);

    test_session_events(session);

    IMFMediaSession_Release(session);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#x.\n", hr);
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

static void test_topology_loader(void)
{
    IMFSampleGrabberSinkCallback test_grabber_callback = { &test_grabber_callback_vtbl };
    IMFTopology *topology, *topology2, *full_topology;
    IMFTopologyNode *src_node, *sink_node;
    IMFPresentationDescriptor *pd;
    IMFSourceResolver *resolver;
    IMFActivate *sink_activate;
    IMFStreamSink *stream_sink;
    unsigned int count, value;
    IMFMediaType *media_type;
    IMFStreamDescriptor *sd;
    MF_OBJECT_TYPE obj_type;
    IMFMediaSource *source;
    IMFTopoLoader *loader;
    IMFByteStream *stream;
    IMFAttributes *attr;
    IMFMediaSink *sink;
    WCHAR *filename;
    BOOL selected;
    HRESULT hr;
    GUID guid;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#x.\n", hr);

    hr = MFCreateTopoLoader(NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = MFCreateTopoLoader(&loader);
    ok(hr == S_OK, "Failed to create topology loader, hr %#x.\n", hr);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#x.\n", hr);

    /* Empty topology */
    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
todo_wine
    ok(hr == MF_E_TOPO_UNSUPPORTED, "Unexpected hr %#x.\n", hr);

    hr = MFCreateSourceResolver(&resolver);
    ok(hr == S_OK, "Failed to create source resolver, hr %#x.\n", hr);

    filename = load_resource(L"test.wav");

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, filename, &stream);
    ok(hr == S_OK, "Failed to create file stream, hr %#x.\n", hr);

    IMFByteStream_QueryInterface(stream, &IID_IMFAttributes, (void **)&attr);
    IMFAttributes_SetString(attr, &MF_BYTESTREAM_CONTENT_TYPE, L"audio/wav");
    IMFAttributes_Release(attr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
            &obj_type, (IUnknown **)&source);
    ok(hr == S_OK || broken(FAILED(hr)) /* Vista */, "Failed to create source, hr %#x.\n", hr);
    if (FAILED(hr))
        return;

    hr = IMFMediaSource_CreatePresentationDescriptor(source, &pd);
todo_wine
    ok(hr == S_OK, "Failed to create descriptor, hr %#x.\n", hr);
    if (FAILED(hr))
        return;

    hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, 0, &selected, &sd);
    ok(hr == S_OK, "Failed to get stream descriptor, hr %#x.\n", hr);

    /* Add source node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &src_node);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_SOURCE, (IUnknown *)source);
    ok(hr == S_OK, "Failed to set node source, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, (IUnknown *)pd);
    ok(hr == S_OK, "Failed to set node pd, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_STREAM_DESCRIPTOR, (IUnknown *)sd);
    ok(hr == S_OK, "Failed to set node sd, hr %#x.\n", hr);

    hr = IMFTopology_AddNode(topology, src_node);
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);

    /* Source node only. */
    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
todo_wine
    ok(hr == MF_E_TOPO_UNSUPPORTED, "Unexpected hr %#x.\n", hr);

    /* Add grabber sink. */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = MFCreateSampleGrabberSinkActivate(media_type, &test_grabber_callback, &sink_activate);
    ok(hr == S_OK, "Failed to create grabber sink, hr %#x.\n", hr);

    IMFMediaType_Release(media_type);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &sink_node);
    ok(hr == S_OK, "Failed to create output node, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetObject(sink_node, (IUnknown *)sink_activate);
    ok(hr == S_OK, "Failed to set object, hr %#x.\n", hr);
    hr = IMFTopology_AddNode(topology, sink_node);
    ok(hr == S_OK, "Failed to add sink node, hr %#x.\n", hr);

    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
todo_wine
    ok(hr == MF_E_TOPO_UNSUPPORTED, "Unexpected hr %#x.\n", hr);

    hr = IMFTopologyNode_ConnectOutput(src_node, 0, sink_node, 0);
    ok(hr == S_OK, "Failed to connect nodes, hr %#x.\n", hr);

    /* Sink was not resolved. */
    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == MF_E_TOPO_SINK_ACTIVATES_UNSUPPORTED, "Unexpected hr %#x.\n", hr);

    hr = IMFActivate_ActivateObject(sink_activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Failed to activate, hr %#x.\n", hr);

    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 0, &stream_sink);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTopologyNode_SetObject(sink_node, (IUnknown *)stream_sink);
    ok(hr == S_OK, "Failed to set object, hr %#x.\n", hr);

    IMFStreamSink_Release(stream_sink);

    hr = IMFTopology_GetCount(topology, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected count %u.\n", count);

    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == S_OK, "Failed to resolve topology, hr %#x.\n", hr);
    ok(full_topology != topology, "Unexpected instance.\n");

    hr = IMFTopology_GetCount(topology, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected count %u.\n", count);

    hr = IMFTopology_GetCount(full_topology, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#x.\n", hr);
todo_wine
    ok(count == 1, "Unexpected count %u.\n", count);

    hr = IMFTopology_GetItemByIndex(full_topology, 0, &guid, NULL);
todo_wine {
    ok(hr == S_OK, "Failed to get attribute key, hr %#x.\n", hr);
    ok(IsEqualGUID(&guid, &MF_TOPOLOGY_RESOLUTION_STATUS), "Unexpected key %s.\n", wine_dbgstr_guid(&guid));
}
    value = 0xdeadbeef;
    hr = IMFTopology_GetUINT32(full_topology, &MF_TOPOLOGY_RESOLUTION_STATUS, &value);
todo_wine {
    ok(hr == S_OK, "Failed to get attribute, hr %#x.\n", hr);
    ok(value == MF_TOPOLOGY_RESOLUTION_SUCCEEDED, "Unexpected value %#x.\n", value);
}
    hr = IMFTopoLoader_Load(loader, full_topology, &topology2, NULL);
    ok(hr == S_OK, "Failed to resolve topology, hr %#x.\n", hr);
    ok(full_topology != topology2, "Unexpected instance.\n");

    IMFTopology_Release(topology2);
    IMFTopology_Release(full_topology);

    IMFMediaSource_Release(source);
    IMFSourceResolver_Release(resolver);
    IMFByteStream_Release(stream);
    IMFTopoLoader_Release(loader);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#x.\n", hr);
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
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = MFShutdownObject((IUnknown *)&testshutdown);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    hr = MFShutdownObject(&testshutdown2);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
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
    IMFTimer *timer;
    unsigned int i;
    DWORD value;
    float rate;
    HRESULT hr;
    BOOL thin;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#x.\n", hr);

    hr = MFCreatePresentationClock(&clock);
    ok(hr == S_OK, "Failed to create presentation clock, hr %#x.\n", hr);

    hr = IMFPresentationClock_QueryInterface(clock, &IID_IMFRateControl, (void **)&rate_control);
    ok(hr == S_OK, "Failed to get rate control interface, hr %#x.\n", hr);

    hr = IMFPresentationClock_GetTimeSource(clock, &time_source);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_GetTimeSource(clock, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_GetClockCharacteristics(clock, &value);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_GetClockCharacteristics(clock, NULL);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_GetTime(clock, &time);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_GetTime(clock, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    value = 1;
    hr = IMFPresentationClock_GetContinuityKey(clock, &value);
    ok(hr == S_OK, "Failed to get continuity key, hr %#x.\n", hr);
    ok(value == 0, "Unexpected value %u.\n", value);

    hr = IMFPresentationClock_GetProperties(clock, &props);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_GetState(clock, 0, &state);
    ok(hr == S_OK, "Failed to get state, hr %#x.\n", hr);
    ok(state == MFCLOCK_STATE_INVALID, "Unexpected state %d.\n", state);

    hr = IMFPresentationClock_GetCorrelatedTime(clock, 0, &clock_time, &systime);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_GetCorrelatedTime(clock, 0, NULL, &systime);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_GetCorrelatedTime(clock, 0, &time, NULL);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    /* Sinks. */
    hr = IMFPresentationClock_AddClockStateSink(clock, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_AddClockStateSink(clock, &test_sink);
    ok(hr == S_OK, "Failed to add a sink, hr %#x.\n", hr);

    hr = IMFPresentationClock_AddClockStateSink(clock, &test_sink);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_RemoveClockStateSink(clock, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_RemoveClockStateSink(clock, &test_sink);
    ok(hr == S_OK, "Failed to remove sink, hr %#x.\n", hr);

    hr = IMFPresentationClock_RemoveClockStateSink(clock, &test_sink);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* State change commands, time source is not set yet. */
    hr = IMFPresentationClock_Start(clock, 0);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_Pause(clock);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_Stop(clock);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    hr = IMFRateControl_SetRate(rate_control, FALSE, 0.0f);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    /* Set default time source. */
    hr = MFCreateSystemTimeSource(&time_source);
    ok(hr == S_OK, "Failed to create time source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetClockCharacteristics(time_source, &value);
    ok(hr == S_OK, "Failed to get time source flags, hr %#x.\n", hr);
    ok(value == (MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ | MFCLOCK_CHARACTERISTICS_FLAG_IS_SYSTEM_CLOCK),
            "Unexpected clock flags %#x.\n", value);

    hr = IMFPresentationClock_SetTimeSource(clock, time_source);
    ok(hr == S_OK, "Failed to set time source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetProperties(time_source, &props2);
    ok(hr == S_OK, "Failed to get time source properties, hr %#x.\n", hr);

    hr = IMFPresentationClock_GetClockCharacteristics(clock, &value);
    ok(hr == S_OK, "Failed to get clock flags, hr %#x.\n", hr);
    ok(value == (MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ | MFCLOCK_CHARACTERISTICS_FLAG_IS_SYSTEM_CLOCK),
            "Unexpected clock flags %#x.\n", value);

    hr = IMFPresentationClock_GetProperties(clock, &props);
    ok(hr == S_OK, "Failed to get clock properties, hr %#x.\n", hr);
    ok(!memcmp(&props, &props2, sizeof(props)), "Unexpected clock properties.\n");

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
        ok(hr == clock_state_change[i].hr, "%u: unexpected hr %#x.\n", i, hr);

        hr = IMFPresentationTimeSource_GetState(time_source, 0, &state);
        ok(hr == S_OK, "%u: failed to get state, hr %#x.\n", i, hr);
        ok(state == clock_state_change[i].source_state, "%u: unexpected state %d.\n", i, state);

        hr = IMFPresentationClock_GetState(clock, 0, &state);
        ok(hr == S_OK, "%u: failed to get state, hr %#x.\n", i, hr);
        ok(state == clock_state_change[i].clock_state, "%u: unexpected state %d.\n", i, state);
    }

    /* Clock time stamps. */
    hr = IMFPresentationClock_Start(clock, 10);
    ok(hr == S_OK, "Failed to start presentation clock, hr %#x.\n", hr);

    hr = IMFPresentationClock_Pause(clock);
    ok(hr == S_OK, "Failed to pause presentation clock, hr %#x.\n", hr);

    hr = IMFPresentationClock_GetTime(clock, &time);
    ok(hr == S_OK, "Failed to get clock time, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &clock_time, &systime);
    ok(hr == S_OK, "Failed to get time source time, hr %#x.\n", hr);
    ok(time == clock_time, "Unexpected clock time.\n");

    hr = IMFPresentationClock_GetCorrelatedTime(clock, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get clock time, hr %#x.\n", hr);
    ok(time == clock_time, "Unexpected clock time.\n");

    IMFPresentationTimeSource_Release(time_source);

    hr = IMFRateControl_GetRate(rate_control, NULL, &rate);
    ok(hr == S_OK, "Failed to get clock rate, hr %#x.\n", hr);

    hr = IMFRateControl_GetRate(rate_control, &thin, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFRateControl_GetRate(rate_control, &thin, &rate);
    ok(hr == S_OK, "Failed to get clock rate, hr %#x.\n", hr);
    ok(rate == 1.0f, "Unexpected rate.\n");
    ok(!thin, "Unexpected thinning.\n");

    hr = IMFPresentationClock_GetState(clock, 0, &state);
    ok(hr == S_OK, "Failed to get clock state, hr %#x.\n", hr);
    ok(state == MFCLOCK_STATE_PAUSED, "Unexpected state %d.\n", state);

    hr = IMFPresentationClock_Start(clock, 0);
    ok(hr == S_OK, "Failed to stop, hr %#x.\n", hr);

    hr = IMFRateControl_SetRate(rate_control, FALSE, 0.0f);
    ok(hr == S_OK, "Failed to set clock rate, hr %#x.\n", hr);
    hr = IMFRateControl_GetRate(rate_control, &thin, &rate);
    ok(hr == S_OK, "Failed to get clock rate, hr %#x.\n", hr);
    ok(rate == 0.0f, "Unexpected rate.\n");
    hr = IMFRateControl_SetRate(rate_control, FALSE, 1.0f);
    ok(hr == S_OK, "Failed to set clock rate, hr %#x.\n", hr);
    hr = IMFRateControl_SetRate(rate_control, FALSE, 0.0f);
    ok(hr == S_OK, "Failed to set clock rate, hr %#x.\n", hr);
    hr = IMFRateControl_SetRate(rate_control, FALSE, 0.5f);
    ok(hr == S_OK, "Failed to set clock rate, hr %#x.\n", hr);
    hr = IMFRateControl_SetRate(rate_control, TRUE, -1.0f);
    ok(hr == MF_E_THINNING_UNSUPPORTED, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_GetState(clock, 0, &state);
    ok(hr == S_OK, "Failed to get clock state, hr %#x.\n", hr);
    ok(state == MFCLOCK_STATE_RUNNING, "Unexpected state %d.\n", state);

    hr = IMFRateControl_GetRate(rate_control, &thin, &rate);
    ok(hr == S_OK, "Failed to get clock rate, hr %#x.\n", hr);
    ok(rate == 0.5f, "Unexpected rate.\n");
    ok(!thin, "Unexpected thinning.\n");

    IMFRateControl_Release(rate_control);

    hr = IMFPresentationClock_QueryInterface(clock, &IID_IMFTimer, (void **)&timer);
    ok(hr == S_OK, "Failed to get timer interface, hr %#x.\n", hr);
    IMFTimer_Release(timer);

    hr = IMFPresentationClock_QueryInterface(clock, &IID_IMFShutdown, (void **)&shutdown);
    ok(hr == S_OK, "Failed to get shutdown interface, hr %#x.\n", hr);

    /* Shutdown behavior. */
    hr = IMFShutdown_GetShutdownStatus(shutdown, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFShutdown_GetShutdownStatus(shutdown, &status);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#x.\n", hr);

    hr = IMFShutdown_Shutdown(shutdown);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    time_source = NULL;
    hr = IMFPresentationClock_GetTimeSource(clock, &time_source);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!!time_source, "Unexpected instance %p.\n", time_source);
    IMFPresentationTimeSource_Release(time_source);

    hr = IMFPresentationClock_GetTime(clock, &time);
    ok(hr == S_OK, "Failed to get time, hr %#x.\n", hr);

    hr = IMFShutdown_GetShutdownStatus(shutdown, &status);
    ok(hr == S_OK, "Failed to get status, hr %#x.\n", hr);
    ok(status == MFSHUTDOWN_COMPLETED, "Unexpected status.\n");

    hr = IMFPresentationClock_Start(clock, 0);
    ok(hr == S_OK, "Failed to start the clock, hr %#x.\n", hr);

    hr = IMFShutdown_GetShutdownStatus(shutdown, &status);
    ok(hr == S_OK, "Failed to get status, hr %#x.\n", hr);
    ok(status == MFSHUTDOWN_COMPLETED, "Unexpected status.\n");

    hr = IMFShutdown_Shutdown(shutdown);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IMFShutdown_Release(shutdown);

    IMFPresentationClock_Release(clock);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
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
    IMFClockStateSink *clocksink;
    IMFMediaEventGenerator *eg;
    IMFMediaSink *sink, *sink2;
    DWORD flags, count, id;
    IMFActivate *activate;
    IMFMediaEvent *event;
    ULONG refcount;
    IUnknown *unk;
    HRESULT hr;
    GUID guid;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#x.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = MFCreateSampleGrabberSinkActivate(NULL, NULL, &activate);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = MFCreateSampleGrabberSinkActivate(NULL, &grabber_callback, &activate);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    EXPECT_REF(media_type, 1);
    hr = MFCreateSampleGrabberSinkActivate(media_type, &grabber_callback, &activate);
    ok(hr == S_OK, "Failed to create grabber activate, hr %#x.\n", hr);
    EXPECT_REF(media_type, 2);

    hr = IMFActivate_GetCount(activate, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#x.\n", hr);
    ok(!count, "Unexpected count %u.\n", count);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Failed to activate object, hr %#x.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Failed to get sink flags, hr %#x.\n", hr);
    ok(flags & MEDIASINK_FIXED_STREAMS, "Unexpected flags %#x.\n", flags);

    hr = IMFMediaSink_GetStreamSinkCount(sink, &count);
    ok(hr == S_OK, "Failed to get stream count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected stream count %u.\n", count);

    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 0, &stream);
    ok(hr == S_OK, "Failed to get sink stream, hr %#x.\n", hr);

    hr = IMFStreamSink_QueryInterface(stream, &IID_IMFMediaEventGenerator, (void **)&unk);
    ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);
    IUnknown_Release(unk);

    hr = IMFStreamSink_GetIdentifier(stream, &id);
    ok(hr == S_OK, "Failed to get stream id, hr %#x.\n", hr);
    ok(id == 0, "Unexpected id %#x.\n", id);

    hr = IMFStreamSink_GetMediaSink(stream, &sink2);
    ok(hr == S_OK, "Failed to get media sink, hr %x.\n", hr);
    ok(sink2 == sink, "Unexpected sink.\n");
    IMFMediaSink_Release(sink2);

    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 1, &stream2);
    ok(hr == MF_E_INVALIDINDEX, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_GetStreamSinkById(sink, 1, &stream2);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_AddStreamSink(sink, 1, NULL, &stream2);
    ok(hr == MF_E_STREAMSINKS_FIXED, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_RemoveStreamSink(sink, 0);
    ok(hr == MF_E_STREAMSINKS_FIXED, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_RemoveStreamSink(sink, 1);
    ok(hr == MF_E_STREAMSINKS_FIXED, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFClockStateSink, (void **)&clocksink);
    ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);
    IMFClockStateSink_Release(clocksink);

    /* Event generator. */
    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFMediaEventGenerator, (void **)&eg);
    ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);

    hr = IMFMediaEventGenerator_GetEvent(eg, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_NO_EVENTS_AVAILABLE, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFPresentationTimeSource, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    hr = IMFStreamSink_QueryInterface(stream, &IID_IMFMediaTypeHandler, (void **)&handler2);
    ok(hr == S_OK, "Failed to get handler interface, hr %#x.\n", hr);

    hr = IMFStreamSink_GetMediaTypeHandler(stream, &handler);
    ok(hr == S_OK, "Failed to get type handler, hr %#x.\n", hr);
    hr = IMFMediaTypeHandler_GetMediaTypeCount(handler, &count);
    ok(hr == S_OK, "Failed to get media type count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected count %u.\n", count);
    ok(handler == handler2, "Unexpected handler.\n");

    IMFMediaTypeHandler_Release(handler);
    IMFMediaTypeHandler_Release(handler2);

    /* Set clock. */
    hr = MFCreatePresentationClock(&clock);
    ok(hr == S_OK, "Failed to create clock object, hr %#x.\n", hr);

    hr = IMFMediaSink_GetPresentationClock(sink, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_GetPresentationClock(sink, &clock2);
    ok(hr == MF_E_NO_CLOCK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_SetPresentationClock(sink, NULL);
    ok(hr == S_OK, "Failed to set presentation clock, hr %#x.\n", hr);

    hr = IMFMediaSink_SetPresentationClock(sink, clock);
    ok(hr == S_OK, "Failed to set presentation clock, hr %#x.\n", hr);

    hr = MFCreateSystemTimeSource(&time_source);
    ok(hr == S_OK, "Failed to create time source, hr %#x.\n", hr);

    hr = IMFPresentationClock_SetTimeSource(clock, time_source);
    ok(hr == S_OK, "Failed to set time source, hr %#x.\n", hr);
    IMFPresentationTimeSource_Release(time_source);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Failed to get sink flags, hr %#x.\n", hr);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Failed to get sink flags, hr %#x.\n", hr);

    hr = IMFStreamSink_GetMediaTypeHandler(stream, &handler);
    ok(hr == S_OK, "Failed to get type handler, hr %#x.\n", hr);

    /* On Win8+ this initialization happens automatically. */
    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type);
    ok(hr == S_OK, "Failed to set media type, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeCount(handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeCount(handler, &count);
    ok(hr == S_OK, "Failed to get media type count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected count %u.\n", count);

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == S_OK, "Failed to get major type, hr %#x.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Audio), "Unexpected major type %s.\n", wine_dbgstr_guid(&guid));

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type2);
    ok(hr == S_OK, "Failed to get current type, hr %#x.\n", hr);
    ok(media_type2 == media_type, "Unexpected media type.\n");
    IMFMediaType_Release(media_type2);

    hr = MFCreateMediaType(&media_type2);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type2, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type2, &MF_MT_SUBTYPE, &MFAudioFormat_Float);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type2, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaType_SetUINT32(media_type2, &MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type2);
    ok(hr == S_OK, "Failed to get current type, hr %#x.\n", hr);
    IMFMediaType_Release(media_type);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type);
    ok(hr == S_OK, "Failed to get current type, hr %#x.\n", hr);
    ok(media_type2 == media_type, "Unexpected media type.\n");
    IMFMediaType_Release(media_type);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, 0, &media_type);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type2, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, NULL);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, &media_type3);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    media_type3 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, &media_type3);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(media_type3 == (void *)0xdeadbeef, "Unexpected media type %p.\n", media_type3);

    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_SAMPLE_SIZE, 1024);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    media_type3 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, &media_type3);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(media_type3 == (void *)0xdeadbeef, "Unexpected media type %p.\n", media_type3);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEventGenerator_GetEvent(eg, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_NO_EVENTS_AVAILABLE, "Unexpected hr %#x.\n", hr);

    hr = IMFStreamSink_GetEvent(stream, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_NO_EVENTS_AVAILABLE, "Unexpected hr %#x.\n", hr);

    EXPECT_REF(clock, 3);
    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
    EXPECT_REF(clock, 1);

    hr = IMFMediaEventGenerator_GetEvent(eg, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_AddStreamSink(sink, 1, NULL, &stream2);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_GetStreamSinkCount(sink, &count);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 0, &stream2);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFStreamSink_GetEvent(stream, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#x.\n", hr);

    hr = IMFStreamSink_GetMediaSink(stream, &sink2);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#x.\n", hr);

    id = 1;
    hr = IMFStreamSink_GetIdentifier(stream, &id);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#x.\n", hr);
    ok(id == 1, "Unexpected id %u.\n", id);

    media_type3 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, &media_type3);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#x.\n", hr);
    ok(media_type3 == (void *)0xdeadbeef, "Unexpected media type %p.\n", media_type3);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, NULL, NULL);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, NULL);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeCount(handler, &count);
    ok(hr == S_OK, "Failed to get type count, hr %#x.\n", hr);

    IMFMediaType_Release(media_type2);
    IMFMediaType_Release(media_type);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, 0, &media_type);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#x.\n", hr);

    IMFMediaTypeHandler_Release(handler);

    handler = (void *)0xdeadbeef;
    hr = IMFStreamSink_GetMediaTypeHandler(stream, &handler);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#x.\n", hr);
    ok(handler == (void *)0xdeadbeef, "Unexpected pointer.\n");

    hr = IMFStreamSink_GetMediaTypeHandler(stream, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    IMFMediaEventGenerator_Release(eg);
    IMFMediaSink_Release(sink);
    IMFStreamSink_Release(stream);

    refcount = IMFActivate_Release(activate);
    ok(!refcount, "Unexpected refcount %u.\n", refcount);

    /* Rateless mode with MF_SAMPLEGRABBERSINK_IGNORE_CLOCK. */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = MFCreateSampleGrabberSinkActivate(media_type, &grabber_callback, &activate);
    ok(hr == S_OK, "Failed to create grabber activate, hr %#x.\n", hr);

    hr = IMFActivate_SetUINT32(activate, &MF_SAMPLEGRABBERSINK_IGNORE_CLOCK, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Failed to activate object, hr %#x.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Failed to get sink flags, hr %#x.\n", hr);
    ok(flags & MEDIASINK_RATELESS, "Unexpected flags %#x.\n", flags);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    IMFMediaSink_Release(sink);

    /* Detaching */
    hr = MFCreateSampleGrabberSinkActivate(media_type, &grabber_callback, &activate);
    ok(hr == S_OK, "Failed to create grabber activate, hr %#x.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Failed to activate object, hr %#x.\n", hr);
    IMFMediaSink_Release(sink);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFActivate_GetCount(activate, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFActivate_DetachObject(activate);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    IMFActivate_Release(activate);

    IMFMediaType_Release(media_type);
    IMFPresentationClock_Release(clock);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
}

static BOOL is_supported_video_type(const GUID *guid)
{
    return IsEqualGUID(guid, &MFVideoFormat_L8)
            || IsEqualGUID(guid, &MFVideoFormat_L16)
            || IsEqualGUID(guid, &MFVideoFormat_D16)
            || IsEqualGUID(guid, &MFVideoFormat_IYUV)
            || IsEqualGUID(guid, &MFVideoFormat_YV12)
            || IsEqualGUID(guid, &MFVideoFormat_NV12)
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
    DWORD input_count, output_count, input_id, output_id, flags;
    DWORD input_min, input_max, output_min, output_max, i, count;
    IMFAttributes *attributes, *attributes2;
    IMFMediaType *media_type, *media_type2;
    MFT_OUTPUT_DATA_BUFFER output_buffer;
    MFT_OUTPUT_STREAM_INFO output_info;
    MFT_INPUT_STREAM_INFO input_info;
    IMFSample *sample, *sample2;
    IMFTransform *transform;
    IMFMediaBuffer *buffer;
    IMFMediaEvent *event;
    IUnknown *unk;
    HRESULT hr;
    GUID guid;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#x.\n", hr);

    hr = CoCreateInstance(&CLSID_VideoProcessorMFT, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTransform,
            (void **)&transform);
    if (FAILED(hr))
    {
        skip("Failed to create Video Processor instance, skipping tests.\n");
        goto failed;
    }

    hr = IMFTransform_QueryInterface(transform, &IID_IMFMediaEventGenerator, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_QueryInterface(transform, &IID_IMFShutdown, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    /* Transform global attributes. */
    hr = IMFTransform_GetAttributes(transform, &attributes);
    ok(hr == S_OK, "Failed to get attributes, hr %#x.\n", hr);
    hr = IMFTransform_GetAttributes(transform, &attributes2);
    ok(hr == S_OK, "Failed to get attributes, hr %#x.\n", hr);
    ok(attributes == attributes2, "Unexpected instance.\n");
    IMFAttributes_Release(attributes);
    IMFAttributes_Release(attributes2);

    hr = IMFTransform_GetStreamLimits(transform, &input_min, &input_max, &output_min, &output_max);
    ok(hr == S_OK, "Failed to get stream limits, hr %#x.\n", hr);
    ok(input_min == input_max && input_min == 1 && output_min == output_max && output_min == 1,
            "Unexpected stream limits.\n");

    hr = IMFTransform_GetStreamCount(transform, &input_count, &output_count);
    ok(hr == S_OK, "Failed to get stream count, hr %#x.\n", hr);
    ok(input_count == 1 && output_count == 1, "Unexpected stream count %u, %u.\n", input_count, output_count);

    hr = IMFTransform_GetStreamIDs(transform, 1, &input_id, 1, &output_id);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    input_id = 100;
    hr = IMFTransform_AddInputStreams(transform, 1, &input_id);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_DeleteInputStream(transform, 0);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputStatus(transform, 0, &flags);
todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputStreamAttributes(transform, 0, &attributes);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputStatus(transform, &flags);
todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputStreamAttributes(transform, 0, &attributes);
    ok(hr == S_OK, "Failed to get output attributes, hr %#x.\n", hr);
    hr = IMFTransform_GetOutputStreamAttributes(transform, 0, &attributes2);
    ok(hr == S_OK, "Failed to get output attributes, hr %#x.\n", hr);
    ok(attributes == attributes2, "Unexpected instance.\n");
    IMFAttributes_Release(attributes);
    IMFAttributes_Release(attributes2);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
todo_wine
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputCurrentType(transform, 1, &media_type);
todo_wine
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputCurrentType(transform, 1, &media_type);
todo_wine
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputStreamInfo(transform, 1, &input_info);
todo_wine
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    memset(&input_info, 0xcc, sizeof(input_info));
    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
todo_wine {
    ok(hr == S_OK, "Failed to get stream info, hr %#x.\n", hr);
    ok(input_info.dwFlags == 0, "Unexpected flag %#x.\n", input_info.dwFlags);
    ok(input_info.cbSize == 0, "Unexpected size %u.\n", input_info.cbSize);
    ok(input_info.cbMaxLookahead == 0, "Unexpected lookahead length %u.\n", input_info.cbMaxLookahead);
    ok(input_info.cbAlignment == 0, "Unexpected alignment %u.\n", input_info.cbAlignment);
}
    hr = MFCreateMediaEvent(MEUnknown, &GUID_NULL, S_OK, NULL, &event);
    ok(hr == S_OK, "Failed to create event object, hr %#x.\n", hr);
    hr = IMFTransform_ProcessEvent(transform, 0, event);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);
    hr = IMFTransform_ProcessEvent(transform, 1, event);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);
    IMFMediaEvent_Release(event);

    /* Configure stream types. */
    for (i = 0;;++i)
    {
        if (FAILED(hr = IMFTransform_GetInputAvailableType(transform, 0, i, &media_type)))
        {
        todo_wine
            ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#x.\n", hr);
            break;
        }

        hr = IMFTransform_GetInputAvailableType(transform, 0, i, &media_type2);
        ok(hr == S_OK, "Failed to get available type, hr %#x.\n", hr);
        ok(media_type != media_type2, "Unexpected instance.\n");
        IMFMediaType_Release(media_type2);

        hr = IMFMediaType_GetMajorType(media_type, &guid);
        ok(hr == S_OK, "Failed to get major type, hr %#x.\n", hr);
        ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected major type.\n");

        hr = IMFMediaType_GetCount(media_type, &count);
        ok(hr == S_OK, "Failed to get attributes count, hr %#x.\n", hr);
        ok(count == 2, "Unexpected count %u.\n", count);

        hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
        ok(hr == S_OK, "Failed to get subtype, hr %#x.\n", hr);
        ok(is_supported_video_type(&guid), "Unexpected media type %s.\n", wine_dbgstr_guid(&guid));

        hr = IMFTransform_SetInputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
        ok(FAILED(hr), "Unexpected hr %#x.\n", hr);

        hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
        ok(FAILED(hr), "Unexpected hr %#x.\n", hr);

        hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type2);
        ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

        /* FIXME: figure out if those require additional attributes or simply advertised but not supported */
        if (IsEqualGUID(&guid, &MFVideoFormat_L8) || IsEqualGUID(&guid, &MFVideoFormat_L16)
                || IsEqualGUID(&guid, &MFVideoFormat_D16) || IsEqualGUID(&guid, &MFVideoFormat_420O)
                || IsEqualGUID(&guid, &MFVideoFormat_A16B16G16R16F))
        {
            IMFMediaType_Release(media_type);
            continue;
        }

        hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, ((UINT64)16 << 32) | 16);
        ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

        hr = IMFTransform_SetInputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
        ok(hr == S_OK, "Failed to test input type %s, hr %#x.\n", wine_dbgstr_guid(&guid), hr);

        hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
        ok(hr == S_OK, "Failed to test input type, hr %#x.\n", hr);

        hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type2);
        ok(hr == S_OK, "Failed to get current type, hr %#x.\n", hr);
        ok(media_type != media_type2, "Unexpected instance.\n");
        IMFMediaType_Release(media_type2);

        hr = IMFTransform_GetInputStatus(transform, 0, &flags);
        ok(hr == S_OK, "Failed to get input status, hr %#x.\n", hr);
        ok(flags == MFT_INPUT_STATUS_ACCEPT_DATA, "Unexpected input status %#x.\n", flags);

        hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
        ok(hr == S_OK, "Failed to get stream info, hr %#x.\n", hr);
        ok(input_info.dwFlags == 0, "Unexpected flags %#x.\n", input_info.dwFlags);
        ok(input_info.cbMaxLookahead == 0, "Unexpected lookahead length %u.\n", input_info.cbMaxLookahead);
        ok(input_info.cbAlignment == 0, "Unexpected alignment %u.\n", input_info.cbAlignment);

        IMFMediaType_Release(media_type);
    }

    /* IYUV -> RGB32 */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_IYUV);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, ((UINT64)16 << 32) | 16);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
todo_wine
    ok(hr == S_OK, "Failed to set input type, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
todo_wine
    ok(hr == S_OK, "Failed to set output type, hr %#x.\n", hr);

    memset(&output_info, 0, sizeof(output_info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
todo_wine
    ok(hr == S_OK, "Failed to get stream info, hr %#x.\n", hr);
    ok(output_info.dwFlags == 0, "Unexpected flags %#x.\n", output_info.dwFlags);
todo_wine
    ok(output_info.cbSize > 0, "Unexpected size %u.\n", output_info.cbSize);
    ok(output_info.cbAlignment == 0, "Unexpected alignment %u.\n", output_info.cbAlignment);

    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#x.\n", hr);

    hr = MFCreateSample(&sample2);
    ok(hr == S_OK, "Failed to create a sample, hr %#x.\n", hr);

    memset(&output_buffer, 0, sizeof(output_buffer));
    output_buffer.pSample = sample;
    flags = 0;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output_buffer, &flags);
todo_wine
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "Unexpected hr %#x.\n", hr);
    ok(output_buffer.dwStatus == 0, "Unexpected buffer status, %#x.\n", output_buffer.dwStatus);
    ok(flags == 0, "Unexpected status %#x.\n", flags);

    hr = IMFTransform_ProcessInput(transform, 0, sample2, 0);
todo_wine
    ok(hr == S_OK, "Failed to push a sample, hr %#x.\n", hr);

    hr = IMFTransform_ProcessInput(transform, 0, sample2, 0);
todo_wine
    ok(hr == MF_E_NOTACCEPTING, "Unexpected hr %#x.\n", hr);

    memset(&output_buffer, 0, sizeof(output_buffer));
    output_buffer.pSample = sample;
    flags = 0;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output_buffer, &flags);
todo_wine
    ok(hr == MF_E_NO_SAMPLE_TIMESTAMP, "Unexpected hr %#x.\n", hr);
    ok(output_buffer.dwStatus == 0, "Unexpected buffer status, %#x.\n", output_buffer.dwStatus);
    ok(flags == 0, "Unexpected status %#x.\n", flags);

    hr = IMFSample_SetSampleTime(sample2, 0);
    ok(hr == S_OK, "Failed to set sample time, hr %#x.\n", hr);
    memset(&output_buffer, 0, sizeof(output_buffer));
    output_buffer.pSample = sample;
    flags = 0;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output_buffer, &flags);
todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);
    ok(output_buffer.dwStatus == 0, "Unexpected buffer status, %#x.\n", output_buffer.dwStatus);
    ok(flags == 0, "Unexpected status %#x.\n", flags);

    hr = MFCreateMemoryBuffer(1024 * 1024, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#x.\n", hr);

    hr = IMFSample_AddBuffer(sample2, buffer);
    ok(hr == S_OK, "Failed to add a buffer, hr %#x.\n", hr);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add a buffer, hr %#x.\n", hr);

    memset(&output_buffer, 0, sizeof(output_buffer));
    output_buffer.pSample = sample;
    flags = 0;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output_buffer, &flags);
todo_wine
    ok(hr == S_OK || broken(FAILED(hr)) /* Win8 */, "Failed to get output buffer, hr %#x.\n", hr);
    ok(output_buffer.dwStatus == 0, "Unexpected buffer status, %#x.\n", output_buffer.dwStatus);
    ok(flags == 0, "Unexpected status %#x.\n", flags);

    if (SUCCEEDED(hr))
    {
        memset(&output_buffer, 0, sizeof(output_buffer));
        output_buffer.pSample = sample;
        flags = 0;
        hr = IMFTransform_ProcessOutput(transform, 0, 1, &output_buffer, &flags);
        ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "Unexpected hr %#x.\n", hr);
        ok(output_buffer.dwStatus == 0, "Unexpected buffer status, %#x.\n", output_buffer.dwStatus);
        ok(flags == 0, "Unexpected status %#x.\n", flags);
    }

    IMFSample_Release(sample2);
    IMFSample_Release(sample);
    IMFMediaBuffer_Release(buffer);

    IMFMediaType_Release(media_type);

    IMFTransform_Release(transform);

failed:
    CoUninitialize();
}

static void test_quality_manager(void)
{
    IMFQualityManager *manager;
    HRESULT hr;

    hr = MFCreateStandardQualityManager(&manager);
    ok(hr == S_OK, "Failed to create quality manager, hr %#x.\n", hr);

    IMFQualityManager_Release(manager);
}

static void test_sar(void)
{
    IMFPresentationClock *present_clock, *present_clock2;
    IMFMediaType *mediatype, *mediatype2, *mediatype3;
    IMFMediaTypeHandler *handler, *handler2;
    IMFPresentationTimeSource *time_source;
    IMFSimpleAudioVolume *simple_volume;
    IMFAudioStreamVolume *stream_volume;
    IMFClockStateSink *state_sink;
    IMFMediaSink *sink, *sink2;
    IMFStreamSink *stream_sink;
    IMFAttributes *attributes;
    DWORD id, flags, count;
    IMFActivate *activate;
    MFCLOCK_STATE state;
    IMFClock *clock;
    IUnknown *unk;
    HRESULT hr;
    GUID guid;
    BOOL mute;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#x.\n", hr);

    hr = MFCreateAudioRenderer(NULL, &sink);
    if (hr == MF_E_NO_AUDIO_PLAYBACK_DEVICE)
    {
        skip("No audio playback device available.\n");
        CoUninitialize();
        return;
    }
    ok(hr == S_OK, "Failed to create renderer, hr %#x.\n", hr);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#x.\n", hr);

    hr = MFCreatePresentationClock(&present_clock);
    ok(hr == S_OK, "Failed to create presentation clock, hr %#x.\n", hr);

    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFPresentationTimeSource, (void **)&time_source);
todo_wine
    ok(hr == S_OK, "Failed to get time source interface, hr %#x.\n", hr);

if (SUCCEEDED(hr))
{
    hr = IMFPresentationTimeSource_GetUnderlyingClock(time_source, &clock);
    ok(hr == MF_E_NO_CLOCK, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetClockCharacteristics(time_source, &flags);
    ok(hr == S_OK, "Failed to get flags, hr %#x.\n", hr);
    ok(flags == MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ, "Unexpected flags %#x.\n", flags);

    hr = IMFPresentationTimeSource_GetState(time_source, 0, &state);
    ok(hr == S_OK, "Failed to get clock state, hr %#x.\n", hr);
    ok(state == MFCLOCK_STATE_INVALID, "Unexpected state %d.\n", state);

    hr = IMFPresentationTimeSource_QueryInterface(time_source, &IID_IMFClockStateSink, (void **)&state_sink);
    ok(hr == S_OK, "Failed to get state sink, hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockStart(state_sink, 0, 0);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    IMFClockStateSink_Release(state_sink);

    IMFPresentationTimeSource_Release(time_source);
}
    hr = IMFMediaSink_AddStreamSink(sink, 123, NULL, &stream_sink);
    ok(hr == MF_E_STREAMSINKS_FIXED, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_RemoveStreamSink(sink, 0);
    ok(hr == MF_E_STREAMSINKS_FIXED, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_GetStreamSinkCount(sink, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_GetStreamSinkCount(sink, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(flags == (MEDIASINK_FIXED_STREAMS | MEDIASINK_CAN_PREROLL), "Unexpected flags %#x.\n", flags);

    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFMediaSinkPreroll, (void **)&unk);
    ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);
    IUnknown_Release(unk);

    /* Events */
    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFMediaEventGenerator, (void **)&unk);
    ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);
    IUnknown_Release(unk);

    /* Clock */
    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFClockStateSink, (void **)&state_sink);
    ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockStart(state_sink, 0, 0);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockPause(state_sink, 0);
    ok(hr == MF_E_INVALID_STATE_TRANSITION, "Unexpected hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockStop(state_sink, 0);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockRestart(state_sink, 0);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    IMFClockStateSink_Release(state_sink);

    hr = IMFMediaSink_SetPresentationClock(sink, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_SetPresentationClock(sink, present_clock);
todo_wine
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    hr = MFCreateSystemTimeSource(&time_source);
    ok(hr == S_OK, "Failed to create time source, hr %#x.\n", hr);

    hr = IMFPresentationClock_SetTimeSource(present_clock, time_source);
    ok(hr == S_OK, "Failed to set time source, hr %#x.\n", hr);
    IMFPresentationTimeSource_Release(time_source);

    hr = IMFMediaSink_SetPresentationClock(sink, present_clock);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_GetPresentationClock(sink, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_GetPresentationClock(sink, &present_clock2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(present_clock == present_clock2, "Unexpected instance.\n");
    IMFPresentationClock_Release(present_clock2);

    /* Stream */
    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 0, &stream_sink);
    ok(hr == S_OK, "Failed to get a stream, hr %#x.\n", hr);

    hr = IMFStreamSink_QueryInterface(stream_sink, &IID_IMFMediaEventGenerator, (void **)&unk);
    ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);
    IUnknown_Release(unk);

    hr = IMFStreamSink_GetIdentifier(stream_sink, &id);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!id, "Unexpected id.\n");

    hr = IMFStreamSink_GetMediaSink(stream_sink, &sink2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(sink == sink2, "Unexpected object.\n");
    IMFMediaSink_Release(sink2);

    hr = IMFStreamSink_GetMediaTypeHandler(stream_sink, &handler);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFStreamSink_QueryInterface(stream_sink, &IID_IMFMediaTypeHandler, (void **)&handler2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(handler2 == handler, "Unexpected instance.\n");
    IMFMediaTypeHandler_Release(handler2);

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == S_OK, "Failed to get major type, hr %#x.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Audio), "Unexpected type %s.\n", wine_dbgstr_guid(&guid));

    count = 0;
    hr = IMFMediaTypeHandler_GetMediaTypeCount(handler, &count);
    ok(hr == S_OK, "Failed to get type count, hr %#x.\n", hr);
    ok(!!count, "Unexpected type count %u.\n", count);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &mediatype);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, mediatype, NULL);
todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, mediatype, NULL);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, mediatype);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get media type, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, 0, &mediatype3);
    ok(hr == S_OK, "Failed to get media type, hr %#x.\n", hr);
    ok(mediatype2 == mediatype3, "Unexpected instance.\n");
    IMFMediaType_Release(mediatype3);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, mediatype2, NULL);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);

    IMFMediaType_Release(mediatype);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, mediatype2);
    ok(hr == S_OK, "Failed to set current type, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &mediatype);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(mediatype == mediatype2, "Unexpected instance.\n");
    IMFMediaType_Release(mediatype);

    /* Type is validated against current type. */
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, mediatype2, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IMFMediaType_Release(mediatype2);

    /* Set partial type. */
    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = IMFMediaType_GetGUID(mediatype2, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Failed to get attribute, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, mediatype);
    ok(hr == S_OK, "Failed to set current type, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &mediatype2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(mediatype == mediatype2, "Unexpected instance.\n");
    IMFMediaType_Release(mediatype2);

    hr = IMFMediaType_GetCount(mediatype, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(count == 2, "Unexpected attribute count %u.\n", count);

    IMFMediaType_Release(mediatype);

    /* Reset back to uninitialized state. */
    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    IMFMediaTypeHandler_Release(handler);

    /* State change with initialized stream. */
    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFClockStateSink, (void **)&state_sink);
    ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockStart(state_sink, 0, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockStart(state_sink, 0, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockPause(state_sink, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockStop(state_sink, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockStop(state_sink, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockPause(state_sink, 0);
    ok(hr == MF_E_INVALID_STATE_TRANSITION, "Unexpected hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockRestart(state_sink, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockRestart(state_sink, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockStop(state_sink, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IMFClockStateSink_Release(state_sink);

    IMFStreamSink_Release(stream_sink);

    /* Volume control */
    hr = MFGetService((IUnknown *)sink, &MR_POLICY_VOLUME_SERVICE, &IID_IMFSimpleAudioVolume, (void **)&simple_volume);
    ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);

    hr = IMFSimpleAudioVolume_GetMute(simple_volume, &mute);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IMFSimpleAudioVolume_Release(simple_volume);

    hr = MFGetService((IUnknown *)sink, &MR_STREAM_VOLUME_SERVICE, &IID_IMFAudioStreamVolume, (void **)&stream_volume);
    ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);

    hr = IMFAudioStreamVolume_GetChannelCount(stream_volume, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFAudioStreamVolume_GetChannelCount(stream_volume, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    IMFAudioStreamVolume_Release(stream_volume);

    hr = MFGetService((IUnknown *)sink, &MR_AUDIO_POLICY_SERVICE, &IID_IMFAudioPolicy, (void **)&unk);
todo_wine
    ok(hr == MF_E_NOT_INITIALIZED, "Failed to get interface, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);

    /* Shutdown */
    EXPECT_REF(present_clock, 2);
    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
    EXPECT_REF(present_clock, 1);

    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_AddStreamSink(sink, 123, NULL, &stream_sink);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_RemoveStreamSink(sink, 0);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_GetStreamSinkCount(sink, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_GetStreamSinkCount(sink, &count);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_SetPresentationClock(sink, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_SetPresentationClock(sink, present_clock);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_GetPresentationClock(sink, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_GetPresentationClock(sink, &present_clock2);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    IMFMediaSink_Release(sink);

    /* Activation */
    hr = MFCreateAudioRendererActivate(&activate);
    ok(hr == S_OK, "Failed to create activation object, hr %#x.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Failed to activate, hr %#x.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink2);
    ok(hr == S_OK, "Failed to activate, hr %#x.\n", hr);
    ok(sink == sink2, "Unexpected instance.\n");
    IMFMediaSink_Release(sink2);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Failed to get sink flags, hr %#x.\n", hr);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    IMFMediaSink_Release(sink);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Failed to activate, hr %#x.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
todo_wine
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    IMFMediaSink_Release(sink);

    hr = IMFActivate_DetachObject(activate);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    IMFActivate_Release(activate);

    IMFPresentationClock_Release(present_clock);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#x.\n", hr);

    /* SAR attributes */
    hr = MFCreateAttributes(&attributes, 0);
    ok(hr == S_OK, "Failed to create attributes, hr %#x.\n", hr);

    /* Specify role. */
    hr = IMFAttributes_SetUINT32(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ROLE, eMultimedia);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = MFCreateAudioRenderer(attributes, &sink);
    ok(hr == S_OK, "Failed to create a sink, hr %#x.\n", hr);
    IMFMediaSink_Release(sink);

    /* Invalid endpoint. */
    hr = IMFAttributes_SetString(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ID, L"endpoint");
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = MFCreateAudioRenderer(attributes, &sink);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFAttributes_DeleteItem(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ROLE);
    ok(hr == S_OK, "Failed to remove attribute, hr %#x.\n", hr);

    hr = MFCreateAudioRenderer(attributes, &sink);
    ok(hr == MF_E_NO_AUDIO_PLAYBACK_DEVICE, "Failed to create a sink, hr %#x.\n", hr);

    CoUninitialize();
}

static void test_evr(void)
{
    IMFVideoSampleAllocatorCallback *allocator_callback;
    IMFStreamSink *stream_sink, *stream_sink2;
    IMFVideoDisplayControl *display_control;
    IMFMediaType *media_type, *media_type2;
    IMFMediaEventGenerator *ev_generator;
    IMFVideoSampleAllocator *allocator;
    IMFMediaTypeHandler *type_handler;
    IMFVideoRenderer *video_renderer;
    IMFClockStateSink *clock_sink;
    IMFMediaSinkPreroll *preroll;
    IMFMediaSink *sink, *sink2;
    IMFAttributes *attributes;
    IMFActivate *activate;
    DWORD flags, count;
    LONG sample_count;
    IMFGetService *gs;
    IMFSample *sample;
    IUnknown *unk;
    UINT64 value;
    HRESULT hr;
    GUID guid;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#x.\n", hr);

    hr = MFCreateVideoRenderer(&IID_IMFVideoRenderer, (void **)&video_renderer);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoRenderer_InitializeRenderer(video_renderer, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IMFVideoRenderer_Release(video_renderer);

    hr = MFCreateVideoRendererActivate(NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = MFCreateVideoRendererActivate(NULL, &activate);
    ok(hr == S_OK, "Failed to create activate object, hr %#x.\n", hr);

    hr = IMFActivate_GetCount(activate, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    hr = IMFActivate_GetUINT64(activate, &MF_ACTIVATE_VIDEO_WINDOW, &value);
    ok(hr == S_OK, "Failed to get attribute, hr %#x.\n", hr);
    ok(!value, "Unexpected value.\n");

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "Failed to activate, hr %#x.\n", hr);

    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFAttributes, (void **)&attributes);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFAttributes_QueryInterface(attributes, &IID_IMFMediaSink, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IUnknown_Release(unk);
    hr = IMFAttributes_GetCount(attributes, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!!count, "Unexpected count %u.\n", count);
    /* Rendering preferences are not immediately propagated to the presenter. */
    hr = IMFAttributes_SetUINT32(attributes, &EVRConfig_ForceBob, 1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = MFGetService((IUnknown *)sink, &MR_VIDEO_RENDER_SERVICE, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFVideoDisplayControl_GetRenderingPrefs(display_control, &flags);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!flags, "Unexpected flags %#x.\n", flags);
    IMFVideoDisplayControl_Release(display_control);
    IMFAttributes_Release(attributes);

    /* Primary stream type handler. */
    hr = IMFMediaSink_GetStreamSinkById(sink, 0, &stream_sink);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFStreamSink_GetMediaTypeHandler(stream_sink, &type_handler);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, &guid);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected type %s.\n", wine_dbgstr_guid(&guid));

    /* Supported types are not advertised. */
    hr = IMFMediaTypeHandler_GetMediaTypeCount(type_handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    count = 1;
    hr = IMFMediaTypeHandler_GetMediaTypeCount(type_handler, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!count, "Unexpected count %u.\n", count);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(type_handler, 0, NULL);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(type_handler, 0, &media_type);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(type_handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(type_handler, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)640 << 32 | 480);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type, &media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    media_type2 = (void *)0x1;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected media type %p.\n", media_type2);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, media_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(type_handler, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFMediaType_QueryInterface(media_type2, &IID_IMFVideoMediaType, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IUnknown_Release(unk);
    IMFMediaType_Release(media_type2);

    IMFMediaType_Release(media_type);

    IMFMediaTypeHandler_Release(type_handler);

    /* Stream uses an allocator. */
    hr = MFGetService((IUnknown *)stream_sink, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IMFVideoSampleAllocator,
            (void **)&allocator);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoSampleAllocator_QueryInterface(allocator, &IID_IMFVideoSampleAllocatorCallback, (void **)&allocator_callback);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    sample_count = 0;
    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_callback, &sample_count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!sample_count, "Unexpected sample count %d.\n", sample_count);

    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample);
todo_wine
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    IMFVideoSampleAllocatorCallback_Release(allocator_callback);
    IMFVideoSampleAllocator_Release(allocator);

    /* Same test for a substream. */
    hr = IMFMediaSink_AddStreamSink(sink, 1, NULL, &stream_sink2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = MFGetService((IUnknown *)stream_sink2, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IMFVideoSampleAllocator,
            (void **)&allocator);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IMFVideoSampleAllocator_Release(allocator);

    hr = IMFMediaSink_RemoveStreamSink(sink, 1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IMFStreamSink_Release(stream_sink2);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(flags == (MEDIASINK_CAN_PREROLL | MEDIASINK_CLOCK_REQUIRED), "Unexpected flags %#x.\n", flags);

    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFMediaSinkPreroll, (void **)&preroll);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IMFMediaSinkPreroll_Release(preroll);

    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFVideoRenderer, (void **)&video_renderer);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IMFVideoRenderer_Release(video_renderer);

    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFMediaEventGenerator, (void **)&ev_generator);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IMFMediaEventGenerator_Release(ev_generator);

    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFClockStateSink, (void **)&clock_sink);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IMFClockStateSink_Release(clock_sink);

    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFGetService, (void **)&gs);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFGetService_GetService(gs, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoMixerControl, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IUnknown_Release(unk);

    IMFGetService_Release(gs);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    /* Activate again. */
    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink2);
    ok(hr == S_OK, "Failed to activate, hr %#x.\n", hr);
todo_wine
    ok(sink == sink2, "Unexpected instance.\n");
    IMFMediaSink_Release(sink2);

    hr = IMFActivate_DetachObject(activate);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink2);
    ok(hr == S_OK, "Failed to activate, hr %#x.\n", hr);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    IMFMediaSink_Release(sink2);
    IMFMediaSink_Release(sink);

    IMFActivate_Release(activate);

    CoUninitialize();
}

static void test_MFCreateSimpleTypeHandler(void)
{
    IMFMediaType *media_type, *media_type2, *media_type3;
    IMFMediaTypeHandler *handler;
    DWORD count;
    HRESULT hr;
    GUID guid;

    hr = MFCreateSimpleTypeHandler(&handler);
    ok(hr == S_OK, "Failed to create object, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeCount(handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, NULL, NULL);
    ok(hr == MF_E_UNEXPECTED, "Unexpected hr %#x.\n", hr);

    count = 0;
    hr = IMFMediaTypeHandler_GetMediaTypeCount(handler, &count);
    ok(hr == S_OK, "Failed to get type count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    media_type = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!media_type, "Unexpected pointer.\n");

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, NULL);
    ok(hr == MF_E_UNEXPECTED, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type);
    ok(hr == S_OK, "Failed to set current type, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, 0, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(media_type2 == media_type, "Unexpected type.\n");
    IMFMediaType_Release(media_type2);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type, &media_type2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, 1, &media_type2);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(media_type == media_type2, "Unexpected pointer.\n");
    IMFMediaType_Release(media_type2);

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == S_OK, "Failed to get major type, hr %#x.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected major type.\n");

    hr = MFCreateMediaType(&media_type3);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type3, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    /* Different major types. */
    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type3, &media_type2);
    ok(hr == E_FAIL, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type3, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    /* Handler missing subtype. */
    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_SUBTYPE, &MFVideoFormat_RGB8);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type3, &media_type2);
    ok(hr == E_FAIL, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    /* Different subtypes. */
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB24);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type3, &media_type2);
    ok(hr == E_FAIL, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    /* Same major/subtype. */
    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_SUBTYPE, &MFVideoFormat_RGB24);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type3, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    /* Set one more attribute. */
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)4 << 32 | 4);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(handler, media_type3, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    IMFMediaType_Release(media_type3);
    IMFMediaType_Release(media_type);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, NULL);
    ok(hr == S_OK, "Failed to set current type, hr %#x.\n", hr);

    media_type = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!media_type, "Unexpected pointer.\n");

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    IMFMediaTypeHandler_Release(handler);
}

static void test_MFGetSupportedMimeTypes(void)
{
    PROPVARIANT value;
    HRESULT hr;

    hr = MFGetSupportedMimeTypes(NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    value.vt = VT_EMPTY;
    hr = MFGetSupportedMimeTypes(&value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(value.vt == (VT_VECTOR | VT_LPWSTR), "Unexpected value type %#x.\n", value.vt);

    PropVariantClear(&value);
}

static void test_MFGetSupportedSchemes(void)
{
    PROPVARIANT value;
    HRESULT hr;

    hr = MFGetSupportedSchemes(NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    value.vt = VT_EMPTY;
    hr = MFGetSupportedSchemes(&value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(value.vt == (VT_VECTOR | VT_LPWSTR), "Unexpected value type %#x.\n", value.vt);

    PropVariantClear(&value);
}

static BOOL is_sample_copier_available_type(IMFMediaType *type)
{
    GUID major = { 0 };
    UINT32 count;
    HRESULT hr;

    hr = IMFMediaType_GetMajorType(type, &major);
    ok(hr == S_OK, "Failed to get major type, hr %#x.\n", hr);

    hr = IMFMediaType_GetCount(type, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    return IsEqualGUID(&major, &MFMediaType_Video) || IsEqualGUID(&major, &MFMediaType_Audio);
}

static void test_sample_copier(void)
{
    DWORD in_min, in_max, out_min, out_max;
    IMFMediaType *mediatype, *mediatype2;
    MFT_OUTPUT_STREAM_INFO output_info;
    IMFSample *sample, *client_sample;
    MFT_INPUT_STREAM_INFO input_info;
    DWORD input_count, output_count;
    MFT_OUTPUT_DATA_BUFFER buffer;
    IMFMediaBuffer *media_buffer;
    IMFAttributes *attributes;
    IMFTransform *copier;
    DWORD flags, status;
    HRESULT hr;

    hr = MFCreateSampleCopierMFT(&copier);
    ok(hr == S_OK, "Failed to create sample copier, hr %#x.\n", hr);

    hr = IMFTransform_GetAttributes(copier, &attributes);
    ok(hr == S_OK, "Failed to get transform attributes, hr %#x.\n", hr);
    IMFAttributes_Release(attributes);

    hr = IMFTransform_GetInputStreamAttributes(copier, 0, &attributes);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputStreamAttributes(copier, 1, &attributes);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputStreamAttributes(copier, 0, &attributes);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputStreamAttributes(copier, 1, &attributes);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_SetOutputBounds(copier, 0, 0);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    /* No dynamic streams. */
    input_count = output_count = 0;
    hr = IMFTransform_GetStreamCount(copier, &input_count, &output_count);
    ok(hr == S_OK, "Failed to get stream count, hr %#x.\n", hr);
    ok(input_count == 1 && output_count == 1, "Unexpected streams count.\n");

    hr = IMFTransform_GetStreamLimits(copier, &in_min, &in_max, &out_min, &out_max);
    ok(hr == S_OK, "Failed to get stream limits, hr %#x.\n", hr);
    ok(in_min == in_max && in_min == 1 && out_min == out_max && out_min == 1, "Unexpected stream limits.\n");

    hr = IMFTransform_GetStreamIDs(copier, 1, &input_count, 1, &output_count);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_DeleteInputStream(copier, 0);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    /* Available types. */
    hr = IMFTransform_GetInputAvailableType(copier, 0, 0, &mediatype);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(is_sample_copier_available_type(mediatype), "Unexpected type.\n");
    IMFMediaType_Release(mediatype);

    hr = IMFTransform_GetInputAvailableType(copier, 0, 1, &mediatype);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(is_sample_copier_available_type(mediatype), "Unexpected type.\n");
    IMFMediaType_Release(mediatype);

    hr = IMFTransform_GetInputAvailableType(copier, 0, 2, &mediatype);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputAvailableType(copier, 1, 0, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputAvailableType(copier, 0, 0, &mediatype);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputAvailableType(copier, 1, 0, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputCurrentType(copier, 0, &mediatype);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputCurrentType(copier, 1, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputCurrentType(copier, 0, &mediatype);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputCurrentType(copier, 1, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#x.\n", hr);

    hr = IMFTransform_ProcessInput(copier, 0, sample, 0);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = IMFTransform_SetOutputType(copier, 0, mediatype, 0);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, &MFVideoFormat_RGB8);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaType_SetUINT64(mediatype, &MF_MT_FRAME_SIZE, ((UINT64)16) << 32 | 16);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFTransform_GetOutputStreamInfo(copier, 0, &output_info);
    ok(hr == S_OK, "Failed to get stream info, hr %#x.\n", hr);
    ok(!output_info.dwFlags, "Unexpected flags %#x.\n", output_info.dwFlags);
    ok(!output_info.cbSize, "Unexpected size %u.\n", output_info.cbSize);
    ok(!output_info.cbAlignment, "Unexpected alignment %u.\n", output_info.cbAlignment);

    hr = IMFTransform_GetInputStreamInfo(copier, 0, &input_info);
    ok(hr == S_OK, "Failed to get stream info, hr %#x.\n", hr);

    ok(!input_info.hnsMaxLatency, "Unexpected latency %s.\n", wine_dbgstr_longlong(input_info.hnsMaxLatency));
    ok(!input_info.dwFlags, "Unexpected flags %#x.\n", input_info.dwFlags);
    ok(!input_info.cbSize, "Unexpected size %u.\n", input_info.cbSize);
    ok(!input_info.cbMaxLookahead, "Unexpected lookahead size %u.\n", input_info.cbMaxLookahead);
    ok(!input_info.cbAlignment, "Unexpected alignment %u.\n", input_info.cbAlignment);

    hr = IMFTransform_SetOutputType(copier, 0, mediatype, 0);
    ok(hr == S_OK, "Failed to set input type, hr %#x.\n", hr);

    hr = IMFTransform_GetOutputStreamInfo(copier, 0, &output_info);
    ok(hr == S_OK, "Failed to get stream info, hr %#x.\n", hr);
    ok(!output_info.dwFlags, "Unexpected flags %#x.\n", output_info.dwFlags);
    ok(output_info.cbSize == 16 * 16, "Unexpected size %u.\n", output_info.cbSize);
    ok(!output_info.cbAlignment, "Unexpected alignment %u.\n", output_info.cbAlignment);

    hr = IMFTransform_GetOutputCurrentType(copier, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get current type, hr %#x.\n", hr);
    IMFMediaType_Release(mediatype2);

    hr = IMFTransform_GetInputCurrentType(copier, 0, &mediatype2);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputStatus(copier, 0, &flags);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    /* Setting input type resets output type. */
    hr = IMFTransform_GetOutputCurrentType(copier, 0, &mediatype2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IMFMediaType_Release(mediatype2);

    hr = IMFTransform_SetInputType(copier, 0, mediatype, 0);
    ok(hr == S_OK, "Failed to set input type, hr %#x.\n", hr);

    hr = IMFTransform_GetOutputCurrentType(copier, 0, &mediatype2);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputAvailableType(copier, 0, 1, &mediatype2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(is_sample_copier_available_type(mediatype2), "Unexpected type.\n");
    IMFMediaType_Release(mediatype2);

    hr = IMFTransform_GetInputStreamInfo(copier, 0, &input_info);
    ok(hr == S_OK, "Failed to get stream info, hr %#x.\n", hr);
    ok(!input_info.hnsMaxLatency, "Unexpected latency %s.\n", wine_dbgstr_longlong(input_info.hnsMaxLatency));
    ok(!input_info.dwFlags, "Unexpected flags %#x.\n", input_info.dwFlags);
    ok(input_info.cbSize == 16 * 16, "Unexpected size %u.\n", input_info.cbSize);
    ok(!input_info.cbMaxLookahead, "Unexpected lookahead size %u.\n", input_info.cbMaxLookahead);
    ok(!input_info.cbAlignment, "Unexpected alignment %u.\n", input_info.cbAlignment);

    hr = IMFTransform_GetOutputAvailableType(copier, 0, 0, &mediatype2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFMediaType_IsEqual(mediatype2, mediatype, &flags);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IMFMediaType_Release(mediatype2);

    hr = IMFTransform_GetInputStatus(copier, 0, &flags);
    ok(hr == S_OK, "Failed to get input status, hr %#x.\n", hr);
    ok(flags == MFT_INPUT_STATUS_ACCEPT_DATA, "Unexpected flags %#x.\n", flags);

    hr = IMFTransform_GetInputCurrentType(copier, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get current type, hr %#x.\n", hr);
    IMFMediaType_Release(mediatype2);

    hr = IMFTransform_GetOutputStatus(copier, &flags);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_SetOutputType(copier, 0, mediatype, 0);
    ok(hr == S_OK, "Failed to set output type, hr %#x.\n", hr);

    hr = IMFTransform_GetOutputStatus(copier, &flags);
    ok(hr == S_OK, "Failed to get output status, hr %#x.\n", hr);
    ok(!flags, "Unexpected flags %#x.\n", flags);

    /* Pushing samples. */
    hr = MFCreateAlignedMemoryBuffer(output_info.cbSize, output_info.cbAlignment, &media_buffer);
    ok(hr == S_OK, "Failed to create media buffer, hr %#x.\n", hr);

    hr = IMFSample_AddBuffer(sample, media_buffer);
    ok(hr == S_OK, "Failed to add a buffer, hr %#x.\n", hr);
    IMFMediaBuffer_Release(media_buffer);

    EXPECT_REF(sample, 1);
    hr = IMFTransform_ProcessInput(copier, 0, sample, 0);
    ok(hr == S_OK, "Failed to process input, hr %#x.\n", hr);
    EXPECT_REF(sample, 2);

    hr = IMFTransform_GetInputStatus(copier, 0, &flags);
    ok(hr == S_OK, "Failed to get input status, hr %#x.\n", hr);
    ok(!flags, "Unexpected flags %#x.\n", flags);

    hr = IMFTransform_GetOutputStatus(copier, &flags);
    ok(hr == S_OK, "Failed to get output status, hr %#x.\n", hr);
    ok(flags == MFT_OUTPUT_STATUS_SAMPLE_READY, "Unexpected flags %#x.\n", flags);

    hr = IMFTransform_ProcessInput(copier, 0, sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputStreamInfo(copier, 0, &output_info);
    ok(hr == S_OK, "Failed to get output info, hr %#x.\n", hr);

    hr = MFCreateAlignedMemoryBuffer(output_info.cbSize, output_info.cbAlignment, &media_buffer);
    ok(hr == S_OK, "Failed to create media buffer, hr %#x.\n", hr);

    hr = MFCreateSample(&client_sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#x.\n", hr);

    hr = IMFSample_AddBuffer(client_sample, media_buffer);
    ok(hr == S_OK, "Failed to add a buffer, hr %#x.\n", hr);
    IMFMediaBuffer_Release(media_buffer);

    status = 0;
    memset(&buffer, 0, sizeof(buffer));
    buffer.pSample = client_sample;
    hr = IMFTransform_ProcessOutput(copier, 0, 1, &buffer, &status);
    ok(hr == S_OK, "Failed to get output, hr %#x.\n", hr);
    EXPECT_REF(sample, 1);

    hr = IMFTransform_ProcessOutput(copier, 0, 1, &buffer, &status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "Failed to get output, hr %#x.\n", hr);

    /* Flushing. */
    hr = IMFTransform_ProcessInput(copier, 0, sample, 0);
    ok(hr == S_OK, "Failed to process input, hr %#x.\n", hr);
    EXPECT_REF(sample, 2);

    hr = IMFTransform_ProcessMessage(copier, MFT_MESSAGE_COMMAND_FLUSH, 0);
    ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);
    EXPECT_REF(sample, 1);

    IMFSample_Release(sample);
    IMFSample_Release(client_sample);

    IMFMediaType_Release(mediatype);
    IMFTransform_Release(copier);
}

static void test_MFGetTopoNodeCurrentType(void)
{
    IMFMediaType *media_type, *media_type2;
    IMFTopologyNode *node;
    HRESULT hr;

    /* Tee node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#x.\n", hr);

    hr = MFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = MFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = MFCreateMediaType(&media_type2);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type2, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    /* Input type returned, if set. */
    hr = IMFTopologyNode_SetInputPrefType(node, 0, media_type2);
    ok(hr == S_OK, "Failed to set media type, hr %#x.\n", hr);

    hr = MFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(media_type == media_type2, "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);

    hr = IMFTopologyNode_SetInputPrefType(node, 0, NULL);
    ok(hr == S_OK, "Failed to set media type, hr %#x.\n", hr);

    hr = MFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    /* Set second output. */
    hr = IMFTopologyNode_SetOutputPrefType(node, 1, media_type2);
    ok(hr == S_OK, "Failed to set media type, hr %#x.\n", hr);

    hr = MFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == E_FAIL, "Unexpected hr %#x.\n", hr);

    hr = IMFTopologyNode_SetOutputPrefType(node, 1, NULL);
    ok(hr == S_OK, "Failed to set media type, hr %#x.\n", hr);

    /* Set first output. */
    hr = IMFTopologyNode_SetOutputPrefType(node, 0, media_type2);
    ok(hr == S_OK, "Failed to set media type, hr %#x.\n", hr);

    hr = MFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(media_type == media_type2, "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);

    hr = IMFTopologyNode_SetOutputPrefType(node, 0, NULL);
    ok(hr == S_OK, "Failed to set media type, hr %#x.\n", hr);

    /* Set primary output. */
    hr = IMFTopologyNode_SetOutputPrefType(node, 1, media_type2);
    ok(hr == S_OK, "Failed to set media type, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_PRIMARYOUTPUT, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = MFGetTopoNodeCurrentType(node, 0, FALSE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(media_type == media_type2, "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);

    hr = MFGetTopoNodeCurrentType(node, 0, TRUE, &media_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(media_type == media_type2, "Unexpected pointer.\n");
    IMFMediaType_Release(media_type);

    IMFTopologyNode_Release(node);
    IMFMediaType_Release(media_type2);
}

START_TEST(mf)
{
    test_topology();
    test_topology_tee_node();
    test_topology_loader();
    test_MFGetService();
    test_sequencer_source();
    test_media_session();
    test_MFShutdownObject();
    test_presentation_clock();
    test_sample_grabber();
    test_video_processor();
    test_quality_manager();
    test_sar();
    test_evr();
    test_MFCreateSimpleTypeHandler();
    test_MFGetSupportedMimeTypes();
    test_MFGetSupportedSchemes();
    test_sample_copier();
    test_MFGetTopoNodeCurrentType();
}
