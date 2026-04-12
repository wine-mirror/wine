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

#include "gst_private.h"

#include "d3d9types.h"
#include "mediaerr.h"
#include "mfapi.h"
#include "mferror.h"
#include "mfobjects.h"
#include "mftransform.h"
#include "wmcodecdsp.h"

#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmo);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

extern GUID MFVideoFormat_ABGR32;

static const GUID MF_XVP_PLAYBACK_MODE = { 0x3c5d293f, 0xad67, 0x4e29, { 0xaf, 0x12, 0xcf, 0x3e, 0x23, 0x8a, 0xcc, 0xe9 } };

static const AVRational USER_TIME_BASE_Q = {1, 10000000};

static const char *debugstr_avtime(INT64 time, AVRational time_base)
{
    time = av_rescale_q_rnd(time, time_base, USER_TIME_BASE_Q, AV_ROUND_PASS_MINMAX);
    if (time == AV_NOPTS_VALUE)
        return "(none)";
    return wine_dbg_sprintf("%I64d", time);
}

static const char *debugstr_avframe(AVFrame *frame)
{
    return wine_dbg_sprintf("fmt %s %dx%d stride %d", av_get_pix_fmt_name(frame->format),
            frame->width, frame->height, frame->linesize[0]);
}

static enum AVPixelFormat pixel_format_from_video_subtype(const GUID *subtype)
{
    switch (subtype->Data1)
    {
    /* MFVideoFormat_420O */   case MAKEFOURCC('4','2','0','O'): break;
    /* MFVideoFormat_AYUV */   case MAKEFOURCC('A','Y','U','V'): return AV_PIX_FMT_AYUV;
    /* MFVideoFormat_I420 */   case MAKEFOURCC('I','4','2','0'): return AV_PIX_FMT_YUV420P;
    /* MFVideoFormat_IYUV */   case MAKEFOURCC('I','Y','U','V'): return AV_PIX_FMT_YUV420P;
    /* MFVideoFormat_NV11 */   case MAKEFOURCC('N','V','1','1'): return AV_PIX_FMT_YUV411P;
    /* MFVideoFormat_NV12 */   case MAKEFOURCC('N','V','1','2'): return AV_PIX_FMT_NV12;
    /* MFVideoFormat_P208 */   case MAKEFOURCC('P','2','0','8'): break;
    /* MFVideoFormat_UYVY */   case MAKEFOURCC('U','Y','V','Y'): return AV_PIX_FMT_UYVY422;
    /* MFVideoFormat_v410 */   case MAKEFOURCC('v','4','1','0'): break;
    /* MFVideoFormat_Y216 */   case MAKEFOURCC('Y','2','1','6'): return AV_PIX_FMT_Y216;
    /* MFVideoFormat_Y41P */   case MAKEFOURCC('Y','4','1','P'): break;
    /* MFVideoFormat_Y41T */   case MAKEFOURCC('Y','4','1','T'): break;
    /* MFVideoFormat_Y42T */   case MAKEFOURCC('Y','4','2','T'): break;
    /* MFVideoFormat_YUY2 */   case MAKEFOURCC('Y','U','Y','2'): return AV_PIX_FMT_YUYV422;
    /* MFVideoFormat_YV12 */   case MAKEFOURCC('Y','V','1','2'): return AV_PIX_FMT_YUV420P;
    /* MFVideoFormat_YVYU */   case MAKEFOURCC('Y','V','Y','U'): return AV_PIX_FMT_YVYU422;
    /* MFVideoFormat_ABGR32 */ case D3DFMT_A8B8G8R8:             return AV_PIX_FMT_RGBA;
    /* MFVideoFormat_ARGB32 */ case D3DFMT_A8R8G8B8:             return AV_PIX_FMT_BGRA;
    /* MFVideoFormat_RGB24 */  case D3DFMT_R8G8B8:               return AV_PIX_FMT_BGR24;
    /* MFVideoFormat_RGB32 */  case D3DFMT_X8R8G8B8:             return AV_PIX_FMT_BGR0;
    /* MFVideoFormat_RGB555 */ case D3DFMT_X1R5G5B5:             return AV_PIX_FMT_RGB555;
    /* MFVideoFormat_RGB565 */ case D3DFMT_R5G6B5:               return AV_PIX_FMT_RGB565;
    /* MFVideoFormat_RGB8 */   case D3DFMT_P8:                   return AV_PIX_FMT_RGB8;
    }

    FIXME("Unsupported subtype %s (%s)\n", debugstr_guid(subtype), debugstr_fourcc(subtype->Data1));
    return AV_PIX_FMT_NONE;
}

