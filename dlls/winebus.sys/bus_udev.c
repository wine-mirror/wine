/*
 * Plug and Play support for hid devices found through udev
 *
 * Copyright 2016 CodeWeavers, Aric Stewart
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
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_POLL_H
# include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif
#ifdef HAVE_LIBUDEV_H
# include <libudev.h>
#endif

#define NONAMELESSUNION

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "wine/debug.h"

#include "bus.h"

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);

#ifdef HAVE_UDEV

static struct udev *udev_context = NULL;
static DRIVER_OBJECT *udev_driver_obj = NULL;

static const WCHAR hidraw_busidW[] = {'H','I','D','R','A','W',0};

#include "initguid.h"
DEFINE_GUID(GUID_DEVCLASS_HIDRAW, 0x3def44ad,0x242e,0x46e5,0x82,0x6d,0x70,0x72,0x13,0xf3,0xaa,0x81);

struct platform_private
{
    struct udev_device *udev_device;
};

static inline struct platform_private *impl_from_DEVICE_OBJECT(DEVICE_OBJECT *device)
{
    return (struct platform_private *)get_platform_private(device);
}

static DWORD get_sysattr_dword(struct udev_device *dev, const char *sysattr, int base)
{
    const char *attr = udev_device_get_sysattr_value(dev, sysattr);
    if (!attr)
    {
        WARN("Could not get %s from device\n", sysattr);
        return 0;
    }
    return strtol(attr, NULL, base);
}

static WCHAR *get_sysattr_string(struct udev_device *dev, const char *sysattr)
{
    const char *attr = udev_device_get_sysattr_value(dev, sysattr);
    WCHAR *dst;
    DWORD len;
    if (!attr)
    {
        WARN("Could not get %s from device\n", sysattr);
        return NULL;
    }
    len = MultiByteToWideChar(CP_UNIXCP, 0, attr, -1, NULL, 0);
    if ((dst = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR))))
        MultiByteToWideChar(CP_UNIXCP, 0, attr, -1, dst, len);
    return dst;
}

static int compare_platform_device(DEVICE_OBJECT *device, void *platform_dev)
{
    struct udev_device *dev1 = impl_from_DEVICE_OBJECT(device)->udev_device;
    struct udev_device *dev2 = platform_dev;
    return strcmp(udev_device_get_syspath(dev1), udev_device_get_syspath(dev2));
}

static const platform_vtbl hidraw_vtbl =
{
    compare_platform_device,
};

static void try_add_device(struct udev_device *dev)
{
    DWORD vid = 0, pid = 0, version = 0;
    struct udev_device *usbdev;
    DEVICE_OBJECT *device = NULL;
    const char *subsystem;
    const char *devnode;
    WCHAR *serial = NULL;
    int fd;

    if (!(devnode = udev_device_get_devnode(dev)))
        return;

    if ((fd = open(devnode, O_RDWR)) == -1)
    {
        WARN("Unable to open udev device %s: %s\n", debugstr_a(devnode), strerror(errno));
        return;
    }
    close(fd);

    usbdev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
    if (usbdev)
    {
        vid     = get_sysattr_dword(usbdev, "idVendor", 16);
        pid     = get_sysattr_dword(usbdev, "idProduct", 16);
        version = get_sysattr_dword(usbdev, "version", 10);
        serial  = get_sysattr_string(usbdev, "serial");
    }

    TRACE("Found udev device %s (vid %04x, pid %04x, version %u, serial %s)\n",
          debugstr_a(devnode), vid, pid, version, debugstr_w(serial));

    subsystem = udev_device_get_subsystem(dev);
    if (strcmp(subsystem, "hidraw") == 0)
    {
        device = bus_create_hid_device(udev_driver_obj, hidraw_busidW, vid, pid, version, 0, serial, FALSE,
                                       &GUID_DEVCLASS_HIDRAW, &hidraw_vtbl, sizeof(struct platform_private));
    }

    if (device)
    {
        impl_from_DEVICE_OBJECT(device)->udev_device = udev_device_ref(dev);
        IoInvalidateDeviceRelations(device, BusRelations);
    }
    else
        WARN("Ignoring device %s with subsystem %s\n", debugstr_a(devnode), subsystem);

    HeapFree(GetProcessHeap(), 0, serial);
}

static void try_remove_device(struct udev_device *dev)
{
    DEVICE_OBJECT *device = bus_find_hid_device(&hidraw_vtbl, dev);
    if (!device) return;

    dev = impl_from_DEVICE_OBJECT(device)->udev_device;
    bus_remove_hid_device(device);
    udev_device_unref(dev);
}

static void build_initial_deviceset(void)
{
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;

    enumerate = udev_enumerate_new(udev_context);
    if (!enumerate)
    {
        WARN("Unable to create udev enumeration object\n");
        return;
    }

    if (udev_enumerate_add_match_subsystem(enumerate, "hidraw") < 0)
        WARN("Failed to add subsystem 'hidraw' to enumeration\n");

    if (udev_enumerate_scan_devices(enumerate) < 0)
        WARN("Enumeration scan failed\n");

    devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devices)
    {
        struct udev_device *dev;
        const char *path;

        path = udev_list_entry_get_name(dev_list_entry);
        if ((dev = udev_device_new_from_syspath(udev_context, path)))
        {
            try_add_device(dev);
            udev_device_unref(dev);
        }
    }

    udev_enumerate_unref(enumerate);
}

static struct udev_monitor *create_monitor(struct pollfd *pfd)
{
    struct udev_monitor *monitor;

    monitor = udev_monitor_new_from_netlink(udev_context, "udev");
    if (!monitor)
    {
        WARN("Unable to get udev monitor object\n");
        return NULL;
    }

    if (udev_monitor_filter_add_match_subsystem_devtype(monitor, "hidraw", NULL) < 0)
        WARN("Failed to add subsystem 'hidraw' to monitor\n");

    if (udev_monitor_enable_receiving(monitor) < 0)
        goto error;

    if ((pfd->fd = udev_monitor_get_fd(monitor)) >= 0)
    {
        pfd->events = POLLIN;
        return monitor;
    }

error:
    WARN("Failed to start monitoring\n");
    udev_monitor_unref(monitor);
    return NULL;
}

static void process_monitor_event(struct udev_monitor *monitor)
{
    struct udev_device *dev;
    const char *action;

    dev = udev_monitor_receive_device(monitor);
    if (!dev)
    {
        FIXME("Failed to get device that has changed\n");
        return;
    }

    action = udev_device_get_action(dev);
    TRACE("Received action %s for udev device %s\n", debugstr_a(action),
          debugstr_a(udev_device_get_devnode(dev)));

    if (!action)
        WARN("No action received\n");
    else if (strcmp(action, "add") == 0)
        try_add_device(dev);
    else if (strcmp(action, "remove") == 0)
        try_remove_device(dev);
    else
        WARN("Unhandled action %s\n", debugstr_a(action));

    udev_device_unref(dev);
}

static DWORD CALLBACK deviceloop_thread(void *args)
{
    struct udev_monitor *monitor;
    HANDLE init_done = args;
    struct pollfd pfd;

    monitor = create_monitor(&pfd);
    build_initial_deviceset();
    SetEvent(init_done);

    while (monitor)
    {
        if (poll(&pfd, 1, -1) <= 0) continue;
        process_monitor_event(monitor);
    }

    TRACE("Monitor thread exiting\n");
    if (monitor)
        udev_monitor_unref(monitor);
    return 0;
}

NTSTATUS WINAPI udev_driver_init(DRIVER_OBJECT *driver, UNICODE_STRING *registry_path)
{
    HANDLE events[2];
    DWORD result;

    TRACE("(%p, %s)\n", driver, debugstr_w(registry_path->Buffer));

    if (!(udev_context = udev_new()))
    {
        ERR("Can't create udev object\n");
        return STATUS_UNSUCCESSFUL;
    }

    udev_driver_obj = driver;
    driver->MajorFunction[IRP_MJ_PNP] = common_pnp_dispatch;
    driver->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = hid_internal_dispatch;

    if (!(events[0] = CreateEventW(NULL, TRUE, FALSE, NULL)))
        goto error;
    if (!(events[1] = CreateThread(NULL, 0, deviceloop_thread, events[0], 0, NULL)))
    {
        CloseHandle(events[0]);
        goto error;
    }

    result = WaitForMultipleObjects(2, events, FALSE, INFINITE);
    CloseHandle(events[0]);
    CloseHandle(events[1]);
    if (result == WAIT_OBJECT_0)
    {
        TRACE("Initialization successful\n");
        return STATUS_SUCCESS;
    }

error:
    ERR("Failed to initialize udev device thread\n");
    udev_unref(udev_context);
    udev_context = NULL;
    udev_driver_obj = NULL;
    return STATUS_UNSUCCESSFUL;
}

#else

NTSTATUS WINAPI udev_driver_init(DRIVER_OBJECT *driver, UNICODE_STRING *registry_path)
{
    WARN("Wine was compiled without UDEV support\n");
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* HAVE_UDEV */
