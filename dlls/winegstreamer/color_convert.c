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

#include "dmort.h"
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

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

DEFINE_MEDIATYPE_GUID(MFVideoFormat_ABGR32, D3DFMT_A8B8G8R8);

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
    /* MEDIASUBTYPE_AYUV */   case MAKEFOURCC('A','Y','U','V'): return AV_PIX_FMT_AYUV;
    /* MEDIASUBTYPE_I420 */   case MAKEFOURCC('I','4','2','0'): return AV_PIX_FMT_YUV420P;
    /* MEDIASUBTYPE_IYUV */   case MAKEFOURCC('I','Y','U','V'): return AV_PIX_FMT_YUV420P;
    /* MEDIASUBTYPE_NV11 */   case MAKEFOURCC('N','V','1','1'): return AV_PIX_FMT_YUV411P;
    /* MEDIASUBTYPE_NV12 */   case MAKEFOURCC('N','V','1','2'): return AV_PIX_FMT_NV12;
    /* MEDIASUBTYPE_UYVY */   case MAKEFOURCC('U','Y','V','Y'): return AV_PIX_FMT_UYVY422;
    /* MEDIASUBTYPE_V216 */   case MAKEFOURCC('V','2','1','6'): break;
    /* MEDIASUBTYPE_V410 */   case MAKEFOURCC('V','4','1','0'): break;
    /* MEDIASUBTYPE_Y41P */   case MAKEFOURCC('Y','4','1','P'): break;
    /* MEDIASUBTYPE_Y41T */   case MAKEFOURCC('Y','4','1','T'): break;
    /* MEDIASUBTYPE_Y42T */   case MAKEFOURCC('Y','4','2','T'): break;
    /* MEDIASUBTYPE_YUY2 */   case MAKEFOURCC('Y','U','Y','2'): return AV_PIX_FMT_YUYV422;
    /* MEDIASUBTYPE_YV12 */   case MAKEFOURCC('Y','V','1','2'): return AV_PIX_FMT_YUV420P;
    /* MEDIASUBTYPE_YVU9 */   case MAKEFOURCC('Y','V','U','9'): break;
    /* MEDIASUBTYPE_YVYU */   case MAKEFOURCC('Y','V','Y','U'): return AV_PIX_FMT_YVYU422;
    /* MEDIASUBTYPE_RGB24 */  case 0xe436eb7d:                  return AV_PIX_FMT_BGR24;
    /* MEDIASUBTYPE_RGB32 */  case 0xe436eb7e:                  return AV_PIX_FMT_BGR0;
    /* MEDIASUBTYPE_RGB555 */ case 0xe436eb7c:                  return AV_PIX_FMT_RGB555;
    /* MEDIASUBTYPE_RGB565 */ case 0xe436eb7b:                  return AV_PIX_FMT_RGB565;
    /* MEDIASUBTYPE_RGB8 */   case 0xe436eb7a:                  return AV_PIX_FMT_RGB8;
    }

    FIXME("Unsupported subtype %s (%s)\n", debugstr_guid(subtype), debugstr_fourcc(subtype->Data1));
    return AV_PIX_FMT_NONE;
}

