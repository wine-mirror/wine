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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#include <errno.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_ASM_TYPES_H
# include <asm/types.h>
#endif
#ifdef HAVE_LINUX_VIDEODEV2_H
# include <linux/videodev2.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "vfw.h"
#include "winnls.h"
#include "winternl.h"
#include "wine/debug.h"

#define CAP_DESC_MAX 32

WINE_DEFAULT_DEBUG_CHANNEL(avicap);


/***********************************************************************
 *             capCreateCaptureWindowW   (AVICAP32.@)
 */
HWND VFWAPI capCreateCaptureWindowW(LPCWSTR lpszWindowName, DWORD dwStyle, INT x,
                                    INT y, INT nWidth, INT nHeight, HWND hWnd,
                                    INT nID)
{
    FIXME("(%s, %08x, %08x, %08x, %08x, %08x, %p, %08x): stub\n",
           debugstr_w(lpszWindowName), dwStyle, x, y, nWidth, nHeight, hWnd, nID);
    return 0;
}

/***********************************************************************
 *             capCreateCaptureWindowA   (AVICAP32.@)
 */
HWND VFWAPI capCreateCaptureWindowA(LPCSTR lpszWindowName, DWORD dwStyle, INT x,
                                    INT y, INT nWidth, INT nHeight, HWND hWnd,
                                    INT nID)
{   UNICODE_STRING nameW;
    HWND retW;

    if (lpszWindowName) RtlCreateUnicodeStringFromAsciiz(&nameW, lpszWindowName);
    else nameW.Buffer = NULL;

    retW = capCreateCaptureWindowW(nameW.Buffer, dwStyle, x, y, nWidth, nHeight,
                                   hWnd, nID);
    RtlFreeUnicodeString(&nameW);

    return retW;
}

#ifdef HAVE_LINUX_VIDEODEV2_H

static int xioctl(int fd, int request, void * arg)
{
   int r;

   do r = ioctl (fd, request, arg);
   while (-1 == r && EINTR == errno);

   return r;
}

static BOOL query_video_device(int devnum, char *name, int namesize, char *version, int versionsize)
{
   int fd;
   char device[16];
   struct stat st;
   struct v4l2_capability caps;

   snprintf(device, sizeof(device), "/dev/video%i", devnum);

   if (stat (device, &st) == -1) {
      /* This is probably because the device does not exist */
      WARN("%s: %s\n", device, strerror(errno));
      return FALSE;
   }

   if (!S_ISCHR (st.st_mode)) {
      ERR("%s: Not a device\n", device);
      return FALSE;
   }

   fd = open(device, O_RDWR | O_NONBLOCK);
   if (fd == -1) {
      ERR("%s: Failed to open: %s\n", device, strerror(errno));
      return FALSE;
   }

   memset(&caps, 0, sizeof(caps));
   if (xioctl(fd, VIDIOC_QUERYCAP, &caps) != -1) {
      BOOL isCaptureDevice;
#ifdef V4L2_CAP_DEVICE_CAPS
      if (caps.capabilities & V4L2_CAP_DEVICE_CAPS)
         isCaptureDevice = caps.device_caps & V4L2_CAP_VIDEO_CAPTURE;
      else
#endif
         isCaptureDevice = caps.capabilities & V4L2_CAP_VIDEO_CAPTURE;
      if (isCaptureDevice) {
         lstrcpynA(name, (char *)caps.card, namesize);
         snprintf(version, versionsize, "%s v%u.%u.%u", (char *)caps.driver, (caps.version >> 16) & 0xFF,
                             (caps.version >> 8) & 0xFF, caps.version & 0xFF);
      }
      close(fd);
      return isCaptureDevice;
   }

   if (errno != EINVAL && errno != 515)
/* errno 515 is used by some webcam drivers for unknown IOICTL command */
      ERR("%s: Querying failed: %s\n", device, strerror(errno));
   else ERR("%s: Querying failed: Not a V4L compatible device\n", device);

   close(fd);
   return FALSE;
}

#else /* HAVE_LINUX_VIDEODEV2_H */

static BOOL query_video_device(int devnum, char *name, int namesize, char *version, int versionsize)
{
   ERR("Video 4 Linux support not enabled\n");
   return FALSE;
}

#endif /* HAVE_LINUX_VIDEODEV2_H */

/***********************************************************************
 *             capGetDriverDescriptionA   (AVICAP32.@)
 */
BOOL VFWAPI capGetDriverDescriptionA(WORD wDriverIndex, LPSTR lpszName,
                                     INT cbName, LPSTR lpszVer, INT cbVer)
{
   BOOL retval;
   WCHAR devname[CAP_DESC_MAX], devver[CAP_DESC_MAX];
   TRACE("--> capGetDriverDescriptionW\n");
   retval = capGetDriverDescriptionW(wDriverIndex, devname, CAP_DESC_MAX, devver, CAP_DESC_MAX);
   if (retval) {
      WideCharToMultiByte(CP_ACP, 0, devname, -1, lpszName, cbName, NULL, NULL);
      WideCharToMultiByte(CP_ACP, 0, devver, -1, lpszVer, cbVer, NULL, NULL);
   }
   return retval;
}

/***********************************************************************
 *             capGetDriverDescriptionW   (AVICAP32.@)
 */
BOOL VFWAPI capGetDriverDescriptionW(WORD wDriverIndex, LPWSTR lpszName,
                                     INT cbName, LPWSTR lpszVer, INT cbVer)
{
   char devname[CAP_DESC_MAX], devver[CAP_DESC_MAX];

   if (!query_video_device(wDriverIndex, devname, CAP_DESC_MAX, devver, CAP_DESC_MAX)) return FALSE;

   MultiByteToWideChar(CP_UNIXCP, 0, devname, -1, lpszName, cbName);
   MultiByteToWideChar(CP_UNIXCP, 0, devver, -1, lpszVer, cbVer);
   TRACE("Version: %s - Name: %s\n", debugstr_w(lpszVer), debugstr_w(lpszName));
   return TRUE;
}
