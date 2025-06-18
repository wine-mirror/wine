/*
 * Speech API (SAPI) automation tests.
 *
 * Copyright 2019 Jactry Zeng for CodeWeavers
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

#define COBJMACROS

#include "sapiddk.h"
#include "sperror.h"

#include "wine/test.h"

static void test_interfaces(void)
{
    ISpeechFileStream *filestream;
    ISpeechBaseStream *basestream;
    IDispatch *dispatch;
    ISpStream *spstrem;
    IUnknown *unk;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SpFileStream, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpeechFileStream, (void **)&filestream);
    ok(hr == S_OK, "Failed to create ISpeechFileStream interface: %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_SpFileStream, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Failed to create IUnknown interface: %#lx.\n", hr);
    IUnknown_Release(unk);

    hr = CoCreateInstance(&CLSID_SpFileStream, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDispatch, (void **)&dispatch);
    ok(hr == S_OK, "Failed to create IDispatch interface: %#lx.\n", hr);
    IDispatch_Release(dispatch);

    hr = CoCreateInstance(&CLSID_SpFileStream, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpeechBaseStream, (void **)&basestream);
    ok(hr == S_OK, "Failed to create ISpeechBaseStream interface: %#lx.\n", hr);
    ISpeechBaseStream_Release(basestream);

    hr = CoCreateInstance(&CLSID_SpFileStream, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpStream, (void **)&spstrem);
    ok(hr == S_OK, "Failed to create ISpStream interface: %#lx.\n", hr);

    ISpStream_Release(spstrem);
    ISpeechFileStream_Release(filestream);
}

START_TEST(automation)
{
    CoInitialize(NULL);
    test_interfaces();
    CoUninitialize();
}