static void video_frame_init_aperture(AVFrame *frame, const MFVIDEOFORMAT *format)
{
    if (format->videoInfo.MinimumDisplayAperture.OffsetX.value
            || format->videoInfo.MinimumDisplayAperture.OffsetY.value
            || format->videoInfo.MinimumDisplayAperture.Area.cx
            || format->videoInfo.MinimumDisplayAperture.Area.cy)
    {
        frame->width = format->videoInfo.MinimumDisplayAperture.OffsetX.value +
                format->videoInfo.MinimumDisplayAperture.Area.cx;
        frame->height = format->videoInfo.MinimumDisplayAperture.OffsetY.value +
                format->videoInfo.MinimumDisplayAperture.Area.cy;
    }
    else
    {
        frame->width = format->videoInfo.dwWidth;
        frame->height = format->videoInfo.dwHeight;
    }
}

static void media_buffer_release(void *opaque, uint8_t *data)
{
    IMediaBuffer *media_buffer = opaque;
    IMF2DBuffer2 *buffer2;

    if (SUCCEEDED(IMediaBuffer_QueryInterface(media_buffer, &IID_IMF2DBuffer2, (void **)&buffer2)))
    {
        IMF2DBuffer2_Unlock2D(buffer2);
        IMF2DBuffer2_Release(buffer2);
    }
    IMediaBuffer_Release(media_buffer);
}

static AVBufferRef *buffer_from_media_buffer(IMediaBuffer *media_buffer, int flags, LONG *pitch)
{
    BYTE *data, *scanline0;
    IMF2DBuffer2 *buffer2;
    AVBufferRef *buffer;
    HRESULT hr;
    DWORD size;

    if (SUCCEEDED(hr = IMediaBuffer_QueryInterface(media_buffer, &IID_IMF2DBuffer2, (void **)&buffer2)))
    {
        MF2DBuffer_LockFlags lock_flags = (flags & AV_BUFFER_FLAG_READONLY) ? MF2DBuffer_LockFlags_Read
                : MF2DBuffer_LockFlags_Write;
        hr = IMF2DBuffer2_Lock2DSize(buffer2, lock_flags, &scanline0, pitch, &data, &size);
        IMF2DBuffer2_Release(buffer2);
    }
    if (FAILED(hr))
    {
        if (FAILED(hr = IMediaBuffer_GetBufferAndLength(media_buffer, &data, &size)))
            return NULL;
        if (!flags && FAILED(hr = IMediaBuffer_GetMaxLength(media_buffer, &size)))
            return NULL;
        *pitch = 0;
    }

    if (!(buffer = av_buffer_create(data, size, media_buffer_release, media_buffer, flags)))
        return NULL;
    IMediaBuffer_AddRef(media_buffer);
    return buffer;
}

static void video_frame_init_from_format(AVFrame *frame, const MFVIDEOFORMAT *format)
{
    frame->format = pixel_format_from_video_subtype(&format->guidFormat);
    frame->width = format->videoInfo.dwWidth;
    frame->height = format->videoInfo.dwHeight;
    frame->color_range = AVCOL_RANGE_UNSPECIFIED;
    frame->color_primaries = AVCOL_PRI_UNSPECIFIED;
    frame->color_trc = AVCOL_TRC_UNSPECIFIED;
    frame->colorspace = AVCOL_SPC_UNSPECIFIED;
    frame->chroma_location = AVCHROMA_LOC_UNSPECIFIED;
}

static int fill_arrays_with_format(uint8_t *planes[4], int strides[4], AVBufferRef *buffer,
        const MFVIDEOFORMAT *format, LONG pitch)
{
    enum AVPixelFormat pix_fmt = pixel_format_from_video_subtype(&format->guidFormat);
    UINT width = format->videoInfo.dwWidth, height = format->videoInfo.dwHeight;
    int size;

    size = av_image_fill_arrays(planes, strides, buffer->data, pix_fmt, width, height, 1);
    TRACE("buffer %p / %#Ix size %d pitch %ld, planes %p,%p,%p strides %d,%d,%d\n", buffer->data, buffer->size,
            size, pitch, planes[0], planes[1], planes[2], strides[0], strides[1], strides[2]);
    if (size > buffer->size)
    {
        ERR("stride %ld linesize %u,%u,%u,%u size %#x buffer %#Ix\n", pitch, strides[0], strides[1],
                strides[2], strides[3], size, buffer->size);
        return -1;
    }

    if (pitch && abs(pitch) != strides[0])
    {
        int new_strides[4] = {0};
        for (int i = 0; i < 4; i++) new_strides[i] = (abs(pitch) * strides[i]) / strides[0];
        for (int i = 1; i < 4 && planes[i]; i++)
        {
            size_t height = (planes[i] - planes[i - 1]) / strides[i - 1];
            planes[i] = planes[i - 1] + height * new_strides[i - 1];
        }
        memcpy(strides, new_strides, sizeof(new_strides));
    }

    if ((format->videoInfo.VideoFlags & MFVideoFlag_BottomUpLinearRep) && pitch <= 0)
    {
        planes[0] = planes[0] + strides[0] * (format->videoInfo.dwHeight - 1);
        strides[0] = -strides[0];
    }

    TRACE("buffer %p / %#Ix size %d stride %ld, planes %p,%p,%p strides %d,%d,%d\n", buffer->data, buffer->size,
            size, pitch, planes[0], planes[1], planes[2], strides[0], strides[1], strides[2]);
    return size;
}

