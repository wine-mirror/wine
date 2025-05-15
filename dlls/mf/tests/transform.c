/*
 * Copyright 2017 Nikolay Sivov
 * Copyright 2022 Rémi Bernon for CodeWeavers
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

#include "control.h"
#include "d3d9types.h"
#include "dmo.h"
#include "mferror.h"
#include "mfidl.h"
#include "mftransform.h"
#include "propvarutil.h"
#include "uuids.h"
#include "wmcodecdsp.h"
#include "mediaerr.h"
#include "amvideo.h"
#include "vfw.h"
#include "ks.h"
#include "ksmedia.h"

#include "mf_test.h"

#include "wine/test.h"

#include "initguid.h"

#include "codecapi.h"
#include "icodecapi.h"

#include "d3d11_4.h"

DEFINE_GUID(DMOVideoFormat_RGB24,D3DFMT_R8G8B8,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(DMOVideoFormat_RGB32,D3DFMT_X8R8G8B8,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(DMOVideoFormat_RGB555,D3DFMT_X1R5G5B5,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(DMOVideoFormat_RGB565,D3DFMT_R5G6B5,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(DMOVideoFormat_RGB8,D3DFMT_P8,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(MFAudioFormat_RAW_AAC1,WAVE_FORMAT_RAW_AAC1,0x0000,0x0010,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71);
DEFINE_GUID(MFVideoFormat_WMV_Unknown,0x7ce12ca9,0xbfbf,0x43d9,0x9d,0x00,0x82,0xb8,0xed,0x54,0x31,0x6b);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_ABGR32,D3DFMT_A8B8G8R8);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_P208,MAKEFOURCC('P','2','0','8'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_VC1S,MAKEFOURCC('V','C','1','S'));
DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_IV50,MAKEFOURCC('I','V','5','0'));

DEFINE_GUID(mft_output_sample_incomplete,0xffffff,0xffff,0xffff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff);

static const GUID test_attr_guid = {0xdeadbeef};

struct media_buffer
{
    IMediaBuffer IMediaBuffer_iface;
    LONG refcount;
    DWORD length;
    DWORD max_length;
    BYTE data[];
};

static inline struct media_buffer *impl_from_IMediaBuffer(IMediaBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct media_buffer, IMediaBuffer_iface);
}

static HRESULT WINAPI media_buffer_QueryInterface(IMediaBuffer *iface, REFIID iid, void **obj)
{
    if (IsEqualIID(iid, &IID_IMediaBuffer)
            || IsEqualIID(iid, &IID_IUnknown))
    {
        *obj = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI media_buffer_AddRef(IMediaBuffer *iface)
{
    struct media_buffer *buffer = impl_from_IMediaBuffer(iface);
    return InterlockedIncrement(&buffer->refcount);
}

static ULONG WINAPI media_buffer_Release(IMediaBuffer *iface)
{
    struct media_buffer *buffer = impl_from_IMediaBuffer(iface);
    ULONG ref = InterlockedDecrement(&buffer->refcount);
    if (!ref)
        free(buffer);
    return ref;
}

static HRESULT WINAPI media_buffer_SetLength(IMediaBuffer *iface, DWORD length)
{
    struct media_buffer *buffer = impl_from_IMediaBuffer(iface);
    if (length > buffer->max_length)
        return E_INVALIDARG;
    buffer->length = length;
    return S_OK;
}

static HRESULT WINAPI media_buffer_GetMaxLength(IMediaBuffer *iface, DWORD *max_length)
{
    struct media_buffer *buffer = impl_from_IMediaBuffer(iface);
    if (!max_length)
        return E_POINTER;
    *max_length = buffer->max_length;
    return S_OK;
}

static HRESULT WINAPI media_buffer_GetBufferAndLength(IMediaBuffer *iface, BYTE **data, DWORD *length)
{
    struct media_buffer *buffer = impl_from_IMediaBuffer(iface);
    if (!data || ! length)
        return E_POINTER;
    *data = buffer->data;
    *length = buffer->length;
    return S_OK;
}

static IMediaBufferVtbl media_buffer_vtbl = {
    media_buffer_QueryInterface,
    media_buffer_AddRef,
    media_buffer_Release,
    media_buffer_SetLength,
    media_buffer_GetMaxLength,
    media_buffer_GetBufferAndLength,
};

HRESULT media_buffer_create(DWORD max_length, struct media_buffer **ret)
{
    struct media_buffer *buffer;

    if (!(buffer = calloc(1, offsetof(struct media_buffer, data[max_length]))))
        return E_OUTOFMEMORY;
    buffer->IMediaBuffer_iface.lpVtbl = &media_buffer_vtbl;
    buffer->refcount = 1;
    buffer->length = 0;
    buffer->max_length = max_length;
    *ret = buffer;
    return S_OK;
}

static BOOL is_compressed_subtype(const GUID *subtype)
{
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_WMV1)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_WMV2)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_WMVA)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_WMVP)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_WVP2)
            || IsEqualGUID(subtype, &MFVideoFormat_WMV_Unknown)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_WVC1)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_WMV3)
            || IsEqualGUID(subtype, &MFVideoFormat_VC1S))
        return TRUE;
    return FALSE;
}

static DWORD subtype_to_tag(const GUID *subtype)
{
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB32)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_RGB24)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_RGB555)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_RGB8))
        return BI_RGB;
    else if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB565))
        return BI_BITFIELDS;
    else if (IsEqualGUID(subtype, &MEDIASUBTYPE_PCM))
        return WAVE_FORMAT_PCM;
    else
        return subtype->Data1;
}

static DWORD subtype_to_bpp(const GUID *subtype)
{
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB8))
        return 8;
    else if (IsEqualGUID(subtype, &MEDIASUBTYPE_NV12)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_YV12)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_IYUV)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_I420)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_NV11))
        return 12;
    else if (IsEqualGUID(subtype, &MEDIASUBTYPE_YUY2)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_UYVY)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_YVYU)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_RGB565)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_RGB555))
        return 16;
    else if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB24))
        return 24;
    else if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB32))
        return 32;
    else
        return 0;
}

static DWORD subtype_to_extra_bytes(const GUID *subtype)
{
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_MSAUDIO1))
        return MSAUDIO1_WFX_EXTRA_BYTES;
    else if (IsEqualGUID(subtype, &MEDIASUBTYPE_WMAUDIO2))
        return WMAUDIO2_WFX_EXTRA_BYTES;
    else if (IsEqualGUID(subtype, &MEDIASUBTYPE_WMAUDIO3))
        return WMAUDIO3_WFX_EXTRA_BYTES;
    else if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB8))
        return sizeof(RGBQUAD) * (1 << 8);
    else if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB565))
        return sizeof(DWORD) * 3;
    else if (is_compressed_subtype(subtype))
        return 4;
    else
        return 0;
}

static void load_resource(const WCHAR *filename, const BYTE **data, DWORD *length)
{
    HRSRC resource = FindResourceW(NULL, filename, (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    *data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    *length = SizeofResource(GetModuleHandleW(NULL), resource);
}

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG expected_refcount, int line)
{
    ULONG refcount;
    IUnknown_AddRef(obj);
    refcount = IUnknown_Release(obj);
    ok_(__FILE__, line)(refcount == expected_refcount, "Unexpected refcount %ld, expected %ld.\n", refcount,
            expected_refcount);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

#define check_member_(file, line, val, exp, fmt, member)                                           \
    ok_ (file, line)((val).member == (exp).member, "Got " #member " " fmt ", expected " fmt ".\n", (val).member, (exp).member)
#define check_member(val, exp, fmt, member) check_member_(__FILE__, __LINE__, val, exp, fmt, member)

const char *debugstr_propvariant(const PROPVARIANT *propvar, BOOL ratio)
{
    char buffer[1024] = {0}, *ptr = buffer;
    UINT i;

    switch (propvar->vt)
    {
        default:
            return wine_dbg_sprintf("??");
        case VT_CLSID:
            return wine_dbg_sprintf("%s", debugstr_guid(propvar->puuid));
        case VT_UI4:
            return wine_dbg_sprintf("%lu", propvar->ulVal);
        case VT_UI8:
            if (ratio)
                return wine_dbg_sprintf("%lu:%lu", propvar->uhVal.HighPart, propvar->uhVal.LowPart);
            else
                return wine_dbg_sprintf("%I64u", propvar->uhVal.QuadPart);
        case VT_VECTOR | VT_UI1:
            ptr += sprintf(ptr, "size %lu, data {", propvar->caub.cElems);
            for (i = 0; i < 128 && i < propvar->caub.cElems; ++i)
                ptr += sprintf(ptr, "0x%02x,", propvar->caub.pElems[i]);
            if (propvar->caub.cElems > 128)
                ptr += sprintf(ptr, "...}");
            else
                ptr += sprintf(ptr - (i ? 1 : 0), "}");
            return wine_dbg_sprintf("%s", buffer);
    }
}

void check_attributes_(const char *file, int line, IMFAttributes *attributes,
        const struct attribute_desc *desc, ULONG limit)
{
    PROPVARIANT value;
    int i, ret;
    HRESULT hr;

    for (i = 0; i < limit && desc[i].key; ++i)
    {
        hr = IMFAttributes_GetItem(attributes, desc[i].key, &value);
        todo_wine_if(desc[i].todo)
        ok_(file, line)(hr == S_OK, "%s missing, hr %#lx\n", debugstr_a(desc[i].name), hr);
        if (hr != S_OK) continue;

        ret = PropVariantCompareEx(&value, &desc[i].value, 0, 0);
        todo_wine_if(desc[i].todo_value)
        ok_(file, line)(ret == 0, "%s mismatch, type %u, value %s\n",
                debugstr_a(desc[i].name), value.vt, debugstr_propvariant(&value, desc[i].ratio));
        PropVariantClear(&value);
    }
}

struct transform_info
{
    const WCHAR *name;
    const GUID *major_type;
    struct
    {
        const GUID *subtype;
        BOOL broken;
    } inputs[32], input_end, outputs[32], output_end;
};

static BOOL check_mft_enum(GUID category, MFT_REGISTER_TYPE_INFO *input_type,
        MFT_REGISTER_TYPE_INFO *output_type, const GUID *expect_class_id)
{
    GUID *class_ids = NULL;
    UINT32 count = 0, i;
    HRESULT hr;

    hr = MFTEnum(category, 0, input_type, output_type, NULL, &class_ids, &count);
    if (FAILED(hr) || count == 0)
    {
        todo_wine
        win_skip("MFTEnum returned %#lx, count %u, skipping tests.\n", hr, count);
        return FALSE;
    }

    ok(hr == S_OK, "MFTEnum returned %#lx\n", hr);
    for (i = 0; i < count; ++i)
        if (IsEqualGUID(expect_class_id, class_ids + i))
            break;
    ok(i < count, "Failed to find transform.\n");
    CoTaskMemFree(class_ids);

    return i < count;
}

static void check_mft_get_info(const GUID *class_id, const struct transform_info *expect)
{
    MFT_REGISTER_TYPE_INFO *input_types = NULL, *output_types = NULL;
    UINT32 input_count = 0, output_count = 0, i;
    WCHAR *name;
    HRESULT hr;

    hr = MFTGetInfo(*class_id, &name, &input_types, &input_count, &output_types, &output_count, NULL);
    ok(hr == S_OK, "MFTEnum returned %#lx\n", hr);
    ok(!wcscmp(name, expect->name), "got name %s\n", debugstr_w(name));

    for (i = 0; i < input_count && expect->inputs[i].subtype; ++i)
    {
        ok(IsEqualGUID(&input_types[i].guidMajorType, expect->major_type),
                "got input[%u] major %s\n", i, debugstr_guid(&input_types[i].guidMajorType));
        ok(IsEqualGUID(&input_types[i].guidSubtype, expect->inputs[i].subtype),
                "got input[%u] subtype %s\n", i, debugstr_guid(&input_types[i].guidSubtype));
    }
    for (; expect->inputs[i].subtype; ++i)
        ok(broken(expect->inputs[i].broken), "missing input[%u] subtype %s\n",
                i, debugstr_guid(expect->inputs[i].subtype));
    for (; i < input_count; ++i)
        ok(0, "extra input[%u] subtype %s\n", i, debugstr_guid(&input_types[i].guidSubtype));

    for (i = 0; expect->outputs[i].subtype; ++i)
    {
        ok(IsEqualGUID(&output_types[i].guidMajorType, expect->major_type),
                "got output[%u] major %s\n", i, debugstr_guid(&output_types[i].guidMajorType));
        ok(IsEqualGUID(&output_types[i].guidSubtype, expect->outputs[i].subtype),
                "got output[%u] subtype %s\n", i, debugstr_guid(&output_types[i].guidSubtype));
    }
    for (; expect->outputs[i].subtype; ++i)
        ok(0, "missing output[%u] subtype %s\n", i, debugstr_guid(expect->outputs[i].subtype));
    for (; i < output_count; ++i)
        ok(0, "extra output[%u] subtype %s\n", i, debugstr_guid(&output_types[i].guidSubtype));

    CoTaskMemFree(output_types);
    CoTaskMemFree(input_types);
    CoTaskMemFree(name);
}

static void check_dmo_get_info(const GUID *class_id, const struct transform_info *expect)
{
    DWORD input_count = 0, output_count = 0;
    DMO_PARTIAL_MEDIATYPE output[32] = {{{0}}};
    DMO_PARTIAL_MEDIATYPE input[32] = {{{0}}};
    WCHAR name[80];
    HRESULT hr;
    int i;

    hr = DMOGetName(class_id, name);
    ok(hr == S_OK, "DMOGetName returned %#lx\n", hr);
    ok(!wcscmp(name, expect->name), "got name %s\n", debugstr_w(name));

    hr = DMOGetTypes(class_id, ARRAY_SIZE(input), &input_count, input,
            ARRAY_SIZE(output), &output_count, output);
    ok(hr == S_OK, "DMOGetTypes returned %#lx\n", hr);

    for (i = 0; i < input_count && expect->inputs[i].subtype; ++i)
    {
        ok(IsEqualGUID(&input[i].type, expect->major_type),
                "got input[%u] major %s\n", i, debugstr_guid(&input[i].type));
        ok(IsEqualGUID(&input[i].subtype, expect->inputs[i].subtype),
                "got input[%u] subtype %s\n", i, debugstr_guid(&input[i].subtype));
    }
    for (; expect->inputs[i].subtype; ++i)
        ok(0, "missing input[%u] subtype %s\n", i, debugstr_guid(expect->inputs[i].subtype));
    for (; i < input_count; ++i)
        ok(0, "extra input[%u] subtype %s\n", i, debugstr_guid(&input[i].subtype));

    for (i = 0; expect->outputs[i].subtype; ++i)
    {
        ok(IsEqualGUID(&output[i].type, expect->major_type),
                "got output[%u] major %s\n", i, debugstr_guid(&output[i].type));
        ok(IsEqualGUID(&output[i].subtype, expect->outputs[i].subtype),
                "got output[%u] subtype %s\n", i, debugstr_guid(&output[i].subtype));
    }
    for (; expect->outputs[i].subtype; ++i)
        ok(0, "missing output[%u] subtype %s\n", i, debugstr_guid(expect->outputs[i].subtype));
    for (; i < output_count; ++i)
        ok(0, "extra output[%u] subtype %s\n", i, debugstr_guid(&output[i].subtype));
}

void init_media_type(IMFMediaType *mediatype, const struct attribute_desc *desc, ULONG limit)
{
    HRESULT hr;
    ULONG i;

    hr = IMFMediaType_DeleteAllItems(mediatype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < limit && desc[i].key; ++i)
    {
        hr = IMFMediaType_SetItem(mediatype, desc[i].key, &desc[i].value);
        ok(hr == S_OK, "SetItem %s returned %#lx\n", debugstr_a(desc[i].name), hr);
    }
}

static void init_dmo_media_type_video(DMO_MEDIA_TYPE *media_type, const GUID *subtype,
        const LONG width, const LONG height, const REFERENCE_TIME time_per_frame)
{
    UINT32 image_size = 0, extra_bytes = subtype_to_extra_bytes(subtype);
    VIDEOINFOHEADER *header = (VIDEOINFOHEADER *)(media_type + 1);
    BOOL compressed = is_compressed_subtype(subtype);

    MFCalculateImageSize(subtype, width, height, &image_size);
    memset(media_type, 0, sizeof(*media_type) + sizeof(*header) + extra_bytes);

    header->rcSource.top = 0;
    header->rcSource.left = 0;
    header->rcSource.right = width;
    header->rcSource.bottom = height;
    header->rcTarget.top = 0;
    header->rcTarget.left = 0;
    header->rcTarget.right = width;
    header->rcTarget.bottom = height;
    header->AvgTimePerFrame = time_per_frame;
    header->bmiHeader.biSize = sizeof(header->bmiHeader);
    header->bmiHeader.biWidth = width;
    header->bmiHeader.biHeight = height;
    header->bmiHeader.biPlanes = 1;
    header->bmiHeader.biBitCount = subtype_to_bpp(subtype);
    header->bmiHeader.biCompression = subtype_to_tag(subtype);
    header->bmiHeader.biSizeImage = image_size;
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB8))
    {
        header->bmiHeader.biClrUsed = 226;
        header->bmiHeader.biClrImportant =  header->bmiHeader.biClrUsed;
    }

    media_type->majortype = MEDIATYPE_Video;
    media_type->subtype = *subtype;
    media_type->bFixedSizeSamples = !compressed;
    media_type->bTemporalCompression = compressed;
    media_type->lSampleSize = image_size;
    media_type->formattype = FORMAT_VideoInfo;
    media_type->pUnk = NULL;
    media_type->cbFormat = sizeof(*header) + extra_bytes;
    media_type->pbFormat = (BYTE *)header;
}

static void init_dmo_media_type_audio(DMO_MEDIA_TYPE *media_type,
        const GUID *subtype, UINT channel_count, UINT rate, UINT bits_per_sample)
{
    WAVEFORMATEX *format = (WAVEFORMATEX *)(media_type + 1);
    DWORD extra_bytes = subtype_to_extra_bytes(subtype);

    memset(media_type, 0, sizeof(*media_type) + sizeof(*format) + extra_bytes);

    media_type->majortype = MEDIATYPE_Audio;
    media_type->subtype = *subtype;
    media_type->bFixedSizeSamples = TRUE;
    media_type->bTemporalCompression = FALSE;
    media_type->lSampleSize = 0;
    media_type->formattype = FORMAT_WaveFormatEx;
    media_type->pUnk = NULL;
    media_type->cbFormat = sizeof(*format) + extra_bytes;
    media_type->pbFormat = (BYTE *)format;

    format->wFormatTag = subtype_to_tag(subtype);
    format->nChannels = channel_count;
    format->nSamplesPerSec = rate;
    format->wBitsPerSample = bits_per_sample;
    format->nBlockAlign = channel_count * bits_per_sample / 8;
    format->nAvgBytesPerSec = format->nBlockAlign * rate;
    format->cbSize = extra_bytes;
}

static void check_mft_optional_methods(IMFTransform *transform, DWORD output_count)
{
    DWORD in_id, out_id, in_count, out_count, in_min, in_max, out_min, out_max;
    PROPVARIANT propvar = {.vt = VT_EMPTY};
    IMFMediaEvent *event;
    HRESULT hr;

    in_min = in_max = out_min = out_max = 0xdeadbeef;
    hr = IMFTransform_GetStreamLimits(transform, &in_min, &in_max, &out_min, &out_max);
    ok(hr == S_OK, "GetStreamLimits returned %#lx\n", hr);
    ok(in_min == 1, "got input_min %lu\n", in_min);
    ok(in_max == 1, "got input_max %lu\n", in_max);
    ok(out_min == output_count, "got output_min %lu\n", out_min);
    ok(out_max == output_count, "got output_max %lu\n", out_max);

    in_count = out_count = 0xdeadbeef;
    hr = IMFTransform_GetStreamCount(transform, &in_count, &out_count);
    ok(hr == S_OK, "GetStreamCount returned %#lx\n", hr);
    ok(in_count == 1, "got input_count %lu\n", in_count);
    ok(out_count == output_count, "got output_count %lu\n", out_count);

    in_count = out_count = 1;
    in_id = out_id = 0xdeadbeef;
    hr = IMFTransform_GetStreamIDs(transform, in_count, &in_id, out_count, &out_id);
    ok(hr == E_NOTIMPL, "GetStreamIDs returned %#lx\n", hr);

    hr = IMFTransform_DeleteInputStream(transform, 0);
    ok(hr == E_NOTIMPL, "DeleteInputStream returned %#lx\n", hr);
    hr = IMFTransform_DeleteInputStream(transform, 1);
    ok(hr == E_NOTIMPL, "DeleteInputStream returned %#lx\n", hr);

    hr = IMFTransform_AddInputStreams(transform, 0, NULL);
    ok(hr == E_NOTIMPL, "AddInputStreams returned %#lx\n", hr);
    in_id = 0xdeadbeef;
    hr = IMFTransform_AddInputStreams(transform, 1, &in_id);
    ok(hr == E_NOTIMPL, "AddInputStreams returned %#lx\n", hr);

    hr = IMFTransform_SetOutputBounds(transform, 0, 0);
    ok(hr == E_NOTIMPL || hr == S_OK, "SetOutputBounds returned %#lx\n", hr);

    hr = MFCreateMediaEvent(MEEndOfStream, &GUID_NULL, S_OK, &propvar, &event);
    ok(hr == S_OK, "MFCreateMediaEvent returned %#lx\n", hr);
    hr = IMFTransform_ProcessEvent(transform, 0, NULL);
    ok(hr == E_NOTIMPL || hr == E_POINTER || hr == E_INVALIDARG, "ProcessEvent returned %#lx\n", hr);
    hr = IMFTransform_ProcessEvent(transform, 1, event);
    ok(hr == E_NOTIMPL, "ProcessEvent returned %#lx\n", hr);
    hr = IMFTransform_ProcessEvent(transform, 0, event);
    ok(hr == E_NOTIMPL, "ProcessEvent returned %#lx\n", hr);
    IMFMediaEvent_Release(event);
}

static void check_mft_get_attributes(IMFTransform *transform, const struct attribute_desc *expect_transform_attributes,
        BOOL expect_output_attributes)
{
    IMFAttributes *attributes, *tmp_attributes;
    UINT32 count;
    HRESULT hr;
    ULONG ref;

    hr = IMFTransform_GetAttributes(transform, &attributes);
    todo_wine_if(expect_transform_attributes && hr == E_NOTIMPL)
    ok(hr == (expect_transform_attributes ? S_OK : E_NOTIMPL), "GetAttributes returned %#lx\n", hr);
    if (hr == S_OK)
    {
        ok(hr == S_OK, "GetAttributes returned %#lx\n", hr);
        check_attributes(attributes, expect_transform_attributes, -1);

        hr = IMFTransform_GetAttributes(transform, &tmp_attributes);
        ok(hr == S_OK, "GetAttributes returned %#lx\n", hr);
        ok(attributes == tmp_attributes, "got attributes %p\n", tmp_attributes);
        IMFAttributes_Release(tmp_attributes);

        ref = IMFAttributes_Release(attributes);
        ok(ref == 1, "Release returned %lu\n", ref);
    }

    hr = IMFTransform_GetOutputStreamAttributes(transform, 0, &attributes);
    todo_wine_if(expect_output_attributes && hr == E_NOTIMPL)
    ok(hr == (expect_output_attributes ? S_OK : E_NOTIMPL)
            || broken(hr == MF_E_UNSUPPORTED_REPRESENTATION) /* Win7 */,
            "GetOutputStreamAttributes returned %#lx\n", hr);
    if (hr == S_OK)
    {
        ok(hr == S_OK, "GetOutputStreamAttributes returned %#lx\n", hr);

        count = 0xdeadbeef;
        hr = IMFAttributes_GetCount(attributes, &count);
        ok(hr == S_OK, "GetCount returned %#lx\n", hr);
        ok(!count, "got %u attributes\n", count);

        hr = IMFTransform_GetOutputStreamAttributes(transform, 0, &tmp_attributes);
        ok(hr == S_OK, "GetAttributes returned %#lx\n", hr);
        ok(attributes == tmp_attributes, "got attributes %p\n", tmp_attributes);
        IMFAttributes_Release(tmp_attributes);

        ref = IMFAttributes_Release(attributes);
        ok(ref == 1, "Release returned %lu\n", ref);

        hr = IMFTransform_GetOutputStreamAttributes(transform, 0, NULL);
        ok(hr == E_NOTIMPL || hr == E_POINTER, "GetOutputStreamAttributes returned %#lx\n", hr);
        hr = IMFTransform_GetOutputStreamAttributes(transform, 1, &attributes);
        ok(hr == MF_E_INVALIDSTREAMNUMBER, "GetOutputStreamAttributes returned %#lx\n", hr);
    }

    hr = IMFTransform_GetInputStreamAttributes(transform, 0, &attributes);
    ok(hr == E_NOTIMPL || broken(hr == MF_E_UNSUPPORTED_REPRESENTATION) /* Win7 */,
            "GetInputStreamAttributes returned %#lx\n", hr);
}

#define check_mft_input_stream_info(a, b) check_mft_input_stream_info_(__LINE__, a, b)
static void check_mft_input_stream_info_(int line, MFT_INPUT_STREAM_INFO *value, const MFT_INPUT_STREAM_INFO *expect)
{
    check_member_(__FILE__, line, *value, *expect, "%I64d", hnsMaxLatency);
    check_member_(__FILE__, line, *value, *expect, "%#lx", dwFlags);
    check_member_(__FILE__, line, *value, *expect, "%#lx", cbSize);
    check_member_(__FILE__, line, *value, *expect, "%#lx", cbMaxLookahead);
    check_member_(__FILE__, line, *value, *expect, "%#lx", cbAlignment);
}

#define check_mft_get_input_stream_info(a, b, c) check_mft_get_input_stream_info_(__LINE__, a, b, c)
static void check_mft_get_input_stream_info_(int line, IMFTransform *transform, HRESULT expect_hr, const MFT_INPUT_STREAM_INFO *expect)
{
    MFT_INPUT_STREAM_INFO info, empty = {0};
    HRESULT hr;

    memset(&info, 0xcd, sizeof(info));
    hr = IMFTransform_GetInputStreamInfo(transform, 0, &info);
    ok_(__FILE__, line)(hr == expect_hr, "GetInputStreamInfo returned %#lx\n", hr);
    check_mft_input_stream_info_(line, &info, expect ? expect : &empty);
}

#define check_mft_output_stream_info(a, b) check_mft_output_stream_info_(__LINE__, a, b)
static void check_mft_output_stream_info_(int line, MFT_OUTPUT_STREAM_INFO *value, const MFT_OUTPUT_STREAM_INFO *expect)
{
    check_member_(__FILE__, line, *value, *expect, "%#lx", dwFlags);
    check_member_(__FILE__, line, *value, *expect, "%#lx", cbSize);
    check_member_(__FILE__, line, *value, *expect, "%#lx", cbAlignment);
}

#define check_mft_get_output_stream_info(a, b, c) check_mft_get_output_stream_info_(__LINE__, a, b, c)
static void check_mft_get_output_stream_info_(int line, IMFTransform *transform, HRESULT expect_hr, const MFT_OUTPUT_STREAM_INFO *expect)
{
    MFT_OUTPUT_STREAM_INFO info, empty = {0};
    HRESULT hr;

    memset(&info, 0xcd, sizeof(info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &info);
    ok_(__FILE__, line)(hr == expect_hr, "GetOutputStreamInfo returned %#lx\n", hr);
    check_mft_output_stream_info_(line, &info, expect ? expect : &empty);
}

#define check_mft_set_input_type_required(a, b) check_mft_set_input_type_required_(__LINE__, a, b)
static void check_mft_set_input_type_required_(int line, IMFTransform *transform, const struct attribute_desc *attributes)
{
    const struct attribute_desc *attr;
    IMFMediaType *media_type;
    HRESULT hr;
    ULONG ref;

    hr = MFCreateMediaType(&media_type);
    ok_(__FILE__, line)(hr == S_OK, "MFCreateMediaType returned hr %#lx.\n", hr);
    init_media_type(media_type, attributes, -1);

    for (attr = attributes; attr && attr->key; attr++)
    {
        winetest_push_context("%s", debugstr_a(attr->name));

        hr = IMFMediaType_DeleteItem(media_type, attr->key);
        ok_(__FILE__, line)(hr == S_OK, "DeleteItem returned %#lx\n", hr);

        hr = IMFTransform_SetInputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
        todo_wine_if(attr->todo)
        ok_(__FILE__, line)(FAILED(hr) == attr->required, "SetInputType returned %#lx.\n", hr);

        if (attr->required_set)
        {
            hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
            ok_(__FILE__, line)(FAILED(hr), "SetInputType Succeeded.\n");
            hr = IMFTransform_SetInputType(transform, 0, NULL, 0);
            ok_(__FILE__, line)(hr == S_OK, "Failed to clear input type.\n");
        }

        hr = IMFMediaType_SetItem(media_type, attr->key, &attr->value);
        ok_(__FILE__, line)(hr == S_OK, "SetItem returned %#lx\n", hr);

        winetest_pop_context();
    }

    hr = IMFTransform_SetInputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
    ok_(__FILE__, line)(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    ref = IMFMediaType_Release(media_type);
    ok_(__FILE__, line)(!ref, "Release returned %lu\n", ref);
}

#define check_mft_set_input_type(a, b, c) check_mft_set_input_type_(__LINE__, a, b, c, FALSE)
static void check_mft_set_input_type_(int line, IMFTransform *transform, const struct attribute_desc *attributes,
        HRESULT expect_hr, BOOL todo)
{
    IMFMediaType *media_type;
    HRESULT hr;

    hr = MFCreateMediaType(&media_type);
    ok_(__FILE__, line)(hr == S_OK, "MFCreateMediaType returned hr %#lx.\n", hr);
    init_media_type(media_type, attributes, -1);

    hr = IMFTransform_SetInputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
    todo_wine_if(todo)
    ok_(__FILE__, line)(hr == expect_hr, "SetInputType returned %#lx.\n", hr);
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    todo_wine_if(todo)
    ok_(__FILE__, line)(hr == expect_hr, "SetInputType returned %#lx.\n", hr);

    IMFMediaType_Release(media_type);
}

#define check_mft_get_input_current_type(a, b) check_mft_get_input_current_type_(__LINE__, a, b, FALSE, FALSE)
static void check_mft_get_input_current_type_(int line, IMFTransform *transform, const struct attribute_desc *attributes,
        BOOL todo_current, BOOL todo_compare)
{
    HRESULT hr, expect_hr = attributes ? S_OK : MF_E_TRANSFORM_TYPE_NOT_SET;
    IMFMediaType *media_type, *current_type;
    BOOL result;

    hr = IMFTransform_GetInputCurrentType(transform, 0, &current_type);
    todo_wine_if(todo_current)
    ok_(__FILE__, line)(hr == expect_hr, "GetInputCurrentType returned hr %#lx.\n", hr);
    if (FAILED(hr))
        return;

    hr = MFCreateMediaType(&media_type);
    ok_(__FILE__, line)(hr == S_OK, "MFCreateMediaType returned hr %#lx.\n", hr);
    init_media_type(media_type, attributes, -1);

    hr = IMFMediaType_Compare(current_type, (IMFAttributes *)media_type,
            MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok_(__FILE__, line)(hr == S_OK, "Compare returned hr %#lx.\n", hr);
    todo_wine_if(todo_compare)
    ok_(__FILE__, line)(result, "got result %u.\n", !!result);

    check_attributes_(__FILE__, line, (IMFAttributes *)current_type, attributes, -1);

    IMFMediaType_Release(media_type);
    IMFMediaType_Release(current_type);
}

#define check_mft_set_output_type_required(a, b) check_mft_set_output_type_required_(__LINE__, a, b)
static void check_mft_set_output_type_required_(int line, IMFTransform *transform, const struct attribute_desc *attributes)
{
    const struct attribute_desc *attr;
    IMFMediaType *media_type;
    HRESULT hr;
    ULONG ref;

    hr = MFCreateMediaType(&media_type);
    ok_(__FILE__, line)(hr == S_OK, "MFCreateMediaType returned hr %#lx.\n", hr);
    init_media_type(media_type, attributes, -1);

    for (attr = attributes; attr && attr->key; attr++)
    {
        winetest_push_context("%s", debugstr_a(attr->name));

        hr = IMFMediaType_DeleteItem(media_type, attr->key);
        ok_(__FILE__, line)(hr == S_OK, "DeleteItem returned %#lx\n", hr);

        hr = IMFTransform_SetOutputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
        ok_(__FILE__, line)(FAILED(hr) == attr->required, "SetOutputType returned %#lx.\n", hr);

        if (attr->required_set)
        {
            hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
            ok_(__FILE__, line)(FAILED(hr), "SetOutputType Succeeded.\n");
            hr = IMFTransform_SetOutputType(transform, 0, NULL, 0);
            ok_(__FILE__, line)(hr == S_OK, "Failed to clear output type.\n");
        }

        hr = IMFMediaType_SetItem(media_type, attr->key, &attr->value);
        ok_(__FILE__, line)(hr == S_OK, "SetItem returned %#lx\n", hr);

        winetest_pop_context();
    }

    hr = IMFTransform_SetOutputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
    ok_(__FILE__, line)(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    ref = IMFMediaType_Release(media_type);
    ok_(__FILE__, line)(!ref, "Release returned %lu\n", ref);
}

#define check_mft_set_output_type(a, b, c) check_mft_set_output_type_(__LINE__, a, b, c, FALSE)
static void check_mft_set_output_type_(int line, IMFTransform *transform, const struct attribute_desc *attributes,
        HRESULT expect_hr, BOOL todo)
{
    IMFMediaType *media_type;
    HRESULT hr;

    hr = MFCreateMediaType(&media_type);
    ok_(__FILE__, line)(hr == S_OK, "MFCreateMediaType returned hr %#lx.\n", hr);
    init_media_type(media_type, attributes, -1);

    hr = IMFTransform_SetOutputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
    ok_(__FILE__, line)(hr == expect_hr, "SetOutputType returned %#lx.\n", hr);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    todo_wine_if(todo)
    ok_(__FILE__, line)(hr == expect_hr, "SetOutputType returned %#lx.\n", hr);

    IMFMediaType_Release(media_type);
}

#define check_mft_get_output_current_type(a, b) check_mft_get_output_current_type_(__LINE__, a, b, FALSE, FALSE)
static void check_mft_get_output_current_type_(int line, IMFTransform *transform, const struct attribute_desc *attributes,
        BOOL todo_current, BOOL todo_compare)
{
    HRESULT hr, expect_hr = attributes ? S_OK : MF_E_TRANSFORM_TYPE_NOT_SET;
    IMFMediaType *media_type, *current_type;
    BOOL result;

    hr = IMFTransform_GetOutputCurrentType(transform, 0, &current_type);
    todo_wine_if(todo_current)
    ok_(__FILE__, line)(hr == expect_hr, "GetOutputCurrentType returned hr %#lx.\n", hr);
    if (FAILED(hr))
        return;

    hr = MFCreateMediaType(&media_type);
    ok_(__FILE__, line)(hr == S_OK, "MFCreateMediaType returned hr %#lx.\n", hr);
    init_media_type(media_type, attributes, -1);

    hr = IMFMediaType_Compare(current_type, (IMFAttributes *)media_type,
            MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok_(__FILE__, line)(hr == S_OK, "Compare returned hr %#lx.\n", hr);
    todo_wine_if(todo_compare)
    ok_(__FILE__, line)(result, "got result %u.\n", !!result);

    check_attributes_(__FILE__, line, (IMFAttributes *)current_type, attributes, -1);

    IMFMediaType_Release(media_type);
    IMFMediaType_Release(current_type);
}

#define check_mft_process_output(a, b, c) check_mft_process_output_(__LINE__, a, b, c)
static HRESULT check_mft_process_output_(int line, IMFTransform *transform, IMFSample *output_sample, DWORD *output_status)
{
    static const DWORD expect_flags = MFT_OUTPUT_DATA_BUFFER_INCOMPLETE | MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE
            | MFT_OUTPUT_DATA_BUFFER_STREAM_END | MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE;
    MFT_OUTPUT_DATA_BUFFER output[3];
    HRESULT hr, ret;
    DWORD status;

    status = 0;
    memset(&output, 0, sizeof(output));
    output[0].pSample = output_sample;
    output[0].dwStreamID = 0;
    ret = IMFTransform_ProcessOutput(transform, 0, 1, output, &status);
    ok_(__FILE__, line)(output[0].dwStreamID == 0, "got dwStreamID %#lx\n", output[0].dwStreamID);
    ok_(__FILE__, line)(output[0].pEvents == NULL, "got pEvents %p\n", output[0].pEvents);
    ok_(__FILE__, line)(output[0].pSample == output_sample, "got pSample %p\n", output[0].pSample);
    ok_(__FILE__, line)((output[0].dwStatus & ~expect_flags) == 0
            || broken((output[0].dwStatus & ~expect_flags) == 6) /* Win7 */
            || broken((output[0].dwStatus & ~expect_flags) == 7) /* Win7 */,
            "got dwStatus %#lx\n", output[0].dwStatus);
    *output_status = output[0].dwStatus & expect_flags;

    if (!output_sample)
        ok_(__FILE__, line)(status == 0, "got status %#lx\n", status);
    else if (ret == MF_E_TRANSFORM_STREAM_CHANGE)
        ok_(__FILE__, line)(status == MFT_PROCESS_OUTPUT_STATUS_NEW_STREAMS,
                "got status %#lx\n", status);
    else
    {
        if (*output_status & MFT_OUTPUT_DATA_BUFFER_INCOMPLETE)
        {
            hr = IMFSample_SetUINT32(output_sample, &mft_output_sample_incomplete, 1);
            ok_(__FILE__, line)(hr == S_OK, "SetUINT32 returned %#lx\n", hr);
        }
        else
        {
            hr = IMFSample_DeleteItem(output_sample, &mft_output_sample_incomplete);
            ok_(__FILE__, line)(hr == S_OK, "DeleteItem returned %#lx\n", hr);
        }
        ok_(__FILE__, line)(status == 0, "got status %#lx\n", status);
    }

    return ret;
}

DWORD compare_nv12(const BYTE *data, DWORD *length, const SIZE *size, const RECT *rect, const BYTE *expect)
{
    DWORD x, y, data_size, diff = 0, width = size->cx, height = size->cy;

    /* skip BMP header and RGB data from the dump */
    data_size = *(DWORD *)(expect + 2);
    *length = *length + data_size;
    expect = expect + data_size;

    for (y = 0; y < height; y++, data += width, expect += width)
    {
        if (y < rect->top || y >= rect->bottom) continue;
        for (x = 0; x < width; x++)
        {
            if (x < rect->left || x >= rect->right) continue;
            diff += abs((int)expect[x] - (int)data[x]);
        }
    }

    for (y = 0; y < height; y += 2, data += width, expect += width)
    {
        if (y < rect->top || y >= rect->bottom) continue;
        for (x = 0; x < width; x += 2)
        {
            if (x < rect->left || x >= rect->right) continue;
            diff += abs((int)expect[x + 0] - (int)data[x + 0]);
            diff += abs((int)expect[x + 1] - (int)data[x + 1]);
        }
    }

    data_size = (rect->right - rect->left) * (rect->bottom - rect->top) * 3 / 2;
    return diff * 100 / 256 / data_size;
}

DWORD compare_i420(const BYTE *data, DWORD *length, const SIZE *size, const RECT *rect, const BYTE *expect)
{
    DWORD i, x, y, data_size, diff = 0, width = size->cx, height = size->cy;

    /* skip BMP header and RGB data from the dump */
    data_size = *(DWORD *)(expect + 2);
    *length = *length + data_size;
    expect = expect + data_size;

    for (y = 0; y < height; y++, data += width, expect += width)
    {
        if (y < rect->top || y >= rect->bottom) continue;
        for (x = 0; x < width; x++)
        {
            if (x < rect->left || x >= rect->right) continue;
            diff += abs((int)expect[x] - (int)data[x]);
        }
    }

    for (i = 0; i < 2; ++i) for (y = 0; y < height; y += 2, data += width / 2, expect += width / 2)
    {
        if (y < rect->top || y >= rect->bottom) continue;
        for (x = 0; x < width; x += 2)
        {
            if (x < rect->left || x >= rect->right) continue;
            diff += abs((int)expect[x / 2] - (int)data[x / 2]);
        }
    }

    data_size = (rect->right - rect->left) * (rect->bottom - rect->top) * 3 / 2;
    return diff * 100 / 256 / data_size;
}

static DWORD compare_abgr32(const BYTE *data, DWORD *length, const SIZE *size, const RECT *rect, const BYTE *expect)
{
    DWORD x, y, data_size, diff = 0, width = size->cx, height = size->cy;

    /* skip BMP header from the dump */
    data_size = *(DWORD *)(expect + 2 + 2 * sizeof(DWORD));
    *length = *length + data_size;
    expect = expect + data_size;

    for (y = 0; y < height; y++, data += width * 4, expect += width * 4)
    {
        if (y < rect->top || y >= rect->bottom) continue;
        for (x = 0; x < width; x++)
        {
            if (x < rect->left || x >= rect->right) continue;
            diff += abs((int)expect[4 * x + 0] - (int)data[4 * x + 0]);
            diff += abs((int)expect[4 * x + 1] - (int)data[4 * x + 1]);
            diff += abs((int)expect[4 * x + 2] - (int)data[4 * x + 2]);
            diff += abs((int)expect[4 * x + 3] - (int)data[4 * x + 3]);
        }
    }

    data_size = (rect->right - rect->left) * (rect->bottom - rect->top) * 4;
    return diff * 100 / 256 / data_size;
}

static DWORD compare_rgb(const BYTE *data, DWORD *length, const SIZE *size, const RECT *rect, const BYTE *expect, UINT bits)
{
    DWORD x, y, step = bits / 8, data_size, diff = 0, width = size->cx, height = size->cy;

    /* skip BMP header from the dump */
    data_size = *(DWORD *)(expect + 2 + 2 * sizeof(DWORD));
    *length = *length + data_size;
    expect = expect + data_size;

    for (y = 0; y < height; y++, data += width * step, expect += width * step)
    {
        if (y < rect->top || y >= rect->bottom) continue;
        for (x = 0; x < width; x++)
        {
            if (x < rect->left || x >= rect->right) continue;
            diff += abs((int)expect[step * x + 0] - (int)data[step * x + 0]);
            diff += abs((int)expect[step * x + 1] - (int)data[step * x + 1]);
            if (step >= 3) diff += abs((int)expect[step * x + 2] - (int)data[step * x + 2]);
        }
    }

    data_size = (rect->right - rect->left) * (rect->bottom - rect->top) * min(step, 3);
    return diff * 100 / 256 / data_size;
}

DWORD compare_rgb32(const BYTE *data, DWORD *length, const SIZE *size, const RECT *rect, const BYTE *expect)
{
    return compare_rgb(data, length, size, rect, expect, 32);
}

DWORD compare_rgb24(const BYTE *data, DWORD *length, const SIZE *size, const RECT *rect, const BYTE *expect)
{
    return compare_rgb(data, length, size, rect, expect, 24);
}

DWORD compare_rgb16(const BYTE *data, DWORD *length, const SIZE *size, const RECT *rect, const BYTE *expect)
{
    return compare_rgb(data, length, size, rect, expect, 16);
}

DWORD compare_pcm16(const BYTE *data, DWORD *length, const SIZE *size, const RECT *rect, const BYTE *expect)
{
    const INT16 *data_pcm = (INT16 *)data, *expect_pcm = (INT16 *)expect;
    DWORD i, data_size = *length / 2, diff = 0;

    for (i = 0; i < data_size; i++)
        diff += abs((int)*expect_pcm++ - (int)*data_pcm++);

    return diff * 100 / 65536 / data_size;
}

static DWORD compare_bytes(const BYTE *data, DWORD *length, const SIZE *size, const RECT *rect, const BYTE *expect)
{
    DWORD i, data_size = *length, diff = 0;

    for (i = 0; i < data_size; i++)
        diff += abs((int)*expect++ - (int)*data++);

    return diff * 100 / 256 / data_size;
}

static void dump_rgb(const BYTE *data, DWORD length, const SIZE *size, HANDLE output, UINT bits)
{
    DWORD width = size->cx, height = size->cy;
    static const char magic[2] = "BM";
    struct
    {
        DWORD length;
        DWORD reserved;
        DWORD offset;
        BITMAPINFOHEADER biHeader;
    } header =
    {
        .length = length + sizeof(header) + 2, .offset = sizeof(header) + 2,
        .biHeader =
        {
            .biSize = sizeof(BITMAPINFOHEADER), .biWidth = width, .biHeight = height, .biPlanes = 1,
            .biBitCount = bits, .biCompression = BI_RGB, .biSizeImage = width * height * (bits / 8),
        },
    };
    DWORD written;
    BOOL ret;

    ret = WriteFile(output, magic, sizeof(magic), &written, NULL);
    ok(ret, "WriteFile failed, error %lu\n", GetLastError());
    ok(written == sizeof(magic), "written %lu bytes\n", written);
    ret = WriteFile(output, &header, sizeof(header), &written, NULL);
    ok(ret, "WriteFile failed, error %lu\n", GetLastError());
    ok(written == sizeof(header), "written %lu bytes\n", written);
    ret = WriteFile(output, data, length, &written, NULL);
    ok(ret, "WriteFile failed, error %lu\n", GetLastError());
    ok(written == length, "written %lu bytes\n", written);
}

void dump_rgb32(const BYTE *data, DWORD length, const SIZE *size, HANDLE output)
{
    return dump_rgb(data, length, size, output, 32);
}

void dump_rgb24(const BYTE *data, DWORD length, const SIZE *size, HANDLE output)
{
    return dump_rgb(data, length, size, output, 24);
}

void dump_rgb16(const BYTE *data, DWORD length, const SIZE *size, HANDLE output)
{
    return dump_rgb(data, length, size, output, 16);
}

void dump_nv12(const BYTE *data, DWORD length, const SIZE *size, HANDLE output)
{
    DWORD written, x, y, width = size->cx, height = size->cy;
    BYTE *rgb32_data = malloc(width * height * 4), *rgb32 = rgb32_data;
    BOOL ret;

    for (y = 0; y < height; y++) for (x = 0; x < width; x++)
    {
        *rgb32++ = data[width * y + x];
        *rgb32++ = data[width * height + width * (y / 2) + (x & ~1) + 0];
        *rgb32++ = data[width * height + width * (y / 2) + (x & ~1) + 1];
        *rgb32++ = 0xff;
    }

    dump_rgb32(rgb32_data, width * height * 4, size, output);
    free(rgb32_data);

    ret = WriteFile(output, data, length, &written, NULL);
    ok(ret, "WriteFile failed, error %lu\n", GetLastError());
    ok(written == length, "written %lu bytes\n", written);
}

void dump_i420(const BYTE *data, DWORD length, const SIZE *size, HANDLE output)
{
    DWORD written, x, y, width = size->cx, height = size->cy;
    BYTE *rgb32_data = malloc(width * height * 4), *rgb32 = rgb32_data;
    BOOL ret;

    for (y = 0; y < height; y++) for (x = 0; x < width; x++)
    {
        *rgb32++ = data[width * y + x];
        *rgb32++ = data[width * height + (width / 2) * (y / 2) + x / 2];
        *rgb32++ = data[width * height + (width / 2) * (y / 2) + (width / 2) * (height / 2) + x / 2];
        *rgb32++ = 0xff;
    }

    dump_rgb32(rgb32_data, width * height * 4, size, output);
    free(rgb32_data);

    ret = WriteFile(output, data, length, &written, NULL);
    ok(ret, "WriteFile failed, error %lu\n", GetLastError());
    ok(written == length, "written %lu bytes\n", written);
}

typedef void (*enum_mf_media_buffers_cb)(IMFMediaBuffer *buffer, const struct buffer_desc *desc, void *context);
static void enum_mf_media_buffers(IMFSample *sample, const struct sample_desc *sample_desc,
        enum_mf_media_buffers_cb callback, void *context)
{
    IMFMediaBuffer *buffer;
    HRESULT hr;
    DWORD i;

    for (i = 0; SUCCEEDED(hr = IMFSample_GetBufferByIndex(sample, i, &buffer)); i++)
    {
        winetest_push_context("buffer %lu", i);
        ok(hr == S_OK, "GetBufferByIndex returned %#lx\n", hr);
        ok(i < sample_desc->buffer_count, "got unexpected buffer\n");

        callback(buffer, sample_desc->buffers + i, context);

        IMFMediaBuffer_Release(buffer);
        winetest_pop_context();
    }
    ok(hr == E_INVALIDARG, "GetBufferByIndex returned %#lx\n", hr);
}

struct enum_mf_sample_state
{
    const struct sample_desc *next_sample;
    struct sample_desc sample;
};

typedef void (*enum_mf_sample_cb)(IMFSample *sample, const struct sample_desc *sample_desc, void *context);
static void enum_mf_samples(IMFCollection *samples, const struct sample_desc *collection_desc,
        enum_mf_sample_cb callback, void *context)
{
    struct enum_mf_sample_state state = {.next_sample = collection_desc};
    IMFSample *sample;
    HRESULT hr;
    DWORD i;

    for (i = 0; SUCCEEDED(hr = IMFCollection_GetElement(samples, i, (IUnknown **)&sample)); i++)
    {
        winetest_push_context("sample %lu", i);
        ok(hr == S_OK, "GetElement returned %#lx\n", hr);

        state.sample.sample_time += state.sample.sample_duration;
        if (!state.sample.repeat_count--)
            state.sample = *state.next_sample++;

        callback(sample, &state.sample, context);

        IMFSample_Release(sample);
        winetest_pop_context();
    }
    ok(hr == E_INVALIDARG, "GetElement returned %#lx\n", hr);
}

static void dump_buffer_data(const struct buffer_desc *buffer_desc, void *data, DWORD length, HANDLE output)
{
    SIZE size = buffer_desc->size;
    DWORD ret, written;

    if (buffer_desc->dump)
        buffer_desc->dump(data, length, &size, output);
    else
    {
        if (buffer_desc->length == -1)
        {
            ret = WriteFile(output, &length, sizeof(length), &written, NULL);
            ok(ret, "WriteFile failed, error %lu\n", GetLastError());
            ok(written == sizeof(length), "written %lu bytes\n", written);
        }

        ret = WriteFile(output, data, length, &written, NULL);
        ok(ret, "WriteFile failed, error %lu\n", GetLastError());
        ok(written == length, "written %lu bytes\n", written);
    }
}

static void dump_mf_2d_buffer(IMFMediaBuffer *buffer, const struct buffer_desc *buffer_desc, HANDLE output)
{
    IMF2DBuffer2 *buffer2d;
    DWORD length;
    LONG stride;
    BYTE *scanline;
    HRESULT hr;
    BYTE *data;

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&buffer2d);
    ok(hr == S_OK, "QueryInterface IMF2DBuffer2 returned %#lx\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(buffer2d, MF2DBuffer_LockFlags_Read, &scanline, &stride, &data, &length);
    ok(hr == S_OK, "Lock2DSize returned %#lx\n", hr);
    dump_buffer_data(buffer_desc, data, length, output);
    hr = IMF2DBuffer2_Unlock2D(buffer2d);
    ok(hr == S_OK, "Unlock2D returned %#lx\n", hr);
    IMF2DBuffer2_Release(buffer2d);
}

static void dump_mf_media_buffer(IMFMediaBuffer *buffer, const struct buffer_desc *buffer_desc, HANDLE output)
{
    DWORD length;
    HRESULT hr;
    BYTE *data;

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, &length);
    ok(hr == S_OK, "Lock returned %#lx\n", hr);
    dump_buffer_data(buffer_desc, data, length, output);
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unlock returned %#lx\n", hr);
}

static void dump_mf_sample(IMFSample *sample, const struct sample_desc *sample_desc, HANDLE output)
{
    enum_mf_media_buffers(sample, sample_desc, dump_mf_media_buffer, output);
}

static void dump_mf_sample_2d(IMFSample *sample, const struct sample_desc *sample_desc, HANDLE output)
{
    enum_mf_media_buffers(sample, sample_desc, dump_mf_2d_buffer, output);
}

static void dump_mf_sample_collection(IMFCollection *samples, const struct sample_desc *collection_desc,
        const WCHAR *output_filename, BOOL use_2d_buffer)
{
    WCHAR path[MAX_PATH];
    HANDLE output;

    GetTempPathW(ARRAY_SIZE(path), path);
    lstrcatW(path, output_filename);

    output = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(output != INVALID_HANDLE_VALUE, "CreateFileW failed, error %lu\n", GetLastError());

    enum_mf_samples(samples, collection_desc, use_2d_buffer ? dump_mf_sample_2d : dump_mf_sample, output);

    trace("created %s\n", debugstr_w(path));
    CloseHandle(output);
}

#define check_mf_2d_buffer(a, b, c) check_mf_2d_buffer_(__FILE__, __LINE__, a, b, c)
static DWORD check_mf_2d_buffer_(const char *file, int line, IMF2DBuffer2 *buffer, const struct buffer_desc *expect,
        const BYTE **expect_data, DWORD *expect_data_len)
{
    DWORD length, diff = 0, expect_length = expect->length;
    BYTE *scanline;
    LONG stride;
    HRESULT hr;
    BYTE *data;

    hr = IMF2DBuffer2_Lock2DSize(buffer, MF2DBuffer_LockFlags_Read, &scanline, &stride, &data, &length);
    ok_(file, line)(hr == S_OK, "Lock2DSize returned %#lx\n", hr);
    todo_wine_if(expect->todo_length)
    ok_(file, line)(length == expect_length, "got length %ld, expected %ld\n", length, expect_length);

    if (*expect_data)
    {
        if (*expect_data_len < length)
            todo_wine_if(expect->todo_length)
            ok_(file, line)(0, "missing %#lx bytes\n", length - *expect_data_len);
        else if (!expect->compare)
            diff = compare_bytes(data, &length, NULL, NULL, *expect_data);
        else
            diff = expect->compare(data, &length, &expect->size, &expect->compare_rect, *expect_data);
    }

    hr = IMF2DBuffer2_Unlock2D(buffer);
    ok_(file, line)(hr == S_OK, "Unlock2D returned %#lx\n", hr);

    *expect_data = *expect_data + min(length, *expect_data_len);
    *expect_data_len = *expect_data_len - min(length, *expect_data_len);

    return diff;
}

#define check_mf_media_buffer(a, b, c) check_mf_media_buffer_(__FILE__, __LINE__, a, b, c)
static DWORD check_mf_media_buffer_(const char *file, int line, IMFMediaBuffer *buffer, const struct buffer_desc *expect,
        const BYTE **expect_data, DWORD *expect_data_len)
{
    DWORD length, diff = 0, expect_length = expect->length;
    HRESULT hr;
    BYTE *data;

    if (expect_length == -1)
    {
        expect_length = *(DWORD *)*expect_data;
        *expect_data = *expect_data + sizeof(DWORD);
        *expect_data_len = *expect_data_len - sizeof(DWORD);
    }

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, &length);
    ok_(file, line)(hr == S_OK, "Lock returned %#lx\n", hr);
    todo_wine_if(expect->todo_length)
    ok_(file, line)(length == expect_length, "got length %ld, expected %ld\n", length, expect_length);

    if (*expect_data)
    {
        if (*expect_data_len < length)
            todo_wine_if(expect->todo_length || expect->todo_data)
            ok_(file, line)(0, "missing %#lx bytes\n", length - *expect_data_len);
        else if (!expect->compare)
            diff = compare_bytes(data, &length, NULL, NULL, *expect_data);
        else
            diff = expect->compare(data, &length, &expect->size, &expect->compare_rect, *expect_data);
    }

    hr = IMFMediaBuffer_Unlock(buffer);
    ok_(file, line)(hr == S_OK, "Unlock returned %#lx\n", hr);

    *expect_data = *expect_data + min(length, *expect_data_len);
    *expect_data_len = *expect_data_len - min(length, *expect_data_len);

    return diff;
}

struct check_mf_sample_context
{
    DWORD total_length;
    const BYTE *data;
    DWORD data_len;
    DWORD diff;
    const char *file;
    int line;
};

static void check_mf_sample_buffer(IMFMediaBuffer *buffer, const struct buffer_desc *expect, void *context)
{
    struct check_mf_sample_context *ctx = context;
    DWORD expect_length = expect->length == -1 ? *(DWORD *)ctx->data : expect->length;
    ctx->diff += check_mf_media_buffer_(ctx->file, ctx->line, buffer, expect, &ctx->data, &ctx->data_len);
    ctx->total_length += expect_length;
}

#define check_mf_sample(a, b, c, d) check_mf_sample_(__FILE__, __LINE__, a, b, c, d)
static DWORD check_mf_sample_(const char *file, int line, IMFSample *sample, const struct sample_desc *expect,
        const BYTE **expect_data, DWORD *expect_data_len)
{
    struct check_mf_sample_context ctx = {.data = *expect_data, .data_len = *expect_data_len, .file = file, .line = line};
    DWORD buffer_count, total_length, sample_flags, expect_length;
    LONGLONG timestamp;
    HRESULT hr;

    if (expect->attributes)
        check_attributes_(file, line, (IMFAttributes *)sample, expect->attributes, -1);

    buffer_count = 0xdeadbeef;
    hr = IMFSample_GetBufferCount(sample, &buffer_count);
    ok_(file, line)(hr == S_OK, "GetBufferCount returned %#lx\n", hr);
    ok_(file, line)(buffer_count == expect->buffer_count,
            "got %lu buffers\n", buffer_count);

    sample_flags = 0xdeadbeef;
    hr = IMFSample_GetSampleFlags(sample, &sample_flags);
    ok_(file, line)(hr == S_OK, "GetSampleFlags returned %#lx\n", hr);
    ok_(file, line)(sample_flags == 0,
            "got sample flags %#lx\n", sample_flags);

    timestamp = 0xdeadbeef;
    hr = IMFSample_GetSampleTime(sample, &timestamp);
    ok_(file, line)(hr == S_OK, "GetSampleTime returned %#lx\n", hr);
    todo_wine_if(expect->todo_time)
    ok_(file, line)(llabs(timestamp - expect->sample_time) <= 50,
            "got sample time %I64d\n", timestamp);

    timestamp = 0xdeadbeef;
    hr = IMFSample_GetSampleDuration(sample, &timestamp);
    ok_(file, line)(hr == S_OK, "GetSampleDuration returned %#lx\n", hr);
    todo_wine_if(expect->todo_duration)
    ok_(file, line)(llabs(timestamp - expect->sample_duration) <= 1,
            "got sample duration %I64d\n", timestamp);

    enum_mf_media_buffers(sample, expect, check_mf_sample_buffer, &ctx);
    if (expect->total_length)
        expect_length = expect->total_length;
    else
        expect_length = ctx.total_length;

    total_length = 0xdeadbeef;
    hr = IMFSample_GetTotalLength(sample, &total_length);
    ok_(file, line)(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    todo_wine_if(expect->todo_length)
    ok_(file, line)(total_length == expect_length,
            "got total length %#lx\n", total_length);
    todo_wine_if(expect->todo_data)
    ok_(file, line)(!*expect_data || *expect_data_len >= expect_length,
            "missing %#lx data\n", expect_length - *expect_data_len);

    *expect_data = ctx.data;
    *expect_data_len = ctx.data_len;

    return ctx.diff / buffer_count;
}

static void check_mf_sample_collection_enum(IMFSample *sample, const struct sample_desc *expect, void *context)
{
    struct check_mf_sample_context *ctx = context;
    ctx->diff += check_mf_sample_(ctx->file, ctx->line, sample, expect, &ctx->data, &ctx->data_len);
}

static void check_mf_sample_2d_buffer(IMFMediaBuffer *buffer, const struct buffer_desc *expect, void *context)
{
    struct check_mf_sample_context *ctx = context;
    IMF2DBuffer2 *buffer2d;
    HRESULT hr;

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&buffer2d);
    ok(hr == S_OK, "QueryInterface IMF2DBuffer2 returned %#lx\n", hr);
    ctx->diff += check_mf_2d_buffer_(ctx->file, ctx->line, buffer2d, expect, &ctx->data, &ctx->data_len);
    IMF2DBuffer2_Release(buffer2d);
}

#define check_mf_sample_2d(a, b, c, d) check_mf_sample_2d_(__FILE__, __LINE__, a, b, c, d)
static DWORD check_mf_sample_2d_(const char *file, int line, IMFSample *sample, const struct sample_desc *expect,
        const BYTE **expect_data, DWORD *expect_data_len)
{
    struct check_mf_sample_context ctx = {.data = *expect_data, .data_len = *expect_data_len, .file = file, .line = line};
    DWORD buffer_count;
    HRESULT hr;

    buffer_count = 0xdeadbeef;
    hr = IMFSample_GetBufferCount(sample, &buffer_count);
    ok_(file, line)(hr == S_OK, "GetBufferCount returned %#lx\n", hr);
    ok_(file, line)(buffer_count == expect->buffer_count,
            "got %lu buffers\n", buffer_count);

    enum_mf_media_buffers(sample, expect, check_mf_sample_2d_buffer, &ctx);

    *expect_data = ctx.data;
    *expect_data_len = ctx.data_len;

    return ctx.diff / buffer_count;
}

static void check_mf_sample_collection_2d_enum(IMFSample *sample, const struct sample_desc *expect, void *context)
{
    struct check_mf_sample_context *ctx = context;
    ctx->diff += check_mf_sample_2d_(ctx->file, ctx->line, sample, expect, &ctx->data, &ctx->data_len);
}

#define check_2d_mf_sample_collection(a, b, c) check_mf_sample_collection_(__FILE__, __LINE__, a, b, c, TRUE)
DWORD check_mf_sample_collection_(const char *file, int line, IMFCollection *samples,
        const struct sample_desc *expect_samples, const WCHAR *expect_data_filename, BOOL use_2d_buffer)
{
    struct check_mf_sample_context ctx = {.file = file, .line = line};
    DWORD count;
    HRESULT hr;

    if (expect_data_filename) load_resource(expect_data_filename, &ctx.data, &ctx.data_len);
    enum_mf_samples(samples, expect_samples, use_2d_buffer ? check_mf_sample_collection_2d_enum : check_mf_sample_collection_enum, &ctx);

    if (expect_data_filename) dump_mf_sample_collection(samples, expect_samples, expect_data_filename, use_2d_buffer);

    hr = IMFCollection_GetElementCount(samples, &count);
    ok_(file, line)(hr == S_OK, "GetElementCount returned %#lx\n", hr);

    return ctx.diff / count;
}

#define check_video_info_header(a, b) check_video_info_header_(__LINE__, a, b)
static void check_video_info_header_(int line, VIDEOINFOHEADER *info, const VIDEOINFOHEADER *expected)
{
    ok_(__FILE__, line)(info->rcSource.left == expected->rcSource.left
            && info->rcSource.top == expected->rcSource.top
            && info->rcSource.right == expected->rcSource.right
            && info->rcSource.bottom == expected->rcSource.bottom,
            "Got unexpected rcSource {%ld, %ld, %ld, %ld}, expected {%ld, %ld, %ld, %ld}.\n",
            info->rcSource.left, info->rcSource.top, info->rcSource.right, info->rcSource.bottom,
            expected->rcSource.left, expected->rcSource.top, expected->rcSource.right, expected->rcSource.bottom);
    ok_(__FILE__, line)(info->rcTarget.left == expected->rcTarget.left
            && info->rcTarget.top == expected->rcTarget.top
            && info->rcTarget.right == expected->rcTarget.right
            && info->rcTarget.bottom == expected->rcTarget.bottom,
            "Got unexpected rcTarget {%ld, %ld, %ld, %ld}, expected {%ld, %ld, %ld, %ld}.\n",
            info->rcTarget.left, info->rcTarget.top, info->rcTarget.right, info->rcTarget.bottom,
            expected->rcTarget.left, expected->rcTarget.top, expected->rcTarget.right, expected->rcTarget.bottom);
    check_member_(__FILE__, line, *info, *expected, "%lu",   dwBitRate);
    check_member_(__FILE__, line, *info, *expected, "%lu",   dwBitErrorRate);
    check_member_(__FILE__, line, *info, *expected, "%I64d", AvgTimePerFrame);
    check_member_(__FILE__, line, *info, *expected, "%lu",   bmiHeader.biSize);
    check_member_(__FILE__, line, *info, *expected, "%ld",   bmiHeader.biWidth);
    check_member_(__FILE__, line, *info, *expected, "%ld",   bmiHeader.biHeight);
    check_member_(__FILE__, line, *info, *expected, "%u",    bmiHeader.biPlanes);
    check_member_(__FILE__, line, *info, *expected, "%u",    bmiHeader.biBitCount);
    check_member_(__FILE__, line, *info, *expected, "%lx",   bmiHeader.biCompression);
    check_member_(__FILE__, line, *info, *expected, "%lu",   bmiHeader.biSizeImage);
    check_member_(__FILE__, line, *info, *expected, "%ld",   bmiHeader.biXPelsPerMeter);
    check_member_(__FILE__, line, *info, *expected, "%ld",   bmiHeader.biYPelsPerMeter);
    todo_wine_if(expected->bmiHeader.biClrUsed != 0)
    check_member_(__FILE__, line, *info, *expected, "%lu",   bmiHeader.biClrUsed);
    todo_wine_if(expected->bmiHeader.biClrImportant != 0)
    check_member_(__FILE__, line, *info, *expected, "%lu",   bmiHeader.biClrImportant);
}

#define check_wave_format(a, b) check_wave_format_(__LINE__, a, b)
static void check_wave_format_(int line, WAVEFORMATEX *info, const WAVEFORMATEX *expected)
{
    check_member_(__FILE__, line, *info, *expected, "%#x", wFormatTag);
    check_member_(__FILE__, line, *info, *expected, "%u",  nChannels);
    check_member_(__FILE__, line, *info, *expected, "%lu", nSamplesPerSec);
    check_member_(__FILE__, line, *info, *expected, "%lu", nAvgBytesPerSec);
    check_member_(__FILE__, line, *info, *expected, "%u",  nBlockAlign);
    check_member_(__FILE__, line, *info, *expected, "%u",  wBitsPerSample);
    check_member_(__FILE__, line, *info, *expected, "%u",  cbSize);
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
    if (IsEqualGUID(&expected->majortype, &MEDIATYPE_Video))
    {
        check_member_(__FILE__, line, *media_type, *expected, "%d",  bFixedSizeSamples);
        check_member_(__FILE__, line, *media_type, *expected, "%d",  bTemporalCompression);
        check_member_(__FILE__, line, *media_type, *expected, "%lu", lSampleSize);
    }
    ok_(__FILE__, line)(IsEqualGUID(&media_type->formattype, &expected->formattype),
            "Got unexpected formattype %s.\n",
            debugstr_guid(&media_type->formattype));
    ok_(__FILE__, line)(media_type->pUnk == NULL, "Got unexpected pUnk %p.\n", media_type->pUnk);
    todo_wine_if(expected->cbFormat
        && expected->cbFormat != sizeof(VIDEOINFOHEADER)
        && IsEqualGUID(&expected->majortype, &MEDIATYPE_Video))
    check_member_(__FILE__, line, *media_type, *expected, "%lu", cbFormat);

    if (expected->pbFormat)
    {
        ok_(__FILE__, line)(!!media_type->pbFormat, "Got NULL pbFormat.\n");
        if (!media_type->pbFormat)
            return;

        if (IsEqualGUID(&media_type->formattype, &FORMAT_VideoInfo)
                && IsEqualGUID(&expected->formattype, &FORMAT_VideoInfo))
            check_video_info_header((VIDEOINFOHEADER *)media_type->pbFormat, (VIDEOINFOHEADER *)expected->pbFormat);
        else if (IsEqualGUID(&media_type->formattype, &FORMAT_WaveFormatEx)
                && IsEqualGUID(&expected->formattype, &FORMAT_WaveFormatEx))
            check_wave_format((WAVEFORMATEX *)media_type->pbFormat, (WAVEFORMATEX *)expected->pbFormat);
    }
}

#define check_dmo_output_data_buffer(a, b, c, d, e) check_dmo_output_data_buffer_(__LINE__, a, b, c, d, e)
static DWORD check_dmo_output_data_buffer_(int line, DMO_OUTPUT_DATA_BUFFER *output_data_buffer,
        const struct buffer_desc *buffer_desc, const WCHAR *expect_data_filename,
        REFERENCE_TIME time_stamp, REFERENCE_TIME time_length)
{
    DWORD diff, data_length, buffer_length, expect_length;
    BYTE *data, *buffer;
    HRESULT hr;

    if (output_data_buffer->dwStatus & DMO_OUTPUT_DATA_BUFFERF_TIME)
        ok_(__FILE__, line)(output_data_buffer->rtTimestamp == time_stamp,
                "Unexpected time stamp %I64d, expected %I64d.\n",
                output_data_buffer->rtTimestamp, time_stamp);
    if (output_data_buffer->dwStatus & DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH)
        todo_wine
        ok_(__FILE__, line)(output_data_buffer->rtTimelength == time_length,
                "Unexpected time length %I64d, expected %I64d.\n",
                output_data_buffer->rtTimelength, time_length);

    load_resource(expect_data_filename, (const BYTE **)&data, &data_length);

    expect_length = buffer_desc->length;
    if (expect_length == -1)
    {
        expect_length = *(DWORD *)data;
        data += sizeof(DWORD);
        data_length = data_length - sizeof(DWORD);
    }

    hr = IMediaBuffer_GetBufferAndLength(output_data_buffer->pBuffer, &buffer, &buffer_length);
    ok(hr == S_OK, "GetBufferAndLength returned %#lx.\n", hr);
    ok_(__FILE__, line)(buffer_length == expect_length, "Unexpected length %#lx, expected %#lx\n", buffer_length, expect_length);

    diff = 0;
    if (data_length < buffer_length)
        ok_(__FILE__, line)(0, "Missing %#lx bytes\n", buffer_length - data_length);
    else if (!buffer_desc->compare)
        diff = compare_bytes(buffer, &buffer_length, NULL, NULL, data);
    else
        diff = buffer_desc->compare(buffer, &buffer_length, &buffer_desc->size, &buffer_desc->compare_rect, data);

    return diff;
}

#define check_dmo_get_output_size_info_video(a, b, c, d, e) check_dmo_get_output_size_info_video_(__LINE__, a, b, c, d, e)
static void check_dmo_get_output_size_info_video_(int line, IMediaObject *dmo,
        const GUID *input_subtype, const GUID *output_subtype, const LONG width, const LONG height)
{
    DWORD size, alignment, expected_size;
    DMO_MEDIA_TYPE *type;
    char buffer[2048];
    HRESULT hr;

    type = (void *)buffer;

    hr = IMediaObject_SetInputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR);
    ok_(__FILE__, line)(hr == S_OK, "Failed to clear input type, hr %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR);
    ok_(__FILE__, line)(hr == S_OK, "Failed to clear output type, hr %#lx.\n", hr);

    init_dmo_media_type_video(type, input_subtype, width, height, 0);
    hr = IMediaObject_SetInputType(dmo, 0, type, 0);
    ok_(__FILE__, line)(hr == S_OK, "SetInputType returned %#lx.\n", hr);

    init_dmo_media_type_video(type, output_subtype, width, height, 0);
    hr = IMediaObject_SetOutputType(dmo, 0, type, 0);
    todo_wine_if(IsEqualGUID(output_subtype, &MEDIASUBTYPE_NV11))
    ok_(__FILE__, line)(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    if (hr != S_OK)
        return;

    size = 0xdeadbeef;
    alignment = 0xdeadbeef;
    hr = MFCalculateImageSize(output_subtype, width, height, (UINT32 *)&expected_size);
    ok_(__FILE__, line)(hr == S_OK, "MFCalculateImageSize returned %#lx.\n", hr);

    hr = IMediaObject_GetOutputSizeInfo(dmo, 0, &size, &alignment);
    ok_(__FILE__, line)(hr == S_OK, "GetOutputSizeInfo returned %#lx.\n", hr);
    ok_(__FILE__, line)(size == expected_size, "Unexpected size %lu, expected %lu.\n", size, expected_size);
    ok_(__FILE__, line)(alignment == 1, "Unexpected alignment %lu.\n", alignment);
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

static BOOL is_supported_video_type(const GUID *guid)
{
    return IsEqualGUID(guid, &MFVideoFormat_L8)
            || IsEqualGUID(guid, &MFVideoFormat_L16)
            || IsEqualGUID(guid, &MFVideoFormat_D16)
            || IsEqualGUID(guid, &MFVideoFormat_IYUV)
            || IsEqualGUID(guid, &MFVideoFormat_YV12)
            || IsEqualGUID(guid, &MFVideoFormat_NV12)
            || IsEqualGUID(guid, &MFVideoFormat_NV21)
            || IsEqualGUID(guid, &MFVideoFormat_420O)
            || IsEqualGUID(guid, &MFVideoFormat_P010)
            || IsEqualGUID(guid, &MFVideoFormat_P016)
            || IsEqualGUID(guid, &MFVideoFormat_UYVY)
            || IsEqualGUID(guid, &MFVideoFormat_YUY2)
            || IsEqualGUID(guid, &MFVideoFormat_P208)
            || IsEqualGUID(guid, &MFVideoFormat_NV11)
            || IsEqualGUID(guid, &MFVideoFormat_AYUV)
            || IsEqualGUID(guid, &MFVideoFormat_ARGB32)
            || IsEqualGUID(guid, &MFVideoFormat_RGB32)
            || IsEqualGUID(guid, &MFVideoFormat_A2R10G10B10)
            || IsEqualGUID(guid, &MFVideoFormat_A16B16G16R16F)
            || IsEqualGUID(guid, &MFVideoFormat_RGB24)
            || IsEqualGUID(guid, &MFVideoFormat_I420)
            || IsEqualGUID(guid, &MFVideoFormat_YVYU)
            || IsEqualGUID(guid, &MFVideoFormat_RGB555)
            || IsEqualGUID(guid, &MFVideoFormat_RGB565)
            || IsEqualGUID(guid, &MFVideoFormat_RGB8)
            || IsEqualGUID(guid, &MFVideoFormat_Y216)
            || IsEqualGUID(guid, &MFVideoFormat_v410)
            || IsEqualGUID(guid, &MFVideoFormat_Y41P)
            || IsEqualGUID(guid, &MFVideoFormat_Y41T)
            || IsEqualGUID(guid, &MFVideoFormat_Y42T)
            || IsEqualGUID(guid, &MFVideoFormat_ABGR32);
}

static BOOL is_sample_copier_available_type(IMFMediaType *type)
{
    GUID major = { 0 };
    UINT32 count;
    HRESULT hr;

    hr = IMFMediaType_GetMajorType(type, &major);
    ok(hr == S_OK, "Failed to get major type, hr %#lx.\n", hr);

    hr = IMFMediaType_GetCount(type, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    return IsEqualGUID(&major, &MFMediaType_Video) || IsEqualGUID(&major, &MFMediaType_Audio);
}

static void test_sample_copier(void)
{
    static const struct attribute_desc expect_transform_attributes[] =
    {
        ATTR_UINT32(MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, 1),
        {0},
    };
    const MFT_OUTPUT_STREAM_INFO initial_output_info = {0}, output_info = {.cbSize = 16 * 16};
    const MFT_INPUT_STREAM_INFO initial_input_info = {0}, input_info = {.cbSize = 16 * 16};
    IMFMediaType *mediatype, *mediatype2;
    IMFSample *sample, *client_sample;
    IMFMediaBuffer *media_buffer;
    MFT_INPUT_STREAM_INFO info;
    DWORD flags, output_status;
    IMFTransform *copier;
    HRESULT hr;
    LONG ref;

    if (!pMFCreateSampleCopierMFT)
    {
        win_skip("MFCreateSampleCopierMFT() is not available.\n");
        return;
    }

    winetest_push_context("copier");

    hr = pMFCreateSampleCopierMFT(&copier);
    ok(hr == S_OK, "Failed to create sample copier, hr %#lx.\n", hr);

    check_interface(copier, &IID_IMFTransform, TRUE);
    check_interface(copier, &IID_IMediaObject, FALSE);
    check_interface(copier, &IID_IPropertyStore, FALSE);
    check_interface(copier, &IID_IPropertyBag, FALSE);

    check_mft_optional_methods(copier, 1);
    check_mft_get_attributes(copier, expect_transform_attributes, FALSE);

    /* Available types. */
    hr = IMFTransform_GetInputAvailableType(copier, 0, 0, &mediatype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(is_sample_copier_available_type(mediatype), "Unexpected type.\n");
    IMFMediaType_Release(mediatype);

    hr = IMFTransform_GetInputAvailableType(copier, 0, 1, &mediatype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(is_sample_copier_available_type(mediatype), "Unexpected type.\n");
    IMFMediaType_Release(mediatype);

    hr = IMFTransform_GetInputAvailableType(copier, 0, 2, &mediatype);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputAvailableType(copier, 1, 0, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputAvailableType(copier, 0, 0, &mediatype);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputAvailableType(copier, 1, 0, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    check_mft_get_input_current_type(copier, NULL);
    check_mft_get_output_current_type(copier, NULL);

    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#lx.\n", hr);

    hr = IMFTransform_ProcessInput(copier, 0, sample, 0);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(copier, 0, mediatype, 0);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, &MFVideoFormat_RGB8);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT64(mediatype, &MF_MT_FRAME_SIZE, ((UINT64)16) << 32 | 16);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    check_mft_get_input_stream_info(copier, S_OK, &initial_input_info);
    check_mft_get_output_stream_info(copier, S_OK, &initial_output_info);

    hr = IMFTransform_SetOutputType(copier, 0, mediatype, 0);
    ok(hr == S_OK, "Failed to set input type, hr %#lx.\n", hr);

    memset(&info, 0xcd, sizeof(info));
    hr = IMFTransform_GetInputStreamInfo(copier, 0, &info);
    ok(hr == S_OK, "GetInputStreamInfo returned %#lx\n", hr);
    check_member(info, initial_input_info, "%I64d", hnsMaxLatency);
    check_member(info, initial_input_info, "%#lx", dwFlags);
    todo_wine
    check_member(info, initial_input_info, "%#lx", cbSize);
    check_member(info, initial_input_info, "%#lx", cbMaxLookahead);
    check_member(info, initial_input_info, "%#lx", cbAlignment);
    check_mft_get_output_stream_info(copier, S_OK, &output_info);

    hr = IMFTransform_GetOutputCurrentType(copier, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get current type, hr %#lx.\n", hr);
    IMFMediaType_Release(mediatype2);

    check_mft_get_input_current_type(copier, NULL);

    hr = IMFTransform_GetInputStatus(copier, 0, &flags);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    /* Setting input type resets output type. */
    hr = IMFTransform_GetOutputCurrentType(copier, 0, &mediatype2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(mediatype2);

    hr = IMFTransform_SetInputType(copier, 0, mediatype, 0);
    ok(hr == S_OK, "Failed to set input type, hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputCurrentType(copier, 0, &mediatype2);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputAvailableType(copier, 0, 1, &mediatype2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(is_sample_copier_available_type(mediatype2), "Unexpected type.\n");
    IMFMediaType_Release(mediatype2);

    check_mft_get_input_stream_info(copier, S_OK, &input_info);
    check_mft_get_output_stream_info(copier, S_OK, &output_info);

    hr = IMFTransform_GetOutputAvailableType(copier, 0, 0, &mediatype2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_IsEqual(mediatype2, mediatype, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(mediatype2);

    hr = IMFTransform_GetInputStatus(copier, 0, &flags);
    ok(hr == S_OK, "Failed to get input status, hr %#lx.\n", hr);
    ok(flags == MFT_INPUT_STATUS_ACCEPT_DATA, "Unexpected flags %#lx.\n", flags);

    hr = IMFTransform_GetInputCurrentType(copier, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get current type, hr %#lx.\n", hr);
    IMFMediaType_Release(mediatype2);

    hr = IMFTransform_GetOutputStatus(copier, &flags);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(copier, 0, mediatype, 0);
    ok(hr == S_OK, "Failed to set output type, hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputStatus(copier, &flags);
    ok(hr == S_OK, "Failed to get output status, hr %#lx.\n", hr);
    ok(!flags, "Unexpected flags %#lx.\n", flags);

    /* Pushing samples. */
    hr = MFCreateAlignedMemoryBuffer(output_info.cbSize, output_info.cbAlignment, &media_buffer);
    ok(hr == S_OK, "Failed to create media buffer, hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(sample, media_buffer);
    ok(hr == S_OK, "Failed to add a buffer, hr %#lx.\n", hr);
    IMFMediaBuffer_Release(media_buffer);

    EXPECT_REF(sample, 1);
    hr = IMFTransform_ProcessInput(copier, 0, sample, 0);
    ok(hr == S_OK, "Failed to process input, hr %#lx.\n", hr);
    EXPECT_REF(sample, 2);

    hr = IMFTransform_GetInputStatus(copier, 0, &flags);
    ok(hr == S_OK, "Failed to get input status, hr %#lx.\n", hr);
    ok(!flags, "Unexpected flags %#lx.\n", flags);

    hr = IMFTransform_GetOutputStatus(copier, &flags);
    ok(hr == S_OK, "Failed to get output status, hr %#lx.\n", hr);
    ok(flags == MFT_OUTPUT_STATUS_SAMPLE_READY, "Unexpected flags %#lx.\n", flags);

    hr = IMFTransform_ProcessInput(copier, 0, sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "Unexpected hr %#lx.\n", hr);

    check_mft_get_input_stream_info(copier, S_OK, &input_info);
    check_mft_get_output_stream_info(copier, S_OK, &output_info);

    hr = MFCreateAlignedMemoryBuffer(output_info.cbSize, output_info.cbAlignment, &media_buffer);
    ok(hr == S_OK, "Failed to create media buffer, hr %#lx.\n", hr);

    hr = MFCreateSample(&client_sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(client_sample, media_buffer);
    ok(hr == S_OK, "Failed to add a buffer, hr %#lx.\n", hr);
    IMFMediaBuffer_Release(media_buffer);

    hr = check_mft_process_output(copier, client_sample, &output_status);
    ok(hr == S_OK, "Failed to get output, hr %#lx.\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
    EXPECT_REF(sample, 1);

    hr = check_mft_process_output(copier, client_sample, &output_status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "Failed to get output, hr %#lx.\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);

    /* Flushing. */
    hr = IMFTransform_ProcessInput(copier, 0, sample, 0);
    ok(hr == S_OK, "Failed to process input, hr %#lx.\n", hr);
    EXPECT_REF(sample, 2);

    hr = IMFTransform_ProcessMessage(copier, MFT_MESSAGE_COMMAND_FLUSH, 0);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

    ref = IMFSample_Release(sample);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFSample_Release(client_sample);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFTransform_Release(copier);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(mediatype);
    ok(ref == 0, "Release returned %ld\n", ref);

    winetest_pop_context();
}

struct sample_metadata
{
    unsigned int flags;
    LONGLONG duration;
    LONGLONG time;
};

static void sample_copier_process(IMFTransform *copier, IMFMediaBuffer *input_buffer,
        IMFMediaBuffer *output_buffer, const struct sample_metadata *md)
{
    static const struct sample_metadata zero_md = { 0, ~0u, ~0u };
    IMFSample *input_sample, *output_sample;
    DWORD flags, output_status;
    LONGLONG time;
    HRESULT hr;
    LONG ref;

    hr = MFCreateSample(&input_sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#lx.\n", hr);

    if (md)
    {
        hr = IMFSample_SetSampleFlags(input_sample, md->flags);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFSample_SetSampleTime(input_sample, md->time);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFSample_SetSampleDuration(input_sample, md->duration);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }

    hr = MFCreateSample(&output_sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#lx.\n", hr);

    hr = IMFSample_SetSampleFlags(output_sample, ~0u);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_SetSampleTime(output_sample, ~0u);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_SetSampleDuration(output_sample, ~0u);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(input_sample, input_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(output_sample, output_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_ProcessInput(copier, 0, input_sample, 0);
    ok(hr == S_OK, "Failed to process input, hr %#lx.\n", hr);

    hr = check_mft_process_output(copier, output_sample, &output_status);
    ok(hr == S_OK, "Failed to get output, hr %#lx.\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);

    if (!md) md = &zero_md;

    hr = IMFSample_GetSampleFlags(output_sample, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(md->flags == flags, "Unexpected flags.\n");
    hr = IMFSample_GetSampleTime(output_sample, &time);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(md->time == time, "Unexpected time.\n");
    hr = IMFSample_GetSampleDuration(output_sample, &time);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(md->duration == time, "Unexpected duration.\n");

    ref = IMFSample_Release(input_sample);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFSample_Release(output_sample);
    ok(ref == 0, "Release returned %ld\n", ref);
}

static void test_sample_copier_output_processing(void)
{
    IMFMediaBuffer *input_buffer, *output_buffer;
    MFT_OUTPUT_STREAM_INFO output_info;
    struct sample_metadata md;
    IMFMediaType *mediatype;
    IMFTransform *copier;
    DWORD max_length;
    HRESULT hr;
    BYTE *ptr;
    LONG ref;

    if (!pMFCreateSampleCopierMFT)
        return;

    hr = pMFCreateSampleCopierMFT(&copier);
    ok(hr == S_OK, "Failed to create sample copier, hr %#lx.\n", hr);

    /* Configure for 16 x 16 of D3DFMT_X8R8G8B8. */
    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT64(mediatype, &MF_MT_FRAME_SIZE, ((UINT64)16) << 32 | 16);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(copier, 0, mediatype, 0);
    ok(hr == S_OK, "Failed to set input type, hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(copier, 0, mediatype, 0);
    ok(hr == S_OK, "Failed to set input type, hr %#lx.\n", hr);

    /* Source and destination are linear buffers, destination is twice as large. */
    hr = IMFTransform_GetOutputStreamInfo(copier, 0, &output_info);
    ok(hr == S_OK, "Failed to get output info, hr %#lx.\n", hr);

    hr = MFCreateAlignedMemoryBuffer(output_info.cbSize, output_info.cbAlignment, &output_buffer);
    ok(hr == S_OK, "Failed to create media buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(output_buffer, &ptr, &max_length, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    memset(ptr, 0xcc, max_length);
    hr = IMFMediaBuffer_Unlock(output_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateAlignedMemoryBuffer(output_info.cbSize, output_info.cbAlignment, &input_buffer);
    ok(hr == S_OK, "Failed to create media buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(input_buffer, &ptr, &max_length, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    memset(ptr, 0xaa, max_length);
    hr = IMFMediaBuffer_Unlock(input_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaBuffer_SetCurrentLength(input_buffer, 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    sample_copier_process(copier, input_buffer, output_buffer, NULL);

    hr = IMFMediaBuffer_Lock(output_buffer, &ptr, &max_length, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ptr[0] == 0xaa && ptr[4] == 0xcc, "Unexpected buffer contents.\n");

    hr = IMFMediaBuffer_Unlock(output_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    md.flags = 123;
    md.time = 10;
    md.duration = 2;
    sample_copier_process(copier, input_buffer, output_buffer, &md);

    ref = IMFMediaBuffer_Release(input_buffer);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaBuffer_Release(output_buffer);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFTransform_Release(copier);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(mediatype);
    ok(ref == 0, "Release returned %ld\n", ref);
}

#define create_sample(a, b) create_sample_(a, b, NULL)
static IMFSample *create_sample_(const BYTE *data, ULONG size, const struct attribute_desc *desc)
{
    IMFMediaBuffer *media_buffer;
    IMFMediaType *media_type;
    IMF2DBuffer2 *buffer2d2;
    BYTE *buffer, *scanline;
    IMFSample *sample;
    DWORD length;
    LONG stride;
    HRESULT hr;
    ULONG ret;

    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "MFCreateSample returned %#lx\n", hr);

    if (!desc)
    {
        hr = MFCreateMemoryBuffer(size, &media_buffer);
        ok(hr == S_OK, "MFCreateMemoryBuffer returned %#lx\n", hr);
    }
    else
    {
        hr = MFCreateMediaType(&media_type);
        ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);
        init_media_type(media_type, desc, -1);
        hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 0, 0, &media_buffer);
        ok(hr == S_OK, "MFCreateMediaBufferFromMediaType returned %#lx\n", hr);
        IMFMediaType_Release(media_type);
    }

    hr = IMFMediaBuffer_Lock(media_buffer, &buffer, NULL, &length);
    ok(hr == S_OK, "Lock returned %#lx\n", hr);
    if (!desc) ok(length == 0, "got length %lu\n", length);
    if (!data) memset(buffer, 0xcd, size);
    else memcpy(buffer, data, size);
    hr = IMFMediaBuffer_Unlock(media_buffer);
    ok(hr == S_OK, "Unlock returned %#lx\n", hr);

    if (SUCCEEDED(hr = IMFMediaBuffer_QueryInterface(media_buffer, &IID_IMF2DBuffer2, (void**)&buffer2d2)))
    {
        ok(hr == S_OK, "QueryInterface IMF2DBuffer2 returned %#lx\n", hr);
        hr = IMF2DBuffer2_Lock2DSize(buffer2d2, MF2DBuffer_LockFlags_Write, &scanline, &stride, &buffer, &length);
        ok(hr == S_OK, "Lock2D returned %#lx\n", hr);
        if (!data) memset(buffer, 0xcd, length);
        hr = IMF2DBuffer2_Unlock2D(buffer2d2);
        ok(hr == S_OK, "Unlock2D returned %#lx\n", hr);
        IMF2DBuffer2_Release(buffer2d2);
    }

    hr = IMFMediaBuffer_SetCurrentLength(media_buffer, data ? size : 0);
    ok(hr == S_OK, "SetCurrentLength returned %#lx\n", hr);
    hr = IMFSample_AddBuffer(sample, media_buffer);
    ok(hr == S_OK, "AddBuffer returned %#lx\n", hr);
    ret = IMFMediaBuffer_Release(media_buffer);
    ok(ret == 1, "Release returned %lu\n", ret);

    return sample;
}

static void test_aac_encoder(void)
{
    const GUID *const class_id = &CLSID_AACMFTEncoder;
    const struct transform_info expect_mft_info =
    {
        .name = L"Microsoft AAC Audio Encoder MFT",
        .major_type = &MFMediaType_Audio,
        .inputs =
        {
            {.subtype = &MFAudioFormat_PCM},
        },
        .outputs =
        {
            {.subtype = &MFAudioFormat_AAC},
        },
    };

    static const struct attribute_desc input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100, .required = TRUE),
        {0},
    };
    const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_AAC, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 12000, .required = TRUE),
        {0},
    };

    static const struct attribute_desc expect_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_AM_FORMAT_TYPE, FORMAT_WaveFormatEx),
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100 * 2),
        ATTR_UINT32(MF_MT_AVG_BITRATE, 44100 * 2 * 8),
        ATTR_UINT32(MF_MT_COMPRESSED, 0),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        {0},
    };
    const struct attribute_desc expect_output_type_desc[] =
    {
        ATTR_GUID(MF_MT_AM_FORMAT_TYPE, FORMAT_WaveFormatEx),
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_AAC),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 12000),
        ATTR_UINT32(MF_MT_AVG_BITRATE, 96000),
        ATTR_UINT32(MF_MT_COMPRESSED, 1),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 0),
        ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        ATTR_UINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, 41),
        ATTR_UINT32(MF_MT_AAC_PAYLOAD_TYPE, 0),
        ATTR_BLOB(MF_MT_USER_DATA, test_aac_codec_data, sizeof(test_aac_codec_data)),
        {0},
    };
    const MFT_OUTPUT_STREAM_INFO initial_output_info = {0}, output_info = {.cbSize = 0x600};
    const MFT_INPUT_STREAM_INFO input_info = {0};

    const struct buffer_desc output_buffer_desc[] =
    {
        {.length = -1 /* variable */},
    };
    const struct attribute_desc output_sample_attributes[] =
    {
        ATTR_UINT32(MFSampleExtension_CleanPoint, 1),
        {0},
    };
    const struct sample_desc output_sample_desc[] =
    {
        {
            .repeat_count = 88,
            .attributes = output_sample_attributes,
            .sample_time = 0, .sample_duration = 113823,
            .buffer_count = 1, .buffers = output_buffer_desc,
        },
    };

    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Audio, MFAudioFormat_AAC};
    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Audio, MFAudioFormat_PCM};
    IMFSample *input_sample, *output_sample;
    IMFCollection *output_samples;
    ULONG i, ret, audio_data_len;
    DWORD length, output_status;
    IMFTransform *transform;
    const BYTE *audio_data;
    HRESULT hr;
    LONG ref;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    winetest_push_context("aacenc");

    if (!check_mft_enum(MFT_CATEGORY_AUDIO_ENCODER, &input_type, &output_type, class_id))
        goto failed;
    check_mft_get_info(class_id, &expect_mft_info);

    if (FAILED(hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_IMediaObject, FALSE);
    check_interface(transform, &IID_IPropertyStore, FALSE);
    check_interface(transform, &IID_IPropertyBag, FALSE);

    check_mft_optional_methods(transform, 1);
    check_mft_get_attributes(transform, NULL, FALSE);
    check_mft_get_input_stream_info(transform, S_OK, &input_info);
    check_mft_get_output_stream_info(transform, S_OK, &initial_output_info);

    check_mft_set_output_type_required(transform, output_type_desc);
    check_mft_set_output_type(transform, output_type_desc, S_OK);
    check_mft_get_output_current_type(transform, expect_output_type_desc);

    check_mft_set_input_type_required(transform, input_type_desc);
    check_mft_set_input_type(transform, input_type_desc, S_OK);
    check_mft_get_input_current_type(transform, expect_input_type_desc);

    check_mft_get_input_stream_info(transform, S_OK, &input_info);
    check_mft_get_output_stream_info(transform, S_OK, &output_info);

    if (!has_video_processor)
    {
        win_skip("Skipping AAC encoder tests on Win7\n");
        goto done;
    }

    load_resource(L"audiodata.bin", &audio_data, &audio_data_len);
    ok(audio_data_len == 179928, "got length %lu\n", audio_data_len);

    input_sample = create_sample(audio_data, audio_data_len);
    hr = IMFSample_SetSampleTime(input_sample, 0);
    ok(hr == S_OK, "SetSampleTime returned %#lx\n", hr);
    hr = IMFSample_SetSampleDuration(input_sample, 10000000);
    ok(hr == S_OK, "SetSampleDuration returned %#lx\n", hr);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, 0);
    ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);
    ref = IMFSample_Release(input_sample);
    ok(ref <= 1, "Release returned %ld\n", ref);

    hr = MFCreateCollection(&output_samples);
    ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

    output_sample = create_sample(NULL, output_info.cbSize);
    for (i = 0; SUCCEEDED(hr = check_mft_process_output(transform, output_sample, &output_status)); i++)
    {
        winetest_push_context("%lu", i);
        ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
        hr = IMFCollection_AddElement(output_samples, (IUnknown *)output_sample);
        ok(hr == S_OK, "AddElement returned %#lx\n", hr);
        ref = IMFSample_Release(output_sample);
        ok(ref == 1, "Release returned %ld\n", ref);
        output_sample = create_sample(NULL, output_info.cbSize);
        winetest_pop_context();
    }
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    ok(i == 88, "got %lu output samples\n", i);

    ret = check_mf_sample_collection(output_samples, output_sample_desc, L"aacencdata.bin");
    ok(ret == 0, "got %lu%% diff\n", ret);
    IMFCollection_Release(output_samples);

    output_sample = create_sample(NULL, output_info.cbSize);
    hr = check_mft_process_output(transform, output_sample, &output_status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    hr = IMFSample_GetTotalLength(output_sample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);

done:
    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

failed:
    winetest_pop_context();
    CoUninitialize();
}

static void test_aac_decoder_subtype(const struct attribute_desc *input_type_desc)
{
    const GUID *const class_id = &CLSID_MSAACDecMFT;
    const struct transform_info expect_mft_info =
    {
        .name = L"Microsoft AAC Audio Decoder MFT",
        .major_type = &MFMediaType_Audio,
        .inputs =
        {
            {.subtype = &MFAudioFormat_AAC},
            {.subtype = &MFAudioFormat_RAW_AAC1},
            {.subtype = &MFAudioFormat_ADTS, .broken = TRUE /* <= w8 */},
        },
        .outputs =
        {
            {.subtype = &MFAudioFormat_Float},
            {.subtype = &MFAudioFormat_PCM},
        },
    };

    static const struct attribute_desc expect_input_attributes[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 6),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 24),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 1152000),
        {0},
    };
    static const media_type_desc expect_available_inputs[] =
    {
        {
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_AAC),
            ATTR_UINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, 0),
            ATTR_UINT32(MF_MT_AAC_PAYLOAD_TYPE, 0),
            /* MF_MT_USER_DATA with some AAC codec data */
        },
        {
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_RAW_AAC1),
        },
        {
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_AAC),
            ATTR_UINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, 0),
            ATTR_UINT32(MF_MT_AAC_PAYLOAD_TYPE, 1),
            /* MF_MT_USER_DATA with some AAC codec data */
        },
        {
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_AAC),
            ATTR_UINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, 0),
            ATTR_UINT32(MF_MT_AAC_PAYLOAD_TYPE, 3),
            /* MF_MT_USER_DATA with some AAC codec data */
        },
        {
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_ADTS),
        },
    };
    static const struct attribute_desc expect_output_attributes[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        {0},
    };
    static const media_type_desc expect_available_outputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
            ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
            ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 4 * 44100),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
            ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 2),
            ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 2 * 44100),
        },
    };
    const struct attribute_desc expect_transform_attributes[] =
    {
        ATTR_UINT32(MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, !has_video_processor /* 1 on W7 */, .todo = TRUE),
        /* more AAC decoder specific attributes from CODECAPI */
        {0},
    };
    static const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100, .required = TRUE),
        {0},
    };
    const MFT_OUTPUT_STREAM_INFO output_info =
    {
        .dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES,
        .cbSize = 0xc000,
    };
    const MFT_INPUT_STREAM_INFO input_info =
    {
        .dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER |
                MFT_INPUT_STREAM_FIXED_SAMPLE_SIZE | MFT_INPUT_STREAM_HOLDS_BUFFERS,
    };

    const struct buffer_desc output_buffer_desc[] =
    {
        {.length = 0x800},
    };
    const struct attribute_desc output_sample_attributes[] =
    {
        ATTR_UINT32(MFSampleExtension_CleanPoint, 1),
        {0},
    };
    const struct sample_desc output_sample_desc[] =
    {
        {
            .attributes = output_sample_attributes + (has_video_processor ? 0 : 1) /* MFSampleExtension_CleanPoint missing on Win7 */,
            .sample_time = 0, .sample_duration = 232200,
            .buffer_count = 1, .buffers = output_buffer_desc,
        },
    };

    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Audio, MFAudioFormat_Float};
    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Audio, MFAudioFormat_AAC};
    IMFSample *input_sample, *output_sample;
    ULONG i, ret, ref, aacenc_data_len;
    IMFCollection *output_samples;
    DWORD length, output_status;
    IMFMediaType *media_type;
    IMFTransform *transform;
    const BYTE *aacenc_data;
    DWORD flags;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    winetest_push_context("aacdec");

    if (!check_mft_enum(MFT_CATEGORY_AUDIO_DECODER, &input_type, &output_type, class_id))
        goto failed;
    check_mft_get_info(class_id, &expect_mft_info);

    if (FAILED(hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_IMediaObject, FALSE);
    check_interface(transform, &IID_IPropertyStore, FALSE);
    check_interface(transform, &IID_IPropertyBag, FALSE);

    check_mft_optional_methods(transform, 1);
    check_mft_get_attributes(transform, expect_transform_attributes, FALSE);
    check_mft_get_input_stream_info(transform, S_OK, &input_info);
    check_mft_get_output_stream_info(transform, S_OK, &output_info);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetOutputAvailableType returned %#lx\n", hr);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetInputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("in %lu", i);
        ok(hr == S_OK, "GetInputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_input_attributes, -1);
        check_media_type(media_type, expect_available_inputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret <= 1, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetInputAvailableType returned %#lx\n", hr);
    ok(i == ARRAY_SIZE(expect_available_inputs)
            || broken(i == 2) /* w7 */ || broken(i == 4) /* w8 */,
            "%lu input media types\n", i);

    /* setting output media type first doesn't work */
    check_mft_set_output_type(transform, output_type_desc, MF_E_TRANSFORM_TYPE_NOT_SET);
    check_mft_get_output_current_type(transform, NULL);

    check_mft_set_input_type_required(transform, input_type_desc);
    check_mft_set_input_type(transform, input_type_desc, S_OK);
    check_mft_get_input_current_type(transform, input_type_desc);

    /* check new output media types */

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_output_attributes, -1);
        check_media_type(media_type, expect_available_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret <= 1, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == ARRAY_SIZE(expect_available_outputs), "%lu input media types\n", i);

    check_mft_set_output_type_required(transform, output_type_desc);
    check_mft_set_output_type(transform, output_type_desc, S_OK);
    check_mft_get_output_current_type(transform, output_type_desc);

    check_mft_get_input_stream_info(transform, S_OK, &input_info);
    check_mft_get_output_stream_info(transform, S_OK, &output_info);

    load_resource(L"aacencdata.bin", &aacenc_data, &aacenc_data_len);
    ok(aacenc_data_len == 24861, "got length %lu\n", aacenc_data_len);

    input_sample = create_sample(aacenc_data + sizeof(DWORD), *(DWORD *)aacenc_data);

    flags = 0;
    hr = IMFTransform_GetInputStatus(transform, 0, &flags);
    ok(hr == S_OK, "Got %#lx\n", hr);
    ok(flags == MFT_INPUT_STATUS_ACCEPT_DATA, "Got flags %#lx.\n", flags);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
    flags = 0xdeadbeef;
    hr = IMFTransform_GetInputStatus(transform, 0, &flags);
    ok(hr == S_OK, "Got %#lx\n", hr);
    ok(!flags, "Got flags %#lx.\n", flags);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);
    flags = 0xdeadbeef;
    hr = IMFTransform_GetInputStatus(transform, 0, &flags);
    ok(hr == S_OK, "Got %#lx\n", hr);
    ok(!flags, "Got flags %#lx.\n", flags);

    /* As output_info.dwFlags doesn't have MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES
     * IMFTransform_ProcessOutput needs a sample or returns MF_E_TRANSFORM_NEED_MORE_INPUT */

    hr = check_mft_process_output(transform, NULL, &output_status);
    ok(hr == E_INVALIDARG, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
    flags = 0xdeadbeef;
    hr = IMFTransform_GetInputStatus(transform, 0, &flags);
    ok(hr == S_OK, "Got %#lx\n", hr);
    ok(!flags, "Got flags %#lx.\n", flags);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, 0);
    ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);
    flags = 0xdeadbeef;
    hr = IMFTransform_GetInputStatus(transform, 0, &flags);
    ok(hr == S_OK, "Got %#lx\n", hr);
    ok(!flags, "Got flags %#lx.\n", flags);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);

    hr = MFCreateCollection(&output_samples);
    ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

    output_sample = create_sample(NULL, output_info.cbSize);
    for (i = 0; SUCCEEDED(hr = check_mft_process_output(transform, output_sample, &output_status)); i++)
    {
        winetest_push_context("%lu", i);
        ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
        hr = IMFCollection_AddElement(output_samples, (IUnknown *)output_sample);
        ok(hr == S_OK, "AddElement returned %#lx\n", hr);
        ref = IMFSample_Release(output_sample);
        ok(ref == 1, "Release returned %ld\n", ref);
        output_sample = create_sample(NULL, output_info.cbSize);
        winetest_pop_context();
    }
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);

    flags = 0;
    hr = IMFTransform_GetInputStatus(transform, 0, &flags);
    ok(hr == S_OK, "Got %#lx\n", hr);
    ok(flags == MFT_INPUT_STATUS_ACCEPT_DATA, "Got flags %#lx.\n", flags);

    ok(output_status == MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE, "got output[0].dwStatus %#lx\n", output_status);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    ok(i == 1, "got %lu output samples\n", i);

    ret = check_mf_sample_collection(output_samples, output_sample_desc, L"aacdecdata.bin");
    todo_wine_if(ret <= 5)
    ok(ret == 0, "got %lu%% diff\n", ret);
    IMFCollection_Release(output_samples);

    output_sample = create_sample(NULL, output_info.cbSize);
    hr = check_mft_process_output(transform, output_sample, &output_status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE, "got output[0].dwStatus %#lx\n", output_status);
    hr = IMFSample_GetTotalLength(output_sample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    ret = IMFSample_Release(input_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

failed:
    winetest_pop_context();
    CoUninitialize();
}

static void test_aac_decoder_channels(const struct attribute_desc *input_type_desc)
{
    static const struct attribute_desc expect_output_attributes[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        {0},
    };
    static const media_type_desc expect_available_outputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
        },
    };
    static const UINT32 expected_mask[7] =
    {
        0,
        0,
        0,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_CENTER,
        KSAUDIO_SPEAKER_QUAD,
        KSAUDIO_SPEAKER_QUAD | SPEAKER_FRONT_CENTER,
        KSAUDIO_SPEAKER_5POINT1,
    };

    UINT32 value, num_channels, expected_chans, format_index, sample_size;
    unsigned int num_channels_index = ~0u;
    struct attribute_desc input_desc[64];
    IMFTransform *transform;
    IMFAttributes *attrs;
    IMFMediaType *type;
    BOOL many_channels;
    ULONG i, ret;
    HRESULT hr;

    for (i = 0; i < ARRAY_SIZE(input_desc); i++)
    {
        input_desc[i] = input_type_desc[i];
        if (!input_desc[i].key)
            break;
        if (IsEqualGUID(input_desc[i].key, &MF_MT_AUDIO_NUM_CHANNELS))
            num_channels_index = i;
    }

    ok(num_channels_index != ~0u, "Could not find MF_MT_AUDIO_NUM_CHANNELS.\n");
    ok(i < ARRAY_SIZE(input_desc), "Too many attributes.\n");

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    winetest_push_context("aacdec channels");

    if (FAILED(hr = CoCreateInstance(&CLSID_MSAACDecMFT, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
    {
        win_skip("AAC decoder transform is not available.\n");
        goto failed;
    }

    hr = MFCreateMediaType(&type);
    ok(hr == S_OK, "got %#lx.\n", hr);
    input_desc[num_channels_index].value.vt = VT_UI8;
    input_desc[num_channels_index].value.ulVal = 1;
    init_media_type(type, input_desc, -1);
    hr = IMFTransform_SetInputType(transform, 0, type, 0);
    ok(hr == S_OK, "got %#lx.\n", hr);
    IMFMediaType_Release(type);
    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &type);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = IMFAttributes_GetUINT32((IMFAttributes *)type, &MF_MT_AUDIO_NUM_CHANNELS, &value);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(value == 2, "got %u.\n", value);
    IMFMediaType_Release(type);
    input_desc[num_channels_index].value.vt = VT_UI4;

    for (num_channels = 0; num_channels < 16; ++num_channels)
    {
        many_channels = num_channels > 2;
        winetest_push_context("chans %u", num_channels);
        input_desc[num_channels_index].value.ulVal = num_channels;

        hr = MFCreateMediaType(&type);
        ok(hr == S_OK, "got %#lx.\n", hr);
        init_media_type(type, input_desc, -1);
        hr = IMFTransform_SetInputType(transform, 0, type, 0);
        IMFMediaType_Release(type);
        if (num_channels <= 6)
            ok(hr == S_OK, "got %#lx.\n", hr);
        else
        {
            ok(hr == MF_E_INVALIDMEDIATYPE, "got %#lx.\n", hr);
            winetest_pop_context();
            continue;
        }

        i = -1;
        while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &type)))
        {
            winetest_push_context("out %lu", i);
            ok(hr == S_OK, "got %#lx.\n", hr);
            check_media_type(type, expect_output_attributes, -1);
            format_index = i % 2;
            sample_size = format_index ? 2 : 4;
            check_media_type(type, expect_available_outputs[format_index], -1);
            attrs = (IMFAttributes *)type;

            hr = IMFAttributes_GetUINT32(attrs, &MF_MT_AUDIO_NUM_CHANNELS, &value);
            ok(hr == S_OK, "got %#lx.\n", hr);
            if (!num_channels || i >= ARRAY_SIZE(expect_available_outputs))
                expected_chans = 2;
            else
                expected_chans = num_channels;
            ok(value == expected_chans, "got %u, expected %u.\n", value, expected_chans);

            hr = IMFAttributes_GetUINT32(attrs, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &value);
            ok(hr == S_OK, "got %#lx.\n", hr);
            ok(value == sample_size * 44100 * expected_chans, "got %u, expected %u.\n",
                    value, sample_size * 44100 * expected_chans);

            hr = IMFAttributes_GetUINT32(attrs, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &value);
            ok(hr == S_OK, "got %#lx.\n", hr);
            ok(value == sample_size * expected_chans, "got %u, expected %u.\n", value, sample_size * expected_chans);

            hr = IMFAttributes_GetUINT32(attrs, &MF_MT_AUDIO_PREFER_WAVEFORMATEX, &value);
            if (many_channels && i < ARRAY_SIZE(expect_available_outputs))
            {
                ok(hr == MF_E_ATTRIBUTENOTFOUND, "got %#lx.\n", hr);
            }
            else
            {
                ok(hr == S_OK, "got %#lx.\n", hr);
                ok(value == 1, "got %u.\n", value);
            }

            value = 0xdeadbeef;
            hr = IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_CHANNEL_MASK, &value);
            if (expected_chans <= 2)
            {
                ok(hr == MF_E_ATTRIBUTENOTFOUND, "got %#lx.\n", hr);
            }
            else
            {
                ok(hr == S_OK, "got %#lx.\n", hr);
                ok(value == expected_mask[expected_chans], "got %#x, expected %#x.\n",
                        value, expected_mask[expected_chans]);
            }

            ret = IMFMediaType_Release(type);
            ok(ret <= 1, "got %lu.\n", ret);
            winetest_pop_context();
        }
        ok(hr == MF_E_NO_MORE_TYPES, "got %#lx.\n", hr);
        if (many_channels)
            ok(i == ARRAY_SIZE(expect_available_outputs) * 2, "got %lu media types.\n", i);
        else
            ok(i == ARRAY_SIZE(expect_available_outputs), "got %lu media types.\n", i);
        winetest_pop_context();
    }

    ret = IMFTransform_Release(transform);
    ok(!ret, "got %lu.\n", ret);

failed:
    winetest_pop_context();
    CoUninitialize();
}

static void test_aac_decoder_user_data(void)
{
    /* https://wiki.multimedia.cx/index.php/MPEG-4_Audio */
    static const BYTE aac_raw_codec_data[] = {0x12, 0x08}; /* short form of 1 channel 44.1 Khz */
    static const BYTE aac_raw_codec_data_long[] = {0x17, 0x80, 0x56, 0x22, 0x08}; /* long form of 1 channel 44.1 Khz */
    static const BYTE aac_raw_codec_data_48khz[] = {0x11, 0x90}; /* short form of 1 channel 48 Khz */
    static const struct attribute_desc raw_aac_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_RAW_AAC1, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_BLOB(MF_MT_USER_DATA, aac_raw_codec_data, sizeof(aac_raw_codec_data), .required = TRUE),
        {0},
    };
    static const struct attribute_desc raw_aac_input_type_desc_long[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_RAW_AAC1, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_BLOB(MF_MT_USER_DATA, aac_raw_codec_data_long, sizeof(aac_raw_codec_data_long), .required = TRUE),
        {0},
    };
    static const struct attribute_desc raw_aac_input_type_desc_48khz[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_RAW_AAC1, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_BLOB(MF_MT_USER_DATA, aac_raw_codec_data_48khz, sizeof(aac_raw_codec_data_48khz), .required = TRUE),
        {0},
    };
    static const struct attribute_desc raw_aac_input_type_desc_mismatch[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_RAW_AAC1, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_BLOB(MF_MT_USER_DATA, aac_raw_codec_data_48khz, sizeof(aac_raw_codec_data_48khz), .required = TRUE),
        {0},
    };
    static const struct attribute_desc aac_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_AAC, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100, .required = TRUE),
        ATTR_BLOB(MF_MT_USER_DATA, test_aac_codec_data, sizeof(test_aac_codec_data), .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 12000),
        ATTR_UINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, 41),
        ATTR_UINT32(MF_MT_AAC_PAYLOAD_TYPE, 0),
        {0},
    };

    static struct {
        const char *name;
        const struct attribute_desc *desc;
        HRESULT exp_result;
        BOOL todo;
        BOOL todo_short;
    } tests[] = {
        { "aac",              aac_input_type_desc,              S_OK,                  FALSE, TRUE },
        { "raw aac",          raw_aac_input_type_desc,          S_OK                               },
        { "raw aac long",     raw_aac_input_type_desc_long,     S_OK,                  FALSE, TRUE },
        { "raw aac 48Khz",    raw_aac_input_type_desc_48khz,    S_OK                               },
        { "raw aac mismatch", raw_aac_input_type_desc_mismatch, MF_E_INVALIDMEDIATYPE, TRUE        },
    };

    const struct attribute_desc *input_type_desc;
    struct attribute_desc input_desc[64];
    unsigned int user_data_index = ~0u;
    IMFTransform *transform;
    ULONG ret, i, j;
    HRESULT hr;

    winetest_push_context("aacdec user_data");
    hr = CoInitialize(NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);

    if (FAILED(hr = CoCreateInstance(&CLSID_MSAACDecMFT, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
    {
        win_skip("AAC decoder transform is not available.\n");
        goto failed;
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        winetest_push_context("%s", tests[i].name);
        user_data_index = ~0u;
        input_type_desc = tests[i].desc;
        for (j = 0; j < ARRAY_SIZE(input_desc); j++)
        {
            input_desc[j] = input_type_desc[j];
            if (!input_desc[j].key)
                break;
            if (IsEqualGUID(input_desc[j].key, &MF_MT_USER_DATA))
                user_data_index = j;
        }

        ok(user_data_index != ~0u, "Could not find MF_MT_USER_DATA.\n");
        ok(i < ARRAY_SIZE(input_desc), "Too many attributes.\n");

        /* confirm standard input result */
        check_mft_set_input_type_(__LINE__, transform, input_desc, tests[i].exp_result, tests[i].todo);

        if (tests[i].exp_result == S_OK)
        {
            /* confirm shorter fails */
            input_desc[user_data_index].value.blob.cbSize = input_type_desc[user_data_index].value.blob.cbSize - 1;
            check_mft_set_input_type_(__LINE__, transform, input_desc, MF_E_INVALIDMEDIATYPE, tests[i].todo_short);

            /* confirm longer is OK */
            input_desc[user_data_index].value.blob.cbSize = input_type_desc[user_data_index].value.blob.cbSize + 1;
            check_mft_set_input_type(transform, input_desc, S_OK);
        }
        winetest_pop_context();
    }

    ret = IMFTransform_Release(transform);
    ok(!ret, "got %lu.\n", ret);

failed:
    winetest_pop_context();
    CoUninitialize();
}

static void test_aac_decoder(void)
{
    static const BYTE aac_raw_codec_data[] = {0x12, 0x08};
    static const struct attribute_desc raw_aac_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_RAW_AAC1, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_BLOB(MF_MT_USER_DATA, aac_raw_codec_data, sizeof(aac_raw_codec_data), .required = TRUE),
        {0},
    };
    static const struct attribute_desc aac_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_AAC, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100, .required = TRUE),
        ATTR_BLOB(MF_MT_USER_DATA, test_aac_codec_data, sizeof(test_aac_codec_data), .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 12000),
        ATTR_UINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, 41),
        ATTR_UINT32(MF_MT_AAC_PAYLOAD_TYPE, 0),
        {0},
    };

    test_aac_decoder_subtype(aac_input_type_desc);
    test_aac_decoder_subtype(raw_aac_input_type_desc);

    test_aac_decoder_channels(aac_input_type_desc);
    test_aac_decoder_channels(raw_aac_input_type_desc);
}

static const BYTE wma_codec_data[10] = {0, 0x44, 0, 0, 0x17, 0, 0, 0, 0, 0};
static const ULONG wmaenc_block_size = 1487;
static const ULONG wmadec_block_size = 0x2000;

static void test_wma_encoder(void)
{
    const GUID *const class_id = &CLSID_CWMAEncMediaObject;
    const struct transform_info expect_mft_info =
    {
        .name = L"WMAudio Encoder MFT",
        .major_type = &MFMediaType_Audio,
        .inputs =
        {
            {.subtype = &MFAudioFormat_PCM},
            {.subtype = &MFAudioFormat_Float},
        },
        .outputs =
        {
            {.subtype = &MFAudioFormat_WMAudioV8},
            {.subtype = &MFAudioFormat_WMAudioV9},
            {.subtype = &MFAudioFormat_WMAudio_Lossless},
        },
    };
    const struct transform_info expect_dmo_info =
    {
        .name = L"WMAudio Encoder DMO",
        .major_type = &MEDIATYPE_Audio,
        .inputs =
        {
            {.subtype = &MEDIASUBTYPE_PCM},
        },
        .outputs =
        {
            {.subtype = &MEDIASUBTYPE_WMAUDIO2},
            {.subtype = &MEDIASUBTYPE_WMAUDIO3},
            {.subtype = &MEDIASUBTYPE_WMAUDIO_LOSSLESS},
        },
    };

    static const struct attribute_desc input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 2 * (32 / 8), .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 2 * (32 / 8) * 22050, .required = TRUE),
        {0},
    };
    const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_WMAudioV8, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 4003, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, wmaenc_block_size, .required = TRUE),
        ATTR_BLOB(MF_MT_USER_DATA, wma_codec_data, sizeof(wma_codec_data), .required = TRUE),
        {0},
    };
    static const struct attribute_desc expect_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 8),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 22050 * 8),
        ATTR_UINT32(MF_MT_AUDIO_CHANNEL_MASK, 3),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        {0},
    };
    const struct attribute_desc expect_output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_WMAudioV8),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 4003),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, wmaenc_block_size),
        ATTR_BLOB(MF_MT_USER_DATA, wma_codec_data, sizeof(wma_codec_data)),
        ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        {0},
    };
    const MFT_OUTPUT_STREAM_INFO output_info =
    {
        .cbSize = wmaenc_block_size,
        .cbAlignment = 1,
    };
    const MFT_INPUT_STREAM_INFO input_info =
    {
        .hnsMaxLatency = 19969161,
        .cbSize = 8,
        .cbAlignment = 1,
    };

    const struct buffer_desc output_buffer_desc[] =
    {
        {.length = wmaenc_block_size},
    };
    const struct attribute_desc output_sample_attributes[] =
    {
        ATTR_UINT32(mft_output_sample_incomplete, 1),
        ATTR_UINT32(MFSampleExtension_CleanPoint, 1),
        {0},
    };
    const struct sample_desc output_sample_desc[] =
    {
        {
            .attributes = output_sample_attributes,
            .sample_time = 0, .sample_duration = 3250794,
            .buffer_count = 1, .buffers = output_buffer_desc,
        },
        {
            .attributes = output_sample_attributes,
            .sample_time = 3250794, .sample_duration = 3715193,
            .buffer_count = 1, .buffers = output_buffer_desc,
        },
        {
            .attributes = output_sample_attributes,
            .sample_time = 6965986, .sample_duration = 3366893,
            .buffer_count = 1, .buffers = output_buffer_desc,
        },
    };

    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Audio, MFAudioFormat_WMAudioV8};
    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Audio, MFAudioFormat_Float};
    IMFSample *input_sample, *output_sample;
    IMFCollection *output_samples;
    DWORD length, output_status;
    IMFMediaType *media_type;
    IMFTransform *transform;
    const BYTE *audio_data;
    ULONG audio_data_len;
    ULONG i, ret;
    HRESULT hr;
    LONG ref;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    winetest_push_context("wmaenc");

    if (!check_mft_enum(MFT_CATEGORY_AUDIO_ENCODER, &input_type, &output_type, class_id))
        goto failed;
    check_mft_get_info(class_id, &expect_mft_info);
    check_dmo_get_info(class_id, &expect_dmo_info);

    if (FAILED(hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_IMediaObject, TRUE);
    check_interface(transform, &IID_IPropertyStore, TRUE);
    check_interface(transform, &IID_IPropertyBag, TRUE);

    check_mft_optional_methods(transform, 1);
    check_mft_get_attributes(transform, NULL, FALSE);
    check_mft_get_input_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);
    check_mft_get_output_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);

    check_mft_set_input_type_required(transform, input_type_desc);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    init_media_type(media_type, input_type_desc, -1);
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 0, "Release returned %lu\n", ret);

    check_mft_set_output_type_required(transform, output_type_desc);
    check_mft_set_output_type(transform, output_type_desc, S_OK);
    check_mft_get_output_current_type(transform, expect_output_type_desc);

    check_mft_set_input_type_required(transform, input_type_desc);
    check_mft_set_input_type(transform, input_type_desc, S_OK);
    check_mft_get_input_current_type(transform, expect_input_type_desc);

    check_mft_get_input_stream_info(transform, S_OK, &input_info);
    check_mft_get_output_stream_info(transform, S_OK, &output_info);

    load_resource(L"audiodata.bin", &audio_data, &audio_data_len);
    ok(audio_data_len == 179928, "got length %lu\n", audio_data_len);

    input_sample = create_sample(audio_data, audio_data_len);
    hr = IMFSample_SetSampleTime(input_sample, 0);
    ok(hr == S_OK, "SetSampleTime returned %#lx\n", hr);
    hr = IMFSample_SetSampleDuration(input_sample, 10000000);
    ok(hr == S_OK, "SetSampleDuration returned %#lx\n", hr);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, 0);
    ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);
    ref = IMFSample_Release(input_sample);
    ok(ref <= 1, "Release returned %ld\n", ref);

    hr = MFCreateCollection(&output_samples);
    ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

    output_sample = create_sample(NULL, output_info.cbSize);
    for (i = 0; SUCCEEDED(hr = check_mft_process_output(transform, output_sample, &output_status)); i++)
    {
        winetest_push_context("%lu", i);
        ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
        ok(output_status == MFT_OUTPUT_DATA_BUFFER_INCOMPLETE, "got output[0].dwStatus %#lx\n", output_status);
        hr = IMFCollection_AddElement(output_samples, (IUnknown *)output_sample);
        ok(hr == S_OK, "AddElement returned %#lx\n", hr);
        ref = IMFSample_Release(output_sample);
        ok(ref == 1, "Release returned %ld\n", ref);
        output_sample = create_sample(NULL, output_info.cbSize);
        winetest_pop_context();
    }
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    ok(i == 3, "got %lu output samples\n", i);

    ret = check_mf_sample_collection(output_samples, output_sample_desc, L"wmaencdata.bin");
    ok(ret == 0, "got %lu%% diff\n", ret);
    IMFCollection_Release(output_samples);

    output_sample = create_sample(NULL, output_info.cbSize);
    hr = check_mft_process_output(transform, output_sample, &output_status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
    hr = IMFSample_GetTotalLength(output_sample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

failed:
    winetest_pop_context();
    CoUninitialize();
}

static void test_wma_decoder(void)
{
    const GUID *const class_id = &CLSID_CWMADecMediaObject;
    const struct transform_info expect_mft_info =
    {
        .name = L"WMAudio Decoder MFT",
        .major_type = &MFMediaType_Audio,
        .inputs =
        {
            {.subtype = &MEDIASUBTYPE_MSAUDIO1},
            {.subtype = &MFAudioFormat_WMAudioV8},
            {.subtype = &MFAudioFormat_WMAudioV9},
            {.subtype = &MFAudioFormat_WMAudio_Lossless},
        },
        .outputs =
        {
            {.subtype = &MFAudioFormat_PCM},
            {.subtype = &MFAudioFormat_Float},
        },
    };
    const struct transform_info expect_dmo_info =
    {
        .name = L"WMAudio Decoder DMO",
        .major_type = &MEDIATYPE_Audio,
        .inputs =
        {
            {.subtype = &MEDIASUBTYPE_MSAUDIO1},
            {.subtype = &MEDIASUBTYPE_WMAUDIO2},
            {.subtype = &MEDIASUBTYPE_WMAUDIO3},
            {.subtype = &MEDIASUBTYPE_WMAUDIO_LOSSLESS},
        },
        .outputs =
        {
            {.subtype = &MEDIASUBTYPE_PCM},
            {.subtype = &MEDIASUBTYPE_IEEE_FLOAT},
        },
    };

    static const media_type_desc expect_available_inputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_MSAUDIO1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_WMAudioV8),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_WMAudioV9),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_WMAudio_Lossless),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
    };
    static const media_type_desc expect_available_outputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
            ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
            ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
            ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 176400),
            ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 8),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
            ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
            ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
            ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 88200),
            ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        },
    };

    const struct attribute_desc input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_WMAudioV8, .required = TRUE),
        ATTR_BLOB(MF_MT_USER_DATA, wma_codec_data, sizeof(wma_codec_data), .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, wmaenc_block_size, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 4003), /* not required by SetInputType, but needed for the transform to work */
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
        {0},
    };
    static const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 2 * (16 / 8), .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 2 * (16 / 8) * 22050, .required = TRUE),
        {0},
    };
    const struct attribute_desc expect_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_WMAudioV8),
        ATTR_BLOB(MF_MT_USER_DATA, wma_codec_data, sizeof(wma_codec_data)),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, wmaenc_block_size),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 4003),
        ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
        {0},
    };
    static const struct attribute_desc expect_output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 22050 * 4),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
        ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        {0},
    };
    const MFT_INPUT_STREAM_INFO input_info =
    {
        .cbSize = wmaenc_block_size,
        .cbAlignment = 1,
    };
    const MFT_OUTPUT_STREAM_INFO output_info =
    {
        .cbSize = wmadec_block_size,
        .cbAlignment = 1,
    };

    const struct buffer_desc output_buffer_desc[] =
    {
        {.length = wmadec_block_size, .compare = compare_pcm16},
        {.length = wmadec_block_size / 2, .compare = compare_pcm16, .todo_length = TRUE},
    };
    const struct attribute_desc output_sample_attributes[] =
    {
        ATTR_UINT32(mft_output_sample_incomplete, 1),
        ATTR_UINT32(MFSampleExtension_CleanPoint, 1),
        {0},
    };
    const struct attribute_desc output_sample_attributes_todo[] =
    {
        ATTR_UINT32(mft_output_sample_incomplete, 1, .todo = TRUE),
        ATTR_UINT32(MFSampleExtension_CleanPoint, 1),
        {0},
    };
    struct sample_desc output_sample_desc[] =
    {
        {
            .attributes = output_sample_attributes + 0,
            .sample_time = 0, .sample_duration = 928798,
            .buffer_count = 1, .buffers = output_buffer_desc + 0, .repeat_count = 1,
        },
        {
            .attributes = output_sample_attributes + 0,
            .sample_time = 1857596, .sample_duration = 928798,
            .buffer_count = 1, .buffers = output_buffer_desc + 0,
        },
        {
            .attributes = output_sample_attributes + 1, /* not MFT_OUTPUT_DATA_BUFFER_INCOMPLETE */
            .sample_time = 2786394, .sample_duration = 464399, .todo_duration = 928798,
            .buffer_count = 1, .buffers = output_buffer_desc + 1, .todo_length = TRUE,
        },
    };

    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Audio, MFAudioFormat_WMAudioV8};
    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Audio, MFAudioFormat_Float};
    IUnknown *unknown, *tmp_unknown, outer = {&test_unk_vtbl};
    IMFSample *input_sample, *output_sample;
    IMFCollection *output_samples;
    DWORD length, output_status;
    IMediaObject *media_object;
    IPropertyBag *property_bag;
    IMFMediaType *media_type;
    IMFTransform *transform;
    const BYTE *wmaenc_data;
    ULONG wmaenc_data_len;
    ULONG i, ret, ref;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    winetest_push_context("wmadec");

    if (!check_mft_enum(MFT_CATEGORY_AUDIO_DECODER, &input_type, &output_type, class_id))
        goto failed;
    check_mft_get_info(class_id, &expect_mft_info);
    check_dmo_get_info(class_id, &expect_dmo_info);

    if (FAILED(hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_IMediaObject, TRUE);
    todo_wine
    check_interface(transform, &IID_IPropertyStore, TRUE);
    check_interface(transform, &IID_IPropertyBag, TRUE);

    check_mft_optional_methods(transform, 1);
    check_mft_get_attributes(transform, NULL, FALSE);
    check_mft_get_input_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);
    check_mft_get_output_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetOutputAvailableType returned %#lx\n", hr);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetInputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("in %lu", i);
        ok(hr == S_OK, "GetInputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_inputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    todo_wine
    ok(hr == MF_E_NO_MORE_TYPES, "GetInputAvailableType returned %#lx\n", hr);
    todo_wine
    ok(i == 4, "%lu input media types\n", i);

    /* setting output media type first doesn't work */
    check_mft_set_output_type(transform, output_type_desc, MF_E_TRANSFORM_TYPE_NOT_SET);
    check_mft_get_output_current_type_(__LINE__, transform, NULL, TRUE, FALSE);

    check_mft_set_input_type_required(transform, input_type_desc);
    check_mft_set_input_type(transform, input_type_desc, S_OK);
    check_mft_get_input_current_type_(__LINE__, transform, expect_input_type_desc, TRUE, FALSE);

    check_mft_get_input_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);
    check_mft_get_output_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);

    /* check new output media types */

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == 2, "%lu output media types\n", i);

    check_mft_set_output_type_required(transform, output_type_desc);
    check_mft_set_output_type(transform, output_type_desc, S_OK);
    check_mft_get_output_current_type_(__LINE__, transform, expect_output_type_desc, TRUE, FALSE);

    check_mft_get_input_stream_info(transform, S_OK, &input_info);
    check_mft_get_output_stream_info(transform, S_OK, &output_info);

    load_resource(L"wmaencdata.bin", &wmaenc_data, &wmaenc_data_len);
    ok(wmaenc_data_len % wmaenc_block_size == 0, "got length %lu\n", wmaenc_data_len);

    input_sample = create_sample(wmaenc_data, wmaenc_block_size / 2);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
    ret = IMFSample_Release(input_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    input_sample = create_sample(wmaenc_data, wmaenc_block_size + 1);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
    ret = IMFSample_Release(input_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    input_sample = create_sample(wmaenc_data, wmaenc_block_size);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);
    ret = IMFSample_Release(input_sample);
    ok(ret == 1, "Release returned %lu\n", ret);

    /* As output_info.dwFlags doesn't have MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES
     * IMFTransform_ProcessOutput needs a sample or returns MF_E_TRANSFORM_NEED_MORE_INPUT */

    hr = check_mft_process_output(transform, NULL, &output_status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE
            || broken(output_status == (MFT_OUTPUT_DATA_BUFFER_INCOMPLETE|MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE)) /* Win7 */,
            "got output[0].dwStatus %#lx\n", output_status);

    input_sample = create_sample(wmaenc_data, wmaenc_block_size);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);
    ret = IMFSample_Release(input_sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    hr = check_mft_process_output(transform, NULL, &output_status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE
            || broken(output_status == (MFT_OUTPUT_DATA_BUFFER_INCOMPLETE|MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE)) /* Win7 */,
            "got output[0].dwStatus %#lx\n", output_status);

    hr = MFCreateCollection(&output_samples);
    ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

    output_sample = create_sample(NULL, output_info.cbSize);
    for (i = 0; SUCCEEDED(hr = check_mft_process_output(transform, output_sample, &output_status)); i++)
    {
        winetest_push_context("%lu", i);
        ok(!(output_status & ~MFT_OUTPUT_DATA_BUFFER_INCOMPLETE), "got output[0].dwStatus %#lx\n", output_status);
        ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
        hr = IMFCollection_AddElement(output_samples, (IUnknown *)output_sample);
        ok(hr == S_OK, "AddElement returned %#lx\n", hr);
        ref = IMFSample_Release(output_sample);
        ok(ref == 1, "Release returned %ld\n", ref);
        output_sample = create_sample(NULL, output_info.cbSize);
        winetest_pop_context();
    }
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    todo_wine_if(i == 3) /* wmadec output depends on ffmpeg version used */
    ok(i == 4, "got %lu output samples\n", i);

    if (!strcmp(winetest_platform, "wine") && i == 3)
        output_sample_desc[1].attributes = output_sample_attributes_todo;

    ret = check_mf_sample_collection(output_samples, output_sample_desc, L"wmadecdata.bin");
    todo_wine_if(ret > 0 && ret <= 10)  /* ffmpeg sometimes offsets the decoded data */
    ok(ret == 0, "got %lu%% diff\n", ret);
    IMFCollection_Release(output_samples);

    output_sample = create_sample(NULL, output_info.cbSize);
    hr = check_mft_process_output(transform, output_sample, &output_status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
    hr = IMFSample_GetTotalLength(output_sample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    input_sample = create_sample(wmaenc_data, wmaenc_block_size);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);

    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);
    ret = IMFSample_Release(input_sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    hr = CoCreateInstance( &CLSID_CWMADecMediaObject, &outer, CLSCTX_INPROC_SERVER, &IID_IUnknown,
                           (void **)&unknown );
    ok( hr == S_OK, "CoCreateInstance returned %#lx\n", hr );
    hr = IUnknown_QueryInterface( unknown, &IID_IMFTransform, (void **)&transform );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IUnknown_QueryInterface( unknown, &IID_IMediaObject, (void **)&media_object );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IUnknown_QueryInterface( unknown, &IID_IPropertyBag, (void **)&property_bag );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IUnknown_QueryInterface( media_object, &IID_IUnknown, (void **)&tmp_unknown );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    ok( unknown != &outer, "got outer IUnknown\n" );
    ok( transform != (void *)unknown, "got IUnknown == IMFTransform\n" );
    ok( media_object != (void *)unknown, "got IUnknown == IMediaObject\n" );
    ok( property_bag != (void *)unknown, "got IUnknown == IPropertyBag\n" );
    ok( tmp_unknown != unknown, "got inner IUnknown\n" );

    check_interface( unknown, &IID_IPersistPropertyBag, FALSE );
    check_interface( unknown, &IID_IAMFilterMiscFlags, FALSE );
    check_interface( unknown, &IID_IMediaSeeking, FALSE );
    check_interface( unknown, &IID_IMediaPosition, FALSE );
    check_interface( unknown, &IID_IReferenceClock, FALSE );
    check_interface( unknown, &IID_IBasicAudio, FALSE );

    ref = IUnknown_Release( tmp_unknown );
    ok( ref == 1, "Release returned %lu\n", ref );
    ref = IPropertyBag_Release( property_bag );
    ok( ref == 1, "Release returned %lu\n", ref );
    ref = IMediaObject_Release( media_object );
    ok( ref == 1, "Release returned %lu\n", ref );
    ref = IMFTransform_Release( transform );
    ok( ref == 1, "Release returned %lu\n", ref );
    ref = IUnknown_Release( unknown );
    ok( ref == 0, "Release returned %lu\n", ref );

failed:
    winetest_pop_context();
    CoUninitialize();
}

static void test_wma_decoder_dmo_input_type(void)
{
    const DMO_MEDIA_TYPE expected_input_types[] =
    {
        {MEDIATYPE_Audio, MEDIASUBTYPE_MSAUDIO1        },
        {MEDIATYPE_Audio, MEDIASUBTYPE_WMAUDIO2        },
        {MEDIATYPE_Audio, MEDIASUBTYPE_WMAUDIO3        },
        {MEDIATYPE_Audio, MEDIASUBTYPE_WMAUDIO_LOSSLESS},
    };

    DMO_MEDIA_TYPE *good_input_type, *bad_input_type, type;
    char buffer_good[1024], buffer_bad[1024];
    DWORD count, i, ret;
    IMediaObject *dmo;
    HRESULT hr;

    winetest_push_context("wmadec");

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "CoInitialize failed, hr %#lx.\n", hr);

    if (FAILED(hr = CoCreateInstance(&CLSID_CWMADecMediaObject, NULL, CLSCTX_INPROC_SERVER, &IID_IMediaObject, (void **)&dmo)))
    {
        CoUninitialize();
        winetest_pop_context();
        return;
    }

    good_input_type = (void *)buffer_good;
    bad_input_type = (void *)buffer_bad;

    /* Test GetInputType. */
    count = ARRAY_SIZE(expected_input_types);
    hr = IMediaObject_GetInputType(dmo, 1, 0, NULL);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputType(dmo, 1, 0, &type);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputType(dmo, 1, count, &type);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputType(dmo, 0, count, &type);
    ok(hr == DMO_E_NO_MORE_ITEMS, "GetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputType(dmo, 0, count, NULL);
    ok(hr == DMO_E_NO_MORE_ITEMS, "GetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputType(dmo, 0, 0xdeadbeef, NULL);
    ok(hr == DMO_E_NO_MORE_ITEMS, "GetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputType(dmo, 0, count - 1, NULL);
    ok(hr == S_OK, "GetInputType returned %#lx.\n", hr);

    i = -1;
    while (SUCCEEDED(hr = IMediaObject_GetInputType(dmo, 0, ++i, &type)))
    {
        winetest_push_context("type %lu", i);
        check_dmo_media_type(&type, &expected_input_types[i]);
        MoFreeMediaType(&type);
        winetest_pop_context();
    }
    ok(hr == DMO_E_NO_MORE_ITEMS, "GetInputType returned %#lx.\n", hr);
    ok(i == count, "%lu types.\n", i);

    /* Test SetInputType. */
    init_dmo_media_type_audio(good_input_type, &MEDIASUBTYPE_WMAUDIO2, 2, 22050, 32);
    memset(bad_input_type, 0, sizeof(buffer_bad));

    hr = IMediaObject_SetInputType(dmo, 1, NULL, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, bad_input_type, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, good_input_type, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, NULL, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, bad_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, NULL, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, bad_input_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, good_input_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, NULL, 0x4);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, bad_input_type, 0x4);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, good_input_type, 0x4);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);

    hr = IMediaObject_SetInputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR | DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == E_INVALIDARG, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR | 0x4);
    ok(hr == E_INVALIDARG, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, NULL, 0);
    ok(hr == E_POINTER, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, NULL, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == E_POINTER, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, NULL, 0x4);
    ok(hr == E_POINTER, "SetInputType returned %#lx.\n", hr);

    hr = IMediaObject_SetInputType(dmo, 0, bad_input_type, 0);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, bad_input_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, bad_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, bad_input_type, 0x4);
    ok(hr == E_INVALIDARG, "SetInputType returned %#lx.\n", hr);

    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, 0x4);
    ok(hr == E_INVALIDARG, "SetInputType returned %#lx.\n", hr);

    /* Test GetInputCurrentType. */
    hr = IMediaObject_SetInputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputCurrentType(dmo, 1, NULL);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetInputCurrentType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputCurrentType(dmo, 0, NULL);
    ok(hr == DMO_E_TYPE_NOT_SET, "GetInputCurrentType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputCurrentType(dmo, 1, &type);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetInputCurrentType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputCurrentType(dmo, 0, &type);
    ok(hr == DMO_E_TYPE_NOT_SET, "GetInputCurrentType returned %#lx.\n", hr);

    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputCurrentType(dmo, 1, NULL);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetInputCurrentType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputCurrentType(dmo, 0, NULL);
    ok(hr == E_POINTER, "GetInputCurrentType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputCurrentType(dmo, 1, &type);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetInputCurrentType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputCurrentType(dmo, 0, &type);
    ok(hr == S_OK, "GetInputCurrentType returned %#lx.\n", hr);
    check_dmo_media_type(&type, good_input_type);
    MoFreeMediaType(&type);

    /* Cleanup. */
    ret = IMediaObject_Release(dmo);
    ok(ret == 0, "Release returned %lu\n", ret);
    CoUninitialize();
    winetest_pop_context();
}

static void test_wma_decoder_dmo_output_type(void)
{
    const UINT channel_count = 2, rate = 22050, bits_per_sample = WMAUDIO_BITS_PER_SAMPLE;
    const GUID* input_subtype = &MEDIASUBTYPE_WMAUDIO2;
    const WAVEFORMATEX expected_output_info =
    {
        WAVE_FORMAT_PCM, channel_count, rate, bits_per_sample * rate * channel_count / 8,
        bits_per_sample * channel_count / 8, WMAUDIO_BITS_PER_SAMPLE, 0
    };
    const DMO_MEDIA_TYPE expected_output_type =
    {
        MEDIATYPE_Audio, MEDIASUBTYPE_PCM, TRUE, FALSE, 0, FORMAT_WaveFormatEx, NULL,
        sizeof(expected_output_info), (BYTE *)&expected_output_info
    };

    char buffer_good_output[1024], buffer_bad_output[1024], buffer_input[1024];
    DMO_MEDIA_TYPE *good_output_type, *bad_output_type, *input_type, type;
    DWORD count, i, ret, size, alignment;
    IMediaObject *dmo;
    HRESULT hr;

    winetest_push_context("wmadec");

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "CoInitialize failed, hr %#lx.\n", hr);

    if (FAILED(hr = CoCreateInstance(&CLSID_CWMADecMediaObject, NULL, CLSCTX_INPROC_SERVER, &IID_IMediaObject, (void **)&dmo)))
    {
        CoUninitialize();
        winetest_pop_context();
        return;
    }

    /* Initialize media types. */
    input_type = (void *)buffer_input;
    good_output_type = (void *)buffer_good_output;
    bad_output_type = (void *)buffer_bad_output;
    init_dmo_media_type_audio(input_type, input_subtype, channel_count, rate, 16);
    ((WAVEFORMATEX *)(input_type + 1))->nBlockAlign = 640;
    ((WAVEFORMATEX *)(input_type + 1))->nAvgBytesPerSec = 2000;
    init_dmo_media_type_audio(good_output_type, &MEDIASUBTYPE_PCM, channel_count, rate, bits_per_sample);
    memset(bad_output_type, 0, sizeof(buffer_bad_output));

    /* Test GetOutputType. */
    hr = IMediaObject_GetOutputType(dmo, 1, 0, NULL);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 0, 0, NULL);
    ok(hr == DMO_E_TYPE_NOT_SET, "GetOutputType returned %#lx.\n", hr);

    hr = IMediaObject_SetInputType(dmo, 0, input_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);

    count = 1;
    hr = IMediaObject_GetOutputType(dmo, 1, 0, NULL);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 1, 0, &type);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 1, count, &type);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 0, count, &type);
    ok(hr == DMO_E_NO_MORE_ITEMS, "GetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 0, count, NULL);
    ok(hr == DMO_E_NO_MORE_ITEMS, "GetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 0, 0xdeadbeef, NULL);
    ok(hr == DMO_E_NO_MORE_ITEMS, "GetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 0, count - 1, NULL);
    ok(hr == S_OK, "GetOutputType returned %#lx.\n", hr);

    i = -1;
    while (SUCCEEDED(hr = IMediaObject_GetOutputType(dmo, 0, ++i, &type)))
    {
        winetest_push_context("type %lu", i);
        check_dmo_media_type(&type, &expected_output_type);
        MoFreeMediaType(&type);
        winetest_pop_context();
    }
    ok(hr == DMO_E_NO_MORE_ITEMS, "GetInputType returned %#lx.\n", hr);
    ok(i == count, "%lu types.\n", i);

    /* Test SetOutputType. */
    hr = IMediaObject_SetInputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, NULL, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, good_output_type, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, 0);
    ok(hr == E_POINTER, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, bad_output_type, 0);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, bad_output_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, 0);
    ok(hr == DMO_E_TYPE_NOT_SET, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_SET, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, 0x4);
    ok(hr == E_INVALIDARG, "SetOutputType returned %#lx.\n", hr);

    hr = IMediaObject_SetInputType(dmo, 0, input_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);

    hr = IMediaObject_SetOutputType(dmo, 1, NULL, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, bad_output_type, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, good_output_type, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, NULL, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, bad_output_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, good_output_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, NULL, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, bad_output_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, good_output_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, NULL, 0x4);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, bad_output_type, 0x4);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, good_output_type, 0x4);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);

    hr = IMediaObject_SetOutputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR | DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == E_INVALIDARG, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR | 0x4);
    ok(hr == E_INVALIDARG, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, 0);
    ok(hr == E_POINTER, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == E_POINTER, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, 0x4);
    ok(hr == E_POINTER, "SetOutputType returned %#lx.\n", hr);

    hr = IMediaObject_SetOutputType(dmo, 0, bad_output_type, 0);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, bad_output_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, bad_output_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, bad_output_type, 0x4);
    ok(hr == E_INVALIDARG, "SetOutputType returned %#lx.\n", hr);

    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, 0);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, 0x4);
    ok(hr == E_INVALIDARG, "SetOutputType returned %#lx.\n", hr);

    /* Test GetOutputCurrentType. */
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputCurrentType(dmo, 1, NULL);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetOutputCurrentType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputCurrentType(dmo, 0, NULL);
    ok(hr == DMO_E_TYPE_NOT_SET, "GetOutputCurrentType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputCurrentType(dmo, 1, &type);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetOutputCurrentType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputCurrentType(dmo, 0, &type);
    ok(hr == DMO_E_TYPE_NOT_SET, "GetOutputCurrentType returned %#lx.\n", hr);

    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, 0);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputCurrentType(dmo, 1, NULL);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetOutputCurrentType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputCurrentType(dmo, 0, NULL);
    ok(hr == E_POINTER, "GetOutputCurrentType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputCurrentType(dmo, 1, &type);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetOutputCurrentType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputCurrentType(dmo, 0, &type);
    ok(hr == S_OK, "GetOutputCurrentType returned %#lx.\n", hr);
    check_dmo_media_type(&type, good_output_type);
    MoFreeMediaType(&type);

    /* Test GetOutputSizeInfo. */
    hr = IMediaObject_GetOutputSizeInfo(dmo, 1, NULL, NULL);
    ok(hr == E_POINTER, "GetOutputSizeInfo returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputSizeInfo(dmo, 0, NULL, NULL);
    ok(hr == E_POINTER, "GetOutputSizeInfo returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputSizeInfo(dmo, 0, &size, NULL);
    ok(hr == E_POINTER, "GetOutputSizeInfo returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputSizeInfo(dmo, 0, NULL, &alignment);
    ok(hr == E_POINTER, "GetOutputSizeInfo returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputSizeInfo(dmo, 0, &size, &alignment);
    ok(hr == S_OK, "GetOutputSizeInfo returned %#lx.\n", hr);
    ok(size == 8192, "Unexpected size %lu.\n", size);
    ok(alignment == 1, "Unexpected alignment %lu.\n", alignment);

    hr = IMediaObject_GetInputCurrentType(dmo, 0, input_type);
    ok(hr == S_OK, "GetInputCurrentType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, input_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputCurrentType(dmo, 0, &type);
    ok(hr == S_OK, "GetOutputCurrentType returned %#lx.\n", hr);

    init_dmo_media_type_audio(input_type, input_subtype, channel_count, rate * 2, 32);
    hr = IMediaObject_SetInputType(dmo, 0, input_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputCurrentType(dmo, 0, &type);
    todo_wine ok(hr == DMO_E_TYPE_NOT_SET, "GetOutputCurrentType returned %#lx.\n", hr);

    /* Cleanup. */
    ret = IMediaObject_Release(dmo);
    ok(ret == 0, "Release returned %lu\n", ret);
    CoUninitialize();
    winetest_pop_context();
}

#define next_h264_sample(a, b) next_h264_sample_(__LINE__, a, b)
static IMFSample *next_h264_sample_(int line, const BYTE **h264_buf, ULONG *h264_len)
{
    const BYTE *sample_data;

    ok_(__FILE__, line)(*h264_len > 4, "invalid h264 length\n");
    ok_(__FILE__, line)(*(UINT32 *)*h264_buf == 0x01000000, "invalid h264 buffer\n");
    sample_data = *h264_buf;

    (*h264_len) -= 4;
    (*h264_buf) += 4;

    while (*h264_len >= 4 && *(UINT32 *)*h264_buf != 0x01000000)
    {
        (*h264_len)--;
        (*h264_buf)++;
    }

    return create_sample(sample_data, *h264_buf - sample_data);
}

static void test_h264_encoder(void)
{
    static const DWORD actual_width = 96, actual_height = 96;
    const GUID *const class_id = &CLSID_MSH264EncoderMFT;
    const struct transform_info expect_mft_info =
    {
        .name = L"H264 Encoder MFT",
        .major_type = &MFMediaType_Video,
        .inputs =
        {
            {.subtype = &MFVideoFormat_IYUV},
            {.subtype = &MFVideoFormat_YV12},
            {.subtype = &MFVideoFormat_NV12},
            {.subtype = &MFVideoFormat_YUY2},
        },
        .outputs =
        {
            {.subtype = &MFVideoFormat_H264},
        },
    };
    static const media_type_desc default_inputs[] =
    {
        {ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_IYUV)},
        {ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YV12)},
        {ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12)},
        {ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2)},
    };
    static const media_type_desc default_outputs[] =
    {
        {ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264)},
    };
    static const struct attribute_desc expect_transform_attributes[] =
    {
        ATTR_UINT32(MFT_ENCODER_SUPPORTS_CONFIG_EVENT, 1),
        {0},
    };
    static const struct attribute_desc expect_common_attributes[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        {0},
    };
    const struct attribute_desc expect_available_input_attributes[] =
    {
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        ATTR_UINT32(MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_Wide),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
        {0},
    };
    const struct attribute_desc input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_UINT32(test_attr_guid, 0),
        {0},
    };
    const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001, .required_set = TRUE),
        ATTR_UINT32(MF_MT_AVG_BITRATE, 193540, .required_set = TRUE),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive, .required_set = TRUE),
        ATTR_UINT32(test_attr_guid, 0),
        {0},
    };
    static const struct attribute_desc test_attributes[] =
     {
        ATTR_RATIO(MF_MT_FRAME_SIZE,           1920, 1080),
        ATTR_RATIO(MF_MT_FRAME_RATE,           10, 1),
        ATTR_UINT32(MF_MT_INTERLACE_MODE,      MFVideoInterlace_MixedInterlaceOrProgressive),
        ATTR_UINT32(MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_Normal),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO,   2, 1),
     };
    const struct attribute_desc expect_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        ATTR_UINT32(test_attr_guid, 0),
        {0},
    };
    const struct attribute_desc expect_output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        ATTR_UINT32(MF_MT_AVG_BITRATE, 193540),
        ATTR_BLOB(MF_MT_MPEG_SEQUENCE_HEADER, test_h264_sequence_header, sizeof(test_h264_sequence_header), .todo = TRUE),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive),
        ATTR_UINT32(test_attr_guid, 0),
        {0},
    };
    const struct attribute_desc expect_codec_api_attributes[] =
    {
        ATTR_UINT32(CODECAPI_AVEncCommonRateControlMode, eAVEncCommonRateControlMode_CBR, .todo = TRUE),
        ATTR_UINT32(CODECAPI_AVEncCommonQuality,           65,     .todo = TRUE),
        ATTR_UINT32(CODECAPI_AVEncCommonBufferSize,        72577,  .todo = TRUE),
        ATTR_UINT32(CODECAPI_AVEncCommonMaxBitRate,        0,      .todo = TRUE),
        ATTR_UINT32(CODECAPI_AVEncCommonMeanBitRate,       193540, .todo = TRUE),
        ATTR_UINT32(CODECAPI_AVEncCommonQualityVsSpeed,    33,     .todo = TRUE),
        ATTR_UINT32(CODECAPI_AVEncH264CABACEnable,         0,      .todo = TRUE),
        ATTR_UINT32(CODECAPI_AVEncH264PPSID,               0,      .todo = TRUE),
        ATTR_UINT32(CODECAPI_AVEncH264SPSID,               0,      .todo = TRUE),
        ATTR_UINT32(CODECAPI_AVEncMPVGOPSize,              0,      .todo = TRUE),
        ATTR_UINT32(CODECAPI_AVEncMPVDefaultBPictureCount, 1,      .todo = TRUE),
        ATTR_UINT64(CODECAPI_AVEncVideoEncodeQP,           26,     .todo = TRUE),
        ATTR_UINT32(CODECAPI_AVEncVideoMaxQP,              51,     .todo = TRUE),
        ATTR_UINT32(CODECAPI_AVEncVideoMinQP,              0,      .todo = TRUE),
        ATTR_UINT32(CODECAPI_AVEncVideoMaxNumRefFrame,     2,      .todo = TRUE),
        {0},
    };
    const struct attribute_desc output_sample_attributes_key[] =
    {
        ATTR_UINT32(MFSampleExtension_CleanPoint, 1),
        {0},
    };
    const struct buffer_desc output_buffer_desc = {.length = -1 /* Variable. */};
    const struct sample_desc output_sample_desc =
    {
        .attributes = output_sample_attributes_key,
        .sample_time = 333333, .sample_duration = 333333,
        .buffer_count = 1, .buffers = &output_buffer_desc,
    };
    MFT_OUTPUT_STREAM_INFO output_info, expect_output_info[] = {{.cbSize = 0x8000}, {.cbSize = 0x3bc400}};
    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Video, MFVideoFormat_H264};
    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Video, MFVideoFormat_NV12};
    IMFSample *input_sample, *output_sample;
    IMFCollection *output_sample_collection;
    const struct attribute_desc *desc;
    ULONG nv12frame_data_size, size;
    const BYTE *nv12frame_data;
    IMFMediaType *media_type;
    IMFTransform *transform;
    ICodecAPI *codec_api;
    DWORD output_status;
    HRESULT hr;
    ULONG ret;
    DWORD i;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    winetest_push_context("h264enc");

    if (!has_video_processor)
    {
        win_skip("Skipping inconsistent h264 encoder tests on Win7.\n");
        goto failed;
    }

    if (!check_mft_enum(MFT_CATEGORY_VIDEO_ENCODER, &input_type, &output_type, class_id))
        goto failed;
    check_mft_get_info(class_id, &expect_mft_info);

    hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "CoCreateInstance returned %#lx.\n", hr);

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_ICodecAPI, TRUE);
    check_interface(transform, &IID_IMediaObject, FALSE);
    check_interface(transform, &IID_IPropertyStore, FALSE);
    check_interface(transform, &IID_IPropertyBag, FALSE);

    check_mft_get_attributes(transform, expect_transform_attributes, FALSE);
    check_mft_get_input_stream_info(transform, S_OK, NULL);
    check_mft_get_output_stream_info(transform, S_OK, NULL);

    /* No input type is available before an output type is set. */
    hr = IMFTransform_GetInputAvailableType(transform, 0, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetInputAvailableType returned %#lx\n", hr);
    check_mft_set_input_type(transform, input_type_desc, MF_E_TRANSFORM_TYPE_NOT_SET);
    check_mft_get_input_current_type(transform, NULL);

    /* Check available output types. */
    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx.\n", hr);
        check_media_type(media_type, expect_common_attributes, -1);
        check_media_type(media_type, default_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx.\n", hr);
    ok(i == ARRAY_SIZE(default_outputs), "%lu output media types.\n", i);

    check_mft_set_output_type_required(transform, output_type_desc);
    check_mft_set_output_type(transform, output_type_desc, S_OK);
    check_mft_get_output_current_type_(__LINE__, transform, expect_output_type_desc, FALSE, TRUE);
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == S_OK, "GetOutputStreamInfo returned %#lx\n", hr);
    check_member(output_info, expect_output_info[0], "%#lx", dwFlags);
    todo_wine
    check_member(output_info, expect_output_info[0], "%#lx", cbSize);
    check_member(output_info, expect_output_info[0], "%#lx", cbAlignment);

    /* Input types can now be enumerated. */
    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetInputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "IMFTransform_GetInputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_common_attributes, -1);
        check_media_type(media_type, expect_available_input_attributes, -1);
        check_media_type(media_type, default_inputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "IMFTransform_GetInputAvailableType returned %#lx\n", hr);
    ok(i == ARRAY_SIZE(default_inputs), "%lu input media types\n", i);

    check_mft_set_input_type_required(transform, input_type_desc);
    check_mft_set_input_type(transform, input_type_desc, S_OK);
    check_mft_get_input_current_type(transform, expect_input_type_desc);
    check_mft_get_input_stream_info(transform, S_OK, NULL);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx.\n", hr);

    /* Input type attributes should match output type attributes. */
    for (i = 0; i < ARRAY_SIZE(test_attributes); ++i)
    {
        winetest_push_context("attr %lu", i);

        init_media_type(media_type, input_type_desc, -1);
        hr = IMFMediaType_SetItem(media_type, test_attributes[i].key, &test_attributes[i].value);
        ok(hr == S_OK, "SetItem returned %#lx.\n", hr);
        hr = IMFTransform_SetInputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
        ok(hr == MF_E_INVALIDMEDIATYPE, "SetInputType returned %#lx.\n", hr);

        winetest_pop_context();
    }

    /* Output info cbSize will change only if we change output type frame size. */
    for (i = 0; i < ARRAY_SIZE(test_attributes); ++i)
    {
        winetest_push_context("attr %lu", i);

        init_media_type(media_type, output_type_desc, -1);
        hr = IMFMediaType_SetItem(media_type, test_attributes[i].key, &test_attributes[i].value);
        ok(hr == S_OK, "SetItem returned %#lx.\n", hr);
        hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
        ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);

        if (IsEqualGUID(test_attributes[i].key, &MF_MT_FRAME_SIZE))
            check_mft_get_output_stream_info(transform, S_OK, &expect_output_info[1]);
        else
        {
            hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
            ok(hr == S_OK, "GetOutputStreamInfo returned %#lx\n", hr);
            check_member(output_info, expect_output_info[0], "%#lx", dwFlags);
            todo_wine
            check_member(output_info, expect_output_info[0], "%#lx", cbSize);
            check_member(output_info, expect_output_info[0], "%#lx", cbAlignment);
        }

        winetest_pop_context();
    }

    hr = IMFTransform_QueryInterface(transform, &IID_ICodecAPI, (void **)&codec_api);
    ok(hr == S_OK, "QueryInterface returned %#lx.\n", hr);
    for (desc = &expect_codec_api_attributes[0]; desc->key; ++desc)
    {
        PROPVARIANT propvar;
        VARIANT var;

        hr = ICodecAPI_GetValue(codec_api, desc->key, &var);
        todo_wine_if(desc->todo)
        ok(hr == S_OK, "%s is missing.\n", debugstr_a(desc->name));
        if (hr != S_OK)
            continue;
        hr = VariantToPropVariant(&var, &propvar);
        ok(hr == S_OK, "VariantToPropVariant returned %#lx.\n", hr);
        ret = PropVariantCompareEx(&propvar, &desc->value, 0, 0);
        todo_wine_if(desc->todo_value)
        ok(ret == 0, "%s mismatch, type %u, value %s.\n",
                debugstr_a(desc->name), propvar.vt, debugstr_propvariant(&propvar, desc->ratio));

        PropVariantClear(&propvar);
        VariantClear(&var);
    }
    ICodecAPI_Release(codec_api);

    check_mft_set_output_type(transform, output_type_desc, S_OK);
    check_mft_set_input_type(transform, input_type_desc, S_OK);

    /* Load input frame. */
    load_resource(L"nv12frame.bmp", &nv12frame_data, &nv12frame_data_size);
    /* Skip BMP header and RGB data from the dump. */
    size = *(DWORD *)(nv12frame_data + 2);
    nv12frame_data_size -= size;
    nv12frame_data += size;
    ok(nv12frame_data_size == 13824, "Got NV12 frame size %lu.\n", nv12frame_data_size);

    /* Process input samples. */
    for (i = 0; i < 16; ++i)
    {
        input_sample = create_sample(nv12frame_data, nv12frame_data_size);
        hr = IMFSample_SetSampleTime(input_sample, i * 333333);
        ok(hr == S_OK, "SetSampleTime returned %#lx.\n", hr);
        hr = IMFSample_SetSampleDuration(input_sample, 333333);
        ok(hr == S_OK, "SetSampleDuration returned %#lx.\n", hr);
        hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
        ok(hr == S_OK || hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx.\n", hr);
        ret = IMFSample_Release(input_sample);
        todo_wine
        ok(ret == 0, "Release returned %ld.\n", ret);
        if (hr != S_OK)
            break;
    }
    todo_wine
    ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx.\n", hr);
    ok(i >= 4, "Processed %ld input samples.\n", i);

    /* Check output sample. */
    hr = MFCreateCollection(&output_sample_collection);
    ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

    output_sample = create_sample(NULL, expect_output_info[0].cbSize);
    hr = check_mft_process_output(transform, output_sample, &output_status);
    todo_wine
    ok(hr == S_OK, "ProcessOutput returned %#lx.\n", hr);
    if (hr != S_OK)
    {
        IMFSample_Release(output_sample);
        goto failed;
    }
    hr = IMFCollection_AddElement(output_sample_collection, (IUnknown *)output_sample);
    ok(hr == S_OK, "AddElement returned %#lx.\n", hr);
    ret = IMFSample_Release(output_sample);
    ok(ret == 1, "Release returned %ld\n", ret);

    output_sample = create_sample(NULL, expect_output_info[0].cbSize);
    hr = check_mft_process_output(transform, output_sample, &output_status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx.\n", hr);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %ld\n", ret);

    ret = check_mf_sample_collection(output_sample_collection, &output_sample_desc, L"h264encdata.bin");
    ok(ret == 0, "Got %lu%% diff\n", ret);
    IMFCollection_Release(output_sample_collection);

    IMFMediaType_Release(media_type);
    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

failed:
    winetest_pop_context();
    CoUninitialize();
}

static void test_h264_decoder(void)
{
    const GUID *const class_id = &CLSID_MSH264DecoderMFT;
    const struct transform_info expect_mft_info =
    {
        .name = L"Microsoft H264 Video Decoder MFT",
        .major_type = &MFMediaType_Video,
        .inputs =
        {
            {.subtype = &MFVideoFormat_H264},
            {.subtype = &MFVideoFormat_H264_ES},
        },
        .outputs =
        {
            {.subtype = &MFVideoFormat_NV12},
            {.subtype = &MFVideoFormat_YV12},
            {.subtype = &MFVideoFormat_IYUV},
            {.subtype = &MFVideoFormat_I420},
            {.subtype = &MFVideoFormat_YUY2},
        },
    };
    static const media_type_desc default_inputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264_ES),
        },
    };
    static const struct attribute_desc expect_transform_attributes[] =
    {
        ATTR_UINT32(MF_LOW_LATENCY, 0),
        ATTR_UINT32(MF_SA_D3D_AWARE, 1),
        ATTR_UINT32(MF_SA_D3D11_AWARE, 1),
        ATTR_UINT32(MFT_DECODER_EXPOSE_OUTPUT_TYPES_IN_NATIVE_ORDER, 0),
        /* more H264 decoder specific attributes from CODECAPI */
        ATTR_UINT32(CODECAPI_AVDecVideoAcceleration_H264, 1),
        {0},
    };
    static const DWORD input_width = 120, input_height = 248;
    const media_type_desc default_outputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, input_width),
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_RATIO(MF_MT_FRAME_SIZE, input_width, input_height),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, input_width * input_height * 3 / 2),
            /* ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0), missing on Win7 */
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YV12),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, input_width),
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_RATIO(MF_MT_FRAME_SIZE, input_width, input_height),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, input_width * input_height * 3 / 2),
            /* ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0), missing on Win7 */
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_IYUV),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, input_width),
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_RATIO(MF_MT_FRAME_SIZE, input_width, input_height),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, input_width * input_height * 3 / 2),
            /* ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0), missing on Win7 */
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, input_width),
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_RATIO(MF_MT_FRAME_SIZE, input_width, input_height),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, input_width * input_height * 3 / 2),
            /* ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0), missing on Win7 */
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, input_width * 2),
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_RATIO(MF_MT_FRAME_SIZE, input_width, input_height),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, input_width * input_height * 2),
            /* ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0), missing on Win7 */
        },
    };
    const struct attribute_desc input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, input_width, input_height),
        {0},
    };
    const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, input_width, input_height, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_RATE, 60000, 1000),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 2, 1),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 3840),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, 3840 * input_height * 3 / 2),
        ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0),
        {0},
    };
    const struct attribute_desc expect_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264),
        ATTR_RATIO(MF_MT_FRAME_SIZE, input_width, input_height),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_AVG_BIT_ERROR_RATE, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_COMPRESSED, 1, .todo = TRUE),
        {0},
    };
    const struct attribute_desc expect_output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_RATIO(MF_MT_FRAME_SIZE, input_width, input_height),
        ATTR_RATIO(MF_MT_FRAME_RATE, 60000, 1000),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 2, 1),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 3840),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, 3840 * input_height * 3 / 2),
        ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0),
        ATTR_UINT32(MF_MT_AVG_BIT_ERROR_RATE, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_COMPRESSED, 0, .todo = TRUE),
        {0},
    };
    static const struct attribute_desc new_output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_RATIO(MF_MT_FRAME_RATE, 1, 1),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 2),
        {0},
    };
    static const struct attribute_desc expect_new_output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_RATIO(MF_MT_FRAME_RATE, 1, 1),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 2),
        ATTR_UINT32(MF_MT_COMPRESSED, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_AVG_BIT_ERROR_RATE, 0, .todo = TRUE),
        {0},
    };
    static const MFVideoArea actual_aperture = {.Area={82,84}};
    static const DWORD actual_width = 96, actual_height = 96;
    const media_type_desc actual_outputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 60000, 1000),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
            /* ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0), missing on Win7 */
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
            ATTR_BLOB(MF_MT_GEOMETRIC_APERTURE, &actual_aperture, 16),
            ATTR_BLOB(MF_MT_PAN_SCAN_APERTURE, &actual_aperture, 16),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YV12),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 60000, 1000),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
            /* ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0), missing on Win7 */
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
            ATTR_BLOB(MF_MT_GEOMETRIC_APERTURE, &actual_aperture, 16),
            ATTR_BLOB(MF_MT_PAN_SCAN_APERTURE, &actual_aperture, 16),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_IYUV),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 60000, 1000),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
            /* ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0), missing on Win7 */
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
            ATTR_BLOB(MF_MT_GEOMETRIC_APERTURE, &actual_aperture, 16),
            ATTR_BLOB(MF_MT_PAN_SCAN_APERTURE, &actual_aperture, 16),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 60000, 1000),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
            /* ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0), missing on Win7 */
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
            ATTR_BLOB(MF_MT_GEOMETRIC_APERTURE, &actual_aperture, 16),
            ATTR_BLOB(MF_MT_PAN_SCAN_APERTURE, &actual_aperture, 16),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_RATIO(MF_MT_FRAME_RATE, 60000, 1000),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 2),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 2),
            /* ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0), missing on Win7 */
            ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
            ATTR_BLOB(MF_MT_GEOMETRIC_APERTURE, &actual_aperture, 16),
            ATTR_BLOB(MF_MT_PAN_SCAN_APERTURE, &actual_aperture, 16),
        },
    };
    const MFT_OUTPUT_STREAM_INFO initial_output_info =
    {
        .dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER |
                MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE,
        .cbSize = 1920 * 1088 * 2,
    };
    const MFT_OUTPUT_STREAM_INFO output_info =
    {
        .dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER |
                MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE,
        .cbSize = input_width * input_height * 2,
    };
    const MFT_OUTPUT_STREAM_INFO actual_output_info =
    {
        .dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER |
                MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE,
        .cbSize = actual_width * actual_height * 2,
    };
    const MFT_INPUT_STREAM_INFO input_info =
    {
        .dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER |
                MFT_INPUT_STREAM_FIXED_SAMPLE_SIZE,
        .cbSize = 0x1000,
    };

    const struct attribute_desc output_sample_attributes[] =
    {
        ATTR_UINT32(MFSampleExtension_CleanPoint, 1),
        {0},
    };
    const struct buffer_desc output_buffer_desc_nv12 =
    {
        .length = actual_width * actual_height * 3 / 2,
        .compare = compare_nv12, .compare_rect = {.right = 82, .bottom = 84},
        .dump = dump_nv12, .size = {.cx = actual_width, .cy = actual_height},
    };
    const struct sample_desc output_sample_desc_nv12 =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 333667,
        .buffer_count = 1, .buffers = &output_buffer_desc_nv12,
    };
    const struct buffer_desc output_buffer_desc_i420 =
    {
        .length = actual_width * actual_height * 3 / 2,
        .compare = compare_i420, .compare_rect = {.right = 82, .bottom = 84},
        .dump = dump_i420, .size = {.cx = actual_width, .cy = actual_height},
    };
    const struct sample_desc expect_output_sample_i420 =
    {
        .attributes = output_sample_attributes,
        .sample_time = 333667, .sample_duration = 333667,
        .buffer_count = 1, .buffers = &output_buffer_desc_i420,
    };

    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Video, MFVideoFormat_H264};
    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Video, MFVideoFormat_NV12};
    IMFSample *input_sample, *output_sample;
    const BYTE *h264_encoded_data;
    IMFCollection *output_samples;
    ULONG h264_encoded_data_len;
    DWORD length, output_status;
    IMFAttributes *attributes;
    IMFMediaType *media_type;
    IMFTransform *transform;
    ULONG i, ret, ref;
    DWORD flags;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    winetest_push_context("h264dec");

    if (!check_mft_enum(MFT_CATEGORY_VIDEO_DECODER, &input_type, &output_type, class_id))
        goto failed;
    check_mft_get_info(class_id, &expect_mft_info);

    if (FAILED(hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_IMediaObject, FALSE);
    check_interface(transform, &IID_IPropertyStore, FALSE);
    check_interface(transform, &IID_IPropertyBag, FALSE);

    check_mft_optional_methods(transform, 1);
    check_mft_get_attributes(transform, expect_transform_attributes, TRUE);
    check_mft_get_input_stream_info(transform, S_OK, &input_info);
    check_mft_get_output_stream_info(transform, S_OK, &initial_output_info);

    hr = IMFTransform_GetAttributes(transform, &attributes);
    ok(hr == S_OK, "GetAttributes returned %#lx\n", hr);
    hr = IMFAttributes_SetUINT32(attributes, &MF_LOW_LATENCY, 1);
    ok(hr == S_OK, "SetUINT32 returned %#lx\n", hr);
    IMFAttributes_Release(attributes);

    /* no output type is available before an input type is set */

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetOutputAvailableType returned %#lx\n", hr);

    flags = 0xdeadbeef;
    hr = IMFTransform_GetInputStatus(transform, 0, &flags);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Got %#lx\n", hr);
    ok(flags == 0xdeadbeef, "Got flags %#lx.\n", flags);

    /* setting output media type first doesn't work */
    check_mft_set_output_type(transform, output_type_desc, MF_E_TRANSFORM_TYPE_NOT_SET);
    check_mft_get_output_current_type(transform, NULL);

    /* check available input types */

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetInputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("in %lu", i);
        ok(hr == S_OK, "GetInputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, default_inputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetInputAvailableType returned %#lx\n", hr);
    ok(i == 2 || broken(i == 1) /* Win7 */, "%lu input media types\n", i);

    check_mft_set_input_type_required(transform, input_type_desc);
    check_mft_set_input_type(transform, input_type_desc, S_OK);
    check_mft_get_input_current_type_(__LINE__, transform, expect_input_type_desc, FALSE, TRUE);

    check_mft_get_input_stream_info(transform, S_OK, &input_info);
    check_mft_get_output_stream_info(transform, S_OK, &output_info);

    /* output types can now be enumerated (though they are actually the same for all input types) */

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, default_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == 5, "%lu output media types\n", i);

    check_mft_set_output_type_required(transform, output_type_desc);
    check_mft_set_output_type(transform, output_type_desc, S_OK);
    check_mft_get_output_current_type_(__LINE__, transform, expect_output_type_desc, FALSE, TRUE);

    /* check that the output media type we've selected don't change the enumeration */

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, default_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == 5, "%lu output media types\n", i);

    check_mft_get_input_stream_info(transform, S_OK, &input_info);
    check_mft_get_output_stream_info(transform, S_OK, &output_info);

    load_resource(L"h264data.bin", &h264_encoded_data, &h264_encoded_data_len);

    /* As output_info.dwFlags doesn't have MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES
     * IMFTransform_ProcessOutput needs a sample or returns an error */

    hr = check_mft_process_output(transform, NULL, &output_status);
    ok(hr == E_INVALIDARG || hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);

    i = 0;
    input_sample = next_h264_sample(&h264_encoded_data, &h264_encoded_data_len);
    while (1)
    {
        output_sample = create_sample(NULL, output_info.cbSize);
        hr = check_mft_process_output(transform, output_sample, &output_status);
        if (hr != MF_E_TRANSFORM_NEED_MORE_INPUT) break;

        ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
        ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
        hr = IMFSample_GetTotalLength(output_sample, &length);
        ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
        ok(length == 0, "got length %lu\n", length);
        ret = IMFSample_Release(output_sample);
        ok(ret == 0, "Release returned %lu\n", ret);

        flags = 0;
        hr = IMFTransform_GetInputStatus(transform, 0, &flags);
        ok(hr == S_OK, "Got %#lx\n", hr);
        ok(flags == MFT_INPUT_STATUS_ACCEPT_DATA, "Got flags %#lx.\n", flags);
        hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
        ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
        ret = IMFSample_Release(input_sample);
        ok(ret <= 1, "Release returned %lu\n", ret);
        input_sample = next_h264_sample(&h264_encoded_data, &h264_encoded_data_len);

        flags = 0;
        hr = IMFTransform_GetInputStatus(transform, 0, &flags);
        ok(hr == S_OK, "Got %#lx\n", hr);
        ok(flags == MFT_INPUT_STATUS_ACCEPT_DATA, "Got flags %#lx.\n", flags);
        hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
        ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
        ret = IMFSample_Release(input_sample);
        ok(ret <= 1, "Release returned %lu\n", ret);
        input_sample = next_h264_sample(&h264_encoded_data, &h264_encoded_data_len);
        i++;

        hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, 0);
        ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);
        hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, 0);
        ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);
    }
    ok(i == 2, "got %lu iterations\n", i);
    ok(h264_encoded_data_len == 7619, "got h264_encoded_data_len %lu\n", h264_encoded_data_len);
    ok(hr == MF_E_TRANSFORM_STREAM_CHANGE, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE, "got output[0].dwStatus %#lx\n", output_status);
    hr = IMFSample_GetTotalLength(output_sample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    check_mft_get_input_stream_info(transform, S_OK, &input_info);
    check_mft_get_output_stream_info(transform, S_OK, &actual_output_info);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, actual_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == 5, "%lu output media types\n", i);

    /* current output type is still the one we selected */
    check_mft_get_output_current_type_(__LINE__, transform, expect_output_type_desc, FALSE, TRUE);

    hr = MFCreateCollection(&output_samples);
    ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

    output_sample = create_sample(NULL, output_info.cbSize);
    hr = check_mft_process_output(transform, output_sample, &output_status);
    ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
    hr = IMFCollection_AddElement(output_samples, (IUnknown *)output_sample);
    ok(hr == S_OK, "AddElement returned %#lx\n", hr);
    ref = IMFSample_Release(output_sample);
    ok(ref == 1, "Release returned %ld\n", ref);

    ret = check_mf_sample_collection(output_samples, &output_sample_desc_nv12, L"nv12frame.bmp");
    ok(ret == 0, "got %lu%% diff\n", ret);
    IMFCollection_Release(output_samples);

    /* we can change it, but only with the correct frame size */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "MFCreateMediaType returned %#lx\n", hr);
    init_media_type(media_type, output_type_desc, -1);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == MF_E_INVALIDMEDIATYPE, "SetOutputType returned %#lx.\n", hr);
    init_media_type(media_type, new_output_type_desc, -1);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    ret = IMFMediaType_Release(media_type);
    ok(ret == 1, "Release returned %lu\n", ret);

    check_mft_get_output_current_type_(__LINE__, transform, expect_new_output_type_desc, FALSE, TRUE);

    output_sample = create_sample(NULL, actual_width * actual_height * 2);
    hr = check_mft_process_output(transform, output_sample, &output_status);
    todo_wine
    ok(hr == MF_E_TRANSFORM_STREAM_CHANGE, "ProcessOutput returned %#lx\n", hr);
    todo_wine
    ok(output_status == MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE, "got output[0].dwStatus %#lx\n", output_status);

    while (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
    {
        ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
        hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
        ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
        ret = IMFSample_Release(input_sample);
        ok(ret <= 1, "Release returned %lu\n", ret);
        input_sample = next_h264_sample(&h264_encoded_data, &h264_encoded_data_len);
        hr = check_mft_process_output(transform, output_sample, &output_status);
    }

    ok(hr == MF_E_TRANSFORM_STREAM_CHANGE, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE, "got output[0].dwStatus %#lx\n", output_status);

    hr = IMFSample_GetTotalLength(output_sample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    hr = MFCreateCollection(&output_samples);
    ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

    output_sample = create_sample(NULL, actual_width * actual_height * 2);
    hr = check_mft_process_output(transform, output_sample, &output_status);
    ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
    hr = IMFCollection_AddElement(output_samples, (IUnknown *)output_sample);
    ok(hr == S_OK, "AddElement returned %#lx\n", hr);
    ref = IMFSample_Release(output_sample);
    ok(ref == 1, "Release returned %ld\n", ref);

    ret = check_mf_sample_collection(output_samples, &expect_output_sample_i420, L"i420frame.bmp");
    todo_wine /* wg_transform_set_output_format() should convert already processed samples instead of dropping */
    ok(ret == 0, "got %lu%% diff\n", ret);
    IMFCollection_Release(output_samples);

    output_sample = create_sample(NULL, actual_width * actual_height * 2);
    hr = check_mft_process_output(transform, output_sample, &output_status);
    todo_wine_if(hr == S_OK)  /* when VA-API plugin is used */
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    do
    {
        flags = 0;
        hr = IMFTransform_GetInputStatus(transform, 0, &flags);
        ok(hr == S_OK, "Got %#lx\n", hr);
        ok(flags == MFT_INPUT_STATUS_ACCEPT_DATA, "Got flags %#lx.\n", flags);
        hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
        ok(hr == S_OK || hr == MF_E_NOTACCEPTING, "Got %#lx\n", hr);
        input_sample = next_h264_sample(&h264_encoded_data, &h264_encoded_data_len);
    } while (hr == S_OK);

    ok(hr == MF_E_NOTACCEPTING, "Got %#lx\n", hr);
    flags = 0;
    hr = IMFTransform_GetInputStatus(transform, 0, &flags);
    ok(hr == S_OK, "Got %#lx\n", hr);
    ok(flags == MFT_INPUT_STATUS_ACCEPT_DATA, "Got flags %#lx.\n", flags);

    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);
    ret = IMFSample_Release(input_sample);
    ok(ret == 0, "Release returned %lu\n", ret);

failed:
    winetest_pop_context();
    CoUninitialize();
}

static void test_h264_decoder_timestamps(void)
{
    struct timestamps
    {
        LONGLONG time;
        LONGLONG duration;
        LONGLONG dts;
    };

    static const DWORD actual_width = 96, actual_height = 96;

    const struct attribute_desc input_type_nofps_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        {0},
    };
    const struct attribute_desc output_type_nofps_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        {0},
    };

    const struct attribute_desc input_type_24fps_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        ATTR_RATIO(MF_MT_FRAME_RATE, 24, 1),
        {0},
    };
    const struct attribute_desc output_type_24fps_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_RATE, 24, 1),
        {0},
    };
    static const struct timestamps exp_nofps[] =
    {
        {      0 , 333667 },
        {  333667, 333667 },
        {  667334, 333667 },
        { 1001001, 333667 },
        { 1334668, 333667 },
        { 1668335, 333667 },
        { 2002002, 333667 },
        { 2335669, 333667 },
        { 2669336, 333667 },
        { 3003003, 333667 },
    };
    static const struct timestamps exp_24fps[] =
    {
        {       0, 416667 },
        {  416667, 416667 },
        {  833334, 416667 },
        { 1250001, 416667 },
        { 1666668, 416667 },
        { 2083335, 416667 },
        { 2500002, 416667 },
        { 2916669, 416667 },
        { 3333336, 416667 },
        { 3750003, 416667 },
    };
    static const struct timestamps input_sample_ts[] =
    {
        { 1334666, 333667, 667333 },
        { 1334666, 333667, 667333 },
        { 1334666, 333667, 667333 },
        { 2669332, 333667, 1000999 },
        { 2001999, 333666, 1334666 },
        { 1668333, 333666 },
        { 2335665, 333667, 2001999 },
        { 4003999, 333667, 2335666 },
        { 3336666, 333666, 2669333 },
        { 3002999, 333667 },
        { 3670332, 333667, 3336666 },
        { 5338666, 333667, 3670333 },
        { 4671332, 333667, 4003999 },
        { 4337666, 333666 },
        { 5004999, 333667, 4671333 },
        { 6673332, 333667, 5004999 },
        { 6005999, 333666, 5338666 },
        { 5672333, 333666 },
        { 6339665, 333667, 6005999 },
        { 8007999, 333667, 6339666 },
        { 7340666, 333666, 6673333 },
        { 7006999, 333667 },
        { 7674332, 333667, 7340666 },
        { 9342666, 333667, 7674333 },
        { 8675332, 333667, 8007999 },
        { 8341666, 333666 },
        { 9008999, 333667, 8675333 },
        { 10677332, 333667, 9008999 },
        { 10009999, 333666, 9342666 },
        { 9676333, 333666 },
        { 10343665, 333667, 10009999 },
        { 12011999, 333667, 10343666 },
        { 11344666, 333666, 10677333 },
        { 11010999, 333667 },
        { 11678332, 333667, 11344666 },
        { 13346666, 333667, 11678333 },
    };
    static const struct timestamps exp_sample_ts[] =
    {
        { 1334666, 333667 },
        { 1668333, 333666 },
        { 2001999, 333666 },
        { 2335665, 333667 },
        { 2669332, 333667 },
        { 3002999, 333667 },
        { 3336666, 333666 },
        { 3670332, 333667 },
        { 4003999, 333667 },
        { 4337666, 333666 },
    };
    static const struct timestamps input_sample_neg_ts[] =
    {
        { -333667, 333667, -1001000 },
        { -333667, 333667, -1001000 },
        { -333667, 333667, -1001000 },
        { 1000999, 333667, -667334 },
        { 333666, 333666, -333667 },
        { 0, 333666 },
        { 667332, 333667, 333666 },
        { 2335666, 333667, 667333 },
        { 1668333, 333666, 1001000 },
        { 1334666, 333667 },
        { 2001999, 333667, 1668333 },
        { 3670333, 333667, 2002000 },
        { 3002999, 333667, 2335666 },
        { 2669333, 333666 },
        { 3336666, 333667, 3003000 },
        { 5004999, 333667, 3336666 },
        { 4337666, 333666, 3670333 },
        { 4004000, 333666 },
        { 4671332, 333667, 4337666 },
        { 6339666, 333667, 4671333 },
        { 5672333, 333666, 5005000 },
        { 5338666, 333667 },
        { 6005999, 333667, 5672333 },
        { 7674333, 333667, 6006000 },
        { 7006999, 333667, 6339666 },
        { 6673333, 333666 },
        { 7340666, 333667, 7007000 },
        { 9008999, 333667, 7340666 },
        { 8341666, 333666, 7674333 },
        { 8008000, 333666 },
        { 8675332, 333667, 8341666 },
        { 10343666, 333667, 8675333 },
        { 9676333, 333666, 9009000 },
        { 9342666, 333667 },
        { 10009999, 333667, 9676333 },
        { 11678333, 333667, 10010000 },
    };
    static const struct timestamps exp_sample_neg_ts[] =
    {
        { -333667, 333667 },
        {       0, 333666 },
        {  333666, 333666 },
        {  667332, 333667 },
        { 1000999, 333667 },
        { 1334666, 333667 },
        { 1668333, 333666 },
        { 2001999, 333667 },
        { 2335666, 333667 },
        { 2669333, 333666 },
    };

    IMFSample *input_sample, *output_sample;
    const BYTE *h264_encoded_data;
    ULONG h264_encoded_data_len;
    IMFTransform *transform;
    LONGLONG duration, time;
    DWORD output_status;
    ULONG i, j, ret;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    winetest_push_context("h264dec timestamps");

    /* Test when no framerate is provided and samples contain no timestamps */
    if (FAILED(hr = CoCreateInstance(&CLSID_MSH264DecoderMFT, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;

    load_resource(L"h264data.bin", &h264_encoded_data, &h264_encoded_data_len);

    check_mft_set_input_type(transform, input_type_nofps_desc, S_OK);
    check_mft_set_output_type(transform, output_type_nofps_desc, S_OK);

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    ok(hr == S_OK, "Got %#lx\n", hr);

    output_sample = create_sample(NULL, actual_width * actual_height * 2);
    for (i = 0; i < ARRAYSIZE(exp_nofps);)
    {
        winetest_push_context("output %ld", i);
        hr = check_mft_process_output(transform, output_sample, &output_status);
        ok(hr == S_OK || hr == MF_E_TRANSFORM_STREAM_CHANGE || hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
        if (hr == S_OK)
        {
            hr = IMFSample_GetSampleTime(output_sample, &time);
            ok(hr == S_OK, "Got %#lx\n", hr);
            ok(time == exp_nofps[i].time, "got time %I64d, expected %I64d\n", time, exp_nofps[i].time);
            hr = IMFSample_GetSampleDuration(output_sample, &duration);
            ok(hr == S_OK, "Got %#lx\n", hr);
            ok(duration == exp_nofps[i].duration, "got duration %I64d, expected %I64d\n", duration, exp_nofps[i].duration);
            ret = IMFSample_Release(output_sample);
            ok(ret == 0, "Release returned %lu\n", ret);
            output_sample = create_sample(NULL, actual_width * actual_height * 2);
            i++;
        }
        else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
        {
            input_sample = next_h264_sample(&h264_encoded_data, &h264_encoded_data_len);
            hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
            ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
            ret = IMFSample_Release(input_sample);
            ok(ret <= 1, "Release returned %lu\n", ret);
        }
        winetest_pop_context();
    }

    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

    /* Test when 24fps framerate is provided and samples contain no timestamps */
    hr = CoCreateInstance(&CLSID_MSH264DecoderMFT, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "Got %#lx\n", hr);

    load_resource(L"h264data.bin", &h264_encoded_data, &h264_encoded_data_len);

    check_mft_set_input_type(transform, input_type_24fps_desc, S_OK);
    check_mft_set_output_type(transform, output_type_24fps_desc, S_OK);

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    ok(hr == S_OK, "Got %#lx\n", hr);

    output_sample = create_sample(NULL, actual_width * actual_height * 2);
    for (i = 0; i < ARRAYSIZE(exp_24fps);)
    {
        winetest_push_context("output %ld", i);
        hr = check_mft_process_output(transform, output_sample, &output_status);
        ok(hr == S_OK || hr == MF_E_TRANSFORM_STREAM_CHANGE || hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
        if (hr == S_OK)
        {
            hr = IMFSample_GetSampleTime(output_sample, &time);
            ok(hr == S_OK, "Got %#lx\n", hr);
            ok(time == exp_24fps[i].time, "got time %I64d, expected %I64d\n", time, exp_24fps[i].time);
            hr = IMFSample_GetSampleDuration(output_sample, &duration);
            ok(hr == S_OK, "Got %#lx\n", hr);
            ok(duration == exp_24fps[i].duration, "got duration %I64d, expected %I64d\n", duration, exp_24fps[i].duration);
            ret = IMFSample_Release(output_sample);
            ok(ret == 0, "Release returned %lu\n", ret);
            output_sample = create_sample(NULL, actual_width * actual_height * 2);
            i++;
        }
        else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
        {
            input_sample = next_h264_sample(&h264_encoded_data, &h264_encoded_data_len);
            hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
            ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
            ret = IMFSample_Release(input_sample);
            ok(ret <= 1, "Release returned %lu\n", ret);
        }
        winetest_pop_context();
    }

    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

    /* Test when supplied sample timestamps disagree with MT_MF_FRAME_RATE */
    hr = CoCreateInstance(&CLSID_MSH264DecoderMFT, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "Got %#lx\n", hr);

    load_resource(L"h264data.bin", &h264_encoded_data, &h264_encoded_data_len);

    check_mft_set_input_type(transform, input_type_24fps_desc, S_OK);
    check_mft_set_output_type(transform, output_type_24fps_desc, S_OK);

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    ok(hr == S_OK, "Got %#lx\n", hr);

    j = 0;
    output_sample = create_sample(NULL, actual_width * actual_height * 2);
    for (i = 0; i < ARRAYSIZE(exp_sample_ts);)
    {
        winetest_push_context("output %ld", i);
        hr = check_mft_process_output(transform, output_sample, &output_status);
        ok(hr == S_OK || hr == MF_E_TRANSFORM_STREAM_CHANGE || hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
        if (hr == S_OK)
        {
            hr = IMFSample_GetSampleTime(output_sample, &time);
            ok(hr == S_OK, "Got %#lx\n", hr);
            ok(time == exp_sample_ts[i].time, "got time %I64d, expected %I64d\n", time, exp_sample_ts[i].time);
            hr = IMFSample_GetSampleDuration(output_sample, &duration);
            ok(hr == S_OK, "Got %#lx\n", hr);
            ok(duration == exp_sample_ts[i].duration, "got duration %I64d, expected %I64d\n", duration, exp_sample_ts[i].duration);
            ret = IMFSample_Release(output_sample);
            ok(ret == 0, "Release returned %lu\n", ret);
            output_sample = create_sample(NULL, actual_width * actual_height * 2);
            i++;
        }
        else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT && j < ARRAYSIZE(input_sample_ts))
        {
            input_sample = next_h264_sample(&h264_encoded_data, &h264_encoded_data_len);
            hr = IMFSample_SetSampleTime(input_sample, input_sample_ts[j].time);
            ok(hr == S_OK, "Got %#lx\n", hr);
            hr = IMFSample_SetSampleDuration(input_sample, input_sample_ts[j].duration);
            ok(hr == S_OK, "Got %#lx\n", hr);
            if (input_sample_ts[j].dts)
            {
                hr = IMFSample_SetUINT64(input_sample, &MFSampleExtension_DecodeTimestamp, input_sample_ts[j].dts);
                ok(hr == S_OK, "Got %#lx\n", hr);
            }
            j++;
            hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
            ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
            ret = IMFSample_Release(input_sample);
            ok(ret <= 1, "Release returned %lu\n", ret);
        }
        else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT && j == ARRAYSIZE(input_sample_ts))
        {
            ok(0, "no more input to provide\n");
            i = ARRAYSIZE(exp_sample_ts);
        }
        winetest_pop_context();
    }

    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

    /* Test when supplied sample timestamps include negative values */
    hr = CoCreateInstance(&CLSID_MSH264DecoderMFT, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "Got %#lx\n", hr);

    load_resource(L"h264data.bin", &h264_encoded_data, &h264_encoded_data_len);

    check_mft_set_input_type(transform, input_type_24fps_desc, S_OK);
    check_mft_set_output_type(transform, output_type_24fps_desc, S_OK);

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    ok(hr == S_OK, "Got %#lx\n", hr);

    j = 0;
    output_sample = create_sample(NULL, actual_width * actual_height * 2);
    for (i = 0; i < ARRAYSIZE(exp_sample_neg_ts);)
    {
        winetest_push_context("output %ld", i);
        hr = check_mft_process_output(transform, output_sample, &output_status);
        ok(hr == S_OK || hr == MF_E_TRANSFORM_STREAM_CHANGE || hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
        if (hr == S_OK)
        {
            hr = IMFSample_GetSampleTime(output_sample, &time);
            ok(hr == S_OK, "Got %#lx\n", hr);
            ok(time == exp_sample_neg_ts[i].time, "got time %I64d, expected %I64d\n", time, exp_sample_neg_ts[i].time);
            hr = IMFSample_GetSampleDuration(output_sample, &duration);
            ok(hr == S_OK, "Got %#lx\n", hr);
            ok(duration == exp_sample_neg_ts[i].duration, "got duration %I64d, expected %I64d\n", duration, exp_sample_neg_ts[i].duration);
            ret = IMFSample_Release(output_sample);
            ok(ret == 0, "Release returned %lu\n", ret);
            output_sample = create_sample(NULL, actual_width * actual_height * 2);
            i++;
        }
        else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT && j < ARRAYSIZE(input_sample_neg_ts))
        {
            input_sample = next_h264_sample(&h264_encoded_data, &h264_encoded_data_len);
            hr = IMFSample_SetSampleTime(input_sample, input_sample_neg_ts[j].time);
            ok(hr == S_OK, "Got %#lx\n", hr);
            hr = IMFSample_SetSampleDuration(input_sample, input_sample_neg_ts[j].duration);
            ok(hr == S_OK, "Got %#lx\n", hr);
            if (input_sample_neg_ts[j].dts)
            {
                hr = IMFSample_SetUINT64(input_sample, &MFSampleExtension_DecodeTimestamp, input_sample_neg_ts[j].dts);
                ok(hr == S_OK, "Got %#lx\n", hr);
            }
            j++;
            hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
            ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
            ret = IMFSample_Release(input_sample);
            ok(ret <= 1, "Release returned %lu\n", ret);
        }
        else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT && j == ARRAYSIZE(input_sample_neg_ts))
        {
            ok(0, "no more input to provide\n");
            i = ARRAYSIZE(exp_sample_neg_ts);
        }
        winetest_pop_context();
    }

    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

failed:
    winetest_pop_context();
    CoUninitialize();
}

static void test_h264_decoder_concat_streams(void)
{
    const struct buffer_desc output_buffer_desc[] =
    {
        {.length = 0x3600},
        {.length = 0x4980},
    };
    const struct attribute_desc output_sample_attributes[] =
    {
        ATTR_UINT32(MFSampleExtension_CleanPoint, 1),
        {0},
    };
    const struct sample_desc output_sample_desc[] =
    {
        {
            .attributes = output_sample_attributes + 0,
            .sample_time = 0, .sample_duration = 400000,
            .buffer_count = 1, .buffers = output_buffer_desc + 0, .repeat_count = 29,
        },
        {
            .attributes = output_sample_attributes + 0,
            .sample_time = 12000000, .sample_duration = 400000,
            .buffer_count = 1, .buffers = output_buffer_desc + 1, .repeat_count = 29,
        },
        {
            .attributes = output_sample_attributes + 0,
            .sample_time = 0, .sample_duration = 400000,
            .buffer_count = 1, .buffers = output_buffer_desc + 0, .repeat_count = 29,
            .todo_time = TRUE,
        },
        {
            .attributes = output_sample_attributes + 0,
            .sample_time = 12000000, .sample_duration = 400000,
            .buffer_count = 1, .buffers = output_buffer_desc + 1, .repeat_count = 6,
            .todo_time = TRUE,
        },
        {0},
    };
    const WCHAR *filenames[] =
    {
        L"h264data-1.bin",
        L"h264data-2.bin",
        L"h264data-1.bin",
        L"h264data-2.bin",
        NULL,
    };
    DWORD output_status, input_count, output_count;
    const WCHAR **filename = filenames;
    const BYTE *h264_encoded_data;
    IMFCollection *output_samples;
    ULONG h264_encoded_data_len;
    IMFAttributes *attributes;
    IMFMediaType *media_type;
    IMFTransform *transform;
    IMFSample *input_sample;
    HRESULT hr;
    LONG ret;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    if (FAILED(hr = CoCreateInstance(&CLSID_MSH264DecoderMFT, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;
    ok(hr == S_OK, "hr %#lx\n", hr);

    hr = IMFTransform_GetAttributes(transform, &attributes);
    ok(hr == S_OK, "GetAttributes returned %#lx\n", hr);
    hr = IMFAttributes_SetUINT32(attributes, &MF_LOW_LATENCY, 1);
    ok(hr == S_OK, "SetUINT32 returned %#lx\n", hr);
    IMFAttributes_Release(attributes);

    hr = IMFTransform_GetInputAvailableType(transform, 0, 0, &media_type);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_RATE, (UINT64)25000 << 32 | 1000);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFMediaType_Release(media_type);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_RATE, (UINT64)50000 << 32 | 1000);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFMediaType_Release(media_type);

    load_resource(*filename, &h264_encoded_data, &h264_encoded_data_len);

    hr = MFCreateCollection(&output_samples);
    ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

    input_count = 0;
    input_sample = next_h264_sample(&h264_encoded_data, &h264_encoded_data_len);
    while (input_sample)
    {
        MFT_OUTPUT_STREAM_INFO info = {0};

        hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
        ok(hr == S_OK || hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);

        if (hr == S_OK && input_count < 2)
        {
            MFT_OUTPUT_DATA_BUFFER data = {.pSample = create_sample(NULL, 1)};
            hr = IMFTransform_ProcessOutput(transform, 0, 1, &data, &output_status);
            ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
            IMFSample_Release(data.pSample);
            hr = S_OK;
        }

        if (hr == S_OK)
        {
            IMFSample_Release(input_sample);

            hr = IMFCollection_GetElementCount(output_samples, &output_count);
            ok(hr == S_OK, "GetElementCount returned %#lx\n", hr);

            if (output_count == 96)
            {
                hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_FLUSH, 0);
                ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);
            }

            if (h264_encoded_data_len <= 4 && *++filename)
            {
                load_resource(*filename, &h264_encoded_data, &h264_encoded_data_len);

                if (!wcscmp(*filename, L"h264data-1.bin"))
                {
                    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, 0);
                    ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);
                }
            }

            if (h264_encoded_data_len > 4)
            {
                input_count++;
                input_sample = next_h264_sample(&h264_encoded_data, &h264_encoded_data_len);
            }
            else
            {
                hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, 0);
                ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);
                input_sample = NULL;
            }
        }

        hr = IMFTransform_GetOutputStreamInfo(transform, 0, &info);
        ok(hr == S_OK, "GetOutputStreamInfo returned %#lx\n", hr);

        while (hr != MF_E_TRANSFORM_NEED_MORE_INPUT)
        {
            MFT_OUTPUT_DATA_BUFFER data = {.pSample = create_sample(NULL, info.cbSize)};

            hr = IMFTransform_ProcessOutput(transform, 0, 1, &data, &output_status);
            todo_wine_if(hr == 0xd0000001)
            ok(hr == S_OK || hr == MF_E_TRANSFORM_NEED_MORE_INPUT || hr == MF_E_TRANSFORM_STREAM_CHANGE,
               "ProcessOutput returned %#lx\n", hr);

            if (hr == S_OK)
            {
                hr = IMFCollection_AddElement(output_samples, (IUnknown *)data.pSample);
                ok(hr == S_OK, "AddElement returned %#lx\n", hr);
            }
            if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
            {
                hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
                ok(hr == S_OK, "got %#lx\n", hr);
                hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_RATE, (UINT64)50000 << 32 | 1000);
                ok(hr == S_OK, "got %#lx\n", hr);
                hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
                ok(hr == S_OK, "got %#lx\n", hr);
                IMFMediaType_Release(media_type);
                hr = IMFTransform_GetOutputStreamInfo(transform, 0, &info);
                ok(hr == S_OK, "GetOutputStreamInfo returned %#lx\n", hr);
            }

            IMFSample_Release(data.pSample);
        }
    }

    hr = IMFCollection_GetElementCount(output_samples, &output_count);
    ok(hr == S_OK, "GetElementCount returned %#lx\n", hr);
    ok(output_count == 96 || broken(output_count == 120) /* Win7 sometimes */, "GetElementCount returned %lu\n", output_count);
    while (broken(output_count > 96)) /* Win7 sometimes */
    {
        IMFSample *sample;
        hr = IMFCollection_RemoveElement(output_samples, --output_count, (IUnknown **)&sample);
        ok(hr == S_OK, "GetElementCount returned %#lx\n", hr);
        IMFSample_Release(sample);
    }

    ret = check_mf_sample_collection(output_samples, output_sample_desc, NULL);
    ok(ret == 0, "got %lu%% diff\n", ret);

    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

failed:
    CoUninitialize();
}

static void test_audio_convert(void)
{
    const GUID *const class_id = &CLSID_CResamplerMediaObject;
    const struct transform_info expect_mft_info =
    {
        .name = L"Resampler MFT",
        .major_type = &MFMediaType_Audio,
        .inputs =
        {
            {.subtype = &MFAudioFormat_PCM},
            {.subtype = &MFAudioFormat_Float},
        },
        .outputs =
        {
            {.subtype = &MFAudioFormat_PCM},
            {.subtype = &MFAudioFormat_Float},
        },
    };
    const struct transform_info expect_dmo_info =
    {
        .name = L"Resampler DMO",
        .major_type = &MEDIATYPE_Audio,
        .inputs =
        {
            {.subtype = &MEDIASUBTYPE_PCM},
            {.subtype = &MEDIASUBTYPE_IEEE_FLOAT},
        },
        .outputs =
        {
            {.subtype = &MEDIASUBTYPE_PCM},
            {.subtype = &MEDIASUBTYPE_IEEE_FLOAT},
        },
    };

    static const media_type_desc expect_available_inputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
    };
    static const media_type_desc expect_available_outputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
            ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
            ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000),
            ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 384000),
            ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 8),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
            ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
            ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000),
            ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 192000),
            ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        },
    };

    static const struct attribute_desc input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 2 * (32 / 8), .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 2 * (32 / 8) * 22050, .required = TRUE),
        {0},
    };
    const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 2 * (16 / 8), .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 2 * (16 / 8) * 44100, .required = TRUE),
        {0},
    };
    static const struct attribute_desc expect_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 8),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 22050 * 8),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1, .todo = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_CHANNEL_MASK, 3, .todo = TRUE),
        {0},
    };
    const struct attribute_desc expect_output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100 * 4),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1, .todo = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1, .todo = TRUE),
        {0},
    };
    const MFT_OUTPUT_STREAM_INFO output_info =
    {
        .cbSize = 4,
        .cbAlignment = 1,
    };
    const MFT_INPUT_STREAM_INFO input_info =
    {
        .cbSize = 8,
        .cbAlignment = 1,
    };

    static const ULONG audioconv_block_size = 0x4000;
    const struct buffer_desc output_buffer_desc[] =
    {
        {.length = audioconv_block_size, .compare = compare_pcm16},
        {.length = 0x3dd8, .compare = compare_pcm16, .todo_length = TRUE},
        {.length = 0xfc, .compare = compare_pcm16},
    };
    const struct attribute_desc output_sample_attributes[] =
    {
        ATTR_UINT32(mft_output_sample_incomplete, 1),
        ATTR_UINT32(MFSampleExtension_CleanPoint, has_video_processor /* 0 on Win7 */, .todo = TRUE),
        {0},
    };
    const struct sample_desc output_sample_desc[] =
    {
        {
            .attributes = output_sample_attributes + 0,
            .sample_time = 0, .sample_duration = 928798,
            .buffer_count = 1, .buffers = output_buffer_desc + 0, .repeat_count = 9,
        },
        {
            .attributes = output_sample_attributes + 1, /* not MFT_OUTPUT_DATA_BUFFER_INCOMPLETE */
            .sample_time = 9287980, .sample_duration = 897506, .todo_duration = 897280,
            .buffer_count = 1, .buffers = output_buffer_desc + 1, .todo_length = TRUE,
        },
        {
            .attributes = output_sample_attributes + 0,
            .sample_time = 10185486, .sample_duration = 14286,
            .buffer_count = 1, .buffers = output_buffer_desc + 2,
        },
    };

    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Audio, MFAudioFormat_PCM};
    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Audio, MFAudioFormat_Float};
    IMFSample *input_sample, *output_sample;
    IMFCollection *output_samples;
    DWORD length, output_status;
    IMFMediaType *media_type;
    IMFTransform *transform;
    const BYTE *audio_data;
    ULONG audio_data_len;
    ULONG i, ret, ref;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    winetest_push_context("resampler");

    if (!check_mft_enum(MFT_CATEGORY_AUDIO_EFFECT, &input_type, &output_type, class_id))
        goto failed;
    check_mft_get_info(class_id, &expect_mft_info);
    check_dmo_get_info(class_id, &expect_dmo_info);

    if (FAILED(hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_IMediaObject, TRUE);
    check_interface(transform, &IID_IPropertyStore, TRUE);
    check_interface(transform, &IID_IPropertyBag, TRUE);
    /* check_interface(transform, &IID_IWMResamplerProps, TRUE); */

    check_mft_optional_methods(transform, 1);
    check_mft_get_attributes(transform, NULL, FALSE);
    check_mft_get_input_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);
    check_mft_get_output_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == 4, "%lu output media types\n", i);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetInputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("in %lu", i);
        ok(hr == S_OK, "GetInputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_inputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetInputAvailableType returned %#lx\n", hr);
    ok(i == 2, "%lu input media types\n", i);

    /* setting output media type first doesn't work */
    check_mft_set_output_type(transform, output_type_desc, MF_E_TRANSFORM_TYPE_NOT_SET);
    check_mft_get_output_current_type(transform, NULL);

    check_mft_set_input_type_required(transform, input_type_desc);
    check_mft_set_input_type(transform, input_type_desc, S_OK);
    check_mft_get_input_current_type_(__LINE__, transform, expect_input_type_desc, FALSE, TRUE);

    check_mft_get_input_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);
    check_mft_get_output_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);

    /* check new output media types */

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == 4, "%lu output media types\n", i);

    check_mft_set_output_type_required(transform, output_type_desc);
    check_mft_set_output_type(transform, output_type_desc, S_OK);
    check_mft_get_output_current_type_(__LINE__, transform, expect_output_type_desc, FALSE, TRUE);

    check_mft_get_input_stream_info(transform, S_OK, &input_info);
    check_mft_get_output_stream_info(transform, S_OK, &output_info);

    load_resource(L"audiodata.bin", &audio_data, &audio_data_len);
    ok(audio_data_len == 179928, "got length %lu\n", audio_data_len);

    input_sample = create_sample(audio_data, audio_data_len);
    hr = IMFSample_SetSampleTime(input_sample, 0);
    ok(hr == S_OK, "SetSampleTime returned %#lx\n", hr);
    hr = IMFSample_SetSampleDuration(input_sample, 10000000);
    ok(hr == S_OK, "SetSampleDuration returned %#lx\n", hr);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, 0);
    ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);
    ret = IMFSample_Release(input_sample);
    ok(ret <= 1, "Release returned %ld\n", ret);

    hr = MFCreateCollection(&output_samples);
    ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

    output_sample = create_sample(NULL, audioconv_block_size);
    for (i = 0; SUCCEEDED(hr = check_mft_process_output(transform, output_sample, &output_status)); i++)
    {
        winetest_push_context("%lu", i);
        ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
        ok(!(output_status & ~MFT_OUTPUT_DATA_BUFFER_INCOMPLETE), "got output[0].dwStatus %#lx\n", output_status);
        hr = IMFCollection_AddElement(output_samples, (IUnknown *)output_sample);
        ok(hr == S_OK, "AddElement returned %#lx\n", hr);
        ref = IMFSample_Release(output_sample);
        ok(ref == 1, "Release returned %ld\n", ref);
        output_sample = create_sample(NULL, audioconv_block_size);
        winetest_pop_context();
    }
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    todo_wine
    ok(i == 12 || broken(i == 11) /* Win7 */, "got %lu output samples\n", i);

    ret = check_mf_sample_collection(output_samples, output_sample_desc, L"audioconvdata.bin");
    ok(ret == 0, "got %lu%% diff\n", ret);
    IMFCollection_Release(output_samples);

    output_sample = create_sample(NULL, audioconv_block_size);
    hr = check_mft_process_output(transform, output_sample, &output_status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
    hr = IMFSample_GetTotalLength(output_sample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

failed:
    winetest_pop_context();
    CoUninitialize();
}

const GUID *wmv_decoder_output_subtypes[] =
{
    &MEDIASUBTYPE_NV12,
    &MEDIASUBTYPE_YV12,
    &MEDIASUBTYPE_IYUV,
    &MEDIASUBTYPE_I420,
    &MEDIASUBTYPE_YUY2,
    &MEDIASUBTYPE_UYVY,
    &MEDIASUBTYPE_YVYU,
    &MEDIASUBTYPE_NV11,
    &MEDIASUBTYPE_RGB32,
    &MEDIASUBTYPE_RGB24,
    &MEDIASUBTYPE_RGB565,
    &MEDIASUBTYPE_RGB555,
    &MEDIASUBTYPE_RGB8,
};

static void test_wmv_encoder(void)
{
    const GUID *const class_id = &CLSID_CWMVXEncMediaObject;
    const struct transform_info expect_mft_info =
    {
        .name = L"WMVideo8 Encoder MFT",
        .major_type = &MFMediaType_Video,
        .inputs =
        {
            {.subtype = &MFVideoFormat_IYUV},
            {.subtype = &MFVideoFormat_I420},
            {.subtype = &MFVideoFormat_YV12},
            {.subtype = &MFVideoFormat_NV11},
            {.subtype = &MFVideoFormat_NV12},
            {.subtype = &MFVideoFormat_YUY2},
            {.subtype = &MFVideoFormat_UYVY},
            {.subtype = &MFVideoFormat_YVYU},
            {.subtype = &MFVideoFormat_YVU9},
            {.subtype = &DMOVideoFormat_RGB32},
            {.subtype = &DMOVideoFormat_RGB24},
            {.subtype = &DMOVideoFormat_RGB565},
            {.subtype = &DMOVideoFormat_RGB555},
            {.subtype = &DMOVideoFormat_RGB8},
        },
        .outputs =
        {
            {.subtype = &MFVideoFormat_WMV1},
            {.subtype = &MFVideoFormat_WMV2},
        },
    };
    const struct transform_info expect_dmo_info =
    {
        .name = L"WMVideo8 Encoder DMO",
        .major_type = &MEDIATYPE_Video,
        .inputs =
        {
            {.subtype = &MEDIASUBTYPE_IYUV},
            {.subtype = &MEDIASUBTYPE_I420},
            {.subtype = &MEDIASUBTYPE_YV12},
            {.subtype = &MEDIASUBTYPE_NV11},
            {.subtype = &MEDIASUBTYPE_NV12},
            {.subtype = &MEDIASUBTYPE_YUY2},
            {.subtype = &MEDIASUBTYPE_UYVY},
            {.subtype = &MEDIASUBTYPE_YVYU},
            {.subtype = &MEDIASUBTYPE_YVU9},
            {.subtype = &MEDIASUBTYPE_RGB32},
            {.subtype = &MEDIASUBTYPE_RGB24},
            {.subtype = &MEDIASUBTYPE_RGB565},
            {.subtype = &MEDIASUBTYPE_RGB555},
            {.subtype = &MEDIASUBTYPE_RGB8},
        },
        .outputs =
        {
            {.subtype = &MEDIASUBTYPE_WMV1},
            {.subtype = &MEDIASUBTYPE_WMV2},
        },
    };

    static const media_type_desc expect_available_inputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_IYUV),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_TRANSFER_FUNCTION, 0),
            ATTR_UINT32(MF_MT_VIDEO_PRIMARIES, 0),
            ATTR_UINT32(MF_MT_YUV_MATRIX, 0),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_TRANSFER_FUNCTION, 0),
            ATTR_UINT32(MF_MT_VIDEO_PRIMARIES, 0),
            ATTR_UINT32(MF_MT_YUV_MATRIX, 0),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YV12),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_TRANSFER_FUNCTION, 0),
            ATTR_UINT32(MF_MT_VIDEO_PRIMARIES, 0),
            ATTR_UINT32(MF_MT_YUV_MATRIX, 0),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV11),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_TRANSFER_FUNCTION, 0),
            ATTR_UINT32(MF_MT_VIDEO_PRIMARIES, 0),
            ATTR_UINT32(MF_MT_YUV_MATRIX, 0),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_TRANSFER_FUNCTION, 0),
            ATTR_UINT32(MF_MT_VIDEO_PRIMARIES, 0),
            ATTR_UINT32(MF_MT_YUV_MATRIX, 0),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_TRANSFER_FUNCTION, 0),
            ATTR_UINT32(MF_MT_VIDEO_PRIMARIES, 0),
            ATTR_UINT32(MF_MT_YUV_MATRIX, 0),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_UYVY),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_TRANSFER_FUNCTION, 0),
            ATTR_UINT32(MF_MT_VIDEO_PRIMARIES, 0),
            ATTR_UINT32(MF_MT_YUV_MATRIX, 0),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YVYU),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_TRANSFER_FUNCTION, 0),
            ATTR_UINT32(MF_MT_VIDEO_PRIMARIES, 0),
            ATTR_UINT32(MF_MT_YUV_MATRIX, 0),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_TRANSFER_FUNCTION, 0),
            ATTR_UINT32(MF_MT_VIDEO_PRIMARIES, 0),
            ATTR_UINT32(MF_MT_YUV_MATRIX, 0),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_TRANSFER_FUNCTION, 0),
            ATTR_UINT32(MF_MT_VIDEO_PRIMARIES, 0),
            ATTR_UINT32(MF_MT_YUV_MATRIX, 0),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB565),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_TRANSFER_FUNCTION, 0),
            ATTR_UINT32(MF_MT_VIDEO_PRIMARIES, 0),
            ATTR_UINT32(MF_MT_YUV_MATRIX, 0),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB555),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_TRANSFER_FUNCTION, 0),
            ATTR_UINT32(MF_MT_VIDEO_PRIMARIES, 0),
            ATTR_UINT32(MF_MT_YUV_MATRIX, 0),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_RGB8),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
            ATTR_UINT32(MF_MT_TRANSFER_FUNCTION, 0),
            ATTR_UINT32(MF_MT_VIDEO_PRIMARIES, 0),
            ATTR_UINT32(MF_MT_YUV_MATRIX, 0),
        },
    };
    static const media_type_desc expect_available_outputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_WMV1),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_UINT32(MF_MT_TRANSFER_FUNCTION, 0),
            ATTR_UINT32(MF_MT_VIDEO_PRIMARIES, 0),
            ATTR_UINT32(MF_MT_YUV_MATRIX, 0),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_WMV2),
            ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
            ATTR_UINT32(MF_MT_TRANSFER_FUNCTION, 0),
            ATTR_UINT32(MF_MT_VIDEO_PRIMARIES, 0),
            ATTR_UINT32(MF_MT_YUV_MATRIX, 0),
        },
    };

    static const DWORD actual_width = 96, actual_height = 96;
    const struct attribute_desc input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height), /* required for SetOutputType */
        {0},
    };
    const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_WMV1, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001, .required = TRUE),
        ATTR_UINT32(MF_MT_AVG_BITRATE, 193540, .required = TRUE),
        {0},
    };

    const struct attribute_desc expect_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        {0},
    };
    const struct attribute_desc expect_output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_WMV1),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        ATTR_UINT32(MF_MT_AVG_BITRATE, 193540),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
        {0},
    };
    static const MFT_OUTPUT_STREAM_INFO empty_output_info =
    {
        .dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER,
    };
    static const MFT_INPUT_STREAM_INFO empty_input_info =
    {
        .dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER,
    };
    static const MFT_OUTPUT_STREAM_INFO expect_output_info =
    {
        .dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER,
        .cbSize = 0x8000,
        .cbAlignment = 1,
    };
    static const MFT_INPUT_STREAM_INFO expect_input_info =
    {
        .dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER,
        .cbSize = 0x3600,
        .cbAlignment = 1,
    };

    const struct buffer_desc output_buffer_desc[] =
    {
        {.length = -1 /* variable */},
    };
    const struct attribute_desc output_sample_attributes_key[] =
    {
        ATTR_UINT32(MFSampleExtension_CleanPoint, 1),
        {0},
    };
    const struct attribute_desc output_sample_attributes[] =
    {
        ATTR_UINT32(MFSampleExtension_CleanPoint, 0),
        {0},
    };
    const struct sample_desc output_sample_desc[] =
    {
        {
            .attributes = output_sample_attributes_key,
            .sample_time = 0, .sample_duration = 333333,
            .buffer_count = 1, .buffers = output_buffer_desc,
        },
        {
            .attributes = output_sample_attributes,
            .sample_time = 333333, .sample_duration = 333333,
            .buffer_count = 1, .buffers = output_buffer_desc, .repeat_count = 4
        },
    };

    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Video, MFVideoFormat_WMV1};
    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Video, MFVideoFormat_NV12};
    IMFSample *input_sample, *output_sample;
    DWORD status, length, output_status;
    MFT_OUTPUT_DATA_BUFFER output;
    IMFCollection *output_samples;
    const BYTE *nv12frame_data;
    IMFMediaType *media_type;
    ULONG nv12frame_data_len;
    IMFTransform *transform;
    ULONG i, ret;
    HRESULT hr;
    LONG ref;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    winetest_push_context("wmvenc");

    if (!check_mft_enum(MFT_CATEGORY_VIDEO_ENCODER, &input_type, &output_type, class_id))
        goto failed;
    check_mft_get_info(class_id, &expect_mft_info);
    check_dmo_get_info(class_id, &expect_dmo_info);

    if (FAILED(hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_IMediaObject, TRUE);
    check_interface(transform, &IID_IPropertyStore, TRUE);
    check_interface(transform, &IID_IPropertyBag, TRUE);

    check_mft_optional_methods(transform, 2);
    check_mft_get_attributes(transform, NULL, FALSE);
    check_mft_get_input_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, &empty_input_info);
    check_mft_get_output_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, &empty_output_info);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetInputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("in 0 %lu", i);
        ok(hr == S_OK, "GetInputAvailableType returned %#lx\n", hr);
        ret = IMFMediaType_Release(media_type);
        ok(ret <= 1, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetInputAvailableType returned %#lx\n", hr);
    ok(i == ARRAY_SIZE(expect_available_inputs), "%lu input media types\n", i);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        ret = IMFMediaType_Release(media_type);
        ok(ret <= 1, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == ARRAY_SIZE(expect_available_outputs), "%lu output media types\n", i);

    check_mft_set_output_type(transform, output_type_desc, MF_E_TRANSFORM_TYPE_NOT_SET);
    check_mft_get_output_current_type(transform, NULL);

    check_mft_set_input_type_required(transform, input_type_desc);
    check_mft_set_input_type(transform, input_type_desc, S_OK);
    check_mft_get_input_current_type_(__LINE__, transform, expect_input_type_desc, FALSE, TRUE);

    check_mft_set_output_type_required(transform, output_type_desc);
    check_mft_set_output_type(transform, output_type_desc, S_OK);
    check_mft_get_output_current_type_(__LINE__, transform, expect_output_type_desc, FALSE, FALSE);

    check_mft_get_input_stream_info(transform, S_OK, &expect_input_info);
    check_mft_get_output_stream_info(transform, S_OK, &expect_output_info);

    if (!has_video_processor)
    {
        win_skip("Skipping WMV encoder tests on Win7\n");
        goto done;
    }

    load_resource(L"nv12frame.bmp", &nv12frame_data, &nv12frame_data_len);
    /* skip BMP header and RGB data from the dump */
    length = *(DWORD *)(nv12frame_data + 2);
    nv12frame_data_len = nv12frame_data_len - length;
    nv12frame_data = nv12frame_data + length;
    ok(nv12frame_data_len == 13824, "got length %lu\n", nv12frame_data_len);

    hr = MFCreateCollection(&output_samples);
    ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

    for (i = 0; i < 5; i++)
    {
        input_sample = create_sample(nv12frame_data, nv12frame_data_len);
        hr = IMFSample_SetSampleTime(input_sample, i * 333333);
        ok(hr == S_OK, "SetSampleTime returned %#lx\n", hr);
        hr = IMFSample_SetSampleDuration(input_sample, 333333);
        ok(hr == S_OK, "SetSampleDuration returned %#lx\n", hr);
        hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
        ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
        ref = IMFSample_Release(input_sample);
        ok(ref <= 1, "Release returned %ld\n", ref);

        output_sample = create_sample(NULL, expect_output_info.cbSize);
        hr = check_mft_process_output(transform, output_sample, &output_status);
        ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
        hr = IMFCollection_AddElement(output_samples, (IUnknown *)output_sample);
        ok(hr == S_OK, "AddElement returned %#lx\n", hr);
        ref = IMFSample_Release(output_sample);
        ok(ref == 1, "Release returned %ld\n", ref);
    }

    ret = check_mf_sample_collection(output_samples, output_sample_desc, L"wmvencdata.bin");
    ok(ret == 0, "got %lu%% diff\n", ret);
    IMFCollection_Release(output_samples);

    output_sample = create_sample(NULL, expect_output_info.cbSize);
    status = 0xdeadbeef;
    memset(&output, 0, sizeof(output));
    output.pSample = output_sample;
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output.pSample == output_sample, "got pSample %p\n", output.pSample);
    ok(output.dwStatus == 0, "got dwStatus %#lx\n", output.dwStatus);
    ok(status == 0, "got status %#lx\n", status);
    hr = IMFSample_GetTotalLength(output.pSample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);

done:
    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

failed:
    winetest_pop_context();
    CoUninitialize();
}

static void test_wmv_decoder(void)
{
    const GUID *const class_id = &CLSID_CWMVDecMediaObject;
    const struct transform_info expect_mft_info =
    {
        .name = L"WMVideo Decoder MFT",
        .major_type = &MFMediaType_Video,
        .inputs =
        {
            {.subtype = &MFVideoFormat_WMV1},
            {.subtype = &MFVideoFormat_WMV2},
            {.subtype = &MFVideoFormat_WMV3},
            {.subtype = &MEDIASUBTYPE_WMVP},
            {.subtype = &MEDIASUBTYPE_WVP2},
            {.subtype = &MEDIASUBTYPE_WMVR},
            {.subtype = &MEDIASUBTYPE_WMVA},
            {.subtype = &MFVideoFormat_WVC1},
            {.subtype = &MFVideoFormat_VC1S},
        },
        .outputs =
        {
            {.subtype = &MFVideoFormat_YV12},
            {.subtype = &MFVideoFormat_YUY2},
            {.subtype = &MFVideoFormat_UYVY},
            {.subtype = &MFVideoFormat_YVYU},
            {.subtype = &MFVideoFormat_NV11},
            {.subtype = &MFVideoFormat_NV12},
            {.subtype = &DMOVideoFormat_RGB32},
            {.subtype = &DMOVideoFormat_RGB24},
            {.subtype = &DMOVideoFormat_RGB565},
            {.subtype = &DMOVideoFormat_RGB555},
            {.subtype = &DMOVideoFormat_RGB8},
        },
    };
    const struct transform_info expect_dmo_info =
    {
        .name = L"WMVideo Decoder DMO",
        .major_type = &MEDIATYPE_Video,
        .inputs =
        {
            {.subtype = &MEDIASUBTYPE_WMV1},
            {.subtype = &MEDIASUBTYPE_WMV2},
            {.subtype = &MEDIASUBTYPE_WMV3},
            {.subtype = &MEDIASUBTYPE_WMVA},
            {.subtype = &MEDIASUBTYPE_WVC1},
            {.subtype = &MEDIASUBTYPE_WMVP},
            {.subtype = &MEDIASUBTYPE_WVP2},
            {.subtype = &MFVideoFormat_VC1S},
        },
        .outputs =
        {
            {.subtype = &MEDIASUBTYPE_YV12},
            {.subtype = &MEDIASUBTYPE_YUY2},
            {.subtype = &MEDIASUBTYPE_UYVY},
            {.subtype = &MEDIASUBTYPE_YVYU},
            {.subtype = &MEDIASUBTYPE_NV11},
            {.subtype = &MEDIASUBTYPE_NV12},
            {.subtype = &MEDIASUBTYPE_RGB32},
            {.subtype = &MEDIASUBTYPE_RGB24},
            {.subtype = &MEDIASUBTYPE_RGB565},
            {.subtype = &MEDIASUBTYPE_RGB555},
            {.subtype = &MEDIASUBTYPE_RGB8},
        },
    };

    static const struct attribute_desc expect_common_attributes[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        {0},
    };
    static const media_type_desc expect_available_inputs[] =
    {
        {ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_WMV1)},
        {ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_WMV2)},
        {ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_WMVA)},
        {ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_WMVP)},
        {ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_WVP2)},
        {ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_WMV_Unknown)},
        {ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_WVC1)},
        {ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_WMV3)},
        {ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_VC1S)},
    };
    static const MFVideoArea actual_aperture = {.Area={96,96}};
    static const DWORD actual_width = 96, actual_height = 96;
    const struct attribute_desc expect_output_attributes[] =
    {
        ATTR_BLOB(MF_MT_GEOMETRIC_APERTURE, &actual_aperture, sizeof(actual_aperture)),
        ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, sizeof(actual_aperture)),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, 2, .todo_value = TRUE),
        {0},
    };
    const media_type_desc expect_available_outputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YV12),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_IYUV),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 2),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 2),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_UYVY),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 2),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 2),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YVYU),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 2),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 2),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV11),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 4),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 4),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 3),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB565),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 2),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 2),
            /* ATTR_BLOB(MF_MT_PALETTE, ... with 12 elements), */
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB555),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 2),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 2),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
            ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB8),
            ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
            ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
            ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height),
            /* ATTR_BLOB(MF_MT_PALETTE, ... with 904 elements), */
        },
    };
    const struct attribute_desc expect_attributes[] =
    {
        ATTR_UINT32(MF_LOW_LATENCY, 0),
        ATTR_UINT32(MF_SA_D3D11_AWARE, 1),
        ATTR_UINT32(MF_SA_D3D_AWARE, 1),
        ATTR_UINT32(MFT_DECODER_EXPOSE_OUTPUT_TYPES_IN_NATIVE_ORDER, 0),
        /* more attributes from CODECAPI */
        {0},
    };
    const struct attribute_desc input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_WMV1, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE, .todo = TRUE),
        {0},
    };
    const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        {0},
    };
    const struct attribute_desc output_type_desc_negative_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, -actual_width),
        {0},
    };
    const struct attribute_desc output_type_desc_rgb[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        {0},
    };
    const struct attribute_desc output_type_desc_rgb_negative_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, -actual_width * 4),
        {0},
    };
    const struct attribute_desc output_type_desc_rgb_positive_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 4),
        {0},
    };
    const struct attribute_desc expect_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_WMV1),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1, .todo = TRUE),
        {0},
    };
    const struct attribute_desc expect_output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
        ATTR_UINT32(MF_MT_VIDEO_NOMINAL_RANGE, 2),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        {0},
    };
    const struct attribute_desc expect_output_type_desc_rgb[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 4),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 4),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
        ATTR_UINT32(MF_MT_VIDEO_NOMINAL_RANGE, 2),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        {0},
    };
    const struct attribute_desc expect_output_type_desc_rgb_negative_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, -actual_width * 4),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 4),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
        ATTR_UINT32(MF_MT_VIDEO_NOMINAL_RANGE, 2),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        {0},
    };
    const MFT_OUTPUT_STREAM_INFO expect_output_info =
    {
        .dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_OUTPUT_STREAM_DISCARDABLE,
        .cbSize = 0x3600,
        .cbAlignment = 1,
    };
    const MFT_OUTPUT_STREAM_INFO expect_output_info_rgb =
    {
        .dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_OUTPUT_STREAM_DISCARDABLE,
        .cbSize = 0x9000,
        .cbAlignment = 1,
    };
    const MFT_OUTPUT_STREAM_INFO empty_output_info =
    {
        .dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_OUTPUT_STREAM_DISCARDABLE,
    };
    const MFT_INPUT_STREAM_INFO expect_input_info =
    {
        .cbSize = 0x3600,
        .cbAlignment = 1,
    };
    const MFT_INPUT_STREAM_INFO expect_input_info_rgb =
    {
        .cbSize = 0x9000,
        .cbAlignment = 1,
    };
    const MFT_INPUT_STREAM_INFO empty_input_info = {0};

    const struct attribute_desc output_sample_attributes[] =
    {
        ATTR_UINT32(MFSampleExtension_CleanPoint, 1),
        {0},
    };
    const struct buffer_desc output_buffer_desc_nv12 =
    {
        .length = actual_width * actual_height * 3 / 2,
        .compare = compare_nv12, .compare_rect = {.right = 82, .bottom = 84},
        .dump = dump_nv12, .size = {.cx = actual_width, .cy = actual_height},
    };
    const struct buffer_desc output_buffer_desc_rgb =
    {
        .length = actual_width * actual_height * 4,
        .compare = compare_rgb32, .compare_rect = {.right = 82, .bottom = 84},
        .dump = dump_rgb32, .size = {.cx = actual_width, .cy = actual_height},
    };
    const struct sample_desc output_sample_desc_nv12 =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 333333,
        .buffer_count = 1, .buffers = &output_buffer_desc_nv12,
    };
    const struct sample_desc output_sample_desc_rgb =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 333333,
        .buffer_count = 1, .buffers = &output_buffer_desc_rgb,
    };

    const struct transform_desc
    {
        const struct attribute_desc *output_type_desc;
        const struct attribute_desc *expect_output_type_desc;
        const MFT_INPUT_STREAM_INFO *expect_input_info;
        const MFT_OUTPUT_STREAM_INFO *expect_output_info;
        const struct sample_desc *output_sample_desc;
        const WCHAR *result_bitmap;
        ULONG delta;
        BOOL new_transform;
        BOOL todo;
    }
    transform_tests[] =
    {
        {
            /* WMV1 -> YUV */
            .output_type_desc = output_type_desc,
            .expect_output_type_desc = expect_output_type_desc,
            .expect_input_info = &expect_input_info,
            .expect_output_info = &expect_output_info,
            .output_sample_desc = &output_sample_desc_nv12,
            .result_bitmap = L"nv12frame.bmp",
            .delta = 0,
        },

        {
            /* WMV1 -> YUV (negative stride) */
            .output_type_desc = output_type_desc_negative_stride,
            .expect_output_type_desc = expect_output_type_desc,
            .expect_input_info = &expect_input_info,
            .expect_output_info = &expect_output_info,
            .output_sample_desc = &output_sample_desc_nv12,
            .result_bitmap = L"nv12frame.bmp",
            .delta = 0,
        },

        {
            /* WMV1 -> RGB */
            .output_type_desc = output_type_desc_rgb,
            .expect_output_type_desc = expect_output_type_desc_rgb,
            .expect_input_info = &expect_input_info_rgb,
            .expect_output_info = &expect_output_info_rgb,
            .output_sample_desc = &output_sample_desc_rgb,
            .result_bitmap = L"rgb32frame-flip.bmp",
            .delta = 5,
        },

        {
            /* WMV1 -> RGB (negative stride) */
            .output_type_desc = output_type_desc_rgb_negative_stride,
            .expect_output_type_desc = expect_output_type_desc_rgb_negative_stride,
            .expect_input_info = &expect_input_info_rgb,
            .expect_output_info = &expect_output_info_rgb,
            .output_sample_desc = &output_sample_desc_rgb,
            .result_bitmap = L"rgb32frame-flip.bmp",
            .delta = 5,
        },

        {
            /* WMV1 -> RGB (positive stride) */
            .output_type_desc = output_type_desc_rgb_positive_stride,
            .expect_output_type_desc = expect_output_type_desc_rgb,
            .expect_input_info = &expect_input_info_rgb,
            .expect_output_info = &expect_output_info_rgb,
            .output_sample_desc = &output_sample_desc_rgb,
            .result_bitmap = L"rgb32frame-flip.bmp",
            .delta = 5,
        },

        {
            /* WMV1 -> RGB (w/ new transform) */
            .output_type_desc = output_type_desc_rgb,
            .expect_output_type_desc = expect_output_type_desc_rgb,
            .expect_input_info = &expect_input_info_rgb,
            .expect_output_info = &expect_output_info_rgb,
            .output_sample_desc = &output_sample_desc_rgb,
            .result_bitmap = L"rgb32frame.bmp",
            .delta = 5,
            .new_transform = TRUE,
        },

        {
            /* WMV1 -> RGB (negative stride, but reusing MFT w/ positive stride) */
            .output_type_desc = output_type_desc_rgb_negative_stride,
            .expect_output_type_desc = expect_output_type_desc_rgb_negative_stride,
            .expect_input_info = &expect_input_info_rgb,
            .expect_output_info = &expect_output_info_rgb,
            .output_sample_desc = &output_sample_desc_rgb,
            .result_bitmap = L"rgb32frame.bmp",
            .delta = 5,
        },

        {
            /* WMV1 -> RGB (negative stride w/ new transform) */
            .output_type_desc = output_type_desc_rgb_negative_stride,
            .expect_output_type_desc = expect_output_type_desc_rgb_negative_stride,
            .expect_input_info = &expect_input_info_rgb,
            .expect_output_info = &expect_output_info_rgb,
            .output_sample_desc = &output_sample_desc_rgb,
            .result_bitmap = L"rgb32frame-flip.bmp",
            .delta = 5,
            .new_transform = TRUE,
        },

        {
            /* WMV1 -> RGB (positive stride w/ new transform) */
            .output_type_desc = output_type_desc_rgb_positive_stride,
            .expect_output_type_desc = expect_output_type_desc_rgb,
            .expect_input_info = &expect_input_info_rgb,
            .expect_output_info = &expect_output_info_rgb,
            .output_sample_desc = &output_sample_desc_rgb,
            .result_bitmap = L"rgb32frame.bmp",
            .delta = 5,
            .new_transform = TRUE,
        },
    };

    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Video, MFVideoFormat_NV12};
    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Video, MFVideoFormat_WMV1};
    IMFSample *input_sample, *output_sample;
    MFT_OUTPUT_STREAM_INFO output_info;
    MFT_INPUT_STREAM_INFO input_info;
    IMFCollection *output_samples;
    IMFMediaType *media_type;
    IMFTransform *transform;
    const BYTE *wmvenc_data;
    ULONG wmvenc_data_len;
    DWORD output_status;
    ULONG i, j, ret, ref;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    winetest_push_context("wmvdec");

    if (!has_video_processor)
    {
        win_skip("Skipping inconsistent WMV decoder tests on Win7\n");
        goto failed;
    }

    if (!check_mft_enum(MFT_CATEGORY_VIDEO_DECODER, &input_type, &output_type, class_id))
        goto failed;
    check_mft_get_info(class_id, &expect_mft_info);
    check_dmo_get_info(class_id, &expect_dmo_info);

    if (FAILED(hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_IMediaObject, TRUE);
    check_interface(transform, &IID_IPropertyStore, TRUE);
    check_interface(transform, &IID_IPropertyBag, TRUE);

    check_mft_optional_methods(transform, 1);
    check_mft_get_attributes(transform, expect_attributes, TRUE);

    memset(&input_info, 0xcd, sizeof(input_info));
    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
    todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetInputStreamInfo returned %#lx\n", hr);
    check_member(input_info, empty_input_info, "%I64d", hnsMaxLatency);
    check_member(input_info, empty_input_info, "%#lx",  dwFlags);
    check_member(input_info, empty_input_info, "%#lx",  cbSize);
    check_member(input_info, empty_input_info, "%#lx",  cbMaxLookahead);
    check_member(input_info, empty_input_info, "%#lx",  cbAlignment);

    memset(&output_info, 0xcd, sizeof(output_info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetOutputStreamInfo returned %#lx\n", hr);
    todo_wine
    check_member(output_info, empty_output_info, "%#lx",  dwFlags);
    check_member(output_info, empty_output_info, "%#lx",  cbSize);
    check_member(output_info, empty_output_info, "%#lx",  cbAlignment);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetOutputAvailableType returned %#lx\n", hr);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetInputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("in %lu", i);
        ok(hr == S_OK, "GetInputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_common_attributes, -1);
        check_media_type(media_type, expect_available_inputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(!ret, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetInputAvailableType returned %#lx\n", hr);
    ok(i == ARRAY_SIZE(expect_available_inputs), "%lu input media types\n", i);

    if (hr == E_NOTIMPL)
        goto skip_tests;

    check_mft_set_output_type(transform, output_type_desc, MF_E_TRANSFORM_TYPE_NOT_SET);
    check_mft_get_output_current_type(transform, NULL);

    check_mft_set_input_type_required(transform, input_type_desc);
    check_mft_set_input_type(transform, input_type_desc, S_OK);
    check_mft_get_input_current_type_(__LINE__, transform, expect_input_type_desc, FALSE, TRUE);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_common_attributes, -1);
        check_media_type(media_type, expect_output_attributes, -1);
        check_media_type(media_type, expect_available_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(!ret, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == ARRAY_SIZE(expect_available_outputs), "%lu input media types\n", i);

    check_mft_set_output_type(transform, output_type_desc_rgb, S_OK);

    for (j = 0; j < ARRAY_SIZE(transform_tests); j++)
    {
        winetest_push_context("transform #%lu", j);

        if (transform_tests[j].new_transform)
        {
            ret = IMFTransform_Release(transform);
            ok(ret == 0, "Release returned %lu\n", ret);

            if (FAILED(hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IMFTransform, (void **)&transform)))
                goto failed;
            check_mft_set_input_type(transform, input_type_desc, S_OK);
        }

        check_mft_set_output_type_required(transform, transform_tests[j].output_type_desc);
        check_mft_set_output_type(transform, transform_tests[j].output_type_desc, S_OK);
        check_mft_get_output_current_type_(__LINE__, transform, transform_tests[j].expect_output_type_desc, FALSE, TRUE);

        memset(&input_info, 0xcd, sizeof(input_info));
        hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
        ok(hr == S_OK, "GetInputStreamInfo returned %#lx\n", hr);
        check_member(input_info, *transform_tests[j].expect_input_info, "%I64d", hnsMaxLatency);
        check_member(input_info, *transform_tests[j].expect_input_info, "%#lx",  dwFlags);
        todo_wine
        check_member(input_info, *transform_tests[j].expect_input_info, "%#lx",  cbSize);
        check_member(input_info, *transform_tests[j].expect_input_info, "%#lx",  cbMaxLookahead);
        todo_wine
        check_member(input_info, *transform_tests[j].expect_input_info, "%#lx",  cbAlignment);

        memset(&output_info, 0xcd, sizeof(output_info));
        hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
        ok(hr == S_OK, "GetOutputStreamInfo returned %#lx\n", hr);
        todo_wine
        check_member(output_info, *transform_tests[j].expect_output_info, "%#lx",  dwFlags);
        todo_wine_if(transform_tests[j].expect_output_info == &expect_output_info)
        check_member(output_info, *transform_tests[j].expect_output_info, "%#lx",  cbSize);
        todo_wine
        check_member(output_info, *transform_tests[j].expect_output_info, "%#lx",  cbAlignment);

        load_resource(L"wmvencdata.bin", &wmvenc_data, &wmvenc_data_len);

        input_sample = create_sample(wmvenc_data + sizeof(DWORD), *(DWORD *)wmvenc_data);
        wmvenc_data_len -= *(DWORD *)wmvenc_data + sizeof(DWORD);
        wmvenc_data += *(DWORD *)wmvenc_data + sizeof(DWORD);
        hr = IMFSample_SetSampleTime(input_sample, 0);
        ok(hr == S_OK, "SetSampleTime returned %#lx\n", hr);
        hr = IMFSample_SetSampleDuration(input_sample, 333333);
        ok(hr == S_OK, "SetSampleDuration returned %#lx\n", hr);
        hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
        ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
        ret = IMFSample_Release(input_sample);
        ok(ret <= 1, "Release returned %ld\n", ret);

        hr = MFCreateCollection(&output_samples);
        ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

        output_sample = create_sample(NULL, transform_tests[j].expect_output_info->cbSize);
        for (i = 0; SUCCEEDED(hr = check_mft_process_output(transform, output_sample, &output_status)); i++)
        {
            winetest_push_context("%lu", i);
            ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
            hr = IMFCollection_AddElement(output_samples, (IUnknown *)output_sample);
            ok(hr == S_OK, "AddElement returned %#lx\n", hr);
            ref = IMFSample_Release(output_sample);
            ok(ref == 1, "Release returned %ld\n", ref);
            output_sample = create_sample(NULL, transform_tests[j].expect_output_info->cbSize);
            winetest_pop_context();
        }
        ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
        ret = IMFSample_Release(output_sample);
        ok(ret == 0, "Release returned %lu\n", ret);
        ok(i == 1, "got %lu output samples\n", i);

        ret = check_mf_sample_collection(output_samples, transform_tests[j].output_sample_desc,
                                         transform_tests[j].result_bitmap);
        todo_wine_if(transform_tests[j].todo)
        ok(ret <= transform_tests[j].delta, "got %lu%% diff\n", ret);
        IMFCollection_Release(output_samples);

        hr = IMFTransform_SetOutputType(transform, 0, NULL, 0);
        ok(hr == S_OK, "SetOutputType returned %#lx\n", hr);

        winetest_pop_context();
    }

skip_tests:
    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

failed:
    winetest_pop_context();
    CoUninitialize();
}

static void test_wmv_decoder_timestamps(void)
{
    struct timestamps
    {
        LONGLONG time;
        LONGLONG duration;
    };

    static const DWORD actual_width = 96, actual_height = 96;

    const struct attribute_desc input_type_nofps_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_WMV1, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE, .todo = TRUE),
        {0},
    };
    const struct attribute_desc output_type_nofps_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        {0},
    };

    const struct attribute_desc input_type_24fps_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_WMV1, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE, .todo = TRUE),
        ATTR_RATIO(MF_MT_FRAME_RATE, 24, 1,),
        {0},
    };
    const struct attribute_desc output_type_24fps_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_RATE, 24, 1,),
        {0},
    };
    static const struct timestamps exp_nofps[] =
    {
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
    };
    static const struct timestamps exp_24fps[] =
    {
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
    };
    static const struct timestamps input_sample_ts[] =
    {
        {  666666, 166667 },
        {  833333, 166666 },
        {  999999, 166666 },
        { 1166665, 166667 },
        { 1333332, 166667 },
        { 1499999, 166667 },
        { 1666666, 166666 },
        { 1833332, 166667 },
        { 1999999, 166667 },
        { 2166666, 166666 },
        { 2333332, 166667 },
        { 2499999, 166667 },
        { 2666666, 166667 },
        { 2833333, 166666 },
        { 2999999, 166666 },
        { 3166665, 166667 },
        { 3333332, 166667 },
        { 3499999, 166667 },
        { 3666666, 166666 },
        { 3833332, 166667 },
        { 3999999, 166667 },
    };
    static const struct timestamps exp_sample_ts[] =
    {
        {  666666, 166667 },
        {  833333, 166666 },
        {  999999, 166666 },
        { 1166665, 166667 },
        { 1333332, 166667 },
        { 1499999, 166667 },
        { 1666666, 166666 },
        { 1833332, 166667 },
        { 1999999, 166667 },
        { 2166666, 166666 },
    };

    IMFSample *input_sample, *output_sample;
    IMFTransform *transform;
    LONGLONG duration, time;
    const BYTE *wmvenc_data;
    ULONG wmvenc_data_len;
    DWORD output_status;
    ULONG i, j, ret;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    winetest_push_context("wmvdec timestamps");

    /* Test when no framerate is provided and samples contain no timestamps */
    if (FAILED(hr = CoCreateInstance(&CLSID_CWMVDecMediaObject, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;

    load_resource(L"wmvencdata.bin", &wmvenc_data, &wmvenc_data_len);

    check_mft_set_input_type(transform, input_type_nofps_desc, S_OK);
    check_mft_set_output_type(transform, output_type_nofps_desc, S_OK);

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    ok(hr == S_OK, "Got %#lx\n", hr);

    output_sample = create_sample(NULL, actual_width * actual_height * 2);
    for (i = 0; i < ARRAYSIZE(exp_nofps);)
    {
        winetest_push_context("output %ld", i);
        hr = check_mft_process_output(transform, output_sample, &output_status);
        ok(hr == S_OK || hr == MF_E_TRANSFORM_STREAM_CHANGE || hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
        if (hr == S_OK)
        {
            hr = IMFSample_GetSampleTime(output_sample, &time);
            ok(hr == S_OK, "Got %#lx\n", hr);
            ok(time == exp_nofps[i].time, "got time %I64d, expected %I64d\n", time, exp_nofps[i].time);
            hr = IMFSample_GetSampleDuration(output_sample, &duration);
            ok(hr == S_OK, "Got %#lx\n", hr);
            ok(duration == exp_nofps[i].duration, "got duration %I64d, expected %I64d\n", duration, exp_nofps[i].duration);
            ret = IMFSample_Release(output_sample);
            ok(ret == 0, "Release returned %lu\n", ret);
            output_sample = create_sample(NULL, actual_width * actual_height * 2);
            i++;
        }
        else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT && wmvenc_data_len)
        {
            input_sample = create_sample(wmvenc_data + sizeof(DWORD), *(DWORD *)wmvenc_data);
            wmvenc_data_len -= *(DWORD *)wmvenc_data + sizeof(DWORD);
            wmvenc_data += *(DWORD *)wmvenc_data + sizeof(DWORD);
            hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
            ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
            ret = IMFSample_Release(input_sample);
            ok(ret <= 1, "Release returned %lu\n", ret);
        }
        else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT && !wmvenc_data_len)
        {
            i = ARRAYSIZE(exp_nofps) + 1;
        }
        winetest_pop_context();
    }

    ok(i == ARRAYSIZE(exp_nofps), "transform requires more input than available\n");

    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

    /* Test when 24fps framerate is provided and samples contain no timestamps */
    hr = CoCreateInstance(&CLSID_CWMVDecMediaObject, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "Got %#lx\n", hr);

    load_resource(L"wmvencdata.bin", &wmvenc_data, &wmvenc_data_len);

    check_mft_set_input_type(transform, input_type_24fps_desc, S_OK);
    check_mft_set_output_type(transform, output_type_24fps_desc, S_OK);

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    ok(hr == S_OK, "Got %#lx\n", hr);

    output_sample = create_sample(NULL, actual_width * actual_height * 2);
    for (i = 0; i < ARRAYSIZE(exp_24fps);)
    {
        winetest_push_context("output %ld", i);
        hr = check_mft_process_output(transform, output_sample, &output_status);
        ok(hr == S_OK || hr == MF_E_TRANSFORM_STREAM_CHANGE || hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
        if (hr == S_OK)
        {
            hr = IMFSample_GetSampleTime(output_sample, &time);
            ok(hr == S_OK, "Got %#lx\n", hr);
            ok(time == exp_24fps[i].time, "got time %I64d, expected %I64d\n", time, exp_24fps[i].time);
            hr = IMFSample_GetSampleDuration(output_sample, &duration);
            ok(hr == S_OK, "Got %#lx\n", hr);
            ok(duration == exp_24fps[i].duration, "got duration %I64d, expected %I64d\n", duration, exp_24fps[i].duration);
            ret = IMFSample_Release(output_sample);
            ok(ret == 0, "Release returned %lu\n", ret);
            output_sample = create_sample(NULL, actual_width * actual_height * 2);
            i++;
        }
        else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT && wmvenc_data_len)
        {
            input_sample = create_sample(wmvenc_data + sizeof(DWORD), *(DWORD *)wmvenc_data);
            wmvenc_data_len -= *(DWORD *)wmvenc_data + sizeof(DWORD);
            wmvenc_data += *(DWORD *)wmvenc_data + sizeof(DWORD);
            hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
            ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
            ret = IMFSample_Release(input_sample);
            ok(ret <= 1, "Release returned %lu\n", ret);
        }
        else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT && !wmvenc_data_len)
        {
            i = ARRAYSIZE(exp_24fps) + 1;
        }
        winetest_pop_context();
    }

    ok(i == ARRAYSIZE(exp_24fps), "transform requires more input than available\n");

    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

    /* Test when provided sample timestamps disagree with MT_MF_FRAME_RATE */
    hr = CoCreateInstance(&CLSID_CWMVDecMediaObject, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "Got %#lx\n", hr);

    load_resource(L"wmvencdata.bin", &wmvenc_data, &wmvenc_data_len);

    check_mft_set_input_type(transform, input_type_24fps_desc, S_OK);
    check_mft_set_output_type(transform, output_type_24fps_desc, S_OK);

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    ok(hr == S_OK, "Got %#lx\n", hr);

    j = 0;
    output_sample = create_sample(NULL, actual_width * actual_height * 2);
    for (i = 0; i < ARRAYSIZE(exp_sample_ts);)
    {
        winetest_push_context("output %ld", i);
        hr = check_mft_process_output(transform, output_sample, &output_status);
        ok(hr == S_OK || hr == MF_E_TRANSFORM_STREAM_CHANGE || hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
        if (hr == S_OK)
        {
            hr = IMFSample_GetSampleTime(output_sample, &time);
            ok(hr == S_OK, "Got %#lx\n", hr);
            ok(time == exp_sample_ts[i].time, "got time %I64d, expected %I64d\n", time, exp_sample_ts[i].time);
            hr = IMFSample_GetSampleDuration(output_sample, &duration);
            ok(hr == S_OK, "Got %#lx\n", hr);
            ok(duration == exp_sample_ts[i].duration, "got duration %I64d, expected %I64d\n", duration, exp_sample_ts[i].duration);
            ret = IMFSample_Release(output_sample);
            ok(ret == 0, "Release returned %lu\n", ret);
            output_sample = create_sample(NULL, actual_width * actual_height * 2);
            i++;
        }
        else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT && j < ARRAYSIZE(input_sample_ts))
        {
            input_sample = create_sample(wmvenc_data + sizeof(DWORD), *(DWORD *)wmvenc_data);
            wmvenc_data_len -= *(DWORD *)wmvenc_data + sizeof(DWORD);
            wmvenc_data += *(DWORD *)wmvenc_data + sizeof(DWORD);
            hr = IMFSample_SetSampleTime(input_sample, input_sample_ts[j].time);
            ok(hr == S_OK, "Got %#lx\n", hr);
            hr = IMFSample_SetSampleDuration(input_sample, input_sample_ts[j].duration);
            ok(hr == S_OK, "Got %#lx\n", hr);
            j++;
            hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
            ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
            ret = IMFSample_Release(input_sample);
            ok(ret <= 1, "Release returned %lu\n", ret);
        }
        else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT && j == ARRAYSIZE(input_sample_ts))
        {
            todo_wine
            ok(0, "no more input to provide\n");
            i = ARRAYSIZE(exp_sample_ts);
        }
        winetest_pop_context();
    }

    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

failed:
    winetest_pop_context();
    CoUninitialize();
}

static void test_wmv_decoder_dmo_input_type(void)
{
    const GUID *input_subtypes[] =
    {
        &MEDIASUBTYPE_WMV1,
        &MEDIASUBTYPE_WMV2,
        &MEDIASUBTYPE_WMVA,
        &MEDIASUBTYPE_WMVP,
        &MEDIASUBTYPE_WVP2,
        &MFVideoFormat_WMV_Unknown,
        &MEDIASUBTYPE_WVC1,
        &MEDIASUBTYPE_WMV3,
        &MFVideoFormat_VC1S,
    };

    DMO_MEDIA_TYPE *good_input_type, *bad_input_type, type;
    char buffer_good_input[2048], buffer_bad_input[2048];
    const GUID *input_subtype = input_subtypes[0];
    LONG width = 16, height = 16;
    VIDEOINFOHEADER *header;
    DWORD count, i, ret;
    IMediaObject *dmo;
    HRESULT hr;

    winetest_push_context("wmvdec");

    if (!has_video_processor)
    {
        win_skip("Skipping WMV decoder DMO tests on Win7.\n");
        winetest_pop_context();
        return;
    }

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "CoInitialize failed, hr %#lx.\n", hr);

    if (FAILED(hr = CoCreateInstance(&CLSID_CWMVDecMediaObject, NULL, CLSCTX_INPROC_SERVER, &IID_IMediaObject, (void **)&dmo)))
    {
        CoUninitialize();
        winetest_pop_context();
        return;
    }

    good_input_type = (void *)buffer_good_input;
    bad_input_type = (void *)buffer_bad_input;
    init_dmo_media_type_video(good_input_type, input_subtype, width, height, 0);
    memset(bad_input_type, 0, sizeof(buffer_bad_input));
    header = (void *)(good_input_type + 1);

    /* Test GetInputType. */
    count = ARRAY_SIZE(input_subtypes);
    hr = IMediaObject_GetInputType(dmo, 1, 0, NULL);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputType(dmo, 1, 0, &type);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputType(dmo, 1, count, &type);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputType(dmo, 0, count, &type);
    ok(hr == DMO_E_NO_MORE_ITEMS, "GetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputType(dmo, 0, count, NULL);
    ok(hr == DMO_E_NO_MORE_ITEMS, "GetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputType(dmo, 0, 0xdeadbeef, NULL);
    ok(hr == DMO_E_NO_MORE_ITEMS, "GetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputType(dmo, 0, count - 1, NULL);
    ok(hr == S_OK, "GetInputType returned %#lx.\n", hr);
    hr = IMediaObject_GetInputType(dmo, 0, count - 1, &type);
    ok(hr == S_OK, "GetInputType returned %#lx.\n", hr);
    MoFreeMediaType(&type);

    i = -1;
    while (SUCCEEDED(hr = IMediaObject_GetInputType(dmo, 0, ++i, &type)))
    {
        winetest_push_context("in %lu", i);

        memset(good_input_type, 0, sizeof(*good_input_type));
        good_input_type->majortype = MEDIATYPE_Video;
        good_input_type->subtype = *input_subtypes[i];
        good_input_type->bTemporalCompression = TRUE;
        check_dmo_media_type(&type, good_input_type);

        MoFreeMediaType(&type);

        winetest_pop_context();
    }
    ok(hr == DMO_E_NO_MORE_ITEMS, "GetInputType returned %#lx.\n", hr);
    ok(i == count, "%lu types.\n", i);

    /* Test SetInputType. */
    hr = IMediaObject_SetInputType(dmo, 1, NULL, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, bad_input_type, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, good_input_type, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, NULL, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, bad_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, NULL, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, bad_input_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, good_input_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, NULL, 0x4);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, bad_input_type, 0x4);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 1, good_input_type, 0x4);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetInputType returned %#lx.\n", hr);

    hr = IMediaObject_SetInputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR | DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR | 0x4);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, NULL, 0);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, NULL, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, NULL, 0x4);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);

    hr = IMediaObject_SetInputType(dmo, 0, bad_input_type, 0);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, bad_input_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, bad_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetInputType(dmo, 0, bad_input_type, 0x4);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(input_subtypes); ++i)
    {
        const GUID *subtype = input_subtypes[i];

        if (IsEqualGUID(subtype, &MEDIASUBTYPE_WMV2)
                || IsEqualGUID(subtype, &MEDIASUBTYPE_WMVA)
                || IsEqualGUID(subtype, &MEDIASUBTYPE_WVP2)
                || IsEqualGUID(subtype, &MEDIASUBTYPE_WVC1)
                || IsEqualGUID(subtype, &MFVideoFormat_VC1S))
        {
            skip("Skipping SetInputType tests for video subtype %s.\n", debugstr_guid(subtype));
            continue;
        }

        winetest_push_context("type %lu", i);

        init_dmo_media_type_video(good_input_type, subtype, width, height, 0);
        hr = IMediaObject_SetInputType(dmo, 0, good_input_type, 0);
        ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
        hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_CLEAR);
        ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
        hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
        ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
        hr = IMediaObject_SetInputType(dmo, 0, good_input_type, 0x4);
        ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);

        winetest_pop_context();
    }

    init_dmo_media_type_video(good_input_type, input_subtype, width, height, 0);
    header->dwBitRate = 0xdeadbeef;
    header->dwBitErrorRate = 0xdeadbeef;
    header->AvgTimePerFrame = 0xdeadbeef;
    header->bmiHeader.biPlanes = 0xdead;
    header->bmiHeader.biBitCount = 0xdead;
    header->bmiHeader.biSizeImage = 0xdeadbeef;
    header->bmiHeader.biXPelsPerMeter = 0xdead;
    header->bmiHeader.biYPelsPerMeter = 0xdead;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);

    init_dmo_media_type_video(good_input_type, input_subtype, width, height, 0);
    good_input_type->majortype = MFMediaType_Default;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);

    init_dmo_media_type_video(good_input_type, &MEDIASUBTYPE_None, width, height, 0);
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);

    init_dmo_media_type_video(good_input_type, input_subtype, width, height, 0);
    good_input_type->formattype = FORMAT_None;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);

    init_dmo_media_type_video(good_input_type, input_subtype, width, height, 0);
    good_input_type->cbFormat = 1;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);

    init_dmo_media_type_video(good_input_type, input_subtype, width, height, 0);
    good_input_type->pbFormat = NULL;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);

    init_dmo_media_type_video(good_input_type, input_subtype, width, height, 0);
    header->bmiHeader.biSize = 0;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    todo_wine
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);
    header->bmiHeader.biSize = 1;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    todo_wine
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);
    header->bmiHeader.biSize = 0xdeadbeef;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    todo_wine
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);

    init_dmo_media_type_video(good_input_type, input_subtype, width, height, 0);
    header->bmiHeader.biWidth = 0;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    todo_wine
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);
    header->bmiHeader.biWidth = -1;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    todo_wine
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);
    header->bmiHeader.biWidth = 4096 + 1;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    todo_wine
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);
    header->bmiHeader.biWidth = 4096;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);

    init_dmo_media_type_video(good_input_type, input_subtype, width, height, 0);
    header->bmiHeader.biHeight = 0;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    todo_wine
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);
    header->bmiHeader.biHeight = 4096 + 1;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    todo_wine
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);
    header->bmiHeader.biHeight = 4096;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    header->bmiHeader.biHeight = -4096;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);

    init_dmo_media_type_video(good_input_type, input_subtype, width, height, 0);
    header->bmiHeader.biCompression = 0;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    todo_wine
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetInputType returned %#lx.\n", hr);
    header->bmiHeader.biCompression = 1;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    header->bmiHeader.biCompression = 2;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    header->bmiHeader.biCompression = 0xdeadbeef;
    hr = IMediaObject_SetInputType(dmo, 0, good_input_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);

    /* Release. */
    ret = IMediaObject_Release(dmo);
    ok(ret == 0, "Release returned %lu\n", ret);
    CoUninitialize();
    winetest_pop_context();
}

static void test_wmv_decoder_dmo_output_type(void)
{
    char buffer_good_output[2048], buffer_bad_output[2048], buffer_input[2048];
    DMO_MEDIA_TYPE *good_output_type, *bad_output_type, *input_type, type;
    const GUID* input_subtype = &MEDIASUBTYPE_WMV1;
    REFERENCE_TIME time_per_frame = 10000000;
    LONG width = 16, height = 16;
    DWORD count, i, ret;
    IMediaObject *dmo;
    HRESULT hr;

    winetest_push_context("wmvdec");

    if (!has_video_processor)
    {
        win_skip("Skipping WMV decoder DMO tests on Win7.\n");
        winetest_pop_context();
        return;
    }

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "CoInitialize failed, hr %#lx.\n", hr);

    if (FAILED(hr = CoCreateInstance(&CLSID_CWMVDecMediaObject, NULL, CLSCTX_INPROC_SERVER, &IID_IMediaObject, (void **)&dmo)))
    {
        CoUninitialize();
        winetest_pop_context();
        return;
    }

    /* Initialize media types. */
    input_type = (void *)buffer_input;
    good_output_type = (void *)buffer_good_output;
    bad_output_type = (void *)buffer_bad_output;
    init_dmo_media_type_video(input_type, input_subtype, width, height, time_per_frame);
    memset(bad_output_type, 0, sizeof(buffer_bad_output));

    /* Test GetOutputType. */
    hr = IMediaObject_GetOutputType(dmo, 1, 0, NULL);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 0, 0, NULL);
    ok(hr == S_OK, "GetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 0, 0, &type);
    ok(hr == DMO_E_TYPE_NOT_SET, "GetOutputType returned %#lx.\n", hr);

    hr = IMediaObject_SetInputType(dmo, 0, input_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);

    count = ARRAY_SIZE(wmv_decoder_output_subtypes);
    hr = IMediaObject_GetOutputType(dmo, 1, 0, NULL);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 1, 0, &type);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 1, count, &type);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 0, count, &type);
    ok(hr == DMO_E_NO_MORE_ITEMS, "GetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 0, count, NULL);
    todo_wine
    ok(hr == S_OK, "GetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 0, 0xdeadbeef, NULL);
    todo_wine
    ok(hr == S_OK, "GetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputType(dmo, 0, count - 1, NULL);
    ok(hr == S_OK, "GetOutputType returned %#lx.\n", hr);

    i = -1;
    while (SUCCEEDED(hr = IMediaObject_GetOutputType(dmo, 0, ++i, &type)))
    {
        winetest_push_context("type %lu", i);
        init_dmo_media_type_video(good_output_type, wmv_decoder_output_subtypes[i], width, height, time_per_frame);
        check_dmo_media_type(&type, good_output_type);
        MoFreeMediaType(&type);
        winetest_pop_context();
    }
    ok(hr == DMO_E_NO_MORE_ITEMS, "GetInputType returned %#lx.\n", hr);
    ok(i == count, "%lu types.\n", i);

    /* Test SetOutputType. */
    init_dmo_media_type_video(good_output_type, &MEDIASUBTYPE_RGB24, width, height, time_per_frame);
    hr = IMediaObject_SetInputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, NULL, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, good_output_type, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, 0);
    ok(hr == E_POINTER, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, 0x4);
    ok(hr == E_POINTER, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR | DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, bad_output_type, 0);
    ok(hr == DMO_E_TYPE_NOT_SET, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, bad_output_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_TYPE_NOT_SET, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, 0);
    ok(hr == DMO_E_TYPE_NOT_SET, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_SET, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_TYPE_NOT_SET, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, 0x4);
    ok(hr == DMO_E_TYPE_NOT_SET, "SetOutputType returned %#lx.\n", hr);

    hr = IMediaObject_SetInputType(dmo, 0, input_type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);

    hr = IMediaObject_SetOutputType(dmo, 1, NULL, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, bad_output_type, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, good_output_type, 0);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, NULL, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, bad_output_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, good_output_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, NULL, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, bad_output_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, good_output_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, NULL, 0x4);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, bad_output_type, 0x4);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 1, good_output_type, 0x4);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "SetOutputType returned %#lx.\n", hr);

    hr = IMediaObject_SetOutputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR | DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, DMO_SET_TYPEF_CLEAR | 0x4);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, 0);
    ok(hr == E_POINTER, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == E_POINTER, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, NULL, 0x4);
    ok(hr == E_POINTER, "SetOutputType returned %#lx.\n", hr);

    hr = IMediaObject_SetOutputType(dmo, 0, bad_output_type, 0);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, bad_output_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, bad_output_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, bad_output_type, 0x4);
    ok(hr == DMO_E_TYPE_NOT_ACCEPTED, "SetOutputType returned %#lx.\n", hr);

    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, 0);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, DMO_SET_TYPEF_CLEAR);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, DMO_SET_TYPEF_TEST_ONLY);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);
    hr = IMediaObject_SetOutputType(dmo, 0, good_output_type, 0x4);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);

    /* Release. */
    ret = IMediaObject_Release(dmo);
    ok(ret == 0, "Release returned %lu\n", ret);
    CoUninitialize();
    winetest_pop_context();
}

static void test_wmv_decoder_dmo_get_size_info(void)
{
    DWORD i, ret, size, alignment;
    IMediaObject *dmo;
    HRESULT hr;

    winetest_push_context("wmvdec");

    if (!has_video_processor)
    {
        win_skip("Skipping WMV decoder DMO tests on Win7.\n");
        winetest_pop_context();
        return;
    }

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "CoInitialize failed, hr %#lx.\n", hr);
    hr = CoCreateInstance(&CLSID_CWMVDecMediaObject, NULL, CLSCTX_INPROC_SERVER, &IID_IMediaObject, (void **)&dmo);
    ok(hr == S_OK, "CoCreateInstance failed, hr %#lx.\n", hr);

    /* Test GetOutputSizeInfo. */
    hr = IMediaObject_GetOutputSizeInfo(dmo, 1, NULL, NULL);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetOutputSizeInfo returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputSizeInfo(dmo, 0, NULL, NULL);
    todo_wine
    ok(hr == E_POINTER, "GetOutputSizeInfo returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputSizeInfo(dmo, 0, &size, NULL);
    todo_wine
    ok(hr == E_POINTER, "GetOutputSizeInfo returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputSizeInfo(dmo, 0, NULL, &alignment);
    todo_wine
    ok(hr == E_POINTER, "GetOutputSizeInfo returned %#lx.\n", hr);
    hr = IMediaObject_GetOutputSizeInfo(dmo, 0, &size, &alignment);
    ok(hr == DMO_E_TYPE_NOT_SET, "GetOutputSizeInfo returned %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(wmv_decoder_output_subtypes); ++i)
    {
        const GUID *subtype = wmv_decoder_output_subtypes[i];

        if (IsEqualGUID(subtype, &MEDIASUBTYPE_RGB565) || IsEqualGUID(subtype, &MEDIASUBTYPE_RGB8))
        {
            skip("Skipping GetOutputSizeInfo tests for output subtype %s.\n", debugstr_guid(subtype));
            continue;
        }

        winetest_push_context("out %lu", i);

        check_dmo_get_output_size_info_video(dmo, &MEDIASUBTYPE_WMV1, subtype, 16, 16);
        check_dmo_get_output_size_info_video(dmo, &MEDIASUBTYPE_WMV1, subtype, 96, 96);
        check_dmo_get_output_size_info_video(dmo, &MEDIASUBTYPE_WMV1, subtype, 320, 240);

        winetest_pop_context();
    }

    ret = IMediaObject_Release(dmo);
    ok(ret == 0, "Release returned %lu\n", ret);
    CoUninitialize();
    winetest_pop_context();
}

static void test_wmv_decoder_media_object(void)
{
    const GUID *const class_id = &CLSID_CWMVDecMediaObject;

    const DWORD data_width = 96, data_height = 96;
    const struct buffer_desc output_buffer_desc_nv12 =
    {
        .length = data_width * data_height * 3 / 2,
        .compare = compare_nv12, .compare_rect = {.right = 82, .bottom = 84},
        .dump = dump_nv12, .size = {.cx = data_width, .cy = data_height},
    };
    DWORD in_count, out_count, size, alignment, wmv_data_length, status, expected_status, diff;
    struct media_buffer *input_media_buffer = NULL, *output_media_buffer = NULL;
    DMO_OUTPUT_DATA_BUFFER output_data_buffer;
    IMediaObject *media_object;
    DMO_MEDIA_TYPE *type;
    const BYTE *wmv_data;
    char buffer[2048];
    HRESULT hr;
    ULONG ret;

    winetest_push_context("wmvdec");

    if (!has_video_processor)
    {
        win_skip("Skipping inconsistent WMV decoder media object tests on Win7.\n");
        winetest_pop_context();
        return;
    }

    type = (DMO_MEDIA_TYPE *)buffer;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "CoInitialize failed, hr %#lx.\n", hr);

    if (FAILED(hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER, &IID_IMediaObject, (void **)&media_object)))
    {
        CoUninitialize();
        winetest_pop_context();
        return;
    }

    /* Test GetStreamCount. */
    in_count = out_count = 0xdeadbeef;
    hr = IMediaObject_GetStreamCount(media_object, &in_count, &out_count);
    ok(hr == S_OK, "GetStreamCount returned %#lx.\n", hr);
    ok(in_count == 1, "Got unexpected in_count %lu.\n", in_count);
    ok(out_count == 1, "Got unexpected in_count %lu.\n", out_count);

    /* Test GetStreamCount with invalid arguments. */
    in_count = out_count = 0xdeadbeef;
    hr = IMediaObject_GetStreamCount(media_object, NULL, &out_count);
    ok(hr == E_POINTER, "GetStreamCount returned %#lx.\n", hr);
    ok(out_count == 0xdeadbeef, "Got unexpected out_count %lu.\n", out_count);
    hr = IMediaObject_GetStreamCount(media_object, &in_count, NULL);
    ok(hr == E_POINTER, "GetStreamCount returned %#lx.\n", hr);
    ok(in_count == 0xdeadbeef, "Got unexpected in_count %lu.\n", in_count);

    /* Test ProcessInput. */
    load_resource(L"wmvencdata.bin", &wmv_data, &wmv_data_length);
    wmv_data_length = *((DWORD *)wmv_data);
    wmv_data += sizeof(DWORD);
    hr = media_buffer_create(wmv_data_length, &input_media_buffer);
    ok(hr == S_OK, "Failed to create input media buffer.\n");
    memcpy(input_media_buffer->data, wmv_data, wmv_data_length);
    input_media_buffer->length = wmv_data_length;

    init_dmo_media_type_video(type, &MEDIASUBTYPE_WMV1, data_width, data_height, 0);
    hr = IMediaObject_SetInputType(media_object, 0, type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    init_dmo_media_type_video(type, &MEDIASUBTYPE_NV12, data_width, data_height, 0);
    hr = IMediaObject_SetOutputType(media_object, 0, type, 0);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);

    hr = IMediaObject_ProcessInput(media_object, 0, &input_media_buffer->IMediaBuffer_iface, 0, 0, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx.\n", hr);

    /* Test ProcessOutput. */
    hr = IMediaObject_GetOutputSizeInfo(media_object, 0, &size, &alignment);
    ok(hr == S_OK, "GetOutputSizeInfo returned %#lx.\n", hr);
    hr = media_buffer_create(size, &output_media_buffer);
    ok(hr == S_OK, "Failed to create output media buffer.\n");
    output_data_buffer.pBuffer = &output_media_buffer->IMediaBuffer_iface;
    output_data_buffer.dwStatus = 0xdeadbeef;
    output_data_buffer.rtTimestamp = 0xdeadbeef;
    output_data_buffer.rtTimelength = 0xdeadbeef;
    hr = IMediaObject_ProcessOutput(media_object, 0, 1, &output_data_buffer, &status);
    ok(hr == S_OK, "ProcessOutput returned %#lx.\n", hr);
    expected_status = DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT | DMO_OUTPUT_DATA_BUFFERF_TIME | DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH;
    ok(output_data_buffer.dwStatus == expected_status, "Got unexpected dwStatus %#lx.\n", output_data_buffer.dwStatus);
    diff = check_dmo_output_data_buffer(&output_data_buffer, &output_buffer_desc_nv12, L"nv12frame.bmp", 0, 0);
    ok(diff == 0, "Got %lu%% diff.\n", diff);

    /* Test GetInputStatus. */
    hr = IMediaObject_GetInputStatus(media_object, 0xdeadbeef, NULL);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetInputStatus returned %#lx.\n", hr);

    status = 0xdeadbeef;
    hr = IMediaObject_GetInputStatus(media_object, 0xdeadbeef, &status);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "GetInputStatus returned %#lx.\n", hr);
    ok(status == 0xdeadbeef, "Unexpected status %#lx.\n", status);

    hr = IMediaObject_GetInputStatus(media_object, 0, NULL);
    ok(hr == E_POINTER, "GetInputStatus returned %#lx.\n", hr);

    hr = IMediaObject_GetInputStatus(media_object, 0, &status);
    ok(hr == S_OK, "GetInputStatus returned %#lx.\n", hr);
    ok(status == DMO_INPUT_STATUSF_ACCEPT_DATA, "Unexpected status %#lx.\n", status);

    /* Test Discontinuity.  */
    hr = IMediaObject_Discontinuity(media_object, 0xdeadbeef);
    ok(hr == DMO_E_INVALIDSTREAMINDEX, "Discontinuity returned %#lx.\n", hr);
    hr = IMediaObject_Discontinuity(media_object, 0);
    ok(hr == S_OK, "Discontinuity returned %#lx.\n", hr);
    hr = IMediaObject_Discontinuity(media_object, 0);
    ok(hr == S_OK, "Discontinuity returned %#lx.\n", hr);
    hr = IMediaObject_GetInputStatus(media_object, 0, &status);
    ok(hr == S_OK, "GetInputStatus returned %#lx.\n", hr);
    ok(status == DMO_INPUT_STATUSF_ACCEPT_DATA, "Unexpected status %#lx.\n", status);

    /* Test Flush. */
    hr = IMediaObject_ProcessInput(media_object, 0, &input_media_buffer->IMediaBuffer_iface, 0, 0, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx.\n", hr);
    hr = IMediaObject_Flush(media_object);
    ok(hr == S_OK, "Flush returned %#lx.\n", hr);
    hr = IMediaObject_Flush(media_object);
    ok(hr == S_OK, "Flush returned %#lx.\n", hr);
    output_media_buffer->length = 0;
    output_data_buffer.pBuffer = &output_media_buffer->IMediaBuffer_iface;
    output_data_buffer.dwStatus = 0xdeadbeef;
    output_data_buffer.rtTimestamp = 0xdeadbeef;
    output_data_buffer.rtTimelength = 0xdeadbeef;
    hr = IMediaObject_ProcessOutput(media_object, 0, 1, &output_data_buffer, &status);
    todo_wine
    ok(hr == S_FALSE, "ProcessOutput returned %#lx.\n", hr);
    ok(output_media_buffer->length == 0, "Unexpected length %#lx.\n", output_media_buffer->length);

    /* Test ProcessOutput with setting framerate. */
    init_dmo_media_type_video(type, &MEDIASUBTYPE_WMV1, data_width, data_height, 100000);
    hr = IMediaObject_SetInputType(media_object, 0, type, 0);
    ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);
    init_dmo_media_type_video(type, &MEDIASUBTYPE_NV12, data_width, data_height, 200000);
    hr = IMediaObject_SetOutputType(media_object, 0, type, 0);
    ok(hr == S_OK, "SetOutputType returned %#lx.\n", hr);

    hr = IMediaObject_ProcessInput(media_object, 0, &input_media_buffer->IMediaBuffer_iface, 0, 0, 300000);
    ok(hr == S_OK, "ProcessInput returned %#lx.\n", hr);

    output_media_buffer->length = 0;
    output_data_buffer.pBuffer = &output_media_buffer->IMediaBuffer_iface;
    output_data_buffer.dwStatus = 0xdeadbeef;
    output_data_buffer.rtTimestamp = 0xdeadbeef;
    output_data_buffer.rtTimelength = 0xdeadbeef;
    hr = IMediaObject_ProcessOutput(media_object, 0, 1, &output_data_buffer, &status);
    ok(hr == S_OK, "ProcessOutput returned %#lx.\n", hr);
    expected_status = DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT | DMO_OUTPUT_DATA_BUFFERF_TIME | DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH;
    ok(output_data_buffer.dwStatus == expected_status, "Got unexpected dwStatus %#lx.\n", output_data_buffer.dwStatus);
    diff = check_dmo_output_data_buffer(&output_data_buffer, &output_buffer_desc_nv12, L"nv12frame.bmp", 0, 300000);
    ok(diff == 0, "Got %lu%% diff.\n", diff);

    ret = IMediaBuffer_Release(&output_media_buffer->IMediaBuffer_iface);
    ok(ret == 0, "Release returned %lu\n", ret);
    ret = IMediaBuffer_Release(&input_media_buffer->IMediaBuffer_iface);
    todo_wine
    ok(ret == 0, "Release returned %lu\n", ret);

    ret = IMediaObject_Release(media_object);
    ok(ret == 0, "Release returned %lu\n", ret);
    CoUninitialize();
    winetest_pop_context();
}

static void test_color_convert(void)
{
    const GUID *const class_id = &CLSID_CColorConvertDMO;
    const struct transform_info expect_mft_info =
    {
        .name = L"Color Converter MFT",
        .major_type = &MFMediaType_Video,
        .inputs =
        {
            {.subtype = &MFVideoFormat_YV12},
            {.subtype = &MFVideoFormat_YUY2},
            {.subtype = &MFVideoFormat_UYVY},
            {.subtype = &MFVideoFormat_AYUV},
            {.subtype = &MFVideoFormat_NV12},
            {.subtype = &DMOVideoFormat_RGB32},
            {.subtype = &DMOVideoFormat_RGB565},
            {.subtype = &MFVideoFormat_I420},
            {.subtype = &MFVideoFormat_IYUV},
            {.subtype = &MFVideoFormat_YVYU},
            {.subtype = &DMOVideoFormat_RGB24},
            {.subtype = &DMOVideoFormat_RGB555},
            {.subtype = &DMOVideoFormat_RGB8},
            {.subtype = &MEDIASUBTYPE_V216},
            {.subtype = &MEDIASUBTYPE_V410},
            {.subtype = &MFVideoFormat_NV11},
            {.subtype = &MFVideoFormat_Y41P},
            {.subtype = &MFVideoFormat_Y41T},
            {.subtype = &MFVideoFormat_Y42T},
            {.subtype = &MFVideoFormat_YVU9},
        },
        .outputs =
        {
            {.subtype = &MFVideoFormat_YV12},
            {.subtype = &MFVideoFormat_YUY2},
            {.subtype = &MFVideoFormat_UYVY},
            {.subtype = &MFVideoFormat_AYUV},
            {.subtype = &MFVideoFormat_NV12},
            {.subtype = &DMOVideoFormat_RGB32},
            {.subtype = &DMOVideoFormat_RGB565},
            {.subtype = &MFVideoFormat_I420},
            {.subtype = &MFVideoFormat_IYUV},
            {.subtype = &MFVideoFormat_YVYU},
            {.subtype = &DMOVideoFormat_RGB24},
            {.subtype = &DMOVideoFormat_RGB555},
            {.subtype = &DMOVideoFormat_RGB8},
            {.subtype = &MEDIASUBTYPE_V216},
            {.subtype = &MEDIASUBTYPE_V410},
            {.subtype = &MFVideoFormat_NV11},
        },
    };
    const struct transform_info expect_dmo_info =
    {
        .name = L"Color Converter DMO",
        .major_type = &MEDIATYPE_Video,
        .inputs =
        {
            {.subtype = &MEDIASUBTYPE_YV12},
            {.subtype = &MEDIASUBTYPE_YUY2},
            {.subtype = &MEDIASUBTYPE_UYVY},
            {.subtype = &MEDIASUBTYPE_AYUV},
            {.subtype = &MEDIASUBTYPE_NV12},
            {.subtype = &MEDIASUBTYPE_RGB32},
            {.subtype = &MEDIASUBTYPE_RGB565},
            {.subtype = &MEDIASUBTYPE_I420},
            {.subtype = &MEDIASUBTYPE_IYUV},
            {.subtype = &MEDIASUBTYPE_YVYU},
            {.subtype = &MEDIASUBTYPE_RGB24},
            {.subtype = &MEDIASUBTYPE_RGB555},
            {.subtype = &MEDIASUBTYPE_RGB8},
            {.subtype = &MEDIASUBTYPE_V216},
            {.subtype = &MEDIASUBTYPE_V410},
            {.subtype = &MEDIASUBTYPE_NV11},
            {.subtype = &MEDIASUBTYPE_Y41P},
            {.subtype = &MEDIASUBTYPE_Y41T},
            {.subtype = &MEDIASUBTYPE_Y42T},
            {.subtype = &MEDIASUBTYPE_YVU9},
        },
        .outputs =
        {
            {.subtype = &MEDIASUBTYPE_YV12},
            {.subtype = &MEDIASUBTYPE_YUY2},
            {.subtype = &MEDIASUBTYPE_UYVY},
            {.subtype = &MEDIASUBTYPE_AYUV},
            {.subtype = &MEDIASUBTYPE_NV12},
            {.subtype = &MEDIASUBTYPE_RGB32},
            {.subtype = &MEDIASUBTYPE_RGB565},
            {.subtype = &MEDIASUBTYPE_I420},
            {.subtype = &MEDIASUBTYPE_IYUV},
            {.subtype = &MEDIASUBTYPE_YVYU},
            {.subtype = &MEDIASUBTYPE_RGB24},
            {.subtype = &MEDIASUBTYPE_RGB555},
            {.subtype = &MEDIASUBTYPE_RGB8},
            {.subtype = &MEDIASUBTYPE_V216},
            {.subtype = &MEDIASUBTYPE_V410},
            {.subtype = &MEDIASUBTYPE_NV11},
        },
    };

    static const media_type_desc expect_available_inputs[20] =
    {
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YV12), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_UYVY), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_AYUV), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB565), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_IYUV), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YVYU), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB555), },
        { ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_RGB8), },
        { ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_V216), },
        { ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_V410), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV11), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_Y41P), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_Y41T), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_Y42T), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YVU9), },
    };
    static const media_type_desc expect_available_outputs[16] =
    {
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YV12), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_UYVY), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_AYUV), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB565), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_IYUV), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YVYU), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB555), },
        { ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_RGB8), },
        { ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_V216), },
        { ATTR_GUID(MF_MT_SUBTYPE, MEDIASUBTYPE_V410), },
        { ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV11), },
    };
    static const media_type_desc expect_available_common =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
    };

    static const MFVideoArea actual_aperture = {.Area={82,84}};
    static const DWORD actual_width = 96, actual_height = 96;
    const struct attribute_desc input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
        {0},
    };
    const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        {0},
    };
    const struct attribute_desc output_type_desc_negative_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, -actual_width * 4),
        {0},
    };
    const struct attribute_desc output_type_desc_positive_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 4),
        {0},
    };
    const struct attribute_desc expect_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 3 / 2, .todo = TRUE),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1, .todo = TRUE),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1, .todo = TRUE),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1, .todo = TRUE),
        {0},
    };
    const struct attribute_desc expect_output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 4),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 4, .todo = TRUE),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1, .todo = TRUE),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1, .todo = TRUE),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1, .todo = TRUE),
        {0},
    };
    const struct attribute_desc expect_output_type_desc_negative_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, -actual_width * 4),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, actual_width * actual_height * 4, .todo = TRUE),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1, .todo = TRUE),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1, .todo = TRUE),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1, .todo = TRUE),
        {0},
    };
    const MFT_OUTPUT_STREAM_INFO output_info =
    {
        .cbSize = actual_width * actual_height * 4,
        .cbAlignment = 1,
    };
    const MFT_INPUT_STREAM_INFO input_info =
    {
        .cbSize = actual_width * actual_height * 3 / 2,
        .cbAlignment = 1,
    };

    const struct buffer_desc output_buffer_desc =
    {
        .length = actual_width * actual_height * 4,
        .compare = compare_rgb32, .compare_rect = {.right = 82, .bottom = 84},
        .dump = dump_rgb32, .size = {.cx = actual_width, .cy = actual_height},
    };
    const struct attribute_desc output_sample_attributes[] =
    {
        ATTR_UINT32(MFSampleExtension_CleanPoint, 0, .todo = TRUE),
        {0},
    };
    const struct sample_desc output_sample_desc =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 10000000,
        .buffer_count = 1, .buffers = &output_buffer_desc,
    };
    const struct transform_desc
    {
        const struct attribute_desc *output_type_desc;
        const struct attribute_desc *expect_output_type_desc;
        const WCHAR *result_bitmap;
        ULONG delta;
    }
    color_conversion_tests[] =
    {

        {
            /* YUV -> RGB */
            .output_type_desc = output_type_desc,
            .expect_output_type_desc = expect_output_type_desc,
            .result_bitmap = L"rgb32frame.bmp",
            .delta = 4, /* Windows return 0, Wine needs 4 */
        },

        {
            /* YUV -> RGB (negative stride) */
            .output_type_desc = output_type_desc_negative_stride,
            .expect_output_type_desc = expect_output_type_desc_negative_stride,
            .result_bitmap = L"rgb32frame-flip.bmp",
            .delta = 6,
        },

        {
            /* YUV -> RGB (positive stride) */
            .output_type_desc = output_type_desc_positive_stride,
            .expect_output_type_desc = expect_output_type_desc,
            .result_bitmap = L"rgb32frame.bmp",
            .delta = 4, /* Windows return 0, Wine needs 4 */
        },

    };

    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Video, MFVideoFormat_NV12};
    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Video, MFVideoFormat_I420};
    IMFSample *input_sample, *output_sample;
    IMFCollection *output_samples;
    DWORD length, output_status;
    const BYTE *nv12frame_data;
    ULONG nv12frame_data_len;
    IMFMediaType *media_type;
    IMFTransform *transform;
    ULONG i, ret, ref;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    winetest_push_context("colorconv");

    if (!check_mft_enum(MFT_CATEGORY_VIDEO_EFFECT, &input_type, &output_type, class_id))
        goto failed;
    check_mft_get_info(class_id, &expect_mft_info);
    check_dmo_get_info(class_id, &expect_dmo_info);

    if (FAILED(hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_IMediaObject, TRUE);
    check_interface(transform, &IID_IPropertyStore, TRUE);
    todo_wine
    check_interface(transform, &IID_IPropertyBag, FALSE);
    todo_wine
    check_interface(transform, &IID_IMFRealTimeClient, TRUE);
    /* check_interface(transform, &IID_IWMColorConvProps, TRUE); */

    check_mft_optional_methods(transform, 1);
    check_mft_get_attributes(transform, NULL, FALSE);
    check_mft_get_input_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);
    check_mft_get_output_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_common, -1);
        check_media_type(media_type, expect_available_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == 16, "%lu output media types\n", i);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetInputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("in %lu", i);
        ok(hr == S_OK, "GetInputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_common, -1);
        check_media_type(media_type, expect_available_inputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetInputAvailableType returned %#lx\n", hr);
    ok(i == 20, "%lu input media types\n", i);

    check_mft_set_input_type_required(transform, input_type_desc);
    check_mft_set_input_type(transform, input_type_desc, S_OK);
    check_mft_get_input_current_type_(__LINE__, transform, expect_input_type_desc, FALSE, TRUE);

    for (i = 0; i < ARRAY_SIZE(color_conversion_tests); i++)
    {
        winetest_push_context("color conversion #%lu", i);
        check_mft_set_output_type_required(transform, color_conversion_tests[i].output_type_desc);
        check_mft_set_output_type(transform, color_conversion_tests[i].output_type_desc, S_OK);
        check_mft_get_output_current_type_(__LINE__, transform, color_conversion_tests[i].expect_output_type_desc, FALSE, TRUE);

        check_mft_get_input_stream_info(transform, S_OK, &input_info);
        check_mft_get_output_stream_info(transform, S_OK, &output_info);

        load_resource(L"nv12frame.bmp", &nv12frame_data, &nv12frame_data_len);
        /* skip BMP header and RGB data from the dump */
        length = *(DWORD *)(nv12frame_data + 2);
        nv12frame_data_len = nv12frame_data_len - length;
        nv12frame_data = nv12frame_data + length;
        ok(nv12frame_data_len == 13824, "got length %lu\n", nv12frame_data_len);

        input_sample = create_sample(nv12frame_data, nv12frame_data_len);
        hr = IMFSample_SetSampleTime(input_sample, 0);
        ok(hr == S_OK, "SetSampleTime returned %#lx\n", hr);
        hr = IMFSample_SetSampleDuration(input_sample, 10000000);
        ok(hr == S_OK, "SetSampleDuration returned %#lx\n", hr);
        hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
        ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
        hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
        ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);
        hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, 0);
        ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);
        ret = IMFSample_Release(input_sample);
        ok(ret <= 1, "Release returned %ld\n", ret);

        hr = MFCreateCollection(&output_samples);
        ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

        output_sample = create_sample(NULL, output_info.cbSize);
        hr = check_mft_process_output(transform, output_sample, &output_status);
        ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
        ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
        hr = IMFCollection_AddElement(output_samples, (IUnknown *)output_sample);
        ok(hr == S_OK, "AddElement returned %#lx\n", hr);
        ref = IMFSample_Release(output_sample);
        ok(ref == 1, "Release returned %ld\n", ref);

        ret = check_mf_sample_collection(output_samples, &output_sample_desc, color_conversion_tests[i].result_bitmap);
        ok(ret <= color_conversion_tests[i].delta, "got %lu%% diff\n", ret);
        IMFCollection_Release(output_samples);

        output_sample = create_sample(NULL, output_info.cbSize);
        hr = check_mft_process_output(transform, output_sample, &output_status);
        ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
        ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
        hr = IMFSample_GetTotalLength(output_sample, &length);
        ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
        ok(length == 0, "got length %lu\n", length);
        ret = IMFSample_Release(output_sample);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }

    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %ld\n", ret);

failed:
    winetest_pop_context();
    CoUninitialize();
}

static void test_video_processor(BOOL use_2d_buffer)
{
    const GUID *const class_id = &CLSID_VideoProcessorMFT;
    const struct transform_info expect_mft_info =
    {
        .name = L"Microsoft Video Processor MFT",
        .major_type = &MFMediaType_Video,
        .inputs =
        {
            {.subtype = &MFVideoFormat_IYUV},
            {.subtype = &MFVideoFormat_YV12},
            {.subtype = &MFVideoFormat_NV12},
            {.subtype = &MFVideoFormat_YUY2},
            {.subtype = &MFVideoFormat_ARGB32},
            {.subtype = &MFVideoFormat_RGB32},
            {.subtype = &MFVideoFormat_NV11},
            {.subtype = &MFVideoFormat_AYUV},
            {.subtype = &MFVideoFormat_UYVY},
            {.subtype = &MEDIASUBTYPE_P208},
            {.subtype = &MFVideoFormat_RGB24},
            {.subtype = &MFVideoFormat_RGB555},
            {.subtype = &MFVideoFormat_RGB565},
            {.subtype = &MFVideoFormat_RGB8},
            {.subtype = &MFVideoFormat_I420},
            {.subtype = &MFVideoFormat_Y216},
            {.subtype = &MFVideoFormat_v410},
            {.subtype = &MFVideoFormat_Y41P},
            {.subtype = &MFVideoFormat_Y41T},
            {.subtype = &MFVideoFormat_Y42T},
            {.subtype = &MFVideoFormat_YVYU},
            {.subtype = &MFVideoFormat_420O},
        },
        .outputs =
        {
            {.subtype = &MFVideoFormat_IYUV},
            {.subtype = &MFVideoFormat_YV12},
            {.subtype = &MFVideoFormat_NV12},
            {.subtype = &MFVideoFormat_YUY2},
            {.subtype = &MFVideoFormat_ARGB32},
            {.subtype = &MFVideoFormat_RGB32},
            {.subtype = &MFVideoFormat_NV11},
            {.subtype = &MFVideoFormat_AYUV},
            {.subtype = &MFVideoFormat_UYVY},
            {.subtype = &MEDIASUBTYPE_P208},
            {.subtype = &MFVideoFormat_RGB24},
            {.subtype = &MFVideoFormat_RGB555},
            {.subtype = &MFVideoFormat_RGB565},
            {.subtype = &MFVideoFormat_RGB8},
            {.subtype = &MFVideoFormat_I420},
            {.subtype = &MFVideoFormat_Y216},
            {.subtype = &MFVideoFormat_v410},
            {.subtype = &MFVideoFormat_Y41P},
            {.subtype = &MFVideoFormat_Y41T},
            {.subtype = &MFVideoFormat_Y42T},
            {.subtype = &MFVideoFormat_YVYU},
        },
    };
    const struct input_type_desc
    {
        GUID guid;
        BOOL optional;
    }
    expect_available_inputs[] =
    {
        {MFVideoFormat_L8, .optional = TRUE /* >= W10 */},
        {MFVideoFormat_L16, .optional = TRUE /* >= W10 */},
        {MFAudioFormat_MPEG, .optional = TRUE /* >= W10 */},
        {MFVideoFormat_IYUV},
        {MFVideoFormat_YV12},
        {MFVideoFormat_NV12},
        {MFVideoFormat_NV21, .optional = TRUE /* >= W11 */},
        {MFVideoFormat_420O},
        {MFVideoFormat_P010, .optional = TRUE /* >= W10 */},
        {MFVideoFormat_P016, .optional = TRUE /* >= W10 */},
        {MFVideoFormat_UYVY},
        {MFVideoFormat_YUY2},
        {MFVideoFormat_P208},
        {MFVideoFormat_NV11},
        {MFVideoFormat_AYUV},
        {MFVideoFormat_ARGB32},
        {MFVideoFormat_ABGR32, .optional = TRUE /* >= W10 */},
        {MFVideoFormat_RGB32},
        {MFVideoFormat_A2R10G10B10, .optional = TRUE /* >= W10 */},
        {MFVideoFormat_A16B16G16R16F, .optional = TRUE /* >= W10 */},
        {MFVideoFormat_RGB24},
        {MFVideoFormat_I420},
        {MFVideoFormat_YVYU},
        {MFVideoFormat_RGB555},
        {MFVideoFormat_RGB565},
        {MFVideoFormat_RGB8},
        {MFVideoFormat_Y216},
        {MFVideoFormat_v410},
        {MFVideoFormat_Y41P},
        {MFVideoFormat_Y41T},
        {MFVideoFormat_Y42T},
    };
    const GUID expect_available_outputs[] =
    {
        MFVideoFormat_A2R10G10B10, /* enumerated with MFVideoFormat_P010 input */
        MFVideoFormat_P010, /* enumerated with MFVideoFormat_A2R10G10B10 input */
        MFVideoFormat_YUY2,
        MFVideoFormat_IYUV,
        MFVideoFormat_I420,
        MFVideoFormat_NV12,
        MFVideoFormat_RGB24,
        MFVideoFormat_ARGB32,
        MFVideoFormat_RGB32,
        MFVideoFormat_YV12,
        MFVideoFormat_Y216, /* enumerated with some input formats */
        MFVideoFormat_UYVY, /* enumerated with some input formats */
        MFVideoFormat_YVYU, /* enumerated with some input formats */
        MFVideoFormat_AYUV,
        MFVideoFormat_RGB555,
        MFVideoFormat_RGB565,
        MFVideoFormat_AYUV, /* some inputs enumerate MFVideoFormat_AYUV after RGB565 */
        MFVideoFormat_NV12, /* P010 enumerates NV12 after (A)RGB32 formats */
        MFVideoFormat_A16B16G16R16F, /* enumerated with MFVideoFormat_P010 input */
        MFVideoFormat_NV21, /* enumerated with some input formats */
    };
    static const media_type_desc expect_available_common =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
    };
    static const struct attribute_desc expect_transform_attributes[] =
    {
        ATTR_UINT32(MFT_SUPPORT_3DVIDEO, 1, .todo = TRUE),
        ATTR_UINT32(MF_SA_D3D11_AWARE, 1),
        /* ATTR_UINT32(MF_SA_D3D_AWARE, 1), only on W7 */
        {0},
    };

    const MFVideoArea actual_aperture = {.Area={82,84}};
    const DWORD actual_width = 96, actual_height = 96, nv12_aligned_width = 128;
    const DWORD extra_width = actual_width + 0x30;
    const DWORD nv12_aligned_extra_width = 192;
    const struct attribute_desc rgb32_with_aperture[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
        {0},
    };
    const struct attribute_desc rgb32_with_aperture_negative_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, -actual_width * 4),
        {0},
    };
    const struct attribute_desc rgb32_with_aperture_positive_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 4),
        {0},
    };
    const struct attribute_desc nv12_default_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        {0},
    };
    const struct attribute_desc nv12_extra_width[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, extra_width, actual_height, .required = TRUE),
        {0},
    };
    const struct attribute_desc rgb32_default_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        {0},
    };
    const struct attribute_desc rgb32_extra_width[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, extra_width, actual_height, .required = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, extra_width * 4),
        {0},
    };
    const struct attribute_desc rgb32_negative_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, -actual_width * 4),
        {0},
    };
    const struct attribute_desc rgb32_positive_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 4),
        {0},
    };
    const struct attribute_desc rgb555_default_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB555, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        {0},
    };
    const struct attribute_desc rgb555_negative_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB555, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, -actual_width * 2),
        {0},
    };
    const struct attribute_desc rgb555_positive_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB555, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, actual_width * 2),
        {0},
    };
    const struct attribute_desc nv12_no_aperture[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 82, 84),
        {0},
    };
    const struct attribute_desc nv12_with_aperture[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, actual_width, actual_height, .required = TRUE),
        ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &actual_aperture, 16),
        ATTR_BLOB(MF_MT_GEOMETRIC_APERTURE, &actual_aperture, 16),
        ATTR_BLOB(MF_MT_PAN_SCAN_APERTURE, &actual_aperture, 16),
        {0},
    };
    const struct attribute_desc rgb32_no_aperture[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 82, 84, .required = TRUE),
        {0},
    };
    const struct attribute_desc rgb32_no_aperture_negative_stride[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32, .required = TRUE),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 82, 84, .required = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, -82 * 4),
        {0},
    };

    const MFT_OUTPUT_STREAM_INFO initial_output_info = {0};
    const MFT_INPUT_STREAM_INFO initial_input_info = {0};
    MFT_OUTPUT_STREAM_INFO output_info = {0};
    MFT_INPUT_STREAM_INFO input_info = {0};

    const struct attribute_desc output_sample_attributes[] =
    {
        ATTR_UINT32(MFSampleExtension_CleanPoint, 1, .todo = TRUE),
        {0},
    };
    const struct buffer_desc rgb32_buffer_desc =
    {
        .length = actual_width * actual_height * 4,
        .compare = compare_rgb32, .compare_rect = {.top = 12, .right = 82, .bottom = 96},
        .dump = dump_rgb32, .size = {.cx = actual_width, .cy = actual_height},
    };
    const struct sample_desc rgb32_sample_desc =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 10000000,
        .buffer_count = 1, .buffers = &rgb32_buffer_desc,
    };

    const struct buffer_desc rgb32_crop_buffer_desc =
    {
        .length = actual_aperture.Area.cx * actual_aperture.Area.cy * 4,
        .compare = compare_rgb32, .compare_rect = {.right = actual_aperture.Area.cx, .bottom = actual_aperture.Area.cy},
        .dump = dump_rgb32, .size = actual_aperture.Area,
    };
    const struct sample_desc rgb32_crop_sample_desc =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 10000000,
        .buffer_count = 1, .buffers = &rgb32_crop_buffer_desc,
    };
    const struct buffer_desc rgb32_crop_buffer_2d_desc =
    {
        .length = actual_width * actual_aperture.Area.cy * 4,
        .compare = compare_rgb32, .compare_rect = {.right = actual_aperture.Area.cx, .bottom = actual_aperture.Area.cy},
        .dump = dump_rgb32, .size = {.cx = actual_width, .cy = actual_aperture.Area.cy},
    };
    const struct sample_desc rgb32_crop_sample_2d_desc =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 10000000,
        .buffer_count = 1, .buffers = &rgb32_crop_buffer_2d_desc,
    };
    const struct buffer_desc rgb32_extra_width_buffer_desc =
    {
        .length = extra_width * actual_height * 4,
        .compare = compare_rgb32, .compare_rect = {.top = 12, .right = 82, .bottom = 96},
        .dump = dump_rgb32, .size = {.cx = extra_width, .cy = actual_height},
    };
    const struct sample_desc rgb32_extra_width_sample_desc =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 10000000,
        .total_length = actual_width * actual_height * 4,
        .buffer_count = 1, .buffers = &rgb32_extra_width_buffer_desc,
    };

    const struct buffer_desc rgb555_buffer_desc =
    {
        .length = actual_width * actual_height * 2,
        .compare = compare_rgb16, .compare_rect = {.top = 12, .right = 82, .bottom = 96},
        .dump = dump_rgb16, .size = {.cx = actual_width, .cy = actual_height},
    };
    const struct sample_desc rgb555_sample_desc =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 10000000,
        .buffer_count = 1, .buffers = &rgb555_buffer_desc,
    };

    const struct buffer_desc nv12_buffer_desc =
    {
        .length = actual_width * actual_height * 3 / 2,
        .compare = compare_nv12, .compare_rect = {.top = 12, .right = 82, .bottom = 96},
        .dump = dump_nv12, .size = {.cx = actual_width, .cy = actual_height},
    };
    const struct sample_desc nv12_sample_desc =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 10000000,
        .buffer_count = 1, .buffers = &nv12_buffer_desc,
    };
    const struct buffer_desc nv12_buffer_2d_desc =
    {
        .length = nv12_aligned_width * actual_height * 3 / 2,
        .compare = compare_nv12, .compare_rect = {.top = 12, .right = 82, .bottom = 96},
        .dump = dump_nv12, .size = {.cx = nv12_aligned_width, .cy = actual_height},
    };
    const struct sample_desc nv12_sample_2d_desc =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 10000000,
        .buffer_count = 1, .buffers = &nv12_buffer_2d_desc,
    };

    const struct buffer_desc nv12_extra_width_buffer_desc =
    {
        .length = extra_width * actual_height * 3 / 2,
        .compare = compare_nv12, .compare_rect = {.top = 12, .right = 82, .bottom = 96},
        .dump = dump_nv12, .size = {.cx = extra_width, .cy = actual_height},
    };
    const struct sample_desc nv12_extra_width_sample_desc =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 10000000,
        .total_length = actual_width * actual_height * 3 / 2,
        .buffer_count = 1, .buffers = &nv12_extra_width_buffer_desc,
    };
    const struct buffer_desc nv12_extra_width_buffer_2d_desc =
    {
        .length = nv12_aligned_extra_width * actual_height * 3 / 2,
        .compare = compare_nv12, .compare_rect = {.top = 12, .right = 82, .bottom = 96},
        .dump = dump_nv12, .size = {.cx = nv12_aligned_extra_width, .cy = actual_height},
    };
    const struct sample_desc nv12_extra_width_sample_2d_desc =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 10000000,
        .buffer_count = 1, .buffers = &nv12_extra_width_buffer_2d_desc,
    };

    const struct transform_desc
    {
        const struct attribute_desc *input_type_desc;
        const struct attribute_desc *input_buffer_desc;
        const WCHAR *input_bitmap;
        const struct attribute_desc *output_type_desc;
        const struct attribute_desc *output_buffer_desc;
        const struct sample_desc *output_sample_desc;
        const struct sample_desc *output_sample_2d_desc;
        const WCHAR *output_bitmap;
        const WCHAR *output_bitmap_1d;
        const WCHAR *output_bitmap_2d;
        ULONG delta;
        BOOL broken;
        BOOL todo;
    }
    video_processor_tests[] =
    {
        { /* Test 0 */
            .input_type_desc = nv12_default_stride, .input_bitmap = L"nv12frame.bmp",
            .input_buffer_desc = use_2d_buffer ? nv12_default_stride : NULL,
            .output_type_desc = rgb32_default_stride, .output_bitmap = L"rgb32frame-flip.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb32_negative_stride : NULL,
            .output_sample_desc = &rgb32_sample_desc, .output_sample_2d_desc = &rgb32_sample_desc,
            .delta = 2, /* Windows returns 0, Wine needs 2 */
        },
        { /* Test 1 */
            .input_type_desc = nv12_default_stride, .input_bitmap = L"nv12frame.bmp",
            .input_buffer_desc = use_2d_buffer ? nv12_default_stride : NULL,
            .output_type_desc = rgb32_negative_stride, .output_bitmap = L"rgb32frame-flip.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb32_negative_stride : NULL,
            .output_sample_desc = &rgb32_sample_desc, .output_sample_2d_desc = &rgb32_sample_desc,
            .delta = 2, /* Windows returns 0, Wine needs 2 */
        },
        { /* Test 2 */
            .input_type_desc = nv12_default_stride, .input_bitmap = L"nv12frame.bmp",
            .input_buffer_desc = use_2d_buffer ? nv12_default_stride : NULL,
            .output_type_desc = rgb32_positive_stride, .output_bitmap = L"rgb32frame.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb32_positive_stride : NULL,
            .output_sample_desc = &rgb32_sample_desc, .output_sample_2d_desc = &rgb32_sample_desc,
            .delta = 6,
        },
        { /* Test 3 */
            .input_type_desc = rgb32_default_stride, .input_bitmap = L"rgb32frame.bmp",
            .input_buffer_desc = use_2d_buffer ? rgb32_negative_stride : NULL,
            .output_type_desc = nv12_default_stride, .output_bitmap = L"nv12frame-flip.bmp", .output_bitmap_2d = L"nv12frame-flip-2d.bmp",
            .output_buffer_desc = use_2d_buffer ? nv12_default_stride : NULL,
            .output_sample_desc = &nv12_sample_desc, .output_sample_2d_desc = &nv12_sample_2d_desc,
            .delta = 2, /* Windows returns 0, Wine needs 2 */
        },
        { /* Test 4 */
            .input_type_desc = rgb32_negative_stride, .input_bitmap = L"rgb32frame.bmp",
            .input_buffer_desc = use_2d_buffer ? rgb32_negative_stride : NULL,
            .output_type_desc = nv12_default_stride, .output_bitmap = L"nv12frame-flip.bmp", .output_bitmap_2d = L"nv12frame-flip-2d.bmp",
            .output_buffer_desc = use_2d_buffer ? nv12_default_stride : NULL,
            .output_sample_desc = &nv12_sample_desc, .output_sample_2d_desc = &nv12_sample_2d_desc,
            .delta = 2, /* Windows returns 0, Wine needs 2 */
        },
        { /* Test 5 */
            .input_type_desc = rgb32_positive_stride, .input_bitmap = L"rgb32frame.bmp",
            .input_buffer_desc = use_2d_buffer ? rgb32_positive_stride : NULL,
            .output_type_desc = nv12_default_stride, .output_bitmap = L"nv12frame.bmp", .output_bitmap_2d = L"nv12frame-2d.bmp",
            .output_buffer_desc = use_2d_buffer ? nv12_default_stride : NULL,
            .output_sample_desc = &nv12_sample_desc, .output_sample_2d_desc = &nv12_sample_2d_desc,
            .delta = 2, /* Windows returns 1, Wine needs 2 */
        },
        { /* Test 6 */
            .input_type_desc = rgb32_negative_stride, .input_bitmap = L"rgb32frame.bmp",
            .input_buffer_desc = use_2d_buffer ? rgb32_negative_stride : NULL,
            .output_type_desc = rgb32_negative_stride, .output_bitmap = L"rgb32frame.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb32_negative_stride : NULL,
            .output_sample_desc = &rgb32_sample_desc, .output_sample_2d_desc = &rgb32_sample_desc,
        },
        { /* Test 7 */
            .input_type_desc = rgb32_negative_stride, .input_bitmap = L"rgb32frame.bmp",
            .input_buffer_desc = use_2d_buffer ? rgb32_negative_stride : NULL,
            .output_type_desc = rgb32_positive_stride, .output_bitmap = L"rgb32frame-flip.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb32_positive_stride : NULL,
            .output_sample_desc = &rgb32_sample_desc, .output_sample_2d_desc = &rgb32_sample_desc,
            .delta = 3, /* Windows returns 3 */
        },
        { /* Test 8 */
            .input_type_desc = rgb32_positive_stride, .input_bitmap = L"rgb32frame.bmp",
            .input_buffer_desc = use_2d_buffer ? rgb32_positive_stride : NULL,
            .output_type_desc = rgb32_negative_stride, .output_bitmap = L"rgb32frame-flip.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb32_negative_stride : NULL,
            .output_sample_desc = &rgb32_sample_desc, .output_sample_2d_desc = &rgb32_sample_desc,
            .delta = 3, /* Windows returns 3 */
        },
        { /* Test 9 */
            .input_type_desc = rgb32_positive_stride, .input_bitmap = L"rgb32frame.bmp",
            .input_buffer_desc = use_2d_buffer ? rgb32_positive_stride : NULL,
            .output_type_desc = rgb32_positive_stride, .output_bitmap = L"rgb32frame.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb32_positive_stride : NULL,
            .output_sample_desc = &rgb32_sample_desc, .output_sample_2d_desc = &rgb32_sample_desc,
        },
        { /* Test 10 */
            .input_type_desc = rgb32_with_aperture, .input_bitmap = L"rgb32frame.bmp",
            .input_buffer_desc = use_2d_buffer ? rgb32_with_aperture : NULL,
            .output_type_desc = rgb32_with_aperture, .output_bitmap = L"rgb32frame.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb32_with_aperture : NULL,
            .output_sample_desc = &rgb32_sample_desc, .output_sample_2d_desc = &rgb32_sample_desc,
            .broken = TRUE, /* old Windows version incorrectly rescale */
        },
        { /* Test 11 */
            .input_type_desc = rgb32_default_stride, .input_bitmap = L"rgb32frame.bmp",
            .input_buffer_desc = use_2d_buffer ? rgb32_negative_stride : NULL,
            .output_type_desc = rgb555_default_stride, .output_bitmap = L"rgb555frame.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb555_negative_stride : NULL,
            .output_sample_desc = &rgb555_sample_desc, .output_sample_2d_desc = &rgb555_sample_desc,
            .delta = 5, /* Windows returns 0, Wine needs 5 */
        },
        { /* Test 12 */
            .input_type_desc = rgb32_default_stride, .input_bitmap = L"rgb32frame.bmp",
            .input_buffer_desc = use_2d_buffer ? rgb32_negative_stride : NULL,
            .output_type_desc = rgb555_negative_stride, .output_bitmap = L"rgb555frame.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb555_negative_stride : NULL,
            .output_sample_desc = &rgb555_sample_desc, .output_sample_2d_desc = &rgb555_sample_desc,
            .delta = 5, /* Windows returns 0, Wine needs 5 */
        },
        { /* Test 13 */
            .input_type_desc = rgb32_default_stride, .input_bitmap = L"rgb32frame.bmp",
            .input_buffer_desc = use_2d_buffer ? rgb32_negative_stride : NULL,
            .output_type_desc = rgb555_positive_stride, .output_bitmap = L"rgb555frame-flip.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb555_positive_stride : NULL,
            .output_sample_desc = &rgb555_sample_desc, .output_sample_2d_desc = &rgb555_sample_desc,
            .delta = 3, /* Windows returns 0, Wine needs 3 */
        },
        { /* Test 14 */
            .input_type_desc = rgb555_default_stride, .input_bitmap = L"rgb555frame.bmp",
            .input_buffer_desc = use_2d_buffer ? rgb555_negative_stride : NULL,
            .output_type_desc = rgb555_positive_stride, .output_bitmap = L"rgb555frame-flip.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb555_positive_stride : NULL,
            .output_sample_desc = &rgb555_sample_desc, .output_sample_2d_desc = &rgb555_sample_desc,
            .delta = 4, /* Windows returns 0, Wine needs 4 */
        },
        { /* Test 15 */
            .input_type_desc = nv12_with_aperture, .input_bitmap = L"nv12frame.bmp",
            .input_buffer_desc = use_2d_buffer ? nv12_with_aperture : NULL,
            .output_type_desc = rgb32_no_aperture, .output_bitmap = L"rgb32frame-crop-flip.bmp", .output_bitmap_2d = L"rgb32frame-crop-flip-2d.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb32_no_aperture_negative_stride : NULL,
            .output_sample_desc = &rgb32_crop_sample_desc, .output_sample_2d_desc = &rgb32_crop_sample_2d_desc,
            .delta = 3, /* Windows returns 3 with 2D buffer */
        },
        { /* Test 16 */
            .input_type_desc = rgb32_no_aperture, .input_bitmap = L"rgb32frame-crop-flip.bmp",
            .input_buffer_desc = use_2d_buffer ? rgb32_no_aperture : NULL,
            .output_type_desc = rgb32_with_aperture, .output_bitmap = L"rgb32frame-flip.bmp", .output_bitmap_1d = L"rgb32frame-flip-2d.bmp", .output_bitmap_2d = L"rgb32frame-flip-2d.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb32_with_aperture : NULL,
            .output_sample_desc = &rgb32_sample_desc, .output_sample_2d_desc = &rgb32_sample_desc,
            .broken = TRUE, /* old Windows version incorrectly rescale */
            .todo = use_2d_buffer,
        },
        { /* Test 17 */
            .input_type_desc = rgb32_with_aperture, .input_bitmap = L"rgb32frame-flip.bmp",
            .input_buffer_desc = use_2d_buffer ? rgb32_with_aperture_negative_stride : NULL,
            .output_type_desc = rgb32_no_aperture, .output_bitmap = L"rgb32frame-crop-flip.bmp", .output_bitmap_2d = L"rgb32frame-crop-flip-2d.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb32_no_aperture_negative_stride : NULL,
            .output_sample_desc = &rgb32_crop_sample_desc, .output_sample_2d_desc = &rgb32_crop_sample_2d_desc,
            .delta = 3, /* Windows returns 3 with 2D buffer */
        },
        { /* Test 18 */
            .input_type_desc = rgb32_with_aperture_positive_stride, .input_bitmap = L"rgb32frame.bmp",
            .input_buffer_desc = use_2d_buffer ? rgb32_with_aperture_positive_stride : NULL,
            .output_type_desc = rgb32_no_aperture, .output_bitmap = L"rgb32frame-crop-flip.bmp", .output_bitmap_2d = L"rgb32frame-crop-flip-2d.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb32_no_aperture_negative_stride : NULL,
            .output_sample_desc = &rgb32_crop_sample_desc, .output_sample_2d_desc = &rgb32_crop_sample_2d_desc,
            .delta = 3, /* Windows returns 3 */
        },
        { /* Test 19 */
            .input_type_desc = nv12_default_stride, .input_bitmap = L"nv12frame.bmp",
            .input_buffer_desc = use_2d_buffer ? nv12_default_stride : NULL,
            .output_type_desc = rgb32_default_stride, .output_bitmap = L"rgb32frame-flip.bmp", .output_bitmap_2d = L"rgb32frame.bmp", .output_bitmap_1d = L"rgb32frame.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb32_default_stride : NULL,
            .output_sample_desc = &rgb32_sample_desc, .output_sample_2d_desc = &rgb32_sample_desc,
            .delta = 3, /* Windows returns 3 with 2D buffer */
            .todo = use_2d_buffer,
        },
        { /* Test 20 */
            .input_type_desc = nv12_default_stride, .input_bitmap = L"nv12frame.bmp",
            .input_buffer_desc = use_2d_buffer ? nv12_default_stride : NULL,
            .output_type_desc = rgb32_default_stride, .output_bitmap = L"rgb32frame-flip.bmp", .output_bitmap_2d = L"rgb32frame.bmp", .output_bitmap_1d = L"rgb32frame.bmp",
            .output_buffer_desc = use_2d_buffer ? rgb32_positive_stride : NULL,
            .output_sample_desc = &rgb32_sample_desc, .output_sample_2d_desc = &rgb32_sample_desc,
            .delta = 3, /* Windows returns 3 with 2D buffer */
            .todo = use_2d_buffer,
        },

        { /* Test 21, 2D only */
            .input_type_desc = nv12_default_stride, .input_bitmap = L"nv12frame.bmp",
            .input_buffer_desc = nv12_default_stride,
            .output_type_desc = rgb32_default_stride, .output_bitmap = L"rgb32frame-extra-width.bmp",
            .output_buffer_desc = rgb32_extra_width,
            .output_sample_desc = &rgb32_extra_width_sample_desc, .output_sample_2d_desc = &rgb32_extra_width_sample_desc,
            .todo = TRUE,
        },
        { /* Test 22, 2D only */
            .input_type_desc = rgb32_default_stride, .input_bitmap = L"rgb32frame-extra-width.bmp",
            .input_buffer_desc = rgb32_extra_width,
            .output_type_desc = nv12_default_stride, .output_bitmap_1d = L"nv12frame.bmp", .output_bitmap_2d = L"nv12frame-2d.bmp",
            .output_buffer_desc = nv12_default_stride,
            .output_sample_desc = &nv12_sample_desc, .output_sample_2d_desc = &nv12_sample_2d_desc,
            .delta = 2, /* Windows returns 2 with 1D buffer */ .todo = TRUE,
        },
        { /* Test 23, 2D only */
            .input_type_desc = rgb32_default_stride, .input_bitmap = L"rgb32frame-extra-width.bmp",
            .input_buffer_desc = rgb32_extra_width,
            .output_type_desc = nv12_default_stride, .output_bitmap_1d = L"nv12frame-extra-width.bmp", .output_bitmap_2d = L"nv12frame-extra-width-2d.bmp",
            .output_buffer_desc = nv12_extra_width,
            .output_sample_desc = &nv12_extra_width_sample_desc, .output_sample_2d_desc = &nv12_extra_width_sample_2d_desc,
            .todo = TRUE,
        },
        { /* Test 24, 2D only */
            .input_type_desc = nv12_default_stride, .input_bitmap = L"nv12frame-extra-width.bmp",
            .input_buffer_desc = nv12_extra_width,
            .output_type_desc = rgb32_default_stride, .output_bitmap_1d = L"rgb32frame.bmp", .output_bitmap_2d = L"rgb32frame.bmp",
            .output_buffer_desc = rgb32_default_stride,
            .output_sample_desc = &rgb32_sample_desc, .output_sample_2d_desc = &rgb32_sample_desc,
            .todo = TRUE,
        },
    };

    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Video, MFVideoFormat_NV12};
    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Video, MFVideoFormat_I420};
    const struct input_type_desc *expect_input = expect_available_inputs;
    DWORD i, j, k, flags, length, output_status;
    IMFSample *input_sample, *output_sample;
    IMFMediaType *media_type, *media_type2;
    IMFCollection *output_samples;
    const WCHAR *output_bitmap;
    IMFTransform *transform;
    IMFMediaBuffer *buffer;
    const BYTE *input_data;
    ULONG input_data_len;
    UINT32 count;
    HRESULT hr;
    ULONG ret;
    GUID guid;
    LONG ref;

    if (use_2d_buffer && !pMFCreateMediaBufferFromMediaType)
    {
        win_skip("MFCreateMediaBufferFromMediaType() is unsupported.\n");
        return;
    }

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    winetest_push_context("videoproc %s", use_2d_buffer ? "2d" : "1d");

    if (!check_mft_enum(MFT_CATEGORY_VIDEO_PROCESSOR, &input_type, &output_type, class_id))
        goto failed;
    check_mft_get_info(class_id, &expect_mft_info);

    if (FAILED(hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;

    todo_wine
    check_interface(transform, &IID_IMFVideoProcessorControl, TRUE);
    todo_wine
    check_interface(transform, &IID_IMFRealTimeClientEx, TRUE);
    check_interface(transform, &IID_IMFMediaEventGenerator, FALSE);
    check_interface(transform, &IID_IMFShutdown, FALSE);

    hr = IMFTransform_GetInputStatus(transform, 0, &flags);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputStatus(transform, &flags);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);

    check_mft_get_input_current_type(transform, NULL);
    check_mft_get_output_current_type(transform, NULL);

    check_mft_get_input_stream_info(transform, S_OK, &initial_input_info);
    check_mft_get_output_stream_info(transform, S_OK, &initial_output_info);

    /* Configure stream types. */
    for (i = 0;;++i)
    {
        if (FAILED(hr = IMFTransform_GetInputAvailableType(transform, 0, i, &media_type)))
        {
            ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);
            break;
        }

        hr = IMFTransform_GetInputAvailableType(transform, 0, i, &media_type2);
        ok(hr == S_OK, "Failed to get available type, hr %#lx.\n", hr);
        ok(media_type != media_type2, "Unexpected instance.\n");
        ref = IMFMediaType_Release(media_type2);
        ok(ref == 0, "Release returned %ld\n", ref);

        hr = IMFMediaType_GetMajorType(media_type, &guid);
        ok(hr == S_OK, "Failed to get major type, hr %#lx.\n", hr);
        ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected major type.\n");

        hr = IMFMediaType_GetCount(media_type, &count);
        ok(hr == S_OK, "Failed to get attributes count, hr %#lx.\n", hr);
        ok(count == 2, "Unexpected count %u.\n", count);

        hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
        ok(hr == S_OK, "Failed to get subtype, hr %#lx.\n", hr);
        ok(is_supported_video_type(&guid), "Unexpected media type %s.\n", wine_dbgstr_guid(&guid));

        hr = IMFTransform_SetInputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
        ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

        hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
        ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

        hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type2);
        ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

        /* FIXME: figure out if those require additional attributes or simply advertised but not supported */
        if (IsEqualGUID(&guid, &MFVideoFormat_L8) || IsEqualGUID(&guid, &MFVideoFormat_L16)
                || IsEqualGUID(&guid, &MFVideoFormat_D16) || IsEqualGUID(&guid, &MFVideoFormat_420O)
                || IsEqualGUID(&guid, &MFVideoFormat_A16B16G16R16F))
        {
            ref = IMFMediaType_Release(media_type);
            ok(ref == 0, "Release returned %ld\n", ref);
            continue;
        }

        hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, ((UINT64)16 << 32) | 16);
        ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

        hr = IMFTransform_SetInputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
        ok(hr == S_OK, "Failed to test input type %s, hr %#lx.\n", wine_dbgstr_guid(&guid), hr);

        hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
        ok(hr == S_OK, "Failed to test input type, hr %#lx.\n", hr);

        hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type2);
        ok(hr == S_OK, "Failed to get current type, hr %#lx.\n", hr);
        ok(media_type != media_type2, "Unexpected instance.\n");
        IMFMediaType_Release(media_type2);

        hr = IMFTransform_GetInputStatus(transform, 0, &flags);
        ok(hr == S_OK, "Failed to get input status, hr %#lx.\n", hr);
        ok(flags == MFT_INPUT_STATUS_ACCEPT_DATA, "Unexpected input status %#lx.\n", flags);

        input_info.cbSize = 0;
        if (IsEqualGUID(&guid, &MFVideoFormat_NV21))
            input_info.cbSize = 0x180;
        else if (!IsEqualGUID(&guid, &MFVideoFormat_P208) && !IsEqualGUID(&guid, &MEDIASUBTYPE_Y41T)
                && !IsEqualGUID(&guid, &MEDIASUBTYPE_Y42T))
        {
            hr = MFCalculateImageSize(&guid, 16, 16, (UINT32 *)&input_info.cbSize);
            todo_wine_if(IsEqualGUID(&guid, &MFVideoFormat_Y216)
                    || IsEqualGUID(&guid, &MFVideoFormat_v410)
                    || IsEqualGUID(&guid, &MFVideoFormat_Y41P))
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        }
        check_mft_get_input_stream_info(transform, S_OK, &input_info);
        check_mft_get_output_stream_info(transform, S_OK, &initial_output_info);

        IMFMediaType_Release(media_type);
    }

    /* IYUV -> RGB32 */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_IYUV);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, ((UINT64)16 << 32) | 16);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "Failed to set input type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "Failed to set output type, hr %#lx.\n", hr);

    hr = MFCalculateImageSize(&MFVideoFormat_IYUV, 16, 16, (UINT32 *)&input_info.cbSize);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCalculateImageSize(&MFVideoFormat_RGB32, 16, 16, (UINT32 *)&output_info.cbSize);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_mft_get_input_stream_info(transform, S_OK, &input_info);
    check_mft_get_output_stream_info(transform, S_OK, &output_info);

    hr = MFCreateSample(&input_sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#lx.\n", hr);

    hr = MFCreateSample(&output_sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#lx.\n", hr);

    hr = check_mft_process_output(transform, output_sample, &output_status);
    todo_wine
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    todo_wine
    ok(hr == S_OK, "Failed to push a sample, hr %#lx.\n", hr);

    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    todo_wine
    ok(hr == MF_E_NOTACCEPTING, "Unexpected hr %#lx.\n", hr);

    hr = check_mft_process_output(transform, output_sample, &output_status);
    todo_wine
    ok(hr == MF_E_NO_SAMPLE_TIMESTAMP, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_SetSampleTime(input_sample, 0);
    ok(hr == S_OK, "Failed to set sample time, hr %#lx.\n", hr);
    hr = check_mft_process_output(transform, output_sample, &output_status);
    todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMemoryBuffer(1024 * 1024, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(input_sample, buffer);
    ok(hr == S_OK, "Failed to add a buffer, hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(output_sample, buffer);
    ok(hr == S_OK, "Failed to add a buffer, hr %#lx.\n", hr);

    hr = check_mft_process_output(transform, output_sample, &output_status);
    todo_wine
    ok(hr == S_OK || broken(FAILED(hr)) /* Win8 */, "Failed to get output buffer, hr %#lx.\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = check_mft_process_output(transform, output_sample, &output_status);
        ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "Unexpected hr %#lx.\n", hr);
    }

    ref = IMFTransform_Release(transform);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFMediaType_Release(media_type);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFSample_Release(input_sample);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFSample_Release(output_sample);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaBuffer_Release(buffer);
    ok(ref == 0, "Release returned %ld\n", ref);


    hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_IMediaObject, FALSE);
    check_interface(transform, &IID_IPropertyStore, FALSE);
    check_interface(transform, &IID_IPropertyBag, FALSE);

    check_mft_optional_methods(transform, 1);
    check_mft_get_attributes(transform, expect_transform_attributes, TRUE);
    check_mft_get_input_stream_info(transform, S_OK, &initial_input_info);
    check_mft_get_output_stream_info(transform, S_OK, &initial_output_info);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetInputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("in %lu", i);

        ok(hr == S_OK, "GetInputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_common, -1);

        hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
        ok(hr == S_OK, "GetGUID returned %#lx\n", hr);

        while (!IsEqualGUID(&expect_input->guid, &guid))
        {
            if (!expect_input->optional)
                break;
            expect_input++;
        }
        ok(IsEqualGUID(&expect_input->guid, &guid), "got subtype %s\n", debugstr_guid(&guid));
        expect_input++;

        /* FIXME: Skip exotic input types which aren't directly accepted */
        if (IsEqualGUID(&guid, &MFVideoFormat_L8) || IsEqualGUID(&guid, &MFVideoFormat_L16)
                || IsEqualGUID(&guid, &MFAudioFormat_MPEG) || IsEqualGUID(&guid, &MFVideoFormat_420O)
                || IsEqualGUID(&guid, &MFVideoFormat_A16B16G16R16F) /* w1064v1507 */)
        {
            IMFMediaType_Release(media_type);
            winetest_pop_context();
            continue;
        }

        hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
        ok(hr == MF_E_ATTRIBUTENOTFOUND, "SetInputType returned %#lx.\n", hr);
        hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)actual_width << 32 | actual_height);
        ok(hr == S_OK, "SetUINT64 returned %#lx.\n", hr);
        hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
        ok(hr == S_OK, "SetInputType returned %#lx.\n", hr);

        hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type2);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx.\n", hr);
        hr = IMFMediaType_IsEqual(media_type, media_type2, &flags);
        ok(hr == S_OK, "IsEqual returned %#lx.\n", hr);
        IMFMediaType_Release(media_type2);

        ret = IMFMediaType_Release(media_type);
        ok(ret == 1, "Release returned %lu\n", ret);

        j = k = 0;
        while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++j, &media_type)))
        {
            winetest_push_context("out %lu", j);
            ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
            check_media_type(media_type, expect_available_common, -1);

            hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
            ok(hr == S_OK, "GetGUID returned %#lx\n", hr);

            for (; k < ARRAY_SIZE(expect_available_outputs); k++)
                if (IsEqualGUID(&expect_available_outputs[k], &guid))
                    break;
            todo_wine_if(IsEqualGUID(&guid, &MFVideoFormat_ABGR32)) /* enumerated on purpose on Wine */
            ok(k < ARRAY_SIZE(expect_available_outputs), "got subtype %s\n", debugstr_guid(&guid));

            ret = IMFMediaType_Release(media_type);
            ok(ret == 0, "Release returned %lu\n", ret);
            winetest_pop_context();
        }
        ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);

        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetInputAvailableType returned %#lx\n", hr);

    /* MFVideoFormat_ABGR32 isn't supported by the video processor in non-D3D mode */
    check_mft_set_input_type(transform, nv12_default_stride, S_OK);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_ABGR32);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    todo_wine ok(hr == MF_E_INVALIDMEDIATYPE, "got %#lx\n", hr);
    IMFMediaType_Release(media_type);

    /* MFVideoFormat_RGB32 output format works */

    check_mft_set_output_type(transform, rgb32_default_stride, S_OK);
    check_mft_get_output_current_type(transform, rgb32_default_stride);

    for (i = 0; i < ARRAY_SIZE(video_processor_tests); i++)
    {
        const struct transform_desc *test = video_processor_tests + i;

        /* skip tests which require 2D buffers when not testing them */
        if (!use_2d_buffer && test->input_buffer_desc)
            continue;

        winetest_push_context("transform #%lu", i);

        check_mft_set_input_type_required(transform, test->input_type_desc);
        check_mft_set_input_type(transform, test->input_type_desc, S_OK);
        check_mft_get_input_current_type(transform, test->input_type_desc);

        check_mft_set_output_type_required(transform, test->output_type_desc);
        check_mft_set_output_type(transform, test->output_type_desc, S_OK);
        check_mft_get_output_current_type(transform, test->output_type_desc);

        if (test->output_sample_desc == &nv12_sample_desc
                || test->output_sample_desc == &nv12_extra_width_sample_desc)
        {
            output_info.cbSize = actual_width * actual_height * 3 / 2;
            check_mft_get_output_stream_info(transform, S_OK, &output_info);
        }
        else if (test->output_sample_desc == &rgb555_sample_desc)
        {
            output_info.cbSize = actual_width * actual_height * 2;
            check_mft_get_output_stream_info(transform, S_OK, &output_info);
        }
        else if (test->output_sample_desc == &rgb32_crop_sample_desc)
        {
            output_info.cbSize = actual_aperture.Area.cx * actual_aperture.Area.cy * 4;
            check_mft_get_output_stream_info(transform, S_OK, &output_info);
        }
        else
        {
            output_info.cbSize = actual_width * actual_height * 4;
            check_mft_get_output_stream_info(transform, S_OK, &output_info);
        }

        if (test->input_type_desc == nv12_default_stride || test->input_type_desc == nv12_with_aperture)
        {
            input_info.cbSize = actual_width * actual_height * 3 / 2;
            check_mft_get_input_stream_info(transform, S_OK, &input_info);
        }
        else if (test->input_type_desc == rgb555_default_stride)
        {
            input_info.cbSize = actual_width * actual_height * 2;
            check_mft_get_input_stream_info(transform, S_OK, &input_info);
        }
        else if (test->input_type_desc == rgb32_no_aperture)
        {
            input_info.cbSize = 82 * 84 * 4;
            check_mft_get_input_stream_info(transform, S_OK, &input_info);
        }
        else
        {
            input_info.cbSize = actual_width * actual_height * 4;
            check_mft_get_input_stream_info(transform, S_OK, &input_info);
        }

        load_resource(test->input_bitmap, &input_data, &input_data_len);
        if (test->input_type_desc == nv12_default_stride || test->input_type_desc == nv12_with_aperture)
        {
            /* skip BMP header and RGB data from the dump */
            length = *(DWORD *)(input_data + 2);
            input_data_len = input_data_len - length;
        }
        else
        {
            /* skip BMP header */
            length = *(DWORD *)(input_data + 2 + 2 * sizeof(DWORD));
            input_data_len -= length;
        }
        if (!test->input_buffer_desc) ok(input_data_len == input_info.cbSize, "got length %lu\n", input_data_len);
        input_data += length;

        input_sample = create_sample_(input_data, input_data_len, test->input_buffer_desc);
        hr = IMFSample_SetSampleTime(input_sample, 0);
        ok(hr == S_OK, "SetSampleTime returned %#lx\n", hr);
        hr = IMFSample_SetSampleDuration(input_sample, 10000000);
        ok(hr == S_OK, "SetSampleDuration returned %#lx\n", hr);
        hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
        ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
        hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
        ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);
        hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, 0);
        ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);
        ret = IMFSample_Release(input_sample);
        ok(ret <= 1, "Release returned %ld\n", ret);

        hr = MFCreateCollection(&output_samples);
        ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

        output_sample = create_sample_(NULL, output_info.cbSize, test->output_buffer_desc);
        hr = check_mft_process_output(transform, output_sample, &output_status);

        ok(hr == S_OK || broken(hr == MF_E_SHUTDOWN) /* w8 */, "ProcessOutput returned %#lx\n", hr);
        if (hr != S_OK)
        {
            win_skip("ProcessOutput returned MF_E_SHUTDOWN, skipping tests.\n");
        }
        else
        {
            ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);

            hr = IMFCollection_AddElement(output_samples, (IUnknown *)output_sample);
            ok(hr == S_OK, "AddElement returned %#lx\n", hr);
            ref = IMFSample_Release(output_sample);
            ok(ref == 1, "Release returned %ld\n", ref);

            output_bitmap = use_2d_buffer && test->output_bitmap_1d ? test->output_bitmap_1d : test->output_bitmap;
            ret = check_mf_sample_collection(output_samples, test->output_sample_desc, output_bitmap);
            todo_wine_if(test->todo)
            ok(ret <= test->delta || broken(test->broken), "1d got %lu%% diff\n", ret);

            if (use_2d_buffer)
            {
                output_bitmap = test->output_bitmap_2d ? test->output_bitmap_2d : test->output_bitmap;
                ret = check_2d_mf_sample_collection(output_samples, test->output_sample_2d_desc, output_bitmap);
                todo_wine_if(test->todo)
                ok(ret <= test->delta || broken(test->broken), "2d got %lu%% diff\n", ret);
            }

            IMFCollection_Release(output_samples);

            output_sample = create_sample_(NULL, output_info.cbSize, test->output_buffer_desc);
            hr = check_mft_process_output(transform, output_sample, &output_status);
            ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
            ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
            hr = IMFSample_GetTotalLength(output_sample, &length);
            ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
            ok(length == 0, "got length %lu\n", length);
        }
        ret = IMFSample_Release(output_sample);
        ok(ret == 0, "Release returned %lu\n", ret);

        winetest_pop_context();

        hr = IMFTransform_SetInputType(transform, 0, NULL, 0);
        ok(hr == S_OK, "got %#lx\n", hr);
        hr = IMFTransform_SetOutputType(transform, 0, NULL, 0);
        ok(hr == S_OK, "got %#lx\n", hr);
    }

    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %ld\n", ret);


    /* check that it is possible to change input media type frame size using geometric aperture */

    hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "got hr %#lx\n", hr);

    check_mft_set_input_type(transform, nv12_no_aperture, S_OK);
    check_mft_get_input_current_type(transform, nv12_no_aperture);

    check_mft_set_output_type(transform, rgb32_no_aperture, S_OK);
    check_mft_get_output_current_type(transform, rgb32_no_aperture);

    check_mft_set_input_type(transform, nv12_with_aperture, S_OK);
    check_mft_get_input_current_type(transform, nv12_with_aperture);

    /* output type is the same as before */
    check_mft_get_output_current_type(transform, rgb32_no_aperture);

    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %ld\n", ret);


failed:
    winetest_pop_context();
    CoUninitialize();
}

static void test_mp3_decoder(void)
{
    const GUID *const class_id = &CLSID_CMP3DecMediaObject;
    const struct transform_info expect_mft_info =
    {
        .name = L"MP3 Decoder MFT",
        .major_type = &MFMediaType_Audio,
        .inputs =
        {
            {.subtype = &MFAudioFormat_MP3},
        },
        .outputs =
        {
            {.subtype = &MFAudioFormat_PCM},
        },
    };
    const struct transform_info expect_dmo_info =
    {
        .name = L"MP3 Decoder DMO",
        .major_type = &MEDIATYPE_Audio,
        .inputs =
        {
            {.subtype = &MFAudioFormat_MP3},
        },
        .outputs =
        {
            {.subtype = &MEDIASUBTYPE_PCM},
        },
    };

    static const ULONG mp3dec_block_size = 0x1200;
    static const media_type_desc expect_available_inputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_MP3),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
    };
    static const media_type_desc expect_available_outputs[] =
    {
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
            ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
            ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
            ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 176400),
            ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 8),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
            ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
            ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
            ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 88200),
            ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
            ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
            ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
            ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100),
            ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 2),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
            ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
            ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
            ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 88200),
            ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
            ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
            ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
            ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100),
            ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 2),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
        {
            ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
            ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
            ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
            ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
            ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
            ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 22050),
            ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
            ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        },
    };

    const struct attribute_desc input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_MP3, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
        {0},
    };
    static const struct attribute_desc output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio, .required = TRUE),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050, .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 2 * (16 / 8), .required = TRUE),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 2 * (16 / 8) * 22050, .required = TRUE),
        {0},
    };
    const struct attribute_desc expect_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_MP3),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        {0},
    };
    static const struct attribute_desc expect_output_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 22050),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 22050 * 4),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
        {0},
    };
    const MFT_OUTPUT_STREAM_INFO output_info =
    {
        .cbSize = mp3dec_block_size,
        .cbAlignment = 1,
    };
    const MFT_INPUT_STREAM_INFO input_info =
    {
        .cbAlignment = 1,
    };

    const struct buffer_desc output_buffer_desc[] =
    {
        {.length = 0x9c0, .compare = compare_pcm16, .todo_length = TRUE},
        {.length = mp3dec_block_size, .compare = compare_pcm16},
        {.length = mp3dec_block_size, .compare = compare_pcm16, .todo_data = TRUE},
    };
    const struct attribute_desc output_sample_attributes[] =
    {
        ATTR_UINT32(mft_output_sample_incomplete, 1),
        ATTR_UINT32(MFSampleExtension_CleanPoint, 1),
        {0},
    };
    const struct sample_desc output_sample_desc[] =
    {
        {
            .attributes = output_sample_attributes + 0,
            .sample_time = 0, .sample_duration = 282993,
            .buffer_count = 1, .buffers = output_buffer_desc + 0,
            .todo_length = TRUE, .todo_duration = TRUE,
        },
        {
            .attributes = output_sample_attributes + 0,
            .sample_time = 282993, .sample_duration = 522449,
            .buffer_count = 1, .buffers = output_buffer_desc + 1, .repeat_count = 18,
            .todo_time = TRUE,
        },
        {
            .attributes = output_sample_attributes + 1, /* not MFT_OUTPUT_DATA_BUFFER_INCOMPLETE */
            .sample_time = 10209524, .sample_duration = 522449,
            .buffer_count = 1, .buffers = output_buffer_desc + 2,
            .todo_time = TRUE, .todo_data = TRUE,
        },
    };

    MFT_REGISTER_TYPE_INFO output_type = {MFMediaType_Audio, MFAudioFormat_PCM};
    MFT_REGISTER_TYPE_INFO input_type = {MFMediaType_Audio, MFAudioFormat_MP3};
    IMFSample *input_sample, *output_sample;
    IMFCollection *output_samples;
    DWORD length, output_status;
    IMFMediaType *media_type;
    IMFTransform *transform;
    const BYTE *mp3enc_data;
    ULONG mp3enc_data_len;
    ULONG i, ret, ref;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    winetest_push_context("mp3dec");

    if (!check_mft_enum(MFT_CATEGORY_AUDIO_DECODER, &input_type, &output_type, class_id))
        goto failed;
    check_mft_get_info(class_id, &expect_mft_info);
    check_dmo_get_info(class_id, &expect_dmo_info);

    if (FAILED(hr = CoCreateInstance(class_id, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;

    check_interface(transform, &IID_IMFTransform, TRUE);
    check_interface(transform, &IID_IMediaObject, TRUE);
    todo_wine
    check_interface(transform, &IID_IPropertyStore, TRUE);
    check_interface(transform, &IID_IPropertyBag, FALSE);

    check_mft_optional_methods(transform, 1);
    check_mft_get_attributes(transform, NULL, FALSE);
    check_mft_get_input_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);
    check_mft_get_output_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "GetOutputAvailableType returned %#lx\n", hr);

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetInputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("in %lu", i);
        ok(hr == S_OK, "GetInputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_inputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetInputAvailableType returned %#lx\n", hr);
    ok(i == ARRAY_SIZE(expect_available_inputs), "%lu input media types\n", i);

    /* setting output media type first doesn't work */
    check_mft_set_output_type(transform, output_type_desc, MF_E_TRANSFORM_TYPE_NOT_SET);
    check_mft_get_output_current_type(transform, NULL);

    check_mft_set_input_type_required(transform, input_type_desc);
    check_mft_set_input_type(transform, input_type_desc, S_OK);
    check_mft_get_input_current_type(transform, expect_input_type_desc);

    check_mft_get_input_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);
    check_mft_get_output_stream_info(transform, MF_E_TRANSFORM_TYPE_NOT_SET, NULL);

    /* check new output media types */

    i = -1;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &media_type)))
    {
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(media_type, expect_available_outputs[i], -1);
        ret = IMFMediaType_Release(media_type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);
    ok(i == ARRAY_SIZE(expect_available_outputs), "%lu output media types\n", i);

    check_mft_set_output_type_required(transform, output_type_desc);
    check_mft_set_output_type(transform, output_type_desc, S_OK);
    check_mft_get_output_current_type(transform, expect_output_type_desc);

    check_mft_get_input_stream_info(transform, S_OK, &input_info);
    check_mft_get_output_stream_info(transform, S_OK, &output_info);

    load_resource(L"mp3encdata.bin", &mp3enc_data, &mp3enc_data_len);
    ok(mp3enc_data_len == 6295, "got length %lu\n", mp3enc_data_len);

    input_sample = create_sample(mp3enc_data, mp3enc_data_len);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == S_OK, "ProcessInput returned %#lx\n", hr);
    ret = IMFSample_Release(input_sample);
    ok(ret == 1, "Release returned %lu\n", ret);

    input_sample = create_sample(mp3enc_data, mp3enc_data_len);
    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "ProcessInput returned %#lx\n", hr);
    ret = IMFSample_Release(input_sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, 0);
    ok(hr == S_OK, "ProcessMessage returned %#lx\n", hr);

    hr = MFCreateCollection(&output_samples);
    ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

    /* first sample is broken */
    output_sample = create_sample(NULL, output_info.cbSize);
    hr = check_mft_process_output(transform, output_sample, &output_status);
    ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == MFT_OUTPUT_DATA_BUFFER_INCOMPLETE, "got output[0].dwStatus %#lx\n", output_status);
    hr = IMFSample_GetTotalLength(output_sample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == mp3dec_block_size /* Win8 */ || length == 0x9c0 /* Win10 */ || length == 0x900 /* Win7 */,
            "got length %lu\n", length);
    hr = IMFCollection_AddElement(output_samples, (IUnknown *)output_sample);
    ok(hr == S_OK, "AddElement returned %#lx\n", hr);
    ref = IMFSample_Release(output_sample);
    ok(ref == 1, "Release returned %ld\n", ref);

    output_sample = create_sample(NULL, output_info.cbSize);
    for (i = 0; SUCCEEDED(hr = check_mft_process_output(transform, output_sample, &output_status)); i++)
    {
        winetest_push_context("%lu", i);
        ok(hr == S_OK, "ProcessOutput returned %#lx\n", hr);
        ok(!(output_status & ~MFT_OUTPUT_DATA_BUFFER_INCOMPLETE), "got output[0].dwStatus %#lx\n", output_status);
        hr = IMFCollection_AddElement(output_samples, (IUnknown *)output_sample);
        ok(hr == S_OK, "AddElement returned %#lx\n", hr);
        ref = IMFSample_Release(output_sample);
        ok(ref == 1, "Release returned %ld\n", ref);
        output_sample = create_sample(NULL, output_info.cbSize);
        winetest_pop_context();
    }
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);
    ok(i == 20 || broken(i == 41) /* Win7 */, "got %lu output samples\n", i);

    if (broken(length != 0x9c0))
        win_skip("Skipping MP3 decoder output sample checks on Win7 / Win8\n");
    else
    {
        ret = check_mf_sample_collection(output_samples, output_sample_desc, L"mp3decdata.bin");
        todo_wine
        ok(ret == 0, "got %lu%% diff\n", ret);
    }
    IMFCollection_Release(output_samples);

    output_sample = create_sample(NULL, mp3dec_block_size);
    hr = check_mft_process_output(transform, output_sample, &output_status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "ProcessOutput returned %#lx\n", hr);
    ok(output_status == 0, "got output[0].dwStatus %#lx\n", output_status);
    hr = IMFSample_GetTotalLength(output_sample, &length);
    ok(hr == S_OK, "GetTotalLength returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    ret = IMFSample_Release(output_sample);
    ok(ret == 0, "Release returned %lu\n", ret);

    ret = IMFTransform_Release(transform);
    ok(ret == 0, "Release returned %lu\n", ret);

failed:
    winetest_pop_context();
    CoUninitialize();
}

static HRESULT get_next_h264_output_sample(IMFTransform *transform, IMFSample **input_sample,
        IMFSample *output_sample, MFT_OUTPUT_DATA_BUFFER *output, const BYTE **data, ULONG *data_len)
{
    DWORD status;
    HRESULT hr;

    while (1)
    {
        status = 0;
        memset(output, 0, sizeof(*output));
        output[0].pSample = output_sample;
        hr = IMFTransform_ProcessOutput(transform, 0, 1, output, &status);
        if (hr != S_OK)
            ok(output[0].pSample == output_sample, "got %p.\n", output[0].pSample);
        if (hr != MF_E_TRANSFORM_NEED_MORE_INPUT)
            return hr;

        ok(status == 0, "got output[0].dwStatus %#lx\n", status);
        hr = IMFTransform_ProcessInput(transform, 0, *input_sample, 0);
        ok(hr == S_OK, "got %#lx\n", hr);
        IMFSample_Release(*input_sample);
        *input_sample = next_h264_sample(data, data_len);
    }
}

static void test_h264_with_dxgi_manager(void)
{
    static const unsigned int set_width = 82, set_height = 84, aligned_width = 96, aligned_height = 96;
    const struct attribute_desc output_sample_attributes[] =
    {
        ATTR_UINT32(MFSampleExtension_CleanPoint, 1),
        {0},
    };
    const struct buffer_desc output_buffer_desc_nv12 =
    {
        .length = aligned_width * aligned_height * 3 / 2,
        .compare = compare_nv12, .compare_rect = {.right = set_width, .bottom = set_height},
        .dump = dump_nv12, .size = {.cx = aligned_width, .cy = aligned_height},
    };
    const struct sample_desc output_sample_desc_nv12 =
    {
        .attributes = output_sample_attributes,
        .sample_time = 333667, .sample_duration = 333667,
        .buffer_count = 1, .buffers = &output_buffer_desc_nv12,
        .todo_time = TRUE, /* _SetInputType() / _SetOutputType() type resets the output sample timestamp on Windows. */
    };

    IMFDXGIDeviceManager *manager = NULL;
    IMFTrackedSample *tracked_sample;
    IMFSample *input_sample, *sample;
    MFT_OUTPUT_DATA_BUFFER output[1];
    IMFTransform *transform = NULL;
    ID3D11Multithread *multithread;
    IMFCollection *output_samples;
    MFT_OUTPUT_STREAM_INFO info;
    IMFDXGIBuffer *dxgi_buffer;
    unsigned int width, height;
    D3D11_TEXTURE2D_DESC desc;
    IMFMediaBuffer *buffer;
    IMFAttributes *attribs;
    ID3D11Texture2D *tex2d;
    IMF2DBuffer2 *buffer2d;
    ID3D11Device *d3d11;
    IMFMediaType *type;
    DWORD status, val;
    UINT64 frame_size;
    MFVideoArea area;
    const BYTE *data;
    ULONG data_len;
    UINT32 value;
    HRESULT hr;
    UINT token;
    GUID guid;
    DWORD ret;

    if (!pMFCreateDXGIDeviceManager)
    {
        win_skip("MFCreateDXGIDeviceManager() is not available, skipping tests.\n");
        return;
    }

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_VIDEO_SUPPORT, NULL, 0,
            D3D11_SDK_VERSION, &d3d11, NULL, NULL);
    if (FAILED(hr))
    {
        skip("D3D11 device creation failed, skipping tests.\n");
        return;
    }

    hr = MFStartup(MF_VERSION, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = ID3D11Device_QueryInterface(d3d11, &IID_ID3D11Multithread, (void **)&multithread);
    ok(hr == S_OK, "got %#lx\n", hr);
    ID3D11Multithread_SetMultithreadProtected(multithread, TRUE);
    ID3D11Multithread_Release(multithread);

    hr = pMFCreateDXGIDeviceManager(&token, &manager);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)d3d11, token);
    ok(hr == S_OK, "got %#lx\n", hr);
    ID3D11Device_Release(d3d11);

    if (FAILED(hr = CoCreateInstance(&CLSID_MSH264DecoderMFT, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;

    hr = IMFTransform_GetAttributes(transform, &attribs);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IMFAttributes_GetUINT32(attribs, &MF_SA_D3D11_AWARE, &value);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(value == 1, "got %u.\n", value);
    IMFAttributes_Release(attribs);

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)transform);
    ok(hr == E_NOINTERFACE, "got %#lx\n", hr);

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)manager);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE), "got %#lx\n", hr);
    if (hr == E_NOINTERFACE)
    {
        win_skip("No hardware video decoding support.\n");
        goto failed;
    }

    hr = MFCreateMediaType(&type);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, &MFVideoFormat_H264);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetUINT64(type, &MF_MT_FRAME_SIZE, 1088 | (1920ull << 32));
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFTransform_SetInputType(transform, 0, type, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFMediaType_Release(type);

    /* MFVideoFormat_ABGR32 output isn't supported by the D3D11-enabled decoder */
    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &type);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, &MFVideoFormat_ABGR32);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFTransform_SetOutputType(transform, 0, type, 0);
    ok(hr == MF_E_INVALIDMEDIATYPE, "got %#lx\n", hr);
    IMFMediaType_Release(type);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &type);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, &MFVideoFormat_NV12);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFTransform_SetOutputType(transform, 0, type, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFMediaType_Release(type);

    status = 0;
    memset(output, 0, sizeof(output));
    hr = IMFTransform_ProcessOutput(transform, 0, 1, output, &status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "got %#lx\n", hr);

    hr = IMFTransform_GetAttributes(transform, &attribs);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFAttributes_GetUINT32(attribs, &MF_SA_D3D11_AWARE, &value);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(value == 1, "got %u.\n", value);
    IMFAttributes_Release(attribs);

    load_resource(L"h264data.bin", &data, &data_len);

    input_sample = next_h264_sample(&data, &data_len);
    hr = get_next_h264_output_sample(transform, &input_sample, NULL, output, &data, &data_len);
    ok(hr == MF_E_TRANSFORM_STREAM_CHANGE, "got %#lx\n", hr);
    ok(!output[0].pSample, "got %p.\n", output[0].pSample);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &type);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size);
    ok(hr == S_OK, "got %#lx\n", hr);
    width = frame_size >> 32;
    height = frame_size & 0xffffffff;
    ok(width == aligned_width, "got %u.\n", width);
    ok(height == aligned_height, "got %u.\n", height);
    memset(&area, 0xcc, sizeof(area));
    hr = IMFMediaType_GetBlob(type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&area, sizeof(area), NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!area.OffsetX.value && !area.OffsetX.fract, "got %d.%d.\n", area.OffsetX.value, area.OffsetX.fract);
    ok(!area.OffsetY.value && !area.OffsetY.fract, "got %d.%d.\n", area.OffsetY.value, area.OffsetY.fract);
    ok(area.Area.cx == set_width, "got %ld.\n", area.Area.cx);
    ok(area.Area.cy == set_height, "got %ld.\n", area.Area.cy);

    hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Failed to get subtype, hr %#lx.\n", hr);
    ok(IsEqualIID(&guid, &MEDIASUBTYPE_NV12), "got guid %s.\n", debugstr_guid(&guid));

    hr = IMFTransform_SetOutputType(transform, 0, type, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFMediaType_Release(type);

    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &info);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(info.dwFlags == (MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER
            | MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE | MFT_OUTPUT_STREAM_PROVIDES_SAMPLES), "got %#lx.\n", info.dwFlags);

    hr = get_next_h264_output_sample(transform, &input_sample, NULL, output, &data, &data_len);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(output[0].dwStatus == 0, "got %#lx.\n", status);
    sample = output[0].pSample;
    IMFSample_Release(sample);

    /* Set input and output type trying to change output frame size (results in MF_E_TRANSFORM_STREAM_CHANGE with
     * frame size reset. */
    hr = MFCreateMediaType(&type);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, &MFVideoFormat_H264);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetUINT64(type, &MF_MT_FRAME_SIZE, 1088 | (1920ull << 32));
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFTransform_SetInputType(transform, 0, type, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFMediaType_Release(type);
    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &type);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, &MFVideoFormat_NV12);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetUINT64(type, &MF_MT_FRAME_SIZE, ((UINT64)1920) << 32 | 1088);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFTransform_SetOutputType(transform, 0, type, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFMediaType_Release(type);

    hr = get_next_h264_output_sample(transform, &input_sample, NULL, output, &data, &data_len);
    ok(hr == MF_E_TRANSFORM_STREAM_CHANGE, "got %#lx\n", hr);
    ok(!output[0].pSample, "got %p.\n", output[0].pSample);

    /* Need to set output type with matching size now, or that hangs on Windows. */
    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &type);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size);
    ok(hr == S_OK, "got %#lx\n", hr);
    width = frame_size >> 32;
    height = frame_size & 0xffffffff;
    ok(width == aligned_width, "got %u.\n", width);
    ok(height == aligned_height, "got %u.\n", height);
    hr = IMFTransform_SetOutputType(transform, 0, type, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFMediaType_Release(type);

    hr = get_next_h264_output_sample(transform, &input_sample, NULL, output, &data, &data_len);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(output[0].dwStatus == 0, "got %#lx.\n", status);
    sample = output[0].pSample;

    hr = IMFSample_QueryInterface(sample, &IID_IMFTrackedSample, (void **)&tracked_sample);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFTrackedSample_Release(tracked_sample);

    hr = IMFSample_GetBufferCount(sample, &val);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(val == 1, "got %lu.\n", val);
    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFDXGIBuffer, (void **)&dxgi_buffer);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&buffer2d);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IMFDXGIBuffer_GetResource(dxgi_buffer, &IID_ID3D11Texture2D, (void **)&tex2d);
    ok(hr == S_OK, "got %#lx\n", hr);
    memset(&desc, 0xcc, sizeof(desc));
    ID3D11Texture2D_GetDesc(tex2d, &desc);
    ok(desc.Format == DXGI_FORMAT_NV12, "got %u.\n", desc.Format);
    ok(!desc.Usage, "got %u.\n", desc.Usage);
    todo_wine ok(desc.BindFlags == D3D11_BIND_DECODER, "got %#x.\n", desc.BindFlags);
    ok(!desc.CPUAccessFlags, "got %#x.\n", desc.CPUAccessFlags);
    ok(!desc.MiscFlags, "got %#x.\n", desc.MiscFlags);
    ok(desc.MipLevels == 1, "git %u.\n", desc.MipLevels);
    ok(desc.Width == aligned_width, "got %u.\n", desc.Width);
    ok(desc.Height == aligned_height, "got %u.\n", desc.Height);

    ID3D11Texture2D_Release(tex2d);
    IMFDXGIBuffer_Release(dxgi_buffer);
    IMF2DBuffer2_Release(buffer2d);
    IMFMediaBuffer_Release(buffer);
    IMFSample_Release(sample);

    status = 0;
    hr = get_next_h264_output_sample(transform, &input_sample, NULL, output, &data, &data_len);
    todo_wine_if(hr == MF_E_UNEXPECTED) /* with some llvmpipe versions */
    ok(hr == S_OK, "got %#lx\n", hr);
    if (hr == MF_E_UNEXPECTED)
    {
        IMFSample_Release(input_sample);
        goto failed;
    }
    ok(sample != output[0].pSample, "got %p.\n", output[0].pSample);
    sample = output[0].pSample;

    hr = MFCreateCollection(&output_samples);
    ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

    hr = IMFCollection_AddElement(output_samples, (IUnknown *)sample);
    ok(hr == S_OK, "AddElement returned %#lx\n", hr);
    IMFSample_Release(sample);

    ret = check_mf_sample_collection(output_samples, &output_sample_desc_nv12, L"nv12frame.bmp");
    ok(ret == 0, "got %lu%% diff\n", ret);
    IMFCollection_Release(output_samples);

    memset(&info, 0xcc, sizeof(info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &info);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(info.dwFlags == (MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER
            | MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE | MFT_OUTPUT_STREAM_PROVIDES_SAMPLES), "got %#lx.\n", info.dwFlags);
    ok(info.cbSize == aligned_width * aligned_height * 2, "got %lu.\n", info.cbSize);
    ok(!info.cbAlignment, "got %lu.\n", info.cbAlignment);

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_SET_D3D_MANAGER, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    memset(&info, 0xcc, sizeof(info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &info);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(info.dwFlags == (MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER
            | MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE), "got %#lx.\n", info.dwFlags);
    if (0)
    {
        /* hangs on Windows. */
        get_next_h264_output_sample(transform, &input_sample, NULL, output, &data, &data_len);
    }

    IMFSample_Release(input_sample);

failed:
    if (manager)
        IMFDXGIDeviceManager_Release(manager);
    if (transform)
        IMFTransform_Release(transform);

    MFShutdown();
    CoUninitialize();
}

static void test_iv50_encoder(void)
{
    static const BITMAPINFOHEADER expect_iv50 =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = 0x60, .biHeight = 0x60,
        .biPlanes = 1, .biBitCount = 0x18,
        .biCompression = mmioFOURCC('I','V','5','0'),
        .biSizeImage = 0x5100,
    };
    const struct buffer_desc iv50_buffer_desc =
    {
        .length = 0x2e8,
    };
    const struct sample_desc iv50_sample_desc =
    {
        .buffer_count = 1, .buffers = &iv50_buffer_desc,
    };
    const BYTE *res_data;
    BYTE rgb_data[0x6c00], iv50_data[0x5100];
    BITMAPINFO rgb_info, iv50_info;
    IMFCollection *collection;
    DWORD flags, res_len, ret;
    IMFSample *sample;
    LRESULT res;
    HRESULT hr;
    HIC hic;

    load_resource(L"rgb555frame.bmp", (const BYTE **)&res_data, &res_len);
    res_data += 2 + 3 * sizeof(DWORD);
    res_len -= 2 + 3 * sizeof(DWORD);
    rgb_info.bmiHeader = *(BITMAPINFOHEADER *)res_data;
    memcpy(rgb_data, res_data + sizeof(BITMAPINFOHEADER), res_len - sizeof(BITMAPINFOHEADER));

    hic = ICOpen(ICTYPE_VIDEO, expect_iv50.biCompression, ICMODE_COMPRESS);
    if (!hic)
    {
        todo_wine
        win_skip("ICOpen failed to created IV50 compressor.\n");
        return;
    }

    res = ICCompressQuery(hic, &rgb_info, NULL);
    todo_wine
    ok(!res, "got res %#Ix\n", res);
    if (res)
        goto done;

    res = ICCompressGetSize(hic, &rgb_info, NULL);
    ok(res == expect_iv50.biSizeImage, "got res %#Ix\n", res);

    res = ICCompressGetFormatSize(hic, &rgb_info);
    ok(res == sizeof(BITMAPINFOHEADER), "got res %#Ix\n", res);
    res = ICCompressGetFormat(hic, &rgb_info, &iv50_info);
    ok(!res, "got res %#Ix\n", res);
    check_member(iv50_info.bmiHeader, expect_iv50, "%#lx", biSize);
    check_member(iv50_info.bmiHeader, expect_iv50, "%#lx", biWidth);
    check_member(iv50_info.bmiHeader, expect_iv50, "%#lx", biHeight);
    check_member(iv50_info.bmiHeader, expect_iv50, "%#x", biPlanes);
    check_member(iv50_info.bmiHeader, expect_iv50, "%#x", biBitCount);
    check_member(iv50_info.bmiHeader, expect_iv50, "%#lx", biCompression);
    check_member(iv50_info.bmiHeader, expect_iv50, "%#lx", biSizeImage);
    check_member(iv50_info.bmiHeader, expect_iv50, "%#lx", biXPelsPerMeter);
    check_member(iv50_info.bmiHeader, expect_iv50, "%#lx", biYPelsPerMeter);
    check_member(iv50_info.bmiHeader, expect_iv50, "%#lx", biClrUsed);
    check_member(iv50_info.bmiHeader, expect_iv50, "%#lx", biClrImportant);
    res = ICCompressQuery(hic, &rgb_info, &iv50_info);
    ok(!res, "got res %#Ix\n", res);

    res = ICCompressBegin(hic, &rgb_info, &iv50_info);
    ok(!res, "got res %#Ix\n", res);
    memset(iv50_data, 0xcd, sizeof(iv50_data));
    res = ICCompress(hic, ICCOMPRESS_KEYFRAME, &iv50_info.bmiHeader, iv50_data, &rgb_info.bmiHeader, rgb_data,
            NULL, &flags, 1, 0, 0, NULL, NULL);
    ok(!res, "got res %#Ix\n", res);
    ok(flags == 0x10, "got flags %#lx\n", flags);
    ok(iv50_info.bmiHeader.biSizeImage == 0x2e8, "got res %#Ix\n", res);
    res = ICCompressEnd(hic);
    ok(!res, "got res %#Ix\n", res);

    hr = MFCreateCollection(&collection);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    sample = create_sample(iv50_data, iv50_info.bmiHeader.biSizeImage);
    ok(!!sample, "got sample %p\n", sample);
    hr = IMFSample_SetSampleTime(sample, 0);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    hr = IMFSample_SetSampleDuration(sample, 0);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    hr = IMFCollection_AddElement(collection, (IUnknown *)sample);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ret = check_mf_sample_collection(collection, &iv50_sample_desc, L"iv50frame.bin");
    ok(ret == 0, "got %lu%% diff\n", ret);
    IMFCollection_Release(collection);

done:
    res = ICClose(hic);
    ok(!res, "got res %#Ix\n", res);
}

static void test_iv50_decoder(void)
{
    static const BITMAPINFOHEADER expect_iv50 =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = 0x60, .biHeight = 0x60,
        .biPlanes = 1, .biBitCount = 24,
        .biCompression = mmioFOURCC('I','V','5','0'),
        .biSizeImage = 0x2e8,
    };
    static const BITMAPINFOHEADER expect_rgb =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = 0x60, .biHeight = 0x60,
        .biPlanes = 1, .biBitCount = 24,
        .biCompression = BI_RGB,
        .biSizeImage = 96 * 96 * 3,
    };
    const struct buffer_desc rgb_buffer_desc =
    {
        .length = 96 * 96 * 3,
        .compare = compare_rgb24, .compare_rect = {.right = 82, .bottom = 84},
        .dump = dump_rgb24, .size = {.cx = 96, .cy = 96},
    };
    const struct sample_desc rgb_sample_desc =
    {
        .buffer_count = 1, .buffers = &rgb_buffer_desc,
    };
    const BYTE *res_data;
    BYTE rgb_data[0x6c00], iv50_data[0x5100];
    BITMAPINFO rgb_info, iv50_info;
    IMFCollection *collection;
    DWORD res_len, ret;
    IMFSample *sample;
    LRESULT res;
    HRESULT hr;
    HIC hic;

    load_resource(L"iv50frame.bin", (const BYTE **)&res_data, &res_len);
    memcpy(iv50_data, res_data, res_len);

    iv50_info.bmiHeader = expect_iv50;
    hic = ICOpen(ICTYPE_VIDEO, expect_iv50.biCompression, ICMODE_DECOMPRESS);
    if (!hic)
    {
        todo_wine
        win_skip("ICOpen failed to created IV50 decompressor.\n");
        return;
    }

    res = ICDecompressGetFormat(hic, &iv50_info, &rgb_info);
    ok(!res, "got res %#Ix\n", res);
    check_member(rgb_info.bmiHeader, expect_rgb, "%#lx", biSize);
    check_member(rgb_info.bmiHeader, expect_rgb, "%#lx", biWidth);
    check_member(rgb_info.bmiHeader, expect_rgb, "%#lx", biHeight);
    check_member(rgb_info.bmiHeader, expect_rgb, "%#x", biPlanes);
    todo_wine
    check_member(rgb_info.bmiHeader, expect_rgb, "%#x", biBitCount);
    check_member(rgb_info.bmiHeader, expect_rgb, "%#lx", biCompression);
    todo_wine
    check_member(rgb_info.bmiHeader, expect_rgb, "%#lx", biSizeImage);
    check_member(rgb_info.bmiHeader, expect_rgb, "%#lx", biXPelsPerMeter);
    check_member(rgb_info.bmiHeader, expect_rgb, "%#lx", biYPelsPerMeter);
    check_member(rgb_info.bmiHeader, expect_rgb, "%#lx", biClrUsed);
    check_member(rgb_info.bmiHeader, expect_rgb, "%#lx", biClrImportant);
    rgb_info.bmiHeader = expect_rgb;

    res = ICDecompressBegin(hic, &iv50_info, &rgb_info);
    ok(!res, "got res %#Ix\n", res);
    res = ICDecompress(hic, 0, &iv50_info.bmiHeader, iv50_data, &rgb_info.bmiHeader, rgb_data);
    ok(!res, "got res %#Ix\n", res);
    res = ICDecompressEnd(hic);
    todo_wine
    ok(!res, "got res %#Ix\n", res);

    res = ICClose(hic);
    ok(!res, "got res %#Ix\n", res);


    hr = MFCreateCollection(&collection);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    sample = create_sample(rgb_data, rgb_info.bmiHeader.biSizeImage);
    ok(!!sample, "got sample %p\n", sample);
    hr = IMFSample_SetSampleTime(sample, 0);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    hr = IMFSample_SetSampleDuration(sample, 0);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    hr = IMFCollection_AddElement(collection, (IUnknown *)sample);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ret = check_mf_sample_collection(collection, &rgb_sample_desc, L"rgb24frame.bmp");
    ok(ret <= 4, "got %lu%% diff\n", ret);
    IMFCollection_Release(collection);
}

static IMFSample *create_d3d_sample(IMFVideoSampleAllocator *allocator, const void *data, ULONG size)
{
    IMFMediaBuffer *media_buffer;
    IMFSample *sample;
    BYTE *buffer;
    HRESULT hr;

    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFSample_SetSampleTime(sample, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFSample_SetSampleDuration(sample, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFSample_GetBufferByIndex(sample, 0, &media_buffer);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaBuffer_Lock(media_buffer, &buffer, NULL, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    if (!data) memset(buffer, 0xcd, size);
    else memcpy(buffer, data, size);
    hr = IMFMediaBuffer_Unlock(media_buffer);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFMediaBuffer_Release(media_buffer);

    return sample;
}

static void test_video_processor_with_dxgi_manager(void)
{
    static const unsigned int set_width = 82, set_height = 84, aligned_width = 96, aligned_height = 96;
    const struct attribute_desc output_sample_attributes[] =
    {
        {0},
    };
    const struct buffer_desc output_buffer_desc_rgb32 =
    {
        .length = aligned_width * aligned_height * 4,
        .compare = compare_rgb32, .compare_rect = {.right = set_width, .bottom = set_height},
        .dump = dump_rgb32, .size = {.cx = aligned_width, .cy = aligned_height},
    };
    const struct sample_desc output_sample_desc_rgb32 =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 0,
        .buffer_count = 1, .buffers = &output_buffer_desc_rgb32,
    };

    const struct buffer_desc output_buffer_desc_rgb32_crop =
    {
        .length = set_width * set_height * 4,
        .compare = compare_rgb32, .compare_rect = {.right = set_width, .bottom = set_height},
        .dump = dump_rgb32, .size = {.cx = set_width, .cy = set_height},
    };
    const struct sample_desc output_sample_desc_rgb32_crop =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 0,
        .buffer_count = 1, .buffers = &output_buffer_desc_rgb32_crop,
    };

    const struct buffer_desc output_buffer_desc_abgr32_crop =
    {
        .length = set_width * set_height * 4,
        .compare = compare_abgr32, .compare_rect = {.right = set_width, .bottom = set_height},
        .dump = dump_rgb32, .size = {.cx = set_width, .cy = set_height},
    };
    const struct sample_desc output_sample_desc_abgr32_crop =
    {
        .attributes = output_sample_attributes,
        .sample_time = 0, .sample_duration = 0,
        .buffer_count = 1, .buffers = &output_buffer_desc_abgr32_crop,
    };

    const GUID expect_available_outputs[] =
    {
        MFVideoFormat_ARGB32,
        MFVideoFormat_ABGR32,
        MFVideoFormat_A2R10G10B10,
        MFVideoFormat_A16B16G16R16F,
        MFVideoFormat_NV12,
        MFVideoFormat_P010,
        MFVideoFormat_YUY2,
        MFVideoFormat_L8,
        MFVideoFormat_L16,
        MFVideoFormat_D16,
    };
    static const media_type_desc expect_available_common =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
    };

    const MFVideoArea aperture = {.Area={set_width, set_height}};
    const struct attribute_desc nv12_with_aperture[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_RATIO(MF_MT_FRAME_SIZE, aligned_width, aligned_height),
        ATTR_BLOB(MF_MT_MINIMUM_DISPLAY_APERTURE, &aperture, 16),
        ATTR_BLOB(MF_MT_GEOMETRIC_APERTURE, &aperture, 16),
        ATTR_BLOB(MF_MT_PAN_SCAN_APERTURE, &aperture, 16),
        {0},
    };
    const struct attribute_desc rgb32_no_aperture[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, set_width, set_height),
        {0},
    };
    const struct attribute_desc abgr32_no_aperture[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_ABGR32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, set_width, set_height),
        {0},
    };

    IMFVideoSampleAllocator *allocator = NULL;
    IMFDXGIDeviceManager *manager = NULL;
    IMFTrackedSample *tracked_sample;
    IMFTransform *transform = NULL;
    ID3D11Multithread *multithread;
    MFT_OUTPUT_DATA_BUFFER output;
    IMFCollection *output_samples;
    MFT_OUTPUT_STREAM_INFO info;
    IMFDXGIBuffer *dxgi_buffer;
    const BYTE *nv12frame_data;
    D3D11_TEXTURE2D_DESC desc;
    ULONG nv12frame_data_len;
    IMFSample *input_sample;
    IMFMediaBuffer *buffer;
    IMFAttributes *attribs;
    ID3D11Texture2D *tex2d;
    IMF2DBuffer2 *buffer2d;
    ID3D11Device *d3d11;
    IMFMediaType *type;
    DWORD status, val;
    ULONG i, j, length;
    UINT32 value;
    HRESULT hr;
    UINT token;
    DWORD ret;

    if (!pMFCreateDXGIDeviceManager || !pMFCreateVideoSampleAllocatorEx)
    {
        win_skip("MFCreateDXGIDeviceManager / MFCreateVideoSampleAllocatorEx are not available, skipping tests.\n");
        return;
    }

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_VIDEO_SUPPORT, NULL, 0,
            D3D11_SDK_VERSION, &d3d11, NULL, NULL);
    if (FAILED(hr))
    {
        skip("D3D11 device creation failed, skipping tests.\n");
        return;
    }

    hr = MFStartup(MF_VERSION, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = ID3D11Device_QueryInterface(d3d11, &IID_ID3D11Multithread, (void **)&multithread);
    ok(hr == S_OK, "got %#lx\n", hr);
    ID3D11Multithread_SetMultithreadProtected(multithread, TRUE);
    ID3D11Multithread_Release(multithread);

    hr = pMFCreateDXGIDeviceManager(&token, &manager);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)d3d11, token);
    ok(hr == S_OK, "got %#lx\n", hr);
    ID3D11Device_Release(d3d11);

    hr = pMFCreateVideoSampleAllocatorEx(&IID_IMFVideoSampleAllocator, (void **)&allocator);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFVideoSampleAllocator_SetDirectXManager(allocator, (IUnknown *)manager);
    ok(hr == S_OK, "got %#lx\n", hr);

    if (FAILED(hr = CoCreateInstance(&CLSID_VideoProcessorMFT, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&transform)))
        goto failed;

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)transform);
    todo_wine ok(hr == E_NOINTERFACE, "got %#lx\n", hr);

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)manager);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE), "got %#lx\n", hr);
    if (hr == E_NOINTERFACE)
    {
        win_skip("No hardware video decoding support.\n");
        goto failed;
    }

    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &info);
    ok(hr == S_OK, "got %#lx\n", hr);
    if (broken(!(info.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES)) /* w8 / w1064v1507 */)
    {
        win_skip("missing video processor sample allocator support.\n");
        goto failed;
    }
    ok(info.dwFlags == MFT_OUTPUT_STREAM_PROVIDES_SAMPLES, "got %#lx.\n", info.dwFlags);

    hr = MFCreateMediaType(&type);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, &MFVideoFormat_NV12);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetUINT64(type, &MF_MT_FRAME_SIZE, (UINT64)96 << 32 | 96);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFTransform_SetInputType(transform, 0, type, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 4, type);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFMediaType_Release(type);

    j = i = 0;
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, ++i, &type)))
    {
        GUID guid;
        winetest_push_context("out %lu", i);
        ok(hr == S_OK, "GetOutputAvailableType returned %#lx\n", hr);
        check_media_type(type, expect_available_common, -1);

        hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &guid);
        ok(hr == S_OK, "GetGUID returned %#lx\n", hr);

        for (; j < ARRAY_SIZE(expect_available_outputs); j++)
            if (IsEqualGUID(&expect_available_outputs[j], &guid))
                break;
        todo_wine_if(i >= 2)
        ok(j < ARRAY_SIZE(expect_available_outputs), "got subtype %s\n", debugstr_guid(&guid));

        ret = IMFMediaType_Release(type);
        ok(ret == 0, "Release returned %lu\n", ret);
        winetest_pop_context();
    }
    ok(hr == MF_E_NO_MORE_TYPES, "GetOutputAvailableType returned %#lx\n", hr);


    /* MFVideoFormat_ABGR32 is supported by the D3D11-enabled video processor */

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &type);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, &MFVideoFormat_ABGR32);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetUINT64(type, &MF_MT_FRAME_SIZE, (UINT64)96 << 32 | 96);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFTransform_SetOutputType(transform, 0, type, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFMediaType_Release(type);


    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &type);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaType_SetUINT64(type, &MF_MT_FRAME_SIZE, (UINT64)96 << 32 | 96);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFTransform_SetOutputType(transform, 0, type, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFMediaType_Release(type);

    status = 0;
    memset(&output, 0, sizeof(output));
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "got %#lx\n", hr);

    load_resource(L"nv12frame.bmp", &nv12frame_data, &nv12frame_data_len);
    /* skip BMP header and RGB data from the dump */
    length = *(DWORD *)(nv12frame_data + 2);
    nv12frame_data_len = nv12frame_data_len - length;
    nv12frame_data = nv12frame_data + length;
    ok(nv12frame_data_len == 13824, "got length %lu\n", nv12frame_data_len);

    /* native wants a dxgi buffer on input */
    input_sample = create_d3d_sample(allocator, nv12frame_data, nv12frame_data_len);

    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &info);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(info.dwFlags == MFT_OUTPUT_STREAM_PROVIDES_SAMPLES, "got %#lx.\n", info.dwFlags);


    status = 0;
    memset(&output, 0, sizeof(output));
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!output.pEvents, "got events\n");
    ok(!!output.pSample, "got no sample\n");
    ok(output.dwStatus == 0, "got %#lx\n", output.dwStatus);
    ok(status == 0, "got %#lx\n", status);

    hr = IMFTransform_GetAttributes(transform, &attribs);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFAttributes_GetUINT32(attribs, &MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT, &value);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFAttributes_Release(attribs);

    hr = IMFTransform_GetOutputStreamAttributes(transform, 0, &attribs);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFAttributes_GetCount(attribs, &value);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(value == 0, "got %u.\n", value);
    IMFAttributes_Release(attribs);


    hr = IMFSample_QueryInterface(output.pSample, &IID_IMFTrackedSample, (void **)&tracked_sample);
    ok(hr == S_OK, "got %#lx\n", hr);
    IMFTrackedSample_Release(tracked_sample);

    hr = IMFSample_GetBufferCount(output.pSample, &val);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(val == 1, "got %lu.\n", val);
    hr = IMFSample_GetBufferByIndex(output.pSample, 0, &buffer);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFDXGIBuffer, (void **)&dxgi_buffer);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&buffer2d);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IMFDXGIBuffer_GetResource(dxgi_buffer, &IID_ID3D11Texture2D, (void **)&tex2d);
    ok(hr == S_OK, "got %#lx\n", hr);
    memset(&desc, 0xcc, sizeof(desc));
    ID3D11Texture2D_GetDesc(tex2d, &desc);
    ok(desc.Format == DXGI_FORMAT_B8G8R8X8_UNORM, "got %#x.\n", desc.Format);
    ok(!desc.Usage, "got %u.\n", desc.Usage);
    ok(desc.BindFlags == (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE), "got %#x.\n", desc.BindFlags);
    ok(!desc.CPUAccessFlags, "got %#x.\n", desc.CPUAccessFlags);
    ok(!desc.MiscFlags, "got %#x.\n", desc.MiscFlags);
    ok(desc.MipLevels == 1, "git %u.\n", desc.MipLevels);
    ok(desc.Width == aligned_width, "got %u.\n", desc.Width);
    ok(desc.Height == aligned_height, "got %u.\n", desc.Height);

    ID3D11Texture2D_Release(tex2d);
    IMFDXGIBuffer_Release(dxgi_buffer);
    IMF2DBuffer2_Release(buffer2d);
    IMFMediaBuffer_Release(buffer);


    hr = MFCreateCollection(&output_samples);
    ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

    hr = IMFCollection_AddElement(output_samples, (IUnknown *)output.pSample);
    ok(hr == S_OK, "AddElement returned %#lx\n", hr);
    IMFSample_Release(output.pSample);

    ret = check_mf_sample_collection(output_samples, &output_sample_desc_rgb32, L"rgb32frame.bmp");
    ok(ret <= 5, "got %lu%% diff\n", ret);

    for (i = 0; i < 9; i++)
    {
        hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
        ok(hr == S_OK, "got %#lx\n", hr);

        status = 0;
        memset(&output, 0, sizeof(output));
        hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
        ok(hr == S_OK, "got %#lx\n", hr);
        ok(!output.pEvents, "got events\n");
        ok(!!output.pSample, "got no sample\n");
        ok(output.dwStatus == 0, "got %#lx\n", output.dwStatus);
        ok(status == 0, "got %#lx\n", status);

        hr = IMFCollection_AddElement(output_samples, (IUnknown *)output.pSample);
        ok(hr == S_OK, "AddElement returned %#lx\n", hr);
        IMFSample_Release(output.pSample);
    }

    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    status = 0;
    memset(&output, 0, sizeof(output));
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == MF_E_SAMPLEALLOCATOR_EMPTY, "got %#lx\n", hr);

    IMFCollection_Release(output_samples);

    status = 0;
    memset(&output, 0, sizeof(output));
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    /* FIXME: Wine sample release happens entirely asynchronously */
    flaky_wine_if(hr == MF_E_SAMPLEALLOCATOR_EMPTY)
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!output.pEvents, "got events\n");
    flaky_wine_if(hr == MF_E_SAMPLEALLOCATOR_EMPTY)
    ok(!!output.pSample, "got no sample\n");
    ok(output.dwStatus == 0, "got %#lx\n", output.dwStatus);
    ok(status == 0, "got %#lx\n", status);
    if (output.pSample)
        IMFSample_Release(output.pSample);


    /* check RGB32 output aperture cropping with D3D buffers */

    check_mft_set_input_type(transform, nv12_with_aperture, S_OK);
    check_mft_set_output_type(transform, rgb32_no_aperture, S_OK);

    load_resource(L"nv12frame.bmp", &nv12frame_data, &nv12frame_data_len);
    /* skip BMP header and RGB data from the dump */
    length = *(DWORD *)(nv12frame_data + 2);
    nv12frame_data_len = nv12frame_data_len - length;
    nv12frame_data = nv12frame_data + length;
    ok(nv12frame_data_len == 13824, "got length %lu\n", nv12frame_data_len);

    input_sample = create_d3d_sample(allocator, nv12frame_data, nv12frame_data_len);

    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &info);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(info.dwFlags == MFT_OUTPUT_STREAM_PROVIDES_SAMPLES, "got %#lx.\n", info.dwFlags);

    status = 0;
    memset(&output, 0, sizeof(output));
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!output.pEvents, "got events\n");
    ok(!!output.pSample, "got no sample\n");
    ok(output.dwStatus == 0, "got %#lx\n", output.dwStatus);
    ok(status == 0, "got %#lx\n", status);
    if (!output.pSample) goto skip_rgb32;

    hr = IMFSample_GetBufferByIndex(output.pSample, 0, &buffer);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFDXGIBuffer, (void **)&dxgi_buffer);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IMFDXGIBuffer_GetResource(dxgi_buffer, &IID_ID3D11Texture2D, (void **)&tex2d);
    ok(hr == S_OK, "got %#lx\n", hr);
    memset(&desc, 0xcc, sizeof(desc));
    ID3D11Texture2D_GetDesc(tex2d, &desc);
    ok(desc.Format == DXGI_FORMAT_B8G8R8X8_UNORM, "got %#x.\n", desc.Format);
    ok(!desc.Usage, "got %u.\n", desc.Usage);
    ok(desc.BindFlags == (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE), "got %#x.\n", desc.BindFlags);
    ok(!desc.CPUAccessFlags, "got %#x.\n", desc.CPUAccessFlags);
    ok(!desc.MiscFlags, "got %#x.\n", desc.MiscFlags);
    ok(desc.MipLevels == 1, "git %u.\n", desc.MipLevels);
    ok(desc.Width == set_width, "got %u.\n", desc.Width);
    ok(desc.Height == set_height, "got %u.\n", desc.Height);

    ID3D11Texture2D_Release(tex2d);
    IMFDXGIBuffer_Release(dxgi_buffer);
    IMFMediaBuffer_Release(buffer);

    hr = MFCreateCollection(&output_samples);
    ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

    hr = IMFCollection_AddElement(output_samples, (IUnknown *)output.pSample);
    ok(hr == S_OK, "AddElement returned %#lx\n", hr);
    IMFSample_Release(output.pSample);

    ret = check_mf_sample_collection(output_samples, &output_sample_desc_rgb32_crop, L"rgb32frame-crop.bmp");
    ok(ret <= 5, "got %lu%% diff\n", ret);

    IMFCollection_Release(output_samples);


skip_rgb32:
    /* check ABGR32 output with D3D buffers */

    check_mft_set_input_type(transform, nv12_with_aperture, S_OK);
    check_mft_set_output_type(transform, abgr32_no_aperture, S_OK);

    load_resource(L"nv12frame.bmp", &nv12frame_data, &nv12frame_data_len);
    /* skip BMP header and RGB data from the dump */
    length = *(DWORD *)(nv12frame_data + 2);
    nv12frame_data_len = nv12frame_data_len - length;
    nv12frame_data = nv12frame_data + length;
    ok(nv12frame_data_len == 13824, "got length %lu\n", nv12frame_data_len);

    input_sample = create_d3d_sample(allocator, nv12frame_data, nv12frame_data_len);

    hr = IMFTransform_ProcessInput(transform, 0, input_sample, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &info);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(info.dwFlags == MFT_OUTPUT_STREAM_PROVIDES_SAMPLES, "got %#lx.\n", info.dwFlags);

    status = 0;
    memset(&output, 0, sizeof(output));
    hr = IMFTransform_ProcessOutput(transform, 0, 1, &output, &status);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!output.pEvents, "got events\n");
    ok(!!output.pSample, "got no sample\n");
    ok(output.dwStatus == 0, "got %#lx\n", output.dwStatus);
    ok(status == 0, "got %#lx\n", status);
    if (!output.pSample) goto skip_abgr32;

    hr = IMFSample_GetBufferByIndex(output.pSample, 0, &buffer);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFDXGIBuffer, (void **)&dxgi_buffer);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IMFDXGIBuffer_GetResource(dxgi_buffer, &IID_ID3D11Texture2D, (void **)&tex2d);
    ok(hr == S_OK, "got %#lx\n", hr);
    memset(&desc, 0xcc, sizeof(desc));
    ID3D11Texture2D_GetDesc(tex2d, &desc);
    ok(desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM, "got %#x.\n", desc.Format);
    ok(!desc.Usage, "got %u.\n", desc.Usage);
    todo_wine ok(desc.BindFlags == D3D11_BIND_RENDER_TARGET, "got %#x.\n", desc.BindFlags);
    ok(!desc.CPUAccessFlags, "got %#x.\n", desc.CPUAccessFlags);
    ok(!desc.MiscFlags, "got %#x.\n", desc.MiscFlags);
    ok(desc.MipLevels == 1, "git %u.\n", desc.MipLevels);
    ok(desc.Width == set_width, "got %u.\n", desc.Width);
    ok(desc.Height == set_height, "got %u.\n", desc.Height);

    ID3D11Texture2D_Release(tex2d);
    IMFDXGIBuffer_Release(dxgi_buffer);
    IMFMediaBuffer_Release(buffer);

    hr = MFCreateCollection(&output_samples);
    ok(hr == S_OK, "MFCreateCollection returned %#lx\n", hr);

    hr = IMFCollection_AddElement(output_samples, (IUnknown *)output.pSample);
    ok(hr == S_OK, "AddElement returned %#lx\n", hr);
    IMFSample_Release(output.pSample);

    ret = check_mf_sample_collection(output_samples, &output_sample_desc_abgr32_crop, L"abgr32frame-crop.bmp");
    ok(ret <= 8 /* NVIDIA needs 5, AMD needs 8 */, "got %lu%% diff\n", ret);

    IMFCollection_Release(output_samples);


skip_abgr32:
    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_SET_D3D_MANAGER, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    memset(&info, 0xcc, sizeof(info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &info);
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine ok(info.dwFlags == MFT_OUTPUT_STREAM_PROVIDES_SAMPLES, "got %#lx.\n", info.dwFlags);


    hr = IMFVideoSampleAllocator_UninitializeSampleAllocator(allocator);
    ok(hr == S_OK, "AddElement returned %#lx\n", hr);

    IMFSample_Release(input_sample);

failed:
    if (allocator)
        IMFVideoSampleAllocator_Release(allocator);
    if (manager)
        IMFDXGIDeviceManager_Release(manager);
    if (transform)
        IMFTransform_Release(transform);

    MFShutdown();
    CoUninitialize();
}

START_TEST(transform)
{
    winetest_mute_threshold = 1;

    init_functions();

    test_sample_copier();
    test_sample_copier_output_processing();
    test_aac_encoder();
    test_aac_decoder();
    test_aac_decoder_user_data();
    test_wma_encoder();
    test_wma_decoder();
    test_wma_decoder_dmo_input_type();
    test_wma_decoder_dmo_output_type();
    test_h264_encoder();
    test_h264_decoder();
    test_h264_decoder_timestamps();
    test_wmv_encoder();
    test_wmv_decoder();
    test_wmv_decoder_timestamps();
    test_wmv_decoder_dmo_input_type();
    test_wmv_decoder_dmo_output_type();
    test_wmv_decoder_dmo_get_size_info();
    test_wmv_decoder_media_object();
    test_audio_convert();
    test_color_convert();
    test_video_processor(FALSE);
    test_video_processor(TRUE);
    test_mp3_decoder();
    test_iv50_encoder();
    test_iv50_decoder();

    test_h264_with_dxgi_manager();
    test_h264_decoder_concat_streams();

    test_video_processor_with_dxgi_manager();
}
