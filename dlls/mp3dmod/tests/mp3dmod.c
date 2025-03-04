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
#include "dmo.h"
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

#define check_member_(line, val, exp, fmt, member)                                           \
    ok_ (__FILE__, line)((val).member == (exp).member, "Got " #member " " fmt ", expected " fmt ".\n", (val).member, (exp).member)
#define check_member(val, exp, fmt, member) check_member_(__LINE__, val, exp, fmt, member)

#define check_wave_format(a, b) check_wave_format_(__LINE__, a, b)
static void check_wave_format_(int line, WAVEFORMATEX *info, const WAVEFORMATEX *expected)
{
    check_member_(line, *info, *expected, "%#x", wFormatTag);
    check_member_(line, *info, *expected, "%u",  nChannels);
    check_member_(line, *info, *expected, "%lu", nSamplesPerSec);
    check_member_(line, *info, *expected, "%lu", nAvgBytesPerSec);
    check_member_(line, *info, *expected, "%u",  nBlockAlign);
    check_member_(line, *info, *expected, "%u",  wBitsPerSample);
    check_member_(line, *info, *expected, "%u",  cbSize);
}

#define check_dmo_media_type(a, b) check_dmo_media_type_(__LINE__, a, b)
static void check_dmo_media_type_(int line, DMO_MEDIA_TYPE *media_type, const DMO_MEDIA_TYPE *expected)
{
    ok_(__FILE__, line)(IsEqualGUID(&media_type->majortype, &expected->majortype),
            "Got unexpected majortype %s, expected %s.\n",
            debugstr_guid(&media_type->majortype), debugstr_guid(&expected->majortype));
    ok_(__FILE__, line)(IsEqualGUID(&media_type->subtype, &expected->subtype),
            "Got unexpected subtype %s, expected %s.\n",
            debugstr_guid(&media_type->subtype), debugstr_guid(&expected->subtype));
    ok_(__FILE__, line)(IsEqualGUID(&media_type->formattype, &expected->formattype),
            "Got unexpected formattype %s.\n",
            debugstr_guid(&media_type->formattype));
    ok_(__FILE__, line)(media_type->pUnk == NULL, "Got unexpected pUnk %p.\n", media_type->pUnk);
    check_member_(line, *media_type, *expected, "%lu", cbFormat);

    if (expected->pbFormat)
    {
        ok_(__FILE__, line)(!!media_type->pbFormat, "Got NULL pbFormat.\n");
        if (!media_type->pbFormat)
            return;

        if (IsEqualGUID(&media_type->formattype, &FORMAT_WaveFormatEx)
                && IsEqualGUID(&expected->formattype, &FORMAT_WaveFormatEx))
            check_wave_format((WAVEFORMATEX *)media_type->pbFormat, (WAVEFORMATEX *)expected->pbFormat);
    }
}

struct test_buffer
{
    IMediaBuffer IMediaBuffer_iface;
    LONG refcount;
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
    struct test_buffer *buffer = impl_from_IMediaBuffer(iface);
    return InterlockedIncrement(&buffer->refcount);
}

static ULONG WINAPI Buffer_Release(IMediaBuffer *iface)
{
    struct test_buffer *buffer = impl_from_IMediaBuffer(iface);
    return InterlockedDecrement(&buffer->refcount);
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
    struct test_buffer outbuf = {.IMediaBuffer_iface = {&Buffer_vtbl}, .refcount = 1};
    struct test_buffer inbuf = {.IMediaBuffer_iface = {&Buffer_vtbl}, .refcount = 1};
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
    ok(hr == S_OK, "got %#lx\n", hr);

    mp3fmt.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
    mp3fmt.wfx.nChannels = 2;
    mp3fmt.wfx.nSamplesPerSec = 48000;
    in.majortype = MEDIATYPE_Audio;
    in.subtype = WMMEDIASUBTYPE_MP3;
    in.formattype = FORMAT_WaveFormatEx;
    in.cbFormat = sizeof(mp3fmt);
    in.pbFormat = (BYTE *)&mp3fmt;

    hr = IMediaObject_SetInputType(dmo, 0, &in, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

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
    ok(hr == S_OK, "got %#lx\n", hr);

    outbuf.len = 0;
    outbuf.maxlen = sizeof(outbuf.data);
    output.pBuffer = &outbuf.IMediaBuffer_iface;
    output.dwStatus = 0xdeadbeef;
    output.rtTimestamp = 0xdeadbeef;
    output.rtTimelength = 0xdeadbeef;
    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_FALSE, "got %#lx\n", hr);
    ok(outbuf.len == 0, "got %lu\n", outbuf.len);
    ok(output.dwStatus == 0, "got %#lx\n", output.dwStatus);
    ok(output.rtTimestamp == 0xdeadbeef, "got %s\n", wine_dbgstr_longlong(output.rtTimestamp));
    ok(output.rtTimelength == 0xdeadbeef, "got %s\n", wine_dbgstr_longlong(output.rtTimelength));

    /* write several frames of mp3 data */
    for (i = 0; i < 5; i++)
        memcpy(inbuf.data + 96 * i, mp3hdr, 4);
    inbuf.len = 96 * 5;
    hr = IMediaObject_ProcessInput(dmo, 1, &inbuf.IMediaBuffer_iface, 0, 0, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "got %#lx\n", hr);
    hr = IMediaObject_ProcessInput(dmo, 0, &inbuf.IMediaBuffer_iface, 0, 0, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(inbuf.refcount == 2, "Got refcount %ld.\n", inbuf.refcount);

    hr = IMediaObject_ProcessInput(dmo, 0, &inbuf.IMediaBuffer_iface, 0, 0, 0);
    ok(hr == DMO_E_NOTACCEPTING, "got %#lx\n", hr);

    outbuf.len = 0;
    outbuf.maxlen = 0;
    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_FALSE, "got %#lx\n", hr);
    ok(outbuf.len == 0, "got %lu\n", outbuf.len);
    ok(output.dwStatus == (O_SYNCPOINT | O_INCOMPLETE), "got %#lx\n", output.dwStatus);
    ok(output.rtTimestamp == 0xdeadbeef, "got %s\n", wine_dbgstr_longlong(output.rtTimestamp));
    ok(output.rtTimelength == 0xdeadbeef, "got %s\n", wine_dbgstr_longlong(output.rtTimelength));

    /* implementations are inconsistent, but should write at least one frame of data */
    outbuf.len = 0;
    outbuf.maxlen = 5000;
    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_OK, "got %#lx\n", hr);
    written = outbuf.len;
    ok(written > 1152 && written <= 5000, "got %lu\n", written);
    ok(output.dwStatus == (O_SYNCPOINT | O_TIME | O_TIMELENGTH | O_INCOMPLETE),
        "got %#lx\n", output.dwStatus);
    ok(output.rtTimestamp == 0, "got %s\n", wine_dbgstr_longlong(output.rtTimestamp));
    ok(output.rtTimelength == samplelen(written, 48000),
        "got %s\n", wine_dbgstr_longlong(output.rtTimelength));
    ok(inbuf.refcount == 2, "Got refcount %ld.\n", inbuf.refcount);
    ok(outbuf.refcount == 1, "Got refcount %ld.\n", inbuf.refcount);

    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_FALSE, "got %#lx\n", hr);
    ok(outbuf.len == written, "expected %lu, got %lu\n", written, outbuf.len);
    ok(output.dwStatus == (O_SYNCPOINT | O_INCOMPLETE), "got %#lx\n", output.dwStatus);
    ok(output.rtTimestamp == 0, "got %s\n", wine_dbgstr_longlong(output.rtTimestamp));
    ok(output.rtTimelength == samplelen(written, 48000),
        "got %s\n", wine_dbgstr_longlong(output.rtTimelength));

    hr = IMediaObject_ProcessInput(dmo, 0, &inbuf.IMediaBuffer_iface, 0, 0, 0);
    ok(hr == DMO_E_NOTACCEPTING, "got %#lx\n", hr);
    ok(inbuf.refcount == 2, "Got refcount %ld.\n", inbuf.refcount);
    ok(outbuf.refcount == 1, "Got refcount %ld.\n", inbuf.refcount);

    /* write the rest */
    outbuf.len = 0;
    outbuf.maxlen = 5000;
    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(written + outbuf.len == (1152 * 5) ||
        broken(written + outbuf.len == (1152 * 5) - 528), /* Win10 */
        "got %lu, total %lu\n", outbuf.len, written + outbuf.len);
    ok(output.dwStatus == (O_SYNCPOINT | O_TIME | O_TIMELENGTH), "got %#lx\n", output.dwStatus);
    ok(output.rtTimestamp == samplelen(written, 48000),
        "got %s\n", wine_dbgstr_longlong(output.rtTimestamp));
    ok(output.rtTimelength == samplelen(outbuf.len, 48000),
        "got %s\n", wine_dbgstr_longlong(output.rtTimelength));
    ok(inbuf.refcount == 1, "Got refcount %ld.\n", inbuf.refcount);
    ok(outbuf.refcount == 1, "Got refcount %ld.\n", inbuf.refcount);
    written += outbuf.len;

    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_FALSE, "got %#lx\n", hr);
    ok(output.dwStatus == 0, "got %#lx\n", output.dwStatus);

    output.pBuffer = NULL;
    output.dwStatus = 0xdeadbeef;
    output.rtTimestamp = 0xdeadbeef;
    output.rtTimelength = 0xdeadbeef;
    hr = IMediaObject_ProcessOutput(dmo, DMO_PROCESS_OUTPUT_DISCARD_WHEN_NO_BUFFER, 1, &output, &status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!output.pBuffer, "Got buffer %p.\n", output.pBuffer);
    ok(!output.dwStatus, "Got status %#lx.\n", output.dwStatus);
    ok(output.rtTimestamp == 0xdeadbeef, "Got timestamp %s.\n", wine_dbgstr_longlong(output.rtTimestamp));
    ok(output.rtTimelength == 0xdeadbeef, "Got length %s.\n", wine_dbgstr_longlong(output.rtTimelength));

    hr = IMediaObject_ProcessInput(dmo, 0, &inbuf.IMediaBuffer_iface, 0, 0, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IMediaObject_ProcessInput(dmo, 0, &inbuf.IMediaBuffer_iface, 0, 0, 0);
    ok(hr == DMO_E_NOTACCEPTING, "Got hr %#lx.\n", hr);

    output.pBuffer = NULL;
    output.dwStatus = 0xdeadbeef;
    output.rtTimestamp = 0xdeadbeef;
    output.rtTimelength = 0xdeadbeef;
    hr = IMediaObject_ProcessOutput(dmo, DMO_PROCESS_OUTPUT_DISCARD_WHEN_NO_BUFFER, 1, &output, &status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!output.pBuffer, "Got buffer %p.\n", output.pBuffer);
    ok(output.dwStatus == O_INCOMPLETE, "Got status %#lx.\n", output.dwStatus);
    ok(output.rtTimestamp == 0xdeadbeef, "Got timestamp %s.\n", wine_dbgstr_longlong(output.rtTimestamp));
    ok(output.rtTimelength == 0xdeadbeef, "Got length %s.\n", wine_dbgstr_longlong(output.rtTimelength));
    ok(inbuf.refcount == 2, "Got refcount %ld.\n", inbuf.refcount);

    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!output.pBuffer, "Got buffer %p.\n", output.pBuffer);
    ok(output.dwStatus == O_INCOMPLETE, "Got status %#lx.\n", output.dwStatus);
    ok(output.rtTimestamp == 0xdeadbeef, "Got timestamp %s.\n", wine_dbgstr_longlong(output.rtTimestamp));
    ok(output.rtTimelength == 0xdeadbeef, "Got length %s.\n", wine_dbgstr_longlong(output.rtTimelength));
    ok(inbuf.refcount == 2, "Got refcount %ld.\n", inbuf.refcount);

    output.pBuffer = &outbuf.IMediaBuffer_iface;
    outbuf.len = 0;
    outbuf.maxlen = 5000;
    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(outbuf.len > 1152 && outbuf.len <= 5000, "Got length %lu.\n", outbuf.len);
    ok(output.dwStatus == (O_SYNCPOINT | O_TIME | O_TIMELENGTH | O_INCOMPLETE),
            "Got status %#lx.\n", output.dwStatus);
    ok(output.rtTimestamp == samplelen(written, 48000), "Got timestamp %s.\n",
            wine_dbgstr_longlong(output.rtTimestamp));
    ok(output.rtTimelength == samplelen(outbuf.len, 48000),
            "Got length %s.\n", wine_dbgstr_longlong(output.rtTimelength));
    ok(inbuf.refcount == 2, "Got refcount %ld.\n", inbuf.refcount);
    ok(outbuf.refcount == 1, "Got refcount %ld.\n", inbuf.refcount);

    hr = IMediaObject_Flush(dmo);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(inbuf.refcount == 1, "Got refcount %ld.\n", inbuf.refcount);

    outbuf.len = 0;
    outbuf.maxlen = 5000;
    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaObject_Flush(dmo);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaObject_ProcessInput(dmo, 0, &inbuf.IMediaBuffer_iface, 0, 0, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IMediaObject_ProcessOutput(dmo, 0, 1, &output, &status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(outbuf.len > 1152 && outbuf.len <= 5000, "Got length %lu.\n", outbuf.len);
    ok(output.dwStatus == (O_SYNCPOINT | O_TIME | O_TIMELENGTH | O_INCOMPLETE),
            "Got status %#lx.\n", output.dwStatus);
    ok(!output.rtTimestamp, "Got timestamp %s.\n", wine_dbgstr_longlong(output.rtTimestamp));
    ok(output.rtTimelength == samplelen(outbuf.len, 48000),
            "Got length %s.\n", wine_dbgstr_longlong(output.rtTimelength));
    ok(inbuf.refcount == 2, "Got refcount %ld.\n", inbuf.refcount);
    ok(outbuf.refcount == 1, "Got refcount %ld.\n", inbuf.refcount);

    IMediaObject_Release(dmo);
    ok(inbuf.refcount == 1, "Got outstanding refcount %ld.\n", inbuf.refcount);
    ok(outbuf.refcount == 1, "Got outstanding refcount %ld.\n", outbuf.refcount);
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
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IUnknown_QueryInterface(unk, &IID_IMediaObject, (void **)&dmo);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IMediaObject_QueryInterface(dmo, &IID_test_outer, (void **)&unk2);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "got unk %p\n", unk2);

    IUnknown_Release(dmo);
    IUnknown_Release(unk);

    hr = CoCreateInstance(&CLSID_CMP3DecMediaObject, &Outer, CLSCTX_INPROC_SERVER,
        &IID_IMediaObject, (void **)&unk);
    ok(hr == E_NOINTERFACE, "got %#lx\n", hr);
}

