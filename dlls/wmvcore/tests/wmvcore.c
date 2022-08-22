/*
 * Copyright 2017 Alistair Leslie-Hughes
 * Copyright 2019 Vijay Kiran Kamuju
 * Copyright 2021 Zebediah Figura for CodeWeavers
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

#include <stdbool.h>
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "initguid.h"
#include "wmsdk.h"
#include "wmsecure.h"
#include "amvideo.h"
#include "uuids.h"
#include "wmcodecdsp.h"

#include "wine/test.h"

static const DWORD test_wmv_duration = 20460000;

HRESULT WINAPI WMCreateWriterPriv(IWMWriter **writer);

static BOOL compare_media_types(const WM_MEDIA_TYPE *a, const WM_MEDIA_TYPE *b)
{
    /* We can't use memcmp(), because WM_MEDIA_TYPE has a hole, which sometimes
     * contains junk. */

    return IsEqualGUID(&a->majortype, &b->majortype)
            && IsEqualGUID(&a->subtype, &b->subtype)
            && a->bFixedSizeSamples == b->bFixedSizeSamples
            && a->bTemporalCompression == b->bTemporalCompression
            && a->lSampleSize == b->lSampleSize
            && IsEqualGUID(&a->formattype, &b->formattype)
            && a->pUnk == b->pUnk
            && a->cbFormat == b->cbFormat
            && !memcmp(a->pbFormat, b->pbFormat, a->cbFormat);
}

static void init_audio_type(WM_MEDIA_TYPE *mt, const GUID *subtype, UINT bits, UINT channels, UINT rate)
{
    WAVEFORMATEX *format = (WAVEFORMATEX *)(mt + 1);

    format->wFormatTag = subtype->Data1;
    format->nChannels = channels;
    format->nSamplesPerSec = rate;
    format->wBitsPerSample = bits;
    format->nBlockAlign = format->nChannels * format->wBitsPerSample / 8;
    format->nAvgBytesPerSec = format->nSamplesPerSec * format->nBlockAlign;
    format->cbSize = 0;

    mt->majortype = MEDIATYPE_Audio;
    mt->subtype = *subtype;
    mt->bFixedSizeSamples = TRUE;
    mt->bTemporalCompression = FALSE;
    mt->lSampleSize = format->nBlockAlign;
    mt->formattype = FORMAT_WaveFormatEx;
    mt->pUnk = NULL;
    mt->cbFormat = sizeof(*format);
    mt->pbFormat = (BYTE *)format;
}

static void init_video_type(WM_MEDIA_TYPE *mt, const GUID *subtype, UINT depth, DWORD compression, const RECT *rect)
{
    VIDEOINFOHEADER *video_info = (VIDEOINFOHEADER *)(mt + 1);

    video_info->rcSource = *rect;
    video_info->rcTarget = *rect;
    video_info->dwBitRate = 0;
    video_info->dwBitErrorRate = 0;
    video_info->AvgTimePerFrame = 0;
    video_info->bmiHeader.biSize = sizeof(video_info->bmiHeader);
    video_info->bmiHeader.biWidth = rect->right;
    video_info->bmiHeader.biHeight = rect->bottom;
    video_info->bmiHeader.biPlanes = 1;
    video_info->bmiHeader.biBitCount = depth;
    video_info->bmiHeader.biCompression = compression;
    video_info->bmiHeader.biSizeImage = rect->right * rect->bottom * 4;
    video_info->bmiHeader.biXPelsPerMeter = 0;
    video_info->bmiHeader.biYPelsPerMeter = 0;
    video_info->bmiHeader.biClrUsed = 0;
    video_info->bmiHeader.biClrImportant = 0;

    mt->majortype = MEDIATYPE_Video;
    mt->subtype = *subtype;
    mt->bFixedSizeSamples = TRUE;
    mt->bTemporalCompression = FALSE;
    mt->lSampleSize = video_info->bmiHeader.biSizeImage;
    mt->formattype = FORMAT_VideoInfo;
    mt->pUnk = NULL;
    mt->cbFormat = sizeof(*video_info);
    mt->pbFormat = (BYTE *)video_info;
}

static WCHAR *load_resource(const WCHAR *name)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    wcscat(pathW, name);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create file %s, error %lu.\n",
            wine_dbgstr_w(pathW), GetLastError());

    res = FindResourceW(NULL, name, (LPCWSTR)RT_RCDATA);
    ok(!!res, "Failed to load resource, error %lu.\n", GetLastError());
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    WriteFile(file, ptr, SizeofResource( GetModuleHandleA(NULL), res), &written, NULL);
    ok(written == SizeofResource(GetModuleHandleA(NULL), res), "Failed to write resource.\n");
    CloseHandle(file);

    return pathW;
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static HRESULT check_interface_(unsigned int line, void *iface, REFIID riid, BOOL supported)
{
    HRESULT hr, expected_hr;
    IUnknown *unknown = iface, *out;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(unknown, riid, (void **)&out);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(out);
    return hr;
}

static void test_wmwriter_interfaces(void)
{
    HRESULT hr;
    IWMWriter *writer;

    hr = WMCreateWriter( NULL, &writer );
    ok(hr == S_OK, "WMCreateWriter failed 0x%08lx\n", hr);
    if(FAILED(hr))
    {
        win_skip("Failed to create IWMWriter\n");
        return;
    }

    check_interface(writer, &IID_IWMWriterSink, FALSE);

    check_interface(writer, &IID_IWMWriter, TRUE);
    check_interface(writer, &IID_IWMWriterAdvanced, TRUE);
    check_interface(writer, &IID_IWMWriterAdvanced2, TRUE);
    check_interface(writer, &IID_IWMWriterAdvanced3, TRUE);
    todo_wine check_interface(writer, &IID_IWMWriterPreprocess, TRUE);
    todo_wine check_interface(writer, &IID_IWMHeaderInfo, TRUE);
    todo_wine check_interface(writer, &IID_IWMHeaderInfo2, TRUE);
    todo_wine check_interface(writer, &IID_IWMHeaderInfo3, TRUE);

    IWMWriter_Release(writer);
}

static void test_wmreader_interfaces(void)
{
    HRESULT hr;
    IWMReader *reader;

    hr = WMCreateReader( NULL, 0, &reader );
    ok(hr == S_OK, "WMCreateReader failed 0x%08lx\n", hr);
    if(FAILED(hr))
    {
        win_skip("Failed to create IWMReader\n");
        return;
    }

    check_interface(reader, &IID_IWMDRMReader, FALSE);
    check_interface(reader, &IID_IWMDRMReader2, FALSE);
    check_interface(reader, &IID_IWMDRMReader3, FALSE);
    check_interface(reader, &IID_IWMSyncReader, FALSE);
    check_interface(reader, &IID_IWMSyncReader2, FALSE);

    check_interface(reader, &IID_IReferenceClock, TRUE);
    check_interface(reader, &IID_IWMHeaderInfo, TRUE);
    check_interface(reader, &IID_IWMHeaderInfo2, TRUE);
    check_interface(reader, &IID_IWMHeaderInfo3, TRUE);
    check_interface(reader, &IID_IWMLanguageList, TRUE);
    check_interface(reader, &IID_IWMPacketSize, TRUE);
    check_interface(reader, &IID_IWMPacketSize2, TRUE);
    check_interface(reader, &IID_IWMProfile, TRUE);
    check_interface(reader, &IID_IWMProfile2, TRUE);
    check_interface(reader, &IID_IWMProfile3, TRUE);
    check_interface(reader, &IID_IWMReader, TRUE);
    check_interface(reader, &IID_IWMReaderAccelerator, TRUE);
    check_interface(reader, &IID_IWMReaderAdvanced, TRUE);
    check_interface(reader, &IID_IWMReaderAdvanced2, TRUE);
    check_interface(reader, &IID_IWMReaderAdvanced3, TRUE);
    check_interface(reader, &IID_IWMReaderAdvanced4, TRUE);
    check_interface(reader, &IID_IWMReaderAdvanced5, TRUE);
    check_interface(reader, &IID_IWMReaderAdvanced6, TRUE);
    check_interface(reader, &IID_IWMReaderNetworkConfig, TRUE);
    check_interface(reader, &IID_IWMReaderNetworkConfig2, TRUE);
    check_interface(reader, &IID_IWMReaderPlaylistBurn, TRUE);
    check_interface(reader, &IID_IWMReaderStreamClock, TRUE);
    check_interface(reader, &IID_IWMReaderTimecode, TRUE);
    check_interface(reader, &IID_IWMReaderTypeNegotiation, TRUE);

    IWMReader_Release(reader);
}

static void test_wmsyncreader_interfaces(void)
{
    HRESULT hr;
    IWMSyncReader *reader;

    hr = WMCreateSyncReader( NULL, 0, &reader );
    ok(hr == S_OK, "WMCreateSyncReader failed 0x%08lx\n", hr);
    if(FAILED(hr))
    {
        win_skip("Failed to create IWMSyncReader\n");
        return;
    }

    check_interface(reader, &IID_IReferenceClock, FALSE);
    check_interface(reader, &IID_IWMDRMReader, FALSE);
    check_interface(reader, &IID_IWMDRMReader2, FALSE);
    check_interface(reader, &IID_IWMDRMReader3, FALSE);
    check_interface(reader, &IID_IWMReader, FALSE);
    check_interface(reader, &IID_IWMReaderAccelerator, FALSE);
    check_interface(reader, &IID_IWMReaderAdvanced, FALSE);
    check_interface(reader, &IID_IWMReaderAdvanced2, FALSE);
    check_interface(reader, &IID_IWMReaderAdvanced3, FALSE);
    check_interface(reader, &IID_IWMReaderAdvanced4, FALSE);
    check_interface(reader, &IID_IWMReaderAdvanced5, FALSE);
    check_interface(reader, &IID_IWMReaderAdvanced6, FALSE);
    check_interface(reader, &IID_IWMReaderNetworkConfig, FALSE);
    check_interface(reader, &IID_IWMReaderNetworkConfig2, FALSE);
    check_interface(reader, &IID_IWMReaderStreamClock, FALSE);
    check_interface(reader, &IID_IWMReaderTypeNegotiation, FALSE);

    check_interface(reader, &IID_IWMHeaderInfo, TRUE);
    check_interface(reader, &IID_IWMHeaderInfo2, TRUE);
    check_interface(reader, &IID_IWMHeaderInfo3, TRUE);
    check_interface(reader, &IID_IWMLanguageList, TRUE);
    check_interface(reader, &IID_IWMPacketSize, TRUE);
    check_interface(reader, &IID_IWMPacketSize2, TRUE);
    check_interface(reader, &IID_IWMProfile, TRUE);
    check_interface(reader, &IID_IWMProfile2, TRUE);
    check_interface(reader, &IID_IWMProfile3, TRUE);
    check_interface(reader, &IID_IWMReaderPlaylistBurn, TRUE);
    check_interface(reader, &IID_IWMReaderTimecode, TRUE);
    check_interface(reader, &IID_IWMSyncReader, TRUE);
    check_interface(reader, &IID_IWMSyncReader2, TRUE);

    IWMSyncReader_Release(reader);
}


static void test_profile_manager_interfaces(void)
{
    HRESULT hr;
    IWMProfileManager  *profile;

    hr = WMCreateProfileManager(&profile);
    ok(hr == S_OK, "WMCreateProfileManager failed 0x%08lx\n", hr);
    if(FAILED(hr))
    {
        win_skip("Failed to create IWMProfileManager\n");
        return;
    }

    IWMProfileManager_Release(profile);
}

static void test_WMCreateWriterPriv(void)
{
    IWMWriter *writer, *writer2;
    HRESULT hr;

    hr = WMCreateWriterPriv(&writer);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IWMWriter_QueryInterface(writer, &IID_IWMWriter, (void**)&writer2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    IWMWriter_Release(writer);
    IWMWriter_Release(writer2);
}

static void test_urlextension(void)
{
    HRESULT hr;

    hr = WMCheckURLExtension(NULL);
    ok(hr == E_INVALIDARG, "WMCheckURLExtension failed 0x%08lx\n", hr);
    hr = WMCheckURLExtension(L"test.mkv");
    ok(hr == NS_E_INVALID_NAME, "WMCheckURLExtension failed 0x%08lx\n", hr);
    hr = WMCheckURLExtension(L"test.mp3");
    todo_wine ok(hr == S_OK, "WMCheckURLExtension failed 0x%08lx\n", hr);
    hr = WMCheckURLExtension(L"abcd://test/test.wmv");
    todo_wine ok(hr == S_OK, "WMCheckURLExtension failed 0x%08lx\n", hr);
    hr = WMCheckURLExtension(L"http://test/t.asf?alt=t.mkv");
    todo_wine ok(hr == S_OK, "WMCheckURLExtension failed 0x%08lx\n", hr);
}

static void test_iscontentprotected(void)
{
    HRESULT hr;
    BOOL drm;

    hr = WMIsContentProtected(NULL, NULL);
    ok(hr == E_INVALIDARG, "WMIsContentProtected failed 0x%08lx\n", hr);
    hr = WMIsContentProtected(NULL, &drm);
    ok(hr == E_INVALIDARG, "WMIsContentProtected failed 0x%08lx\n", hr);
    hr = WMIsContentProtected(L"test.mp3", NULL);
    ok(hr == E_INVALIDARG, "WMIsContentProtected failed 0x%08lx\n", hr);
    hr = WMIsContentProtected(L"test.mp3", &drm);
    ok(hr == S_FALSE, "WMIsContentProtected failed 0x%08lx\n", hr);
    ok(drm == FALSE, "got %0dx\n", drm);
}

static LONG outstanding_buffers;

struct buffer
{
    INSSBuffer INSSBuffer_iface;
    LONG refcount;

    DWORD capacity, size;
    BYTE data[1];
};

static inline struct buffer *impl_from_INSSBuffer(INSSBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct buffer, INSSBuffer_iface);
}

static HRESULT WINAPI buffer_QueryInterface(INSSBuffer *iface, REFIID iid, void **out)
{
    if (winetest_debug > 1)
        trace("%04lx: INSSBuffer::QueryInterface(%s)\n", GetCurrentThreadId(), debugstr_guid(iid));

    if (!IsEqualGUID(iid, &IID_INSSBuffer3) && !IsEqualGUID(iid, &IID_IMediaBuffer))
        ok(0, "Unexpected IID %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI buffer_AddRef(INSSBuffer *iface)
{
    struct buffer *buffer = impl_from_INSSBuffer(iface);

    return InterlockedIncrement(&buffer->refcount);
}

static ULONG WINAPI buffer_Release(INSSBuffer *iface)
{
    struct buffer *buffer = impl_from_INSSBuffer(iface);
    ULONG refcount = InterlockedDecrement(&buffer->refcount);

    if (!refcount)
    {
        InterlockedDecrement(&outstanding_buffers);
        free(buffer);
    }
    return refcount;
}

static HRESULT WINAPI buffer_GetLength(INSSBuffer *iface, DWORD *size)
{
    struct buffer *buffer = impl_from_INSSBuffer(iface);

    if (winetest_debug > 1)
        trace("%04lx: INSSBuffer::GetLength()\n", GetCurrentThreadId());

    *size = buffer->size;
    return S_OK;
}

static HRESULT WINAPI buffer_SetLength(INSSBuffer *iface, DWORD size)
{
    struct buffer *buffer = impl_from_INSSBuffer(iface);

    if (winetest_debug > 1)
        trace("%04lx: INSSBuffer::SetLength(%lu)\n", GetCurrentThreadId(), size);

    ok(size <= buffer->capacity, "Got size %lu, buffer capacity %lu.\n", size, buffer->capacity);

    buffer->size = size;
    return S_OK;
}

static HRESULT WINAPI buffer_GetMaxLength(INSSBuffer *iface, DWORD *size)
{
    struct buffer *buffer = impl_from_INSSBuffer(iface);

    if (winetest_debug > 1)
        trace("%04lx: INSSBuffer::GetMaxLength()\n", GetCurrentThreadId());

    *size = buffer->capacity;
    return S_OK;
}

static HRESULT WINAPI buffer_GetBuffer(INSSBuffer *iface, BYTE **data)
{
    struct buffer *buffer = impl_from_INSSBuffer(iface);

    if (winetest_debug > 1)
        trace("%04lx: INSSBuffer::GetBuffer()\n", GetCurrentThreadId());

    *data = buffer->data;
    return S_OK;
}

static HRESULT WINAPI buffer_GetBufferAndLength(INSSBuffer *iface, BYTE **data, DWORD *size)
{
    struct buffer *buffer = impl_from_INSSBuffer(iface);

    if (winetest_debug > 1)
        trace("%04lx: INSSBuffer::GetBufferAndLength()\n", GetCurrentThreadId());

    *size = buffer->size;
    *data = buffer->data;
    return S_OK;
}

static const INSSBufferVtbl buffer_vtbl =
{
    buffer_QueryInterface,
    buffer_AddRef,
    buffer_Release,
    buffer_GetLength,
    buffer_SetLength,
    buffer_GetMaxLength,
    buffer_GetBuffer,
    buffer_GetBufferAndLength,
};

struct teststream
{
    IStream IStream_iface;
    LONG refcount;
    HANDLE file;
};

static struct teststream *impl_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, struct teststream, IStream_iface);
}

static HRESULT WINAPI stream_QueryInterface(IStream *iface, REFIID iid, void **out)
{
    if (winetest_debug > 1)
        trace("%04lx: IStream::QueryInterface(%s)\n", GetCurrentThreadId(), debugstr_guid(iid));

    if (!IsEqualGUID(iid, &IID_IWMGetSecureChannel) && !IsEqualGUID(iid, &IID_IWMIStreamProps))
        ok(0, "Unexpected IID %s.\n", debugstr_guid(iid));

    return E_NOINTERFACE;
}

static ULONG WINAPI stream_AddRef(IStream *iface)
{
    struct teststream *stream = impl_from_IStream(iface);

    return InterlockedIncrement(&stream->refcount);
}

static ULONG WINAPI stream_Release(IStream *iface)
{
    struct teststream *stream = impl_from_IStream(iface);

    return InterlockedDecrement(&stream->refcount);
}

static HRESULT WINAPI stream_Read(IStream *iface, void *data, ULONG size, ULONG *ret_size)
{
    struct teststream *stream = impl_from_IStream(iface);

    if (winetest_debug > 2)
        trace("%04lx: IStream::Read(size %lu)\n", GetCurrentThreadId(), size);

    ok(size > 0, "Got zero size.\n");
    ok(!!ret_size, "Got NULL ret_size pointer.\n");
    if (!ReadFile(stream->file, data, size, ret_size, NULL))
        return HRESULT_FROM_WIN32(GetLastError());
    return S_OK;
}

