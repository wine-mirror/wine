/* GStreamer Decoder Transform
 *
 * Copyright 2021 Derek Lesho
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

#include "gst_private.h"

#include "mfapi.h"
#include "mferror.h"
#include "mfobjects.h"
#include "mftransform.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

const GUID *h264_input_types[] = {&MFVideoFormat_H264};
/* NV12 comes first https://docs.microsoft.com/en-us/windows/win32/medfound/mft-decoder-expose-output-types-in-native-order . thanks to @vitorhnn */
const GUID *h264_output_types[] = {&MFVideoFormat_NV12, &MFVideoFormat_I420, &MFVideoFormat_IYUV, &MFVideoFormat_YUY2, &MFVideoFormat_YV12};

const GUID *aac_input_types[] = {&MFAudioFormat_AAC};
const GUID *aac_output_types[] = {&MFAudioFormat_Float};

static struct decoder_desc
{
    const GUID *major_type;
    const GUID **input_types;
    unsigned int input_types_count;
    const GUID **output_types;
    unsigned int output_types_count;
} decoder_descs[] =
{
    { /* DECODER_TYPE_H264 */
        &MFMediaType_Video,
        h264_input_types,
        ARRAY_SIZE(h264_input_types),
        h264_output_types,
        ARRAY_SIZE(h264_output_types),
    },
    { /* DECODER_TYPE_AAC */
        &MFMediaType_Audio,
        aac_input_types,
        ARRAY_SIZE(aac_input_types),
        aac_output_types,
        ARRAY_SIZE(aac_output_types),
    }
};

struct pipeline_event
{
    enum
    {
        PIPELINE_EVENT_NONE,
        PIPELINE_EVENT_PARSER_STARTED,
        PIPELINE_EVENT_READ_REQUEST,
    } type;
    union
    {
        struct
        {
            struct wg_parser_stream *stream;
        } parser_started;
    } u;
};

struct mf_decoder
{
    IMFTransform IMFTransform_iface;
    LONG refcount;
    enum decoder_type type;
    IMFMediaType *input_type, *output_type;
    CRITICAL_SECTION cs, help_cs, event_cs;
    CONDITION_VARIABLE help_cv, event_cv;
    BOOL flushing, draining, eos, helper_thread_shutdown, video;
    HANDLE helper_thread, read_thread;
    uint64_t offset_tracker;
    struct wg_parser *wg_parser;
    struct wg_parser_stream *wg_stream;

    struct
    {
        enum
        {
            HELP_REQ_NONE,
            HELP_REQ_START_PARSER,
        } type;
    } help_request;

    struct pipeline_event event;
};

static struct mf_decoder *impl_mf_decoder_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct mf_decoder, IMFTransform_iface);
}

static HRESULT WINAPI mf_decoder_QueryInterface (IMFTransform *iface, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFTransform) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFTransform_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI mf_decoder_AddRef(IMFTransform *iface)
{
    struct mf_decoder *decoder = impl_mf_decoder_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&decoder->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI mf_decoder_Release(IMFTransform *iface)
{
    struct mf_decoder *decoder = impl_mf_decoder_from_IMFTransform(iface);
    ULONG refcount = InterlockedDecrement(&decoder->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (decoder->input_type)
        {
            IMFMediaType_Release(decoder->input_type);
            decoder->input_type = NULL;
        }

        if (decoder->output_type)
        {
            IMFMediaType_Release(decoder->output_type);
            decoder->output_type = NULL;
        }

        if (decoder->wg_parser)
        {
            /* NULL wg_parser is possible if the wg_parser creation failed. */

            if (decoder->wg_stream)
                wg_parser_disconnect(decoder->wg_parser);

            EnterCriticalSection(&decoder->event_cs);
            decoder->helper_thread_shutdown = TRUE;
            WakeAllConditionVariable(&decoder->event_cv);
            LeaveCriticalSection(&decoder->event_cs);

            EnterCriticalSection(&decoder->help_cs);
            WakeAllConditionVariable(&decoder->help_cv);
            LeaveCriticalSection(&decoder->help_cs);

            if (WaitForSingleObject(decoder->helper_thread, 10000) != WAIT_OBJECT_0)
                FIXME("Failed waiting for helper thread to terminate.\n");
            CloseHandle(decoder->helper_thread);
            if (WaitForSingleObject(decoder->read_thread, 10000) != WAIT_OBJECT_0)
                FIXME("Failed waiting for read thread to terminate.\n");
            CloseHandle(decoder->read_thread);

            wg_parser_destroy(decoder->wg_parser);
        }

        DeleteCriticalSection(&decoder->cs);
        DeleteCriticalSection(&decoder->help_cs);
        DeleteCriticalSection(&decoder->event_cs);

        heap_free(decoder);
    }

    return refcount;
}

static HRESULT WINAPI mf_decoder_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum, DWORD *input_maximum,
        DWORD *output_minimum, DWORD *output_maximum)
{
    TRACE("%p, %p, %p, %p, %p.\n", iface, input_minimum, input_maximum, output_minimum, output_maximum);

    *input_minimum = *input_maximum = *output_minimum = *output_maximum = 1;

    return S_OK;
}

