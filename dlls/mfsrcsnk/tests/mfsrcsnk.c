/*
 * Copyright 2022 Nikolay Sivov for CodeWeavers
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

#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"

#include "wine/test.h"

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

static void test_wave_sink(void)
{
    IMFMediaType *media_type, *media_type2;
    IMFMediaTypeHandler *type_handler;
    IMFPresentationClock *clock;
    IMFStreamSink *stream_sink;
    IMFMediaSink *sink, *sink2;
    IMFByteStream *bytestream;
    DWORD id, count, flags;
    HRESULT hr;
    GUID guid;

    hr = MFCreateWAVEMediaSink(NULL, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateWAVEMediaSink(NULL, NULL, &sink);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, 2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_NUM_CHANNELS, 2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, 8);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTempFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_DELETE_IF_EXIST, 0, &bytestream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateWAVEMediaSink(bytestream, NULL, &sink);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateWAVEMediaSink(bytestream, media_type, &sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Sink tests */
    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(flags == (MEDIASINK_FIXED_STREAMS | MEDIASINK_RATELESS), "Unexpected flags %#lx.\n", flags);

    hr = IMFMediaSink_GetStreamSinkCount(sink, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetStreamSinkCount(sink, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %lu.\n", count);

    hr = IMFMediaSink_AddStreamSink(sink, 123, media_type, &stream_sink);
    ok(hr == MF_E_STREAMSINKS_FIXED, "Unexpected hr %#lx.\n", hr);

    check_interface(sink, &IID_IMFMediaEventGenerator, TRUE);
    check_interface(sink, &IID_IMFFinalizableMediaSink, TRUE);
    check_interface(sink, &IID_IMFClockStateSink, TRUE);

    /* Clock */
    hr = MFCreatePresentationClock(&clock);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_SetPresentationClock(sink, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_SetPresentationClock(sink, clock);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFPresentationClock_Release(clock);

    /* Stream tests */
    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 0, &stream_sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFStreamSink_GetIdentifier(stream_sink, &id);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(id == 1, "Unexpected id %#lx.\n", id);
    IMFStreamSink_Release(stream_sink);

    hr = IMFMediaSink_GetStreamSinkById(sink, 0, &stream_sink);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaSink_GetStreamSinkById(sink, id, &stream_sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamSink_GetMediaSink(stream_sink, &sink2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaSink_Release(sink2);

    check_interface(stream_sink, &IID_IMFMediaEventGenerator, TRUE);
    check_interface(stream_sink, &IID_IMFMediaTypeHandler, TRUE);

    hr = IMFStreamSink_GetMediaTypeHandler(stream_sink, &type_handler);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaTypeHandler_GetMajorType(type_handler, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Audio), "Unexpected major type.\n");

    hr = IMFMediaTypeHandler_GetMediaTypeCount(type_handler, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %lu.\n", count);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(type_handler, &media_type2);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IMFMediaType_SetUINT32(media_type2, &MF_MT_AUDIO_NUM_CHANNELS, 1);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, NULL);
        ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
        hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, media_type2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        IMFMediaType_Release(media_type2);
    }

    /* Shutdown state */
    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamSink_GetMediaSink(stream_sink, &sink2);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamSink_GetIdentifier(stream_sink, &id);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaTypeHandler_GetMajorType(type_handler, &guid);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#lx.\n", hr);

    IMFStreamSink_Release(stream_sink);

    hr = IMFMediaSink_AddStreamSink(sink, 123, media_type, &stream_sink);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 0, &stream_sink);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaSink_GetStreamSinkById(sink, 0, &stream_sink);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    IMFMediaSink_Release(sink);

    IMFMediaTypeHandler_Release(type_handler);
    IMFMediaType_Release(media_type);
    IMFByteStream_Release(bytestream);
}

START_TEST(mfsrcsnk)
{
    HRESULT hr;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    test_wave_sink();

    hr = MFShutdown();
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
}
