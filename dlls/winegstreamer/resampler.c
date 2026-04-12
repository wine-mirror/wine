/* Copyright 2022 Rémi Bernon for CodeWeavers
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
#include <stddef.h>

#include "windef.h"
#include "winbase.h"

#include "gst_private.h"

#include "dmoreg.h"
#include "dmort.h"
#include "dshow.h"
#include "mediaerr.h"
#include "mfapi.h"
#include "mferror.h"
#include "mfobjects.h"
#include "mftransform.h"
#include "wmcodecdsp.h"

#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static const AVRational USER_TIME_BASE_Q = {1, 10000000};

static void clear_dmo_media_type(DMO_MEDIA_TYPE *mt)
{
    MoFreeMediaType(mt);
    memset(mt, 0, sizeof(*mt));
}

static enum AVSampleFormat sample_format_from_wave_format_tag(UINT tag, UINT depth)
{
    switch (tag)
    {
    case WAVE_FORMAT_PCM:
        if (depth == 32) return AV_SAMPLE_FMT_S32;
        if (depth == 16) return AV_SAMPLE_FMT_S16;
        if (depth == 8)  return AV_SAMPLE_FMT_U8;
        break;
    case WAVE_FORMAT_IEEE_FLOAT:
        if (depth == 64) return AV_SAMPLE_FMT_DBL;
        if (depth == 32) return AV_SAMPLE_FMT_FLT;
        break;
    }

    FIXME("Format tag %#x depth %#x implemented\n", tag, depth);
    return AV_SAMPLE_FMT_NONE;
}

static void audio_frame_init_from_format(AVFrame *frame, const WAVEFORMATEXTENSIBLE *format)
{
    if (format->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        WAVEFORMATEXTENSIBLE *extensible = CONTAINING_RECORD(format, WAVEFORMATEXTENSIBLE, Format);
        frame->format = sample_format_from_wave_format_tag(extensible->SubFormat.Data1, extensible->Format.wBitsPerSample);
        av_channel_layout_from_mask(&frame->ch_layout, extensible->dwChannelMask);
    }
    else
    {
        frame->format = sample_format_from_wave_format_tag(format->Format.wFormatTag, format->Format.wBitsPerSample);
        av_channel_layout_default(&frame->ch_layout, format->Format.nChannels);
    }
    frame->sample_rate = format->Format.nSamplesPerSec;
}

static const char *debugstr_avtime(INT64 time, AVRational time_base)
{
    time = av_rescale_q_rnd(time, time_base, USER_TIME_BASE_Q, AV_ROUND_PASS_MINMAX);
    if (time == AV_NOPTS_VALUE)
        return "(none)";
    return wine_dbg_sprintf("%I64d", time);
}

static void media_buffer_release(void *opaque, uint8_t *data)
{
    IMediaBuffer *media_buffer = opaque;
    IMediaBuffer_Release(media_buffer);
}

static AVBufferRef *buffer_from_media_buffer(IMediaBuffer *media_buffer, int flags)
{
    AVBufferRef *buffer;
    HRESULT hr;
    DWORD size;
    BYTE *data;

    if (FAILED(hr = IMediaBuffer_GetBufferAndLength(media_buffer, &data, &size)))
        return NULL;
    if (!flags && FAILED(hr = IMediaBuffer_GetMaxLength(media_buffer, &size)))
        return NULL;

    if (!(buffer = av_buffer_create(data, size, media_buffer_release, media_buffer, flags)))
        return NULL;
    IMediaBuffer_AddRef(media_buffer);
    return buffer;
}

static int audio_frame_wrap_buffer(AVFrame *frame, const WAVEFORMATEXTENSIBLE *format, AVBufferRef *buffer)
{
    int size;

    TRACE("frame %p, type %p, buffer %p\n", frame, format, buffer);

    audio_frame_init_from_format(frame, format);
    frame->nb_samples = buffer->size / format->Format.nBlockAlign;

    size = av_samples_fill_arrays(frame->data, frame->linesize, buffer->data,
            frame->ch_layout.nb_channels, frame->nb_samples, frame->format, 1);
    if (size > buffer->size)
        return -1;

    frame->opaque = (void *)-1; /* avoid reusing frame */
    frame->buf[0] = av_buffer_ref(buffer);
    frame->extended_data = frame->data;
    return size;
}

static int audio_frame_copy_from_buffer(AVFrame *frame, const WAVEFORMATEXTENSIBLE *format, AVBufferRef *buffer)
{
    UINT8 *input_planes[4];
    int size;

    ERR("frame %p, buffer %p\n", frame, buffer);

    size = av_samples_fill_arrays(input_planes, NULL, buffer->data, frame->ch_layout.nb_channels,
            frame->nb_samples, frame->format, 1);
    av_samples_copy(frame->data, input_planes, 0, 0, buffer->size / format->Format.nBlockAlign,
            frame->ch_layout.nb_channels, frame->format);
    return size;
}

static int audio_frame_copy_to_buffer(AVFrame *frame, const WAVEFORMATEXTENSIBLE *format, AVBufferRef *buffer)
{
    UINT8 *output_planes[4];
    int size;

    ERR("frame %p, buffer %p\n", frame, buffer);

    size = av_samples_fill_arrays(output_planes, NULL, buffer->data, frame->ch_layout.nb_channels,
            frame->nb_samples, frame->format, 1);
    av_samples_copy(output_planes, frame->data, 0, 0, frame->nb_samples,
            frame->ch_layout.nb_channels, frame->format);
    return size;
}