static HRESULT WINAPI mf_decoder_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    TRACE("%p %p %p.\n", iface, inputs, outputs);

    *inputs = *outputs = 1;

    return S_OK;
}

static HRESULT WINAPI mf_decoder_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    TRACE("%p %u %p %u %p.\n", iface, input_size, inputs, output_size, outputs);

    return E_NOTIMPL;
}

static HRESULT WINAPI mf_decoder_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    struct mf_decoder *decoder = impl_mf_decoder_from_IMFTransform(iface);

    TRACE("%p %u %p\n", decoder, id, info);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    info->dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_DOES_NOT_ADDREF;
    info->cbAlignment = 0;
    info->cbSize = 0;
    /* TODO: retrieve following fields from gstreamer */
    info->hnsMaxLatency = 0;
    info->cbMaxLookahead = 0;
    return S_OK;
}

static HRESULT WINAPI mf_decoder_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct mf_decoder *decoder = impl_mf_decoder_from_IMFTransform(iface);
    MFT_OUTPUT_STREAM_INFO stream_info = {};
    GUID output_subtype;
    UINT64 framesize;

    TRACE("%p %u %p\n", decoder, id, info);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&decoder->cs);

    if (!decoder->output_type)
    {
        LeaveCriticalSection(&decoder->cs);
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    if (decoder->video)
    {
        stream_info.dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER |
                              MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES;
        stream_info.cbSize = 0;
        if (SUCCEEDED(IMFMediaType_GetGUID(decoder->output_type, &MF_MT_SUBTYPE, &output_subtype)) &&
            SUCCEEDED(IMFMediaType_GetUINT64(decoder->output_type, &MF_MT_FRAME_SIZE, &framesize)))
        {
            MFCalculateImageSize(&output_subtype, framesize >> 32, (UINT32) framesize, &stream_info.cbSize);
        }
        if (!stream_info.cbSize)
            ERR("Failed to get desired output buffer size\n");
    }
    else
    {
        stream_info.dwFlags = MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES;
        stream_info.cbSize = 4;
    }
    stream_info.cbAlignment = 0;

    LeaveCriticalSection(&decoder->cs);

    *info = stream_info;

    return S_OK;
}

static HRESULT WINAPI mf_decoder_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    FIXME("%p, %p. semi-stub!\n", iface, attributes);

    return MFCreateAttributes(attributes, 0);
}

static HRESULT WINAPI mf_decoder_GetInputStreamAttributes(IMFTransform *iface, DWORD id,
        IMFAttributes **attributes)
{
    FIXME("%p, %u, %p.\n", iface, id, attributes);

    return E_NOTIMPL;
}

static HRESULT WINAPI mf_decoder_GetOutputStreamAttributes(IMFTransform *iface, DWORD id,
        IMFAttributes **attributes)
{
    FIXME("%p, %u, %p.\n", iface, id, attributes);

    return E_NOTIMPL;
}

static HRESULT WINAPI mf_decoder_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    TRACE("%p, %u.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI mf_decoder_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    TRACE("%p, %u, %p.\n", iface, streams, ids);

    return E_NOTIMPL;
}

static HRESULT WINAPI mf_decoder_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    struct mf_decoder *decoder = impl_mf_decoder_from_IMFTransform(iface);
    IMFMediaType *input_type;
    HRESULT hr;

    TRACE("%p, %u, %u, %p\n", decoder, id, index, type);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (index >= decoder_descs[decoder->type].input_types_count)
        return MF_E_NO_MORE_TYPES;

    if (FAILED(hr = MFCreateMediaType(&input_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_SetGUID(input_type, &MF_MT_MAJOR_TYPE, decoder_descs[decoder->type].major_type)))
    {
        IMFMediaType_Release(input_type);
        return hr;
    }

    if (FAILED(hr = IMFMediaType_SetGUID(input_type, &MF_MT_SUBTYPE, decoder_descs[decoder->type].input_types[index])))
    {
        IMFMediaType_Release(input_type);
        return hr;
    }

    *type = input_type;

    return S_OK;
}