static void test_stream_info(void)
{
    MPEGLAYER3WAVEFORMAT input_format =
    {
        .wfx.nSamplesPerSec = 48000,
    };
    DMO_MEDIA_TYPE input_mt =
    {
        .majortype = MEDIATYPE_Audio,
        .subtype = WMMEDIASUBTYPE_MP3,
        .formattype = FORMAT_WaveFormatEx,
        .cbFormat = sizeof(input_format),
        .pbFormat = (BYTE *)&input_format,
    };

    WAVEFORMATEX output_format =
    {
        .nSamplesPerSec = 48000,
    };
    DMO_MEDIA_TYPE output_mt =
    {
        .formattype = FORMAT_WaveFormatEx,
        .cbFormat = sizeof(output_format),
        .pbFormat = (BYTE *)&output_format,
    };

    DWORD input_count, output_count, flags, size, lookahead, alignment;
    WORD channels, depth;
    IMediaObject *dmo;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_CMP3DecMediaObject, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMediaObject, (void **)&dmo);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaObject_GetStreamCount(dmo, &input_count, &output_count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(input_count == 1, "Got input count %lu.\n", input_count);
    ok(output_count == 1, "Got output count %lu.\n", output_count);

    flags = 0xdeadbeef;
    hr = IMediaObject_GetInputStreamInfo(dmo, 0, &flags);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!flags, "Got flags %#lx.\n", flags);

    flags = 0xdeadbeef;
    hr = IMediaObject_GetOutputStreamInfo(dmo, 0, &flags);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!flags, "Got flags %#lx.\n", flags);

    hr = IMediaObject_GetInputSizeInfo(dmo, 1, &size, &lookahead, &alignment);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "Got hr %#lx.\n", hr);
    hr = IMediaObject_GetOutputSizeInfo(dmo, 1, &size, &alignment);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "Got hr %#lx.\n", hr);

    hr = IMediaObject_GetInputSizeInfo(dmo, 0, &size, &lookahead, &alignment);
    ok(hr == DMO_E_TYPE_NOT_SET, "Got hr %#lx.\n", hr);
    hr = IMediaObject_GetOutputSizeInfo(dmo, 0, &size, &alignment);
    ok(hr == DMO_E_TYPE_NOT_SET, "Got hr %#lx.\n", hr);

    for (channels = 1; channels <= 2; ++channels)
    {
        input_format.wfx.nChannels = channels;
        output_format.nChannels = channels;

        hr = IMediaObject_SetInputType(dmo, 0, &input_mt, 0);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IMediaObject_GetInputSizeInfo(dmo, 0, &size, &lookahead, &alignment);
        ok(hr == DMO_E_TYPE_NOT_SET, "Got hr %#lx.\n", hr);
        hr = IMediaObject_GetOutputSizeInfo(dmo, 0, &size, &alignment);
        ok(hr == DMO_E_TYPE_NOT_SET, "Got hr %#lx.\n", hr);

        for (depth = 8; depth <= 16; depth += 8)
        {
            output_format.wBitsPerSample = depth;
            output_format.nBlockAlign = channels * depth / 8;
            output_format.nAvgBytesPerSec = 48000 * output_format.nBlockAlign;

            hr = IMediaObject_SetOutputType(dmo, 0, &output_mt, 0);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);

            size = lookahead = alignment = 0xdeadbeef;
            hr = IMediaObject_GetInputSizeInfo(dmo, 0, &size, &lookahead, &alignment);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(!size || broken(size == output_format.nBlockAlign) /* Vista */,
                    "Got size %lu for %u channels, depth %u.\n", size, channels, depth);
            ok(lookahead == 0xdeadbeef, "Got lookahead %lu.\n", lookahead);
            ok(alignment == 1, "Got alignment %lu.\n", alignment);

            size = alignment = 0xdeadbeef;
            hr = IMediaObject_GetOutputSizeInfo(dmo, 0, &size, &alignment);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            /* Vista returns the expected size; all later versions act as if
             * channels == 2 for some reason. */
            ok(size >= channels * 1152 * depth / 8,
                    "Got size %lu for %u channels, depth %u.\n", size, channels, depth);
            ok(alignment == 1, "Got alignment %lu.\n", alignment);
        }

        hr = IMediaObject_SetOutputType(dmo, 0, &output_mt, DMO_SET_TYPEF_CLEAR);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
    }

    IMediaObject_Release(dmo);
}