static void video_frame_init_aperture(AVFrame *frame, const MFVIDEOFORMAT *format)
{
    /* unlike the video processor, color converter doesn't really support aperture */
    frame->width = format->videoInfo.dwWidth;
    frame->height = format->videoInfo.dwHeight;
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
        ERR("stride %lu linesize %u,%u,%u,%u size %#x buffer %#Ix\n", pitch, strides[0], strides[1],
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

    /* unlike the video processor, color converter doesn't care about 2D buffer pitch */
    if (format->videoInfo.VideoFlags & MFVideoFlag_BottomUpLinearRep)
    {
        planes[0] = planes[0] + strides[0] * (height - 1);
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
    &MFVideoFormat_YV12,
    &MFVideoFormat_YUY2,
    &MFVideoFormat_UYVY,
    &MFVideoFormat_AYUV,
    &MFVideoFormat_NV12,
    &MFVideoFormat_RGB32,
    &MFVideoFormat_RGB565,
    &MFVideoFormat_I420,
    &MFVideoFormat_IYUV,
    &MFVideoFormat_YVYU,
    &MFVideoFormat_RGB24,
    &MFVideoFormat_RGB555,
    &MEDIASUBTYPE_RGB8,
    &MEDIASUBTYPE_V216,
    &MEDIASUBTYPE_V410,
    &MFVideoFormat_NV11,
    &MFVideoFormat_Y41P,
    &MFVideoFormat_Y41T,
    &MFVideoFormat_Y42T,
    &MFVideoFormat_YVU9,
};
static const GUID *const output_types[] =
{
    &MFVideoFormat_YV12,
    &MFVideoFormat_YUY2,
    &MFVideoFormat_UYVY,
    &MFVideoFormat_AYUV,
    &MFVideoFormat_NV12,
    &MFVideoFormat_RGB32,
    &MFVideoFormat_RGB565,
    &MFVideoFormat_I420,
    &MFVideoFormat_IYUV,
    &MFVideoFormat_YVYU,
    &MFVideoFormat_RGB24,
    &MFVideoFormat_RGB555,
    &MEDIASUBTYPE_RGB8,
    &MEDIASUBTYPE_V216,
    &MEDIASUBTYPE_V410,
    &MFVideoFormat_NV11,
};

struct color_convert
{
    IUnknown IUnknown_inner;
    IMFTransform IMFTransform_iface;
    IMediaObject IMediaObject_iface;
    IPropertyBag IPropertyBag_iface;
    IPropertyStore IPropertyStore_iface;
    IUnknown *outer;
    LONG refcount;

    IMFMediaType *input_type;
    DMO_MEDIA_TYPE input_mt;
    MFVIDEOFORMAT input_format;
    MFT_INPUT_STREAM_INFO input_info;
    IMFMediaType *output_type;
    DMO_MEDIA_TYPE output_mt;
    MFVIDEOFORMAT output_format;
    MFT_OUTPUT_STREAM_INFO output_info;

    wg_transform_t wg_transform;
    struct wg_sample_queue *wg_sample_queue;

    SwsContext *context;
    AVFrame input_frame;
    AVFrame output_frame;
    AVFrame current_frame;
};

static HRESULT color_convert_process_frame(struct color_convert *impl, AVFrame *input_frame,
        DMO_OUTPUT_DATA_BUFFER *output)
{
    AVFrame output_frame = {0};
    AVBufferRef *buffer;
    IMFSample *sample;
    int size, ret;
    HRESULT hr;
    LONG pitch;

    if (!(buffer = buffer_from_media_buffer(output->pBuffer, 0, &pitch)))
        return E_OUTOFMEMORY;
    if ((size = video_frame_wrap_buffer(&output_frame, &impl->output_format, buffer, pitch)) < 0)
        av_frame_move_ref(&output_frame, &impl->output_frame);

    TRACE("input %s output %s\n", debugstr_avframe(input_frame), debugstr_avframe(&output_frame));

    if ((ret = sws_scale_frame(impl->context, &output_frame, input_frame)) < 0)
        ERR("error ret %d (%s)\n", -ret, av_err2str(ret));
    else if ((ret = size) < 0)
        ret = video_frame_copy_to_buffer(&output_frame, &impl->output_format, buffer, pitch);

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
        return S_FALSE;

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

static HRESULT color_convert_process_input(struct color_convert *impl, const DMO_OUTPUT_DATA_BUFFER *input)
{
    LONGLONG pts = input->rtTimestamp, duration = input->rtTimelength;
    AVBufferRef *buffer;
    IMFSample *sample;
    HRESULT hr;
    LONG pitch;
    int ret;

    if (!impl->context)
        return DMO_E_TYPE_NOT_SET;
    if (impl->current_frame.width)
        return DMO_E_NOTACCEPTING;

    if (!(buffer = buffer_from_media_buffer(input->pBuffer, AV_BUFFER_FLAG_READONLY, &pitch)))
        return E_OUTOFMEMORY;
    if ((ret = video_frame_wrap_buffer(&impl->current_frame, &impl->input_format, buffer, pitch)) < 0)
    {
        ret = video_frame_copy_from_buffer(&impl->input_frame, &impl->input_format, buffer, pitch);
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
    TRACE("input frame size %ux%u, time %s, duration %s\n", impl->current_frame.width,
            impl->current_frame.height, debugstr_avtime(impl->current_frame.pts, USER_TIME_BASE_Q),
            debugstr_avtime(impl->current_frame.duration, USER_TIME_BASE_Q));
    return S_OK;
}

static HRESULT color_convert_process_output(struct color_convert *impl, DMO_OUTPUT_DATA_BUFFER *output)
{
    HRESULT hr;

    if (!impl->context)
        return DMO_E_TYPE_NOT_SET;
    if (!impl->current_frame.width)
        return S_FALSE;

    hr = color_convert_process_frame(impl, &impl->current_frame, output);
    if (!impl->current_frame.opaque)
        av_frame_move_ref(&impl->input_frame, &impl->current_frame);
    else
        av_frame_unref(&impl->current_frame);

    return hr;
}

static void color_convert_cleanup(struct color_convert *impl)
{
    memset(&impl->input_format, 0, sizeof(impl->input_format));
    memset(&impl->output_format, 0, sizeof(impl->output_format));
    av_frame_unref(&impl->current_frame);
    av_frame_unref(&impl->output_frame);
    av_frame_unref(&impl->input_frame);
    sws_free_context(&impl->context);
}

static BOOL get_default_stride_sign(const BITMAPINFOHEADER *header)
{
    return header->biCompression == BI_RGB || header->biCompression == BI_BITFIELDS ? -1 : 1;
}

static const GUID *get_dmo_subtype(const GUID *subtype)
{
    if (IsEqualGUID(subtype, &MFVideoFormat_RGB8))
        return &MEDIASUBTYPE_RGB8;
    if (IsEqualGUID(subtype, &MFVideoFormat_RGB555))
        return &MEDIASUBTYPE_RGB555;
    if (IsEqualGUID(subtype, &MFVideoFormat_RGB565))
        return &MEDIASUBTYPE_RGB565;
    if (IsEqualGUID(subtype, &MFVideoFormat_RGB24))
        return &MEDIASUBTYPE_RGB24;
    if (IsEqualGUID(subtype, &MFVideoFormat_RGB32))
        return &MEDIASUBTYPE_RGB32;
    return subtype;
}

static HRESULT init_video_format(const DMO_MEDIA_TYPE *type, MFVIDEOFORMAT *format)
{
    if (IsEqualGUID(&type->formattype, &FORMAT_VideoInfo))
    {
        const VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)type->pbFormat;
        memset(format, 0, sizeof(*format));
        format->videoInfo.dwWidth = vih->bmiHeader.biWidth;
        format->videoInfo.dwHeight = abs(vih->bmiHeader.biHeight);
        if (get_default_stride_sign(&vih->bmiHeader) < 0 && vih->bmiHeader.biHeight > 0)
            format->videoInfo.VideoFlags |= MFVideoFlag_BottomUpLinearRep;
        format->guidFormat = type->subtype;
        return S_OK;
    }
    if (IsEqualGUID(&type->formattype, &FORMAT_VideoInfo2))
    {
        const VIDEOINFOHEADER2 *vih = (VIDEOINFOHEADER2 *)type->pbFormat;
        memset(format, 0, sizeof(*format));
        format->videoInfo.dwWidth = vih->bmiHeader.biWidth;
        format->videoInfo.dwHeight = abs(vih->bmiHeader.biHeight);
        if (get_default_stride_sign(&vih->bmiHeader) < 0 && vih->bmiHeader.biHeight > 0)
            format->videoInfo.VideoFlags |= MFVideoFlag_BottomUpLinearRep;
        format->guidFormat = type->subtype;
        return S_OK;
    }
    if (IsEqualGUID(&type->formattype, &FORMAT_MFVideoFormat))
    {
        *format = *(MFVIDEOFORMAT *)type->pbFormat;
        format->guidFormat = *get_dmo_subtype(&format->guidFormat);
        return S_OK;
    }

    FIXME("Format %s not implemented\n", debugstr_guid(&type->formattype));
    return E_NOTIMPL;
}

static HRESULT color_convert_init(struct color_convert *impl)
{
    HRESULT hr;
    int ret;

    color_convert_cleanup(impl);

    if (!(impl->context = sws_alloc_context()))
        return E_OUTOFMEMORY;
    if (FAILED(hr = init_video_format(&impl->input_mt, &impl->input_format))
            || FAILED(hr = init_video_format(&impl->output_mt, &impl->output_format)))
    {
        color_convert_cleanup(impl);
        return hr;
    }

    video_frame_init_from_format(&impl->input_frame, &impl->input_format);
    if ((ret = av_frame_get_buffer(&impl->input_frame, 0)) < 0)
        goto failed;
    video_frame_init_aperture(&impl->input_frame, &impl->input_format);

    video_frame_init_from_format(&impl->output_frame, &impl->output_format);
    if ((ret = av_frame_get_buffer(&impl->output_frame, 0)) < 0)
        goto failed;
    video_frame_init_aperture(&impl->output_frame, &impl->output_format);

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
    color_convert_cleanup(impl);
    return E_FAIL;
}

static struct color_convert *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct color_convert, IUnknown_inner);
}

static void update_video_aperture(MFVideoInfo *input_info, MFVideoInfo *output_info)
{
    static const MFVideoArea empty_area = {0};

    /* Tests show that the color converter ignores aperture entirely, probably a side
     * effect of an internal conversion to VIDEOINFOHEADER2, as the component is also
     * exposing a IMediaObject interface, and designed for dshow.
     */

    input_info->GeometricAperture = empty_area;
    input_info->MinimumDisplayAperture = empty_area;
    input_info->PanScanAperture = empty_area;

    output_info->GeometricAperture = empty_area;
    output_info->MinimumDisplayAperture = empty_area;
    output_info->PanScanAperture = empty_area;
}

static HRESULT normalize_media_types(IMFMediaType **input_type, IMFMediaType **output_type)
{
    MFVIDEOFORMAT *input_format, *output_format;
    UINT32 size;
    HRESULT hr;

    if (FAILED(hr = MFCreateMFVideoFormatFromMFMediaType(*input_type, &input_format, &size)))
        return hr;
    if (FAILED(hr = MFCreateMFVideoFormatFromMFMediaType(*output_type, &output_format, &size)))
    {
        CoTaskMemFree(input_format);
        return hr;
    }

    update_video_aperture(&input_format->videoInfo, &output_format->videoInfo);

    if (FAILED(hr = MFCreateVideoMediaType(input_format, (IMFVideoMediaType **)input_type)))
        goto done;
    if (FAILED(hr = MFCreateVideoMediaType(output_format, (IMFVideoMediaType **)output_type)))
    {
        IMFMediaType_Release(*input_type);
        *input_type = NULL;
    }

done:
    CoTaskMemFree(input_format);
    CoTaskMemFree(output_format);
    return hr;
}

static HRESULT try_create_wg_transform(struct color_convert *impl)
{
    IMFMediaType *input_type = impl->input_type, *output_type = impl->output_type;
    struct wg_transform_attrs attrs = {0};
    HRESULT hr;

    if (impl->wg_transform)
    {
        wg_transform_destroy(impl->wg_transform);
        impl->wg_transform = 0;
    }

    if (FAILED(hr = normalize_media_types(&input_type, &output_type)))
        return hr;
    hr = wg_transform_create_mf(input_type, output_type, &attrs, &impl->wg_transform);
    IMFMediaType_Release(output_type);
    IMFMediaType_Release(input_type);

    return hr;
}

static HRESULT WINAPI unknown_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct color_convert *impl = impl_from_IUnknown(iface);

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
    struct color_convert *impl = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&impl->refcount);

    TRACE("iface %p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI unknown_Release(IUnknown *iface)
{
    struct color_convert *impl = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&impl->refcount);

    TRACE("iface %p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        color_convert_cleanup(impl);
        if (impl->wg_transform)
            wg_transform_destroy(impl->wg_transform);
        if (impl->input_type)
            IMFMediaType_Release(impl->input_type);
        if (impl->output_type)
            IMFMediaType_Release(impl->output_type);

        wg_sample_queue_destroy(impl->wg_sample_queue);
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

static struct color_convert *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct color_convert, IMFTransform_iface);
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
    struct color_convert *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    if (!impl->input_type || !impl->output_type)
    {
        memset(info, 0, sizeof(*info));
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    *info = impl->input_info;
    return S_OK;
}

static HRESULT WINAPI transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct color_convert *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    if (!impl->input_type || !impl->output_type)
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
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, 1)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, 1)))
        goto done;

    IMFMediaType_AddRef((*type = media_type));