static HRESULT WINAPI mf_decoder_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    struct mf_decoder *decoder = impl_mf_decoder_from_IMFTransform(iface);
    IMFMediaType *output_type;
    HRESULT hr;

    TRACE("%p, %u, %u, %p\n", decoder, id, index, type);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (index >= decoder_descs[decoder->type].output_types_count)
        return MF_E_NO_MORE_TYPES;

    if (FAILED(hr = MFCreateMediaType(&output_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_SetGUID(output_type, &MF_MT_MAJOR_TYPE, decoder_descs[decoder->type].major_type)))
    {
        IMFMediaType_Release(output_type);
        return hr;
    }

    if (FAILED(hr = IMFMediaType_SetGUID(output_type, &MF_MT_SUBTYPE, decoder_descs[decoder->type].output_types[index])))
    {
        IMFMediaType_Release(output_type);
        return hr;
    }

    *type = output_type;

    return S_OK;
}

static HRESULT WINAPI mf_decoder_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct mf_decoder *decoder = impl_mf_decoder_from_IMFTransform(iface);
    struct wg_format input_format;
    GUID major_type, subtype;
    unsigned int i;
    HRESULT hr;

    TRACE("%p, %u, %p, %#x.\n", decoder, id, type, flags);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (!type)
    {
        if (flags & MFT_SET_TYPE_TEST_ONLY)
            return S_OK;

        EnterCriticalSection(&decoder->cs);

        if (decoder->wg_stream)
        {
            decoder->wg_stream = NULL;
            wg_parser_disconnect(decoder->wg_parser);
        }

        if (decoder->input_type)
        {
            IMFMediaType_Release(decoder->input_type);
            decoder->input_type = NULL;
        }

        LeaveCriticalSection(&decoder->cs);

        return S_OK;
    }

    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major_type)))
        return MF_E_INVALIDTYPE;
    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return MF_E_INVALIDTYPE;

    if (!(IsEqualGUID(&major_type, decoder_descs[decoder->type].major_type)))
        return MF_E_INVALIDTYPE;

    for (i = 0; i < decoder_descs[decoder->type].input_types_count; i++)
    {
        if (IsEqualGUID(&subtype, decoder_descs[decoder->type].input_types[i]))
            break;
        if (i == decoder_descs[decoder->type].input_types_count)
            return MF_E_INVALIDTYPE;
    }

    mf_media_type_to_wg_format(type, &input_format);
    if (!input_format.major_type)
        return MF_E_INVALIDTYPE;

    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    EnterCriticalSection(&decoder->cs);

    hr = S_OK;

    if (decoder->wg_stream)
    {
        decoder->wg_stream = NULL;
        wg_parser_disconnect(decoder->wg_parser);
    }

    if (!decoder->input_type)
        hr = MFCreateMediaType(&decoder->input_type);

    if (SUCCEEDED(hr) && FAILED(hr = IMFMediaType_CopyAllItems(type, (IMFAttributes*) decoder->input_type)))
    {
        IMFMediaType_Release(decoder->input_type);
        decoder->input_type = NULL;
    }

    if (decoder->input_type && decoder->output_type)
    {
        EnterCriticalSection(&decoder->help_cs);
        while(decoder->help_request.type != HELP_REQ_NONE)
            SleepConditionVariableCS(&decoder->help_cv, &decoder->help_cs, INFINITE);
        decoder->help_request.type = HELP_REQ_START_PARSER;
        LeaveCriticalSection(&decoder->help_cs);
        WakeAllConditionVariable(&decoder->help_cv);
    }

    LeaveCriticalSection(&decoder->cs);
    return hr;
}