static BOOL check_arrays_alignment(uint8_t *planes[4], int strides[4], int size, int align)
{
    if (size % align)
        return FALSE;
    for (int i = 0; i < 4; i++)
        if ((UINT_PTR)planes[i] % align)
            return FALSE;
    for (int i = 0; i < 4; i++)
        if (strides[i] % align)
            return FALSE;
    return TRUE;
}

static BOOL video_frame_wrap_buffer(AVFrame *frame, const MFVIDEOFORMAT *format, AVBufferRef *buffer, LONG pitch)
{
    int size;

    TRACE("frame %p, info %p, buffer %p\n", frame, format, buffer);

    video_frame_init_from_format(frame, format);
    video_frame_init_aperture(frame, format);

    if ((size = fill_arrays_with_format(frame->data, frame->linesize, buffer, format, pitch)) < 0)
        return size;
    /* swscale requires 16byte alignment for data pointers when SSE2 optimizations are used. */
    if (!check_arrays_alignment(frame->data, frame->linesize, size, 16))
    {
        ERR("frame %s isn't aligned to 16 bytes\n", debugstr_avframe(frame));
        return -1;
    }

    frame->opaque = (void *)-1;
    frame->buf[0] = av_buffer_ref(buffer);
    frame->extended_data = frame->data;
    return size;
}

static int video_frame_copy_from_buffer(AVFrame *frame, const MFVIDEOFORMAT *format, AVBufferRef *buffer,
        LONG pitch)
{
    int size, input_strides[4];
    UINT8 *input_planes[4];

    ERR("frame %s, format %p, buffer %p\n", debugstr_avframe(frame), format, buffer);

    size = fill_arrays_with_format(input_planes, input_strides, buffer, format, pitch);
    av_image_copy(frame->data, frame->linesize, (const UINT8 **)input_planes, input_strides,
            frame->format, format->videoInfo.dwWidth, format->videoInfo.dwHeight);
    return size;
}

static int video_frame_copy_to_buffer(AVFrame *frame, const MFVIDEOFORMAT *format, AVBufferRef *buffer,
        LONG pitch)
{
    int size, output_strides[4];
    UINT8 *output_planes[4];

    ERR("frame %s, format %p, buffer %p\n", debugstr_avframe(frame), format, buffer);

    size = fill_arrays_with_format(output_planes, output_strides, buffer, format, pitch);
    av_image_copy(output_planes, output_strides, (const UINT8 **)frame->data, frame->linesize,
            frame->format, format->videoInfo.dwWidth, format->videoInfo.dwHeight);
    return size;
}

static const GUID *const input_types[] =
{
    &MFVideoFormat_IYUV,
    &MFVideoFormat_YV12,
    &MFVideoFormat_NV12,
    &MFVideoFormat_420O,
    &MFVideoFormat_UYVY,
    &MFVideoFormat_YUY2,
    &MEDIASUBTYPE_P208,
    &MFVideoFormat_NV11,
    &MFVideoFormat_AYUV,
    &MFVideoFormat_ARGB32,
    &MFVideoFormat_RGB32,
    &MFVideoFormat_RGB24,
    &MFVideoFormat_I420,
    &MFVideoFormat_YVYU,
    &MFVideoFormat_RGB555,
    &MFVideoFormat_RGB565,
    &MFVideoFormat_RGB8,
    &MFVideoFormat_Y216,
    &MFVideoFormat_v410,
    &MFVideoFormat_Y41P,
    &MFVideoFormat_Y41T,
    &MFVideoFormat_Y42T,
};
static const GUID *const output_types[] =
{
    &MFVideoFormat_YUY2,
    &MFVideoFormat_IYUV,
    &MFVideoFormat_I420,
    &MFVideoFormat_NV12,
    &MFVideoFormat_RGB24,
    &MFVideoFormat_ARGB32,
    &MFVideoFormat_RGB32,
    &MFVideoFormat_YV12,
    &MFVideoFormat_AYUV,
    &MFVideoFormat_RGB555,
    &MFVideoFormat_RGB565,
    &MFVideoFormat_ABGR32,
};

struct video_processor
{
    IMFTransform IMFTransform_iface;
    LONG refcount;

    IMFAttributes *attributes;
    IMFAttributes *output_attributes;

    IMFMediaType *input_type;
    MFVIDEOFORMAT *input_format;
    MFT_INPUT_STREAM_INFO input_info;
    IMFMediaType *output_type;
    MFVIDEOFORMAT *output_format;
    MFT_OUTPUT_STREAM_INFO output_info;

    SwsContext *context;
    AVFrame input_frame;
    AVFrame output_frame;
    AVFrame current_frame;

    IUnknown *device_manager;
    IMFVideoSampleAllocatorEx *allocator;
};