done:
    IMFMediaType_Release(media_type);
    return hr;
}

static HRESULT WINAPI transform_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    IMFMediaType *media_type;
    const GUID *subtype;
    HRESULT hr;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    *type = NULL;

    if (index >= ARRAY_SIZE(output_types))
        return MF_E_NO_MORE_TYPES;
    subtype = output_types[index];

    if (FAILED(hr = MFCreateMediaType(&media_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, subtype)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, 1)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, 1)))
        goto done;

    IMFMediaType_AddRef((*type = media_type));

done:
    IMFMediaType_Release(media_type);
    return hr;
}

static HRESULT WINAPI transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct color_convert *impl = impl_from_IMFTransform(iface);
    GUID major, subtype;
    UINT64 frame_size;
    UINT32 stride;
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
        if (impl->wg_transform)
        {
            wg_transform_destroy(impl->wg_transform);
            impl->wg_transform = 0;
        }

        return S_OK;
    }

    if (FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major)) ||
            FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return MF_E_ATTRIBUTENOTFOUND;

    if (!IsEqualGUID(&major, &MFMediaType_Video)
            || IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size))
        return E_INVALIDARG;

    for (i = 0; i < ARRAY_SIZE(input_types); ++i)
        if (IsEqualGUID(&subtype, input_types[i]))
            break;
    if (i == ARRAY_SIZE(input_types))
        return MF_E_INVALIDMEDIATYPE;
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (!impl->input_type && FAILED(hr = MFCreateMediaType(&impl->input_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_CopyAllItems(type, (IMFAttributes *)impl->input_type)))
    {
        IMFMediaType_Release(impl->input_type);
        impl->input_type = NULL;
    }
    if (FAILED(IMFMediaType_GetUINT32(impl->input_type, &MF_MT_DEFAULT_STRIDE, &stride)))
    {
        if (FAILED(hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, frame_size >> 32, (LONG *)&stride)))
        {
            IMFMediaType_Release(impl->input_type);
            impl->input_type = NULL;
        }
        if (FAILED(hr = IMFMediaType_SetUINT32(impl->input_type, &MF_MT_DEFAULT_STRIDE, abs((INT32)stride))))
        {
            IMFMediaType_Release(impl->input_type);
            impl->input_type = NULL;
        }
    }

    if (impl->output_type && FAILED(hr = try_create_wg_transform(impl)))
    {
        IMFMediaType_Release(impl->input_type);
        impl->input_type = NULL;
    }

    if (FAILED(hr) || FAILED(MFCalculateImageSize(&subtype, frame_size >> 32, (UINT32)frame_size,
            (UINT32 *)&impl->input_info.cbSize)))
        impl->input_info.cbSize = 0;

    return hr;
}