static HRESULT WINAPI mf_decoder_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct mf_decoder *decoder = impl_mf_decoder_from_IMFTransform(iface);
    struct wg_format output_format;
    GUID major_type, subtype;
    HRESULT hr;
    unsigned int i;

    TRACE("%p, %u, %p, %#x.\n", decoder, id, type, flags);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (!type)
    {
        if (flags & MFT_SET_TYPE_TEST_ONLY)
            return S_OK;

        EnterCriticalSection(&decoder->cs);

        if (decoder->wg_stream)
        {
            decoder->wg_stream = NULL;
            wg_parser_disconnect(decoder->wg_parser);
        }

        if (decoder->output_type)
        {
            IMFMediaType_Release(decoder->output_type);
            decoder->output_type = NULL;
        }

        LeaveCriticalSection(&decoder->cs);

        return S_OK;
    }

    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major_type)))
        return MF_E_INVALIDTYPE;
    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return MF_E_INVALIDTYPE;

    if (!(IsEqualGUID(&major_type, decoder_descs[decoder->type].major_type)))
        return MF_E_INVALIDTYPE;

    for (i = 0; i < decoder_descs[decoder->type].output_types_count; i++)
    {
        if (IsEqualGUID(&subtype, decoder_descs[decoder->type].output_types[i]))
            break;
        if (i == decoder_descs[decoder->type].output_types_count)
            return MF_E_INVALIDTYPE;
    }

    mf_media_type_to_wg_format(type, &output_format);
    if (!output_format.major_type)
        return MF_E_INVALIDTYPE;

    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    EnterCriticalSection(&decoder->cs);

    hr = S_OK;

    if (decoder->wg_stream)
    {
        decoder->wg_stream = NULL;
        wg_parser_disconnect(decoder->wg_parser);
    }

    if (!decoder->output_type)
        hr = MFCreateMediaType(&decoder->output_type);

    if (SUCCEEDED(hr) && FAILED(hr = IMFMediaType_CopyAllItems(type, (IMFAttributes*) decoder->output_type)))
    {
        IMFMediaType_Release(decoder->output_type);
        decoder->output_type = NULL;
    }

    if (decoder->input_type && decoder->output_type)
    {
        EnterCriticalSection(&decoder->help_cs);
        while(decoder->help_request.type != HELP_REQ_NONE)
            SleepConditionVariableCS(&decoder->help_cv, &decoder->help_cs, INFINITE);
        decoder->help_request.type = HELP_REQ_START_PARSER;
        LeaveCriticalSection(&decoder->help_cs);
        WakeAllConditionVariable(&decoder->help_cv);
    }

    LeaveCriticalSection(&decoder->cs);
    return hr;
}