static HRESULT video_processor_process_frame(struct video_processor *impl, AVFrame *input_frame,
        DMO_OUTPUT_DATA_BUFFER *output)
{
    AVFrame output_frame = {0};
    AVBufferRef *buffer;
    IMFSample *sample;
    int size, ret;
    LONG pitch;
    HRESULT hr;

    if (!(buffer = buffer_from_media_buffer(output->pBuffer, 0, &pitch)))
        return E_OUTOFMEMORY;
    if ((size = video_frame_wrap_buffer(&output_frame, impl->output_format, buffer, pitch)) < 0)
        av_frame_move_ref(&output_frame, &impl->output_frame);

    TRACE("input %s output %s\n", debugstr_avframe(input_frame), debugstr_avframe(&output_frame));

    if ((ret = sws_scale_frame(impl->context, &output_frame, input_frame)) < 0)
        ERR("error ret %d (%s)\n", -ret, av_err2str(ret));
    else if ((ret = size) < 0)
        ret = video_frame_copy_to_buffer(&output_frame, impl->output_format, buffer, pitch);

    av_buffer_unref(&buffer);

    if (!output_frame.opaque)
        av_frame_move_ref(&impl->output_frame, &output_frame);
    else
        av_frame_unref(&output_frame);

    output->dwStatus = 0;
    IMediaBuffer_SetLength(output->pBuffer, ret >= 0 ? ret : 0);
    if (ret < 0)
        return E_FAIL;
    if (!ret)
        return MF_E_TRANSFORM_NEED_MORE_INPUT;

    if (input_frame->flags & AV_FRAME_FLAG_KEY)
        output->dwStatus |= DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT;
    output->rtTimestamp = input_frame->pts;
    if (output->rtTimestamp != INT64_MIN)
        output->dwStatus |= DMO_OUTPUT_DATA_BUFFERF_TIME;
    output->rtTimelength = input_frame->duration;
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

static HRESULT video_processor_process_input(struct video_processor *impl, const DMO_OUTPUT_DATA_BUFFER *input)
{
    LONGLONG pts = input->rtTimestamp, duration = input->rtTimelength;
    AVBufferRef *buffer;
    IMFSample *sample;
    HRESULT hr;
    LONG pitch;
    int ret;

    if (!impl->context)
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    if (impl->current_frame.width)
        return MF_E_NOTACCEPTING;

    if (!(buffer = buffer_from_media_buffer(input->pBuffer, AV_BUFFER_FLAG_READONLY, &pitch)))
        return E_OUTOFMEMORY;
    if ((ret = video_frame_wrap_buffer(&impl->current_frame, impl->input_format, buffer, pitch)) < 0)
    {
        ret = video_frame_copy_from_buffer(&impl->input_frame, impl->input_format, buffer, pitch);
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

    impl->current_frame.pts = pts;
    impl->current_frame.duration = duration;
    TRACE("input frame %s, time %s, duration %s\n", debugstr_avframe(&impl->current_frame),
            debugstr_avtime(impl->current_frame.pts, USER_TIME_BASE_Q),
            debugstr_avtime(impl->current_frame.duration, USER_TIME_BASE_Q));
    return S_OK;
}

static HRESULT video_processor_process_output(struct video_processor *impl, DMO_OUTPUT_DATA_BUFFER *output)
{
    HRESULT hr;

    if (!impl->context)
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    if (!impl->current_frame.width)
        return MF_E_TRANSFORM_NEED_MORE_INPUT;

    hr = video_processor_process_frame(impl, &impl->current_frame, output);
    if (!impl->current_frame.opaque)
        av_frame_move_ref(&impl->input_frame, &impl->current_frame);
    else
        av_frame_unref(&impl->current_frame);

    return hr;
}

static void video_processor_cleanup(struct video_processor *impl)
{
    CoTaskMemFree(impl->output_format);
    impl->output_format = NULL;
    CoTaskMemFree(impl->input_format);
    impl->input_format = NULL;
    av_frame_unref(&impl->current_frame);
    av_frame_unref(&impl->output_frame);
    av_frame_unref(&impl->input_frame);
    sws_free_context(&impl->context);
}

static void update_video_aperture(MFVideoInfo *input_info, MFVideoInfo *output_info)
{
    RECT input_rect, output_rect;

    get_mf_video_content_rect(input_info, &input_rect);
    get_mf_video_content_rect(output_info, &output_rect);

    if (!EqualRect(&input_rect, &output_rect))
    {
        FIXME("Mismatched content size %s vs %s\n", wine_dbgstr_rect(&input_rect),
                wine_dbgstr_rect(&output_rect));
    }

    input_info->MinimumDisplayAperture.OffsetX.value = input_rect.left;
    input_info->MinimumDisplayAperture.OffsetY.value = input_rect.top;
    input_info->MinimumDisplayAperture.Area.cx = input_rect.right - input_rect.left;
    input_info->MinimumDisplayAperture.Area.cy = input_rect.bottom - input_rect.top;
    output_info->MinimumDisplayAperture = input_info->MinimumDisplayAperture;
}

static HRESULT create_video_format(BOOL bottom_up, IMFMediaType *media_type, MFVIDEOFORMAT **format)
{
    LONG stride;
    UINT32 size;
    HRESULT hr;

    if (FAILED(hr = MFCreateMFVideoFormatFromMFMediaType(media_type, format, &size)))
        return hr;
    if (bottom_up && FAILED(IMFMediaType_GetItem(media_type, &MF_MT_DEFAULT_STRIDE, NULL))
            && SUCCEEDED(MFGetStrideForBitmapInfoHeader((*format)->guidFormat.Data1, 1, &stride)) && stride < 0)
        (*format)->videoInfo.VideoFlags |= MFVideoFlag_BottomUpLinearRep;
    return hr;
}

static HRESULT video_processor_init(struct video_processor *impl)
{
    BOOL bottom_up = !impl->device_manager; /* when not D3D-enabled, the transform outputs bottom up RGB buffers */
    HRESULT hr;
    int ret;

    video_processor_cleanup(impl);

    if (!(impl->context = sws_alloc_context()))
        return E_OUTOFMEMORY;
    if (FAILED(hr = create_video_format(bottom_up, impl->input_type, &impl->input_format))
            || FAILED(hr = create_video_format(bottom_up, impl->output_type, &impl->output_format)))
    {
        video_processor_cleanup(impl);
        return hr;
    }

    update_video_aperture(&impl->input_format->videoInfo, &impl->output_format->videoInfo);

    video_frame_init_from_format(&impl->input_frame, impl->input_format);
    if ((ret = av_frame_get_buffer(&impl->input_frame, 0)) < 0)
        goto failed;
    video_frame_init_aperture(&impl->input_frame, impl->input_format);

    video_frame_init_from_format(&impl->output_frame, impl->output_format);
    if ((ret = av_frame_get_buffer(&impl->output_frame, 0)) < 0)
        goto failed;
    video_frame_init_aperture(&impl->output_frame, impl->input_format);

    av_opt_set(impl->context, "sws_flags", "neighbor", 0);
    av_opt_set_int(impl->context, "threads", 0, 0);
    av_opt_set_int(impl->context, "srcw", impl->input_frame.width, 0);
    av_opt_set_int(impl->context, "srch", impl->input_frame.height, 0);
    av_opt_set_pixel_fmt(impl->context, "src_format", impl->input_frame.format, 0);
    av_opt_set_int(impl->context, "dstw", impl->output_frame.width, 0);
    av_opt_set_int(impl->context, "dsth", impl->output_frame.height, 0);
    av_opt_set_pixel_fmt(impl->context, "dst_format", impl->output_frame.format, 0);

    if ((ret = sws_init_context(impl->context, NULL, NULL)) < 0)
        goto failed;

    TRACE("input %s -> output %s\n", debugstr_avframe(&impl->input_frame), debugstr_avframe(&impl->output_frame));
    return S_OK;

failed:
    ERR("ret %d\n", ret);
    video_processor_cleanup(impl);
    return E_FAIL;
}

static HRESULT video_processor_init_allocator(struct video_processor *processor)
{
    IMFVideoSampleAllocatorEx *allocator;
    UINT32 count;
    HRESULT hr;

    if (processor->allocator)
        return S_OK;

    if (FAILED(hr = MFCreateVideoSampleAllocatorEx(&IID_IMFVideoSampleAllocatorEx, (void **)&allocator)))
        return hr;
    if (FAILED(IMFAttributes_GetUINT32(processor->attributes, &MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT, &count)))
        count = 2;
    if (FAILED(hr = IMFVideoSampleAllocatorEx_SetDirectXManager(allocator, processor->device_manager))
            || FAILED(hr = IMFVideoSampleAllocatorEx_InitializeSampleAllocatorEx(allocator, count, max(count + 2, 10),
            processor->output_attributes, processor->output_type)))
    {
        IMFVideoSampleAllocatorEx_Release(allocator);
        return hr;
    }

    processor->allocator = allocator;
    return S_OK;
}

static HRESULT video_processor_uninit_allocator(struct video_processor *processor)
{
    HRESULT hr;

    if (!processor->allocator)
        return S_OK;

    if (SUCCEEDED(hr = IMFVideoSampleAllocatorEx_UninitializeSampleAllocator(processor->allocator)))
        hr = IMFVideoSampleAllocatorEx_SetDirectXManager(processor->allocator, NULL);
    IMFVideoSampleAllocatorEx_Release(processor->allocator);
    processor->allocator = NULL;

    return hr;
}

static struct video_processor *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct video_processor, IMFTransform_iface);
}

