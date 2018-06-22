/*
 * Copyright 2018 Zebediah Figura
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
#include "windef.h"
#include "wingdi.h"
#include "mmreg.h"
#include "mmsystem.h"
#include "mediaerr.h"
#include "wmcodecdsp.h"
#include "uuids.h"
#include "wine/test.h"

#include "initguid.h"
DEFINE_GUID(WMMEDIASUBTYPE_MP3, 0x00000055, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

/* shorter aliases for flags */
#define O_SYNCPOINT  DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT
#define O_TIME       DMO_OUTPUT_DATA_BUFFERF_TIME
#define O_TIMELENGTH DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH
#define O_INCOMPLETE DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE

static REFERENCE_TIME samplelen(DWORD samples, int rate)
{
    return (REFERENCE_TIME) 10000000 * samples / rate;
}

struct test_buffer {
    IMediaBuffer IMediaBuffer_iface;
    BYTE data[5000];
    DWORD len;
    DWORD maxlen;
};

static inline struct test_buffer *impl_from_IMediaBuffer(IMediaBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct test_buffer, IMediaBuffer_iface);
}

static HRESULT WINAPI Buffer_QueryInterface(IMediaBuffer *iface, REFIID iid, void **obj)
{
    if (IsEqualIID(iid, &IID_IMediaBuffer) || IsEqualIID(iid, &IID_IUnknown))
    {
        *obj = iface;
        return S_OK;
    }
    ok(0, "Unexpected IID %s\n", wine_dbgstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Buffer_AddRef(IMediaBuffer *iface)
{
    return 2;
}

static ULONG WINAPI Buffer_Release(IMediaBuffer *iface)
{
    return 1;
}

static HRESULT WINAPI Buffer_SetLength(IMediaBuffer *iface, DWORD len)
{
    struct test_buffer *This = impl_from_IMediaBuffer(iface);
    This->len = len;
    return S_OK;
}

static HRESULT WINAPI Buffer_GetMaxLength(IMediaBuffer *iface, DWORD *len)
{
    struct test_buffer *This = impl_from_IMediaBuffer(iface);
    *len = This->maxlen;
    return S_OK;
}

static HRESULT WINAPI Buffer_GetBufferAndLength(IMediaBuffer *iface, BYTE **buf, DWORD *len)
{
    struct test_buffer *This = impl_from_IMediaBuffer(iface);
    if (buf) *buf = This->data;
    if (len) *len = This->len;
    return S_OK;
}

static IMediaBufferVtbl Buffer_vtbl = {
    Buffer_QueryInterface,
    Buffer_AddRef,
    Buffer_Release,
    Buffer_SetLength,
    Buffer_GetMaxLength,
    Buffer_GetBufferAndLength,
};

static void test_convert(void)
{
    static const BYTE mp3hdr[] = {0xff,0xfb,0x14,0xc4};
    struct test_buffer outbuf = {{&Buffer_vtbl}};
    struct test_buffer inbuf = {{&Buffer_vtbl}};
    DMO_MEDIA_TYPE in = {{0}}, out = {{0}};
    MPEGLAYER3WAVEFORMAT mp3fmt = {{0}};
    DMO_OUTPUT_DATA_BUFFER output;
    WAVEFORMATEX wavfmt = {0};
    IMediaObject *dmo;
    DWORD written;
    DWORD status;
    HRESULT hr;
    int i;

    hr = CoCreateInstance(&CLSID_CMP3DecMediaObject, NULL, CLSCTX_INPROC_SERVER,
        &IID_IMediaObject, (void **)&dmo);
    ok(hr == S_OK, "got %#x\n", hr);

    mp3fmt.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
    mp3fmt.wfx.nChannels = 2;
    mp3fmt.wfx.nSamplesPerSec = 48000;
    in.majortype = MEDIATYPE_Audio;
    in.subtype = WMMEDIASUBTYPE_MP3;
    in.formattype = FORMAT_WaveFormatEx;
    in.cbFormat = sizeof(mp3fmt);
    in.pbFormat = (BYTE *)&mp3fmt;

    hr = IMediaObject_SetInputType(dmo, 0, &in, 0);
    ok(hr == S_OK, "got %#x\n", hr);

    wavfmt.wFormatTag = WAVE_FORMAT_PCM;
    wavfmt.nChannels = 1;
    wavfmt.nSamplesPerSec = 48000;
    wavfmt.nAvgBytesPerSec = 48000 * ((1 * 8) / 8);
    wavfmt.nBlockAlign = (1 * 8) / 8;
    wavfmt.wBitsPerSample = 8;
    out.majortype = MEDIATYPE_Audio;
    out.subtype = MEDIASUBTYPE_PCM;
    out.formattype = FORMAT_WaveFormatEx;
    out.cbFormat = sizeof(wavfmt);
    out.pbFormat = (BYTE *)&wavfmt;

    hr = IMediaObject_SetOutputType(dmo, 0, &out, 0);
    ok(hr == S_OK, "got %#x\n", hr);

    outbuf.len = 0;
    outbuf.maxlen = sizeof(outbuf.data);
    output.pBuffer = &outbuf.IMediaBuffer_iface;
    output.dwStatus = 0xdeadbeef;
    output.rtTimestamp = 0xdeadbeef;
    output.rtTimelength = 0xdeadbeef;
    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_FALSE, "got %#x\n", hr);
    ok(outbuf.len == 0, "got %u\n", outbuf.len);
    ok(output.dwStatus == 0, "got %#x\n", output.dwStatus);
    ok(output.rtTimestamp == 0xdeadbeef, "got %s\n", wine_dbgstr_longlong(output.rtTimestamp));
    ok(output.rtTimelength == 0xdeadbeef, "got %s\n", wine_dbgstr_longlong(output.rtTimelength));

    /* write several frames of mp3 data */
    for (i = 0; i < 5; i++)
        memcpy(inbuf.data + 96 * i, mp3hdr, 4);
    inbuf.len = 96 * 5;
    hr = IMediaObject_ProcessInput(dmo, 0, &inbuf.IMediaBuffer_iface, 0, 0, 0);
    ok(hr == S_OK, "got %#x\n", hr);

    hr = IMediaObject_ProcessInput(dmo, 0, &inbuf.IMediaBuffer_iface, 0, 0, 0);
    ok(hr == DMO_E_NOTACCEPTING, "got %#x\n", hr);

    outbuf.len = 0;
    outbuf.maxlen = 0;
    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_FALSE, "got %#x\n", hr);
    ok(outbuf.len == 0, "got %u\n", outbuf.len);
    ok(output.dwStatus == (O_SYNCPOINT | O_INCOMPLETE), "got %#x\n", output.dwStatus);
    ok(output.rtTimestamp == 0xdeadbeef, "got %s\n", wine_dbgstr_longlong(output.rtTimestamp));
    ok(output.rtTimelength == 0xdeadbeef, "got %s\n", wine_dbgstr_longlong(output.rtTimelength));

    /* implementations are inconsistent, but should write at least one frame of data */
    outbuf.len = 0;
    outbuf.maxlen = 5000;
    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_OK, "got %#x\n", hr);
    written = outbuf.len;
    ok(written > 1152 && written <= 5000, "got %u\n", written);
    ok(output.dwStatus == (O_SYNCPOINT | O_TIME | O_TIMELENGTH | O_INCOMPLETE),
        "got %#x\n", output.dwStatus);
    ok(output.rtTimestamp == 0, "got %s\n", wine_dbgstr_longlong(output.rtTimestamp));
    ok(output.rtTimelength == samplelen(written, 48000),
        "got %s\n", wine_dbgstr_longlong(output.rtTimelength));

    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_FALSE, "got %#x\n", hr);
    ok(outbuf.len == written, "expected %u, got %u\n", written, outbuf.len);
    ok(output.dwStatus == (O_SYNCPOINT | O_INCOMPLETE), "got %#x\n", output.dwStatus);
    ok(output.rtTimestamp == 0, "got %s\n", wine_dbgstr_longlong(output.rtTimestamp));
    ok(output.rtTimelength == samplelen(written, 48000),
        "got %s\n", wine_dbgstr_longlong(output.rtTimelength));

    hr = IMediaObject_ProcessInput(dmo, 0, &inbuf.IMediaBuffer_iface, 0, 0, 0);
    ok(hr == DMO_E_NOTACCEPTING, "got %#x\n", hr);

    /* write the rest */
    outbuf.len = 0;
    outbuf.maxlen = 5000;
    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(written + outbuf.len == (1152 * 5) ||
        broken(written + outbuf.len == (1152 * 5) - 528), /* Win10 */
        "got %u, total %u\n", outbuf.len, written + outbuf.len);
    ok(output.dwStatus == (O_SYNCPOINT | O_TIME | O_TIMELENGTH), "got %#x\n", output.dwStatus);
    ok(output.rtTimestamp == samplelen(written, 48000),
        "got %s\n", wine_dbgstr_longlong(output.rtTimestamp));
    ok(output.rtTimelength == samplelen(outbuf.len, 48000),
        "got %s\n", wine_dbgstr_longlong(output.rtTimelength));

    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_FALSE, "got %#x\n", hr);
    ok(output.dwStatus == 0, "got %#x\n", output.dwStatus);

    hr = IMediaObject_ProcessInput(dmo, 0, &inbuf.IMediaBuffer_iface, 0, 0, 0);
    ok(hr == S_OK, "got %#x\n", hr);

    IMediaObject_Release(dmo);
}