static HRESULT MF_RESULT_FROM_DMO(HRESULT hr)
{
    switch (hr)
    {
    case DMO_E_INVALIDSTREAMINDEX: return MF_E_INVALIDSTREAMNUMBER;
    case DMO_E_INVALIDTYPE:        return MF_E_INVALIDMEDIATYPE;
    case DMO_E_TYPE_NOT_SET:       return MF_E_TRANSFORM_TYPE_NOT_SET;
    case DMO_E_NOTACCEPTING:       return MF_E_NOTACCEPTING;
    case DMO_E_TYPE_NOT_ACCEPTED:  return MF_E_INVALIDMEDIATYPE;
    case DMO_E_NO_MORE_ITEMS:      return MF_E_NO_MORE_TYPES;
    case S_FALSE:                  return MF_E_TRANSFORM_NEED_MORE_INPUT;
    }

    return hr;
}

static const GUID *audio_formats[] =
{
    &MFAudioFormat_Float,
    &MFAudioFormat_PCM,
};

struct resampler
{
    IUnknown IUnknown_inner;
    IMFTransform IMFTransform_iface;
    IMediaObject IMediaObject_iface;
    IPropertyBag IPropertyBag_iface;
    IPropertyStore IPropertyStore_iface;
    IWMResamplerProps IWMResamplerProps_iface;
    IUnknown *outer;
    LONG refcount;

    DMO_MEDIA_TYPE input_type;
    WAVEFORMATEXTENSIBLE input_format;
    MFT_INPUT_STREAM_INFO input_info;
    DMO_MEDIA_TYPE output_type;
    WAVEFORMATEXTENSIBLE output_format;
    MFT_OUTPUT_STREAM_INFO output_info;

    struct SwrContext *context;
    AVFrame input_frame;
    AVFrame output_frame;
    AVFrame current_frame;
    AVRational time_base;
};

static INT64 resampler_time_to_user(struct resampler *impl, INT64 time)
{
    return av_rescale_q_rnd(time, impl->time_base, USER_TIME_BASE_Q, AV_ROUND_PASS_MINMAX);
}

static INT64 resampler_time_from_user(struct resampler *impl, INT64 time)
{
    return av_rescale_q_rnd(time, USER_TIME_BASE_Q, impl->time_base, AV_ROUND_PASS_MINMAX);
}

static HRESULT resampler_process_frame(struct resampler *impl, AVFrame *input_frame, DMO_OUTPUT_DATA_BUFFER *output)
{
    INT64 pts, duration = INT64_MAX;
    AVFrame output_frame = {0};
    AVBufferRef *buffer;
    IMFSample *sample;
    HRESULT hr;
    int ret;

    if (!(buffer = buffer_from_media_buffer(output->pBuffer, 0)))
        return E_OUTOFMEMORY;
    if ((ret = audio_frame_wrap_buffer(&output_frame, &impl->output_format, buffer)) < 0)
        av_frame_move_ref(&output_frame, &impl->output_frame);

    pts = swr_next_pts(impl->context, input_frame->pts);
    if ((ret = swr_convert(impl->context, output_frame.data, buffer->size / impl->output_format.Format.nBlockAlign,
            (const uint8_t **)input_frame->data, input_frame->nb_samples)) < 0)
        ERR("error ret %d (%s)\n", -ret, av_err2str(ret));
    else if ((output_frame.nb_samples = ret))
    {
        if (!output_frame.opaque)
            ret = audio_frame_copy_to_buffer(&output_frame, &impl->output_format, buffer);
        else
            ret = output_frame.nb_samples * impl->output_format.Format.nBlockAlign;
        duration = av_rescale(output_frame.nb_samples, 10000000, output_frame.sample_rate);
    }

    if (!output_frame.opaque)
        av_frame_move_ref(&impl->output_frame, &output_frame);
    else
        av_frame_unref(&output_frame);
    av_buffer_unref(&buffer);

    output->dwStatus = 0;
    IMediaBuffer_SetLength(output->pBuffer, ret >= 0 ? ret : 0);
    if (ret < 0)
        return E_FAIL;
    if (!ret)
        return S_FALSE;

    output->dwStatus |= DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT;
    if (swr_get_out_samples(impl->context, 0))
        output->dwStatus |= DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE;
    output->rtTimestamp = resampler_time_to_user(impl, pts);
    if (output->rtTimestamp != INT64_MIN)
        output->dwStatus |= DMO_OUTPUT_DATA_BUFFERF_TIME;
    output->rtTimelength = duration;
    if (output->rtTimelength != INT64_MIN)
        output->dwStatus |= DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH;

    if (SUCCEEDED(hr = IMediaBuffer_QueryInterface(output->pBuffer, &IID_IMFSample, (void **)&sample)))
    {
        if (output->rtTimestamp != INT64_MIN)
            IMFSample_SetSampleTime(sample, output->rtTimestamp);
        if (output->rtTimelength != INT64_MIN)
            IMFSample_SetSampleDuration(sample, output->rtTimelength);
        if (output->dwStatus & DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT)
            IMFSample_SetUINT32(sample, &MFSampleExtension_CleanPoint, 1);
        IMFSample_Release(sample);
    }

    TRACE("returning output size %#x, pts %s, duration %s status %#lx\n", ret,
            debugstr_avtime(output->rtTimestamp, USER_TIME_BASE_Q),
            debugstr_avtime(output->rtTimelength, USER_TIME_BASE_Q), output->dwStatus);
    return S_OK;
}