static HRESULT WINAPI video_processor_QueryInterface(IMFTransform *iface, REFIID iid, void **out)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IMFTransform))
        *out = &impl->IMFTransform_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI video_processor_AddRef(IMFTransform *iface)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&impl->refcount);

    TRACE("iface %p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI video_processor_Release(IMFTransform *iface)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedDecrement(&impl->refcount);

    TRACE("iface %p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        video_processor_uninit_allocator(impl);
        if (impl->device_manager)
            IUnknown_Release(impl->device_manager);
        if (impl->input_type)
            IMFMediaType_Release(impl->input_type);
        if (impl->output_type)
            IMFMediaType_Release(impl->output_type);
        if (impl->attributes)
            IMFAttributes_Release(impl->attributes);
        if (impl->output_attributes)
            IMFAttributes_Release(impl->output_attributes);
        video_processor_cleanup(impl);
        free(impl);
    }

    return refcount;
}

static HRESULT WINAPI video_processor_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    TRACE("iface %p, input_minimum %p, input_maximum %p, output_minimum %p, output_maximum %p.\n",
            iface, input_minimum, input_maximum, output_minimum, output_maximum);
    *input_minimum = *input_maximum = *output_minimum = *output_maximum = 1;
    return S_OK;
}

static HRESULT WINAPI video_processor_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    TRACE("iface %p, inputs %p, outputs %p.\n", iface, inputs, outputs);
    *inputs = *outputs = 1;
    return S_OK;
}

