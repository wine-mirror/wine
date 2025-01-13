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

static void test_spstream(void)
{
    ISpStream *stream;
    ISpMMSysAudio *mmaudio;
    IStream *base_stream, *base_stream2;
    GUID fmtid, fmtid2;
    WAVEFORMATEX *wfx = NULL, *wfx2 = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SpMMAudioOut, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpMMSysAudio, (void **)&mmaudio);
    ok(hr == S_OK, "Failed to create ISpMMSysAudio interface: %#lx.\n", hr);

    hr = ISpMMSysAudio_GetFormat(mmaudio, &fmtid, &wfx);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(IsEqualGUID(&fmtid, &SPDFID_WaveFormatEx), "got %s.\n", wine_dbgstr_guid(&fmtid));

    hr = ISpMMSysAudio_QueryInterface(mmaudio, &IID_IStream, (void **)&base_stream);
    ok(hr == S_OK, "Failed to get IStream interface from mmaudio: %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_SpStream, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpStream, (void **)&stream);
    ok(hr == S_OK, "Failed to create ISpStream interface: %#lx.\n", hr);

    hr = ISpStream_SetBaseStream(stream, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "got %#lx.\n", hr);

    hr = ISpStream_SetBaseStream(stream, base_stream, NULL, NULL);
    ok(hr == E_INVALIDARG, "got %#lx.\n", hr);

    hr = ISpStream_GetBaseStream(stream, &base_stream2);
    ok(hr == SPERR_UNINITIALIZED, "got %#lx.\n", hr);

    hr = ISpStream_SetBaseStream(stream, base_stream, &SPDFID_Text, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpStream_SetBaseStream(stream, base_stream, &fmtid, wfx);
    ok(hr == SPERR_ALREADY_INITIALIZED, "got %#lx.\n", hr);

    ISpStream_Release(stream);

    hr = CoCreateInstance(&CLSID_SpStream, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpStream, (void **)&stream);
    ok(hr == S_OK, "Failed to create ISpStream interface: %#lx.\n", hr);

    hr = ISpStream_GetFormat(stream, &fmtid2, &wfx2);
    ok(hr == SPERR_UNINITIALIZED, "got %#lx.\n", hr);

    hr = ISpStream_SetBaseStream(stream, base_stream, &SPDFID_WaveFormatEx, NULL);
    ok(hr == E_INVALIDARG, "got %#lx.\n", hr);

    hr = ISpStream_SetBaseStream(stream, base_stream, &SPDFID_WaveFormatEx, wfx);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpStream_GetBaseStream(stream, &base_stream2);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(base_stream2 == base_stream, "got %p.\n", base_stream2);
    IStream_Release(base_stream2);

    hr = ISpStream_GetFormat(stream, NULL, NULL);
    ok(hr == E_POINTER, "got %#lx.\n", hr);

    hr = ISpStream_GetFormat(stream, &fmtid2, NULL);
    ok(hr == E_POINTER, "got %#lx.\n", hr);

    hr = ISpStream_GetFormat(stream, &fmtid2, &wfx2);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(IsEqualGUID(&fmtid2, &SPDFID_WaveFormatEx), "got %s.\n", wine_dbgstr_guid(&fmtid2));
    ok(!memcmp(wfx, wfx2, sizeof(WAVEFORMATEX)), "wfx mismatch.\n");
    CoTaskMemFree(wfx2);

    hr = ISpStream_Close(stream);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpStream_SetBaseStream(stream, base_stream, &fmtid, wfx);
    ok(hr == SPERR_ALREADY_INITIALIZED, "got %#lx.\n", hr);

    hr = ISpStream_GetBaseStream(stream, &base_stream2);
    ok(hr == SPERR_STREAM_CLOSED, "got %#lx.\n", hr);

    hr = ISpStream_GetFormat(stream, &fmtid2, &wfx2);
    ok(hr == SPERR_STREAM_CLOSED, "got %#lx.\n", hr);

    hr = ISpStream_Close(stream);
    ok(hr == SPERR_STREAM_CLOSED, "got %#lx.\n", hr);

    ISpStream_Release(stream);
    IStream_Release(base_stream);
    ISpMMSysAudio_Release(mmaudio);
}

START_TEST(stream)
{
    CoInitialize(NULL);
    test_interfaces();
    test_spstream();
    CoUninitialize();
}