static HRESULT WINAPI transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct color_convert *impl = impl_from_IMFTransform(iface);
    GUID major, subtype;
    UINT64 frame_size;
    UINT32 stride;
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
        if (impl->wg_transform)
        {
            wg_transform_destroy(impl->wg_transform);
            impl->wg_transform = 0;
        }

        return S_OK;
    }

    if (FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major)) ||
            FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return MF_E_ATTRIBUTENOTFOUND;

    if (!IsEqualGUID(&major, &MFMediaType_Video)
            || IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size))
        return E_INVALIDARG;

    for (i = 0; i < ARRAY_SIZE(output_types); ++i)
        if (IsEqualGUID(&subtype, output_types[i]))
            break;
    if (i == ARRAY_SIZE(output_types))
        return MF_E_INVALIDMEDIATYPE;
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (!impl->output_type && FAILED(hr = MFCreateMediaType(&impl->output_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_CopyAllItems(type, (IMFAttributes *)impl->output_type)))
    {
        IMFMediaType_Release(impl->output_type);
        impl->output_type = NULL;
    }
    if (FAILED(IMFMediaType_GetUINT32(impl->output_type, &MF_MT_DEFAULT_STRIDE, &stride)))
    {
        if (FAILED(hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, frame_size >> 32, (LONG *)&stride)))
        {
            IMFMediaType_Release(impl->output_type);
            impl->output_type = NULL;
        }
        if (FAILED(hr = IMFMediaType_SetUINT32(impl->output_type, &MF_MT_DEFAULT_STRIDE, abs((INT32)stride))))
        {
            IMFMediaType_Release(impl->output_type);
            impl->output_type = NULL;
        }
    }

    if (impl->input_type && FAILED(hr = try_create_wg_transform(impl)))
    {
        IMFMediaType_Release(impl->output_type);
        impl->output_type = NULL;
    }

    if (FAILED(hr) || FAILED(MFCalculateImageSize(&subtype, frame_size >> 32, (UINT32)frame_size,
            (UINT32 *)&impl->output_info.cbSize)))
        impl->output_info.cbSize = 0;

    return hr;
}