static HRESULT WINAPI video_processor_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    TRACE("iface %p, input_size %lu, inputs %p, output_size %lu, outputs %p.\n", iface,
            input_size, inputs, output_size, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    *info = impl->input_info;
    return S_OK;
}

static HRESULT WINAPI video_processor_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    *info = impl->output_info;
    return S_OK;
}

static HRESULT WINAPI video_processor_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, attributes %p\n", iface, attributes);

    if (!attributes)
        return E_POINTER;

    IMFAttributes_AddRef((*attributes = impl->attributes));
    return S_OK;
}

static HRESULT WINAPI video_processor_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    TRACE("iface %p, id %#lx, attributes %p.\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, attributes %p\n", iface, id, attributes);

    if (!attributes)
        return E_POINTER;
    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    IMFAttributes_AddRef((*attributes = impl->output_attributes));
    return S_OK;
}

static HRESULT WINAPI video_processor_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    TRACE("iface %p, id %#lx.\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    TRACE("iface %p, streams %lu, ids %p.\n", iface, streams, ids);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    IMFMediaType *media_type;
    const GUID *subtype;
    HRESULT hr;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    *type = NULL;

    if (index >= ARRAY_SIZE(input_types))
        return MF_E_NO_MORE_TYPES;
    subtype = input_types[index];

    if (FAILED(hr = MFCreateMediaType(&media_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, subtype)))
        goto done;

    IMFMediaType_AddRef((*type = media_type));

done:
    IMFMediaType_Release(media_type);
    return hr;
}

static HRESULT WINAPI video_processor_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    IMFMediaType *media_type;
    UINT64 frame_size;
    GUID subtype;
    HRESULT hr;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    *type = NULL;

    if (!impl->input_type)
        return MF_E_NO_MORE_TYPES;

    if (FAILED(hr = IMFMediaType_GetGUID(impl->input_type, &MF_MT_SUBTYPE, &subtype))
            || FAILED(hr = IMFMediaType_GetUINT64(impl->input_type, &MF_MT_FRAME_SIZE, &frame_size)))
        return hr;

    if (index > ARRAY_SIZE(output_types))
        return MF_E_NO_MORE_TYPES;
    if (index > 0)
        subtype = *output_types[index - 1];

    if (FAILED(hr = MFCreateMediaType(&media_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &subtype)))
        goto done;

    IMFMediaType_AddRef((*type = media_type));

done:
    IMFMediaType_Release(media_type);
    return hr;
}

static HRESULT WINAPI video_processor_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    GUID major, subtype;
    UINT64 frame_size;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (!type)
    {
        if (impl->input_type)
        {
            IMFMediaType_Release(impl->input_type);
            impl->input_type = NULL;
        }
        video_processor_cleanup(impl);
        return S_OK;
    }

    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major))
            || !IsEqualGUID(&major, &MFMediaType_Video))
        return E_INVALIDARG;
    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(hr = IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)))
        return hr;

    for (i = 0; i < ARRAY_SIZE(input_types); ++i)
        if (IsEqualGUID(&subtype, input_types[i]))
            break;
    if (i == ARRAY_SIZE(input_types))
        return MF_E_INVALIDMEDIATYPE;
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (impl->input_type)
        IMFMediaType_Release(impl->input_type);
    IMFMediaType_AddRef((impl->input_type = type));

    if (impl->output_type && FAILED(hr = video_processor_init(impl)))
    {
        IMFMediaType_Release(impl->input_type);
        impl->input_type = NULL;
    }

    if (FAILED(hr) || FAILED(MFCalculateImageSize(&subtype, frame_size >> 32, (UINT32)frame_size,
            (UINT32 *)&impl->input_info.cbSize)))
        impl->input_info.cbSize = 0;
    return hr;
}

