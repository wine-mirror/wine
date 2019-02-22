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
#include "mfapi.h"
#include "mferror.h"
#include "mfidl.h"

#include "wine/test.h"

static void test_topology(void)
{
    IMFTopologyNode *node, *node2;
    IMFTopology *topology;
    WORD count;
    HRESULT hr;
    TOPOID id;

    hr = MFCreateTopology(NULL);
    ok(hr == E_POINTER, "got %#x\n", hr);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "got %#x\n", hr);

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

    count = 1;
    hr = IMFTopology_GetNodeCount(topology, &count);
todo_wine {
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected node count %u.\n", count);
}
    /* Same id, different nodes. */
    hr = IMFTopology_AddNode(topology, node);
todo_wine
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);

    count = 0;
    hr = IMFTopology_GetNodeCount(topology, &count);
todo_wine {
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected node count %u.\n", count);
}
    hr = IMFTopology_AddNode(topology, node2);
todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);
    IMFTopologyNode_Release(node2);

    hr = IMFTopology_GetNodeByID(topology, id, &node2);
todo_wine {
    ok(hr == S_OK, "Failed to get a node, hr %#x.\n", hr);
    ok(node2 == node, "Unexpected node.\n");
}
    if (SUCCEEDED(hr))
        IMFTopologyNode_Release(node2);

    /* Change node id, add it again. */
    hr = IMFTopologyNode_SetTopoNodeID(node, ++id);
    ok(hr == S_OK, "Failed to set node id, hr %#x.\n", hr);

    hr = IMFTopology_GetNodeByID(topology, id, &node2);
todo_wine {
    ok(hr == S_OK, "Failed to get a node, hr %#x.\n", hr);
    ok(node2 == node, "Unexpected node.\n");
}
    if (SUCCEEDED(hr))
        IMFTopologyNode_Release(node2);

    hr = IMFTopology_GetNodeByID(topology, id + 1, &node2);
todo_wine
    ok(hr == MF_E_NOT_FOUND, "Unexpected hr %#x.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
todo_wine
    ok(hr == E_INVALIDARG, "Failed to add a node, hr %#x.\n", hr);

    hr = IMFTopology_GetNode(topology, 0, &node2);
todo_wine {
    ok(hr == S_OK, "Failed to get a node, hr %#x.\n", hr);
    ok(node2 == node, "Unexpected node.\n");
}
    if (SUCCEEDED(hr))
        IMFTopologyNode_Release(node2);

    hr = IMFTopology_GetNode(topology, 1, &node2);
todo_wine
    ok(hr == MF_E_INVALIDINDEX, "Failed to get a node, hr %#x.\n", hr);

    hr = IMFTopology_GetNode(topology, -2, &node2);
todo_wine
    ok(hr == MF_E_INVALIDINDEX, "Failed to get a node, hr %#x.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);
    hr = IMFTopology_AddNode(topology, node2);
todo_wine
    ok(hr == S_OK, "Failed to add a node, hr %#x.\n", hr);
    IMFTopologyNode_Release(node2);

    count = 0;
    hr = IMFTopology_GetNodeCount(topology, &count);
todo_wine {
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(count == 2, "Unexpected node count %u.\n", count);
}
    /* Remove with detached node, existing id. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#x.\n", hr);
    hr = IMFTopologyNode_SetTopoNodeID(node2, id);
    ok(hr == S_OK, "Failed to set node id, hr %#x.\n", hr);
    hr = IMFTopology_RemoveNode(topology, node2);
todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);
    IMFTopologyNode_Release(node2);

    hr = IMFTopology_RemoveNode(topology, node);
todo_wine
    ok(hr == S_OK, "Failed to remove a node, hr %#x.\n", hr);

    count = 0;
    hr = IMFTopology_GetNodeCount(topology, &count);
todo_wine {
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected node count %u.\n", count);
}
    hr = IMFTopology_Clear(topology);
todo_wine
    ok(hr == S_OK, "Failed to clear topology, hr %#x.\n", hr);

    count = 1;
    hr = IMFTopology_GetNodeCount(topology, &count);
todo_wine {
    ok(hr == S_OK, "Failed to get node count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected node count %u.\n", count);
}
    hr = IMFTopology_Clear(topology);
todo_wine
    ok(hr == S_OK, "Failed to clear topology, hr %#x.\n", hr);

    hr = IMFTopologyNode_SetTopoNodeID(node, 123);
    ok(hr == S_OK, "Failed to set node id, hr %#x.\n", hr);

    IMFTopologyNode_Release(node);
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

START_TEST(mf)
{
    test_topology();
    test_MFGetService();
    test_MFCreateSequencerSource();
}
