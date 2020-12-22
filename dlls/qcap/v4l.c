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

#if 0
#pragma makedep unix
#endif

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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "initguid.h"
#include "qcap_private.h"
#include "winternl.h"

#ifdef HAVE_LINUX_VIDEODEV2_H

WINE_DEFAULT_DEBUG_CHANNEL(qcap);
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

struct video_capture_device
{
    const struct caps *current_caps;
    struct caps *caps;
    LONG caps_count;

    int image_size, image_pitch;
    BYTE *image_data;

    int fd, mmap;
};

static int xioctl(int fd, int request, void * arg)
{
    int r;

    do {
        r = video_ioctl (fd, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

static void CDECL v4l_device_destroy(struct video_capture_device *device)
{
    if (device->fd != -1)
        video_close(device->fd);
    if (device->caps_count)
        free(device->caps);
    free(device->image_data);
    free(device);
}

static const struct caps *find_caps(struct video_capture_device *device, const AM_MEDIA_TYPE *mt)
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

static HRESULT CDECL v4l_device_check_format(struct video_capture_device *device, const AM_MEDIA_TYPE *mt)
{
    TRACE("device %p, mt %p.\n", device, mt);

    if (!mt)
        return E_POINTER;

    if (!IsEqualGUID(&mt->majortype, &MEDIATYPE_Video))
        return E_FAIL;

    if (find_caps(device, mt))
        return S_OK;

    return E_FAIL;
}

static HRESULT set_caps(struct video_capture_device *device, const struct caps *caps)
{
    struct v4l2_format format = {0};
    LONG width, height, image_size;
    BYTE *image_data;

    width = caps->video_info.bmiHeader.biWidth;
    height = caps->video_info.bmiHeader.biHeight;
    image_size = width * height * caps->video_info.bmiHeader.biBitCount / 8;

    if (!(image_data = malloc(image_size)))
    {
        ERR("Failed to allocate memory.\n");
        return E_OUTOFMEMORY;
    }

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
        free(image_data);
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    device->current_caps = caps;
    device->image_size = image_size;
    device->image_pitch = width * caps->video_info.bmiHeader.biBitCount / 8;
    free(device->image_data);
    device->image_data = image_data;
    return S_OK;
}

static HRESULT CDECL v4l_device_set_format(struct video_capture_device *device, const AM_MEDIA_TYPE *mt)
{
    const struct caps *caps;

    caps = find_caps(device, mt);
    if (!caps)
        return E_FAIL;

    if (device->current_caps == caps)
        return S_OK;

    return set_caps(device, caps);
}

static void CDECL v4l_device_get_format(struct video_capture_device *device, AM_MEDIA_TYPE *mt, VIDEOINFOHEADER *format)
{
    *mt = device->current_caps->media_type;
    *format = device->current_caps->video_info;
}

static HRESULT CDECL v4l_device_get_media_type(struct video_capture_device *device,
        unsigned int index, AM_MEDIA_TYPE *mt, VIDEOINFOHEADER *format)
{
    unsigned int caps_count = (device->current_caps) ? 1 : device->caps_count;

    if (index >= caps_count)
        return VFW_S_NO_MORE_ITEMS;

    if (device->current_caps)
    {
        *mt = device->current_caps->media_type;
        *format = device->current_caps->video_info;
    }
    else
    {
        *mt = device->caps[index].media_type;
        *format = device->caps[index].video_info;
    }
    return S_OK;
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

static HRESULT CDECL v4l_device_get_prop_range(struct video_capture_device *device, VideoProcAmpProperty property,
        LONG *min, LONG *max, LONG *step, LONG *default_value, LONG *flags)
{
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

static HRESULT CDECL v4l_device_get_prop(struct video_capture_device *device,
        VideoProcAmpProperty property, LONG *value, LONG *flags)
{
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

static HRESULT CDECL v4l_device_set_prop(struct video_capture_device *device,
        VideoProcAmpProperty property, LONG value, LONG flags)
{
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

static void reverse_image(struct video_capture_device *device, LPBYTE output, const BYTE *input)
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

static BOOL CDECL v4l_device_read_frame(struct video_capture_device *device, BYTE *data)
{
    while (video_read(device->fd, device->image_data, device->image_size) < 0)
    {
        if (errno != EAGAIN)
        {
            ERR("Failed to read frame: %s\n", strerror(errno));
            return FALSE;
        }
    }

    reverse_image(device, data, device->image_data);
    return TRUE;
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

static void CDECL v4l_device_get_caps(struct video_capture_device *device, LONG index,
        AM_MEDIA_TYPE *type, VIDEOINFOHEADER *format, VIDEO_STREAM_CONFIG_CAPS *vscc)
{
    *vscc = device->caps[index].config;
    *type = device->caps[index].media_type;
    *format = device->caps[index].video_info;
}

static LONG CDECL v4l_device_get_caps_count(struct video_capture_device *device)
{
    return device->caps_count;
}

struct video_capture_device * CDECL v4l_device_create(USHORT index)
{
    struct v4l2_frmsizeenum frmsize = {0};
    struct video_capture_device *device;
    struct v4l2_capability caps = {{0}};
    struct v4l2_format format = {0};
    BOOL have_libv4l2;
    char path[20];
    HRESULT hr;
    int fd, i;

    have_libv4l2 = video_init();

    if (!(device = calloc(1, sizeof(*device))))
        return NULL;

    sprintf(path, "/dev/video%i", index);
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

        new_caps = realloc(device->caps, (device->caps_count + 1) * sizeof(*device->caps));
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

    if (FAILED(hr = set_caps(device, &device->caps[0])))
    {
        if (hr == VFW_E_TYPE_NOT_ACCEPTED && !have_libv4l2)
            ERR_(winediag)("You may need libv4l2 to use this device.\n");
        goto error;
    }

    TRACE("Format: %d bpp - %dx%d.\n", device->current_caps->video_info.bmiHeader.biBitCount,
            device->current_caps->video_info.bmiHeader.biWidth,
            device->current_caps->video_info.bmiHeader.biHeight);

    return device;

error:
    v4l_device_destroy(device);
    return NULL;
}

const struct video_capture_funcs v4l_funcs =
{
    .create = v4l_device_create,
    .destroy = v4l_device_destroy,
    .check_format = v4l_device_check_format,
    .set_format = v4l_device_set_format,
    .get_format = v4l_device_get_format,
    .get_media_type = v4l_device_get_media_type,
    .get_caps = v4l_device_get_caps,
    .get_caps_count = v4l_device_get_caps_count,
    .get_prop_range = v4l_device_get_prop_range,
    .get_prop = v4l_device_get_prop,
    .set_prop = v4l_device_set_prop,
    .read_frame = v4l_device_read_frame,
};

NTSTATUS CDECL __wine_init_unix_lib(HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out)
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;

    *(const struct video_capture_funcs **)ptr_out = &v4l_funcs;
    return STATUS_SUCCESS;
}

#endif /* HAVE_LINUX_VIDEODEV2_H */