static HRESULT WINAPI video_processor_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    GUID major, subtype;
    UINT64 frame_size;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (!type)
    {
        if (impl->output_type)
        {
            IMFMediaType_Release(impl->output_type);
            impl->output_type = NULL;
        }
        video_processor_cleanup(impl);
        return S_OK;
    }

    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major))
            || !IsEqualGUID(&major, &MFMediaType_Video))
        return E_INVALIDARG;
    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(hr = IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)))
        return hr;

    for (i = 0; i < ARRAY_SIZE(output_types); ++i)
        if (IsEqualGUID(&subtype, output_types[i]))
            break;
    if (i == ARRAY_SIZE(output_types))
        return MF_E_INVALIDMEDIATYPE;
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (FAILED(hr = video_processor_uninit_allocator(impl)))
        return hr;

    if (impl->output_type)
        IMFMediaType_Release(impl->output_type);
    IMFMediaType_AddRef((impl->output_type = type));

    if (impl->input_type && FAILED(hr = video_processor_init(impl)))
    {
        IMFMediaType_Release(impl->output_type);
        impl->output_type = NULL;
    }

    if (FAILED(hr) || FAILED(MFCalculateImageSize(&subtype, frame_size >> 32, (UINT32)frame_size,
            (UINT32 *)&impl->output_info.cbSize)))
        impl->output_info.cbSize = 0;
    return hr;
}

static HRESULT WINAPI video_processor_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p.\n", iface, id, type);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (!impl->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(type)))
        return hr;

    if (FAILED(hr = IMFMediaType_CopyAllItems(impl->input_type, (IMFAttributes *)*type)))
        IMFMediaType_Release(*type);

    return hr;
}

static HRESULT WINAPI video_processor_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p.\n", iface, id, type);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (!impl->output_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(type)))
        return hr;

    if (FAILED(hr = IMFMediaType_CopyAllItems(impl->output_type, (IMFAttributes *)*type)))
        IMFMediaType_Release(*type);

    return hr;
}

static HRESULT WINAPI video_processor_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);

    FIXME("iface %p, id %#lx, flags %p stub!\n", iface, id, flags);

    if (!impl->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *flags = MFT_INPUT_STATUS_ACCEPT_DATA;
    return S_OK;
}

static HRESULT WINAPI video_processor_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);

    FIXME("iface %p, flags %p stub!\n", iface, flags);

    if (!impl->output_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *flags = MFT_OUTPUT_STATUS_SAMPLE_READY;
    return S_OK;
}

static HRESULT WINAPI video_processor_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    TRACE("iface %p, lower %I64d, upper %I64d.\n", iface, lower, upper);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    FIXME("iface %p, id %#lx, event %p stub!\n", iface, id, event);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    struct video_processor *processor = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, message %#x, param %Ix.\n", iface, message, param);

    switch (message)
    {
    case MFT_MESSAGE_SET_D3D_MANAGER:
        if (FAILED(hr = video_processor_uninit_allocator(processor)))
            return hr;

        if (processor->device_manager)
        {
            processor->output_info.dwFlags &= ~MFT_OUTPUT_STREAM_PROVIDES_SAMPLES;
            IUnknown_Release(processor->device_manager);
        }
        if ((processor->device_manager = (IUnknown *)param))
        {
            IUnknown_AddRef(processor->device_manager);
            processor->output_info.dwFlags |= MFT_OUTPUT_STREAM_PROVIDES_SAMPLES;
        }
        return S_OK;

    default:
        FIXME("Ignoring message %#x.\n", message);
        return S_OK;
    }
}

static HRESULT WINAPI video_processor_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    DMO_OUTPUT_DATA_BUFFER input = {.dwStatus = flags};
    IMFMediaBuffer *buffer;
    HRESULT hr;

    TRACE("iface %p, id %#lx, sample %p, flags %#lx.\n", iface, id, sample, flags);

    if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer)))
        return hr;
    if (SUCCEEDED(hr = MFCreateLegacyMediaBufferOnMFMediaBuffer(sample, buffer, 0, &input.pBuffer)))
    {
        hr = video_processor_process_input(impl, &input);
        IMediaBuffer_Release(input.pBuffer);
    }
    IMFMediaBuffer_Release(buffer);

    return hr;
}