static HRESULT resampler_process_input(struct resampler *impl, const DMO_OUTPUT_DATA_BUFFER *input)
{
    LONGLONG pts = input->rtTimestamp, duration = input->rtTimelength;
    AVBufferRef *buffer;
    IMFSample *sample;
    HRESULT hr;
    int ret;

    if (!impl->context)
        return DMO_E_TYPE_NOT_SET;
    if (impl->current_frame.nb_samples)
        return DMO_E_NOTACCEPTING;

    if (!(buffer = buffer_from_media_buffer(input->pBuffer, AV_BUFFER_FLAG_READONLY)))
        return E_OUTOFMEMORY;
    if ((ret = audio_frame_wrap_buffer(&impl->current_frame, &impl->input_format, buffer)) < 0)
    {
        ret = audio_frame_copy_from_buffer(&impl->input_frame, &impl->input_format, buffer);
        av_frame_move_ref(&impl->current_frame, &impl->input_frame);
    }
    av_buffer_unref(&buffer);

    if (SUCCEEDED(hr = IMediaBuffer_QueryInterface(input->pBuffer, &IID_IMFSample, (void **)&sample)))
    {
        if (FAILED(IMFSample_GetSampleTime(sample, &pts)))
            pts = INT64_MIN;
        if (FAILED(IMFSample_GetSampleDuration(sample, &duration)))
            duration = INT64_MIN;
        IMFSample_Release(sample);
    }

    impl->current_frame.pts = resampler_time_from_user(impl, pts);
    impl->current_frame.duration = resampler_time_from_user(impl, duration);
    TRACE("input samples %u, time %s, duration %s\n", impl->current_frame.nb_samples,
            debugstr_avtime(impl->current_frame.pts, impl->time_base),
            debugstr_avtime(impl->current_frame.duration, impl->time_base));
    return S_OK;
}

static HRESULT resampler_process_output(struct resampler *impl, DMO_OUTPUT_DATA_BUFFER *output)
{
    HRESULT hr;

    if (!impl->context)
        return DMO_E_TYPE_NOT_SET;

    hr = resampler_process_frame(impl, &impl->current_frame, output);
    if (!impl->current_frame.opaque)
        av_frame_move_ref(&impl->input_frame, &impl->current_frame);
    else
        av_frame_unref(&impl->current_frame);

    return hr;
}

static void resampler_cleanup(struct resampler *impl)
{
    impl->time_base.num = impl->time_base.den = 1;
    memset(&impl->input_format, 0, sizeof(impl->input_format));
    memset(&impl->output_format, 0, sizeof(impl->output_format));
    av_frame_unref(&impl->current_frame);
    av_frame_unref(&impl->output_frame);
    av_frame_unref(&impl->input_frame);
    swr_free(&impl->context);
}

static HRESULT init_audio_format(const DMO_MEDIA_TYPE *type, WAVEFORMATEXTENSIBLE *format)
{
    if (IsEqualGUID(&type->formattype, &FORMAT_WaveFormatEx))
    {
        const WAVEFORMATEX *wfx = (WAVEFORMATEX *)type->pbFormat;
        if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
            *format = *(WAVEFORMATEXTENSIBLE *)wfx;
        else
        {
            memset(format, 0, sizeof(*format));
            format->Format = *wfx;
            format->dwChannelMask = 0;
            format->SubFormat = type->subtype;
        }
        return S_OK;
    }

    FIXME("Format %s not implemented\n", debugstr_guid(&type->formattype));
    return E_NOTIMPL;
}

static HRESULT resampler_init(struct resampler *impl)
{
    HRESULT hr;
    int ret;

    resampler_cleanup(impl);

    if (!(impl->context = swr_alloc()))
        return E_OUTOFMEMORY;
    if (FAILED(hr = init_audio_format(&impl->input_type, &impl->input_format))
            || FAILED(hr = init_audio_format(&impl->output_type, &impl->output_format)))
    {
        resampler_cleanup(impl);
        return hr;
    }

    audio_frame_init_from_format(&impl->input_frame, &impl->input_format);
    impl->input_frame.nb_samples = impl->input_format.Format.nSamplesPerSec;
    if ((ret = av_frame_get_buffer(&impl->input_frame, 0)) < 0)
        goto failed;
    audio_frame_init_from_format(&impl->output_frame, &impl->output_format);
    impl->output_frame.nb_samples = impl->output_format.Format.nSamplesPerSec;
    if ((ret = av_frame_get_buffer(&impl->output_frame, 0)) < 0)
        goto failed;

    av_opt_set_chlayout(impl->context, "in_chlayout", &impl->input_frame.ch_layout, 0);
    av_opt_set_int(impl->context, "in_sample_rate", impl->input_frame.sample_rate, 0);
    av_opt_set_sample_fmt(impl->context, "in_sample_fmt", impl->input_frame.format, 0);
    impl->time_base.den *= impl->input_frame.sample_rate;

    av_opt_set_chlayout(impl->context, "out_chlayout", &impl->output_frame.ch_layout, 0);
    av_opt_set_int(impl->context, "out_sample_rate", impl->output_frame.sample_rate, 0);
    av_opt_set_sample_fmt(impl->context, "out_sample_fmt", impl->output_frame.format, 0);
    impl->time_base.den *= impl->output_frame.sample_rate;

    if ((ret = swr_init(impl->context)) < 0)
        goto failed;
    return S_OK;

failed:
    ERR("ret %d\n", ret);
    resampler_cleanup(impl);
    return E_FAIL;
}

static struct resampler *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct resampler, IUnknown_inner);
}