static void test_media_types(void)
{
    MPEGLAYER3WAVEFORMAT mp3fmt =
    {
        .wfx.nChannels = 2,
        .wfx.nSamplesPerSec = 48000,
    };
    DMO_MEDIA_TYPE input_mt =
    {
        .majortype = MEDIATYPE_Audio,
        .subtype = WMMEDIASUBTYPE_MP3,
        .formattype = FORMAT_WaveFormatEx,
        .cbFormat = sizeof(mp3fmt),
        .pbFormat = (BYTE *)&mp3fmt,
    };

    WAVEFORMATEX expect_wfx =
    {
        .wFormatTag = WAVE_FORMAT_PCM,
        .nSamplesPerSec = 48000,
    };

    WAVEFORMATEX output_format =
    {
        .nChannels = 1,
        .nSamplesPerSec = 48000,
        .nAvgBytesPerSec = 48000,
        .nBlockAlign = 1,
        .wBitsPerSample = 8,
    };
    DMO_MEDIA_TYPE output_mt =
    {
        .formattype = FORMAT_WaveFormatEx,
        .cbFormat = sizeof(output_format),
        .pbFormat = (BYTE *)&output_format,
    };

    DMO_MEDIA_TYPE mt;
    IMediaObject *dmo;
    HRESULT hr;
    DWORD i;

    hr = CoCreateInstance(&CLSID_CMP3DecMediaObject, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMediaObject, (void **)&dmo);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    memset(&mt, 0xcc, sizeof(DMO_MEDIA_TYPE));
    hr = IMediaObject_GetInputType(dmo, 1, 0, &mt);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "Got hr %#lx.\n", hr);
    hr = IMediaObject_GetInputType(dmo, 0, 0, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&mt.majortype, &MEDIATYPE_Audio), "Got major type %s.\n", wine_dbgstr_guid(&mt.majortype));
    ok(IsEqualGUID(&mt.subtype, &WMMEDIASUBTYPE_MP3), "Got subtype %s.\n", wine_dbgstr_guid(&mt.subtype));
    ok(mt.bFixedSizeSamples == 0xcccccccc, "Got fixed size %d.\n", mt.bFixedSizeSamples);
    ok(mt.bTemporalCompression == 0xcccccccc, "Got temporal compression %d.\n", mt.bTemporalCompression);
    ok(mt.lSampleSize == 0xcccccccc, "Got sample size %lu.\n", mt.lSampleSize);
    ok(IsEqualGUID(&mt.formattype, &GUID_NULL), "Got format type %s.\n",
            wine_dbgstr_guid(&mt.formattype));
    ok(!mt.pUnk, "Got pUnk %p.\n", mt.pUnk);
    ok(!mt.cbFormat, "Got format size %lu.\n", mt.cbFormat);
    ok(!mt.pbFormat, "Got format block %p.\n", mt.pbFormat);

    hr = IMediaObject_GetInputType(dmo, 0, 1, &mt);
    ok(hr == DMO_E_NO_MORE_ITEMS, "Got hr %#lx.\n", hr);

    memset(&mt, 0xcc, sizeof(DMO_MEDIA_TYPE));
    hr = IMediaObject_GetOutputType(dmo, 1, 0, &mt);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "Got hr %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 0, 0, &mt);
    ok(hr == DMO_E_TYPE_NOT_SET, "Got hr %#lx.\n", hr);

    hr = IMediaObject_SetOutputType(dmo, 0, &output_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_SET, "Got hr %#lx.\n", hr);

    hr = IMediaObject_GetInputCurrentType(dmo, 1, &mt);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "Got hr %#lx.\n", hr);

    hr = IMediaObject_GetInputCurrentType(dmo, 0, &mt);
    ok(hr == DMO_E_TYPE_NOT_SET, "Got hr %#lx.\n", hr);

    hr = IMediaObject_SetInputType(dmo, 1, &input_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "Got hr %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, &input_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaObject_GetInputCurrentType(dmo, 0, &mt);
    ok(hr == DMO_E_TYPE_NOT_SET, "Got hr %#lx.\n", hr);

    input_mt.majortype = GUID_NULL;
    hr = IMediaObject_SetInputType(dmo, 0, &input_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    input_mt.majortype = MEDIATYPE_Stream;
    hr = IMediaObject_SetInputType(dmo, 0, &input_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    input_mt.majortype = MEDIATYPE_Audio;

    input_mt.subtype = GUID_NULL;
    hr = IMediaObject_SetInputType(dmo, 0, &input_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    input_mt.subtype = MEDIASUBTYPE_PCM;
    hr = IMediaObject_SetInputType(dmo, 0, &input_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    input_mt.subtype = WMMEDIASUBTYPE_MP3;

    input_mt.formattype = GUID_NULL;
    hr = IMediaObject_SetInputType(dmo, 0, &input_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    input_mt.formattype = FORMAT_None;
    hr = IMediaObject_SetInputType(dmo, 0, &input_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    input_mt.formattype = FORMAT_WaveFormatEx;

    hr = IMediaObject_SetInputType(dmo, 0, &input_mt, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaObject_GetInputCurrentType(dmo, 0, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    check_dmo_media_type(&mt, &input_mt);
    MoFreeMediaType(&mt);

    for (i = 0; i < 4; ++i)
    {
        memset(&mt, 0xcc, sizeof(DMO_MEDIA_TYPE));
        hr = IMediaObject_GetOutputType(dmo, 0, i, &mt);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(IsEqualGUID(&mt.majortype, &MEDIATYPE_Audio), "Got major type %s.\n", wine_dbgstr_guid(&mt.majortype));
        ok(IsEqualGUID(&mt.subtype, &MEDIASUBTYPE_PCM), "Got subtype %s.\n", wine_dbgstr_guid(&mt.subtype));
        ok(mt.bFixedSizeSamples == 0xcccccccc, "Got fixed size %d.\n", mt.bFixedSizeSamples);
        ok(mt.bTemporalCompression == 0xcccccccc, "Got temporal compression %d.\n", mt.bTemporalCompression);
        ok(mt.lSampleSize == 0xcccccccc, "Got sample size %lu.\n", mt.lSampleSize);
        ok(IsEqualGUID(&mt.formattype, &FORMAT_WaveFormatEx), "Got format type %s.\n",
                wine_dbgstr_guid(&mt.formattype));
        ok(!mt.pUnk, "Got pUnk %p.\n", mt.pUnk);
        ok(mt.cbFormat >= sizeof(WAVEFORMATEX), "Got format size %lu.\n", mt.cbFormat);
        ok(!!mt.pbFormat, "Got format block %p.\n", mt.pbFormat);

        expect_wfx.nChannels = (i / 2) ? 1 : 2;
        expect_wfx.wBitsPerSample = (i % 2) ? 8 : 16;
        expect_wfx.nBlockAlign = expect_wfx.nChannels * expect_wfx.wBitsPerSample / 8;
        expect_wfx.nAvgBytesPerSec = 48000 * expect_wfx.nBlockAlign;
        ok(!memcmp(mt.pbFormat, &expect_wfx, sizeof(WAVEFORMATEX)), "Format blocks didn't match.\n");

        MoFreeMediaType(&mt);
    }

    hr = IMediaObject_GetOutputType(dmo, 0, 4, &mt);
    ok(hr == DMO_E_NO_MORE_ITEMS, "Got hr %#lx.\n", hr);

    mp3fmt.wfx.nChannels = 1;
    hr = IMediaObject_SetInputType(dmo, 0, &input_mt, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaObject_GetInputCurrentType(dmo, 0, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    check_dmo_media_type(&mt, &input_mt);
    MoFreeMediaType(&mt);

    for (i = 0; i < 2; ++i)
    {
        memset(&mt, 0xcc, sizeof(DMO_MEDIA_TYPE));
        hr = IMediaObject_GetOutputType(dmo, 0, i, &mt);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(IsEqualGUID(&mt.majortype, &MEDIATYPE_Audio), "Got major type %s.\n", wine_dbgstr_guid(&mt.majortype));
        ok(IsEqualGUID(&mt.subtype, &MEDIASUBTYPE_PCM), "Got subtype %s.\n", wine_dbgstr_guid(&mt.subtype));
        ok(mt.bFixedSizeSamples == 0xcccccccc, "Got fixed size %d.\n", mt.bFixedSizeSamples);
        ok(mt.bTemporalCompression == 0xcccccccc, "Got temporal compression %d.\n", mt.bTemporalCompression);
        ok(mt.lSampleSize == 0xcccccccc, "Got sample size %lu.\n", mt.lSampleSize);
        ok(IsEqualGUID(&mt.formattype, &FORMAT_WaveFormatEx), "Got format type %s.\n",
                wine_dbgstr_guid(&mt.formattype));
        ok(!mt.pUnk, "Got pUnk %p.\n", mt.pUnk);
        ok(mt.cbFormat >= sizeof(WAVEFORMATEX), "Got format size %lu.\n", mt.cbFormat);
        ok(!!mt.pbFormat, "Got format block %p.\n", mt.pbFormat);

        expect_wfx.nChannels = 1;
        expect_wfx.wBitsPerSample = (i % 2) ? 8 : 16;
        expect_wfx.nBlockAlign = expect_wfx.nChannels * expect_wfx.wBitsPerSample / 8;
        expect_wfx.nAvgBytesPerSec = 48000 * expect_wfx.nBlockAlign;
        ok(!memcmp(mt.pbFormat, &expect_wfx, sizeof(WAVEFORMATEX)), "Format blocks didn't match.\n");

        MoFreeMediaType(&mt);
    }

    hr = IMediaObject_GetOutputType(dmo, 0, 2, &mt);
    ok(hr == DMO_E_NO_MORE_ITEMS, "Got hr %#lx.\n", hr);

    hr = IMediaObject_SetOutputType(dmo, 1, &output_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "Got hr %#lx.\n", hr);

    hr = IMediaObject_GetOutputCurrentType(dmo, 1, &mt);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "Got hr %#lx.\n", hr);

    hr = IMediaObject_GetOutputCurrentType(dmo, 0, &mt);
    ok(hr == DMO_E_TYPE_NOT_SET, "Got hr %#lx.\n", hr);

    hr = IMediaObject_SetOutputType(dmo, 0, &output_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaObject_GetOutputCurrentType(dmo, 0, &mt);
    ok(hr == DMO_E_TYPE_NOT_SET, "Got hr %#lx.\n", hr);

    output_mt.formattype = GUID_NULL;
    hr = IMediaObject_SetOutputType(dmo, 0, &output_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    output_mt.formattype = FORMAT_None;
    hr = IMediaObject_SetOutputType(dmo, 0, &output_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    output_mt.formattype = FORMAT_WaveFormatEx;

    hr = IMediaObject_SetOutputType(dmo, 0, &output_mt, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaObject_GetOutputCurrentType(dmo, 0, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    check_dmo_media_type(&mt, &output_mt);
    MoFreeMediaType(&mt);

    output_format.nSamplesPerSec = 24000;
    output_format.nAvgBytesPerSec = 24000;
    hr = IMediaObject_SetOutputType(dmo, 0, &output_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    output_format.nSamplesPerSec = 12000;
    output_format.nAvgBytesPerSec = 12000;
    hr = IMediaObject_SetOutputType(dmo, 0, &output_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    output_format.nSamplesPerSec = 6000;
    output_format.nAvgBytesPerSec = 6000;
    hr = IMediaObject_SetOutputType(dmo, 0, &output_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);

    output_format.nSamplesPerSec = 12000;
    output_format.nAvgBytesPerSec = 6000;
    hr = IMediaObject_SetOutputType(dmo, 0, &output_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    output_format.nSamplesPerSec = 48000;
    output_format.nAvgBytesPerSec = 48000;
    output_format.nChannels = 2;
    hr = IMediaObject_SetOutputType(dmo, 0, &output_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    /* Windows accepts 32 bits per sample but does not enumerate it */
    output_format.nChannels = 1;
    output_format.wBitsPerSample = 32;
    output_format.nBlockAlign = 4;
    output_format.nSamplesPerSec = 48000;
    output_format.nAvgBytesPerSec = 192000;
    hr = IMediaObject_SetOutputType(dmo, 0, &output_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    mp3fmt.wfx.nChannels = 0;
    hr = IMediaObject_SetInputType(dmo, 0, &input_mt, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);

    IMediaObject_Release(dmo);
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
    test_stream_info();
    test_media_types();

    CoUninitialize();
}