static HRESULT WINAPI video_processor_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *output, DWORD *output_status)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    DMO_OUTPUT_DATA_BUFFER dmo_output = {0};
    BOOL playback_mode, provide_samples;
    IMFSample *output_sample;
    IMFMediaBuffer *buffer;
    HRESULT hr;

    TRACE("iface %p, flags %#lx, count %lu, output %p, output_status %p.\n", iface, flags, count,
            output, output_status);

    if (count != 1)
        return E_INVALIDARG;
    if (!impl->input_format || !impl->output_format)
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    if (!impl->current_frame.width)
        return MF_E_TRANSFORM_NEED_MORE_INPUT;

    output->dwStatus = 0;

    if (FAILED(IMFAttributes_GetUINT32(impl->attributes, &MF_XVP_PLAYBACK_MODE, (UINT32 *) &playback_mode)))
        playback_mode = FALSE;

    provide_samples = (impl->output_info.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES) && !playback_mode;

    if (provide_samples)
    {
        if (FAILED(hr = video_processor_init_allocator(impl)))
            return hr;
        if (FAILED(hr = IMFVideoSampleAllocatorEx_AllocateSample(impl->allocator, &output_sample)))
            return hr;
    }
    else
    {
        if (!(output_sample = output->pSample))
            return E_INVALIDARG;
        IMFSample_AddRef(output_sample);
    }

    if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(output_sample, &buffer)))
        goto done;
    if (SUCCEEDED(hr = MFCreateLegacyMediaBufferOnMFMediaBuffer(output_sample, buffer, 0,
                          &dmo_output.pBuffer)))
    {
        hr = video_processor_process_output(impl, &dmo_output);
        IMediaBuffer_Release(dmo_output.pBuffer);
    }
    IMFMediaBuffer_Release(buffer);

    output->dwStatus = *output_status = 0;

    if (provide_samples)
    {
        output->pSample = output_sample;
        IMFSample_AddRef(output_sample);
    }

done:
    IMFSample_Release(output_sample);
    return hr;
}

static const IMFTransformVtbl video_processor_vtbl =
{
    video_processor_QueryInterface,
    video_processor_AddRef,
    video_processor_Release,
    video_processor_GetStreamLimits,
    video_processor_GetStreamCount,
    video_processor_GetStreamIDs,
    video_processor_GetInputStreamInfo,
    video_processor_GetOutputStreamInfo,
    video_processor_GetAttributes,
    video_processor_GetInputStreamAttributes,
    video_processor_GetOutputStreamAttributes,
    video_processor_DeleteInputStream,
    video_processor_AddInputStreams,
    video_processor_GetInputAvailableType,
    video_processor_GetOutputAvailableType,
    video_processor_SetInputType,
    video_processor_SetOutputType,
    video_processor_GetInputCurrentType,
    video_processor_GetOutputCurrentType,
    video_processor_GetInputStatus,
    video_processor_GetOutputStatus,
    video_processor_SetOutputBounds,
    video_processor_ProcessEvent,
    video_processor_ProcessMessage,
    video_processor_ProcessInput,
    video_processor_ProcessOutput,
};

static const char *debugstr_version(UINT version)
{
    return wine_dbg_sprintf("%u.%u.%u", AV_VERSION_MAJOR(version), AV_VERSION_MINOR(version),
            AV_VERSION_MICRO(version));
}

HRESULT video_processor_create(REFIID riid, void **ret)
{
    const MFVIDEOFORMAT input_format =
    {
        .dwSize = sizeof(MFVIDEOFORMAT),
        .videoInfo = {.dwWidth = 1920, .dwHeight = 1080},
        .guidFormat = MFVideoFormat_I420,
    };
    const MFVIDEOFORMAT output_format =
    {
        .dwSize = sizeof(MFVIDEOFORMAT),
        .videoInfo = {.dwWidth = 1920, .dwHeight = 1080},
        .guidFormat = MFVideoFormat_NV12,
    };
    struct video_processor *impl;
    HRESULT hr;

    TRACE("riid %s, ret %p.\n", debugstr_guid(riid), ret);

    TRACE("avutil version %s\n", debugstr_version(avutil_version()));
    TRACE("swscale version %s\n", debugstr_version(swscale_version()));

    if (FAILED(hr = check_video_transform_support(&input_format, &output_format)))
    {
        ERR_(winediag)("GStreamer doesn't support video conversion, please install appropriate plugins.\n");
        return hr;
    }

    if (!(impl = calloc(1, sizeof(*impl))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = MFCreateAttributes(&impl->attributes, 0)))
        goto failed;
    if (FAILED(hr = IMFAttributes_SetUINT32(impl->attributes, &MF_SA_D3D11_AWARE, TRUE)))
        goto failed;
    /* native only has MF_SA_D3D_AWARE on Win7, but it is useful to have in mfreadwrite */
    if (FAILED(hr = IMFAttributes_SetUINT32(impl->attributes, &MF_SA_D3D_AWARE, TRUE)))
        goto failed;
    if (FAILED(hr = MFCreateAttributes(&impl->output_attributes, 0)))
        goto failed;

    impl->IMFTransform_iface.lpVtbl = &video_processor_vtbl;
    impl->refcount = 1;

    *ret = &impl->IMFTransform_iface;
    TRACE("Created %p\n", *ret);
    return S_OK;

failed:
    if (impl->output_attributes)
        IMFAttributes_Release(impl->output_attributes);
    if (impl->attributes)
        IMFAttributes_Release(impl->attributes);
    sws_free_context(&impl->context);
    free(impl);
    return hr;
}