static HRESULT WINAPI stream_Write(IStream *iface, const void *data, ULONG size, ULONG *ret_size)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Seek(IStream *iface, LARGE_INTEGER offset, DWORD method, ULARGE_INTEGER *ret_offset)
{
    struct teststream *stream = impl_from_IStream(iface);
    LARGE_INTEGER size;

    if (winetest_debug > 2)
        trace("%04lx: IStream::Seek(offset %I64u, method %#lx)\n", GetCurrentThreadId(), offset.QuadPart, method);

    GetFileSizeEx(stream->file, &size);
    ok(offset.QuadPart < size.QuadPart, "Expected offset less than size %I64u, got %I64u.\n",
            size.QuadPart, offset.QuadPart);

    ok(method == STREAM_SEEK_SET, "Got method %#lx.\n", method);
    ok(!ret_offset, "Got unexpected ret_offset pointer %p\n", ret_offset);

    if (!SetFilePointerEx(stream->file, offset, &offset, method))
        return HRESULT_FROM_WIN32(GetLastError());
    return S_OK;
}

static HRESULT WINAPI stream_SetSize(IStream *iface, ULARGE_INTEGER size)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_CopyTo(IStream *iface, IStream *dest, ULARGE_INTEGER size,
        ULARGE_INTEGER *read_size, ULARGE_INTEGER *write_size)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Commit(IStream *iface, DWORD flags)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Revert(IStream *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_LockRegion(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD type)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_UnlockRegion(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD type)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Stat(IStream *iface, STATSTG *stat, DWORD flags)
{
    struct teststream *stream = impl_from_IStream(iface);
    LARGE_INTEGER size;

    if (winetest_debug > 1)
        trace("%04lx: IStream::Stat(flags %#lx)\n", GetCurrentThreadId(), flags);

    ok(flags == STATFLAG_NONAME, "Got flags %#lx.\n", flags);

    stat->type = 0xdeadbeef;
    GetFileSizeEx(stream->file, &size);
    stat->cbSize.QuadPart = size.QuadPart;
    stat->grfMode = 0;
    stat->grfLocksSupported = TRUE;

    return S_OK;
}

static HRESULT WINAPI stream_Clone(IStream *iface, IStream **out)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IStreamVtbl stream_vtbl =
{
    stream_QueryInterface,
    stream_AddRef,
    stream_Release,
    stream_Read,
    stream_Write,
    stream_Seek,
    stream_SetSize,
    stream_CopyTo,
    stream_Commit,
    stream_Revert,
    stream_LockRegion,
    stream_UnlockRegion,
    stream_Stat,
    stream_Clone,
};

static void teststream_init(struct teststream *stream, HANDLE file)
{
    memset(stream, 0, sizeof(*stream));
    stream->IStream_iface.lpVtbl = &stream_vtbl;
    stream->refcount = 1;
    stream->file = file;
}

static void test_reader_attributes(IWMProfile *profile)
{
    WORD size, stream_number, ret_stream_number;
    IWMHeaderInfo *header_info;
    IWMStreamConfig *config;
    WMT_ATTR_DATATYPE type;
    ULONG count, i;
    QWORD duration;
    DWORD dword;
    HRESULT hr;

    IWMProfile_QueryInterface(profile, &IID_IWMHeaderInfo, (void **)&header_info);

    hr = IWMProfile_GetStreamCount(profile, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);

    for (i = 0; i < count; ++i)
    {
        hr = IWMProfile_GetStream(profile, i, &config);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        hr = IWMStreamConfig_GetStreamNumber(config, &stream_number);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ret_stream_number = stream_number;

        size = sizeof(DWORD);
        type = 0xdeadbeef;
        dword = 0xdeadbeef;
        hr = IWMHeaderInfo_GetAttributeByName(header_info, &ret_stream_number,
                L"WM/VideoFrameRate", &type, (BYTE *)&dword, &size);
        ok(hr == ASF_E_NOTFOUND, "Got hr %#lx.\n", hr);
        ok(type == 0xdeadbeef, "Got type %#x.\n", type);
        ok(size == sizeof(DWORD), "Got size %u.\n", size);
        ok(dword == 0xdeadbeef, "Got frame rate %lu.\n", dword);
        ok(ret_stream_number == stream_number, "Expected stream number %u, got %u.\n",
                stream_number, ret_stream_number);

        size = sizeof(QWORD);
        type = 0xdeadbeef;
        duration = 0xdeadbeef;
        hr = IWMHeaderInfo_GetAttributeByName(header_info, &ret_stream_number,
                L"Duration", &type, (BYTE *)&duration, &size);
        ok(hr == ASF_E_NOTFOUND, "Got hr %#lx.\n", hr);
        ok(type == 0xdeadbeef, "Got type %#x.\n", type);
        ok(size == sizeof(QWORD), "Got size %u.\n", size);
        ok(ret_stream_number == stream_number, "Expected stream number %u, got %u.\n",
                stream_number, ret_stream_number);

        size = sizeof(DWORD);
        type = 0xdeadbeef;
        dword = 0xdeadbeef;
        hr = IWMHeaderInfo_GetAttributeByName(header_info, &ret_stream_number,
                L"Seekable", &type, (BYTE *)&dword, &size);
        ok(hr == ASF_E_NOTFOUND, "Got hr %#lx.\n", hr);
        ok(type == 0xdeadbeef, "Got type %#x.\n", type);
        ok(size == sizeof(DWORD), "Got size %u.\n", size);
        ok(ret_stream_number == stream_number, "Expected stream number %u, got %u.\n",
                stream_number, ret_stream_number);

        IWMStreamConfig_Release(config);
    }

    /* WM/VideoFrameRate with a NULL stream number. */

    size = sizeof(DWORD);
    type = 0xdeadbeef;
    dword = 0xdeadbeef;
    hr = IWMHeaderInfo_GetAttributeByName(header_info, NULL,
            L"WM/VideoFrameRate", &type, (BYTE *)&dword, &size);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(type == 0xdeadbeef, "Got type %#x.\n", type);
    ok(size == sizeof(DWORD), "Got size %u.\n", size);
    ok(dword == 0xdeadbeef, "Got frame rate %lu.\n", dword);

    /* And with a zero stream number. */

    stream_number = 0;
    size = sizeof(DWORD);
    type = 0xdeadbeef;
    dword = 0xdeadbeef;
    hr = IWMHeaderInfo_GetAttributeByName(header_info, &stream_number,
            L"WM/VideoFrameRate", &type, (BYTE *)&dword, &size);
    ok(hr == ASF_E_NOTFOUND, "Got hr %#lx.\n", hr);
    ok(type == 0xdeadbeef, "Got type %#x.\n", type);
    ok(size == sizeof(DWORD), "Got size %u.\n", size);
    ok(dword == 0xdeadbeef, "Got frame rate %lu.\n", dword);
    ok(stream_number == 0, "Got stream number %u.\n", stream_number);

    /* Duration with a NULL stream number. */

    size = sizeof(QWORD);
    type = 0xdeadbeef;
    duration = 0xdeadbeef;
    hr = IWMHeaderInfo_GetAttributeByName(header_info, NULL,
            L"Duration", &type, (BYTE *)&duration, &size);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(type == 0xdeadbeef, "Got type %#x.\n", type);
    ok(size == sizeof(QWORD), "Got size %u.\n", size);
    ok(duration == 0xdeadbeef, "Got duration %I64u.\n", duration);

    /* And with a zero stream number. */

    size = sizeof(QWORD);
    type = 0xdeadbeef;
    duration = 0xdeadbeef;
    hr = IWMHeaderInfo_GetAttributeByName(header_info, &stream_number,
            L"Duration", &type, (BYTE *)&duration, &size);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(type == WMT_TYPE_QWORD, "Got type %#x.\n", type);
    ok(size == sizeof(QWORD), "Got size %u.\n", size);
    ok(duration == test_wmv_duration, "Got duration %I64u.\n", duration);
    ok(stream_number == 0, "Got stream number %u.\n", stream_number);

    /* Pass a too-small size. */

    size = sizeof(QWORD) - 1;
    type = 0xdeadbeef;
    duration = 0xdeadbeef;
    hr = IWMHeaderInfo_GetAttributeByName(header_info, &stream_number,
            L"Duration", &type, (BYTE *)&duration, &size);
    ok(hr == ASF_E_BUFFERTOOSMALL, "Got hr %#lx.\n", hr);
    ok(type == 0xdeadbeef, "Got type %#x.\n", type);
    ok(size == sizeof(QWORD), "Got size %u.\n", size);
    ok(duration == 0xdeadbeef, "Got duration %I64u.\n", duration);
    ok(stream_number == 0, "Got stream number %u.\n", stream_number);

    /* Pass a NULL buffer. */

    size = 0xdead;
    type = 0xdeadbeef;
    hr = IWMHeaderInfo_GetAttributeByName(header_info, &stream_number,
            L"Duration", &type, NULL, &size);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(type == WMT_TYPE_QWORD, "Got type %#x.\n", type);
    ok(size == sizeof(QWORD), "Got size %u.\n", size);
    ok(stream_number == 0, "Got stream number %u.\n", stream_number);

    size = sizeof(DWORD);
    type = 0xdeadbeef;
    dword = 0xdeadbeef;
    hr = IWMHeaderInfo_GetAttributeByName(header_info, &stream_number,
            L"Seekable", &type, (BYTE *)&dword, &size);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(type == WMT_TYPE_BOOL, "Got type %#x.\n", type);
    ok(size == sizeof(DWORD), "Got size %u.\n", size);
    ok(dword == TRUE, "Got duration %I64u.\n", duration);
    ok(stream_number == 0, "Got stream number %u.\n", stream_number);

    IWMHeaderInfo_Release(header_info);
}

static void test_sync_reader_selection(IWMSyncReader *reader)
{
    WMT_STREAM_SELECTION selections[2];
    WORD stream_numbers[2];
    QWORD pts, duration;
    INSSBuffer *sample;
    DWORD flags;
    HRESULT hr;

    selections[0] = 0xdeadbeef;
    hr = IWMSyncReader_GetStreamSelected(reader, 0, &selections[0]);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(selections[0] == 0xdeadbeef, "Got selection %#x.\n", selections[0]);

    selections[0] = 0xdeadbeef;
    hr = IWMSyncReader_GetStreamSelected(reader, 1, &selections[0]);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(selections[0] == WMT_ON, "Got selection %#x.\n", selections[0]);

    selections[0] = 0xdeadbeef;
    hr = IWMSyncReader_GetStreamSelected(reader, 2, &selections[0]);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(selections[0] == WMT_ON, "Got selection %#x.\n", selections[0]);

    selections[0] = 0xdeadbeef;
    hr = IWMSyncReader_GetStreamSelected(reader, 3, &selections[0]);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(selections[0] == 0xdeadbeef, "Got selection %#x.\n", selections[0]);

    hr = IWMSyncReader_SetStreamsSelected(reader, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    stream_numbers[0] = 1;
    stream_numbers[1] = 0;
    selections[0] = selections[1] = WMT_OFF;
    hr = IWMSyncReader_SetStreamsSelected(reader, 2, stream_numbers, selections);
    ok(hr == NS_E_INVALID_REQUEST, "Got hr %#lx.\n", hr);

    selections[0] = 0xdeadbeef;
    hr = IWMSyncReader_GetStreamSelected(reader, 1, &selections[0]);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(selections[0] == WMT_ON, "Got selection %#x.\n", selections[0]);

    stream_numbers[0] = stream_numbers[1] = 1;
    selections[0] = selections[1] = WMT_OFF;
    hr = IWMSyncReader_SetStreamsSelected(reader, 2, stream_numbers, selections);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    selections[0] = 0xdeadbeef;
    hr = IWMSyncReader_GetStreamSelected(reader, 1, &selections[0]);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(selections[0] == WMT_OFF, "Got selection %#x.\n", selections[0]);

    selections[0] = 0xdeadbeef;
    hr = IWMSyncReader_GetStreamSelected(reader, 2, &selections[0]);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(selections[0] == WMT_ON, "Got selection %#x.\n", selections[0]);

    hr = IWMSyncReader_GetNextSample(reader, 1, &sample, &pts, &duration, &flags, NULL, NULL);
    ok(hr == NS_E_INVALID_REQUEST, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_GetNextSample(reader, 2, &sample, &pts, &duration, &flags, NULL, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    INSSBuffer_Release(sample);

    for (;;)
    {
        hr = IWMSyncReader_GetNextSample(reader, 2, &sample, &pts, &duration, &flags, NULL, NULL);
        if (hr == NS_E_NO_MORE_SAMPLES)
            break;
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        INSSBuffer_Release(sample);
    }

    hr = IWMSyncReader_GetNextSample(reader, 1, &sample, &pts, &duration, &flags, NULL, NULL);
    ok(hr == NS_E_INVALID_REQUEST, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_SetRange(reader, 0, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_GetNextSample(reader, 0, &sample, &pts, &duration,
            &flags, NULL, &stream_numbers[0]);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(stream_numbers[0] == 2, "Got stream number %u.\n", stream_numbers[0]);
    INSSBuffer_Release(sample);

    for (;;)
    {
        hr = IWMSyncReader_GetNextSample(reader, 0, &sample, &pts, &duration,
                &flags, NULL, &stream_numbers[0]);
        if (hr == NS_E_NO_MORE_SAMPLES)
            break;
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(stream_numbers[0] == 2, "Got stream number %u.\n", stream_numbers[0]);
        INSSBuffer_Release(sample);
    }

    stream_numbers[0] = stream_numbers[1] = 2;
    selections[0] = selections[1] = WMT_OFF;
    hr = IWMSyncReader_SetStreamsSelected(reader, 2, stream_numbers, selections);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_SetRange(reader, 0, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_GetNextSample(reader, 0, &sample, &pts, &duration,
            &flags, NULL, &stream_numbers[0]);
    ok(hr == NS_E_NO_MORE_SAMPLES, "Got hr %#lx.\n", hr);

    stream_numbers[0] = 1;
    stream_numbers[1] = 2;
    selections[0] = selections[1] = WMT_ON;
    hr = IWMSyncReader_SetStreamsSelected(reader, 2, stream_numbers, selections);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
}

static void test_sync_reader_compressed(IWMSyncReader *reader)
{
    QWORD pts, duration;
    INSSBuffer *sample;
    WORD stream_number;
    DWORD flags;
    HRESULT hr;

    hr = IWMSyncReader_SetReadStreamSamples(reader, 0, TRUE);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader_SetReadStreamSamples(reader, 1, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader_SetReadStreamSamples(reader, 2, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader_SetReadStreamSamples(reader, 3, TRUE);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_SetRange(reader, 0, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_GetNextSample(reader, 0, &sample, &pts, &duration, &flags, NULL, &stream_number);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    INSSBuffer_Release(sample);

    for (;;)
    {
        hr = IWMSyncReader_GetNextSample(reader, 0, &sample, &pts, &duration, &flags, NULL, &stream_number);
        if (hr == NS_E_NO_MORE_SAMPLES)
            break;
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        INSSBuffer_Release(sample);
    }

    hr = IWMSyncReader_SetReadStreamSamples(reader, 1, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader_SetReadStreamSamples(reader, 2, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
}

static void check_sync_get_output_setting(IWMSyncReader *reader, DWORD output, const WCHAR *name,
        WMT_ATTR_DATATYPE expect_type, DWORD expect_value, HRESULT expect_hr)
{
    WMT_ATTR_DATATYPE type;
    DWORD value;
    HRESULT hr;
    WORD size;

    winetest_push_context("%s", debugstr_w(name));

    value = 0;
    type = expect_type;
    if (expect_type == WMT_TYPE_BOOL)
        size = sizeof(BOOL);
    else if (expect_type == WMT_TYPE_WORD)
        size = sizeof(WORD);
    else
        size = sizeof(DWORD);

    hr = IWMSyncReader_GetOutputSetting(reader, output, name, &type, (BYTE *)&value, &size);
    todo_wine
    ok(hr == expect_hr, "Got hr %#lx.\n", hr);

    if (SUCCEEDED(hr))
    {
        ok(type == expect_type, "Got type %u.\n", type);
        ok(value == expect_value, "Got value %lu.\n", value);
        if (type == WMT_TYPE_BOOL)
            ok(size == sizeof(BOOL), "Got size %u\n", size);
        else if (type == WMT_TYPE_WORD)
            ok(size == sizeof(WORD), "Got size %u\n", size);
        else
            ok(size == sizeof(DWORD), "Got size %u\n", size);
    }

    winetest_pop_context();
}

static void check_sync_set_output_setting(IWMSyncReader *reader, DWORD output, const WCHAR *name,
        WMT_ATTR_DATATYPE type, DWORD value, HRESULT expect_hr, BOOL todo)
{
    HRESULT hr;
    WORD size;

    winetest_push_context("%s", debugstr_w(name));

    if (type == WMT_TYPE_BOOL)
        size = sizeof(BOOL);
    else if (type == WMT_TYPE_WORD)
        size = sizeof(WORD);
    else
        size = sizeof(DWORD);

    hr = IWMSyncReader_SetOutputSetting(reader, output, name, type, (BYTE *)&value, size);
    todo_wine_if(todo)
    ok(hr == expect_hr, "Got hr %#lx.\n", hr);

    winetest_pop_context();
}

static void test_sync_reader_settings(void)
{
    const WCHAR *filename = load_resource(L"test.wmv");
    struct teststream stream;
    WMT_ATTR_DATATYPE type;
    IWMSyncReader *reader;
    DWORD value;
    HRESULT hr;
    WORD size;
    HANDLE file;
    BOOL ret;

    hr = WMCreateSyncReader(NULL, 0, &reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    type = WMT_TYPE_BOOL;
    size = sizeof(BOOL);
    value = 0;
    hr = IWMSyncReader_GetOutputSetting(reader, 0, L"AllowInterlacedOutput",
            &type, (BYTE *)&value, &size);
    todo_wine
    ok(hr == E_UNEXPECTED, "Got hr %#lx.\n", hr);

    file = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to open %s, error %lu.\n", debugstr_w(file), GetLastError());

    teststream_init(&stream, file);

    hr = IWMSyncReader_OpenStream(reader, &stream.IStream_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(stream.refcount > 1, "Got refcount %ld.\n", stream.refcount);

    check_sync_get_output_setting(reader, 0, L"AllowInterlacedOutput",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_sync_get_output_setting(reader, 0, L"DedicatedDeliveryThread",
            WMT_TYPE_BOOL, 0, NS_E_INVALID_REQUEST);
    check_sync_get_output_setting(reader, 0, L"DeliverOnReceive",
            WMT_TYPE_BOOL, 0, S_OK);
    check_sync_get_output_setting(reader, 0, L"EnableDiscreteOutput",
            WMT_TYPE_BOOL, 0, S_OK);
    check_sync_get_output_setting(reader, 0, L"EnableFrameInterpolation",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_sync_get_output_setting(reader, 0, L"JustInTimeDecode",
            WMT_TYPE_BOOL, 0, NS_E_INVALID_REQUEST);
    check_sync_get_output_setting(reader, 0, L"NeedsPreviousSample",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_sync_get_output_setting(reader, 0, L"ScrambledAudio",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_sync_get_output_setting(reader, 0, L"SingleOutputBuffer",
            WMT_TYPE_BOOL, 0, NS_E_INVALID_REQUEST);
    check_sync_get_output_setting(reader, 0, L"SoftwareScaling",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_sync_get_output_setting(reader, 0, L"VideoSampleDurations",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_sync_get_output_setting(reader, 0, L"EnableWMAProSPDIFOutput",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_sync_get_output_setting(reader, 0, L"StreamLanguage",
            WMT_TYPE_WORD, 0, NS_E_INVALID_REQUEST);
    check_sync_get_output_setting(reader, 0, L"DynamicRangeControl",
            WMT_TYPE_DWORD, -1, S_OK);
    check_sync_get_output_setting(reader, 0, L"EarlyDataDelivery",
            WMT_TYPE_DWORD, 0, S_OK);
    check_sync_get_output_setting(reader, 0, L"SpeakerConfig",
            WMT_TYPE_DWORD, -1, S_OK);

    check_sync_get_output_setting(reader, 1, L"AllowInterlacedOutput",
            WMT_TYPE_BOOL, 0, S_OK);
    check_sync_get_output_setting(reader, 1, L"DedicatedDeliveryThread",
            WMT_TYPE_BOOL, 0, NS_E_INVALID_REQUEST);
    check_sync_get_output_setting(reader, 1, L"DeliverOnReceive",
            WMT_TYPE_BOOL, 0, S_OK);
    check_sync_get_output_setting(reader, 1, L"EnableDiscreteOutput",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_sync_get_output_setting(reader, 1, L"EnableFrameInterpolation",
            WMT_TYPE_BOOL, 0, S_OK);
    check_sync_get_output_setting(reader, 1, L"JustInTimeDecode",
            WMT_TYPE_BOOL, 0, NS_E_INVALID_REQUEST);
    check_sync_get_output_setting(reader, 1, L"NeedsPreviousSample",
            WMT_TYPE_BOOL, 0, NS_E_INVALID_REQUEST);
    check_sync_get_output_setting(reader, 1, L"ScrambledAudio",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_sync_get_output_setting(reader, 1, L"SingleOutputBuffer",
            WMT_TYPE_BOOL, 0, NS_E_INVALID_REQUEST);
    check_sync_get_output_setting(reader, 1, L"SoftwareScaling",
            WMT_TYPE_BOOL, 1, S_OK);
    check_sync_get_output_setting(reader, 1, L"VideoSampleDurations",
            WMT_TYPE_BOOL, 0, S_OK);
    check_sync_get_output_setting(reader, 1, L"EnableWMAProSPDIFOutput",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_sync_get_output_setting(reader, 1, L"StreamLanguage",
            WMT_TYPE_WORD, 0, NS_E_INVALID_REQUEST);
    check_sync_get_output_setting(reader, 1, L"DynamicRangeControl",
            WMT_TYPE_DWORD, 0, E_INVALIDARG);
    check_sync_get_output_setting(reader, 1, L"EarlyDataDelivery",
            WMT_TYPE_DWORD, 0, S_OK);
    check_sync_get_output_setting(reader, 1, L"SpeakerConfig",
            WMT_TYPE_DWORD, 0, E_INVALIDARG);

    check_sync_set_output_setting(reader, 0, L"AllowInterlacedOutput",
            WMT_TYPE_BOOL, 0, E_INVALIDARG, TRUE);
    check_sync_set_output_setting(reader, 0, L"DedicatedDeliveryThread",
            WMT_TYPE_BOOL, 0, NS_E_INVALID_REQUEST, TRUE);
    check_sync_set_output_setting(reader, 0, L"DeliverOnReceive",
            WMT_TYPE_BOOL, 1, S_OK, TRUE);
    check_sync_set_output_setting(reader, 0, L"EnableDiscreteOutput",
            WMT_TYPE_BOOL, 1, S_OK, FALSE);
    check_sync_set_output_setting(reader, 0, L"EnableFrameInterpolation",
            WMT_TYPE_BOOL, 0, E_INVALIDARG, TRUE);
    check_sync_set_output_setting(reader, 0, L"JustInTimeDecode",
            WMT_TYPE_BOOL, 0, NS_E_INVALID_REQUEST, TRUE);
    check_sync_set_output_setting(reader, 0, L"NeedsPreviousSample",
            WMT_TYPE_BOOL, 0, E_INVALIDARG, TRUE);
    check_sync_set_output_setting(reader, 0, L"ScrambledAudio",
            WMT_TYPE_BOOL, 0, E_INVALIDARG, TRUE);
    check_sync_set_output_setting(reader, 0, L"SingleOutputBuffer",
            WMT_TYPE_BOOL, 0, NS_E_INVALID_REQUEST, TRUE);
    check_sync_set_output_setting(reader, 0, L"SoftwareScaling",
            WMT_TYPE_BOOL, 0, E_INVALIDARG, TRUE);
    check_sync_set_output_setting(reader, 0, L"VideoSampleDurations",
            WMT_TYPE_BOOL, 0, E_INVALIDARG, TRUE);
    check_sync_set_output_setting(reader, 0, L"EnableWMAProSPDIFOutput",
            WMT_TYPE_BOOL, 1, S_OK, TRUE);
    check_sync_set_output_setting(reader, 0, L"StreamLanguage",
            WMT_TYPE_WORD, 1, S_OK, TRUE);
    check_sync_set_output_setting(reader, 0, L"DynamicRangeControl",
            WMT_TYPE_DWORD, 1, S_OK, TRUE);
    check_sync_set_output_setting(reader, 0, L"EarlyDataDelivery",
            WMT_TYPE_DWORD, 1000, S_OK, TRUE);
    check_sync_set_output_setting(reader, 0, L"SpeakerConfig",
            WMT_TYPE_DWORD, 1, S_OK, FALSE);

    check_sync_set_output_setting(reader, 1, L"AllowInterlacedOutput",
            WMT_TYPE_BOOL, 1, S_OK, TRUE);
    check_sync_set_output_setting(reader, 1, L"DedicatedDeliveryThread",
            WMT_TYPE_BOOL, 0, NS_E_INVALID_REQUEST, TRUE);
    check_sync_set_output_setting(reader, 1, L"DeliverOnReceive",
            WMT_TYPE_BOOL, 1, S_OK, TRUE);
    check_sync_set_output_setting(reader, 1, L"EnableDiscreteOutput",
            WMT_TYPE_BOOL, 0, E_INVALIDARG, TRUE);
    check_sync_set_output_setting(reader, 1, L"EnableFrameInterpolation",
            WMT_TYPE_BOOL, 1, S_OK, TRUE);
    check_sync_set_output_setting(reader, 1, L"JustInTimeDecode",
            WMT_TYPE_BOOL, 0, NS_E_INVALID_REQUEST, TRUE);
    check_sync_set_output_setting(reader, 1, L"NeedsPreviousSample",
            WMT_TYPE_BOOL, 0, E_INVALIDARG, TRUE);
    check_sync_set_output_setting(reader, 1, L"ScrambledAudio",
            WMT_TYPE_BOOL, 0, E_INVALIDARG, TRUE);
    check_sync_set_output_setting(reader, 1, L"SingleOutputBuffer",
            WMT_TYPE_BOOL, 0, NS_E_INVALID_REQUEST, TRUE);
    check_sync_set_output_setting(reader, 1, L"SoftwareScaling",
            WMT_TYPE_BOOL, 1, S_OK, TRUE);
    check_sync_set_output_setting(reader, 1, L"VideoSampleDurations",
            WMT_TYPE_BOOL, 1, S_OK, FALSE);
    check_sync_set_output_setting(reader, 1, L"EnableWMAProSPDIFOutput",
            WMT_TYPE_BOOL, 0, E_INVALIDARG, TRUE);
    check_sync_set_output_setting(reader, 1, L"StreamLanguage",
            WMT_TYPE_WORD, 1, S_OK, TRUE);
    check_sync_set_output_setting(reader, 1, L"DynamicRangeControl",
            WMT_TYPE_DWORD, 0, E_INVALIDARG, TRUE);
    check_sync_set_output_setting(reader, 1, L"EarlyDataDelivery",
            WMT_TYPE_DWORD, 2000, S_OK, TRUE);
    check_sync_set_output_setting(reader, 1, L"SpeakerConfig",
            WMT_TYPE_DWORD, 0, E_INVALIDARG, TRUE);

    IWMSyncReader_Release(reader);

    ok(stream.refcount == 1, "Got outstanding refcount %ld.\n", stream.refcount);
    CloseHandle(stream.file);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %lu.\n", debugstr_w(filename), GetLastError());
}

static void test_sync_reader_streaming(void)
{
    DWORD size, capacity, flags, output_number, expect_output_number;
    const WCHAR *filename = load_resource(L"test.wmv");
    WORD stream_numbers[2], stream_number;
    IWMStreamConfig *config, *config2;
    bool eos[2] = {0}, first = true;
    struct teststream stream;
    ULONG i, j, count, ref;
    IWMSyncReader *reader;
    IWMProfile *profile;
    QWORD pts, duration;
    INSSBuffer *sample;
    BYTE *data, *data2;
    HANDLE file;
    HRESULT hr;
    BOOL ret;

    file = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to open %s, error %lu.\n", debugstr_w(file), GetLastError());

    teststream_init(&stream, file);

    hr = WMCreateSyncReader(NULL, 0, &reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IWMSyncReader_QueryInterface(reader, &IID_IWMProfile, (void **)&profile);

    hr = IWMSyncReader_OpenStream(reader, &stream.IStream_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(stream.refcount > 1, "Got refcount %ld.\n", stream.refcount);

    hr = IWMProfile_GetStreamCount(profile, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    count = 0xdeadbeef;
    hr = IWMProfile_GetStreamCount(profile, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);

    count = 0xdeadbeef;
    hr = IWMSyncReader_GetOutputCount(reader, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);

    for (i = 0; i < 2; ++i)
    {
        hr = IWMProfile_GetStream(profile, i, &config);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IWMProfile_GetStream(profile, i, &config2);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(config2 != config, "Expected different objects.\n");
        ref = IWMStreamConfig_Release(config2);
        ok(!ref, "Got outstanding refcount %ld.\n", ref);

        stream_numbers[i] = 0xdead;
        hr = IWMStreamConfig_GetStreamNumber(config, &stream_numbers[i]);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(stream_numbers[i] == i + 1, "Got stream number %u.\n", stream_numbers[i]);

        ref = IWMStreamConfig_Release(config);
        ok(!ref, "Got outstanding refcount %ld.\n", ref);
    }

    hr = IWMProfile_GetStream(profile, 2, &config);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    while (!eos[0] || !eos[1])
    {
        for (j = 0; j < 2; ++j)
        {
            stream_number = pts = duration = flags = output_number = 0xdeadbeef;
            hr = IWMSyncReader_GetNextSample(reader, stream_numbers[j], &sample,
                    &pts, &duration, &flags, &output_number, &stream_number);
            if (first)
                ok(hr == S_OK, "Expected at least one valid sample; got hr %#lx.\n", hr);
            else if (eos[j])
                ok(hr == NS_E_NO_MORE_SAMPLES, "Got hr %#lx.\n", hr);
            else
                ok(hr == S_OK || hr == NS_E_NO_MORE_SAMPLES, "Got hr %#lx.\n", hr);

            if (hr == S_OK)
            {
                hr = INSSBuffer_GetBufferAndLength(sample, &data, &size);
                ok(hr == S_OK, "Got hr %#lx.\n", hr);

                hr = INSSBuffer_GetBuffer(sample, &data2);
                ok(hr == S_OK, "Got hr %#lx.\n", hr);
                ok(data2 == data, "Data pointers didn't match.\n");

                hr = INSSBuffer_GetMaxLength(sample, &capacity);
                ok(hr == S_OK, "Got hr %#lx.\n", hr);
                ok(size <= capacity, "Size %lu exceeds capacity %lu.\n", size, capacity);

                hr = INSSBuffer_SetLength(sample, capacity + 1);
                ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

                hr = INSSBuffer_SetLength(sample, capacity - 1);
                ok(hr == S_OK, "Got hr %#lx.\n", hr);

                hr = INSSBuffer_GetBufferAndLength(sample, &data2, &size);
                ok(hr == S_OK, "Got hr %#lx.\n", hr);
                ok(data2 == data, "Data pointers didn't match.\n");
                ok(size == capacity - 1, "Expected size %lu, got %lu.\n", capacity - 1, size);

                ref = INSSBuffer_Release(sample);
                ok(!ref, "Got outstanding refcount %ld.\n", ref);

                hr = IWMSyncReader_GetOutputNumberForStream(reader, stream_number, &expect_output_number);
                ok(hr == S_OK, "Got hr %#lx.\n", hr);
                ok(output_number == expect_output_number, "Expected output number %lu, got %lu.\n",
                        expect_output_number, output_number);
            }
            else
            {
                ok(pts == 0xdeadbeef, "Got PTS %I64u.\n", pts);
                ok(duration == 0xdeadbeef, "Got duration %I64u.\n", duration);
                ok(flags == 0xdeadbeef, "Got flags %#lx.\n", flags);
                ok(output_number == 0xdeadbeef, "Got output number %lu.\n", output_number);
                eos[j] = true;
            }

            ok(stream_number == stream_numbers[j], "Expected stream number %u, got %u.\n",
                    stream_numbers[j], stream_number);
        }
        first = false;
    }

    hr = IWMSyncReader_GetNextSample(reader, stream_numbers[0], &sample,
            &pts, &duration, &flags, NULL, NULL);
    ok(hr == NS_E_NO_MORE_SAMPLES, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_GetNextSample(reader, stream_numbers[1], &sample,
            &pts, &duration, &flags, NULL, NULL);
    ok(hr == NS_E_NO_MORE_SAMPLES, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_SetRange(reader, 0, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_GetNextSample(reader, stream_numbers[0], &sample, &pts, &duration, &flags, NULL, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    INSSBuffer_Release(sample);

    hr = IWMSyncReader_GetNextSample(reader, 0, &sample, &pts, &duration, &flags, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_GetNextSample(reader, 0, &sample, &pts, &duration, &flags, &output_number, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    INSSBuffer_Release(sample);

    hr = IWMSyncReader_GetNextSample(reader, 0, &sample, &pts, &duration, &flags, NULL, &stream_number);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    INSSBuffer_Release(sample);

    for (;;)
    {
        stream_number = pts = duration = flags = output_number = 0xdeadbeef;
        hr = IWMSyncReader_GetNextSample(reader, 0, &sample,
                &pts, &duration, &flags, &output_number, &stream_number);
        ok(hr == S_OK || hr == NS_E_NO_MORE_SAMPLES, "Got hr %#lx.\n", hr);

        if (hr == S_OK)
        {
            hr = INSSBuffer_GetBufferAndLength(sample, &data, &size);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);

            hr = INSSBuffer_GetBuffer(sample, &data2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(data2 == data, "Data pointers didn't match.\n");

            hr = INSSBuffer_GetMaxLength(sample, &capacity);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(size <= capacity, "Size %lu exceeds capacity %lu.\n", size, capacity);

            ref = INSSBuffer_Release(sample);
            ok(!ref, "Got outstanding refcount %ld.\n", ref);
        }
        else
        {
            ok(pts == 0xdeadbeef, "Got PTS %I64u.\n", pts);
            ok(duration == 0xdeadbeef, "Got duration %I64u.\n", duration);
            ok(flags == 0xdeadbeef, "Got flags %#lx.\n", flags);
            ok(output_number == 0xdeadbeef, "Got output number %lu.\n", output_number);
            ok(stream_number == 0xbeef, "Got stream number %u.\n", stream_number);
            break;
        }
    }

    hr = IWMSyncReader_GetNextSample(reader, 0, &sample,
            &pts, &duration, &flags, NULL, &stream_number);
    ok(hr == NS_E_NO_MORE_SAMPLES, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_GetNextSample(reader, stream_numbers[0], &sample,
            &pts, &duration, &flags, NULL, NULL);
    ok(hr == NS_E_NO_MORE_SAMPLES, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_GetNextSample(reader, stream_numbers[1], &sample,
            &pts, &duration, &flags, NULL, NULL);
    ok(hr == NS_E_NO_MORE_SAMPLES, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_SetRange(reader, 0, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    test_sync_reader_selection(reader);
    test_sync_reader_compressed(reader);

    test_reader_attributes(profile);

    hr = IWMSyncReader_Close(reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_Close(reader);
    ok(hr == NS_E_INVALID_REQUEST, "Got hr %#lx.\n", hr);

    ok(stream.refcount == 1, "Got outstanding refcount %ld.\n", stream.refcount);

    SetFilePointer(stream.file, 0, NULL, FILE_BEGIN);
    hr = IWMSyncReader_OpenStream(reader, &stream.IStream_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(stream.refcount > 1, "Got refcount %ld.\n", stream.refcount);

    IWMProfile_Release(profile);
    ref = IWMSyncReader_Release(reader);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    ok(stream.refcount == 1, "Got outstanding refcount %ld.\n", stream.refcount);
    CloseHandle(stream.file);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %lu.\n", debugstr_w(filename), GetLastError());
}

static void check_video_type(const WM_MEDIA_TYPE *mt)
{
    const VIDEOINFOHEADER *video_info = (const VIDEOINFOHEADER *)mt->pbFormat;
    static const RECT rect = {.right = 64, .bottom = 48};

    ok(IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo), "Got format %s.\n", debugstr_guid(&mt->formattype));
    ok(mt->bFixedSizeSamples == TRUE, "Got fixed size %d.\n", mt->bFixedSizeSamples);
    ok(!mt->bTemporalCompression, "Got temporal compression %d.\n", mt->bTemporalCompression);
    ok(!mt->pUnk, "Got pUnk %p.\n", mt->pUnk);

    ok(EqualRect(&video_info->rcSource, &rect), "Got source rect %s.\n", wine_dbgstr_rect(&rect));
    ok(EqualRect(&video_info->rcTarget, &rect), "Got target rect %s.\n", wine_dbgstr_rect(&rect));
    ok(!video_info->dwBitRate, "Got bit rate %lu.\n", video_info->dwBitRate);
    ok(!video_info->dwBitErrorRate, "Got bit error rate %lu.\n", video_info->dwBitErrorRate);
    ok(video_info->bmiHeader.biSize == sizeof(video_info->bmiHeader),
            "Got size %lu.\n", video_info->bmiHeader.biSize);
    ok(video_info->bmiHeader.biWidth == 64, "Got width %ld.\n", video_info->bmiHeader.biWidth);
    ok(video_info->bmiHeader.biHeight == 48, "Got height %ld.\n", video_info->bmiHeader.biHeight);
    ok(video_info->bmiHeader.biPlanes == 1, "Got planes %d.\n", video_info->bmiHeader.biPlanes);
}

static void check_audio_type(const WM_MEDIA_TYPE *mt)
{
    const WAVEFORMATEX *wave_format = (const WAVEFORMATEX *)mt->pbFormat;

    ok(IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_PCM), "Got subtype %s.\n", debugstr_guid(&mt->subtype));
    ok(IsEqualGUID(&mt->formattype, &FORMAT_WaveFormatEx), "Got format %s.\n", debugstr_guid(&mt->formattype));
    ok(mt->bFixedSizeSamples == TRUE, "Got fixed size %d.\n", mt->bFixedSizeSamples);
    ok(!mt->bTemporalCompression, "Got temporal compression %d.\n", mt->bTemporalCompression);
    ok(!mt->pUnk, "Got pUnk %p.\n", mt->pUnk);

    ok(wave_format->wFormatTag == WAVE_FORMAT_PCM, "Got tag %#x.\n", wave_format->wFormatTag);
}

static void test_stream_media_props(IWMStreamConfig *config, const GUID *majortype)
{
    char mt_buffer[2000];
    WM_MEDIA_TYPE *mt = (WM_MEDIA_TYPE *)mt_buffer;
    IWMMediaProps *props;
    DWORD size, ret_size;
    HRESULT hr;

    hr = IWMStreamConfig_QueryInterface(config, &IID_IWMMediaProps, (void **)&props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    size = 0xdeadbeef;
    hr = IWMMediaProps_GetMediaType(props, NULL, &size);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(size != 0xdeadbeef && size >= sizeof(WM_MEDIA_TYPE), "Got size %lu.\n", size);

    ret_size = size - 1;
    hr = IWMMediaProps_GetMediaType(props, mt, &ret_size);
    ok(hr == ASF_E_BUFFERTOOSMALL, "Got hr %#lx.\n", hr);
    ok(ret_size == size, "Expected size %lu, got %lu.\n", size, ret_size);

    ret_size = sizeof(mt_buffer);
    memset(mt_buffer, 0xcc, sizeof(mt_buffer));
    hr = IWMMediaProps_GetMediaType(props, mt, &ret_size);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(ret_size == size, "Expected size %lu, got %lu.\n", size, ret_size);
    ok(size == sizeof(WM_MEDIA_TYPE) + mt->cbFormat, "Expected size %Iu, got %lu.\n",
            sizeof(WM_MEDIA_TYPE) + mt->cbFormat, size);
    ok(IsEqualGUID(&mt->majortype, majortype), "Expected major type %s, got %s.\n",
            debugstr_guid(majortype), debugstr_guid(&mt->majortype));

    IWMMediaProps_Release(props);
}

static void test_sync_reader_types(void)
{
    char mt_buffer[2000], mt2_buffer[2000];
    const WCHAR *filename = load_resource(L"test.wmv");
    IWMOutputMediaProps *output_props, *output_props2;
    WM_MEDIA_TYPE *mt2 = (WM_MEDIA_TYPE *)mt2_buffer;
    WM_MEDIA_TYPE *mt = (WM_MEDIA_TYPE *)mt_buffer;
    bool got_video = false, got_audio = false;
    DWORD size, ret_size, output_number;
    WORD stream_number, stream_number2;
    GUID majortype, majortype2;
    struct teststream stream;
    IWMStreamConfig *config;
    ULONG count, ref, i, j;
    IWMSyncReader *reader;
    IWMProfile *profile;
    HANDLE file;
    HRESULT hr;
    BOOL ret;

    file = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to open %s, error %lu.\n", debugstr_w(file), GetLastError());

    teststream_init(&stream, file);

    hr = WMCreateSyncReader(NULL, 0, &reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IWMSyncReader_QueryInterface(reader, &IID_IWMProfile, (void **)&profile);

    hr = IWMSyncReader_OpenStream(reader, &stream.IStream_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(stream.refcount > 1, "Got refcount %ld.\n", stream.refcount);

    for (i = 0; i < 2; ++i)
    {
        winetest_push_context("Stream %lu", i);

        hr = IWMProfile_GetStream(profile, i, &config);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        stream_number = 0xdead;
        hr = IWMStreamConfig_GetStreamNumber(config, &stream_number);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(stream_number == i + 1, "Got stream number %u.\n", stream_number);

        hr = IWMStreamConfig_GetStreamType(config, &majortype);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        if (!i)
            ok(IsEqualGUID(&majortype, &MEDIATYPE_Video), "Got major type %s.\n", debugstr_guid(&majortype));
        else
            ok(IsEqualGUID(&majortype, &MEDIATYPE_Audio), "Got major type %s.\n", debugstr_guid(&majortype));

        test_stream_media_props(config, &majortype);

        ref = IWMStreamConfig_Release(config);
        ok(!ref, "Got outstanding refcount %ld.\n", ref);

        output_number = 0xdeadbeef;
        hr = IWMSyncReader_GetOutputNumberForStream(reader, stream_number, &output_number);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        todo_wine ok(output_number == 1 - i, "Got output number %lu.\n", output_number);

        stream_number2 = 0xdead;
        hr = IWMSyncReader_GetStreamNumberForOutput(reader, output_number, &stream_number2);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(stream_number2 == stream_number, "Expected stream number %u, got %u.\n", stream_number, stream_number2);

        hr = IWMSyncReader_GetOutputProps(reader, output_number, &output_props);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        ret_size = sizeof(mt_buffer);
        hr = IWMOutputMediaProps_GetMediaType(output_props, mt, &ret_size);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        memset(&majortype2, 0xcc, sizeof(majortype2));
        hr = IWMOutputMediaProps_GetType(output_props, &majortype2);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(IsEqualGUID(&majortype2, &majortype), "Expected major type %s, got %s.\n",
                debugstr_guid(&majortype), debugstr_guid(&majortype2));

        hr = IWMOutputMediaProps_SetMediaType(output_props, NULL);
        ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

        memset(mt2_buffer, 0, sizeof(mt2_buffer));
        hr = IWMOutputMediaProps_SetMediaType(output_props, mt2);
        ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

        if (IsEqualGUID(&majortype, &MEDIATYPE_Audio))
        {
            WAVEFORMATEX *format = (WAVEFORMATEX *)mt->pbFormat;

            init_audio_type(mt2, &MEDIASUBTYPE_IEEE_FLOAT, 32, format->nChannels * 2, format->nSamplesPerSec);
            hr = IWMOutputMediaProps_SetMediaType(output_props, mt2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            hr = IWMSyncReader_SetOutputProps(reader, output_number, output_props);
            ok(hr == NS_E_AUDIO_CODEC_NOT_INSTALLED, "Got hr %#lx.\n", hr);

            init_audio_type(mt2, &MEDIASUBTYPE_PCM, 8, 1, 11025);
            hr = IWMOutputMediaProps_SetMediaType(output_props, mt2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            hr = IWMSyncReader_SetOutputProps(reader, output_number, output_props);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);

            init_audio_type(mt2, &MEDIASUBTYPE_IEEE_FLOAT, 32, format->nChannels, format->nSamplesPerSec / 4);
            hr = IWMOutputMediaProps_SetMediaType(output_props, mt2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            hr = IWMSyncReader_SetOutputProps(reader, output_number, output_props);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
        }
        else
        {
            VIDEOINFO *info = (VIDEOINFO *)mt->pbFormat;
            RECT rect = info->rcTarget;

            init_video_type(mt2, &MEDIASUBTYPE_RGB32, 32, BI_RGB, &rect);
            hr = IWMOutputMediaProps_SetMediaType(output_props, mt2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            hr = IWMSyncReader_SetOutputProps(reader, output_number, output_props);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);

            init_video_type(mt2, &MEDIASUBTYPE_NV12, 12, MAKEFOURCC('N','V','1','2'), &rect);
            hr = IWMOutputMediaProps_SetMediaType(output_props, mt2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            hr = IWMSyncReader_SetOutputProps(reader, output_number, output_props);
            todo_wine
            ok(hr == NS_E_INVALID_OUTPUT_FORMAT, "Got hr %#lx.\n", hr);

            InflateRect(&rect, 10, 10);

            init_video_type(mt2, &MEDIASUBTYPE_RGB32, 32, BI_RGB, &rect);
            hr = IWMOutputMediaProps_SetMediaType(output_props, mt2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            hr = IWMSyncReader_SetOutputProps(reader, output_number, output_props);
            ok(hr == NS_E_INVALID_OUTPUT_FORMAT, "Got hr %#lx.\n", hr);
        }

        hr = IWMOutputMediaProps_SetMediaType(output_props, mt);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        hr = IWMSyncReader_SetOutputProps(reader, output_number, output_props);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        ref = IWMOutputMediaProps_Release(output_props);
        ok(!ref, "Got outstanding refcount %ld.\n", ref);

        if (IsEqualGUID(&majortype, &MEDIATYPE_Audio))
        {
            got_audio = true;
            check_audio_type(mt);
        }
        else
        {
            ok(IsEqualGUID(&majortype, &MEDIATYPE_Video), "Got major type %s.\n", debugstr_guid(&majortype));
            got_video = true;
            check_video_type(mt);
        }

        count = 0;
        hr = IWMSyncReader_GetOutputFormatCount(reader, output_number, &count);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(count > 0, "Got count %lu.\n", count);

        for (j = 0; j < count; ++j)
        {
            winetest_push_context("Format %lu", j);

            hr = IWMSyncReader_GetOutputFormat(reader, output_number, j, &output_props);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);

            hr = IWMSyncReader_GetOutputFormat(reader, output_number, j, &output_props2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(output_props2 != output_props, "Expected different objects.\n");
            ref = IWMOutputMediaProps_Release(output_props2);
            ok(!ref, "Got outstanding refcount %ld.\n", ref);

            size = 0xdeadbeef;
            hr = IWMOutputMediaProps_GetMediaType(output_props, NULL, &size);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(size != 0xdeadbeef && size >= sizeof(WM_MEDIA_TYPE), "Got size %lu.\n", size);

            ret_size = size - 1;
            hr = IWMOutputMediaProps_GetMediaType(output_props, mt, &ret_size);
            ok(hr == ASF_E_BUFFERTOOSMALL, "Got hr %#lx.\n", hr);
            ok(ret_size == size, "Expected size %lu, got %lu.\n", size, ret_size);

            ret_size = sizeof(mt_buffer);
            memset(mt_buffer, 0xcc, sizeof(mt_buffer));
            hr = IWMOutputMediaProps_GetMediaType(output_props, mt, &ret_size);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(ret_size == size, "Expected size %lu, got %lu.\n", size, ret_size);
            ok(size == sizeof(WM_MEDIA_TYPE) + mt->cbFormat, "Expected size %Iu, got %lu.\n",
                    sizeof(WM_MEDIA_TYPE) + mt->cbFormat, size);

            ok(IsEqualGUID(&mt->majortype, &majortype), "Got major type %s.\n", debugstr_guid(&mt->majortype));

            if (IsEqualGUID(&mt->majortype, &MEDIATYPE_Audio))
            {
                ok(IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_PCM), "Got subtype %s.\n", debugstr_guid(&mt->subtype));
                check_audio_type(mt);
            }
            else
            {
                ok(!IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_AYUV), "Got subtype %s.\n", debugstr_guid(&mt->subtype));
                check_video_type(mt);
            }

            memset(&majortype2, 0xcc, sizeof(majortype2));
            hr = IWMOutputMediaProps_GetType(output_props, &majortype2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(IsEqualGUID(&majortype2, &majortype), "Expected major type %s, got %s.\n",
                    debugstr_guid(&majortype), debugstr_guid(&majortype2));

            hr = IWMSyncReader_SetOutputProps(reader, output_number, output_props);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            hr = IWMSyncReader_SetOutputProps(reader, 1 - output_number, output_props);
            if (!i)
                todo_wine ok(hr == ASF_E_BADMEDIATYPE, "Got hr %#lx.\n", hr);
            else
                ok(hr == NS_E_INCOMPATIBLE_FORMAT
                        || hr == NS_E_INVALID_OUTPUT_FORMAT /* win10 */, "Got hr %#lx.\n", hr);
            hr = IWMSyncReader_SetOutputProps(reader, 2, output_props);
            ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

            hr = IWMSyncReader_GetOutputProps(reader, output_number, &output_props2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(output_props2 != output_props, "Expected different objects.\n");

            ret_size = sizeof(mt2_buffer);
            hr = IWMOutputMediaProps_GetMediaType(output_props2, mt2, &ret_size);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(compare_media_types(mt, mt2), "Media types didn't match.\n");

            ref = IWMOutputMediaProps_Release(output_props2);
            ok(!ref, "Got outstanding refcount %ld.\n", ref);
            ref = IWMOutputMediaProps_Release(output_props);
            ok(!ref, "Got outstanding refcount %ld.\n", ref);

            winetest_pop_context();
        }

        hr = IWMSyncReader_GetOutputFormat(reader, output_number, count, &output_props);
        ok(hr == NS_E_INVALID_OUTPUT_FORMAT, "Got hr %#lx.\n", hr);

        hr = IWMSyncReader_GetOutputProps(reader, output_number, &output_props);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IWMSyncReader_GetOutputProps(reader, output_number, &output_props2);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(output_props2 != output_props, "Expected different objects.\n");

        ref = IWMOutputMediaProps_Release(output_props2);
        ok(!ref, "Got outstanding refcount %ld.\n", ref);
        ref = IWMOutputMediaProps_Release(output_props);
        ok(!ref, "Got outstanding refcount %ld.\n", ref);

        winetest_pop_context();
    }

    ok(got_audio, "No audio stream was enumerated.\n");
    ok(got_video, "No video stream was enumerated.\n");

    count = 0xdeadbeef;
    hr = IWMSyncReader_GetOutputFormatCount(reader, 2, &count);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(count == 0xdeadbeef, "Got count %#lx.\n", count);

    output_props = (void *)0xdeadbeef;
    hr = IWMSyncReader_GetOutputProps(reader, 2, &output_props);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(output_props == (void *)0xdeadbeef, "Got output props %p.\n", output_props);

    output_props = (void *)0xdeadbeef;
    hr = IWMSyncReader_GetOutputFormat(reader, 2, 0, &output_props);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(output_props == (void *)0xdeadbeef, "Got output props %p.\n", output_props);

    IWMProfile_Release(profile);
    ref = IWMSyncReader_Release(reader);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    ok(stream.refcount == 1, "Got outstanding refcount %ld.\n", stream.refcount);
    CloseHandle(stream.file);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %lu.\n", debugstr_w(filename), GetLastError());
}

static void test_sync_reader_file(void)
{
    const WCHAR *filename = load_resource(L"test.wmv");
    IWMSyncReader *reader;
    IWMProfile *profile;
    DWORD count;
    HRESULT hr;
    ULONG ref;
    BOOL ret;

    hr = WMCreateSyncReader(NULL, 0, &reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IWMSyncReader_QueryInterface(reader, &IID_IWMProfile, (void **)&profile);

    hr = IWMSyncReader_Open(reader, filename);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    count = 0xdeadbeef;
    hr = IWMSyncReader_GetOutputCount(reader, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);

    hr = IWMSyncReader_Close(reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_Close(reader);
    ok(hr == NS_E_INVALID_REQUEST, "Got hr %#lx.\n", hr);

    hr = IWMSyncReader_Open(reader, filename);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IWMProfile_Release(profile);
    ref = IWMSyncReader_Release(reader);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %lu.\n", debugstr_w(filename), GetLastError());
}

struct callback
{
    IWMReaderCallback IWMReaderCallback_iface;
    IWMReaderCallbackAdvanced IWMReaderCallbackAdvanced_iface;
    IWMReaderAllocatorEx IWMReaderAllocatorEx_iface;
    LONG refcount;
    HANDLE expect_opened, got_opened;
    HANDLE expect_started, got_started;
    HANDLE expect_stopped, got_stopped;
    HANDLE expect_eof, got_eof;
    unsigned int closed_count, started_count, end_of_streaming_count, eof_count, sample_count;
    bool all_streams_off;
    bool allocated_samples;
    void *expect_context;

    bool read_compressed;
    DWORD max_stream_sample_size[2];

    bool dedicated_threads;
    DWORD callback_tid;
    DWORD output_tid[2];

    QWORD last_pts;

    QWORD expect_ontime;
    HANDLE ontime_event;
};

static struct callback *impl_from_IWMReaderCallback(IWMReaderCallback *iface)
{
    return CONTAINING_RECORD(iface, struct callback, IWMReaderCallback_iface);
}

static HRESULT WINAPI callback_QueryInterface(IWMReaderCallback *iface, REFIID iid, void **out)
{
    struct callback *callback = impl_from_IWMReaderCallback(iface);

    if (winetest_debug > 1)
        trace("%04lx: IWMReaderCallback::QueryInterface(%s)\n", GetCurrentThreadId(), debugstr_guid(iid));

    if (IsEqualGUID(iid, &IID_IWMReaderAllocatorEx))
        *out = &callback->IWMReaderAllocatorEx_iface;
    else if (IsEqualGUID(iid, &IID_IWMReaderCallbackAdvanced))
        *out = &callback->IWMReaderCallbackAdvanced_iface;
    else
    {
        if (!IsEqualGUID(iid, &IID_IWMCredentialCallback))
            ok(0, "Unexpected IID %s.\n", debugstr_guid(iid));

        return E_NOINTERFACE;
    }

    IWMReaderCallback_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI callback_AddRef(IWMReaderCallback *iface)
{
    struct callback *callback = impl_from_IWMReaderCallback(iface);

    return InterlockedIncrement(&callback->refcount);
}

static ULONG WINAPI callback_Release(IWMReaderCallback *iface)
{
    struct callback *callback = impl_from_IWMReaderCallback(iface);

    return InterlockedDecrement(&callback->refcount);
}

static HRESULT WINAPI callback_OnStatus(IWMReaderCallback *iface, WMT_STATUS status,
        HRESULT hr, WMT_ATTR_DATATYPE type, BYTE *value, void *context)
{
    struct callback *callback = impl_from_IWMReaderCallback(iface);
    DWORD ret;

    if (winetest_debug > 1)
        trace("%lu: %04lx: IWMReaderCallback::OnStatus(status %u, hr %#lx, type %#x, value %p)\n",
                GetTickCount(), GetCurrentThreadId(), status, hr, type, value);

    switch (status)
    {
        case WMT_OPENED:
            ok(type == WMT_TYPE_DWORD, "Got type %#x.\n", type);
            ok(!*(DWORD *)value, "Got value %#lx.\n", *(DWORD *)value);
            ok(context == (void *)0xdeadbeef, "Got unexpected context %p.\n", context);
            ret = WaitForSingleObject(callback->expect_opened, 100);
            ok(!ret, "Wait timed out.\n");
            SetEvent(callback->got_opened);
            break;

        case WMT_STARTED:
            callback->callback_tid = GetCurrentThreadId();
            ok(type == WMT_TYPE_DWORD, "Got type %#x.\n", type);
            ok(!*(DWORD *)value, "Got value %#lx.\n", *(DWORD *)value);
            ok(context == (void *)callback->expect_context, "Got unexpected context %p.\n", context);
            ret = WaitForSingleObject(callback->expect_started, 100);
            ok(!ret, "Wait timed out.\n");
            callback->end_of_streaming_count = callback->eof_count = callback->sample_count = 0;
            ++callback->started_count;
            ResetEvent(callback->got_eof);
            SetEvent(callback->got_started);
            break;

        case WMT_STOPPED:
            ok(callback->callback_tid == GetCurrentThreadId(), "got wrong thread\n");
            ok(type == WMT_TYPE_DWORD, "Got type %#x.\n", type);
            ok(!*(DWORD *)value, "Got value %#lx.\n", *(DWORD *)value);
            ok(context == (void *)callback->expect_context, "Got unexpected context %p.\n", context);
            ret = WaitForSingleObject(callback->expect_stopped, 100);
            ok(!ret, "Wait timed out.\n");
            SetEvent(callback->got_stopped);
            break;

        case WMT_CLOSED:
            ok(type == WMT_TYPE_DWORD, "Got type %#x.\n", type);
            ok(!*(DWORD *)value, "Got value %#lx.\n", *(DWORD *)value);
            ok(context == (void *)callback->expect_context, "Got unexpected context %p.\n", context);
            ++callback->closed_count;
            break;

        case WMT_END_OF_STREAMING:
            ok(callback->callback_tid == GetCurrentThreadId(), "got wrong thread\n");
            ok(type == WMT_TYPE_DWORD, "Got type %#x.\n", type);
            ok(!*(DWORD *)value, "Got value %#lx.\n", *(DWORD *)value);
            ok(context == (void *)callback->expect_context, "Got unexpected context %p.\n", context);
            ++callback->end_of_streaming_count;
            break;

        case WMT_EOF:
            ok(callback->callback_tid == GetCurrentThreadId(), "got wrong thread\n");
            ok(type == WMT_TYPE_DWORD, "Got type %#x.\n", type);
            ok(!*(DWORD *)value, "Got value %#lx.\n", *(DWORD *)value);
            ok(context == (void *)callback->expect_context, "Got unexpected context %p.\n", context);
            if (callback->all_streams_off)
                ok(callback->sample_count == 0, "Got %u samples.\n", callback->sample_count);
            else
                ok(callback->sample_count > 0, "Got no samples.\n");
            ret = WaitForSingleObject(callback->expect_eof, 100);
            ok(!ret, "Wait timed out.\n");
            ++callback->eof_count;
            SetEvent(callback->got_eof);
            ok(callback->end_of_streaming_count == 1, "Got %u WMT_END_OF_STREAMING callbacks.\n",
                    callback->end_of_streaming_count);
            break;

        /* Not sent when not using IWMReaderAdvanced::DeliverTime(). */
        case WMT_END_OF_SEGMENT:
            ok(callback->callback_tid == GetCurrentThreadId(), "got wrong thread\n");
            ok(type == WMT_TYPE_QWORD, "Got type %#x.\n", type);
            ok(*(QWORD *)value == 3000, "Got value %#lx.\n", *(DWORD *)value);
            ok(context == (void *)callback->expect_context, "Got unexpected context %p.\n", context);
            if (callback->all_streams_off)
                ok(callback->sample_count == 0, "Got %u samples.\n", callback->sample_count);
            else
                ok(callback->sample_count > 0, "Got no samples.\n");
            ok(callback->eof_count == 1, "Got %u WMT_EOF callbacks.\n",
                    callback->eof_count);
            break;

        default:
            ok(0, "Unexpected status %#x.\n", status);
    }

    ok(hr == S_OK || hr == E_ABORT, "Got hr %#lx.\n", hr);
    return S_OK;
}

static void check_async_sample(struct callback *callback, INSSBuffer *sample)
{
    DWORD size, capacity;
    BYTE *data, *data2;
    HRESULT hr;

    if (callback->allocated_samples)
    {
        struct buffer *buffer = impl_from_INSSBuffer(sample);

        ok(sample->lpVtbl == &buffer_vtbl, "Buffer vtbl didn't match.\n");
        ok(buffer->size > 0 && buffer->size <= buffer->capacity, "Got size %ld.\n", buffer->size);
    }
    else
    {
        ok(sample->lpVtbl != &buffer_vtbl, "Buffer vtbl shouldn't match.\n");

        hr = INSSBuffer_GetBufferAndLength(sample, &data, &size);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = INSSBuffer_GetBuffer(sample, &data2);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(data2 == data, "Data pointers didn't match.\n");

        hr = INSSBuffer_GetMaxLength(sample, &capacity);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(size <= capacity, "Size %lu exceeds capacity %lu.\n", size, capacity);

        hr = INSSBuffer_SetLength(sample, capacity + 1);
        ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

        hr = INSSBuffer_SetLength(sample, capacity - 1);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = INSSBuffer_GetBufferAndLength(sample, &data2, &size);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(data2 == data, "Data pointers didn't match.\n");
        ok(size == capacity - 1, "Expected size %lu, got %lu.\n", capacity - 1, size);
    }
}

static HRESULT WINAPI callback_OnSample(IWMReaderCallback *iface, DWORD output,
        QWORD time, QWORD duration, DWORD flags, INSSBuffer *sample, void *context)
{
    struct callback *callback = impl_from_IWMReaderCallback(iface);

    if (winetest_debug > 1)
        trace("%lu: %04lx: IWMReaderCallback::OnSample(output %lu, time %I64u, duration %I64u, flags %#lx)\n",
                GetTickCount(), GetCurrentThreadId(), output, time, duration, flags);

    if (callback->dedicated_threads)
    {
        todo_wine
        ok(callback->callback_tid != GetCurrentThreadId(), "got wrong thread\n");
    }
    else
        ok(callback->callback_tid == GetCurrentThreadId(), "got wrong thread\n");

    if (!callback->output_tid[output])
        callback->output_tid[output] = GetCurrentThreadId();
    else
        ok(callback->output_tid[output] == GetCurrentThreadId(), "got wrong thread\n");

    if (callback->dedicated_threads && callback->output_tid[1 - output])
    {
        todo_wine
        ok(callback->output_tid[1 - output] != GetCurrentThreadId(), "got wrong thread\n");
    }

    ok(context == (void *)callback->expect_context, "Got unexpected context %p.\n", context);

    check_async_sample(callback, sample);

    ok(!callback->read_compressed, "OnSample() should not be called when reading compressed samples.\n");
    ok(callback->started_count > 0, "Got %u WMT_STARTED callbacks.\n", callback->started_count);
    ok(!callback->eof_count, "Got %u WMT_EOF callbacks.\n", callback->eof_count);
    ++callback->sample_count;

    return S_OK;
}

static const IWMReaderCallbackVtbl callback_vtbl =
{
    callback_QueryInterface,
    callback_AddRef,
    callback_Release,
    callback_OnStatus,
    callback_OnSample,
};

static struct callback *impl_from_IWMReaderCallbackAdvanced(IWMReaderCallbackAdvanced *iface)
{
    return CONTAINING_RECORD(iface, struct callback, IWMReaderCallbackAdvanced_iface);
}

static HRESULT WINAPI callback_advanced_QueryInterface(IWMReaderCallbackAdvanced *iface, REFIID iid, void **out)
{
    struct callback *callback = impl_from_IWMReaderCallbackAdvanced(iface);
    return IWMReaderCallback_QueryInterface(&callback->IWMReaderCallback_iface, iid, out);
}

static ULONG WINAPI callback_advanced_AddRef(IWMReaderCallbackAdvanced *iface)
{
    struct callback *callback = impl_from_IWMReaderCallbackAdvanced(iface);
    return IWMReaderCallback_AddRef(&callback->IWMReaderCallback_iface);
}

static ULONG WINAPI callback_advanced_Release(IWMReaderCallbackAdvanced *iface)
{
    struct callback *callback = impl_from_IWMReaderCallbackAdvanced(iface);
    return IWMReaderCallback_Release(&callback->IWMReaderCallback_iface);
}

static HRESULT WINAPI callback_advanced_OnStreamSample(IWMReaderCallbackAdvanced *iface,
        WORD stream_number, QWORD pts, QWORD duration, DWORD flags, INSSBuffer *sample, void *context)
{
    struct callback *callback = impl_from_IWMReaderCallbackAdvanced(iface);

    if (winetest_debug > 1)
        trace("%lu: %04lx: IWMReaderCallbackAdvanced::OnStreamSample(stream %u, pts %I64u, duration %I64u, flags %#lx)\n",
                GetTickCount(), GetCurrentThreadId(), stream_number, pts, duration, flags);

    if (callback->dedicated_threads)
    {
        todo_wine
        ok(callback->callback_tid != GetCurrentThreadId(), "got wrong thread\n");
    }
    else
    {
        ok(callback->callback_tid == GetCurrentThreadId(), "got wrong thread\n");
        ok(callback->last_pts <= pts, "got pts %I64d\n", pts);
        callback->last_pts = pts;
    }

    if (!callback->output_tid[stream_number - 1])
        callback->output_tid[stream_number - 1] = GetCurrentThreadId();
    else
        ok(callback->output_tid[stream_number - 1] == GetCurrentThreadId(), "got wrong thread\n");

    if (callback->dedicated_threads && callback->output_tid[2 - stream_number])
    {
        todo_wine
        ok(callback->output_tid[2 - stream_number] != GetCurrentThreadId(), "got wrong thread\n");
    }

    ok(context == (void *)callback->expect_context, "Got unexpected context %p.\n", context);

    check_async_sample(callback, sample);

    ok(callback->read_compressed, "OnStreamSample() should not be called unless reading compressed samples.\n");
    ok(callback->started_count > 0, "Got %u WMT_STARTED callbacks.\n", callback->started_count);
    ok(!callback->eof_count, "Got %u WMT_EOF callbacks.\n", callback->eof_count);
    ++callback->sample_count;

    return S_OK;
}

static HRESULT WINAPI callback_advanced_OnTime(IWMReaderCallbackAdvanced *iface, QWORD time, void *context)
{
    struct callback *callback = impl_from_IWMReaderCallbackAdvanced(iface);

    if (winetest_debug > 1)
        trace("%lu: %04lx: IWMReaderCallbackAdvanced::OnTime(time %I64u)\n",
                GetTickCount(), GetCurrentThreadId(), time);

    ok(callback->callback_tid == GetCurrentThreadId(), "got wrong thread\n");

    ok(time == callback->expect_ontime, "Got time %I64u.\n", time);
    ok(context == (void *)callback->expect_context, "Got unexpected context %p.\n", context);
    SetEvent(callback->ontime_event);
    return S_OK;
}

static HRESULT WINAPI callback_advanced_OnStreamSelection(IWMReaderCallbackAdvanced *iface,
        WORD count, WORD *stream_numbers, WMT_STREAM_SELECTION *selections, void *context)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI callback_advanced_OnOutputPropsChanged(IWMReaderCallbackAdvanced *iface,
        DWORD output, WM_MEDIA_TYPE *mt, void *context)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI callback_advanced_AllocateForStream(IWMReaderCallbackAdvanced *iface,
        WORD stream_number, DWORD size, INSSBuffer **sample, void *context)
{
    struct callback *callback = impl_from_IWMReaderCallbackAdvanced(iface);
    DWORD max_size = callback->max_stream_sample_size[stream_number - 1];
    struct buffer *object;

    if (winetest_debug > 1)
        trace("%lu: %04lx: IWMReaderCallbackAdvanced::AllocateForStream(output %u, size %lu)\n",
                GetTickCount(), GetCurrentThreadId(), stream_number, size);

    todo_wine
    ok(callback->callback_tid != GetCurrentThreadId(), "got wrong thread\n");
    todo_wine_if(callback->output_tid[stream_number - 1])
    ok(callback->output_tid[stream_number - 1] != GetCurrentThreadId(), "got wrong thread\n");

    ok(callback->read_compressed, "AllocateForStream() should only be called when reading compressed samples.\n");
    ok(callback->allocated_samples, "AllocateForStream() should only be called when using a custom allocator.\n");

    ok(size <= max_size, "Got size %lu, max stream sample size %lu.\n", size, max_size);

    if (!(object = malloc(offsetof(struct buffer, data[size]))))
        return E_OUTOFMEMORY;

    size = max(size, 65536);

    object->INSSBuffer_iface.lpVtbl = &buffer_vtbl;
    object->refcount = 1;
    object->capacity = size;
    /* Native seems to break if we set the size to zero. */
    object->size = size;
    *sample = &object->INSSBuffer_iface;

    InterlockedIncrement(&outstanding_buffers);

    ok(!context, "Got unexpected context %p.\n", context);
    return S_OK;
}

static HRESULT WINAPI callback_advanced_AllocateForOutput(IWMReaderCallbackAdvanced *iface,
        DWORD output, DWORD size, INSSBuffer **sample, void *context)
{
    struct callback *callback = impl_from_IWMReaderCallbackAdvanced(iface);
    struct buffer *object;

    if (winetest_debug > 1)
        trace("%lu: %04lx: IWMReaderCallbackAdvanced::AllocateForOutput(output %lu, size %lu)\n",
                GetTickCount(), GetCurrentThreadId(), output, size);

    todo_wine
    ok(callback->callback_tid != GetCurrentThreadId(), "got wrong thread\n");
    todo_wine_if(callback->output_tid[output])
    ok(callback->output_tid[output] != GetCurrentThreadId(), "got wrong thread\n");

    if (!callback->read_compressed)
    {
        /* Actually AllocateForOutput() isn't called when reading compressed
         * samples either, but native seems to have some sort of race that
         * causes one call to this function to happen in
         * test_async_reader_allocate_compressed(). */
        ok(callback->allocated_samples,
                "AllocateForOutput() should only be called when using a custom allocator.\n");
    }

    if (!(object = malloc(offsetof(struct buffer, data[size]))))
        return E_OUTOFMEMORY;

    size = max(size, 65536);

    object->INSSBuffer_iface.lpVtbl = &buffer_vtbl;
    object->refcount = 1;
    object->capacity = size;
    object->size = 0;
    *sample = &object->INSSBuffer_iface;

    InterlockedIncrement(&outstanding_buffers);

    ok(!context, "Got unexpected context %p.\n", context);
    return S_OK;
}

static const IWMReaderCallbackAdvancedVtbl callback_advanced_vtbl =
{
    callback_advanced_QueryInterface,
    callback_advanced_AddRef,
    callback_advanced_Release,
    callback_advanced_OnStreamSample,
    callback_advanced_OnTime,
    callback_advanced_OnStreamSelection,
    callback_advanced_OnOutputPropsChanged,
    callback_advanced_AllocateForStream,
    callback_advanced_AllocateForOutput,
};

static struct callback *impl_from_IWMReaderAllocatorEx(IWMReaderAllocatorEx *iface)
{
    return CONTAINING_RECORD(iface, struct callback, IWMReaderAllocatorEx_iface);
}

static HRESULT WINAPI callback_allocator_QueryInterface(IWMReaderAllocatorEx *iface, REFIID iid, void **out)
{
    struct callback *callback = impl_from_IWMReaderAllocatorEx(iface);
    return IWMReaderCallback_QueryInterface(&callback->IWMReaderCallback_iface, iid, out);
}

static ULONG WINAPI callback_allocator_AddRef(IWMReaderAllocatorEx *iface)
{
    struct callback *callback = impl_from_IWMReaderAllocatorEx(iface);
    return IWMReaderCallback_AddRef(&callback->IWMReaderCallback_iface);
}

static ULONG WINAPI callback_allocator_Release(IWMReaderAllocatorEx *iface)
{
    struct callback *callback = impl_from_IWMReaderAllocatorEx(iface);
    return IWMReaderCallback_Release(&callback->IWMReaderCallback_iface);
}

static HRESULT WINAPI callback_allocator_AllocateForStreamEx(IWMReaderAllocatorEx *iface,
        WORD stream_number, DWORD size, INSSBuffer **sample, DWORD flags,
        QWORD pts, QWORD duration, void *context)
{
    struct callback *callback = impl_from_IWMReaderAllocatorEx(iface);
    struct buffer *object;

    ok(callback->allocated_samples, "Unexpected call.\n");
    ok(stream_number, "got stream_number %u.\n", stream_number);
    ok(!flags, "got flags %#lx.\n", flags);
    ok(!pts, "got pts %I64d.\n", pts);
    ok(!duration, "got duration %I64d.\n", duration);
    ok(!context, "got context %p.\n", context);

    if (!(object = malloc(offsetof(struct buffer, data[size]))))
        return E_OUTOFMEMORY;
    object->INSSBuffer_iface.lpVtbl = &buffer_vtbl;
    object->refcount = 1;
    object->capacity = size;
    object->size = size;

    *sample = &object->INSSBuffer_iface;
    InterlockedIncrement(&outstanding_buffers);
    return S_OK;
}

static HRESULT WINAPI callback_allocator_AllocateForOutputEx(IWMReaderAllocatorEx *iface,
        DWORD output, DWORD size, INSSBuffer **sample, DWORD flags,
        QWORD pts, QWORD duration, void *context)
{
    struct callback *callback = impl_from_IWMReaderAllocatorEx(iface);
    struct buffer *object;

    ok(callback->allocated_samples, "Unexpected call.\n");
    ok(!flags, "got flags %#lx.\n", flags);
    ok(!pts, "got pts %I64d.\n", pts);
    ok(!duration, "got duration %I64d.\n", duration);
    ok(!context, "got context %p.\n", context);

    if (!(object = malloc(offsetof(struct buffer, data[size]))))
        return E_OUTOFMEMORY;
    object->INSSBuffer_iface.lpVtbl = &buffer_vtbl;
    object->refcount = 1;
    object->capacity = size;
    object->size = size;

    *sample = &object->INSSBuffer_iface;
    InterlockedIncrement(&outstanding_buffers);
    return S_OK;
}

static const IWMReaderAllocatorExVtbl callback_allocator_vtbl =
{
    callback_allocator_QueryInterface,
    callback_allocator_AddRef,
    callback_allocator_Release,
    callback_allocator_AllocateForStreamEx,
    callback_allocator_AllocateForOutputEx,
};

static void callback_init(struct callback *callback)
{
    memset(callback, 0, sizeof(*callback));
    callback->IWMReaderCallback_iface.lpVtbl = &callback_vtbl;
    callback->IWMReaderCallbackAdvanced_iface.lpVtbl = &callback_advanced_vtbl;
    callback->IWMReaderAllocatorEx_iface.lpVtbl = &callback_allocator_vtbl;
    callback->refcount = 1;
    callback->expect_opened = CreateEventW(NULL, FALSE, FALSE, NULL);
    callback->got_opened = CreateEventW(NULL, FALSE, FALSE, NULL);
    callback->expect_started = CreateEventW(NULL, FALSE, FALSE, NULL);
    callback->got_started = CreateEventW(NULL, FALSE, FALSE, NULL);
    callback->expect_stopped = CreateEventW(NULL, FALSE, FALSE, NULL);
    callback->got_stopped = CreateEventW(NULL, FALSE, FALSE, NULL);
    callback->expect_eof = CreateEventW(NULL, FALSE, FALSE, NULL);
    callback->got_eof = CreateEventW(NULL, FALSE, FALSE, NULL);
    callback->ontime_event = CreateEventW(NULL, FALSE, FALSE, NULL);
}

static void callback_cleanup(struct callback *callback)
{
    CloseHandle(callback->got_opened);
    CloseHandle(callback->expect_opened);
    CloseHandle(callback->got_started);
    CloseHandle(callback->expect_started);
    CloseHandle(callback->got_stopped);
    CloseHandle(callback->expect_stopped);
    CloseHandle(callback->got_eof);
    CloseHandle(callback->expect_eof);
    CloseHandle(callback->ontime_event);
}

#define wait_opened_callback(a) wait_opened_callback_(__LINE__, a)
static void wait_opened_callback_(int line, struct callback *callback)
{
    DWORD ret;

    ret = WaitForSingleObject(callback->got_opened, 0);
    ok_(__FILE__, line)(ret == WAIT_TIMEOUT, "Got unexpected WMT_OPENED.\n");
    SetEvent(callback->expect_opened);
    ret = WaitForSingleObject(callback->got_opened, 1000);
    ok_(__FILE__, line)(!ret, "Wait timed out.\n");
}

#define wait_started_callback(a) wait_started_callback_(__LINE__, a)
static void wait_started_callback_(int line, struct callback *callback)
{
    DWORD ret;

    ret = WaitForSingleObject(callback->got_started, 0);
    ok_(__FILE__, line)(ret == WAIT_TIMEOUT, "Got unexpected WMT_STARTED.\n");
    SetEvent(callback->expect_started);
    ret = WaitForSingleObject(callback->got_started, 1000);
    ok_(__FILE__, line)(!ret, "Wait timed out.\n");
}

#define wait_stopped_callback(a) wait_stopped_callback_(__LINE__, a)
static void wait_stopped_callback_(int line, struct callback *callback)
{
    DWORD ret;

    ret = WaitForSingleObject(callback->got_stopped, 0);
    ok_(__FILE__, line)(ret == WAIT_TIMEOUT, "Got unexpected WMT_STOPPED.\n");
    SetEvent(callback->expect_stopped);
    ret = WaitForSingleObject(callback->got_stopped, 1000);
    ok_(__FILE__, line)(!ret, "Wait timed out.\n");
}

#define wait_eof_callback(a) wait_eof_callback_(__LINE__, a)
static void wait_eof_callback_(int line, struct callback *callback)
{
    DWORD ret;

    ret = WaitForSingleObject(callback->got_eof, 0);
    ok_(__FILE__, line)(ret == WAIT_TIMEOUT, "Got unexpected WMT_EOF.\n");
    SetEvent(callback->expect_eof);
    ret = WaitForSingleObject(callback->got_eof, 1000);
    ok_(__FILE__, line)(!ret, "Wait timed out.\n");
    ok_(__FILE__, line)(callback->eof_count == 1, "Got %u WMT_EOF callbacks.\n", callback->eof_count);
}

static void check_async_get_output_setting(IWMReaderAdvanced2 *reader, DWORD output, const WCHAR *name,
        WMT_ATTR_DATATYPE expect_type, DWORD expect_value, HRESULT expect_hr)
{
    WMT_ATTR_DATATYPE type;
    DWORD value;
    HRESULT hr;
    WORD size;

    winetest_push_context("%s", debugstr_w(name));

    value = 0;
    type = expect_type;
    if (expect_type == WMT_TYPE_BOOL)
        size = sizeof(BOOL);
    else if (expect_type == WMT_TYPE_WORD)
        size = sizeof(WORD);
    else
        size = sizeof(DWORD);

    hr = IWMReaderAdvanced2_GetOutputSetting(reader, output, name, &type, (BYTE *)&value, &size);
    todo_wine
    ok(hr == expect_hr, "Got hr %#lx.\n", hr);

    if (SUCCEEDED(hr))
    {
        ok(type == expect_type, "Got type %u.\n", type);
        ok(value == expect_value, "Got value %lu.\n", value);
        if (type == WMT_TYPE_BOOL)
            ok(size == sizeof(BOOL), "Got size %u\n", size);
        else if (type == WMT_TYPE_WORD)
            ok(size == sizeof(WORD), "Got size %u\n", size);
        else
            ok(size == sizeof(DWORD), "Got size %u\n", size);
    }

    winetest_pop_context();
}

static void check_async_set_output_setting(IWMReaderAdvanced2 *reader, DWORD output, const WCHAR *name,
        WMT_ATTR_DATATYPE type, DWORD value, HRESULT expect_hr)
{
    HRESULT hr;
    WORD size;

    winetest_push_context("%s", debugstr_w(name));

    if (type == WMT_TYPE_BOOL)
        size = sizeof(BOOL);
    else if (type == WMT_TYPE_WORD)
        size = sizeof(WORD);
    else
        size = sizeof(DWORD);

    hr = IWMReaderAdvanced2_SetOutputSetting(reader, output, name, type, (BYTE *)&value, size);
    todo_wine
    ok(hr == expect_hr, "Got hr %#lx.\n", hr);

    winetest_pop_context();
}

static void run_async_reader(IWMReader *reader, IWMReaderAdvanced2 *advanced, struct callback *callback)
{
    HRESULT hr;

    callback->closed_count = 0;
    callback->started_count = 0;
    callback->sample_count = 0;
    callback->end_of_streaming_count = 0;
    callback->eof_count = 0;
    callback->callback_tid = 0;
    callback->last_pts = 0;
    memset(callback->output_tid, 0, sizeof(callback->output_tid));

    check_async_set_output_setting(advanced, 0, L"DedicatedDeliveryThread",
            WMT_TYPE_BOOL, callback->dedicated_threads, S_OK);
    check_async_set_output_setting(advanced, 1, L"DedicatedDeliveryThread",
            WMT_TYPE_BOOL, callback->dedicated_threads, S_OK);

    callback->expect_context = (void *)0xfacade;
    hr = IWMReader_Start(reader, 0, 0, 1.0f, (void *)0xfacade);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* When all streams are disabled we may get an EOF callback right
     * after the first Start, or after the second if it took a bit more
     * time to be processed. This is unpredictable so skip the test
     */
    if (!callback->all_streams_off)
    {
        hr = IWMReader_Start(reader, 0, 0, 1.0f, (void *)0xfacade);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        wait_started_callback(callback);
    }

    wait_started_callback(callback);

    hr = IWMReaderAdvanced2_SetUserProvidedClock(advanced, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_DeliverTime(advanced, test_wmv_duration * 2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    wait_eof_callback(callback);

    hr = IWMReader_Stop(reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReader_Stop(reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    wait_stopped_callback(callback);
    wait_stopped_callback(callback);

    ok(!outstanding_buffers, "Got %ld outstanding buffers.\n", outstanding_buffers);
}

static void test_async_reader_allocate(IWMReader *reader,
        IWMReaderAdvanced2 *advanced, struct callback *callback)
{
    BOOL allocate;
    HRESULT hr;

    callback->allocated_samples = true;

    hr = IWMReaderAdvanced2_GetAllocateForOutput(advanced, 0, &allocate);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(!allocate, "Got allocate %d.\n", allocate);
    hr = IWMReaderAdvanced2_GetAllocateForOutput(advanced, 1, &allocate);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(!allocate, "Got allocate %d.\n", allocate);
    hr = IWMReaderAdvanced2_GetAllocateForOutput(advanced, 2, &allocate);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IWMReaderAdvanced2_GetAllocateForStream(advanced, 0, &allocate);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_GetAllocateForStream(advanced, 1, &allocate);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(!allocate, "Got allocate %d.\n", allocate);
    hr = IWMReaderAdvanced2_GetAllocateForStream(advanced, 2, &allocate);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(!allocate, "Got allocate %d.\n", allocate);
    hr = IWMReaderAdvanced2_GetAllocateForStream(advanced, 3, &allocate);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IWMReaderAdvanced2_SetAllocateForOutput(advanced, 0, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetAllocateForOutput(advanced, 1, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetAllocateForOutput(advanced, 2, TRUE);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IWMReaderAdvanced2_GetAllocateForOutput(advanced, 0, &allocate);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(allocate == TRUE, "Got allocate %d.\n", allocate);
    hr = IWMReaderAdvanced2_GetAllocateForOutput(advanced, 1, &allocate);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(allocate == TRUE, "Got allocate %d.\n", allocate);

    hr = IWMReaderAdvanced2_GetAllocateForStream(advanced, 1, &allocate);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(!allocate, "Got allocate %d.\n", allocate);
    hr = IWMReaderAdvanced2_GetAllocateForStream(advanced, 2, &allocate);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(!allocate, "Got allocate %d.\n", allocate);

    run_async_reader(reader, advanced, callback);

    callback->allocated_samples = false;

    hr = IWMReaderAdvanced2_SetAllocateForOutput(advanced, 0, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetAllocateForOutput(advanced, 1, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IWMReaderAdvanced2_SetAllocateForStream(advanced, 0, TRUE);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetAllocateForStream(advanced, 1, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetAllocateForStream(advanced, 2, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetAllocateForStream(advanced, 3, TRUE);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IWMReaderAdvanced2_GetAllocateForOutput(advanced, 0, &allocate);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(!allocate, "Got allocate %d.\n", allocate);
    hr = IWMReaderAdvanced2_GetAllocateForOutput(advanced, 1, &allocate);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(!allocate, "Got allocate %d.\n", allocate);

    hr = IWMReaderAdvanced2_GetAllocateForStream(advanced, 1, &allocate);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(allocate == TRUE, "Got allocate %d.\n", allocate);
    hr = IWMReaderAdvanced2_GetAllocateForStream(advanced, 2, &allocate);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(allocate == TRUE, "Got allocate %d.\n", allocate);

    run_async_reader(reader, advanced, callback);

    hr = IWMReaderAdvanced2_SetAllocateForStream(advanced, 1, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetAllocateForStream(advanced, 2, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
}

static void test_async_reader_selection(IWMReader *reader,
        IWMReaderAdvanced2 *advanced, struct callback *callback)
{
    WMT_STREAM_SELECTION selections[2];
    WORD stream_numbers[2];
    HRESULT hr;

    selections[0] = 0xdeadbeef;
    hr = IWMReaderAdvanced2_GetStreamSelected(advanced, 0, &selections[0]);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(selections[0] == 0xdeadbeef, "Got selection %#x.\n", selections[0]);

    selections[0] = 0xdeadbeef;
    hr = IWMReaderAdvanced2_GetStreamSelected(advanced, 1, &selections[0]);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(selections[0] == WMT_ON, "Got selection %#x.\n", selections[0]);

    selections[0] = 0xdeadbeef;
    hr = IWMReaderAdvanced2_GetStreamSelected(advanced, 2, &selections[0]);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(selections[0] == WMT_ON, "Got selection %#x.\n", selections[0]);

    selections[0] = 0xdeadbeef;
    hr = IWMReaderAdvanced2_GetStreamSelected(advanced, 3, &selections[0]);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(selections[0] == 0xdeadbeef, "Got selection %#x.\n", selections[0]);

    hr = IWMReaderAdvanced2_SetStreamsSelected(advanced, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    stream_numbers[0] = 1;
    stream_numbers[1] = 0;
    selections[0] = selections[1] = WMT_OFF;
    hr = IWMReaderAdvanced2_SetStreamsSelected(advanced, 2, stream_numbers, selections);
    ok(hr == NS_E_INVALID_REQUEST, "Got hr %#lx.\n", hr);

    selections[0] = 0xdeadbeef;
    hr = IWMReaderAdvanced2_GetStreamSelected(advanced, 1, &selections[0]);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(selections[0] == WMT_ON, "Got selection %#x.\n", selections[0]);

    stream_numbers[0] = stream_numbers[1] = 1;
    selections[0] = selections[1] = WMT_OFF;
    hr = IWMReaderAdvanced2_SetStreamsSelected(advanced, 2, stream_numbers, selections);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    selections[0] = 0xdeadbeef;
    hr = IWMReaderAdvanced2_GetStreamSelected(advanced, 1, &selections[0]);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(selections[0] == WMT_OFF, "Got selection %#x.\n", selections[0]);

    selections[0] = 0xdeadbeef;
    hr = IWMReaderAdvanced2_GetStreamSelected(advanced, 2, &selections[0]);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(selections[0] == WMT_ON, "Got selection %#x.\n", selections[0]);

    run_async_reader(reader, advanced, callback);

    stream_numbers[0] = stream_numbers[1] = 2;
    selections[0] = selections[1] = WMT_OFF;
    hr = IWMReaderAdvanced2_SetStreamsSelected(advanced, 2, stream_numbers, selections);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    callback->all_streams_off = true;
    run_async_reader(reader, advanced, callback);
    callback->all_streams_off = false;

    stream_numbers[0] = 1;
    stream_numbers[1] = 2;
    selections[0] = selections[1] = WMT_ON;
    hr = IWMReaderAdvanced2_SetStreamsSelected(advanced, 2, stream_numbers, selections);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
}

static void test_async_reader_compressed(IWMReader *reader,
        IWMReaderAdvanced2 *advanced, struct callback *callback)
{
    HRESULT hr;

    hr = IWMReaderAdvanced2_GetMaxStreamSampleSize(advanced, 0, &callback->max_stream_sample_size[0]);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_GetMaxStreamSampleSize(advanced, 3, &callback->max_stream_sample_size[0]);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_GetMaxStreamSampleSize(advanced, 1, &callback->max_stream_sample_size[0]);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(callback->max_stream_sample_size[0] > 0, "Expected nonzero size.\n");
    hr = IWMReaderAdvanced2_GetMaxStreamSampleSize(advanced, 2, &callback->max_stream_sample_size[1]);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(callback->max_stream_sample_size[1] > 0, "Expected nonzero size.\n");

    hr = IWMReaderAdvanced2_SetReceiveStreamSamples(advanced, 0, TRUE);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetReceiveStreamSamples(advanced, 3, TRUE);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetReceiveStreamSamples(advanced, 1, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetReceiveStreamSamples(advanced, 2, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    callback->read_compressed = true;
    run_async_reader(reader, advanced, callback);
    callback->read_compressed = false;

    hr = IWMReaderAdvanced2_SetReceiveStreamSamples(advanced, 1, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetReceiveStreamSamples(advanced, 2, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
}

static void test_async_reader_allocate_compressed(IWMReader *reader,
        IWMReaderAdvanced2 *advanced, struct callback *callback)
{
    HRESULT hr;

    callback->read_compressed = true;

    hr = IWMReaderAdvanced2_SetReceiveStreamSamples(advanced, 1, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetReceiveStreamSamples(advanced, 2, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    callback->allocated_samples = true;

    hr = IWMReaderAdvanced2_SetAllocateForStream(advanced, 1, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetAllocateForStream(advanced, 2, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    run_async_reader(reader, advanced, callback);

    hr = IWMReaderAdvanced2_SetAllocateForStream(advanced, 1, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetAllocateForStream(advanced, 2, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IWMReaderAdvanced2_SetAllocateForOutput(advanced, 0, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetAllocateForOutput(advanced, 1, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    callback->allocated_samples = false;

    run_async_reader(reader, advanced, callback);

    hr = IWMReaderAdvanced2_SetAllocateForOutput(advanced, 0, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetAllocateForOutput(advanced, 1, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IWMReaderAdvanced2_SetReceiveStreamSamples(advanced, 1, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetReceiveStreamSamples(advanced, 2, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    callback->read_compressed = false;
}

static void test_async_reader_settings(void)
{
    const WCHAR *filename = load_resource(L"test.wmv");
    IWMReaderAdvanced2 *reader_advanced;
    struct callback callback;
    WMT_ATTR_DATATYPE type;
    IWMReader *reader;
    DWORD value;
    HRESULT hr;
    WORD size;
    BOOL ret;

    callback_init(&callback);

    hr = WMCreateReader(NULL, 0, &reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReader_QueryInterface(reader, &IID_IWMReaderAdvanced2, (void **)&reader_advanced);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    type = WMT_TYPE_BOOL;
    size = sizeof(BOOL);
    value = 0;
    hr = IWMReaderAdvanced2_GetOutputSetting(reader_advanced, 0, L"AllowInterlacedOutput",
            &type, (BYTE *)&value, &size);
    todo_wine
    ok(hr == E_UNEXPECTED, "Got hr %#lx.\n", hr);

    hr = IWMReader_Open(reader, filename, &callback.IWMReaderCallback_iface, (void **)0xdeadbeef);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    wait_opened_callback(&callback);

    check_async_get_output_setting(reader_advanced, 0, L"AllowInterlacedOutput",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_get_output_setting(reader_advanced, 0, L"DedicatedDeliveryThread",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_get_output_setting(reader_advanced, 0, L"DeliverOnReceive",
            WMT_TYPE_BOOL, 0, S_OK);
    check_async_get_output_setting(reader_advanced, 0, L"EnableDiscreteOutput",
            WMT_TYPE_BOOL, 0, S_OK);
    check_async_get_output_setting(reader_advanced, 0, L"EnableFrameInterpolation",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_get_output_setting(reader_advanced, 0, L"JustInTimeDecode",
            WMT_TYPE_BOOL, 0, S_OK);
    check_async_get_output_setting(reader_advanced, 0, L"NeedsPreviousSample",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_get_output_setting(reader_advanced, 0, L"ScrambledAudio",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_get_output_setting(reader_advanced, 0, L"SingleOutputBuffer",
            WMT_TYPE_BOOL, 0, S_OK);
    check_async_get_output_setting(reader_advanced, 0, L"SoftwareScaling",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_get_output_setting(reader_advanced, 0, L"VideoSampleDurations",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_get_output_setting(reader_advanced, 0, L"EnableWMAProSPDIFOutput",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_get_output_setting(reader_advanced, 0, L"StreamLanguage",
            WMT_TYPE_WORD, 0, NS_E_INVALID_REQUEST);
    check_async_get_output_setting(reader_advanced, 0, L"DynamicRangeControl",
            WMT_TYPE_DWORD, -1, S_OK);
    check_async_get_output_setting(reader_advanced, 0, L"EarlyDataDelivery",
            WMT_TYPE_DWORD, 0, S_OK);
    check_async_get_output_setting(reader_advanced, 0, L"SpeakerConfig",
            WMT_TYPE_DWORD, -1, S_OK);

    check_async_get_output_setting(reader_advanced, 1, L"AllowInterlacedOutput",
            WMT_TYPE_BOOL, 0, S_OK);
    check_async_get_output_setting(reader_advanced, 1, L"DedicatedDeliveryThread",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_get_output_setting(reader_advanced, 1, L"DeliverOnReceive",
            WMT_TYPE_BOOL, 0, S_OK);
    check_async_get_output_setting(reader_advanced, 1, L"EnableDiscreteOutput",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_get_output_setting(reader_advanced, 1, L"EnableFrameInterpolation",
            WMT_TYPE_BOOL, 0, S_OK);
    check_async_get_output_setting(reader_advanced, 1, L"JustInTimeDecode",
            WMT_TYPE_BOOL, 0, S_OK);
    check_async_get_output_setting(reader_advanced, 1, L"NeedsPreviousSample",
            WMT_TYPE_BOOL, 0, NS_E_INVALID_REQUEST);
    check_async_get_output_setting(reader_advanced, 1, L"ScrambledAudio",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_get_output_setting(reader_advanced, 1, L"SingleOutputBuffer",
            WMT_TYPE_BOOL, 0, S_OK);
    check_async_get_output_setting(reader_advanced, 1, L"SoftwareScaling",
            WMT_TYPE_BOOL, 1, S_OK);
    check_async_get_output_setting(reader_advanced, 1, L"VideoSampleDurations",
            WMT_TYPE_BOOL, 0, S_OK);
    check_async_get_output_setting(reader_advanced, 1, L"EnableWMAProSPDIFOutput",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_get_output_setting(reader_advanced, 1, L"StreamLanguage",
            WMT_TYPE_WORD, 0, NS_E_INVALID_REQUEST);
    check_async_get_output_setting(reader_advanced, 1, L"DynamicRangeControl",
            WMT_TYPE_DWORD, 0, E_INVALIDARG);
    check_async_get_output_setting(reader_advanced, 1, L"EarlyDataDelivery",
            WMT_TYPE_DWORD, 0, S_OK);
    check_async_get_output_setting(reader_advanced, 1, L"SpeakerConfig",
            WMT_TYPE_DWORD, 0, E_INVALIDARG);

    check_async_set_output_setting(reader_advanced, 0, L"AllowInterlacedOutput",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_set_output_setting(reader_advanced, 0, L"DedicatedDeliveryThread",
            WMT_TYPE_BOOL, 1, S_OK);
    check_async_set_output_setting(reader_advanced, 0, L"DeliverOnReceive",
            WMT_TYPE_BOOL, 1, S_OK);
    check_async_set_output_setting(reader_advanced, 0, L"EnableDiscreteOutput",
            WMT_TYPE_BOOL, 1, S_OK);
    check_async_set_output_setting(reader_advanced, 0, L"EnableFrameInterpolation",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_set_output_setting(reader_advanced, 0, L"JustInTimeDecode",
            WMT_TYPE_BOOL, 1, S_OK);
    check_async_set_output_setting(reader_advanced, 0, L"NeedsPreviousSample",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_set_output_setting(reader_advanced, 0, L"ScrambledAudio",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_set_output_setting(reader_advanced, 0, L"SingleOutputBuffer",
            WMT_TYPE_BOOL, 1, S_OK);
    check_async_set_output_setting(reader_advanced, 0, L"SoftwareScaling",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_set_output_setting(reader_advanced, 0, L"VideoSampleDurations",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_set_output_setting(reader_advanced, 0, L"EnableWMAProSPDIFOutput",
            WMT_TYPE_BOOL, 1, S_OK);
    check_async_set_output_setting(reader_advanced, 0, L"StreamLanguage",
            WMT_TYPE_WORD, 0, S_OK);
    check_async_set_output_setting(reader_advanced, 0, L"DynamicRangeControl",
            WMT_TYPE_DWORD, 1, S_OK);
    check_async_set_output_setting(reader_advanced, 0, L"EarlyDataDelivery",
            WMT_TYPE_DWORD, 1000, S_OK);
    check_async_set_output_setting(reader_advanced, 0, L"SpeakerConfig",
            WMT_TYPE_DWORD, 1, S_OK);

    check_async_set_output_setting(reader_advanced, 1, L"AllowInterlacedOutput",
            WMT_TYPE_BOOL, 1, S_OK);
    check_async_set_output_setting(reader_advanced, 1, L"DedicatedDeliveryThread",
            WMT_TYPE_BOOL, 1, S_OK);
    check_async_set_output_setting(reader_advanced, 1, L"DeliverOnReceive",
            WMT_TYPE_BOOL, 1, S_OK);
    check_async_set_output_setting(reader_advanced, 1, L"EnableDiscreteOutput",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_set_output_setting(reader_advanced, 1, L"EnableFrameInterpolation",
            WMT_TYPE_BOOL, 1, S_OK);
    check_async_set_output_setting(reader_advanced, 1, L"JustInTimeDecode",
            WMT_TYPE_BOOL, 1, S_OK);
    check_async_set_output_setting(reader_advanced, 1, L"NeedsPreviousSample",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_set_output_setting(reader_advanced, 1, L"ScrambledAudio",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_set_output_setting(reader_advanced, 1, L"SingleOutputBuffer",
            WMT_TYPE_BOOL, 1, S_OK);
    check_async_set_output_setting(reader_advanced, 1, L"SoftwareScaling",
            WMT_TYPE_BOOL, 1, S_OK);
    check_async_set_output_setting(reader_advanced, 1, L"VideoSampleDurations",
            WMT_TYPE_BOOL, 1, S_OK);
    check_async_set_output_setting(reader_advanced, 1, L"EnableWMAProSPDIFOutput",
            WMT_TYPE_BOOL, 0, E_INVALIDARG);
    check_async_set_output_setting(reader_advanced, 1, L"StreamLanguage",
            WMT_TYPE_WORD, 0, S_OK);
    check_async_set_output_setting(reader_advanced, 1, L"DynamicRangeControl",
            WMT_TYPE_DWORD, 0, E_INVALIDARG);
    check_async_set_output_setting(reader_advanced, 1, L"EarlyDataDelivery",
            WMT_TYPE_DWORD, 2000, S_OK);
    check_async_set_output_setting(reader_advanced, 1, L"SpeakerConfig",
            WMT_TYPE_DWORD, 0, E_INVALIDARG);

    SetEvent(callback.expect_started);
    hr = IWMReader_Start(reader, 0, 0, 1, (void **)NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMReader_Close(reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IWMReaderAdvanced2_Release(reader_advanced);
    IWMReader_Release(reader);

    callback_cleanup(&callback);

    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %lu.\n", debugstr_w(filename), GetLastError());
}

static void test_async_reader_streaming(void)
{
    const WCHAR *filename = load_resource(L"test.wmv");
    IWMReaderAdvanced2 *advanced;
    struct teststream stream;
    struct callback callback;
    IWMStreamConfig *config;
    WORD stream_numbers[2];
    IWMProfile *profile;
    ULONG i, count, ref;
    IWMReader *reader;
    HANDLE file;
    HRESULT hr;
    BOOL ret;

    file = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to open %s, error %lu.\n", debugstr_w(file), GetLastError());

    teststream_init(&stream, file);
    callback_init(&callback);

    hr = WMCreateReader(NULL, 0, &reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IWMReader_QueryInterface(reader, &IID_IWMProfile, (void **)&profile);
    IWMReader_QueryInterface(reader, &IID_IWMReaderAdvanced2, (void **)&advanced);

    hr = IWMReader_Stop(reader);
    ok(hr == E_UNEXPECTED, "Got hr %#lx.\n", hr);
    hr = IWMReader_Start(reader, 0, 0, 1.0, NULL);
    ok(hr == NS_E_INVALID_REQUEST, "Got hr %#lx.\n", hr);

    hr = IWMReaderAdvanced2_OpenStream(advanced, &stream.IStream_iface, &callback.IWMReaderCallback_iface, (void **)0xdeadbeef);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(stream.refcount > 1, "Got refcount %ld.\n", stream.refcount);
    ok(callback.refcount > 1, "Got refcount %ld.\n", callback.refcount);
    wait_opened_callback(&callback);

    hr = IWMReaderAdvanced2_OpenStream(advanced, &stream.IStream_iface, &callback.IWMReaderCallback_iface, (void **)0xdeadbee0);
    ok(hr == E_UNEXPECTED, "Got hr %#lx.\n", hr);

    count = 0xdeadbeef;
    hr = IWMReader_GetOutputCount(reader, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);

    for (i = 0; i < 2; ++i)
    {
        hr = IWMProfile_GetStream(profile, i, &config);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        stream_numbers[i] = 0xdead;
        hr = IWMStreamConfig_GetStreamNumber(config, &stream_numbers[i]);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(stream_numbers[i] == i + 1, "Got stream number %u.\n", stream_numbers[i]);

        ref = IWMStreamConfig_Release(config);
        ok(!ref, "Got outstanding refcount %ld.\n", ref);
    }

    hr = IWMReader_Start(reader, 0, 0, 1.0f, (void *)NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    wait_started_callback(&callback);

    /* By default the reader will time itself, and attempt to deliver samples
     * according to their presentation time. Call DeliverTime with the file
     * duration in order to request all samples as fast as possible. */
    hr = IWMReaderAdvanced2_DeliverTime(advanced, test_wmv_duration * 2);
    ok(hr == E_UNEXPECTED, "Got hr %#lx.\n", hr);
    hr = IWMReaderAdvanced2_SetUserProvidedClock(advanced, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    callback.expect_ontime = 0;
    hr = IWMReaderAdvanced2_DeliverTime(advanced, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = WaitForSingleObject(callback.ontime_event, 1000);
    ok(!ret, "Wait timed out.\n");
    callback.expect_ontime = test_wmv_duration / 2;
    hr = IWMReaderAdvanced2_DeliverTime(advanced, test_wmv_duration / 2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = WaitForSingleObject(callback.ontime_event, 1000);
    ok(!ret, "Wait timed out.\n");
    callback.expect_ontime = test_wmv_duration * 2;
    hr = IWMReaderAdvanced2_DeliverTime(advanced, test_wmv_duration * 2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    wait_eof_callback(&callback);

    ret = WaitForSingleObject(callback.ontime_event, 1000);
    ok(!ret, "Wait timed out.\n");

    hr = IWMReader_Start(reader, 0, 0, 1.0f, (void *)NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    wait_started_callback(&callback);

    hr = IWMReaderAdvanced2_DeliverTime(advanced, test_wmv_duration * 2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    wait_eof_callback(&callback);

    hr = IWMReader_Stop(reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    wait_stopped_callback(&callback);

    hr = IWMReader_Stop(reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    wait_stopped_callback(&callback);

    test_reader_attributes(profile);

    trace("Checking default settings.\n");
    trace("  with stream selection\n");
    test_async_reader_selection(reader, advanced, &callback);
    trace("  with sample allocation\n");
    test_async_reader_allocate(reader, advanced, &callback);
    trace("  with compressed sample\n");
    test_async_reader_compressed(reader, advanced, &callback);
    trace("  with compressed sample allocation\n");
    test_async_reader_allocate_compressed(reader, advanced, &callback);

    callback.dedicated_threads = TRUE;
    trace("Checking DedicatedDeliveryThread.\n");
    trace("  with stream selection\n");
    test_async_reader_selection(reader, advanced, &callback);
    trace("  with sample allocation\n");
    test_async_reader_allocate(reader, advanced, &callback);
    trace("  with compressed sample\n");
    test_async_reader_compressed(reader, advanced, &callback);
    trace("  with compressed sample allocation\n");
    test_async_reader_allocate_compressed(reader, advanced, &callback);
    callback.dedicated_threads = FALSE;

    hr = IWMReader_Close(reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(callback.closed_count == 1, "Got %u WMT_CLOSED callbacks.\n", callback.closed_count);
    ok(callback.refcount == 1, "Got outstanding refcount %ld.\n", callback.refcount);
    ret = WaitForSingleObject(callback.got_stopped, 0);
    ok(ret == WAIT_TIMEOUT, "Got unexpected WMT_STOPPED.\n");
    callback_cleanup(&callback);

    hr = IWMReader_Stop(reader);
    ok(hr == E_UNEXPECTED, "Got hr %#lx.\n", hr);

    ok(stream.refcount == 1, "Got outstanding refcount %ld.\n", stream.refcount);
    CloseHandle(stream.file);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %lu.\n", debugstr_w(filename), GetLastError());

    hr = IWMReader_Close(reader);
    ok(hr == NS_E_INVALID_REQUEST, "Got hr %#lx.\n", hr);

    IWMReaderAdvanced2_Release(advanced);
    IWMProfile_Release(profile);
    ref = IWMReader_Release(reader);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_async_reader_types(void)
{
    char mt_buffer[2000], mt2_buffer[2000];
    const WCHAR *filename = load_resource(L"test.wmv");
    IWMOutputMediaProps *output_props, *output_props2;
    WM_MEDIA_TYPE *mt2 = (WM_MEDIA_TYPE *)mt2_buffer;
    WM_MEDIA_TYPE *mt = (WM_MEDIA_TYPE *)mt_buffer;
    bool got_video = false, got_audio = false;
    DWORD size, ret_size, output_number;
    IWMReaderAdvanced2 *advanced;
    GUID majortype, majortype2;
    struct teststream stream;
    struct callback callback;
    IWMStreamConfig *config;
    ULONG count, ref, i, j;
    IWMProfile *profile;
    IWMReader *reader;
    HANDLE file;
    HRESULT hr;
    BOOL ret;

    file = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to open %s, error %lu.\n", debugstr_w(file), GetLastError());

    teststream_init(&stream, file);
    callback_init(&callback);

    hr = WMCreateReader(NULL, 0, &reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IWMReader_QueryInterface(reader, &IID_IWMProfile, (void **)&profile);
    IWMReader_QueryInterface(reader, &IID_IWMReaderAdvanced2, (void **)&advanced);

    hr = IWMReaderAdvanced2_OpenStream(advanced, &stream.IStream_iface, &callback.IWMReaderCallback_iface, (void **)0xdeadbeef);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(stream.refcount > 1, "Got refcount %ld.\n", stream.refcount);
    ok(callback.refcount > 1, "Got refcount %ld.\n", callback.refcount);
    wait_opened_callback(&callback);

    for (i = 0; i < 2; ++i)
    {
        winetest_push_context("Stream %lu", i);

        hr = IWMProfile_GetStream(profile, i, &config);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IWMStreamConfig_GetStreamType(config, &majortype);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        if (!i)
            ok(IsEqualGUID(&majortype, &MEDIATYPE_Video), "Got major type %s.\n", debugstr_guid(&majortype));
        else
            ok(IsEqualGUID(&majortype, &MEDIATYPE_Audio), "Got major type %s.\n", debugstr_guid(&majortype));

        test_stream_media_props(config, &majortype);

        ref = IWMStreamConfig_Release(config);
        ok(!ref, "Got outstanding refcount %ld.\n", ref);

        winetest_pop_context();
    }

    for (i = 0; i < 2; ++i)
    {
        winetest_push_context("Output %lu", i);
        output_number = i;

        hr = IWMReader_GetOutputProps(reader, output_number, &output_props);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        ret_size = sizeof(mt_buffer);
        hr = IWMOutputMediaProps_GetMediaType(output_props, mt, &ret_size);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        majortype = mt->majortype;

        memset(&majortype2, 0xcc, sizeof(majortype2));
        hr = IWMOutputMediaProps_GetType(output_props, &majortype2);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(IsEqualGUID(&majortype2, &majortype), "Expected major type %s, got %s.\n",
                debugstr_guid(&majortype), debugstr_guid(&majortype2));

        hr = IWMOutputMediaProps_SetMediaType(output_props, NULL);
        ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

        memset(mt2_buffer, 0, sizeof(mt2_buffer));
        hr = IWMOutputMediaProps_SetMediaType(output_props, mt2);
        ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

        if (IsEqualGUID(&majortype, &MEDIATYPE_Audio))
        {
            WAVEFORMATEX *format = (WAVEFORMATEX *)mt->pbFormat;

            init_audio_type(mt2, &MEDIASUBTYPE_IEEE_FLOAT, 32, format->nChannels * 2, format->nSamplesPerSec);
            hr = IWMOutputMediaProps_SetMediaType(output_props, mt2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            hr = IWMReader_SetOutputProps(reader, output_number, output_props);
            ok(hr == NS_E_AUDIO_CODEC_NOT_INSTALLED, "Got hr %#lx.\n", hr);

            init_audio_type(mt2, &MEDIASUBTYPE_PCM, 8, 1, 11025);
            hr = IWMOutputMediaProps_SetMediaType(output_props, mt2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            hr = IWMReader_SetOutputProps(reader, output_number, output_props);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);

            init_audio_type(mt2, &MEDIASUBTYPE_IEEE_FLOAT, 32, format->nChannels, format->nSamplesPerSec / 4);
            hr = IWMOutputMediaProps_SetMediaType(output_props, mt2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            hr = IWMReader_SetOutputProps(reader, output_number, output_props);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
        }
        else
        {
            VIDEOINFO *info = (VIDEOINFO *)mt->pbFormat;
            RECT rect = info->rcTarget;

            init_video_type(mt2, &MEDIASUBTYPE_RGB32, 32, BI_RGB, &rect);
            hr = IWMOutputMediaProps_SetMediaType(output_props, mt2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            hr = IWMReader_SetOutputProps(reader, output_number, output_props);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);

            init_video_type(mt2, &MEDIASUBTYPE_NV12, 12, MAKEFOURCC('N','V','1','2'), &rect);
            hr = IWMOutputMediaProps_SetMediaType(output_props, mt2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            hr = IWMReader_SetOutputProps(reader, output_number, output_props);
            todo_wine
            ok(hr == NS_E_INVALID_OUTPUT_FORMAT, "Got hr %#lx.\n", hr);

            InflateRect(&rect, 10, 10);

            init_video_type(mt2, &MEDIASUBTYPE_RGB32, 32, BI_RGB, &rect);
            hr = IWMOutputMediaProps_SetMediaType(output_props, mt2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            hr = IWMReader_SetOutputProps(reader, output_number, output_props);
            ok(hr == NS_E_INVALID_OUTPUT_FORMAT, "Got hr %#lx.\n", hr);
        }

        hr = IWMOutputMediaProps_SetMediaType(output_props, mt);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        hr = IWMReader_SetOutputProps(reader, output_number, output_props);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        ref = IWMOutputMediaProps_Release(output_props);
        ok(!ref, "Got outstanding refcount %ld.\n", ref);

        if (IsEqualGUID(&majortype, &MEDIATYPE_Audio))
        {
            got_audio = true;
            check_audio_type(mt);

            /* R.U.S.E. enumerates all audio formats, picks the first one it
             * likes, and then sets the wrong stream to that format.
             * Accordingly we need the first audio format to be the default
             * format, and we need it to be a format that the game is happy
             * with. In particular it has to be PCM. */

            hr = IWMReader_GetOutputFormat(reader, output_number, 0, &output_props);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);

            ret_size = sizeof(mt2_buffer);
            hr = IWMOutputMediaProps_GetMediaType(output_props, mt2, &ret_size);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);

            ref = IWMOutputMediaProps_Release(output_props);
            ok(!ref, "Got outstanding refcount %ld.\n", ref);

            /* The sample size might differ. */
            mt2->lSampleSize = mt->lSampleSize;
            ok(compare_media_types(mt, mt2), "Media types didn't match.\n");
        }
        else
        {
            ok(IsEqualGUID(&majortype, &MEDIATYPE_Video), "Got major type %s.\n", debugstr_guid(&majortype));
            /* Shadowgrounds assumes that the initial video type will be RGB24. */
            ok(IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_RGB24), "Got subtype %s.\n", debugstr_guid(&mt->subtype));
            got_video = true;
            check_video_type(mt);
        }

        count = 0;
        hr = IWMReader_GetOutputFormatCount(reader, output_number, &count);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(count > 0, "Got count %lu.\n", count);

        for (j = 0; j < count; ++j)
        {
            winetest_push_context("Format %lu", j);

            hr = IWMReader_GetOutputFormat(reader, output_number, j, &output_props);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);

            hr = IWMReader_GetOutputFormat(reader, output_number, j, &output_props2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(output_props2 != output_props, "Expected different objects.\n");
            ref = IWMOutputMediaProps_Release(output_props2);
            ok(!ref, "Got outstanding refcount %ld.\n", ref);

            size = 0xdeadbeef;
            hr = IWMOutputMediaProps_GetMediaType(output_props, NULL, &size);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(size != 0xdeadbeef && size >= sizeof(WM_MEDIA_TYPE), "Got size %lu.\n", size);

            ret_size = size - 1;
            hr = IWMOutputMediaProps_GetMediaType(output_props, mt, &ret_size);
            ok(hr == ASF_E_BUFFERTOOSMALL, "Got hr %#lx.\n", hr);
            ok(ret_size == size, "Expected size %lu, got %lu.\n", size, ret_size);

            ret_size = sizeof(mt_buffer);
            memset(mt_buffer, 0xcc, sizeof(mt_buffer));
            hr = IWMOutputMediaProps_GetMediaType(output_props, mt, &ret_size);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(ret_size == size, "Expected size %lu, got %lu.\n", size, ret_size);
            ok(size == sizeof(WM_MEDIA_TYPE) + mt->cbFormat, "Expected size %Iu, got %lu.\n",
                    sizeof(WM_MEDIA_TYPE) + mt->cbFormat, size);

            ok(IsEqualGUID(&mt->majortype, &majortype), "Got major type %s.\n", debugstr_guid(&mt->majortype));

            if (IsEqualGUID(&mt->majortype, &MEDIATYPE_Audio))
            {
                ok(IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_PCM), "Got subtype %s.\n", debugstr_guid(&mt->subtype));
                check_audio_type(mt);
            }
            else
            {
                ok(!IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_AYUV), "Got subtype %s.\n", debugstr_guid(&mt->subtype));
                check_video_type(mt);
            }

            memset(&majortype2, 0xcc, sizeof(majortype2));
            hr = IWMOutputMediaProps_GetType(output_props, &majortype2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(IsEqualGUID(&majortype2, &majortype), "Expected major type %s, got %s.\n",
                    debugstr_guid(&majortype), debugstr_guid(&majortype2));

            hr = IWMReader_SetOutputProps(reader, output_number, output_props);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            hr = IWMReader_SetOutputProps(reader, 1 - output_number, output_props);
            if (!i)
                ok(hr == NS_E_INCOMPATIBLE_FORMAT /* win < 8, win10 1507-1809 */
                        || hr == ASF_E_BADMEDIATYPE /* win8, win10 1909+ */, "Got hr %#lx.\n", hr);
            else
                todo_wine ok(hr == NS_E_INVALID_REQUEST, "Got hr %#lx.\n", hr);
            hr = IWMReader_SetOutputProps(reader, 2, output_props);
            ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

            hr = IWMReader_GetOutputProps(reader, output_number, &output_props2);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(output_props2 != output_props, "Expected different objects.\n");

            ret_size = sizeof(mt2_buffer);
            hr = IWMOutputMediaProps_GetMediaType(output_props2, mt2, &ret_size);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(compare_media_types(mt, mt2), "Media types didn't match.\n");

            ref = IWMOutputMediaProps_Release(output_props2);
            ok(!ref, "Got outstanding refcount %ld.\n", ref);
            ref = IWMOutputMediaProps_Release(output_props);
            ok(!ref, "Got outstanding refcount %ld.\n", ref);

            winetest_pop_context();
        }

        hr = IWMReader_GetOutputFormat(reader, output_number, count, &output_props);
        ok(hr == NS_E_INVALID_OUTPUT_FORMAT, "Got hr %#lx.\n", hr);

        hr = IWMReader_GetOutputProps(reader, output_number, &output_props);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IWMReader_GetOutputProps(reader, output_number, &output_props2);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(output_props2 != output_props, "Expected different objects.\n");

        ref = IWMOutputMediaProps_Release(output_props2);
        ok(!ref, "Got outstanding refcount %ld.\n", ref);
        ref = IWMOutputMediaProps_Release(output_props);
        ok(!ref, "Got outstanding refcount %ld.\n", ref);

        winetest_pop_context();
    }

    ok(got_audio, "No audio stream was enumerated.\n");
    ok(got_video, "No video stream was enumerated.\n");

    count = 0xdeadbeef;
    hr = IWMReader_GetOutputFormatCount(reader, 2, &count);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(count == 0xdeadbeef, "Got count %#lx.\n", count);

    output_props = (void *)0xdeadbeef;
    hr = IWMReader_GetOutputProps(reader, 2, &output_props);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(output_props == (void *)0xdeadbeef, "Got output props %p.\n", output_props);

    output_props = (void *)0xdeadbeef;
    hr = IWMReader_GetOutputFormat(reader, 2, 0, &output_props);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(output_props == (void *)0xdeadbeef, "Got output props %p.\n", output_props);

    IWMReaderAdvanced2_Release(advanced);
    IWMProfile_Release(profile);
    ref = IWMReader_Release(reader);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    ok(stream.refcount == 1, "Got outstanding refcount %ld.\n", stream.refcount);
    CloseHandle(stream.file);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %lu.\n", debugstr_w(filename), GetLastError());
}

static void test_async_reader_file(void)
{
    const WCHAR *filename = load_resource(L"test.wmv");
    struct callback callback;
    IWMReader *reader;
    DWORD count;
    HRESULT hr;
    ULONG ref;
    BOOL ret;

    callback_init(&callback);

    hr = WMCreateReader(NULL, 0, &reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IWMReader_Open(reader, filename, &callback.IWMReaderCallback_iface, (void **)0xdeadbeef);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(callback.refcount > 1, "Got refcount %ld.\n", callback.refcount);
    wait_opened_callback(&callback);

    hr = IWMReader_Open(reader, filename, &callback.IWMReaderCallback_iface, (void **)0xdeadbee0);
    ok(hr == E_UNEXPECTED, "Got hr %#lx.\n", hr);

    count = 0xdeadbeef;
    hr = IWMReader_GetOutputCount(reader, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);

    callback.expect_context = (void *)0xfacade;
    hr = IWMReader_Start(reader, 0, 0, 1.0f, (void *)0xfacade);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    wait_started_callback(&callback);

    hr = IWMReader_Close(reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(callback.closed_count == 1, "Got %u WMT_CLOSED callbacks.\n", callback.closed_count);
    ok(callback.refcount == 1, "Got outstanding refcount %ld.\n", callback.refcount);
    ret = WaitForSingleObject(callback.got_stopped, 0);
    ok(ret == WAIT_TIMEOUT, "Got unexpected WMT_STOPPED.\n");
    callback_cleanup(&callback);

    hr = IWMReader_Close(reader);
    ok(hr == NS_E_INVALID_REQUEST, "Got hr %#lx.\n", hr);

    ref = IWMReader_Release(reader);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ok(callback.closed_count == 1, "Got %u WMT_CLOSED callbacks.\n", callback.closed_count);
    ok(callback.refcount == 1, "Got outstanding refcount %ld.\n", callback.refcount);
    callback_cleanup(&callback);

    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %lu.\n", debugstr_w(filename), GetLastError());
}

static void test_sync_reader_allocator(void)
{
    const WCHAR *filename = load_resource(L"test.wmv");
    IWMReaderAllocatorEx *allocator;
    struct teststream stream;
    struct callback callback;
    DWORD output_num, flags;
    IWMSyncReader2 *reader;
    QWORD pts, duration;
    INSSBuffer *sample;
    WORD stream_num;
    HANDLE file;
    HRESULT hr;
    BOOL ret;

    callback_init(&callback);

    hr = WMCreateSyncReader(NULL, 0, (IWMSyncReader **)&reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    file = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to open %s, error %lu.\n", debugstr_w(file), GetLastError());

    teststream_init(&stream, file);

    hr = IWMSyncReader2_OpenStream(reader, &stream.IStream_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(stream.refcount > 1, "Got refcount %ld.\n", stream.refcount);


    hr = IWMSyncReader2_GetAllocateForOutput(reader, -1, &allocator);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_GetAllocateForStream(reader, 0, &allocator);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_GetAllocateForOutput(reader, 0, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_GetAllocateForStream(reader, 1, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_SetAllocateForOutput(reader, -1, &callback.IWMReaderAllocatorEx_iface);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_SetAllocateForStream(reader, 0, &callback.IWMReaderAllocatorEx_iface);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);


    allocator = (void *)0xdeadbeef;
    hr = IWMSyncReader2_GetAllocateForOutput(reader, 0, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!allocator, "Got allocator %p.\n", allocator);
    allocator = (void *)0xdeadbeef;
    hr = IWMSyncReader2_GetAllocateForStream(reader, 1, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!allocator, "Got allocator %p.\n", allocator);


    hr = IWMSyncReader2_SetAllocateForStream(reader, 1, &callback.IWMReaderAllocatorEx_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    allocator = (void *)0xdeadbeef;
    hr = IWMSyncReader2_GetAllocateForOutput(reader, 0, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!allocator, "Got allocator %p.\n", allocator);
    allocator = (void *)0xdeadbeef;
    hr = IWMSyncReader2_GetAllocateForStream(reader, 2, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!allocator, "Got allocator %p.\n", allocator);
    hr = IWMSyncReader2_GetAllocateForStream(reader, 1, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(allocator == &callback.IWMReaderAllocatorEx_iface, "Got allocator %p.\n", allocator);

    hr = IWMSyncReader2_SetAllocateForStream(reader, 1, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    allocator = (void *)0xdeadbeef;
    hr = IWMSyncReader2_GetAllocateForOutput(reader, 0, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!allocator, "Got allocator %p.\n", allocator);
    allocator = (void *)0xdeadbeef;
    hr = IWMSyncReader2_GetAllocateForStream(reader, 1, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!allocator, "Got allocator %p.\n", allocator);


    hr = IWMSyncReader2_SetAllocateForOutput(reader, 0, &callback.IWMReaderAllocatorEx_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    allocator = (void *)0xdeadbeef;
    hr = IWMSyncReader2_GetAllocateForOutput(reader, 1, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!allocator, "Got allocator %p.\n", allocator);
    allocator = (void *)0xdeadbeef;
    hr = IWMSyncReader2_GetAllocateForStream(reader, 1, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!allocator, "Got allocator %p.\n", allocator);
    hr = IWMSyncReader2_GetAllocateForOutput(reader, 0, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(allocator == &callback.IWMReaderAllocatorEx_iface, "Got allocator %p.\n", allocator);

    hr = IWMSyncReader2_SetAllocateForOutput(reader, 0, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    allocator = (void *)0xdeadbeef;
    hr = IWMSyncReader2_GetAllocateForOutput(reader, 0, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!allocator, "Got allocator %p.\n", allocator);
    allocator = (void *)0xdeadbeef;
    hr = IWMSyncReader2_GetAllocateForStream(reader, 1, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!allocator, "Got allocator %p.\n", allocator);


    hr = IWMSyncReader2_GetStreamNumberForOutput(reader, 0, &stream_num);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_SetAllocateForStream(reader, stream_num, &callback.IWMReaderAllocatorEx_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_SetReadStreamSamples(reader, stream_num, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_SetAllocateForOutput(reader, 1, &callback.IWMReaderAllocatorEx_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    callback.allocated_samples = true;

    hr = IWMSyncReader2_GetStreamNumberForOutput(reader, 0, &stream_num);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_GetNextSample(reader, stream_num, &sample, &pts, &duration, &flags,
            &output_num, &stream_num);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(sample->lpVtbl == &buffer_vtbl, "Buffer vtbl didn't match.\n");
    INSSBuffer_Release(sample);

    hr = IWMSyncReader2_GetStreamNumberForOutput(reader, 1, &stream_num);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_GetNextSample(reader, stream_num, &sample, &pts, &duration, &flags,
            &output_num, &stream_num);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(sample->lpVtbl == &buffer_vtbl, "Buffer vtbl didn't match.\n");
    INSSBuffer_Release(sample);

    callback.allocated_samples = false;


    /* without compressed sample read, allocator isn't used */
    hr = IWMSyncReader2_GetStreamNumberForOutput(reader, 0, &stream_num);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_SetReadStreamSamples(reader, stream_num, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_GetNextSample(reader, stream_num, &sample, &pts, &duration, &flags,
            &output_num, &stream_num);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(sample->lpVtbl != &buffer_vtbl, "Unexpected buffer vtbl.\n");
    INSSBuffer_Release(sample);


    /* cannot change or remove allocators after they've been used */
    hr = IWMSyncReader2_GetStreamNumberForOutput(reader, 0, &stream_num);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_SetAllocateForStream(reader, stream_num, NULL);
    todo_wine
    ok(hr == NS_E_INVALID_REQUEST, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_SetAllocateForOutput(reader, 0, &callback.IWMReaderAllocatorEx_iface);
    todo_wine
    ok(hr == NS_E_INVALID_REQUEST, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_SetAllocateForOutput(reader, 1, NULL);
    todo_wine
    ok(hr == NS_E_INVALID_REQUEST, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_GetStreamNumberForOutput(reader, 0, &stream_num);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_SetReadStreamSamples(reader, stream_num, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    callback.allocated_samples = true;

    hr = IWMSyncReader2_GetStreamNumberForOutput(reader, 1, &stream_num);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_GetNextSample(reader, stream_num, &sample, &pts, &duration, &flags,
            &output_num, &stream_num);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine
    ok(sample->lpVtbl == &buffer_vtbl, "Buffer vtbl didn't match.\n");
    INSSBuffer_Release(sample);

    hr = IWMSyncReader2_GetStreamNumberForOutput(reader, 0, &stream_num);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IWMSyncReader2_GetNextSample(reader, stream_num, &sample, &pts, &duration, &flags,
            &output_num, &stream_num);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine
    ok(sample->lpVtbl == &buffer_vtbl, "Unexpected buffer vtbl.\n");
    INSSBuffer_Release(sample);

    callback.allocated_samples = false;


    IWMSyncReader2_Release(reader);

    ok(stream.refcount == 1, "Got outstanding refcount %ld.\n", stream.refcount);
    CloseHandle(stream.file);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %lu.\n", debugstr_w(filename), GetLastError());

    callback_cleanup(&callback);
}

START_TEST(wmvcore)
{
    HRESULT hr;

    hr = CoInitialize(0);
    ok(hr == S_OK, "failed to init com\n");
    if(hr != S_OK)
        return;

    winetest_mute_threshold = 3;  /* FIXME: thread tests print too many "got wrong thread" todos */

    test_wmreader_interfaces();
    test_wmsyncreader_interfaces();
    test_wmwriter_interfaces();
    test_profile_manager_interfaces();
    test_WMCreateWriterPriv();
    test_urlextension();
    test_iscontentprotected();
    test_sync_reader_allocator();
    test_sync_reader_settings();
    test_sync_reader_streaming();
    test_sync_reader_types();
    test_sync_reader_file();
    test_async_reader_settings();
    test_async_reader_streaming();
    test_async_reader_types();
    test_async_reader_file();

    CoUninitialize();
}
