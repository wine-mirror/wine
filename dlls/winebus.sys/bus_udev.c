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
#ifdef HAVE_LINUX_HIDRAW_H
# include <linux/hidraw.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_LINUX_INPUT_H
# include <linux/input.h>
# undef SW_MAX
# if defined(EVIOCGBIT) && defined(EV_ABS) && defined(BTN_PINKIE)
#  define HAS_PROPER_INPUT_HEADER
# endif
#endif

#define NONAMELESSUNION

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "ddk/hidtypes.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "bus.h"

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);

#ifdef HAVE_UDEV

WINE_DECLARE_DEBUG_CHANNEL(hid_report);

static struct udev *udev_context = NULL;
static DRIVER_OBJECT *udev_driver_obj = NULL;
static DWORD disable_hidraw = 0;
static DWORD disable_input = 0;

static const WCHAR hidraw_busidW[] = {'H','I','D','R','A','W',0};
static const WCHAR lnxev_busidW[] = {'L','N','X','E','V',0};

#include "initguid.h"
DEFINE_GUID(GUID_DEVCLASS_HIDRAW, 0x3def44ad,0x242e,0x46e5,0x82,0x6d,0x70,0x72,0x13,0xf3,0xaa,0x81);
DEFINE_GUID(GUID_DEVCLASS_LINUXEVENT, 0x1b932c0d,0xfea7,0x42cd,0x8e,0xaa,0x0e,0x48,0x79,0xb6,0x9e,0xaa);

struct platform_private
{
    struct udev_device *udev_device;
    int device_fd;

    HANDLE report_thread;
    int control_pipe[2];
};

static inline struct platform_private *impl_from_DEVICE_OBJECT(DEVICE_OBJECT *device)
{
    return (struct platform_private *)get_platform_private(device);
}

static inline WCHAR *strdupAtoW(const char *src)
{
    WCHAR *dst;
    DWORD len;
    if (!src) return NULL;
    len = MultiByteToWideChar(CP_UNIXCP, 0, src, -1, NULL, 0);
    if ((dst = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR))))
        MultiByteToWideChar(CP_UNIXCP, 0, src, -1, dst, len);
    return dst;
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
    if (!attr)
    {
        WARN("Could not get %s from device\n", sysattr);
        return NULL;
    }
    return strdupAtoW(attr);
}

static int compare_platform_device(DEVICE_OBJECT *device, void *platform_dev)
{
    struct udev_device *dev1 = impl_from_DEVICE_OBJECT(device)->udev_device;
    struct udev_device *dev2 = platform_dev;
    return strcmp(udev_device_get_syspath(dev1), udev_device_get_syspath(dev2));
}

static NTSTATUS hidraw_get_reportdescriptor(DEVICE_OBJECT *device, BYTE *buffer, DWORD length, DWORD *out_length)
{
#ifdef HAVE_LINUX_HIDRAW_H
    struct hidraw_report_descriptor descriptor;
    struct platform_private *private = impl_from_DEVICE_OBJECT(device);

    if (ioctl(private->device_fd, HIDIOCGRDESCSIZE, &descriptor.size) == -1)
    {
        WARN("ioctl(HIDIOCGRDESCSIZE) failed: %d %s\n", errno, strerror(errno));
        return STATUS_UNSUCCESSFUL;
    }

    *out_length = descriptor.size;

    if (length < descriptor.size)
        return STATUS_BUFFER_TOO_SMALL;
    if (!descriptor.size)
        return STATUS_SUCCESS;

    if (ioctl(private->device_fd, HIDIOCGRDESC, &descriptor) == -1)
    {
        WARN("ioctl(HIDIOCGRDESC) failed: %d %s\n", errno, strerror(errno));
        return STATUS_UNSUCCESSFUL;
    }

    memcpy(buffer, descriptor.value, descriptor.size);
    return STATUS_SUCCESS;
#else
    return STATUS_NOT_IMPLEMENTED;
#endif
}

