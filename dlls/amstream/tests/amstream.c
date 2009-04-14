/*
 * Unit tests for MultiMedia Stream functions
 *
 * Copyright (C) 2009 Christian Costa
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
#include "initguid.h"
#include "amstream.h"

#define FILE_LEN 9
static const char fileA[FILE_LEN] = "test.avi";

IAMMultiMediaStream* pams;

static int create_ammultimediastream(void)
{
    return S_OK == CoCreateInstance(
        &CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMultiMediaStream, (LPVOID*)&pams);
}

static void release_ammultimediastream(void)
{
    IAMMultiMediaStream_Release(pams);
}

static void renderfile(const char * fileA)
{
    HRESULT hr;
    WCHAR fileW[FILE_LEN];
    IGraphBuilder* pgraph;

    MultiByteToWideChar(CP_ACP, 0, fileA, -1, fileW, FILE_LEN);

    hr = IAMMultiMediaStream_GetFilterGraph(pams, &pgraph);
    ok(hr==S_OK, "GetFilterGraph returned: %x\n", hr);
    ok(pgraph==NULL, "Filtergraph should not be created yet\n");

    if (pgraph)
        IGraphBuilder_Release(pgraph);

    hr = IAMMultiMediaStream_OpenFile(pams, fileW, 0);
    ok(hr==S_OK, "OpenFile returned: %x\n", hr);

    hr = IAMMultiMediaStream_GetFilterGraph(pams, &pgraph);
    ok(hr==S_OK, "GetFilterGraph returned: %x\n", hr);
    ok(pgraph!=NULL, "Filtergraph should be created\n");

    if (pgraph)
        IGraphBuilder_Release(pgraph);
}

static void test_render(void)
{
    HANDLE h;

    if (!create_ammultimediastream())
        return;

    h = CreateFileA(fileA, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        renderfile(fileA);
    }

    release_ammultimediastream();
}

START_TEST(amstream)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    test_render();
    CoUninitialize();
}