static HRESULT WINAPI transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct color_convert *impl = impl_from_IMFTransform(iface);
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

static HRESULT WINAPI transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct color_convert *impl = impl_from_IMFTransform(iface);
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
    FIXME("iface %p, message %#x, param %#Ix stub!\n", iface, message, param);
    return S_OK;
}

static HRESULT WINAPI transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct color_convert *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, sample %p, flags %#lx.\n", iface, id, sample, flags);

    if (!impl->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    return wg_transform_push_mf(impl->wg_transform, sample, impl->wg_sample_queue);
}

static HRESULT WINAPI transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct color_convert *impl = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#lx, count %lu, samples %p, status %p.\n", iface, flags, count, samples, status);

    if (count != 1)
        return E_INVALIDARG;

    if (!impl->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *status = samples->dwStatus = 0;
    if (!samples->pSample)
        return E_INVALIDARG;

    if (SUCCEEDED(hr = wg_transform_read_mf(impl->wg_transform, samples->pSample,
            impl->output_info.cbSize, &samples->dwStatus, NULL)))
        wg_sample_queue_flush(impl->wg_sample_queue, false);

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

static struct color_convert *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct color_convert, IMediaObject_iface);
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
    FIXME("iface %p, input %p, output %p stub!\n", iface, input, output);
    return E_NOTIMPL;
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

