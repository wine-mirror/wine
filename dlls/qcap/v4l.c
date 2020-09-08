/*
 * v4l2 backend to the VFW Capture filter
 *
 * Copyright 2005 Maarten Lankhorst
 * Copyright 2019 Zebediah Figura
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

#define BIONIC_IOCTL_NO_SIGNEDNESS_OVERLOAD  /* work around ioctl breakage on Android */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <errno.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif
#ifdef HAVE_LINUX_VIDEODEV2_H
#include <linux/videodev2.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "qcap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

#ifdef HAVE_LINUX_VIDEODEV2_H

WINE_DECLARE_DEBUG_CHANNEL(winediag);

static typeof(open) *video_open = open;
static typeof(close) *video_close = close;
static typeof(ioctl) *video_ioctl = ioctl;
static typeof(read) *video_read = read;

static BOOL video_init(void)
{
#ifdef SONAME_LIBV4L2
    static void *video_lib;

    if (video_lib)
        return TRUE;
    if (!(video_lib = dlopen(SONAME_LIBV4L2, RTLD_NOW)))
        return FALSE;
    video_open = dlsym(video_lib, "v4l2_open");
    video_close = dlsym(video_lib, "v4l2_close");
    video_ioctl = dlsym(video_lib, "v4l2_ioctl");
    video_read = dlsym(video_lib, "v4l2_read");

    return TRUE;
#else
    return FALSE;
#endif
}

struct caps
{
    __u32 pixelformat;
    AM_MEDIA_TYPE media_type;
    VIDEOINFOHEADER video_info;
    VIDEO_STREAM_CONFIG_CAPS config;
};

struct v4l_device
{
    struct video_capture_device d;

    const struct caps *current_caps;
    struct caps *caps;
    LONG caps_count;

    int image_size, image_pitch;

    struct strmbase_source *pin;
    int fd, mmap;

    HANDLE thread;
    FILTER_STATE state;
    CONDITION_VARIABLE state_cv;
    CRITICAL_SECTION state_cs;
};

static inline struct v4l_device *v4l_device(struct video_capture_device *iface)
{
    return CONTAINING_RECORD(iface, struct v4l_device, d);
}