static HRESULT WINAPI mf_decoder_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("%p, %u, %p.\n", iface, id, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI mf_decoder_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("%p, %u, %p.\n", iface, id, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI mf_decoder_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    FIXME("%p, %u, %p\n", iface, id, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI mf_decoder_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    FIXME("%p, %p.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI mf_decoder_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    FIXME("%p, %s, %s.\n", iface, wine_dbgstr_longlong(lower), wine_dbgstr_longlong(upper));

    return E_NOTIMPL;
}

static HRESULT WINAPI mf_decoder_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    FIXME("%p, %u, %p.\n", iface, id, event);

    return E_NOTIMPL;
}

static DWORD CALLBACK helper_thread_func(PVOID ctx)
{
    struct mf_decoder *decoder = (struct mf_decoder *)ctx;

    for(;;)
    {
        EnterCriticalSection(&decoder->help_cs);

        while(!decoder->helper_thread_shutdown && decoder->help_request.type == HELP_REQ_NONE)
            SleepConditionVariableCS(&decoder->help_cv, &decoder->help_cs, INFINITE);
        if (decoder->helper_thread_shutdown)
        {
            LeaveCriticalSection(&decoder->help_cs);
            return 0;
        }

        switch(decoder->help_request.type)
        {
            case HELP_REQ_START_PARSER:
            {
                struct wg_format input_format, output_format;
                struct wg_rect wg_aperture = {0};
                MFVideoArea *aperture = NULL;
                UINT32 aperture_size;

                decoder->help_request.type = HELP_REQ_NONE;
                LeaveCriticalSection(&decoder->help_cs);

                mf_media_type_to_wg_format(decoder->input_type, &input_format);
                mf_media_type_to_wg_format(decoder->output_type, &output_format);

                if (SUCCEEDED(IMFMediaType_GetAllocatedBlob(decoder->output_type,
                    &MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8 **) &aperture, &aperture_size)))
                {
                    TRACE("Decoded media's aperture: x: %u %u/65536, y: %u %u/65536, area: %u x %u\n",
                        aperture->OffsetX.value, aperture->OffsetX.fract,
                        aperture->OffsetY.value, aperture->OffsetY.fract, aperture->Area.cx, aperture->Area.cy);

                    /* TODO: verify aperture params? */

                    wg_aperture.left = aperture->OffsetX.value;
                    wg_aperture.top = aperture->OffsetY.value;
                    wg_aperture.right = aperture->Area.cx;
                    wg_aperture.bottom = aperture->Area.cy;

                    CoTaskMemFree(aperture);
                }

                wg_parser_connect_unseekable(decoder->wg_parser,
                    &input_format, 1, &output_format, aperture ? &wg_aperture : NULL);

                EnterCriticalSection(&decoder->event_cs);
                while (!decoder->helper_thread_shutdown && decoder->event.type != PIPELINE_EVENT_NONE)
                    SleepConditionVariableCS(&decoder->event_cv, &decoder->event_cs, INFINITE);

                if (decoder->helper_thread_shutdown)
                {
                    LeaveCriticalSection(&decoder->event_cs);
                    return 0;
                }

                decoder->event.type = PIPELINE_EVENT_PARSER_STARTED;
                decoder->event.u.parser_started.stream = wg_parser_get_stream(decoder->wg_parser, 0);

                LeaveCriticalSection(&decoder->event_cs);
                WakeAllConditionVariable(&decoder->event_cv);

                break;
            }
            default:
                assert(0);
        }
    }
}

/* We use a separate thread to wait for reads, as we may want to wait to WAIT_ANY
   on a read and another event. */
static DWORD CALLBACK read_thread_func(PVOID ctx)
{
    struct mf_decoder *decoder = (struct mf_decoder *)ctx;
    uint64_t offset;
    uint32_t size;

    for (;;)
    {
        if (decoder->helper_thread_shutdown)
            break;

        if (!wg_parser_get_next_read_offset(decoder->wg_parser, &offset, &size))
            continue;

        EnterCriticalSection(&decoder->event_cs);
        while (!decoder->helper_thread_shutdown && decoder->event.type != PIPELINE_EVENT_NONE)
            SleepConditionVariableCS(&decoder->event_cv, &decoder->event_cs, INFINITE);

        if (decoder->helper_thread_shutdown)
        {
            LeaveCriticalSection(&decoder->event_cs);
            break;
        }

        decoder->event.type = PIPELINE_EVENT_READ_REQUEST;
        WakeAllConditionVariable(&decoder->event_cv);
        while (!decoder->helper_thread_shutdown && decoder->event.type == PIPELINE_EVENT_READ_REQUEST)
            SleepConditionVariableCS(&decoder->event_cv, &decoder->event_cs, INFINITE);
        LeaveCriticalSection(&decoder->event_cs);
    }

    return 0;
}

static struct pipeline_event get_pipeline_event(struct mf_decoder *decoder)
{
    struct pipeline_event ret;

    EnterCriticalSection(&decoder->event_cs);
    while(decoder->event.type == PIPELINE_EVENT_NONE)
        SleepConditionVariableCS(&decoder->event_cv, &decoder->event_cs, INFINITE);

    ret = decoder->event;

    if (ret.type != PIPELINE_EVENT_READ_REQUEST)
    {
        decoder->event.type = PIPELINE_EVENT_NONE;
        WakeAllConditionVariable(&decoder->event_cv);
    }

    LeaveCriticalSection(&decoder->event_cs);

    return ret;
}

static HRESULT WINAPI mf_decoder_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    struct mf_decoder *decoder = impl_mf_decoder_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("%p, %x %lu.\n", decoder, message, param);

    EnterCriticalSection(&decoder->cs);
    if (!decoder->input_type || !decoder->output_type)
    {
        LeaveCriticalSection(&decoder->cs);
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    hr = S_OK;

    switch (message)
    {
        case MFT_MESSAGE_NOTIFY_BEGIN_STREAMING:
        case MFT_MESSAGE_NOTIFY_START_OF_STREAM:
            break;
        case MFT_MESSAGE_NOTIFY_END_OF_STREAM:
        {
            if (param)
            {
                hr = MF_E_INVALIDSTREAMNUMBER;
                break;
            }
            if (!decoder->wg_stream)
            {
                ERR("End-Of-Stream marked on a decoder MFT which hasn't finished initialization\n");
                hr = E_FAIL;
                break;
            }

            decoder->eos = TRUE;
            break;
        }
        case MFT_MESSAGE_COMMAND_DRAIN:
        {
            struct pipeline_event pip_event;

            if (!decoder->wg_stream)
            {
                ERR("Drain requested on a decoder MFT which hasn't finished initialization\n");
                hr = E_FAIL;
                break;
            }

            pip_event = get_pipeline_event(decoder);
            assert(pip_event.type == PIPELINE_EVENT_READ_REQUEST);

            wg_parser_push_data(decoder->wg_parser, WG_READ_EOS, NULL, 0);

            EnterCriticalSection(&decoder->event_cs);
            decoder->event.type = PIPELINE_EVENT_NONE;
            LeaveCriticalSection(&decoder->event_cs);
            WakeAllConditionVariable(&decoder->event_cv);

            decoder->draining = TRUE;
            decoder->offset_tracker = 0;
            break;
        }
        case MFT_MESSAGE_COMMAND_FLUSH:
        {
            struct pipeline_event pip_event;

            if (!decoder->wg_stream)
            {
                ERR("Flush requested on a decoder MFT which hasn't finished initialization\n");
                hr = E_FAIL;
                break;
            }

            pip_event = get_pipeline_event(decoder);
            assert(pip_event.type == PIPELINE_EVENT_READ_REQUEST);

            wg_parser_push_data(decoder->wg_parser, WG_READ_FLUSHING, NULL, 0);

            EnterCriticalSection(&decoder->event_cs);
            decoder->event.type = PIPELINE_EVENT_NONE;
            LeaveCriticalSection(&decoder->event_cs);
            WakeAllConditionVariable(&decoder->event_cv);

            decoder->offset_tracker = 0;
            break;
        }
        default:
        {
            ERR("Unhandled message type %x.\n", message);
            hr = E_FAIL;
            break;
        }
    }

    LeaveCriticalSection(&decoder->cs);
    return hr;
}

static HRESULT WINAPI mf_decoder_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct mf_decoder *decoder = impl_mf_decoder_from_IMFTransform(iface);
    struct pipeline_event pip_event;
    IMFMediaBuffer *buffer = NULL;
    HRESULT hr = S_OK;
    BYTE *buffer_data;
    DWORD buffer_size;
    uint32_t size = 0;
    uint64_t offset;

    TRACE("%p, %u, %p, %#x.\n", decoder, id, sample, flags);

    if (flags)
        WARN("Unsupported flags %#x\n", flags);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&decoder->cs);

    if (!decoder->input_type || !decoder->output_type)
    {
        LeaveCriticalSection(&decoder->cs);
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    if (decoder->draining)
    {
        LeaveCriticalSection(&decoder->cs);
        return MF_E_NOTACCEPTING;
    }

    if (!decoder->wg_stream)
    {
        pip_event = get_pipeline_event(decoder);

        switch (pip_event.type)
        {
            case PIPELINE_EVENT_PARSER_STARTED:
                decoder->wg_stream = pip_event.u.parser_started.stream;
                break;
            case PIPELINE_EVENT_READ_REQUEST:
                break;
            default:
                assert(0);
        }
    }

    if (decoder->wg_stream && !wg_parser_stream_drain(decoder->wg_stream))
    {
        LeaveCriticalSection(&decoder->cs);
        return MF_E_NOTACCEPTING;
    }

    /* At this point, we either have a pre-init read request, or drained pipeline */

    if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer)))
        goto done;

    if (FAILED(hr = IMFMediaBuffer_Lock(buffer, &buffer_data, NULL, &buffer_size)))
        goto done;

    pip_event = get_pipeline_event(decoder);
    assert(pip_event.type == PIPELINE_EVENT_READ_REQUEST);

    for(;;)
    {
        uint32_t copy_size;

        if (!wg_parser_get_next_read_offset(decoder->wg_parser, &offset, &size))
            continue;

        copy_size = min(size, buffer_size);

        if (offset != decoder->offset_tracker)
        {
            ERR("A seek is needed, MFTs don't support this!\n");
            wg_parser_push_data(decoder->wg_parser, WG_READ_FAILURE, NULL, 0);
            IMFMediaBuffer_Unlock(buffer);
            hr = E_FAIL;
            goto done;
        }

        wg_parser_push_data(decoder->wg_parser, WG_READ_SUCCESS, buffer_data, buffer_size);

        decoder->offset_tracker += copy_size;

        if (buffer_size <= size)
            break;

        buffer_data += copy_size;
        buffer_size -= copy_size;

        WARN("Input sample split into multiple read requests\n");
    }

    EnterCriticalSection(&decoder->event_cs);
    decoder->event.type = PIPELINE_EVENT_NONE;
    LeaveCriticalSection(&decoder->event_cs);
    WakeAllConditionVariable(&decoder->event_cv);

    IMFMediaBuffer_Unlock(buffer);

    done:
    if (buffer)
        IMFMediaBuffer_Release(buffer);
    LeaveCriticalSection(&decoder->cs);
    return hr;
}

