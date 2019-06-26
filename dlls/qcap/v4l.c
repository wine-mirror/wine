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

#define COBJMACROS

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

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"
#include "vfwmsgs.h"
#include "amvideo.h"
#include "wine/debug.h"
#include "wine/library.h"

#include "qcap_main.h"
#include "capture.h"

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
    if (!(video_lib = wine_dlopen(SONAME_LIBV4L2, RTLD_NOW, NULL, 0)))
        return FALSE;
    video_open = wine_dlsym(video_lib, "v4l2_open", NULL, 0);
    video_close = wine_dlsym(video_lib, "v4l2_close", NULL, 0);
    video_ioctl = wine_dlsym(video_lib, "v4l2_ioctl", NULL, 0);
    video_read = wine_dlsym(video_lib, "v4l2_read", NULL, 0);

    return TRUE;
#else
    return FALSE;
#endif
}

struct _Capture
{
    UINT width, height, bitDepth, fps, outputwidth, outputheight;
    BOOL swresize;

    CRITICAL_SECTION CritSect;

    BaseOutputPin *pin;
    int fd, mmap;
    BOOL iscommitted, stopped;

    HANDLE thread;
};

static int xioctl(int fd, int request, void * arg)
{
    int r;

    do {
        r = video_ioctl (fd, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

HRESULT qcap_driver_destroy(Capture *capBox)
{
    TRACE("%p\n", capBox);

    if( capBox->fd != -1 )
        video_close(capBox->fd);
    capBox->CritSect.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&capBox->CritSect);
    CoTaskMemFree(capBox);
    return S_OK;
}

HRESULT qcap_driver_check_format(Capture *device, const AM_MEDIA_TYPE *mt)
{
    HRESULT hr;
    TRACE("device %p, mt %p.\n", device, mt);
    dump_AM_MEDIA_TYPE(mt);

    if (!mt)
        return E_POINTER;

    if (!IsEqualGUID(&mt->majortype, &MEDIATYPE_Video))
        return S_FALSE;

    if (IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo) && mt->pbFormat
            && mt->cbFormat >= sizeof(VIDEOINFOHEADER))
    {
        VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)mt->pbFormat;
        if (vih->bmiHeader.biBitCount == 24 && vih->bmiHeader.biCompression == BI_RGB)
            hr = S_OK;
        else
        {
            FIXME("Unsupported compression %#x, bpp %u.\n", vih->bmiHeader.biCompression,
                    vih->bmiHeader.biBitCount);
            hr = S_FALSE;
        }
    }
    else
        hr = VFW_E_INVALIDMEDIATYPE;

    return hr;
}

HRESULT qcap_driver_set_format(Capture *device, AM_MEDIA_TYPE *mt)
{
    struct v4l2_format format = {0};
    int newheight, newwidth;
    VIDEOINFOHEADER *vih;
    int fd = device->fd;
    HRESULT hr;

    if (FAILED(hr = qcap_driver_check_format(device, mt)))
        return hr;
    vih = (VIDEOINFOHEADER *)mt->pbFormat;

    newwidth = vih->bmiHeader.biWidth;
    newheight = vih->bmiHeader.biHeight;

    if (device->height == newheight && device->width == newwidth)
        return S_OK;

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_G_FMT, &format) == -1)
    {
        ERR("Failed to get current format: %s\n", strerror(errno));
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    format.fmt.pix.width = newwidth;
    format.fmt.pix.height = newheight;

    if (!xioctl(fd, VIDIOC_S_FMT, &format)
            && format.fmt.pix.width == newwidth
            && format.fmt.pix.height == newheight)
    {
        device->width = newwidth;
        device->height = newheight;
        device->swresize = FALSE;
    }
    else
    {
        TRACE("Using software resize: %dx%d -> %dx%d.\n",
               format.fmt.pix.width, format.fmt.pix.height, device->width, device->height);
        device->swresize = TRUE;
    }
    device->outputwidth = format.fmt.pix.width;
    device->outputheight = format.fmt.pix.height;
    return S_OK;
}