static HRESULT WINAPI unknown_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct resampler *impl = impl_from_IUnknown(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown))
        *out = &impl->IUnknown_inner;
    else if (IsEqualGUID(iid, &IID_IMFTransform))
        *out = &impl->IMFTransform_iface;
    else if (IsEqualGUID(iid, &IID_IMediaObject))
        *out = &impl->IMediaObject_iface;
    else if (IsEqualIID(iid, &IID_IPropertyBag))
        *out = &impl->IPropertyBag_iface;
    else if (IsEqualIID(iid, &IID_IPropertyStore))
        *out = &impl->IPropertyStore_iface;
    else if (IsEqualIID(iid, &IID_IWMResamplerProps))
        *out = &impl->IWMResamplerProps_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI unknown_AddRef(IUnknown *iface)
{
    struct resampler *impl = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&impl->refcount);

    TRACE("iface %p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI unknown_Release(IUnknown *iface)
{
    struct resampler *impl = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&impl->refcount);

    TRACE("iface %p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        MoFreeMediaType(&impl->input_type);
        MoFreeMediaType(&impl->output_type);
        resampler_cleanup(impl);
        free(impl);
    }

    return refcount;
}

static const IUnknownVtbl unknown_vtbl =
{
    unknown_QueryInterface,
    unknown_AddRef,
    unknown_Release,
};

static struct resampler *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct resampler, IMFTransform_iface);
}

static HRESULT WINAPI transform_QueryInterface(IMFTransform *iface, REFIID iid, void **out)
{
    return IUnknown_QueryInterface(impl_from_IMFTransform(iface)->outer, iid, out);
}

static ULONG WINAPI transform_AddRef(IMFTransform *iface)
{
    return IUnknown_AddRef(impl_from_IMFTransform(iface)->outer);
}

static ULONG WINAPI transform_Release(IMFTransform *iface)
{
    return IUnknown_Release(impl_from_IMFTransform(iface)->outer);
}

static HRESULT WINAPI transform_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    TRACE("iface %p, input_minimum %p, input_maximum %p, output_minimum %p, output_maximum %p.\n",
            iface, input_minimum, input_maximum, output_minimum, output_maximum);
    *input_minimum = *input_maximum = *output_minimum = *output_maximum = 1;
    return S_OK;
}

static HRESULT WINAPI transform_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    TRACE("iface %p, inputs %p, outputs %p.\n", iface, inputs, outputs);
    *inputs = *outputs = 1;
    return S_OK;
}

static HRESULT WINAPI transform_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    TRACE("iface %p, input_size %lu, inputs %p, output_size %lu, outputs %p.\n", iface,
            input_size, inputs, output_size, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    struct resampler *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    if (IsEqualGUID(&impl->input_type.majortype, &GUID_NULL) || IsEqualGUID(&impl->output_type.majortype, &GUID_NULL))
    {
        memset(info, 0, sizeof(*info));
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    *info = impl->input_info;
    return S_OK;
}

static HRESULT WINAPI transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct resampler *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    if (IsEqualGUID(&impl->input_type.majortype, &GUID_NULL) || IsEqualGUID(&impl->output_type.majortype, &GUID_NULL))
    {
        memset(info, 0, sizeof(*info));
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    *info = impl->output_info;
    return S_OK;
}

static HRESULT WINAPI transform_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    TRACE("iface %p, attributes %p.\n", iface, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    TRACE("iface %p, id %#lx, attributes %p.\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    TRACE("iface %p, id %#lx, attributes %p.\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    TRACE("iface %p, id %#lx.\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    TRACE("iface %p, streams %lu, ids %p.\n", iface, streams, ids);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    struct resampler *impl = impl_from_IMFTransform(iface);
    DMO_MEDIA_TYPE mt = {0};
    HRESULT hr;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    if (FAILED(hr = MF_RESULT_FROM_DMO(IMediaObject_GetInputType(&impl->IMediaObject_iface, id, index, &mt))))
        return hr;
    hr = MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION, &mt, type);
    MoFreeMediaType(&mt);
    return hr;
}

static HRESULT WINAPI transform_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    struct resampler *impl = impl_from_IMFTransform(iface);
    DMO_MEDIA_TYPE mt = {0};
    HRESULT hr;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    if (FAILED(hr = MF_RESULT_FROM_DMO(IMediaObject_GetOutputType(&impl->IMediaObject_iface, id, index, &mt))))
        return hr;
    hr = MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION, &mt, type);
    MoFreeMediaType(&mt);
    return hr;
}

static HRESULT WINAPI transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    DWORD dmo_flags = flags & MFT_SET_TYPE_TEST_ONLY ? DMO_SET_TYPEF_TEST_ONLY : 0;
    struct resampler *impl = impl_from_IMFTransform(iface);
    DMO_MEDIA_TYPE tmp = {0}, *mt = type ? &tmp : NULL;
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (type && FAILED(hr = MFInitAMMediaTypeFromMFMediaType(type, FORMAT_WaveFormatEx, (AM_MEDIA_TYPE *)&tmp)))
        return hr;
    hr = MF_RESULT_FROM_DMO(IMediaObject_SetInputType(&impl->IMediaObject_iface, id, mt, dmo_flags));
    MoFreeMediaType(&tmp);
    return hr;
}