static HRESULT get_available_dmo_media_type(const GUID *subtype, DMO_MEDIA_TYPE *type, BOOL output)
{
    VIDEOINFOHEADER *vih;

    if (!(vih = CoTaskMemAlloc(sizeof(*vih))))
        return E_OUTOFMEMORY;
    memset(vih, 0, sizeof(*vih));

    memset(type, 0, sizeof(*type));
    type->majortype = MEDIATYPE_Video;
    type->formattype = FORMAT_VideoInfo;
    type->subtype = *subtype;
    type->pbFormat = (BYTE *)vih;
    type->cbFormat = sizeof(*vih);
    return S_OK;
}

static HRESULT WINAPI media_object_GetInputType(IMediaObject *iface, DWORD index, DWORD type_index,
        DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %lu, type_index %lu, type %p stub!\n", iface, index, type_index, type);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (type_index >= ARRAY_SIZE(input_types))
        return DMO_E_NO_MORE_ITEMS;
    return get_available_dmo_media_type(input_types[type_index], type, FALSE);
}

static HRESULT WINAPI media_object_GetOutputType(IMediaObject *iface, DWORD index, DWORD type_index,
        DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %lu, type_index %lu, type %p stub!\n", iface, index, type_index, type);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (type_index >= ARRAY_SIZE(output_types))
        return DMO_E_NO_MORE_ITEMS;
    return get_available_dmo_media_type(output_types[type_index], type, TRUE);
}

