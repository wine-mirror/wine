/*
 * Copyright 2018 Alistair Leslie-Hughes
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
#include "winuser.h"
#include "winreg.h"

#include "initguid.h"
#include "ole2.h"

DEFINE_GUID(GUID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

#undef INITGUID
#include "guiddef.h"
#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"
#include "mfreadwrite.h"

#include "wine/heap.h"
#include "wine/test.h"

static HRESULT (WINAPI *pMFCreateMFByteStreamOnStream)(IStream *stream, IMFByteStream **bytestream);

static void init_functions(void)
{
    HMODULE mod = GetModuleHandleA("mfplat.dll");

#define X(f) if (!(p##f = (void*)GetProcAddress(mod, #f))) return;
    X(MFCreateMFByteStreamOnStream);
#undef X
}

static IMFByteStream *get_resource_stream(const char *name)
{
    IMFByteStream *bytestream;
    IStream *stream;
    DWORD written;
    HRESULT hr;
    HRSRC res;
    void *ptr;

    res = FindResourceA(NULL, name, (const char *)RT_RCDATA);
    ok(res != 0, "Resource %s wasn't found.\n", name);

    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Failed to create memory stream, hr %#x.\n", hr);

    hr = IStream_Write(stream, ptr, SizeofResource(GetModuleHandleA(NULL), res), &written);
    ok(hr == S_OK, "Failed to write stream content, hr %#x.\n", hr);

    hr = pMFCreateMFByteStreamOnStream(stream, &bytestream);
    ok(hr == S_OK, "Failed to create bytestream, hr %#x.\n", hr);
    IStream_Release(stream);

    return bytestream;
}

static void test_factory(void)
{
    IMFReadWriteClassFactory *factory, *factory2;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_MFReadWriteClassFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IMFReadWriteClassFactory,
            (void **)&factory);
    ok(hr == S_OK, "Failed to create class factory, hr %#x.\n", hr);

    hr = CoCreateInstance(&CLSID_MFReadWriteClassFactory, (IUnknown *)factory, CLSCTX_INPROC_SERVER, &IID_IMFReadWriteClassFactory,
            (void **)&factory2);
    ok(hr == CLASS_E_NOAGGREGATION, "Unexpected hr %#x.\n", hr);

    IMFReadWriteClassFactory_Release(factory);

    CoUninitialize();
}

struct async_callback
{
    IMFSourceReaderCallback IMFSourceReaderCallback_iface;
    LONG refcount;
    HANDLE event;
};

static struct async_callback *impl_from_IMFSourceReaderCallback(IMFSourceReaderCallback *iface)
{
    return CONTAINING_RECORD(iface, struct async_callback, IMFSourceReaderCallback_iface);
}

static HRESULT WINAPI async_callback_QueryInterface(IMFSourceReaderCallback *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IMFSourceReaderCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFSourceReaderCallback_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI async_callback_AddRef(IMFSourceReaderCallback *iface)
{
    struct async_callback *callback = impl_from_IMFSourceReaderCallback(iface);
    return InterlockedIncrement(&callback->refcount);
}

static ULONG WINAPI async_callback_Release(IMFSourceReaderCallback *iface)
{
    struct async_callback *callback = impl_from_IMFSourceReaderCallback(iface);
    ULONG refcount = InterlockedDecrement(&callback->refcount);

    if (!refcount)
    {
        heap_free(callback);
    }

    return refcount;
}

static HRESULT WINAPI async_callback_OnReadSample(IMFSourceReaderCallback *iface, HRESULT hr, DWORD stream_index,
        DWORD stream_flags, LONGLONG timestamp, IMFSample *sample)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI async_callback_OnFlush(IMFSourceReaderCallback *iface, DWORD stream_index)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI async_callback_OnEvent(IMFSourceReaderCallback *iface, DWORD stream_index, IMFMediaEvent *event)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IMFSourceReaderCallbackVtbl async_callback_vtbl =
{
    async_callback_QueryInterface,
    async_callback_AddRef,
    async_callback_Release,
    async_callback_OnReadSample,
    async_callback_OnFlush,
    async_callback_OnEvent,
};

static struct async_callback *create_async_callback(void)
{
    struct async_callback *callback;

    callback = heap_alloc(sizeof(*callback));
    callback->IMFSourceReaderCallback_iface.lpVtbl = &async_callback_vtbl;
    callback->refcount = 1;

    return callback;
}

static void test_source_reader(void)
{
    IMFMediaType *mediatype, *mediatype2;
    DWORD stream_flags, actual_index;
    struct async_callback *callback;
    IMFAttributes *attributes;
    IMFSourceReader *reader;
    IMFMediaSource *source;
    IMFByteStream *stream;
    LONGLONG timestamp;
    IMFSample *sample;
    BOOL selected;
    HRESULT hr;

    if (!pMFCreateMFByteStreamOnStream)
    {
        win_skip("MFCreateMFByteStreamOnStream() not found\n");
        return;
    }

    stream = get_resource_stream("test.wav");

    hr = MFCreateSourceReaderFromByteStream(stream, NULL, &reader);
todo_wine
    ok(hr == S_OK, "Failed to create source reader, hr %#x.\n", hr);

    if (FAILED(hr))
    {
        IMFByteStream_Release(stream);
        return;
    }

    /* Access underlying media source object. */
    hr = IMFSourceReader_GetServiceForStream(reader, MF_SOURCE_READER_MEDIASOURCE, &GUID_NULL, &IID_IMFMediaSource,
            (void **)&source);
    ok(hr == S_OK, "Failed to get media source interface, hr %#x.\n", hr);
    IMFMediaSource_Release(source);

    /* Stream selection. */
    hr = IMFSourceReader_GetStreamSelection(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, &selected);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_GetStreamSelection(reader, 100, &selected);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    selected = FALSE;
    hr = IMFSourceReader_GetStreamSelection(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, &selected);
    ok(hr == S_OK, "Failed to get stream selection, hr %#x.\n", hr);
    ok(selected, "Unexpected selection.\n");

    selected = FALSE;
    hr = IMFSourceReader_GetStreamSelection(reader, 0, &selected);
    ok(hr == S_OK, "Failed to get stream selection, hr %#x.\n", hr);
    ok(selected, "Unexpected selection.\n");

    hr = IMFSourceReader_SetStreamSelection(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 100, TRUE);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, FALSE);
    ok(hr == S_OK, "Failed to deselect a stream, hr %#x.\n", hr);

    selected = TRUE;
    hr = IMFSourceReader_GetStreamSelection(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, &selected);
    ok(hr == S_OK, "Failed to get stream selection, hr %#x.\n", hr);
    ok(!selected, "Unexpected selection.\n");

    hr = IMFSourceReader_SetStreamSelection(reader, MF_SOURCE_READER_ALL_STREAMS, TRUE);
    ok(hr == S_OK, "Failed to deselect a stream, hr %#x.\n", hr);

    selected = FALSE;
    hr = IMFSourceReader_GetStreamSelection(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, &selected);
    ok(hr == S_OK, "Failed to get stream selection, hr %#x.\n", hr);
    ok(selected, "Unexpected selection.\n");

    /* Native media type. */
    hr = IMFSourceReader_GetNativeMediaType(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_GetNativeMediaType(reader, 100, 0, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_GetNativeMediaType(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &mediatype);
    ok(hr == S_OK, "Failed to get native mediatype, hr %#x.\n", hr);
    hr = IMFSourceReader_GetNativeMediaType(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get native mediatype, hr %#x.\n", hr);
    ok(mediatype != mediatype2, "Unexpected media type instance.\n");
    IMFMediaType_Release(mediatype2);
    IMFMediaType_Release(mediatype);

    /* MF_SOURCE_READER_CURRENT_TYPE_INDEX is Win8+ */
    hr = IMFSourceReader_GetNativeMediaType(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM,
            MF_SOURCE_READER_CURRENT_TYPE_INDEX, &mediatype);
    ok(hr == S_OK || broken(hr == MF_E_NO_MORE_TYPES), "Failed to get native mediatype, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
        IMFMediaType_Release(mediatype);

    hr = IMFSourceReader_GetNativeMediaType(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 1, &mediatype);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#x.\n", hr);

    /* Current media type. */
    hr = IMFSourceReader_GetCurrentMediaType(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 100, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_GetCurrentMediaType(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, &mediatype);
    ok(hr == S_OK, "Failed to get current media type, hr %#x.\n", hr);
    IMFMediaType_Release(mediatype);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &mediatype);
    ok(hr == S_OK, "Failed to get current media type, hr %#x.\n", hr);
    IMFMediaType_Release(mediatype);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected stream index %u\n", actual_index);
    ok(!stream_flags, "Unexpected stream flags %#x.\n", stream_flags);
    IMFSample_Release(sample);

    /* There is no video stream. */
    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);
    ok(actual_index == MF_SOURCE_READER_FIRST_VIDEO_STREAM, "Unexpected stream index %u\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ERROR, "Unexpected stream flags %#x.\n", stream_flags);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, &stream_flags, &timestamp,
            &sample);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, &timestamp, &sample);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected stream index %u\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ENDOFSTREAM, "Unexpected stream flags %#x.\n", stream_flags);
    ok(!sample, "Unexpected sample object.\n");

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            &actual_index, &stream_flags, &timestamp, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected stream index %u\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ENDOFSTREAM, "Unexpected stream flags %#x.\n", stream_flags);
    ok(!sample, "Unexpected sample object.\n");

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            &actual_index, &stream_flags, &timestamp, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            &actual_index, NULL, &timestamp, &sample);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            NULL, &stream_flags, &timestamp, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(stream_flags == MF_SOURCE_READERF_ENDOFSTREAM, "Unexpected stream flags %#x.\n", stream_flags);
    ok(!sample, "Unexpected sample object.\n");

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            &actual_index, &stream_flags, NULL, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected stream index %u\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ENDOFSTREAM, "Unexpected stream flags %#x.\n", stream_flags);
    ok(!sample, "Unexpected sample object.\n");

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_ANY_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            &actual_index, &stream_flags, NULL, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected stream index %u\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ENDOFSTREAM, "Unexpected stream flags %#x.\n", stream_flags);
    ok(!sample, "Unexpected sample object.\n");

    /* Flush. */
    hr = IMFSourceReader_Flush(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_Flush(reader, 100);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_Flush(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM);
    ok(hr == S_OK, "Failed to flush stream, hr %#x.\n", hr);

    hr = IMFSourceReader_Flush(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM);
    ok(hr == S_OK, "Failed to flush stream, hr %#x.\n", hr);

    hr = IMFSourceReader_Flush(reader, MF_SOURCE_READER_ALL_STREAMS);
    ok(hr == S_OK, "Failed to flush all streams, hr %#x.\n", hr);

    IMFSourceReader_Release(reader);

    /* Async mode. */
    callback = create_async_callback();

    hr = MFCreateAttributes(&attributes, 1);
    ok(hr == S_OK, "Failed to create attributes object, hr %#x.\n", hr);

    hr = IMFAttributes_SetUnknown(attributes, &MF_SOURCE_READER_ASYNC_CALLBACK,
            (IUnknown *)&callback->IMFSourceReaderCallback_iface);
    ok(hr == S_OK, "Failed to set attribute value, hr %#x.\n", hr);
    IMFSourceReaderCallback_Release(&callback->IMFSourceReaderCallback_iface);

    hr = MFCreateSourceReaderFromByteStream(stream, attributes, &reader);
    ok(hr == S_OK, "Failed to create source reader, hr %#x.\n", hr);
    IMFAttributes_Release(attributes);

    IMFSourceReader_Release(reader);

    IMFByteStream_Release(stream);
}

START_TEST(mfplat)
{
    HRESULT hr;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    init_functions();

    test_factory();
    test_source_reader();

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
}