static HRESULT WINAPI transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    DWORD dmo_flags = flags & MFT_SET_TYPE_TEST_ONLY ? DMO_SET_TYPEF_TEST_ONLY : 0;
    struct resampler *impl = impl_from_IMFTransform(iface);
    DMO_MEDIA_TYPE tmp = {0}, *mt = type ? &tmp : NULL;
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (type && FAILED(hr = MFInitAMMediaTypeFromMFMediaType(type, FORMAT_WaveFormatEx, (AM_MEDIA_TYPE *)&tmp)))
        return hr;
    hr = MF_RESULT_FROM_DMO(IMediaObject_SetOutputType(&impl->IMediaObject_iface, id, mt, dmo_flags));
    MoFreeMediaType(&tmp);
    return hr;
}

static HRESULT WINAPI transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct resampler *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, type %p.\n", iface, id, type);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;
    if (IsEqualGUID(&impl->input_type.majortype, &GUID_NULL))
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    return MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION, &impl->input_type, type);
}

static HRESULT WINAPI transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct resampler *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, type %p.\n", iface, id, type);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;
    if (IsEqualGUID(&impl->output_type.majortype, &GUID_NULL))
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    return MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION, &impl->output_type, type);
}

static HRESULT WINAPI transform_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    FIXME("iface %p, id %#lx, flags %p stub!\n", iface, id, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p stub!\n", iface, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    TRACE("iface %p, lower %I64d, upper %I64d.\n", iface, lower, upper);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    FIXME("iface %p, id %#lx, event %p stub!\n", iface, id, event);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    FIXME("iface %p, message %#x, param %p stub!\n", iface, message, (void *)param);
    return S_OK;
}

static HRESULT WINAPI transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct resampler *impl = impl_from_IMFTransform(iface);
    IMediaBuffer *dmo_buffer;
    IMFMediaBuffer *buffer;
    HRESULT hr;

    TRACE("iface %p, id %#lx, sample %p, flags %#lx.\n", iface, id, sample, flags);

    if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer)))
        return hr;
    if (SUCCEEDED(hr = MFCreateLegacyMediaBufferOnMFMediaBuffer(sample, buffer, 0, &dmo_buffer)))
    {
        hr = MF_RESULT_FROM_DMO(IMediaObject_ProcessInput(&impl->IMediaObject_iface, id, dmo_buffer, 0, 0, 0));
        IMediaBuffer_Release(dmo_buffer);
    }
    IMFMediaBuffer_Release(buffer);

    return hr;
}

static HRESULT WINAPI transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *output, DWORD *output_status)
{
    struct resampler *impl = impl_from_IMFTransform(iface);
    DMO_OUTPUT_DATA_BUFFER dmo_output = {0};
    IMFMediaBuffer *buffer;
    DWORD dmo_status;
    HRESULT hr;

    TRACE("iface %p, flags %#lx, count %lu, output %p, output_status %p.\n", iface, flags, count,
            output, output_status);

    if (count != 1)
        return E_INVALIDARG;

    if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(output->pSample, &buffer)))
        return hr;
    if (SUCCEEDED(hr = MFCreateLegacyMediaBufferOnMFMediaBuffer(output->pSample, buffer, 0,
                          &dmo_output.pBuffer)))
    {
        hr = MF_RESULT_FROM_DMO(IMediaObject_ProcessOutput(&impl->IMediaObject_iface, flags, 1,
                &dmo_output, &dmo_status));
        IMediaBuffer_Release(dmo_output.pBuffer);
    }
    IMFMediaBuffer_Release(buffer);

    output->dwStatus = *output_status = 0;
    if (hr == S_FALSE)
        output->dwStatus = MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE;
    else if (dmo_output.dwStatus & DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE)
        output->dwStatus = MFT_OUTPUT_DATA_BUFFER_INCOMPLETE;
    return hr;
}

static const IMFTransformVtbl transform_vtbl =
{
    transform_QueryInterface,
    transform_AddRef,
    transform_Release,
    transform_GetStreamLimits,
    transform_GetStreamCount,
    transform_GetStreamIDs,
    transform_GetInputStreamInfo,
    transform_GetOutputStreamInfo,
    transform_GetAttributes,
    transform_GetInputStreamAttributes,
    transform_GetOutputStreamAttributes,
    transform_DeleteInputStream,
    transform_AddInputStreams,
    transform_GetInputAvailableType,
    transform_GetOutputAvailableType,
    transform_SetInputType,
    transform_SetOutputType,
    transform_GetInputCurrentType,
    transform_GetOutputCurrentType,
    transform_GetInputStatus,
    transform_GetOutputStatus,
    transform_SetOutputBounds,
    transform_ProcessEvent,
    transform_ProcessMessage,
    transform_ProcessInput,
    transform_ProcessOutput,
};

static struct resampler *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct resampler, IMediaObject_iface);
}

static HRESULT WINAPI media_object_QueryInterface(IMediaObject *iface, REFIID iid, void **obj)
{
    return IUnknown_QueryInterface(impl_from_IMediaObject(iface)->outer, iid, obj);
}

static ULONG WINAPI media_object_AddRef(IMediaObject *iface)
{
    return IUnknown_AddRef(impl_from_IMediaObject(iface)->outer);
}

static ULONG WINAPI media_object_Release(IMediaObject *iface)
{
    return IUnknown_Release(impl_from_IMediaObject(iface)->outer);
}

static HRESULT WINAPI media_object_GetStreamCount(IMediaObject *iface, DWORD *input, DWORD *output)
{
    TRACE("iface %p, input %p, output %p\n", iface, input, output);
    *input = *output = 1;
    return S_OK;
}