static HRESULT check_dmo_media_type(const DMO_MEDIA_TYPE *type, UINT32 *image_size,
        const GUID *const *formats, UINT format_count)
{
    ULONG i;

    if (!IsEqualGUID(&type->majortype, &MEDIATYPE_Video))
        return DMO_E_INVALIDTYPE;
    for (i = 0; i < format_count; ++i)
        if (IsEqualGUID(get_dmo_subtype(&type->subtype), get_dmo_subtype(formats[i])))
            break;
    if (i == format_count)
        return DMO_E_INVALIDTYPE;

    if (IsEqualGUID(&type->formattype, &FORMAT_VideoInfo)
            && type->cbFormat >= sizeof(VIDEOINFOHEADER))
    {
        VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)type->pbFormat;
        if (!vih->bmiHeader.biWidth || !vih->bmiHeader.biHeight)
            return DMO_E_INVALIDTYPE;
        *image_size = vih->bmiHeader.biSizeImage;
        return S_OK;
    }
    if (IsEqualGUID(&type->formattype, &FORMAT_VideoInfo2)
            && type->cbFormat >= sizeof(VIDEOINFOHEADER2))
    {
        VIDEOINFOHEADER2 *vih = (VIDEOINFOHEADER2 *)type->pbFormat;
        if (!vih->bmiHeader.biWidth || !vih->bmiHeader.biHeight)
            return DMO_E_INVALIDTYPE;
        *image_size = vih->bmiHeader.biSizeImage;
        return S_OK;
    }
    if (IsEqualGUID(&type->formattype, &FORMAT_MFVideoFormat)
            && type->cbFormat >= sizeof(MFVIDEOFORMAT))
    {
        MFVIDEOFORMAT *fmt = (MFVIDEOFORMAT *)type->pbFormat;
        if (!fmt->videoInfo.dwWidth || !fmt->videoInfo.dwHeight)
            return DMO_E_INVALIDTYPE;
        return MFCalculateImageSize(&type->subtype, fmt->videoInfo.dwWidth, fmt->videoInfo.dwHeight,
            (UINT32 *)image_size);
    }

    return DMO_E_INVALIDTYPE;
}

static void clear_dmo_media_type(DMO_MEDIA_TYPE *mt)
{
    MoFreeMediaType(mt);
    memset(mt, 0, sizeof(*mt));
}