static int xioctl(int fd, int request, void * arg)
{
    int r;

    do {
        r = video_ioctl (fd, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

static void v4l_device_destroy(struct video_capture_device *iface)
{
    struct v4l_device *device = v4l_device(iface);

    device->state_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&device->state_cs);
    if (device->fd != -1)
        video_close(device->fd);
    if (device->caps_count)
        heap_free(device->caps);
    heap_free(device);
}

static const struct caps *find_caps(struct v4l_device *device, const AM_MEDIA_TYPE *mt)
{
    const VIDEOINFOHEADER *video_info = (VIDEOINFOHEADER *)mt->pbFormat;
    LONG index;

    if (mt->cbFormat < sizeof(VIDEOINFOHEADER) || !video_info)
        return NULL;

    for (index = 0; index < device->caps_count; index++)
    {
        struct caps *caps = &device->caps[index];

        if (IsEqualGUID(&mt->formattype, &caps->media_type.formattype)
                && video_info->bmiHeader.biWidth == caps->video_info.bmiHeader.biWidth
                && video_info->bmiHeader.biHeight == caps->video_info.bmiHeader.biHeight)
            return caps;
    }
    return NULL;
}

static HRESULT v4l_device_check_format(struct video_capture_device *iface, const AM_MEDIA_TYPE *mt)
{
    struct v4l_device *device = v4l_device(iface);

    TRACE("device %p, mt %p.\n", device, mt);

    if (!mt)
        return E_POINTER;

    if (!IsEqualGUID(&mt->majortype, &MEDIATYPE_Video))
        return E_FAIL;

    if (find_caps(device, mt))
        return S_OK;

    return E_FAIL;
}

static BOOL set_caps(struct v4l_device *device, const struct caps *caps)
{
    struct v4l2_format format = {0};
    LONG width, height;

    width = caps->video_info.bmiHeader.biWidth;
    height = caps->video_info.bmiHeader.biHeight;

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = caps->pixelformat;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    if (xioctl(device->fd, VIDIOC_S_FMT, &format) == -1
            || format.fmt.pix.pixelformat != caps->pixelformat
            || format.fmt.pix.width != width
            || format.fmt.pix.height != height)
    {
        ERR("Failed to set pixel format: %s.\n", strerror(errno));
        return FALSE;
    }

    device->current_caps = caps;
    device->image_size = width * height * caps->video_info.bmiHeader.biBitCount / 8;
    device->image_pitch = width * caps->video_info.bmiHeader.biBitCount / 8;

    return TRUE;
}

static HRESULT v4l_device_set_format(struct video_capture_device *iface, const AM_MEDIA_TYPE *mt)
{
    struct v4l_device *device = v4l_device(iface);
    const struct caps *caps;

    caps = find_caps(device, mt);
    if (!caps)
        return E_FAIL;

    if (device->current_caps == caps)
        return S_OK;

    if (!set_caps(device, caps))
        return VFW_E_TYPE_NOT_ACCEPTED;

    return S_OK;
}

static HRESULT v4l_device_get_format(struct video_capture_device *iface, AM_MEDIA_TYPE *mt)
{
    struct v4l_device *device = v4l_device(iface);

    return CopyMediaType(mt, &device->current_caps->media_type);
}

static __u32 v4l2_cid_from_qcap_property(VideoProcAmpProperty property)
{
    switch (property)
    {
    case VideoProcAmp_Brightness:
        return V4L2_CID_BRIGHTNESS;
    case VideoProcAmp_Contrast:
        return V4L2_CID_CONTRAST;
    case VideoProcAmp_Hue:
        return V4L2_CID_HUE;
    case VideoProcAmp_Saturation:
        return V4L2_CID_SATURATION;
    default:
        FIXME("Unhandled property %d.\n", property);
        return 0;
    }
}

static HRESULT v4l_device_get_prop_range(struct video_capture_device *iface, VideoProcAmpProperty property,
        LONG *min, LONG *max, LONG *step, LONG *default_value, LONG *flags)
{
    struct v4l_device *device = v4l_device(iface);
    struct v4l2_queryctrl ctrl;

    ctrl.id = v4l2_cid_from_qcap_property(property);

    if (xioctl(device->fd, VIDIOC_QUERYCTRL, &ctrl) == -1)
    {
        WARN("Failed to query control: %s\n", strerror(errno));
        return E_PROP_ID_UNSUPPORTED;
    }

    *min = ctrl.minimum;
    *max = ctrl.maximum;
    *step = ctrl.step;
    *default_value = ctrl.default_value;
    *flags = VideoProcAmp_Flags_Manual;
    return S_OK;
}

static HRESULT v4l_device_get_prop(struct video_capture_device *iface,
        VideoProcAmpProperty property, LONG *value, LONG *flags)
{
    struct v4l_device *device = v4l_device(iface);
    struct v4l2_control ctrl;

    ctrl.id = v4l2_cid_from_qcap_property(property);

    if (xioctl(device->fd, VIDIOC_G_CTRL, &ctrl) == -1)
    {
        WARN("Failed to get property: %s\n", strerror(errno));
        return E_FAIL;
    }

    *value = ctrl.value;
    *flags = VideoProcAmp_Flags_Manual;

    return S_OK;
}

static HRESULT v4l_device_set_prop(struct video_capture_device *iface,
        VideoProcAmpProperty property, LONG value, LONG flags)
{
    struct v4l_device *device = v4l_device(iface);
    struct v4l2_control ctrl;

    ctrl.id = v4l2_cid_from_qcap_property(property);
    ctrl.value = value;

    if (xioctl(device->fd, VIDIOC_S_CTRL, &ctrl) == -1)
    {
        WARN("Failed to set property: %s\n", strerror(errno));
        return E_FAIL;
    }

    return S_OK;
}

static void reverse_image(struct v4l_device *device, LPBYTE output, const BYTE *input)
{
    int inoffset, outoffset, pitch;

    /* the whole image needs to be reversed,
       because the dibs are messed up in windows */
    outoffset = device->image_size;
    pitch = device->image_pitch;
    inoffset = 0;
    while (outoffset > 0)
    {
        int x;
        outoffset -= pitch;
        for (x = 0; x < pitch; x++)
            output[outoffset + x] = input[inoffset + x];
        inoffset += pitch;
    }
}

static DWORD WINAPI ReadThread(void *arg)
{
    struct v4l_device *device = arg;
    HRESULT hr;
    IMediaSample *pSample = NULL;
    unsigned char *pTarget, *image_data;

    if (!(image_data = heap_alloc(device->image_size)))
    {
        ERR("Failed to allocate memory.\n");
        return 0;
    }

    for (;;)
    {
        EnterCriticalSection(&device->state_cs);

        while (device->state == State_Paused)
            SleepConditionVariableCS(&device->state_cv, &device->state_cs, INFINITE);

        if (device->state == State_Stopped)
        {
            LeaveCriticalSection(&device->state_cs);
            break;
        }

        LeaveCriticalSection(&device->state_cs);

        hr = BaseOutputPinImpl_GetDeliveryBuffer(device->pin, &pSample, NULL, NULL, 0);
        if (SUCCEEDED(hr))
        {
            int len;
            
            IMediaSample_SetActualDataLength(pSample, device->image_size);

            len = IMediaSample_GetActualDataLength(pSample);
            TRACE("Data length: %d KB\n", len / 1024);

            IMediaSample_GetPointer(pSample, &pTarget);

            while (video_read(device->fd, image_data, device->image_size) == -1)
            {
                if (errno != EAGAIN)
                {
                    ERR("Failed to read frame: %s\n", strerror(errno));
                    break;
                }
            }

            reverse_image(device, pTarget, image_data);
            hr = IMemInputPin_Receive(device->pin->pMemInputPin, pSample);
            IMediaSample_Release(pSample);
        }

        if (FAILED(hr) && hr != VFW_E_NOT_CONNECTED)
        {
            TRACE("Return %x, stop IFilterGraph\n", hr);
            break;
        }
    }

    heap_free(image_data);
    return 0;
}

static void v4l_device_init_stream(struct video_capture_device *iface)
{
    struct v4l_device *device = v4l_device(iface);
    ALLOCATOR_PROPERTIES req_props, ret_props;
    HRESULT hr;

    req_props.cBuffers = 3;
    req_props.cbBuffer = device->image_size;
    req_props.cbAlign = 1;
    req_props.cbPrefix = 0;

    hr = IMemAllocator_SetProperties(device->pin->pAllocator, &req_props, &ret_props);
    if (FAILED(hr))
        ERR("Failed to set allocator properties (buffer size %u), hr %#x.\n", req_props.cbBuffer, hr);

    if (SUCCEEDED(hr))
    {
        if (FAILED(hr = IMemAllocator_Commit(device->pin->pAllocator)))
            ERR("Failed to commit allocator, hr %#x.\n", hr);
    }

    device->state = State_Paused;
    device->thread = CreateThread(NULL, 0, ReadThread, device, 0, NULL);
}

static void v4l_device_start_stream(struct video_capture_device *iface)
{
    struct v4l_device *device = v4l_device(iface);
    EnterCriticalSection(&device->state_cs);
    device->state = State_Running;
    LeaveCriticalSection(&device->state_cs);
}

static void v4l_device_stop_stream(struct video_capture_device *iface)
{
    struct v4l_device *device = v4l_device(iface);
    EnterCriticalSection(&device->state_cs);
    device->state = State_Paused;
    LeaveCriticalSection(&device->state_cs);
}

static void v4l_device_cleanup_stream(struct video_capture_device *iface)
{
    struct v4l_device *device = v4l_device(iface);
    HRESULT hr;

    EnterCriticalSection(&device->state_cs);
    device->state = State_Stopped;
    LeaveCriticalSection(&device->state_cs);
    WakeConditionVariable(&device->state_cv);
    WaitForSingleObject(device->thread, INFINITE);
    CloseHandle(device->thread);
    device->thread = NULL;

    hr = IMemAllocator_Decommit(device->pin->pAllocator);
    if (hr != S_OK && hr != VFW_E_NOT_COMMITTED)
        ERR("Failed to decommit allocator, hr %#x.\n", hr);
}


static void fill_caps(__u32 pixelformat, __u32 width, __u32 height,
        __u32 max_fps, __u32 min_fps, struct caps *caps)
{
    LONG depth = 24;

    memset(caps, 0, sizeof(*caps));
    caps->video_info.dwBitRate = width * height * depth * max_fps;
    caps->video_info.bmiHeader.biSize = sizeof(caps->video_info.bmiHeader);
    caps->video_info.bmiHeader.biWidth = width;
    caps->video_info.bmiHeader.biHeight = height;
    caps->video_info.bmiHeader.biPlanes = 1;
    caps->video_info.bmiHeader.biBitCount = depth;
    caps->video_info.bmiHeader.biCompression = BI_RGB;
    caps->video_info.bmiHeader.biSizeImage = width * height * depth / 8;
    caps->media_type.majortype = MEDIATYPE_Video;
    caps->media_type.subtype = MEDIASUBTYPE_RGB24;
    caps->media_type.bFixedSizeSamples = TRUE;
    caps->media_type.bTemporalCompression = FALSE;
    caps->media_type.lSampleSize = width * height * depth / 8;
    caps->media_type.formattype = FORMAT_VideoInfo;
    caps->media_type.pUnk = NULL;
    caps->media_type.cbFormat = sizeof(VIDEOINFOHEADER);
    /* We reallocate the caps array, so pbFormat has to be set after all caps
     * have been enumerated. */
    caps->config.MaxFrameInterval = 10000000 * max_fps;
    caps->config.MinFrameInterval = 10000000 * min_fps;
    caps->config.MaxOutputSize.cx = width;
    caps->config.MaxOutputSize.cy = height;
    caps->config.MinOutputSize.cx = width;
    caps->config.MinOutputSize.cy = height;
    caps->config.guid = FORMAT_VideoInfo;
    caps->config.MinBitsPerSecond = width * height * depth * min_fps;
    caps->config.MaxBitsPerSecond = width * height * depth * max_fps;
    caps->pixelformat = pixelformat;
}

static HRESULT v4l_device_get_caps(struct video_capture_device *iface, LONG index,
        AM_MEDIA_TYPE **type, VIDEO_STREAM_CONFIG_CAPS *vscc)
{
    struct v4l_device *device = v4l_device(iface);

    if (index >= device->caps_count)
        return S_FALSE;

    *type = CreateMediaType(&device->caps[index].media_type);
    if (!*type)
        return E_OUTOFMEMORY;

    if (vscc)
        memcpy(vscc, &device->caps[index].config, sizeof(VIDEO_STREAM_CONFIG_CAPS));
    return S_OK;
}

static LONG v4l_device_get_caps_count(struct video_capture_device *iface)
{
    struct v4l_device *device = v4l_device(iface);

    return device->caps_count;
}

static const struct video_capture_device_ops v4l_device_ops =
{
    .destroy = v4l_device_destroy,
    .check_format = v4l_device_check_format,
    .set_format = v4l_device_set_format,
    .get_format = v4l_device_get_format,
    .get_caps = v4l_device_get_caps,
    .get_caps_count = v4l_device_get_caps_count,
    .get_prop_range = v4l_device_get_prop_range,
    .get_prop = v4l_device_get_prop,
    .set_prop = v4l_device_set_prop,
    .init_stream = v4l_device_init_stream,
    .start_stream = v4l_device_start_stream,
    .stop_stream = v4l_device_stop_stream,
    .cleanup_stream = v4l_device_cleanup_stream,
};

struct video_capture_device *v4l_device_create(struct strmbase_source *pin, USHORT card)
{
    struct v4l2_frmsizeenum frmsize = {0};
    struct v4l2_capability caps = {{0}};
    struct v4l2_format format = {0};
    struct v4l_device *device;
    BOOL have_libv4l2;
    char path[20];
    int fd, i;

    have_libv4l2 = video_init();

    if (!(device = heap_alloc_zero(sizeof(*device))))
        return NULL;

    sprintf(path, "/dev/video%i", card);
    TRACE("Opening device %s.\n", path);
#ifdef O_CLOEXEC
    if ((fd = video_open(path, O_RDWR | O_NONBLOCK | O_CLOEXEC)) == -1 && errno == EINVAL)
#endif
        fd = video_open(path, O_RDWR | O_NONBLOCK);
    if (fd == -1)
    {
        WARN("Failed to open video device: %s\n", strerror(errno));
        goto error;
    }
    fcntl(fd, F_SETFD, FD_CLOEXEC);  /* in case O_CLOEXEC isn't supported */
    device->fd = fd;

    if (xioctl(fd, VIDIOC_QUERYCAP, &caps) == -1)
    {
        WARN("Failed to query device capabilities: %s\n", strerror(errno));
        goto error;
    }

#ifdef V4L2_CAP_DEVICE_CAPS
    if (caps.capabilities & V4L2_CAP_DEVICE_CAPS)
        caps.capabilities = caps.device_caps;
#endif

    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        WARN("Device does not support single-planar video capture.\n");
        goto error;
    }

    if (!(caps.capabilities & V4L2_CAP_READWRITE))
    {
        WARN("Device does not support read().\n");
        if (!have_libv4l2)
#ifdef SONAME_LIBV4L2
            ERR_(winediag)("Reading from %s requires libv4l2, but it could not be loaded.\n", path);
#else
            ERR_(winediag)("Reading from %s requires libv4l2, but Wine was compiled without libv4l2 support.\n", path);
#endif
        goto error;
    }

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_G_FMT, &format) == -1)
    {
        ERR("Failed to get device format: %s\n", strerror(errno));
        goto error;
    }

    format.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
    if (xioctl(fd, VIDIOC_TRY_FMT, &format) == -1
            || format.fmt.pix.pixelformat != V4L2_PIX_FMT_BGR24)
    {
        ERR("This device doesn't support V4L2_PIX_FMT_BGR24 format.\n");
        goto error;
    }

    frmsize.pixel_format = V4L2_PIX_FMT_BGR24;
    while (xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) != -1)
    {
        struct v4l2_frmivalenum frmival = {0};
        __u32 max_fps = 30, min_fps = 30;
        struct caps *new_caps;

        frmival.pixel_format = format.fmt.pix.pixelformat;
        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
        {
            frmival.width = frmsize.discrete.width;
            frmival.height = frmsize.discrete.height;
        }
        else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE)
        {
            frmival.width = frmsize.stepwise.max_width;
            frmival.height = frmsize.stepwise.min_height;
        }
        else
        {
            FIXME("Unhandled frame size type: %d.\n", frmsize.type);
            continue;
        }

        if (xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) != -1)
        {
            if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE)
            {
                max_fps = frmival.discrete.denominator / frmival.discrete.numerator;
                min_fps = max_fps;
            }
            else if (frmival.type == V4L2_FRMIVAL_TYPE_STEPWISE
                    || frmival.type == V4L2_FRMIVAL_TYPE_CONTINUOUS)
            {
                max_fps = frmival.stepwise.max.denominator / frmival.stepwise.max.numerator;
                min_fps = frmival.stepwise.min.denominator / frmival.stepwise.min.numerator;
            }
        }
        else
            ERR("Failed to get fps: %s.\n", strerror(errno));

        new_caps = heap_realloc(device->caps, (device->caps_count + 1) * sizeof(*device->caps));
        if (!new_caps)
            goto error;
        device->caps = new_caps;
        fill_caps(format.fmt.pix.pixelformat, frmsize.discrete.width, frmsize.discrete.height,
                max_fps, min_fps, &device->caps[device->caps_count]);
        device->caps_count++;

        frmsize.index++;
    }

    /* We reallocate the caps array, so we have to delay setting pbFormat. */
    for (i = 0; i < device->caps_count; ++i)
        device->caps[i].media_type.pbFormat = (BYTE *)&device->caps[i].video_info;

    if (!set_caps(device, &device->caps[0]))
    {
        ERR("Failed to set pixel format: %s\n", strerror(errno));
        if (!have_libv4l2)
            ERR_(winediag)("You may need libv4l2 to use this device.\n");
        goto error;
    }

    device->d.ops = &v4l_device_ops;
    device->pin = pin;
    device->state = State_Stopped;
    InitializeConditionVariable(&device->state_cv);
    InitializeCriticalSection(&device->state_cs);
    device->state_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": v4l_device.state_cs");

    TRACE("Format: %d bpp - %dx%d.\n", device->current_caps->video_info.bmiHeader.biBitCount,
            device->current_caps->video_info.bmiHeader.biWidth,
            device->current_caps->video_info.bmiHeader.biHeight);

    return &device->d;

error:
    v4l_device_destroy(&device->d);
    return NULL;
}

#else

struct video_capture_device *v4l_device_create(struct strmbase_source *pin, USHORT card)
{
    ERR("v4l2 was not present at compilation time.\n");
    return NULL;
}

#endif /* defined(VIDIOCMCAPTURE) */