static HRESULT WINAPI mf_decoder_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct mf_decoder *decoder = impl_mf_decoder_from_IMFTransform(iface);
    MFT_OUTPUT_DATA_BUFFER *relevant_buffer = NULL;
    struct wg_parser_buffer wg_buffer;
    struct pipeline_event pip_event;
    IMFMediaBuffer *buffer;
    DWORD buffer_len;
    unsigned int i;
    BYTE *data;
    HRESULT hr;

    TRACE("%p, %#x, %u, %p, %p.\n", iface, flags, count, samples, status);

    if (flags)
        WARN("Unsupported flags %#x\n", flags);

    for (i = 0; i < count; i++)
    {
        MFT_OUTPUT_DATA_BUFFER *out_buffer = &samples[i];

        if (out_buffer->dwStreamID != 0)
            return MF_E_INVALIDSTREAMNUMBER;

        if (relevant_buffer)
            return MF_E_INVALIDSTREAMNUMBER;

        relevant_buffer = out_buffer;
    }

    if (!relevant_buffer)
        return S_OK;

    EnterCriticalSection(&decoder->cs);

    if (!decoder->input_type || !decoder->output_type)
    {
        LeaveCriticalSection(&decoder->cs);
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    if (!decoder->wg_stream)
    {
        pip_event = get_pipeline_event(decoder);

        switch (pip_event.type)
        {
            case PIPELINE_EVENT_PARSER_STARTED:
                decoder->wg_stream = pip_event.u.parser_started.stream;
                break;
            case PIPELINE_EVENT_READ_REQUEST:
                LeaveCriticalSection(&decoder->cs);
                return MF_E_TRANSFORM_NEED_MORE_INPUT;
            default:
                assert(0);
        }
    }

    if (wg_parser_stream_drain(decoder->wg_stream))
    {
        /* this would be unexpected, as we should get the EOS-event when a drain command completes. */
        assert (!decoder->draining);

        LeaveCriticalSection(&decoder->cs);
        return MF_E_TRANSFORM_NEED_MORE_INPUT;
    }

    if (!wg_parser_stream_get_buffer(decoder->wg_stream, &wg_buffer))
    {
        if (!decoder->draining)
        {
            LeaveCriticalSection(&decoder->cs);
            WARN("Received EOS event while not draining\n");
            return E_FAIL;
        }
        decoder->draining = FALSE;
        LeaveCriticalSection(&decoder->cs);
        return MF_E_TRANSFORM_NEED_MORE_INPUT;
    }

    if (relevant_buffer->pSample)
    {
        if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(relevant_buffer->pSample, &buffer)))
        {
            ERR("Failed to get buffer from sample, hr %#x.\n", hr);
            LeaveCriticalSection(&decoder->cs);
            return hr;
        }
    }
    else
    {
        if (FAILED(hr = MFCreateMemoryBuffer(wg_buffer.size, &buffer)))
        {
            ERR("Failed to create buffer, hr %#x.\n", hr);
            LeaveCriticalSection(&decoder->cs);
            return hr;
        }

        if (FAILED(hr = MFCreateSample(&relevant_buffer->pSample)))
        {
            ERR("Failed to create sample, hr %#x.\n", hr);
            LeaveCriticalSection(&decoder->cs);
            IMFMediaBuffer_Release(buffer);
            return hr;
        }

        if (FAILED(hr = IMFSample_AddBuffer(relevant_buffer->pSample, buffer)))
        {
            ERR("Failed to add buffer, hr %#x.\n", hr);
            goto out;
        }
    }

    if (FAILED(hr = IMFMediaBuffer_GetMaxLength(buffer, &buffer_len)))
    {
        ERR("Failed to get buffer size, hr %#x.\n", hr);
        goto out;
    }

    if (buffer_len < wg_buffer.size)
    {
        WARN("Client's buffer is smaller (%u bytes) than the output sample (%u bytes)\n",
            buffer_len, wg_buffer.size);

        if (FAILED(hr = IMFMediaBuffer_SetCurrentLength(buffer, buffer_len)))
        {
            ERR("Failed to set size, hr %#x.\n", hr);
            goto out;
        }
    }
    else if (FAILED(hr = IMFMediaBuffer_SetCurrentLength(buffer, wg_buffer.size)))
    {
        ERR("Failed to set size, hr %#x.\n", hr);
        goto out;
    }


    if (FAILED(hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL)))
    {
        ERR("Failed to lock buffer, hr %#x.\n", hr);
        goto out;
    }

    if (!wg_parser_stream_copy_buffer(decoder->wg_stream, data, 0, min(buffer_len, wg_buffer.size)))
    {
        hr = E_FAIL;
        goto out;
    }

    if (FAILED(hr = IMFMediaBuffer_Unlock(buffer)))
    {
        ERR("Failed to unlock buffer, hr %#x.\n", hr);
        goto out;
    }

    if (FAILED(hr = IMFSample_SetSampleTime(relevant_buffer->pSample, wg_buffer.pts)))
    {
        ERR("Failed to set sample time, hr %#x.\n", hr);
        goto out;
    }

    if (FAILED(hr = IMFSample_SetSampleDuration(relevant_buffer->pSample, wg_buffer.duration)))
    {
        ERR("Failed to set sample duration, hr %#x.\n", hr);
        goto out;
    }

    relevant_buffer->dwStatus = 0;
    relevant_buffer->pEvents = NULL;
    *status = 0;

    out:
    if (SUCCEEDED(hr))
        wg_parser_stream_release_buffer(decoder->wg_stream);
    LeaveCriticalSection(&decoder->cs);

    if (FAILED(hr))
    {
        IMFSample_Release(relevant_buffer->pSample);
        relevant_buffer->pSample = NULL;
    }

    IMFMediaBuffer_Release(buffer);

    return hr;
}

