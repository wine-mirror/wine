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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>

#define COBJMACROS

#include "wine/test.h"
#include "uuids.h"
#include "dshow.h"
#include "control.h"

IGraphBuilder* pgraph;

static void createfiltergraph()
{
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (LPVOID*)&pgraph);
    ok(hr==S_OK, "Creating filtergraph returned: %lx\n", hr);
}

#if 0
static void renderfile()
{
    WCHAR file[] = {'t','e','s','t','.','a','v','i',0};
    HRESULT hr;

    hr = IGraphBuilder_RenderFile(pgraph, file, NULL);
    ok(hr==S_OK, "RenderFile returned: %lx\n", hr);
}

static void rungraph()
{
    HRESULT hr;
    IMediaControl* pmc;

    hr = IGraphBuilder_QueryInterface(pgraph, &IID_IMediaControl, (LPVOID*)&pmc);
    ok(hr==S_OK, "Cannot get IMediaControl interface returned: %lx\n", hr);

    hr = IMediaControl_Run(pmc);
    ok(hr==S_OK, "Cannot run the graph returned: %lx\n", hr);

    Sleep(20000); 
}
#endif

static void releasefiltergraph()
{
    HRESULT hr;

    hr = IGraphBuilder_Release(pgraph);
    ok(hr==S_OK, "Releasing filtergraph returned: %lx\n", hr);
}

START_TEST(filtergraph)
{
    createfiltergraph();
#if 0
    renderfile();
    rungraph();
#endif
    releasefiltergraph();
}