static const GUID IID_test_outer = {0xdeadbeef,0,0,{0,0,0,0,0,0,0,0x66}};

static HRESULT WINAPI Outer_QueryInterface(IUnknown *iface, REFIID iid, void **obj)
{
    if (IsEqualGUID(iid, &IID_test_outer))
    {
        *obj = (IUnknown *)0xdeadbeef;
        return S_OK;
    }
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Outer_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI Outer_Release(IUnknown *iface)
{
    return 1;
}

static IUnknownVtbl Outer_vtbl = {
    Outer_QueryInterface,
    Outer_AddRef,
    Outer_Release,
};

static IUnknown Outer = { &Outer_vtbl };

static void test_aggregation(void)
{
    IUnknown *unk, *unk2;
    IMediaObject *dmo;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_CMP3DecMediaObject, &Outer, CLSCTX_INPROC_SERVER,
        &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "got %#x\n", hr);

    hr = IUnknown_QueryInterface(unk, &IID_IMediaObject, (void **)&dmo);
    ok(hr == S_OK, "got %#x\n", hr);

    hr = IMediaObject_QueryInterface(dmo, &IID_test_outer, (void **)&unk2);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "got unk %p\n", unk2);

    IUnknown_Release(dmo);
    IUnknown_Release(unk);

    hr = CoCreateInstance(&CLSID_CMP3DecMediaObject, &Outer, CLSCTX_INPROC_SERVER,
        &IID_IMediaObject, (void **)&unk);
    ok(hr == E_NOINTERFACE, "got %#x\n", hr);
}

START_TEST(mp3dmod)
{
    IMediaObject *dmo;
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoCreateInstance(&CLSID_CMP3DecMediaObject, NULL, CLSCTX_INPROC_SERVER,
        &IID_IMediaObject, (void **)&dmo);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("CLSID_CMP3DecMediaObject not available\n");
        return;
    }
    IMediaObject_Release(dmo);

    test_convert();
    test_aggregation();

    CoUninitialize();
}
