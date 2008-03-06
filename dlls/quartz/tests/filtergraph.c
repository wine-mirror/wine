/*
 * Unit tests for Direct Show functions
 *
 * Copyright (C) 2004 Christian Costa
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

#include <assert.h>

#define COBJMACROS

#include "wine/test.h"
#include "uuids.h"
#include "dshow.h"
#include "control.h"

static const WCHAR file[] = {'t','e','s','t','.','a','v','i',0};

IGraphBuilder* pgraph;

static int createfiltergraph(void)
{
    return S_OK == CoCreateInstance(
        &CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (LPVOID*)&pgraph);
}

static void renderfile(void)
{
    HRESULT hr;

    hr = IGraphBuilder_RenderFile(pgraph, file, NULL);
    ok(hr==S_OK, "RenderFile returned: %x\n", hr);
}

static void rungraph(void)
{
    HRESULT hr;
    IMediaControl* pmc;
    IMediaEvent* pme;
    HANDLE hEvent;

    hr = IGraphBuilder_QueryInterface(pgraph, &IID_IMediaControl, (LPVOID*)&pmc);
    ok(hr==S_OK, "Cannot get IMediaControl interface returned: %x\n", hr);

    hr = IMediaControl_Run(pmc);
    ok(hr==S_FALSE, "Cannot run the graph returned: %x\n", hr);

    hr = IGraphBuilder_QueryInterface(pgraph, &IID_IMediaEvent, (LPVOID*)&pme);
    ok(hr==S_OK, "Cannot get IMediaEvent interface returned: %x\n", hr);

    hr = IMediaEvent_GetEventHandle(pme, (OAEVENT*)&hEvent);
    ok(hr==S_OK, "Cannot get event handle returned: %x\n", hr);

    /* WaitForSingleObject(hEvent, INFINITE); */
    Sleep(20000);

    hr = IMediaControl_Release(pme);
    ok(hr==2, "Releasing mediaevent returned: %x\n", hr);

    hr = IMediaControl_Stop(pmc);
    ok(hr==S_OK, "Cannot stop the graph returned: %x\n", hr);
    
    hr = IMediaControl_Release(pmc);
    ok(hr==1, "Releasing mediacontrol returned: %x\n", hr);
}

static void releasefiltergraph(void)
{
    HRESULT hr;

    hr = IGraphBuilder_Release(pgraph);
    ok(hr==0, "Releasing filtergraph returned: %x\n", hr);
}

static void test_render_run(void)
{
    HANDLE h;

    if (!createfiltergraph())
        return;

    h = CreateFileW(file, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        renderfile();
        rungraph();
    }

    releasefiltergraph();
}

static void test_graph_builder(void)
{
    HRESULT hr;
    IBaseFilter *pF = NULL;
    IBaseFilter *pF2 = NULL;
    IPin *pIn = NULL;
    IEnumPins *pEnum = NULL;
    PIN_DIRECTION dir;
    static const WCHAR testFilterW[] = {'t','e','s','t','F','i','l','t','e','r',0};
    static const WCHAR fooBarW[] = {'f','o','o','B','a','r',0};

    if (!createfiltergraph())
        return;

    /* create video filter */
    hr = CoCreateInstance(&CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (LPVOID*)&pF);
    ok(hr == S_OK, "CoCreateInstance failed with %x\n", hr);
    ok(pF != NULL, "pF is NULL\n");

    /* add the two filters to the graph */
    hr = IGraphBuilder_AddFilter(pgraph, pF, testFilterW);
    ok(hr == S_OK, "failed to add pF to the graph: %x\n", hr);

    /* find the pins */
    hr = IBaseFilter_EnumPins(pF, &pEnum);
    ok(hr == S_OK, "IBaseFilter_EnumPins failed for pF: %x\n", hr);
    ok(pEnum != NULL, "pEnum is NULL\n");
    hr = IEnumPins_Next(pEnum, 1, &pIn, NULL);
    ok(hr == S_OK, "IEnumPins_Next failed for pF: %x\n", hr);
    ok(pIn != NULL, "pIn is NULL\n");
    hr = IPin_QueryDirection(pIn, &dir);
    ok(hr == S_OK, "IPin_QueryDirection failed: %x\n", hr);
    ok(dir == PINDIR_INPUT, "pin has wrong direction\n");

    hr = IGraphBuilder_FindFilterByName(pgraph, fooBarW, &pF2);
    ok(hr == VFW_E_NOT_FOUND, "IGraphBuilder_FindFilterByName returned %x\n", hr);
    ok(pF2 == NULL, "IGraphBuilder_FindFilterByName returned %p\n", pF2);
    hr = IGraphBuilder_FindFilterByName(pgraph, testFilterW, &pF2);
    ok(hr == S_OK, "IGraphBuilder_FindFilterByName returned %x\n", hr);
    ok(pF2 != NULL, "IGraphBuilder_FindFilterByName returned NULL\n");
    hr = IGraphBuilder_FindFilterByName(pgraph, testFilterW, NULL);
    ok(hr == E_POINTER, "IGraphBuilder_FindFilterByName returned %x\n", hr);
    releasefiltergraph();
}

static void test_graph_builder_addfilter(void)
{
    HRESULT hr;
    IBaseFilter *pF = NULL;
    static const WCHAR testFilterW[] = {'t','e','s','t','F','i','l','t','e','r',0};

    if (!createfiltergraph())
        return;

    hr = IGraphBuilder_AddFilter(pgraph, NULL, testFilterW);
    ok(hr == E_POINTER, "IGraphBuilder_AddFilter returned: %x\n", hr);

    /* create video filter */
    hr = CoCreateInstance(&CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (LPVOID*)&pF);
    ok(hr == S_OK, "CoCreateInstance failed with %x\n", hr);
    ok(pF != NULL, "pF is NULL\n");
    if (!pF) {
        skip("failed to created filter, skipping\n");
        return;
    }

    hr = IGraphBuilder_AddFilter(pgraph, pF, NULL);
    ok(hr == S_OK, "IGraphBuilder_AddFilter returned: %x\n", hr);
    releasefiltergraph();
}

static void test_filter_graph2(void)
{
    HRESULT hr;
    IFilterGraph2 *pF = NULL;

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (LPVOID*)&pF);
    ok(hr == S_OK, "CoCreateInstance failed with %x\n", hr);
    ok(pF != NULL, "pF is NULL\n");

    hr = IFilterGraph2_Release(pF);
    ok(hr == 0, "IFilterGraph2_Release returned: %x\n", hr);
}

START_TEST(filtergraph)
{
    CoInitialize(NULL);
    test_render_run();
    test_graph_builder();
    test_graph_builder_addfilter();
    test_filter_graph2();
}
