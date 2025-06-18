/*
 * Copyright 2002 Dmitry Timoshkov for CodeWeavers
 * Copyright 2005 Maarten Lankhorst
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

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#ifdef HAVE_LINUX_VIDEODEV2_H
# include <linux/videodev2.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"

#include "unixlib.h"

#ifdef HAVE_LINUX_VIDEODEV2_H

WINE_DEFAULT_DEBUG_CHANNEL(avicap);

static int xioctl(int fd, int request, void *arg)
{
    int ret;

    do
        ret = ioctl(fd, request, arg);
    while (ret < 0 && errno == EINTR);

    return ret;
}

static void v4l_umbstowcs(const char *src, DWORD srclen, WCHAR *dst, DWORD dstlen)
{
    DWORD ret = ntdll_umbstowcs(src, srclen, dst, dstlen - 1);

    dst[ret] = 0;
}

static NTSTATUS get_device_desc(void *args)
{
    struct get_device_desc_params *params = args;
    struct v4l2_capability caps = {{0}};
    char device[16];
    struct stat st;
    int fd;

    snprintf(device, sizeof(device), "/dev/video%u", params->index);

    if (stat(device, &st) < 0)
    {
        /* This is probably because the device does not exist */
        WARN("Failed to stat %s: %s\n", device, strerror(errno));
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if (!S_ISCHR(st.st_mode))
    {
        ERR("%s is not a character device.\n", device);
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    if ((fd = open(device, O_RDWR | O_NONBLOCK)) < 0)
    {
        ERR("%s: Failed to open: %s\n", device, strerror(errno));
        return STATUS_UNSUCCESSFUL;
    }

    if (!xioctl(fd, VIDIOC_QUERYCAP, &caps))
    {
        BOOL is_capture_device;
#ifdef V4L2_CAP_DEVICE_CAPS
        if (caps.capabilities & V4L2_CAP_DEVICE_CAPS)
            is_capture_device = caps.device_caps & V4L2_CAP_VIDEO_CAPTURE;
        else
#endif
            is_capture_device = caps.capabilities & V4L2_CAP_VIDEO_CAPTURE;
        if (is_capture_device)
        {
            char version[CAP_DESC_MAX];
            int ret;

            v4l_umbstowcs((char *)caps.card, strlen((char *)caps.card),
                    params->name, ARRAY_SIZE(params->name));

            ret = snprintf(version, ARRAY_SIZE(version), "%s v%u.%u.%u", (char *)caps.driver,
                    (caps.version >> 16) & 0xff, (caps.version >> 8) & 0xff, caps.version & 0xff);
            v4l_umbstowcs(version, ret, params->version, ARRAY_SIZE(params->version));
        }
        close(fd);
        return is_capture_device ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    }

    /* errno 515 is used by some webcam drivers for unknown IOCTL commands. */
    ERR("Failed to get capabilities for %s: %s\n", device, strerror(errno));

    close(fd);
    return STATUS_UNSUCCESSFUL;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    get_device_desc,
};

#ifdef _WIN64

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    get_device_desc,
};

#endif  /* _WIN64 */

#endif /* HAVE_LINUX_VIDEODEV2_H */