static HRESULT WINAPI media_object_SetInputType(IMediaObject *iface, DWORD index,
        const DMO_MEDIA_TYPE *type, DWORD flags)
{
    struct color_convert *impl = impl_from_IMediaObject(iface);
    UINT32 image_size;
    HRESULT hr;

    TRACE("converter %p, index %#lx, type %p, flags %#lx.\n", impl, index, type, flags);

    if (index != 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (!type)
    {
        clear_dmo_media_type(&impl->input_mt);
        color_convert_cleanup(impl);
        return S_OK;
    }

    if (FAILED(hr = check_dmo_media_type(type, &image_size, input_types, ARRAY_SIZE(input_types))))
        return hr;
    if (flags & DMO_SET_TYPEF_TEST_ONLY)
        return S_OK;

    clear_dmo_media_type(&impl->input_mt);
    if (FAILED(hr = MoCopyMediaType(&impl->input_mt, type)))
        return hr;
    impl->input_info.cbSize = image_size;

    if (!IsEqualGUID(&impl->output_mt.majortype, &GUID_NULL)
            && FAILED(hr = color_convert_init(impl)))
    {
        clear_dmo_media_type(&impl->input_mt);
        impl->input_info.cbSize = 0;
    }
    return hr;
}

static HRESULT WINAPI media_object_SetOutputType(IMediaObject *iface, DWORD index,
        const DMO_MEDIA_TYPE *type, DWORD flags)
{
    struct color_convert *impl = impl_from_IMediaObject(iface);
    UINT32 image_size;
    HRESULT hr;

    TRACE("converter %p, index %#lx, type %p, flags %#lx.\n", impl, index, type, flags);

    if (index != 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (!type)
    {
        clear_dmo_media_type(&impl->output_mt);
        color_convert_cleanup(impl);
        return S_OK;
    }

    if (FAILED(hr = check_dmo_media_type(type, &image_size, output_types, ARRAY_SIZE(output_types))))
        return hr;
    if (flags & DMO_SET_TYPEF_TEST_ONLY)
        return S_OK;

    clear_dmo_media_type(&impl->output_mt);
    if (FAILED(hr = MoCopyMediaType(&impl->output_mt, type)))
        return hr;
    impl->output_info.cbSize = image_size;

    if (!IsEqualGUID(&impl->input_mt.majortype, &GUID_NULL)
            && FAILED(hr = color_convert_init(impl)))
    {
        clear_dmo_media_type(&impl->output_mt);
        impl->output_info.cbSize = 0;
    }
    return hr;
}

static HRESULT WINAPI media_object_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    struct color_convert *impl = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %#lx, type %p.\n", iface, index, type);

    if (index != 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (IsEqualGUID(&impl->input_mt.majortype, &GUID_NULL))
        return DMO_E_TYPE_NOT_SET;
    return MoCopyMediaType(type, &impl->input_mt);
}

static HRESULT WINAPI media_object_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    struct color_convert *impl = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %#lx, type %p.\n", iface, index, type);

    if (index != 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (IsEqualGUID(&impl->output_mt.majortype, &GUID_NULL))
        return DMO_E_TYPE_NOT_SET;
    return MoCopyMediaType(type, &impl->output_mt);
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
    FIXME("iface %p, index %lu, latency %s stub!\n", iface, index, wine_dbgstr_longlong(latency));
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
    struct color_convert *impl = impl_from_IMediaObject(iface);

    TRACE("converter %p, index %lu, buffer %p, flags %#lx, timestamp %I64d, timelength %I64d\n",
            impl, index, buffer, flags, timestamp, timelength);

    return color_convert_process_input(impl, &input);
}

static HRESULT WINAPI media_object_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count,
        DMO_OUTPUT_DATA_BUFFER *output, DWORD *output_status)
{
    struct color_convert *impl = impl_from_IMediaObject(iface);

    TRACE("converter %p, flags %#lx, count %lu, output %p, output_status %p\n", impl, flags, count,
            output, output_status);

    if (count != 1)
        return E_INVALIDARG;
    if (flags)
        FIXME("Unimplemented flags %#lx\n", flags);

    return color_convert_process_output(impl, output);
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

static struct color_convert *impl_from_IPropertyBag(IPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, struct color_convert, IPropertyBag_iface);
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

static struct color_convert *impl_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, struct color_convert, IPropertyStore_iface);
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

static const char *debugstr_version(UINT version)
{
    return wine_dbg_sprintf("%u.%u.%u", AV_VERSION_MAJOR(version), AV_VERSION_MINOR(version),
            AV_VERSION_MICRO(version));
}

HRESULT color_convert_create(IUnknown *outer, IUnknown **out)
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
    struct color_convert *impl;
    HRESULT hr;

    TRACE("outer %p, out %p.\n", outer, out);

    TRACE("avutil version %s\n", debugstr_version(avutil_version()));
    TRACE("swscale version %s\n", debugstr_version(swscale_version()));

    if (FAILED(hr = check_video_transform_support(&input_format, &output_format)))
    {
        ERR_(winediag)("GStreamer doesn't support video conversion, please install appropriate plugins.\n");
        return hr;
    }

    if (!(impl = calloc(1, sizeof(*impl))))
        return E_OUTOFMEMORY;
    if (FAILED(hr = wg_sample_queue_create(&impl->wg_sample_queue)))
    {
        free(impl);
        return hr;
    }

    impl->IUnknown_inner.lpVtbl = &unknown_vtbl;
    impl->IMFTransform_iface.lpVtbl = &transform_vtbl;
    impl->IMediaObject_iface.lpVtbl = &media_object_vtbl;
    impl->IPropertyBag_iface.lpVtbl = &property_bag_vtbl;
    impl->IPropertyStore_iface.lpVtbl = &property_store_vtbl;
    impl->refcount = 1;
    impl->outer = outer ? outer : &impl->IUnknown_inner;

    impl->input_info.cbAlignment = 1;
    impl->output_info.cbAlignment = 1;

    *out = &impl->IUnknown_inner;
    TRACE("Created %p\n", *out);
    return S_OK;
}

HRESULT WINAPI winegstreamer_create_color_converter(IMFTransform **out)
{
    IUnknown *unknown;
    HRESULT hr;

    TRACE("out %p.\n", out);

    if (!init_gstreamer())
        return E_FAIL;

    if (FAILED(hr = color_convert_create(NULL, &unknown)))
        return hr;

    return IUnknown_QueryInterface(unknown, &IID_IMFTransform, (void**)out);
}