static NTSTATUS hidraw_get_string(DEVICE_OBJECT *device, DWORD index, WCHAR *buffer, DWORD length)
{
    struct udev_device *usbdev;
    struct platform_private *private = impl_from_DEVICE_OBJECT(device);
    WCHAR *str = NULL;

    usbdev = udev_device_get_parent_with_subsystem_devtype(private->udev_device, "usb", "usb_device");
    if (usbdev)
    {
        switch (index)
        {
            case HID_STRING_ID_IPRODUCT:
                str = get_sysattr_string(usbdev, "product");
                break;
            case HID_STRING_ID_IMANUFACTURER:
                str = get_sysattr_string(usbdev, "manufacturer");
                break;
            case HID_STRING_ID_ISERIALNUMBER:
                str = get_sysattr_string(usbdev, "serial");
                break;
            default:
                ERR("Unhandled string index %08x\n", index);
                return STATUS_NOT_IMPLEMENTED;
        }
    }
    else
    {
#ifdef HAVE_LINUX_HIDRAW_H
        switch (index)
        {
            case HID_STRING_ID_IPRODUCT:
            {
                char buf[MAX_PATH];
                if (ioctl(private->device_fd, HIDIOCGRAWNAME(MAX_PATH), buf) == -1)
                    WARN("ioctl(HIDIOCGRAWNAME) failed: %d %s\n", errno, strerror(errno));
                else
                    str = strdupAtoW(buf);
                break;
            }
            case HID_STRING_ID_IMANUFACTURER:
                break;
            case HID_STRING_ID_ISERIALNUMBER:
                break;
            default:
                ERR("Unhandled string index %08x\n", index);
                return STATUS_NOT_IMPLEMENTED;
        }
#else
        return STATUS_NOT_IMPLEMENTED;
#endif
    }

    if (!str)
    {
        if (!length) return STATUS_BUFFER_TOO_SMALL;
        buffer[0] = 0;
        return STATUS_SUCCESS;
    }

    if (length <= strlenW(str))
    {
        HeapFree(GetProcessHeap(), 0, str);
        return STATUS_BUFFER_TOO_SMALL;
    }

    strcpyW(buffer, str);
    HeapFree(GetProcessHeap(), 0, str);
    return STATUS_SUCCESS;
}

static DWORD CALLBACK device_report_thread(void *args)
{
    DEVICE_OBJECT *device = (DEVICE_OBJECT*)args;
    struct platform_private *private = impl_from_DEVICE_OBJECT(device);
    struct pollfd plfds[2];

    plfds[0].fd = private->device_fd;
    plfds[0].events = POLLIN;
    plfds[0].revents = 0;
    plfds[1].fd = private->control_pipe[0];
    plfds[1].events = POLLIN;
    plfds[1].revents = 0;

    while (1)
    {
        int size;
        BYTE report_buffer[1024];

        if (poll(plfds, 2, -1) <= 0) continue;
        if (plfds[1].revents)
            break;
        size = read(plfds[0].fd, report_buffer, sizeof(report_buffer));
        if (size == -1)
            TRACE_(hid_report)("Read failed. Likely an unplugged device\n");
        else if (size == 0)
            TRACE_(hid_report)("Failed to read report\n");
        else
            process_hid_report(device, report_buffer, size);
    }
    return 0;
}