static const IMFTransformVtbl mf_decoder_vtbl =
{
    mf_decoder_QueryInterface,
    mf_decoder_AddRef,
    mf_decoder_Release,
    mf_decoder_GetStreamLimits,
    mf_decoder_GetStreamCount,
    mf_decoder_GetStreamIDs,
    mf_decoder_GetInputStreamInfo,
    mf_decoder_GetOutputStreamInfo,
    mf_decoder_GetAttributes,
    mf_decoder_GetInputStreamAttributes,
    mf_decoder_GetOutputStreamAttributes,
    mf_decoder_DeleteInputStream,
    mf_decoder_AddInputStreams,
    mf_decoder_GetInputAvailableType,
    mf_decoder_GetOutputAvailableType,
    mf_decoder_SetInputType,
    mf_decoder_SetOutputType,
    mf_decoder_GetInputCurrentType,
    mf_decoder_GetOutputCurrentType,
    mf_decoder_GetInputStatus,
    mf_decoder_GetOutputStatus,
    mf_decoder_SetOutputBounds,
    mf_decoder_ProcessEvent,
    mf_decoder_ProcessMessage,
    mf_decoder_ProcessInput,
    mf_decoder_ProcessOutput,
};

HRESULT decode_transform_create(REFIID riid, void **obj, enum decoder_type type)
{
    struct mf_decoder *object;
    struct wg_parser *parser;

    TRACE("%s, %p %u.\n", debugstr_guid(riid), obj, type);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFTransform_iface.lpVtbl = &mf_decoder_vtbl;
    object->refcount = 1;

    object->type = type;
    object->video = decoder_descs[type].major_type == &MFMediaType_Video;

    InitializeCriticalSection(&object->cs);
    InitializeCriticalSection(&object->help_cs);
    InitializeCriticalSection(&object->event_cs);
    InitializeConditionVariable(&object->help_cv);
    InitializeConditionVariable(&object->event_cv);

    if (!(parser = wg_parser_create(WG_PARSER_DECODEBIN, TRUE)))
    {
        ERR("Failed to create Decoder MFT type %u: Unspecified GStreamer error\n", type);
        IMFTransform_Release(&object->IMFTransform_iface);
        return E_OUTOFMEMORY;
    }
    object->wg_parser = parser;

    object->helper_thread = CreateThread(NULL, 0, helper_thread_func, object, 0, NULL);
    object->read_thread = CreateThread(NULL, 0, read_thread_func, object, 0, NULL);

    *obj = &object->IMFTransform_iface;
    return S_OK;
}