static HRESULT WINAPI media_object_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %lu, flags %p stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %lu, flags %p stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT get_available_dmo_media_type(DWORD index, DMO_MEDIA_TYPE *type, BOOL output)
{
    const GUID *subtype;
    WAVEFORMATEX *wfx;

    if (!(wfx = CoTaskMemAlloc(sizeof(*wfx))))
        return E_OUTOFMEMORY;
    subtype = audio_formats[index % ARRAY_SIZE(audio_formats)];

    memset(wfx, 0, sizeof(*wfx));
    wfx->wFormatTag = subtype->Data1;

    memset(type, 0, sizeof(*type));
    type->majortype = MFMediaType_Audio;
    type->formattype = FORMAT_WaveFormatEx;
    type->subtype = *subtype;
    type->pbFormat = (BYTE *)wfx;
    type->cbFormat = sizeof(*wfx);
    if (index < ARRAY_SIZE(audio_formats))
        return S_OK;

    wfx->wBitsPerSample = IsEqualGUID(subtype, &MFAudioFormat_Float) ? 32 : 16;
    wfx->nChannels = 2;
    wfx->nSamplesPerSec = 48000;
    wfx->nBlockAlign = wfx->wBitsPerSample * wfx->nChannels / 8;
    wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;
    return S_OK;
}

static HRESULT WINAPI media_object_GetInputType(IMediaObject *iface, DWORD index, DWORD type_index,
        DMO_MEDIA_TYPE *type)
{
    TRACE("iface %p, index %lu, type_index %lu, type %p\n", impl_from_IMediaObject(iface), index,
            type_index, type);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (type_index >= ARRAY_SIZE(audio_formats))
        return DMO_E_NO_MORE_ITEMS;
    return type ? get_available_dmo_media_type(type_index, type, FALSE) : S_OK;
}

static HRESULT WINAPI media_object_GetOutputType(IMediaObject *iface, DWORD index, DWORD type_index,
        DMO_MEDIA_TYPE *type)
{
    TRACE("iface %p, index %lu, type_index %lu, type %p\n", impl_from_IMediaObject(iface), index,
            type_index, type);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (type_index >= 2 * ARRAY_SIZE(audio_formats))
        return DMO_E_NO_MORE_ITEMS;
    return type ? get_available_dmo_media_type(type_index, type, TRUE) : S_OK;
}

static HRESULT check_dmo_media_type(const DMO_MEDIA_TYPE *type, UINT *block_alignment)
{
    WAVEFORMATEX *wfx;
    ULONG i;

    if (!IsEqualGUID(&type->majortype, &MEDIATYPE_Audio))
        return DMO_E_INVALIDTYPE;
    if (!IsEqualGUID(&type->formattype, &FORMAT_WaveFormatEx) || type->cbFormat < sizeof(*wfx))
        return DMO_E_INVALIDTYPE;

    for (i = 0; i < ARRAY_SIZE(audio_formats); ++i)
        if (IsEqualGUID(&type->subtype, audio_formats[i]))
            break;
    if (i == ARRAY_SIZE(audio_formats))
        return DMO_E_INVALIDTYPE;

    wfx = (WAVEFORMATEX *)type->pbFormat;
    if (!wfx->wBitsPerSample || !wfx->nAvgBytesPerSec || !wfx->nChannels || !wfx->nSamplesPerSec || !wfx->nBlockAlign)
        return DMO_E_INVALIDTYPE;

    *block_alignment = wfx->nBlockAlign;
    return S_OK;
}