static NTSTATUS begin_report_processing(DEVICE_OBJECT *device)
{
    struct platform_private *private = impl_from_DEVICE_OBJECT(device);

    if (private->report_thread)
        return STATUS_SUCCESS;

    if (pipe(private->control_pipe) != 0)
    {
        ERR("Control pipe creation failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    private->report_thread = CreateThread(NULL, 0, device_report_thread, device, 0, NULL);
    if (!private->report_thread)
    {
        ERR("Unable to create device report thread\n");
        close(private->control_pipe[0]);
        close(private->control_pipe[1]);
        return STATUS_UNSUCCESSFUL;
    }
    else
        return STATUS_SUCCESS;
}

static NTSTATUS hidraw_set_output_report(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *written)
{
    struct platform_private* ext = impl_from_DEVICE_OBJECT(device);
    ssize_t rc;

    if (id != 0)
        rc = write(ext->device_fd, report, length);
    else
    {
        BYTE report_buffer[1024];

        if (length + 1 > sizeof(report_buffer))
        {
            ERR("Output report buffer too small\n");
            return STATUS_UNSUCCESSFUL;
        }

        report_buffer[0] = 0;
        memcpy(&report_buffer[1], report, length);
        rc = write(ext->device_fd, report_buffer, length + 1);
    }
    if (rc > 0)
    {
        *written = rc;
        return STATUS_SUCCESS;
    }
    else
    {
        *written = 0;
        return STATUS_UNSUCCESSFUL;
    }
}

static NTSTATUS hidraw_get_feature_report(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *read)
{
#if defined(HAVE_LINUX_HIDRAW_H) && defined(HIDIOCGFEATURE)
    int rc;
    struct platform_private* ext = impl_from_DEVICE_OBJECT(device);
    report[0] = id;
    length = min(length, 0x1fff);
    rc = ioctl(ext->device_fd, HIDIOCGFEATURE(length), report);
    if (rc >= 0)
    {
        *read = rc;
        return STATUS_SUCCESS;
    }
    else
    {
        *read = 0;
        return STATUS_UNSUCCESSFUL;
    }
#else
    *read = 0;
    return STATUS_NOT_IMPLEMENTED;
#endif
}

static NTSTATUS hidraw_set_feature_report(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *written)
{
#if defined(HAVE_LINUX_HIDRAW_H) && defined(HIDIOCSFEATURE)
    int rc;
    struct platform_private* ext = impl_from_DEVICE_OBJECT(device);
    BYTE *feature_buffer;
    BYTE buffer[1024];

    if (id == 0)
    {
        if (length + 1 > sizeof(buffer))
        {
            ERR("Output feature buffer too small\n");
            return STATUS_UNSUCCESSFUL;
        }
        buffer[0] = 0;
        memcpy(&buffer[1], report, length);
        feature_buffer = buffer;
        length = length + 1;
    }
    else
        feature_buffer = report;
    length = min(length, 0x1fff);
    rc = ioctl(ext->device_fd, HIDIOCSFEATURE(length), feature_buffer);
    if (rc >= 0)
    {
        *written = rc;
        return STATUS_SUCCESS;
    }
    else
    {
        *written = 0;
        return STATUS_UNSUCCESSFUL;
    }
#else
    *written = 0;
    return STATUS_NOT_IMPLEMENTED;
#endif
}

static const platform_vtbl hidraw_vtbl =
{
    compare_platform_device,
    hidraw_get_reportdescriptor,
    hidraw_get_string,
    begin_report_processing,
    hidraw_set_output_report,
    hidraw_get_feature_report,
    hidraw_set_feature_report,
};

#ifdef HAS_PROPER_INPUT_HEADER

static NTSTATUS lnxev_get_reportdescriptor(DEVICE_OBJECT *device, BYTE *buffer, DWORD length, DWORD *out_length)
{
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS lnxev_get_string(DEVICE_OBJECT *device, DWORD index, WCHAR *buffer, DWORD length)
{
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS lnxev_begin_report_processing(DEVICE_OBJECT *device)
{
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS lnxev_set_output_report(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *written)
{
    *written = 0;
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS lnxev_get_feature_report(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *read)
{
    *read = 0;
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS lnxev_set_feature_report(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *written)
{
    *written = 0;
    return STATUS_NOT_IMPLEMENTED;
}

static const platform_vtbl lnxev_vtbl = {
    compare_platform_device,
    lnxev_get_reportdescriptor,
    lnxev_get_string,
    lnxev_begin_report_processing,
    lnxev_set_output_report,
    lnxev_get_feature_report,
    lnxev_set_feature_report,
};
#endif

static int check_same_device(DEVICE_OBJECT *device, void* context)
{
    return !compare_platform_device(device, context);
}

static void try_add_device(struct udev_device *dev)
{
    DWORD vid = 0, pid = 0, version = 0;
    struct udev_device *usbdev = NULL;
    DEVICE_OBJECT *device = NULL;
    const char *subsystem;
    const char *devnode;
    WCHAR *serial = NULL;
    const char* gamepad = NULL;
    int fd;

    if (!(devnode = udev_device_get_devnode(dev)))
        return;

    if ((fd = open(devnode, O_RDWR)) == -1)
    {
        WARN("Unable to open udev device %s: %s\n", debugstr_a(devnode), strerror(errno));
        return;
    }

    subsystem = udev_device_get_subsystem(dev);
    usbdev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
    if (usbdev)
    {
#ifdef HAS_PROPER_INPUT_HEADER
        const platform_vtbl *other_vtbl = NULL;
        DEVICE_OBJECT *dup = NULL;
        if (strcmp(subsystem, "hidraw") == 0)
            other_vtbl = &lnxev_vtbl;
        else if (strcmp(subsystem, "input") == 0)
            other_vtbl = &hidraw_vtbl;

        if (other_vtbl)
            dup = bus_enumerate_hid_devices(other_vtbl, check_same_device, dev);
        if (dup)
        {
            TRACE("Duplicate cross bus device (%p) found, not adding the new one\n", dup);
            close(fd);
            return;
        }
#endif
        vid     = get_sysattr_dword(usbdev, "idVendor", 16);
        pid     = get_sysattr_dword(usbdev, "idProduct", 16);
        version = get_sysattr_dword(usbdev, "version", 10);
        serial  = get_sysattr_string(usbdev, "serial");
    }
#ifdef HAS_PROPER_INPUT_HEADER
    else
    {
        struct input_id device_id = {0};
        char device_uid[255];

        if (ioctl(fd, EVIOCGID, &device_id) < 0)
            WARN("ioctl(EVIOCGID) failed: %d %s\n", errno, strerror(errno));
        device_uid[0] = 0;
        if (ioctl(fd, EVIOCGUNIQ(254), device_uid) >= 0 && device_uid[0])
            serial = strdupAtoW(device_uid);

        gamepad = udev_device_get_property_value(dev, "ID_INPUT_JOYSTICK");
        vid = device_id.vendor;
        pid = device_id.product;
        version = device_id.version;
    }
#else
    else
        WARN("Could not get device to query VID, PID, Version and Serial\n");
#endif

    TRACE("Found udev device %s (vid %04x, pid %04x, version %u, serial %s)\n",
          debugstr_a(devnode), vid, pid, version, debugstr_w(serial));

    if (strcmp(subsystem, "hidraw") == 0)
    {
        device = bus_create_hid_device(udev_driver_obj, hidraw_busidW, vid, pid, version, 0, serial, FALSE,
                                       &GUID_DEVCLASS_HIDRAW, &hidraw_vtbl, sizeof(struct platform_private));
    }
#ifdef HAS_PROPER_INPUT_HEADER
    else if (strcmp(subsystem, "input") == 0)
    {
        device = bus_create_hid_device(udev_driver_obj, lnxev_busidW, vid, pid, version, 0, serial, (gamepad != NULL),
                                       &GUID_DEVCLASS_LINUXEVENT, &lnxev_vtbl, sizeof(struct platform_private));
    }
#endif

    if (device)
    {
        struct platform_private *private = impl_from_DEVICE_OBJECT(device);
        private->udev_device = udev_device_ref(dev);
        private->device_fd = fd;
        IoInvalidateDeviceRelations(device, BusRelations);
    }
    else
    {
        WARN("Ignoring device %s with subsystem %s\n", debugstr_a(devnode), subsystem);
        close(fd);
    }

    HeapFree(GetProcessHeap(), 0, serial);
}

static void try_remove_device(struct udev_device *dev)
{
    DEVICE_OBJECT *device = NULL;
    struct platform_private* private;

    device = bus_find_hid_device(&hidraw_vtbl, dev);
#ifdef HAS_PROPER_INPUT_HEADER
    if (device == NULL)
        device = bus_find_hid_device(&lnxev_vtbl, dev);
#endif
    if (!device) return;

    IoInvalidateDeviceRelations(device, RemovalRelations);

    private = impl_from_DEVICE_OBJECT(device);

    if (private->report_thread)
    {
        write(private->control_pipe[1], "q", 1);
        WaitForSingleObject(private->report_thread, INFINITE);
        close(private->control_pipe[0]);
        close(private->control_pipe[1]);
        CloseHandle(private->report_thread);
    }

    dev = private->udev_device;
    close(private->device_fd);
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

    if (!disable_hidraw)
        if (udev_enumerate_add_match_subsystem(enumerate, "hidraw") < 0)
            WARN("Failed to add subsystem 'hidraw' to enumeration\n");
#ifdef HAS_PROPER_INPUT_HEADER
    if (!disable_input)
    {
        if (udev_enumerate_add_match_subsystem(enumerate, "input") < 0)
            WARN("Failed to add subsystem 'input' to enumeration\n");
    }
#endif

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
    int systems = 0;

    monitor = udev_monitor_new_from_netlink(udev_context, "udev");
    if (!monitor)
    {
        WARN("Unable to get udev monitor object\n");
        return NULL;
    }

    if (!disable_hidraw)
    {
        if (udev_monitor_filter_add_match_subsystem_devtype(monitor, "hidraw", NULL) < 0)
            WARN("Failed to add 'hidraw' subsystem to monitor\n");
        else
            systems++;
    }
#ifdef HAS_PROPER_INPUT_HEADER
    if (!disable_input)
    {
        if (udev_monitor_filter_add_match_subsystem_devtype(monitor, "input", NULL) < 0)
            WARN("Failed to add 'input' subsystem to monitor\n");
        else
            systems++;
    }
#endif
    if (systems == 0)
    {
        WARN("No subsystems added to monitor\n");
        goto error;
    }

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
    static const WCHAR hidraw_disabledW[] = {'D','i','s','a','b','l','e','H','i','d','r','a','w',0};
    static const UNICODE_STRING hidraw_disabled = {sizeof(hidraw_disabledW) - sizeof(WCHAR), sizeof(hidraw_disabledW), (WCHAR*)hidraw_disabledW};
    static const WCHAR input_disabledW[] = {'D','i','s','a','b','l','e','I','n','p','u','t',0};
    static const UNICODE_STRING input_disabled = {sizeof(input_disabledW) - sizeof(WCHAR), sizeof(input_disabledW), (WCHAR*)input_disabledW};

    TRACE("(%p, %s)\n", driver, debugstr_w(registry_path->Buffer));

    if (!(udev_context = udev_new()))
    {
        ERR("Can't create udev object\n");
        return STATUS_UNSUCCESSFUL;
    }

    udev_driver_obj = driver;
    driver->MajorFunction[IRP_MJ_PNP] = common_pnp_dispatch;
    driver->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = hid_internal_dispatch;

    disable_hidraw = check_bus_option(registry_path, &hidraw_disabled, 0);
    if (disable_hidraw)
        TRACE("UDEV hidraw devices disabled in registry\n");

#ifdef HAS_PROPER_INPUT_HEADER
    disable_input = check_bus_option(registry_path, &input_disabled, 0);
    if (disable_input)
        TRACE("UDEV input devices disabled in registry\n");
#endif

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
