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

#undef INITGUID
#include <guiddef.h>
#include "mfapi.h"
#include "mferror.h"
#include "mfidl.h"

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
    IMFCollection *collection, *collection2;
    IUnknown test_unk2 = { &test_unk_vtbl };
    IUnknown test_unk = { &test_unk_vtbl };
    IMFTopologyNode *node, *node2, *node3;
    IMFMediaType *mediatype, *mediatype2;
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

    hr = IMFTopologyNode_GetInputCount(node, &count);
    ok(hr == S_OK, "Failed to get output count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected count %u.\n", count);

    hr = IMFTopologyNode_SetInputPrefType(node, 1, mediatype);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#x.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 3, mediatype);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#x.\n", hr);

    hr = IMFTopologyNode_SetOutputPrefType(node, 4, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#x.\n", hr);

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

static void test_MFCreateSequencerSource(void)
{
    IMFSequencerSource *seq_source;
    HRESULT hr;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#x.\n", hr);

    hr = MFCreateSequencerSource(NULL, &seq_source);
    ok(hr == S_OK, "Failed to create sequencer source, hr %#x.\n", hr);

    IMFSequencerSource_Release(seq_source);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#x.\n", hr);
}

struct test_callback
{
    IMFAsyncCallback IMFAsyncCallback_iface;
    HANDLE event;
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
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    IMFMediaSession *session;
    IUnknown *state, *obj;
    HRESULT hr;

    ok(result != NULL, "Unexpected result object.\n");

    state = IMFAsyncResult_GetStateNoAddRef(result);
    if (state && SUCCEEDED(IUnknown_QueryInterface(state, &IID_IMFMediaSession, (void **)&session)))
    {
        IMFMediaEvent *event;

        hr = IMFMediaSession_EndGetEvent(session, result, &event);
        ok(hr == S_OK, "Failed to finalize GetEvent, hr %#x.\n", hr);

        hr = IMFAsyncResult_GetObject(result, &obj);
        ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

        IMFMediaEvent_Release(event);

        hr = IMFMediaSession_EndGetEvent(session, result, &event);
        ok(hr == E_FAIL, "Unexpected result, hr %#x.\n", hr);

        IMFMediaSession_Release(session);

        SetEvent(callback->event);
    }

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
    callback->event = NULL;
}

static void test_session_events(IMFMediaSession *session)
{
    struct test_callback callback, callback2;
    IMFAsyncResult *result;
    IMFMediaEvent *event;
    HRESULT hr;
    DWORD ret;

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

    callback.event = CreateEventA(NULL, FALSE, FALSE, NULL);

    hr = IMFMediaSession_QueueEvent(session, MEError, &GUID_NULL, E_FAIL, NULL);
    ok(hr == S_OK, "Failed to queue event, hr %#x.\n", hr);

    ret = WaitForSingleObject(callback.event, 100);
    ok(ret == WAIT_OBJECT_0, "Unexpected return value %#x.\n", ret);

    CloseHandle(callback.event);

    hr = MFCreateAsyncResult(NULL, &callback.IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#x.\n", hr);

    hr = IMFMediaSession_EndGetEvent(session, result, &event);
    ok(hr == E_FAIL, "Unexpected hr %#x.\n", hr);

    /* Shutdown behavior. */
    hr = IMFMediaSession_Shutdown(session);
todo_wine
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    hr = IMFMediaSession_GetEvent(session, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_QueueEvent(session, MEError, &GUID_NULL, E_FAIL, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_BeginGetEvent(session, &callback.IMFAsyncCallback_iface, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_BeginGetEvent(session, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_EndGetEvent(session, result, &event);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);
    IMFAsyncResult_Release(result);

    /* Already shut down. */
    hr = IMFMediaSession_Shutdown(session);
todo_wine
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);
}

static void test_media_session(void)
{
    MFCLOCK_PROPERTIES clock_props;
    IMFMediaSession *session;
    IMFRateControl *rc, *rc2;
    IMFRateSupport *rs;
    IMFGetService *gs;
    IMFClock *clock;
    IUnknown *unk;
    HRESULT hr;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#x.\n", hr);

    hr = MFCreateMediaSession(NULL, &session);
    ok(hr == S_OK, "Failed to create media session, hr %#x.\n", hr);

    hr = IMFMediaSession_QueryInterface(session, &IID_IMFAttributes, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSession_QueryInterface(session, &IID_IMFGetService, (void **)&gs);
    ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);

    hr = IMFGetService_GetService(gs, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport, (void **)&rs);
    ok(hr == S_OK, "Failed to get rate support interface, hr %#x.\n", hr);

    hr = IMFGetService_GetService(gs, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateControl, (void **)&rc);
    ok(hr == S_OK, "Failed to get rate control interface, hr %#x.\n", hr);

    hr = IMFRateSupport_QueryInterface(rs, &IID_IMFMediaSession, (void **)&unk);
    ok(hr == S_OK, "Failed to get session interface, hr %#x.\n", hr);
    ok(unk == (IUnknown *)session, "Unexpected pointer.\n");
    IUnknown_Release(unk);

    hr = IMFMediaSession_GetClock(session, &clock);
    ok(hr == S_OK, "Failed to get clock, hr %#x.\n", hr);

    hr = IMFClock_QueryInterface(clock, &IID_IMFRateControl, (void **)&rc2);
    ok(hr == S_OK, "Failed to get rate control, hr %#x.\n", hr);
    IMFRateControl_Release(rc2);

    hr = IMFClock_GetProperties(clock, &clock_props);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    IMFRateControl_Release(rc);
    IMFRateSupport_Release(rs);

    IMFGetService_Release(gs);

    test_session_events(session);

    IMFMediaSession_Release(session);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#x.\n", hr);
}

static void test_topology_loader(void)
{
    IMFTopoLoader *loader;
    HRESULT hr;

    hr = MFCreateTopoLoader(NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = MFCreateTopoLoader(&loader);
    ok(hr == S_OK, "Failed to create topology loader, hr %#x.\n", hr);

    IMFTopoLoader_Release(loader);
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
    IMFRateControl *rate_control;
    IMFPresentationClock *clock;
    MFCLOCK_PROPERTIES props;
    IMFShutdown *shutdown;
    LONGLONG clock_time;
    MFCLOCK_STATE state;
    IMFTimer *timer;
    MFTIME systime;
    unsigned int i;
    DWORD value;
    HRESULT hr;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#x.\n", hr);

    hr = MFCreatePresentationClock(&clock);
    ok(hr == S_OK, "Failed to create presentation clock, hr %#x.\n", hr);

    hr = IMFPresentationClock_GetTimeSource(clock, &time_source);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_GetClockCharacteristics(clock, &value);
todo_wine
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    value = 1;
    hr = IMFPresentationClock_GetContinuityKey(clock, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(value == 0, "Unexpected value %u.\n", value);

    hr = IMFPresentationClock_GetProperties(clock, &props);
    ok(hr == MF_E_CLOCK_NO_TIME_SOURCE, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationClock_GetState(clock, 0, &state);
    ok(hr == S_OK, "Failed to get state, hr %#x.\n", hr);
    ok(state == MFCLOCK_STATE_INVALID, "Unexpected state %d.\n", state);

    hr = IMFPresentationClock_GetCorrelatedTime(clock, 0, &clock_time, &systime);
todo_wine
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

    /* Set default time source. */
    hr = MFCreateSystemTimeSource(&time_source);
    ok(hr == S_OK, "Failed to create time source, hr %#x.\n", hr);

    hr = IMFPresentationClock_SetTimeSource(clock, time_source);
    ok(hr == S_OK, "Failed to set time source, hr %#x.\n", hr);

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

    IMFPresentationTimeSource_Release(time_source);

    hr = IMFPresentationClock_QueryInterface(clock, &IID_IMFRateControl, (void **)&rate_control);
    ok(hr == S_OK, "Failed to get rate control interface, hr %#x.\n", hr);
    IMFRateControl_Release(rate_control);

    hr = IMFPresentationClock_QueryInterface(clock, &IID_IMFTimer, (void **)&timer);
    ok(hr == S_OK, "Failed to get timer interface, hr %#x.\n", hr);
    IMFTimer_Release(timer);

    hr = IMFPresentationClock_QueryInterface(clock, &IID_IMFShutdown, (void **)&shutdown);
    ok(hr == S_OK, "Failed to get shutdown interface, hr %#x.\n", hr);
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
    return E_NOTIMPL;
}

static HRESULT WINAPI grabber_callback_OnProcessSample(IMFSampleGrabberSinkCallback *iface, REFGUID major_type,
        DWORD sample_flags, LONGLONG sample_time, LONGLONG sample_duration, const BYTE *buffer, DWORD sample_size)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI grabber_callback_OnShutdown(IMFSampleGrabberSinkCallback *iface)
{
    return E_NOTIMPL;
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
    IMFMediaType *media_type;
    IMFActivate *activate;
    ULONG refcount;
    HRESULT hr;

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = MFCreateSampleGrabberSinkActivate(NULL, NULL, &activate);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = MFCreateSampleGrabberSinkActivate(NULL, &grabber_callback, &activate);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = MFCreateSampleGrabberSinkActivate(media_type, &grabber_callback, &activate);
    ok(hr == S_OK, "Failed to create grabber activate, hr %#x.\n", hr);

    refcount = IMFMediaType_Release(media_type);
    ok(refcount == 1, "Unexpected refcount %u.\n", refcount);

    IMFActivate_Release(activate);
}

START_TEST(mf)
{
    test_topology();
    test_topology_loader();
    test_MFGetService();
    test_MFCreateSequencerSource();
    test_media_session();
    test_MFShutdownObject();
    test_presentation_clock();
    test_sample_grabber();
}