static HRESULT WINAPI media_object_SetInputType(IMediaObject *iface, DWORD index,
        const DMO_MEDIA_TYPE *type, DWORD flags)
{
    struct resampler *impl = impl_from_IMediaObject(iface);
    UINT32 block_alignment;
    HRESULT hr;

    TRACE("iface %p, index %#lx, type %p, flags %#lx.\n", iface, index, type, flags);

    if (index != 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (!type)
    {
        clear_dmo_media_type(&impl->input_type);
        resampler_cleanup(impl);
        return S_OK;
    }

    if (FAILED(hr = check_dmo_media_type(type, &block_alignment)))
        return hr;
    if (flags & DMO_SET_TYPEF_TEST_ONLY)
        return S_OK;

    MoFreeMediaType(&impl->input_type);
    if (FAILED(hr = MoCopyMediaType(&impl->input_type, type)))
        return hr;
    impl->input_info.cbSize = block_alignment;

    if (!IsEqualGUID(&impl->output_type.majortype, &GUID_NULL)
            && FAILED(hr = resampler_init(impl)))
    {
        clear_dmo_media_type(&impl->input_type);
        impl->input_info.cbSize = 0;
    }

    return hr;
}

static HRESULT WINAPI media_object_SetOutputType(IMediaObject *iface, DWORD index,
        const DMO_MEDIA_TYPE *type, DWORD flags)
{
    struct resampler *impl = impl_from_IMediaObject(iface);
    UINT32 block_alignment;
    HRESULT hr;

    TRACE("iface %p, index %#lx, type %p, flags %#lx.\n", iface, index, type, flags);

    if (index != 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (!type)
    {
        clear_dmo_media_type(&impl->output_type);
        resampler_cleanup(impl);
        return S_OK;
    }

    if (IsEqualGUID(&impl->input_type.majortype, &GUID_NULL))
        return DMO_E_TYPE_NOT_SET;
    if (FAILED(hr = check_dmo_media_type(type, &block_alignment)))
        return hr;
    if (flags & DMO_SET_TYPEF_TEST_ONLY)
        return S_OK;

    MoFreeMediaType(&impl->output_type);
    if (FAILED(hr = MoCopyMediaType(&impl->output_type, type)))
        return hr;
    impl->output_info.cbSize = block_alignment;

    if (!IsEqualGUID(&impl->input_type.majortype, &GUID_NULL)
            && FAILED(hr = resampler_init(impl)))
    {
        clear_dmo_media_type(&impl->output_type);
        impl->output_info.cbSize = 0;
    }

    return hr;
}

static HRESULT WINAPI media_object_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    struct resampler *impl = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %#lx, type %p.\n", iface, index, type);

    if (index != 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (IsEqualGUID(&impl->input_type.majortype, &GUID_NULL))
        return DMO_E_TYPE_NOT_SET;
    return MoCopyMediaType(type, &impl->input_type);
}

static HRESULT WINAPI media_object_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    struct resampler *impl = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %#lx, type %p.\n", iface, index, type);

    if (index != 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (IsEqualGUID(&impl->output_type.majortype, &GUID_NULL))
        return DMO_E_TYPE_NOT_SET;
    return MoCopyMediaType(type, &impl->output_type);
}

static HRESULT WINAPI media_object_GetInputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size,
        DWORD *lookahead, DWORD *alignment)
{
    FIXME("iface %p, index %lu, size %p, lookahead %p, alignment %p stub!\n", iface, index, size,
            lookahead, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    FIXME("iface %p, index %lu, size %p, alignment %p stub!\n", iface, index, size, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    FIXME("iface %p, index %lu, latency %p stub!\n", iface, index, latency);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    FIXME("iface %p, index %lu, latency %I64d stub!\n", iface, index, latency);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_Flush(IMediaObject *iface)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_Discontinuity(IMediaObject *iface, DWORD index)
{
    FIXME("iface %p, index %lu stub!\n", iface, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_AllocateStreamingResources(IMediaObject *iface)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_FreeStreamingResources(IMediaObject *iface)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %lu, flags %p stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_ProcessInput(IMediaObject *iface, DWORD index,
        IMediaBuffer *buffer, DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME timelength)
{
    DMO_OUTPUT_DATA_BUFFER input = {.pBuffer = buffer, .dwStatus = flags, .rtTimestamp = timestamp,
            .rtTimelength = timelength};
    struct resampler *impl = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %lu, buffer %p, flags %#lx, timestamp %I64d, timelength %I64d\n", iface,
            index, buffer, flags, timestamp, timelength);

    return resampler_process_input(impl, &input);
}

static HRESULT WINAPI media_object_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count,
        DMO_OUTPUT_DATA_BUFFER *output, DWORD *output_status)
{
    struct resampler *impl = impl_from_IMediaObject(iface);

    TRACE("iface %p, flags %#lx, count %lu, output %p, output_status %p\n", iface, flags, count,
            output, output_status);

    if (count != 1)
        return E_INVALIDARG;
    if (flags)
        FIXME("Unimplemented flags %#lx\n", flags);

    return resampler_process_output(impl, output);
}

static HRESULT WINAPI media_object_Lock(IMediaObject *iface, LONG lock)
{
    FIXME("iface %p, lock %ld stub!\n", iface, lock);
    return E_NOTIMPL;
}

static const IMediaObjectVtbl media_object_vtbl =
{
    media_object_QueryInterface,
    media_object_AddRef,
    media_object_Release,
    media_object_GetStreamCount,
    media_object_GetInputStreamInfo,
    media_object_GetOutputStreamInfo,
    media_object_GetInputType,
    media_object_GetOutputType,
    media_object_SetInputType,
    media_object_SetOutputType,
    media_object_GetInputCurrentType,
    media_object_GetOutputCurrentType,
    media_object_GetInputSizeInfo,
    media_object_GetOutputSizeInfo,
    media_object_GetInputMaxLatency,
    media_object_SetInputMaxLatency,
    media_object_Flush,
    media_object_Discontinuity,
    media_object_AllocateStreamingResources,
    media_object_FreeStreamingResources,
    media_object_GetInputStatus,
    media_object_ProcessInput,
    media_object_ProcessOutput,
    media_object_Lock,
};

static struct resampler *impl_from_IPropertyBag(IPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, struct resampler, IPropertyBag_iface);
}

static HRESULT WINAPI property_bag_QueryInterface(IPropertyBag *iface, REFIID iid, void **out)
{
    return IUnknown_QueryInterface(impl_from_IPropertyBag(iface)->outer, iid, out);
}

static ULONG WINAPI property_bag_AddRef(IPropertyBag *iface)
{
    return IUnknown_AddRef(impl_from_IPropertyBag(iface)->outer);
}

static ULONG WINAPI property_bag_Release(IPropertyBag *iface)
{
    return IUnknown_Release(impl_from_IPropertyBag(iface)->outer);
}

static HRESULT WINAPI property_bag_Read(IPropertyBag *iface, const WCHAR *prop_name, VARIANT *value,
        IErrorLog *error_log)
{
    FIXME("iface %p, prop_name %s, value %p, error_log %p stub!\n", iface, debugstr_w(prop_name), value, error_log);
    return E_NOTIMPL;
}

static HRESULT WINAPI property_bag_Write(IPropertyBag *iface, const WCHAR *prop_name, VARIANT *value)
{
    FIXME("iface %p, prop_name %s, value %p stub!\n", iface, debugstr_w(prop_name), value);
    return S_OK;
}

static const IPropertyBagVtbl property_bag_vtbl =
{
    property_bag_QueryInterface,
    property_bag_AddRef,
    property_bag_Release,
    property_bag_Read,
    property_bag_Write,
};

static struct resampler *impl_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, struct resampler, IPropertyStore_iface);
}

static HRESULT WINAPI property_store_QueryInterface(IPropertyStore *iface, REFIID iid, void **out)
{
    return IUnknown_QueryInterface(impl_from_IPropertyStore(iface)->outer, iid, out);
}

static ULONG WINAPI property_store_AddRef(IPropertyStore *iface)
{
    return IUnknown_AddRef(impl_from_IPropertyStore(iface)->outer);
}

static ULONG WINAPI property_store_Release(IPropertyStore *iface)
{
    return IUnknown_Release(impl_from_IPropertyStore(iface)->outer);
}

static HRESULT WINAPI property_store_GetCount(IPropertyStore *iface, DWORD *count)
{
    FIXME("iface %p, count %p stub!\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI property_store_GetAt(IPropertyStore *iface, DWORD index, PROPERTYKEY *key)
{
    FIXME("iface %p, index %lu, key %p stub!\n", iface, index, key);
    return E_NOTIMPL;
}

static HRESULT WINAPI property_store_GetValue(IPropertyStore *iface, REFPROPERTYKEY key, PROPVARIANT *value)
{
    FIXME("iface %p, key %p, value %p stub!\n", iface, key, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI property_store_SetValue(IPropertyStore *iface, REFPROPERTYKEY key, REFPROPVARIANT value)
{
    FIXME("iface %p, key %p, value %p stub!\n", iface, key, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI property_store_Commit(IPropertyStore *iface)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static const IPropertyStoreVtbl property_store_vtbl =
{
    property_store_QueryInterface,
    property_store_AddRef,
    property_store_Release,
    property_store_GetCount,
    property_store_GetAt,
    property_store_GetValue,
    property_store_SetValue,
    property_store_Commit,
};

static struct resampler *impl_from_IWMResamplerProps(IWMResamplerProps *iface)
{
    return CONTAINING_RECORD(iface, struct resampler, IWMResamplerProps_iface);
}

static HRESULT WINAPI resampler_props_QueryInterface(IWMResamplerProps *iface, REFIID iid, void **out)
{
    return IUnknown_QueryInterface(impl_from_IWMResamplerProps(iface)->outer, iid, out);
}

static ULONG WINAPI resampler_props_AddRef(IWMResamplerProps *iface)
{
    return IUnknown_AddRef(impl_from_IWMResamplerProps(iface)->outer);
}

static ULONG WINAPI resampler_props_Release(IWMResamplerProps *iface)
{
    return IUnknown_Release(impl_from_IWMResamplerProps(iface)->outer);
}

static HRESULT WINAPI resampler_props_SetHalfFilterLength(IWMResamplerProps *iface, LONG length)
{
    FIXME("iface %p, count %lu stub!\n", iface, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI resampler_props_SetUserChannelMtx(IWMResamplerProps *iface, ChMtxType *conversion_matrix)
{
    FIXME("iface %p, userChannelMtx %p stub!\n", iface, conversion_matrix);
    return E_NOTIMPL;
}

static const IWMResamplerPropsVtbl resampler_props_vtbl =
{
    resampler_props_QueryInterface,
    resampler_props_AddRef,
    resampler_props_Release,
    resampler_props_SetHalfFilterLength,
    resampler_props_SetUserChannelMtx,
};

static const char *debugstr_version(UINT version)
{
    return wine_dbg_sprintf("%u.%u.%u", AV_VERSION_MAJOR(version), AV_VERSION_MINOR(version),
            AV_VERSION_MICRO(version));
}

HRESULT resampler_create(IUnknown *outer, IUnknown **out)
{
    static const WAVEFORMATEX output_format =
    {
        .wFormatTag = WAVE_FORMAT_IEEE_FLOAT, .wBitsPerSample = 32, .nSamplesPerSec = 44100, .nChannels = 1,
    };
    static const WAVEFORMATEX input_format =
    {
        .wFormatTag = WAVE_FORMAT_PCM, .wBitsPerSample = 16, .nSamplesPerSec = 44100, .nChannels = 1,
    };
    struct resampler *impl;
    HRESULT hr;

    TRACE("outer %p, out %p.\n", outer, out);

    TRACE("avutil version %s\n", debugstr_version(avutil_version()));
    TRACE("swresample version %s\n", debugstr_version(swresample_version()));

    if (FAILED(hr = check_audio_transform_support(&input_format, &output_format)))
    {
        ERR_(winediag)("GStreamer doesn't support audio resampling, please install appropriate plugins.\n");
        return hr;
    }

    if (!(impl = calloc(1, sizeof(*impl))))
        return E_OUTOFMEMORY;
    impl->IUnknown_inner.lpVtbl = &unknown_vtbl;
    impl->IMFTransform_iface.lpVtbl = &transform_vtbl;
    impl->IMediaObject_iface.lpVtbl = &media_object_vtbl;
    impl->IPropertyBag_iface.lpVtbl = &property_bag_vtbl;
    impl->IPropertyStore_iface.lpVtbl = &property_store_vtbl;
    impl->IWMResamplerProps_iface.lpVtbl = &resampler_props_vtbl;
    impl->refcount = 1;
    impl->outer = outer ? outer : &impl->IUnknown_inner;

    impl->input_info.cbAlignment = 1;
    impl->output_info.cbAlignment = 1;

    *out = &impl->IUnknown_inner;
    TRACE("Created resampler %p\n", *out);
    return S_OK;
}
