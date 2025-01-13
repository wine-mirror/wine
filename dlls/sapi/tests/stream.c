/*
 * Speech API (SAPI) stream tests.
 *
 * Copyright 2020 Gijs Vermeulen
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

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown *obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "Unexpected refcount %ld, expected %ld.\n", rc, ref);
}

static void test_interfaces(void)
{
    ISpStreamFormat *stream_format;
    ISequentialStream *seq_stream;
    ISpStream *speech_stream;
    IDispatch *dispatch;
    IStream *stream;
    IUnknown *unk;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SpStream, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpStream, (void **)&speech_stream);
    ok(hr == S_OK, "Failed to create ISpStream interface: %#lx.\n", hr);
    EXPECT_REF(speech_stream, 1);

    hr = CoCreateInstance(&CLSID_SpStream, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Failed to create IUnknown interface: %#lx.\n", hr);
    EXPECT_REF(unk, 1);
    EXPECT_REF(speech_stream, 1);
    IUnknown_Release(unk);

    hr = CoCreateInstance(&CLSID_SpStream, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDispatch, (void **)&dispatch);
    ok(hr == E_NOINTERFACE, "Succeeded to create IDispatch interface: %#lx.\n", hr);
    ok(!dispatch, "Expected NULL dispatch, got %p.", dispatch);

    hr = CoCreateInstance(&CLSID_SpStream, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISequentialStream, (void **)&seq_stream);
    ok(hr == S_OK, "Failed to create ISequentialStream interface: %#lx.\n", hr);
    EXPECT_REF(seq_stream, 1);
    EXPECT_REF(speech_stream, 1);
    ISequentialStream_Release(seq_stream);

    hr = CoCreateInstance(&CLSID_SpStream, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IStream, (void **)&stream);
    ok(hr == S_OK, "Failed to create IStream interface: %#lx.\n", hr);
    EXPECT_REF(stream, 1);
    EXPECT_REF(speech_stream, 1);
    IStream_Release(stream);

    hr = CoCreateInstance(&CLSID_SpStream, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpStreamFormat, (void **)&stream_format);
    ok(hr == S_OK, "Failed to create ISpStreamFormat interface: %#lx.\n", hr);
    EXPECT_REF(stream_format, 1);
    EXPECT_REF(speech_stream, 1);
    ISpStreamFormat_Release(stream_format);


    ISpStream_Release(speech_stream);
}

START_TEST(stream)
{
    CoInitialize(NULL);
    test_interfaces();
    CoUninitialize();
}