HRESULT qcap_driver_get_format(const Capture *capBox, AM_MEDIA_TYPE ** mT)
{
    VIDEOINFOHEADER *vi;

    mT[0] = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (!mT[0])
        return E_OUTOFMEMORY;
    vi = CoTaskMemAlloc(sizeof(VIDEOINFOHEADER));
    mT[0]->cbFormat = sizeof(VIDEOINFOHEADER);
    if (!vi)
    {
        CoTaskMemFree(mT[0]);
        mT[0] = NULL;
        return E_OUTOFMEMORY;
    }
    mT[0]->majortype = MEDIATYPE_Video;
    mT[0]->subtype = MEDIASUBTYPE_RGB24;
    mT[0]->formattype = FORMAT_VideoInfo;
    mT[0]->bFixedSizeSamples = TRUE;
    mT[0]->bTemporalCompression = FALSE;
    mT[0]->pUnk = NULL;
    mT[0]->lSampleSize = capBox->outputwidth * capBox->outputheight * capBox->bitDepth / 8;
    TRACE("Output format: %dx%d - %d bits = %u KB\n", capBox->outputwidth,
          capBox->outputheight, capBox->bitDepth, mT[0]->lSampleSize/1024);
    vi->rcSource.left = 0; vi->rcSource.top = 0;
    vi->rcTarget.left = 0; vi->rcTarget.top = 0;
    vi->rcSource.right = capBox->width; vi->rcSource.bottom = capBox->height;
    vi->rcTarget.right = capBox->outputwidth; vi->rcTarget.bottom = capBox->outputheight;
    vi->dwBitRate = capBox->fps * mT[0]->lSampleSize;
    vi->dwBitErrorRate = 0;
    vi->AvgTimePerFrame = (LONGLONG)10000000.0 / (LONGLONG)capBox->fps;
    vi->bmiHeader.biSize = 40;
    vi->bmiHeader.biWidth = capBox->outputwidth;
    vi->bmiHeader.biHeight = capBox->outputheight;
    vi->bmiHeader.biPlanes = 1;
    vi->bmiHeader.biBitCount = 24;
    vi->bmiHeader.biCompression = BI_RGB;
    vi->bmiHeader.biSizeImage = mT[0]->lSampleSize;
    vi->bmiHeader.biClrUsed = vi->bmiHeader.biClrImportant = 0;
    vi->bmiHeader.biXPelsPerMeter = 100;
    vi->bmiHeader.biYPelsPerMeter = 100;
    mT[0]->pbFormat = (void *)vi;
    dump_AM_MEDIA_TYPE(mT[0]);
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

HRESULT qcap_driver_get_prop_range(Capture *device, VideoProcAmpProperty property,
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

HRESULT qcap_driver_get_prop(Capture *device, VideoProcAmpProperty property,
        LONG *value, LONG *flags)
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

HRESULT qcap_driver_set_prop(Capture *device, VideoProcAmpProperty property,
        LONG value, LONG flags)
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

static void Resize(const Capture * capBox, LPBYTE output, const BYTE *input)
{
    /* the whole image needs to be reversed,
       because the dibs are messed up in windows */
    if (!capBox->swresize)
    {
        int depth = capBox->bitDepth / 8;
        int inoffset = 0, outoffset = capBox->height * capBox->width * depth;
        int ow = capBox->width * depth;
        while (outoffset > 0)
        {
            int x;
            outoffset -= ow;
            for (x = 0; x < ow; x++)
                output[outoffset + x] = input[inoffset + x];
            inoffset += ow;
        }
    }
    else
    {
        HDC dc_s, dc_d;
        HBITMAP bmp_s, bmp_d;
        int depth = capBox->bitDepth / 8;
        int inoffset = 0, outoffset = (capBox->outputheight) * capBox->outputwidth * depth;
        int ow = capBox->outputwidth * depth;
        LPBYTE myarray;

        /* FIXME: Improve software resizing: add error checks and optimize */

        myarray = CoTaskMemAlloc(capBox->outputwidth * capBox->outputheight * depth);
        dc_s = CreateCompatibleDC(NULL);
        dc_d = CreateCompatibleDC(NULL);
        bmp_s = CreateBitmap(capBox->width, capBox->height, 1, capBox->bitDepth, input);
        bmp_d = CreateBitmap(capBox->outputwidth, capBox->outputheight, 1, capBox->bitDepth, NULL);
        SelectObject(dc_s, bmp_s);
        SelectObject(dc_d, bmp_d);
        StretchBlt(dc_d, 0, 0, capBox->outputwidth, capBox->outputheight,
                   dc_s, 0, 0, capBox->width, capBox->height, SRCCOPY);
        GetBitmapBits(bmp_d, capBox->outputwidth * capBox->outputheight * depth, myarray);
        while (outoffset > 0)
        {
            int i;

            outoffset -= ow;
            for (i = 0; i < ow; i++)
                output[outoffset + i] = myarray[inoffset + i];
            inoffset += ow;
        }
        CoTaskMemFree(myarray);
        DeleteObject(dc_s);
        DeleteObject(dc_d);
        DeleteObject(bmp_s);
        DeleteObject(bmp_d);
    }
}

static DWORD WINAPI ReadThread(LPVOID lParam)
{
    Capture * capBox = lParam;
    HRESULT hr;
    IMediaSample *pSample = NULL;
    ULONG framecount = 0;
    unsigned char *pTarget, *image_data;
    unsigned int image_size;

    image_size = capBox->height * capBox->width * 3;
    if (!(image_data = heap_alloc(image_size)))
    {
        ERR("Failed to allocate memory.\n");
        capBox->thread = 0;
        capBox->stopped = TRUE;
        return 0;
    }

    while (1)
    {
        EnterCriticalSection(&capBox->CritSect);
        if (capBox->stopped)
            break;
        hr = BaseOutputPinImpl_GetDeliveryBuffer(capBox->pin, &pSample, NULL, NULL, 0);
        if (SUCCEEDED(hr))
        {
            int len;
            
            if (!capBox->swresize)
                len = capBox->height * capBox->width * capBox->bitDepth / 8;
            else
                len = capBox->outputheight * capBox->outputwidth * capBox->bitDepth / 8;
            IMediaSample_SetActualDataLength(pSample, len);

            len = IMediaSample_GetActualDataLength(pSample);
            TRACE("Data length: %d KB\n", len / 1024);

            IMediaSample_GetPointer(pSample, &pTarget);

            while (video_read(capBox->fd, image_data, image_size) == -1)
            {
                if (errno != EAGAIN)
                {
                    ERR("Failed to read frame: %s\n", strerror(errno));
                    break;
                }
            }

            Resize(capBox, pTarget, image_data);
            hr = BaseOutputPinImpl_Deliver(capBox->pin, pSample);
            TRACE("%p -> Frame %u: %x\n", capBox, ++framecount, hr);
            IMediaSample_Release(pSample);
        }
        if (FAILED(hr) && hr != VFW_E_NOT_CONNECTED)
        {
            TRACE("Return %x, stop IFilterGraph\n", hr);
            capBox->thread = 0;
            capBox->stopped = TRUE;
            break;
        }
        LeaveCriticalSection(&capBox->CritSect);
    }

    LeaveCriticalSection(&capBox->CritSect);
    heap_free(image_data);
    return 0;
}

HRESULT qcap_driver_run(Capture *capBox, FILTER_STATE *state)
{
    HANDLE thread;
    HRESULT hr;

    TRACE("%p -> (%p)\n", capBox, state); 

    if (*state == State_Running) return S_OK;

    EnterCriticalSection(&capBox->CritSect);

    capBox->stopped = FALSE;

    if (*state == State_Stopped)
    {
        *state = State_Running;
        if (!capBox->iscommitted)
        {
            ALLOCATOR_PROPERTIES ap, actual;

            capBox->iscommitted = TRUE;

            ap.cBuffers = 3;
            if (!capBox->swresize)
                ap.cbBuffer = capBox->width * capBox->height;
            else
                ap.cbBuffer = capBox->outputwidth * capBox->outputheight;
            ap.cbBuffer = (ap.cbBuffer * capBox->bitDepth) / 8;
            ap.cbAlign = 1;
            ap.cbPrefix = 0;

            hr = IMemAllocator_SetProperties(capBox->pin->pAllocator, &ap, &actual);

            if (SUCCEEDED(hr))
                hr = IMemAllocator_Commit(capBox->pin->pAllocator);

            TRACE("Committing allocator: %x\n", hr);
        }

        thread = CreateThread(NULL, 0, ReadThread, capBox, 0, NULL);
        if (thread)
        {
            capBox->thread = thread;
            SetThreadPriority(thread, THREAD_PRIORITY_LOWEST);
            LeaveCriticalSection(&capBox->CritSect);
            return S_OK;
        }
        ERR("Creating thread failed.. %u\n", GetLastError());
        LeaveCriticalSection(&capBox->CritSect);
        return E_FAIL;
    }

    ResumeThread(capBox->thread);
    *state = State_Running;
    LeaveCriticalSection(&capBox->CritSect);
    return S_OK;
}

HRESULT qcap_driver_pause(Capture *capBox, FILTER_STATE *state)
{
    TRACE("%p -> (%p)\n", capBox, state);     

    if (*state == State_Paused)
        return S_OK;
    if (*state == State_Stopped)
        qcap_driver_run(capBox, state);

    EnterCriticalSection(&capBox->CritSect);
    *state = State_Paused;
    SuspendThread(capBox->thread);
    LeaveCriticalSection(&capBox->CritSect);

    return S_OK;
}

HRESULT qcap_driver_stop(Capture *capBox, FILTER_STATE *state)
{
    TRACE("%p -> (%p)\n", capBox, state);

    if (*state == State_Stopped)
        return S_OK;

    EnterCriticalSection(&capBox->CritSect);

    if (capBox->thread)
    {
        if (*state == State_Paused)
            ResumeThread(capBox->thread);
        capBox->stopped = TRUE;
        capBox->thread = 0;
        if (capBox->iscommitted)
        {
            HRESULT hr;

            capBox->iscommitted = FALSE;

            hr = IMemAllocator_Decommit(capBox->pin->pAllocator);

            if (hr != S_OK && hr != VFW_E_NOT_COMMITTED)
                WARN("Decommitting allocator: %x\n", hr);
        }
    }

    *state = State_Stopped;
    LeaveCriticalSection(&capBox->CritSect);
    return S_OK;
}

Capture * qcap_driver_init(BaseOutputPin *pin, USHORT card)
{
    struct v4l2_capability caps = {{0}};
    struct v4l2_format format = {0};
    Capture *device = NULL;
    BOOL have_libv4l2;
    char path[20];
    int fd;

    have_libv4l2 = video_init();

    if (!(device = CoTaskMemAlloc(sizeof(*device))))
        return NULL;

    InitializeCriticalSection(&device->CritSect);
    device->CritSect.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": Capture.CritSect");

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
    if (xioctl(fd, VIDIOC_S_FMT, &format) == -1
            || format.fmt.pix.pixelformat != V4L2_PIX_FMT_BGR24)
    {
        ERR("Failed to set pixel format: %s\n", strerror(errno));
        if (!have_libv4l2)
            ERR_(winediag)("You may need libv4l2 to use this device.\n");
        goto error;
    }

    device->outputwidth = device->width = format.fmt.pix.width;
    device->outputheight = device->height = format.fmt.pix.height;
    device->swresize = FALSE;
    device->bitDepth = 24;
    device->pin = pin;
    device->fps = 3;
    device->stopped = FALSE;
    device->iscommitted = FALSE;

    TRACE("Format: %d bpp - %dx%d.\n", device->bitDepth, device->width, device->height);

    return device;

error:
    qcap_driver_destroy(device);
    return NULL;
}

#else

Capture * qcap_driver_init(BaseOutputPin *pin, USHORT card)
{
    static const char msg[] =
        "The v4l headers were not available at compile time,\n"
        "so video capture support is not available.\n";
    MESSAGE(msg);
    return NULL;
}

#define FAIL_WITH_ERR \
    ERR("v4l absent: shouldn't be called\n"); \
    return E_NOTIMPL

HRESULT qcap_driver_destroy(Capture *capBox)
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_check_format(Capture *device, const AM_MEDIA_TYPE *mt)
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_set_format(Capture *capBox, AM_MEDIA_TYPE * mT)
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_get_format(const Capture *capBox, AM_MEDIA_TYPE ** mT)
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_get_prop_range( Capture *capBox,
        VideoProcAmpProperty  Property, LONG *pMin, LONG *pMax,
        LONG *pSteppingDelta, LONG *pDefault, LONG *pCapsFlags )
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_get_prop(Capture *capBox,
        VideoProcAmpProperty Property, LONG *lValue, LONG *Flags)
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_set_prop(Capture *capBox, VideoProcAmpProperty Property,
        LONG lValue, LONG Flags)
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_run(Capture *capBox, FILTER_STATE *state)
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_pause(Capture *capBox, FILTER_STATE *state)
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_stop(Capture *capBox, FILTER_STATE *state)
{
    FAIL_WITH_ERR;
}

#endif /* defined(VIDIOCMCAPTURE) */
